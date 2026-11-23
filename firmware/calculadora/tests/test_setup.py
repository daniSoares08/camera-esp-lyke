import binascii
import importlib.util
import json
from pathlib import Path
import random
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('calculator_setup', ROOT / 'configurar_usb.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def packet(payload, seq=1):
    header = bytearray(b'\xc3\x3c\x01\x01') + seq.to_bytes(2, 'little') + b'\x00\x04'
    crc = binascii.crc_hqx(header[2:] + payload, 0xFFFF)
    return header + crc.to_bytes(2, 'little') + payload


class SerialTests(unittest.TestCase):
    def test_fragmented_frames_and_configuration_ack(self):
        payload = bytes((i * 73) % 256 for i in range(1024))
        raw = b'ESP-ROM startup\r\n' + packet(payload, 9) + b'\nCFG {"id":"test","ok":true}\n' + packet(payload, 10)
        decoder = module.Decoder()
        messages = []
        rng = random.Random(26)
        while raw:
            size = rng.randint(1, 137)
            messages.extend(decoder.feed(raw[:size]))
            raw = raw[size:]
        self.assertEqual([v for kind, v in messages if kind == 'frame'], [payload, payload])
        self.assertEqual([v for kind, v in messages if kind == 'ack'], [{'id': 'test', 'ok': True}])

    def test_bad_frame_does_not_overwrite_display(self):
        payload = bytes(1024)
        corrupt = packet(payload)
        corrupt[-1] ^= 1
        messages = module.Decoder().feed(corrupt + packet(payload, 2))
        self.assertEqual(messages, [('frame', payload)])

    def test_measured_map_is_preserved(self):
        source = json.loads((ROOT.parents[1] / 'outputs' / 'mapa_teclado' / 'mapa_final.json').read_text(encoding='utf-8'))
        header = (ROOT / 'common' / 'keyboard_map.h').read_text()
        count = 0
        for key in source['keys']:
            value = source['mapping'][key['id']]
            a, b = (value['a'], value['b']) if value else (-2, -2)
            self.assertIn('{"%s",%d,%d}' % (key['id'], a, b), header)
            count += value is not None
        self.assertEqual(count, 46)


if __name__ == '__main__':
    unittest.main()
