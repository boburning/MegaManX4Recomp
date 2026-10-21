#!/usr/bin/env python3
"""Package an edited UTF-8 translation JSON for X4's ordinary Mods installer.

No disc, glyph codes, binary patching or C++ compilation is needed. The source
digests stay attached to the readable original messages; the installer gets a
fresh immutable catalog identity. Japanese media remains separately selectable.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / 'mods/preloaded/packages/mmx4.localization/1.0.0'
SUPPORTED = set('0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz/↓↑←→+-,.!?"\' áéíóúüñÁÉÍÓÚÜÑ¿¡àâêôãõçÀÂÊÔÃÕÇ\n\f')


def validate(data):
    if not isinstance(data, dict) or data.get('schema') != 1:
        raise ValueError('Expected translation schema 1')
    if not isinstance(data.get('language'), str) or not re.fullmatch(r'[a-z][a-z0-9-]{0,15}', data['language']):
        raise ValueError('A stable language code is required, for example es or pt-br')
    if not isinstance(data.get('entries'), list):
        raise ValueError('entries must be an array')
    ids = set()
    for entry in data['entries']:
        if not isinstance(entry, dict):
            raise ValueError('Each translation entry must be an object')
        key = entry.get('id', '')
        if not isinstance(key, str) or not re.fullmatch(r'dialogue\.(x|zero)\.\d{3}', key) or key in ids:
            raise ValueError(f'Invalid or duplicate message ID: {key}')
        ids.add(key)
        if not isinstance(entry.get('original'), str) or not entry['original']:
            raise ValueError(f'{key}: readable original text is required')
        if not isinstance(entry.get('source_sha256'), str) or not re.fullmatch(r'[0-9a-f]{64}', entry['source_sha256']):
            raise ValueError(f'{key}: preserve its source SHA-256')
        target = entry.get('translation')
        if not isinstance(target, str):
            raise ValueError(f'{key}: translation must be plain text; use empty text for native fallback')
        unsupported = set(target) - SUPPORTED
        if unsupported:
            raise ValueError(f'{key}: unsupported native-font glyphs: {sorted(unsupported)}')
        if any(len(word) > 20 for word in target.split()):
            raise ValueError(f'{key}: a word exceeds the native 20-column textbox')
        if target and (target[0] in '\n\f' or target[-1] in '\n\f' or re.search(r'[\n\f]{2}', target)):
            raise ValueError(f'{key}: empty lines/pages are not supported')
    return sum(bool(e['translation']) for e in data['entries'])


def build(catalog, output):
    raw = catalog.read_bytes()
    if len(raw) > 8 * 1024 * 1024:
        raise ValueError('Catalog exceeds 8 MiB')
    data = json.loads(raw.decode('utf-8'))
    count = validate(data)
    manifest = (PACKAGE / 'manifest.toml').read_text(encoding='utf-8')
    # Only the catalog resource changes. The native donor stays independent.
    locale=data['language']
    slot='pt-br' if locale=='pt-br' else 'es'
    resource='catalog-pt-br' if locale=='pt-br' else 'catalog'
    before, separator, tail = manifest.partition(f'id = "{resource}"')
    if not separator:
        raise ValueError('Bundled language package is missing its catalog resource')
    tail = re.sub(r'(?m)^size = \d+$', f'size = {len(raw)}', tail, count=1)
    tail = re.sub(r'(?m)^sha256 = "[0-9a-f]{64}"$', f'sha256 = "{hashlib.sha256(raw).hexdigest()}"', tail, count=1)
    manifest = before + separator + tail
    # The installed package retains both selector choices. Update the selected
    # catalog and keep the other bundled resource with its verified identity.
    default=re.compile(r'(?ms)(\[\[option\]\]\nfeature = "translation"\nid = "language".*?^default = )"[^"]+"')
    manifest=default.sub(lambda m:m[1]+json.dumps(locale),manifest,count=1)
    if locale not in ('es','pt-br'):
        label=data.get('language_name',locale)
        if not isinstance(label,str) or not label.strip():raise ValueError('language_name must be readable text')
        manifest=manifest.replace('value = "es"\nlabel = "Español"',
            f'value = {json.dumps(locale)}\nlabel = {json.dumps(label,ensure_ascii=False)}',1)
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists():
        raise ValueError(f'Output already exists: {output}')
    with zipfile.ZipFile(output, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr('manifest.toml', manifest.encode('utf-8'))
        for language in ('es','pt-br'):
            archive.writestr(f'catalog/{language}.json',raw if language==slot else (PACKAGE/f'catalog/{language}.json').read_bytes())
    return count, len(data['entries'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--catalog', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path, help='New .psxmod/ZIP file')
    args = parser.parse_args()
    try:
        count, total = build(args.catalog, args.output)
    except (ValueError, OSError) as exc:
        parser.exit(1, f'{exc}\n')
    print(f'Packaged {count}/{total} translated messages: {args.output}')


if __name__ == '__main__':
    main()
