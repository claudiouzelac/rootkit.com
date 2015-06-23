/* inp.c
 *
 *
 *	- Blocking Interrupt -
 *
 *
 *
 *      University of Seoul, Computer Science
 *      2004920042 Chul-woong, Lee
 *
 * 	contact :: chpie@naver.com
 *                 http://chpie.tistory.com/
 *                 http://cafe.naver.com/inphook.cafe/
 *
 *
 *  2007. 12. 30. 
 */
#include <ntddk.h>
#include "inp.h"

// Function prototypes
VOID DrvUnload(IN PDRIVER_OBJECT);
NTSTATUS DriverEntry(IN PDRIVER_OBJECT, IN PUNICODE_STRING);
NTSTATUS IoDeviceControl(IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS IoReadWrite(IN	PDEVICE_OBJECT, IN PIRP);
NTSTATUS IoDispatch(IN PDEVICE_OBJECT, IN PIRP);

//
// Apic related function
//
void InpIOAPICDisconnection(unsigned long **);
void InpIOAPICConnection(unsigned long **);
ULONG InpGetKeyboardInterruptVector(void);
void Inp_Mask_KeyboardInterrupt(void);
void Inp_Unmask_KeyboardInterrupt(void);

/*
 *
 *      Structures
 *
 */
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DrvUnload)
#pragma alloc_text(PAGE, IoDeviceControl)
#pragma alloc_text(PAGE, IoReadWrite)
#pragma alloc_text(PAGE, IoDispatch)
#endif

#define NT_DEVICE_NAME  L"\\Device\\INP" // device name and symbolic link name
#define DOS_DEVICE_NAME L"\\DosDevices\\INP"
#define NT_KEYBOARD_NAME0 L"\\Device\\KeyboardClass0" // keyboard driver's name

#define IOCTL_REQUEST_DATA        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_REGISTER_EVENT      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define OUTPUT_BUFFER_FULL 0x1
#define INPUT_BUFFER_FULL  0x2
#define MOUSE_OUTPUT_BUFFER_FULL 0x20
#define BUFFER_FULL   (OUTPUT_BUFFER_FULL|MOUSE_OUTPUT_BUFFER_FULL)

/*
 *
 *
 *  	Global Variables.
 *
 */
PDEVICE_EXTENSION globalPdx; // backup pointer;
PULONG            IOAPICGate; // mapped physical 0xfec00000 by the InpIOAPICConnection()

// communication
PKEVENT pEvent = NULL;
UCHAR   data[2];


/*
 *
 *          	Codes
 *
 */
void Inp_Mask_KeyboardInterrupt(void)
{
	ULONG Buffer;

	IOAPICGate[0] = 0x10 + 2 * 1; // Open IRQ1's Redirection Table
	Buffer = IOAPICGate[4];

	_asm
	{
		bts Buffer, 16 // raise up bit offset 16 - Interrupt masked
	}

	IOAPICGate[0] = 0x10 + 2 * 1;
	IOAPICGate[4] = Buffer; // write
}

void Inp_Unmask_KeyboardInterrupt(void)
{
	ULONG Buffer;

	IOAPICGate[0] = 0x10 + 2 * 1; // Open IRQ1's Redirection Table
	Buffer = IOAPICGate[4];

	_asm
	{
		btr Buffer, 16 // clean up bit offset 16 - Interrupt is not masked
	}

	IOAPICGate[0] = 0x10 + 2 * 1;
	IOAPICGate[4] = Buffer; // write
}

void InpIOAPICConnection(unsigned long ** MappedVirtualVariable)
{
	PHYSICAL_ADDRESS IOAPIC_AREA;     // i/o apic mapped pointer
	
	//
	//  Connect physical memory 0xfec00000 to [MappedVirtualVariable]
	//
	
	IOAPIC_AREA = RtlConvertLongToLargeInteger(0xFEC00000);
	*MappedVirtualVariable = MmMapIoSpace(IOAPIC_AREA, 0x20, MmNonCached);
}

void InpIOAPICDisconnection(unsigned long ** MappedVirtualVariable)
{
	MmUnmapIoSpace(*MappedVirtualVariable, 0x20);
}

ULONG InpGetKeyboardInterruptVector(void)
{
	ULONG ReturnValue = 0;
	
	// IOREGSEL writing
	IOAPICGate[0] = 0x10 + 2 * 1; // 0x10 + 2 * IRQ, Keyboard's irq is 1
	// read IOWIN
	ReturnValue = IOAPICGate[4] & 0xFF;

	if (!ReturnValue)
	{
		DbgPrint("GetKeyboardInterruptVector() failed. i'll use the default vector 0x93.");
		ReturnValue = 0x93; // default ps/2 keyboard vector on wintelXP
	}
	
	return ReturnValue;
}

