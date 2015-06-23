#ifndef __KbdHook_h__
#define __KbdHook_h__

//////////////////////////
// PROTOTYPES
//////////////////////////
NTSTATUS HookKeyboard(IN PDRIVER_OBJECT pDriverObject);
NTSTATUS DispatchRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS OnReadCompletion(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, IN PVOID Context);

#endif