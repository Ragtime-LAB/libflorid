#include "florid/Arm.hpp"
#include <cstring>
#include <cstdio>

int main() {
    auto arm = florid::Arm::create("usb:///dev/ttyACM0");
    arm->enable();

    float T_target[16]{};
    T_target[0]  = 1.0f;  T_target[1]  = 0.0f; T_target[2]  = 0.0f; T_target[3]  = 0.5f;
    T_target[4]  = 0.0f;  T_target[5]  = 1.0f; T_target[6]  = 0.0f; T_target[7]  = 0.0f;
    T_target[8]  = 0.0f;  T_target[9]  = 0.0f; T_target[10] = 1.0f; T_target[11] = 0.3f;
    T_target[12] = 0.0f;  T_target[13] = 0.0f; T_target[14] = 0.0f; T_target[15] = 1.0f;

    arm->control([&](const florid::ArmState& /*state*/, florid::ArmControl& /*ctrl*/) {
        florid::CartesianPose cmd;
        std::memcpy(cmd.m_T, T_target, 16 * sizeof(float));
        return cmd;
    });

    return 0;
}
