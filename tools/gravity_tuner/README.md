# gravity_tuner — libflorid 重力补偿参数调试工具

图形化（tkinter）重力补偿调参工具。加载 URDF → 通过 USB 连接 FCI 机械臂 →
向各关节发送纯重力前馈力矩的 MIT 帧，在界面上实时调整 **kGravity**、
**连杆质量**、**逐轴 scale（含方向）**，观察实测力矩与前馈力矩是否吻合，
最后把调好的 **质量** 与 **轴方向** 保存回 URDF。

```
tau[i] = sign · kGravity · scale[i] · g_i(q, base_gravity, masses)
```

- `sign`      — 全局反向开关（Invert gravity）
- `kGravity`  — 全局前馈系数（默认 0.5）
- `scale[i]`  — 逐轴缩放（可负，负号即该轴方向取反，默认 1.0）
- `g_i(...)`  — 用 Pinocchio 从 URDF 实时计算的重力矩（质量可调）

发送的 MIT 帧满足：`q = dq = kp = kd = 0`，`firmware_gravity = False`
（纯主机侧前馈，不依赖固件重力模型）。

---

## 目录结构

```
tools/gravity_tuner/
├── gravity_tuner/
│   ├── __init__.py     # 导出 GravityModel / GravityCompController
│   ├── __main__.py     # python -m gravity_tuner 入口
│   ├── model.py        # URDF → Pinocchio 模型；质量/scale 运行时修改；save()
│   ├── controller.py   # 后台控制线程；MIT 前馈帧；E-stop 监控；安全保持
│   └── app.py          # tkinter 图形界面
└── tests/
    └── smoke_test.py   # 无硬件冒烟测试
```

---

## 1. 环境要求

- **操作系统**：Linux（USB 路径形如 `/dev/ttyACM0`）
- **conda**（Miniconda / Miniforge 均可，推荐 Miniforge）
- **C++ 编译工具链**（构建 pyflorid 绑定用）：`gcc/g++`、`cmake`、`ninja`
- **Python ≥ 3.10**

> 说明：`pinocchio`（运行时动力学）与 `pyflorid`（pybind 绑定）都必须装在
> **同一个 Python 环境**里，且 `_pyflorid` 的 `.so` 需匹配该 Python 版本。

---

## 2. 创建 conda 环境并安装依赖

```bash
# 1) 创建环境（示例用 python 3.12，>=3.10 均可）
conda create -n gravity_tuner python=3.12 -y
conda activate gravity_tuner

# 2) 运行时依赖（conda-forge 提供预编译的 pinocchio）
conda install -c conda-forge pinocchio numpy

# 3) 构建 pyflorid 所需工具
conda install -c conda-forge cmake ninja pybind11

# 4) 构建后端（pip 方式需要 scikit-build-core）
pip install scikit-build-core
```

验证：

```bash
python -c "import pinocchio, numpy, pybind11; print('deps ok')"
```

---

## 3. 构建并安装 pyflorid 绑定

`pyflorid` 是这个工具与 FCI 设备通信的 pybind11 绑定，需要从源码编译
（内部会构建整个 `libflorid` C++ SDK，注意克隆仓库时要带上子模块）：

```bash
git clone --recurse-submodules <repo-url> libflorid
cd libflorid
```

### 方式 A：pip 构建（推荐，一条命令）

```bash
conda activate gravity_tuner
pip install ./pyflorid
```

`pyflorid/pyproject.toml` 会自动打开 `-DBUILD_PYFLORID=ON` 并打包成
`pyflorid` 包安装到当前环境。

### 方式 B：cmake 手动构建（离线 / 调试用）

```bash
conda activate gravity_tuner
PYBIND11_DIR=$(python -c "import pybind11; print(pybind11.get_cmake_dir())")

cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_PYFLORID=ON -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF \
      -Dpybind11_DIR="$PYBIND11_DIR"
cmake --build build --target _pyflorid -j"$(nproc)"

# 把编译好的 .so 放进当前 conda 环境的 site-packages/pyflorid/
SITE=$(python -c 'import sysconfig; print(sysconfig.get_paths()["purelib"])')
mkdir -p "$SITE/pyflorid"
cp build/pyflorid/_pyflorid.cpython-*.so "$SITE/pyflorid/"
```

> 若手动放置 `_pyflorid.so`，务必与 `pyflorid/__init__.py` 放在同一目录。

### 验证安装

```bash
python -c "import pyflorid, pinocchio, numpy; print('OK:', pyflorid.Arm, pyflorid.Model)"
```

---

## 4. 运行冒烟测试（可选，无硬件）

```bash
cd libflorid
python tools/gravity_tuner/tests/smoke_test.py
```

预期输出（每行均需 OK，最后 `ALL PASSED`）：

```
model  OK: ...
controller OK: ...
empty  OK: ...
scale  OK: ...
save   OK: ...
fault  OK: ...
gravity_tuner smoke test: ALL PASSED
```

---

## 5. 启动图形界面

```bash
conda activate gravity_tuner
cd libflorid/tools/gravity_tuner
python -m gravity_tuner
```

---

## 6. 图形界面使用指南

界面从上到下依次为 5 个区块。推荐操作顺序如下。

### ① URDF model

- **Browse...** 选择 URDF 文件；**Load** 解析并构建 Pinocchio 模型。
- 解析成功后状态栏显示 `loaded N-DOF model from <path>`，下方参数表出现
  N 行（对应关节 J1…JN）。
- **Save** 把当前调好的质量与轴方向写回**当前** URDF 文件；
  **Save As...** 另存为。
- 要求：URDF 中可动关节顺序与机械臂 J1…J6 一一对应（与
  `scripts/urdf2traits.py` 相同的约定）。

