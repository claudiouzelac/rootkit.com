#include <winsock2.h>
#include <windows.h>
#include "ntdll.h"

#ifndef _ENGINE_
#define _ENGINE_

//////////////////////////////////////////////////////////////////
// z0mbie's LDE32 v1.05 (Length-Disassembler Engine)
// (http://z0mbie.host.sk/)
//
extern int _cdecl disasm_main(BYTE* opcodeptr);
//////////////////////////////////////////////////////////////////

// ASM stuff
#define NOP 0x90
#define MAX_FUNC_LEN 0x10
#define LONG_JMP_OPCODE 0xE9

// JUMPS
#define LONG_JMP "\xE9\x00\x00\x00\x00" // short jmp
#define LONG_JMP_OFFSET 0x1

#define MOVEAX_JMP "\xB8\x00\x00\x00\x00\xFF\xE0" // mov eax,addr;call eax
#define MOVEAX_JMP_OFFSET 0x1

// function that returns address to jump (possibility to calc relative jmps)
typedef DWORD (*JMPCALC)(DWORD,DWORD,DWORD); // 

// struct that holds the jump's info
typedef struct _JMP_TABLE
{
	CHAR*   pcJmpCode;
	INT     iCodeSize;
	INT     iOffset;
	JMPCALC jcFunc;
	JMPCALC jcStub;
} 
JMP_TABLE,*PJMP_TABLE;

#define RELATIVE_JMP 0
#define IMAGE_NT_SIGNATURE1 0x00004550    // 00EP

// used to get module addresses remotely (as in other process)
typedef struct _LDR_MODULE
{
    LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID BaseAddress;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashTableEntry;
    ULONG TimeDateStamp;
} 
LDR_MODULE, *PLDR_MODULE;

// Functions

LPVOID engine_NtGetModuleHandleA(LPSTR);
LPVOID engine_NtLoadLibraryA(LPSTR);
LPVOID engine_NtGetProcAddress(HMODULE,LPSTR);
HMODULE engine_GetRemoteModuleHandle(HANDLE,LPSTR);
LPVOID engine_GetRemoteProcAddress(HANDLE,HMODULE,LPSTR);
DWORD engine_GetPEImageSize(LPVOID);
BOOL engine_SaveRemoteVar(HANDLE,LPVOID,LPVOID);
BOOL engine_LoadRemoteVar(HANDLE,LPVOID,LPVOID);
BOOL engine_CopyImageToProcess(HANDLE,LPVOID);
void engine_BuildJMPBuffer(CHAR*,DWORD,INT);
// prolly the most important function, this one does the magic ;)
LPVOID engine_HookFunctionInProcess(HANDLE,LPSTR,LPSTR,LPVOID,PDWORD,LPVOID*,INT);
BOOL engine_UnHookFunctionInProcess(HANDLE,LPSTR,LPSTR,LPVOID,DWORD);

DWORD engine_LONG_JMP_FUNC_JMP(DWORD,DWORD,DWORD);
DWORD engine_LONG_JMP_STUB_JMP(DWORD,DWORD,DWORD);
DWORD engine_MOVEAX_JMP_FUNC_JMP(DWORD,DWORD,DWORD);
DWORD engine_MOVEAX_JMP_STUB_JMP(DWORD,DWORD,DWORD);

#endif