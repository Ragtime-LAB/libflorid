# Willow 静态重力标定采集

## 最新 willow-v0.2 全流程

最新版完整 description 包已复制到 `model/willow-v0.2/`，源 URDF SHA256 为
`276062732e001dec2aba9f407d94c0a0aca7d07bbd6f0894e9bde7e874728afa`；源文件不会被覆盖。

1. `python run_200_pose_collection.py`：只启动 200 个尚未完成的分散姿态，复用
   `collector.py` 的逐点落盘和断点续扫。
2. `python fit_static_mass_and_export_urdf.py`：按 sample_id 固定拆分 80% 拟合、
   20% 验证，生成 `model/identified/willow-v0.2.static-mass-calibrated.urdf`
   和误差报告。静态数据只拟合重力可观的连杆质量与六轴力矩零偏；COM 和完整惯量
   张量保留最新版 URDF 名义值，不虚称静态数据能完成完整惯性辨识。
3. `python run_gravity_feedforward.py`：MIT `kp=0`、小 `kd`、主机重力力矩前馈、
   固件重力关闭，含 3 秒力矩渐入、力矩限幅、状态看门狗、软限位和异常全轴失能。

两个硬件脚本默认均不使能。最新版 URDF 中的关节 limit 是 CAD 导出的 0 占位值，
运行时不采用它；SDK 与最新版 URDF 的六轴顺序、符号和零偏按实物确认使用 identity 映射。

该目录用于 Willow 六轴机械臂的静态姿态采集。候选网格原始数量为
`4 × 4 × 5 × 5 × 5 = 2000`，经已有 URDF 软限位与自碰撞预览后，当前
`static_design.csv` 保留 1170 个姿态。

## J1 策略

实物 J1 编码器零点位于机械限位附近，因此采集器不会命令 J1 到 `0 rad`。
启用硬件后，程序先被动读取启动姿态，并将当时的 J1 角度复制到本次所有
目标中。J1 全程保持不动；重力辨识激励由 J2-J6 提供。

## 默认安全状态

`collector.py` 默认：

- `ENABLE_HARDWARE = False`
- `LIMITS_VALIDATED = False`
- 第一次启用默认采集 200 组空间分散的 pilot 姿态
- 需要精确输入确认短语才会执行 `arm.enable()`
- 任意异常或退出都会尝试执行全轴 `arm.disable()`

在完成机械限位、方向、增益和急停确认前，不要修改上述两个布尔值。

## 断点续扫

数据逐行追加到 `runs/static_gravity.csv`。每组写入后都会 `flush` 并
`fsync`，然后以临时文件替换的方式原子更新 `runs/progress.json`。CSV 中
记录 `run_id`、本次序号和固定的 manifest `sample_id`。重新启动时，仅把
角度和力矩字段完整的行视为完成，并跳过这些 sample_id；若中断发生在
运动或采样途中，没有完整行，该组下次会自动重跑。

每次从剩余清单中用 J2-J6 的归一化关节空间最远点策略挑选 200 个分散
姿态，再从机械臂当前姿态做最近邻排序。这样既避免“前 200 个姿态扎堆”，
又减少无谓的大跨度运动。不要手工修改已经写入的 `sample_id`。

## 禁用预览

在 PyCharm 中运行 `collector.py`。默认只打印：清单数量、已完成数量、
剩余数量、J1 策略、输出路径和本次上限，不连接或移动机械臂。

## 数据字段

每组保存目标角、反馈角、速度、反馈力矩、MIT 增益、设备时间、序号和
错误位。静止等待时间默认 3 秒，采样窗口默认 0.5 秒并保存中位数。
