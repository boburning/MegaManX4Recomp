import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import tempfile
import tomllib
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('fmv_import', ROOT / 'tools/import_fmv_subtitles.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FixtureDisc:
    def __init__(self, path):
        self.files = {'STR/' + name + '.STR': (index * 2, 4096)
                      for index, name in enumerate(module.MOVIES)}

    def sectors(self, lba, count):
        video = bytearray(2352)
        video[:16] = b'raw-sector-head!'
        video[24:28] = b'\x60\x01\x01\x80'
        struct.pack_into('<I', video, 32, lba + 1)
        audio = bytearray(2352)
        audio[:16] = b'raw-sector-head!'
        audio[18] = audio[22] = 4
        audio[24:] = bytes([lba + 1]) * (2352 - 24)
        return iter((bytes(video), bytes(audio)))

    def close(self):
        pass


class ImportTests(unittest.TestCase):
    def test_movie_extraction_preserves_xa_headers_audio_and_subtitle_pixels(self):
        with tempfile.TemporaryDirectory() as temp, patch.object(module, 'Disc', FixtureDisc):
            pack = Path(temp) / 'test.x4fmv'
            inventory = module.extract_movies('unused', pack)
            raw = pack.read_bytes()
            self.assertEqual(raw[:16], struct.pack('<8sII', b'X4FMV001', 10, 2336))
            self.assertEqual(len(inventory), 10)
            for index in range(10):
                movie_id, count, offset = struct.unpack_from('<IIQ', raw, 16 + index * 16)
                self.assertEqual((movie_id, count, offset), (index + 1, 2, (index * 2 + 1) * 2336))
                expected = b''.join(s[16:] for s in FixtureDisc(None).sectors(index * 2, 2))
                self.assertEqual(raw[offset:offset + count * 2336], expected)
                self.assertEqual(inventory[index]['sha256'], hashlib.sha256(expected).hexdigest())
                self.assertEqual(inventory[index]['frames'], 1)
                self.assertEqual(inventory[index]['audio_sectors'], 1)
            self.assertEqual(len(raw), 21 * 2336)

    def test_unrecognized_inputs_do_not_invoke_a_decoder(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            archive = root / 'not-a-release.zip';archive.write_bytes(b'invalid')
            disc = root / 'disc.bin';disc.write_bytes(b'not-a-disc')
            with patch.object(module.subprocess, 'run') as run:
                with self.assertRaisesRegex(ValueError, 'Unrecognized archive'):
                    module.build(archive, disc, root / 'out.psxmod', root, root / 'no-decoder')
                run.assert_not_called()

    def test_existing_movie_resource_is_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            archive = root / 'source.zip';archive.write_bytes(b'source')
            disc = root / 'disc.bin';disc.write_bytes(b'disc')
            media = root / 'out.x4fmv';media.write_bytes(b'preserve')
            with self.assertRaisesRegex(ValueError, 'overwrite'):
                module.build(archive, disc, root / 'out.psxmod', root, root / 'no-decoder')
            self.assertEqual(media.read_bytes(), b'preserve')

    def test_one_manifest_selects_exactly_one_attributed_movie_resource(self):
        with tempfile.TemporaryDirectory() as temp:
            pack = Path(temp) / 'movie.x4fmv';pack.write_bytes(b'verified fixture')
            for language in ('en', 'es'):
                with self.assertRaisesRegex(ValueError, 'attributed release'):
                    module.manifest(language, pack)
        package = ROOT / 'mods/preloaded/packages/mmx4.japanese-fmvs/1.0.0'
        manifest = tomllib.loads((package / 'manifest.toml').read_text(encoding='utf-8'))
        self.assertEqual(len(manifest['feature']), 1)
        self.assertFalse(manifest['feature'][0]['default_enabled'])
        self.assertEqual(manifest['option'][0]['id'], 'subtitles')
        self.assertEqual([(c['value'], c['label']) for c in manifest['option'][0]['choice']],
                         [('none', 'No subtitles'), ('en', 'English'), ('es', 'Español')])
        self.assertEqual({r['when']['subtitles'] for r in manifest['resource']}, {'none','en','es'})
        self.assertTrue(all(r['required'] for r in manifest['resource']))
        self.assertNotIn('derived_disc', manifest)
        self.assertNotIn('patch', manifest)
        self.assertEqual(manifest['plugin'][0]['id'], manifest['id'])
        urls = {a['url'] for a in manifest['author_link']}
        self.assertTrue(all(s['url'] in urls for s in module.SOURCES.values()))

    def test_raw_disc_parser_rejects_cooked_or_invalid_images(self):
        with tempfile.TemporaryDirectory() as temp:
            invalid = Path(temp) / 'invalid.bin'
            for data in (b'cooked ISO', bytes(2352 * 17)):
                invalid.write_bytes(data)
                with self.assertRaises(ValueError):module.Disc(invalid)

    def test_bundled_metadata_and_attribution_match_import_provenance(self):
        for language in ('en', 'es'):
            package = ROOT / 'mods/preloaded/packages/mmx4.japanese-fmvs/1.0.0'
            manifest = tomllib.loads((package / 'manifest.toml').read_text(encoding='utf-8'))
            provenance = json.loads((package / f'provenance/{language}.json').read_text(encoding='utf-8'))
            resource = next(r for r in manifest['resource'] if r['when']['subtitles']==language)
            self.assertEqual(resource['sha256'], provenance['pack_sha256'])
            self.assertEqual(resource['size'], 2336 * (1 + sum(m['sectors'] for m in provenance['movies'])))
            self.assertEqual(provenance['archive_sha256'], module.SOURCES[language]['archive'])
            self.assertEqual(provenance['patch_sha256'], module.SOURCES[language]['patch'])
            self.assertEqual((package / 'CREDITS.md').read_bytes(), (module.CREDITS / 'CREDITS.md').read_bytes())
            upstream = 'acediez-Undub-v1.13-Readme.txt' if language == 'en' else 'El-Retro-Dan-Final-LEEME.txt'
            self.assertEqual((package / 'upstream' / upstream).read_bytes(), (module.CREDITS / upstream).read_bytes())

    @unittest.skipUnless(os.name == 'nt', 'Windows drive rule')
    def test_generated_data_on_system_drive_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'F: or G:'):
            module.non_system_path('C:/Temp/x4-fmv')


if __name__ == '__main__':
    unittest.main()
