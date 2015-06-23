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

//
// Structures
//
typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT  DeviceObject;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

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

//
// Global Variables
//
PDEVICE_EXTENSION deviceExtension;
ULONG             OldISR;        
PKINTERRUPT       PKInt;
KINTERRUPT        KInt; 
PKEVENT           pEvent = NULL; 
UCHAR             data[2];

char              requestTick = 0;

char              buffer[6];
PIdtr_t           Idtr = (PIdtr_t)buffer; // ''

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
    PIdtEntry_t            IdtEntry;
	ULONG	               Vector;
	PULONG				   array = NULL;
 	int                    InterruptObject;

	_asm mov ebx,0xfec00000
	_asm or  ebx,0x13
	_asm mov eax,0xc0000000
 	_asm mov dword ptr [eax], ebx
	
	array[0]=0x10 + 2 * IRQ; //write 0x10 + 2 * IRQ to IOREGSEL
	Vector=(array[4]&0xff);  // read IOWIN register

    _asm sidt buffer
    IdtEntry=(PIdtEntry_t)Idtr->Base;

	InterruptObject = ((unsigned int)IdtEntry[Vector].OffsetHigh<<16U)|
                      (IdtEntry[Vector].OffsetLow);

	//
	// Entry ~ DispatchCode 
	//
	InterruptObject -= 0x3c;
	
    PKInt = (PKINTERRUPT)((unsigned int)InterruptObject);
														  
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
