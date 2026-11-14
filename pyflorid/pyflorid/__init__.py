try:
    from _pyflorid import *
except ImportError:
    from ._pyflorid import *

__all__ = [
    "Version", "FirmwareType", "DeviceInfo", "DeviceDescriptor", "DeviceSelector",
    "DeviceAccessStatus", "DeviceCompatibility", "discover_devices", "wait_for_device",
    "UnderwaterTeleop", "UnderwaterMode", "UnderwaterStatus",
    "UnderwaterArmStatus", "UnderwaterArmCommand", "UnderwaterControlKind",
    "UnderwaterCommandResult", "UnderwaterBusinessStatus", "UnderwaterRpcError",
    "UnderwaterRpcResult", "UnderwaterSettings", "UnderwaterSettingsSnapshot",
    "UnderwaterSettingsRpcResult", "UnderwaterSettingsBusinessStatus",
    "UnderwaterSettingsSource", "UnderwaterFirmwareUpdateResult",
    "FirmwareUpdateError", "FirmwareRebootMode", "FirmwareInstallState",
    "FirmwareUploadState", "FirmwareUploadOptions", "FirmwareInstallOptions",
    "FirmwareDeviceInfoResult", "FirmwareRebootResult", "FirmwareUploadProgress",
    "FirmwareInstallResult", "BootStatus",
]

try:
    from _pyflorid import _SingleUsb
except ImportError:
    from ._pyflorid import _SingleUsb
from .single import UnderwaterSingle, DeviceRejected
__all__ += ["UnderwaterSingle", "DeviceRejected"]
