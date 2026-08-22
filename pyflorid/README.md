# pyflorid

Python bindings for [libflorid](https://github.com/Ragtime-LAB/libflorid) — a
C++20 variable-frequency arm control SDK (USB transport, 6-DOF, real-time
control loop).

This package wraps the native `florid` library with pybind11 and exposes the
`Arm`, `Gripper`, control-loop command types, and the `Model` kinematics
interface to Python.

## Install

```bash
pip install pyflorid
```

Or build from source (requires a C++20 toolchain, pybind11, and NumPy):

```bash
pip install ./pyflorid
```

See the top-level `README.md` for the full SDK documentation.
