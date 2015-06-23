/*++

Module Name:

    antihooks.c

Abstract:

    Public engine declarations.

Author:

    90210 30-Oct-2004

--*/

#ifndef _PULLOUT_
#define _PULLOUT_

#include "internal.h"

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
	PUCHAR	szName;
	ULONG	dwType;
} IMPORT_ENTRY, *PIMPORT_ENTRY;

typedef	
VOID 
(__stdcall *FIXRELOCS_CALLBACK)(
	PUCHAR pImage,				// imagebase of the parsed module
	PULONG pFixedAddress,		// address of the pointer in the resulting code
	ULONG OriginalFixupRva,		// RVA of the pointer in the parsed module
	ULONG TargetRva				// RVA this pointer points to in the parsed module
);

typedef	
VOID 
(__stdcall *INSTRUCTION_CALLBACK)(	
	PUCHAR pInstructionAddress,	// pointer to the instruction
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
	ULONG	dwImageBase;				// preferred imagebase
	PUCHAR	hModule;					// mapped imagebase
	ULONG	dwSizeOfImage;				// module virtual size
	PSECTION	pSectionsWithCode;		// valid code regions for data exports detection
	LIST_ENTRY	Subs;					// user-selected subs
	LIST_ENTRY	CodeRegions;			// regions in which selected subs may execute
	DWORD	dwSizeOfRegions;			// size of the marked code
	PBYTE	pNewCode;					// reconstructed code will be here
} MODULE_INFORMATION,*PMODULE_INFORMATION;



#endif	// _PULLOUT_