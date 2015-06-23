/*++

Module Name:

    sw_remove.c

Abstract:

    Shadow Walker remover entry.

Author:

    90210 19-Aug-2005

--*/

#include "sw_remove.h"



static BOOLEAN NTAPI CheckForIretdCallback(PUCHAR pInstructionAddress,PVOID Param)
{
	PBRANCH	CurrentBranch;

	CurrentBranch=(PBRANCH)Param;	// it's supplied as a callback parameter

	// is current opcode an 'iretd'?
	if (0xcf==*pInstructionAddress) {
		// mark and terminate this branch
		CurrentBranch->Kind=KIND_ENDSWITHIRETD;
		return FALSE;
	}

	// continue walking code
	return TRUE;
}

static NTSTATUS AnalyzeBranches(PVOID CodeAddress,PLIST_ENTRY KnownBranches)
{
	LIST_ENTRY	Branches;
	PBRANCH	CurrentBranch;
	PKNOWN_BRANCH	KnownBranch;
	NTSTATUS	status;
	BOOLEAN	bNewKnownBranch;
	ULONG	bLeftChildKind,bRightChildKind;

	if (!CodeAddress || !KnownBranches)
		return STATUS_INVALID_PARAMETER;

	InitializeListHead(&Branches);
	InitializeListHead(KnownBranches);
	
	// allocate first branch
	CurrentBranch=ExAllocatePool(PagedPool,sizeof(BRANCH));
	RtlZeroMemory(CurrentBranch,sizeof(BRANCH));
	CurrentBranch->Start=CodeAddress;
	InsertTailList(&Branches,&CurrentBranch->le);

	// process branches that were found during disasming
	// loop until there is no more not analyzed branches
	
	DbgPrint("Walking branches...\n");
	
	CurrentBranch=(PBRANCH)Branches.Flink;	
	while (CurrentBranch!=(PBRANCH)&Branches) {

		CurrentBranch=CONTAINING_RECORD(CurrentBranch,BRANCH,le);

		status=WalkBranch(CurrentBranch->Start,					
					&Branches,
					CurrentBranch,
					CheckForIretdCallback,	// call CheckForIretdCallback(CurrentBranch) on each opcode
					CurrentBranch,
					FALSE);		// do not enter subroutines by call
		
		if (status==STATUS_UNSUCCESSFUL) {
			DbgPrint("WalkBranch(): disasming failed\n");
			return STATUS_UNSUCCESSFUL;
		}

		if (status==STATUS_UNPREDICTABLE_JUMP)
			// treat unpredictable jumps as jumps to the original interrupt handler
			CurrentBranch->Kind=KIND_PASSDOWN;
		
		CurrentBranch=(PBRANCH)CurrentBranch->le.Flink;		
	}


	// now mark all iretd-ending branches with EndsWithIretd flag:
	// that is, we mark a branch if both it's children have EndsWithIretd flag set.
	// loop until there is no more branches to mark.

	DbgPrint("Analyzing code endpoints...\n");

	do {		
		bNewKnownBranch=FALSE;

		CurrentBranch=(PBRANCH)Branches.Flink;
		while (CurrentBranch!=(PBRANCH)&Branches) {

			CurrentBranch=CONTAINING_RECORD(CurrentBranch,BRANCH,le);

			if (CurrentBranch->LeftChild)
				bLeftChildKind=CurrentBranch->LeftChild->Kind;
			else
				bLeftChildKind=KIND_UNKNOWN;
			if (CurrentBranch->RightChild)
				bRightChildKind=CurrentBranch->RightChild->Kind;
			else
				// non conditional jumps do not have right branch,
				// but we should assign them corresponding kind if their left branch'es kind is known.
				bRightChildKind=bLeftChildKind;

		
			// if both children end equally (both iretd or both passdown) then 
			// change current branch'es kind to the one of the children's.
			if ((CurrentBranch->Kind==KIND_UNKNOWN) && 
				(bLeftChildKind!=KIND_UNKNOWN) &&
				(bLeftChildKind==bRightChildKind)) {
				CurrentBranch->Kind=bLeftChildKind;
				bNewKnownBranch=TRUE;
			}

			CurrentBranch=(PBRANCH)CurrentBranch->le.Flink;
		}


	} while (bNewKnownBranch);

	// iterate through the branches once again and fill the list of the known branches
	CurrentBranch=(PBRANCH)Branches.Flink;
	while (CurrentBranch!=(PBRANCH)&Branches) {

		CurrentBranch=CONTAINING_RECORD(CurrentBranch,BRANCH,le);

		if (CurrentBranch->Kind!=KIND_UNKNOWN) {
			// insert this branch to the list: we have determined it's kind
			KnownBranch=ExAllocatePool(PagedPool,sizeof(KNOWN_BRANCH));
			RtlZeroMemory(KnownBranch,sizeof(KNOWN_BRANCH));
			KnownBranch->Address=CurrentBranch->Start;
			KnownBranch->Kind=CurrentBranch->Kind;
			InsertTailList(KnownBranches,&KnownBranch->le);
		}
	
		CurrentBranch=(PBRANCH)CurrentBranch->le.Flink;
	}

	// free found branches
	FreeDoubleLinkedList(&Branches);

	return STATUS_SUCCESS;
}

