// callgate_usermode_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>

void TestCallgate(int GDT_Selector);

int _tmain(int argc, _TCHAR* argv[])
{
	char _c[24];
	printf("testing the callgate...\n");
	printf("please enter the callgate number to use: ");
	gets(_c);
	int num = atoi(_c);
	TestCallgate(num);

	return 0;
}

void TestCallgate(int GDT_Selector)
{
	/* 
		To access a call gate, a far pointer to the gate is provided as a target 
		operand in a CALL or JMP instruction. The segment selector from this pointer 
		identifies the call gate. The offset from the pointer is  required, but not 
		used or checked by the processor. (The offset can be set to any value.)
	*/

	// the assembler won't generate a far call for us, so we use this hack to force
	// a far call.

	// call FAR 0x69:0xAAAAAAAA
	//          ^^^^--- remember that this is the only part that matters - it must
	//                  match the callgate entry in the GDT

	DWORD	res = 0x777;
	WORD	farcall[3];
	WORD	selector = (WORD) GDT_Selector;
	
	farcall[0] = 0xAAAA;
	farcall[1] = 0xAAAA;
	farcall[2] = (selector<<3);

	printf("about to call thru callgate %d", GDT_Selector);

	__asm
    {
        push	0x444
		push	0x333
		push	0x222
		push	0x111
        call fword ptr [farcall]
        mov res,eax
    }

	printf("result was 0x%08X", res);
}


