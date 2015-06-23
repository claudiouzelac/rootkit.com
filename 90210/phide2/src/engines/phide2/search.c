/*++

Module Name:

    search.c

Abstract:

    Symbols searcher code.

Author:

    90210 7-Dec-2004

--*/

#include "search.h"

//#define DEBUG_SEARCHER

WANTED_SYMBOLS	RVAs={0};
static DWORD	KeTickCount_RVA=0,PsCreateSystemThread_RVA=0;

extern BOOLEAN	g_bCheckedBuild;
extern ULONG	g_MajorVersion, g_MinorVersion;

static PCHAR	NonNT_szCallsOfKeBalanceSetManager[]={
			"MmQuerySystemSize",
			"KeSetPriorityThread",
			"KeInitializeTimer",
			"KeSetTimer",
			"KeWaitForMultipleObjects"};
static PCHAR	NT_szCallsOfKeBalanceSetManager[]={
			"KeSetPriorityThread",
			"KeInitializeTimer",
			"KeSetTimer",
			"KeWaitForMultipleObjects"};

static PCHAR	NonNT_szCallsOfKeSwapProcessOrStack[]={
			"KeSetPriorityThread",
			"KeWaitForSingleObject"};
static PCHAR	NT_szCallsOfKeSwapProcessOrStack[]={
			"MmQuerySystemSize",
			"KeSetPriorityThread",
			"KeWaitForSingleObject"};

static DWORD	NonNT_dwCallsOfKeBalanceSetManager[sizeof(NonNT_szCallsOfKeBalanceSetManager)>>2];
static DWORD	NT_dwCallsOfKeBalanceSetManager[sizeof(NT_szCallsOfKeBalanceSetManager)>>2];

static DWORD	NonNT_dwCallsOfKeSwapProcessOrStack[sizeof(NonNT_szCallsOfKeSwapProcessOrStack)>>2];
static DWORD	NT_dwCallsOfKeSwapProcessOrStack[sizeof(NT_szCallsOfKeSwapProcessOrStack)>>2];

static PCHAR*	g_pszCallsOfKeBalanceSetManager=0;
static PCHAR*	g_pszCallsOfKeSwapProcessOrStack=0;

static DWORD*	g_pdwCallsOfKeBalanceSetManager=0;
static DWORD*	g_pdwCallsOfKeSwapProcessOrStack=0;

static DWORD	g_dwNumberOfCallsOfKeBalanceSetManager=0;
static DWORD	g_dwNumberOfCallsOfKeSwapProcessOrStack=0;


#define MAX_STACK_DEPTH	20
static PBYTE	Stack[MAX_STACK_DEPTH];	// array of pointers to 'pushes'
static DWORD	StackPointer=0;
static DWORD	LastUsages[3]={0};

static void FindFunctionRegions(PMODULE_INFORMATION pmi,
						 PFUNCTION_INFORMATION pfi,
						 INSTRUCTION_CALLBACK InstructionCallback,
						 DWORD	dwCallbackParam,
						 BOOL bFollowCalls)
{
	PBYTE	dwEip;
	DWORD	dwRva;
	PCODE_REGION	pRegion,pLast;
	
	PBYTE	pCoverage,pOpcodeStart;
	LIST_ENTRY	Branches;
	PBRANCH	pBranch2Process;
	NTSTATUS	status;

	dwEip=(PBYTE)((DWORD)pmi->hModule+pfi->dwRva);
	// possible data export
	if (!PointerToCode(pmi,dwEip)) return;

	// allocate code coverage bitfield	
	pCoverage=ExAllocatePool(PagedPool,pmi->dwSizeOfImage>>3);	
	RtlZeroMemory(pCoverage,pmi->dwSizeOfImage>>3);
	pOpcodeStart=ExAllocatePool(PagedPool,pmi->dwSizeOfImage>>3);
	RtlZeroMemory(pOpcodeStart,pmi->dwSizeOfImage>>3);

	InitializeListHead(&Branches);	
	pBranch2Process=ExAllocatePool(PagedPool,sizeof(BRANCH));
	RtlZeroMemory(pBranch2Process,sizeof(BRANCH));
	pBranch2Process->dwRva=pfi->dwRva;
	InsertTailList(&Branches,&pBranch2Process->le);

	// process branches that were found during disasming
	do {
		status=WalkBranch(pmi,
						pBranch2Process->dwRva,
						pOpcodeStart,
						pCoverage,
						&Branches,
						InstructionCallback,
						dwCallbackParam,
						bFollowCalls);
		pBranch2Process=(PBRANCH)pBranch2Process->le.Flink;
		// loop until there is no more not analyzed branches
	} while ((&pBranch2Process->le!=&Branches) && (status==STATUS_SUCCESS));

	// free allocated memory for this function
	FreeDoubleLinkedList(&Branches);
	
	// collect regions of code
	for (dwRva=0;dwRva<pmi->dwSizeOfImage;dwRva++) {
		if (pCoverage[dwRva>>3])
			if (pCoverage[dwRva>>3] & (1<<(dwRva % 8))) {
			
				pRegion=ExAllocatePool(PagedPool,sizeof(CODE_REGION));
				RtlZeroMemory(pRegion,sizeof(CODE_REGION));
				pRegion->dwStartRva=dwRva;				

				while (pCoverage[dwRva>>3] & (1<<(dwRva % 8))) dwRva++;
				pRegion->dwEndRva=--dwRva;

				// add it to the end
				pLast=pfi->pCodeRegions;
				while (pLast && pLast->Next) pLast=pLast->Next;
				if (!pLast)
					pfi->pCodeRegions=pRegion;
				else
					pLast->Next=pRegion;
			}
	}	

	ExFreePool(pCoverage);
	ExFreePool(pOpcodeStart);
}