### ② Device

- URI 输入框默认 `usb:///dev/ttyACM0`（`usb://` 前缀必需）。
- **Connect** 连接 FCI 设备；连接成功后指示灯变绿，状态栏显示 `connected`。
- **Disconnect** 断开。

> 设备号不一定是 ACM0，可先 `ls /dev/ttyACM*` 确认。

### ③ kGravity（全局前馈系数）

- 拖动滑条或输入数值，范围 0.0–2.0，默认 **0.5**。修改即时生效。
- 调参建议：先设一个较小的值（如 0.2）确认方向正确，再逐步加大逼近 1.0。

### ④ 控制栏

- **▶ Start gravity compensation**：先向设备发送 `enable`，然后启动
  MIT 前馈控制线程（每收到一帧状态就发一帧 `tau`）。
- **■ Stop**：停止前先发约 0.5s 的温和保持帧（小 kp/kd + 当前重力前馈），
  避免机械臂自由坠落。
- **Invert gravity (−)**：全局反向开关。打开后所有轴前馈取反（对应上式
  中 `sign = -1`），用于方向判断反了时一键翻转。
- 右侧显示 `loop: running/idle` 与 E-stop 状态。

### ⑤ 参数表（连杆质量 / 逐轴 scale）

| Joint | URDF link | Mass [kg] | Scale |
|---|---|---|---|
| J1 | l1 | 质量滑条+数值框 | 逐轴缩放（可负） |
| … | … | … | … |

- **Mass**：修改 Pinocchio 中该连杆的质量，即时影响重力矩计算。
  注意：连杆质量**同时影响其近端所有关节**的重力矩（物理上如此）。
- **Scale**：该关节前馈力矩独立缩放。设成**负数**即该轴方向取反；
  用于逐轴微调，默认 1.0。`scale` 是运行时参数，**不写入 URDF**。
- 所有修改即时生效，无需重启控制。

### ⑥ 实时显示区

- 每关节显示 `q`（当前关节角）、`t_meas`（实测力矩）、`t_ff`（发送的前馈力矩）。
- 调试目标：调节 Mass/Scale/kGravity，使 **t_ff ≈ t_meas**（两者符号与幅值接近），
  此时重力前馈与真实机械臂最匹配。
- 出现 E-stop / 错误时状态栏变红并自动停止控制；用 **Clear fault** 清除提示。

### 保存到 URDF

调好参数后点 **Save**：

- 每个连杆的 `<inertial><mass value="..."/>` 更新为当前质量；
- **scale 为负**的关节，其 `<axis xyz="..."/>` 被就地取反（保存轴方向）；
- 不向 URDF 增加任何自定义属性。

保存后重新 Load 该 URDF 即可复现质量与方向（scale 重置为 1.0）。

---

## 7. 安全注意事项

- 重力补偿为**纯前馈**（kp=kd=0），模型有偏差时机械臂会漂移，调试时请
  把手指放在 **E-stop 上**，并且以小 kGravity（如 0.2）起步。
- 首次连接建议先手动握持，确认输出力矩方向正确（方向错了立刻按 Invert
  或 Stop）。
- 工具会自动监测固件错误位（E-stop、watchdog、碰撞等），检测到即停止，
  但不要依赖它作为唯一保护。
- 避免在机械臂带负载/抓取重物时调试，应先记录负载在质量/scale 中补偿。

---

## 8. 常见问题（FAQ）

**Q1：`python -m gravity_tuner` 报 `ModuleNotFoundError: pyflorid`？**
A：没安装绑定或装到了别的 Python。确认 `conda activate gravity_tuner`
后再 `pip install ./pyflorid`，并 `python -c "import pyflorid"` 验证。

**Q2：连接失败 / `cannot create Arm from URI ...`？**
A：检查设备号：`ls /dev/ttyACM*`，把界面 URI 改成实际路径（如
`usb:///dev/ttyACM1`）。`usb://` 前缀不能省略。

**Q3：点 Load 后崩出 SIGABRT / 卡住？**
A：`load()` 已对空/不存在的路径做了校验；若你的 URDF 本身有语法问题，
请先在别的工具里校验。URDF 文件需真实存在、非空、且至少含一个可动关节。

**Q4：连接正常但 q 一直显示 0、调 kGravity 没反应？**
A：确认已点 **▶ Start**（控制线程启动后才会发送/读取）。`read_once()`
非阻塞，控制器会自动跳过空状态帧；若仍全 0，检查 E-stop 与设备 `mode`。

**Q5：力矩方向是反的？**
A：先点 **Invert gravity (−)** 全局翻转；若只有个别轴反，把对应轴的
**Scale** 设成负数；最后 Save 会把方向写回 URDF 的 `<axis>`。

**Q6：改质量/scale 时界面上数值被回弹？**
A：中间值（如输入过程中的 `1.`、`-`）会被忽略，输入完整数字后回车生效。

**Q7：如何做无硬件测试？**
A：运行 `tools/gravity_tuner/tests/smoke_test.py`，或通过 `start_with()`
注入假设备；`Arm.create()` 只接受真实的 USB/UDP transport URI。

---

## 9. 无硬件的程序化用法

```python
from gravity_tuner.model import GravityModel
from gravity_tuner.controller import GravityCompController

m = GravityModel().load("arm.urdf")
m.set_mass(3, 2.5)          # 调关节3连杆质量
m.set_scale(2, -1.0)        # 调关节2 scale 并取反方向

ctrl = GravityCompController(m, k_gravity=0.5)
ctrl.set_reverse(True)      # 全局反向
ctrl.connect("usb:///dev/ttyACM0")
ctrl.start()                # enable + 启动控制线程
# ... 调参 ...
ctrl.stop()
```
