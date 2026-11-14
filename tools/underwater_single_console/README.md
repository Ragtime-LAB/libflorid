# Underwater Single 单臂控制台

一个窗口连接一台三关节＋夹爪设备，USB Bulk 与 UART7／RS232 都可独立控制。
主／从臂由固件报告：主臂蓝色、从臂绿色。需要固件 **0.3.0 / Protocol 3.1**。
旧双臂控制台 `tools/underwater_console` 保留，不用于本单臂产品。

## 构建并启动

```sh
cd ~/codings/libflorid-underwater-single
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_PYFLORID=ON -DBUILD_TESTS=ON -DLF_ENABLE_WLC=OFF
cmake --build build -j4
python3 -m pip install 'pyserial>=3.5'
python3 tools/underwater_single_console/run.py
```

Python 需要 Tk（Linux 通常由发行版 `python3-tk` 提供）。正常构建使用已提交的生成快照，
不需要安装 WLC／Rust。`run.py` 优先加载本工作区 `build/pyflorid`，其次
`build-underwater-sdk/pyflorid`，否则使用已安装的 pyflorid。USB 需要对应设备权限；
UART 需要串口设备访问权限。

## 从只读反馈开始

1. 选择 USB Bulk 或 UART7 / RS232，点击搜索；搜索只枚举设备，不探测或使能电机。
   也可输入完整 USB URI，或串口路径 `/dev/ttyUSB0`／`COM3`。
2. 连接后核对 UID、固件版本、主／从角色以及四个电机的反馈。顺序为
   `joint1、joint2、joint3、夹爪`，CAN ID 为 `1、2、3、4`。
3. 拖动模式显式请求 Drag，手动控制显式请求 Control。初始 Kp、Kd、外加力矩均为零，
   默认 MIT＋重力。未编辑目标角度首次从新鲜反馈填入；“读取当前角度到目标”可重新填入。
4. 输入有限数值，按 Enter 或离开输入框应用；Kp／Kd 不得为负。错误字段标红且整组目标不应用。
   滑条仅为编辑参考，数值输入不自动裁剪。四电机中仅前三关节叠加模型重力。
5. 停机点击始终可见的红色“失能 · Damp”。查看实际 mode、位图及失能反馈，不能把请求成功
   当成电机执行确认。

小窗口内主体可纵／横向滚动，连接与模式按钮固定在顶部。反馈界面约 30 Hz 刷新，
命令由独立线程以 250 Hz 提交，不依赖 GUI 刷新；状态同样目标 250 Hz。
界面显示的是 SDK 命令提交／状态交付率，不能替代设备采样率或线路抖动测量。

**断开／关闭不主动发送 Damp**：本地立即停止命令生产，Control 由固件在有效命令间隔
超过 100 ms 后退出并请求失能；Drag 保留模式。需要立即请求失能时，请先点击 Damp 再断开。
无自动重连、补发历史轨迹或恢复 Control。

## 两条链路不抢占运动

进入 Control 的链路持有该次会话，状态报告 `source=1 USB` 或 `source=2 UART7`。
另一链路可查询或请求 Damp，不能用 Drag／Control 抢占，也不能覆盖实时命令；拒绝提示
“先 Damp 再切换”。USB 拔线不影响 UART 所有者。切换入口先 Damp，再在新链路显式进入 Control。

UART 固定 **921600、8N1、无流控**。板端为 PE8 TX／PE7 RX 的 MCU 电平；接 RS232 时必须
使用支持 921600 的电平转换器，不能将 RS232 电压直接接板端。

## 设置与 USB 升级

- 设置页只有三关节 gravity_scale。读取只读；“填入 1.0”只改输入框；“应用并保存”才写入。
  保存不隐式切换模式，独立存储线程写 NVS 后返回实际值、启动内 revision 和 storage_error。
  保存期间命令流继续；写失败保留原活动值。摩擦、零位、电机 ID 和夹爪重力不在此接口。
- 升级页只接受 USB。选择对应角色的 `zephyr.signed.bin`，上传到 secondary slot，查看设备确认的
  字节进度，随后显式“重启并确认版本”。普通 bin 和 MCUboot bootloader 不是升级镜像。
  固件沿用 Damp／新失能反馈确认和升级占用流程，结束不会恢复控制。
- UART 下升级／重启会明确提示改用 USB；不会悄悄切换连接。

SDK 接口与契约见 [单臂 SDK 文档](../../docs/single.md)。

## 验证

```sh
ctest --test-dir build --output-on-failure
python3 tools/underwater_single_console/tests/test_console.py -v
python3 tools/underwater_single_console/run.py --smoke
```

本次无 WLC 快照构建及 8 项 CTest 通过（含原双臂回归、四电机 USB endpoint 和 8 项 Python
模拟链路检查）；另有 3 项 Tk 交互测试通过，已检查实际窗口与滚动布局。
模拟串口覆盖等待保存时持续发命令、Damp 越过慢保存、回执超时不自动重试、源互斥及关闭停发。
尚未烧录，USB／UART7／RS232 实物 250 Hz、CAN 输出及真实 MCUboot 更新／回退需台架验收。


## 本地校零与掉线提示（固件 0.4.0 / Protocol 3.2）

承托机械臂，长按 PA15 至少 1 秒后松开进入 Zeroing，主臂蓝／从臂绿快闪；
摆到 URDF 零姿态后短按保存四电机零点。校零期间无重力／摩擦／PD，保持姿态直到结束。
GUI 显示步骤及失败原因；Damp 可取消，正常 Drag／Control 会由固件返回 Busy。
成功或重试耗尽均回 Damp，不自动恢复控制；检查 USART1 结果及零位后，可点击
“读取当前角度到目标”，再显式进入新 Control 会话。新功能不影响原 0.3.0 固件连接。

掉线蜂鸣器为 PB15／TIM12_CH2：曾回复电机超过100 ms无反馈即双短音，从未回复者启动满5 s
报警，全部恢复静音；Damp 下失能反馈也算正常。蜂鸣不改变控制模式。
