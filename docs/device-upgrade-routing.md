# 共享产品路由、RAM 验证与 MCUboot 接入

前半部分保留首次 RAM HIL 的历史结果；当前 MCUboot 实现见文末“MCUboot 接入”。

2026-09-10。FCI `device` 组合 arm revision 8 和 upgrade v1，不改变已有消息 ID、
arm managed RPC 线格式或 upgrade mapped RPC 字段。下述路由实机验证使用的编译器为 WLC
`18b830af2cdd535bdfc6e2bbd3f147f9a9a4ce29`（0.7.0-dev / ABI 32），
Wirelink 为 `e180aa846893735f90d516271a5dedb599cb9264`（包含实机发现的 USB RX
短尾部修复，ABI 32 不变）。

`FciWirelinkEndpoint` 持有唯一 `fci_device_endpoint_t`、USB adapter 和 executor。
`FciUpgradeClient` 只注册服务和 direct BulkStatus 回调，借用该 endpoint；没有自己的
链路、USB 连接或线程。mapped RPC 使用同一个 RPC client 的自动 ID 分配器，避免与
managed arm 调用编号碰撞。产品配置为 2048 字节 payload、8192 字节 RX FIFO、10 个
RPC 槽；SDK arm 队列仍为 8（7 个公开调用加续租保留任务），为升级管理留下容量。

## 入口

已有 Arm 时，先停止控制循环，再取得共享视图；视图保持原会话存活，不会第二次打开 USB：

```cpp
arm->stop();
auto updater = arm->firmwareUpdater();
auto boot = updater.bootStatus();
auto admitted = updater.startUpload(image_bytes);
auto progress = updater.progress();
updater.cancel(); // 异步；等待 progress.active() == false 后再开始下一次
```

恢复工具可以直接连接，不调用 `Arm::create()`、不获取控制租约、不依赖遥测：

```cpp
auto updater = florid::FirmwareUpdater::create("usb://2fe3:574c/SERIAL");
if (updater) {
    auto boot = updater->bootStatus();
    // boot.m_max_chunk_size == 0 表示该构建没有可写入的镜像 sink。
}
```

`startUpload()` 复制输入；`progress()` 返回线程安全快照。RPC、CRC/bulk 状态和 direct
发送均由已有 owner 推进，业务线程不接触 Wirelink 上下文。输入按 FCI v1 application /
secondary / flags=0 发起，版本元数据当前为零；本轮尚不解析 MCUboot 镜像头。
调用方应检查入队结果和最终状态，不能把 `startUpload()` 返回成功当成上传完成。

取消已发出的 StartUpgrade 时，客户端先等到响应以取得 transfer ID，再发送 Abort；
接收端也接受 Begin 之前的 Abort。Start 响应丢失到整体超时时，客户端无法知道 ID，
设备侧依靠 reservation/receiver idle timeout 清理；不要把本地 timeout 当成远端取消确认。
USB 断线、peer session 改变和关闭会终止本地任务并释放 mapped RPC 槽。重新打开使用
新会话，不自动重放旧镜像。当前不支持跨会话断点续传。

## 首次 RAM HIL 的边界

- 完成路由、RAM sink 验证和升级模式互斥；没有 Flash 写入、MCUboot swap、签名验证、
  试启动确认或真正 Reboot。
- Willow 默认无写入 sink：GetBootStatus 可用，StartUpgrade/Reboot 返回 UNSUPPORTED。
  BootStatus 的 slot 信息为 NONE/0，不伪装成 MCUboot 的实际槽状态。
- HIL 使用 64 KiB RAM、2031 字节 chunk 和 CRC32C 校验，不能作为生产固件升级器。
  上传 `Completed` 仅表示 sink 接收并验证完成，不表示镜像已安装或可启动。
- firmware upgrade mode 会撤销租约；需要重新取得控制权限，不自动恢复运动。

## 验证

- FCI schema/严格 C11/C++ 组合：12/12。
- libflorid Debug（WLC ON）、Release（snapshot OFF）、ASan/UBSan：各 7/7。
  覆盖独立管理 RPC、同连接上传、BUSY、丢失 Status、重复 chunk、取消、超时、关闭；
  包含 owner 时间采样后提交任务的确定性回归。
- 固件 native_sim：arm 25/25、device 7/7、旧 upgrade 21/21。
  device 包含 Start 响应 ACK 门控、8 个 arm pending 时管理 RPC、CRC 失败、最大 chunk、
  安全态等待/接收超时、Abort 和会话更换。
