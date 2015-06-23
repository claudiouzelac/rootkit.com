/*
	This driver demonstrates the hooking of imported functions
	in kernel mode with two different methods.
	The first one is the well known IAT manipulation method.
	The function pointers within the IAT are changed to point to
	the hooks instead of the original functions.
	The second one is a kind of hooking that manipulates the code/data
	that refers to the function pointers within the IAT, rather than
	to alter the function pointers within the IAT directly.
	For this second method the base relocation information of the 
	PE file structure is parsed to find all those references.
	Wich method is used by this driver is controled by defines.
	See below.

	If you find any bugs please report them.
	You will find an actual email address of me at rootkit.com

	sd_ (Sumari Dancer)
*/


extern "C" {
#include "ntddk.h"
}

typedef ULONG	DWORD;
typedef PULONG	PDWORD;       
typedef USHORT	WORD;
typedef UCHAR	BYTE;

namespace NT {
#pragma warning(disable: 4005 4091)
#include "winnt.h"
#pragma warning(default: 4005 4091)
}

/*
	defines
	DO_HOOK_IAT:	hooking by changing the function pointers within the IAT
	DO_HOOK_RELOC:	hooking by changing the references to the function pointers within the IAT
	DO_HOOK:		realy hook, rather than to simulate
	SAVE_BUILD:		turn some extra checks on
	DBGPRMPT:		some string to focus Debug View on the output of this driver
	DebugPrint:		two macros, one enables, the other disables debugging output
	DebugPrintBla:	two macros, one enables, the other disables extended debugging output
*/
#define DO_HOOK_IAT
//#define DO_HOOK_RELOC
#define DO_HOOK
//#define SAVE_BUILD

#define DBGPRMPT "HookIAT: "
#define DebugPrint(Args) DbgPrint Args
//#define DebugPrint(Args)
//#define DebugPrintBla(Args) DbgPrint Args
#define DebugPrintBla(Args)

#define IOCTL_TRANSFER_TYPE(CtrlCode)   (CtrlCode & 0x3)
#define IAT_HOOK_DEVICE 0x00008822

const WCHAR DeviceLink[]  = L"\\DosDevices\\HookIAT";
const WCHAR DeviceName[]  = L"\\Device\\HookIAT";
PDEVICE_OBJECT g_HookDevice;

typedef NTSTATUS (*PIoCreateDevice)(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN PUNICODE_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN Reserved,
    OUT PDEVICE_OBJECT *DeviceObject
    );
typedef VOID (*PIoDeleteDevice)(
    IN PDEVICE_OBJECT DeviceObject
    );
typedef NTSTATUS (*PIoCreateSymbolicLink)(
    IN PUNICODE_STRING SymbolicLinkName,
    IN PUNICODE_STRING DeviceName
    );
typedef NTSTATUS (*PIoDeleteSymbolicLink)(
    IN PUNICODE_STRING SymbolicLinkName
    );

NTSTATUS
IoCreateDeviceHook(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN PUNICODE_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN Reserved,
    OUT PDEVICE_OBJECT *DeviceObject
    );

