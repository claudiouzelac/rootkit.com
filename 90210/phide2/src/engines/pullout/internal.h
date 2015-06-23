/*++

Module Name:

    internal.h

Abstract:

    Internal header.

Author:

    90210 05-Oct-2004

--*/

#ifndef _INTERNAL_
#define _INTERNAL_

#include <ntddk.h>

typedef unsigned char BOOL, *PBOOL;
typedef unsigned char BYTE, *PBYTE;
typedef unsigned long DWORD, *PDWORD;
typedef unsigned short WORD, *PWORD;

#include "pe.h"
#include "nt.h"
#include "catchy32.h"

#define TOPREFERRED(address) ((DWORD)(address)-(DWORD)pmi->hModule+pmi->dwImageBase)
#define RVATOVA(base,offset) ((PVOID)((DWORD)(base)+(DWORD)(offset)))
#define ibaseDD *(PDWORD)&ibase

typedef struct {
	LIST_ENTRY	le;
	DWORD	dwRva;
} BRANCH, *PBRANCH;

typedef struct CODE_REGION {
	LIST_ENTRY	le;	
	DWORD	dwOldRva;			// rva in the original module	
	DWORD	dwNewRva;			// rva in the reconstructed code
	DWORD	dwSize;				// size of the region
} CODE_REGION, *PCODE_REGION;

typedef struct FUNCTION_INFORMATION {
	LIST_ENTRY	le;	
	DWORD	dwRva;				// rva in the original module
	PDWORD	pImport;			// if it is not NULL, pImport points to the import that must be patched
	DWORD	pNewCode;			// function entry in the reconstructed code
} FUNCTION_INFORMATION, *PFUNCTION_INFORMATION;


#endif	// _INTERNAL_