#include <pybind11/chrono.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "florid/DeviceDiscovery.hpp"
#include "florid/DeviceTypes.hpp"

#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace py = pybind11;

void bind_underwater(py::module_& m);
void bind_single(py::module_& m);

PYBIND11_MODULE(_pyflorid, m) {
    m.doc() = "Python SDK for the Ragtime Underwater dual-arm controller";

    py::enum_<florid::FirmwareType>(m, "FirmwareType")
        .value("StandardArm", florid::FirmwareType::kStandardArm)
        .value("MobileArm", florid::FirmwareType::kMobileArm)
        .value("CobotArm", florid::FirmwareType::kCobotArm)
        .value("Unknown", florid::FirmwareType::kUnknown);

    py::enum_<florid::DeviceAccessStatus>(m, "DeviceAccessStatus")
        .value("NotProbed", florid::DeviceAccessStatus::kNotProbed)
        .value("Ready", florid::DeviceAccessStatus::kReady)
        .value("PermissionDenied", florid::DeviceAccessStatus::kPermissionDenied)
        .value("Busy", florid::DeviceAccessStatus::kBusy)
        .value("Disconnected", florid::DeviceAccessStatus::kDisconnected)
        .value("Unavailable", florid::DeviceAccessStatus::kUnavailable);

    py::enum_<florid::DeviceCompatibility>(m, "DeviceCompatibility")
        .value("Unknown", florid::DeviceCompatibility::kUnknown)
        .value("Compatible", florid::DeviceCompatibility::kCompatible)
        .value("ProtocolMismatch", florid::DeviceCompatibility::kProtocolMismatch)
        .value("IdentityMismatch", florid::DeviceCompatibility::kIdentityMismatch)
        .value("ProbeFailed", florid::DeviceCompatibility::kProbeFailed);

    py::class_<florid::Version>(m, "Version")
        .def(py::init<>())
        .def_readwrite("major", &florid::Version::m_major)
        .def_readwrite("minor", &florid::Version::m_minor)
        .def_readwrite("patch", &florid::Version::m_patch);

    py::class_<florid::DeviceInfo>(m, "DeviceInfo")
        .def(py::init<>())
        .def_readonly("protocol_version", &florid::DeviceInfo::m_protocol_version)
        .def_readonly("fw_version", &florid::DeviceInfo::m_firmware_version)
        .def_readonly("board_name", &florid::DeviceInfo::m_board_name)
        .def_readonly("custom_name", &florid::DeviceInfo::m_custom_name)
        .def_readonly("fw_type", &florid::DeviceInfo::m_firmware_type)
        .def_readonly("serial_number", &florid::DeviceInfo::m_serial_number);

    py::class_<florid::DeviceDescriptor>(m, "DeviceDescriptor")
        .def_property_readonly("vendor_id", [](const florid::DeviceDescriptor& s_device) { return s_device.m_usb.m_vendor_id; })
        .def_property_readonly("product_id", [](const florid::DeviceDescriptor& s_device) { return s_device.m_usb.m_product_id; })
        .def_property_readonly("bus_number", [](const florid::DeviceDescriptor& s_device) { return s_device.m_usb.m_bus_number; })
        .def_property_readonly("device_address", [](const florid::DeviceDescriptor& s_device) { return s_device.m_usb.m_device_address; })
        .def_property_readonly("port_path", [](const florid::DeviceDescriptor& s_device) { return s_device.m_usb.m_port_path; })
        .def_property_readonly("manufacturer", [](const florid::DeviceDescriptor& s_device) { return s_device.m_usb.m_manufacturer; })
        .def_property_readonly("product", [](const florid::DeviceDescriptor& s_device) { return s_device.m_usb.m_product; })
        .def_property_readonly("serial_number", [](const florid::DeviceDescriptor& s_device) { return s_device.serialNumber(); })
        .def_property_readonly("uri", [](const florid::DeviceDescriptor& s_device) { return s_device.uri(); })
        .def_readonly("display_name", &florid::DeviceDescriptor::m_display_name)
        .def_readonly("device_info", &florid::DeviceDescriptor::m_device_info)
        .def_readonly("access", &florid::DeviceDescriptor::m_access)
        .def_readonly("compatibility", &florid::DeviceDescriptor::m_compatibility)
        .def_readonly("error_message", &florid::DeviceDescriptor::m_error_message);

    py::class_<florid::DeviceSelector>(m, "DeviceSelector")
        .def(py::init<>())
        .def_readwrite("serial_number", &florid::DeviceSelector::m_serial_number)
        .def_readwrite("custom_name", &florid::DeviceSelector::m_custom_name)
        .def_readwrite("firmware_type", &florid::DeviceSelector::m_firmware_type)
        .def_static("by_serial", &florid::DeviceSelector::bySerial)
        .def_static("by_custom_name", &florid::DeviceSelector::byCustomName);

    m.def("discover_devices", [](bool s_probe, std::uint32_t s_timeout_ms) {
        const auto s_result = florid::discoverDevices({
            .m_probe = s_probe,
            .m_probe_timeout = std::chrono::milliseconds{s_timeout_ms},
        });
        if (!s_result) {
            throw std::runtime_error(florid::deviceDiscoveryErrorMessage(s_result.m_error));
        }
        return s_result.m_devices;
    }, py::arg("probe") = false, py::arg("probe_timeout_ms") = 1500);

    m.def("wait_for_device", [](const florid::DeviceSelector& s_selector, std::uint32_t s_timeout_ms) {
        auto s_result = florid::waitForDevice(s_selector, std::chrono::milliseconds{s_timeout_ms});
        if (!s_result) {
            throw std::runtime_error(florid::deviceDiscoveryErrorMessage(s_result.m_error));
        }
        return *s_result.m_device;
    }, py::arg("selector"), py::arg("timeout_ms") = 10'000);

    bind_underwater(m);
    bind_single(m);
}