- Willow/H723、RAM HIL/H723 和真实 USB host 工具编译通过。
- 实机已烧录/校验 inert H723 RAM HIL，未备份、未运行电机应用。USB 为
  `2fe3:574c/323738373233511200260036`、Full Speed 12 Mbps。
  独立恢复入口和共享 Arm 入口各连续 10/10 轮通过；共 40 次 64 KiB 完整上传、
  20 次取消，包含 BUSY 重试、上传中的管理 RPC、取消后重传和独立入口的 peer 重开。
- 首轮上传暴露 Zephyr/Astrial USB 适配层的短环尾互等：部分 COBS 帧不能先被消费，
  旧适配层却等待 ring 排空。新 pin 使用 packet-aligned direct RX，短尾部只暂存一个
  USB 包，保留背压字节；host 在暂存字节发布后再次唤醒 owner，避免漏唤醒。
  新增 FS/HS packet 回归及 ASan/UBSan 各 5/5，core protocol 44/44；SDK 在新 pin 下
  Release 与 ASan/UBSan 各 7/7。HS 是软件测试，不是本次实机速度。
- 原有控制工具 3 × 10000 次回显全部正确，零 payload/dispatch/transport 错误；
  **严格性能门槛未通过**：gap 29.5111% / 0.3557% / 0.2868%，p99
  19.927 / 2.807 / 2.756 ms。机器有并行编译，尚无隔离负载对照，不能宣称性能验收完成。
  完整本地日志见固件工作区 `build/device-hil/ram-upload-repeat-*.log` 和
  `build/device-hil/arm-regression-no-probe.log`。

该阶段的下一步是严格性能复测，以及单独接入/验证 `UpgradeManager`/Flash sink 和 MCUboot
生命周期。现有公开 Arm API 保持不变；新增 C++ 升级 API 尚未绑定到 Python。

## 后续 WLC Flash 优化（2026-09-10）

预生成快照已用 WLC `85b1bdc`（0.7.0-dev / ABI 32）刷新：按 endpoint 角色裁剪
接收分支，并共享 managed RPC 校验、准入/重放和响应完成逻辑。业务 codec、公开
头文件、schema/profile identity 和线格式均不变；不需要改变 SDK 业务调用。

使用该编译器时，Willow 在相同 `-O2 + LTO` 配置下由 402,340 B 减至 383,124 B，
未改 Ruckig 或启用 `-Os`。libflorid Release 的 `LF_ENABLE_WLC=ON` 与默认 OFF
快照模式各 7/7；`lf_check_wirelink` 确认快照与现场生成一致。

本次只做构建及软件测试，没有重新烧录或复测硬件性能；上述 HIL 记录仍对应原版本。
完整优化原理及回归门槛见配套 WLC 仓库 `docs/rpc-flash.md`，固件测量记录见
Ragtime 仓库 `firmware/docs/willow-flash.md`。

## MCUboot 接入（2026-09-10）

Willow 的 `ragtime-mcuboot` sysbuild 现在将 `UpgradeManager` 作为共享服务的 Flash
sink；无需 Wirelink、FCI schema 或快照变更。standalone Willow 仍无写入能力。
`BootStatus` 新增 active/confirmed/pending slot 及两个 slot 的物理地址和大小。

`FirmwareUpdater::reboot(FirmwareRebootMode::kTryBoot)` 使用同一 owner 的 mapped
RPC；已有 Arm 的共享视图要求先停止控制，与上传相同。不在上传期间插入复位；
也支持 `kNormal`。当前 SDK 等到响应接受且观察到设备离开旧会话才返回成功，
不表示已经完成试启动或确认。固件等待响应 ACK 后才复位，SDK 会在这段时间保持 owner 工作。

```sh
cmake -S . -B build/upgrade -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
cmake --build build/upgrade --target florid_example_firmware_update
build/upgrade/examples/florid_example_firmware_update usb://2fe3:574c/SERIAL --status
build/upgrade/examples/florid_example_firmware_update usb://2fe3:574c/SERIAL \
    /path/to/willow/zephyr/zephyr.signed.bin --reboot
# 或仅上传，之后单独执行：
build/upgrade/examples/florid_example_firmware_update usb://2fe3:574c/SERIAL --reboot-only
# 普通重启；用于未确认 test image 的下一次启动/回滚：
build/upgrade/examples/florid_example_firmware_update usb://2fe3:574c/SERIAL --reboot-normal
```

