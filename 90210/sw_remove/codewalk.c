/*++

Module Name:

    codewalk.c

Abstract:

    Engine to walk the code.

Author:

    90210 19-Aug-2005

--*/

#include "codewalk.h"




// calculates eip after 5- or 6-bytes jump
static PUCHAR CalcDest32(PUCHAR uEip,ULONG uJmpSize)
{
	return (PUCHAR)(*(PULONG)(uEip+uJmpSize)+(ULONG)uEip+uJmpSize+4);
}

// calculates eip after 2-bytes jump
static PUCHAR CalcDest8(PUCHAR uEip)
{
	return (PUCHAR)(*(signed char*)(uEip+1)+(ULONG)uEip+2);
}

// checks if current opcode transfers execution to undeterminable place
// bad jumps: 0ff20h-0ff2fh, 0ff60h-0ff6fh, 0ffe0h-0ffefh
static BOOLEAN ShouldBreak(PUCHAR uEip)
{
	// iretd?
	if (*uEip==0xcf) return TRUE;

	if (*uEip++!=0xff) return FALSE;

	return (((0x20<=*uEip) && (*uEip<0x30)) ||
			((0x60<=*uEip) && (*uEip<0x70)) ||
			((0xe0<=*uEip) && (*uEip<0xf0)));
}

// checks if current opcode is ret. if it is, returns paramcount (stack correction/4)
static BOOLEAN ShouldReturn(PUCHAR uEip,PULONG uParamCount)
{
	if (*uEip==0xc2) {
		*uParamCount=((*(PUSHORT)(uEip+1))>>2);

		return TRUE;
	}	
	if (*uEip==0xc3) {
		*uParamCount=0;
		return TRUE;
	}
	return FALSE;
}

static PUCHAR GetJumpDest(PUCHAR uEip,PBOOLEAN bFork,BOOLEAN bFollowCalls)
{
	// call (0xe8)
	if (bFollowCalls && (0xe8==*uEip)) {
		*bFork=TRUE;
		return CalcDest32(uEip,1);
	}

	// unconditional jumps (0xe9, 0xeb)
	if (0xe9==*uEip) {		
		*bFork=FALSE;
		return CalcDest32(uEip,1);
	}

	if (0xeb==*uEip) {		
		*bFork=FALSE;
		return CalcDest8(uEip);
	}

	// conditional jumps (0x70-0x7f) and ecx-oriented jumps (0xe0-0xe3)
	if (((0x70<=*uEip) && (*uEip<=0x7f)) ||
		((0xe0<=*uEip) && (*uEip<=0xe3))) {
		*bFork=TRUE;
		return CalcDest8(uEip);
	}

	// extended conditionals (0x0f80-0x0f8f)
	if (*uEip++!=0x0f)
		return 0;
	else 
		if ((0x80<=*uEip) && (*uEip<=0x8f)) {
			*bFork=TRUE;
			return CalcDest32(uEip-1,2);
		}

	// it is not a jmp
	return 0;
}


void NTAPI FreeDoubleLinkedList(PLIST_ENTRY ListHead)
{
	PLIST_ENTRY Head=ListHead,Next;
	if (!ListHead) return;
		
	ListHead=ListHead->Flink;
	while (ListHead!=Head) {
		Next=ListHead->Flink;
		RemoveEntryList(ListHead);
		ExFreePool(ListHead);
		ListHead=Next;
	}
	return;
}



static PBRANCH FindBranch(PLIST_ENTRY Branches, PVOID BranchStart)  
{
	PBRANCH	Branch;

	Branch=(PBRANCH)Branches->Flink;
	while (Branch!=(PBRANCH)Branches) {

		Branch=CONTAINING_RECORD(Branch,BRANCH,le);
		if (Branch->Start==BranchStart)
			return Branch;
		
		Branch=(PBRANCH)Branch->le.Flink;
	}

	return NULL;
}