char ReadOutputBuffer(char * status, PKEVENT kill)
{
	LARGE_INTEGER delayTime = RtlConvertLongToLargeInteger(-6000); // (n * 100) nano-seconds
	int count;
	char dummy, ret = 0;

	for (count = 0; count < 0xA0000 && !KeReadStateEvent(kill); count++)
	{
		dummy = READ_PORT_UCHAR((PUCHAR)0x64);

		if ((dummy & BUFFER_FULL) == OUTPUT_BUFFER_FULL)
		{
			ret = READ_PORT_UCHAR((PUCHAR)0x60); // Read
			break;
		}

		KeDelayExecutionThread(KernelMode, FALSE, &delayTime);
	}

	if (count == 0xA0000)
	{
		*status = -1; // | Reject
	}
	else if (KeReadStateEvent(kill))
	{
		*status = -1;
		ret = -1;
	}
	else
	{
		*status = dummy;
	}

	return ret;
}

int GenerateScancode(char scancode)
{
	int count = 0xA0000;

	while(--count > 0 && (READ_PORT_UCHAR((PUCHAR)0x64) & INPUT_BUFFER_FULL));
	if (!count) return -1;
	WRITE_PORT_UCHAR((PUCHAR)0x64, 0xD2); // Generate a scancode

	count = 0xA0000;
	while(--count > 0 && (READ_PORT_UCHAR((PUCHAR)0x64) & INPUT_BUFFER_FULL));
	if (!count) return -1;
	WRITE_PORT_UCHAR((PUCHAR)0x60, scancode); // Put a Value

	// wait until drain
	while(!(READ_PORT_UCHAR((PUCHAR)0x64) & OUTPUT_BUFFER_FULL));

	return 0;
}

void ThreadProc(PDEVICE_EXTENSION pdx) // IRQL equals to PASSIVE_LEVEL
{
 	static unsigned char engine[] = { 0xCD, 0x00, 0xC3 }; // int xx, ret
 	unsigned char kccByte; // 0  XLATE  ME  KE  IGNLK  SYSF  MIE  KIE //
	LARGE_INTEGER delayTime = RtlConvertLongToLargeInteger(-6000); // (n * 100) nano-seconds

	//--------------------------------------------------------------------------
	// Write Keyboard controller RAM - disabling keyboard output buffer interrupt
	//
	kccByte = 0x46; // disable KIE bit

	while((READ_PORT_UCHAR((PUCHAR)0x64) & INPUT_BUFFER_FULL));
	WRITE_PORT_UCHAR((PUCHAR)0x64, 0x60); // Write KCCB

	while((READ_PORT_UCHAR((PUCHAR)0x64) & INPUT_BUFFER_FULL));
	WRITE_PORT_UCHAR((PUCHAR)0x60, kccByte);
	//--------------------------------------------------------------------------
	//Inp_Mask_KeyboardInterrupt(); // Double-Blocking

	//
	// Polling
	//
	while(!KeReadStateEvent(&pdx->kill))
	{
		if  (KeGetCurrentIrql() == PASSIVE_LEVEL)
		{
			if ((data[0] = ReadOutputBuffer(&data[1], &pdx->kill)) == -1) break;

			if (data[1] != -1)
			{
				if (pEvent)
					KeSetEvent(pEvent, 0, 0);

				if (GenerateScancode(data[0]))
					DbgPrint("inp :: GenerateScancode() Timeout");

				// Gate Firing - A Keyboard press event emulator
				// 
				//
				//  INT instruction
				//
				//  0xCD imm8 - Interrupt vector number specified by immediate byte.
				//
				engine[1] = (unsigned char)InpGetKeyboardInterruptVector();
				((void (*)(void))engine)(); // <-- Execute _asm INT InpGetKeyboardInterruptVector();
			}		

			KeDelayExecutionThread(KernelMode, FALSE, &delayTime);
		}
	}

	//Inp_Unmask_KeyboardInterrupt();
	//--------------------------------------------------------------------------
	// Write Keyboard controller RAM - enabling keyboard output buffer interrupt
	//
	kccByte = 0x47;

	while((READ_PORT_UCHAR((PUCHAR)0x64) & INPUT_BUFFER_FULL));
	WRITE_PORT_UCHAR((PUCHAR)0x64, 0x60); // Write KCCB

	while((READ_PORT_UCHAR((PUCHAR)0x64) & INPUT_BUFFER_FULL));
	WRITE_PORT_UCHAR((PUCHAR)0x60, kccByte);
	//--------------------------------------------------------------------------
		
	PsTerminateSystemThread(STATUS_SUCCESS);
}

