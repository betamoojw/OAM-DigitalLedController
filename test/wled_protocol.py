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
    palette: int | None = None
    effect: int | None = None
    effect_speed: int | None = None
    effect_intensity: int | None = None


def parse_state(frame: str) -> WledState | None:
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

    nightlight = state.get("nl")
    if nightlight is not None:
        if not isinstance(nightlight, dict):
            return None
        if "on" in nightlight and not isinstance(nightlight["on"], bool):
            return None
        for key, minimum, maximum in (("dur", 1, 255), ("mode", 0, 3), ("tbri", 0, 255)):
            if key in nightlight and (not isinstance(nightlight[key], int) or not minimum <= nightlight[key] <= maximum):
                return None

    segments = state.get("seg", [])
    if not isinstance(segments, list):
        return None
    segment = segments[0] if segments else {}
    if not isinstance(segment, dict):
        return None
    for key in ("pal", "fx", "sx", "ix", "cct"):
        if key in segment and (not isinstance(segment[key], int) or not 0 <= segment[key] <= 255):
            return None

    color = segment.get("col")
    if color is not None:
        if not isinstance(color, list) or not color or not isinstance(color[0], list) or len(color[0]) not in (3, 4):
            return None
        if any(not isinstance(value, int) or not 0 <= value <= 255 for value in color[0]):
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
        cct=segment.get("cct"),
        nightlight=nightlight.get("on") if nightlight else None,
        palette=segment.get("pal"),
        effect=segment.get("fx"),
        effect_speed=segment.get("sx"),
        effect_intensity=segment.get("ix"),
    )


def scene_from_preset(preset: int) -> int | None:
    return preset - 1 if 1 <= preset <= 64 else None


class WledFrameParser:
    """Bounded serial framing model for the RP2040 receive loop."""

    def __init__(self, capacity: int = 1024, timeout_ms: int = 1000) -> None:
        self.capacity = capacity
        self.timeout_ms = timeout_ms
        self.buffer = ""
        self.depth = 0
        self.in_string = False
        self.escaped = False
        self.started_ms: int | None = None
        self.discarding = False
        self.resync_after_overrun = False

    def feed(self, data: str, now_ms: int, overrun: bool = False) -> list[WledState]:
        if self.depth and self.started_ms is not None and now_ms - self.started_ms >= self.timeout_ms:
            self.reset()
        if overrun:
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
                    self.reset()
        return states

    def reset(self) -> None:
        self.buffer = ""
        self.depth = 0
        self.in_string = False
        self.escaped = False
        self.started_ms = None
        self.discarding = False
        self.resync_after_overrun = False
