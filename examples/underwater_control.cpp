#include <florid/UnderwaterTeleop.hpp>
#include <chrono>
#include <cstdio>
#include <thread>
#include <string_view>

int main(int argc, char** argv) {
    const bool s_help = argc == 2 && std::string_view(argv[1]) == "--help";
    if (argc != 2 || s_help) {
        std::printf("Usage: %s USB_BULK_URI\nStreams zero-gain MitWithGravity for ten seconds, then disables.\n", argv[0]);
        return s_help ? 0 : 2;
    }
    auto s_connection = florid::UnderwaterTeleop::connect(argv[1]);
    if (!s_connection) { std::fprintf(stderr, "%s\n", s_connection.m_message.c_str()); return 1; }
    auto& s_device = *s_connection.m_device;
    florid::UnderwaterArmCommand s_command{};
    s_command.m_kind = florid::UnderwaterControlKind::kMitWithGravity;
    // Motor coordinates: master j1..j4, gripper, slave j1..j4, gripper.
    // q/dq/kp/kd/tau all default to zero. Fill them before publishing position control.
    const auto s_reply = s_device.enterControl();
    if (!s_reply) { std::fprintf(stderr, "Control error=%d\n", int(s_reply.m_error)); return 1; }
    auto s_deadline = std::chrono::steady_clock::now();
    for (unsigned s_i = 0; s_i < 2500; ++s_i) {
        const auto s_sent = s_device.writeArmCommand(s_command);
        if (!s_sent) { std::fprintf(stderr, "command error=%d\n", int(s_sent.m_error)); break; }
        if (s_i % 250 == 0) {
            if (const auto s_status = s_device.readArmStatus()) {
                std::printf("seq=%u mode=%d fresh=0x%x applied=0x%x cmd=%u stop=%u reject=%u\n",
                    s_status->m_sequence, int(s_status->m_mode), s_status->m_fresh_mask,
                    s_status->m_applied_mask, s_status->m_applied_command_sequence,
                    s_status->m_control_stop_reason, s_status->m_command_reject_reason);
            }
        }
        s_deadline += std::chrono::milliseconds(4);
        std::this_thread::sleep_until(s_deadline);
    }
    return s_device.disable() ? 0 : 1;
}
