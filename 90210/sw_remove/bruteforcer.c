/*++

Module Name:

    bruteforcer.c

Abstract:

    Single step tracer for bruteforcing the int 0x0e handler.

Author:

    90210 19-Aug-2005

--*/

#include "bruteforcer.h"


PUCHAR	g_Int0EHandler;
PUCHAR	g_OldInt01Handler;

ULONG	g_PageToCheck;

PETHREAD	g_BruteforcerThread;

ULONG	g_SavedEsp;
USHORT	g_SavedFS;
USHORT	g_SavedDS;

LIST_ENTRY g_KnownBranches;

PVOID	g_NtoskrnlStart;
PVOID	g_NtoskrnlEnd;

KEVENT	g_ShutdownEvent;

ULONG	g_HiddenPagesCounter=0;


// updates TLB entries for the hidden pages
static VOID TouchHiddenPages()
{
	PHIDDEN_PAGE	HiddenPage;
	PVOID	Address;

	HiddenPage=(PHIDDEN_PAGE)g_HiddenPages.Flink;
	while (HiddenPage!=(PHIDDEN_PAGE)&g_HiddenPages) {
		HiddenPage=CONTAINING_RECORD(HiddenPage,HIDDEN_PAGE,le);

		Address=HiddenPage->Address;
		
		__asm {
			mov		eax, Address
			invlpg	[eax]		// clear TLB cache for this page
			mov		eax, [eax]	// touch the page: force our int 0x0e handler to make this page present
		}

		HiddenPage=(PHIDDEN_PAGE)HiddenPage->le.Flink;
	}
}

static BOOLEAN NTAPI IsReadFromCR2(PVOID Address)
{
	// check for 0F 20 /r (mov reg32, cr2)
	return (0x0038ffff & *(PULONG)Address)==0x0010200f;	
}


static __declspec(naked) CallInt0EHandler()
{
	__asm {

		// set TRACE_FLAG
		pushfd
		or		byte ptr [esp+1],1
		popfd

		// emulate page fault on execution
		mov		eax, g_PageToCheck
		
		// mid-page addresses are more likely to point to the [DriverStart..DriverEnd] region
		add		eax, PAGE_ADDRESS_ADDITION	
		
		pushfd
		and		byte ptr [esp+1], 0xfe	// clear the TRACE_FLAG in the stack

		push 	cs
		push	eax	// emulate page fault eip

		push	0000b	// push ErrorCode
			  ; 0                    = The fault was not caused by reserved bit violation.
			  ;  0                   = Kernel mode page fault.
			  ;   0                  = The access causing the fault was a read.
			  ;    0                 = The fault was caused by a non-present page.
		
		// tracer will care for the rest
		jmp		[g_Int0EHandler]
	}
}

static VOID BruteforcerThread(PVOID StartContext)
{
	NTSTATUS	Status;

	g_BruteforcerThread=PsGetCurrentThread();

	__asm {
		// save esp, fs, ds for iterations and safe thread termination
		mov		g_SavedEsp, esp

		mov		ax, fs
		mov		g_SavedFS, ax

		mov		ax, ds
		mov		g_SavedDS, ax

		jmp		CallInt0EHandler
	}
}

static VOID AdvanceToTheNextPage()
{
	__asm {
		// restore registers
		mov		esp, g_SavedEsp
		
		mov		ax, g_SavedFS
		mov		fs, ax

		mov		ax, g_SavedDS
		mov		ds, ax
		mov		es, ax
	}

	if (KeReadStateEvent(&g_ShutdownEvent)) {
		// we should terminate tracing
		DbgPrint("bruteforcing aborted at page 0x%08X\n",g_PageToCheck);

		SetInterruptHandler(0x01,g_OldInt01Handler,0x8e);
		PsTerminateSystemThread(STATUS_SUCCESS);
	}

	
	if ((g_PageToCheck & 0xf0000000)==g_PageToCheck)
		DbgPrint("0x%08X...\n",g_PageToCheck);

	g_PageToCheck+=0x1000;

	// stop if we checked the last page
	if (g_PageToCheck<(ULONG)*MmSystemRangeStart) {

		DbgPrint("Bruteforcing finished, %d hidden page(s) revealed\n",g_HiddenPagesCounter);
		g_BruteforcerThread=NULL;
		
		// restore original int 1 handler
		SetInterruptHandler(0x01,g_OldInt01Handler,0x8e);

		// free list of the known branches
		FreeDoubleLinkedList(&g_KnownBranches);

		// install our int 0x0e handler to handle SW's hidden page faults

		// don't install if we don't know the real ntoskrnl's int 0x0e handler
		// or we didn't find any hidden pages
		if (g_RealInt0EHandler && g_HiddenPagesCounter) {
			InstallNewPageFaultHandler();

			// invalidate and update TLB entries for all hidden pages
			TouchHiddenPages();
			
		} else
			DbgPrint("Current int 0x0E handler was NOT removed\n");
		
		PsTerminateSystemThread(STATUS_SUCCESS);
	}

	// call SW handler with the next page
	__asm
		jmp		CallInt0EHandler
}


