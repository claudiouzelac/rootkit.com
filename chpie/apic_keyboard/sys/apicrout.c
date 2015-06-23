/*
 * apicrout.c
 *
 *           
 *
 *         - x86 APIC Manipulation -
 *
 *
 *
 *         http://chpie.org
 *
 *
 *
 *  2007. 4. 14
 */

#include <ntddk.h>
#include "i8042.h"

#define NT_DEVICE_NAME  L"\\Device\\APICROUT" // device name and symbolic link name
#define DOS_DEVICE_NAME L"\\DosDevices\\APICROUT"
#define NT_KEYBOARD_NAME0 L"\\Device\\KeyboardClass0" // keyboard driver's name

#define IOCTL_REQUEST_DATA        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_REGISTER_EVENT      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

VOID DrvUnload(IN PDRIVER_OBJECT);
NTSTATUS DriverEntry(IN PDRIVER_OBJECT, IN PUNICODE_STRING);
NTSTATUS IoDeviceControl(IN	PDEVICE_OBJECT, IN PIRP);
NTSTATUS IoReadWrite(IN	PDEVICE_OBJECT, IN PIRP);
NTSTATUS IoDispatch(IN PDEVICE_OBJECT, IN PIRP);
VOID StartManipulation(VOID);
VOID CleanUPManipulation(VOID);
VOID MPCreateThread(VOID (*FunctionPointer)(IN PKDPC, IN PVOID, IN PVOID, IN PVOID));
VOID MPHandlerSetup(IN PKDPC, IN PVOID, IN PVOID, IN PVOID);
VOID x8259AConfiguration(ULONG);
VOID MapIOAPICArea(unsigned long **);
VOID UnmapIOAPICArea(unsigned long **);
volatile void NewHandler(void);
volatile void NewKeyboardHandler(void);
void InpKeyboardDpcFunction(IN PKDPC, IN PVOID, IN PVOID, IN PVOID);
PKINTERRUPT GetI8042PrtInterruptObject(void);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DrvUnload)
#pragma alloc_text(PAGE, IoDeviceControl)
#pragma alloc_text(PAGE, IoReadWrite)
#pragma alloc_text(PAGE, IoDispatch)
#endif

typedef struct idtentry {
        unsigned short OffsetLow;
        unsigned short Selector;
        unsigned char Reserved;
        unsigned char Type:3;
		unsigned char D:1;
        unsigned char Always0:1;
        unsigned char Dpl:2;
        unsigned char Present:1;
        unsigned short OffsetHigh;
} IdtEntry_t, *PIdtEntry_t;

#define ICW2_MASTER 0x30// low 3 bits are ignored.
#define ICW2_SLAVE  0xa1
#define IO_APIC_BASE 0xFEC00000 // I/O APIC Physical Base Address

#define INPUT_BUFFER_FULL  0x02
#define OUTPUT_BUFFER_FULL 0x01

ULONG   allProcessorDone; // count for the multi-processor support
ULONG   KeyboardHardwareIrql; // A Keyboard related IRQL
KDPC    kDpc; // a DPC Object
PKEVENT pEvent = NULL; // Event Reference
UCHAR   data[9]; // Packet
                 // data[7:0] internal scancode
				 // data[8]   last 0x64 status port data

////
//    Used to communicate with the APIC
////
PULONG IOAPICGate;
ULONG  IOAPICOldLowbyte;
ULONG  IOAPICOldHighbyte;
ULONG  IOAPICKeyboardVector;

/*
 * 
 *       Codes
 *
 */
void InpGetByteAsynchronous(OUT PUCHAR Byte)
{
	// Asynchronous port i/o from i8042dep.c
	ULONG i = 0;
	UCHAR response;

	while (i < 0xA000 && ((UCHAR)(response = READ_PORT_UCHAR((PUCHAR)0x64) & OUTPUT_BUFFER_FULL))
				                          != OUTPUT_BUFFER_FULL )
	{
		if (response & OUTPUT_BUFFER_FULL)
			*Byte = READ_PORT_UCHAR((PUCHAR)0x60); // Eat
		else
		{
			// Try again
			i++;
		}		
	}

	if (i >= 0xA000)
		return; // Time out

	*Byte = READ_PORT_UCHAR((PUCHAR)0x60);	
}

