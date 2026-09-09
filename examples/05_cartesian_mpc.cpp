#include "florid/Arm.hpp"
#include "florid/Model.hpp"
#include "florid/traits/WillowTraits.hpp"
#include "WillowMPCTraits.hpp"
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
volatile std::sig_atomic_t s_interrupted = 0;
void interrupt(int) { s_interrupted = 1; }
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

void printStatus(const florid::MPCControlStatus& s, double elapsed) {
    std::printf("solves=%llu outputs=%llu (%.1f Hz average) solve=%.3f/%.3f ms last/max "
                "late_max=%.3f ms skipped=%llu/%llu failures=%llu stop=%s\n",
        static_cast<unsigned long long>(s.m_solves), static_cast<unsigned long long>(s.m_outputs),
        elapsed > 0 ? s.m_outputs / elapsed : 0,
        s.m_last_solve_ms, s.m_max_solve_ms, s.m_max_output_lateness_ms,
        static_cast<unsigned long long>(s.m_missed_planning_periods),
        static_cast<unsigned long long>(s.m_missed_output_periods),
        static_cast<unsigned long long>(s.m_solver_failures), florid::mpcStopReasonMessage(s.m_stop_reason));
}

void writeCsv(std::ofstream& csv, double t, const florid::CartesianPose& target,
              const florid::ArmState& state, const florid::MPCControlStatus& status) {
    if (!csv.is_open()) return;
    csv << t << ',' << target.m_T[12] << ',' << target.m_T[13] << ',' << target.m_T[14];
    for (const float* values : {state.m_q, state.m_dq, status.m_reference_q, status.m_reference_dq})
        for (int i = 0; i < 6; ++i) csv << ',' << values[i];
    csv << ',' << status.m_solves << ',' << status.m_outputs << ',' << status.m_solver_status
        << ',' << status.m_last_solve_ms << ',' << status.m_state_age_ms << ',' << status.m_plan_age_ms
        << ',' << int(status.m_stop_reason) << '\n';
}
}

