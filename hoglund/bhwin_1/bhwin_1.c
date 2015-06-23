// BASIC DEVICE DRIVER

#include "ntddk.h"

VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("BHWIN: OnUnload called\n");
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	DbgPrint("BHWin is al1v3!");

	theDriverObject->DriverUnload  = OnUnload; 

	return STATUS_SUCCESS;
}

