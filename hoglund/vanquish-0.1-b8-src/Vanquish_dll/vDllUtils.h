/******************************************************************************\

	Vanquish DLL DllUtils - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __VANQUISH_DLL_DLLUTILS__H__
#define __VANQUISH_DLL_DLLUTILS__H__

#include "Vanquish_dll.h"

//default load handlers
#define LOAD_KERNEL32__DLLUTILS \
	INITIAL_REPLACE_API(CreateProcessW, KERNEL32); \
	INITIAL_REPLACE_API(FreeLibrary, KERNEL32);

#define LOAD_ADVAPI32__DLLUTILS \
	INITIAL_REPLACE_API(CreateProcessAsUserW, ADVAPI32);

#define LOAD_USER32__DLLUTILS
#define LOAD_WS2_32__DLLUTILS

//default UNload handlers
#define UNLOAD_KERNEL32__DLLUTILS \
	FINAL_RESTORE_API(FreeLibrary); \
	FINAL_RESTORE_API(CreateProcessW);

#define UNLOAD_ADVAPI32__DLLUTILS \
	FINAL_RESTORE_API(CreateProcessAsUserW);

#define UNLOAD_USER32__DLLUTILS
#define UNLOAD_WS2_32__DLLUTILS

//declaring handlers
#define DECLARE_DLLUTILS \
	DECLARE_NEWENTRY(CreateProcessW); \
	DECLARE_NEWENTRY(CreateProcessAsUserW); \
	DECLARE_NEWENTRY(FreeLibrary);

//extern declare
#define EXTERN_DLLUTILS \
	EXTERN_NEWENTRY(CreateProcessW); \
	EXTERN_NEWENTRY(CreateProcessAsUserW); \
	EXTERN_NEWENTRY(FreeLibrary);

//Vanquish NEW API declarations follows
//Declarations are in conformance with the real Windows API header files
//Difference between this and real api prototypes:
//    1) prefixed V to the name
//    2) replaced WINAPI with NEWAPI

//Original header definitions: (winbase.h)
//PS: CreateProcessA calls CreateProcessW

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
    );

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
    );

NEWAPI
BOOL
WINAPI
VFreeLibrary(
    HMODULE hLibModule
    );

#endif//__VANQUISH_DLL_DLLUTILS__H__