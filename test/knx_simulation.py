"""Hardware-free simulation of the DigitalLedModule KNX input boundary."""

from __future__ import annotations

import colorsys
import json
from dataclasses import dataclass, field


KO_DIM_SW = 201
KO_DIM_REL = 202
KO_DIM_ABS = 203
KO_SCENE = 206
KO_RED = 208
KO_GREEN = 210
KO_BLUE = 212
KO_RGB = 214
KO_NIGHTLIGHT = 216
KO_PRESET = 218
KO_PALETTE = 220
KO_EFFECT = 222
KO_EFFECT_SPEED = 224
KO_EFFECT_INTENSITY = 226
KO_WHITE = 228
KO_CCT = 230
KO_WARM_WHITE = 232
KO_COLD_WHITE = 234
KO_HUE = 236
KO_SATURATION = 238
KO_VALUE = 240
KO_RGBW = 242
KO_RED_REL = 244
KO_GREEN_REL = 245
KO_BLUE_REL = 246
KO_WHITE_REL = 247
KO_WARM_WHITE_REL = 248
KO_COLD_WHITE_REL = 249
KO_HUE_REL = 250
KO_SATURATION_REL = 251
KO_VALUE_REL = 252
KO_EFFECT_REL = 253
KO_SEGMENT_SW = 254
KO_SEGMENT_BRIGHTNESS = 256
KO_SEGMENT_REVERSE = 258
KO_SEGMENT_MIRROR = 260
KO_SEGMENT_FREEZE = 262
KO_SEGMENT_SELECTED = 264
KO_TRANSITION = 266
KO_SYNC_SEND = 268
KO_SYNC_RECEIVE = 270
KO_SYNC_SEND_GROUPS = 272
KO_SYNC_RECEIVE_GROUPS = 274
KO_LIVE_OVERRIDE = 276
KO_MAIN_SEGMENT = 278
KO_NEXT_PLAYLIST = 280
KO_STATE_REQUEST = 282
KO_REBOOT = 283
KO_EFFECT_CUSTOM1 = 284
KO_EFFECT_CUSTOM2 = 286
KO_EFFECT_CUSTOM3 = 288
KO_EFFECT_OPTION1 = 290
KO_EFFECT_OPTION2 = 292
KO_EFFECT_OPTION3 = 294
KO_SEGMENT_START = 296
KO_SEGMENT_STOP = 298
KO_SEGMENT_GROUPING = 300
KO_SEGMENT_SPACING = 302
KO_SEGMENT_OFFSET = 304
KO_SECONDARY_RGBW = 306
KO_TERTIARY_RGBW = 308
KO_PLAYLIST_SAVE = 310
KO_PLAYLIST_START = 311
KO_PRESET_ACTION_ARM = 312
KO_PRESET_SAVE = 314
KO_PRESET_DELETE = 315
KO_PRESET_NEXT = 316
KO_PRESET_PREVIOUS = 317
KO_PRESET_RANDOM = 318
KO_LIVE = 319
KO_LEDMAP = 333
KO_TIMEBASE = 335
KO_NIGHTLIGHT_DURATION = 336
KO_NIGHTLIGHT_MODE = 338
KO_NIGHTLIGHT_TARGET = 340
KO_SEGMENT_NAME = 343
KO_SEGMENT_ACTION_ARM = 346
KO_SEGMENT_DELETE = 348
KO_SEGMENT_APPLY = 349
KO_PLAYLIST_STOP = 350
KO_DEVICE_TIME = 363
KO_MANAGED_SEGMENT1_ON = 366
KO_MANAGED_SEGMENT3_NAME = 383
KO_MATRIX_START_Y = 384
KO_MATRIX_STOP_Y = 385
KO_MATRIX_REVERSE_Y = 386
KO_MATRIX_MIRROR_Y = 387
KO_MATRIX_TRANSPOSE = 388
KO_SEGMENT_SET = 389
KO_SOUND_SIMULATION = 390
KO_MAP_1D_2D = 391


def encode_dpt(dpt: str, value: object) -> bytes:
    if dpt == "DPT1":
        if not isinstance(value, bool):
            raise ValueError("DPT1 requires bool")
        return bytes([int(value)])
    if dpt == "DPT3":
        direction, step = value
        if direction not in (0, 1) or step not in range(8):
            raise ValueError("DPT3 direction/step out of range")
        return bytes([(direction << 3) | step])
    if dpt == "DPT5":
        if not isinstance(value, int) or not 0 <= value <= 255:
            raise ValueError("DPT5 value out of range")
        return bytes([value])
    if dpt == "DPT5Angle":
        if not isinstance(value, int) or not 0 <= value <= 360:
            raise ValueError("DPT5 angle out of range")
        return bytes([round(value * 255 / 360)])
    if dpt == "DPT7":
        if not isinstance(value, int) or not 0 <= value <= 65535:
            raise ValueError("DPT7 value out of range")
        return value.to_bytes(2, "big")
    if dpt == "DPT8":
        if not isinstance(value, int) or not -32768 <= value <= 32767:
            raise ValueError("DPT8 value out of range")
        return value.to_bytes(2, "big", signed=True)
    if dpt == "DPT12":
        if not isinstance(value, int) or not 0 <= value <= 0xFFFFFFFF:
            raise ValueError("DPT12 value out of range")
        return value.to_bytes(4, "big")
    if dpt == "DPT16":
        if not isinstance(value, str) or len(value.encode("ascii")) > 14:
            raise ValueError("DPT16 requires at most 14 ASCII bytes")
        return value.encode("ascii").ljust(14, b"\x00")
    if dpt == "DPT17":
        if not isinstance(value, int) or not 0 <= value <= 63:
            raise ValueError("DPT17 scene out of range")
        return bytes([value])
    if dpt == "DPT232":
        if not isinstance(value, int) or not 0 <= value <= 0xFFFFFF:
            raise ValueError("DPT232 RGB out of range")
        return value.to_bytes(3, "big")
    if dpt == "DPT251":
        if not isinstance(value, int) or not 0 <= value <= 0xFFFFFFFF:
            raise ValueError("DPT251 RGBW out of range")
        return value.to_bytes(4, "big") + b"\x00\x0f"
    raise ValueError(f"unsupported DPT: {dpt}")


