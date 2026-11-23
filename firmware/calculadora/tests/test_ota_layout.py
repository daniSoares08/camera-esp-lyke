import csv
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class PartitionTests(unittest.TestCase):
    def test_slots_fit_without_moving_existing_user_data(self):
        for target, flash in [('s3_calculadora', 16 * 1024**2),
                              ('devkit_display', 4 * 1024**2)]:
            with self.subTest(target=target):
                lines = [s for s in (ROOT / target / 'partitions.csv').read_text().splitlines()
                         if s.strip() and not s.startswith('#')]
                rows = {r[0]: (int(r[3], 0), int(r[4], 0)) for r in csv.reader(lines)}
                self.assertEqual(rows['nvs'], (0x9000, 0x5000))
                self.assertEqual(rows['spiffs'], (0x310000, 0xe0000))
                self.assertEqual(rows['otadata'], (0xe000, 0x2000))
                self.assertEqual(rows['app0'][1], rows['app1'][1])
                for name in ('app0', 'app1'):
                    self.assertEqual(rows[name][0] % 0x10000, 0)
                ordered = sorted(rows.values())
                for (start, size), (next_start, _) in zip(ordered, ordered[1:]):
                    self.assertLessEqual(start + size, next_start)
                self.assertLessEqual(max(a+b for a, b in rows.values()), flash)


if __name__ == '__main__':
    unittest.main()
