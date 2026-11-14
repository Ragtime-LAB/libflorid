// Native USB backend for the transport-neutral Python UnderwaterSingle API.
#include "florid/UsbDiscovery.hpp"
#include "florid/detail/AstrialBulkTransport.hpp"
#include "florid/detail/FciWirelinkEndpoint.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <mutex>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;
using namespace florid;
using namespace florid::detail;
namespace {
template <class T> py::dict state_dict(const T &s) {
    py::dict out;
    out["mode"] = s.mode;
    out["kind"] = s.kind;
    out["sequence"] = s.sequence;
    out["timestamp_us"] = s.timestamp_us;
    out["control_session"] = s.control_session;
    out["applied_command_sequence"] = s.applied_command_sequence;
    out["applied_mask"] = s.applied_mask;
    out["fresh_mask"] = s.fresh_mask;
    out["enabled_mask"] = s.enabled_mask;
    out["available_mask"] = s.available_mask;
    out["session"] = s.control_session;
    out["applied_sequence"] = s.applied_command_sequence;
    out["host_timestamp_us"] = s.last_sdk_timestamp_us;
    out["flags"] = int(s.command_received) | (int(s.upgrading) << 1);
    out["stop_reason"] = s.control_stop_reason;
    out["reject_reason"] = s.command_reject_reason;
    out["source"] = s.control_source;
    out["enabling"] = s.enabling;
    out["q"] = std::array<float, 4>{s.q[0], s.q[1], s.q[2], s.q[3]};
    out["dq"] = std::array<float, 4>{s.dq[0], s.dq[1], s.dq[2], s.dq[3]};
    out["tau"] = std::array<float, 4>{s.tau[0], s.tau[1], s.tau[2], s.tau[3]};
    std::array<unsigned, 4> temperature{}, status{}, age{};
    for (size_t i = 0; i < 4; ++i) {
        temperature[i] = s.motor_info[i] & 255;
        status[i] = (s.motor_info[i] >> 8) & 255;
        age[i] = s.feedback_age_ms[i];
    }
    out["temperature"] = temperature;
    out["motor_status"] = status;
    out["age_ms"] = age;
    return out;
}
class SingleUsb {
    std::unique_ptr<Transport> m_transport;
    FciWirelinkEndpoint m_endpoint;
    std::mutex m_rpc;
    std::vector<uint64_t> m_retained;
    void reap() {
        std::lock_guard lock(m_rpc);
        FciOperationResult result{};
        std::erase_if(m_retained, [&](uint64_t id) {
            return m_endpoint.takeOperation(id, result) != FciEndpointStatus::kBusy;
        });
    }
    void retain(uint64_t id) {
        std::lock_guard lock(m_rpc);
        m_retained.push_back(id);
    }

