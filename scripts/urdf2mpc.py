#!/usr/bin/env python3
# ---------------------------------------------------------
# urdf2mpc.py — acados OCP code generator for libflorid MPC
#
# 用法:
#   python3 urdf2mpc.py --urdf <path.urdf> --outdir <dir> --name WillowMPCTraits
#
# 输出:
#   generated/c_generated_code/ — 模型、重力参考和 acados C 包装器
#   generated/WillowMPCTraits.hpp — float SDK / double acados 适配
#   --wrappers-only 只更新 C/C++ 包装器，依赖 NumPy，无需模型生成环境。
# ---------------------------------------------------------

import argparse
import os
import re
from pathlib import Path
from string import Template
import numpy as np

NQ, NV, NX, NU = 6, 6, 12, 6


def build_explicit_dynamics(urdf_path):
    import casadi as ca
    import pinocchio as pin
    import pinocchio.casadi as cpin

    model = pin.buildModelFromUrdf(urdf_path)
    if model.nq != NQ or model.nv != NV:
        raise ValueError("Willow MPC requires a six-joint fixed-base model")
    cmodel = cpin.Model(model)
    cdata = cmodel.createData()

    ee_joint_id = cmodel.njoints - 1
    g_vec = ca.SX([0.0, 0.0, -9.81])

    x    = ca.SX.sym('x',  NX)
    u    = ca.SX.sym('u',  NU)
    xdot = ca.SX.sym('xdot', NX)

    q   = x[:NQ]
    dq  = x[NQ:]
    tau = u

    cpin.forwardKinematics(cmodel, cdata, q)
    cpin.updateFramePlacements(cmodel, cdata)
    T_fk = cdata.oMi[ee_joint_id].homogeneous
    fk_trans = T_fk[12:15]

    cpin.crba(cmodel, cdata, q)
    M = cdata.M

    cpin.computeCoriolisMatrix(cmodel, cdata, q, dq)
    C_times_dq = ca.mtimes(cdata.C, dq)

    cmodel.gravity = cpin.Motion(g_vec, ca.SX.zeros(3))
    cpin.computeGeneralizedGravity(cmodel, cdata, q)
    g_out = cdata.g

    tau_net = tau - C_times_dq - g_out
    ddq = ca.solve(M, tau_net)

    f_expl = ca.vertcat(dq, ddq)
    f_impl = xdot - f_expl
    gravity = ca.Function('willow_mpc_gravity', [x], [ca.densify(g_out)])
    return f_expl, f_impl, x, u, xdot, fk_trans, gravity


def build_acados_ocp(f_expl, f_impl, x, u, xdot, fk_trans, horizon=5, dt=0.020):
    import casadi as ca
    from acados_template import AcadosOcp, AcadosModel

    meta = solver_metadata(horizon, dt)
    model = AcadosModel()
    model.name = 'willow_mpc'
    model.x = x; model.xdot = xdot; model.u = u
    model.f_expl_expr = f_expl; model.f_impl_expr = f_impl

    q_lower, q_upper = meta['q_lower'], meta['q_upper']
    tau_limit, dq_limit = meta['tau_limit'], meta['dq_limit']

    ocp = AcadosOcp()
    ocp.model = model
    ocp.solver_options.N_horizon = horizon
    ocp.solver_options.tf = horizon * dt
    ocp.solver_options.qp_solver = 'FULL_CONDENSING_HPIPM'
    ocp.solver_options.hpipm_mode = 'ROBUST'
    ocp.solver_options.hessian_approx = 'GAUSS_NEWTON'
    ocp.solver_options.integrator_type = 'ERK'
    ocp.solver_options.sim_method_num_steps = meta['integration_steps']
    ocp.solver_options.sim_method_num_stages = 4
    ocp.solver_options.nlp_solver_type = 'SQP_RTI'
    ocp.solver_options.nlp_solver_max_iter = 5
    ocp.solver_options.levenberg_marquardt = 1e-4
    ocp.solver_options.globalization = 'FIXED_STEP'

    dq_expr = x[NQ:]; tau_expr = u
    ocp.model.cost_y_expr   = ca.vertcat(fk_trans, dq_expr, tau_expr)
    ocp.model.cost_y_expr_e = ca.vertcat(fk_trans, dq_expr)

    ny, ny_e = meta['ny'], meta['ny_e']
    ocp.cost.cost_type = ocp.cost.cost_type_e = 'NONLINEAR_LS'
    ocp.cost.W = np.diag(meta['weights'])
    ocp.cost.W_e = np.diag(meta['terminal_weights'])
    ocp.cost.yref = np.zeros(ny); ocp.cost.yref_e = np.zeros(ny_e)

    ocp.constraints.x0 = np.zeros(NX)
    ocp.constraints.lbx = np.concatenate([q_lower, -dq_limit])
    ocp.constraints.ubx = np.concatenate([q_upper,  dq_limit])
    ocp.constraints.idxbx = np.arange(NX); ocp.dims.nbx = NX
    ocp.constraints.lbu = -tau_limit; ocp.constraints.ubu = tau_limit
    ocp.constraints.idxbu = np.arange(NU); ocp.dims.nbu = NU
    ocp.constraints.lbx_e = ocp.constraints.lbx.copy()
    ocp.constraints.ubx_e = ocp.constraints.ubx.copy()
    ocp.constraints.idxbx_e = np.arange(NX)

    ocp.code_gen_options.code_export_directory = 'c_generated_code'
    ocp.code_gen_options.json_file = 'acados_ocp_willow.json'

    return ocp, meta


