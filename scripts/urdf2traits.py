import os
import re
import argparse
import casadi
import sys

try:
    import pinocchio as pin
    import pinocchio.casadi as cpin
except ImportError as e:
    # Pinocchio 3.x 或未装 casadi 的情况
    try:
        import pinocchio as pin
        cpin = pin.casadi
    except AttributeError:
        print(f"ERROR: {e}", file=sys.stderr)
        print("Pinocchio must be built with CasADi support.", file=sys.stderr)
        print("  pip:  pip install pin (pinocchio + casadi)", file=sys.stderr)
        print("  conda: conda install -c conda-forge pinocchio", file=sys.stderr)
        sys.exit(1)
def generate_traits_hpp(urdf_path, output_dir, traits_name="CustomRobotTraits"):
    print(f"Loading URDF from: {urdf_path}")
    
    # 1. 加载 Pinocchio 模型
    model = pin.buildModelFromUrdf(urdf_path)
    cmodel = cpin.Model(model)
    cdata = cmodel.createData()

    nq = cmodel.nq
    nv = cmodel.nv
    kDOF = nq

    ee_joint_id = cmodel.njoints - 1

    # 2. 符号变量
    q    = casadi.SX.sym('q', nq, 1)
    dq   = casadi.SX.sym('dq', nv, 1)
    g_vec = casadi.SX.sym('g_vec', 3, 1)

    # 3. 运动学
    cpin.forwardKinematics(cmodel, cdata, q)
    cpin.updateFramePlacements(cmodel, cdata)
    cpin.computeJointJacobians(cmodel, cdata, q)

    T_fk = cdata.oMi[ee_joint_id].homogeneous

    J_zero = cpin.getJointJacobian(cmodel, cdata, ee_joint_id,
                                    pin.ReferenceFrame.LOCAL_WORLD_ALIGNED)
    J_zero_rm = J_zero.T      # 转行主序

    J_body = cpin.getJointJacobian(cmodel, cdata, ee_joint_id,
                                    pin.ReferenceFrame.LOCAL)
    J_body_rm = J_body.T      # 转行主序

    # 4. 动力学
    cpin.crba(cmodel, cdata, q)
    M = cdata.M
    M_symm = casadi.triu(M) + casadi.triu(M, False).T
    M_rm = M_symm.T           # 转行主序

    cpin.computeCoriolisMatrix(cmodel, cdata, q, dq)
    C_out = casadi.mtimes(cdata.C, dq)

    cmodel.gravity = cpin.Motion(g_vec, casadi.SX.zeros(3))
    cpin.computeGeneralizedGravity(cmodel, cdata, q)
    g_out = cdata.g

    # 5. CasADi 函数 → 输出临时 C 文件
    funcs = []
    funcs.append(casadi.Function('casadi_fk',              [q],       [T_fk]))
    funcs.append(casadi.Function('casadi_zero_jacobian',   [q],       [J_zero_rm]))
    funcs.append(casadi.Function('casadi_body_jacobian',   [q],       [J_body_rm]))
    funcs.append(casadi.Function('casadi_mass',            [q],       [M_rm]))
    funcs.append(casadi.Function('casadi_coriolis',        [q, dq],   [C_out]))
    funcs.append(casadi.Function('casadi_gravity',         [q, g_vec],[g_out]))

    # pose 一个 joint 一个函数
    pose_funcs = []
    for i in range(1, cmodel.njoints):
        T_pose = cdata.oMi[i].homogeneous
        pose_funcs.append(casadi.Function(f'casadi_pose_{i}', [q], [T_pose]))
    funcs.extend(pose_funcs)

    os.makedirs(output_dir, exist_ok=True)

    temp_basename = "temp_casadi.c"
    temp_file = os.path.join(output_dir, temp_basename)

    cg = casadi.CodeGenerator(temp_basename)
    for f in funcs:
        cg.add(f)
    cg.generate(output_dir + os.sep)

    # 6. 读取 C 代码
    with open(temp_file, 'r') as f:
        c_code = f.read()
    os.remove(temp_file)

    # ── 转换 ────────────────────────────────────────────────
    # double → float（CasADi 3.7.x 用 #define 而非 typedef）
    c_code = c_code.replace("#ifndef casadi_real\n#define casadi_real double\n#endif\n",
                            "#define casadi_real float\n")
    c_code = c_code.replace("typedef double casadi_real;",
                            "typedef float casadi_real;")
    # casadi_int → int（Arduino 兼容，long long 太重）
    c_code = c_code.replace("#define casadi_int long long int\n",
                            "#define casadi_int int\n")

    # 移除 math.h（已在顶部 include）
    c_code = c_code.replace('#include <math.h>', '// math.h already included at top level')

    # 去掉 extern "C" — 我们在 namespace 内编译为 C++
    c_code = c_code.replace('#ifdef __cplusplus\nextern "C" {\n#endif\n', '')
    c_code = re.sub(r'#ifdef __cplusplus\n} /\* extern "C" \*/\n#endif\n', '', c_code)

    # CASADI_SYMBOL_EXPORT → 空（namespace 内不需要）
    c_code = c_code.replace('CASADI_SYMBOL_EXPORT ', '')

    # 去掉 CasADi 自动生成的 file-local 函数 static（namespace 内不需要）
    c_code = re.sub(r'(?m)^static ', 'static ', c_code)  # keep for now, harmless

    # 7. 包装到 namespace + 构造 pose switch
    pose_cases = ""
    for i in range(1, cmodel.njoints):
        pose_cases += f"            case {i}: {{ "
        pose_cases += f"const float* a_[1] = {{q}}; float* r_[1] = {{T_out}}; "
        pose_cases += f"detail::casadi_pose_{i}(a_, r_, nullptr, nullptr, 0); "
        pose_cases += f"}} break;\n"

    njoints_total = cmodel.njoints

    hpp = f"""#pragma once
// ---------------------------------------------------------
// 由 urdf2traits.py 自动生成 — DO NOT EDIT DIRECTLY!
// URDF: {os.path.basename(urdf_path)}
// DOF:  {kDOF}   Joints: {njoints_total}
// ---------------------------------------------------------

#include <cmath>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <cstdlib>

namespace florid::detail {{

{c_code}

}} // namespace florid::detail

namespace florid {{

struct {traits_name} {{
    static constexpr int  kDOF         = {kDOF};
    static constexpr bool kHasMass     = true;
    static constexpr bool kHasCoriolis = true;

    // 推荐的安全参数（可覆盖）
    static constexpr float collision_lower[6]   = {{20, 20, 20, 20, 10, 10}};
    static constexpr float collision_upper[6]   = {{20, 20, 20, 20, 10, 10}};
    static constexpr float cartesian_impedance[6] = {{600, 600, 600, 30, 30, 30}};
    static constexpr uint16_t watchdog_timeout_ms = 500;

    // ── 运动学 ──────────────────────────────────────────
    static void fk(const float* q, float* T_out) {{
        const float* a[1] = {{q}};
        float* r[1] = {{T_out}};
        detail::casadi_fk(a, r, nullptr, nullptr, 0);
    }}

    static void zeroJacobian(const float* q, float* J_out) {{
        const float* a[1] = {{q}};
        float* r[1] = {{J_out}};
        detail::casadi_zero_jacobian(a, r, nullptr, nullptr, 0);
    }}

    static void bodyJacobian(const float* q, float* J_out) {{
        const float* a[1] = {{q}};
        float* r[1] = {{J_out}};
        detail::casadi_body_jacobian(a, r, nullptr, nullptr, 0);
    }}

    static void pose(int frame, const float* q, float* T_out) {{
        switch(frame) {{
{pose_cases}
            default: break;
        }}
    }}

    // ── 动力学 ──────────────────────────────────────────
    static void gravity(const float* q, const float g_vec[3], float* g_out) {{
        const float* a[2] = {{q, g_vec}};
        float* r[1] = {{g_out}};
        detail::casadi_gravity(a, r, nullptr, nullptr, 0);
    }}

    static void mass(const float* q, float* M_out) {{
        const float* a[1] = {{q}};
        float* r[1] = {{M_out}};
        detail::casadi_mass(a, r, nullptr, nullptr, 0);
    }}

    static void coriolis(const float* q, const float* dq, float* C_out) {{
        const float* a[2] = {{q, dq}};
        float* r[1] = {{C_out}};
        detail::casadi_coriolis(a, r, nullptr, nullptr, 0);
    }}
}};

}} // namespace florid
"""
    out_path = os.path.join(output_dir, f"{traits_name}.hpp")
    with open(out_path, 'w') as f:
        f.write(hpp)

    print(f"-> {out_path}  ({os.path.getsize(out_path)} bytes)")
    print("Done.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--urdf",  required=True)
    parser.add_argument("--outdir", default="generated")
    parser.add_argument("--name",  default="CustomRobotTraits")
    args = parser.parse_args()
    generate_traits_hpp(args.urdf, args.outdir, args.name)