VOID
IoDeleteDeviceHook(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IoCreateSymbolicLinkHook(
    IN PUNICODE_STRING SymbolicLinkName,
    IN PUNICODE_STRING DeviceName
    );

NTSTATUS
IoDeleteSymbolicLinkHook(
    IN PUNICODE_STRING SymbolicLinkName
    );

typedef struct
{
	PSTR	RoutineName;
	PVOID	HookAddress;
	PVOID	OriginalAddress;
	PVOID*	IATAddress;
} IAT_HOOK_ENTRY, *PIAT_HOOK_ENTRY;

typedef struct
{
	PSTR	LibraryName;
	PIAT_HOOK_ENTRY Hooks;
	ULONG	HookCount;

} IAT_HOOK, *PIAT_HOOK;

enum
{
	EnIoCreateDevice,
	EnIoDeleteDevice,
	EnIoCreateSymbolicLink,
	EnIoDeleteSymbolicLink,
	EnNumberOfHooks
};

enum
{
	EnHookLibKernel,
	EnNumberOfHookedLibs
};

IAT_HOOK_ENTRY g_Entries[EnNumberOfHooks] = 
{
	{"IoCreateDevice",			(PVOID) IoCreateDeviceHook,			NULL, NULL},
	{"IoDeleteDevice",			(PVOID) IoDeleteDeviceHook,			NULL, NULL},
	{"IoCreateSymbolicLink",	(PVOID) IoCreateSymbolicLinkHook,	NULL, NULL},
	{"IoDeleteSymbolicLink",	(PVOID) IoDeleteSymbolicLinkHook,	NULL, NULL}
};

IAT_HOOK g_Hooks[EnNumberOfHookedLibs] = 
{
	{"ntoskrnl.exe", g_Entries, sizeof(g_Entries)/sizeof(IAT_HOOK_ENTRY)},
};

#if defined(DO_HOOK_RELOC)
NTSTATUS
ToggleHook();

NTSTATUS 
NTAPI
PatchCodeByRelocations(
	PUCHAR Module, 
	PVOID AddressLookFor, 
	PVOID AddressChangeTo, 
	PULONG NumberOfPatches OPTIONAL
	);
#endif defined(DO_HOOK_RELOC)

VOID 
DriverUnload(
	IN PDRIVER_OBJECT DriverObject
	);

NTSTATUS 
DriverDispatch(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS
GetIATAddressOfImportedFuction(
	IN PUCHAR Module,
	IN PCSTR LibraryName,
	IN PCSTR FunctionName,
	OUT PVOID * FunctionAddress
	);

NTSTATUS 
Hook(
	PUCHAR Module
	);

NTSTATUS
Unhook(
	PUCHAR Module
	);

inline
BOOLEAN
IsAddressValid(
	PVOID Address
	)
{
	return Address && MmIsAddressValid(Address);
}

inline 
VOID 
EnableWP()
{
	_asm
	{
		mov eax,cr0
		or eax,00010000h
		mov cr0,eax
	}
}

inline 
VOID 
DisableWP()
{
	_asm
	{
		mov eax,cr0
		and eax,not 00010000h
		mov cr0,eax
	}
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	DebugPrint((DBGPRMPT "DriverEntry start\n"));
	DebugPrint((DBGPRMPT "Driver Image %08x - %08x\n", 
		DriverObject->DriverStart, 
		(ULONG) DriverObject->DriverStart + DriverObject->DriverSize));
	__try
	{
		Hook((PUCHAR)DriverObject->DriverStart);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DebugPrint((DBGPRMPT "Exception installing hooks\n"));
	}

    UNICODE_STRING DeviceNameUnicodeString;
    UNICODE_STRING DeviceLinkUnicodeString;        

    RtlInitUnicodeString(&DeviceNameUnicodeString, DeviceName);
    RtlInitUnicodeString(&DeviceLinkUnicodeString, DeviceLink);

	NTSTATUS Status = IoCreateDevice(DriverObject, 0, &DeviceNameUnicodeString, 
		IAT_HOOK_DEVICE, 0, TRUE, &g_HookDevice );

    if(! NT_SUCCESS(Status))
	{
        DebugPrint((DBGPRMPT "Failed to create device!\n"));
        return Status;
    }
 
	Status = IoCreateSymbolicLink(&DeviceLinkUnicodeString, &DeviceNameUnicodeString);
    if(! NT_SUCCESS(Status)) 
	{
		IoDeleteDevice(DriverObject->DeviceObject);
        DebugPrint((DBGPRMPT "Failed to create symbolic link!\n"));
        return Status;
    }

    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]        =
    DriverObject->MajorFunction[IRP_MJ_CREATE]          =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           =
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = DriverDispatch;

    DriverObject->DriverUnload = DriverUnload;

#if defined(DO_HOOK_RELOC)
	ToggleHook();
#endif // defined(DO_HOOK_RELOC)

	DebugPrint((DBGPRMPT "DriverEntry exit status: %08x\n", Status));

    return Status;
}

NTSTATUS DriverDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status = Irp->IoStatus.Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation (Irp);
    ULONG IOCtrlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
	DebugPrint((DBGPRMPT "DriverDispatch control code: %08x\n", IOCtrlCode));

    switch (Stack->MajorFunction) 
	{
    case IRP_MJ_CREATE:
        break;

    case IRP_MJ_SHUTDOWN:
        break;

    case IRP_MJ_CLOSE:
        break;

    case IRP_MJ_DEVICE_CONTROL:
        break;

	default:
		Status = Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		DebugPrint((DBGPRMPT "Invalid device request\n"));
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	DebugPrint((DBGPRMPT "DriverDispatch exit status: %08x\n", Status));
	return Status;   
}

VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	DebugPrint((DBGPRMPT "DriverUnload start\n"));
    UNICODE_STRING DeviceLinkUnicodeString;
	RtlInitUnicodeString(&DeviceLinkUnicodeString, DeviceLink);
	IoDeleteSymbolicLink(&DeviceLinkUnicodeString);
	IoDeleteDevice(g_HookDevice);
	__try
	{
		Unhook((PUCHAR)DriverObject->DriverStart);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DebugPrint((DBGPRMPT "Exception uninstalling hooks\n"));
	}
	DebugPrint((DBGPRMPT "DriverUnload end\n"));
}