def decode_dpt(dpt: str, payload: bytes) -> object:
    if dpt == "DPT1" and len(payload) == 1:
        return bool(payload[0])
    if dpt == "DPT3" and len(payload) == 1:
        return (payload[0] >> 3 & 1, payload[0] & 7)
    if dpt in ("DPT5", "DPT17") and len(payload) == 1:
        return payload[0]
    if dpt == "DPT5Angle" and len(payload) == 1:
        return round(payload[0] * 360 / 255)
    if dpt == "DPT7" and len(payload) == 2:
        return int.from_bytes(payload, "big")
    if dpt == "DPT8" and len(payload) == 2:
        return int.from_bytes(payload, "big", signed=True)
    if dpt == "DPT12" and len(payload) == 4:
        return int.from_bytes(payload, "big")
    if dpt == "DPT16" and len(payload) == 14:
        return payload.rstrip(b"\x00").decode("ascii")
    if dpt == "DPT232" and len(payload) == 3:
        return int.from_bytes(payload, "big")
    if dpt == "DPT251" and len(payload) == 6:
        return int.from_bytes(payload[:4], "big")
    raise ValueError(f"invalid {dpt} payload")


@dataclass
class SimulatedGroupObject:
    number: int
    raw_value: object
    dpt: str | None = None
    rgbw_mask: int = 0x0F

    @classmethod
    def from_dpt(cls, number: int, dpt: str, value: object) -> "SimulatedGroupObject":
        payload = encode_dpt(dpt, value)
        return cls(number, payload, dpt, payload[5] & 0x0F if dpt == "DPT251" else 0x0F)

    def asap(self) -> int:
        return self.number

    def value(self, _dpt: object) -> object:
        return decode_dpt(self.dpt, self.raw_value) if self.dpt else self.raw_value


