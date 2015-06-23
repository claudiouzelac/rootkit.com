/******************************************************************************\

	Vanquish DLL DllUtils - Copyright (c)2003-2005 XShadow, All rights reserved.

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
	INITIAL_REPLACE_API(CreateProcessA, KERNEL32); \
	INITIAL_REPLACE_API(CreateProcessW, KERNEL32); \
	INITIAL_REPLACE_API(LoadLibraryExW, KERNEL32); \
	INITIAL_REPLACE_API(FreeLibrary, KERNEL32);/* \
	INITIAL_REPLACE_API(GetModuleFileNameA, KERNEL32); \
	INITIAL_REPLACE_API(GetModuleFileNameW, KERNEL32);
	*/

#define LOAD_ADVAPI32__DLLUTILS \
	INITIAL_REPLACE_API(CreateProcessAsUserA, ADVAPI32); \
	INITIAL_REPLACE_API(CreateProcessAsUserW, ADVAPI32);

#define LOAD_USER32__DLLUTILS
#define LOAD_GINADLL__DLLUTILS

//default UNload handlers
#define UNLOAD_KERNEL32__DLLUTILS \
	FINAL_RESTORE_API(FreeLibrary); \
	FINAL_RESTORE_API(LoadLibraryExW); \
	FINAL_RESTORE_API(CreateProcessA); \
	FINAL_RESTORE_API(CreateProcessW);/* \
	FINAL_RESTORE_API(GetModuleFileNameA); \
	FINAL_RESTORE_API(GetModuleFileNameW);
	*/

#define UNLOAD_ADVAPI32__DLLUTILS \
	FINAL_RESTORE_API(CreateProcessAsUserA); \
	FINAL_RESTORE_API(CreateProcessAsUserW);

#define UNLOAD_USER32__DLLUTILS
#define UNLOAD_GINADLL__DLLUTILS

//declaring handlers
#define DECLARE_DLLUTILS \
	DECLARE_NEWENTRY(CreateProcessA); \
	DECLARE_NEWENTRY(CreateProcessW); \
	DECLARE_NEWENTRY(CreateProcessAsUserA); \
	DECLARE_NEWENTRY(CreateProcessAsUserW); \
	DECLARE_NEWENTRY(LoadLibraryExW); \
	DECLARE_NEWENTRY(FreeLibrary);/* \
	DECLARE_NEWENTRY(GetModuleFileNameA); \
	DECLARE_NEWENTRY(GetModuleFileNameW);
	*/

//extern declare
#define EXTERN_DLLUTILS \
	EXTERN_NEWENTRY(CreateProcessA); \
	EXTERN_NEWENTRY(CreateProcessW); \
	EXTERN_NEWENTRY(CreateProcessAsUserA); \
	EXTERN_NEWENTRY(CreateProcessAsUserW); \
	EXTERN_NEWENTRY(LoadLibraryExW); \
	EXTERN_NEWENTRY(FreeLibrary);/* \
	EXTERN_NEWENTRY(GetModuleFileNameA); \
	EXTERN_NEWENTRY(GetModuleFileNameW);
	*/

//Vanquish NEW API declarations follows
//Declarations are in conformance with the real Windows API header files
//Difference between this and real api prototypes:
//    1) prefixed V to the name
//    2) replaced WINAPI with NEWAPI

//Original header definitions: (winbase.h)
//PS: CreateProcessA DOES NOT call CreateProcessW
//    LoadLibrary calls LoadLibraryEx

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
    );

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
    );

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
    );

NEWAPI
HMODULE
WINAPI
VLoadLibraryExW(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    );

NEWAPI
BOOL
WINAPI
VFreeLibrary(
    HMODULE hLibModule
    );
/*
NEWAPI
DWORD
WINAPI
VGetModuleFileNameA(
	HMODULE hModule,
	LPSTR lpFilename,
	DWORD nSize
	);

NEWAPI
DWORD
WINAPI
VGetModuleFileNameW(
	HMODULE hModule,
	LPWSTR lpFilename,
	DWORD nSize
	);
*/
#endif//__VANQUISH_DLL_DLLUTILS__H__