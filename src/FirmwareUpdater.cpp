#include "florid/FirmwareUpdater.hpp"
#include "florid/detail/ArmImpl.hpp"
#include "florid/detail/AstrialBulkTransport.hpp"
#include "florid/detail/FciWirelinkEndpoint.hpp"

namespace florid {
struct FirmwareUpdater::Impl {
    // Shared Arm views keep the existing session alive; they never open USB.
    std::shared_ptr<ArmImpl> m_arm;
    std::unique_ptr<AstrialBulkTransport> m_transport;
    std::unique_ptr<detail::FciWirelinkEndpoint> m_endpoint;
    detail::FciUpgradeClient* m_client{};
    ~Impl() { if (m_endpoint) m_endpoint->stop(); }
};

FirmwareUpdater::FirmwareUpdater() : m_impl(std::make_unique<Impl>()) {}
FirmwareUpdater::FirmwareUpdater(std::shared_ptr<ArmImpl> s_arm) : FirmwareUpdater() {
    m_impl->m_arm = std::move(s_arm);
    if (m_impl->m_arm) m_impl->m_client = &m_impl->m_arm->upgradeClient();
}
FirmwareUpdater::~FirmwareUpdater() = default;
FirmwareUpdater::FirmwareUpdater(FirmwareUpdater&&) noexcept = default;
FirmwareUpdater& FirmwareUpdater::operator=(FirmwareUpdater&&) noexcept = default;

std::unique_ptr<FirmwareUpdater> FirmwareUpdater::create(const std::string& s_usb_uri) {
    const auto s_selection = resolveUsbBulkDevice(s_usb_uri);
    if (!s_selection) return nullptr;
    try {
        auto s_result = std::unique_ptr<FirmwareUpdater>(new FirmwareUpdater());
        auto& s_impl = *s_result->m_impl;
        const auto& s_device = *s_selection.m_device;
        s_impl.m_transport = std::make_unique<AstrialBulkTransport>(s_device.m_vendor_id,
            s_device.m_product_id, s_device.m_serial_number, s_device.m_port_path);
        s_impl.m_endpoint = std::make_unique<detail::FciWirelinkEndpoint>();
        if (s_impl.m_endpoint->initialize() != detail::FciEndpointStatus::kOk ||
            s_impl.m_endpoint->attachDirectTransport(*s_impl.m_transport) != detail::FciEndpointStatus::kOk ||
            s_impl.m_endpoint->start() != detail::FciEndpointStatus::kOk) return nullptr;
        s_impl.m_client = &s_impl.m_endpoint->upgrade();
        return s_result;
    } catch (const std::exception&) { return nullptr; }
}

BootStatus FirmwareUpdater::bootStatus(std::chrono::milliseconds s_timeout) {
    return m_impl && m_impl->m_client ? m_impl->m_client->bootStatus(s_timeout)
        : BootStatus{.m_error = FirmwareUpdateError::kNotConnected};
}
FirmwareUpdateError FirmwareUpdater::startUpload(std::span<const std::uint8_t> s_image,
                                                const FirmwareUploadOptions& s_options) {
    if (m_impl && m_impl->m_arm && !m_impl->m_arm->firmwareUploadAllowed())
        return FirmwareUpdateError::kBusy;
    return m_impl && m_impl->m_client ? m_impl->m_client->startUpload(s_image, s_options)
        : FirmwareUpdateError::kNotConnected;
}
FirmwareUploadProgress FirmwareUpdater::progress() const {
    return m_impl && m_impl->m_client ? m_impl->m_client->progress()
        : FirmwareUploadProgress{.m_state = FirmwareUploadState::kFailed, .m_error = FirmwareUpdateError::kNotConnected};
}
void FirmwareUpdater::cancel() { if (m_impl && m_impl->m_client) m_impl->m_client->cancel(); }
} // namespace florid
