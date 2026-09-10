#pragma once

#include "florid/DeviceTypes.hpp"
#include <chrono>
#include <cstdint>
#include <optional>

namespace florid {

enum class FirmwareUpdateError {
    kNone, kNotConnected, kBusy, kInvalidArgument, kTimeout, kDeviceRejected,
    kProtocolError, kTransportError, kCancelled, kIdentityMismatch,
    kProtocolMismatch,
};

struct FirmwareDeviceInfoResult {
    FirmwareUpdateError m_error{FirmwareUpdateError::kNone};
    std::int32_t m_device_status{};
    std::optional<DeviceInfo> m_info;
    std::uint64_t m_peer_session{}; // query's Wirelink session, NOT a device serial/identity
    [[nodiscard]] explicit operator bool() const noexcept {
        return m_error == FirmwareUpdateError::kNone && m_info.has_value();
    }
};

struct BootStatus {
    FirmwareUpdateError m_error{FirmwareUpdateError::kNone};
    std::int32_t m_device_status{};
    std::uint16_t m_protocol_version{};
    std::int32_t m_boot_state{};
    std::uint16_t m_write_alignment{};
    std::uint16_t m_max_chunk_size{}; // zero means no writable image sink
    std::uint64_t m_bytes_written{};
    std::uint32_t m_active_transfer_id{};
    std::int32_t m_recovery_reason{};
    std::int32_t m_active_slot{}, m_confirmed_slot{}, m_pending_slot{};
    std::uint32_t m_slot0_address{}, m_slot0_size{}, m_slot1_address{}, m_slot1_size{};
};

enum class FirmwareRebootMode { kNormal = 0, kTryBoot = 2 };
struct FirmwareRebootResult {
    FirmwareUpdateError m_error{FirmwareUpdateError::kNone};
    std::int32_t m_device_status{};
    bool m_request_accepted{};
    bool m_departure_observed{}; // transport loss or a different Wirelink peer session
};

struct FirmwareUploadOptions {
    // Internal-flash defaults include H7 sector-erase stalls. RAM/custom sinks
    // may explicitly choose shorter deadlines. No reconnect/resume/replay.
    std::chrono::milliseconds m_timeout{120000}; // includes queueing and cleanup
    std::uint32_t m_status_timeout_ms{3000};
    std::uint32_t m_busy_retry_ms{10};
    std::uint16_t m_max_retries{10};
};

enum class FirmwareInstallState { kUnknown, kTargetConfirmed, kPreviousFirmware };

struct FirmwareInstallOptions {
    std::chrono::milliseconds m_query_timeout{2000};
    std::chrono::milliseconds m_reboot_timeout{5000};
    // Starts after reboot() returns; includes reconnect and health confirmation.
    std::chrono::milliseconds m_boot_timeout{60000};
    std::chrono::milliseconds m_poll_interval{250};
};

struct FirmwareInstallResult {
    FirmwareUpdateError m_error{FirmwareUpdateError::kNone};
    FirmwareInstallState m_state{FirmwareInstallState::kUnknown};
    FirmwareRebootResult m_reboot{};
    std::optional<DeviceInfo> m_previous_info;
    std::optional<DeviceInfo> m_device_info;
    std::optional<BootStatus> m_boot_status;
    // Only a new peer session with matching identity and confirmed primary
    // counts. This verifies major/minor/patch, NOT image hash/build identity.
    [[nodiscard]] explicit operator bool() const noexcept {
        return m_error == FirmwareUpdateError::kNone &&
               m_state == FirmwareInstallState::kTargetConfirmed;
    }
};

enum class FirmwareUploadState { kIdle, kQueued, kStarting, kUploading, kCancelling, kCompleted, kFailed, kCancelled };

struct FirmwareUploadProgress {
    FirmwareUploadState m_state{FirmwareUploadState::kIdle};
    FirmwareUpdateError m_error{FirmwareUpdateError::kNone};
    std::int32_t m_device_status{};
    std::uint64_t m_total_bytes{};
    std::uint64_t m_acknowledged_bytes{};
    std::uint32_t m_transfer_id{};
    std::uint32_t m_retries{};
    std::uint32_t m_busy_responses{};
    [[nodiscard]] bool active() const noexcept {
        return m_state == FirmwareUploadState::kQueued || m_state == FirmwareUploadState::kStarting ||
               m_state == FirmwareUploadState::kUploading || m_state == FirmwareUploadState::kCancelling;
    }
};

} // namespace florid
