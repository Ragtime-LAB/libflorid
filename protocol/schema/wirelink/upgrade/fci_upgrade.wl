version 1;

// Upgrade v1 allocation map:
//   0x7000-0x701f  enums and value objects
//   0x7020-0x702f  constant-memory bulk transfer
//   0x7100-0x711f  typed control RPC

enum UpgradeStatus = 28673 { // 0x7001
  UPGRADE_OK = 0;
  UPGRADE_BUSY = 1;
  UPGRADE_INVALID_STATE = 2;
  UPGRADE_INVALID_ARGUMENT = 3;
  UPGRADE_OUT_OF_RANGE = 4;
  UPGRADE_CRC_MISMATCH = 5;
  UPGRADE_FLASH_FAILURE = 6;
  UPGRADE_VERIFY_FAILURE = 7;
  UPGRADE_SIGNATURE_FAILURE = 8;
  UPGRADE_UNSUPPORTED = 9;
  UPGRADE_INTERNAL_ERROR = 10;
}

enum ImageId = 28674 { // 0x7002
  IMAGE_APPLICATION = 0;
}

enum SlotId = 28675 { // 0x7003
  SLOT_NONE = 0;
  SLOT_PRIMARY = 1;
  SLOT_SECONDARY = 2;
}

enum BootState = 28676 { // 0x7004
  BOOT_IDLE = 0;
  BOOT_RECOVERY = 1;
  BOOT_UPGRADING = 2;
  BOOT_PENDING_TEST = 3;
  BOOT_BOOTING = 4;
}

enum RecoveryReason = 28677 { // 0x7005
  RECOVERY_NONE = 0;
  RECOVERY_KEY_PRESSED = 1;
  RECOVERY_NO_BOOTABLE_IMAGE = 2;
  RECOVERY_IMAGE_VALIDATION_FAILED = 3;
  RECOVERY_UPGRADE_REQUESTED = 4;
  RECOVERY_INTERNAL_FAILURE = 5;
}

enum RebootMode = 28678 { // 0x7006
  REBOOT_NORMAL = 0;
  REBOOT_STAY_IN_RECOVERY = 1;
  REBOOT_TRY_BOOT_IMMEDIATELY = 2;
}

enum UpgradeBulkPhase = 28679 { // 0x7007
  UPGRADE_BULK_PHASE_NONE = 0;
  UPGRADE_BULK_PHASE_BEGIN = 1;
  UPGRADE_BULK_PHASE_CHUNK = 2;
  UPGRADE_BULK_PHASE_END = 3;
  UPGRADE_BULK_PHASE_ABORT = 4;
}

enum UpgradeBulkStatusCode = 28680 { // 0x7008
  UPGRADE_BULK_STATUS_OK = 0;
  UPGRADE_BULK_STATUS_BUSY = 1;
  UPGRADE_BULK_STATUS_OUT_OF_ORDER = 2;
  UPGRADE_BULK_STATUS_CONFLICT = 3;
  UPGRADE_BULK_STATUS_INVALID = 4;
  UPGRADE_BULK_STATUS_WRITE_FAILED = 5;
  UPGRADE_BULK_STATUS_INTEGRITY_FAILED = 6;
  UPGRADE_BULK_STATUS_ABORTED = 7;
  UPGRADE_BULK_STATUS_TIMED_OUT = 8;
}

// Mirrors the MCUboot image-version representation without native struct
// layout or endian assumptions.
message ImageVersion = 28689 { // 0x7011
  required uint8 major = 1;
  required uint8 minor = 2;
  required uint16 revision = 3;
  required uint32 build = 4;
}

message BootStatusInfo = 28690 { // 0x7012
  required uint16 protocol_version = 1;
  required BootState boot_state = 2;
  required RecoveryReason recovery_reason = 3;
  required SlotId active_slot = 4;
  required SlotId confirmed_slot = 5;
  required SlotId pending_slot = 6;
  required uint16 write_align = 7;
  required uint16 max_chunk_size = 8;
  required fixed32 slot0_address = 9;
  required fixed32 slot0_size = 10;
  required fixed32 slot1_address = 11;
  required fixed32 slot1_size = 12;
  required fixed64 bytes_written = 13;
  optional fixed32 active_transfer_id = 14;
}

// These five messages map one-for-one to wirelink/bulk.h. Chunk data remains
// borrowed and is bounded so codegen can publish a deterministic frame limit.
message BulkBegin = 28704 { // 0x7020
  required fixed32 transfer_id = 1;
  required fixed64 total_length = 2;
  required fixed32 requested_chunk_size = 3;
  required fixed32 object_crc32c = 4;
}

message BulkChunk = 28705 { // 0x7021
  required fixed32 transfer_id = 1;
  required fixed64 offset = 2;
  // 2031 bytes plus the worst-case 17-byte field overhead fits the current
  // 2048-byte Wirelink frame payload ceiling exactly.
  required bytes<2031> data = 3;
}

message BulkEnd = 28706 { // 0x7022
  required fixed32 transfer_id = 1;
  required fixed64 total_length = 2;
  required fixed32 object_crc32c = 3;
}

message BulkAbort = 28707 { // 0x7023
  required fixed32 transfer_id = 1;
  required int32 reason = 2;
}

message BulkStatus = 28708 { // 0x7024
  required fixed32 transfer_id = 1;
  required UpgradeBulkPhase phase = 2;
  required UpgradeBulkStatusCode code = 3;
  required fixed64 next_offset = 4;
  required fixed32 accepted_chunk_size = 5;
}

message GetBootStatusRequest = 28928 { // 0x7100
  required uint32 operation_id = 1;
}

message GetBootStatusResponse = 28929 { // 0x7101
  required uint32 operation_id = 1;
  required UpgradeStatus status = 2;
  optional BootStatusInfo info = 3;
}

// StartUpgrade prepares the sink and returns a fresh transfer ID. The bulk
// Begin repeats length/CRC so retries and resumed transfers remain
// self-describing at the application layer.
message StartUpgradeRequest = 28930 { // 0x7102
  required uint32 operation_id = 1;
  required ImageId image_id = 2;
  required SlotId target_slot = 3;
  required uint8 flags = 4;
  required ImageVersion image_version = 5;
  required fixed64 image_size = 6;
  required fixed32 image_crc32c = 7;
}

message StartUpgradeResponse = 28931 { // 0x7103
  required uint32 operation_id = 1;
  required UpgradeStatus status = 2;
  optional fixed32 transfer_id = 3;
  optional fixed64 next_offset = 4;
  optional uint16 write_align = 5;
  optional uint16 max_chunk_size = 6;
}

message RebootRequest = 28932 { // 0x7104
  required uint32 operation_id = 1;
  required RebootMode mode = 2;
}

message RebootResponse = 28933 { // 0x7105
  required uint32 operation_id = 1;
  required UpgradeStatus status = 2;
  required RebootMode mode = 3;
}