static NTSTATUS GetNtoskrnlRegion(PVOID* pNtoskrnlStart,PVOID* pNtoskrnlEnd)
{
	PMODULES	pModules=(PMODULES)&pModules;
	ULONG	rc,uNeededSize;

	if (!pNtoskrnlStart || !pNtoskrnlEnd)
		return STATUS_UNSUCCESSFUL;

	// get system modules - ntoskrnl is always first there
	rc=ZwQuerySystemInformation(SystemModuleInformation,pModules,4,&uNeededSize);
	if (rc==STATUS_INFO_LENGTH_MISMATCH) {
		pModules=ExAllocatePool(PagedPool,uNeededSize);
		RtlZeroMemory(pModules,uNeededSize);
		rc=ZwQuerySystemInformation(SystemModuleInformation,pModules,uNeededSize,NULL);
	}

	if (!NT_SUCCESS(rc)) {

#ifdef DEBUG
		DbgPrint("GetNtoskrnlRegion(): strange NtQuerySystemInformation()!\n");
#endif
		return STATUS_NTOSKRNL_NOT_FOUND;
	}

	*pNtoskrnlStart=pModules->smi.Base;
	*pNtoskrnlEnd=(PUCHAR)pModules->smi.Base+pModules->smi.Size;
	
	ExFreePool(pModules);
	return STATUS_SUCCESS;
}

static NTSTATUS IsInterruptHooked(UCHAR bInterruptNumber,UCHAR IdtNumber,PBOOLEAN pbHooked,PULONG NtoskrnlHandler)
{
	PVOID	IntHandler;
	NTSTATUS	Status;
	
	if (!pbHooked || !NtoskrnlHandler)
		return STATUS_INVALID_PARAMETER;


	IntHandler=GetInterruptHandler(bInterruptNumber,IdtNumber);

	*NtoskrnlHandler=0;
	if (!NT_SUCCESS(Status=FindIDTEntry(bInterruptNumber,NtoskrnlHandler))) {
		DbgPrint("FindIDTEntry(): status %08X\n",Status);
	}

	// if we have the real ntoskrnl handler address, simply compare it with the current handler
	if ((*NtoskrnlHandler && ((ULONG)IntHandler!=*NtoskrnlHandler)) ||
		// we don't have the real address, so check - 
		// whether int 0x0e handler belongs to the ntoskrnl region or not.
		((IntHandler<g_NtoskrnlStart) || (g_NtoskrnlEnd<IntHandler))) {
		// it does not - hence it's hooked.
		*pbHooked=TRUE;
		return STATUS_SUCCESS;
	}

	// TODO: 
	// - examine whole int handler control flow graph for code splicing or detours

	*pbHooked=FALSE;
	return STATUS_SUCCESS;
}

