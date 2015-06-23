#include <winsock2.h>
#include <windows.h>
#include "ntdll.h"
#include "misc.h"
#pragma comment(lib,"ntdll")

#include "safe.h"

// alloc buffer ...
LPVOID misc_AllocBuffer(DWORD dwSize)
{
	LPVOID lpBuffer=NULL;
	DWORD dwLen=dwSize;

	if (NT_SUCCESS(NtAllocateVirtualMemory(NtCurrentProcess(),&lpBuffer,0,&dwLen,MEM_COMMIT|MEM_TOP_DOWN,PAGE_READWRITE)))
		return lpBuffer;

	return NULL;
}

// free buffer ...
void misc_FreeBuffer(LPVOID* lpBuffer)
{
	DWORD dwFree=0;

	NtFreeVirtualMemory(NtCurrentProcess(),lpBuffer,&dwFree,MEM_RELEASE);
}

// get "debug" privilege
void misc_GetDebugPriv(void)
{
	HANDLE hToken;
	LUID DebugNameValue;
	TOKEN_PRIVILEGES Privileges;
	DWORD dwRet;

	OpenProcessToken(GetCurrentProcess(),
			 TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken);
	LookupPrivilegeValue(NULL,"SeDebugPrivilege",&DebugNameValue);
	Privileges.PrivilegeCount=1;
	Privileges.Privileges[0].Luid=DebugNameValue;
	Privileges.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken,FALSE,&Privileges,sizeof(Privileges),
			      NULL,&dwRet);
	CloseHandle(hToken);
}

// get current process id
DWORD misc_NtGetCurrentProcessId(void)
{
	PROCESS_BASIC_INFORMATION pbi;

	if (NT_SUCCESS(NtQueryInformationProcess(NtCurrentProcess(),ProcessBasicInformation,&pbi,sizeof(pbi),NULL)))
		return pbi.uUniqueProcessId;

	return 0;
}

// get current thread id
DWORD misc_NtGetCurrentThreadId(void)
{
	THREAD_BASIC_INFORMATION tbi;

	if (NT_SUCCESS(NtQueryInformationThread(NtCurrentThread(),ThreadBasicInformation,&tbi,sizeof(tbi),NULL)))
		return (DWORD)tbi.ClientId.UniqueThread;

	return 0;
}

// get pid id by thread handle
DWORD misc_GetPidByThread(HANDLE hThread)
{
	THREAD_BASIC_INFORMATION tbi;
	DWORD dwReturnLen;
	
	if (!NT_SUCCESS(NtQueryInformationThread(hThread,ThreadBasicInformation,&tbi,sizeof(tbi),&dwReturnLen)))
		return 0;

	return (DWORD)tbi.ClientId.UniqueProcess;
}


// append something to a file
int misc_WriteDataToFile(LPSTR lpFile,LPVOID pvBuffer,ULONG ulBufferLength)
{
	HANDLE hFile;
	DWORD dwBytesWritten;
	DWORD dwFileSize,dwFileSizeHigh=0;
	OVERLAPPED olOverlapped;

	hFile=CreateFile(lpFile,GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,
		NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

	if (!hFile)
		return 0;

	// set filepointer to end of file
	dwFileSize=GetFileSize(hFile,&dwFileSizeHigh);
	olOverlapped.hEvent=NULL;
	olOverlapped.Offset=dwFileSize;
	olOverlapped.OffsetHigh=dwFileSizeHigh;

	// write to file
	if (!WriteFile(hFile,pvBuffer,ulBufferLength,&dwBytesWritten,&olOverlapped))
	{
		CloseHandle(hFile);
		return 0;
	}
	
	CloseHandle(hFile);
	return 1;
}


// get process list
LPVOID misc_GetProcessListBuffer(void)
{
	LPVOID lpBuffer=NULL;
	DWORD dwLen=1024;
	NTSTATUS ntStatus;

	// create start buffer
	lpBuffer=misc_AllocBuffer(dwLen);
	if (!lpBuffer)
		return NULL;
	
	// get the full process & thread list
	while ((ntStatus=SafeNtQuerySystemInformation(SystemProcessInformation,lpBuffer,dwLen,NULL))==STATUS_INFO_LENGTH_MISMATCH)
	{
		misc_FreeBuffer(&lpBuffer);
		dwLen+=1024;
		lpBuffer=NULL;

		lpBuffer=misc_AllocBuffer(dwLen);
		if (!lpBuffer)
			return NULL;
	}

	// function failed ?
	if (!NT_SUCCESS(ntStatus))
	{
		misc_FreeBuffer(&lpBuffer);
		return NULL;
	}

	return lpBuffer;
}

BOOL misc_GetOSVersion(DWORD* pdwMajorVersion,DWORD* pdwMinorVersion)
{
	OSVERSIONINFO ovi;
	
	ovi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);

	if (GetVersionEx(&ovi))
	{
		if (pdwMajorVersion) *pdwMajorVersion=ovi.dwMajorVersion;
		if (pdwMinorVersion) *pdwMinorVersion=ovi.dwMinorVersion;
		return TRUE;
	}
	else
	{
		if (pdwMajorVersion) *pdwMajorVersion=(DWORD)(LOBYTE(LOWORD(GetVersion())));
		if (pdwMinorVersion) *pdwMinorVersion=(DWORD)(HIBYTE(LOWORD(GetVersion())));
		return TRUE;
	}

	return FALSE;
}