static BOOL IsPush(WORD wOpcode,PBYTE StackDelta)
{
	BYTE	bOpcode=wOpcode & 0xff;
	BOOL	rc;

	rc=(((bOpcode & ~0x07)==0x50) ||			// push reg32	(50..57)
		((bOpcode & ~0x18)==0x06) ||			// push cs/ds/es/ss	(0E, 1E, 06, 16)
		((wOpcode & ~0x0800)==0xa00f) ||		// push fs/gs (0F A0, 0F A8)
		((bOpcode & ~0x02)==0x68) ||			// push imm32/imm8 (68, 6A)
		((wOpcode & ~0xc700)==0x30ff) ||		// push r/m (FF /6)
		(bOpcode==0x60) || (bOpcode==0x9c));	// pushad, pushfd
	
	*StackDelta=0;
	if (rc)
		if (bOpcode==0x60) 
			*StackDelta=8;						// 8 dwords go on stack if pushad
		else
			*StackDelta=1;
	return rc;
}

static BOOL IsPop(WORD wOpcode,PBYTE StackDelta)
{
	BYTE	bOpcode=wOpcode & 0xff;
	BOOL	rc;

	rc=(((bOpcode & ~0x07)==0x58) ||			// pop reg32	(58..5F)
		(bOpcode==0x1f) ||						// pop ds (1F)
		((bOpcode & ~0x10)==0x07) ||			// pop es/ss (07, 17)
		((wOpcode & ~0x0800)==0xa10f) ||		// pop fs/gs (0F A1, 0F A9)
		((wOpcode & ~0xc700)==0x008f) ||		// pop r/m (8F /0)
		(bOpcode==0x61) || (bOpcode==0x9d));	// popad, popfd

	*StackDelta=0;
	if (rc)
		if (bOpcode==0x61)
			*StackDelta=8;						// 8 dwords are taken from stack if popad
		else
			*StackDelta=1;
	return rc;
}


static PFUNCTION_INFORMATION AddFunction(PLIST_ENTRY Head,DWORD dwRva)
{
	PFUNCTION_INFORMATION pfi;
	
	pfi=ExAllocatePool(PagedPool,sizeof(FUNCTION_INFORMATION));
	RtlZeroMemory(pfi,sizeof(FUNCTION_INFORMATION));
	
	pfi->dwRva=dwRva;
	InsertTailList(Head,&pfi->le);

	return pfi;
}

static BOOL __stdcall FindUsedGlobalsCallback(PBYTE pImage,DWORD dwRva,DWORD dwParam,PDWORD rc)
{
	PFUNCTION_INFORMATION	pfi;
	PCODE_REGION	pcr;
	PABSOLUTE_POINTER	pap;
	PMODULE_INFORMATION pmi=(PMODULE_INFORMATION)dwParam;
	BOOL	bDuplicate;
	DWORD dwPointsToRva;	
	PLIST_ENTRY	ListHead;

	dwPointsToRva=*(PDWORD)(pImage+dwRva)-pmi->dwImageBase;

	// if it is a 'call [mem32]', skip it
	if (*(PWORD)(dwRva+(DWORD)pmi->hModule-2)==0x15ff)
		return TRUE;

	ListHead=&pmi->Subs;
	pfi=(PFUNCTION_INFORMATION)ListHead->Flink;	

	// for all functions
	while (&pfi->le!=ListHead) {
		// check their code regions
		for (pcr=pfi->pCodeRegions;pcr;pcr=pcr->Next)
			// if this relo item is used in this region
			if (pcr->dwStartRva<dwRva && dwRva<pcr->dwEndRva) {
								
				bDuplicate=FALSE;
				// don't add new pointer if it is already here
				for (pap=pfi->pPointers;pap;pap=pap->Next)
					if (pap->dwPointsToRva==dwPointsToRva)
						bDuplicate=TRUE;

						if (!bDuplicate) {
							pap=ExAllocatePool(PagedPool,sizeof(ABSOLUTE_POINTER));
							RtlZeroMemory(pap,sizeof(ABSOLUTE_POINTER));
							pap->dwPointsToRva=dwPointsToRva;
							pap->dwFirstRva=dwRva;
							if (pfi->pPointers) pap->Next=pfi->pPointers;
							pfi->pPointers=pap;
						}
						// continue check other functions
						break;
			}
		pfi=(PFUNCTION_INFORMATION)pfi->le.Flink;
	}

	// continue parsing
	return TRUE;
}

