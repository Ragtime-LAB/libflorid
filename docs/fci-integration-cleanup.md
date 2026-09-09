# FCI 接入收敛：共享 profile 与 owner 推进

日期：2026-09-09。范围仅包含本轮计划 1–3 的主机部分；MPC、托管 RPC
迁移、默认端点组装和实板测试均不包含在内。

## 改动与边界

- FCI 固定到 `ee8aff8036588649cf5da0510ae41636bdcd8c78`。19 个 arm RPC
  统一定义于 `protocol/schema/wirelink/arm/services.bind.wl`，与 host profile
  组合；host 声明 client、COBS 和出站 send 路由，不增加接收邮箱。
- WLC 仍为 v0.5.0 / ABI 30，Wirelink 仍为 `009a5e9`。Schema identity
  `0x7b0f695d2a4abfb3`、字段映射 RPC、可靠性和产品 COBS + NONE 配置不变。
  Host profile identity 更新为 `0xd194a0c69a5bce53`。
- 快照输入校验包含共享 services 文件，覆盖内容改变和 Windows CRLF 换行。
  重生成后 `generated/wirelink/codec/` 相对 `f05a6db` 无任何变化。
- `s_progress()` 不再因已经消费的遥测、RPC 终态或租约过期请求额外 owner pass。
  新启动的 RPC 仍保留一次推进机会；RX、适配器和协议期限继续由现有 hint/notify
  处理。RPC 排队、租约策略、锁和单 owner 所有权不变。

## 确定性回归

`testConsumedTelemetryDoesNotRequestAnotherPass` 在启动 owner 前通过测试适配器
预投喂两帧完整遥测，记录首次 readiness 查询前的 service 次数；没有 sleep 或耗时阈值。
使用相同新测试、生成代码和 Wirelink，仅替换 endpoint 实现：

- `f05a6db` 的原实现：2 次，按预期被新断言拒绝。
- 本轮实现：1 次；LATEST 合并后 acquire/release 各一次。

这说明该场景少一次推进，不代表所有负载 CPU 或延迟减半。没有运行性能 benchmark。
本地对照源码、对象和二进制位于 `build/fci-profile-owner-telemetry-wlc/`。

## 验证

| 配置 | 结果 |
| --- | --- |
| GCC Release，WLC ON，io_uring OFF，SDK 示例 | 构建通过；CTest 6/6 |
| GCC Release，WLC OFF，io_uring ON | 构建通过；CTest 6/6 |
| Clang Debug，ASan/UBSan，MPC OFF | CTest 6/6 |
| FCI endpoint 完整回归重复运行 | 30/30 |
| `lf_check_wirelink` | 新生成代码与快照一致 |
| FCI 独立构建，host / firmware 各为默认角色 | 各 9/9，含严格 C11/C++20 消费 |

三个 SDK 构建目录分别以 `fci-profile-owner-telemetry-wlc`、`-snapshot`、
`-sanitize` 命名，CTest 日志位于各目录的 `Testing/Temporary/LastTest.log`。
固件配套验收见 Ragtime_Firmwares 的 `firmware/doc/fci-integration-cleanup.md`。

本轮在 `dev/fci-profile-owner-telemetry` 本地分支完成，不推送、不合并 main、
不创建 tag。共享协议提交需先推送，之后消费者的依赖 pin 才能由远端获取。