void StopThread(PDEVICE_EXTENSION pdx)
{
	KeSetEvent(&pdx->kill, 0, FALSE);
	KeWaitForSingleObject(pdx->hThread, Executive, KernelMode, FALSE, NULL);
	ObDereferenceObject(pdx->hThread);
}

NTSTATUS StartPollThread(PDEVICE_EXTENSION pdx)
{
	//
	// Programming the Windows Driver Model - p. 764 : Polling device
	//
	NTSTATUS status;
	HANDLE hThread;
	OBJECT_ATTRIBUTES oa;

	KeInitializeEvent(&pdx->kill, NotificationEvent, FALSE);
	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

	status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &oa, NULL, NULL, (PKSTART_ROUTINE)ThreadProc, pdx);
	
	if (!NT_SUCCESS(status))
		return status;

	ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*)&pdx->hThread, NULL);

	ZwClose(hThread);

	return STATUS_SUCCESS;
}

/*
 *
 *					 Driver Template
 *
 */

VOID DrvUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pDeviceObject;
	UNICODE_STRING uniWin32NameString;

	StopThread(globalPdx);

	InpIOAPICDisconnection(&IOAPICGate);
	
	if (pEvent)
	{
		ObDereferenceObject(pEvent); // delete event reference
		pEvent = NULL;
	}
		
	pDeviceObject = pDriverObject->DeviceObject;

	RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );
	IoDeleteSymbolicLink( &uniWin32NameString );
	IoDeleteDevice( pDriverObject->DeviceObject );
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING RegistryPath)
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
                    FILE_DEVICE_UNKNOWN,	// 
                    0,				// No standard device characteristics
                    FALSE,			// not exclusive device
                    &pDeviceObject
                    );
	if( !NT_SUCCESS(ntStatus) )
	{
		return ntStatus;
	}

	globalPdx = pDeviceObject->DeviceExtension;

	// create dispatch points for create/open, close, unload
	pDriverObject->DriverUnload = DrvUnload;

	RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );
	ntStatus = IoCreateSymbolicLink( &uniWin32NameString, &uniNtNameString );
	if (!NT_SUCCESS(ntStatus))
	{
		IoDeleteDevice( pDriverObject->DeviceObject );
	}

	for( i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++ )
		pDriverObject->MajorFunction[i] = IoDispatch;
	
	pDriverObject->MajorFunction[IRP_MJ_READ]	    = 
	pDriverObject->MajorFunction[IRP_MJ_WRITE]	    = IoReadWrite;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoDeviceControl;

	/////                            /////
	//                                  //
	//             Start!!!             //
	//                                  //
	/////                            /////
	InpIOAPICConnection(&IOAPICGate);
	StartPollThread(globalPdx);

	return STATUS_SUCCESS;
} //DriverEntry

NTSTATUS IoDispatch(IN PDEVICE_OBJECT pDeviceObject, IN	PIRP pIrp)
{
	NTSTATUS iStatus = STATUS_SUCCESS;

	pIrp->IoStatus.Status		= iStatus;
	pIrp->IoStatus.Information	= 0;
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	return iStatus;
}

NTSTATUS IoDeviceControl(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS			iStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION		pStack;
	ULONG				iTransfered = 0;

	HANDLE hEvent;
	
  	pStack = IoGetCurrentIrpStackLocation(pIrp);
	
	switch( pStack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_REGISTER_EVENT:
			hEvent = * (PHANDLE) pIrp->AssociatedIrp.SystemBuffer;
			iStatus = ObReferenceObjectByHandle(hEvent, EVENT_MODIFY_STATE, *ExEventObjectType, pIrp->RequestorMode, (PVOID *)&pEvent, NULL);
			break;
			
		case IOCTL_REQUEST_DATA:
			memcpy( (void *)pIrp->AssociatedIrp.SystemBuffer, (const void *)data, sizeof(char [2]));
			iTransfered = sizeof(char [2]);
			break;

		default:
			iStatus = STATUS_INVALID_PARAMETER;
			break;
	}

	pIrp->IoStatus.Status		= iStatus;
	pIrp->IoStatus.Information	= iTransfered;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	
	return iStatus;
}

NTSTATUS IoReadWrite(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS			iStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION		pStack;
	ULONG				iTransfered = 0;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	
	pIrp->IoStatus.Status		= iStatus;
	pIrp->IoStatus.Information	= iTransfered;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	
	return iStatus;
}