static void FindSharedGlobals(PMODULE_INFORMATION pmi,
					   INSTRUCTION_CALLBACK InstructionCallback,
					   DWORD dwCallbackParameter,
					   BOOL bFollowCalls)
{
	PFUNCTION_INFORMATION	pfi;
	PABSOLUTE_POINTER	pap,pFirstSubPap;
	BOOL	bPointerFound;
	PLIST_ENTRY	ListHead;
	
	ListHead=&pmi->Subs;
	pfi=(PFUNCTION_INFORMATION)ListHead->Flink;	

	// for all functions
	while (&pfi->le!=ListHead) {
		FindFunctionRegions(pmi,pfi,InstructionCallback,dwCallbackParameter,bFollowCalls);
		pfi=(PFUNCTION_INFORMATION)pfi->le.Flink;
	}

	ParseRelocs(pmi->hModule,FindUsedGlobalsCallback,(DWORD)pmi);

	// find static pointers that are used by every function in list	
	for (pFirstSubPap=(*(PFUNCTION_INFORMATION*)&pmi->Subs)->pPointers;pFirstSubPap;pFirstSubPap=pFirstSubPap->Next) {
		
		ListHead=&pmi->Subs;
		pfi=(PFUNCTION_INFORMATION)ListHead->Flink;	

		while (&pfi->le!=ListHead) {
			bPointerFound=FALSE;
			for (pap=pfi->pPointers;pap;pap=pap->Next)
				if (pap->dwPointsToRva==pFirstSubPap->dwPointsToRva) {
					bPointerFound=TRUE;
					break;
				}
			if (!bPointerFound) break;
			pfi=(PFUNCTION_INFORMATION)pfi->le.Flink;
		}

		ListHead=&pmi->Subs;
		pfi=(PFUNCTION_INFORMATION)ListHead->Flink;	

		while (&pfi->le!=ListHead) {
			for (pap=pfi->pPointers;pap;pap=pap->Next)
				if (pap->dwPointsToRva==pFirstSubPap->dwPointsToRva)
					pap->bShared=bPointerFound;

			pfi=(PFUNCTION_INFORMATION)pfi->le.Flink;
		}

	}
}

static void NTAPI FreeAddedFunctions(PLIST_ENTRY ListHead)
{
	PLIST_ENTRY Head=ListHead,Next;
	if (!ListHead) return;
		
	ListHead=ListHead->Flink;
	while (ListHead!=Head) {
		Next=ListHead->Flink;

		FreeLinkedList(((PFUNCTION_INFORMATION)ListHead)->pPointers);
		FreeLinkedList(((PFUNCTION_INFORMATION)ListHead)->pCodeRegions);

		RemoveEntryList(ListHead);
		ExFreePool(ListHead);
		ListHead=Next;
	}
	return;

}


static BOOL __stdcall FindORCallback(PBYTE pImage,DWORD dwRva,DWORD dwPointer,PDWORD rc)
{
	DWORD	dwPointsTo=*(PDWORD)(pImage+dwRva),len;
	PBYTE	Eip;
	WORD	wOpcode;

	// N.B. this code looks only for "or r/m[8], <some>" instructions.
	// "or reg[8], ..." checks are skipped.

	if (dwPointsTo!=dwPointer) 
		// this is another fixup
		return TRUE;

	// our ORs opcodes are always 2-bytes long
	wOpcode=*(PWORD)(pImage+dwRva-2);

	if (((wOpcode & ~0xc703)==0x0880) ||	// or r/m[8], imm[8]
		((wOpcode & ~0xff01)==8)) {			// or r/m[8], reg[8]
		// step back and disasm above opcodes to be sure that first byte of this "or" 
		// does not belong to the previous instruction. 
		// Pretty lame. Need to trace code above, but ntoskrnl is mostly a compiler 
		// generated code, so this hack works.
		Eip=dwRva+pImage-30;
		len=0;
		do {
			Eip+=len;
			len=c_Catchy(Eip);
		} while ((len!=CATCHY_ERROR) && (Eip+len<dwRva+pImage));

		// does this opcode actually start as OR?
		if ((DWORD)(Eip-pImage)==dwRva-2) {
			*rc=TRUE;
			// stop enumerating fixups
			return FALSE;
		}
	}

	return TRUE;
}