NTSTATUS NTAPI WalkBranch(
				PUCHAR Address,			
				PLIST_ENTRY Branches,
				PBRANCH CurrentBranch,
				INSTRUCTION_CALLBACK InstructionCallback,
				PVOID dwCallbackParam,
				BOOLEAN bFollowCalls)
{
	ULONG	len,dwParamCount,uJumpSize;
	PUCHAR	pDest,pNextOpcode;
	BOOLEAN	bConditional,bWasPrefix;	
	PBRANCH	pb;

	__try {
		
		for (;;) {
			
			pNextOpcode=(PUCHAR)Address;			
			// skip seg prefixes
			bWasPrefix=FALSE;
			while ((*pNextOpcode==0x26) || (*pNextOpcode==0x2e) || (*pNextOpcode==0x36) || (*pNextOpcode==0x3e) /*||
				(*pNextOpcode==0x65) || (*pNextOpcode==0x66)*/) {
				// mark instruction prefixes
				
				Address++;
				pNextOpcode++;
				bWasPrefix=TRUE;
			}

			if (!bWasPrefix) {
				len=c_Catchy((PVOID)Address);			
				if (!len || (len==CATCHY_ERROR)) {
					// bad opcode
#ifdef DEBUG_DISASM
					DbgPrint("bad opcode at %08X\n",Address);
#endif
					return STATUS_UNSUCCESSFUL;
				}
				
				pNextOpcode+=len;
				
			} else
				Address=pNextOpcode;

			if (InstructionCallback) 
				if (!InstructionCallback(Address,dwCallbackParam))
					// drop this branch on user request
					return STATUS_SUCCESS;

			// ret?
			if (ShouldReturn(Address,&dwParamCount)) {
#ifdef DEBUG_DISASM
				DbgPrint("ret %i*4 at %08X\n",dwParamCount,Address);
#endif
				return STATUS_SUCCESS;
			}

			// this branch is over - can't predict execution flow
			if (ShouldBreak(Address)) {
#ifdef DEBUG_DISASM
				DbgPrint("Unpredictable jump at %08X!\n",Address);
#endif
				return STATUS_UNPREDICTABLE_JUMP;
			}
		
			pDest=GetJumpDest(Address,&bConditional,bFollowCalls);

			
			if (pDest) {

				// it's a jump

				uJumpSize=c_Catchy(Address);

				if (pDest==Address+uJumpSize) {
					// jcc $+<jcc_size>: set new eip						
					Address=pDest;
					continue;
				} else {

					if (!bConditional) {
						// it's a non conditional jump. check if it's dest is a start of a branch:
						// drop jump if it is, add branch to the list otherwise.

						// this code avoids deadlocks in parsing "jmp $-2", for example.

						if (!(pb=FindBranch(Branches,pDest))) {
							// this branch has not been found, add it to the list

							pb=ExAllocatePool(PagedPool,sizeof(BRANCH));
							RtlZeroMemory(pb,sizeof(BRANCH));
							pb->Start=pDest;

							InsertTailList(Branches,&pb->le);
						}

						CurrentBranch->LeftChild=pb;
						
						// drop the branch
						return STATUS_SUCCESS;
					}
					// it's a conditional. attach left and right branches to the current one.
					// this is not recursive because i hate 'kernel stack is less than 0x500' bugcheck.

					// check for walked branches
					if (!(pb=FindBranch(Branches,Address+uJumpSize))) {
						// this branch has not been found, add it to the list					

						// allocate and attach left child to the current branch (jump is not taken)
						pb=ExAllocatePool(PagedPool,sizeof(BRANCH));
						RtlZeroMemory(pb,sizeof(BRANCH));
						pb->Start=Address+uJumpSize;

						InsertTailList(Branches,&pb->le);
					}

					CurrentBranch->LeftChild=pb;

					if (!(pb=FindBranch(Branches,pDest))) {
						// this branch has not been found, add it to the list

						// allocate and attach right child to the current branch (jump is taken)
						pb=ExAllocatePool(PagedPool,sizeof(BRANCH));
						RtlZeroMemory(pb,sizeof(BRANCH));
						pb->Start=pDest;

						InsertTailList(Branches,&pb->le);
					}

					CurrentBranch->RightChild=pb;

					// current branch is over: if there are new branches found they are added to the list.
					return STATUS_SUCCESS;
				}
			}
			// advance to the next instruction
			Address=pNextOpcode;
		}
		

	} __except (EXCEPTION_EXECUTE_HANDLER) {
#ifdef DEBUG
		DbgPrint("WalkBranch(): Bug at Address==%08X\n",Address);
#endif
		return STATUS_UNSUCCESSFUL;
	}

	// should never get here
}

