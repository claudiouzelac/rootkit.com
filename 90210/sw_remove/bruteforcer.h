/*++

Module Name:

    bruteforcer.h

Abstract:

    Int 0x0e handler bruteforcer definitions.

Author:

    90210 19-Aug-2005

--*/

#ifndef _BRUTEFORCER_
#define _BRUTEFORCER_

#include <ntddk.h>
#include "util.h"
#include "codewalk.h"
#include "int0e.h"

extern PVOID	g_NtoskrnlStart;
extern PVOID	g_NtoskrnlEnd;


typedef struct _KNOWN_BRANCH {
	LIST_ENTRY	le;
	PVOID	Address;	// execution will somehow come to the iretd or to the unpredictable jump from this address
	ULONG	Kind;	// kind of the branch - does it pass execution to the original handler or not (KIND_PASSDOWN or KIND_ENDS_WITH_IRETD)
} KNOWN_BRANCH, *PKNOWN_BRANCH;


NTSTATUS NTAPI StartTracer();

#define	PAGE_ADDRESS_ADDITION	0x800

extern PUCHAR	g_Int0EHandler;
extern LIST_ENTRY g_KnownBranches;
extern ULONG	g_PageToCheck;
extern KEVENT	g_ShutdownEvent;
extern PETHREAD	g_BruteforcerThread;


#endif	// _BRUTEFORCER_