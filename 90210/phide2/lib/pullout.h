/*++

Module Name:

    pullout.h

Abstract:

    Public engine declarations.

Author:

    90210 19-Nov-2004

--*/

#ifndef _PULLOUT_
#define _PULLOUT_

#define	PULLOUT_VERSION		"0.0.3"


#define STATUS_PASSIVE_LEVEL_REQUIRED	0xC00F0000
#define STATUS_NTOSKRNL_NOT_FOUND		0xC00F0001
#define STATUS_MAP_IMAGE_FAILED			0xC00F0002
#define STATUS_ADD_FUNCTION_FAILED		0xC00F0003
#define STATUS_COVERAGE_ERROR			0xC00F0004
#define STATUS_CODE_REBUILDING_FAILED	0xC00F0005

#define	IMPORT_BY_NAME	0
#define	IMPORT_BY_RVA	1
#define	IMPORT_BY_ADDRESS	2
#define	IMPORT_BY_SERVICE_ID	3

typedef struct _IMPORT_ENTRY {
	PCHAR	szName;
	ULONG	dwType;
} IMPORT_ENTRY, *PIMPORT_ENTRY;


// callback for patching relocs in the generated code
typedef	
VOID 
(__stdcall *FIXRELOCS_CALLBACK)(
	PUCHAR pImage,				// imagebase of the parsed module
	PULONG pFixedAddress,		// address of the pointer in the resulting code
	ULONG OriginalFixupRva,		// RVA of the pointer in the parsed module
	ULONG TargetRva				// RVA this pointer points to in the parsed module
);

// multi-purpose instruction callback
typedef	
VOID 
(__stdcall *INSTRUCTION_CALLBACK)(	
	PUCHAR pInstructionAddress,	// pointer to the instruction in the ORIGINAL code
	ULONG dwInstructionRva,		// rva of the instruction
	ULONG dwParam				// user-specified parameter
);

typedef	
BOOLEAN
(__stdcall *RELOCS_CALLBACK)(
	PUCHAR pImage,				// imagebase of the module
	ULONG dwRva,				// rva of the reloc
	ULONG dwParam,				// user-specified parameter
	PULONG rc					// callback return code
);

typedef	struct SECTION {
	struct	SECTION	*Next;
	IMAGE_SECTION_HEADER	sh;
} SECTION, *PSECTION;

typedef struct _MODULE_INFORMATION {	
	ULONG	dwImageBase;			// preferred imagebase
	PUCHAR	hModule;				// mapped imagebase
	ULONG	dwSizeOfImage;			// module virtual size
	PSECTION	pSectionsWithCode;	// valid code regions for data exports detection
	LIST_ENTRY	Subs;				// user-selected subs
	LIST_ENTRY	CodeRegions;		// regions in which selected subs may execute
	DWORD	dwSizeOfRegions;		// size of the marked code
	PBYTE	pNewCode;				// reconstructed code will be here
} MODULE_INFORMATION,*PMODULE_INFORMATION;

#ifdef __cplusplus
extern "C" {
#endif

// main engine entry
NTSTATUS 
NTAPI
PullOutCode(
	IN PIMPORT_ENTRY Import,						// filled array of IMPORT_ENTRY structures
	IN OUT PULONG CodeEntries,						// array for code entries in the resulting code
	OUT PUCHAR *NewCode OPTIONAL,					// address of the buffer with the resulting code
	OUT PULONG NewCodeSize OPTIONAL,				// size of the resulting code
	IN PUCHAR pModuleForImportPatching OPTIONAL,	// imagebase of the module which imports need to be included to the code
	IN FIXRELOCS_CALLBACK FixRelocsCallback OPTIONAL// user-defined callback for fixing absolute pointers in the resulting code
);

// helper functions for disassembler

NTSTATUS 
NTAPI
WalkBranch(
	IN PMODULE_INFORMATION pmi,						// filled structure describing module with code
	IN ULONG dwEipRva,								// rva to start walk from
	IN OUT PUCHAR pOpcodeStart,						// user-allocated bitfield which will be filled with opcode starts
	IN OUT PUCHAR pCoverage,						// user-allocated bitfield which will be filled with code coverage map
	IN OUT PLIST_ENTRY Branches,					// double linked list of code branches found during walk of this branch
	IN INSTRUCTION_CALLBACK InstructionCallback OPTIONAL,// this callback will be called on each instruction
	IN ULONG dwCallbackParam OPTIONAL,				// user-specified parameter for callback
	IN BOOLEAN bFollowCalls							// specifies whether found subfunctions should be placed to the Branches list
);

BOOLEAN
NTAPI
PointerToCode(
	PMODULE_INFORMATION pmi,
	PUCHAR pAddr
);

// useful stuff

VOID 
NTAPI 
FreeDoubleLinkedList(
	IN PLIST_ENTRY ListHead
);

VOID
NTAPI 
FreeLinkedList(
	IN PVOID ListHead
);

ULONG 
NTAPI
GetHeaders(
	PUCHAR ibase,
	PIMAGE_FILE_HEADER *pfh,
	PIMAGE_OPTIONAL_HEADER *poh,
	PIMAGE_SECTION_HEADER *psh
);

ULONG
NTAPI 
GetProcRva(
	PUCHAR hModule,
	PCHAR szProcName
);

ULONG
NTAPI
ParseRelocs(
	PUCHAR pImage,
	RELOCS_CALLBACK RelocsCallback,
	ULONG dwParam
);

ULONG
NTAPI
FindKiServiceTable(
	PUCHAR pImage
);

NTSTATUS 
NTAPI 
MapNtoskrnlImage(
	PMODULE_INFORMATION pmi,
	PULONG pdwKernelBase
);

VOID 
NTAPI 
UnmapNtoskrnlImage(
	PMODULE_INFORMATION pmi
);


DWORD	KiServiceTable_RVA;


#ifdef __cplusplus
}
#endif

#pragma comment(lib, "pullout")




#endif	// _PULLOUT_