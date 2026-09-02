#include "florid/Arm.hpp"
#include "florid/Model.hpp"
#include "florid/traits/PantheraTraits.hpp"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>

static std::atomic<bool> g_running{true};

void s_signalHandler(int) { g_running = false; }

void s_printUsage(const char* s_prog) {
    fprintf(stderr, "Usage: %s <uri>\n", s_prog);
    exit(1);
}

int main(int s_argc, char** s_argv) {
    if (s_argc < 2) s_printUsage(s_argv[0]);

    std::string s_uri = s_argv[1];

    signal(SIGINT, s_signalHandler);
    signal(SIGTERM, s_signalHandler);

    printf("Connecting to %s ...\n", s_uri.c_str());
    auto s_arm = florid::Arm::create(s_uri);
    if (!s_arm) {
        fprintf(stderr, "Failed to create Arm.\n");
        return 1;
    }
    printf("Connected. fw_dt=%u us\n\n", s_arm->firmwarePeriodUs());

    printf("Homing ...\n");
    s_arm->home();
    printf("Home done.\n\n");

    // ── Model: compile-time generated dynamics ──
    florid::Model<florid::PantheraTraits> s_model;
    printf("Model: DOF=%d, hasMass=%d, hasCoriolis=%d\n\n",
           s_model.s_kDOF,
           florid::PantheraTraits::kHasMass,
           florid::PantheraTraits::kHasCoriolis);

    // ── Gravity compensation PD control ──
    printf("Starting gravity compensation (kp=0, kd=0 — pure gravity only)\n");
    printf("WARNING: arm will float freely. Press Ctrl+C to stop.\n");

    bool s_initialized = false;
    float s_q_des[6]{};

    s_arm->control([&](const florid::ArmState& s_state,
                        florid::ArmControl&) -> florid::JointMIT
    {
        constexpr float g_kp = 0.0f;
        constexpr float g_kd = 0.0f;

        // First callback: latch current position as target
        if (!s_initialized) {
            for (int s_i = 0; s_i < 6; ++s_i)
                s_q_des[s_i] = s_state.m_q[s_i];
            s_initialized = true;
        }

        // Host-side gravity compensation via Model
        float s_g[6];
        s_model.gravity(s_state.m_q, s_state.m_base_gravity, s_g);

        florid::JointMIT s_cmd;
        for (int s_i = 0; s_i < 6; ++s_i) {
            float s_err_q  = s_q_des[s_i] - s_state.m_q[s_i];
            float s_err_dq = 0.0f - s_state.m_dq[s_i];

            float s_pd = g_kp * s_err_q + g_kd * s_err_dq;
            s_cmd.m_tau[s_i] = s_pd + s_g[s_i];
            s_cmd.m_kp[s_i] = 0.0f;
            s_cmd.m_kd[s_i] = 0.0f;
            s_cmd.m_q[s_i] = 0.0f;
            s_cmd.m_dq[s_i] = 0.0f;
        }
        s_cmd.m_firmware_gravity = false;

        if (!g_running) {
            return florid::JointMIT::MotionFinished(s_cmd);
        }
        return s_cmd;
    });

    printf("Control loop ended.\n");
    return 0;
}
