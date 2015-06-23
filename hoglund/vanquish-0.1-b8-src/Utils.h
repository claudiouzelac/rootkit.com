/******************************************************************************\

	Vanquish Utils - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __VANQUISH_UTILS__H__
#define __VANQUISH_UTILS__H__

#define _WIN32_WINNT 0x0500
#include <windows.h>

//return number of characters in string
#define STR_NUMCHA(lpsz) strlen(lpsz)
#define STR_NUMCHW(lpsz) wcslen(lpsz)

//return size of string (from no of chars) in bytes without trailing null
#define STR_SIZEA(num) ((num) * sizeof(CHAR))
#define STR_SIZEW(num) ((num) * sizeof(WCHAR))

//return size of string (from no of chars) in bytes with trailing null
#define STRB_SIZEA(num) (((num) + 1) * sizeof(CHAR))
#define STRB_SIZEW(num) (((num) + 1) * sizeof(WCHAR))

//some quick things macro-ed
#define OLD_API(vapi) lpOld##vapi
#define ADDR_OF(vapi, vmod) Vanquish_ModuleFunction(#vapi, hm##vmod)
#define MODULE(vmod) (hm##vmod)
#define FLAG(vapi) dwV##vapi

#define LOAD_MODULE(vmod) { if ((hm##vmod = GetModuleHandleW(L#vmod L".DLL")) == NULL) hm##vmod = LoadLibraryW(L#vmod L".DLL"); }
//#define LOAD_MODULE(vmod) { hm##vmod = GetModuleHandleW(L#vmod L".DLL"); }

//declaring statements
#define DECLARE_FLAG(vapi) DWORD dwV##vapi
#define DECLARE_BUF(vapi) BYTE bufV##vapi[JMP_SIZE]
#define DECLARE_BUFHOOK(vapi) BYTE bufHookV##vapi[JMP_SIZE]
#define DECLARE_ADDR(vapi) LPVOID lpOld##vapi

//generalised declaring statements
#define DECLARE_NEWENTRY(vapi) DECLARE_FLAG(vapi) = 0; DECLARE_BUF(vapi); DECLARE_BUFHOOK(vapi); DECLARE_ADDR(vapi) = NULL
#define EXTERN_NEWENTRY(vapi) extern DECLARE_FLAG(vapi); extern DECLARE_BUF(vapi); extern DECLARE_BUFHOOK(vapi); extern DECLARE_ADDR(vapi)
#define DECLARE_MODULE(vmod) HMODULE hm##vmod

//exception handling(actually exception reporting...)
#define VBEGIN __try {
#define VEND } __except(NOWEXCEPTION) {}
#define NOWEXCEPTION Vanquish_Exception(__FILE__, __LINE__, GetExceptionCode(), GetExceptionInformation())

//prototypes
void Vanquish_DumpDWORD(DWORD x);
void Vanquish_Dump(LPCSTR what);
void Vanquish_DumpW(LPCWSTR what);
void Vanquish_Dump2Log(LPCSTR what);
void Vanquish_DumpWithErrorCode(LPCSTR what, DWORD pre_error);
int Vanquish_Exception(LPCSTR lpFile, DWORD dwLine, DWORD dwExcep, LPEXCEPTION_POINTERS lpEp);

#endif //__VANQUISH_UTILS__H__
