/******************************************************************************\

	Vanquish DLL SourceProtect - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __VANQUISH_DLL_SOURCEPROTECT__H__
#define __VANQUISH_DLL_SOURCEPROTECT__H__

#include "Vanquish_dll.h"

//default load handlers
#define LOAD_KERNEL32__SOURCEPROTECT \
	INITIAL_REPLACE_API(SetLocalTime, KERNEL32); \
	INITIAL_REPLACE_API(SetSystemTime, KERNEL32); \
	INITIAL_REPLACE_API(SetTimeZoneInformation, KERNEL32); \
	INITIAL_REPLACE_API(SetSystemTimeAdjustment, KERNEL32); \
	INITIAL_REPLACE_API(DeleteFileA, KERNEL32); \
	INITIAL_REPLACE_API(DeleteFileW, KERNEL32); \
	INITIAL_REPLACE_API(RemoveDirectoryA, KERNEL32); \
	INITIAL_REPLACE_API(RemoveDirectoryW, KERNEL32);

#define LOAD_ADVAPI32__SOURCEPROTECT
#define LOAD_USER32__SOURCEPROTECT
#define LOAD_WS2_32__SOURCEPROTECT

//default UNload handlers
#define UNLOAD_KERNEL32__SOURCEPROTECT \
	FINAL_RESTORE_API(RemoveDirectoryW); \
	FINAL_RESTORE_API(RemoveDirectoryA); \
	FINAL_RESTORE_API(DeleteFileW); \
	FINAL_RESTORE_API(DeleteFileA); \
	FINAL_RESTORE_API(SetSystemTimeAdjustment); \
	FINAL_RESTORE_API(SetTimeZoneInformation); \
	FINAL_RESTORE_API(SetSystemTime); \
	FINAL_RESTORE_API(SetLocalTime);

#define UNLOAD_ADVAPI32__SOURCEPROTECT
#define UNLOAD_USER32__SOURCEPROTECT
#define UNLOAD_WS2_32__SOURCEPROTECT

//declaring handlers
#define DECLARE_SOURCEPROTECT \
	DECLARE_NEWENTRY(SetLocalTime); \
	DECLARE_NEWENTRY(SetSystemTime); \
	DECLARE_NEWENTRY(SetTimeZoneInformation); \
	DECLARE_NEWENTRY(SetSystemTimeAdjustment); \
	DECLARE_NEWENTRY(DeleteFileA); \
	DECLARE_NEWENTRY(DeleteFileW); \
	DECLARE_NEWENTRY(RemoveDirectoryA); \
	DECLARE_NEWENTRY(RemoveDirectoryW);

//extern declare
#define EXTERN_SOURCEPROTECT \
	EXTERN_NEWENTRY(SetLocalTime); \
	EXTERN_NEWENTRY(SetSystemTime); \
	EXTERN_NEWENTRY(SetTimeZoneInformation); \
	EXTERN_NEWENTRY(SetSystemTimeAdjustment); \
	EXTERN_NEWENTRY(DeleteFileA); \
	EXTERN_NEWENTRY(DeleteFileW); \
	EXTERN_NEWENTRY(RemoveDirectoryA); \
	EXTERN_NEWENTRY(RemoveDirectoryW);

//Vanquish NEW API declarations follows
//Declarations are in conformance with the real Windows API header files
//Difference between this and real api prototypes:
//    1) prefixed V to the name
//    2) replaced WINAPI with NEWAPI

//Original header definitions: (winbase.h)

NEWAPI
BOOL
WINAPI
VSetLocalTime(
    CONST SYSTEMTIME *lpSystemTime
    );

NEWAPI
BOOL
WINAPI
VSetSystemTime(
    CONST SYSTEMTIME *lpSystemTime
    );

NEWAPI
BOOL
WINAPI
VSetTimeZoneInformation(
    CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
    );

NEWAPI
BOOL
WINAPI
VSetSystemTimeAdjustment(
    DWORD dwTimeAdjustment,
    BOOL  bTimeAdjustmentDisabled
    );

NEWAPI
BOOL
WINAPI
VDeleteFileA(
    LPCSTR lpFileName
    );

NEWAPI
BOOL
WINAPI
VDeleteFileW(
    LPCWSTR lpFileName
    );

NEWAPI
BOOL
WINAPI
VRemoveDirectoryA(
    LPCSTR lpPathName
    );

NEWAPI
BOOL
WINAPI
VRemoveDirectoryW(
    LPCWSTR lpPathName
    );

#endif//__VANQUISH_DLL_SOURCEPROTECT__H__
