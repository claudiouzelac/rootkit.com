/*++

Module Name:

    int0e.h

Abstract:

    Page fault handler definitions.

Author:

    90210 19-Aug-2005

--*/

#ifndef _INT0E_
#define _INT0E_

#include <ntddk.h>
#include "util.h"

extern LIST_ENTRY	g_HiddenPages;
extern ULONG	g_RealInt0EHandler;

typedef struct _HIDDEN_PAGE {
	LIST_ENTRY	le;
	PVOID	Address;
	PULONG	Pte;
} HIDDEN_PAGE, *PHIDDEN_PAGE;


VOID InstallNewPageFaultHandler();

#endif	// _INT0E_