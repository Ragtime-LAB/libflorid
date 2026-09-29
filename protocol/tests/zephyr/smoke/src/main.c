#include <zephyr/kernel.h>

#include "fci_arm.h"
#include "fci_arm_runtime.h"

int main(void)
{
    arm_diagnostics_t diagnostics = {0};

    diagnostics.has_uptime_s = true;
    diagnostics.uptime_s = 1U;
    diagnostics.has_tick_count = true;
    diagnostics.has_mode_entry_ms = true;
    diagnostics.has_bus_healthy = true;
    diagnostics.has_bus_state = true;
    diagnostics.has_tx_error_count = true;
    diagnostics.has_rx_error_count = true;
    diagnostics.has_joint_healthy_mask = true;
    diagnostics.has_joint_temperature_c = true;
    diagnostics.has_gripper_healthy = true;
    diagnostics.has_gripper_temperature_c = true;
    return arm_diagnostics_encoded_size(&diagnostics) == SIZE_MAX;
}