static BOOL FindWaitLists(PMODULE_INFORMATION pmi)
{
	DWORD	i,dwSharedGlobals,dwFuncRva,dwKSAT;
	PCHAR	pszNames[]={"KeWaitForSingleObject",
						"KeWaitForMultipleObjects",
						"KeDelayExecutionThread",
						NULL};

	DWORD	a=0,b=0,pWaitListHeads[2]={0,0};
	PABSOLUTE_POINTER	pap;		

	for (i=0;pszNames[i];i++) {
		if (!(dwFuncRva=GetProcRva(pmi->hModule,pszNames[i]))) {
#ifdef DEBUG_SEARCHER
			DbgPrint("FindWaitLists(): Failed to get rva of %s!\n",pszNames[i]);
#endif
			return FALSE;
		}
		AddFunction(&pmi->Subs,dwFuncRva);
	}
	
	// do not need to set instruction callback	
	// do not enter calls
	FindSharedGlobals(pmi,NULL,0,FALSE);

	for (i=0,pap=(*(PFUNCTION_INFORMATION*)&pmi->Subs)->pPointers;pap;pap=pap->Next) {
		if (pap->bShared && (pap->dwPointsToRva!=KeTickCount_RVA)) {			
			if (i<2) pWaitListHeads[i]=pap->dwPointsToRva;
			i++;
		}
	}

	FreeAddedFunctions(&pmi->Subs);

	if (i!=2) {
#ifdef DEBUG_SEARCHER
		DbgPrint("FindWaitLists(): Weird number of shared pointers!!!\n");
#endif
		return FALSE;
	}

	// sort listheads
	if (pWaitListHeads[0]>pWaitListHeads[1]) {
		i=pWaitListHeads[0];
		pWaitListHeads[0]=pWaitListHeads[1];
		pWaitListHeads[1]=i;
	}

	// xp has only KiWaitListHead, so check for addressing Blink/Flink pointers:
	// if there are two LIST_HEADs (like on 2k), addresses will not be so close
	if (pWaitListHeads[1]-pWaitListHeads[0]==4) pWaitListHeads[1]=0;

	RVAs.WaitLists[0]=pWaitListHeads[0];
	RVAs.WaitLists[1]=pWaitListHeads[1];

	// find KiDispatcherReadyListHead
	// in 2k, find shared globals of KeSetAffinityThread and NtYieldExecution.
	// in xp, find shared globals of KeDelayExecutionThread and NtYieldExecution.

	if (!(dwKSAT=GetProcRva(pmi->hModule,"KeSetAffinityThread"))) {
#ifdef DEBUG_SEARCHER
		DbgPrint("FindWaitLists(): Failed to get rva of KeSetAffinityThread!\n");
#endif
		return FALSE;
	}

	AddFunction(&pmi->Subs,dwKSAT);
	AddFunction(&pmi->Subs,RVAs.NtYieldExecution);	
	FindSharedGlobals(pmi,NULL,0,FALSE);

	for (dwSharedGlobals=0,pap=(*(PFUNCTION_INFORMATION*)&pmi->Subs)->pPointers;pap;pap=pap->Next) {
		if (pap->bShared) {
			if (dwSharedGlobals++<2)
				if (a) 
					b=pap->dwPointsToRva;
				else
					a=pap->dwPointsToRva;
		}
	}

	if (!dwSharedGlobals) {
		FreeAddedFunctions(&pmi->Subs);
		// try xp style
		// dwFuncRva == rva of KeDelayExecutionThread
		AddFunction(&pmi->Subs,dwFuncRva);
		AddFunction(&pmi->Subs,RVAs.NtYieldExecution);
		FindSharedGlobals(pmi,NULL,0,FALSE);

		for (pap=(*(PFUNCTION_INFORMATION*)&pmi->Subs)->pPointers;pap;pap=pap->Next) {
			if (pap->bShared) {				
				if (dwSharedGlobals++<2)
					if (a) 
						b=pap->dwPointsToRva;
					else
						a=pap->dwPointsToRva;
			}
		}
	}
	FreeAddedFunctions(&pmi->Subs);

	if (!dwSharedGlobals || !(a && b)) {
#ifdef DEBUG_SEARCHER
		DbgPrint("FindWaitLists(): Failed to find KiDispatcherReadyListHead!\n");
#endif
		return FALSE;
	}
	if (dwSharedGlobals!=2) {
#ifdef DEBUG_SEARCHER
		DbgPrint("FindWaitLists(): Weird kernel: %d shared globals found on KiDispatcherReadyListHead!\n",dwSharedGlobals);
#endif
		return FALSE;
	}

	// We found KiReadySummary and KiDispatcherReadyListHead in a and b,
	// but we don't know what of them is a and what is b.
	// KiDispatcherReadyListHead is a pointer, so it will be strange to meet
	// "or KiDispatcherReadyListHead, <some>" generated by the compiler.
	// But ORs are very common for KiReadySummary because of the SetMember macro.
	if (ParseRelocs(pmi->hModule,FindORCallback,a+pmi->dwImageBase)) {
		RVAs.KiReadySummary=a;
		RVAs.KiDispatcherReadyListHead=b;
	}
	if (ParseRelocs(pmi->hModule,FindORCallback,b+pmi->dwImageBase)) {
		if (RVAs.KiReadySummary) {
#ifdef DEBUG_SEARCHER
			DbgPrint("FindWaitLists(): Both globals shared by NtYieldExecution() and KeSetAffinityThread() are ORed!\n");
#endif
			return FALSE;
		}
		RVAs.KiReadySummary=b;
		RVAs.KiDispatcherReadyListHead=a;
	}

	return TRUE;
}

static void __stdcall FindMmInitSystemThreadsCallback(PBYTE pInstructionAddress,
								DWORD dwInstructionRva,
								DWORD dwParam)
{
	DWORD	dwFunctionRva,i;
	PFUNCTION_INFORMATION	pfi=(PFUNCTION_INFORMATION)dwParam;
	
	// return if not "call rel32"
	if (*pInstructionAddress!=0xe8) 
		return;

	dwFunctionRva=dwInstructionRva+5+*(PDWORD)(1+pInstructionAddress);

	// check for call to a function that belongs to one of function sets
	for (i=0;i<g_dwNumberOfCallsOfKeBalanceSetManager;i++)
		if (dwFunctionRva==g_pdwCallsOfKeBalanceSetManager[i])
			pfi->KeBalanceSetManagerCalls|=1<<i;

	for (i=0;i<g_dwNumberOfCallsOfKeSwapProcessOrStack;i++)
		if (dwFunctionRva==g_pdwCallsOfKeSwapProcessOrStack[i])
			pfi->KeSwapProcessOrStackCalls|=1<<i;

	// is set fully matched?
	if (pfi->KeBalanceSetManagerCalls==(ULONG)(1<<g_dwNumberOfCallsOfKeBalanceSetManager)-1)
		// function found
		RVAs.KeBalanceSetManager=pfi->dwRva;

	if (pfi->KeSwapProcessOrStackCalls==(ULONG)(1<<g_dwNumberOfCallsOfKeSwapProcessOrStack)-1)
		RVAs.KeSwapProcessOrStack=pfi->dwRva;
}