static ULONG NTAPI GetBranchKind(PVOID Address)
{
	PKNOWN_BRANCH	KnownBranch;

	// iterate through the branches and try to find current eip as the start of one of them
	KnownBranch=(PKNOWN_BRANCH)g_KnownBranches.Flink;
	while (KnownBranch!=(PKNOWN_BRANCH)&g_KnownBranches) {

		KnownBranch=CONTAINING_RECORD(KnownBranch,KNOWN_BRANCH,le);
		
		if (KnownBranch->Address==Address)
			// return the kind
			return KnownBranch->Kind;
	
		KnownBranch=(PKNOWN_BRANCH)KnownBranch->le.Flink;
	}

	return KIND_UNKNOWN;
}

// called by the Int01Handler on every hidden page 
static VOID NTAPI OnHiddenPage(PVOID HiddenPageAddress)
{
	PHIDDEN_PAGE	HiddenPage;

	DbgPrint("Hidden page found: 0x%08X\n",HiddenPageAddress);
	g_HiddenPagesCounter++;

	HiddenPage=(PHIDDEN_PAGE)ExAllocatePool(NonPagedPool,sizeof(HIDDEN_PAGE));
	HiddenPage->Address=HiddenPageAddress;
	HiddenPage->Pte=GetPte(HiddenPageAddress);
	
	InsertTailList(&g_HiddenPages,&HiddenPage->le);
}



static __declspec(naked) Int01Handler()
{
	__asm {
			
		pushad
		push	fs

		mov		bx, 0x30
		mov		fs, bx

		call	KeGetCurrentThread
		cmp		eax, g_BruteforcerThread
		jnz		continue_exception	// it's not our thread

		mov		esi, [esp+32+4]	// eip

		// if we're within ntoskrnl region, advance to the next page -
		// we missed the passdown branch
		cmp		esi, g_NtoskrnlStart
		jb		not_within_ntoskrnl
		cmp		esi, g_NtoskrnlEnd
		jb		passdown_branch_found	// this is ntoskrnl code

not_within_ntoskrnl:
		
		// check for mov reg32, cr2
		push	esi
		call	IsReadFromCR2
		or		al, al
		jz		not_read_from_cr2

		// emulate this cr2 read : write to the reg32 the next page address instead of cr2 value
		// and skip this "mov reg32, cr2" instruction

		// read the third instruction byte: it's a 0xx010yyyb, where yyy is a reg32 number.
		// we will patch corresponding register on the stack in the pushad struct - 
		// they are in the backward order there
		movzx	eax, byte ptr [esi+2]
		and		al, 7
		not		eax

		mov		ecx, g_PageToCheck
		add		ecx, PAGE_ADDRESS_ADDITION
		
		// patch the saved register
		mov		[esp+4+32+eax*4], ecx

		// adjust eip: skip "mov reg32, cr2" instruction
		add		dword ptr [esp+32+4], 3		

		jmp		return_without_passdown

not_read_from_cr2:

		// check for iretd
		cmp		byte ptr [esi], 0xcf
		jz		iretd_branch_found
		
		// find current eip in the list of the iretd branches
		push	esi
		call	GetBranchKind
		or		eax, eax	// KIND_UNKNOWN==0
		jz		return_without_passdown	// it's normal branch, trace it further

		// it's a known branch: either iretd- or passdown-type
		cmp		eax, KIND_ENDSWITHIRETD
		jnz		passdown_branch_found

iretd_branch_found:

		// we found a hidden page!
		push	g_PageToCheck
		call	OnHiddenPage

passdown_branch_found:

		mov		dword ptr [esp+32+4], offset AdvanceToTheNextPage	// patch return eip
		and		byte ptr [esp+32+4+8+1], 0xfe	// clear the TRACE_FLAG


return_without_passdown:

		pop		fs
		popad
		iretd


continue_exception:

		pop		fs
		popad
		jmp		[g_OldInt01Handler]		
	}
}


NTSTATUS NTAPI StartTracer()
{
	NTSTATUS	Status;
	HANDLE	hThread;

	g_OldInt01Handler=SetInterruptHandler(0x01,&Int01Handler,0x8e);

	if (!NT_SUCCESS(Status=PsCreateSystemThread(&hThread,
						(ACCESS_MASK)0L,
						NULL,
						0,
						NULL,
						BruteforcerThread,
						NULL))) {
#ifdef DEBUG
		DbgPrint("StartTracer(): PsCreateSystemThread() failed with status %08X\n",status);
#endif
		return Status;
	}

	ZwClose(hThread);

	DbgPrint("Bruteforcer thread started\n");

	return STATUS_SUCCESS;
}
