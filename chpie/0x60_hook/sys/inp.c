/* inp.c
 *
 *
 *	- Global Specific I/O Address Space Trap -
 *
 *
 * 	http://chpie.org
 *
 *
 *
 * 2007. 2 : first build.
 *
 */
#include <ntddk.h>
#include "inp.h"

// Function prototypes
VOID DrvUnload(IN PDRIVER_OBJECT);
NTSTATUS DriverEntry(IN PDRIVER_OBJECT, IN PUNICODE_STRING);
NTSTATUS IoDeviceControl(IN	PDEVICE_OBJECT, IN PIRP);
NTSTATUS IoReadWrite(IN	PDEVICE_OBJECT, IN PIRP);
NTSTATUS IoDispatch(IN PDEVICE_OBJECT, IN PIRP);
void Initialization(VOID (*FunctionPointer)(IN PKDPC, IN PVOID, IN PVOID, IN PVOID));
void MPInitializationThread(IN PKDPC, IN PVOID, IN PVOID, IN PVOID);
void MPDisabledDebugRegisters(IN PKDPC, IN PVOID, IN PVOID, IN PVOID);
void MPDisabledThread(IN PKDPC, IN PVOID, IN PVOID, IN PVOID);
void BreakpointSetup(void);
void HandlerSetup(void);
volatile void NewHandler(void);

/*
 *
 *      Structures
 *
 */
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(NONPAGED, NewHandler)
#pragma alloc_text(PAGE, DrvUnload)
#pragma alloc_text(PAGE, IoDeviceControl)
#pragma alloc_text(PAGE, IoReadWrite)
#pragma alloc_text(PAGE, IoDispatch)
#pragma alloc_text(PAGE, Initialization)
#pragma alloc_text(PAGE, MPInitializationThread)
#pragma alloc_text(PAGE, BreakpointSetup)
#pragma alloc_text(PAGE, HandlerSetup)
#endif

#define NT_DEVICE_NAME  L"\\Device\\INP" // device name and symbolic link name
#define DOS_DEVICE_NAME L"\\DosDevices\\INP"

#define IOCTL_REQUEST_DATA        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_REGISTER_EVENT      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STOP_DEBUG_REGISTER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define EXIT_SIGNAL 0x1
#define CR4_DE_BIT 0x8
#define DR7_BP0_ENABLE_BIT 0xE0002
#define DR7_GENERAL_DETECTION_BIT 0x2000
#define DR7_INIT_IMAGE 0x00000400

/*
 *
 *
 *  	Global Variables.
 *
 */
PULONG           OldHandler;	// Old Handler's Pointer of INT#1

// Multi-processor
long allProcessorDone;
unsigned char * Packet; //Must referenced with KeGetCurrentProcessorNumber() !!!

// communication
PKEVENT pEvent = NULL;
UCHAR   data[2];

/*
 *
 *          	Codes
 *
 */

