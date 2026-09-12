#pragma once

#include "florid/DeviceDiscovery.hpp"
#include "florid/FirmwareUpdate.hpp"
#include "florid/UnderwaterTypes.hpp"
#include <chrono>
#include <memory>
#include <span>
#include <string>

namespace florid {
class Transport;
struct UnderwaterConnectionResult;
namespace detail { struct UnderwaterTeleopTestAccess; }

struct UnderwaterFirmwareUpdateResult {
    FirmwareUploadProgress m_upload;
    std::optional<FirmwareInstallResult> m_install;
    [[nodiscard]] explicit operator bool() const noexcept {
        return m_upload.m_state == FirmwareUploadState::kCompleted &&
               m_install.has_value() && static_cast<bool>(*m_install);
    }
};

// Dual four-axis arms plus grippers, using one Wirelink connection for mode
// RPCs, ten-motor telemetry/commands and upgrades. No six-axis Arm lease is used.
class UnderwaterTeleop {
public:
    // Explicit Bulk URI/descriptor, or serial:// CDC path. These methods open
    // a connection and query status only; they never enable motors.
    static UnderwaterConnectionResult connect(const std::string& s_uri,
        std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    static UnderwaterConnectionResult connect(const DeviceDescriptor& s_device,
        std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    static std::unique_ptr<UnderwaterTeleop> create(const std::string& s_uri);

    ~UnderwaterTeleop();
    UnderwaterTeleop(UnderwaterTeleop&&) noexcept;
    UnderwaterTeleop& operator=(UnderwaterTeleop&&) noexcept;
    UnderwaterTeleop(const UnderwaterTeleop&) = delete;
    UnderwaterTeleop& operator=(const UnderwaterTeleop&) = delete;
    // Local close does not send Disable. Firmware leaves Control on disconnect or
    // command timeout; Gravity/Teleop keep their existing disconnect behavior.
    // Serialize close/move with other operations on this object.
    void close();

    UnderwaterRpcResult status(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    UnderwaterRpcResult enable(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    UnderwaterRpcResult disable(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    // Enter through RPC, then publish commands every 4 ms. The firmware stops
    // Control after >100 ms without a new valid command or on USB disconnect.
    UnderwaterRpcResult enterControl(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    std::optional<UnderwaterArmStatus> readArmStatus() const;
    UnderwaterCommandResult writeArmCommand(const UnderwaterArmCommand& s_command);
    // Set every configured motor's CURRENT pose as zero. Requires Damp.
    // Success means post-zero feedback arrived; failure contains missing motor masks.
    UnderwaterRpcResult zeroing(std::chrono::milliseconds s_timeout = std::chrono::seconds(5));
    UnderwaterRpcResult enterTeleop(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));

    UnderwaterSettingsRpcResult settings(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    // Reads firmware defaults without changing active or persisted settings.
    UnderwaterSettingsRpcResult defaultSettings(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    // One transaction: persist to NVS, then apply. No SDK mode change or gate.
    // Inspect the returned revision/source/reason, including storage failures.
    UnderwaterSettingsRpcResult setSettings(const UnderwaterSettings& s_settings,
        std::chrono::milliseconds s_timeout = std::chrono::seconds(5));

    FirmwareDeviceInfoResult deviceInfo(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    BootStatus bootStatus(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    FirmwareUpdateError startFirmwareUpload(std::span<const std::uint8_t> s_image,
        const FirmwareUploadOptions& s_options = {});
    FirmwareUploadProgress firmwareProgress() const;
    void cancelFirmwareUpload();
    // Upload completion is image staging, not installation. The firmware
    // enters Damp and confirms the previously online motors are disabled first.
    FirmwareUploadProgress uploadFirmware(std::span<const std::uint8_t> s_image,
        const FirmwareUploadOptions& s_options = {});
    FirmwareRebootResult reboot(FirmwareRebootMode s_mode = FirmwareRebootMode::kTryBoot,
        std::chrono::milliseconds s_timeout = std::chrono::seconds(5));
    FirmwareInstallResult rebootAndWait(const Version& s_expected_version,
        const FirmwareInstallOptions& s_options = {});
    // Existing upgrade verification requires a different major/minor/patch.
    // Same-version rebuilds can use uploadFirmware()/reboot()/bootStatus().
    UnderwaterFirmwareUpdateResult updateFirmware(std::span<const std::uint8_t> s_image,
        const Version& s_expected_version, const FirmwareUploadOptions& s_upload_options = {},
        const FirmwareInstallOptions& s_install_options = {});

private:
    UnderwaterTeleop();
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    UnderwaterRpcResult s_mode(UnderwaterMode s_mode, std::chrono::milliseconds s_timeout);
    static UnderwaterConnectionResult s_connectTransport(std::unique_ptr<Transport> s_transport,
        std::chrono::milliseconds s_timeout);
    friend struct detail::UnderwaterTeleopTestAccess;
};

struct UnderwaterConnectionResult {
    std::unique_ptr<UnderwaterTeleop> m_device;
    UnderwaterRpcError m_error{UnderwaterRpcError::kNone};
    std::error_code m_system_error;
    std::string m_message;
    UnderwaterRpcResult m_probe;
    [[nodiscard]] explicit operator bool() const noexcept {
        return m_device != nullptr && m_error == UnderwaterRpcError::kNone;
    }
};
} // namespace florid
