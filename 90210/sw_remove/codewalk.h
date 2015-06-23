/*++

Module Name:

    codewalk.h

Abstract:

    Code walker definitions.

Author:

    90210 19-Aug-2005

--*/

#ifndef _CODEWALK_
#define _CODEWALK_

#include <ntddk.h>
#include "catchy32.h"


#define	NOT_COUNTED	0xffffffff
#define	END_BY_IRETD	0xfffffffe


#define	KIND_UNKNOWN	0
#define	KIND_ENDSWITHIRETD	1
#define	KIND_PASSDOWN	2

typedef struct BRANCH {
	LIST_ENTRY	le;
	PUCHAR	Start;
	struct	BRANCH *LeftChild;
	struct	BRANCH *RightChild;
	ULONG	Kind;	// kind of a branch: ENDS_WITH_IRETD or PASSDOWN
} BRANCH, *PBRANCH;

typedef	
BOOLEAN 
(__stdcall *INSTRUCTION_CALLBACK)(	
	PUCHAR pInstructionAddress,	// pointer to the instruction
	PVOID Param					// user-specified parameter
);

#define	STATUS_UNPREDICTABLE_JUMP	0xc0fff000

NTSTATUS NTAPI WalkBranch(
				PUCHAR Address,
				PLIST_ENTRY Branches,
				PBRANCH CurrentBranch,
				INSTRUCTION_CALLBACK InstructionCallback,
				PVOID dwCallbackParam,
				BOOLEAN bFollowCalls);

void NTAPI FreeDoubleLinkedList(PLIST_ENTRY ListHead);
void NTAPI FreeLinkedList(PVOID ListHead);

PUCHAR CalcDest32(PUCHAR dwEip,ULONG dwJmpSize);


#endif	// _CODEWALK_