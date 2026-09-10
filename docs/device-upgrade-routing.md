# 共享产品路由与 RAM 上传验证

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

## 本轮边界

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

下一步是严格性能复测，以及单独接入/验证 `UpgradeManager`/Flash sink 和 MCUboot
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
