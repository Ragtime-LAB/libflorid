# FCI 接入收敛：托管 RPC 与默认端点

本轮接续 [计划 1–3](fci-integration-cleanup.md)，完成剩余的 4–5。
MPC、性能 benchmark、实板测试和发布不在本轮范围内。

## 从哪里读起

1. `protocol/schema/wirelink/arm/services.bind.wl`：19 个 RPC 的请求/响应配对。
2. `src/FciWirelinkEndpoint.cpp` 的 `initialize()`：生成端点、平台环境、
   adapter 和产品 policy 接入唯一 executor。
3. `s_startNext()`：业务参数转成 owned request，调用生成的 `_async()`。
4. `s_complete()` / `s_finalize()`：完成回调只转换业务结果并通知 SDK 等待者。

`FciWirelinkEndpoint` 持有一个 `fci_arm_endpoint_t`，不再手工拼装协议缓冲区、
runtime arena 和 RPC 客户端。调用编号、响应解码、终态通知和 RPC 槽回收由生成
代码处理，不再经过业务层的 inspect/release 分支。

## 保留的产品责任

SDK 仍有串行业务队列、租约续租预留任务、条件变量和可读取的结果快照。
SDK request ID 是本地任务句柄，不是线上 RPC 调用编号。排队时间计入调用超时；
在队列中到期的任务不会占用 Wirelink RPC 槽。已接受的调用在关闭时仅完成一次。

生产者可能在 owner 读取本轮时间后提交；这种“提交时间稍晚于本轮快照”的负年龄
按零处理，不让无符号减法将新调用误判为超时。回归通过 adapter service 固定该顺序，
修复前稳定失败，修复后通过；没有靠增加调用超时时间绕过问题。

遥测仍按既有 generation 语义消费 LATEST；产品校验、控制租约和单 owner 不变。
policy 的期限合并进生成端点 hint，不创建第二个循环。已消费遥测不再触发多余
owner pass 的上一轮回归仍通过。

新增普通 RPC 只需 schema/profile、固件 handler/注册，以及需要暴露的 SDK 参数与
结果转换。无需再增加协议终态轮询、解码和释放分支。慢任务才需要固件复制 typed token。

## 配套构建与部署

Arm revision 8 改用 managed metadata v2，移除业务消息中的 operation_id/status。
**必须与对应固件一起部署，不支持旧 Arm RPC 格式回退。** 遥测、实时控制消息、
upgrade 和 dual 的线格式不变。详见
[共享协议说明](../protocol/docs/arm-managed-endpoints.md)。

生成代码要求开发版 WLC ABI 31；已发布 v0.5.0 是 ABI 30。从配套 WLC 开发分支
执行 `cargo build --release`，然后配置 `-DLF_ENABLE_WLC=ON` 和
`-DWLC_EXECUTABLE=/absolute/path/to/wlc`。`lf_update_wirelink` 更新快照，
`lf_check_wirelink` 校验快照；`LF_ENABLE_WLC=OFF` 不调用编译器。
八个 RPC 槽、4096 字节 COBS FIFO 由共享 CMake 函数统一传播，不能在业务文件单独覆盖。

## 验证记录

- GCC Release、WLC ON：SDK 与示例构建通过，CTest 6/6。
- Clang Debug、WLC OFF、ASan/UBSan：CTest 6/6。
- Sanitizer 下 transport_pipeline、fci_wirelink_endpoint 各连续通过 30 次；
  包括修复前稳定失败的晚提交时钟回归，不以重跑碰巧成功代替修复。
- 共享协议：CTest 10/10，包含直接连接两个生成端点的 managed RPC 测试，
  验证请求快照、owned 响应、拒绝和超过槽容量次数的自动回收。
- 固件：native_sim/QEMU 共 71/71；产品构建见配套固件报告。
- WLC：142 项测试、`cargo fmt --check` 和严格 Clippy 通过。
- Wirelink：主机 CTest 19/19；时钟、frozen-v1 与 current/previous codec 单元
  测试 17/17；生成 runtime 的 QEMU 集成 5/5。后者使用仅含 Wirelink/CMSIS
  的 `ZEPHYR_MODULES`，排除当前 Ragtime workspace 全局模块引入的 CXX 配置错误。

没有测 CPU 或延迟，也没有在 H7 烧录。本轮仅本地 dev 提交；依赖尚未推送。
发布开发分支时须先推送 Wirelink/WLC/FCI，再推送引用这些提交的消费者。
