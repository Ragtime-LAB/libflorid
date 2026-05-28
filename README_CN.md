# libflorid — 变频率机械臂控制 SDK

**libflorid** 是一个用于机械臂实时运动控制的 C++ 库。通过 TCP+UDP 与臂端控制器通信，支持可变频率控制帧（最高 1kHz）。臂端插值算法自动处理帧率不匹配，客户端可以以 100Hz、10Hz 或任何低于 1kHz 的频率发送，无需重新配置。

公共 API 不依赖 STL（头文件中不使用 `std::vector`、`std::function`、`std::string`），同时适用于 Linux 主机开发和 Arduino、STM32 等嵌入式平台。

## 核心特性

- **变频率控制**：1Hz 至 1kHz，无需模式协商。臂端自动插值到内部 1kHz 伺服循环。
- **编译期动力学**：CasADi + Pinocchio 从 URDF 生成优化后的 C 代码。完整的前向运动学、雅可比矩阵、质量矩阵、科氏力和重力——全部在编译期求解，运行时零分配。
- **公开头文件零 STL**：兼容 Arduino 和裸机工具链（仅使用 C++17 语言特性，无 STL 容器）。
- **每帧自带阻抗增益**：每个控制包携带自身的 `kp/kd` 值，消除 TCP/UDP 竞态条件。
- **IMU 重力自动对齐**：臂端 IMU 上报 `base_gravity` 字段，重力补偿自动适应安装方向（桌装、墙装、倒装、斜装）。
- **在线模式切换**：通过 TCP 可靠命令在关节位置、关节速度、力矩、笛卡尔位姿和笛卡尔速度控制模式之间切换，无需停止控制回路。
- **平台无关传输**：注入自定义 `Transport` 实现以适配 CAN 总线、串口、UAVCAN。
- **仅打包第三方代码**：无 submodule，无 FetchContent — 只需 `git clone` 和 `cmake`。

## 系统要求

### 主机端 (Linux)

| 要求 | 最低版本 |
|---|---|
| 操作系统 | Ubuntu 20.04+ / Arch / 任意 Linux |
| 编译器 | GCC 9+ 或 Clang 14+ |
| CMake | 3.20+ |
| 构建系统 | Ninja（推荐）或 Make |

可选（仅运行 URDF 到 Traits 生成器时需要）：

| 工具 | 用途 |
|---|---|
| Python 3.9+ | 生成器运行时 |
| CasADi | 符号化代码生成 |
| Pinocchio（带 CasADi 绑定） | 运动学/动力学导出 |

### 嵌入式端

任意带有 C++17 编译器和最小 C 标准库（`<stdint.h>`、`<stddef.h>`、`<math.h>`）的平台。库核心无运行时堆分配。

## 快速开始

```bash
git clone https://github.com/Ragtime-LAB/libflorid.git
cd libflorid
cmake -S . -B build -G Ninja
cmake --build build
cd build && ctest --output-on-failure
```

## 使用示例

```cpp
#include <florid/Arm.hpp>
#include <florid/Model.hpp>
#include <florid/traits/PantheraTraits.hpp>

int main() {
    // 一行连接 (TCP 6041, UDP 6040)
    florid::Arm arm("192.168.1.100");

    // 应用 traits 中的安全默认值
    arm.setCollisionBehavior(
        florid::PantheraTraits::collision_lower,
        florid::PantheraTraits::collision_upper);
    arm.setWatchdogTimeout(florid::Duration(200));
    arm.setMaxFrequencyHz(1000.0);

    // Model 使用编译期生成的动力学
    florid::Model<florid::PantheraTraits> model;

    // 力矩模式控制 + 重力补偿
    arm.control([&](const florid::ArmState& s, florid::ArmControl&)
        -> florid::Torques
    {
        float g[6];
        model.gravity(s.q, s.base_gravity, g);  // IMU 自适应

        florid::Torques cmd;
        for (int i = 0; i < 6; ++i)
            cmd.tau[i] = 600.0f * (q_desired[i] - s.q[i])    // PD
                       + 50.0f  * (0.0f - s.dq[i])           // 阻尼
                       + g[i];                                // 重力
        return cmd;
    });
}
```

## 架构