// this function will be called on each opcode met by code walker, seg prefixes are skipped
static void __stdcall FindThreadsCallback(PBYTE pInstructionAddress,
								DWORD dwInstructionRva,
								DWORD dwParam)
{
	// this code doesn't handle esp modifications, it just counts pushes and pops and
	// maintains list of pointers to pushes. this is enough for now.

	WORD	wOpcode=*(PWORD)pInstructionAddress;	
	BYTE	StackDelta=0;
	PMODULE_INFORMATION	pNewMi,pmi=(PMODULE_INFORMATION)dwParam;
	PFUNCTION_INFORMATION	pfi;

	if (IsPush(wOpcode,&StackDelta)) {
		if (StackPointer>(DWORD)MAX_STACK_DEPTH-StackDelta) {
			// there's not enough place in stack to hold this push,
			// stack shifting needed
			memcpy(Stack,&Stack[StackPointer-(MAX_STACK_DEPTH-StackDelta)],4*(2*MAX_STACK_DEPTH-StackPointer-StackDelta));
			StackPointer=MAX_STACK_DEPTH-StackDelta;	// stack has now place for this push
		}

		// save push and correct stack pointer
		while (StackDelta--)
				Stack[StackPointer++]=pInstructionAddress;
		return;
	}

	if (IsPop(wOpcode,&StackDelta)) {
		if (StackPointer<StackDelta)
			// stack contains less elements than 'pop' wants to remove?
			StackPointer=0;
		else
			// correct stack pointer
			StackPointer-=StackDelta;
	}	

	// watch for PsCreateSystemThread calls
	if ((wOpcode & 0xff)==0xe8) {
		if (dwInstructionRva+5+*(PDWORD)(pInstructionAddress+1)==PsCreateSystemThread_RVA) {
			// StartRoutine is 6th parameter
			if (StackPointer<6) {
#ifdef DEBUG_SEARCHER
				DbgPrint("FindThreadsCallback(): stack is too empty!\n");
#endif
			} else {
				if (*Stack[StackPointer-6]!=0x68)
#ifdef DEBUG_SEARCHER
					DbgPrint("FindThreadsCallback(): 6th parameter of PsCeateSystemThread() is pushed not by imm32!\n");
#else
				;
#endif
				else {
					pNewMi=ExAllocatePool(PagedPool,sizeof(MODULE_INFORMATION));
					memcpy(pNewMi,pmi,sizeof(MODULE_INFORMATION));
					InitializeListHead(&pNewMi->Subs);

					pfi=AddFunction(&pNewMi->Subs,*(PDWORD)(1+Stack[StackPointer-6])-pmi->dwImageBase);
					// get list of functions that are called by this thread,
					// compare function sets
					FindFunctionRegions(pmi,
									pfi,
									FindMmInitSystemThreadsCallback,
									(DWORD)pfi,
									FALSE);
#ifdef DEBUG_SEARCHER
					DbgPrint(".%08X: KeBalanceSetManagerCalls: %x, KeSwapProcessOrStackCalls: %x\n",pfi->dwRva,pfi->KeBalanceSetManagerCalls,pfi->KeSwapProcessOrStackCalls);
#endif
					
					FreeAddedFunctions(&pNewMi->Subs);
					ExFreePool(pNewMi);
				}
			}
		}
	}	
	return;
}

static BOOL FindBalmgrThreads(PMODULE_INFORMATION pmi)
{
	PIMAGE_FILE_HEADER	pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	PFUNCTION_INFORMATION	pfi;
	PLIST_ENTRY	ListHead;

	GetHeaders((PCHAR)pmi->hModule,&pfh,&poh,&psh);

	// walk code from the kernel entry point
	AddFunction(&pmi->Subs,poh->AddressOfEntryPoint);
	
	// set FindThreadsCallback as instruction callback to save info about pushes,
	// pass pmi as callback parameter,
	// enter calls	
	ListHead=&pmi->Subs;
	pfi=(PFUNCTION_INFORMATION)ListHead->Flink;	

	// for all functions
	while (&pfi->le!=ListHead) {	
		FindFunctionRegions(pmi,pfi,FindThreadsCallback,(DWORD)pmi,TRUE);
		pfi=(PFUNCTION_INFORMATION)pfi->le.Flink;
	}

	FreeAddedFunctions(&pmi->Subs);
	
	if (RVAs.KeBalanceSetManager && RVAs.KeSwapProcessOrStack)
		return TRUE;
	else
		return FALSE;
}


static DWORD Abs(signed long a)
{
	return a>0 ? a : (DWORD)-a;
}

static BOOL __stdcall FindKiStackInSwapListHeadCallback(PBYTE pImage,DWORD dwRva,DWORD dwParam,PDWORD rc)
{
	PMODULE_INFORMATION	pmi=(PMODULE_INFORMATION)dwParam;
	DWORD	i,dwPointsToRva=*(PDWORD)(pImage+dwRva)-pmi->dwImageBase;

	*rc=0;

	for (i=0;i<3;i++)
		if (dwPointsToRva==RVAs.BalmgrLists[i]) {
			LastUsages[i]=dwRva;
			break;
		}

	if (i==3) return TRUE;

	if ((Abs(LastUsages[0]-LastUsages[1])<=MAX_INITLIST_DELTA) &&
		(Abs(LastUsages[0]-LastUsages[2])<=MAX_INITLIST_DELTA) &&
		(Abs(LastUsages[1]-LastUsages[2])<=MAX_INITLIST_DELTA) &&
		LastUsages[0]) {
		
		i=0;
		if (LastUsages[0]<LastUsages[1]) i++;
		if (LastUsages[i]<LastUsages[2]) i=2;

		RVAs.KiStackInSwapListHead=RVAs.BalmgrLists[i];
		RVAs.BalmgrLists[i]=0;

		*rc=TRUE;
		return FALSE;
	}
		

	return TRUE;
}


static BOOL __stdcall CheckForPointerTo(PBYTE pImage,DWORD dwRva,DWORD dwParam,PDWORD rc)
{
	DWORD dwPointsToRva=*(PDWORD)(pImage+dwRva);
	
	if (dwPointsToRva==dwParam) {
		// pointer found, stop parsing
		*rc=TRUE;
		return FALSE;
	}
	// nothing found; continue parsing
	return TRUE;
}

