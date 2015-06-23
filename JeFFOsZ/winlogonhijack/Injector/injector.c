////////////////////////////////////////////////////////////////////
//
// This file contains the functions that are used for injecting data 
// and loading a dll in a remote process. Written by JeFFOsZ
//
////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include "injector.h"

BOOL GetDebugPriv(PTOKEN_PRIVILEGES ptkpPrev)
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;
	BOOL bRet;
	ULONG ulRet;

	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken))
		return FALSE;

	bRet=LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &sedebugnameValue);
	if (!bRet)
	{
		CloseHandle(hToken);
		return bRet;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	bRet=AdjustTokenPrivileges(hToken,FALSE,&tkp,sizeof(tkp),ptkpPrev,&ulRet);

	CloseHandle(hToken);
 
	return bRet;
}

BOOL RestorePrivileges(TOKEN_PRIVILEGES tkp)
{
	HANDLE hToken;
	BOOL bRet;
	
	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken))
		return FALSE;
	
	bRet=AdjustTokenPrivileges(hToken,FALSE,&tkp,sizeof(tkp),NULL,NULL);
	
	CloseHandle(hToken);

	return bRet;
}

BOOL IsWinNt(PDWORD pdwMajorVersion,PDWORD pdwMinorVersion)
{
	OSVERSIONINFO ovi;
	DWORD lRet;
	
	ovi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);

	lRet=GetVersionEx(&ovi);
	if (!lRet)
		return FALSE;

	if (ovi.dwPlatformId!=VER_PLATFORM_WIN32_NT)
		return FALSE;
	
	*pdwMajorVersion=ovi.dwMajorVersion;
	*pdwMinorVersion=ovi.dwMinorVersion;

	return TRUE;
}

LPVOID InjectData(HANDLE hProcess,LPVOID lpData,ULONG ulFuncLen)
{
	LPVOID lpAddress=NULL;
	DWORD dwOldProtect;
	DWORD BytesWritten=0;
	
	// Allocate memory for lpData int the remote process
	lpAddress=VirtualAllocEx(hProcess,NULL,ulFuncLen,MEM_COMMIT|MEM_TOP_DOWN,PAGE_EXECUTE_READWRITE);
	if (lpAddress)
	{
		// Change the protection for the allocated memory
		if (VirtualProtectEx(hProcess,lpAddress,ulFuncLen,PAGE_EXECUTE_READWRITE,&dwOldProtect))
		{
			// ...
			FlushInstructionCache(hProcess,lpAddress,ulFuncLen);
			// Write lpData into the remote process
			if (WriteProcessMemory(hProcess,lpAddress,lpData,ulFuncLen,&BytesWritten))
			{
				// Restore old protection :)
				VirtualProtectEx(hProcess,lpAddress,ulFuncLen,dwOldProtect,NULL);
				// Return remote address for lpData
				return lpAddress;
			}
			// Restore old protection :)
			VirtualProtectEx(hProcess,lpAddress,ulFuncLen,dwOldProtect,NULL);
		}
	}
	return 0;
}

// !! we use LoadDllInProcessEx instead !!!

// Loads a dll in a process, uses kernel32.LoadLibraryA
// NOTE: we should use NTDLL.LdrLoadDll instead !
/*BOOL LoadDllInProcess(DWORD dwPid,char* DllPathName)
{
	HANDLE hProcess,hThread;
	ULONG ulSize;
	SECURITY_ATTRIBUTES saSecAttr;
	DWORD dwActual;
	LPVOID lpAddress=NULL;
	HMODULE hmKernel32;
	FARPROC fpLoadLibraryA;
	
	// Open process
	if (hProcess=OpenProcess(PROCESS_ALL_ACCESS,0,dwPid)) 
	{
		ulSize=strlen(DllPathName)+1; // size to write
		if (lpAddress=InjectData(hProcess,DllPathName,ulSize)) 
		// Inject the LoadLibraryA parameter in the process
		{
			// Set security attributes
			saSecAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
			saSecAttr.lpSecurityDescriptor = NULL;
			saSecAttr.bInheritHandle = TRUE;
			// Get address from kernel32
			hmKernel32=GetModuleHandle("kernel32");
			// Get address from kernel32.LoadLibraryA
			fpLoadLibraryA=GetProcAddress(hmKernel32,"LoadLibraryA");
			// Execute LoadLibraryA in the process
			if (hThread=CreateRemoteThread(hProcess,&saSecAttr,0,(LPTHREAD_START_ROUTINE)fpLoadLibraryA,lpAddress,0,&dwActual))
			{
				// Wait till thread responds
				WaitForSingleObject(hThread, INFINITE); 
				// Get result for unhooking ??
				// Free memory allocated by InjectData
				VirtualFreeEx(hProcess, lpAddress, ulSize, MEM_RELEASE); 
				// close remotethreead handle
				CloseHandle(hThread);
				// close process handle
				CloseHandle(hProcess);
				return TRUE;
			}
		}
		CloseHandle(hProcess);
	}
	return FALSE;
}*/

