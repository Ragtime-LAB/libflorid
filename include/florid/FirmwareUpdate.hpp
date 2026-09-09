#pragma once

#include <chrono>
#include <cstdint>

namespace florid {

enum class FirmwareUpdateError {
    kNone, kNotConnected, kBusy, kInvalidArgument, kTimeout, kDeviceRejected,
    kProtocolError, kTransportError, kCancelled,
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
};

struct FirmwareUploadOptions {
    std::chrono::milliseconds m_timeout{30000}; // includes queueing and cleanup
    std::uint32_t m_status_timeout_ms{250};
    std::uint32_t m_busy_retry_ms{10};
    std::uint16_t m_max_retries{5};
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