static BOOL __stdcall FindKiSwapEventCallback(PBYTE pImage,DWORD dwRva,DWORD dwParam,PDWORD rc)
{
	PPMI_AND_PAP	pParam=(PPMI_AND_PAP)dwParam;	
	DWORD	dwPointsToRva=*(PDWORD)(pImage+dwRva)-pParam->pmi->dwImageBase;

	if (dwPointsToRva!=pParam->pap->dwPointsToRva) 
		// this is another fixup
		return TRUE;

	// used by KeSwapProcessOrStack?
	if (dwRva==pParam->pap->dwFirstRva) return TRUE;

	// KiSwapEvent is a SynchronizationEvent
	// is this instruction a "mov mem32, SynchronizationEvent"?
	if ((*(PWORD)(pImage+dwRva-2)==0x05c6) && (*(PBYTE)(pImage+dwRva+4)==1)) {

		// KiSwapEvent points to the KEVENT structure, which is initialized by a
		// KeInitializeEvent macro (not function). This macro sets KEVENT.Header.Size to 4,
		// that is, makes *(&KEVENT+2)=4.
		if (ParseRelocs(pImage,CheckForPointerTo,dwPointsToRva+pParam->pmi->dwImageBase+2)) {
			pParam->pap->bShared=TRUE;
			RVAs.KiSwapEvent=dwPointsToRva;
			// stop enumerating fixups
			return FALSE;
		}			
	}

	return TRUE;
}

static BOOL FindBalmgrLists(PMODULE_INFORMATION pmi)
{
	PFUNCTION_INFORMATION	pKSPOS;
	PABSOLUTE_POINTER	pap,FirstPap;
	PMI_AND_PAP	CallbackParam;
	DWORD	i;

	AddFunction(&pmi->Subs,RVAs.KeBalanceSetManager);
	pKSPOS=AddFunction(&pmi->Subs,RVAs.KeSwapProcessOrStack);

	// do not need to set instruction callback	
	// do not enter calls
	FindSharedGlobals(pmi,NULL,0,FALSE);

	for (i=0,pap=pKSPOS->pPointers;pap;pap=pap->Next,i++)
		FirstPap=pap;

	if (i==6) {
#ifdef DEBUG_SEARCHER
		DbgPrint("FindBalmgrLists(): NT or XP kernel - 6 pointers\n");
#endif
		// 2k KeSwapProcessOrStack uses 5 globals: KiSwapEvent, KiStackOutSwapRequest,
		// KiProcessOutSwapListHead, KiProcessInSwapListHead and KiStackInSwapListHead.

		// XP KeSwapProcessOrStack uses one additional global: KiSwappingThread,
		// and KeBalanceSetManager don't use KiSwapEvent (it calls KiSetSwapEvent instead).

		// NT4 KeSwapProcessOrStack uses another additional global: KiStackProtectTime.
		// It is the very first global used by KeSwapProcessOrStack.

		// FirstPap contains first global used by the KeSwapProcessOrStack.
		// it's the KiSwappingThread in xp (which is set to current thread on function startup).
		// mark it as shared to skip it later.
		FirstPap->bShared=TRUE;
		if ((g_MajorVersion==5) && (g_MinorVersion==1))
			// 1st used global of XP KSPOS is KiSwappingThread,
			// 1st used global of NT4 KSPOS is KiStackProtectTime.
			RVAs.KiSwappingThread=FirstPap->dwPointsToRva;
	}

	if (i!=6 && i!=5) {
#ifdef DEBUG_SEARCHER
		DbgPrint("FindBalmgrLists(): Weird kernel! %d balmgr shared globals found!\n",i);
#endif
		return FALSE;
	}


	// find KiSwapEvent. it is initialized in KiInitSystem by KeInitializeEvent macro.
	// check for this initialization, store KiSwapEvent RVA and mark it as shared too.
	CallbackParam.pmi=pmi;
	for (pap=pKSPOS->pPointers;pap && !RVAs.KiSwapEvent;pap=pap->Next) {
		CallbackParam.pap=pap;
		ParseRelocs(pmi->hModule,FindKiSwapEventCallback,(DWORD)&CallbackParam);
	}

	for (i=0,pap=pKSPOS->pPointers;pap;pap=pap->Next)
		if (!pap->bShared) {
			// all globals that are not marked as shared are those we're searching for:
			// KiProcessInSwapListHead, KiProcessOutSwapListHead, KiStackInSwapListHead.
			// store them.
			if (i<3) RVAs.BalmgrLists[i++]=pap->dwPointsToRva;
		} else
			// KiSwapEvent, KiSwappingThread and KiStackOutSwapRequest are marked as shared
			if ((pap->dwPointsToRva!=RVAs.KiSwapEvent) &&
				(pap->dwPointsToRva!=RVAs.KiSwappingThread))				
				RVAs.KiStackOutSwapRequest=pap->dwPointsToRva;

	FreeAddedFunctions(&pmi->Subs);

	if (i!=3) {
#ifdef DEBUG_SEARCHER
		DbgPrint("FindBalmgrLists(): Weird kernel! %d balmgr lists found!\n",i);
#endif
		return FALSE;
	}
	
	if (!ParseRelocs(pmi->hModule,FindKiStackInSwapListHeadCallback,(DWORD)pmi))
		return FALSE;

	return TRUE;
}



static DWORD GetRealServiceHandler(PBYTE pImage,DWORD dwServiceId)
{
	PDWORD	KiServiceTable;
	PIMAGE_FILE_HEADER	pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;

	GetHeaders(pImage,&pfh,&poh,&psh);

	// do not search this rva if it has been already found
	if (!KiServiceTable_RVA) {
		if (!(KiServiceTable_RVA=FindKiServiceTable(pImage))) {
#ifdef DEBUG
			DbgPrint("GetRealServiceHandler(): Failed to get KiServiceTable RVA!\n");
#endif
			return 0;
		}
	}
	KiServiceTable=(PDWORD)(KiServiceTable_RVA+pImage);
	// at this time we have KiServiceTable address pointing in our mapping.
	// get rva of the appropriate handler.
	return KiServiceTable[dwServiceId]-poh->ImageBase;
}

