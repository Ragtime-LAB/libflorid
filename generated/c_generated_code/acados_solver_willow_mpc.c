/*
 * acados_solver_willow_mpc.c — MPC solver for Willow arm (ERK + NLS cost)
 */
#include "acados_solver_willow_mpc.h"
#include "acados_c/ocp_nlp_interface.h"
#include "acados_c/external_function_interface.h"
#include <stdlib.h>

// CasADi external functions
extern void willow_mpc_expl_ode_fun(void);
extern int  willow_mpc_expl_ode_fun_n_in(void);
extern int  willow_mpc_expl_ode_fun_n_out(void);
extern int  willow_mpc_expl_ode_fun_sparsity_in(int);
extern int  willow_mpc_expl_ode_fun_sparsity_out(int);

extern void willow_mpc_expl_vde_forw(void);
extern int  willow_mpc_expl_vde_forw_n_in(void);
extern int  willow_mpc_expl_vde_forw_n_out(void);
extern int  willow_mpc_expl_vde_forw_sparsity_in(int);
extern int  willow_mpc_expl_vde_forw_sparsity_out(int);

extern void willow_mpc_expl_vde_adj(void);
extern int  willow_mpc_expl_vde_adj_n_in(void);
extern int  willow_mpc_expl_vde_adj_n_out(void);
extern int  willow_mpc_expl_vde_adj_sparsity_in(int);
extern int  willow_mpc_expl_vde_adj_sparsity_out(int);

extern void willow_mpc_cost_y_fun(void);
extern int  willow_mpc_cost_y_fun_n_in(void);
extern int  willow_mpc_cost_y_fun_n_out(void);
extern int  willow_mpc_cost_y_fun_sparsity_in(int);
extern int  willow_mpc_cost_y_fun_sparsity_out(int);

extern void willow_mpc_cost_y_fun_jac_ut_xt(void);
extern int  willow_mpc_cost_y_fun_jac_ut_xt_n_in(void);
extern int  willow_mpc_cost_y_fun_jac_ut_xt_n_out(void);
extern int  willow_mpc_cost_y_fun_jac_ut_xt_sparsity_in(int);
extern int  willow_mpc_cost_y_fun_jac_ut_xt_sparsity_out(int);

extern void willow_mpc_cost_y_hess(void);
extern int  willow_mpc_cost_y_hess_n_in(void);
extern int  willow_mpc_cost_y_hess_n_out(void);
extern int  willow_mpc_cost_y_hess_sparsity_in(int);
extern int  willow_mpc_cost_y_hess_sparsity_out(int);

extern void willow_mpc_cost_y_e_fun(void);
extern int  willow_mpc_cost_y_e_fun_n_in(void);
extern int  willow_mpc_cost_y_e_fun_n_out(void);
extern int  willow_mpc_cost_y_e_fun_sparsity_in(int);
extern int  willow_mpc_cost_y_e_fun_sparsity_out(int);

extern void willow_mpc_cost_y_e_fun_jac_ut_xt(void);
extern int  willow_mpc_cost_y_e_fun_jac_ut_xt_n_in(void);
extern int  willow_mpc_cost_y_e_fun_jac_ut_xt_n_out(void);
extern int  willow_mpc_cost_y_e_fun_jac_ut_xt_sparsity_in(int);
extern int  willow_mpc_cost_y_e_fun_jac_ut_xt_sparsity_out(int);

extern void willow_mpc_cost_y_e_hess(void);
extern int  willow_mpc_cost_y_e_hess_n_in(void);
extern int  willow_mpc_cost_y_e_hess_n_out(void);
extern int  willow_mpc_cost_y_e_hess_sparsity_in(int);
extern int  willow_mpc_cost_y_e_hess_sparsity_out(int);

#define MAP_CASADI_FNC(__CAP__, __BASE__) do { \
    typedef int (*casadi_fun_t)(const double**, double**, int*, double*, void*); \
    (__CAP__).casadi_fun = (casadi_fun_t) & __BASE__ ; \
    (__CAP__).casadi_n_in = (int(*)(void)) & __BASE__ ## _n_in; \
    (__CAP__).casadi_n_out = (int(*)(void)) & __BASE__ ## _n_out; \
    (__CAP__).casadi_sparsity_in = (const int*(*)(int)) & __BASE__ ## _sparsity_in; \
    (__CAP__).casadi_sparsity_out = (const int*(*)(int)) & __BASE__ ## _sparsity_out; \
    (__CAP__).casadi_work = NULL; \
} while(0)