void InpPutByteAsynchronous(IN PUCHAR Port, IN UCHAR Byte)
{
	ULONG i = 0;
	UCHAR response;

	while (i++ < 0xA000 && ((response = READ_PORT_UCHAR((PUCHAR)0x64)) & INPUT_BUFFER_FULL))
	{
		//
		// Do nothing.
		//
	}

	WRITE_PORT_UCHAR(Port, Byte);
}

volatile _declspec (naked) void NewKeyboardHandler(void) // - Non-Paged
{
	//
	// ebp - 4 :: oldirql
	//
	_asm
	{
		push ebp
		mov ebp, esp
		sub esp, 0x4
		push fs
		push ds
		push es
		mov  eax, 0x30
		mov  fs, ax
		mov  eax, 0x23
		mov  ds, ax
		mov  es, ax	

		xor eax, eax
		mov eax, KeyboardHardwareIrql
		call dword ptr [KfRaiseIrql] // KeRaiseIrql(kKeyboardHardwareIrql, ebp - 4);
		mov dword ptr [ebp - 4], eax

		push 0xFF
		call x8259AConfiguration // mask 8259A for ExtINT
		
		//
		//
		//   8042, 8048 Communication
		//
		//   It works, but unstable.
                //   using the DPC handling is more stable.
		//   if you wanna to test this code,
		//   please remove the keyboard handling code in the DPC handler
                //
		/*
		in al, 0x64
		mov data[1], al
		push offset data[0]
		call InpGetByteAsynchronous

		push 0xD2
		push 0x64
		call InpPutByteAsynchronous
		xor eax, eax
		mov  al, data[0]
		push eax
		push 0x60
		call InpPutByteAsynchronous
                */
	
		//
		// A Deferred procedure call Requesting
		//
		push 0
		push offset InpKeyboardDpcFunction
		mov eax, offset kDpc
		push eax
		call dword ptr [KeInitializeDpc] // KeInitializeDpc(&kDpc, InpKeyboardDpcFunction, 0);
		push 0
		push 0
		mov eax, offset kDpc
		push eax
		call dword ptr [KeInsertQueueDpc] // KeInsertQueueDpc(&kDpc, 0, 0);

		push 0xFD
		call x8259AConfiguration // Release IRQ1

		mov ecx, dword ptr [ebp - 4]
		call dword ptr [KfLowerIrql] // KeLowerIrql(dword ptr [ebp - 4]);
		
		pop es
		pop ds
		pop fs
		add esp, 4
		pop ebp
		ret
	}
}

void InpKeyboardDpcFunction(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
	//
	// NewKeyboardHandler's DPC function
	//

        _asm
	{
		push eax
		in al, 0x64
		mov data[1], al
		in al, 0x60
		mov data[0], al
		pop eax
	}

	if (pEvent)
		KeSetEvent(pEvent, 0, 0);

	DbgPrint("Keyboard :: 0x%x, 0x%x\n", data[0], data[1]);	
}

volatile _declspec( naked ) void NewHandler(void) // - Non-Paged
{
	// ebx :: IDT Entry pointer
	// ecx :: Vector
	// edx :: IDT[Vector] Handler's offset
	//
	_asm
	{
		push ebp
		mov ebp, esp
		sub esp, 12
		pushad

		call NewKeyboardHandler // <--- Handler
		
		mov ecx, IOAPICKeyboardVector

		sidt fword ptr [ebp - 8]
		// memory on the stack
		// -8 -7 | -6 -5 -4 -3  | -2 -1
		// 00 00 | 00 00 00 00  | 00 00
		// Limit | Base Address
		mov ebx, dword ptr [ebp - 8 + 2] // ebx = IDTR Base offset
		shl ecx, 3 // multiplies 8
		add ebx, ecx
		mov dx, word ptr [ebx + 6]
		shl edx, 0x10
		mov dx, word ptr [ebx] // edx = IDT[Vector].BaseOffset
		mov dword ptr [ebp - 8 - 4], edx // save it

		popad
		add esp, 12
		pop ebp
		// jump!
		jmp dword ptr [esp - 4 - 12] // <---- jump to the windows Keyboard Handler
	}
}