@dataclass
class SimulatedController:
    nightlight_duration: int = 3
    nightlight_mode: int = 1
    nightlight_target: int = 0
    default_brightness: int = 255
    default_palette: int = 0
    default_effect: int = 0
    default_effect_speed: int = 128
    default_effect_intensity: int = 128
    cct_kelvin_min: int = 2700
    cct_kelvin_max: int = 6500
    segment_id: int = 0
    transition_ms: int = 700
    sync_send_groups: int = 1
    sync_receive_groups: int = 1
    suppress_sync_notifications: bool = False
    playlist_preset_slot: int = 20
    playlist_presets: tuple[int, ...] = (1, 2, 3, 0, 0)
    playlist_durations: tuple[int, ...] = (100, 100, 100, 100, 100)
    playlist_transitions: tuple[int, ...] = (7, 7, 7, 7, 7)
    playlist_repeat: int = 0
    playlist_end: int = 0
    playlist_shuffle: bool = False
    preset_random_min: int = 1
    preset_random_max: int = 250
    destructive_preset_actions: bool = True
    destructive_segment_actions: bool = False
    playlist_stop_preset: int = 1
    preset_save_segment_bounds: bool = False
    preset_save_brightness: bool = True
    preset_save_selected_segments: bool = False
    brightness: int = 0
    red: int = 0
    green: int = 0
    blue: int = 0
    white: int = 0
    cct: int = 127
    on: bool = False
    nightlight: bool = False
    nightlight_remaining: int = -1
    preset: int = -1
    palette: int = 0
    effect: int = 0
    effect_speed: int = 0
    effect_intensity: int = 0
    segment_on: bool = True
    segment_brightness: int = 255
    segment_reverse: bool = False
    segment_mirror: bool = False
    segment_freeze: bool = False
    segment_selected: bool = False
    sync_send: bool = False
    sync_receive: bool = False
    live_override: int = 0
    main_segment: int = 0
    playlist: int = -1
    effect_custom1: int = 0
    effect_custom2: int = 0
    effect_custom3: int = 0
    effect_option1: bool = False
    effect_option2: bool = False
    effect_option3: bool = False
    segment_start: int = 0
    segment_stop: int = 0
    segment_grouping: int = 1
    segment_spacing: int = 0
    segment_offset: int = 0
    segment_name: str = ""
    segment_present: bool = False
    segment_action_armed: bool = False
    managed_segment_ids: tuple[int, int, int] = (1, 2, 3)
    managed_segments: list[dict[str, object]] = field(default_factory=lambda: [
        {"on": False, "bri": 0, "cct": 127, "color": 0, "options": 0, "name": ""},
        {"on": False, "bri": 0, "cct": 127, "color": 0, "options": 0, "name": ""},
        {"on": False, "bri": 0, "cct": 127, "color": 0, "options": 0, "name": ""},
    ])
    matrix_start_y: int = 0
    matrix_stop_y: int = 1
    matrix_reverse_y: bool = False
    matrix_mirror_y: bool = False
    matrix_transpose: bool = False
    segment_set: int = 0
    sound_simulation: int = 0
    map_1d_2d: int = 0
    secondary_color: int = 0
    tertiary_color: int = 0
    preset_action_armed: bool = False
    live: bool = False
    ledmap: int = 0
    commands: list[dict] = field(default_factory=list)
    feedback: dict[str, object] = field(default_factory=dict)
    feedback_writes: list[tuple[str, object]] = field(default_factory=list)

    def __post_init__(self) -> None:
        if not 1 <= self.nightlight_duration <= 255 or not 0 <= self.nightlight_mode <= 3:
            raise ValueError("invalid nightlight parameters")
        byte_parameters = (self.nightlight_target, self.default_brightness, self.default_palette, self.default_effect,
                           self.default_effect_speed, self.default_effect_intensity)
        if any(not 0 <= value <= 255 for value in byte_parameters):
            raise ValueError("invalid byte parameter")
        if not 2000 <= self.cct_kelvin_min < self.cct_kelvin_max <= 10000:
            raise ValueError("invalid CCT range")
        if not 0 <= self.segment_id <= 63 or not 0 <= self.transition_ms <= 65535:
            raise ValueError("invalid WLED command parameters")
        self.brightness = self.default_brightness
        self.palette = self.default_palette
        self.effect = self.default_effect
        self.effect_speed = self.default_effect_speed
        self.effect_intensity = self.default_effect_intensity
        self.on = self.brightness != 0
        self._publish_defaults()

    def _send(self, command: dict) -> None:
        if self.suppress_sync_notifications and "v" not in command and "rb" not in command:
            command = json.loads(json.dumps(command))
            command.setdefault("udpn", {})["nn"] = True
        self.commands.append(command)

    def _publish_defaults(self) -> None:
        self._set_feedback("brightness", self.brightness)
        self._set_feedback("on", self.on)
        self._set_feedback("palette", self.palette)
        self._set_feedback("effect", self.effect)
        self._set_feedback("effect_speed", self.effect_speed)
        self._set_feedback("effect_intensity", self.effect_intensity)

    def _set_feedback(self, key: str, value: object) -> None:
        if self.feedback.get(key) == value:
            return
        self.feedback[key] = value
        self.feedback_writes.append((key, value))

    def apply_wled_state(self, frame: str) -> None:
        root = json.loads(frame)
        state = root["state"]
        if "on" in state:
            self.on = bool(state["on"])
            self._set_feedback("on", self.on)
        if "bri" in state:
            self.brightness = int(state["bri"])
            self._set_feedback("brightness", self.brightness)
        if "nl" in state and "on" in state["nl"]:
            self.nightlight = bool(state["nl"]["on"])
            self._set_feedback("nightlight", self.nightlight)
        if "nl" in state:
            for key, attribute in (
                ("dur", "nightlight_duration"), ("mode", "nightlight_mode"),
                ("tbri", "nightlight_target"), ("rem", "nightlight_remaining"),
            ):
                if key in state["nl"]:
                    setattr(self, attribute, int(state["nl"][key]))
                    self._set_feedback(attribute, getattr(self, attribute))
        if "ps" in state:
            self.preset = int(state["ps"])
            self._set_feedback("preset", self.preset)
            if 1 <= self.preset <= 64:
                self._set_feedback("scene", self.preset - 1)
        if "transition" in state:
            self.transition_ms = min(int(state["transition"]) * 100, 65535)
        for key, attribute in (("pl", "playlist"), ("lor", "live_override"), ("mainseg", "main_segment")):
            if key in state:
                setattr(self, attribute, int(state[key]))
        sync = state.get("udpn", {})
        for key, attribute in (
            ("send", "sync_send"), ("recv", "sync_receive"),
            ("sgrp", "sync_send_groups"), ("rgrp", "sync_receive_groups"),
        ):
            if key in sync:
                setattr(self, attribute, sync[key])
        segments = state.get("seg", [])
        segment = next((item for index, item in enumerate(segments) if item.get("id", index) == self.segment_id), None)
        self.segment_present = segment is not None
        self._set_feedback("segment_present", self.segment_present)
        segment = segment or {}
        for key, attribute in (
            ("on", "segment_on"), ("bri", "segment_brightness"), ("rev", "segment_reverse"),
            ("mi", "segment_mirror"), ("frz", "segment_freeze"), ("sel", "segment_selected"),
            ("startY", "matrix_start_y"), ("stopY", "matrix_stop_y"), ("rY", "matrix_reverse_y"),
            ("mY", "matrix_mirror_y"), ("tp", "matrix_transpose"), ("set", "segment_set"),
            ("si", "sound_simulation"), ("m12", "map_1d_2d"),
        ):
            if key in segment:
                setattr(self, attribute, segment[key])
        if "cct" in segment:
            self.cct = int(segment["cct"])
            self._publish_color_feedback()
            self._set_feedback("cct", self.cct_to_kelvin(self.cct))
        if "n" in segment:
            self.segment_name = str(segment["n"])[:14]
            self._set_feedback("segment_name", self.segment_name)
        color = segment.get("col", [])
        if color:
            self.red, self.green, self.blue = map(int, color[0][:3])
            if len(color[0]) == 4:
                self.white = int(color[0][3])
            self._publish_color_feedback()
        for key, attribute in (("pal", "palette"), ("fx", "effect"), ("sx", "effect_speed"), ("ix", "effect_intensity")):
            if key in segment:
                value = int(segment[key])
                setattr(self, attribute, value)
                self._set_feedback(attribute, value)
        for managed_index, managed_id in enumerate(self.managed_segment_ids):
            managed_segment = next(
                (item for index, item in enumerate(segments) if item.get("id", index) == managed_id),
                None,
            )
            managed = self.managed_segments[managed_index]
            if managed_segment is None:
                managed["options"] = int(managed["options"]) & 0x0F
                continue
            managed["on"] = bool(managed_segment.get("on", managed["on"]))
            managed["bri"] = int(managed_segment.get("bri", managed["bri"]))
            managed["cct"] = int(managed_segment.get("cct", managed["cct"]))
            options = (
                (1 if managed_segment.get("sel", False) else 0)
                | (2 if managed_segment.get("rev", False) else 0)
                | (4 if managed_segment.get("mi", False) else 0)
                | (8 if managed_segment.get("frz", False) else 0)
                | 0x80
            )
            managed["options"] = options
            managed["name"] = str(managed_segment.get("n", managed["name"]))[:14]
            if managed_segment.get("col"):
                components = list(managed_segment["col"][0]) + [0]
                managed["color"] = (int(components[0]) << 24) | (int(components[1]) << 16) | (int(components[2]) << 8) | int(components[3])
            self._set_feedback(f"managed_segment_{managed_index + 1}", dict(managed))
        info = root.get("info", {})
        if info:
            leds = info.get("leds", {})
            filesystem = info.get("fs", {})
            self._set_feedback("fs_used_kb", int(filesystem.get("u", 0)))
            self._set_feedback("fs_total_kb", int(filesystem.get("t", 0)))
            self._set_feedback("presets_modified", int(filesystem.get("pmt", 0)))
            self._set_feedback("led_map_count", len(info.get("maps", [])))
            self._set_feedback("realtime_source_ip", info.get("lip", ""))
            self._set_feedback("websocket_clients", int(info.get("ws", -1)))
            self._set_feedback("has_rgbw", bool(leds.get("rgbw", False)))
            self._set_feedback("has_white", bool(leds.get("wv", 0)))
            self._set_feedback("has_cct", bool(leds.get("cct", 0)))
            self._set_feedback("is_matrix", "matrix" in leds)
        self._publish_advanced_feedback()

    def process_input(self, ko: SimulatedGroupObject) -> None:
        value = ko.value(None)
        if KO_MANAGED_SEGMENT1_ON <= ko.asap() <= KO_MANAGED_SEGMENT3_NAME:
            self._process_managed_segment_input(ko, value)
            return
        if ko.asap() == KO_DIM_SW:
            self.on = bool(value)
            self._send({"on": self.on, "tt": min(round(self.transition_ms / 100), 655)})
            self._set_feedback("on", self.on)
        elif ko.asap() == KO_DIM_ABS:
            self.brightness = max(0, min(255, int(value)))
            self.on = self.brightness != 0
            self._send({"bri": self.brightness, "tt": min(round(self.transition_ms / 100), 655)})
            self._set_feedback("brightness", self.brightness)
            self._set_feedback("on", self.on)
        elif ko.asap() == KO_DIM_REL:
            direction, step = value
            if 1 <= step <= 7:
                delta = round(255 / (1 << (step - 1)))
                self.brightness = max(0, min(255, self.brightness + (delta if direction else -delta)))
                self.on = self.brightness != 0
                self._send({"bri": self.brightness, "tt": min(round(self.transition_ms / 100), 655)})
                self._set_feedback("brightness", self.brightness)
                self._set_feedback("on", self.on)
        elif ko.asap() in (KO_RED, KO_GREEN, KO_BLUE, KO_RGB):
            if ko.asap() == KO_RGB:
                self.red, self.green, self.blue = (value >> 16) & 255, (value >> 8) & 255, value & 255
            elif ko.asap() == KO_RED:
                self.red = int(value)
            elif ko.asap() == KO_GREEN:
                self.green = int(value)
            else:
                self.blue = int(value)
            self._send_rgbw()
        elif ko.asap() == KO_NIGHTLIGHT:
            self.nightlight = bool(value)
            self._send({"nl": {"on": self.nightlight, "dur": self.nightlight_duration, "mode": self.nightlight_mode, "tbri": self.nightlight_target}, "tt": min(round(self.transition_ms / 100), 655)})
            self._set_feedback("nightlight", self.nightlight)
        elif ko.asap() in (KO_NIGHTLIGHT_DURATION, KO_NIGHTLIGHT_MODE, KO_NIGHTLIGHT_TARGET):
            key, attribute = {
                KO_NIGHTLIGHT_DURATION: ("dur", "nightlight_duration"),
                KO_NIGHTLIGHT_MODE: ("mode", "nightlight_mode"),
                KO_NIGHTLIGHT_TARGET: ("tbri", "nightlight_target"),
            }[ko.asap()]
            numeric = int(value)
            if key == "mode" and not 0 <= numeric <= 3:
                return
            setattr(self, attribute, numeric)
            self._send({"nl": {key: numeric}})
            self._set_feedback(attribute, numeric)
        elif ko.asap() in (KO_SCENE, KO_PRESET):
            self.preset = int(value) + 1 if ko.asap() == KO_SCENE else int(value)
            self._send({"ps": self.preset, "tt": min(round(self.transition_ms / 100), 655)})
            self._set_feedback("preset", self.preset)
            if 1 <= self.preset <= 64:
                self._set_feedback("scene", self.preset - 1)
        elif ko.asap() in (KO_PALETTE, KO_EFFECT, KO_EFFECT_SPEED, KO_EFFECT_INTENSITY):
            key = {KO_PALETTE: "pal", KO_EFFECT: "fx", KO_EFFECT_SPEED: "sx", KO_EFFECT_INTENSITY: "ix"}[ko.asap()]
            setattr(self, {"pal": "palette", "fx": "effect", "sx": "effect_speed", "ix": "effect_intensity"}[key], int(value))
            self.on = True
            self._send({"on": True, "tt": min(round(self.transition_ms / 100), 655), "seg": [{"id": self.segment_id, key: int(value)}]})
            self._publish_defaults()
        elif ko.asap() in (KO_WHITE, KO_RGBW):
            if ko.asap() == KO_RGBW:
                if not ko.rgbw_mask & 0x0F:
                    return
                if ko.rgbw_mask & 0x08:
                    self.red = (int(value) >> 24) & 255
                if ko.rgbw_mask & 0x04:
                    self.green = (int(value) >> 16) & 255
                if ko.rgbw_mask & 0x02:
                    self.blue = (int(value) >> 8) & 255
                if ko.rgbw_mask & 0x01:
                    self.white = int(value) & 255
            else:
                self.white = int(value)
            self._send_rgbw()
        elif ko.asap() == KO_CCT:
            kelvin = max(self.cct_kelvin_min, min(self.cct_kelvin_max, int(value)))
            self.cct = self.kelvin_to_cct(kelvin)
            self._send({"tt": min(round(self.transition_ms / 100), 655), "seg": [{"id": self.segment_id, "cct": self.cct}]})
            self._publish_color_feedback()
            self._set_feedback("cct", self.cct_to_kelvin(self.cct))
        elif ko.asap() in (KO_WARM_WHITE, KO_COLD_WHITE):
            warm = round(self.white * (255 - self.cct) / 255)
            cold = round(self.white * self.cct / 255)
            if ko.asap() == KO_WARM_WHITE:
                warm = int(value)
            else:
                cold = int(value)
            total = warm + cold
            self.white = min(255, total)
            if total:
                self.cct = round(cold * 255 / total)
            self._send({"on": any((self.red, self.green, self.blue, self.white)), "tt": min(round(self.transition_ms / 100), 655),
                        "seg": [{"id": self.segment_id, "col": [[self.red, self.green, self.blue, self.white]], "cct": self.cct}]})
            self._publish_color_feedback()
            self._set_feedback("cct", self.cct_to_kelvin(self.cct))
        elif ko.asap() in (KO_HUE, KO_SATURATION, KO_VALUE):
            hue, saturation, hsv_value = colorsys.rgb_to_hsv(self.red / 255, self.green / 255, self.blue / 255)
            if ko.asap() == KO_HUE:
                hue = (int(value) % 360) / 360
            elif ko.asap() == KO_SATURATION:
                saturation = int(value) / 255
            else:
                hsv_value = int(value) / 255
            self.red, self.green, self.blue = (round(component * 255) for component in colorsys.hsv_to_rgb(hue, saturation, hsv_value))
            self._send_rgbw()
        elif ko.asap() in (
            KO_RED_REL, KO_GREEN_REL, KO_BLUE_REL, KO_WHITE_REL, KO_WARM_WHITE_REL, KO_COLD_WHITE_REL,
            KO_HUE_REL, KO_SATURATION_REL, KO_VALUE_REL, KO_EFFECT_REL,
        ):
            direction, step = value
            if not 1 <= step <= 7:
                return
            full_scale = 30 if ko.asap() == KO_HUE_REL else 10 if ko.asap() == KO_EFFECT_REL else 255
            delta = round(full_scale / (1 << (step - 1))) * (1 if direction else -1)
            if ko.asap() in (KO_RED_REL, KO_GREEN_REL, KO_BLUE_REL, KO_WHITE_REL):
                attribute = {
                    KO_RED_REL: "red", KO_GREEN_REL: "green", KO_BLUE_REL: "blue", KO_WHITE_REL: "white"
                }[ko.asap()]
                setattr(self, attribute, max(0, min(255, getattr(self, attribute) + delta)))
                self._send_rgbw()
            elif ko.asap() in (KO_WARM_WHITE_REL, KO_COLD_WHITE_REL):
                warm = round(self.white * (255 - self.cct) / 255)
                cold = round(self.white * self.cct / 255)
                target = KO_WARM_WHITE if ko.asap() == KO_WARM_WHITE_REL else KO_COLD_WHITE
                current = warm if target == KO_WARM_WHITE else cold
                self.process_input(SimulatedGroupObject(target, max(0, min(255, current + delta))))
            elif ko.asap() == KO_EFFECT_REL:
                self.process_input(SimulatedGroupObject(KO_EFFECT, max(0, min(255, self.effect + delta))))
            else:
                hue, saturation, hsv_value = colorsys.rgb_to_hsv(self.red / 255, self.green / 255, self.blue / 255)
                if ko.asap() == KO_HUE_REL:
                    hue = (hue + delta / 360) % 1
                elif ko.asap() == KO_SATURATION_REL:
                    saturation = max(0, min(1, saturation + delta / 255))
                else:
                    hsv_value = max(0, min(1, hsv_value + delta / 255))
                self.red, self.green, self.blue = (round(component * 255) for component in colorsys.hsv_to_rgb(hue, saturation, hsv_value))
                self._send_rgbw()
        elif ko.asap() in (KO_SEGMENT_SW, KO_SEGMENT_REVERSE, KO_SEGMENT_MIRROR, KO_SEGMENT_FREEZE, KO_SEGMENT_SELECTED):
            key, attribute = {
                KO_SEGMENT_SW: ("on", "segment_on"),
                KO_SEGMENT_REVERSE: ("rev", "segment_reverse"),
                KO_SEGMENT_MIRROR: ("mi", "segment_mirror"),
                KO_SEGMENT_FREEZE: ("frz", "segment_freeze"),
                KO_SEGMENT_SELECTED: ("sel", "segment_selected"),
            }[ko.asap()]
            setattr(self, attribute, bool(value))
            self._send({"tt": min(round(self.transition_ms / 100), 655), "seg": [{"id": self.segment_id, key: bool(value)}]})
            self._publish_advanced_feedback()
        elif ko.asap() == KO_SEGMENT_BRIGHTNESS:
            self.segment_brightness = int(value)
            self._send({"on": True, "tt": min(round(self.transition_ms / 100), 655), "seg": [{"id": self.segment_id, "bri": int(value)}]})
            self._publish_advanced_feedback()
        elif ko.asap() == KO_TRANSITION:
            deciseconds = round(int(value) / 100)
            self.transition_ms = min(deciseconds * 100, 65535)
            self._send({"transition": deciseconds})
            self._publish_advanced_feedback()
        elif ko.asap() in (KO_SYNC_SEND, KO_SYNC_RECEIVE):
            send = ko.asap() == KO_SYNC_SEND
            setattr(self, "sync_send" if send else "sync_receive", bool(value))
            self._send({"udpn": {"send": bool(value), "sgrp": self.sync_send_groups}} if send else
                       {"udpn": {"rgrp": self.sync_receive_groups if value else 0}})
            self._publish_advanced_feedback()
        elif ko.asap() in (KO_SYNC_SEND_GROUPS, KO_SYNC_RECEIVE_GROUPS):
            send = ko.asap() == KO_SYNC_SEND_GROUPS
            setattr(self, "sync_send_groups" if send else "sync_receive_groups", int(value))
            self._send({"udpn": {("sgrp" if send else "rgrp"): int(value)}})
            self._publish_advanced_feedback()
        elif ko.asap() == KO_LIVE_OVERRIDE and 0 <= int(value) <= 2:
            self.live_override = int(value)
            self._send({"lor": self.live_override})
            self._publish_advanced_feedback()
        elif ko.asap() == KO_MAIN_SEGMENT:
            self.main_segment = int(value)
            self._send({"mainseg": self.main_segment})
            self._publish_advanced_feedback()
        elif ko.asap() == KO_NEXT_PLAYLIST and value:
            self._send({"np": True})
        elif ko.asap() == KO_STATE_REQUEST and value:
            self._send({"v": True})
        elif ko.asap() == KO_REBOOT and value:
            self._send({"rb": True})
        elif ko.asap() in (KO_EFFECT_CUSTOM1, KO_EFFECT_CUSTOM2, KO_EFFECT_CUSTOM3):
            key, attribute = {
                KO_EFFECT_CUSTOM1: ("c1", "effect_custom1"),
                KO_EFFECT_CUSTOM2: ("c2", "effect_custom2"),
                KO_EFFECT_CUSTOM3: ("c3", "effect_custom3"),
            }[ko.asap()]
            if key == "c3" and int(value) > 31:
                return
            setattr(self, attribute, int(value))
            self._send({"tt": min(round(self.transition_ms / 100), 655),
                        "seg": [{"id": self.segment_id, key: int(value)}]})
        elif ko.asap() in (KO_EFFECT_OPTION1, KO_EFFECT_OPTION2, KO_EFFECT_OPTION3):
            key, attribute = {
                KO_EFFECT_OPTION1: ("o1", "effect_option1"),
                KO_EFFECT_OPTION2: ("o2", "effect_option2"),
                KO_EFFECT_OPTION3: ("o3", "effect_option3"),
            }[ko.asap()]
            setattr(self, attribute, bool(value))
            self._send({"tt": min(round(self.transition_ms / 100), 655),
                        "seg": [{"id": self.segment_id, key: bool(value)}]})
        elif ko.asap() in (KO_SEGMENT_START, KO_SEGMENT_STOP, KO_SEGMENT_GROUPING, KO_SEGMENT_SPACING, KO_SEGMENT_OFFSET):
            key, attribute = {
                KO_SEGMENT_START: ("start", "segment_start"),
                KO_SEGMENT_STOP: ("stop", "segment_stop"),
                KO_SEGMENT_GROUPING: ("grp", "segment_grouping"),
                KO_SEGMENT_SPACING: ("spc", "segment_spacing"),
                KO_SEGMENT_OFFSET: ("of", "segment_offset"),
            }[ko.asap()]
            setattr(self, attribute, int(value))
            self._send({"tt": min(round(self.transition_ms / 100), 655),
                        "seg": [{"id": self.segment_id, key: int(value)}]})
        elif ko.asap() in (KO_SECONDARY_RGBW, KO_TERTIARY_RGBW):
            index = 1 if ko.asap() == KO_SECONDARY_RGBW else 2
            attribute = "secondary_color" if index == 1 else "tertiary_color"
            setattr(self, attribute, int(value))
            colors = [
                [self.red, self.green, self.blue, self.white],
                list(int(self.secondary_color).to_bytes(4, "big")),
                list(int(self.tertiary_color).to_bytes(4, "big")),
            ]
            self._send({"tt": min(round(self.transition_ms / 100), 655),
                        "seg": [{"id": self.segment_id, "col": colors[:index + 1]}]})
        elif ko.asap() == KO_PLAYLIST_SAVE and value:
            if not self.preset_action_armed or not self.destructive_preset_actions:
                return
            entries = [
                (preset, duration, transition)
                for preset, duration, transition in zip(
                    self.playlist_presets, self.playlist_durations, self.playlist_transitions
                )
                if 1 <= preset <= 250
            ]
            self._send({
                "psave": self.playlist_preset_slot, "n": "KNX Playlist", "o": True, "on": True,
                "playlist": {
                    "ps": [entry[0] for entry in entries],
                    "dur": [entry[1] for entry in entries],
                    "transition": [entry[2] for entry in entries],
                    "repeat": self.playlist_repeat, "end": self.playlist_end, "r": self.playlist_shuffle,
                },
            })
            self.preset_action_armed = False
        elif ko.asap() == KO_PLAYLIST_START and value:
            self._send({"ps": self.playlist_preset_slot, "tt": min(round(self.transition_ms / 100), 655)})
        elif ko.asap() == KO_PRESET_ACTION_ARM:
            self.preset_action_armed = bool(value) and self.destructive_preset_actions
        elif ko.asap() in (KO_PRESET_SAVE, KO_PRESET_DELETE):
            if not self.preset_action_armed or not self.destructive_preset_actions:
                return
            if ko.asap() == KO_PRESET_SAVE:
                self._send({
                    "psave": int(value),
                    "sb": self.preset_save_segment_bounds,
                    "ib": self.preset_save_brightness,
                    "sc": self.preset_save_selected_segments,
                })
            else:
                self._send({"pdel": int(value)})
            self.preset_action_armed = False
        elif ko.asap() in (KO_PRESET_NEXT, KO_PRESET_PREVIOUS, KO_PRESET_RANDOM) and value:
            selection = {
                KO_PRESET_NEXT: "~",
                KO_PRESET_PREVIOUS: "~-",
                KO_PRESET_RANDOM: f"{self.preset_random_min}~{self.preset_random_max}r",
            }[ko.asap()]
            self._send({"ps": selection})
        elif ko.asap() == KO_LIVE:
            self.live = bool(value)
            self._send({"live": self.live})
        elif ko.asap() == KO_LEDMAP and 0 <= int(value) <= 15:
            self.ledmap = int(value)
            self._send({"ledmap": self.ledmap})
        elif ko.asap() == KO_TIMEBASE:
            self._send({"tb": int(value)})
        elif ko.asap() == KO_SEGMENT_NAME:
            self.segment_name = str(value)[:14]
            self._send({"tt": min(round(self.transition_ms / 100), 655), "seg": [{"id": self.segment_id, "n": self.segment_name}]})
            self._set_feedback("segment_name", self.segment_name)
        elif ko.asap() == KO_SEGMENT_ACTION_ARM:
            self.segment_action_armed = bool(value) and self.destructive_segment_actions
            self._set_feedback("segment_action_armed", self.segment_action_armed)
        elif ko.asap() == KO_SEGMENT_DELETE:
            if value and self.destructive_segment_actions and self.segment_action_armed:
                self._send({"tt": min(round(self.transition_ms / 100), 655), "seg": [{"id": self.segment_id, "stop": 0}]})
                self.segment_action_armed = False
                self._set_feedback("segment_action_armed", False)
        elif ko.asap() == KO_SEGMENT_APPLY:
            if value and self.segment_stop > self.segment_start:
                segment = {
                    "id": self.segment_id, "start": self.segment_start, "stop": self.segment_stop,
                    "grp": max(self.segment_grouping, 1), "spc": self.segment_spacing, "of": self.segment_offset,
                }
                if self.segment_name:
                    segment["n"] = self.segment_name
                self._send({"tt": min(round(self.transition_ms / 100), 655), "seg": [segment]})
        elif ko.asap() == KO_PLAYLIST_STOP:
            if value and 1 <= self.playlist_stop_preset <= 250:
                self._send({"ps": self.playlist_stop_preset, "tt": min(round(self.transition_ms / 100), 655)})
        elif ko.asap() == KO_DEVICE_TIME:
            epoch = int(value)
            if epoch not in (0, 0xFFFFFFFF):
                self._send({"time": epoch})
        elif ko.asap() in (KO_MATRIX_START_Y, KO_MATRIX_STOP_Y):
            key = "startY" if ko.asap() == KO_MATRIX_START_Y else "stopY"
            attribute = "matrix_start_y" if ko.asap() == KO_MATRIX_START_Y else "matrix_stop_y"
            setattr(self, attribute, int(value))
            self._send({"tt": min(round(self.transition_ms / 100), 655), "seg": [{"id": self.segment_id, key: int(value)}]})
        elif ko.asap() in (KO_MATRIX_REVERSE_Y, KO_MATRIX_MIRROR_Y, KO_MATRIX_TRANSPOSE):
            key, attribute = {
                KO_MATRIX_REVERSE_Y: ("rY", "matrix_reverse_y"),
                KO_MATRIX_MIRROR_Y: ("mY", "matrix_mirror_y"),
                KO_MATRIX_TRANSPOSE: ("tp", "matrix_transpose"),
            }[ko.asap()]
            setattr(self, attribute, bool(value))
            self._send({"tt": min(round(self.transition_ms / 100), 655), "seg": [{"id": self.segment_id, key: bool(value)}]})
        elif ko.asap() in (KO_SEGMENT_SET, KO_SOUND_SIMULATION, KO_MAP_1D_2D):
            key, attribute, maximum = {
                KO_SEGMENT_SET: ("set", "segment_set", 3),
                KO_SOUND_SIMULATION: ("si", "sound_simulation", 3),
                KO_MAP_1D_2D: ("m12", "map_1d_2d", 7),
            }[ko.asap()]
            numeric = int(value)
            if 0 <= numeric <= maximum:
                setattr(self, attribute, numeric)
                self._send({"on": True, "tt": min(round(self.transition_ms / 100), 655), "seg": [{"id": self.segment_id, key: numeric}]})

    def _process_managed_segment_input(self, ko: SimulatedGroupObject, value: object) -> None:
        offset = ko.asap() - KO_MANAGED_SEGMENT1_ON
        index, field_index = divmod(offset, 6)
        managed = self.managed_segments[index]
        segment: dict[str, object] = {"id": self.managed_segment_ids[index]}
        if field_index == 0:
            managed["on"] = bool(value)
            segment["on"] = bool(value)
        elif field_index == 1:
            managed["bri"] = int(value)
            segment["bri"] = int(value)
        elif field_index == 2:
            cct = self.kelvin_to_cct(int(value))
            managed["cct"] = cct
            segment["cct"] = cct
        elif field_index == 3:
            if not ko.rgbw_mask & 0x0F:
                return
            color = int(managed["color"])
            for mask, shift in ((0x08, 24), (0x04, 16), (0x02, 8), (0x01, 0)):
                if ko.rgbw_mask & mask:
                    color = (color & ~(0xFF << shift)) | (((int(value) >> shift) & 0xFF) << shift)
            managed["color"] = color
            segment["col"] = [[(color >> 24) & 255, (color >> 16) & 255, (color >> 8) & 255, color & 255]]
        elif field_index == 4:
            options = int(value) & 0x0F
            managed["options"] = options
            segment.update({"sel": bool(options & 1), "rev": bool(options & 2), "mi": bool(options & 4), "frz": bool(options & 8)})
        else:
            managed["name"] = str(value)[:14]
            segment["n"] = managed["name"]
        self._send({"tt": min(round(self.transition_ms / 100), 655), "seg": [segment]})

    def _send_rgbw(self) -> None:
        self.on = any((self.red, self.green, self.blue, self.white))
        self._send({"on": self.on, "tt": min(round(self.transition_ms / 100), 655),
                    "seg": [{"id": self.segment_id, "col": [[self.red, self.green, self.blue, self.white]]}]})
        self._set_feedback("on", self.on)
        self._publish_color_feedback()

    def _publish_color_feedback(self) -> None:
        self._set_feedback("red", self.red)
        self._set_feedback("green", self.green)
        self._set_feedback("blue", self.blue)
        self._set_feedback("white", self.white)
        self._set_feedback("rgb", (self.red << 16) | (self.green << 8) | self.blue)
        self._set_feedback("rgbw", (self.red << 24) | (self.green << 16) | (self.blue << 8) | self.white)
        self._set_feedback("rgbw_mask", 0x0F)
        self._set_feedback("warm_white", round(self.white * (255 - self.cct) / 255))
        self._set_feedback("cold_white", round(self.white * self.cct / 255))
        hue, saturation, value = colorsys.rgb_to_hsv(self.red / 255, self.green / 255, self.blue / 255)
        self._set_feedback("hue", round(hue * 360))
        self._set_feedback("saturation", round(saturation * 255))
        self._set_feedback("value", round(value * 255))

    def _publish_advanced_feedback(self) -> None:
        for key in (
            "segment_on", "segment_brightness", "segment_reverse", "segment_mirror", "segment_freeze",
            "segment_selected", "transition_ms", "sync_send", "sync_receive", "sync_send_groups",
            "sync_receive_groups", "live_override", "main_segment", "playlist",
        ):
            value = getattr(self, key)
            self._set_feedback(key, 0 if key == "playlist" and value < 1 else value)
        self._set_feedback("playlist_active", self.playlist >= 0)
        self._set_feedback("preset_active", self.preset >= 1)

    def cct_to_kelvin(self, cct: int) -> int:
        return self.cct_kelvin_min + round(cct * (self.cct_kelvin_max - self.cct_kelvin_min) / 255)

    def kelvin_to_cct(self, kelvin: int) -> int:
        return round((kelvin - self.cct_kelvin_min) * 255 / (self.cct_kelvin_max - self.cct_kelvin_min))


@dataclass
class PollScheduler:
    interval_seconds: int
    first_request: bool = True
    last_request_ms: int = 0

    def due(self, now_ms: int) -> bool:
        if self.first_request:
            self.first_request = False
            self.last_request_ms = now_ms
            return True
        if self.interval_seconds == 0:
            return False
        if now_ms - self.last_request_ms >= self.interval_seconds * 1000:
            self.last_request_ms = now_ms
            return True
        return False