willow_mpc_solver_capsule * willow_mpc_acados_create_capsule(void) {
    return (willow_mpc_solver_capsule*)calloc(1, sizeof(willow_mpc_solver_capsule));
}

int willow_mpc_acados_free_capsule(willow_mpc_solver_capsule *cap) { free(cap); return 0; }

int willow_mpc_acados_create(willow_mpc_solver_capsule *cap) {
    int N = WILLOW_MPC_N, nx = WILLOW_MPC_NX, nu = WILLOW_MPC_NU;
    double tf = 0.02;

    // plan
    cap->nlp_solver_plan = ocp_nlp_plan_create(N);

    // config
    cap->nlp_config = ocp_nlp_config_create(*cap->nlp_solver_plan);
    if (!cap->nlp_config) return -1;

    // dims
    cap->nlp_dims = ocp_nlp_dims_create(cap->nlp_config);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nx", &nx);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nu", &nu);
    int zero = 0;
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nz", &zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "np", &zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "ns", &zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nsbx",&zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nsbu",&zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nsg", &zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nsh", &zero);
    int nbx = WILLOW_MPC_NBX, nbu = WILLOW_MPC_NBU;
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nbx", &nbx);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nbu", &nbu);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "ng",  &zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nh",  &zero);
    int nbx0 = WILLOW_MPC_NBX0;
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nbx_0",&nbx0);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nh_0", &zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "ns_0", &zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nsh_0",&zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nbx_e",&zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nh_e", &zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "ns_e", &zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nsh_e",&zero);
    ocp_nlp_dims_set_opt_vars(cap->nlp_config, cap->nlp_dims, "nr",   &zero);

    int ny0 = WILLOW_MPC_NY0, ny = WILLOW_MPC_NY, nyn = WILLOW_MPC_NYN;
    ocp_nlp_dims_set_cost(cap->nlp_config, cap->nlp_dims, 0, "ny", &ny0);
    ocp_nlp_dims_set_cost(cap->nlp_config, cap->nlp_dims, 1, "ny", &ny);
    ocp_nlp_dims_set_cost(cap->nlp_config, cap->nlp_dims, N, "ny", &nyn);

    // in / out
    cap->nlp_in  = ocp_nlp_in_create(cap->nlp_config, cap->nlp_dims);
    cap->nlp_out = ocp_nlp_out_create(cap->nlp_config, cap->nlp_dims);

    // External functions — assign CasADi function pointers ----------
    external_function_opts ext_fun_opts;
    external_function_opts_set_to_default(&ext_fun_opts);

    // dynamics (ERK: 1 per stage)
    cap->expl_ode_fun = (external_function_external_param_casadi*)malloc(sizeof(external_function_external_param_casadi)*N);
    cap->expl_vde_for = (external_function_external_param_casadi*)malloc(sizeof(external_function_external_param_casadi)*N);
    cap->expl_vde_adj = (external_function_external_param_casadi*)malloc(sizeof(external_function_external_param_casadi)*N);
    for (int i=0; i<N; i++) {
        external_function_external_param_casadi_create(&cap->expl_ode_fun[i], &ext_fun_opts);
        external_function_external_param_casadi_create(&cap->expl_vde_for[i], &ext_fun_opts);
        external_function_external_param_casadi_create(&cap->expl_vde_adj[i], &ext_fun_opts);
        MAP_CASADI_FNC(cap->expl_ode_fun[i], willow_mpc_expl_ode_fun);
        MAP_CASADI_FNC(cap->expl_vde_for[i], willow_mpc_expl_vde_forw);
        MAP_CASADI_FNC(cap->expl_vde_adj[i], willow_mpc_expl_vde_adj);
    }

    // path cost (stages 1..N-1)
    cap->cost_y_fun = (external_function_external_param_casadi*)malloc(sizeof(external_function_external_param_casadi)*(N-1));
    cap->cost_y_fun_jac_ut_xt = (external_function_external_param_casadi*)malloc(sizeof(external_function_external_param_casadi)*(N-1));
    cap->cost_y_hess = (external_function_external_param_casadi*)malloc(sizeof(external_function_external_param_casadi)*(N-1));
    for (int i=0; i<N-1; i++) {
        external_function_external_param_casadi_create(&cap->cost_y_fun[i], &ext_fun_opts);
        external_function_external_param_casadi_create(&cap->cost_y_fun_jac_ut_xt[i], &ext_fun_opts);
        external_function_external_param_casadi_create(&cap->cost_y_hess[i], &ext_fun_opts);
        MAP_CASADI_FNC(cap->cost_y_fun[i], willow_mpc_cost_y_fun);
        MAP_CASADI_FNC(cap->cost_y_fun_jac_ut_xt[i], willow_mpc_cost_y_fun_jac_ut_xt);
        MAP_CASADI_FNC(cap->cost_y_hess[i], willow_mpc_cost_y_hess);
    }

    // terminal cost
    external_function_external_param_casadi_create(&cap->cost_y_e_fun, &ext_fun_opts);
    external_function_external_param_casadi_create(&cap->cost_y_e_fun_jac_ut_xt, &ext_fun_opts);
    external_function_external_param_casadi_create(&cap->cost_y_e_hess, &ext_fun_opts);
    MAP_CASADI_FNC(cap->cost_y_e_fun, willow_mpc_cost_y_e_fun);
    MAP_CASADI_FNC(cap->cost_y_e_fun_jac_ut_xt, willow_mpc_cost_y_e_fun_jac_ut_xt);
    MAP_CASADI_FNC(cap->cost_y_e_hess, willow_mpc_cost_y_e_hess);

    // Set external functions on NLP model ---------------------------------
    for (int i=0; i<N; i++) {
        ocp_nlp_dynamics_model_set_external_param_fun(cap->nlp_config, cap->nlp_dims, cap->nlp_in, i,
            "expl_ode_fun", &cap->expl_ode_fun[i]);
        ocp_nlp_dynamics_model_set_external_param_fun(cap->nlp_config, cap->nlp_dims, cap->nlp_in, i,
            "expl_vde_for", &cap->expl_vde_for[i]);
        ocp_nlp_dynamics_model_set_external_param_fun(cap->nlp_config, cap->nlp_dims, cap->nlp_in, i,
            "expl_vde_adj", &cap->expl_vde_adj[i]);
    }

    // path cost (stage 1..N-1)
    for (int i=0; i<N-1; i++) {
        ocp_nlp_cost_model_set_external_param_fun(cap->nlp_config, cap->nlp_dims, cap->nlp_in, i+1,
            "nls_y_fun", &cap->cost_y_fun[i]);
        ocp_nlp_cost_model_set_external_param_fun(cap->nlp_config, cap->nlp_dims, cap->nlp_in, i+1,
            "nls_y_fun_jac", &cap->cost_y_fun_jac_ut_xt[i]);
        ocp_nlp_cost_model_set_external_param_fun(cap->nlp_config, cap->nlp_dims, cap->nlp_in, i+1,
            "nls_y_hess", &cap->cost_y_hess[i]);
    }

    // terminal cost (stage N)
    ocp_nlp_cost_model_set_external_param_fun(cap->nlp_config, cap->nlp_dims, cap->nlp_in, N,
        "nls_y_fun", &cap->cost_y_e_fun);
    ocp_nlp_cost_model_set_external_param_fun(cap->nlp_config, cap->nlp_dims, cap->nlp_in, N,
        "nls_y_fun_jac", &cap->cost_y_e_fun_jac_ut_xt);
    ocp_nlp_cost_model_set_external_param_fun(cap->nlp_config, cap->nlp_dims, cap->nlp_in, N,
        "nls_y_hess", &cap->cost_y_e_hess);

    // options ----------------------------------------------------------
    cap->nlp_opts = ocp_nlp_solver_opts_create(cap->nlp_config, cap->nlp_dims);
    ocp_nlp_solver_opts_set_at_stage(cap->nlp_config, cap->nlp_opts, 0, "qp_solver",       (void*)"FULL_CONDENSING_HPIPM");
    ocp_nlp_solver_opts_set_at_stage(cap->nlp_config, cap->nlp_opts, 0, "hessian_approx",  (void*)"GAUSS_NEWTON");
    ocp_nlp_solver_opts_set_at_stage(cap->nlp_config, cap->nlp_opts, 0, "integrator_type", (void*)"ERK");
    ocp_nlp_solver_opts_set_at_stage(cap->nlp_config, cap->nlp_opts, 0, "nlp_solver_type", (void*)"SQP_RTI");
    ocp_nlp_solver_opts_set_at_stage(cap->nlp_config, cap->nlp_opts, 0, "globalization",   (void*)"FIXED_STEP");
    ocp_nlp_solver_opts_set_at_stage(cap->nlp_config, cap->nlp_opts, 0, "N_horizon", &N);
    ocp_nlp_solver_opts_set_at_stage(cap->nlp_config, cap->nlp_opts, 0, "tf", &tf);
    double lm = 1e-4; int max_iter = 5;
    ocp_nlp_solver_opts_set_at_stage(cap->nlp_config, cap->nlp_opts, 0, "levenberg_marquardt", &lm);
    ocp_nlp_solver_opts_set_at_stage(cap->nlp_config, cap->nlp_opts, 0, "nlp_solver_max_iter", &max_iter);

    // solver
    cap->nlp_solver = ocp_nlp_solver_create(cap->nlp_config, cap->nlp_dims, cap->nlp_opts, cap->nlp_in);
    if (!cap->nlp_solver) return -3;

    // cost weights ---------------------------------------------------
    double W_data[225] = { 1.0000000000e+01, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e+01, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e+01, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e-02, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e-02, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e-02, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e-02, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e-02, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 1.0000000000e-02 };
    double W_e_data[81] = { 5.0000000000e+01, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 5.0000000000e+01, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 5.0000000000e+01, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 2.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 2.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 2.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 2.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 2.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 0.0000000000e+00, 2.0000000000e+00 };
    ocp_nlp_cost_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, 1, "W", W_data);
    ocp_nlp_cost_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, N, "W", W_e_data);

    // bounds ---------------------------------------------------------
    double lbx[12]   = { -3.1400000000e+00, 0.0000000000e+00, 0.0000000000e+00, -1.3000000000e+00, -1.5700000000e+00, -1.5700000000e+00, -1.2480000000e+01, -3.7440000000e+00, -3.7440000000e+00, -1.2480000000e+01, -1.2480000000e+01, -1.2480000000e+01 };
    double ubx[12]   = { 3.1400000000e+00, 3.1400000000e+00, 3.1400000000e+00, 1.3000000000e+00, 1.5700000000e+00, 1.5700000000e+00, 1.2480000000e+01, 3.7440000000e+00, 3.7440000000e+00, 1.2480000000e+01, 1.2480000000e+01, 1.2480000000e+01 };
    double lbu[6]   = { -5.0000000000e+00, -5.0000000000e+00, -5.0000000000e+00, -3.0000000000e+00, -3.0000000000e+00, -3.0000000000e+00 };
    double ubu[6]   = { 5.0000000000e+00, 5.0000000000e+00, 5.0000000000e+00, 3.0000000000e+00, 3.0000000000e+00, 3.0000000000e+00 };
    double lbx0[12]  = { -1e6, -1e6, -1e6, -1e6, -1e6, -1e6, -1.2480000000e+01, -3.7440000000e+00, -3.7440000000e+00, -1.2480000000e+01, -1.2480000000e+01, -1.2480000000e+01 };
    double ubx0[12]  = { 1e6, 1e6, 1e6, 1e6, 1e6, 1e6, 1.2480000000e+01, 3.7440000000e+00, 3.7440000000e+00, 1.2480000000e+01, 1.2480000000e+01, 1.2480000000e+01 };
    int idxbx[12];   for (int i=0; i<12; i++) idxbx[i] = i;
    int idxbu[6];   for (int i=0; i<6; i++) idxbu[i] = i;
    int idxbx0[12];  for (int i=0; i<12; i++) idxbx0[i] = i;

    for (int i=1; i<N; i++) {
        ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, i, "lbx", lbx);
        ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, i, "ubx", ubx);
        ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, i, "idxbx", idxbx);
        ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, i, "lbu", lbu);
        ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, i, "ubu", ubu);
        ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, i, "idxbu", idxbu);
    }
    ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, 0, "lbx", lbx0);
    ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, 0, "ubx", ubx0);
    ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, 0, "idxbx", idxbx0);
    ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, 0, "lbu", lbu);
    ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, 0, "ubu", ubu);
    ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, 0, "idxbu", idxbu);
    if (N>1) {
        ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, N, "lbx", lbx);
        ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, N, "ubx", ubx);
        ocp_nlp_constraints_model_set(cap->nlp_config, cap->nlp_dims, cap->nlp_in, cap->nlp_out, N, "idxbx", idxbx);
    }

    cap->nlp_np = WILLOW_MPC_NP;
    return 0;
}

