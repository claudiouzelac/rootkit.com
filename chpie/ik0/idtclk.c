/* idtclk.c
 *
 *
 *	- x86 Limit trap based IDT memory cloaking -
 *
 *
 *      Beist security research group
 *
 * 	contact :: chpie@naver.com
 *		   http://beist.org/
 *                 http://chpie.tistory.com/
 *
 */
#include <ntddk.h>
#include "idtclk.h"

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

#define NT_DEVICE_NAME  L"\\Device\\IDTCLK" // device name and symbolic link name
#define DOS_DEVICE_NAME L"\\DosDevices\\IDTCLK"

/*
 *
 *
 *  	Global Variables.
 *
 */

// MP support
PIdtEntry_t * realIDT; // double pointer
PULONG OldHandler; // Back-up ptr of the General_Protection_Fault handler
long allProcessorDone;

/*
 *
 *          	Codes
 *
 */

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
		mov eax, [eax]
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

			call KeGetCurrentProcessorNumber
			shl eax, 2
			add eax, realIDT
			mov ebx, [eax] // ebx = IDTR Base offset = realIDT]
			
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

	*IdtrLimit = 0xF8;// 0 to 31 are allowed.
	//*IdtrLimit = (8 * (n - 1));
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

void MPCopyIDT(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2) // make a duplicated IDT
{
	unsigned long i = KeGetCurrentProcessorNumber();
	unsigned char    buffer[6];
	unsigned int *   IdtrBaseAddress = (unsigned int *)&buffer[2]; // Idtr->Base
	unsigned short * IdtrLimit = (unsigned short *)&buffer[0];
	PIdtEntry_t      IdtEntry;
	
	_asm sidt buffer;
  	IdtEntry=(PIdtEntry_t)*IdtrBaseAddress;	// Get a base address of idt

	_asm cli;
	memcpy(realIDT[i], IdtEntry, sizeof(IdtEntry_t) * 0xFF);
	_asm sti;

	InterlockedIncrement(&allProcessorDone);
}

void MPDirtyIDT(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2) // make a duplicated IDT
{
	unsigned long i = KeGetCurrentProcessorNumber();
	unsigned char    buffer[6];
	unsigned int *   IdtrBaseAddress = (unsigned int *)&buffer[2]; // Idtr->Base
	unsigned short * IdtrLimit = (unsigned short *)&buffer[0];
	PIdtEntry_t      IdtEntry;
	
	_asm sidt buffer;
  	IdtEntry=(PIdtEntry_t)*IdtrBaseAddress;	// Get a base address of idt

	_asm cli;
	memset(&IdtEntry[32], 0x41, sizeof(IdtEntry_t) * 0xFF);
	_asm sti;

	InterlockedIncrement(&allProcessorDone);
}

void MPRestoreIDT(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2) // make a duplicated IDT
{
	unsigned long i = KeGetCurrentProcessorNumber();
	unsigned char    buffer[6];
	unsigned int *   IdtrBaseAddress = (unsigned int *)&buffer[2]; // Idtr->Base
	unsigned short * IdtrLimit = (unsigned short *)&buffer[0];
	PIdtEntry_t      IdtEntry;
	
	_asm sidt buffer;
  	IdtEntry=(PIdtEntry_t)*IdtrBaseAddress;	// Get a base address of idt

	_asm cli;
	memcpy(IdtEntry, realIDT[i], sizeof(IdtEntry_t) * 0xFF);
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
	int loop;
	
	Initialization(MPRestoreIDT);
	Initialization(MPDisabledThread);

	if (OldHandler)
		ExFreePool(OldHandler);

	for (loop = 0; loop < KeNumberProcessors; loop++)
	{
		ExFreePool(realIDT[loop]);
	}
	ExFreePool(realIDT);
		
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
	int i, loop;
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

	// make non-paged IDT area
	realIDT = (PIdtEntry_t *)ExAllocatePoolWithTag(NonPagedPool, i, (ULONG)'sh17');

	for (loop = 0; loop < i / sizeof(PULONG); loop ++)
	{
		realIDT[loop] = (PIdtEntry_t)ExAllocatePoolWithTag(NonPagedPool, sizeof(IdtEntry_t) * 0xFF, (ULONG)'sh17');
		RtlZeroMemory(realIDT[loop], sizeof(IdtEntry_t) * 0xFF);
	}	

	/////                            /////
	//                                  //
	//             Start!!!             //
	//                                  //
	/////                            /////
	Initialization(MPCopyIDT);
	Initialization(MPInitializationThread);
	Initialization(MPDirtyIDT);

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
