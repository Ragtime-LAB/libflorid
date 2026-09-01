# libflorid — 机械臂控制 SDK

**libflorid** 是用于 Ragtime Usb2Arm / Willow 六自由度机械臂的 C++20 SDK。它通过 USB 或 UDP 使用生成的 FCI Wirelink 绑定与臂端控制器通信，提供六种实时控制模式及夹爪控制，并通过 `Model<Traits>` 提供编译期生成的动力学。Python 绑定位于 `pyflorid/`。

## 核心特性

- **USB 串口传输**：通过 `Arm::create("usb:///dev/ttyACM0")` 连接。UDP 传输使用 `Arm::create("udp://<ip>:<port>")`，SDK 将固定本地端点绑定（如 `udp://192.168.1.200:5080`），并从收到的第一个数据报学习设备源端点。两种传输都承载由底层传输保证完整性的 Wirelink COBS 字节流。
- **六种控制模式**：`JointMIT`、`JointPosVel`、`JointVel`、`JointPVT`、`CartesianPose`、`CartesianVelocities`。每个控制帧自带 `kp/kd`、可选固件重力标志以及 `MotionFinished` 标记。
- **两种控制方式**：阻塞式 `Arm::control(cb)` 在内部线程按固件周期运行回调；或 `Arm::start*Control()` 返回轮询式 `ActiveControl<T>`，提供 `readOnce()`/`writeOnce()`（Python 绑定使用这种方式）。
- **夹爪控制**：`arm->gripper()` 支持各关节控制模式（电机 joint_id 为 7），状态见 `GripperState` / `ArmState`。
- **编译期动力学**：`Model<WillowTraits>` / `Model<PantheraTraits>` 提供正运动学、位姿、零位/体坐标雅可比、质量矩阵、科氏力与重力——由 `scripts/urdf2traits.py` 从 URDF 生成，编译期求解，运行时零分配。
- **电机寄存器**：按关节（1–6 为臂关节，7 为夹爪）读写控制环增益与保护参数、保存到 Flash、设置零点。
- **设备管理**：读取/更新 `DeviceInfo` 与 `DeviceSettings`，读取 `ArmDiagnostics`，配置重连策略、错误恢复、负载/EE 坐标系及关节/笛卡尔阻抗。
- **可选 MPC**：基于 acados 求解器的 `florid::CartesianMPCSolver<WillowMPCTraits>`（`-DBUILD_MPC=ON` 时构建）。
- **Python 绑定**：通过 pip 安装 `pyflorid`（pybind11），以 snake_case 命名暴露同样的 API。

## 系统要求

| 要求 | 最低版本 |
|---|---|
| 编译器 | GCC 12+ 或 Clang 15+（C++20） |
| CMake | 3.20+ |
| Wirelink + `wlc` | 兼容 ABI 7 的版本 |
| 构建系统 | Ninja（推荐）或 Make |
| 操作系统 | Linux（经 `3rdparty/astrial` 使用 USB 串口） |

可选构建工具：

| 工具 | 用途 |
|---|---|
| pybind11 + NumPy（≥ 2.0）头文件 | Python 绑定（`-DBUILD_PYFLORID=ON`） |
| acados | MPC（`-DBUILD_MPC=ON`） |
| Python 3.9+ + CasADi + Pinocchio（带 CasADi 绑定） | 从 URDF 重新生成 traits / MPC 源码 |

## 子模块

```bash
git submodule update --init --recursive
```

- `protocol/` → FCI `.wl` schema 与 host/firmware binding profile
- `3rdparty/astrial`（USB 串口；内部以普通目录方式附带 asio / tl-expected / readerwriterqueue）
- `3rdparty/acados` — 仅在 `-DBUILD_MPC=ON` 时需要

## 构建与测试

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

默认值：`BUILD_TESTS=OFF`、`BUILD_EXAMPLES=ON`、`BUILD_PYFLORID=OFF`、`BUILD_MPC=OFF`。

