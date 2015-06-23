#ifndef __DriverEntry_h__
#define __DriverEntry_h__

const WCHAR deviceName[]  = L"\\Device\\mmHook";

//@@@@@@@@@@@@@@@@@@@@@@
// PROTOTYPES
//@@@@@@@@@@@@@@@@@@@@@@
extern "C" NTSTATUS DriverEntry( IN PDRIVER_OBJECT  pDriverObject, IN PUNICODE_STRING RegistryPath );
void OnUnload( IN PDRIVER_OBJECT pDriverObject);
static void DummyFunction(void);
static void DummyFunctionEnds(void);
#endif