首次需要 SWD 安装 bootloader 和 signed slot0。上传必须是未填满 slot 的 MCUboot
`zephyr.signed.bin`，而非 ELF/HEX/raw/merged 文件。SDK 默认上传配置允许 H7 擦除引起较长暂停。
早期工具的 Reboot RPC 后 250 ms sleep 已移除；现在由 SDK 等待可观察的离开事件。
新接口还能沿原 USB transport 自动重连，核验 `pending=NONE`、`confirmed=PRIMARY`
和目标固件版本，见下一节。

Flash 接收器读回校验 CRC32C、检查 header/TLV 边界后只请求 test boot；真正签名
认证由 MCUboot 完成。Willow 在控制 tick 与 USB owner 连续健康推进 5 秒后确认。
未确认的新应用禁止覆盖回滚槽，已有 pending 镜像也不能覆盖。中断上传后重连从头
开始，不自动恢复运动。升级会关闭电机输出，必须保持机械臂有支撑。

本轮 H723 实机已完成 Flash 上传、试启动确认、无效签名拒绝、未确认镜像禁止覆盖、
普通重启回滚、pending 禁止覆盖、主机进程中断后重传。最终恢复正常自动确认版本。
Release snapshot / WLC ON / ASan+UBSan 各 7/7，快照检查一致。详细证据及探针限制
见 Ragtime 工作区 `firmware/docs/willow-mcuboot-hil.md`；真实断电、swap 途中掉电
及机械互锁验证尚未完成。这不改变前面 RAM HIL 的历史测量或控制性能边界。
新增 API 仍仅为 C++，未绑定 Python。

## SDK 升级接口完善（软件验证轮）

本轮只改 libflorid，不改 Wirelink/FCI/固件，不烧录设备。上面的实机结果属于上一轮，
不构成本轮新重连/核验接口的 HIL 验收。断电恢复重复中断测试仍延期。

### 连接与所有权

`FirmwareUpdater::connect()` 新增 `DeviceSelector`、`DeviceDescriptor`、USB URI 三种入口。
返回 `FirmwareConnectionResult`，区分未找到、歧义、权限、占用、transport、查询超时、
协议不兼容、身份不符，并保留系统错误、设备信息和 selector 歧义候选。
空 selector 要求唯一设备；descriptor 必须指定 VID/PID 和序列号或端口路径。
`create(uri)` 是返回 nullptr 的便利包装，现在也进行只读身份/协议探测。

打开后在同一 endpoint 执行 GetDeviceInfo，检查 USB serial / 已发现的身份与固件返回值，
再检查 FCI 兼容性；不创建 Arm、不获取租约、不要求电机初始化。
按 custom name / firmware type 选择时发现阶段需要额外只读 probe，正式打开后再次匹配。
`connect` 的 timeout 是每次只读 probe 的截止时间，不是整个枚举/系统 USB open 的总时限。

已有 Arm 时用 `arm->stop(); auto updater = arm->firmwareUpdater();`。
所有共享视图仍指向唯一 endpoint，连只读查询/核验的协调状态也是 endpoint 所有；
不要额外调用 `connect()` 抢同一 USB 接口。`deviceInfo()` 总是查询现场，不是 Arm 的启动缓存。
调用者在业务线程执行阻塞方法，串行安排控制模式转换；`progress()` / `cancel()` 可并发。
同类只读 RPC 在途时另一调用返回 Busy；核验的轮询间隙仍允许业务侧只读管理查询。
`cancel()` 仅取消上传，不取消已发出的重启，也不取消只读核验等待。

### 上传、重启与安装结果是三个阶段

`FirmwareUploadOptions` 默认值为整体 120 s、BulkStatus 等待 3000 ms、BUSY 间隔 10 ms、
重试上限 10。整体时限包含排队/Start/上传/清理，不能用重试次数乘以阶段时限延长它。
RAM 或其他自定义 sink 可以显式缩短；这些值是 H7 的默认配置，不是所有 Flash 的耗时保证。
仍使用单次传输内的协议重试，不进行跨重连上传重放或断点续传。

`reboot()` 只有 `m_request_accepted && m_departure_observed` 才返回 `kNone`。
departure 可以是 USB Disconnected/Reconnecting 或新的 Wirelink peer session；
本地析构/stop 不算。接受了请求但截止时间内未离开，返回 Timeout 并保留 accepted 标志。
断线也可能是拔线，所以该结果绝不是安装成功。RPC 超时、响应丢失不能直接推断远端未重启。

