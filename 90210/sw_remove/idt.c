/*++

Module Name:

    idt.c

Abstract:

    Ntoskrnl's _IDT searcher.

Author:

    90210 19-Aug-2005

--*/

#include "idt.h"

static BOOLEAN GetHeaders(PUCHAR ibase,
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
	*pfh=(PIMAGE_FILE_HEADER)((PUCHAR)*pfh+sizeof(IMAGE_NT_SIGNATURE));
	
	*poh=(PIMAGE_OPTIONAL_HEADER)((PUCHAR)*pfh+sizeof(IMAGE_FILE_HEADER));
	if ((*poh)->Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		return FALSE;
	
	*psh=(PIMAGE_SECTION_HEADER)((PUCHAR)*poh+sizeof(IMAGE_OPTIONAL_HEADER));
	return TRUE;
}


static ULONG ParseRelocs(PUCHAR pImage,RELOCS_CALLBACK RelocsCallback,PVOID Param)
{
	PIMAGE_FILE_HEADER	pfh;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	PIMAGE_BASE_RELOCATION	pbr;
	PIMAGE_FIXUP_ENTRY	pfe;

	ULONG	dwFixups=0,i,dwPointerRva,rc=0;
	BOOLEAN	bFirstChunk,bContinueParsing;

	GetHeaders(pImage,&pfh,&poh,&psh);
	
	if ((poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) &&
		(!((pfh->Characteristics)&IMAGE_FILE_RELOCS_STRIPPED))) {

		pbr=(PIMAGE_BASE_RELOCATION)RVATOVA(poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,pImage);

		bContinueParsing=bFirstChunk=TRUE;

		// 1st IMAGE_BASE_RELOCATION.VirtualAddress of ntoskrnl is 0
		while ((bFirstChunk || pbr->VirtualAddress) && bContinueParsing) {
			bFirstChunk=FALSE;

			pfe=(PIMAGE_FIXUP_ENTRY)((ULONG)pbr+sizeof(IMAGE_BASE_RELOCATION));
			for (i=0;i<(pbr->SizeOfBlock-sizeof(IMAGE_BASE_RELOCATION))>>1;i++,pfe++) {
				if (pfe->type==IMAGE_REL_BASED_HIGHLOW) {
					dwFixups++;
					dwPointerRva=pbr->VirtualAddress+pfe->offset;

					rc=0;
					// stop walking relocs if callback returned false
					if (!(bContinueParsing=RelocsCallback(pImage,dwPointerRva,Param,&rc)))
						break;
				}
#ifdef DEBUG
					else
						if (pfe->type!=IMAGE_REL_BASED_ABSOLUTE)
							DbgPrint("ParseRelocs(): relo type %d found at .%X\n",pfe->type,pbr->VirtualAddress+pfe->offset);
#endif
			}
			*(PULONG)&pbr+=pbr->SizeOfBlock;
		}
	}	
#ifdef DEBUG
	if (!dwFixups) DbgPrint("ParseRelocs(): No fixups!\n");
#endif
	return rc;
}



static NTSTATUS MapNtoskrnlImage(PMODULE_INFORMATION pmi,PULONG pdwKernelBase)
{	
	UNICODE_STRING	FileName;	
	OBJECT_ATTRIBUTES	oa;
	HANDLE	hKernelFile;
	IO_STATUS_BLOCK	iosb;
	PCHAR	pKernelName;
	PMODULES	pModules=(PMODULES)&pModules;
	ULONG	rc,dwNeededSize;
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
	ULONG	ViewSize;
#else
	FILE_STANDARD_INFORMATION	fsi;
	PUCHAR	NtoskrnlFileBuffer;
	PIMAGE_FILE_HEADER	pfh;
	LARGE_INTEGER	tmp;
	PIMAGE_OPTIONAL_HEADER	poh;
	PIMAGE_SECTION_HEADER	psh;
	ULONG	i;
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
			DbgPrint("MapNtoskrnlImage(): strange NtQuerySystemInformation()!\n");
#endif
			return STATUS_NTOSKRNL_NOT_FOUND;
		}
		if (!NT_SUCCESS(rc)) goto strange;

		// kernel imagebase
		*pdwKernelBase=(ULONG)pModules->smi.Base;
		// kernel filename - it may be renamed in the boot.ini
		pKernelName=pModules->smi.ModuleNameOffset+pModules->smi.ImageName;

		// read SystemRoot value from HKLM\Software\Microsoft\Windows NT\CurrentVersion
		status=RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,L"",rqrtSystemRoot,NULL,NULL);
		if (status!=STATUS_SUCCESS) {
			return STATUS_NTOSKRNL_NOT_FOUND;
		}

		if (*(PUSHORT)((ULONG)wSystemRoot+SystemRoot.Length-2)!='\\') {
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

				pmi->dwImageBase=poh->ImageBase;
			
				ExFreePool(NtoskrnlFileBuffer);
			}