volatile _declspec( naked ) void NewHandler(void) // - Non-Paged
{
	//
	//	- Interrupt 1 Handler -
	//
	//  offset   | contains
	//  ---------+-----------------------------
	//  ebp + 10 : EFLAGS Context
	//  ebp + 8  : CS  Context
	//  ebp + 4  : EIP Context
	//	ebp      : old-aged ebp
	//  ebp - 4  : DR_6 Frame
	//  ebp - 8  : KeGetCurrentProcessorNumber
	//
	#define LOCAL_STACK_VALUE 0x8
	_asm 
	{
		push ebp
		mov  ebp, esp
		sub  esp, LOCAL_STACK_VALUE
		
		pushad
		push fs
		push ds
		push es

		mov  eax, 0x00000030
		mov  fs, ax
		mov  eax, 0x00000023
		mov  ds, ax
		mov  es, ax

		mov eax, dr6
		mov dword ptr [ebp - 4], eax
	
		call KeGetCurrentProcessorNumber
		mov dword ptr [ebp - 8], eax

		// Check EXIT PACKET
		mov eax, Packet		// ref Packet[KeGetCurrentProcessorNumber()]
		add eax, dword ptr [ebp - 8]
		cmp [eax], EXIT_SIGNAL
		je Exit_Command_Raised

		mov eax, dword ptr [ebp - 4]

		// Check General Detection Raised
		btr eax, 13 // General-Detection Bit in the DR6
		jc  General_Detection_Raised

		// Check Break-point condition #0 Raised
		btr eax, 0
		jc  Break_Point_0_Trap
		
		jmp Exit_Process
		
	Exit_Command_Raised:
		mov dword ptr [ebp - 4], eax // clear dr6's BD Flag(bit offset 13)
		add dword ptr [ebp + 4], 0x3 // eip_image + 3
		jmp Exit_on_Raise

	General_Detection_Raised:
		mov dword ptr [ebp - 4], eax // clear dr6's BD Flag(bit offset 13)
		add dword ptr [ebp + 4], 0x3 // eip_image + 3
		jmp Exit_Process

	Break_Point_0_Trap:
		mov dword ptr [ebp - 4], eax // clear dr6's B0 Flag(bit offset 0)
		
		//
		// Filtering PS/2 mouse signal
		//
		in al, 0x64
		cmp al, 0x14
		jne Exit_Process

		//
		// BP0_Keyboard_Raised:
		//
		// :: Dispatch Keyboard data transmission.
		//
		//
		// Pseudo code
		//
		// if (KeyboardType == PS/2 Compatible)
		//    Parse Opcode (0xEC, 0xED is 1byte, 0xE4, 0xE5 is 2byte align)
		//    if Opcode == 'IN' Class
		//       data[0] = AL image in the stack
		//       KeSetEvent(pEvent)
		//    else
		//       // It is 'OUT' class opcode
		// 
			mov data[1], al
			//BP0_Input_Opcode_Check:
			mov ecx, dword ptr [ebp + 4]
			dec ecx
			mov bl, byte ptr [ecx]
			cmp bl, 0xEC
			je  BP0_Send_Packet_To_App
			cmp bl, 0xED
			je  BP0_Send_Packet_To_App

			//BP0_Input_Opcode_Check_Next:
			dec ecx
			mov bl, byte ptr [ecx]
			cmp bl, 0xE4
			je  BP0_Send_Packet_To_App
			cmp bl, 0xE5
			jne Exit_process

			BP0_Send_Packet_To_App:
				xor ecx, ecx
				mov ecx, dword ptr [ebp - LOCAL_STACK_VALUE - 4] // Eax(pushad) in the stack
				mov data[0], cl
				cmp pEvent, 0
				je BP0_Packet_not_requested
				push 0 // Wait
				push 0 // Increment
				mov eax, pEvent
				push eax
				call ds:KeSetEvent

				jmp Exit_Process // debug
			
			BP0_Packet_not_requested:
		
		jmp Exit_Process
		

	Exit_Process:
		mov eax, dword ptr [ebp - 4]
		mov dr6, eax
		mov eax, dr7
		or  eax, DR7_GENERAL_DETECTION_BIT	// Re-enable General Detection
		mov dr7, eax

	Exit_on_Raise:
		pop es
		pop ds
		pop fs
		popad
		add esp, LOCAL_STACK_VALUE
		pop ebp
		iretd
	}
}

