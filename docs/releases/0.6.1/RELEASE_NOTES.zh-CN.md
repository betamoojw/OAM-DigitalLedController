# OAM Digital LED Controller 0.6.1 发布说明

## 变更

- 将 LED 模块的 ETS 参数标签、帮助文本和通信对象名称统一为英文，区分开关状态和亮度状态。保留原始德语 XML；它们不作为单独的 ETS 语言资源加载。
- 保留上一版的 ETS 6.4.1 导入兼容性修复。通信对象编号、DPT、参数布局和 ETS 默认语言均未改变。
- 固件修订号升至 `1`，ETS 应用版本仍为 `0.6`。OpenKNX 使用 ETS 应用版本和固件修订号组成发布版本 `0.6.1`。

## 发布文件

- `OAM-DigitalLEDController-0.6.1.knxprod`：ETS 产品文件。
- `OAM-DigitalLEDController-0.6.1.uf2`：RP2040 Release 固件。
- `SHA256SUMS.txt`：包内文件的 SHA-256 校验值。

硬件接线和调试步骤见[硬件集成手册](../../HARDWARE_INTEGRATION.zh-CN.md)。
