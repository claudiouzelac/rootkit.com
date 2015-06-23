#include "ntddk.h"
#include "stdarg.h"
#include "stdio.h"
#include "ntiologc.h"

#include "rk_interrupt.h"
#include "ioctlcmd.h"

void GetProcessNameOffset()
{
    PEPROCESS curproc;
    int i;
	curproc = PsGetCurrentProcess();
    for( i = 0; i < 3*PAGE_SIZE; i++ ) 
	{
        if( !strncmp( "System", (PCHAR) curproc + i, strlen("System") ))
		{
            gProcessNameOffset = i;
		}
    }
}


NTSTATUS
OnStubDispatch(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    PIO_STACK_LOCATION      irpStack;
    PVOID                   inputBuffer;
    PVOID                   outputBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    ULONG                   ioControlCode;
	NTSTATUS				ntstatus;

	UNICODE_STRING          deviceLinkUnicodeString;

	DbgPrint("sTrace: OnStubDispatch called.\n");
    //
    // Go ahead and set the request up as successful
    //
    ntstatus = Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get the pointer to the input/output buffer and its length
    //
    inputBuffer             = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength       = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBuffer            = Irp->AssociatedIrp.SystemBuffer;
    outputBufferLength      = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode           = irpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:
		DbgPrint("sTrace: IRP_MJ_CREATE entered.\n");
        break;

    case IRP_MJ_SHUTDOWN:
		DbgPrint("sTrace: IRP_MJ_SHUTDOWN entered.\n");

		RtlInitUnicodeString( &deviceLinkUnicodeString, deviceLinkBuffer );
		IoDeleteSymbolicLink( &deviceLinkUnicodeString );
		// Delete the device object
		//
		IoDeleteDevice( DeviceObject );

        break;

    case IRP_MJ_CLOSE:
		DbgPrint("sTrace: IRP_MJ_CLOSE entered.\n");

        break;

    case IRP_MJ_DEVICE_CONTROL:
		DbgPrint("sTrace: IRP_MJ_DEVICE_CONTROL entered.\n");

        if(IOCTL_TRANSFER_TYPE(ioControlCode) == METHOD_NEITHER) {
            outputBuffer = Irp->UserBuffer;
        }

        // Its a request from rootkit 
        ntstatus = sTraceDeviceControl(	irpStack->FileObject, TRUE,
												inputBuffer, inputBufferLength, 
												outputBuffer, outputBufferLength,
												ioControlCode, &Irp->IoStatus, DeviceObject );
        break;
    }

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return ntstatus;  
}




NTSTATUS
sTraceDeviceControl(
    IN PFILE_OBJECT FileObject, 
    IN BOOLEAN Wait,
    IN PVOID InputBuffer, 
    IN ULONG InputBufferLength, 
    OUT PVOID OutputBuffer, 
    IN ULONG OutputBufferLength, 
    IN ULONG IoControlCode, 
    OUT PIO_STATUS_BLOCK IoStatus, 
    IN PDEVICE_OBJECT DeviceObject 
    ) 
{
	NTSTATUS ntStatus;
//    UNICODE_STRING          deviceLinkUnicodeString;

	DbgPrint("sTrace: sTraceDeviceControl called.\n");

	IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = 0;

    switch ( IoControlCode ) 
	{

	case IOCTL_STRACE_INIT:
		DbgPrint("sTrace: IOCTL_STRACE_INIT entered.\n");

		if ((InputBufferLength < sizeof(int) * 2) || (InputBuffer == NULL))
		{
			IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}
		gProcessID    = (ULONG) (*(ULONG *)InputBuffer);
		gSyscallLimit = (ULONG) (*((ULONG *)InputBuffer+1));	

		break;

	case IOCTL_GET_SYSCALL_NUM:

		DbgPrint("sTrace: IOCTL_GET_SYSCALL_NUM entered.\n");

		if ((OutputBufferLength < sizeof(DWORD)) || (OutputBuffer == NULL))
		{
			IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}
		*(DWORD *)OutputBuffer = KeServiceDescriptorTable.NumberOfServices;

		IoStatus->Information = sizeof(DWORD);
		
		break;
	
	case IOCTL_REPORT_SYSCALL_NAMES:

		DbgPrint("sTrace: IOCTL_REPORT_SYSCALL_NAMES entered.\n");
		
		// Table already initialized. Reboot if you just installed a service pack.
		if (name_array != NULL)
		{
			break;
		}
		if ((InputBufferLength < KeServiceDescriptorTable.NumberOfServices*FUNC_NAME_LEN) || (InputBuffer == NULL))
		{
			IoStatus->Status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}
		name_array = (char *)ExAllocatePool(NonPagedPool, KeServiceDescriptorTable.NumberOfServices*FUNC_NAME_LEN);

		if (name_array == NULL)
		{
			IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RtlCopyMemory(name_array, InputBuffer, KeServiceDescriptorTable.NumberOfServices*FUNC_NAME_LEN);
		break;

	default:
		DbgPrint("sTrace: Default entered = INVALID_DEVICE_REQUEST.\n");
		IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

    return IoStatus->Status;
}


VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("sTrace: OnUnload called.\n");

	UnhookInterrupts();
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	int i;
    NTSTATUS                ntStatus;
    UNICODE_STRING          deviceNameUnicodeString;
    UNICODE_STRING          deviceLinkUnicodeString;    

	DbgPrint("sTrace: Loading.\n");

	GetProcessNameOffset();

	    // Setup our name and symbolic link. 
    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer );
    RtlInitUnicodeString (&deviceLinkUnicodeString,
                          deviceLinkBuffer );
    //
    // Set up the device
    //
    ntStatus = IoCreateDevice ( theDriverObject,
                                0, // For driver extension
                                &deviceNameUnicodeString,
                                FILE_DEVICE_STRACE_TRAP,
                                0,
                                FALSE,
                                &g_StraceTrapDevice );
    if( NT_SUCCESS(ntStatus)) {
        ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
                                         &deviceNameUnicodeString );

			// Register a dispatch function
		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) 
		{
        	theDriverObject->MajorFunction[i] = OnStubDispatch;
		}

		theDriverObject->DriverUnload  = OnUnload; 

    }

        			
	HookInterrupts();

	return STATUS_SUCCESS;
}

