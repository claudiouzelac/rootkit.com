/******************************************************************************\

	Vanquish DLL SourceProtect - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "vSourceProtect.h"
#include "..\Utils.h"

//string constants
LPCSTR SRCPROTTITLEA = "System Protection";
LPCSTR NOTIMEA = "You cannot modify system time! Instead, your attempt has been logged :)";
LPCSTR NODELA = "You cannot delete protected files/folders! Instead, your attempt has been logged :)";
LPCSTR NODELREPORTA = "WARNING! Tried to DELETE protected files/folders:\r\n%s\r\n"; 
LPCSTR NODELREPORTW = "WARNING! Tried to DELETE protected files/folders:\r\n%S\r\n"; 
const UINT MSGBOXSTYLE = MB_OK | MB_ICONWARNING | MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST;

//normal path
LPCSTR PROT_STRA = "D:\\MY";
LPCWSTR PROT_STRW = L"D:\\MY";
const DWORD PROT_STRCN = 5;

//old unc style path
LPCSTR PROT_STR_UNC1A = "\\\\?\\D:\\MY";
LPCWSTR PROT_STR_UNC1W = L"\\\\?\\D:\\MY";
const DWORD PROT_STR_UNC1CN = 9;

//new unc style path
LPCSTR PROT_STR_UNC2A = "\\\\.\\D:\\MY";
LPCWSTR PROT_STR_UNC2W = L"\\\\.\\D:\\MY";
const DWORD PROT_STR_UNC2CN = 9;

DECLARE_SOURCEPROTECT

void NoTimeChgNotify()
{
	MessageBoxA(NULL, NOTIMEA, SRCPROTTITLEA, MSGBOXSTYLE);
	VRTWriteLog(FALSE, 0, NULL, NOTIMEA);
}

void NoDeleteA(LPSTR what)
{
	MessageBox(NULL, NODELA, SRCPROTTITLEA, MSGBOXSTYLE);
	VRTWriteLog(FALSE, 0, NULL, NODELREPORTA, what);
}

void NoDeleteW(LPWSTR what)
{
	MessageBox(NULL, NODELA, SRCPROTTITLEA, MSGBOXSTYLE);
	VRTWriteLog(FALSE, 0, NULL, NODELREPORTW, what);
}

BOOL IsProtectedA(LPCSTR what)
{
	DWORD nch;

	//nch = STR_NUMCHA(PROT_STRA); //number of chars in string
	nch = PROT_STRCN;
	if (IsBadReadPtr(what, STRB_SIZEA(nch))) return FALSE;
	if (CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, what, nch, PROT_STRA, nch) == CSTR_EQUAL) return TRUE;

	//nch = STR_NUMCHA(PROT_STR_UNC1A); //number of chars in string
	nch = PROT_STR_UNC1CN;
	if (IsBadReadPtr(what, STRB_SIZEA(nch))) return FALSE;
	if (CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, what, nch, PROT_STR_UNC1A, nch) == 0) return TRUE;

	//nch = STR_NUMCHA(PROT_STR_UNC2A); //number of chars in string
	nch = PROT_STR_UNC2CN;
	if (IsBadReadPtr(what, STRB_SIZEA(nch))) return FALSE;
	if (CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, what, nch, PROT_STR_UNC2A, nch) == 0) return TRUE;

	return FALSE;
}

BOOL IsProtectedW(LPCWSTR what)
{
	DWORD nch;

	//nch = STR_NUMCHW(PROT_STRW); //number of chars in string
	nch = PROT_STRCN;
	if (IsBadReadPtr(what, STRB_SIZEW(nch))) return FALSE;
	if (CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, what, nch, PROT_STRW, nch) == 0) return TRUE;

	//nch = STR_NUMCHW(PROT_STR_UNC1W); //number of chars in string
	nch = PROT_STR_UNC1CN;
	if (IsBadReadPtr(what, STRB_SIZEW(nch))) return FALSE;
	if (CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, what, nch, PROT_STR_UNC1W, nch) == 0) return TRUE;

	//nch = STR_NUMCHW(PROT_STR_UNC2W); //number of chars in string
	nch = PROT_STR_UNC2CN;
	if (IsBadReadPtr(what, STRB_SIZEW(nch))) return FALSE;
	if (CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, what, nch, PROT_STR_UNC2W, nch) == 0) return TRUE;

	return FALSE;
}

NEWAPI
BOOL
WINAPI
VSetLocalTime(
    CONST SYSTEMTIME *lpSystemTime
    )
{
	NoTimeChgNotify();
	return FALSE;
}

NEWAPI
BOOL
WINAPI
VSetSystemTime(
    CONST SYSTEMTIME *lpSystemTime
    )
{
	NoTimeChgNotify();
	return FALSE;
}

NEWAPI
BOOL
WINAPI
VSetTimeZoneInformation(
    CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
    )
{
	NoTimeChgNotify();
	return FALSE;
}

NEWAPI
BOOL
WINAPI
VSetSystemTimeAdjustment(
    DWORD dwTimeAdjustment,
    BOOL  bTimeAdjustmentDisabled
    )
{
	NoTimeChgNotify();
	return FALSE;
}

NEWAPI
BOOL
WINAPI
VDeleteFileA(
    LPCSTR lpFileName
    )
{
	BOOL retValue = FALSE;
	if (IsProtectedA(lpFileName))
		NoDeleteA((LPSTR)lpFileName);
	else
	{
		RESTORE_API(DeleteFileA);
		OLDCALL(DeleteFileA, 1);
		REPLACE_API(DeleteFileA);
	}
	return retValue;
}

NEWAPI
BOOL
WINAPI
VDeleteFileW(
    LPCWSTR lpFileName
    )
{
	BOOL retValue = FALSE;
	if (IsProtectedW(lpFileName))
		NoDeleteW((LPWSTR)lpFileName);
	else
	{
		RESTORE_API(DeleteFileW);
		OLDCALL(DeleteFileW, 1);
		REPLACE_API(DeleteFileW);
	}
	return retValue;
}

NEWAPI
BOOL
WINAPI
VRemoveDirectoryA(
    LPCSTR lpPathName
    )
{
	BOOL retValue = FALSE;
	if (IsProtectedA(lpPathName))
		NoDeleteA((LPSTR)lpPathName);
	else
	{
		RESTORE_API(RemoveDirectoryA);
		OLDCALL(RemoveDirectoryA, 1);
		REPLACE_API(RemoveDirectoryA);
	}
	return retValue;
}

NEWAPI
BOOL
WINAPI
VRemoveDirectoryW(
    LPCWSTR lpPathName
    )
{
	BOOL retValue = FALSE;
	if (IsProtectedW(lpPathName))
		NoDeleteW((LPWSTR)lpPathName);
	else
	{
		RESTORE_API(RemoveDirectoryW);
		OLDCALL(RemoveDirectoryW, 1);
		REPLACE_API(RemoveDirectoryW);
	}
	return retValue;
}
