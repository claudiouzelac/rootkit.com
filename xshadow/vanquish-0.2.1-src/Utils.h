/******************************************************************************\

	Vanquish Utils - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __VANQUISH_UTILS__H__
#define __VANQUISH_UTILS__H__

#include <windows.h>

//constants
#define WAITTIMEOUT 1000
#define VANQUISH_DLL_UNLOAD_KEY (0xfac13bed)

//return number of characters in string
#define STR_NUMCHA(lpsz) lstrlenA((lpsz))
#define STR_NUMCHW(lpsz) lstrlenW((lpsz))

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
#define MODULE_UPDATEFLAG(vmod) (dwhm##vmod)
#define FLAG(vapi) dwV##vapi
#define UPDATEFLAG 1000

//#define LOAD_MODULE(vmod) { if ((hm##vmod = GetModuleHandleW(L#vmod L".DLL")) == NULL) hm##vmod = LoadLibraryW(L#vmod L".DLL"); }
//#define LOAD_MODULE(vmod) { hm##vmod = GetModuleHandleW(L#vmod L".DLL"); }
#define LOAD_MODULE(vmod) { HMODULE hmTMP##vmod = GetModuleHandleW(L#vmod L".DLL"); if ((hmTMP##vmod != NULL) && (hmTMP##vmod != hm##vmod)) { hm##vmod = hmTMP##vmod; dwhm##vmod = UPDATEFLAG; } else { dwhm##vmod = 0; } }
#define LOAD_MODULE_GINA(vmod) { HMODULE hmTMP##vmod = GetModuleHandleW(lpszGina); if ((hmTMP##vmod != NULL) && (hmTMP##vmod != hm##vmod)) { hm##vmod = hmTMP##vmod; dwhm##vmod = UPDATEFLAG; } else { dwhm##vmod = 0; } }

//declaring statements
#define DECLARE_FLAG(vapi) DWORD dwV##vapi
#define DECLARE_BUF(vapi) BYTE bufV##vapi[JMP_SIZE]
#define DECLARE_BUFHOOK(vapi) BYTE bufHookV##vapi[JMP_SIZE]
#define DECLARE_ADDR(vapi) LPVOID lpOld##vapi

//generalised declaring statements
#define DECLARE_NEWENTRY(vapi) DECLARE_FLAG(vapi) = 0; DECLARE_BUF(vapi); DECLARE_BUFHOOK(vapi); DECLARE_ADDR(vapi) = NULL
#define EXTERN_NEWENTRY(vapi) extern DECLARE_FLAG(vapi); extern DECLARE_BUF(vapi); extern DECLARE_BUFHOOK(vapi); extern DECLARE_ADDR(vapi)
#define DECLARE_MODULE(vmod) HMODULE hm##vmod; DWORD MODULE_UPDATEFLAG(vmod)
#define EXTERN_MODULE(vmod) extern HMODULE hm##vmod; extern DWORD MODULE_UPDATEFLAG(vmod)

//prototypes
//VRT stands for Vanquish RunTime Library
void __cdecl VRTWriteLog(BOOL bWinError, DWORD dwError, HMODULE hModule, LPCSTR lpFmt, ...);
void __cdecl VRTvWriteLogLOWLEVEL(LPCSTR lpFmt, va_list vaList);
void __cdecl VRTWriteLogLOWLEVEL(LPCSTR lpFmt, ...);
LONG VRTExceptionFilter(LPEXCEPTION_POINTERS ExceptionInfo);
VOID VRTInit();
VOID VRTDeInit();
LPVOID VRTAlloc(DWORD dwSize);
LPVOID VRTReAlloc(LPVOID lpOldMem, DWORD dwSize);
VOID VRTFree(LPVOID lpMem);
LPVOID VRTCopyMemory(LPVOID dst, LPCVOID src, DWORD count);
LPVOID VRTFillMemory(LPVOID dst, DWORD count, BYTE fill);
VOID VRTCommonExecutionBegin();
VOID VRTCommonExecutionEnd();

#endif //__VANQUISH_UTILS__H__