#if defined(DO_HOOK_RELOC)
NTSTATUS
ToggleHook()
{
	PIoCreateSymbolicLink OrigIoCreateSymbolicLink = (PIoCreateSymbolicLink) 
		g_Hooks[EnHookLibKernel].Hooks[EnIoCreateSymbolicLink].OriginalAddress;
	PIoCreateSymbolicLink HookIoCreateSymbolicLink = (PIoCreateSymbolicLink) 
		g_Hooks[EnHookLibKernel].Hooks[EnIoCreateSymbolicLink].HookAddress;
	if(!OrigIoCreateSymbolicLink || !HookIoCreateSymbolicLink)
	{
		return STATUS_UNSUCCESSFUL;
	}
	PIoCreateSymbolicLink* IoCreateSymbolicLinkPtr = (PIoCreateSymbolicLink*) 
		&g_Hooks[EnHookLibKernel].Hooks[EnIoCreateSymbolicLink].HookAddress;

	PIoDeleteSymbolicLink OrigIoDeleteSymbolicLink = (PIoDeleteSymbolicLink) 
		g_Hooks[EnHookLibKernel].Hooks[EnIoDeleteSymbolicLink].OriginalAddress;
	PIoDeleteSymbolicLink HookIoDeleteSymbolicLink = (PIoDeleteSymbolicLink) 
		g_Hooks[EnHookLibKernel].Hooks[EnIoDeleteSymbolicLink].HookAddress;
	if(!OrigIoDeleteSymbolicLink || !HookIoDeleteSymbolicLink)
	{
		return STATUS_UNSUCCESSFUL;
	}
	PIoDeleteSymbolicLink* IoDeleteSymbolicLinkPtr = (PIoDeleteSymbolicLink*) 
		&g_Hooks[EnHookLibKernel].Hooks[EnIoDeleteSymbolicLink].HookAddress;

    UNICODE_STRING DeviceNameUnicodeString;
    UNICODE_STRING DeviceLinkUnicodeString;        
	const WCHAR DummyLink[]  = L"\\DosDevices\\RelocHooker";

    RtlInitUnicodeString(&DeviceNameUnicodeString, DeviceName);
    RtlInitUnicodeString(&DeviceLinkUnicodeString, DummyLink);

	DebugPrint((DBGPRMPT "+++ Toggle hooks start\n"));
	for(int i=0; i<10; i++)
	{
		if(i%2)
		{
			*IoCreateSymbolicLinkPtr = HookIoCreateSymbolicLink;
			*IoDeleteSymbolicLinkPtr = HookIoDeleteSymbolicLink;
			DebugPrint((DBGPRMPT "--- Calls should be hooked\n"));
		}
		else
		{
			*IoCreateSymbolicLinkPtr = OrigIoCreateSymbolicLink;
			*IoDeleteSymbolicLinkPtr = OrigIoDeleteSymbolicLink;
			DebugPrint((DBGPRMPT "--- Calls should not be hooked\n"));
		}
		IoCreateSymbolicLink(&DeviceLinkUnicodeString, &DeviceNameUnicodeString);
		IoDeleteSymbolicLink(&DeviceLinkUnicodeString);
	}
	DebugPrint((DBGPRMPT "+++ Toggle hooks stop\n"));
	return STATUS_SUCCESS;
}
#endif // defined(DO_HOOK_RELOC)

NTSTATUS
IoCreateDeviceHook(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN PUNICODE_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN Reserved,
    OUT PDEVICE_OBJECT *DeviceObject
    )
{
	NTSTATUS Status = ((PIoCreateDevice) 
		g_Hooks[EnHookLibKernel].Hooks[EnIoCreateDevice].OriginalAddress)(
			DriverObject,
			DeviceExtensionSize,
			DeviceName OPTIONAL,
			DeviceType,
			DeviceCharacteristics,
			Reserved,
			DeviceObject
		);

	DebugPrint((DBGPRMPT "*** IoCreateDeviceHook: %.*ws\n",
		DeviceName ? DeviceName->Length : 0,
		DeviceName ? DeviceName->Buffer : L""));
	return Status;
}

