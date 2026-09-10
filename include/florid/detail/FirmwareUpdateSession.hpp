#pragma once

#include "florid/FirmwareUpdate.hpp"
#include <mutex>

namespace florid::detail {
class FciWirelinkEndpoint;

// Business-thread orchestration over an existing endpoint. No USB ownership,
// second protocol owner, control lease, or firmware confirmation command.
class FirmwareUpdateSession {
public:
    explicit FirmwareUpdateSession(FciWirelinkEndpoint& s_endpoint) : m_endpoint(s_endpoint) {}
    FirmwareDeviceInfoResult deviceInfo(std::chrono::milliseconds s_timeout);
    FirmwareInstallResult rebootAndWait(const Version& s_expected, const FirmwareInstallOptions& s_options);

private:
    FirmwareDeviceInfoResult s_deviceInfo(std::chrono::milliseconds s_timeout);
    FciWirelinkEndpoint& m_endpoint;
    std::mutex m_call_mutex;
    std::uint64_t m_info_request{}; // at most one outstanding query, including local timeout
};
} // namespace florid::detail
