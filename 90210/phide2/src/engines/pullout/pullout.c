/*++

Module Name:

    pullout.c

Abstract:

    Engine to cover, pull out and rebuild the code.

Author:

    90210 05-Oct-2004

--*/

#include "pullout.h"

DWORD	KiServiceTable_RVA=0;

// compile with general debug prints
//#define DEBUG

// compile with disassembler debug prints
//#define DEBUG_DISASM

// compile with linker debug prints
//#define DEBUG_FIXRELS


// There are two methods of creating new virtual image of ntoskrnl:
// by mapping view of ntoskrnl file section and by copying sections 
// manually to the allocated pool. 
// This defines the way how to create the image. Creating a mapping of a section
// seems to be more reliable, but it causes ntice to reload and apply ntoskrnl exports
// to the newly created view, thus making the driver debugging really hard.

//#define MAP_NTOSKRNL_SECTION

// Note that memory with reconstructed code is always allocated by 
// MmAllocateNonCachedMemory.

DWORD NTAPI GetHeaders(PCHAR ibase,
		PIMAGE_FILE_HEADER *pfh,
		PIMAGE_OPTIONAL_HEADER *poh,
		PIMAGE_SECTION_HEADER *psh)

{
	PIMAGE_DOS_HEADER mzhead=(PIMAGE_DOS_HEADER)ibase;
	
	if	((mzhead->e_magic!=IMAGE_DOS_SIGNATURE) ||		
		(ibaseDD[mzhead->e_lfanew]!=IMAGE_NT_SIGNATURE))
		return FALSE;
	
	*pfh=(PIMAGE_FILE_HEADER)&ibase[mzhead->e_lfanew];
	if (((PIMAGE_NT_HEADERS)*pfh)->Signature!=IMAGE_NT_SIGNATURE) 
		return FALSE;
	*pfh=(PIMAGE_FILE_HEADER)((PBYTE)*pfh+sizeof(IMAGE_NT_SIGNATURE));
	
	*poh=(PIMAGE_OPTIONAL_HEADER)((PBYTE)*pfh+sizeof(IMAGE_FILE_HEADER));
	if ((*poh)->Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		return FALSE;
	
	*psh=(PIMAGE_SECTION_HEADER)((PBYTE)*poh+sizeof(IMAGE_OPTIONAL_HEADER));
	return TRUE;
}

// calculates eip after 5- or 6-bytes jump
static PBYTE CalcDest32(PBYTE dwEip,DWORD dwJmpSize)
{
	return (PBYTE)(*(PDWORD)(dwEip+dwJmpSize)+(DWORD)dwEip+dwJmpSize+4);
}

// calculates eip after 2-bytes jump
static PBYTE CalcDest8(PBYTE dwEip)
{
	return (PBYTE)(*(signed char*)(dwEip+1)+(DWORD)dwEip+2);
}

// checks if current opcode transfers execution to undeterminable place
// bad jumps: 0ff20h-0ff2fh, 0ff60h-0ff6fh, 0ffe0h-0ffefh
static BOOL ShouldBreak(PBYTE dwEip)
{
	// iretd?
	if (*dwEip==0xcf) return TRUE;

	if (*dwEip++!=0xff) return FALSE;

	return (((0x20<=*dwEip) && (*dwEip<0x30)) ||
			((0x60<=*dwEip) && (*dwEip<0x70)) ||
			((0xe0<=*dwEip) && (*dwEip<0xf0)));
}

// checks if current opcode is ret. if it is, returns paramcount (stack correction/4)
static BOOL ShouldReturn(PBYTE dwEip,PDWORD dwParamCount)
{
	if (*dwEip==0xc2) {
		*dwParamCount=((*(PWORD)(dwEip+1))>>2);

		return TRUE;
	}	
	if (*dwEip==0xc3) {
		*dwParamCount=0;
		return TRUE;
	}
	return FALSE;
}

static PBYTE GetJumpDest(PBYTE dwEip,PBOOL bFork,BOOL bFollowCalls)
{
	// call (0xe8)
	if (bFollowCalls && (0xe8==*dwEip)) {
		*bFork=TRUE;
		return CalcDest32(dwEip,1);
	}

	// unconditional jumps (0xe9, 0xeb)
	if (0xe9==*dwEip) {		
		*bFork=FALSE;
		return CalcDest32(dwEip,1);
	}

	if (0xeb==*dwEip) {		
		*bFork=FALSE;
		return CalcDest8(dwEip);
	}

	// conditinal jumps (0x70-0x7f) and ecx-oriented jumps (0xe0-0xe3)
	if (((0x70<=*dwEip) && (*dwEip<=0x7f)) ||
		((0xe0<=*dwEip) && (*dwEip<=0xe3))) {
		*bFork=TRUE;
		return CalcDest8(dwEip);
	}

	// extended contitionals (0x0f80-0x0f8f)
	if (*dwEip++!=0x0f)
		return 0;
	else 
		if ((0x80<=*dwEip) && (*dwEip<=0x8f)) {
			*bFork=TRUE;
			return CalcDest32(dwEip-1,2);
		}

	// it is not a jmp
	return 0;
}


void NTAPI FreeDoubleLinkedList(PLIST_ENTRY ListHead)
{
	PLIST_ENTRY Head=ListHead,Next;
	if (!ListHead) return;
		
	ListHead=ListHead->Flink;
	while (ListHead!=Head) {
		Next=ListHead->Flink;
		RemoveEntryList(ListHead);
		ExFreePool(ListHead);
		ListHead=Next;
	}
	return;

}

void NTAPI FreeLinkedList(PVOID ListHead)
{	
	PVOID pFree;
	while (ListHead) {
		pFree=ListHead;
		ListHead=*(PVOID*)ListHead;
		ExFreePool(pFree);
	}
}

BOOL NTAPI PointerToCode(PMODULE_INFORMATION pmi,PBYTE pAddr)
{
	PSECTION	ps;
	DWORD	dwRva=(DWORD)pAddr-(DWORD)pmi->hModule;

	// find section with code that contains given pointer
	for (ps=pmi->pSectionsWithCode;ps;ps=ps->Next)
		if ((ps->sh.VirtualAddress<=dwRva) && (dwRva<ps->sh.VirtualAddress+ps->sh.SizeOfRawData))
			return TRUE;

	return FALSE;
}


NTSTATUS NTAPI WalkBranch(PMODULE_INFORMATION pmi,
				DWORD dwEipRva,
				PBYTE pOpcodeStart,
				PBYTE pCoverage,
				PLIST_ENTRY Branches,
				INSTRUCTION_CALLBACK InstructionCallback,
				DWORD dwCallbackParam,
				BOOL bFollowCalls)
{
	DWORD	len,dwParamCount;
	PBYTE	pDest,pCode,dwEip=0;
	BOOL	bConditional,bWasPrefix;	
	PBRANCH	pb;

	__try {
		dwEip=dwEipRva+pmi->hModule;
		// until eip is out of bounds		
		for (;PointerToCode(pmi,dwEip);) {

			dwEipRva=(DWORD)dwEip-(DWORD)pmi->hModule;
			// already been here?
			if (pOpcodeStart[dwEipRva>>3] & (1<<(dwEipRva % 8)))
				// opcode starts here - this means branch has been already analyzed
				return STATUS_SUCCESS;
			
			if (pCoverage[dwEipRva>>3] & (1<<(dwEipRva % 8))) {
				// there's no opcode that starts here, but this byte has 
				// already been covered - we have a disasm error: a branch that
				// starts from a middle of instruction. This is bad.
#ifdef DEBUG_DISASM
				DbgPrint("Jump to the middle of instruction: dest==%08X\n",TOPREFERRED(dwEipRva));
#endif
				return -1;				
			}

			// mark current instruction
			pOpcodeStart[dwEipRva>>3] |= 1<<(dwEipRva % 8);

			pCode=dwEip;			
			// skip seg prefixes
			bWasPrefix=FALSE;
			while ((*pCode==0x26) || (*pCode==0x2e) || (*pCode==0x36) || (*pCode==0x3e) /*||
				(*pCode==0x65) || (*pCode==0x66)*/) {
				// mark instruction prefixes
				pCoverage[dwEipRva>>3] |= 1<<(dwEipRva % 8);
				dwEipRva++;
				pCode++;
				bWasPrefix=TRUE;
			}

			if (!bWasPrefix) {
				len=c_Catchy((PVOID)dwEip);			
				if (!len || (len==CATCHY_ERROR)) {
					// bad opcode
#ifdef DEBUG_DISASM
					DbgPrint("bad opcode at %08X\n",TOPREFERRED(dwEip));
#endif
					return -1;
				}
				// mark instruction bytes
				for (;len;len--,pCode++,dwEipRva++) 
					pCoverage[dwEipRva>>3] |= 1<<(dwEipRva % 8);
			} else
				dwEip=pCode;

			if (InstructionCallback) InstructionCallback(dwEip,(DWORD)dwEip-(DWORD)pmi->hModule,dwCallbackParam);

			// ret?
			if (ShouldReturn(dwEip,&dwParamCount)) {
#ifdef DEBUG_DISASM
				DbgPrint("ret %i*4 at %08X\n",dwParamCount,TOPREFERRED(dwEip));
#endif
				return STATUS_SUCCESS;
			}

			// this branch is over - can't predict execution flow
			if (ShouldBreak(dwEip)) {
#ifdef DEBUG_DISASM
				DbgPrint("Unpredictable jump at %08X!\n",TOPREFERRED(dwEip));
#endif
				return STATUS_SUCCESS;
			}
		
			pDest=GetJumpDest(dwEip,&bConditional,bFollowCalls);

			if (pDest) {
				// it is a jump to known location
				if (!PointerToCode(pmi,pDest)) {
					// broken jump, stop walking no matter it is conditional or not
#ifdef DEBUG_DISASM
					DbgPrint("broken jmp/call at eip==%08X (->%08X)\n",TOPREFERRED(dwEip),TOPREFERRED(pDest));
#endif
					return -1;
				}

				if (!bConditional) {
					// nonconditional jump: set new eip						
					dwEip=pDest;
					continue;
				} else {
					// its a conditional. add new branch start to the branch list

					// this is not recursive because i hate 'kernel stack is less 
					// than 0x500' bugcheck.
					
					// don't add if jump to the next instruction
					if (pDest!=dwEip+c_Catchy(dwEip)) {
						pb=ExAllocatePool(PagedPool,sizeof(BRANCH));
						RtlZeroMemory(pb,sizeof(BRANCH));
						pb->dwRva=pDest-pmi->hModule;
						InsertTailList(Branches,&pb->le);
					}

				}
			}
			// advance to the next instruction
			dwEip=pCode;
		}
		
		// run out of the section with code
		return -1;

	} __except (EXCEPTION_EXECUTE_HANDLER) {
#ifdef DEBUG
		DbgPrint("WalkBranch(): Bug at dwEip=%08X\n",TOPREFERRED(dwEip));
#endif
		return -1;
	}

	// should never get here
	return -1;
}


static NTSTATUS FindCodeRegions(PMODULE_INFORMATION pmi)
{
	PBYTE	dwEip;
	DWORD	dwRva,dwTotalCodeSize,dwRegionSize;//,esp_before;
	PCODE_REGION	pRegion;
	PFUNCTION_INFORMATION	pfi;
	PLIST_ENTRY	ListHead;
	LIST_ENTRY	Branches;
	PBRANCH	pBranch2Process;
	PBYTE	pCoverage,pOpcodeStart;
	NTSTATUS	status;
	
	// allocate code coverage bitfield	
	pCoverage=ExAllocatePool(PagedPool,pmi->dwSizeOfImage>>3);
	RtlZeroMemory(pCoverage,pmi->dwSizeOfImage>>3);
	// allocate opcode starts bitfield (just to check correctness of the disassembly)
	pOpcodeStart=ExAllocatePool(PagedPool,pmi->dwSizeOfImage>>3);
	RtlZeroMemory(pOpcodeStart,pmi->dwSizeOfImage>>3);


	ListHead=&pmi->Subs;
	pfi=(PFUNCTION_INFORMATION)ListHead->Flink;	

	while (&pfi->le!=ListHead) {
		if (pfi->dwRva) {
#ifdef DEBUG
			DbgPrint("FindCodeRegions(): starting at .%08X\n",pfi->dwRva);
#endif
			dwEip=(PBYTE)((DWORD)pmi->hModule+pfi->dwRva);
			// possible data export
			if (!PointerToCode(pmi,dwEip)) continue;
	
			InitializeListHead(&Branches);
			pBranch2Process=ExAllocatePool(PagedPool,sizeof(BRANCH));
			RtlZeroMemory(pBranch2Process,sizeof(BRANCH));
			pBranch2Process->dwRva=pfi->dwRva;
			InsertTailList(&Branches,&pBranch2Process->le);

			// process branches that were found during disasming
			do {
				status=WalkBranch(pmi,pBranch2Process->dwRva,pOpcodeStart,pCoverage,&Branches,NULL,0,TRUE);
				pBranch2Process=(PBRANCH)pBranch2Process->le.Flink;
				// loop until there is no more not analyzed branches
			} while ((&pBranch2Process->le!=&Branches) && (status==STATUS_SUCCESS));

			// free allocated memory for this function and advance to the next function
			FreeDoubleLinkedList(&Branches);

			// WalkBranch returned bad status.
			if (status!=STATUS_SUCCESS)
				return status;
		}
		pfi=(PFUNCTION_INFORMATION)pfi->le.Flink;
	}

	// collect regions of code
	for (dwRva=dwTotalCodeSize=0;dwRva<pmi->dwSizeOfImage;dwRva++) {
		if (pCoverage[dwRva>>3])
			if (pCoverage[dwRva>>3] & (1<<(dwRva % 8))) {
				
				pRegion=ExAllocatePool(PagedPool,sizeof(CODE_REGION));
				RtlZeroMemory(pRegion,sizeof(CODE_REGION));
				pRegion->dwOldRva=dwRva;

				dwRegionSize=0;
				while (pCoverage[dwRva>>3] & (1<<(dwRva % 8))) dwRva++, dwRegionSize++, dwTotalCodeSize++;
				pRegion->dwSize=dwRegionSize;

				// link it to the end
				InsertTailList(&pmi->CodeRegions,&pRegion->le);
#ifdef DEBUG_DISASM
				DbgPrint("code region: %08X .. %08X\n",
							pRegion->dwOldRva+pmi->dwImageBase,
							pRegion->dwOldRva+dwRegionSize+pmi->dwImageBase-1);
#endif
			}
	}
	
#ifdef DEBUG
	DbgPrint("\nFindCodeRegions(): Size of covered code: %d bytes\n",dwTotalCodeSize);
#endif

	pmi->dwSizeOfRegions=dwTotalCodeSize;

	ExFreePool(pCoverage);
	ExFreePool(pOpcodeStart);

	return STATUS_SUCCESS;
}


static void GetSectionsWithCode(PMODULE_INFORMATION pmi)
{
	PSECTION	ps;
	PIMAGE_FILE_HEADER	pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	DWORD	i;

	if (!pmi || !pmi->hModule) return;

	GetHeaders(pmi->hModule,&pfh,&poh,&psh);

	pmi->dwSizeOfImage=poh->SizeOfImage;
	pmi->dwImageBase=poh->ImageBase;

	// find all sections that may contain code
	for (i=0;i<pfh->NumberOfSections;i++,psh++)
		if ((psh->Characteristics & (/*IMAGE_SCN_CNT_CODE | */
									IMAGE_SCN_MEM_READ | 
									IMAGE_SCN_MEM_EXECUTE))==(/*IMAGE_SCN_CNT_CODE |*/
															IMAGE_SCN_MEM_READ |
															IMAGE_SCN_MEM_EXECUTE)) {
			ps=ExAllocatePool(PagedPool,sizeof(SECTION));
			RtlZeroMemory(ps,sizeof(SECTION));
			memcpy(&ps->sh,psh,sizeof(IMAGE_SECTION_HEADER));
			ps->Next=pmi->pSectionsWithCode;
			pmi->pSectionsWithCode=ps;
		}
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

DWORD NTAPI GetProcRva(PBYTE hModule,PCHAR szProcName)
{
	PIMAGE_FILE_HEADER	pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	PIMAGE_EXPORT_DIRECTORY	ped;
	BOOL	bDumbLinker,bFindByOrdinal;
	DWORD	i,dwOrdinals=0;
	PDWORD	pName;
	WORD	nOrdinal;
	DWORD	dwFuncRva=0;
	DWORD	dwReturnRva=0;

	try {

		bFindByOrdinal=((DWORD)szProcName==(WORD)szProcName);

		GetHeaders(hModule,&pfh,&poh,&psh);

		if (poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) {
			ped=(PIMAGE_EXPORT_DIRECTORY)RVATOVA(hModule,poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

			pName=(PDWORD)RVATOVA(hModule,ped->AddressOfNames);
			for (i=0;i<ped->NumberOfFunctions;i++,pName++) {
				
				bDumbLinker=FALSE;

				// hack to avoid access violation when addressing bad pointers:
				// some dumb linkers set NumberOfFunctions to the largest ordinal
				// and NumberOfNames to the real functions number.
				if (i*2+(DWORD)RVATOVA(hModule,ped->AddressOfNameOrdinals)==				
					(DWORD)RVATOVA(hModule,ped->Name)) {
					bDumbLinker=TRUE;
					break;
				}
				try {
					nOrdinal=((PWORD)RVATOVA(hModule,ped->AddressOfNameOrdinals))[i];
					dwFuncRva=((PDWORD)(RVATOVA(hModule,ped->AddressOfFunctions)))[nOrdinal];
				} except(EXCEPTION_EXECUTE_HANDLER) {
					bDumbLinker=TRUE;					
				}

				// don't want to mess up my stack
				if (bDumbLinker) break;
				
				nOrdinal+=(WORD)ped->Base;

				if (!bFindByOrdinal && pName && i<ped->NumberOfNames) {					
					if (!strcmp(szProcName,(PCHAR)RVATOVA(hModule,*pName))) {
						dwReturnRva=dwFuncRva;
						break;
					}
				} else
					if (bFindByOrdinal && (nOrdinal==(WORD)szProcName)) {
						dwReturnRva=dwFuncRva;
						break;
					}				
			}			
		} 
#ifdef DEBUG		
			else
				DbgPrint("GetProcRva(): No export directory\n");
#endif
	} except(EXCEPTION_EXECUTE_HANDLER) {
#ifdef DEBUG
		DbgPrint("GetProcRva(): Exception while enumerating exports: failed on %i of %i\n",i,ped->NumberOfFunctions);
#endif
	}

	return dwReturnRva;

}


DWORD ParseRelocs(PBYTE pImage,RELOCS_CALLBACK RelocsCallback,DWORD dwParam)
{
	PIMAGE_FILE_HEADER	pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	PIMAGE_BASE_RELOCATION	pbr;
	PIMAGE_FIXUP_ENTRY	pfe;

	DWORD	dwFixups=0,i,dwPointerRva,rc=0;
	BOOL	bFirstChunk,bContinueParsing;

	GetHeaders(pImage,&pfh,&poh,&psh);
	
	if ((poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) &&
		(!((pfh->Characteristics)&IMAGE_FILE_RELOCS_STRIPPED))) {

		pbr=(PIMAGE_BASE_RELOCATION)RVATOVA(poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,pImage);

		bContinueParsing=bFirstChunk=TRUE;

		// 1st IMAGE_BASE_RELOCATION.VirtualAddress of ntoskrnl is 0
		while ((bFirstChunk || pbr->VirtualAddress) && bContinueParsing) {
			bFirstChunk=FALSE;

			pfe=(PIMAGE_FIXUP_ENTRY)((DWORD)pbr+sizeof(IMAGE_BASE_RELOCATION));
			for (i=0;i<(pbr->SizeOfBlock-sizeof(IMAGE_BASE_RELOCATION))>>1;i++,pfe++) {
				if (pfe->type==IMAGE_REL_BASED_HIGHLOW) {
					dwFixups++;
					dwPointerRva=pbr->VirtualAddress+pfe->offset;

					rc=0;
					// stop walking relocs if callback returned false
					if (!(bContinueParsing=RelocsCallback(pImage,dwPointerRva,dwParam,&rc)))
						break;
				}
#ifdef DEBUG
					else
						if (pfe->type!=IMAGE_REL_BASED_ABSOLUTE)
							DbgPrint("ParseRelocs(): relo type %d found at .%X\n",pfe->type,pbr->VirtualAddress+pfe->offset);
#endif
			}
			*(PDWORD)&pbr+=pbr->SizeOfBlock;
		}
	}	
#ifdef DEBUG
	if (!dwFixups) DbgPrint("ParseRelocs(): No fixups!\n");
#endif
	return rc;
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

static BOOL __stdcall FindKSDTReference(PBYTE pImage,DWORD dwRva,DWORD dwKSDT_address,PDWORD rc)
{
	DWORD dwPointsToRva;
	dwPointsToRva=*(PDWORD)(pImage+dwRva);
	
	// dwKSDT_address == dwKSDT_RVA+preferred ibase
	if (dwPointsToRva==dwKSDT_address) {
		// check for mov [mem32],imm32. we are trying to find 
		// "mov ds:_KeServiceDescriptorTable.Base, offset _KiServiceTable"
		// from the KiInitSystem.
		if (*(PWORD)(pImage+dwRva-2)==0x05c7) {
			// check for a reloc presence at "offset _KiServiceTable"
			if (ParseRelocs(pImage,CheckForFixupAt,dwRva+4)) {
				// if reloc found, ParseRelocs return TRUE (as set by CheckForFixupAt)
				// set rc to the KiServiceTable address (based to the preferred address)
				*rc=*(PDWORD)(pImage+dwRva+4);				
				// we must return FALSE to stop enumerating relocs.
				return FALSE;
			}
		}
	}
	// nothing found; continue parsing
	return TRUE;
}

DWORD NTAPI FindKiServiceTable(PBYTE pImage)
{
	PIMAGE_FILE_HEADER	pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	DWORD tmp,KiServiceTable=0,dwKSDT_RVA;

	dwKSDT_RVA=GetProcRva(pImage,"KeServiceDescriptorTable");
	if (!dwKSDT_RVA) {
#ifdef DEBUG
		DbgPrint("FindKiServiceTable(): Failed to get KeServiceDescriptorTable RVA!\n");
#endif
		return 0;
	}

	GetHeaders(pImage,&pfh,&poh,&psh);
	tmp=ParseRelocs(pImage,FindKSDTReference,dwKSDT_RVA+poh->ImageBase);

	// based to the preferred address
	if (tmp) KiServiceTable=tmp-poh->ImageBase;
	
	return KiServiceTable;
}



static void FixRelocs(PMODULE_INFORMATION pmi,DWORD dwOriginalBase,FIXRELOCS_CALLBACK FixrelsCallback)
{
	PIMAGE_FILE_HEADER	pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	PIMAGE_BASE_RELOCATION	pbr;
	PIMAGE_FIXUP_ENTRY	pfe;	
	PCODE_REGION	pCurrentRegion;

	DWORD	dwFixups=0,i,dwPointerRva,dwRegionRva,dwFixupRva;
	BOOL	bFirstChunk,bAllRegionsProcessed;

	GetHeaders(pmi->hModule,&pfh,&poh,&psh);
	
	pCurrentRegion=(PCODE_REGION)pmi->CodeRegions.Flink;

	if ((poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) &&
		(!((pfh->Characteristics)&IMAGE_FILE_RELOCS_STRIPPED))) {
		
		pbr=(PIMAGE_BASE_RELOCATION)RVATOVA(poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,pmi->hModule);

		bFirstChunk=TRUE;
		// 1st IMAGE_BASE_RELOCATION.VirtualAddress of ntoskrnl is 0
		while (bFirstChunk || pbr->VirtualAddress) {
			bFirstChunk=FALSE;

			pfe=(PIMAGE_FIXUP_ENTRY)((DWORD)pbr+sizeof(IMAGE_BASE_RELOCATION));

			for (i=0;i<(pbr->SizeOfBlock-sizeof(IMAGE_BASE_RELOCATION))>>1;i++,pfe++) {

				if (pfe->type==IMAGE_REL_BASED_HIGHLOW) {
					dwFixups++;
					dwPointerRva=pbr->VirtualAddress+pfe->offset;

					bAllRegionsProcessed=FALSE;
					// advance to the next region with relocs if needed
					while (dwPointerRva>pCurrentRegion->dwSize+pCurrentRegion->dwOldRva-4) {
						if (pCurrentRegion==(PCODE_REGION)&pmi->CodeRegions) {
							bAllRegionsProcessed=TRUE;
							break;
						}
						pCurrentRegion=(PCODE_REGION)pCurrentRegion->le.Flink;
					}

					// all regions enumerated?
					if ((pCurrentRegion==(PCODE_REGION)&pmi->CodeRegions) || bAllRegionsProcessed)
						break;

					// is reloc pointing into this region?
					if ((pCurrentRegion->dwOldRva<=dwPointerRva) &&
						(dwPointerRva<=pCurrentRegion->dwSize+pCurrentRegion->dwOldRva-4)) {

						// calculate region-based rva
						dwRegionRva=dwPointerRva-pCurrentRegion->dwOldRva;
						dwFixupRva=*(PDWORD)(pmi->pNewCode+pCurrentRegion->dwNewRva+dwRegionRva)-pmi->dwImageBase;
						// fix reloc
						*(PDWORD)(pmi->pNewCode+pCurrentRegion->dwNewRva+dwRegionRva)=dwFixupRva+dwOriginalBase;

						if (FixrelsCallback)
							FixrelsCallback((PCHAR)dwOriginalBase,
								(PDWORD)(pmi->pNewCode+pCurrentRegion->dwNewRva+dwRegionRva),
								dwPointerRva,
								dwFixupRva);
					}			

				} 
#ifdef DEBUG				
					else
						if (pfe->type!=IMAGE_REL_BASED_ABSOLUTE)
							DbgPrint("FixRelocs(): relo type %d found at .%X\n",pfe->type,pbr->VirtualAddress+pfe->offset);
#endif
			}
			*(PDWORD)&pbr+=pbr->SizeOfBlock;
		}
	}
	
#ifdef DEBUG
	if (!dwFixups) DbgPrint("FixRelocs(): No fixups!\n");
#endif
}

static void FixJumpsAndCalls(PMODULE_INFORMATION pmi)
{
	PBYTE	pCode,pDest;
	PCODE_REGION	pcr,pLabelRegion;
	DWORD	len,dwRvaInOriginalModule,dwNewLabelRva;
	BOOL	bFork;	

	// for all regions
	for (pcr=(PCODE_REGION)pmi->CodeRegions.Flink;
		pcr!=(PCODE_REGION)&pmi->CodeRegions;
		pcr=(PCODE_REGION)pcr->le.Flink)

		// for all region bytes
		for (pCode=pmi->pNewCode+pcr->dwNewRva;pCode<pmi->pNewCode+pcr->dwNewRva+pcr->dwSize;) {

			// skip opcode prefixes, if any
			while ((*pCode==0x26) || (*pCode==0x2e) || (*pCode==0x36) || (*pCode==0x3e))
				pCode++;			

			len=c_Catchy(pCode);
			if (!len || (len==CATCHY_ERROR)) {
				// bad opcode
#ifdef DEBUG_DISASM
				DbgPrint("bad opcode at %08X\n",TOPREFERRED(pCode));
#endif
				return;
			}

			dwNewLabelRva=-1;

			if (pDest=GetJumpDest(pCode,&bFork,TRUE)) {
				// calculate old label rva
				dwRvaInOriginalModule=pDest-pmi->pNewCode-pcr->dwNewRva+pcr->dwOldRva;

				// is label in the current region?
				if ((pcr->dwOldRva<=dwRvaInOriginalModule) &&
						(dwRvaInOriginalModule<pcr->dwOldRva+pcr->dwSize)) {

						// label is here, get new label rva						
						dwNewLabelRva=dwRvaInOriginalModule-pcr->dwOldRva+pcr->dwNewRva;						
					}
				else
					for (pLabelRegion=(PCODE_REGION)pmi->CodeRegions.Flink;
						pLabelRegion!=(PCODE_REGION)&pmi->CodeRegions;
						pLabelRegion=(PCODE_REGION)pLabelRegion->le.Flink)

						if ((pLabelRegion->dwOldRva<=dwRvaInOriginalModule) &&
							(dwRvaInOriginalModule<pLabelRegion->dwOldRva+pLabelRegion->dwSize)) {

							// found region with the label, get new label rva
							dwNewLabelRva=dwRvaInOriginalModule-pLabelRegion->dwOldRva+pLabelRegion->dwNewRva;
							break;
						}

				if (dwNewLabelRva==-1) {
					// should never get here
#ifdef DEBUG_FIXRELS
					DbgPrint("FixJumpsAndCalls(): Label region not found! OldRva==.%08X\n",dwRvaInOriginalModule);
#endif
					return;
				}

				// patch new delta to the instruction
				switch (len) {
					case 5:
					case 6:
						*(PDWORD)(pCode+len-4)=dwNewLabelRva-(len+pCode-pmi->pNewCode);
						break;
					case 2:
#ifdef DEBUG_FIXRELS
						if ((char)(dwNewLabelRva-(len+pCode-pmi->pNewCode))!=(long)(dwNewLabelRva-(len+pCode-pmi->pNewCode)))
							// should never happen
							DbgPrint("FixJumpsAndCalls(): Jump expanding needed?! (%08X->%08X)\n",pCode-pmi->pNewCode,dwNewLabelRva);
#endif
						*(PBYTE)(pCode+1)=(BYTE)(dwNewLabelRva-(len+pCode-pmi->pNewCode));
						break;
				}
			}

			pCode+=len;
		}	
}

static void AddImportedFunctions(PMODULE_INFORMATION pmi,PBYTE pImage)
{
	PIMAGE_FILE_HEADER	pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	PIMAGE_IMPORT_DESCRIPTOR	pid;	
	PIMAGE_IMPORT_BY_NAME	*ppibn;
	PFUNCTION_INFORMATION	pfi;
	PCHAR	szLibName;
	DWORD	dwFuncRva,i;

#define	KERNEL	"ntoskrnl.exe"

	GetHeaders(pImage,&pfh,&poh,&psh);

	try {
		if (poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress) {
			pid=(PIMAGE_IMPORT_DESCRIPTOR)RVATOVA(pImage,poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
			for (;pid->Name;pid++) {
				szLibName=RVATOVA(pImage,pid->Name);
				if (!_stricmp(szLibName,KERNEL)) {
					for (i=0,ppibn=RVATOVA(pImage,pid->OriginalFirstThunk);*ppibn;ppibn++,i++) {					
						dwFuncRva=GetProcRva(pmi->hModule,(*ppibn)->Name);
						if (dwFuncRva) {
							pfi=AddFunction(&pmi->Subs,dwFuncRva);
							pfi->pImport=(PDWORD)RVATOVA(pImage,4*i+pid->FirstThunk);
						}
					}
				}
			}
		}
	} except (EXCEPTION_EXECUTE_HANDLER) {
#ifdef DEBUG
		DbgPrint("AddImportedFunctions(): Bug\n");
#endif
	}
}

static void PatchDWORD(PDWORD pWhere,DWORD pWhat)
{
	PMDL	pMdl;

	pMdl=IoAllocateMdl(pWhere,PAGE_SIZE,FALSE,FALSE,NULL);
	MmBuildMdlForNonPagedPool(pMdl);

	*(PDWORD)MmMapLockedPages(pMdl,KernelMode)=pWhat;

	return;
}

static void RepatchImports(PLIST_ENTRY ListHead)
{
	PFUNCTION_INFORMATION	pfi;

	pfi=(PFUNCTION_INFORMATION)ListHead->Flink;
	while (&pfi->le!=ListHead) {
		if (pfi->pImport) PatchDWORD(pfi->pImport,pfi->pNewCode);
		pfi=(PFUNCTION_INFORMATION)pfi->le.Flink;
	}
}

NTSTATUS NTAPI MapNtoskrnlImage(PMODULE_INFORMATION pmi,PDWORD pdwKernelBase)
{	
	UNICODE_STRING	FileName;	
	OBJECT_ATTRIBUTES	oa;
	HANDLE	hKernelFile;
	IO_STATUS_BLOCK	iosb;
	PCHAR	pKernelName;
	PMODULES	pModules=(PMODULES)&pModules;
	DWORD	rc,dwNeededSize;
	NTSTATUS	status;

#define MAX_PATH 0x104
	WCHAR	wSystemRoot[MAX_PATH+1],wFileName[MAX_PATH+1];
	UNICODE_STRING SystemRoot={0,(MAX_PATH+1)<<1,wSystemRoot};
	RTL_QUERY_REGISTRY_TABLE rqrtSystemRoot[]={
		{NULL,
			RTL_QUERY_REGISTRY_DIRECT,
			L"SystemRoot",
			&SystemRoot,
			REG_NONE,
			NULL,
			0},
		{NULL,0,NULL,NULL,REG_NONE,NULL,0}
	};


#ifdef MAP_NTOSKRNL_SECTION
	HANDLE	hKernelSection;
	DWORD	ViewSize;
#else
	FILE_STANDARD_INFORMATION	fsi;
	PBYTE	NtoskrnlFileBuffer;
	PIMAGE_FILE_HEADER	pfh;
	LARGE_INTEGER	tmp;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	DWORD	i;
#endif

	// sanity check
	if (!pmi || !pdwKernelBase) return STATUS_UNSUCCESSFUL;

	// Most of these 'tries' are useless because almost all memory we're operating
	// here is above 2Gb. But dereferencing bad pointers is less risky.
	try {
		// get system modules - ntoskrnl is always first there
		rc=ZwQuerySystemInformation(SystemModuleInformation,pModules,4,&dwNeededSize);
		if (rc==STATUS_INFO_LENGTH_MISMATCH) {
			pModules=ExAllocatePool(PagedPool,dwNeededSize);
			RtlZeroMemory(pModules,dwNeededSize);
			rc=ZwQuerySystemInformation(SystemModuleInformation,pModules,dwNeededSize,NULL);
		} else {
strange:
#ifdef DEBUG
			DbgPrint("PullOutCode(): strange NtQuerySystemInformation()!\n");
#endif
			return STATUS_NTOSKRNL_NOT_FOUND;
		}
		if (!NT_SUCCESS(rc)) goto strange;

		// kernel imagebase
		*pdwKernelBase=(DWORD)pModules->smi.Base;
		// kernel filename - it may be renamed in the boot.ini
		pKernelName=pModules->smi.ModuleNameOffset+pModules->smi.ImageName;

		// read SystemRoot value from HKLM\Software\Microsoft\Windows NT\CurrentVersion
		status=RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,L"",rqrtSystemRoot,NULL,NULL);
		if (status!=STATUS_SUCCESS) {
			return STATUS_NTOSKRNL_NOT_FOUND;
		}

		if (*(PWORD)((DWORD)wSystemRoot+SystemRoot.Length-2)!='\\') {
			if (!NT_SUCCESS(RtlAppendUnicodeToString(&SystemRoot,L"\\")))
				return STATUS_NTOSKRNL_NOT_FOUND;
		}

		// build a pathname of the ntoskrnl.exe
		_snwprintf(wFileName,
					sizeof(wFileName),
					L"\\DosDevices\\%ssystem32\\%S",
					wSystemRoot,
					pKernelName);	

		RtlInitUnicodeString(&FileName,wFileName);	

		InitializeObjectAttributes(&oa,&FileName,OBJ_CASE_INSENSITIVE,NULL,NULL);

		if (NT_SUCCESS(NtOpenFile(&hKernelFile,
								GENERIC_READ | SYNCHRONIZE,
								&oa,
								&iosb,
								FILE_SHARE_READ,
								FILE_SYNCHRONOUS_IO_NONALERT))) {
			RtlZeroMemory(pmi,sizeof(MODULE_INFORMATION));
#ifdef MAP_NTOSKRNL_SECTION
			// map view of section
			InitializeObjectAttributes(&oa,NULL,OBJ_CASE_INSENSITIVE,NULL,NULL);
			if (NT_SUCCESS(NtCreateSection(&hKernelSection,SECTION_MAP_READ,&oa,0,PAGE_READONLY,SEC_IMAGE,hKernelFile))) {
				ViewSize=0;
				NtMapViewOfSection(hKernelSection,NtCurrentProcess(),&mi.hModule,0,0,0,&ViewSize,TRUE,0,PAGE_READONLY);
				NtClose(hKernelSection);
			}
			NtClose(hKernelFile);
#else
			// read file and build virtual image

			ZwQueryInformationFile(hKernelFile,&iosb,&fsi,sizeof(FILE_STANDARD_INFORMATION),FileStandardInformation);
			NtoskrnlFileBuffer=ExAllocatePool(PagedPool,fsi.EndOfFile.LowPart);
			RtlZeroMemory(NtoskrnlFileBuffer,fsi.EndOfFile.LowPart);
			tmp.QuadPart=0;
			rc=NtReadFile(hKernelFile,
					NULL,
					NULL,
					NULL,
					&iosb,
					NtoskrnlFileBuffer,
					fsi.EndOfFile.LowPart,
					&tmp,
					NULL);
			NtClose(hKernelFile);

			if (rc==STATUS_SUCCESS) {
				GetHeaders(NtoskrnlFileBuffer,&pfh,&poh,&psh);			

				pmi->hModule=ExAllocatePool(PagedPool,poh->SizeOfImage);
				RtlZeroMemory(pmi->hModule,poh->SizeOfImage);

				// copy header to the virtual image buffer
				memcpy(pmi->hModule,NtoskrnlFileBuffer,poh->SizeOfHeaders);			
				// copy sections
				for (i=0;i<pfh->NumberOfSections;i++,psh++)				
					memcpy(pmi->hModule+psh->VirtualAddress,
							NtoskrnlFileBuffer+psh->PointerToRawData,
							psh->SizeOfRawData);
			
				ExFreePool(NtoskrnlFileBuffer);
			}
#endif		
		}

		ExFreePool(pModules);

		InitializeListHead(&pmi->CodeRegions);
		InitializeListHead(&pmi->Subs);

		// enumerate module sections and find those containing code
		GetSectionsWithCode(pmi);

	} except(EXCEPTION_EXECUTE_HANDLER) {
		// return 'map failed' status
		pmi->hModule=0;
	}
	
	if (!pmi->hModule)
		return STATUS_MAP_IMAGE_FAILED;


	return STATUS_SUCCESS;

}

VOID NTAPI UnmapNtoskrnlImage(PMODULE_INFORMATION pmi)
{
	if (!pmi || !pmi->hModule) return;

	try {
#ifdef MAP_NTOSKRNL_SECTION
		ZwUnmapViewOfSection(NtCurrentProcess(),pmi->hModule);
#else
		ExFreePool(pmi->hModule);
#endif
	} except(EXCEPTION_EXECUTE_HANDLER) {
	}

	return;
}


NTSTATUS NTAPI PullOutCode(PIMPORT_ENTRY Import,PDWORD CodeEntries,PBYTE *NewCode,PDWORD NewCodeSize,PBYTE pModuleForImportPatching,FIXRELOCS_CALLBACK FixrelsCallback)
{
	MODULE_INFORMATION	mi;
	DWORD	dwFuncRva,i,dwThisRva,dwKSDT;
	DWORD	dwKernelBase,pWaitListHeads[2]={0,0};
	PCODE_REGION	pcr;
	PFUNCTION_INFORMATION	pfi;
	PLIST_ENTRY	ListHead;
	NTSTATUS	status;
	PDWORD	KiServiceTable;


	if (KeGetCurrentIrql()!=PASSIVE_LEVEL)
		return STATUS_PASSIVE_LEVEL_REQUIRED;
	
	RtlZeroMemory(&mi,sizeof(mi));
	if (!NT_SUCCESS(status=MapNtoskrnlImage(&mi,&dwKernelBase)))
		return status;

	try {
		for (i=0;Import[i].szName;i++) {
			dwFuncRva=0;
			switch (Import[i].dwType) {
				case IMPORT_BY_NAME:
					if (!(dwFuncRva=GetProcRva(mi.hModule,Import[i].szName))) {
#ifdef DEBUG
						DbgPrint("PullOutCode(): Failed to get %s rva!\n",Import[i].szName);
#endif
					}
					break;
				case IMPORT_BY_RVA:
					dwFuncRva=(DWORD)Import[i].szName;
					break;
				case IMPORT_BY_ADDRESS:
					dwFuncRva=(DWORD)Import[i].szName-dwKernelBase;
					break;
				case IMPORT_BY_SERVICE_ID:
					// do not search this rva if it has been already found
					if (!KiServiceTable_RVA) {
						if (!(KiServiceTable_RVA=FindKiServiceTable(mi.hModule))) {
#ifdef DEBUG
							DbgPrint("PullOutCode(): Failed to get KiServiceTable RVA!\n");
#endif
							break;
						}
					}
					KiServiceTable=(PDWORD)(KiServiceTable_RVA+mi.hModule);
					// at this time we have KiServiceTable address pointing in our mapping.
					// get rva of the appropriate handler.
					dwFuncRva=KiServiceTable[(DWORD)Import[i].szName]-mi.dwImageBase;
					break;
				default:
#ifdef DEBUG
					DbgPrint("PullOutCode(): Invalid IMPORT_ENTRY.dwType specified at entry #%d: %d\n",i,Import[i].dwType);
#endif
					break;
			}		
			AddFunction(&mi.Subs,dwFuncRva);
		}
	} except(EXCEPTION_EXECUTE_HANDLER) {
		return STATUS_ADD_FUNCTION_FAILED;
	}


	try {
		if (pModuleForImportPatching) AddImportedFunctions(&mi,pModuleForImportPatching);

		// trace all execution paths and build a list of regions
		// to which execution may be transferred
		if (!NT_SUCCESS(status=FindCodeRegions(&mi)))
			return status;

		mi.pNewCode=MmAllocateNonCachedMemory(mi.dwSizeOfRegions);
	
		if (!mi.pNewCode) {
#ifdef DEBUG
			DbgPrint("PullOutCode(): failed to allocate %d bytes of non-cached memory",mi.dwSizeOfRegions);
#endif
			return 0;
		}

		RtlZeroMemory(mi.pNewCode,mi.dwSizeOfRegions);

		// merge separate regions into one big piece of code
		// and return new api entries
		for (pcr=(PCODE_REGION)mi.CodeRegions.Flink, dwThisRva=0;
			pcr!=(PCODE_REGION)&mi.CodeRegions;
			pcr=(PCODE_REGION)pcr->le.Flink) {

			pcr->dwNewRva=dwThisRva;
			memmove(mi.pNewCode+dwThisRva,(PBYTE)mi.hModule+pcr->dwOldRva,pcr->dwSize);

			i=0;
			ListHead=&mi.Subs;
			pfi=(PFUNCTION_INFORMATION)ListHead->Flink;		
			while (&pfi->le!=ListHead) {
				// found region with old function rva?
				if ((pcr->dwOldRva<=pfi->dwRva) && (pfi->dwRva<pcr->dwOldRva+pcr->dwSize))
					// return new function entry
					if (!pfi->pImport)
						CodeEntries[i]=(DWORD)mi.pNewCode+pcr->dwNewRva+(pfi->dwRva-pcr->dwOldRva);
					else
						// they will be patched later
						pfi->pNewCode=(DWORD)mi.pNewCode+pcr->dwNewRva+(pfi->dwRva-pcr->dwOldRva);
				pfi=(PFUNCTION_INFORMATION)pfi->le.Flink;
				i++;
			}
			dwThisRva+=pcr->dwSize;
		}
	} except(EXCEPTION_EXECUTE_HANDLER) {
		return STATUS_COVERAGE_ERROR;
	}


	try {
		// fix relocation items - set them to the original imagebase
		FixRelocs(&mi,dwKernelBase,FixrelsCallback);
	
		// ntoskrnl image isn't needed any more
		UnmapNtoskrnlImage(&mi);

		// fix relative jmp's and call's
		FixJumpsAndCalls(&mi);

		// patch new import
		if (pModuleForImportPatching) RepatchImports(&mi.Subs);

		FreeLinkedList(mi.pSectionsWithCode);
		FreeDoubleLinkedList(&mi.Subs);
		FreeDoubleLinkedList(&mi.CodeRegions);

		// return reconstructed code address, if needed
		if (NewCode) *NewCode=mi.pNewCode;
		if (NewCodeSize) *NewCodeSize=mi.dwSizeOfRegions;

	} except(EXCEPTION_EXECUTE_HANDLER) {
		return STATUS_CODE_REBUILDING_FAILED;
	}

	return STATUS_SUCCESS;
}

