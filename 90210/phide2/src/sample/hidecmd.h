/*++

Module Name:

    hidecmd.h

Abstract:

    Engine sample usage definitions.

Author:

    90210 30-Nov-2004

--*/

#ifndef _HIDECMD_
#define _HIDECMD_

#include <ntddk.h>
#include "phide2.h"

#define	HIDDEN_PROCESS	"cmd.exe"

#define SYSNAME    "System"

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID	DllBase;
	PVOID	EntryPoint;
	ULONG	SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG	Flags;
	USHORT	LoadCount;
	USHORT	TlsIndex;
	PVOID	SectionPointer;
	ULONG	CheckSum;
	ULONG	TimeDateStamp;
	PVOID	EntryPointActivationContext;
	PVOID	PatchInformation;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

#endif	// _HIDECMD_