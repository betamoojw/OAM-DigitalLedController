import unittest

from knx_simulation import (
    KO_DIM_ABS,
    KO_DIM_REL,
    KO_DIM_SW,
    KO_EFFECT,
    KO_EFFECT_CUSTOM1,
    KO_EFFECT_CUSTOM3,
    KO_EFFECT_INTENSITY,
    KO_EFFECT_OPTION2,
    KO_EFFECT_SPEED,
    KO_LIVE_OVERRIDE,
    KO_LIVE,
    KO_LEDMAP,
    KO_MAIN_SEGMENT,
    KO_GREEN,
    KO_CCT,
    KO_COLD_WHITE,
    KO_HUE,
    KO_NIGHTLIGHT,
    KO_NIGHTLIGHT_DURATION,
    KO_NIGHTLIGHT_MODE,
    KO_NIGHTLIGHT_TARGET,
    KO_PALETTE,
    KO_PRESET,
    KO_PLAYLIST_SAVE,
    KO_PLAYLIST_START,
    KO_PLAYLIST_STOP,
    KO_PRESET_ACTION_ARM,
    KO_PRESET_DELETE,
    KO_PRESET_NEXT,
    KO_PRESET_RANDOM,
    KO_PRESET_SAVE,
    KO_RED,
    KO_RGB,
    KO_RGBW,
    KO_SATURATION,
    KO_SCENE,
    KO_SEGMENT_BRIGHTNESS,
    KO_SEGMENT_FREEZE,
    KO_SEGMENT_MIRROR,
    KO_SEGMENT_REVERSE,
    KO_SEGMENT_SELECTED,
    KO_SEGMENT_SW,
    KO_SEGMENT_START,
    KO_SEGMENT_STOP,
    KO_SEGMENT_OFFSET,
    KO_SEGMENT_NAME,
    KO_SEGMENT_ACTION_ARM,
    KO_SEGMENT_DELETE,
    KO_SEGMENT_APPLY,
    KO_SECONDARY_RGBW,
    KO_STATE_REQUEST,
    KO_SYNC_RECEIVE,
    KO_SYNC_RECEIVE_GROUPS,
    KO_SYNC_SEND,
    KO_SYNC_SEND_GROUPS,
    KO_TRANSITION,
    KO_TIMEBASE,
    KO_DEVICE_TIME,
    KO_MANAGED_SEGMENT1_ON,
    KO_MANAGED_SEGMENT3_NAME,
    KO_MATRIX_START_Y,
    KO_MATRIX_STOP_Y,
    KO_MATRIX_REVERSE_Y,
    KO_MATRIX_MIRROR_Y,
    KO_MATRIX_TRANSPOSE,
    KO_SEGMENT_SET,
    KO_SOUND_SIMULATION,
    KO_MAP_1D_2D,
    KO_VALUE,
    KO_WARM_WHITE,
    KO_WHITE,
    KO_WHITE_REL,
    SimulatedController,
    SimulatedGroupObject,
    PollScheduler,
    decode_dpt,
    encode_dpt,
)


