"""Host-side WLED state contract used by protocol regression tests."""

from __future__ import annotations

import json
from dataclasses import dataclass
from typing import Any


@dataclass
class WledState:
    on: bool | None = None
    brightness: int | None = None
    preset: int | None = None
    red: int | None = None
    green: int | None = None
    blue: int | None = None
    white: int | None = None
    cct: int | None = None
    nightlight: bool | None = None
    nightlight_duration: int | None = None
    nightlight_mode: int | None = None
    nightlight_target: int | None = None
    nightlight_remaining: int | None = None
    segment_present: bool = False
    segment_name: str | None = None
    segment_on: bool | None = None
    segment_brightness: int | None = None
    segment_reverse: bool | None = None
    segment_mirror: bool | None = None
    segment_freeze: bool | None = None
    segment_selected: bool | None = None
    palette: int | None = None
    effect: int | None = None
    effect_speed: int | None = None
    effect_intensity: int | None = None
    transition_ms: int | None = None
    playlist: int | None = None
    sync_send: bool | None = None
    sync_receive: bool | None = None
    sync_send_groups: int | None = None
    sync_receive_groups: int | None = None
    live_override: int | None = None
    main_segment: int | None = None
    effect_custom1: int | None = None
    effect_custom2: int | None = None
    effect_custom3: int | None = None
    effect_option1: bool | None = None
    effect_option2: bool | None = None
    effect_option3: bool | None = None
    segment_start: int | None = None
    segment_stop: int | None = None
    segment_grouping: int | None = None
    segment_spacing: int | None = None
    segment_offset: int | None = None
    matrix_start_y: int | None = None
    matrix_stop_y: int | None = None
    matrix_reverse_y: bool | None = None
    matrix_mirror_y: bool | None = None
    matrix_transpose: bool | None = None
    segment_set: int | None = None
    sound_simulation: int | None = None
    map_1d_2d: int | None = None
    secondary_color: tuple[int, ...] | None = None
    tertiary_color: tuple[int, ...] | None = None
    error: int | None = None
    ledmap: int | None = None
    live: bool | None = None
    live_segment: int | None = None
    live_mode: str | None = None
    wifi_rssi: int | None = None
    wifi_signal: int | None = None
    uptime: int | None = None
    free_heap: int | None = None
    build_id: int | None = None
    effect_count: int | None = None
    palette_count: int | None = None
    max_segments: int | None = None
    version: str | None = None
    node_count: int | None = None
    led_current_ma: int | None = None
    led_max_current_ma: int | None = None
    ip_address: str | None = None
    wifi_bssid: str | None = None
    wifi_channel: int | None = None
    led_count: int | None = None
    led_fps: int | None = None
    matrix_width: int | None = None
    matrix_height: int | None = None
    local_time: str | None = None
    fs_used_kb: int | None = None
    fs_total_kb: int | None = None
    presets_modified: int | None = None
    led_map_count: int | None = None
    realtime_source_ip: str | None = None
    websocket_clients: int | None = None
    has_rgbw: bool | None = None
    has_white: bool | None = None
    has_cct: bool | None = None
    is_matrix: bool = False