NTSTATUS __stdcall RemoteThread(RemoteProcessData *rpd)
{
	
	NTSTATUS rc=
		(NTSTATUS)rpd->pLdrLoadDll(
					rpd->PathToFile,
					rpd->Flags,
					&rpd->ModuleFileName,
					&rpd->ModuleHandle
					);
	return rc;
}
void __stdcall EndRemoteThread(void) { }

// Loads a dll in a process (uses ntdll.LdrLoadData)
DWORD LoadDllInProcessEx(DWORD dwPid,char* DllPathName)
{
	HANDLE hProcess,hThread;
	RemoteProcessData rpd;
	PWSTR pwModuleFileName;
	HANDLE hModule=NULL;
	UNICODE_STRING usModule;
	LPVOID lpParameters,lpThread;
	SECURITY_ATTRIBUTES saSecAttr;
	DWORD dwActual,dwResult=0,rc;

	hProcess=OpenProcess(PROCESS_ALL_ACCESS,0,dwPid);
	if (hProcess==NULL)
		goto cleanup;

	rpd.pLdrLoadDll=(LDRLOADDLL)GetProcAddress(GetModuleHandle("ntdll"),"LdrLoadDll");
	if (!rpd.pLdrLoadDll)
		goto cleanup;
		
	rpd.Flags=0;
	rpd.PathToFile=NULL;
	rpd.ModuleHandle=NULL;
	
	pwModuleFileName=(PWSTR)malloc((strlen(DllPathName)*2)+1);
	if (!pwModuleFileName)
		goto cleanup;
		
	MultiByteToWideChar(CP_ACP,0,DllPathName,strlen(DllPathName),pwModuleFileName,(strlen(DllPathName)*2)+1);
	usModule.Buffer=(PWSTR)InjectData(hProcess,pwModuleFileName,(strlen(DllPathName)*2)+1);

	free(pwModuleFileName);

	if (!usModule.Buffer)
		goto cleanup;
	
	usModule.Length=(strlen(DllPathName)*2)+1;
	usModule.MaximumLength=(strlen(DllPathName)*2)+1;

	memcpy(&rpd.ModuleFileName,&usModule,sizeof(UNICODE_STRING));

	lpParameters=InjectData(hProcess,&rpd,sizeof(RemoteProcessData)+4096);
	if (!lpParameters)
		goto cleanup;

	lpThread=InjectData(hProcess,&RemoteThread,(PBYTE)EndRemoteThread-(PBYTE)RemoteThread+4096);
	if (!lpThread)
		goto cleanup;

	// Set security attributes
	saSecAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
	saSecAttr.lpSecurityDescriptor = NULL;
	saSecAttr.bInheritHandle = TRUE;

	hThread=CreateRemoteThread(hProcess,&saSecAttr,0,(LPTHREAD_START_ROUTINE)lpThread,lpParameters,0,&dwActual);
	if (hThread==NULL)
		goto cleanup;
	
	rc=WaitForSingleObject(hThread, INFINITE); 

	switch (rc) 
	{
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			break;
		case WAIT_OBJECT_0:
			if (ReadProcessMemory(hProcess,lpParameters,&rpd,sizeof(RemoteProcessData),&dwActual))
				dwResult=(DWORD)rpd.ModuleHandle;
			break;
		default:
			break;
	}

cleanup:
	if (rpd.ModuleFileName.Buffer!=NULL)
		VirtualFreeEx(hProcess,rpd.ModuleFileName.Buffer,0,MEM_RELEASE);

	if (lpParameters!=NULL)
		VirtualFreeEx(hProcess,lpParameters,0,MEM_RELEASE);

	if (lpThread!=NULL)
		VirtualFreeEx(hProcess,lpThread,0,MEM_RELEASE);

	if (hThread) CloseHandle(hThread);
	if (hProcess) CloseHandle(hProcess);

	return dwResult;
}