static VOID DumpKnownBranches()
{
	PKNOWN_BRANCH	KnownBranch;
	
	// dump all known branches and show their kinds

	DbgPrint("Dumping known branches:\n");

	KnownBranch=(PKNOWN_BRANCH)g_KnownBranches.Flink;
	while (KnownBranch!=(PKNOWN_BRANCH)&g_KnownBranches) {

		KnownBranch=CONTAINING_RECORD(KnownBranch,KNOWN_BRANCH,le);
		
		switch (KnownBranch->Kind) {
		
		case KIND_ENDSWITHIRETD:
			DbgPrint("   %08X -> iretd\n",KnownBranch->Address);
			break;
		case KIND_PASSDOWN:
			DbgPrint("   %08X -> passdown\n",KnownBranch->Address);
			break;
		default:
			DbgPrint("   %08X - unknown (%08X)!\n",KnownBranch->Address,KnownBranch->Kind);
		}
	
		KnownBranch=(PKNOWN_BRANCH)KnownBranch->le.Flink;
	}

}


static NTSTATUS NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{

	KeSetEvent(&g_ShutdownEvent,0,FALSE);

	if (g_BruteforcerThread) {
		DbgPrint("waiting for the bruteforcer shutdown...\n");
		KeWaitForSingleObject(g_BruteforcerThread,
			Executive,
			KernelMode,
			FALSE,
			NULL);
	}

	// restore SW's page fault handler
	SetInterruptHandler(0x0E,g_Int0EHandler,0x8e);

	// free hidden pages list
	FreeDoubleLinkedList(&g_HiddenPages);

	DbgPrint("Shadow Walker remover unloaded\n");
	return STATUS_SUCCESS;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject,PUNICODE_STRING RegistryPath)
{
	NTSTATUS	Status;
	BOOLEAN	bInt0EHooked;


	DriverObject->DriverUnload=DriverUnload; 

	// start to check from the first kernelmode page
	g_PageToCheck=(ULONG)*MmSystemRangeStart;
	
	// initialize multiprocessor module
	mp_Init();

	KeInitializeEvent(&g_ShutdownEvent,NotificationEvent,0);
	InitializeListHead(&g_HiddenPages);

	if (!NT_SUCCESS(Status=GetNtoskrnlRegion(&g_NtoskrnlStart,&g_NtoskrnlEnd))) {
		DbgPrint("GetNtoskrnlRegion(): status %08X\n",Status);
		return STATUS_UNSUCCESSFUL;
	}

	if (!NT_SUCCESS(Status=IsInterruptHooked(0x0e,0,&bInt0EHooked,&g_RealInt0EHandler))) {
		DbgPrint("IsInterruptHooked(): status %08X\n",Status);
		return STATUS_UNSUCCESSFUL;
	}
	

	// get int 0x0e handler from the first idt
	g_Int0EHandler=GetInterruptHandler(0x0e,0);

	if (g_RealInt0EHandler)
		DbgPrint("Real ntoskrnl int 0x0E handler: 0x%08X, current: 0x%08X\n",g_RealInt0EHandler,g_Int0EHandler);
	else
		// we were not able to find real handler
		DbgPrint("Real ntoskrnl int 0x0E handler was not found: Shadow Walker removing is not possible\n");

	if (!bInt0EHooked) {
		DbgPrint("Looks like interrupt 0x0E is not hooked\n");
		return STATUS_UNSUCCESSFUL;
	}
		
	
	Status=AnalyzeBranches(g_Int0EHandler,&g_KnownBranches);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("AnalyzeBranches(): status %08X\n",Status);
		return STATUS_UNSUCCESSFUL;
	}

	// for debug purposes
	DumpKnownBranches();

	// begin bruteforcing the int 0x0e handler
	StartTracer();

	return STATUS_SUCCESS; 
}