VOID
IoDeleteDeviceHook(
    IN PDEVICE_OBJECT DeviceObject
    )
{
	((PIoDeleteDevice) g_Hooks[EnHookLibKernel].Hooks[EnIoDeleteDevice].OriginalAddress)(DeviceObject);
	DebugPrint((DBGPRMPT "*** IoDeleteDeviceHook\n"));
}

NTSTATUS
IoCreateSymbolicLinkHook(
    IN PUNICODE_STRING SymbolicLinkName,
    IN PUNICODE_STRING DeviceName
    )
{
	NTSTATUS Status = ((PIoCreateSymbolicLink) 
		g_Hooks[EnHookLibKernel].Hooks[EnIoCreateSymbolicLink].OriginalAddress)
		(SymbolicLinkName, DeviceName);

	DebugPrint((DBGPRMPT "*** IoCreateSymbolicLinkHook: %.*ws -> %.*ws\n", 
		SymbolicLinkName->Length, SymbolicLinkName->Buffer,
		DeviceName->Length, DeviceName->Buffer));
	return Status;
}

NTSTATUS
IoDeleteSymbolicLinkHook(
    IN PUNICODE_STRING SymbolicLinkName
    )
{
	DebugPrint((DBGPRMPT "*** IoDeleteSymbolicLinkHook: %.*ws\n", 
		SymbolicLinkName->Length, SymbolicLinkName->Buffer));
	NTSTATUS Status = ((PIoDeleteSymbolicLink) 
		g_Hooks[EnHookLibKernel].Hooks[EnIoDeleteSymbolicLink].OriginalAddress)
		(SymbolicLinkName);
	return Status;
}

