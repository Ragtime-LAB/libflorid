"""Underwater mode RPCs and upgrade on one explicit USB connection.

Closing the SDK leaves the robot in its current mode; use 'disable' explicitly.
"""
import argparse
from pathlib import Path
import pyflorid as f


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("uri", help="usb://VID:PID/SERIAL (production Bulk)")
    parser.add_argument("action", choices=["status", "enable", "disable", "teleop", "boot", "upload", "update", "reboot"])
    parser.add_argument("--image", type=Path, help="zephyr.signed.bin")
    parser.add_argument("--version", help="expected FCI major.minor.patch for update")
    args = parser.parse_args()
    if args.action in ("upload", "update") and args.image is None:
        parser.error("upload/update requires --image")
    expected = f.Version()
    if args.action == "update":
        try:
            expected.major, expected.minor, expected.patch = map(int, args.version.split("."))
        except (AttributeError, TypeError, ValueError):
            parser.error("update requires --version MAJOR.MINOR.PATCH")

    with f.UnderwaterTeleop.connect(args.uri) as device:
        if args.action in ("upload", "update"):
            image = args.image.read_bytes()
            if args.action == "upload":
                result = device.upload_firmware(image)
                print(result.state, result.error, result.device_status, result.acknowledged_bytes)
                return 0 if result.state == f.FirmwareUploadState.Completed else 1
            result = device.update_firmware(image, expected)
            print("upload:", result.upload.error, result.upload.device_status)
            if result.install is not None:
                print("install:", result.install.state, result.install.error)
            return 0 if result else 1
        if args.action == "boot":
            boot = device.boot_status()
            print(boot.error, boot.device_status, "active", boot.active_slot,
                  "confirmed", boot.confirmed_slot, "pending", boot.pending_slot)
            return 0 if boot.error == f.FirmwareUpdateError.None_ else 1
        if args.action == "reboot":
            result = device.reboot()
            print(result.error, result.device_status, result.request_accepted, result.departure_observed)
            return 0 if result.error == f.FirmwareUpdateError.None_ else 1
        result = getattr(device, "enter_teleop" if args.action == "teleop" else args.action)()
        print("accepted:", bool(result), "error:", result.error, "device_status:", result.device_status)
        if result.info:
            info = result.info
            print(info.mode, "enabling", hex(info.enabling_arms), "gravity", hex(info.gravity_arms),
                  "master enabled", hex(info.master_enabled_mask), "slave enabled", hex(info.slave_enabled_mask),
                  "upgrading", info.upgrading, info.reason)
        return 0 if result else 1


if __name__ == "__main__":
    raise SystemExit(main())