def parse_state(frame: str, segment_id: int = 0) -> WledState | None:
    try:
        root = json.loads(frame)
    except json.JSONDecodeError:
        return None

    state = root.get("state")
    if not isinstance(state, dict):
        return None

    for key in ("on",):
        if key in state and not isinstance(state[key], bool):
            return None
    for key in ("bri",):
        if key in state and not isinstance(state[key], int):
            return None
        if key in state and not 0 <= state[key] <= 255:
            return None
    if "ps" in state and (not isinstance(state["ps"], int) or not -1 <= state["ps"] <= 250):
        return None
    for key, minimum, maximum in (
        ("transition", 0, 655), ("pl", -1, 250), ("lor", 0, 2), ("mainseg", 0, 255),
        ("error", 0, 255), ("ledmap", 0, 15),
    ):
        if key in state and (not isinstance(state[key], int) or not minimum <= state[key] <= maximum):
            return None

    nightlight = state.get("nl")
    if nightlight is not None:
        if not isinstance(nightlight, dict):
            return None
        if "on" in nightlight and not isinstance(nightlight["on"], bool):
            return None
        for key, minimum, maximum in (("dur", 0, 255), ("mode", 0, 3), ("tbri", 0, 255), ("rem", -1, 15300)):
            if key in nightlight and (not isinstance(nightlight[key], int) or not minimum <= nightlight[key] <= maximum):
                return None

    sync = state.get("udpn")
    if sync is not None:
        if not isinstance(sync, dict):
            return None
        for key in ("send", "recv"):
            if key in sync and not isinstance(sync[key], bool):
                return None
        for key in ("sgrp", "rgrp"):
            if key in sync and (not isinstance(sync[key], int) or not 0 <= sync[key] <= 255):
                return None

    segments = state.get("seg", [])
    if not isinstance(segments, list):
        return None
    if any(not isinstance(item, dict) for item in segments):
        return None
    segment = next((item for index, item in enumerate(segments) if item.get("id", index) == segment_id), None)
    segment_object = segment or {}
    for key, maximum in (
        ("pal", 255), ("fx", 255), ("sx", 255), ("ix", 255), ("bri", 255),
        ("c1", 255), ("c2", 255), ("c3", 31), ("start", 65535), ("stop", 65535),
        ("grp", 255), ("spc", 255),
        ("startY", 65535), ("stopY", 65535), ("set", 3), ("si", 3), ("m12", 7),
    ):
        if key in segment_object and (not isinstance(segment_object[key], int) or not 0 <= segment_object[key] <= maximum):
            return None
    if "of" in segment_object and (not isinstance(segment_object["of"], int) or not -32768 <= segment_object["of"] <= 32767):
        return None
    if "cct" in segment_object and (
        not isinstance(segment_object["cct"], int)
        or not (0 <= segment_object["cct"] <= 255 or 1900 <= segment_object["cct"] <= 10091)
    ):
        return None
    for key in ("on", "rev", "mi", "frz", "sel", "o1", "o2", "o3", "rY", "mY", "tp"):
        if key in segment_object and not isinstance(segment_object[key], bool):
            return None
    if "n" in segment_object and not isinstance(segment_object["n"], str):
        return None

    color = segment_object.get("col")
    if color is not None:
        if not isinstance(color, list) or not 1 <= len(color) <= 3:
            return None
        for entry in color:
            if not isinstance(entry, list) or len(entry) not in (3, 4):
                return None
            if any(not isinstance(value, int) or not 0 <= value <= 255 for value in entry):
                return None

    info = root.get("info", {})
    if not isinstance(info, dict):
        return None
    wifi = info.get("wifi", {})
    leds = info.get("leds", {})
    if not isinstance(wifi, dict) or not isinstance(leds, dict):
        return None
    for key, minimum, maximum in (("rssi", -127, 0), ("signal", 0, 100)):
        if key in wifi and (not isinstance(wifi[key], int) or not minimum <= wifi[key] <= maximum):
            return None
    if "maxseg" in leds and (not isinstance(leds["maxseg"], int) or not 0 <= leds["maxseg"] <= 255):
        return None
    for key in ("count", "fps", "pwr", "maxpwr"):
        if key in leds and (not isinstance(leds[key], int) or not 0 <= leds[key] <= 65535):
            return None
    if "rgbw" in leds and not isinstance(leds["rgbw"], bool):
        return None
    if "wv" in leds and leds["wv"] not in (0, 2):
        return None
    if "cct" in leds and leds["cct"] not in (0, 4):
        return None
    matrix = leds.get("matrix", {})
    if not isinstance(matrix, dict):
        return None
    for key in ("w", "h"):
        if key in matrix and (not isinstance(matrix[key], int) or not 0 <= matrix[key] <= 65535):
            return None
    for key in ("vid", "uptime", "freeheap"):
        if key in info and (not isinstance(info[key], int) or not 0 <= info[key] <= 0xFFFFFFFF):
            return None
    for key in ("fxcount", "palcount"):
        if key in info and (not isinstance(info[key], int) or not 0 <= info[key] <= 65535):
            return None
    if "live" in info and not isinstance(info["live"], bool):
        return None
    if "liveseg" in info and (not isinstance(info["liveseg"], int) or not -1 <= info["liveseg"] <= 255):
        return None
    for key in ("lm", "ver"):
        if key in info and not isinstance(info[key], str):
            return None
    for key in ("ip", "time"):
        if key in info and not isinstance(info[key], str):
            return None
    if "ndc" in info and (not isinstance(info["ndc"], int) or not -1 <= info["ndc"] <= 150):
        return None
    for key in ("bssid",):
        if key in wifi and not isinstance(wifi[key], str):
            return None
    if "channel" in wifi and (not isinstance(wifi["channel"], int) or not 0 <= wifi["channel"] <= 255):
        return None
    filesystem = info.get("fs", {})
    if not isinstance(filesystem, dict):
        return None
    for key in ("u", "t", "pmt"):
        if key in filesystem and (not isinstance(filesystem[key], int) or not 0 <= filesystem[key] <= 0xFFFFFFFF):
            return None
    maps = info.get("maps", [])
    if not isinstance(maps, list) or any(not isinstance(entry, dict) for entry in maps):
        return None
    if "lip" in info and not isinstance(info["lip"], str):
        return None
    if "ws" in info and (not isinstance(info["ws"], int) or not -1 <= info["ws"] <= 32767):
        return None

    rgb = color[0] if color else []
    return WledState(
        on=state.get("on"),
        brightness=state.get("bri"),
        preset=state.get("ps"),
        red=rgb[0] if len(rgb) >= 3 else None,
        green=rgb[1] if len(rgb) >= 3 else None,
        blue=rgb[2] if len(rgb) >= 3 else None,
        white=rgb[3] if len(rgb) == 4 else None,
        cct=segment_object.get("cct"),
        nightlight=nightlight.get("on") if nightlight else None,
        nightlight_duration=nightlight.get("dur") if nightlight else None,
        nightlight_mode=nightlight.get("mode") if nightlight else None,
        nightlight_target=nightlight.get("tbri") if nightlight else None,
        nightlight_remaining=nightlight.get("rem") if nightlight else None,
        segment_present=segment is not None,
        segment_name=segment_object.get("n"),
        segment_on=segment_object.get("on"),
        segment_brightness=segment_object.get("bri"),
        segment_reverse=segment_object.get("rev"),
        segment_mirror=segment_object.get("mi"),
        segment_freeze=segment_object.get("frz"),
        segment_selected=segment_object.get("sel"),
        palette=segment_object.get("pal"),
        effect=segment_object.get("fx"),
        effect_speed=segment_object.get("sx"),
        effect_intensity=segment_object.get("ix"),
        transition_ms=min(state["transition"] * 100, 65535) if "transition" in state else None,
        playlist=state.get("pl"),
        sync_send=sync.get("send") if sync else None,
        sync_receive=sync.get("recv") if sync else None,
        sync_send_groups=sync.get("sgrp") if sync else None,
        sync_receive_groups=sync.get("rgrp") if sync else None,
        live_override=state.get("lor"),
        main_segment=state.get("mainseg"),
        effect_custom1=segment_object.get("c1"),
        effect_custom2=segment_object.get("c2"),
        effect_custom3=segment_object.get("c3"),
        effect_option1=segment_object.get("o1"),
        effect_option2=segment_object.get("o2"),
        effect_option3=segment_object.get("o3"),
        segment_start=segment_object.get("start"),
        segment_stop=segment_object.get("stop"),
        segment_grouping=segment_object.get("grp"),
        segment_spacing=segment_object.get("spc"),
        segment_offset=segment_object.get("of"),
        matrix_start_y=segment_object.get("startY"),
        matrix_stop_y=segment_object.get("stopY"),
        matrix_reverse_y=segment_object.get("rY"),
        matrix_mirror_y=segment_object.get("mY"),
        matrix_transpose=segment_object.get("tp"),
        segment_set=segment_object.get("set"),
        sound_simulation=segment_object.get("si"),
        map_1d_2d=segment_object.get("m12"),
        secondary_color=tuple(color[1]) if color and len(color) > 1 else None,
        tertiary_color=tuple(color[2]) if color and len(color) > 2 else None,
        error=state.get("error", 0),
        ledmap=state.get("ledmap"),
        live=info.get("live"),
        live_segment=info.get("liveseg"),
        live_mode=info.get("lm"),
        wifi_rssi=wifi.get("rssi"),
        wifi_signal=wifi.get("signal"),
        uptime=info.get("uptime"),
        free_heap=info.get("freeheap"),
        build_id=info.get("vid"),
        effect_count=info.get("fxcount"),
        palette_count=info.get("palcount"),
        max_segments=leds.get("maxseg"),
        version=info.get("ver"),
        node_count=info.get("ndc"),
        led_current_ma=leds.get("pwr"),
        led_max_current_ma=leds.get("maxpwr"),
        ip_address=info.get("ip"),
        wifi_bssid=wifi.get("bssid"),
        wifi_channel=wifi.get("channel"),
        led_count=leds.get("count"),
        led_fps=leds.get("fps"),
        matrix_width=matrix.get("w"),
        matrix_height=matrix.get("h"),
        local_time=info.get("time"),
        fs_used_kb=filesystem.get("u"),
        fs_total_kb=filesystem.get("t"),
        presets_modified=filesystem.get("pmt"),
        led_map_count=len(maps),
        realtime_source_ip=info.get("lip"),
        websocket_clients=info.get("ws"),
        has_rgbw=leds.get("rgbw"),
        has_white=bool(leds["wv"]) if "wv" in leds else None,
        has_cct=bool(leds["cct"]) if "cct" in leds else None,
        is_matrix="matrix" in leds,
    )


