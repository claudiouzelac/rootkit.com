/*++

Module Name:

    phide2.h

Abstract:

    Main engine definitions.

Author:

    90210 7-Dec-2004

--*/

#ifndef _PHIDE2_
#define _PHIDE2_

#include "internal.h"

#define	PHIDE_VERSION		"2.0.0"


#ifndef STATUS_PASSIVE_LEVEL_REQUIRED

#define STATUS_PASSIVE_LEVEL_REQUIRED	0xC00F0000
#define STATUS_NTOSKRNL_NOT_FOUND		0xC00F0001
#define STATUS_MAP_IMAGE_FAILED			0xC00F0002
#define STATUS_ADD_FUNCTION_FAILED		0xC00F0003
#define STATUS_COVERAGE_ERROR			0xC00F0004
#define STATUS_CODE_REBUILDING_FAILED	0xC00F0005

#endif

#define STATUS_ALREADY_STARTED	0xC00F1000
#define STATUS_UNSUPPORTED_OS	0xC00F1001

typedef	
BOOLEAN
(__stdcall *ISPROCESSHIDDEN_CALLBACK)(
	PEPROCESS Process
);

NTSTATUS 
ProcessHide(
	IN ISPROCESSHIDDEN_CALLBACK IsProcessHidden
);

NTSTATUS 
ShutdownPhide(
);


#endif	// _PHIDE2_