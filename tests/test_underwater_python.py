"""Public Python packaging/binding smoke test; no hardware or USB enumeration."""
import sys

sys.path.insert(0, sys.argv[1])
sys.path.insert(0, sys.argv[2])
import pyflorid as f

assert f.UnderwaterMode.Damp.value == 0
assert f.UnderwaterMode.Retract.value == 5
assert f.UnderwaterBusinessStatus.Busy.value == 3
assert f.UnderwaterTeleop.create("invalid://") is None
try:
    f.UnderwaterTeleop.connect("invalid://")
except RuntimeError as exc:
    assert "explicit" in str(exc)
else:
    raise AssertionError("invalid URI must raise a connection error")
upload = f.FirmwareUploadOptions()
assert upload.timeout_ms == 120_000
upload.timeout_ms = 1234
assert upload.timeout_ms == 1234
install = f.FirmwareInstallOptions()
assert install.boot_timeout_ms == 60_000
install.query_timeout_ms = 123
assert install.query_timeout_ms == 123
for name in ("enable", "disable", "enter_teleop", "status", "device_info", "boot_status", "reboot",
             "upload_firmware", "update_firmware", "firmware_progress", "cancel_firmware_upload",
             "settings", "default_settings", "set_settings"):
    assert callable(getattr(f.UnderwaterTeleop, name))
for name in f.__all__:
    assert hasattr(f, name), name
settings = f.UnderwaterSettings()
assert settings.buzzer_alarm_enabled is True
settings.buzzer_alarm_enabled = False
assert settings.buzzer_alarm_enabled is False
settings.angle_scale = [1.0, -1.0, 0.0, 2.5, 3.0]
assert settings.angle_scale == [1.0, -1.0, 0.0, 2.5, 3.0]
settings.gravity_scale = [1.0] * 4 + [-0.5] * 4
assert settings.gravity_scale[-1] == -0.5
settings.friction_deadband_rad_s = [0.03] * 10
assert len(settings.friction_deadband_rad_s) == 10
try:
    settings.kp = [1.0, 2.0]
except TypeError:
    pass
else:
    raise AssertionError("fixed five-axis field must preserve cardinality")
assert f.UnderwaterSettingsBusinessStatus.StorageFailed.value == 2
assert f.UnderwaterSettingsSource.Corrupt.value == 2
print("underwater Python bindings passed")

assert f.UnderwaterMode.Control.value == 3
for name in ("enter_control", "read_arm_status", "write_arm_command", "zeroing"):
    assert callable(getattr(f.UnderwaterTeleop, name))
command = f.UnderwaterArmCommand()
assert command.kind == f.UnderwaterControlKind.MIT
command.kind = f.UnderwaterControlKind.MitWithGravity
command.q = [i * 0.125 for i in range(10)]
assert command.q[-1] == 1.125
assert len(command.kp) == 10
try:
    command.tau = [0.0] * 9
except TypeError:
    pass
else:
    raise AssertionError("command requires all ten motors")
