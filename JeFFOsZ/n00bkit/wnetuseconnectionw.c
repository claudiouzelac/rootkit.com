#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "ntdll.h"

#include "wnetuseconnectionw.h"
#include "lsalogonuser.h"

// mpr.WNetUseConnectionW
DWORD WINAPI NewWNetUseConnectionW(
  HWND				hwndOwner,
  LPNETRESOURCEW	lpwNetResource,
  LPCWSTR			lpwUserID,
  LPCWSTR			lpwPassword,
  DWORD				dwFlags,
  LPWSTR			lpwAccessName,
  LPDWORD			lpBufferSize,
  LPDWORD			lpResult
)
{
	DWORD dwResult;
	
	dwResult=OldWNetUseConnectionW(hwndOwner,lpwNetResource,lpwUserID,lpwPassword,dwFlags,lpwAccessName,lpBufferSize,lpResult);
	if (dwResult==NO_ERROR)
	{
		// hmmm, lpuser/pass is NULL ???
		if (!lpwUserID)
			OutputDebugStringW(lpwUserID);
	}

	return dwResult;
}

