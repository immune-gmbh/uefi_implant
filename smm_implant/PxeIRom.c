#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/SmmServicesTableLib.h>  // gSmst
#include <Protocol/SmmSwDispatch2.h>       // EFI_SMM_SW_DISPATCH2_PROTOCOL
#include <Library/IoLib.h>

EFI_SMM_SW_REGISTER_CONTEXT     Context;

EFI_STATUS  
ImplantLoop(IN EFI_HANDLE  DispatchHandle,
                    IN CONST VOID  *Context         OPTIONAL,
                    IN OUT VOID    *CommBuffer      OPTIONAL,
                    IN OUT UINTN   *CommBufferSize  OPTIONAL)
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BackdoorEntry (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS                       Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL   *BackdoorEntry;
  EFI_HANDLE                       BackdoorHandle;
  DEBUG ((EFI_D_INFO, "[PONY] Register SMI...\n"));
  Status = gSmst->SmmLocateProtocol(
                      &gEfiSmmSwDispatch2ProtocolGuid,
                      NULL,
                      &BackdoorEntry
                  );
  DEBUG ((EFI_D_INFO, "[PONY] Locat SMI Protocol %r\n", Status));
  PonyRegisterContext.SwSmiInputValue = 0xDD;

  if(!EFI_ERROR(Status)){
    Status = BackdoorEntry->Register( BackdoorEntry,
                                    ImplantLoop, 
                                    &Context, 
                                    &BackdoorHandle );
    }
  return EFI_SUCCESS;
}