// set service to be enabled in safeboot
int misc_SetServiceSafeBoot(LPSTR lpSvcName)
{
	LPSTR lpFullKey;
	HKEY hkSafe;
	CHAR* cSafeKeys[]={"Minimal","Network",NULL};
	int i=0;

	while (cSafeKeys[i])
	{
		if (!(lpFullKey=(LPSTR)malloc(strlen(SAFEBOOT_KEY)+strlen(cSafeKeys[i])+
			strlen("\\")+strlen(lpSvcName)+1)))
			return 0;
	
		strcpy(lpFullKey,SAFEBOOT_KEY);
		strcat(lpFullKey,cSafeKeys[i]);
		strcat(lpFullKey,"\\");
		strcat(lpFullKey,lpSvcName);

		RegCreateKey(HKEY_LOCAL_MACHINE,lpFullKey,&hkSafe);
		RegCloseKey(hkSafe);
		free(lpFullKey);
		i++;
	}

	return 1;
}

// installs current process as service
int misc_InstallService(LPSTR lpSvcName,LPSTR lpDispName,LPSTR lpDesc)
{
	LPSTR lpFileName,lpFullDesc;
	SC_HANDLE schSCManager,schService;
	HKEY hkDesc;
	BOOL bCreate;

	// allocate filename string
	if (!(lpFileName=(LPSTR)malloc(MAX_PATH+1)))
		return 0;

	// get current process path+filename
	if (!GetModuleFileName(NULL,lpFileName,MAX_PATH))
	{
		free(lpFileName);
		return 0;
	}

	// get handle to SCM
	if (!(schSCManager=OpenSCManager(0,0,SC_MANAGER_CREATE_SERVICE)))
	{
		free(lpFileName);
		return 0;
	}
	
	// Create service
	schService=CreateService(schSCManager,lpSvcName,lpDispName,SERVICE_START,
		SERVICE_WIN32_OWN_PROCESS,SERVICE_AUTO_START,SERVICE_ERROR_IGNORE,
		lpFileName,NULL,NULL,NULL,NULL,NULL);

	// if already exist, open it
	if (GetLastError()==ERROR_SERVICE_EXISTS)
		schService=OpenService(schSCManager,lpSvcName,SERVICE_START);

	// we need a valid handle
	if (!schService)
	{
		CloseServiceHandle(schSCManager);
		free(lpFileName);
		return 0;
	}
	
	// allocate full service key string
	if (!(lpFullDesc=(LPSTR)malloc(strlen(SVCDESC_KEY)+strlen(lpSvcName)+1)))
	{
		CloseServiceHandle(schSCManager);
		CloseServiceHandle(schService);
		free(lpFileName);
		return 0;
	}

	strcpy(lpFullDesc,SVCDESC_KEY);
	strcat(lpFullDesc,lpSvcName);

	// create description/imagepath key
	if (RegCreateKey(HKEY_LOCAL_MACHINE,lpFullDesc,&hkDesc)==ERROR_SUCCESS)
	{
		RegSetValueEx(hkDesc,"Description",0,REG_EXPAND_SZ,(CONST BYTE*)lpDesc,strlen(lpDesc)+1);
		RegSetValueEx(hkDesc,"ImagePath",0,REG_EXPAND_SZ,(CONST BYTE*)lpFileName,strlen(lpFileName)+1);
		RegCloseKey(hkDesc);
	}
	
	free(lpFullDesc);
	free(lpFileName);

	// enable safeboot
	misc_SetServiceSafeBoot(lpSvcName);

	// start service
	bCreate=StartService(schService,0,NULL);
	if (bCreate||GetLastError()==ERROR_SERVICE_ALREADY_RUNNING)
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return 1;
	}
		
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	
	return 0;
}

unsigned short misc_htons(unsigned short hostshort) 
{
  return ((hostshort>>8)&0xff)|(hostshort<<8);
}