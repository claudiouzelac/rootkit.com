// BASIC DEVICE DRIVER

#include "ntddk.h"

#pragma pack(1)
typedef struct ServiceDescriptorEntry {
	unsigned int *ServiceTableBase;
	unsigned int *ServiceCounterTableBase; //Used only in checked build
	unsigned int NumberOfServices;
	unsigned char *ParamTableBase;
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()

__declspec(dllimport)  ServiceDescriptorTableEntry_t KeServiceDescriptorTable;
#define SYSTEMSERVICE(_function)  KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)_function+1)]

// Length of process name (rounded up to next DWORD)
#define PROCNAMELEN     20
// Maximum length of NT process name
#define NT_PROCNAMELEN  16

ULONG gProcessNameOffset;


// function prototype
NTSYSAPI
NTSTATUS
NTAPI ZwQuerySystemInformation(
            IN ULONG SystemInformationClass,
                        IN PVOID SystemInformation,
                        IN ULONG SystemInformationLength,
                        OUT PULONG ReturnLength);

// typedef so we can make a working call thru the saved pointer
typedef NTSTATUS (*ZWQUERYSYSTEMINFORMATION)(
            ULONG SystemInformationCLass,
			PVOID SystemInformation,
			ULONG SystemInformationLength,
			PULONG ReturnLength
);

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
		DbgPrint("BHWIN: NewZwQuerySystemInformation() from %s\n", aProcessName);


        rc = ((ZWQUERYSYSTEMINFORMATION)(OldZwQuerySystemInformation)) (
                        SystemInformationClass,
                        SystemInformation,
                        SystemInformationLength,
                        ReturnLength );

		DbgPrint(" real ZwQuerySystemInfo returned %d", rc);

		return rc;
}

VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("BHWIN: OnUnload called\n");

	// put back the old function pointer
	InterlockedExchange( (PLONG) &SYSTEMSERVICE(ZwQuerySystemInformation), 
						 (LONG) OldZwQuerySystemInformation);

}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	DbgPrint("BHWin is al1v3!");

	GetProcessNameOffset();

	theDriverObject->DriverUnload  = OnUnload; 

	// place the hook using InterlockedExchange (no need to disable interrupts)
	// this uses the LOCK instruction to lock the memory bus during the next instruction 
	// Example:
	// LOCK INC DWORD PTR [EDX+04] 
	// This staves off collisions on multi-processor machines, while cli/sti only disable interrupts
	// on the current processor.
	//
	OldZwQuerySystemInformation = 
		(ZWQUERYSYSTEMINFORMATION) InterlockedExchange(		(PLONG) &SYSTEMSERVICE(ZwQuerySystemInformation), 
															(LONG) NewZwQuerySystemInformation);

	return STATUS_SUCCESS;
}

