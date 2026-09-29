#include <stddef.h>
#include <stdint.h>

#include "fci_arm.h"
#include "fci_arm_runtime.h"

int main(void)
{
    joint_mit_command_t command = {0};
    cartesian_pose_command_t cartesian = {0};
    motor_feedback_t feedback = {0};
    acquire_control_lease_response_t lease = {0};

    command.has_position = true;
    command.has_velocity = true;
    command.has_torque = true;
    command.has_kp = true;
    command.has_kd = true;
    command.has_dt_us = true;
    command.dt_us = 1000U;
    command.has_sequence = true;
    command.sequence = 1U;
    command.has_gravity_compensation = true;
    command.gravity_compensation = false;
    command.has_sdk_timestamp_us = true;
    command.sdk_timestamp_us = UINT64_C(1000);
    command.has_lease_token = true;
    command.lease_token = UINT64_C(0x0123456789abcdef);

    cartesian.has_transform = true;
    cartesian.has_kp = true;
    cartesian.has_kd = true;
    cartesian.has_dt_us = true;
    cartesian.dt_us = 1000U;
    cartesian.has_sequence = true;
    cartesian.sequence = 2U;
    cartesian.has_gravity_compensation = true;
    cartesian.gravity_compensation = false;
    cartesian.has_sdk_timestamp_us = true;
    cartesian.sdk_timestamp_us = UINT64_C(2000);
    cartesian.has_lease_token = true;
    cartesian.lease_token = command.lease_token;

    feedback.has_position_rad = true;
    feedback.has_velocity_rad_s = true;
    feedback.has_torque_nm = true;
    feedback.has_temperature_c = true;
    feedback.has_device_status_bits = true;
    feedback.device_status_bits = 0U;
    feedback.has_enabled_mask = true;
    feedback.enabled_mask = 0x7fU;

    lease.has_lease_token = true;
    lease.lease_token = command.lease_token;
    lease.has_granted_timeout_ms = true;
    lease.granted_timeout_ms = 1000U;

    if (joint_mit_command_encoded_size(&command) == SIZE_MAX) {
        return 1;
    }
    if (JOINT_MIT_COMMAND_MESSAGE_ID != 25345U) {
        return 2;
    }
    if (cartesian_pose_command_encoded_size(&cartesian) == SIZE_MAX ||
        CARTESIAN_POSE_COMMAND_MESSAGE_ID != 25353U) {
        return 3;
    }
    if (motor_feedback_encoded_size(&feedback) == SIZE_MAX ||
        MOTOR_FEEDBACK_MESSAGE_ID != 24578U) {
        return 4;
    }
    if (acquire_control_lease_response_encoded_size(&lease) == SIZE_MAX ||
        ACQUIRE_CONTROL_LEASE_REQUEST_MESSAGE_ID != 25097U ||
        EMERGENCY_STOP_RESPONSE_MESSAGE_ID != 25348U) {
        return 5;
    }
    if (SET_ZERO_REQUEST_MESSAGE_ID != 0x6107U ||
        SET_ZERO_RESPONSE_MESSAGE_ID != 0x6108U ||
        CLEAR_ERROR_REQUEST_MESSAGE_ID != 0x6109U ||
        CLEAR_ERROR_RESPONSE_MESSAGE_ID != 0x610aU ||
        CLEAR_FAULTS_REQUEST_MESSAGE_ID != 0x6205U ||
        CLEAR_FAULTS_RESPONSE_MESSAGE_ID != 0x6206U ||
        ACQUIRE_CONTROL_LEASE_RESPONSE_MESSAGE_ID != 0x620aU ||
        RELEASE_CONTROL_LEASE_REQUEST_MESSAGE_ID != 0x620bU ||
        RELEASE_CONTROL_LEASE_RESPONSE_MESSAGE_ID != 0x620cU ||
        GET_MOTOR_FEEDBACK_REQUEST_MESSAGE_ID != 0x6213U ||
        GET_MOTOR_FEEDBACK_RESPONSE_MESSAGE_ID != 0x6214U ||
        EMERGENCY_STOP_REQUEST_MESSAGE_ID != 0x6303U ||
        CARTESIAN_VELOCITY_COMMAND_MESSAGE_ID != 0x630aU) {
        return 6;
    }
    if (sizeof(fci_arm_runtime_instance_t) == 0U) {
        return 7;
    }

    return 0;
}