#endif		
		}

		ExFreePool(pModules);


	} except(EXCEPTION_EXECUTE_HANDLER) {
		// return 'map failed' status
		pmi->hModule=0;
	}
	
	if (!pmi->hModule)
		return STATUS_MAP_IMAGE_FAILED;


	return STATUS_SUCCESS;

}

static VOID UnmapNtoskrnlImage(PMODULE_INFORMATION pmi)
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

static BOOLEAN NTAPI CheckForFixupAt(PUCHAR pImage,ULONG dwRva,PVOID Param,PULONG rc)
{
	if (dwRva==(ULONG)Param) {
		// fixup found, stop parsing
		*rc=TRUE;
		return FALSE;
	}
	// nothing found; continue parsing
	return TRUE;
}

static BOOLEAN NTAPI FindIDTCallback(PUCHAR pImage,ULONG dwRva,PVOID Param,PULONG rc)
{
	PULONG	uPointsTo;
	ULONG	uPointsToRva,i;
	PMODULE_INFORMATION	pmi=Param;

	// get the address this reloc points to
	uPointsToRva=*(PULONG)(pImage+dwRva)-pmi->dwImageBase;
	uPointsTo=(PULONG)(uPointsToRva+pmi->hModule);

	// check for standard ntoskrnl IDT layout: 
	// _IDT    dd offset _KiTrap00		// reloc here
	//         dd 88E00h				// sel 8, seg present, dpl 0, 32bit gate
	//         dd offset _KiTrap01		// reloc here
	//         dd 88E00h
	//         dd offset _KiTrap02		// reloc here
	//         dd 88E00h
	//         dd offset _KiTrap03		// reloc here
	//         dd 8EE00h				// sel 8, seg present, dpl 3, 32bit gate
	//         ...

	// first check the flags
	if ((uPointsTo[1]!=0x88E00) ||
		(uPointsTo[3]!=0x88E00) ||
		(uPointsTo[5]!=0x88E00) ||
		(uPointsTo[7]!=0x8EE00))	// int 3 has dpl 3
		// it's not the _IDT; continue parsing
		return TRUE;

	// check for relocs at handlers' addresses (only first 8 handlers)
	for (i=0;i<8;i++)
		if (!ParseRelocs(pmi->hModule,CheckForFixupAt,(PVOID)(uPointsToRva+i*8)))
			return TRUE;
	
	// we found the _IDT, return it's rva
	*rc=uPointsToRva;
	return FALSE;
}


NTSTATUS NTAPI FindIDTEntry(UCHAR InterruptNumber,PULONG IdtEntry)
{
	MODULE_INFORMATION	mi;
	ULONG	NtoskrnlIbase;
	NTSTATUS	Status;
	PIDT_ENTRY	_IDT;


	if (!NT_SUCCESS(Status=MapNtoskrnlImage(&mi,&NtoskrnlIbase)))
		return Status;
	
	_IDT=(PIDT_ENTRY)ParseRelocs(mi.hModule,&FindIDTCallback,&mi);
	if (!_IDT) {
		UnmapNtoskrnlImage(&mi);
		return STATUS_IDT_NOT_FOUND;
	}

	DbgPrint("ntoskrnl._IDT found at .%08X\n",_IDT);
	
	// ParseRelocs returned the rva, so convert it to the address
	_IDT=(PIDT_ENTRY)((ULONG)_IDT+mi.hModule);
	
	// read the IDT entry, rebase it to the current ntoskrnl imagabase
	*IdtEntry=_IDT[InterruptNumber].Address-(ULONG)mi.dwImageBase+NtoskrnlIbase;
	
	UnmapNtoskrnlImage(&mi);
	return STATUS_SUCCESS;
}