def scene_from_preset(preset: int) -> int | None:
    return preset - 1 if 1 <= preset <= 64 else None


class WledFrameParser:
    """Bounded serial framing model for the RP2040 receive loop."""

    def __init__(self, capacity: int = 8192, timeout_ms: int = 1000) -> None:
        self.capacity = capacity
        self.timeout_ms = timeout_ms
        self.buffer = ""
        self.depth = 0
        self.in_string = False
        self.escaped = False
        self.started_ms: int | None = None
        self.discarding = False
        self.resync_after_overrun = False
        self.json_errors = 0
        self.oversized_frames = 0
        self.uart_overruns = 0
        self.last_valid_response_ms: int | None = None

    def feed(self, data: str, now_ms: int, overrun: bool = False) -> list[WledState]:
        if self.depth and self.started_ms is not None and now_ms - self.started_ms >= self.timeout_ms:
            self.reset()
        if overrun:
            self.uart_overruns += 1
            self.reset()
            self.resync_after_overrun = True

        states: list[WledState] = []
        for character in data:
            if self.resync_after_overrun or self.discarding:
                if character in "\r\n":
                    self.reset()
                continue
            if character in "\r\n":
                if self.depth:
                    self.reset()
                continue
            if self.depth == 0:
                if character != "{":
                    continue
                self.buffer = ""
                self.depth = 0
                self.in_string = False
                self.escaped = False
                self.started_ms = now_ms
            if len(self.buffer) >= self.capacity - 1:
                self.oversized_frames += 1
                self.discarding = True
                continue
            self.buffer += character
            if self.in_string:
                if self.escaped:
                    self.escaped = False
                elif character == "\\":
                    self.escaped = True
                elif character == '"':
                    self.in_string = False
            elif character == '"':
                self.in_string = True
            elif character == "{":
                self.depth += 1
            elif character == "}":
                self.depth -= 1
                if self.depth == 0:
                    state = parse_state(self.buffer)
                    if state is not None:
                        states.append(state)
                        self.last_valid_response_ms = now_ms
                    else:
                        try:
                            json.loads(self.buffer)
                        except json.JSONDecodeError:
                            self.json_errors += 1
                    self.reset()
        return states

    def online(self, now_ms: int, timeout_seconds: int) -> bool:
        if self.last_valid_response_ms is None:
            return False
        return timeout_seconds == 0 or now_ms - self.last_valid_response_ms < timeout_seconds * 1000

    def reset(self) -> None:
        self.buffer = ""
        self.depth = 0
        self.in_string = False
        self.escaped = False
        self.started_ms = None
        self.discarding = False
        self.resync_after_overrun = False
