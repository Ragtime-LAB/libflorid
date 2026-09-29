/* SPDX-License-Identifier: Apache-2.0 */

#include "fci_arm_firmware_runtime.h"
#include "fci_arm_host_runtime.h"

_Static_assert(FCI_ARM_HOST_RUNTIME_HAS_RPC_CLIENT == 1, "host calls RPCs");
_Static_assert(FCI_ARM_HOST_RUNTIME_HAS_RPC_SERVER == 0, "host has no RPC server");
_Static_assert(FCI_ARM_FIRMWARE_RUNTIME_HAS_RPC_CLIENT == 0, "firmware has no client");
_Static_assert(FCI_ARM_FIRMWARE_RUNTIME_HAS_RPC_SERVER == 1, "firmware serves RPCs");
_Static_assert(FCI_ARM_HOST_RUNTIME_HAS_MANAGED_RPC == 1 &&
               FCI_ARM_FIRMWARE_RUNTIME_HAS_MANAGED_RPC == 1,
               "revision-8 RPC metadata is owned by Wirelink");
_Static_assert(FCI_ARM_FIRMWARE_SCHEMA_IDENTITY == FCI_ARM_HOST_SCHEMA_IDENTITY,
               "shared profiles must not alter the wire schema");

int main(void) {
    fci_arm_host_runtime_config_t host;
    fci_arm_firmware_runtime_config_t firmware;

    if (fci_arm_host_runtime_config_defaults(&host) != WL_OK) {
        return 1;
    }
    if (fci_arm_firmware_runtime_config_defaults(&firmware) != WL_OK) {
        return 2;
    }
    if (fci_arm_host_runtime_config_enable_client(&host) != WL_OK ||
        fci_arm_host_runtime_config_enable_server(&host) != WL_ERR_NOT_SUPPORTED ||
        fci_arm_firmware_runtime_config_enable_client(&firmware) != WL_ERR_NOT_SUPPORTED ||
        fci_arm_firmware_runtime_config_enable_server(&firmware) != WL_OK) {
        return 3;
    }
    /* Outbound-only routes generate typed sends without a receive mailbox. */
    (void)fci_arm_host_endpoint_send_joint_mit_command;
    (void)fci_arm_host_endpoint_send_gripper_pvt_command;
    (void)fci_arm_firmware_endpoint_send_arm_status;
    (void)fci_arm_firmware_endpoint_send_arm_diagnostics;
    return 0;
}