`rebootAndWait(expectedVersion, options)` 做一次显式 test reboot，然后只读等待：

1. 查询旧身份和版本，要求非空序列号/板名及兼容 FCI；目标版本必须不同于当前版本。
2. 发送一次 reboot 并保留接受/离开结果；响应丢失或超时后继续观察，绝不自动重发 reboot。
3. 原 Astrial USB transport 按原 VID/PID、serial、port 自动重连，不新开 endpoint。
   核验阶段禁止另一笔上传/重启和控制命令，仍允许只读管理查询。
4. 必须看到不同的 Wirelink peer session，并重新查询一致的身份/板型/协议、版本与 BootStatus。
   仅新会话的 `IDLE + active=PRIMARY + confirmed=PRIMARY + pending=NONE`、有效双槽布局且无活动传输，
   才可能得到 `kTargetConfirmed`；运行并确认旧版本则是 `kPreviousFirmware`，不算成功。
   其他情况继续等到截止时间，最终为 `kUnknown`；身份/协议不符立即报错。

`FirmwareInstallResult` 保留重启结果、旧信息、最近新会话设备信息与 boot 状态供 UI 展示。
只有结果转 bool 为 true 才表示目标版本已确认；`m_error == kNone` 加 `kPreviousFirmware`
表示观察成功但更新未生效。它不区分 MCUboot 签名拒绝和已发生的回滚。
默认每次查询 2 s、重启阶段 5 s、随后重连/健康确认阶段 60 s，轮询间隔 250 ms；
boot 阶段的查询/间隔被剩余总时间裁短。调度延迟和 teardown 不属于硬实时保证。

该版本核验只比较 **FCI 报告的 major/minor/patch**，不是 MCUboot header 的 build 字段，
不是镜像 hash/签名证明。相同版本号无法区分新旧二进制：组合接口在发 reboot 前返回
InvalidArgument；同版本重刷请明确使用 `startUpload()` + `reboot()` + 只读查询，不能宣称
SDK 已核验新二进制身份。发布流程应保证新应用的 FCI 版本与调用方预期一致。

设备须在同一 USB 端口返回并保留原 VID/PID/serial；改变 USB 身份、移动端口或 FCI 不兼容
不能靠“任选一个设备”兜底。核验超时后保持结果未知，只读查询或交由用户处理；不要通过
再次调用 `rebootAndWait()` 来重试查询，因为它会重新发送一次 reboot。

SDK 从不发送“确认镜像”命令、不自动恢复控制租约/电机输出。确认仍由固件健康策略负责；
核验完成后如需恢复控制，由上位机显式重新建立 Arm 控制会话。

```cpp
auto connection = florid::FirmwareUpdater::connect(
    florid::DeviceSelector::bySerial("323738373233511200260036"));
if (!connection) { /* 展示 m_error / m_system_error / m_error_message */ return; }
auto& updater = *connection.m_updater;
auto info = updater.deviceInfo();
auto boot = updater.bootStatus();
// 上位机选择/校验镜像与目标产品、展示机械安全提示；SDK 不自行选版本。
if (updater.startUpload(image_bytes) != florid::FirmwareUpdateError::kNone) return;
while (updater.progress().active()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
if (updater.progress().m_state != florid::FirmwareUploadState::kCompleted) return;
auto installed = updater.rebootAndWait(florid::Version{0, 0, 2});
if (!installed) { /* 区分旧版 / 未知 / 身份错误，不盲目重启或重传 */ }
```

命令行工具同时使用上述接口，不再自行保留 ACK sleep 或 H7 timing 魔法值：

```sh
florid_example_firmware_update usb://2fe3:574c/SERIAL zephyr.signed.bin --install 0.0.2
# 已上传的 pending 镜像：
florid_example_firmware_update usb://2fe3:574c/SERIAL --reboot-and-wait 0.0.2
```

软件测试使用真实 Wirelink peer / bulk receiver 和 managed FCI 编解码，覆盖现场身份查询、
重复查询超时后的槽回收、响应 ACK 后切换会话、接受但未离开、仅本地关闭、目标确认、
旧版确认、目标未确认、身份/协议不符、丢失 reboot 响应、拒绝重启和核验期间互斥。
最终 Release snapshot、WLC ON、ASan/UBSan 三组均 7/7；`lf_check_wirelink` 一致，
命令行 `--help` 通过。以上不包含本轮新接口的 USB 实机重启/重连验收。
