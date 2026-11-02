# OAM Digital LED Controller 0.6.0 发布说明

发布日期：2026-11-02

## 发布概述

0.6.0 完成了 WLED JSON API 与 KNX TP Digital LED 模块的高级对象扩展。
Digital LED 通信对象范围现为 201–409，FunctionBlocks 继续从对象 410
开始，两个模块的对象空间没有重叠。

本版本以 WLED 官方 `main` 源码提交
`fce41f2655c3087d3eaf95c028d2856967cbc86e` 为协议依据，只映射可从
WLED 串口 `state`/`info` 回复中稳定验证的字段。

## 主要新增功能

### ETS 兼容性

- 布尔参数使用 ETS 6.4.1 接受的 `TypeNumber UIHint="CheckBox"` 表达。
- Development 和 Release `.knxprod` 均不包含非法的
  `TypeRestriction UIHint="CheckBox"`。

### 多 Segment

- 增加三个可配置附加 Segment。
- 每个附加 Segment 提供开关、亮度、CCT、RGBW、Options 和名称。
- Options bit 0–3 分别表示 selected、reverse、mirror 和 freeze。
- Options bit 7 是只读存在状态，表示该 Segment 出现在最新 WLED 回复中。
- 三个附加 Segment ID 必须彼此不同，并与主 Segment ID 不同。

### 2D Matrix

- Y 起止坐标。
- Y 反向、Y 镜像和转置。
- Segment set、Sound Simulation 和 1D-to-2D 映射模式。

### 文件系统与 Realtime

- 文件系统已用/总容量。
- Preset 文件修改时间。
- LED Map 数量。
- Realtime 来源 IP。
- WebSocket 客户端数量。

### 通信健康

- WLED 在线状态和最后有效回复年龄。
- JSON 反序列化错误计数。
- 超过 8 KB 的串口帧计数。
- UART FIFO overflow 计数。
- 可配置离线超时；0 表示收到首次有效回复后不再因超时离线。

### 电流和能力状态

- WLED 估算电流占配置上限的百分比。
- 可配置电流警告阈值、警告状态和超限状态。
- RGBW、白光、CCT 和 Matrix 能力反馈。

## 同时包含的既有扩展

- 完整颜色、CCT、效果、Palette、Preset、Playlist 和 Nightlight 控制。
- Segment 布局、名称、第二/第三颜色及受保护删除。
- UDP Sync、Realtime、LED Map 和 Timebase。
- WLED 网络、LED、Matrix、时间及运行状态反馈。
- Preset 保存/删除和 Segment 删除均使用独立 30 秒解锁。

## 发布文件

- `OAM-DigitalLEDController-0.6.0.knxprod`：ETS Release 产品。
- `OAM-DigitalLEDController-0.6.0.uf2`：RP2040 Release 固件。
- `knxprod.h`：与 Release KNX 产品对应的生成接口。
- `TEST_REPORT.zh-CN.md`：完整软件验证报告。
- `test-results.txt`：54 项主机测试原始输出。
- `SHA256SUMS.txt`：发布文件 SHA-256 校验值。

## 安装

1. 在 ETS 中导入 `.knxprod` 文件。
2. 将 `.uf2` 文件写入目标 RP2040 控制器。
3. 在 ETS 中配置主 Segment、可选对象组和附加 Segment ID。
4. 通过独立 UART 将控制器连接到运行兼容 WLED 固件的设备。
5. 下载 KNX 应用程序后检查 WLED 在线对象和错误计数。

接线和总线联调参考：

- `HARDWARE_INTEGRATION.zh-CN.md`
- `HARDWARE_INTEGRATION.en.md`

## 已知限制

- 本发布没有在真实 DigitalLedController、KNX TP 总线或 WLED 硬件上验证。
- WLED 串口回复不提供 SSID、Preset 名称、Playlist 项目进度、定时器状态或
  实测功率，因此不会伪造这些反馈。
- `info.leds.pwr` 是 WLED 估算电流，不是电气测量值；电流警告不能作为安全
  断电机制。
- Segment 名称受 KNX DPT 16 限制，最多发送 14 个 ASCII 字符。
- Digital LED 对象空间已使用到 409；新增对象必须重新规划模块对象范围或
  调整现有映射。

## 源码状态

此发布包由 Git 基线 `12f01a5` 加尚未提交的 ETS 6.4.1 兼容性修复构建。
正式打标签或公开发布前应先提交修复源码，并再次执行完整 CI。
