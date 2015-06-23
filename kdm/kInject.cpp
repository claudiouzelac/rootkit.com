// inject.cpp : Designed to inject a DLL into another process space
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <windows.h>
#include <tlhelp32.h>   // CreateToolhelp32Snapshot, etc...

#define DIE 1
#define MAX_SIZE 4096 /* For runtime injection */

/******************************************************************/
/*/////////////////////////////////////////////////////////////////
//////////////Process Information Functions//////////////////////////
/////////////////////////////////////////////////////////////////*/
int GetPidByName(char* nom)
{
    HINSTANCE   hLib;
    PROCESSENTRY32 PEntry;
    HANDLE hTool32;
    //Functions types :
    HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD,DWORD);
    BOOL (WINAPI *pProcess32First)(HANDLE,LPPROCESSENTRY32);
    BOOL (WINAPI *pProcess32Next)(HANDLE,LPPROCESSENTRY32);
    
    hLib = LoadLibrary("Kernel32.DLL");
    
    //Functions addresses :
    pCreateToolhelp32Snapshot=(HANDLE(WINAPI *)(DWORD,DWORD)) GetProcAddress( hLib,"CreateToolhelp32Snapshot");
    pProcess32First=(BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32))GetProcAddress( hLib, "Process32First" );
    pProcess32Next=(BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32))GetProcAddress( hLib, "Process32Next" );
    
    PEntry.dwSize = sizeof(PROCESSENTRY32);     //Set Size of structure before use
    hTool32 = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0); //Create SnapShot
    
    pProcess32First(hTool32, &PEntry);    //Get first process
    if(!strcmp(PEntry.szExeFile,nom)) return PEntry.th32ProcessID; //If correct, return Pid.

    while( pProcess32Next(hTool32,&PEntry) )
        if(!strcmp(PEntry.szExeFile,nom)) return PEntry.th32ProcessID; //If correct, return Pid.
    FreeLibrary(hLib);
    
    return 0;
}
/******************************************************************/
//usage :
/*
  0				1					2			3					4					5
inject.exe <process_name/path> <dll_path> --create / --runtime	--resolve			--force
inject.exe "C:\Program Files\Internet Explorer\IEXPLORE.EXE" C:\IAThijackDLL.dll --create
*/
char usage[]= "kInject.exe [process path/Pid] [dll path] [--create / --runtime] [--resolve] [--force]\n"
              "--create     : program will create the process before injecting\n"  
              "--runtime    : inject already existing process\n"
              "--resolve    : get process id from executable name\n"
              "--force      : load SeDebugPrivilege to break into target process\n";

// Error handling routine
void DispError(char *message, int die)
{
	printf("\n%s\n", message);
	getchar();
	if(die) ExitProcess(0);
	return;
}

/*/////////////////////////////////////////////////////////////////
//////////////INJECTION CREATE REMOTETHREAD////////////////////////
/////////////////////////////////////////////////////////////////*/
//Injects DLLFile into a process identified by its handle (hModule)
int InjectDll(HANDLE hModule, char *DLLFile)
{
	//char DLLFile[]="C:\\cInjectedDll.dll";
	int LenWrite = strlen(DLLFile) + 1;
	char * AllocMem = (char *) VirtualAllocEx(hModule,NULL, LenWrite, MEM_COMMIT,PAGE_READWRITE); //allocation pour WriteProcessMemory
	WriteProcessMemory(hModule, AllocMem , DLLFile, LenWrite, NULL);
	//PTHREAD_START_ROUTINE Injector = (PTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
   LPTHREAD_START_ROUTINE Injector = ( LPTHREAD_START_ROUTINE ) GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");	

	if(!Injector) DispError("[!] Error while getting LoadLibraryA address.",DIE);
	
	HANDLE hThread = CreateRemoteThread(hModule, NULL, 0, Injector, (void *) AllocMem, 0, NULL);
	
	if(!hThread) DispError("[!] Cannot create thread.",DIE);

	DWORD Result = WaitForSingleObject(hThread, 10*1000); //Time out : 10 secondes
	if(Result==WAIT_ABANDONED || Result==WAIT_TIMEOUT || Result==WAIT_FAILED)
		DispError("[!] Thread TIME OUT.",DIE);

	Sleep(1000);
	/*VirtualFreeEx(hModule, (void *) AllocMem, 0, MEM_RELEASE);
	if(hThread!=NULL) CloseHandle(hThread);*/

return 1;
}

//SE_DEBUG_NAME
int LoadPrivilege()
{
	HANDLE hToken;
	LUID Val;
	TOKEN_PRIVILEGES tp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
		return(GetLastError());

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Val))
		return(GetLastError());

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = Val;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof (tp), NULL, NULL))
		return(GetLastError());

	CloseHandle(hToken);

	return 1;
}
/* ******************************** */
/*
inject.exe "C:\Program Files\Internet Explorer\IEXPLORE.EXE" C:\IAThijackDLL.dll --runtime 
inject.exe 3284 C:\IAThijackDLL.dll --runtime  // inject pid 3284
inject.exe iexplorer.exe C:\IAThijackDLL.dll --runtime --resolve // inject pid 3284
*/

int main(int argc, char* argv[])
{
	DWORD ProcPid=0;

	printf(" ** Running kInject v1.0 by Kdm (kodmaker@netcourrier.com) **\n\n");
	if(argc < 3 )
	{
		DispError(usage, 0);
		return 0;
	}

	// Run and inject process
	if(strcmp(argv[3], "--create")==0 || strcmp(argv[3], "-c")==0)
	{
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		memset(&si, 0, sizeof(si));   
		si.cb = sizeof(si);
		//si.wShowWindow = SW_HIDE;
		//"C:\\Program Files\\Internet Explorer\\IEXPLORE.EXE"
		printf("Creating process %s...", argv[1]);
		if(!CreateProcess(NULL,argv[1],NULL, NULL, true, 0, NULL, NULL,&si, &pi))
		{
			DispError("[!] CreateProcess failed",DIE);
		}
		printf(" OK.\nInjecting DLL %s...", argv[2]);
		InjectDll(pi.hProcess, argv[2]);
		printf(" OK\n");

		getchar();
		return 0;
	}
	

	// Inject a process that's already running
	if(strcmp(argv[3], "--runtime")==0 || strcmp(argv[3], "-r")==0)
	{
		/*
		//Get Process Id from exe name
		if(argc>3 && strcmp(argv[4],"--resolve") == 0)
		{
			ProcPid = GetPidByName(argv[1]);
			if(ProcPid==0) DispError("GetPidByName failed.", DIE);
			printf("Process %s has PID: %d\n", argv[1], ProcPid);
		} 
		else
		{
			ProcPid = atol(argv[1]);
		}
		*/
		ProcPid = atol(argv[1]);
		
		HANDLE hProc;
		hProc = OpenProcess(PROCESS_ALL_ACCESS, true,ProcPid);
		if(hProc==NULL)
		{
			printf("OpenProcess failed, triggering DebugPrivilege...");
			if(LoadPrivilege()!=1) DispError("DebugPrivilege : load FAILED", DIE);
			printf(" OK");
		}

		hProc = OpenProcess(PROCESS_ALL_ACCESS, true,ProcPid);
		if(hProc==NULL) DispError("Still can't open process. (Sure it exists ?)", DIE);
		
		printf("Injecting DLL %s in Pid: %d...", argv[2], ProcPid);
		InjectDll(hProc, argv[2]);
		printf(" OK\n");

		getchar();

		return 0;
	}

	printf("Unknow command parameter.");
	return 0;
}


