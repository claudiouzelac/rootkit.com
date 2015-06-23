/*++

Module Name:

    search.h

Abstract:

    Searcher code definitions.

Author:

    90210 7-Dec-2004

--*/

#ifndef _SEARCH_
#define _SEARCH_

#include <ntddk.h>
#include "catchy32.h"
#include "pe.h"
#include "pullout.h"

#define	MAX_INITLIST_DELTA	0x40

typedef struct {
	DWORD	KeBalanceSetManager;
	DWORD	KeSwapProcessOrStack;
	DWORD	NtYieldExecution;

	DWORD	KiDispatcherReadyListHead;
	DWORD	WaitLists[2];
	DWORD	KiStackInSwapListHead;
	DWORD	BalmgrLists[3];

	DWORD	KiReadySummary;
	DWORD	KiReadyQueueIndex;

	DWORD	KiSwapEvent;
	DWORD	KiStackOutSwapRequest;
	DWORD	KiSwappingThread;

	DWORD	PsActiveProcessHead;
} WANTED_SYMBOLS;

typedef struct ABSOLUTE_POINTER {
	struct	ABSOLUTE_POINTER	*Next;
	ULONG	dwPointsToRva;
	ULONG	dwFirstRva;		// first rva of function tree where this pointer was met
	BOOLEAN	bShared;
} ABSOLUTE_POINTER, *PABSOLUTE_POINTER;

typedef struct CODE_REGION {
	struct	CODE_REGION	*Next;
	ULONG	dwStartRva;
	ULONG	dwEndRva;
} CODE_REGION, *PCODE_REGION;

typedef struct FUNCTION_INFORMATION {
	LIST_ENTRY	le;	
	PCODE_REGION	pCodeRegions;
	PABSOLUTE_POINTER	pPointers;
	ULONG	dwRva;
	ULONG	KeBalanceSetManagerCalls;
	ULONG	KeSwapProcessOrStackCalls;
} FUNCTION_INFORMATION, *PFUNCTION_INFORMATION;

typedef struct {
	LIST_ENTRY	le;
	DWORD	dwRva;
} BRANCH, *PBRANCH;

typedef struct {
	PMODULE_INFORMATION	pmi;
	PABSOLUTE_POINTER	pap;
} PMI_AND_PAP, *PPMI_AND_PAP;

NTSTATUS FindWantedSymbols(PMODULE_INFORMATION pmi);

#endif	// _SEARCH_