"""Verify the initial USB image and readback guards without opening a device."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from bootstrap_usb import FLASH, KEEP, ROOT, internal_flash_matches, verify_readback, verify_release


class BootstrapTest(unittest.TestCase):
    def test_actual_dfu_descriptors(self):
        for value in ['64*02Kg', '064*002Kg']:
            self.assertTrue(internal_flash_matches('name="@Internal Flash   /0x08000000/'+value+'"'))
        for value in ['@Internal Flash /0x08000000/128*02Kg',
                      '@Internal Flash /0x08002000/64*02Kg',
                      '@Option Bytes /0x1FFF7800/01*40Be']:
            self.assertFalse(internal_flash_matches('name="'+value+'"'))

    def test_release_and_preserved_tail(self):
        _, image = verify_release(ROOT / '.build/lyke_stm32/lyke-r11')
        before = bytes((i * 19) % 256 for i in range(FLASH))
        after = image + before[KEEP:]
        verify_readback(before, after, image)
        damaged = bytearray(after)
        damaged[-1] ^= 1
        with self.assertRaisesRegex(ValueError, 'Mapa/preferencias'):
            verify_readback(before, damaged, image)
        damaged = bytearray(after)
        damaged[100] ^= 1
        with self.assertRaisesRegex(ValueError, 'Programa/carregador'):
            verify_readback(before, damaged, image)
        with self.assertRaisesRegex(ValueError, 'incompleta'):
            verify_readback(before[:-1], after, image)

    def test_legacy_loader_manifest_rejected(self):
        source = ROOT / '.build/lyke_stm32/lyke-r11'
        manifest = json.loads((source / 'manifest.json').read_text())
        manifest['loader_protocol'] = 'LYKE BOOT'
        with tempfile.TemporaryDirectory() as folder:
            release = Path(folder)
            (release / 'manifest.json').write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, 'outro alvo'):
                verify_release(release)


if __name__ == '__main__':
    unittest.main()
