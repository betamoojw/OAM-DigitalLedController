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

    def test_advanced_state_and_configured_segment_are_parsed(self):
        state = parse_state(
            '{"state":{"transition":7,"pl":3,"lor":2,"mainseg":2,'
            '"udpn":{"send":true,"recv":false,"sgrp":5,"rgrp":9},'
            '"seg":[{"id":0,"bri":1},{"id":2,"on":true,"bri":120,"rev":true,"mi":false,"frz":true,"sel":true}]}}',
            segment_id=2,
        )
        self.assertIsNotNone(state)
        self.assertEqual((state.transition_ms, state.playlist, state.live_override, state.main_segment), (700, 3, 2, 2))
        self.assertEqual((state.sync_send, state.sync_receive, state.sync_send_groups, state.sync_receive_groups), (True, False, 5, 9))
        self.assertEqual((state.segment_on, state.segment_brightness), (True, 120))
        self.assertEqual((state.segment_reverse, state.segment_mirror, state.segment_freeze, state.segment_selected), (True, False, True, True))

    def test_extended_segment_and_device_info_are_parsed(self):
        state = parse_state(
            '{"state":{"error":9,"ledmap":2,"nl":{"on":true,"dur":9,"mode":2,"tbri":16,"rem":420},'
            '"seg":[{"id":0,"n":"Kitchen","start":10,"stop":100,"grp":2,"spc":1,"of":-3,'
            '"startY":1,"stopY":15,"rY":true,"mY":false,"tp":true,"set":2,"si":3,"m12":4,'
            '"c1":11,"c2":22,"c3":31,"o1":true,"o2":false,"o3":true,'
            '"col":[[1,2,3,4],[5,6,7,8],[9,10,11,12]]}]},'
            '"info":{"ver":"0.16.0","vid":2607201,"fxcount":180,"palcount":90,"uptime":1234,"freeheap":50000,'
            '"live":true,"liveseg":0,"lm":"DDP","lip":"10.0.0.8","ws":2,"ndc":4,"ip":"192.168.1.42","time":"2026-11-2, 15:46:06",'
            '"fs":{"u":123,"t":2048,"pmt":1800000000},"maps":[{"id":0},{"id":1,"n":"Desk"}],'
            '"wifi":{"rssi":-60,"signal":80,"channel":6,"bssid":"AA:BB:CC:DD:EE:FF"},'
            '"leds":{"maxseg":32,"count":300,"fps":42,"pwr":1750,"maxpwr":3000,"rgbw":true,"wv":2,"cct":4,"matrix":{"w":20,"h":15}}}}'
        )
        self.assertIsNotNone(state)
        self.assertEqual((state.effect_custom1, state.effect_custom2, state.effect_custom3), (11, 22, 31))
        self.assertEqual((state.segment_start, state.segment_stop, state.segment_grouping, state.segment_spacing, state.segment_offset), (10, 100, 2, 1, -3))
        self.assertEqual(state.secondary_color, (5, 6, 7, 8))
        self.assertEqual(state.tertiary_color, (9, 10, 11, 12))
        self.assertEqual((state.version, state.build_id, state.max_segments), ("0.16.0", 2607201, 32))
        self.assertEqual((state.live, state.live_mode, state.wifi_rssi, state.wifi_signal), (True, "DDP", -60, 80))
        self.assertEqual((state.nightlight_duration, state.nightlight_mode, state.nightlight_target, state.nightlight_remaining), (9, 2, 16, 420))
        self.assertEqual((state.segment_present, state.segment_name), (True, "Kitchen"))
        self.assertEqual((state.node_count, state.led_current_ma, state.led_max_current_ma), (4, 1750, 3000))
        self.assertEqual((state.ip_address, state.wifi_bssid, state.wifi_channel), ("192.168.1.42", "AA:BB:CC:DD:EE:FF", 6))
        self.assertEqual((state.led_count, state.led_fps, state.matrix_width, state.matrix_height), (300, 42, 20, 15))
        self.assertEqual(state.local_time, "2026-11-2, 15:46:06")
        self.assertEqual(
            (state.matrix_start_y, state.matrix_stop_y, state.matrix_reverse_y, state.matrix_mirror_y, state.matrix_transpose),
            (1, 15, True, False, True),
        )
        self.assertEqual((state.segment_set, state.sound_simulation, state.map_1d_2d), (2, 3, 4))
        self.assertEqual((state.fs_used_kb, state.fs_total_kb, state.presets_modified, state.led_map_count), (123, 2048, 1800000000, 2))
        self.assertEqual((state.realtime_source_ip, state.websocket_clients), ("10.0.0.8", 2))
        self.assertEqual((state.has_rgbw, state.has_white, state.has_cct, state.is_matrix), (True, True, True, True))

    def test_malformed_json_is_rejected(self):
        self.assertIsNone(parse_state('{"state":{"bri":'))

    def test_invalid_state_field_is_rejected(self):
        self.assertIsNone(parse_state('{"state":{"bri":300}}'))
        self.assertIsNone(parse_state('{"state":{"nl":{"mode":4}}}'))
        self.assertIsNone(parse_state('{"state":{"seg":[{"col":[[1,2]]}]}}'))
        self.assertIsNone(parse_state('{"state":{"seg":[{"cct":256}]}}'))
        self.assertIsNone(parse_state('{"state":{"lor":3}}'))
        self.assertIsNone(parse_state('{"state":{"udpn":{"send":1}}}'))
        self.assertIsNone(parse_state('{"state":{"seg":[{"frz":1}]}}'))
        self.assertIsNone(parse_state('{"state":{"nl":{"rem":15301}}}'))
        self.assertIsNone(parse_state('{"state":{"seg":[{"n":12}]}}'))
        self.assertIsNone(parse_state('{"state":{},"info":{"ndc":151}}'))
        self.assertIsNone(parse_state('{"state":{},"info":{"leds":{"maxpwr":65536}}}'))
        self.assertIsNone(parse_state('{"state":{"seg":[{"set":4}]}}'))
        self.assertIsNone(parse_state('{"state":{},"info":{"leds":{"wv":1}}}'))
        self.assertIsNone(parse_state('{"state":{},"info":{"ws":-2}}'))

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
        self.assertEqual(parser.uart_overruns, 1)

    def test_communication_health_tracks_valid_frames_and_errors(self):
        parser = WledFrameParser(capacity=48)
        self.assertFalse(parser.online(0, 15))
        parser.feed('{"state":{"bri":oops}}\r\n', 1)
        self.assertEqual(parser.json_errors, 1)
        parser.feed('{"state":{"bri":' + "1" * 64 + '}}\r\n', 2)
        self.assertEqual(parser.oversized_frames, 1)
        parser.feed('{"state":{"bri":42}}\r\n', 3000)
        self.assertTrue(parser.online(4000, 2))
        self.assertFalse(parser.online(5000, 2))
        self.assertTrue(parser.online(500000, 0))


if __name__ == "__main__":
    unittest.main()
