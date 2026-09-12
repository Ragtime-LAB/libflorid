"""Read the version from a little-endian MCUboot image header."""
from dataclasses import dataclass
import struct


@dataclass(frozen=True)
class ImageVersion:
    major: int
    minor: int
    patch: int
    build: int

    def __str__(self):
        return f"{self.major}.{self.minor}.{self.patch}+{self.build}"

    def matches(self, running):
        # DeviceInfo currently exposes major/minor/patch, without a build ID.
        return (self.major, self.minor, self.patch) == (running.major, running.minor, running.patch)


def image_version(header):
    if len(header) < 32 or struct.unpack_from("<I", header)[0] != 0x96F3B83D:
        return None
    return ImageVersion(*struct.unpack_from("<BBHI", header, 20))
