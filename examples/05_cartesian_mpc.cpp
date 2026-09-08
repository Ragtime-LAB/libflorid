#include "florid/Arm.hpp"
#include "florid/Model.hpp"
#include "florid/traits/WillowTraits.hpp"
#include "WillowMPCTraits.hpp"
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <thread>

int main(int s_argc, char** s_argv) {
    if (s_argc != 2) {
        std::fprintf(stderr, "Usage: %s <uri>\n", s_argv[0]);
        return 1;
    }
    try {
        auto s_arm = florid::Arm::create(s_argv[1]);
        auto s_control = s_arm->startCartesianPoseControl();
        float s_initial_T[16]{};
        const auto s_initial_state = s_control->readOnce();
        florid::Model<florid::WillowTraits>{}.forwardKinematics(s_initial_state.m_q, s_initial_T);
        const auto s_start = std::chrono::steady_clock::now();
        auto s_next = s_start;
        const auto s_period = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(florid::WillowMPCTraits::kDt));

        // Willow position MPC: move 1 cm along model-base X and return in 4 s.
        // The OCP does not constrain orientation or use Cartesian kp/kd.
        while (true) {
            std::this_thread::sleep_until(s_next);
            s_control->readOnce();
            const double s_t = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - s_start).count();
            florid::CartesianPose s_cmd;
            std::memcpy(s_cmd.m_T, s_initial_T, sizeof(s_initial_T));
            if (s_t >= 4.0) break;
            // Column-major homogeneous transform: translation is [12..14].
            s_cmd.m_T[12] += static_cast<float>(0.005 * (1.0 - std::cos(s_t * 1.5707963267948966)));
            s_control->writeOnce(s_cmd);
            s_next += s_period;
            // Skip missed periods instead of sending a burst of stale commands.
            if (s_next < std::chrono::steady_clock::now())
                s_next = std::chrono::steady_clock::now() + s_period;
        }
        s_arm->stop();
    } catch (const std::exception& s_error) {
        std::fprintf(stderr, "%s\n", s_error.what());
        return 1;
    }
}
