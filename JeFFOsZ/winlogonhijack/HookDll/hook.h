////////////////////////////////////////////////////////////////////
//
// Header file for hook.c. Contains definitions used for 
// (un)hooking API and building stub. Written by JeFFOsZ.
//
////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <windows.h>
///////////////////////////////////////////////////////////////////
// z0mbie's LDE32 v1.05 (Length-Disassembler Engine)
// (http://z0mbie.host.sk/)
//
extern int  __cdecl disasm_main(BYTE* opcodeptr);

///////////////////////////////////////////////////////////////////
// ASM stuff
//
#define JMP 0xE9 // jmp
#define NOP 0x90 // nop
#define JMP_SIZE 5 // jmp xxxxxxxx size
#define STUB_SIZE 0x10+JMP_SIZE // original "stub",max size of 1 function + jmp

///////////////////////////////////////////////////////////////////
// Function that builds a jmp in a buffer
// Arg 1: pointer to an allocated buffer
// Arg 2: address for the jmp
//
VOID BuildJMPBuffer(CHAR*,DWORD);

///////////////////////////////////////////////////////////////////
// Function that write jumpcode for particular function, saves
// the old data in an allocated "stub" buffer, fixes the stub,
// in current process.
// Arg 1: name of the module
// Arg 2: name of the function
// Arg 3: pointer to new function
// Returns the address of our "stub" for calling original process
//
LPVOID HookFunctionInCurrentProcess(LPCSTR,LPCSTR,LPVOID);

///////////////////////////////////////////////////////////////////
// Function that unhooks a particilar function in the current 
// process.
// Arg 1: name of the module
// Arg 2: name of the function
// Arg 3: pointer to the stub for the function
// Returns true if the function succeeds.
//
BOOL UnHookFunctionInCurrentProcess(LPCSTR,LPCSTR,LPVOID);
