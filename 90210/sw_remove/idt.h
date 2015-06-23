/*++

Module Name:

    idt.h

Abstract:

    _IDT searcher's definitions.

Author:

    90210 19-Aug-2005

--*/

#ifndef _IDT_
#define _IDT_

#include <ntddk.h>
#include "pe.h"
#include "nt.h"

#define STATUS_NTOSKRNL_NOT_FOUND		0xC00F0001
#define STATUS_MAP_IMAGE_FAILED			0xC00F0002
#define	STATUS_IDT_NOT_FOUND			0xC00F0100


#define RVATOVA(base,offset) ((PVOID)((ULONG)(base)+(ULONG)(offset)))
#define ibaseDD *(PULONG)&ibase

typedef struct _MODULE_INFORMATION {	
	ULONG	dwImageBase;				// preferred imagebase
	PUCHAR	hModule;					// mapped imagebase
//	ULONG	dwSizeOfImage;				// module virtual size
} MODULE_INFORMATION,*PMODULE_INFORMATION;


typedef	
BOOLEAN
(__stdcall *RELOCS_CALLBACK)(
	PUCHAR pImage,				// imagebase of the module
	ULONG dwRva,				// rva of the reloc
	PVOID Param,				// user-specified parameter
	PULONG rc					// callback return code
);


typedef struct _IDT_ENTRY {
	ULONG	Address;
	USHORT	Access;
	USHORT	SegmentSelector;
} IDT_ENTRY, *PIDT_ENTRY;

NTSTATUS 
NTAPI 
FindIDTEntry(
	UCHAR InterruptNumber,
	PULONG IdtEntry
);

#endif	// _IDT_