# OAM Digital LED Controller 0.6.0 完整测试报告

测试日期：2026-11-02

测试平台：Windows / RP2040 PlatformIO

源码基线：`12f01a5`（包含尚未提交的 ETS 6.4.1 兼容性修复）

## 1. 测试环境

| 项目 | 版本或配置 |
|---|---|
| Python | 3.14.8 |
| PlatformIO Core | 6.2.0 |
| OpenKNXproducer | 4.3.12 |
| 固件环境 | `release_OAM_DigitalLEDController` |
| MCU | RP2040，256 KB RAM，16 MB Flash |
| ETS ApplicationVersion | 0.6 |
| Digital LED 模块版本 | 0.6.0 |
| WLED 协议基准 | `fce41f2655c3087d3eaf95c028d2856967cbc86e` |

## 2. 主机自动测试

执行命令：

```powershell
python -m unittest discover -s test -p "test_*.py" -v
```

结果：**54/54 通过，0 失败，0 错误。**

| 测试文件 | 数量 | 覆盖范围 |
|---|---:|---|
| `test_knx_simulation.py` | 28 | KNX 对象到 WLED JSON 命令、位图、Segment、Matrix 和扩展信息 |
| `test_wled_protocol.py` | 12 | WLED `state`/`info` 字段、边界、串口恢复和通信健康 |
| `test_wled_protocol_fuzz.py` | 7 | 随机分片、损坏、截断、超长帧、噪声和 UART overflow |
| `test_ets_xml.py` | 4 | ETS 参数、DPT、动态可见性、209 个对象和 409/410 边界 |
| `test_firmware_startup.py` | 3 | 固件修订版、Flash 模块 ID 和 OpenKNX 启动顺序 |

### 模糊测试规模

- 250 组随机有效 WLED 状态和随机串口分片。
- 500 组随机噪声后的同步恢复。
- 500 组随机损坏 JSON 后的同步恢复。
- 30 个不同位置的截断超时。
- 100 次连续 UART overflow 和恢复。
- 8 KB 缓冲区边界、JSON 错误分类和语义错误分类。

总计覆盖超过 1,380 组确定性异常输入场景，随机种子固定，可重复执行。

## 3. ETS/OpenKNX 产品验证

执行命令：

```powershell
.\scripts\Generate-KnxProduct.ps1 `
  -Producer .tools\OpenKNXproducer\tools\Windows\OpenKNXproducer-x64.exe `
  -Configuration Release
```

结果：**成功。**

OpenKNXproducer 完整性检查全部通过，包括：

- ID 唯一性和命名空间。
- ParameterRef、ComObjectRef 和 AllocatorRef 完整性。
- 参数类型、Union 内存和参数值。
- 通信对象编号、大小和 DPT 引用。
- Baggage、脚本方法和模块版本。
- Application data 和 Memory size。

关键检查：

- ETS ApplicationVersion：6。
- 参数数量：75。
- Digital LED 通信对象数量：209。
- Digital LED 全局对象范围：201–409。
- FunctionBlocks 起始偏移：410。
- 对象 410 未被 Digital LED 模块使用。
- Development 与 Release XML 均能生成产品。
- 两个 `.knxprod` 内均不存在非法的
  `TypeRestriction UIHint="CheckBox"`。
- 两个 `.knxprod` 内的 Digital LED 布尔类型均使用 ETS 6.4.1 接受的
  `TypeNumber UIHint="CheckBox"`。

## 4. Release 固件构建

执行命令：

```powershell
pio run -e release_OAM_DigitalLEDController
```

结果：**成功。**

| 资源 | 使用量 |
|---|---:|
| RAM | 65,980 / 262,144 字节（25.2%） |
| 应用 Flash | 472,856 / 1,458,176 字节（32.4%） |
| 固件分区 | 484,892 字节 |
| KNX DATA | 18,732 / 32,768 字节 |
| KNX FREE | 14,036 字节 |

固件元数据：

- OpenKnxId：`0xAF`
- ApplicationNumber：`0x01`
- ApplicationVersion：`0x06`
- FirmwareRevision：`0x00`

## 5. 启动流程验证

已验证以下调用顺序：

```text
Serial.begin
→ openknx.init(firmwareRevision)
→ addModule(Digital LED)
→ addModule(FunctionBlocks)
→ addModule(Logic)
→ addModule(VirtualButton)
→ openknx.setup
→ openknx.loop
```

Flash 模块 ID 1–4 与生成的 `ETS_ModuleId_*` 编号相互独立。运行时和 UF2
现在均使用 FirmwareRevision 0。

## 6. 未执行的测试

由于没有 DigitalLedController 实物，本次未执行：

- KNX TP 物理层发送和接收。
- ETS 实际下载及 Group Address 总线联调。
- RP2040 与 WLED 设备之间的 UART 电气连接。
- 多 Segment 和 2D Matrix 的真实 LED 显示验证。
- UART FIFO overflow 的真实硬件注入。
- 电源故障、掉电保存和长期烧机。
- WLED 不同版本及不同 MCU 构建的兼容性矩阵。

这些项目是正式硬件发布前仍需完成的验证，不应由主机模拟测试替代。

## 7. 结论

0.6.0 已通过协议模型、模糊输入、KNX 映射、ETS XML、启动流程、
OpenKNXproducer 和 RP2040 Release 编译验证。未发现阻止软件发布包生成的
问题。

由于缺少实体硬件，本构建应视为“软件验证完成、硬件验证待完成”的发布候选。