NTSTATUS 
Hook(
	PUCHAR Module
	)
{
	for(unsigned i=0; i< sizeof(g_Hooks)/sizeof(IAT_HOOK); i++)
	{
		for(unsigned e=0; e<g_Hooks[i].HookCount; e++)
		{
			DebugPrintBla((DBGPRMPT "GetIATAddressOfImportedFuction: %s@%s: %08x, %08x, %08x\n", 
				g_Hooks[i].Hooks[e].RoutineName, g_Hooks[i].LibraryName,
				g_Hooks[i].Hooks[e].IATAddress, g_Hooks[i].Hooks[e].HookAddress, g_Hooks[i].Hooks[e].OriginalAddress));

			NTSTATUS Status = GetIATAddressOfImportedFuction(
				Module,
				g_Hooks[i].LibraryName,
				g_Hooks[i].Hooks[e].RoutineName,
				(PVOID*) &g_Hooks[i].Hooks[e].IATAddress);

			if(!NT_SUCCESS(Status))
			{
				return Status;
			}

			DebugPrint((DBGPRMPT "IAT Address of: %s@%s: %08x [->%08x]\n", 
				g_Hooks[i].Hooks[e].RoutineName, g_Hooks[i].LibraryName,
				g_Hooks[i].Hooks[e].IATAddress, *g_Hooks[i].Hooks[e].IATAddress));

			g_Hooks[i].Hooks[e].OriginalAddress = *g_Hooks[i].Hooks[e].IATAddress;
		}
	}

	for(unsigned i=0; i< sizeof(g_Hooks)/sizeof(IAT_HOOK); i++)
	{
		for(unsigned e=0; e<g_Hooks[i].HookCount; e++)
		{
#if defined(DO_HOOK_IAT) && defined(DO_HOOK)
			_asm cli;
			DisableWP();
			InterlockedExchangePointer(g_Hooks[i].Hooks[e].IATAddress, g_Hooks[i].Hooks[e].HookAddress);
			EnableWP();
			_asm sti;
#elif defined(DO_HOOK_RELOC)
			ULONG NumberOfPatches;
			NTSTATUS Status = PatchCodeByRelocations(
				Module, 
				g_Hooks[i].Hooks[e].IATAddress, 
				&g_Hooks[i].Hooks[e].HookAddress, 
				&NumberOfPatches);
			if(!NT_SUCCESS(Status))
			{
				return Status;
			}
			DebugPrintBla((DBGPRMPT "Hook:   %d references to %s@%s patched,   %08x -> %08x\n",
				NumberOfPatches, g_Hooks[i].Hooks[e].RoutineName, g_Hooks[i].LibraryName,
				g_Hooks[i].Hooks[e].IATAddress, g_Hooks[i].Hooks[e].HookAddress));
#else
			DebugPrintBla((DBGPRMPT "Hook:   IAT: %08x, original: %08x, hook: %08x\n"));
#endif
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS
Unhook(
	PUCHAR Module
	)
{
	for(unsigned i=0; i< sizeof(g_Hooks)/sizeof(IAT_HOOK); i++)
	{
		for(unsigned e=0; e<g_Hooks[i].HookCount; e++)
		{
#if defined(DO_HOOK_IAT) && defined(DO_HOOK)
			if(!g_Hooks[i].Hooks[e].IATAddress || !g_Hooks[i].Hooks[e].OriginalAddress)
			{
				return STATUS_UNSUCCESSFUL;
			}
			_asm cli;
			DisableWP();
			InterlockedExchangePointer(g_Hooks[i].Hooks[e].IATAddress, g_Hooks[i].Hooks[e].OriginalAddress);
			EnableWP();
			_asm sti;
#elif defined(DO_HOOK_RELOC)
			ULONG NumberOfPatches;
			NTSTATUS Status = PatchCodeByRelocations(
				Module, 
				&g_Hooks[i].Hooks[e].HookAddress, 
				g_Hooks[i].Hooks[e].IATAddress, 
				&NumberOfPatches);
			if(!NT_SUCCESS(Status))
			{
				return Status;
			}
			DebugPrintBla((DBGPRMPT "Unhook: %d references to %s@%s unpatched, %08x -> %08x\n",
				NumberOfPatches, g_Hooks[i].Hooks[e].RoutineName, g_Hooks[i].LibraryName,
				g_Hooks[i].Hooks[e].HookAddress, g_Hooks[i].Hooks[e].IATAddress));
#else
			DebugPrintBla((DBGPRMPT "Unhook: IAT: %08x, original: %08x, hook: %08x\n",
				g_Hooks[i].Hooks[e].IATAddress, 
				g_Hooks[i].Hooks[e].OriginalAddress, 
				g_Hooks[i].Hooks[e].HookAddress));
#endif
		}
	}
	return STATUS_SUCCESS;
}

NT::PIMAGE_NT_HEADERS
GetImageNtHeaders(
	PUCHAR Module
	)
{
#if defined(SAVE_BUILD)
	if(!IsAddressValid(Module))
	{
		DebugPrint((DBGPRMPT "!IsAddressValid(Module)\n"));
		return NULL;
	}
#endif //  defined(SAVE_BUILD)
	NT::PIMAGE_DOS_HEADER ImageDosHeader = (NT::PIMAGE_DOS_HEADER) Module;
	if(IMAGE_DOS_SIGNATURE != ImageDosHeader->e_magic)
	{
		DebugPrint((DBGPRMPT "e_magic != IMAGE_DOS_SIGNATURE\n"));
		return NULL;
	}
	NT::PIMAGE_NT_HEADERS NtHeaders = (NT::PIMAGE_NT_HEADERS) (ImageDosHeader->e_lfanew + Module);
	if(IMAGE_NT_SIGNATURE != NtHeaders->Signature)
	{
		DebugPrint((DBGPRMPT "Signature != IMAGE_NT_SIGNATURE\n"));
		return NULL;
	}
	DebugPrintBla((DBGPRMPT "NtHeaders at %08x\n", INtHeaders));
	return NtHeaders;
}


NTSTATUS
GetIATAddressOfImportedFuction(
	IN PUCHAR Module,
	IN PCSTR LibraryName,
	IN PCSTR FunctionName,
	OUT PVOID * FunctionAddress
	)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
#if defined(SAVE_BUILD)
	if(!IsAddressValid((PVOID)LibraryName) || !IsAddressValid((PVOID)FunctionName))
	{
		DebugPrint((DBGPRMPT "!IsAddressValid((PVOID)LibraryName) || !IsAddressValid((PVOID)FunctionName)\n"));
		return Status;
	}
#endif //  defined(SAVE_BUILD)
	*FunctionAddress = NULL;

	NT::PIMAGE_NT_HEADERS NtHeaders = GetImageNtHeaders(Module);
	if(!NtHeaders)
	{
		DebugPrint((DBGPRMPT "NtHeaders not found\n"));
		return Status;
	}

	ULONG VirtualAddress = NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	DebugPrintBla((DBGPRMPT "Virtual Address of Import Directory: %08x\n", VirtualAddress));

	NT::PIMAGE_IMPORT_DESCRIPTOR ImportDir = (NT::PIMAGE_IMPORT_DESCRIPTOR) (Module + VirtualAddress);

#if defined(SAVE_BUILD)
	if(!IsAddressValid(ImportDir))
	{
		DebugPrint((DBGPRMPT "Import directory not found\n"));
		return Status;
	}
#endif //  defined(SAVE_BUILD)

	DebugPrintBla((DBGPRMPT "Import directory found at %08x, first thunk: %08x\n", 
		ImportDir, ImportDir->FirstThunk));

	for(; ImportDir->FirstThunk; ImportDir++)
	{
		PCSTR LibName = (PCSTR) (ImportDir->Name + Module );
#if defined(SAVE_BUILD)
		if(!IsAddressValid((PVOID)LibName))
		{
			DebugPrint((DBGPRMPT "!IsAddressValid(LibName) %08x\n", LibName));
			continue;
		}
#endif //  defined(SAVE_BUILD)
		if(!_stricmp(LibName, LibraryName))
		{
			DebugPrintBla((DBGPRMPT "Library entry found\n"));
			NT::PIMAGE_THUNK_DATA OrigIT = (NT::PIMAGE_THUNK_DATA)
				(Module + ImportDir->OriginalFirstThunk);
			NT::PIMAGE_THUNK_DATA AddrIT = (NT::PIMAGE_THUNK_DATA)
				(Module + ImportDir->FirstThunk);

#if defined(SAVE_BUILD)
			if(!IsAddressValid(OrigIT) || !IsAddressValid(AddrIT))
			{
				DebugPrint((DBGPRMPT "!IsAddressValid(OrigIT) || !!IsAddressValid(AddrIT)\n"));
				DebugPrint((DBGPRMPT "OrigIT: %08x, AddrIT: %08x\n", OrigIT, AddrIT));
				return Status;
			}
#endif //  defined(SAVE_BUILD)

			for(;OrigIT->u1.AddressOfData; OrigIT++, AddrIT++)
			{
				NT::PIMAGE_IMPORT_BY_NAME ImportByName = (NT::PIMAGE_IMPORT_BY_NAME) OrigIT->u1.AddressOfData;
#if defined(SAVE_BUILD)
				if(!IsAddressValid(ImportByName))
				{
					DebugPrint((DBGPRMPT "!IsAddressValid(ImportByName)\n"));
					return Status;
				}
#endif //  defined(SAVE_BUILD)
				PCSTR FuncName = (PCSTR) ImportByName->Name;
#if defined(SAVE_BUILD)
				if(!IsAddressValid((PVOID)FuncName))
				{
					DebugPrint((DBGPRMPT "!IsAddressValid(FuncName)\n"));
					return Status;
				}
#endif //  defined(SAVE_BUILD)
				if(!strcmp(FuncName, FunctionName))
				{
					DebugPrintBla((DBGPRMPT "IAT entry found, address of entry %08x, address of function %08x \n", 
						&AddrIT->u1.Function, AddrIT->u1.Function));
					*FunctionAddress = (PVOID) &AddrIT->u1.Function;
					Status = STATUS_SUCCESS;
					break;
				}
			}
			break;
		}
	}

	return Status;
}

#if defined(DO_HOOK_RELOC)
NTSTATUS 
NTAPI
PatchCodeByRelocations(
	PUCHAR Module, 
	PVOID AddressLookFor, 
	PVOID AddressChangeTo, 
	PULONG NumberOfPatches OPTIONAL)
{
	DebugPrintBla((DBGPRMPT "Looking for base relocations pointing to %08x\n", AddressLookFor));
	ULONG Patches = 0;
	if(NumberOfPatches)
	{
		*NumberOfPatches = Patches;
	}

	NT::PIMAGE_NT_HEADERS NtHeaders = GetImageNtHeaders(Module);
	if(!NtHeaders)
	{
		DebugPrint((DBGPRMPT "NtHeaders not found\n"));
		return NULL;
	}

	ULONG VirtualAddress = NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
	DebugPrintBla((DBGPRMPT "Virtual Address of Base Reloction Directory: %08x\n", VirtualAddress));
	NT::PIMAGE_BASE_RELOCATION	RelocDir = (NT::PIMAGE_BASE_RELOCATION) (Module + VirtualAddress);
#if defined(SAVE_BUILD)
	if(!IsAddressValid(RelocDir))
	{
		DebugPrint((DBGPRMPT "Base relocation directory not found\n"));
		return STATUS_UNSUCCESSFUL;
	}
#endif //  defined(SAVE_BUILD)
	DebugPrintBla((DBGPRMPT "Base relocation directory found at %08x, size of first block %08x\n", 
		RelocDir, RelocDir->SizeOfBlock));
	for(; RelocDir->SizeOfBlock; RelocDir = (NT::PIMAGE_BASE_RELOCATION) 
		(RelocDir->SizeOfBlock + (ULONG) RelocDir))
	{
		ULONG Count = (RelocDir->SizeOfBlock - sizeof(NT::IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
		ULONG BlockRVA = RelocDir->VirtualAddress;
		PUSHORT Relocs = (PUSHORT) (sizeof(NT::IMAGE_BASE_RELOCATION) + (ULONG) RelocDir);
#if defined(SAVE_BUILD)
		if(!IsAddressValid(Relocs))
		{
			DebugPrint((DBGPRMPT "!IsAddressValid(Relocs)\n"));
			return STATUS_UNSUCCESSFUL;
		}
#endif //  defined(SAVE_BUILD)
		DebugPrintBla((DBGPRMPT "Base relocation block, %d entries, RVA: %08x\n", Count, BlockRVA));
		for(ULONG i=0; i<Count; i++)
		{
			USHORT Reloc = Relocs[i];
			UCHAR RelocType = ((Reloc&0xf000) >> 12);
			Reloc &= 0x0fff;
			PVOID* RelocPointer = NULL;
			switch(RelocType)
			{
			case IMAGE_REL_BASED_ABSOLUTE:
				break;
			case IMAGE_REL_BASED_HIGH:
				RelocPointer = (PVOID*) (Module + Reloc + (BlockRVA&0xffff));
				break;
			case IMAGE_REL_BASED_LOW:
				RelocPointer = (PVOID*) (Module + Reloc + (BlockRVA>>16));
				break;
			case IMAGE_REL_BASED_HIGHLOW:
				RelocPointer = (PVOID*) (Module + Reloc + BlockRVA);
				break;
			default:
				DebugPrint((DBGPRMPT "Unsupported base relocation type: %x %03x\n", RelocType, Reloc));
				break;
			}
			if(!RelocPointer)
			{
				continue;
			}
			DebugPrintBla((DBGPRMPT "Base relocation entry points to %08x [RVA: %08x]\n", 
				RelocPointer, (PUCHAR)RelocPointer - Module));
#if defined(SAVE_BUILD)
			if(!IsAddressValid(RelocPointer))
			{
				DebugPrint((DBGPRMPT "!IsAddressValid(RelocPointer)\n"));
				return STATUS_UNSUCCESSFUL;
			}
#endif //  defined(SAVE_BUILD)
			DebugPrintBla((DBGPRMPT "Base relocation entry value is  %08x [RVA: %08x]\n", 
				*RelocPointer, (PUCHAR)*RelocPointer - Module));
			if(*RelocPointer == AddressLookFor)
			{
				DebugPrintBla((DBGPRMPT "Base relocation entry found, %08x refers to %08x\n", 
					RelocPointer, AddressLookFor));
				if(AddressChangeTo)
				{
					DebugPrint((DBGPRMPT "Patching %08x refering to %08x [->%08x] to refer to %08x [->%08x]\n", 
						RelocPointer, AddressLookFor, *(PVOID*)AddressLookFor, 
						AddressChangeTo, *(PVOID*)AddressChangeTo));
#ifdef DO_HOOK
					_asm cli;
					DisableWP();
					InterlockedExchangePointer(RelocPointer, AddressChangeTo);
					EnableWP();
					_asm sti;
#endif // DO_HOOK
				}
				Patches++;
			}
		}
	}
	if(NumberOfPatches)
	{
		*NumberOfPatches = Patches;
	}
	return (Patches > 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}
#endif // defined(DO_HOOK_RELOC)

