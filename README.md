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
effect index can also be adjusted with relative dimming objects.

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
discarded without blocking the KNX service loop.

Hardware wiring, burn-in commands and the KNX Group Address test table are in
[Chinese](docs/HARDWARE_INTEGRATION.zh-CN.md) and [English](docs/HARDWARE_INTEGRATION.en.md).
