# libflorid-underwater

Ragtime Underwater 双臂控制器的 C++20 与 Python SDK。SDK 通过 USB Bulk 和生成的
FCI Wirelink 绑定提供设备发现、模式 RPC、设备设置、零点校准、固件升级、250 Hz
十电机状态以及 MIT 控制命令。

公开入口为 `florid::UnderwaterTeleop`。支持 Damp、Gravity、Teleop、Control、
Zeroing 和遥操对齐过渡状态。Control 模式只接受 MIT 与带固件重力补偿的 MIT，
电机顺序为主臂 j1～j4、主夹爪、从臂 j1～j4、从夹爪。

## 构建

```bash
git submodule update --init 3rdparty/astrial 3rdparty/wirelink
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

普通构建直接使用仓库内的 Wirelink 快照，不需要 WLC 或 Rust。协议重新生成步骤见
[`protocol-patches/README.md`](protocol-patches/README.md)。

Python 模块可通过 `-DBUILD_PYFLORID=ON` 构建，也可以直接安装：

```bash
python3 -m pip install .
```

## C++ 使用

```cpp
#include <florid/UnderwaterTeleop.hpp>

auto connection = florid::UnderwaterTeleop::connect("usb://2fe3:574c/SERIAL");
if (!connection) return 1;
auto& robot = *connection.m_device;

robot.enable();       // Gravity
robot.enterTeleop();  // 对齐后进入跟随
robot.disable();      // Damp
```

`examples/underwater_control.cpp` 展示 4 ms 控制循环。SDK 进程关闭时不会静默改变机械臂
模式；需要失能时应明确请求 Damp。

## Python 上位机

上位机支持 USB 搜索、模式控制、零点校准、持久化设备设置、固件升级和十电机 MIT 控制：

```bash
cmake -S . -B build-underwater-sdk -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_PYFLORID=ON -DBUILD_TESTS=ON
cmake --build build-underwater-sdk
python3 tools/underwater_console/run.py
```

操作细节见 [`tools/underwater_console/README.md`](tools/underwater_console/README.md)。

## 目录

- `include/florid/UnderwaterTeleop.hpp`：公开 C++ API
- `include/florid/UnderwaterTypes.hpp`：RPC、设置、状态与命令类型
- `src/`：USB、设备发现、Wirelink 端点与升级实现
- `pyflorid/`：Python 绑定与示例
- `tools/underwater_console/`：Tk 上位机
- `generated/wirelink/`：已提交的 ABI 32 协议快照
- `tests/`：协议、升级、控制和打包测试

此分支已移除水下机械臂不使用的通用六轴 Arm API、UDP、URDF 模型生成、重力调参器
和 Acados MPC 运行库。

## 许可证

ISC。第三方组件沿用 `3rdparty/` 内各自的许可证。
