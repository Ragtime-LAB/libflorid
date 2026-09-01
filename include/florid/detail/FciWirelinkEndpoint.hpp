#ifndef FLORID_DETAIL_FCI_WIRELINK_ENDPOINT_HPP
#define FLORID_DETAIL_FCI_WIRELINK_ENDPOINT_HPP

#include "florid/ArmState.hpp"
#include "florid/ControlTypes.hpp"
#include "florid/DeviceTypes.hpp"
#include "florid/detail/WirelinkExecutor.hpp"

#include "fci_arm_runtime.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <string_view>

namespace florid::detail {

enum class FciEndpointStatus : std::uint8_t {
    kOk,
    kNotReady,
    kQueueFull,
    kBusy,
    kInvalidArgument,
    kNoLease,
    kNoData,
    kCodecError,
    kLinkError,
    kTimeout,
    kCancelled,
    kDomainError,
    kInternalError,
};

enum class FciOperationState : std::uint8_t {
    kUnknown,
    kQueued,
    kLinkPending,
    kWaitingResponse,
    kCompleted,
    kTimedOut,
    kCancelled,
    kLinkFailed,
    kDomainError,
};

enum class FciControlLeaseState : std::uint8_t {
    kReleased,
    kAcquireQueued,
    kAcquiring,
    kHeld,
    kRenewQueued,
    kRenewing,
    kReleaseQueued,
    kReleasing,
    kExpired,
    kFailed,
};

enum class FciArmMode : std::uint8_t {
    kPc,
    kDrag,
    kDamp,
    kRetracting,
    kTeleop,
};

enum class FciMotorControlMode : std::uint8_t {
    kMit = 1,
    kPositionVelocity = 2,
    kVelocity = 3,
    kPvt = 4,
};

struct FciSubmitResult {
    FciEndpointStatus m_status{FciEndpointStatus::kInternalError};
    std::uint64_t m_request_id{};
};

struct FciOperationResult {
    std::uint64_t m_request_id{};
    std::uint32_t m_operation_id{};
    FciOperationState m_state{FciOperationState::kUnknown};
    FciEndpointStatus m_status{FciEndpointStatus::kNoData};
    std::int32_t m_domain_status{};
    std::int32_t m_link_status{};
};

struct FciControlLeaseSnapshot {
    FciControlLeaseState m_state{FciControlLeaseState::kReleased};
    FciEndpointStatus m_status{FciEndpointStatus::kNoLease};
    std::int32_t m_domain_status{};
    std::uint64_t m_token{};
    std::uint32_t m_granted_timeout_ms{};
};

struct FciArmStatusSnapshot {
    ArmState m_state{};
    std::uint64_t m_last_sdk_timestamp_us{};
    std::uint64_t m_generation{};
};

struct FciWirelinkEndpointConfig {
    std::uint64_t m_session_id{1};
    std::uint16_t m_max_retries{2};
    std::uint32_t m_ack_timeout_ms{20};
};

struct FciWirelinkEndpointStats {
    std::uint64_t m_dispatch_calls{};
    std::uint64_t m_dispatch_errors{};
    std::uint64_t m_runtime_poll_calls{};
    std::uint64_t m_runtime_timeouts{};
    std::uint64_t m_latest_acquires{};
    std::uint64_t m_latest_releases{};
    std::uint64_t m_rpc_started{};
    std::uint64_t m_rpc_released{};
    std::uint64_t m_rpc_cancelled{};
    std::size_t m_runtime_storage_bytes{};
};

class FciWirelinkEndpoint {
public:
    static constexpr std::size_t s_kOperationCapacity = 8;
    static constexpr std::size_t s_kPublicOperationCapacity = 7;
    static constexpr std::size_t s_kMaximumDeviceNameBytes = 31;

    using ArmStatusCallback = void (*)(void* s_user_data,
                                       const FciArmStatusSnapshot& s_status)
        noexcept;
    using DiagnosticsCallback = void (*)(void* s_user_data,
                                         const ArmDiagnostics& s_diagnostics)
        noexcept;

    FciWirelinkEndpoint() = default;
    ~FciWirelinkEndpoint();

    FciWirelinkEndpoint(const FciWirelinkEndpoint&) = delete;
    FciWirelinkEndpoint& operator=(const FciWirelinkEndpoint&) = delete;

