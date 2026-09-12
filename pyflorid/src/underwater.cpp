#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <florid/UnderwaterTeleop.hpp>

namespace py = pybind11;
using namespace florid;
using Ms = std::chrono::milliseconds;

void bind_underwater(py::module_& m) {
    py::enum_<UnderwaterSettingsBusinessStatus>(m, "UnderwaterSettingsBusinessStatus")
        .value("Ok", UnderwaterSettingsBusinessStatus::kOk).value("Busy", UnderwaterSettingsBusinessStatus::kBusy)
        .value("StorageFailed", UnderwaterSettingsBusinessStatus::kStorageFailed)
        .value("InternalError", UnderwaterSettingsBusinessStatus::kInternalError);
    py::enum_<UnderwaterSettingsSource>(m, "UnderwaterSettingsSource")
        .value("Defaults", UnderwaterSettingsSource::kDefaults).value("Stored", UnderwaterSettingsSource::kStored)
        .value("Corrupt", UnderwaterSettingsSource::kCorrupt).value("StorageError", UnderwaterSettingsSource::kStorageError);
    py::class_<UnderwaterSettings>(m, "UnderwaterSettings")
        .def(py::init<>())
        .def_readwrite("angle_scale", &UnderwaterSettings::m_angle_scale)
        .def_readwrite("angle_offset_rad", &UnderwaterSettings::m_angle_offset_rad)
        .def_readwrite("kp", &UnderwaterSettings::m_kp)
        .def_readwrite("kd", &UnderwaterSettings::m_kd)
        .def_readwrite("gravity_scale", &UnderwaterSettings::m_gravity_scale)
        .def_readwrite("friction_coulomb_positive_nm", &UnderwaterSettings::m_friction_coulomb_positive_nm)
        .def_readwrite("friction_coulomb_negative_nm", &UnderwaterSettings::m_friction_coulomb_negative_nm)
        .def_readwrite("friction_viscous_nm_per_rad_s", &UnderwaterSettings::m_friction_viscous_nm_per_rad_s)
        .def_readwrite("friction_deadband_rad_s", &UnderwaterSettings::m_friction_deadband_rad_s)
        .def_readwrite("friction_transition_rad_s", &UnderwaterSettings::m_friction_transition_rad_s)
        .def_readwrite("friction_filter_time_constant_s", &UnderwaterSettings::m_friction_filter_time_constant_s)
        .def_readwrite("force_gain", &UnderwaterSettings::m_force_gain)
        .def_readwrite("force_low_gain", &UnderwaterSettings::m_force_low_gain)
        .def_readwrite("force_low_load_nm", &UnderwaterSettings::m_force_low_load_nm)
        .def_readwrite("force_high_load_nm", &UnderwaterSettings::m_force_high_load_nm)
        .def_readwrite("force_velocity_filter_s", &UnderwaterSettings::m_force_velocity_filter_s)
        .def_readwrite("force_load_filter_s", &UnderwaterSettings::m_force_load_filter_s)
        .def_readwrite("buzzer_alarm_enabled", &UnderwaterSettings::m_buzzer_alarm_enabled);
    py::class_<UnderwaterSettingsSnapshot>(m, "UnderwaterSettingsSnapshot")
        .def_readonly("settings", &UnderwaterSettingsSnapshot::m_settings)
        .def_readonly("revision", &UnderwaterSettingsSnapshot::m_revision)
        .def_readonly("source", &UnderwaterSettingsSnapshot::m_source)
        .def_readonly("storage_error", &UnderwaterSettingsSnapshot::m_storage_error);
    py::class_<UnderwaterSettingsRpcResult>(m, "UnderwaterSettingsRpcResult")
        .def_readonly("error", &UnderwaterSettingsRpcResult::m_error)
        .def_readonly("device_status", &UnderwaterSettingsRpcResult::m_device_status)
        .def_readonly("snapshot", &UnderwaterSettingsRpcResult::m_snapshot)
        .def_readonly("reason", &UnderwaterSettingsRpcResult::m_reason)
        .def("__bool__", [](const UnderwaterSettingsRpcResult& s_result) { return bool(s_result); });
    py::enum_<UnderwaterMode>(m, "UnderwaterMode")
        .value("Damp", UnderwaterMode::kDamp).value("Gravity", UnderwaterMode::kGravity)
        .value("Teleop", UnderwaterMode::kTeleop).value("Control", UnderwaterMode::kControl)
        .value("Usb", UnderwaterMode::kUsb)
        .value("Zeroing", UnderwaterMode::kZeroing).value("Retract", UnderwaterMode::kRetract);
    py::enum_<UnderwaterBusinessStatus>(m, "UnderwaterBusinessStatus")
        .value("Ok", UnderwaterBusinessStatus::kOk)
        .value("InvalidMode", UnderwaterBusinessStatus::kInvalidMode)
        .value("InvalidState", UnderwaterBusinessStatus::kInvalidState)
        .value("Busy", UnderwaterBusinessStatus::kBusy)
        .value("Rejected", UnderwaterBusinessStatus::kRejected)
        .value("InternalError", UnderwaterBusinessStatus::kInternalError);
    py::enum_<UnderwaterRpcError>(m, "UnderwaterRpcError")
        .value("None_", UnderwaterRpcError::kNone).value("NotConnected", UnderwaterRpcError::kNotConnected)
        .value("InvalidArgument", UnderwaterRpcError::kInvalidArgument).value("Busy", UnderwaterRpcError::kBusy)
        .value("Timeout", UnderwaterRpcError::kTimeout).value("TransportError", UnderwaterRpcError::kTransportError)
        .value("ProtocolError", UnderwaterRpcError::kProtocolError).value("Cancelled", UnderwaterRpcError::kCancelled)
        .value("DeviceRejected", UnderwaterRpcError::kDeviceRejected);
    py::class_<UnderwaterStatus>(m, "UnderwaterStatus")
        .def_readonly("mode", &UnderwaterStatus::m_mode)
        .def_readonly("uptime_ms", &UnderwaterStatus::m_uptime_ms)
        .def_readonly("control_session", &UnderwaterStatus::m_control_session)
        .def_readonly("enabling_arms", &UnderwaterStatus::m_enabling_arms)
        .def_readonly("gravity_arms", &UnderwaterStatus::m_gravity_arms)
        .def_readonly("master_available_mask", &UnderwaterStatus::m_master_available_mask)
        .def_readonly("slave_available_mask", &UnderwaterStatus::m_slave_available_mask)
        .def_readonly("master_enabled_mask", &UnderwaterStatus::m_master_enabled_mask)
        .def_readonly("slave_enabled_mask", &UnderwaterStatus::m_slave_enabled_mask)
        .def_readonly("master_disable_pending_mask", &UnderwaterStatus::m_master_disable_pending_mask)
        .def_readonly("slave_disable_pending_mask", &UnderwaterStatus::m_slave_disable_pending_mask)
        .def_readonly("master_disabled_mask", &UnderwaterStatus::m_master_disabled_mask)
        .def_readonly("slave_disabled_mask", &UnderwaterStatus::m_slave_disabled_mask)
        .def_readonly("upgrading", &UnderwaterStatus::m_upgrading)
        .def_readonly("reason", &UnderwaterStatus::m_reason);
    py::class_<UnderwaterRpcResult>(m, "UnderwaterRpcResult")
        .def_readonly("error", &UnderwaterRpcResult::m_error)
        .def_readonly("device_status", &UnderwaterRpcResult::m_device_status)
        .def_readonly("info", &UnderwaterRpcResult::m_info)
        .def("__bool__", [](const UnderwaterRpcResult& s_result) { return bool(s_result); });

    py::enum_<UnderwaterControlKind>(m, "UnderwaterControlKind")
        .value("MIT", UnderwaterControlKind::kMIT)
        .value("MitWithGravity", UnderwaterControlKind::kMitWithGravity);
    py::class_<UnderwaterArmCommand>(m, "UnderwaterArmCommand")
        .def(py::init<>())
        .def_readwrite("q", &UnderwaterArmCommand::m_q)
        .def_readwrite("dq", &UnderwaterArmCommand::m_dq)
        .def_readwrite("kp", &UnderwaterArmCommand::m_kp)
        .def_readwrite("kd", &UnderwaterArmCommand::m_kd)
        .def_readwrite("tau", &UnderwaterArmCommand::m_tau)
        .def_readwrite("kind", &UnderwaterArmCommand::m_kind);
    py::class_<UnderwaterArmStatus>(m, "UnderwaterArmStatus")
        .def_readonly("q", &UnderwaterArmStatus::m_q)
        .def_readonly("dq", &UnderwaterArmStatus::m_dq)
        .def_readonly("tau", &UnderwaterArmStatus::m_tau)
        .def_readonly("temperature_c", &UnderwaterArmStatus::m_temperature_c)
        .def_readonly("motor_status", &UnderwaterArmStatus::m_motor_status)
        .def_readonly("feedback_age_ms", &UnderwaterArmStatus::m_feedback_age_ms)
        .def_readonly("fresh_mask", &UnderwaterArmStatus::m_fresh_mask)
        .def_readonly("enabled_mask", &UnderwaterArmStatus::m_enabled_mask)
        .def_readonly("available_mask", &UnderwaterArmStatus::m_available_mask)
        .def_readonly("applied_mask", &UnderwaterArmStatus::m_applied_mask)
        .def_readonly("mode", &UnderwaterArmStatus::m_mode)
        .def_readonly("kind", &UnderwaterArmStatus::m_kind)
        .def_readonly("sequence", &UnderwaterArmStatus::m_sequence)
        .def_readonly("applied_command_sequence", &UnderwaterArmStatus::m_applied_command_sequence)
        .def_readonly("timestamp_us", &UnderwaterArmStatus::m_timestamp_us)
        .def_readonly("control_session", &UnderwaterArmStatus::m_control_session)
        .def_readonly("last_sdk_timestamp_us", &UnderwaterArmStatus::m_last_sdk_timestamp_us)
        .def_readonly("command_received", &UnderwaterArmStatus::m_command_received)
        .def_readonly("control_stop_reason", &UnderwaterArmStatus::m_control_stop_reason)
        .def_readonly("commands_received", &UnderwaterArmStatus::m_commands_received)
        .def_readonly("commands_overwritten", &UnderwaterArmStatus::m_commands_overwritten)
        .def_readonly("commands_rejected", &UnderwaterArmStatus::m_commands_rejected)
        .def_readonly("command_reject_reason", &UnderwaterArmStatus::m_command_reject_reason);
    py::class_<UnderwaterCommandResult>(m, "UnderwaterCommandResult")
        .def_readonly("error", &UnderwaterCommandResult::m_error)
        .def_readonly("sequence", &UnderwaterCommandResult::m_sequence)
        .def("__bool__", [](const UnderwaterCommandResult& s_result) { return bool(s_result); });

    py::enum_<FirmwareUpdateError>(m, "FirmwareUpdateError")
        .value("None_", FirmwareUpdateError::kNone).value("NotConnected", FirmwareUpdateError::kNotConnected)
        .value("Busy", FirmwareUpdateError::kBusy).value("InvalidArgument", FirmwareUpdateError::kInvalidArgument)
        .value("Timeout", FirmwareUpdateError::kTimeout).value("DeviceRejected", FirmwareUpdateError::kDeviceRejected)
        .value("ProtocolError", FirmwareUpdateError::kProtocolError).value("TransportError", FirmwareUpdateError::kTransportError)
        .value("Cancelled", FirmwareUpdateError::kCancelled).value("IdentityMismatch", FirmwareUpdateError::kIdentityMismatch)
        .value("ProtocolMismatch", FirmwareUpdateError::kProtocolMismatch);
    py::enum_<FirmwareRebootMode>(m, "FirmwareRebootMode")
        .value("Normal", FirmwareRebootMode::kNormal).value("TryBoot", FirmwareRebootMode::kTryBoot);
    py::enum_<FirmwareInstallState>(m, "FirmwareInstallState")
        .value("Unknown", FirmwareInstallState::kUnknown).value("TargetConfirmed", FirmwareInstallState::kTargetConfirmed)
        .value("PreviousFirmware", FirmwareInstallState::kPreviousFirmware);
    py::enum_<FirmwareUploadState>(m, "FirmwareUploadState")
        .value("Idle", FirmwareUploadState::kIdle).value("Queued", FirmwareUploadState::kQueued)
        .value("Starting", FirmwareUploadState::kStarting).value("Uploading", FirmwareUploadState::kUploading)
        .value("Cancelling", FirmwareUploadState::kCancelling).value("Completed", FirmwareUploadState::kCompleted)
        .value("Failed", FirmwareUploadState::kFailed).value("Cancelled", FirmwareUploadState::kCancelled);
    py::class_<FirmwareUploadOptions>(m, "FirmwareUploadOptions")
        .def(py::init<>())
        .def_property("timeout_ms", [](const FirmwareUploadOptions& s_o) { return s_o.m_timeout.count(); },
            [](FirmwareUploadOptions& s_o, std::int64_t s_ms) { s_o.m_timeout = Ms(s_ms); })
        .def_readwrite("status_timeout_ms", &FirmwareUploadOptions::m_status_timeout_ms)
        .def_readwrite("busy_retry_ms", &FirmwareUploadOptions::m_busy_retry_ms)
        .def_readwrite("max_retries", &FirmwareUploadOptions::m_max_retries);
    auto s_install_options = py::class_<FirmwareInstallOptions>(m, "FirmwareInstallOptions").def(py::init<>());
    const auto s_timeout_property = [&](const char* s_name, Ms FirmwareInstallOptions::*s_field) {
        s_install_options.def_property(s_name,
            [s_field](const FirmwareInstallOptions& s_o) { return (s_o.*s_field).count(); },
            [s_field](FirmwareInstallOptions& s_o, std::int64_t s_ms) { s_o.*s_field = Ms(s_ms); });
    };
    s_timeout_property("query_timeout_ms", &FirmwareInstallOptions::m_query_timeout);
    s_timeout_property("reboot_timeout_ms", &FirmwareInstallOptions::m_reboot_timeout);
    s_timeout_property("boot_timeout_ms", &FirmwareInstallOptions::m_boot_timeout);
    s_timeout_property("poll_interval_ms", &FirmwareInstallOptions::m_poll_interval);
    py::class_<BootStatus>(m, "BootStatus")
        .def_readonly("error", &BootStatus::m_error).def_readonly("device_status", &BootStatus::m_device_status)
        .def_readonly("protocol_version", &BootStatus::m_protocol_version).def_readonly("boot_state", &BootStatus::m_boot_state)
        .def_readonly("recovery_reason", &BootStatus::m_recovery_reason)
        .def_readonly("write_alignment", &BootStatus::m_write_alignment).def_readonly("max_chunk_size", &BootStatus::m_max_chunk_size)
        .def_readonly("bytes_written", &BootStatus::m_bytes_written).def_readonly("active_transfer_id", &BootStatus::m_active_transfer_id)
        .def_readonly("active_slot", &BootStatus::m_active_slot).def_readonly("confirmed_slot", &BootStatus::m_confirmed_slot)
        .def_readonly("pending_slot", &BootStatus::m_pending_slot)
        .def_readonly("slot0_address", &BootStatus::m_slot0_address).def_readonly("slot0_size", &BootStatus::m_slot0_size)
        .def_readonly("slot1_address", &BootStatus::m_slot1_address).def_readonly("slot1_size", &BootStatus::m_slot1_size);
    py::class_<FirmwareDeviceInfoResult>(m, "FirmwareDeviceInfoResult")
        .def_readonly("error", &FirmwareDeviceInfoResult::m_error).def_readonly("device_status", &FirmwareDeviceInfoResult::m_device_status)
        .def_readonly("info", &FirmwareDeviceInfoResult::m_info).def_readonly("peer_session", &FirmwareDeviceInfoResult::m_peer_session)
        .def("__bool__", [](const FirmwareDeviceInfoResult& s_r) { return bool(s_r); });
    py::class_<FirmwareRebootResult>(m, "FirmwareRebootResult")
        .def_readonly("error", &FirmwareRebootResult::m_error).def_readonly("device_status", &FirmwareRebootResult::m_device_status)
        .def_readonly("request_accepted", &FirmwareRebootResult::m_request_accepted)
        .def_readonly("departure_observed", &FirmwareRebootResult::m_departure_observed);
    py::class_<FirmwareUploadProgress>(m, "FirmwareUploadProgress")
        .def_readonly("state", &FirmwareUploadProgress::m_state).def_readonly("error", &FirmwareUploadProgress::m_error)
        .def_readonly("device_status", &FirmwareUploadProgress::m_device_status)
        .def_readonly("total_bytes", &FirmwareUploadProgress::m_total_bytes)
        .def_readonly("acknowledged_bytes", &FirmwareUploadProgress::m_acknowledged_bytes)
        .def_readonly("transfer_id", &FirmwareUploadProgress::m_transfer_id)
        .def_readonly("retries", &FirmwareUploadProgress::m_retries).def_readonly("busy_responses", &FirmwareUploadProgress::m_busy_responses)
        .def_property_readonly("active", &FirmwareUploadProgress::active);
    py::class_<FirmwareInstallResult>(m, "FirmwareInstallResult")
        .def_readonly("error", &FirmwareInstallResult::m_error).def_readonly("state", &FirmwareInstallResult::m_state)
        .def_readonly("reboot", &FirmwareInstallResult::m_reboot).def_readonly("previous_info", &FirmwareInstallResult::m_previous_info)
        .def_readonly("device_info", &FirmwareInstallResult::m_device_info).def_readonly("boot_status", &FirmwareInstallResult::m_boot_status)
        .def("__bool__", [](const FirmwareInstallResult& s_r) { return bool(s_r); });
    py::class_<UnderwaterFirmwareUpdateResult>(m, "UnderwaterFirmwareUpdateResult")
        .def_readonly("upload", &UnderwaterFirmwareUpdateResult::m_upload)
        .def_readonly("install", &UnderwaterFirmwareUpdateResult::m_install)
        .def("__bool__", [](const UnderwaterFirmwareUpdateResult& s_r) { return bool(s_r); });

    auto s_device = py::class_<UnderwaterTeleop, std::unique_ptr<UnderwaterTeleop>>(m, "UnderwaterTeleop");
    const auto s_connected = [](UnderwaterConnectionResult s_result) {
        if (!s_result) throw std::runtime_error(s_result.m_message +
            (s_result.m_system_error ? ": " + s_result.m_system_error.message() : "") +
            " (error=" + std::to_string(static_cast<int>(s_result.m_error)) + ")");
        return std::move(s_result.m_device);
    };
    s_device.def_static("connect", [s_connected](const std::string& s_uri, std::int64_t s_ms) {
        return s_connected(UnderwaterTeleop::connect(s_uri, Ms(s_ms)));
    }, py::arg("uri"), py::arg("timeout_ms") = 2000, py::call_guard<py::gil_scoped_release>());
    s_device.def_static("connect_device", [s_connected](const DeviceDescriptor& s_descriptor, std::int64_t s_ms) {
        return s_connected(UnderwaterTeleop::connect(s_descriptor, Ms(s_ms)));
    }, py::arg("device"), py::arg("timeout_ms") = 2000, py::call_guard<py::gil_scoped_release>());
    s_device.def_static("create", &UnderwaterTeleop::create, py::arg("uri"), py::call_guard<py::gil_scoped_release>());
    s_device.def("close", &UnderwaterTeleop::close, py::call_guard<py::gil_scoped_release>());
    s_device.def("__enter__", [](UnderwaterTeleop& s_self) -> UnderwaterTeleop& { return s_self; }, py::return_value_policy::reference_internal);
    s_device.def("__exit__", [](UnderwaterTeleop& s_self, py::object, py::object, py::object) {
        py::gil_scoped_release s_release; s_self.close();
    });
    const auto s_rpc = [&](const char* s_name, auto s_method) {
        s_device.def(s_name, [s_method](UnderwaterTeleop& s_self, std::int64_t s_ms) {
            return (s_self.*s_method)(Ms(s_ms));
        }, py::arg("timeout_ms") = 2000, py::call_guard<py::gil_scoped_release>());
    };
    s_rpc("status", &UnderwaterTeleop::status);
    s_rpc("enable", &UnderwaterTeleop::enable);
    s_rpc("disable", &UnderwaterTeleop::disable);
    s_rpc("enter_teleop", &UnderwaterTeleop::enterTeleop);
    s_device.def("zeroing", [](UnderwaterTeleop& s_self, std::int64_t s_ms) {
        return s_self.zeroing(Ms(s_ms));
    }, py::arg("timeout_ms") = 5000, py::call_guard<py::gil_scoped_release>());
    s_rpc("enter_control", &UnderwaterTeleop::enterControl);
    s_device.def("read_arm_status", &UnderwaterTeleop::readArmStatus, py::call_guard<py::gil_scoped_release>());
    s_device.def("write_arm_command", &UnderwaterTeleop::writeArmCommand,
        py::arg("command"), py::call_guard<py::gil_scoped_release>());
    s_rpc("settings", &UnderwaterTeleop::settings);
    s_rpc("default_settings", &UnderwaterTeleop::defaultSettings);
    s_device.def("set_settings", [](UnderwaterTeleop& s_self, UnderwaterSettings s_settings, std::int64_t s_ms) {
        py::gil_scoped_release s_release;
        return s_self.setSettings(s_settings, Ms(s_ms));
    }, py::arg("settings"), py::arg("timeout_ms") = 5000);
    s_rpc("device_info", &UnderwaterTeleop::deviceInfo);
    s_rpc("boot_status", &UnderwaterTeleop::bootStatus);
    s_device.def("reboot", [](UnderwaterTeleop& s_self, FirmwareRebootMode s_mode, std::int64_t s_ms) {
        return s_self.reboot(s_mode, Ms(s_ms));
    }, py::arg("mode") = FirmwareRebootMode::kTryBoot, py::arg("timeout_ms") = 5000,
       py::call_guard<py::gil_scoped_release>());
    s_device.def("reboot_and_wait", &UnderwaterTeleop::rebootAndWait, py::arg("expected_version"),
        py::arg("options") = FirmwareInstallOptions{}, py::call_guard<py::gil_scoped_release>());
    const auto s_upload = [&](const char* s_name, auto s_method) {
        s_device.def(s_name, [s_method](UnderwaterTeleop& s_self, py::bytes s_image,
                                       const FirmwareUploadOptions& s_options) {
            const std::string s_bytes = s_image;
            py::gil_scoped_release s_release;
            return (s_self.*s_method)(std::span(reinterpret_cast<const std::uint8_t*>(s_bytes.data()), s_bytes.size()), s_options);
        }, py::arg("image"), py::arg("options") = FirmwareUploadOptions{});
    };
    s_upload("start_firmware_upload", &UnderwaterTeleop::startFirmwareUpload);
    s_upload("upload_firmware", &UnderwaterTeleop::uploadFirmware);
    s_device.def("firmware_progress", &UnderwaterTeleop::firmwareProgress, py::call_guard<py::gil_scoped_release>());
    s_device.def("cancel_firmware_upload", &UnderwaterTeleop::cancelFirmwareUpload, py::call_guard<py::gil_scoped_release>());
    s_device.def("update_firmware", [](UnderwaterTeleop& s_self, py::bytes s_image,
        const Version& s_expected, const FirmwareUploadOptions& s_upload, const FirmwareInstallOptions& s_install) {
        const std::string s_bytes = s_image;
        py::gil_scoped_release s_release;
        return s_self.updateFirmware(std::span(reinterpret_cast<const std::uint8_t*>(s_bytes.data()), s_bytes.size()),
                                      s_expected, s_upload, s_install);
    }, py::arg("image"), py::arg("expected_version"), py::arg("upload_options") = FirmwareUploadOptions{},
       py::arg("install_options") = FirmwareInstallOptions{});
}
