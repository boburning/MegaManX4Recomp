import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import tomllib
import unittest
import zipfile

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('translation_mod',ROOT/'tools/build_translation_mod.py')
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class TranslationModTests(unittest.TestCase):
    def setUp(self):
        self.source=json.loads((module.PACKAGE/'catalog/es.json').read_text(encoding='utf-8'))

    def test_edited_unicode_catalog_gets_new_verified_identity(self):
        self.source['entries'][0]['translation']='¡Sí, señor!'
        with tempfile.TemporaryDirectory() as directory:
            catalog=Path(directory)/'edited.json';output=Path(directory)/'spanish.psxmod'
            catalog.write_text(json.dumps(self.source,ensure_ascii=False),encoding='utf-8')
            module.build(catalog,output)
            with zipfile.ZipFile(output) as archive:
                raw=archive.read('catalog/es.json')
                manifest=tomllib.loads(archive.read('manifest.toml').decode())
                resource=next(r for r in manifest['resource'] if r['id']=='catalog')
                self.assertEqual(raw,catalog.read_bytes())
                self.assertEqual(resource['sha256'],hashlib.sha256(raw).hexdigest())
                self.assertEqual(resource['size'],len(raw))
                self.assertEqual(len([o for o in manifest['option'] if o['feature']=='japanese-media']),2)
                self.assertNotIn('requirement',manifest)
            before=output.read_bytes()
            with self.assertRaises(ValueError):module.build(catalog,output)
            self.assertEqual(before,output.read_bytes())

    def test_ambiguous_id_rejected(self):
        self.source['entries'].append(self.source['entries'][0])
        with self.assertRaisesRegex(ValueError,'duplicate'):module.validate(self.source)

    def test_portuguese_catalog_preserves_both_language_resources(self):
        self.source=json.loads((module.PACKAGE/'catalog/pt-br.json').read_text(encoding='utf-8'))
        self.assertEqual(module.validate(self.source),102)
        self.source['entries'][0]['translation']='À ação! Você está pronto?'
        with tempfile.TemporaryDirectory() as directory:
            catalog=Path(directory)/'pt-br.json';output=Path(directory)/'portuguese.psxmod'
            catalog.write_text(json.dumps(self.source,ensure_ascii=False),encoding='utf-8')
            module.build(catalog,output)
            with zipfile.ZipFile(output) as archive:
                manifest=tomllib.loads(archive.read('manifest.toml').decode())
                language=next(o for o in manifest['option'] if o['feature']=='translation')
                self.assertEqual(language['default'],'pt-br')
                for resource in (r for r in manifest['resource'] if r['feature']=='translation'):
                    raw=archive.read(resource['default'])
                    self.assertEqual(hashlib.sha256(raw).hexdigest(),resource['sha256'])
                    self.assertEqual(len(raw),resource['size'])

    def test_community_language_names_its_selector_without_new_native_code(self):
        self.source['language']='en'
        self.source['language_name']='English community draft'
        with tempfile.TemporaryDirectory() as directory:
            catalog=Path(directory)/'english.json';output=Path(directory)/'english.psxmod'
            catalog.write_text(json.dumps(self.source,ensure_ascii=False),encoding='utf-8')
            module.build(catalog,output)
            with zipfile.ZipFile(output) as archive:
                manifest=tomllib.loads(archive.read('manifest.toml').decode())
                language=next(o for o in manifest['option'] if o['feature']=='translation')
                self.assertEqual(language['default'],'en')
                self.assertEqual(language['choice'][0],dict(value='en',label='English community draft'))
                self.assertEqual(json.loads(archive.read('catalog/es.json'))['language'],'en')

    def test_unsupported_glyph_and_overlong_word_rejected(self):
        for invalid in ('Ω','a'*21):
            self.source['entries'][0]['translation']=invalid
            with self.assertRaises(ValueError):module.validate(self.source)

    def test_empty_translation_preserves_native_fallback(self):
        self.source['entries'][0]['translation']=''
        self.assertEqual(module.validate(self.source),101)

if __name__=='__main__':unittest.main()
