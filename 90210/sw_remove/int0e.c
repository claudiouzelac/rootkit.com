/*++

Module Name:

    int0e.c

Abstract:

    Page fault handler - a revealer for the hidden pages.

Author:

    90210 19-Aug-2005

--*/

#include "int0e.h"

LIST_ENTRY	g_HiddenPages;
ULONG	g_RealInt0EHandler;

// return TRUE if this page fault is handled (page is hidden), FALSE otherwise
static BOOLEAN NTAPI HandleSWPageFaults(PVOID Address)
{
	PHIDDEN_PAGE	HiddenPage;

	HiddenPage=(PHIDDEN_PAGE)g_HiddenPages.Flink;
	while (HiddenPage!=(PHIDDEN_PAGE)&g_HiddenPages) {
		HiddenPage=CONTAINING_RECORD(HiddenPage,HIDDEN_PAGE,le);
		
		if (HiddenPage->Address==(PVOID)((ULONG)Address & 0xfffff000)) {
			// some code (possibly which was hidden) cleared the P bit in this pte;
			// it's a hidden page: mark it Present and handle the page fault
			*(PUCHAR)HiddenPage->Pte|=1;
			return TRUE;
		}

		HiddenPage=(PHIDDEN_PAGE)HiddenPage->le.Flink;
	}

	// this page is not hidden: passdown the page fault
	return FALSE;
}

static __declspec(naked) Int0EHandler()
{
	__asm {
		pushad
		push	fs

		mov		bx, 0x30
		mov		fs, bx

		mov		eax, [esp+32+4]	// ErrorCode
		test	eax, 4
		jnz		passdown_pagefault	// it's a usermode pagefault

		mov		eax, cr2

		push	eax
		call	HandleSWPageFaults
		or		al, al
		jz		passdown_pagefault

		// page fault was handled
		pop		fs
		popad
		
		add		esp, 4
		iretd

passdown_pagefault:

		// page fault was not handled: call the original handler
		pop		fs
		popad
		jmp		[g_RealInt0EHandler]
	}
}

VOID InstallNewPageFaultHandler()
{
	SetInterruptHandler(0x0E,&Int0EHandler,0x8e);
	
	DbgPrint("Shadow Walker remover is online\n");
}