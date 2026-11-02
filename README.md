# OAM Digital LED Controller

KNX TP OpenKNX/OAM actuator for WLED-compatible digital LED controllers.

## Architecture

The firmware follows the OpenKNX lifecycle used by the reference
`KNX_Universal_Digital_LED_Controller` project:

`src/main.cpp` registers `DigitalLedModule` -> the module maps KNX group
objects -> WLED JSON commands are sent over a dedicated UART.

The first implementation targets an RP2040 board with a KNX TP UART and a
second UART connected to WLED. It supports switching, absolute and relative
brightness, RGB/RGBW and HSV color, white, tunable/warm/cold white,
scenes/presets, palette, effect, speed and intensity. Color channels and the
effect index can also be adjusted with relative dimming objects. ETS can
dynamically expose segment controls (on, brightness, reverse, mirror, freeze
and selection), UDP sync controls, transition timing, live override, main
segment, playlist status/advance, explicit state requests and WLED reboot.
Optional ETS groups additionally expose effect-specific sliders/options,
segment geometry, secondary and tertiary RGBW colors, a five-entry fixed
playlist, armed Preset save/delete operations, realtime status, WLED health
and capability telemetry, LED maps and effect timebase control. Version 0.5
adds runtime nightlight parameters, protected segment topology operations,
playlist stop/active feedback, Preset-active feedback, UDP node count,
estimated LED current and current-limit feedback, packed IPv4/BSSID/channel
status, LED/FPS/matrix telemetry, and WLED local date/time exchange.
Version 0.6 uses the remaining Digital LED object range through 409 for three
additional statically commissioned segments, 2D matrix controls, filesystem
and LED-map telemetry, realtime source status, serial communication health,
current-limit warnings, and LED capability feedback.

## Build

The project expects the OpenKNX dependency repositories in `lib/` as directory
links, matching the OAM workflow. Restore those dependencies first, then run:

```powershell
.\scripts\Restore-Dependencies.ps1
```

```text
pio run -e develop_OAM_DigitalLEDController
```

## ETS/OpenKNX XML workflow

The product definition is maintained by OpenKNXproducer XML, not Kaenx-Creator:

- `src/DigitalLEDController.xml`: development product entry point
- `src/DigitalLEDController-Release.xml`: release product entry point
- `src/DigitalLEDController.conf.xml`: version and module configuration
- `lib/OFM-DigitalLEDController/src/DigitalLedModule.share.xml`: LED objects and parameters
- `lib/OFM-DigitalLEDController/src/DigitalLedModule.templ.xml`: channel UI and object references

The product also includes ConfigTransfer, FunctionBlocks, LogicModule and
VirtualButton as reusable OpenKNX modules.

Generate the ETS product and firmware header with:

```powershell
.\scripts\Generate-KnxProduct.ps1 -Producer C:\path\to\OpenKNXproducer-x64.exe
```

Use `-Configuration Release` for the release XML. The generated product is
written to `outputs/`; `include/knxprod.h` is the firmware interface generated
from the same XML source.

CI validates both development and Release products and uploads the generated
`.knxprod`, `knxprod.h`, UF2 firmware and host-test report as workflow artifacts.

## WLED protocol

Commands use the WLED serial JSON API and end with CR/LF. State polling sends
`{"v":true}` periodically. Incoming frames are bounded and parsed only after
one complete JSON object is received; malformed or oversized frames are
discarded without blocking the KNX service loop. Segment commands include the
ETS-configured WLED segment ID, while state feedback is taken from that same
segment. ETS exposes transition time in milliseconds and converts WLED's
`transition`/`tt` values to their native 100 ms units.

Playlist save and Preset save/delete objects require a separate arm telegram
and automatically disarm after one operation or 30 seconds. Fixed playlists
are saved into the configured WLED Preset slot before they are started. WLED build-dependent
effect options and capability values are reported but are not treated as
portable configuration across WLED versions.

Segment deletion has a separate 30-second arm object and automatically disarms
after one delete command. Applying segment geometry requires `stop > start`;
the configured segment name is included so WLED does not clear it while bounds
change. The IPv4 feedback is packed into DPT 12.001 as `0xAABBCCDD`, and BSSID
feedback is a 12-character hexadecimal string without separators.

The serial state/info response does not expose the connected SSID, current
Preset name, Playlist item/progress, timer status, measured watts, or per-node
online state. Those values are intentionally not synthesized. Configuration
only `/json/cfg` fields are also excluded because WLED does not treat that API
as stable runtime state.

Additional segments use six bidirectional objects each: on, brightness, CCT,
RGBW, options and name. The options byte uses bits 0..3 for selected, reverse,
mirror and freeze; feedback bit 7 indicates that the configured segment ID was
present in the latest WLED response. The three ETS segment IDs must be unique
and different from the primary segment ID.

Communication health is local to the KNX controller. A syntactically and
semantically valid WLED state response refreshes the online timeout. Separate
counters report malformed JSON, frames larger than the 8 KB serial buffer and
UART FIFO overruns. Current warning feedback is derived from WLED's estimated
`leds.pwr` and configured `leds.maxpwr`; it is not an electrical measurement or
a safety cutoff.

Hardware wiring, burn-in commands and the KNX Group Address test table are in
[Chinese](docs/HARDWARE_INTEGRATION.zh-CN.md) and [English](docs/HARDWARE_INTEGRATION.en.md).
