import unittest

from wled_protocol import WledFrameParser, parse_state, scene_from_preset


class WledProtocolTests(unittest.TestCase):
    def test_state_reply_updates_rgb_effects_and_nightlight(self):
        state = parse_state(
            '{"state":{"on":true,"bri":180,"ps":12,"nl":{"on":true,"dur":3,"mode":1,"tbri":0},'
            '"seg":[{"col":[[10,20,30,40]],"cct":127,"pal":4,"fx":7,"sx":100,"ix":90}]}}'
        )
        self.assertIsNotNone(state)
        self.assertEqual((state.red, state.green, state.blue), (10, 20, 30))
        self.assertEqual((state.white, state.cct), (40, 127))
        self.assertEqual((state.palette, state.effect, state.effect_speed, state.effect_intensity), (4, 7, 100, 90))
        self.assertTrue(state.nightlight)

    def test_malformed_json_is_rejected(self):
        self.assertIsNone(parse_state('{"state":{"bri":'))

    def test_invalid_state_field_is_rejected(self):
        self.assertIsNone(parse_state('{"state":{"bri":300}}'))
        self.assertIsNone(parse_state('{"state":{"nl":{"mode":4}}}'))
        self.assertIsNone(parse_state('{"state":{"seg":[{"col":[[1,2]]}]}}'))
        self.assertIsNone(parse_state('{"state":{"seg":[{"cct":256}]}}'))

    def test_preset_and_scene_mapping(self):
        state = parse_state('{"state":{"ps":64}}')
        self.assertEqual(state.preset, 64)
        self.assertEqual(scene_from_preset(state.preset), 63)
        self.assertIsNone(scene_from_preset(65))
        self.assertIsNone(scene_from_preset(-1))

    def test_missing_fields_do_not_invent_feedback(self):
        state = parse_state('{"state":{"on":false}}')
        self.assertFalse(state.on)
        self.assertIsNone(state.brightness)
        self.assertIsNone(state.red)

    def test_continuous_json_frames_are_kept_separate(self):
        parser = WledFrameParser()
        states = parser.feed('{"state":{"bri":10}}\r\n{"state":{"bri":20}}\r\n', 0)
        self.assertEqual([state.brightness for state in states], [10, 20])

    def test_oversized_frame_is_discarded_until_newline(self):
        parser = WledFrameParser(capacity=32)
        states = parser.feed('{"state":{"bri":' + ('1' * 64) + '}}\r\n{"state":{"bri":9}}\r\n', 0)
        self.assertEqual([state.brightness for state in states], [9])

    def test_truncated_frame_expires(self):
        parser = WledFrameParser()
        self.assertEqual(parser.feed('{"state":{"bri":', 0), [])
        self.assertEqual(parser.feed('{"state":{"bri":7}}\r\n', 1001)[0].brightness, 7)

    def test_uart_overrun_resynchronizes_at_newline(self):
        parser = WledFrameParser()
        self.assertEqual(parser.feed('{"state":{"bri":', 0, overrun=True), [])
        self.assertEqual(parser.feed('corrupt\r\n{"state":{"bri":42}}\r\n', 1)[0].brightness, 42)


if __name__ == "__main__":
    unittest.main()