  public:
    explicit SingleUsb(const std::string &uri) {
        const auto selection = resolveUsbBulkDevice(uri);
        if (!selection)
            throw std::runtime_error(usbDiscoveryErrorMessage(selection.m_error));
        const auto &d = *selection.m_device;
        m_transport = std::make_unique<AstrialBulkTransport>(d.m_vendor_id, d.m_product_id,
                                                             d.m_serial_number, d.m_port_path);
        if (m_endpoint.initialize() != FciEndpointStatus::kOk ||
            m_endpoint.attachDirectTransport(*m_transport) != FciEndpointStatus::kOk ||
            m_endpoint.start() != FciEndpointStatus::kOk)
            throw std::runtime_error(
                "USB Wirelink open failed; inspect device permissions and USB interface");
    }
    ~SingleUsb() { m_endpoint.stop(); }
    void close() { m_endpoint.stop(); }
    py::dict request(uint8_t op, uint8_t mode, const std::array<float, 3> &gravity, int timeout) {
        single_response_value_t response{};
        {
            py::gil_scoped_release release;
            if (timeout <= 0)
                throw std::invalid_argument("timeout must be positive");
            reap();
            FciOperationResult operation;
            single_request_value_t request{};
            request.has_operation = request.has_mode = request.has_gravity_scale = true;
            request.operation = op;
            request.mode = mode;
            std::copy(gravity.begin(), gravity.end(), request.gravity_scale);
            const auto submitted = m_endpoint.singleExchange(request, timeout);
            if (submitted.m_status != FciEndpointStatus::kOk)
                throw std::runtime_error("USB request could not be submitted");
            const auto request_id = submitted.m_request_id;
            (void)m_endpoint.waitOperation(request_id, std::chrono::milliseconds(timeout), operation);
            const auto status = m_endpoint.takeSingleResponse(request_id, operation, response);
            if (status == FciEndpointStatus::kBusy)
                retain(request_id);
            if (status != FciEndpointStatus::kOk && status != FciEndpointStatus::kDomainError)
                throw std::runtime_error("USB response missing (status=" + std::to_string(int(status)) +
                                         "); execution unknown; query actual mode before retrying");
        }
        py::dict out;
        out["result"] = response.result;
        out["state"] = state_dict(response.state);
        out["boot_id"] = response.boot_id;
        out["uid"] = std::string(response.uid.data, response.uid.length);
        out["role"] = response.role;
        out["gravity_scale"] = std::array<float, 3>{response.gravity_scale[0], response.gravity_scale[1],
                                                    response.gravity_scale[2]};
        out["revision"] = response.revision;
        out["storage_error"] = response.storage_error;
        out["disabled_mask"] = response.disabled_mask;
        out["disable_pending_mask"] = response.disable_pending_mask;
        out["reason"] = std::string(response.reason.data, response.reason.length);
        return out;
    }
    uint64_t rx_count() const { return m_endpoint.singleRxCount(); }
    py::object state() {
        const auto value = m_endpoint.singleState();
        return value ? py::object(state_dict(*value)) : py::object(py::none());
    }
    void write(const std::array<std::array<float, 4>, 5> &values, uint8_t kind, uint64_t session,
               uint32_t sequence, uint64_t timestamp) {
        if (!session || (kind != 1 && kind != 2))
            throw std::invalid_argument("valid session and MIT kind required");
        single_command_t command{};
        float *targets[]{command.q, command.dq, command.kp, command.kd, command.tau};
        for (size_t v = 0; v < 5; ++v)
            for (size_t i = 0; i < 4; ++i) {
                if (!std::isfinite(values[v][i]) || ((v == 2 || v == 3) && values[v][i] < 0))
                    throw std::invalid_argument("finite values and nonnegative gains required");
                targets[v][i] = values[v][i];
            }
        command.has_q = command.has_dq = command.has_kp = command.has_kd = command.has_tau = true;
        command.has_kind = command.has_sequence = command.has_control_session =
            command.has_sdk_timestamp_us = true;
        command.kind = underwater_control_kind_t(kind);
        command.sequence = sequence;
        command.control_session = session;
        command.sdk_timestamp_us = timestamp;
        if (m_endpoint.sendSingleCommand(command) != FciEndpointStatus::kOk)
            throw std::runtime_error("USB command TX queue unavailable");
    }
    auto device_info(int timeout) {
        reap();
        FciOperationResult result{};
        DeviceInfo info;
        const auto id = m_endpoint.getDeviceInfo(timeout);
        if (id.m_status != FciEndpointStatus::kOk)
            throw std::runtime_error("device info submit failed");
        (void)m_endpoint.waitOperation(id.m_request_id, std::chrono::milliseconds(timeout), result);
        const auto status = m_endpoint.takeDeviceInfo(id.m_request_id, result, info);
        if (status == FciEndpointStatus::kBusy)
            retain(id.m_request_id);
        if (status != FciEndpointStatus::kOk)
            throw std::runtime_error("device info read failed");
        return info;
    }
    auto boot_status(int timeout) {
        return m_endpoint.upgrade().bootStatus(std::chrono::milliseconds(timeout));
    }
    auto progress() { return m_endpoint.upgrade().progress(); }
    void cancel_upload() { m_endpoint.upgrade().cancel(); }
    auto start_upload(const py::bytes &bytes) {
        const std::string data = bytes;
        py::gil_scoped_release release;
        return m_endpoint.upgrade().startUpload(
            std::span(reinterpret_cast<const uint8_t *>(data.data()), data.size()), {});
    }
    auto reboot(const Version &version) {
        return m_endpoint.firmwareSession().rebootAndWait(version, {});
    }
};
} // namespace
void bind_single(py::module_ &m) {
    py::class_<SingleUsb>(m, "_SingleUsb")
        .def(py::init<const std::string &>())
        .def("request", &SingleUsb::request, py::arg("operation"), py::arg("mode") = 0,
             py::arg("gravity") = std::array<float, 3>{}, py::arg("timeout_ms") = 2000)
        .def_property_readonly("rx_count", &SingleUsb::rx_count)
        .def("state", &SingleUsb::state)
        .def("write_command", &SingleUsb::write)
        .def("device_info", &SingleUsb::device_info, py::arg("timeout_ms") = 2000,
             py::call_guard<py::gil_scoped_release>())
        .def("close", &SingleUsb::close, py::call_guard<py::gil_scoped_release>())
        .def("boot_status", &SingleUsb::boot_status, py::arg("timeout_ms") = 2000,
             py::call_guard<py::gil_scoped_release>())
        .def("start_upload", &SingleUsb::start_upload)
        .def("firmware_progress", &SingleUsb::progress)
        .def("cancel_upload", &SingleUsb::cancel_upload)
        .def("reboot_and_wait", &SingleUsb::reboot, py::call_guard<py::gil_scoped_release>());
}