static BOOL FindNtYieldExecution(PBYTE pImage)
{
	DWORD	ZwYieldExecution_RVA;

	if (!(ZwYieldExecution_RVA=GetProcRva(pImage,"ZwYieldExecution")))
		return FALSE;

	RVAs.NtYieldExecution=GetRealServiceHandler(pImage,
								// good ol' way to get service id ;)
								*(PDWORD)(pImage+ZwYieldExecution_RVA+1));
	return RVAs.NtYieldExecution!=0;
}	


static void __stdcall FindKiScanReadyQueuesCallback(PBYTE pInstructionAddress,
											DWORD dwInstructionRva,
											DWORD dwParam)
{
	PMODULE_INFORMATION	pmi=(PMODULE_INFORMATION)dwParam,pNewMI;
	PABSOLUTE_POINTER	pap;
	PFUNCTION_INFORMATION	pfi;

	BYTE	bOpcode=*pInstructionAddress;
	DWORD	dwCalledRva,i,dwProbablyKiReadyQueueIndex,Found;


	if (bOpcode==0xe8) {
		dwCalledRva=dwInstructionRva+5+*(PDWORD)(1+pInstructionAddress);

		pNewMI=ExAllocatePool(PagedPool,sizeof(MODULE_INFORMATION));
		memcpy(pNewMI,pmi,sizeof(MODULE_INFORMATION));
		InitializeListHead(&pNewMI->Subs);

		pfi=AddFunction(&pNewMI->Subs,dwCalledRva);
		
		// get list of the globals used by this function
		FindSharedGlobals(pNewMI,NULL,0,FALSE);		

		for (Found=i=0,pap=pfi->pPointers;pap;pap=pap->Next,i++) {
			// KiScanReadyQueues uses 4 or 6 globals: 3 of them we have already found
			if (pap->dwPointsToRva==KeTickCount_RVA) 
				Found|=1;
			else
			if (pap->dwPointsToRva==RVAs.KiReadySummary) 
				Found|=2;
			else
			if (pap->dwPointsToRva==RVAs.KiDispatcherReadyListHead) 
				Found|=4;
			else
			// checked builds have stdcall RtlAssert call with two strings as parameters.
			// the remaining unknown global which is not pushed is probably KiReadyQueueIndex.
			if (*(pInstructionAddress-1)!=0x68) 
				dwProbablyKiReadyQueueIndex=pap->dwPointsToRva;
		}

		// all 3 known symbols found and there are 4 globals on free build, 6 on checked?
		if ((Found==7) && ((!g_bCheckedBuild && (i==4)) || (g_bCheckedBuild && (i==6))) &&
			// additional check: KiReadyQueueIndex is initialized with 1
			(*(PDWORD)(dwProbablyKiReadyQueueIndex+pmi->hModule)==1))
			RVAs.KiReadyQueueIndex=dwProbablyKiReadyQueueIndex;

		FreeAddedFunctions(&pNewMI->Subs);
		ExFreePool(pNewMI);
	}
}



static BOOL FindKiReadyQueueIndex(PMODULE_INFORMATION pmi)
{
	
	PFUNCTION_INFORMATION	pfi;
	
	pfi=AddFunction(&pmi->Subs,RVAs.KeBalanceSetManager);
	// walk execution tree of KeBalanceSetManager and analyze all functions
	// that are called by it.
	FindFunctionRegions(pmi,pfi,FindKiScanReadyQueuesCallback,(DWORD)pmi,FALSE);
	FreeAddedFunctions(&pmi->Subs);

	return RVAs.KiReadyQueueIndex!=0;
}

static BOOL __stdcall CheckForFixupAt(PBYTE pImage,DWORD dwRva,DWORD dwParam,PDWORD rc)
{
	if (dwRva==dwParam) {
		// fixup found, stop parsing
		*rc=TRUE;
		return FALSE;
	}
	// nothing found; continue parsing
	return TRUE;
}

static BOOL FindPsLists(PMODULE_INFORMATION pmi)
{
	PSECTION	ps;
	PBYTE	pImage,Eip;
	DWORD	dwOpcodes,len,dwRva;
	BOOLEAN	bDumpFound;
	BYTE	bOffset;

	// loop through all sections with code
	for (ps=pmi->pSectionsWithCode;ps;ps=ps->Next) {
		for (pImage=ps->sh.VirtualAddress+pmi->hModule;
			pImage<ps->sh.VirtualAddress+pmi->hModule+ps->sh.SizeOfRawData;
			pImage++) {
			if (*(PDWORD)pImage=='EGAP') {

				Eip=pImage-1;
				// clear helper flag
				bDumpFound=FALSE;
				dwOpcodes=len=0;
				dwRva=Eip-pmi->hModule;
				do {
					// is this is a "mov [reg32+imm8],imm32"?
					// don't handle esp case
					bOffset=*(Eip+2);
					if ((*(PWORD)Eip | 0x3f00)==0x7fc7) {
						if ((bOffset==4) && (*(PDWORD)(Eip+3)=='PMUD'))
							bDumpFound=TRUE;

#if 0
						// mov dword ptr [e??+18h], offset _PsLoadedModuleList ?
						if (bDumpFound && (bOffset==0x18) && ParseRelocs(pmi->hModule,CheckForFixupAt,dwRva+3)) 
							RVAs.PsLoadedModuleList=*(PDWORD)(Eip+3)-(DWORD)pmi->dwImageBase;
#endif
						// mov dword ptr [e??+1Ch], offset _PsActiveProcessHead ?
						if (bDumpFound && (bOffset==0x1c) && ParseRelocs(pmi->hModule,CheckForFixupAt,dwRva+3)) 
							RVAs.PsActiveProcessHead=*(PDWORD)(Eip+3)-(DWORD)pmi->dwImageBase;
					}

					Eip+=len;
					dwRva+=len;
					len=c_Catchy(Eip);
					dwOpcodes++;
				// loop until disasm error or search success. check only 100 instructions after 'PAGE'.
				} while ((len!=CATCHY_ERROR) &&
						(dwOpcodes<100) && 
						!RVAs.PsActiveProcessHead &&
						(Eip<ps->sh.VirtualAddress+pmi->hModule+ps->sh.SizeOfRawData));
			}
		}		
	}

	return (BOOL)RVAs.PsActiveProcessHead;
}