VOID x8259AConfiguration(ULONG master_mask)
{
	/*
	 * lkd> !pic
	 * ----- IRQ Number ----- 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
	 * Physically in service:  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
	 * Physically masked:      Y  .  Y  Y  Y  Y  Y  Y  Y  Y  Y  Y  Y  Y  Y  Y
	 * Physically requested:   .  .  .  Y  Y  Y  .  .  .  .  Y  Y  .  .  .  .
	 */
	_asm
	{
		pushfd
		pushad

		xor eax, eax
		mov eax, 0x11
		out 0x20, al	// master port a
		out 0xa0, al	// slave port a

		mov eax, ICW2_MASTER
		out 0x21, al	// master offset ICW2_MASTER
		mov eax, ICW2_SLAVE
		out 0xa1, al	// slave

		mov eax, 0x4
		out 0x21, al	// slaves attached to IR line 2
		mov eax, 0x2
		out 0xa1, al	// This slave in IR Line 2 of master

		mov eax, 0x7
		out 0x21, al	// set as master
		mov eax, 0x1
		out 0xa1, al	// set as slave

		mov eax, master_mask	// mask all 8259a IRQs except IRQ1
		out 0x21, al
		mov eax, 0xff
		out 0xa1, al

		popad
		popfd
	}
}

VOID MapIOAPICArea(unsigned long ** MappedVirtualVariable)
{
	PHYSICAL_ADDRESS IOAPIC_AREA;     // i/o apic mapped pointer
	
	//
	//  Connect physical memory 0xfec00000 to [MappedVirtualVariable]
	//
	
	IOAPIC_AREA = RtlConvertLongToLargeInteger(IO_APIC_BASE);
	*MappedVirtualVariable = MmMapIoSpace(IOAPIC_AREA, 0x40, MmNonCached);
}

VOID UnmapIOAPICArea(unsigned long ** MappedVirtualVariable)
{
	MmUnmapIoSpace(*MappedVirtualVariable, 0x40);
}

VOID StartManipulation(VOID)
{
	ULONG IRQ = 1;
	ULONG Buffer;
	
	// Getting Hardware IRQL to corresponding to the IRQ 1
	// using i8042prt driver's Interrupt Object
	KeyboardHardwareIrql = (ULONG)GetI8042PrtInterruptObject()->Irql;
	if (!KeyboardHardwareIrql)
		KeyboardHardwareIrql = 0x8; // default on uni-processor x86 family	
	DbgPrint("Keyboard Hardware IRQL :: 0x%x\n", KeyboardHardwareIrql);
	
	MPCreateThread(MPHandlerSetup); // IDT Setup

	x8259AConfiguration(0xFD);	// re-configuration for change the vector
	
	MapIOAPICArea(&IOAPICGate); //**********************************************

	IOAPICGate[0] = 0x10 + 2 * IRQ + 1;
	Buffer = IOAPICGate[4];
	IOAPICOldHighbyte = Buffer; // save it for clean-up

	Buffer = 0; // Physical Destination APIC ID with 0x00
	IOAPICGate[0] = 0x10 + 2 * IRQ + 1;
	IOAPICGate[4] = Buffer; // write

	IOAPICGate[0] = 0x10 + 2 * IRQ;
	Buffer = IOAPICGate[4];
	IOAPICOldLowbyte = Buffer;

	Buffer &= 0xFF;  // now buffer has the vector value
	IOAPICKeyboardVector = Buffer;

	DbgPrint("Keyboard Vector :: 0x%x\n", Buffer);

	// Delivery Mode [10:8] to 111b (ExtINT)
	// Destination Mode [11] to 0 (Physical)
	//
	Buffer = Buffer | 0x700; // Vector(may be 0x93) + ExtINT + Physical Destination + High active + Edge sensitive + non masked
	IOAPICGate[0] = 0x10 + 2 * IRQ;
	IOAPICGate[4] = Buffer;		// write

	UnmapIOAPICArea(&IOAPICGate); //********************************************
}

