import unittest

from knx_simulation import (
    KO_DIM_ABS,
    KO_DIM_REL,
    KO_DIM_SW,
    KO_EFFECT,
    KO_EFFECT_INTENSITY,
    KO_EFFECT_SPEED,
    KO_GREEN,
    KO_CCT,
    KO_COLD_WHITE,
    KO_HUE,
    KO_NIGHTLIGHT,
    KO_PALETTE,
    KO_PRESET,
    KO_RED,
    KO_RGB,
    KO_RGBW,
    KO_SATURATION,
    KO_SCENE,
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
        self.assertEqual(controller.commands[-2:], [{"on": False}, {"bri": 128}])
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

    def test_group_object_dpt_wire_encoding_and_decoding(self):
        self.assertEqual(decode_dpt("DPT1", encode_dpt("DPT1", True)), True)
        self.assertEqual(decode_dpt("DPT3", encode_dpt("DPT3", (1, 4))), (1, 4))
        self.assertEqual(decode_dpt("DPT5", encode_dpt("DPT5", 200)), 200)
        self.assertAlmostEqual(decode_dpt("DPT5Angle", encode_dpt("DPT5Angle", 300)), 300, delta=1)
        self.assertEqual(decode_dpt("DPT17", encode_dpt("DPT17", 63)), 63)
        self.assertEqual(decode_dpt("DPT232", encode_dpt("DPT232", 0x102030)), 0x102030)
        self.assertEqual(decode_dpt("DPT7", encode_dpt("DPT7", 6500)), 6500)
        self.assertEqual(decode_dpt("DPT251", encode_dpt("DPT251", 0x10203040)), 0x10203040)

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