NTSTATUS FindWantedSymbols(PMODULE_INFORMATION pmi)
{
	DWORD	i;

	if (!pmi) return STATUS_UNSUCCESSFUL;

	// get rvas of some exported symbols
	if (!(PsCreateSystemThread_RVA=GetProcRva(pmi->hModule,"PsCreateSystemThread")))
		return STATUS_UNSUCCESSFUL;

	if (!(KeTickCount_RVA=GetProcRva(pmi->hModule,"KeTickCount")))
		return STATUS_UNSUCCESSFUL;

	if ((g_MajorVersion==4) && (g_MinorVersion==0)) {
		// NT calls
		g_pszCallsOfKeBalanceSetManager=NT_szCallsOfKeBalanceSetManager;
		g_pszCallsOfKeSwapProcessOrStack=NT_szCallsOfKeSwapProcessOrStack;

		g_pdwCallsOfKeBalanceSetManager=NT_dwCallsOfKeBalanceSetManager;
		g_pdwCallsOfKeSwapProcessOrStack=NT_dwCallsOfKeSwapProcessOrStack;

		g_dwNumberOfCallsOfKeBalanceSetManager=sizeof(NT_szCallsOfKeBalanceSetManager)>>2;
		g_dwNumberOfCallsOfKeSwapProcessOrStack=sizeof(NT_szCallsOfKeSwapProcessOrStack)>>2;
	} else {
		// 2k and XP calls
		g_pszCallsOfKeBalanceSetManager=NonNT_szCallsOfKeBalanceSetManager;
		g_pszCallsOfKeSwapProcessOrStack=NonNT_szCallsOfKeSwapProcessOrStack;

		g_pdwCallsOfKeBalanceSetManager=NonNT_dwCallsOfKeBalanceSetManager;
		g_pdwCallsOfKeSwapProcessOrStack=NonNT_dwCallsOfKeSwapProcessOrStack;

		g_dwNumberOfCallsOfKeBalanceSetManager=sizeof(NonNT_szCallsOfKeBalanceSetManager)>>2;
		g_dwNumberOfCallsOfKeSwapProcessOrStack=sizeof(NonNT_szCallsOfKeSwapProcessOrStack)>>2;		
	}

	for (i=0;i<g_dwNumberOfCallsOfKeBalanceSetManager;i++)
		if (!(g_pdwCallsOfKeBalanceSetManager[i]=GetProcRva(pmi->hModule,g_pszCallsOfKeBalanceSetManager[i])))
			return STATUS_UNSUCCESSFUL;

	for (i=0;i<g_dwNumberOfCallsOfKeSwapProcessOrStack;i++)
		if (!(g_pdwCallsOfKeSwapProcessOrStack[i]=GetProcRva(pmi->hModule,g_pszCallsOfKeSwapProcessOrStack[i])))
			return STATUS_UNSUCCESSFUL;

	// NtYieldExecution
#ifdef DEBUG_SEARCHER
	DbgPrint("Entering FindNtYieldExecution()\n");
#endif
	if (!FindNtYieldExecution(pmi->hModule))
		return STATUS_UNSUCCESSFUL;

	// KiWaitInListHead, KiWaitOutListHead, KeDispatcherReadyListHead, KiReadySummary
#ifdef DEBUG_SEARCHER
	DbgPrint("Entering FindWaitLists()\n");
#endif
	if (!FindWaitLists(pmi))
		return STATUS_UNSUCCESSFUL;

	// KeBalanceSetManager, KeSwapProcessOrStack
#ifdef DEBUG_SEARCHER
	DbgPrint("Entering FindBalmgrThreads()\n");
#endif
	if (!FindBalmgrThreads(pmi))
		return STATUS_UNSUCCESSFUL;

	// KiStackInSwapListHead, KiProcessInSwapListHead, KiProcessOutSwapListHead, KiSwapEvent,
	// KiStackOutSwapRequest
#ifdef DEBUG_SEARCHER
	DbgPrint("Entering FindBalmgrLists()\n");
#endif
	if (!FindBalmgrLists(pmi))
		return STATUS_UNSUCCESSFUL;

	// KiReadyQueueIndex
#ifdef DEBUG_SEARCHER
	DbgPrint("Entering FindKiReadyQueueIndex()\n");
#endif
	if (!FindKiReadyQueueIndex(pmi))
		return STATUS_UNSUCCESSFUL;

	// PsActiveProcessHead
#ifdef DEBUG_SEARCHER
	DbgPrint("Entering FindPsLists()\n");
#endif
	if (!FindPsLists(pmi))
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}
