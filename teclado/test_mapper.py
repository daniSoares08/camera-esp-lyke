"""Behavior checks with synthetic contacts; never presented as physical mapping."""
import itertools
import tempfile
import unittest
from pathlib import Path

from server import FIRMWARE, KEYS, PINS, Mapper, header_text


class MapperTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.mapper = Mapper('TEST', Path(self.tmp.name))
        self.mapper.ingest(dict(type='hello', firmware=FIRMWARE, pins=PINS, enabled=True))
        self.mapper.action('start')

    def down(self, a, b):
        self.mapper.ingest(dict(type='down', a=a, b=b))
        self.mapper.ingest(dict(type='up', a=a, b=b))

    def test_complete_49_resume_and_export(self):
        contacts = list(itertools.combinations(PINS, 2))[:48] + [(48, -1)]
        for a, b in contacts:
            self.down(a, b)
        self.assertEqual(len(KEYS), 49)
        self.assertEqual(self.mapper.state()['mapped'], 49)
        self.assertFalse(self.mapper.armed)
        self.assertTrue(self.mapper.document()['complete'])
        restored = Mapper('TEST', Path(self.tmp.name))
        self.assertEqual(restored.mapping, self.mapper.mapping)
        self.assertEqual(restored.target, 49)
        self.assertIn('{"ADD", 48, -1}', header_text(restored))

    def test_duplicates_ambiguity_pause_and_correction(self):
        self.down(20, 19)
        self.assertEqual(self.mapper.target, 1)
        self.down(19, 20)
        self.mapper.ingest(dict(type='ambiguous'))
        self.assertEqual(self.mapper.target, 1)
        self.mapper.action('pause')
        self.down(47, 48)
        self.assertEqual(self.mapper.target, 1)
        self.mapper.action('start')
        self.down(47, 48)
        self.mapper.action('undo')
        self.assertEqual(self.mapper.target, 1)
        self.assertNotIn('B', self.mapper.mapping)
        self.mapper.action('start')
        self.down(46, 48)
        self.assertEqual(self.mapper.mapping['B'], dict(a=46, b=48))

    def test_skipped_is_pending_and_remappable(self):
        self.mapper.action('skip')
        self.assertIsNone(self.mapper.mapping['A'])
        self.assertEqual(self.mapper.target, 1)
        self.mapper.action('select', 0)
        self.assertFalse(self.mapper.armed)
        self.mapper.action('start')
        self.down(45, 46)
        self.assertEqual(self.mapper.target, 1)
        self.assertFalse(self.mapper.document()['complete'])

    def test_disconnect_reset_requires_resume(self):
        self.mapper.ingest(dict(type='hello', firmware=FIRMWARE, pins=PINS, enabled=False))
        self.assertFalse(self.mapper.armed)
        self.mapper.connected = False
        self.down(19, 20)
        self.assertEqual(self.mapper.mapping, {})
        with self.assertRaises(ValueError):
            self.mapper.action('start')

    def test_start_ignores_queued_stopped_heartbeat(self):
        self.mapper.armed = False
        self.mapper.ingest(dict(type='hello', firmware=FIRMWARE, pins=PINS, enabled=False, uptime_ms=1000))
        self.mapper.action('start')
        self.mapper.ingest(dict(type='hello', firmware=FIRMWARE, pins=PINS, enabled=False, uptime_ms=2000))
        self.assertTrue(self.mapper.armed)
        self.mapper.ingest(dict(type='hello', firmware=FIRMWARE, pins=PINS, enabled=True, uptime_ms=2001))
        self.assertTrue(self.mapper.armed)


if __name__ == '__main__':
    unittest.main()
