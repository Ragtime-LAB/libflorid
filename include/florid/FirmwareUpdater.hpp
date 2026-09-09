#pragma once

#include "florid/FirmwareUpdate.hpp"
#include <memory>
#include <span>
#include <string>

namespace florid {
class Arm;
class ArmImpl;

// Upload transport only: completion means the device sink accepted/verified
// the object, not that MCUboot installed or confirmed a bootable image.
class FirmwareUpdater {
public:
    // Recovery entry: one USB Bulk session, no Arm::create(), lease, motor
    // initialization, or telemetry requirement. nullptr means open failed.
    static std::unique_ptr<FirmwareUpdater> create(const std::string& s_usb_uri);
    ~FirmwareUpdater();
    FirmwareUpdater(FirmwareUpdater&&) noexcept;
    FirmwareUpdater& operator=(FirmwareUpdater&&) noexcept;
    FirmwareUpdater(const FirmwareUpdater&) = delete;
    FirmwareUpdater& operator=(const FirmwareUpdater&) = delete;

    BootStatus bootStatus(std::chrono::milliseconds s_timeout = std::chrono::seconds(2));
    // Copies the image. Call from a business thread after stopping control.
    FirmwareUpdateError startUpload(std::span<const std::uint8_t> s_image,
                                    const FirmwareUploadOptions& s_options = {});
    FirmwareUploadProgress progress() const;
    void cancel(); // asynchronous; poll progress until terminal before retrying

private:
    FirmwareUpdater();
    explicit FirmwareUpdater(std::shared_ptr<ArmImpl> s_arm);
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    friend class Arm;
};
} // namespace florid