VOID CleanUPManipulation(VOID)
{
	ULONG IRQ = 1;
	ULONG Buffer;

	//MPCreateThread(MPHandlerRestore);
	
	x8259AConfiguration(0xFF); // mask all

	MapIOAPICArea(&IOAPICGate);
	IOAPICGate[0] = 0x10 + 2 * IRQ + 1;
	IOAPICGate[4] = IOAPICOldHighbyte;
	
	_asm
	{
		nop
		nop     // Delay
		nop
		nop
	}
	
	IOAPICGate[0] = 0x10 + 2 * IRQ;
	IOAPICGate[4] = IOAPICOldLowbyte;

	UnmapIOAPICArea(&IOAPICGate);
}

PKINTERRUPT GetI8042PrtInterruptObject(void)
{
	PDEVICE_OBJECT pDeviceObject = NULL; // Keyboard DeviceObject
	PFILE_OBJECT fileObject;
	UNICODE_STRING keyName;
	PPORT_KEYBOARD_EXTENSION KeyboardExtension;
	PKINTERRUPT ReturnValue = NULL;
		
	RtlInitUnicodeString( &keyName, NT_KEYBOARD_NAME0 );
	
	// Getting the DeviceObject top-of-the-stack of the kbdclass device
	IoGetDeviceObjectPointer(&keyName,
							  FILE_READ_ATTRIBUTES,
							  &fileObject, 
							  &pDeviceObject);
	
	// if fails 
	if( !pDeviceObject )
	{
		return NULL;
	}

	// Tracking the DeviceStack
	//
	//
	// If it is not a i8042prt
	while( pDeviceObject->DeviceType != FILE_DEVICE_8042_PORT )
	{
		// go to the lower level object
		if (((PR_DEVOBJ_EXTENSION)pDeviceObject->DeviceObjectExtension)->AttachedTo)
			pDeviceObject = ((PR_DEVOBJ_EXTENSION)pDeviceObject->DeviceObjectExtension)->AttachedTo;
		else // here is lowest-level and couldn't find i8042prt
		    return NULL;
	}
	//
	// pDeviceObject == i8042prt's DeviceObject
	//
	ReturnValue = (PKINTERRUPT)((PPORT_KEYBOARD_EXTENSION)pDeviceObject->DeviceExtension)->InterruptObject;
	
	return ReturnValue;
}

VOID MPCreateThread(VOID (*FunctionPointer)(IN PKDPC, IN PVOID, IN PVOID, IN PVOID))
{
	/*
	 *
	 * Multi-Processor Consideration ::
	 *
	 * Each processor has it's own IDT.
	 * 
	 */
	CCHAR i;
	long currentProcessor;
	PKDPC pkDpc;
	KIRQL oldIrql, currentIrql;
	
	currentIrql = KeGetCurrentIrql();
	
	if (currentIrql < DISPATCH_LEVEL)
		KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

	InterlockedAnd(&allProcessorDone, 0);

	pkDpc = (PKDPC)ExAllocatePoolWithTag(NonPagedPool, KeNumberProcessors * sizeof(KDPC), (ULONG)' pni');
	
	if (!pkDpc)
	{
		DbgPrint("Insufficient Resource error\n");
		return;
	}

	currentProcessor = KeGetCurrentProcessorNumber();

	for (i = 0; i < KeNumberProcessors; i++)
	{
		KeInitializeDpc(&pkDpc[i],
						FunctionPointer,
						NULL);
		KeSetTargetProcessorDpc(&pkDpc[i], i);
		KeInsertQueueDpc(&pkDpc[i], NULL, NULL);
	}

	// wait for all of the processor's hooking initialization.
	while(InterlockedCompareExchange(&allProcessorDone, KeNumberProcessors - 1, KeNumberProcessors - 1) != KeNumberProcessors - 1)
	{
		_asm pause;
	}
	
	if (currentIrql < DISPATCH_LEVEL)
		KeLowerIrql(oldIrql);

	if (pkDpc)
	{
		ExFreePool(pkDpc);
		pkDpc = NULL;
	}
}