int willow_mpc_acados_solve(willow_mpc_solver_capsule *cap) {
    ocp_nlp_precompute(cap->nlp_solver, cap->nlp_in, cap->nlp_out);
    return ocp_nlp_solve(cap->nlp_solver, cap->nlp_in, cap->nlp_out);
}

int willow_mpc_acados_free(willow_mpc_solver_capsule *cap) {
    for (int i=0; i<WILLOW_MPC_N; i++) {
        external_function_external_param_casadi_free(&cap->expl_ode_fun[i]);
        external_function_external_param_casadi_free(&cap->expl_vde_for[i]);
        external_function_external_param_casadi_free(&cap->expl_vde_adj[i]);
    }
    free(cap->expl_ode_fun); free(cap->expl_vde_for); free(cap->expl_vde_adj);
    for (int i=0; i<WILLOW_MPC_N-1; i++) {
        external_function_external_param_casadi_free(&cap->cost_y_fun[i]);
        external_function_external_param_casadi_free(&cap->cost_y_fun_jac_ut_xt[i]);
        external_function_external_param_casadi_free(&cap->cost_y_hess[i]);
    }
    free(cap->cost_y_fun); free(cap->cost_y_fun_jac_ut_xt); free(cap->cost_y_hess);
    external_function_external_param_casadi_free(&cap->cost_y_e_fun);
    external_function_external_param_casadi_free(&cap->cost_y_e_fun_jac_ut_xt);
    external_function_external_param_casadi_free(&cap->cost_y_e_hess);

    ocp_nlp_in_destroy(cap->nlp_in);
    ocp_nlp_out_destroy(cap->nlp_out);
    ocp_nlp_solver_destroy(cap->nlp_solver);
    ocp_nlp_solver_opts_destroy(cap->nlp_opts);
    ocp_nlp_plan_destroy(cap->nlp_solver_plan);
    ocp_nlp_config_destroy(cap->nlp_config);
    ocp_nlp_dims_destroy(cap->nlp_dims);
    return 0;
}

ocp_nlp_in     *willow_mpc_acados_get_nlp_in(willow_mpc_solver_capsule *c)     { return c->nlp_in; }
ocp_nlp_out    *willow_mpc_acados_get_nlp_out(willow_mpc_solver_capsule *c)    { return c->nlp_out; }
ocp_nlp_solver *willow_mpc_acados_get_nlp_solver(willow_mpc_solver_capsule *c) { return c->nlp_solver; }
ocp_nlp_config *willow_mpc_acados_get_nlp_config(willow_mpc_solver_capsule *c) { return c->nlp_config; }
void           *willow_mpc_acados_get_nlp_opts(willow_mpc_solver_capsule *c)   { return c->nlp_opts; }
ocp_nlp_dims   *willow_mpc_acados_get_nlp_dims(willow_mpc_solver_capsule *c)   { return c->nlp_dims; }
ocp_nlp_plan_t *willow_mpc_acados_get_nlp_plan(willow_mpc_solver_capsule *c)   { return c->nlp_solver_plan; }