```
┌──────────────────────────────────────────────┐
│                   用户代码                     │
│    Arm arm("ip"); Model<Traits> model;         │
├──────────────────────────────────────────────┤
│  Arm.hpp     — 公开 API（STL-free）            │
│  Model.hpp   — template <typename Traits>     │
│  ArmConfig.hpp — 编译期配置（已废弃）          │
├──────────────────────────────────────────────┤
│  core/       — ArmCore, types, traits helper  │
│  detail/     — Transport, Seqlock, ASIO impl  │
├──────────────────────────────────────────────┤
│  protocols/  — 二进制协议定义                   │
│  3rdparty/   — BasicLinearAlgebra（内嵌）      │
│  traits/     — 生成的 PantheraTraits.hpp       │
└──────────────────────────────────────────────┘
```

| 层 | 说明 |
|---|---|
| `Arm` | 公开入口。`Arm(ip)` 自动创建 TCP+UDP。`Arm(Transport&, Transport*)` 允许为嵌入式平台注入自定义传输实现。 |
| `Model<Traits>` | 无状态计算类。所有成员委托到静态 `Traits` 方法。模板参数在编译期选择臂型。 |
| `ArmCore` | 内部引擎。序列化/反序列化协议包，管理序列号，通过 `SeqlockBuf` 维护线程安全状态。 |
| `Traits`（生成） | 静态结构体，包含 `kDOF`、`kHasMass`、`kHasCoriolis` 以及全部 FK/Jacobian/mass/coriolis/gravity 函数。由 `urdf2traits.py` 从 URDF 生成。 |
| `Protocol` | 仅头文件的二进制协议库。包类型通过可变参数模板 `Dispatcher` 在编译期分发。 |
| `BasicLinearAlgebra` | 内嵌的单头文件矩阵库（MIT）。提供 `Matrix<R,C>`，支持编译期维度检查。 |

## 构建选项

```bash
cmake -S . -B build \
    -DBUILD_TESTS=ON       # 构建单元测试（含协议测试）
    -DBUILD_EXAMPLES=ON    # 构建示例程序（默认 ON）
    -DCMAKE_BUILD_TYPE=Release
```

## 示例

13 个示例程序，演示常见用法：

| 示例 | 演示内容 |
|---|---|
| `echo_arm_state` | 通过 `arm.read()` 持续监控状态 |
| `send_joint` | 关节位置指令，逐关节轮回测试 |
| `send_cartesian` | 笛卡尔位姿指令，线性往复 |
| `send` | 变频率发送，带抖动 |
| `active_joint_control` | `startJointPositionControl()` 手动控制循环 |
| `joint_impedance_control` | 力矩模式 PD + IMU 重力补偿 |
| `cartesian_impedance_control` | 笛卡尔弹簧阻尼，使用 Model FK + Jacobian |
| `force_control` | PI 力控，使用 Jacobian 转置 |
| `generate_consecutive_motions` | 多段运动，`MotionFinished` 切换 |
| `motion_with_control` | 混合力矩 + 运动回调 |
| `joint_velocity_motion` | 速度控制模式 |
| `joint_point_to_point_motion` | 三次多项点到点运动 |
| `switch_control_mode` | 在线 TCP 切换控制模式 |

构建示例：

```bash
cmake -S . -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

运行示例：

```bash
./build/examples/echo_arm_state/florid_example_echo_arm_state 192.168.1.100
```

## 切换臂型

生成的 traits 文件位于 `include/florid/traits/`。要切换到不同臂型，只需改一行 include 和一个模板参数：

```cpp
// Panthera
#include <florid/traits/PantheraTraits.hpp>
florid::Model<florid::PantheraTraits> model;

// Willow
#include <florid/traits/WillowTraits.hpp>
florid::Model<florid::WillowTraits> model;
```

其余代码完全不变——Arm、回调、示例、安全配置均不受影响。

## 测试

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

测试覆盖：
- 协议包大小、类型、序列化（60 项测试）
- Model FK 正交性、对称性及正确性（9 项测试，随 traits 增加而扩展）
- Config 类型枚举值（新增配置项时自动扩展）

## 许可证

libflorid 采用 ISC 许可证。

第三方代码：
- `3rdparty/BasicLinearAlgebra/` — MIT 许可证（版权所有 2019 tomstewart89）