int main(int s_argc, char** s_argv) {
    if (s_argc < 2 || std::strcmp(s_argv[1], "--help") == 0) {
        std::printf("Usage: %s <uri> [--hold-only] [--csv path]\n"
                    "Willow position MPC: 50 Hz planning, internal 500 Hz output.\n"
                    "Holds 1 s, moves 1 cm along model-base X and back over 4 s, then holds 1 s.\n"
                    "--hold-only keeps the initial position for the whole 6 s. Ctrl-C stops output.\n",
                    s_argv[0]);
        return s_argc < 2 ? 1 : 0;
    }
    try {
        bool s_hold_only = false;
        std::string s_csv_path;
        for (int i = 2; i < s_argc; ++i) {
            if (std::strcmp(s_argv[i], "--hold-only") == 0) s_hold_only = true;
            else if (std::strcmp(s_argv[i], "--csv") == 0 && i + 1 < s_argc) s_csv_path = s_argv[++i];
            else throw std::runtime_error("Unknown or incomplete argument; see --help");
        }
        std::ofstream s_csv;
        if (!s_csv_path.empty()) {
            s_csv.open(s_csv_path);
            if (!s_csv) throw std::runtime_error("Cannot open CSV output");
            s_csv.exceptions(std::ios::badbit | std::ios::failbit);
            s_csv << "t,target_x,target_y,target_z";
            for (const char* s_field : {"q", "dq", "reference_q", "reference_dq"})
                for (int i = 0; i < 6; ++i) s_csv << ',' << s_field << i;
            s_csv << ",solves,outputs,solver_status,solve_ms,state_age_ms,plan_age_ms,stop_reason\n";
        }
        std::signal(SIGINT, interrupt);
        std::signal(SIGTERM, interrupt);
        auto s_arm = florid::Arm::create(s_argv[1]);
        florid::ArmState s_initial_state;
        const auto s_deadline = Clock::now() + 2s;
        do {
            s_initial_state = s_arm->readOnce();
            if (s_initial_state.m_seq != 0) break;
            if (s_interrupted) return 0;
            std::this_thread::sleep_for(2ms);
        } while (Clock::now() < s_deadline);
        if (s_initial_state.m_seq == 0) throw std::runtime_error("No fresh joint feedback within 2 seconds");
        if (s_initial_state.m_errors) throw std::runtime_error("Device reports a fault; initial hold refused");
        for (int i = 0; i < 6; ++i) {
            if (!std::isfinite(s_initial_state.m_q[i]) || !std::isfinite(s_initial_state.m_dq[i]) ||
                s_initial_state.m_q[i] < florid::WillowMPCTraits::kQLower[i] ||
                s_initial_state.m_q[i] > florid::WillowMPCTraits::kQUpper[i] ||
                std::abs(s_initial_state.m_dq[i]) > 0.05f)
                throw std::runtime_error("Start with a stationary Willow inside its joint limits");
        }
        florid::Model<florid::WillowTraits> s_model;
        float s_g[6], s_gravity[3]{0, 0, -9.81f}, s_initial_T[16];
        s_model.gravity(s_initial_state.m_q, s_gravity, s_g);
        for (int i = 0; i < 6; ++i)
            if (!std::isfinite(s_g[i]) || std::abs(s_g[i]) >= florid::WillowMPCTraits::kTauLimit[i])
                throw std::runtime_error("Initial pose cannot balance gravity within the MPC torque limits");
        s_model.forwardKinematics(s_initial_state.m_q, s_initial_T);
        florid::MPCControlConfig s_config;
        s_config.max_joint_velocity = 0.5f;
        s_config.max_joint_acceleration = 10.0f;
        auto s_control = s_arm->startCartesianPoseControl(s_config);
        std::printf("Willow only; position tracking does not hold orientation. Firmware period: %u us.\n"
                    "Starting %s, target updates 50 Hz; interpolation is owned by libflorid.\n",
                    s_arm->firmwarePeriodUs(), s_hold_only ? "6-second hold" : "1 cm out-and-back motion");
        const auto s_start = Clock::now();
        auto s_next = s_start;
        int s_last_report = -1;
        florid::CartesianPose s_last_target;
        std::memcpy(s_last_target.m_T, s_initial_T, sizeof(s_initial_T));
        while (!s_interrupted) {
            std::this_thread::sleep_until(s_next);
            const double s_t = std::chrono::duration<double>(Clock::now() - s_start).count();
            const auto s_status = s_control->status();
            if (s_status.m_state == florid::MPCControlState::kFault) {
                writeCsv(s_csv, s_t, s_last_target, s_control->readOnce(), s_status);
                printStatus(s_status, s_t);
                throw std::runtime_error(florid::mpcStopReasonMessage(s_status.m_stop_reason));
            }
            if (s_t >= 6.0) break;
            if (s_t >= 1.0 && s_status.m_state != florid::MPCControlState::kRunning)
                throw std::runtime_error("MPC did not establish the initial hold");
            florid::CartesianPose s_cmd;
            std::memcpy(s_cmd.m_T, s_initial_T, sizeof(s_initial_T));
            if (!s_hold_only && s_t > 1.0 && s_t < 5.0)
                s_cmd.m_T[12] += static_cast<float>(0.005 * (1.0 - std::cos((s_t - 1.0) * 1.5707963267948966)));
            s_control->writeOnce(s_cmd);
            s_last_target = s_cmd;
            if (int(s_t) != s_last_report) { printStatus(s_status, s_t); s_last_report = int(s_t); }
            writeCsv(s_csv, s_t, s_cmd, s_control->readOnce(), s_status);
            s_next += florid::MPCControlConfig::planning_period;
            if (s_next <= Clock::now()) s_next = Clock::now() + florid::MPCControlConfig::planning_period;
        }
        s_control->stop();
        const auto s_elapsed = std::chrono::duration<double>(Clock::now() - s_start).count();
        const auto s_final_status = s_control->status();
        writeCsv(s_csv, s_elapsed, s_last_target, s_control->readOnce(), s_final_status);
        printStatus(s_final_status, s_elapsed);
    } catch (const std::exception& s_error) {
        std::fprintf(stderr, "%s\n", s_error.what());
        return 1;
    }
    return 0;
}
