/*++

Module Name:

    internal.h

Abstract:

    Internal header.

Author:

    90210 5-Dec-2004

--*/

#ifndef _PH2_INTERNAL_
#define _PH2_INTERNAL_

#include <ntddk.h>
#include "search.h"
#include "pullout.h"


#define ClearMember(Member, Set) \
    Set = Set & (~(1 << (Member)))

#define SetMember(Member, Set) \
    Set = Set | (1 << (Member))

typedef struct {
	ULONG	ET_ApcState_Process;
	ULONG	ET_WaitListEntry;
	ULONG	EP_SwapListEntry;
	ULONG	EP_ActiveProcessLinks;
} STRUCTURE_OFFSETS, *PSTRUCTURE_OFFSETS;

typedef struct _SYSTEM_MODULE_INFORMATION {
    ULONG   Reserved[2];
    PVOID   Base;
    ULONG   Size;
    ULONG   Flags;
    USHORT  Index;
    USHORT  Unknown;
    USHORT  LoadCount;
    USHORT  ModuleNameOffset;
    CHAR    ImageName[256];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef struct {
    ULONG	dwNumberOfModules;
    SYSTEM_MODULE_INFORMATION	smi;
} MODULES, *PMODULES;

#define	SystemModuleInformation	11

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID	DllBase;
	PVOID	EntryPoint;
	ULONG	SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG	Flags;
	USHORT	LoadCount;
	USHORT	TlsIndex;
	PVOID	SectionPointer;
	ULONG	CheckSum;
	ULONG	TimeDateStamp;
	PVOID	EntryPointActivationContext;
	PVOID	PatchInformation;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;


#define	NUMBER_OF_THREADS_LISTS	3
#define NUMBER_OF_PROCESSES_LISTS	2

#include <pshpack1.h>
typedef struct {
	PLIST_ENTRY	KiDispatcherReadyListHead;
	PLIST_ENTRY	Threads[NUMBER_OF_THREADS_LISTS];		// KiWaitInListHead, 
														// KiWaitOutListHead, 
														// KiStackInSwapListHead

	PLIST_ENTRY Processes[NUMBER_OF_PROCESSES_LISTS];	// KiProcessInSwapListHead, 
														// KiProcessOutSwapListHead
} SCHEDULER_LISTS, *PSCHEDULER_LISTS;

typedef struct {
	ULONG	KiDispatcherReadyListHead;
	ULONG	Threads[NUMBER_OF_THREADS_LISTS];		// KiWaitInListHead, 
													// KiWaitOutListHead, 
													// KiStackInSwapListHead, 
	ULONG	Processes[NUMBER_OF_PROCESSES_LISTS];	// KiProcessInSwapListHead, 
													// KiProcessOutSwapListHead
} SCHEDULER_LISTS_RVAS, *PSCHEDULER_LISTS_RVAS;

#include <poppack.h>

NTSTATUS
NTAPI
ZwQuerySystemInformation(    
    ULONG    SystemInformationClass,
    PVOID    SystemInformation,
    ULONG    SystemInformationLength,
    PULONG   ReturnLength
    );


typedef enum _EXCLUDER_EVENT {
	TimerExpiration,
	ShutdownEvent,
	MaximumObject
} EXCLUDER_EVENT;


// threads needed to move objects from original to "hidden" lists and to share quantums
#define NUMBER_OF_THREADS	2

typedef NTSTATUS (NTAPI *PROC)();
typedef NTSTATUS (NTAPI *BALMGR_FUNC)(VOID *);
typedef 
NTSTATUS 
(NTAPI *KWFMO_FUNC)(
  IN ULONG  Count,
  IN PVOID  Object[],
  IN WAIT_TYPE  WaitType,
  IN KWAIT_REASON  WaitReason,
  IN KPROCESSOR_MODE  WaitMode,
  IN BOOLEAN  Alertable,
  IN PLARGE_INTEGER  Timeout  OPTIONAL,
  IN PKWAIT_BLOCK  WaitBlockArray  OPTIONAL
  );



#endif	// _PH2_INTERNAL_