def solver_metadata(horizon=5, dt=0.020):
    if horizon < 1 or not np.isfinite(dt) or dt <= 0:
        raise ValueError("horizon must be >= 1 and dt must be finite and positive")
    return {
        'nx': NX, 'nu': NU, 'nq': NQ, 'horizon': horizon, 'dt': dt,
        'integration_steps': int(np.ceil(dt / 0.004)),
        'q_lower': np.array([-3.14, 0.0, 0.0, -1.3, -1.57, -1.57]),
        'q_upper': np.array([3.14, 3.14, 3.14, 1.3, 1.57, 1.57]),
        'tau_limit': np.array([5.0, 5.0, 5.0, 3.0, 3.0, 3.0]),
        'dq_limit': np.array([12.48, 3.744, 3.744, 12.48, 12.48, 12.48]),
        'ny': 15, 'ny_e': 9,
        # Position is measured in metres; the old 10/50 weights made centimetre
        # errors negligible relative to joint velocity and torque penalties.
        'weights': np.array([1000.] * 3 + [1.] * NQ + [0.01] * NU),
        'terminal_weights': np.array([5000.] * 3 + [2.] * NQ),
    }


def generate_wrappers(outdir, meta, traits_name):
    """Render the C API and C++ adapter without loading the model/codegen tools."""
    if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', traits_name):
        raise ValueError("name must be a C++ identifier")
    def numbers(values, suffix=''):
        return ', '.join(repr(float(v)) + suffix for v in values)

    data = dict(meta, traits_name=traits_name)
    for key in ('q_lower', 'q_upper', 'dq_limit', 'tau_limit'):
        data[key] = numbers(meta[key], 'f')
    data.update(
        W=numbers(np.diag(meta['weights']).ravel()),
        We=numbers(np.diag(meta['terminal_weights']).ravel()),
        lbx=numbers(np.concatenate([meta['q_lower'], -meta['dq_limit']])),
        ubx=numbers(np.concatenate([meta['q_upper'], meta['dq_limit']])),
        lbu=numbers(-meta['tau_limit']), ubu=numbers(meta['tau_limit']),
    )
    outdir = Path(outdir)
    templates = Path(__file__).resolve().parent / 'templates'
    outputs = {
        'acados_solver_willow_mpc.h.in': outdir / 'c_generated_code/acados_solver_willow_mpc.h',
        'acados_solver_willow_mpc.c.in': outdir / 'c_generated_code/acados_solver_willow_mpc.c',
        'MPCTraits.hpp.in': outdir / f'{traits_name}.hpp',
    }
    for template, output in outputs.items():
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(Template((templates / template).read_text()).substitute(data))
        print(f"-> {output}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--urdf')
    parser.add_argument('--wrappers-only', action='store_true',
                        help='refresh C/C++ adapters for the existing position model (NumPy only)')
    parser.add_argument('--outdir',  default='generated')
    parser.add_argument('--name',    default='WillowMPCTraits')
    parser.add_argument('--horizon', type=int,   default=5)
    parser.add_argument('--dt',      type=float, default=0.020)
    args = parser.parse_args()

    meta = solver_metadata(args.horizon, args.dt)
    if args.wrappers_only:
        generate_wrappers(args.outdir, meta, args.name)
        return
    if not args.urdf:
        parser.error("--urdf is required unless --wrappers-only is used")

    outdir_abs = os.path.abspath(args.outdir)
    os.makedirs(outdir_abs, exist_ok=True)

    print(f"URDF: {args.urdf}")
    f_expl, f_impl, x, u, xdot, fk_trans, gravity = build_explicit_dynamics(args.urdf)

    print(f"Building OCP  H={args.horizon}  dt={args.dt*1000:.0f}ms")
    ocp, meta = build_acados_ocp(f_expl, f_impl, x, u, xdot, fk_trans,
                                  horizon=args.horizon, dt=args.dt)
    print(f"  nx={meta['nx']}  nu={meta['nu']}  nq={meta['nq']}")

    code_gen_dir = os.path.join(outdir_abs, 'c_generated_code')
    json_path = os.path.join(outdir_abs, 'acados_ocp_willow.json')

    ocp.name = ocp.model.name
    ocp.code_gen_options.code_export_directory = code_gen_dir
    ocp.code_gen_options.json_file = json_path
    ocp.code_gen_options.casadi_code_gen_options = {
        "mex": False, "casadi_int": "int", "casadi_real": "double",
    }

    print("Generating CasADi external functions ...")
    ocp.generate_external_functions()

    import casadi as ca
    gravity_codegen = ca.CodeGenerator('willow_mpc_gravity.c', {
        'with_header': True, 'casadi_int': 'int', 'casadi_real': 'double',
    })
    gravity_codegen.add(gravity)
    gravity_codegen.generate(os.path.join(code_gen_dir, 'willow_mpc_model', ''))

    print("Writing solver wrapper ...")
    generate_wrappers(outdir_abs, meta, args.name)
    print("Done.")


if __name__ == '__main__':
    main()
