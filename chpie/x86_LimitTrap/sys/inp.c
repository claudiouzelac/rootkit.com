/* inp.c
 *
 *
 *	- x86 Limit trap -
 *
 *
 * 	http://chpie.org
 *
 */
#include <ntddk.h>
#include "inp.h"
#include "i8042.h"

// Function prototypes
VOID DrvUnload(IN PDRIVER_OBJECT);
NTSTATUS DriverEntry(IN PDRIVER_OBJECT, IN PUNICODE_STRING);
NTSTATUS IoDeviceControl(IN	PDEVICE_OBJECT, IN PIRP);
NTSTATUS IoReadWrite(IN	PDEVICE_OBJECT, IN PIRP);
NTSTATUS IoDispatch(IN PDEVICE_OBJECT, IN PIRP);
void Initialization(VOID (*FunctionPointer)(IN PKDPC, IN PVOID, IN PVOID, IN PVOID));
void MPInitializationThread(IN PKDPC, IN PVOID, IN PVOID, IN PVOID);
void MPDisabledThread(IN PKDPC, IN PVOID, IN PVOID, IN PVOID);
void HandlerSetup(void);
volatile void NewHandler(void);
volatile void NewKeyboardHandler(void);
void InpKeyboardDpcFunction(IN PKDPC, IN PVOID, IN PVOID, IN PVOID);
PKINTERRUPT GetI8042PrtInterruptObject(void);

//
// i/o Apic related function
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

#define INP_KBD_DPC_HANDLING //
			     //  If you wanna to handle the keyboard with raw code,
			     //
			     //  remove this define.
			     //
			     //  if INP_KBD_DPC_HANDLING exists, inp.sys will be
			     //  use a DPC for keyboard handling.
			     //

/*
 *
 *
 *  	Global Variables.
 *
 */
PULONG           IOAPICGate; // mapped physical 0xfec00000 by the InpIOAPICConnection()
PULONG           OldHandler;	// Old Handler's Pointer of INT#1
KDPC             kDpc; // a DPC Object
ULONG            KeyboardInterruptVector; // keyboard vector number corresponds to irq 1
ULONG            kKeyboardHardwareIrql; // keyboard hardware irql

// MP support
long allProcessorDone;

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

volatile _declspec( naked ) void NewKeyboardHandler(void) // - Non-Paged
{
	//
	// ebp - 4   :: oldirql
	//
	#define _inp__NewKeyboardHandler_Local_variable_fullsize 0x4
	_asm
	{
		push ebp
		mov ebp, esp
		sub esp, _inp__NewKeyboardHandler_Local_variable_fullsize
		pushad
		push fs
		push ds
		push es

		mov  eax, 0x30
		mov  fs, ax
		mov  eax, 0x23
		mov  ds, ax
		mov  es, ax		
		
		//
		// Raise irql to 0x8 <-- From i8042prt's InterruptObject
		//
		xor eax, eax
		mov ecx, kKeyboardHardwareIrql
		call dword ptr [KfRaiseIrql] // KeRaiseIrql(kKeyboardHardwareIrql, ebp - 4);
		mov dword ptr [ebp - 4], eax

#ifndef INP_KBD_DPC_HANDLING
		call Inp_Mask_KeyboardInterrupt
		//
		//
		// 8042, 8048 Communication (Safe block)
		//
		//
    		mov ecx, 0xA000
     Read_0x60:	in al, 0x64
     		test al, 01b  // Output buffer full
		loopz Read_0x60

     		in al, 0x60
		mov data[0], al

		nop
		nop
		nop
	
		mov ecx, 0xA000
	Rtpb:	in al, 0x64
		test al, 00100000b // MOBFx
		jz Write0xD2
		in al, 0x60 // Drain buffer
		pause
		loop Rtpb
		//
		// Re-generating a scancode.
		//
		//     0xD2   Write Keyboard Output Register: on PS/2 systems the next data
		//            byte written to port 60h input register is written to port 60h
		//            output register as if initiated by a device;
		//            invokes interrupt if enabled
		//
     Write0xD2:	mov ecx, 0xA000
     Wait_IBF:  in al, 0x64
     		test al, 10b  // Input buffer full
		loopnz Wait_IBF
		mov al, 0xD2  // ---------------------> 0xD2
		out 0x64, al

		mov ecx, 0xA000
     Wait_IBF2: in al, 0x64
     		test al, 10b  // Input buffer full
		loopnz Wait_IBF2
		mov al, data[0] // -------------------> Scancode
		out 0x60, al

		mov ecx, 0xA000
      Wait4obf:	in al, 0x64
		test al, OUTPUT_BUFFER_FULL   // Wait until the 8042 read & process the scancode
		loopz Wait4obf

		mov data[1], al

		call Inp_Unmask_KeyboardInterrupt
#endif

		//
		// Requesting a Deferred Procedure Call
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

		mov ecx, dword ptr [ebp - 4]
		call dword ptr [KfLowerIrql] // KeLowerIrql(dword ptr [ebp - 4]);
		pop es
		pop ds
		pop fs
		popad
		add esp, _inp__NewKeyboardHandler_Local_variable_fullsize
		pop ebp
		ret
	}
	#undef _inp__NewKeyboardHandler_Local_variable_fullsize
}

