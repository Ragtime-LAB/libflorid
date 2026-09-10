#pragma once

#include "florid/FirmwareUpdate.hpp"
#include <fci_device_endpoint.h>
#include <wirelink/bulk.h>

#include <condition_variable>
#include <mutex>
#include <span>
#include <vector>

namespace florid::detail {

// One service on FciWirelinkEndpoint's existing executor. All Wirelink calls
// are owner-only; callers submit copied data and inspect locked snapshots.
class FciUpgradeClient {
public:
    void configure(fci_device_endpoint_config_t& s_config);
    void attach(fci_device_endpoint_t& s_endpoint, void (*s_notify)(void*), void* s_context);
    void connected(bool s_running);
    void disconnected(bool s_departed = false); // owner only; local close is NOT a reboot
    void transportState(bool s_connected); // owner only; edge-triggered loss
    std::uint64_t peerSession() const { std::lock_guard s_lock(m_mutex); return m_peer_session; }
    FirmwareUpdateError beginVerification();
    void endVerification();
    const wl_endpoint_service_t& service() const { return m_service; }
    BootStatus bootStatus(std::chrono::milliseconds s_timeout);
    FirmwareRebootResult reboot(FirmwareRebootMode s_mode, std::chrono::milliseconds s_timeout,
                                bool s_verification = false);
    FirmwareUpdateError startUpload(std::span<const std::uint8_t> s_image, const FirmwareUploadOptions& s_options);
    FirmwareUploadProgress progress() const;
    void cancel();
    bool active() const { std::lock_guard s_lock(m_mutex); return m_progress.active() || m_reboot_pending || m_verifying; }

private:
    mutable std::mutex m_mutex;
    std::mutex m_boot_call_mutex;
    std::mutex m_reboot_call_mutex;
    std::condition_variable m_changed;
    fci_device_endpoint_t* m_endpoint{};
    void (*m_notify)(void*){};
    void* m_notify_context{};
    wl_endpoint_service_t m_service{};
    wl_bulk_sender_t m_sender{};
    std::vector<std::uint8_t> m_image;
    FirmwareUploadOptions m_options{};
    FirmwareUploadProgress m_progress{};
    std::uint32_t m_crc{}, m_start_id{}, m_boot_id{}, m_since_ms{}, m_boot_since_ms{}, m_boot_timeout_ms{};
    bool m_running{}, m_cancel{}, m_boot_pending{}, m_boot_done{};
    BootStatus m_boot{};
    FirmwareRebootResult m_reboot{};
    FirmwareRebootMode m_reboot_mode{FirmwareRebootMode::kTryBoot};
    std::uint32_t m_reboot_id{}, m_reboot_since_ms{}, m_reboot_timeout_ms{};
    bool m_reboot_pending{}, m_reboot_done{};
    bool m_transport_connected{true}, m_verifying{};
    std::uint64_t m_peer_session{};

    fci_device_runtime_t* s_runtime() { return fci_device_endpoint_runtime(m_endpoint); }
    void s_cancelRpc(std::uint32_t& s_id);
    void s_finish(FirmwareUpdateError s_error);
    void s_progress(std::uint32_t s_now);
    void s_pollBoot(std::uint32_t s_now);
    void s_pollReboot(std::uint32_t s_now);
    void s_pollStart(std::uint32_t s_now);
    static std::int32_t s_status(void*, const bulk_status_t*, wl_delivery_t);
    static std::uint32_t s_now();
};

} // namespace florid::detail
