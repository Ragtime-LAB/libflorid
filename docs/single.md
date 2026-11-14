# UnderwaterSingle：四电机、两种连接、同一 Python API

```python
from pyflorid import UnderwaterSingle

# 替换为实际枚举的 USB URI；串口改为 serial:///dev/ttyUSB0 或 serial://COM3。
with UnderwaterSingle.connect("usb://2fe3:574c/SERIAL") as arm:
    print(arm.info())             # 硬件 UID、boot_id、主／从角色、固件版本
    print(arm.read_state())       # 本地最新状态；尚未交付则 None，不触发运动
    print(arm.settings())         # 三轴重力比例、revision、storage_error
    arm.disable()                # 显式请求 Damp；不是等待电机失能完成
```

`UnderwaterSingle` 是 Python 产品入口：USB 后端由 C++ Wirelink endpoint 提供，
串口后端由 Python 单 owner 线程提供。本次未增加 C++ 串口产品 API。
连接／查询不进入 Control；关闭不发 Damp，也不会自动重连。

## 显式启动实时控制

q／dq／kp／kd／tau 五个数组均按 `[j1,j2,j3,gripper]` 排列，
单位依次为 rad、rad/s、Nm/rad、Nm/(rad/s)、Nm。
`kind=1` 为纯 MIT，`kind=2` 为 MIT＋固件模型重力（仅三关节）。

```python
# arm 已连接，state 是已经读到并核对过的四电机反馈。
# read_state() 尚未交付时返回 None；先继续读取，再执行本段。
arm.start_control(state.q, [0]*4, [0]*4, [0]*4, [0]*4, kind=2)
# 已进入新会话，由 SDK 独立线程每 4 ms 提交最新目标。
arm.update_command(state.q, [0]*4, [0]*4, [0]*4, [0]*4, kind=2)
# 后续程序持续更新目标并检查反馈；结束时显式失能。
arm.disable()
```

`start_control` 每次显式请求一个新会话，`update_command` 只更新本地活动会话目标，
不会隐式进入模式。SDK 不追赶调度暂停期间的历史命令。`stop_sending()` 只停本地发送，
Control 有效命令间隔超过 100 ms 后由固件退出 Damp；恢复需显式重新 start_control。
`drag()` 为重力＋摩擦拖动模式。正常可请求模式只有 0 Damp、1 Drag、3 Control。

`read_state()` 的四元素数组包含 q、dq、tau、温度、原始状态码和反馈年龄；同时报告 mode、
source、session、sequence、applied_sequence、fresh/enabled/available/applied_mask、stop_reason、
reject_reason、enabling。applied 表示提交 CAN，不是到位确认。无反馈年龄为 UINT32_MAX。
`status()` 发起只读 RPC；其 disabled_mask／disable_pending_mask 用于观察失能确认。

## 会话、错误与设置

- source：0 None、1 USB、2 UART7；进入 Control 的链路独占会话。另一链路可读状态／设置及 Damp，
  不能 Drag／抢占 Control。跨源模式请求 Busy，命令拒绝 Source=7。切换前先 Damp。
- 业务拒绝抛 `DeviceRejected`，提供 `result`、`reason` 和实际 `snapshot`；超时抛传输异常，
  不自动重试。超时不代表请求未执行，先查询实际状态／设置。
- `settings()` 只读，`settings([s1,s2,s3])` 应用并保存三个有限 float。成功返回实际活动值；
  Flash 在独立工作线程处理，不阻塞实时链路。revision 是启动内写入版本，storage_error 为 Zephyr errno。
  失败保留活动值，异常 snapshot 提供实际设置。两条链路共用存储，无隐式模式切换。
- `tx_count` 是命令提交数，`rx_count` 是 SDK 实际交付状态数；GUI 约 30 Hz 读取缓存，不冒充 250 Hz。
  `last_error` 报告本地发送失败；控制仍须监控状态的新鲜度、停止原因和电机位图。

## 传输契约

USB 在原设备管理／升级 endpoint 上增加：

| 消息 | ID | 用途 |
|---|---:|---|
| SingleRequest | 30209 | managed RPC 请求，operation=0 状态／身份、1 模式、2 读设置、3 保存设置 |
| SingleResponse | 30210 | result、四电机 state、UID、boot_id、role、实际设置、失能位图和 reason |
| SingleCommand | 30211 | latest 四电机 MIT 命令 |
| SingleState | 30212 | latest 250 Hz 状态 |

权威源为 `protocol/schema/wirelink/single/`，由 device profile 组合；原双臂十电机消息未改布局。
UART 独立 Protocol 3.1 使用 DYNAMIXEL 风格帧头／填充／CRC16，A1 命令仍 102 B，A2 状态仍120 B。
新增 A6 身份44 B、A7/A8 设置20 B。USB 和 UART 报告相同硬件 UID 和本次 boot_id。
完整串口字段手册位于固件 `firmware/apps/underwater_single/SERIAL_PROTOCOL_MANUAL.md`／`.pdf`。

USB 支持 `start_upload(image_bytes)`、`firmware_progress()`、`cancel_upload()` 和
`reboot_and_wait(Version)`，复用原升级生命周期及失能确认；重启验证成功后刷新身份。
UART 不支持升级。操作界面与启动方式见 [控制台说明](../tools/underwater_single_console/README.md)。


## 固件 0.4.0：本地校零与掉线报警

Protocol 3.2 不改四电机帧长，新增 `mode=4 Zeroing` 和 `stop_reason=5 ZeroingFailed`。
PA15 长按至少 1 秒后松开进入 Zeroing（可结束 USB／UART Control 会话），短按才保存四电机
硬件零点；200 ms 等待、最多 3 次重试后成功或失败均回 Damp。期间只有零 MIT，需承托机械臂，
保持 URDF 零姿态直到流程结束。校零中正常 Drag／Control 请求 Busy，Damp 可取消。
SDK 不增加远程校零方法，也不自动恢复旧会话。重新控制前核对零位和新目标。

GUI 显示校零操作提示、失败原因，并提醒用“读取当前角度到目标”更新零位后的输入；
单个 Damp 状态不代表一定校零成功，成功／取消详情看 USART1 日志。失败可能已修改部分电机零点。
报警监测四电机反馈：曾回复后超过100 ms掉线即双短音，从未回复者启动满5 s后报警；
全部恢复立即静音。它不改变已有模式、恢复或升级规则。
旧 0.3.0 / Protocol 3.1 固件仍可使用原 USB／UART 控制与设置 API。