void InpKeyboardDpcFunction(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
	//
	// NewKeyboardHandler's DPC function
	//
	if (pEvent)
	{
		_asm
		{
			#ifdef INP_KBD_DPC_HANDLING

			in al, 0x64
			mov data[1], al
			in al, 0x60
			mov data[0], al

			#endif
		}

		KeSetEvent(pEvent, 0, 0);
	}
}

volatile _declspec( naked ) void NewHandler(void) // - Non-Paged
{
	// General-Protection Fault handler
	//
	// buffer    == dword ptr [ebp - 8]
	// handler   == dword ptr [ebp - 8 - 4]
	//
	_asm
	{
		push ebp
		mov ebp, esp
		sub esp, 12 // local
		pushad

		mov eax, dword ptr [ebp + 4] // errorcode
		bt  eax, 1 // idt flag
		jc  is_idt_reference

		call KeGetCurrentProcessorNumber
		shl eax, 2
		add eax, OldHandler
		mov dword ptr [ebp - 8 - 4], eax
		
		popad
		add esp, 12
		pop ebp
		// jump!
		jmp dword ptr [esp - 4 - 12] // cs is already 0x08

		is_idt_reference:
			bt eax, 0
			jc external_event

			mov eax, dword ptr [ebp + 8] // eip
			xor ebx, ebx
			mov bl, byte ptr [eax]
			cmp bl, 0xCD // int n instruction
			jne external_event

			add dword ptr [ebp + 8], 2 // skip the instruction int n

		external_event:
			//
			// * register usage *
			//
			// ebx :: IDT Entry pointer
			// ecx :: Vector
			// edx :: IDT[Vector] Handler's offset
			//
			mov ecx, dword ptr [ebp + 4] //error code
			shr ecx, 3 // delete ext, idt, gdt/ldt flags
			and ecx, 0xFF // get vector
			sidt fword ptr [ebp - 8]
			// memory on the stack
			// -8 -7 | -6 -5 -4 -3  | -2 -1
			// 00 00 | 00 00 00 00  | 00 00
			// Limit | Base address |

			// Filtering my lovely :$ keyboard interrupt
			//
			cmp ecx, KeyboardInterruptVector
			jne dispatch_interrupt

			call NewKeyboardHandler // ** Hooked **

		dispatch_interrupt:
			mov ebx, dword ptr [ebp - 8 + 2] // ebx = IDTR Base offset
			shl ecx, 3 // multiplies 8
			add ebx, ecx
			mov dx, word ptr [ebx + 6]
			shl edx, 0x10
			mov dx, word ptr [ebx] // edx = IDT[Vector].BaseOffset
			mov dword ptr [ebp - 8 - 4], edx // save it

			popad
			add esp, 12 // destroy local area
			pop ebp
			add esp, 4 // remove error code
			// jump!
			jmp dword ptr [esp - 4 - 4 - 12] // cs is already 0x08
	}	
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

	//
	// Getting Hardware IRQL to corresponding to the IRQ 1
	// using i8042prt driver's Interrupt Object
	kKeyboardHardwareIrql = (ULONG)GetI8042PrtInterruptObject()->Irql;
	if (!kKeyboardHardwareIrql)
		kKeyboardHardwareIrql = 0x8; // default on single-processor x86 family
	
	DbgPrint("Keyboard Hardware IRQL :: 0x%x\n", kKeyboardHardwareIrql);

	if (!ReturnValue)
		ReturnValue = 0x93; // default ps/2 keyboard vector on wintelXP
	
	return ReturnValue;
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
		DbgPrint("IoGetDeviceObjectPointer Fail.");
		return NULL;
	}
	else
		DbgPrint("Top Object : %x\n", pDeviceObject);

	DbgPrint("Tracking %x \t/Driver/i8042prt\n", ((PR_DEVOBJ_EXTENSION)pDeviceObject->DeviceObjectExtension)->AttachedTo);

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

	DbgPrint("i8042prt.sys->Interrupt Object :: 0x%p", ReturnValue);
	
	return ReturnValue;
}