class KnxGroupObjectSimulationTests(unittest.TestCase):
    def test_ets_defaults_are_applied_at_startup(self):
        controller = SimulatedController(default_brightness=180, default_palette=4, default_effect=7, default_effect_speed=90, default_effect_intensity=70)
        self.assertEqual(controller.feedback["brightness"], 180)
        self.assertEqual(controller.feedback["palette"], 4)
        self.assertEqual(controller.feedback["effect"], 7)
        self.assertEqual(controller.feedback["effect_speed"], 90)
        self.assertEqual(controller.feedback["effect_intensity"], 70)

    def test_switch_and_absolute_brightness(self):
        controller = SimulatedController()
        controller.process_input(SimulatedGroupObject(KO_DIM_SW, False))
        controller.process_input(SimulatedGroupObject(KO_DIM_ABS, 128))
        self.assertEqual(controller.commands[-2:], [{"on": False, "tt": 7}, {"bri": 128, "tt": 7}])
        self.assertTrue(controller.feedback["on"])
        self.assertEqual(controller.feedback["brightness"], 128)

    def test_relative_brightness_uses_direction_and_step(self):
        controller = SimulatedController(default_brightness=128)
        controller.process_input(SimulatedGroupObject(KO_DIM_REL, (1, 2)))
        self.assertEqual(controller.brightness, 255)
        controller.process_input(SimulatedGroupObject(KO_DIM_REL, (0, 3)))
        self.assertEqual(controller.brightness, 191)

    def test_individual_rgb_and_combined_rgb(self):
        controller = SimulatedController()
        controller.process_input(SimulatedGroupObject(KO_RED, 10))
        controller.process_input(SimulatedGroupObject(KO_GREEN, 20))
        controller.process_input(SimulatedGroupObject(KO_RGB, 0x102030))
        self.assertEqual((controller.red, controller.green, controller.blue), (16, 32, 48))
        self.assertEqual(controller.feedback["rgb"], 0x102030)

    def test_rgbw_white_and_relative_white(self):
        controller = SimulatedController()
        controller.process_input(SimulatedGroupObject(KO_RGBW, 0x10203040))
        self.assertEqual((controller.red, controller.green, controller.blue, controller.white), (16, 32, 48, 64))
        controller.process_input(SimulatedGroupObject(KO_RGBW, 0xAABBCCDD, rgbw_mask=0x01))
        self.assertEqual((controller.red, controller.green, controller.blue, controller.white), (16, 32, 48, 221))
        controller.process_input(SimulatedGroupObject(KO_WHITE, 100))
        controller.process_input(SimulatedGroupObject(KO_WHITE_REL, (0, 2)))
        self.assertEqual(controller.white, 0)
        self.assertEqual(controller.feedback["rgbw"], 0x10203000)

    def test_cct_warm_and_cold_white_mapping(self):
        controller = SimulatedController(cct_kelvin_min=2700, cct_kelvin_max=6500)
        controller.process_input(SimulatedGroupObject(KO_CCT, 6500))
        self.assertEqual(controller.cct, 255)
        controller.process_input(SimulatedGroupObject(KO_WARM_WHITE, 80))
        controller.process_input(SimulatedGroupObject(KO_COLD_WHITE, 20))
        self.assertEqual(controller.feedback["cct"], controller.cct_to_kelvin(controller.cct))
        self.assertEqual((controller.feedback["warm_white"], controller.feedback["cold_white"]), (80, 20))

    def test_hsv_controls_reconcile_rgb_feedback(self):
        controller = SimulatedController()
        controller.process_input(SimulatedGroupObject(KO_RGB, 0xFF0000))
        controller.process_input(SimulatedGroupObject(KO_SATURATION, 255))
        controller.process_input(SimulatedGroupObject(KO_VALUE, 128))
        self.assertEqual((controller.red, controller.green, controller.blue), (128, 0, 0))
        controller.process_input(SimulatedGroupObject.from_dpt(KO_HUE, "DPT5Angle", 300))
        self.assertAlmostEqual(controller.red, 128, delta=2)
        self.assertEqual(controller.green, 0)
        self.assertAlmostEqual(controller.blue, 128, delta=2)

    def test_scene_and_preset_feedback(self):
        controller = SimulatedController()
        controller.process_input(SimulatedGroupObject(KO_SCENE, 4))
        self.assertEqual(controller.preset, 5)
        self.assertEqual(controller.feedback["scene"], 4)
        controller.process_input(SimulatedGroupObject(KO_PRESET, 100))
        self.assertEqual(controller.preset, 100)
        self.assertEqual(controller.feedback["scene"], 4)

    def test_nightlight_uses_ets_parameters(self):
        controller = SimulatedController(nightlight_duration=9, nightlight_mode=2, nightlight_target=16)
        controller.process_input(SimulatedGroupObject(KO_NIGHTLIGHT, True))
        self.assertEqual(controller.commands[-1]["nl"], {"on": True, "dur": 9, "mode": 2, "tbri": 16})
        self.assertTrue(controller.feedback["nightlight"])

    def test_nightlight_runtime_parameters_are_independently_writable(self):
        controller = SimulatedController()
        controller.process_input(SimulatedGroupObject(KO_NIGHTLIGHT_DURATION, 12))
        controller.process_input(SimulatedGroupObject(KO_NIGHTLIGHT_MODE, 3))
        controller.process_input(SimulatedGroupObject(KO_NIGHTLIGHT_TARGET, 24))
        self.assertEqual(
            controller.commands[-3:],
            [{"nl": {"dur": 12}}, {"nl": {"mode": 3}}, {"nl": {"tbri": 24}}],
        )

    def test_palette_and_effect_feedback(self):
        controller = SimulatedController()
        controller.process_input(SimulatedGroupObject(KO_PALETTE, 3))
        controller.process_input(SimulatedGroupObject(KO_EFFECT, 8))
        controller.process_input(SimulatedGroupObject(KO_EFFECT_SPEED, 90))
        controller.process_input(SimulatedGroupObject(KO_EFFECT_INTENSITY, 120))
        self.assertEqual(controller.feedback["palette"], 3)
        self.assertEqual(controller.feedback["effect"], 8)
        self.assertEqual(controller.feedback["effect_speed"], 90)
        self.assertEqual(controller.feedback["effect_intensity"], 120)

    def test_segment_commands_target_configured_segment_and_publish_feedback(self):
        controller = SimulatedController(segment_id=3, transition_ms=900)
        for ko, value in (
            (KO_SEGMENT_SW, False),
            (KO_SEGMENT_BRIGHTNESS, 120),
            (KO_SEGMENT_REVERSE, True),
            (KO_SEGMENT_MIRROR, True),
            (KO_SEGMENT_FREEZE, True),
            (KO_SEGMENT_SELECTED, True),
        ):
            controller.process_input(SimulatedGroupObject(ko, value))
        self.assertEqual(controller.commands[-1], {"tt": 9, "seg": [{"id": 3, "sel": True}]})
        self.assertEqual(controller.feedback["segment_brightness"], 120)
        self.assertTrue(controller.feedback["segment_freeze"])

    def test_sync_transition_and_advanced_commands(self):
        controller = SimulatedController(sync_send_groups=3, sync_receive_groups=5)
        controller.process_input(SimulatedGroupObject(KO_TRANSITION, 1250))
        controller.process_input(SimulatedGroupObject(KO_SYNC_SEND, True))
        controller.process_input(SimulatedGroupObject(KO_SYNC_RECEIVE, True))
        controller.process_input(SimulatedGroupObject(KO_SYNC_SEND_GROUPS, 7))
        controller.process_input(SimulatedGroupObject(KO_SYNC_RECEIVE_GROUPS, 9))
        controller.process_input(SimulatedGroupObject(KO_LIVE_OVERRIDE, 2))
        controller.process_input(SimulatedGroupObject(KO_MAIN_SEGMENT, 4))
        controller.process_input(SimulatedGroupObject(KO_STATE_REQUEST, True))
        self.assertEqual(controller.transition_ms, 1200)
        self.assertEqual(controller.commands[-1], {"v": True})
        self.assertEqual((controller.sync_send_groups, controller.sync_receive_groups), (7, 9))
        self.assertEqual((controller.live_override, controller.main_segment), (2, 4))

    def test_status_selects_configured_segment(self):
        controller = SimulatedController(segment_id=2)
        controller.apply_wled_state(
            '{"state":{"transition":8,"pl":4,"lor":1,"mainseg":2,'
            '"udpn":{"send":true,"recv":false,"sgrp":3,"rgrp":5},'
            '"seg":[{"id":0,"bri":1},{"id":2,"on":true,"bri":99,"rev":true,"mi":true,"frz":true,"sel":true}]}}'
        )
        self.assertEqual(controller.feedback["segment_brightness"], 99)
        self.assertEqual(controller.feedback["transition_ms"], 800)
        self.assertEqual(controller.feedback["playlist"], 4)
        self.assertTrue(controller.feedback["sync_send"])

    def test_effect_layout_and_secondary_color_commands(self):
        controller = SimulatedController(segment_id=2)
        controller.process_input(SimulatedGroupObject(KO_EFFECT_CUSTOM1, 44))
        controller.process_input(SimulatedGroupObject(KO_EFFECT_CUSTOM3, 31))
        controller.process_input(SimulatedGroupObject(KO_EFFECT_OPTION2, True))
        controller.process_input(SimulatedGroupObject(KO_SEGMENT_START, 10))
        controller.process_input(SimulatedGroupObject(KO_SEGMENT_STOP, 100))
        controller.process_input(SimulatedGroupObject(KO_SEGMENT_OFFSET, -3))
        controller.process_input(SimulatedGroupObject(KO_SECONDARY_RGBW, 0x10203040))
        self.assertEqual(controller.commands[-1]["seg"][0]["id"], 2)
        self.assertEqual(controller.commands[-1]["seg"][0]["col"][1], [16, 32, 48, 64])
        self.assertEqual((controller.effect_custom1, controller.effect_custom3, controller.effect_option2), (44, 31, True))

    def test_segment_management_requires_arm_for_delete(self):
        controller = SimulatedController(
            destructive_segment_actions=True,
            segment_id=2,
            segment_start=10,
            segment_stop=80,
            segment_grouping=2,
            segment_spacing=1,
            segment_offset=-3,
        )
        controller.process_input(SimulatedGroupObject(KO_SEGMENT_NAME, "Kitchen"))
        controller.process_input(SimulatedGroupObject(KO_SEGMENT_DELETE, True))
        self.assertNotEqual(controller.commands[-1]["seg"][0].get("stop"), 0)
        controller.process_input(SimulatedGroupObject(KO_SEGMENT_APPLY, True))
        self.assertEqual(
            controller.commands[-1]["seg"][0],
            {"id": 2, "start": 10, "stop": 80, "grp": 2, "spc": 1, "of": -3, "n": "Kitchen"},
        )
        controller.process_input(SimulatedGroupObject(KO_SEGMENT_ACTION_ARM, True))
        controller.process_input(SimulatedGroupObject(KO_SEGMENT_DELETE, True))
        self.assertEqual(controller.commands[-1]["seg"][0], {"id": 2, "stop": 0})
        self.assertFalse(controller.segment_action_armed)

    def test_playlist_and_armed_preset_management(self):
        controller = SimulatedController(playlist_preset_slot=20)
        controller.process_input(SimulatedGroupObject(KO_PRESET_ACTION_ARM, True))
        controller.process_input(SimulatedGroupObject(KO_PLAYLIST_SAVE, True))
        self.assertEqual(controller.commands[-1]["psave"], 20)
        self.assertEqual(controller.commands[-1]["playlist"]["ps"], [1, 2, 3])
        controller.process_input(SimulatedGroupObject(KO_PLAYLIST_START, True))
        self.assertEqual(controller.commands[-1]["ps"], 20)
        controller.process_input(SimulatedGroupObject(KO_PRESET_SAVE, 12))
        self.assertNotIn("psave", controller.commands[-1])
        controller.process_input(SimulatedGroupObject(KO_PRESET_ACTION_ARM, True))
        controller.process_input(SimulatedGroupObject(KO_PRESET_SAVE, 12))
        self.assertEqual(controller.commands[-1], {"psave": 12, "sb": False, "ib": True, "sc": False})
        controller.process_input(SimulatedGroupObject(KO_PRESET_ACTION_ARM, True))
        controller.process_input(SimulatedGroupObject(KO_PRESET_DELETE, 12))
        self.assertEqual(controller.commands[-1], {"pdel": 12})
        controller.process_input(SimulatedGroupObject(KO_PRESET_NEXT, True))
        controller.process_input(SimulatedGroupObject(KO_PRESET_RANDOM, True))
        self.assertEqual(controller.commands[-2:], [{"ps": "~"}, {"ps": "1~250r"}])

    def test_playlist_stop_and_device_time_commands(self):
        controller = SimulatedController(playlist_stop_preset=7)
        controller.process_input(SimulatedGroupObject(KO_PLAYLIST_STOP, True))
        controller.process_input(SimulatedGroupObject(KO_DEVICE_TIME, 1_800_000_000))
        self.assertEqual(controller.commands[-2], {"ps": 7, "tt": 7})
        self.assertEqual(controller.commands[-1], {"time": 1_800_000_000})

    def test_three_managed_segments_use_compact_bidirectional_objects(self):
        controller = SimulatedController(managed_segment_ids=(4, 5, 6))
        controller.process_input(SimulatedGroupObject(KO_MANAGED_SEGMENT1_ON, True))
        controller.process_input(SimulatedGroupObject(KO_MANAGED_SEGMENT1_ON + 3, 0x10203040))
        controller.process_input(SimulatedGroupObject(KO_MANAGED_SEGMENT1_ON + 4, 0x0D))
        controller.process_input(SimulatedGroupObject(KO_MANAGED_SEGMENT3_NAME, "Matrix Right"))
        self.assertEqual(controller.commands[-4]["seg"][0], {"id": 4, "on": True})
        self.assertEqual(controller.commands[-3]["seg"][0]["col"][0], [16, 32, 48, 64])
        self.assertEqual(
            controller.commands[-2]["seg"][0],
            {"id": 4, "sel": True, "rev": False, "mi": True, "frz": True},
        )
        self.assertEqual(controller.commands[-1]["seg"][0], {"id": 6, "n": "Matrix Right"})

    def test_matrix_2d_commands_are_range_checked(self):
        controller = SimulatedController(segment_id=2)
        for ko, value in (
            (KO_MATRIX_START_Y, 1),
            (KO_MATRIX_STOP_Y, 16),
            (KO_MATRIX_REVERSE_Y, True),
            (KO_MATRIX_MIRROR_Y, True),
            (KO_MATRIX_TRANSPOSE, True),
            (KO_SEGMENT_SET, 3),
            (KO_SOUND_SIMULATION, 2),
            (KO_MAP_1D_2D, 7),
        ):
            controller.process_input(SimulatedGroupObject(ko, value))
        self.assertEqual(controller.commands[-1]["seg"][0], {"id": 2, "m12": 7})
        command_count = len(controller.commands)
        controller.process_input(SimulatedGroupObject(KO_SEGMENT_SET, 4))
        self.assertEqual(len(controller.commands), command_count)

    def test_extended_info_feedback_is_not_synthesized(self):
        controller = SimulatedController()
        controller.apply_wled_state(
            '{"state":{"seg":[]},"info":{"lip":"10.0.0.8","ws":-1,'
            '"fs":{"u":100,"t":2000,"pmt":1800000000},"maps":[{"id":0},{"id":1}],'
            '"leds":{"rgbw":true,"wv":2,"cct":4,"matrix":{"w":8,"h":8}}}}'
        )
        self.assertEqual(controller.feedback["led_map_count"], 2)
        self.assertEqual(controller.feedback["realtime_source_ip"], "10.0.0.8")
        self.assertEqual(controller.feedback["websocket_clients"], -1)
        self.assertTrue(controller.feedback["has_rgbw"])
        self.assertTrue(controller.feedback["is_matrix"])

    def test_realtime_ledmap_timebase_and_no_notify(self):
        controller = SimulatedController(suppress_sync_notifications=True)
        controller.process_input(SimulatedGroupObject(KO_LIVE, True))
        controller.process_input(SimulatedGroupObject(KO_LEDMAP, 4))
        controller.process_input(SimulatedGroupObject(KO_TIMEBASE, 123456))
        self.assertTrue(all(command["udpn"]["nn"] for command in controller.commands))
        self.assertEqual(controller.commands[-2]["ledmap"], 4)
        self.assertEqual(controller.commands[-1]["tb"], 123456)

    def test_group_object_dpt_wire_encoding_and_decoding(self):
        self.assertEqual(decode_dpt("DPT1", encode_dpt("DPT1", True)), True)
        self.assertEqual(decode_dpt("DPT3", encode_dpt("DPT3", (1, 4))), (1, 4))
        self.assertEqual(decode_dpt("DPT5", encode_dpt("DPT5", 200)), 200)
        self.assertAlmostEqual(decode_dpt("DPT5Angle", encode_dpt("DPT5Angle", 300)), 300, delta=1)
        self.assertEqual(decode_dpt("DPT17", encode_dpt("DPT17", 63)), 63)
        self.assertEqual(decode_dpt("DPT232", encode_dpt("DPT232", 0x102030)), 0x102030)
        self.assertEqual(decode_dpt("DPT7", encode_dpt("DPT7", 6500)), 6500)
        self.assertEqual(decode_dpt("DPT251", encode_dpt("DPT251", 0x10203040)), 0x10203040)
        self.assertEqual(decode_dpt("DPT8", encode_dpt("DPT8", -1)), -1)
        self.assertEqual(decode_dpt("DPT12", encode_dpt("DPT12", 0xC0A8012A)), 0xC0A8012A)
        self.assertEqual(decode_dpt("DPT16", encode_dpt("DPT16", "AABBCCDDEEFF")), "AABBCCDDEEFF")

    def test_dpt_and_parameter_boundaries_are_rejected(self):
        with self.assertRaises(ValueError):
            encode_dpt("DPT5", 256)
        with self.assertRaises(ValueError):
            encode_dpt("DPT17", 64)
        with self.assertRaises(ValueError):
            encode_dpt("DPT3", (1, 8))
        with self.assertRaises(ValueError):
            SimulatedController(nightlight_duration=0)
        with self.assertRaises(ValueError):
            SimulatedController(default_effect=256)

    def test_feedback_writes_are_deduplicated(self):
        controller = SimulatedController(default_brightness=100)
        initial_writes = len(controller.feedback_writes)
        controller.process_input(SimulatedGroupObject.from_dpt(KO_DIM_SW, "DPT1", False))
        first_write_count = len(controller.feedback_writes)
        controller.process_input(SimulatedGroupObject.from_dpt(KO_DIM_SW, "DPT1", False))
        self.assertGreater(first_write_count, initial_writes)
        self.assertEqual(len(controller.feedback_writes), first_write_count)

    def test_wled_status_reconciles_feedback_without_duplicate_writes(self):
        controller = SimulatedController(default_brightness=100)
        controller.apply_wled_state('{"state":{"on":true,"bri":80,"ps":4,"seg":[{"fx":9}]}}')
        writes_after_reply = len(controller.feedback_writes)
        self.assertEqual((controller.brightness, controller.preset, controller.effect), (80, 4, 9))
        controller.apply_wled_state('{"state":{"on":true,"bri":80,"ps":4,"seg":[{"fx":9}]}}')
        self.assertEqual(len(controller.feedback_writes), writes_after_reply)

    def test_poll_scheduler_sends_once_then_uses_seconds(self):
        scheduler = PollScheduler(interval_seconds=5)
        self.assertTrue(scheduler.due(100))
        self.assertFalse(scheduler.due(5099))
        self.assertTrue(scheduler.due(5100))

    def test_poll_scheduler_zero_disables_periodic_requests(self):
        scheduler = PollScheduler(interval_seconds=0)
        self.assertTrue(scheduler.due(0))
        self.assertFalse(scheduler.due(1_000_000))


if __name__ == "__main__":
    unittest.main()
