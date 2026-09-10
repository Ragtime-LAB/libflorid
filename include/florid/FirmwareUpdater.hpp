#pragma once

#include "florid/FirmwareUpdate.hpp"
#include "florid/DeviceDiscovery.hpp"
#include <memory>
#include <span>
#include <string>

namespace florid {
class Arm;
class ArmImpl;
struct FirmwareConnectionResult;

enum class FirmwareConnectionError {
    kNone, kInvalidSelector, kEnumerationFailed, kDeviceNotFound, kAmbiguous,
    kPermissionDenied, kBusy, kTransportError, kTimeout, kProtocolMismatch,
    kIdentityMismatch, kProbeFailed,
};

// Shared/standalone upgrade management. Upload completion only means the sink
// accepted the object; rebootAndWait() separately observes installation.
class FirmwareUpdater {
public:
    // Recovery entry: one USB Bulk session, no Arm::create(), lease, motor
    // initialization, or telemetry requirement. nullptr means open/probe failed.
    static std::unique_ptr<FirmwareUpdater> create(const std::string& s_usb_uri);
    // Standalone management connection. Probes identity/protocol in this same
    // session, without acquiring an Arm control lease. Descriptor/URI opens
    // preserve USB errors; selectors reject ambiguous matches.
    static FirmwareConnectionResult connect(const DeviceSelector& s_selector = {},
        std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    static FirmwareConnectionResult connect(const DeviceDescriptor& s_device,
        std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    static FirmwareConnectionResult connect(const std::string& s_usb_uri,
        std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    ~FirmwareUpdater();
    FirmwareUpdater(FirmwareUpdater&&) noexcept;
    FirmwareUpdater& operator=(FirmwareUpdater&&) noexcept;
    FirmwareUpdater(const FirmwareUpdater&) = delete;
    FirmwareUpdater& operator=(const FirmwareUpdater&) = delete;

    BootStatus bootStatus(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    FirmwareDeviceInfoResult deviceInfo(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    // Waits for response acceptance AND observed departure, keeping the owner
    // alive for the response ACK. Neither departure nor a timeout proves boot.
    FirmwareRebootResult reboot(FirmwareRebootMode s_mode = FirmwareRebootMode::kTryBoot,
                               std::chrono::milliseconds s_timeout = std::chrono::seconds(5));
    // Explicit test reboot, then bounded read-only verification over automatic
    // USB reconnect (same serial/port, same endpoint, including Arm views).
    // Requires a DIFFERENT expected FCI major/minor/patch version; equal versions
    // return InvalidArgument before reboot since FCI has no running image hash.
    // Never retries reboot/upload, confirms an image, or reacquires control.
    FirmwareInstallResult rebootAndWait(const Version& s_expected_version,
                                        const FirmwareInstallOptions& s_options = {});
    // Copies the image. Call from a business thread after stopping control.
    FirmwareUpdateError startUpload(std::span<const std::uint8_t> s_image,
                                    const FirmwareUploadOptions& s_options = {});
    FirmwareUploadProgress progress() const;
    void cancel(); // asynchronous; poll progress until terminal before retrying

    // Blocking queries/reboot/verification belong on a business thread, never
    // the control callback. Stop control first and serialize mode transitions
    // across Arm/updater views. progress()/cancel() may run concurrently.

private:
    FirmwareUpdater();
    explicit FirmwareUpdater(std::shared_ptr<ArmImpl> s_arm);
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    friend class Arm;
};

struct FirmwareConnectionResult {
    std::unique_ptr<FirmwareUpdater> m_updater;
    FirmwareConnectionError m_error{FirmwareConnectionError::kNone};
    FirmwareUpdateError m_probe_error{FirmwareUpdateError::kNone};
    std::int32_t m_device_status{};
    std::error_code m_system_error;
    std::string m_error_message;
    std::optional<DeviceDescriptor> m_device;
    std::vector<DeviceDescriptor> m_candidates;
    [[nodiscard]] explicit operator bool() const noexcept {
        return m_error == FirmwareConnectionError::kNone && m_updater != nullptr;
    }
};
} // namespace florid