    FciEndpointStatus initialize(
        const FciWirelinkEndpointConfig& s_config = {});
    FciEndpointStatus setSink(wl_sink_fn s_sink, void* s_user_data) noexcept;
    FciEndpointStatus setCallbacks(ArmStatusCallback s_arm_status,
                                   DiagnosticsCallback s_diagnostics,
                                   void* s_user_data) noexcept;
    FciEndpointStatus start() noexcept;
    void stop() noexcept;

    FciEndpointStatus feedBytes(const std::uint8_t* s_data,
                                std::size_t s_size,
                                std::size_t& s_accepted) noexcept;
    void notify() noexcept { m_executor.notify(); }

    FciSubmitResult acquireControlLease(std::uint32_t s_requested_timeout_ms,
                                        std::uint32_t s_rpc_timeout_ms) noexcept;
    FciSubmitResult releaseControlLease(
        std::uint32_t s_rpc_timeout_ms) noexcept;
    FciSubmitResult getDeviceInfo(std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult setDeviceInfo(std::string_view s_custom_name,
                                  FirmwareType s_firmware_type,
                                  std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult getDeviceSettings(std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult setDeviceSettings(const DeviceSettings& s_settings,
                                      std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult setArmControlMode(FciMotorControlMode s_mode,
                                      std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult setGripperControlMode(FciMotorControlMode s_mode,
                                          std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult setArmMode(FciArmMode s_mode,
                               std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult home(std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult setZero(std::uint8_t s_joint_id,
                            std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult clearError(std::uint8_t s_joint_id,
                               std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult clearFaults(std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult emergencyStop(std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult readMotorRegister(std::uint8_t s_joint_id,
                                      std::uint8_t s_register_id,
                                      std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult writeMotorRegister(std::uint8_t s_joint_id,
                                       std::uint8_t s_register_id,
                                       float s_value,
                                       std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult storeMotorParameters(
        std::uint8_t s_joint_id, std::uint32_t s_timeout_ms) noexcept;
    FciSubmitResult setMotorZero(std::uint8_t s_joint_id,
                                 std::uint32_t s_timeout_ms) noexcept;

    FciEndpointStatus inspectOperation(
        std::uint64_t s_request_id,
        FciOperationResult& s_result) const noexcept;
    // Waits without polling the owner thread. A local wait expiry returns
    // kBusy and leaves the RPC live; kTimeout is reserved for the RPC's own
    // terminal timeout state.
    FciEndpointStatus waitOperation(
        std::uint64_t s_request_id, std::chrono::milliseconds s_wait,
        FciOperationResult& s_result) const noexcept;
    FciEndpointStatus takeOperation(std::uint64_t s_request_id,
                                    FciOperationResult& s_result) noexcept;
    FciEndpointStatus takeDeviceInfo(std::uint64_t s_request_id,
                                     FciOperationResult& s_result,
                                     DeviceInfo& s_info) noexcept;
    FciEndpointStatus takeDeviceSettings(std::uint64_t s_request_id,
                                         FciOperationResult& s_result,
                                         DeviceSettings& s_settings) noexcept;
    FciEndpointStatus takeMotorRegister(std::uint64_t s_request_id,
                                        FciOperationResult& s_result,
                                        float& s_value) noexcept;

    FciControlLeaseSnapshot controlLease() const noexcept;
    FciEndpointStatus latestArmStatus(
        FciArmStatusSnapshot& s_status) const noexcept;
    FciEndpointStatus latestDiagnostics(
        ArmDiagnostics& s_diagnostics) const noexcept;

    FciEndpointStatus sendJointMit(const JointMIT& s_command,
                                   std::uint32_t s_dt_us,
                                   std::uint64_t s_sdk_timestamp_us) noexcept;
    FciEndpointStatus sendGripperMit(
        const JointMIT& s_command, std::uint32_t s_dt_us,
        std::uint64_t s_sdk_timestamp_us) noexcept;
    FciEndpointStatus sendJointPositionVelocity(
        const JointPosVel& s_command,
        std::uint64_t s_sdk_timestamp_us) noexcept;
    FciEndpointStatus sendJointVelocity(
        const JointVel& s_command,
        std::uint64_t s_sdk_timestamp_us) noexcept;
    FciEndpointStatus sendJointPvt(
        const JointPVT& s_command,
        std::uint64_t s_sdk_timestamp_us) noexcept;
    FciEndpointStatus sendCartesianPose(
        const CartesianPose& s_command, std::uint32_t s_dt_us,
        std::uint64_t s_sdk_timestamp_us) noexcept;
    FciEndpointStatus sendCartesianVelocity(
        const CartesianVelocities& s_command, std::uint32_t s_dt_us,
        std::uint64_t s_sdk_timestamp_us) noexcept;
    FciEndpointStatus sendGripperPositionVelocity(
        const JointPosVel& s_command,
        std::uint64_t s_sdk_timestamp_us) noexcept;
    FciEndpointStatus sendGripperVelocity(
        const JointVel& s_command,
        std::uint64_t s_sdk_timestamp_us) noexcept;
    FciEndpointStatus sendGripperPvt(
        const JointPVT& s_command,
        std::uint64_t s_sdk_timestamp_us) noexcept;

    WirelinkExecutorStats executorStats() const noexcept {
        return m_executor.stats();
    }
    FciWirelinkEndpointStats stats() const noexcept;

private:
    enum class RpcKind : std::uint8_t {
        kNone,
        kAcquireLease,
        kReleaseLease,
        kGetDeviceInfo,
        kSetDeviceInfo,
        kGetDeviceSettings,
        kSetDeviceSettings,
        kSetArmControlMode,
        kSetGripperControlMode,
        kSetArmMode,
        kHome,
        kSetZero,
        kClearError,
        kClearFaults,
        kEmergencyStop,
        kMotorRegisterRead,
        kMotorRegisterWrite,
        kMotorStoreParameters,
        kMotorSetZero,
    };

    struct OperationRequest {
        DeviceSettings m_device_settings{};
        std::array<char, s_kMaximumDeviceNameBytes + 1> m_custom_name{};
        std::uint8_t m_custom_name_size{};
        FirmwareType m_firmware_type{FirmwareType::kUnknown};
        std::uint64_t m_lease_token{};
        std::uint32_t m_requested_lease_timeout_ms{};
        std::uint8_t m_joint_id{};
        std::uint8_t m_register_id{};
        FciArmMode m_arm_mode{FciArmMode::kPc};
        FciMotorControlMode m_control_mode{FciMotorControlMode::kMit};
        float m_value{};
    };

    struct FixedDeviceInfo {
        Version m_protocol_version{};
        Version m_firmware_version{};
        std::array<char, s_kMaximumDeviceNameBytes + 1> m_board_name{};
        std::array<char, s_kMaximumDeviceNameBytes + 1> m_custom_name{};
        std::uint8_t m_board_name_size{};
        std::uint8_t m_custom_name_size{};
        FirmwareType m_firmware_type{FirmwareType::kUnknown};
        bool m_valid{};
    };

    struct OperationSlot {
        std::uint64_t m_request_id{};
        std::uint32_t m_operation_id{};
        std::uint32_t m_timeout_ms{};
        OperationRequest m_request{};
        RpcKind m_kind{RpcKind::kNone};
        FciOperationState m_state{FciOperationState::kUnknown};
        FciEndpointStatus m_status{FciEndpointStatus::kNoData};
        std::int32_t m_domain_status{};
        std::int32_t m_link_status{};
        FixedDeviceInfo m_device_info{};
        DeviceSettings m_device_settings{};
        float m_motor_register_value{};
        bool m_device_settings_valid{};
        bool m_motor_register_valid{};
        bool m_internal{};
        bool m_used{};
    };

    struct AtomicStats {
        std::atomic<std::uint64_t> m_dispatch_calls{};
        std::atomic<std::uint64_t> m_dispatch_errors{};
        std::atomic<std::uint64_t> m_runtime_poll_calls{};
        std::atomic<std::uint64_t> m_runtime_timeouts{};
        std::atomic<std::uint64_t> m_latest_acquires{};
        std::atomic<std::uint64_t> m_latest_releases{};
        std::atomic<std::uint64_t> m_rpc_started{};
        std::atomic<std::uint64_t> m_rpc_released{};
        std::atomic<std::uint64_t> m_rpc_cancelled{};
    };

    // The rev4 host profile requires 4,048 bytes for eight 256-byte RPC
    // response slots and three retained telemetry values. Initialization
    // verifies the generated requirements so schema growth fails explicitly.
    static constexpr std::size_t s_kRuntimeStorageSize = 4096;
    static constexpr std::size_t s_kTxPayloadSize = 256;
    static constexpr std::size_t s_kTxUnitSize = 320;
    static constexpr std::size_t s_kControlUnitSize = 64;
    static constexpr std::size_t s_kRxFifoSize = 4096;
    static constexpr std::size_t s_kRxFallbackSize = 320;

    static void s_onEvent(void* s_user_data, wl_ctx_t& s_context,
                          const wl_event_t& s_event) noexcept;
    static bool s_applicationProgress(void* s_user_data, wl_ctx_t& s_context,
                                      wl_time_ms_t s_now_ms) noexcept;
    static std::uint32_t s_applicationDeadline(
        const void* s_user_data, wl_time_ms_t s_now_ms) noexcept;
    static void s_quiesce(void* s_user_data) noexcept;

    static wl_time_ms_t s_nowMs() noexcept;
    static bool s_terminal(FciOperationState s_state) noexcept;
    static FciOperationState s_operationState(
        wl_rpc_client_state_t s_state) noexcept;
    static FciEndpointStatus s_endpointStatus(int s_result) noexcept;
    static std::uint32_t s_until(wl_time_ms_t s_now,
                                 wl_time_ms_t s_target) noexcept;

    bool s_progress(wl_ctx_t& s_context, wl_time_ms_t s_now_ms) noexcept;
    bool s_drainLatest() noexcept;
    bool s_finishActive(wl_ctx_t& s_context,
                        wl_time_ms_t s_now_ms) noexcept;
    bool s_startNext(wl_ctx_t& s_context, wl_time_ms_t s_now_ms) noexcept;
    bool s_scheduleRenewal(wl_time_ms_t s_now_ms) noexcept;
    void s_stopOnOwner() noexcept;
    void s_finalize(std::size_t s_index,
                    const wl_rpc_client_result_t& s_client,
                    wl_ctx_t& s_context,
                    wl_time_ms_t s_now_ms) noexcept;
    void s_releaseRuntimeOperation(RpcKind s_kind,
                                   std::uint32_t s_operation_id) noexcept;
    FciSubmitResult s_submit(RpcKind s_kind, std::uint32_t s_timeout_ms,
                             const OperationRequest& s_request,
                             bool s_internal) noexcept;
    FciEndpointStatus s_commandToken(std::uint64_t& s_token) const noexcept;
    FciEndpointStatus s_submitLatestPayload(
        std::uint16_t s_message_id, const std::uint8_t* s_payload,
        std::size_t s_payload_size) noexcept;
    std::size_t s_findSlot(std::uint64_t s_request_id) const noexcept;
    std::size_t s_findQueuedSlot() const noexcept;
    std::size_t s_allocateSlot(bool s_internal) const noexcept;
    FciOperationResult s_result(const OperationSlot& s_slot) const noexcept;

    WirelinkExecutor m_executor;
    fci_arm_runtime_instance_t m_runtime_instance{};
    alignas(std::max_align_t)
        std::array<std::byte, s_kRuntimeStorageSize> m_runtime_storage{};
    std::array<std::uint8_t, s_kTxPayloadSize> m_tx_payload{};
    std::array<std::uint8_t, s_kTxPayloadSize> m_rpc_encode_scratch{};
    std::array<std::uint8_t, s_kTxUnitSize> m_tx_unit{};
    std::array<std::uint8_t, s_kControlUnitSize> m_control_unit{};
    std::array<std::uint8_t, s_kRxFifoSize> m_rx_fifo{};
    std::array<std::uint8_t, s_kRxFallbackSize> m_rx_fallback{};

    mutable std::mutex m_mutex;
    mutable std::condition_variable m_operation_changed;
    std::array<OperationSlot, s_kOperationCapacity> m_operations{};
    std::size_t m_active_operation{s_kOperationCapacity};
    std::uint64_t m_next_request_id{1};
    FciControlLeaseSnapshot m_lease{};
    wl_time_ms_t m_lease_renew_at_ms{};
    wl_time_ms_t m_lease_expire_at_ms{};
    std::uint32_t m_lease_requested_timeout_ms{};
    FciArmStatusSnapshot m_latest_arm_status{};
    ArmDiagnostics m_latest_diagnostics{};
    std::uint64_t m_diagnostics_generation{};
    ArmStatusCallback m_arm_status_callback{};
    DiagnosticsCallback m_diagnostics_callback{};
    void* m_callback_user_data{};
    bool m_initialized{};
    bool m_running{};
    std::size_t m_runtime_storage_bytes{};
    std::atomic<std::uint32_t> m_command_sequence{};
    AtomicStats m_stats{};
};

} // namespace florid::detail

#endif // FLORID_DETAIL_FCI_WIRELINK_ENDPOINT_HPP