如果 Wirelink 未安装为 CMake package，请传入
`-DWIRELINK_SOURCE_DIR=/path/to/wirelink`；若 `wlc` 不在 `PATH`，再传入
`-DWLC_EXECUTABLE=/path/to/wlc`。FCI host 源码只生成到构建目录，不提交生成物。

## 快速开始

```cpp
#include <florid/Arm.hpp>
#include <florid/Model.hpp>
#include <florid/traits/WillowTraits.hpp>

int main() {
    // 一行连接（USB）
    auto arm = florid::Arm::create("usb:///dev/ttyACM0");
    if (!arm) return 1;

    arm->home();

    // Willow 臂的编译期生成动力学
    florid::Model<florid::WillowTraits> model;

    float q_des[6] = {0, 0, 0, 0, 0, 0};

    // MIT（阻抗/力矩）模式 + 主机侧重力补偿
    arm->control([&](const florid::ArmState& s, florid::ArmControl&) -> florid::JointMIT {
        float g[6];
        model.gravity(s.m_q, s.m_base_gravity, g);  // IMU 自适应

        florid::JointMIT cmd;
        for (int i = 0; i < 6; ++i) {
            cmd.m_q[i]    = 0.0f;
            cmd.m_dq[i]   = 0.0f;
            cmd.m_tau[i]  = 600.0f * (q_des[i] - s.m_q[i])  // PD
                          + 50.0f  * (0.0f - s.m_dq[i])     // 阻尼
                          + g[i];                            // 重力
            cmd.m_kp[i]   = 600.0f;
            cmd.m_kd[i]   = 50.0f;
        }
        cmd.m_firmware_gravity = false;
        return cmd;
    });
}
```

### Python

```bash
pip install .                 # 或：pip install -e .
```

```python
import numpy as np
from pyflorid import Arm, JointMIT

arm = Arm.create("usb:///dev/ttyACM0")
ctrl = arm.start_joint_mit_control()

state = ctrl.read_once()
q_des = np.array(state.q, dtype=np.float32)

cmd = JointMIT()
cmd.q = q_des
cmd.dq = np.zeros(6, dtype=np.float32)
cmd.tau = np.zeros(6, dtype=np.float32)
cmd.kp = np.full(6, 10.0, dtype=np.float32)
cmd.kd = np.full(6, 0.2, dtype=np.float32)
cmd.firmware_gravity = True
ctrl.write_once(cmd)
```

C++ 中以 `s_` 前缀命名的方法绑定为 snake_case 名称（`firmware_period_us`、`start_joint_mit_control`、`read_once`、`write_once` …）。完整示例见 `pyflorid/examples/pd_hold.py`。

## 架构

```
┌──────────────────────────────────────────────────────┐
│                 用户代码（C++ / Python）                │
│   Arm::create("usb://...")   Model<Traits>   Gripper  │
├──────────────────────────────────────────────────────┤
│  include/florid/     公开 API（Arm, Model, types）     │
│    core/    ActiveControl                              │
│    detail/  Transport, ArmImpl, FciWirelinkEndpoint,  │
│             WirelinkExecutor, LatencyEstimator        │
│    traits/  WillowTraits, PantheraTraits（生成）      │
│    mpc/     CartesianMPC                               │
├──────────────────────────────────────────────────────┤
│  src/                 实现（Arm/ArmImpl/...）          │
│  protocol/            FCI .wl schema + binding profile│
│  3rdparty/            astrial（USB 串口）, acados      │
│  generated/           acados 求解器 + WillowMPCTraits  │
│  pyflorid/            Python 绑定（pybind11）          │
└──────────────────────────────────────────────────────┘
```

