"""Extract movie-only native mods from the two supported owner-supplied archives.

XDelta is used only as an offline extraction oracle in the explicit scratch
directory. The resulting mod contains ten STR streams, attribution and their
verified identities. It never replaces the runtime disc or imports patch code.
Requires Python 3.11+, xdelta3, and 7-Zip for the English 7z archive.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tomllib
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
CREDITS = ROOT / 'docs/fmv-subtitles'
DISC_MD5 = '1c425b49bb25c45b3964b2d565dd0ec0'
MOVIES = ('OP_U', 'X1_U', 'X2_U', 'X3_U', 'X4_U', 'Z1_U', 'Z2_U', 'Z3_U', 'Z4_U', 'Z5_U')
SOURCES = {
    'en': dict(archive='24d2a8a7c142d724736c999ef74e98350821a17d62de76e6b94a27cc00e610b3',
               patch='b3708d66af783d918d370b60297c791c447590e3794c5ae226d554111ffaac80',
               patch_name='Mega Man X4 (USA) [Undub v1.13 + Retranslation].xdelta',
               readme='Readme.txt', name='English', author='acediez / Hondoori',
               url='https://www.romhacking.net/forum/index.php?topic=26255.0'),
    'es': dict(archive='e0ca3dc5686bc94309e523a6438ec1b2f6ae3b991f111c1bc98f5b4a4b8fd189',
               patch='c074583f97aa21cdf8c5b77d3256d34b921eb626e8d69196697503f33478f411',
               patch_name='MEGA MAN X4 RETRADUCIDO FINAL.xdelta',
               readme='LEEME.txt', name='Español', author='El Retro Dan',
               url='https://www.youtube.com/watch?v=dM_LnAhPoPI'),
}


def digest(path, algorithm='sha256'):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, algorithm).hexdigest()


def non_system_path(path):
    return Path(path).resolve()


class Disc:
    """Bounded, read-only ISO9660 access to one raw MODE2/2352 data track."""
    def __init__(self, path):
        self.file = Path(path).open('rb')
        self.size = Path(path).stat().st_size
        try:
            if self.size % 2352:
                raise ValueError('Expected a raw MODE2/2352 BIN')
            pvd = self.read(16, 2048)
            if pvd[:7] != b'\x01CD001\x01':
                raise ValueError('Missing ISO9660 volume descriptor')
            self.files = {}
            self.directory(*self.extent(pvd[156:190]), '', set())
        except BaseException:
            self.file.close()
            raise

    def close(self):
        self.file.close()

    def extent(self, record):
        if len(record) < 34:
            raise ValueError('Truncated directory record')
        lba, size = struct.unpack_from('<I', record, 2)[0], struct.unpack_from('<I', record, 10)[0]
        if (lba != struct.unpack_from('>I', record, 6)[0] or
                size != struct.unpack_from('>I', record, 14)[0] or
                (lba + (size + 2047) // 2048) * 2352 > self.size):
            raise ValueError('Invalid ISO9660 extent')
        return lba, size

    def sectors(self, lba, count):
        if lba < 0 or count < 0 or (lba + count) * 2352 > self.size:
            raise ValueError('Movie outside the source image')
        self.file.seek(lba * 2352)
        for _ in range(count):
            raw = self.file.read(2352)
            if len(raw) != 2352 or raw[15] != 2 or raw[16:20] != raw[20:24]:
                raise ValueError('Invalid raw Mode-2 sector')
            yield raw

    def read(self, lba, size):
        if size > 8 * 1024 * 1024:
            raise ValueError('Directory or ordinary file exceeds 8 MiB')
        return b''.join(raw[24:2072] for raw in self.sectors(lba, (size + 2047) // 2048))[:size]

    def directory(self, lba, size, prefix, visited):
        if lba in visited or len(visited) >= 64:
            raise ValueError('Cyclic or excessive ISO9660 directories')
        visited.add(lba)
        data = self.read(lba, size)
        pos = 0
        while pos < len(data):
            length = data[pos]
            if not length:
                pos = (pos // 2048 + 1) * 2048
                continue
            record = data[pos:pos + length]
            if length < 34 or len(record) != length or 33 + record[32] > length:
                raise ValueError('Invalid ISO9660 directory record')
            pos += length
            name = record[33:33 + record[32]]
            if name in (b'\0', b'\1'):
                continue
            name = name.decode('ascii').split(';')[0]
            if not name or '/' in name or '\\' in name or name in ('.', '..'):
                raise ValueError('Invalid ISO9660 file name')
            full = prefix + name
            extent = self.extent(record)
            if record[25] & 2:
                self.directory(*extent, full + '/', visited)
            elif full in self.files:
                raise ValueError('Duplicate ISO9660 file name')
            else:
                self.files[full] = extent


def extract_movies(image, output):
    disc = Disc(image)
    try:
        entries, provenance = [], []
        with output.open('xb') as pack:
            pack.write(b'\0' * 2336)
            for index, name in enumerate(MOVIES, 1):
                path = 'STR/' + name + '.STR'
                lba, length = disc.files[path]
                if not length or length % 2048 or length // 2048 > 40000:
                    raise ValueError('Unexpected movie length: ' + path)
                count = length // 2048
                offset, sha = pack.tell(), hashlib.sha256()
                video, audio, frames = 0, 0, set()
                for raw in disc.sectors(lba, count):
                    sector = raw[16:]
                    # Original PS1 STR v2 frame header (after the XA subheader).
                    if raw[24:28] == b'\x60\x01\x01\x80':
                        video += 1
                        frames.add(struct.unpack_from('<I', raw, 32)[0])
                    if raw[18] & 4:
                        audio += 1
                    pack.write(sector)
                    sha.update(sector)
                if not video or not audio:
                    raise ValueError('Movie lacks native MDEC video or XA audio: ' + path)
                entries.append((index, count, offset))
                provenance.append(dict(path=path, oracle_lba=lba, sectors=count,
                                       sha256=sha.hexdigest(), video_sectors=video,
                                       audio_sectors=audio, frames=len(frames)))
            pack.seek(0)
            pack.write(struct.pack('<8sII', b'X4FMV001', 10, 2336))
            for entry in entries:
                pack.write(struct.pack('<IIQ', *entry))
        return provenance
    finally:
        disc.close()


def metadata_directory():
    for layout in ('mods/preloaded/packages', 'mods/bundled'):
        directory = ROOT / layout / 'mmx4.japanese-fmvs/1.0.0'
        if (directory / 'manifest.toml').is_file() and (directory / 'provenance').is_dir():
            return directory
    raise ValueError('Japanese FMVs metadata is missing; run from the release folder or a source checkout')


def manifest(language, pack):
    # Both imports use the same three-choice mod. Only the selected media is
    # required; audiovisual data stays in its external verified sidecar.
    template = metadata_directory() / 'manifest.toml'
    text = template.read_text(encoding='utf-8')
    resource_id = {'en': 'english-movies', 'es': 'spanish-movies'}[language]
    resource = next(r for r in tomllib.loads(text)['resource'] if r['id'] == resource_id)
    if pack.stat().st_size != resource['size'] or digest(pack) != resource['sha256']:
        raise ValueError('Extracted movie pack does not match the attributed release')
    return text


def build(archive, disc, output, scratch, xdelta, seven_zip=None):
    archive, disc = Path(archive).resolve(strict=True), Path(disc).resolve(strict=True)
    output, scratch = non_system_path(output), non_system_path(scratch)
    media_output = output.with_suffix('.x4fmv')
    if output.suffix.lower() != '.psxmod':
        raise ValueError('--output must name a new .psxmod file')
    if output.exists() or media_output.exists():
        raise ValueError('Refusing to overwrite ' + str(output))
    identity = digest(archive)
    language = next((key for key, item in SOURCES.items() if item['archive'] == identity), None)
    if language is None:
        raise ValueError('Unrecognized archive; supported releases and SHA-256 hashes are in docs/FMV_SUBTITLES.md')
    metadata = metadata_directory()
    if disc.stat().st_size != 592217136 or digest(disc, 'md5') != DISC_MD5:
        raise ValueError('Expected the unmodified USA SLUS-00561 raw BIN documented in DISC.md')
    source = SOURCES[language]
    xdelta = Path(xdelta).resolve(strict=True)
    scratch.mkdir(parents=True, exist_ok=True)
    if shutil.disk_usage(scratch).free < 2 * 1024**3:
        raise ValueError('Extraction requires 2 GiB free scratch space')
    output.parent.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(TEMP=str(scratch), TMP=str(scratch))
    with tempfile.TemporaryDirectory(prefix='x4-fmv-', dir=scratch) as temporary:
        task = Path(temporary)
        patch, readme, oracle, pack = [task / name for name in ('source.xdelta', 'readme.txt', 'oracle.bin', 'movies.x4fmv')]
        for name, target in ((source['patch_name'], patch), (source['readme'], readme)):
            if language == 'es':
                with zipfile.ZipFile(archive) as zipped, zipped.open(name) as member, target.open('xb') as dest:
                    shutil.copyfileobj(member, dest)
            else:
                if seven_zip is None:
                    raise ValueError('--seven-zip is required for the supplied English 7z archive')
                with target.open('xb') as dest:
                    subprocess.run([str(Path(seven_zip).resolve(strict=True)), 'e', '-so', str(archive), name],
                                   stdout=dest, check=True, env=env, timeout=300)
        if digest(patch) != source['patch']:
            raise ValueError('Patch checksum mismatch')
        subprocess.run([str(xdelta), '-d', '-s', str(disc), str(patch), str(oracle)],
                       check=True, env=env, timeout=300)
        if oracle.stat().st_size != 595862736:
            raise ValueError('Unexpected extraction oracle size')
        movies = extract_movies(oracle, pack)
        provenance = dict(schema=1, extractor='mmx4-fmv-import-v1', language=language,
                          scope='Ten movie streams only; no executable, script, font, menu, voice bank or disc image.',
                          archive_name=archive.name, archive_sha256=identity,
                          patch_name=source['patch_name'], patch_sha256=source['patch'],
                          source_disc_md5=DISC_MD5, oracle_sha256=digest(oracle),
                          pack_sha256=digest(pack), movies=movies)
        # Publish only a complete archive. Keep temporary output on the same
        # volume as its destination; neither source nor an existing mod is touched.
        with tempfile.TemporaryDirectory(prefix='x4-mod-', dir=output.parent) as staging:
            pending = Path(staging) / output.name
            with zipfile.ZipFile(pending, 'x', compression=zipfile.ZIP_STORED) as mod:
                mod.writestr('manifest.toml', manifest(language, pack))
                mod.writestr('provenance.json', json.dumps(provenance, indent=2, ensure_ascii=False) + '\n')
                mod.write(CREDITS / 'CREDITS.md', 'CREDITS.md')
                for path in sorted(CREDITS.iterdir()):
                    if path.is_file() and path.name != 'CREDITS.md':
                        mod.write(path, 'upstream/' + path.name)
                for path in sorted((metadata / 'provenance').iterdir()):
                    mod.write(path, 'provenance/' + path.name)
            # Exclusive creation also protects against a destination appearing
            # during the long decode; failed copies remove only our new file.
            created = []
            try:
                # Large media is an external verified resource; the framework's
                # 256 MiB mod-archive expansion limit stays intact.
                for source_file, target in ((pack, media_output), (pending, output)):
                    with target.open('xb') as dest:
                        created.append(target)
                        with source_file.open('rb') as src:
                            shutil.copyfileobj(src, dest)
            except BaseException:
                for target in created:
                    target.unlink()
                raise
    return language, movies


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('archive', 'disc', 'output', 'scratch', 'xdelta'):
        parser.add_argument('--' + key, required=True, type=Path)
    parser.add_argument('--seven-zip', type=Path)
    args = parser.parse_args()
    try:
        language, movies = build(**vars(args))
    except (ValueError, OSError, KeyError, subprocess.SubprocessError, zipfile.BadZipFile) as exc:
        parser.exit(1, str(exc) + '\n')
    print(f'Created {language} native movie mod with {len(movies)} credited STR streams: {args.output}')
    print(f'Select Japanese FMVs, choose its subtitle language, then select: {args.output.with_suffix(".x4fmv")}')


if __name__ == '__main__':
    main()