VOID MPHandlerSetup(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
	unsigned char    buffer[6];
	unsigned int *   IdtrBaseAddress = (unsigned int *)&buffer[2]; // Idtr->Base
	unsigned short * IdtrLimit = (unsigned short *)&buffer[0];
	PIdtEntry_t      IdtEntry;
	
	_asm sidt buffer;
  	IdtEntry=(PIdtEntry_t)*IdtrBaseAddress;	// Get a base address of idt

	// IDT 0x93
	// 0008 6bec 815d 8e00
	//
	// 815d high
	// 8e 10001110 ->
	//             Type 110
	//             D    1
	//                  0
	//             DPL  00
	//             P    1
	// 00 reserved
	// 08 selector
	// 6bec low
	IdtEntry[ICW2_MASTER + 1].Present = 1; // The gate is present on the memory
	IdtEntry[ICW2_MASTER + 1].Type = 0x6; // Interrupt Gate (110b)
	IdtEntry[ICW2_MASTER + 1].D = 1; // 32-bit size
	IdtEntry[ICW2_MASTER + 1].Always0 = 0; // Reserved 0
	IdtEntry[ICW2_MASTER + 1].Selector = 0x08; // Code segment selector
	IdtEntry[ICW2_MASTER + 1].Reserved = 0;
	IdtEntry[ICW2_MASTER + 1].Dpl = 0; // Ring 0
	
	IdtEntry[ICW2_MASTER + 1].OffsetLow=(unsigned short)NewHandler;
 	IdtEntry[ICW2_MASTER + 1].OffsetHigh=(unsigned short)((unsigned int)NewHandler>>16);

	InterlockedIncrement(&allProcessorDone);
	DbgPrint("Processor [%x] :: Complete.\n", allProcessorDone);
}

/*
 *
 *	 Driver Template
 *
 */

VOID DrvUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pDeviceObject;
	UNICODE_STRING uniWin32NameString;
	
	CleanUPManipulation();
	
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
	ULONG i;

	RtlInitUnicodeString( &uniNtNameString, NT_DEVICE_NAME );
	ntStatus = IoCreateDevice (
					pDriverObject,
                    0,	// DeviceExtensionSize
                    &uniNtNameString,
                    FILE_DEVICE_UNKNOWN,		// 
                    0,							// No standard device characteristics
                    FALSE,						// not exclusive device
                    &pDeviceObject
                    );
	if( !NT_SUCCESS(ntStatus) )
	{
		return ntStatus;
	}

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
	
	pDriverObject->MajorFunction[IRP_MJ_READ]			= 
	pDriverObject->MajorFunction[IRP_MJ_WRITE]			= IoReadWrite;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoDeviceControl;

	/////                            /////
	//                                  //
	//             Start!!!             //
	//                                  //
	/////                            /////
	StartManipulation();

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

NTSTATUS IoDeviceControl(IN	PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS				iStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION		pStack;
	ULONG					iTransfered = 0;

	HANDLE hEvent;
	
  	pStack = IoGetCurrentIrpStackLocation(pIrp);
	
	switch( pStack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_REGISTER_EVENT:
			hEvent = * (PHANDLE) pIrp->AssociatedIrp.SystemBuffer;
			iStatus = ObReferenceObjectByHandle(hEvent, EVENT_MODIFY_STATE, *ExEventObjectType, pIrp->RequestorMode, (PVOID *)&pEvent, NULL);
			break;
			
		case IOCTL_REQUEST_DATA:
			memcpy( (void *)pIrp->AssociatedIrp.SystemBuffer, (const void *)data, sizeof(char [9]));
			iTransfered = sizeof(char [9]);
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

NTSTATUS IoReadWrite(IN PDEVICE_OBJECT pDeviceObject, IN	PIRP pIrp)
{
	NTSTATUS				iStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION		pStack;
	ULONG					iTransfered = 0;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	
	pIrp->IoStatus.Status		= iStatus;
	pIrp->IoStatus.Information	= iTransfered;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	
	return iStatus;
}