void Initialization(VOID (*FunctionPointer)(IN PKDPC, IN PVOID, IN PVOID, IN PVOID))
{
	/*
	 *
	 * Multi-Processor Consideration ::
	 *
	 * Each processor has it's own IDT and Debug registers.
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

	pkDpc = (PKDPC)ExAllocatePool(NonPagedPool, KeNumberProcessors * sizeof(KDPC));
	
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
	HandlerSetup();
	_asm cli;
	BreakpointSetup();
	_asm sti;	

	InterlockedIncrement(&allProcessorDone);
	DbgPrint("allProcessorDone :: %x\n", allProcessorDone);
}

void MPDisabledThread(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{	
	unsigned long i = KeGetCurrentProcessorNumber();
	unsigned char    buffer[6];
	unsigned int *   IdtrBaseAddress = (unsigned int *)&buffer[2]; // Idtr->Base
	PIdtEntry_t      IdtEntry;
	
	_asm sidt buffer;
  	IdtEntry=(PIdtEntry_t)*IdtrBaseAddress;	// Get a base address of idt

	_asm cli;
	IdtEntry[1].OffsetLow=(unsigned short)OldHandler[i];
	IdtEntry[1].OffsetHigh=(unsigned short)((unsigned int)OldHandler[i]>>16);
	IdtEntry[1].Dpl = 0x3;
	_asm sti;

	InterlockedIncrement(&allProcessorDone);
}

void MPDisabledDebugRegisters(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
	unsigned long i = KeGetCurrentProcessorNumber();

	Packet[i] = EXIT_SIGNAL;	

	_asm mov eax, dr7;			// touch Trap!!!
	_asm mov eax, dr7;

	_asm
	{
		xor eax, eax
		mov dr0, eax
		mov eax, DR7_INIT_IMAGE	// Double check
		mov dr7, eax
		mov eax, dr6
		btr eax, 13
		btr eax, 0
		mov dr6, eax
	}

	InterlockedIncrement(&allProcessorDone);
}

void BreakpointSetup(void)
{	
	_asm
	{
		push eax

		_emit 0x0f      	// mov eax, cr4(0F20E0)
		_emit 0x20 
		_emit 0xe0
		or eax, CR4_DE_BIT	// Enable Debug Extension
		_emit 0x0f      	// mov cr4, eax(0F22E0)
		_emit 0x22 
		_emit 0xe0
	
		mov eax, 0x60		        	   // 0x60 Port
		mov dr0, eax
		
		mov eax, DR7_INIT_IMAGE
		or  eax, DR7_BP0_ENABLE_BIT 	   // Enable Hardware Breakpoint 0
		or  eax, DR7_GENERAL_DETECTION_BIT // Enable General Detection
		mov dr7, eax

		pop eax
	}
}

void HandlerSetup()
{
	unsigned long i = KeGetCurrentProcessorNumber();
	unsigned char    buffer[6];
	unsigned int *   IdtrBaseAddress = (unsigned int *)&buffer[2]; // Idtr->Base
	PIdtEntry_t      IdtEntry;
	
	_asm sidt buffer;
  	IdtEntry=(PIdtEntry_t)*IdtrBaseAddress;	// Get a base address of idt

	OldHandler[i] = ((unsigned int)IdtEntry[1].OffsetHigh<<16U)|
				    (IdtEntry[1].OffsetLow);

	IdtEntry[1].Dpl = 0x0; // adjust DPL

	_asm nop;
	_asm nop;
	_asm nop;
	
  	_asm cli;
	IdtEntry[1].OffsetLow=(unsigned short)NewHandler;
 	IdtEntry[1].OffsetHigh=(unsigned short)((unsigned int)NewHandler>>16);
	_asm sti;
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
	
	Initialization(MPDisabledDebugRegisters);
	Initialization(MPDisabledThread);
	
	if (Packet)
		ExFreePool(Packet);
	
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

	i = KeNumberProcessors * sizeof(unsigned char);
	
	Packet = (unsigned char * )ExAllocatePool(NonPagedPool, i);	// Init exit packet
	RtlZeroMemory(Packet, i);
	
	i = KeNumberProcessors * sizeof(PULONG);
	
	OldHandler = (PULONG)ExAllocatePool(NonPagedPool, i);
	RtlZeroMemory(OldHandler, i);
	/////                            /////
	//                                  //
	//             Start!!!             //
	//                                  //
	/////                            /////
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