| 层 | 说明 |
|---|---|
| `Arm` | 公开入口。可移动的 PIMPL，封装 `detail::ArmImpl`。`Arm::create(uri)` 构建传输并在构造时获取 `DeviceInfo`/`DeviceSettings`；固件周期来自 `firmware_dt_us`。 |
| `Arm::control(...)` | 阻塞式控制循环；回调在内部线程上运行并返回六种控制类型之一。`ArmControl` 提供延迟/抖动诊断与 `finishMotion()`/`stopControl()`。 |
| `ActiveControl<T>` | `start*Control()` 返回的读写轮询句柄。用 `readOnce()` 读取 `ArmState`，用 `writeOnce()` 发送指令。 |
| `Gripper` | `arm->gripper()`；为夹爪电机（joint_id 7）提供相同的控制模式与 `ActiveControl` 轮询。 |
| `Model<Traits>` | 无状态计算类，委托给生成的 `Traits`（`fk`、`pose`、雅可比、`mass`、`coriolis`、`gravity`）。更换模板参数即可切换臂型。 |
| `detail::Transport` | 抽象字节传输（`send`/`setReceiveCallback`/`poll`）。I/O callback 只 feed 字节并唤醒 endpoint executor。 |
| `FciWirelinkEndpoint` | 从 `protocol/schema/wirelink/arm/*.wl` 生成的单 owner host runtime。遥测从 borrowed LATEST view 复制为稳定快照，实时指令按 message ID 合并最新值，配置操作使用 typed reliable RPC 和可续租 control lease。 |

## 构建选项

```bash
cmake -S . -B build \
    -DBUILD_TESTS=ON       # 单元测试（mock 传输，无需硬件）
    -DBUILD_EXAMPLES=ON    # 示例程序（默认 ON）
    -DBUILD_PYFLORID=ON    # Python 绑定（需 Python + NumPy 开发头文件）
    -DBUILD_MPC=ON         # acados MPC（拉取 generated/ + 3rdparty/acados）
    -DCMAKE_BUILD_TYPE=Release
```

## 示例

每个可执行程序接收一个 USB 设备路径，例如：

```bash
./build/examples/florid_example_00_echo_arm_state /dev/ttyACM0
```

| 示例 | 演示内容 |
|---|---|
| `00_echo_arm_state` | 列出 USB 设备 + 通过 `Arm::read` 流式读取 `ArmState` |
| `00_read_diagnostics` | 读取 `ArmDiagnostics` 遥测 |
| `01_drag_mode` | `Arm::drag()` + 状态流 |
| `01_gripper_move` | `Gripper::startJointMITControl()` 开合循环 |
| `01_joint_sine_motion` | `Arm::home()` + 关节正弦运动 |
| `02_gravity_compensation` | `Model<Traits>::gravity` + `JointMIT` 中的 PD |
| `03_active_joint_control` | `startJointMITControl()` 轮询循环 |
| `03_mode_switching` | 循环切换 MIT / PVT / PosVel 控制模式 |
| `04_motor_registers` | 读写/保存电机寄存器（joint_id 1–7） |
| `05_cartesian_mpc` | 笛卡尔位姿 + `CartesianMPCSolver`（需 `-DBUILD_MPC=ON`） |

## 切换臂型

生成的 traits 位于 `include/florid/traits/`。要切换到不同臂型，只需改一行 include 和一个模板参数：

```cpp
// Willow
#include <florid/traits/WillowTraits.hpp>
florid::Model<florid::WillowTraits> model;

// Panthera
#include <florid/traits/PantheraTraits.hpp>
florid::Model<florid::PantheraTraits> model;
```

其余代码——`Arm`、回调、示例——完全不变。

## 测试

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

测试无需硬件。`tests/test_transport_pipeline` 通过碎片化 Wirelink device peer 驱动 `ArmImpl`，覆盖：

- 带 lease 的连接与确定性释放
- typed 设备设置、元数据和电机寄存器 RPC
- borrowed payload 释放后的稳定 `ArmStatus` 快照
- arm/gripper 指令在碎片化 COBS 字节流上的编码

## 许可证

libflorid 采用 ISC 许可证。

第三方代码：

- `protocol/` → FCI Wirelink schema 与 binding profile
- `3rdparty/astrial`（USB 串口；附带 asio、tl-expected、readerwriterqueue）
- `3rdparty/acados`（MPC，仅在 `-DBUILD_MPC=ON` 时）