void Initialization(VOID (*FunctionPointer)(IN PKDPC, IN PVOID, IN PVOID, IN PVOID))
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

void MPInitializationThread(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
	unsigned long i = KeGetCurrentProcessorNumber();
	unsigned char    buffer[6];
	unsigned int *   IdtrBaseAddress = (unsigned int *)&buffer[2]; // Idtr->Base
	unsigned short * IdtrLimit = (unsigned short *)&buffer[0];
	PIdtEntry_t      IdtEntry;
	
	_asm sidt buffer;
  	IdtEntry=(PIdtEntry_t)*IdtrBaseAddress;	// Get a base address of idt

	OldHandler[i] = ((unsigned int)IdtEntry[0xD].OffsetHigh<<16U)|
				    (IdtEntry[0xD].OffsetLow);

	_asm nop;
	_asm nop;
	_asm nop;
	
  	_asm cli;
	IdtEntry[0xD].OffsetLow=(unsigned short)NewHandler;
 	IdtEntry[0xD].OffsetHigh=(unsigned short)((unsigned int)NewHandler>>16);

	*IdtrLimit = 0xFF;// 0 to 31 are allowed.
	//*IdtrLimit = (8 * n - 1);
	_asm lidt buffer;

	_asm sti;

	InterlockedIncrement(&allProcessorDone);
	DbgPrint("Processor [%x] :: Complete.\n", allProcessorDone);
}

void MPDisabledThread(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{	
	unsigned long i = KeGetCurrentProcessorNumber();
	unsigned char    buffer[6];
	unsigned int *   IdtrBaseAddress = (unsigned int *)&buffer[2]; // Idtr->Base
	unsigned short * IdtrLimit = (unsigned short *)&buffer[0];
	PIdtEntry_t      IdtEntry;
	
	_asm sidt buffer;
  	IdtEntry=(PIdtEntry_t)*IdtrBaseAddress;	// Get a base address of idt

	_asm cli;

	*IdtrLimit = 0xFFFF;
	_asm lidt buffer;

	IdtEntry[0xD].OffsetLow=(unsigned short)OldHandler[i];
	IdtEntry[0xD].OffsetHigh=(unsigned short)((unsigned int)OldHandler[i]>>16);
	_asm sti;

	InterlockedIncrement(&allProcessorDone);
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
	
	Initialization(MPDisabledThread);

	InpIOAPICDisconnection(&IOAPICGate);
	
	if (OldHandler)
		ExFreePool(OldHandler);

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
	KIRQL testIrql;

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

	i = KeNumberProcessors * sizeof(PULONG);
	
	OldHandler = (PULONG)ExAllocatePoolWithTag(NonPagedPool, i, (ULONG)' pni');
	RtlZeroMemory(OldHandler, i);
	/////                            /////
	//                                  //
	//             Start!!!             //
	//                                  //
	/////                            /////
	InpIOAPICConnection(&IOAPICGate);
	KeyboardInterruptVector = InpGetKeyboardInterruptVector();
	Initialization(MPInitializationThread);

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
