/******************************************************************************\

	Vanquish Utils - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "Utils.h"
#include <time.h>

//VRT mutex
LPCWSTR VRTLOG_MUTEX = L"VRTMutexCommonExec__v";

//the log
LPCWSTR VANQUISH_LOG = L"c:\\vanquish.log";

//buffer for log
LPSTR lpVRTLogBuffer = NULL;
LPSTR lpVRTAppName = NULL;
LPSTR lpVRTLogDate = NULL;

const DWORD VRTLOGBUFFER_SZ = 1024;
const DWORD VRTAPPNAME_SZ = 256;
const DWORD VRTLOGDATE_SZ = 128;
const DWORD VRTHEAP_DEFAULT = 81920; //twenty pages; should that be enough?
const DWORD VRTSKIP_CEXESYNC = 0xffffffff;
const DWORD STACKINFO_SZ = 1024;

//old exception filter
LPTOP_LEVEL_EXCEPTION_FILTER lpOldTLEF;

extern HMODULE hVanquishModule; //in Vanquish_dll.cpp or Autoloader.cpp

HANDLE hHeap, hMutexVRT;

///////////////////////////////LOG FUNCTIONS////////////////////////////////////

void __cdecl VRTWriteLog(BOOL bWinError, DWORD dwError, HMODULE hModule, LPCSTR lpFmt, ...)
{
	LPSTR lpWinError = NULL;
	va_list vaList;

	//process sync so that there is only one log write at a time in the system
	if (dwError != VRTSKIP_CEXESYNC) VRTCommonExecutionBegin();

	//get current date&&time
	GetTimeFormatA(LOCALE_NEUTRAL, LOCALE_NOUSEROVERRIDE, NULL, NULL, lpVRTLogDate, VRTLOGDATE_SZ);
	VRTWriteLogLOWLEVEL("\r\n***Application: %s\r\n***Time: %s\r\n", lpVRTAppName, lpVRTLogDate);
	GetDateFormatA(LOCALE_NEUTRAL, LOCALE_NOUSEROVERRIDE, NULL, NULL, lpVRTLogDate, VRTLOGDATE_SZ);
	VRTWriteLogLOWLEVEL("***Date: %s\r\n", lpVRTLogDate);

	//get the win error message
	if (bWinError && dwError)
	{
		// | FORMAT_MESSAGE_FROM_HMODULE
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, (hModule == NULL) ? NULL : hModule, dwError, 0, (LPSTR)&lpWinError, 0, NULL);
		VRTWriteLogLOWLEVEL("0x%08x: %s", dwError, (lpWinError == NULL) ? "???" : lpWinError);
		LocalFree(lpWinError);
	}

	//write it
	va_start(vaList, lpFmt);
	VRTvWriteLogLOWLEVEL(lpFmt, vaList);
	va_end(vaList);

	//release the sync system (other DLLs can write now!)
	if (dwError != VRTSKIP_CEXESYNC) VRTCommonExecutionEnd();
}

void __cdecl VRTvWriteLogLOWLEVEL(LPCSTR lpFmt, va_list vaList)
{
	HANDLE hFile;
	DWORD dwSize, dwActual;
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;

	//printf to buffer
	dwSize = wvsprintfA(lpVRTLogBuffer, lpFmt, vaList);

	//init security shit
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = &sd;

	//open file
	hFile = CreateFileW(VANQUISH_LOG, GENERIC_WRITE, FILE_SHARE_READ, &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (!hFile) return; //quit silently; (no c:\ access)

	//append
	SetFilePointer(hFile, 0, NULL, FILE_END);

	//write data
	WriteFile(hFile, lpVRTLogBuffer, dwSize, &dwActual, NULL);

	//FlushFileBuffers(hFile);
	CloseHandle(hFile);
}

void __cdecl VRTWriteLogLOWLEVEL(LPCSTR lpFmt, ...)
{
	HANDLE hFile;
	DWORD dwSize, dwActual;
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;
	va_list vaList;

	//printf to buffer
	va_start(vaList, lpFmt);
	dwSize = wvsprintfA(lpVRTLogBuffer, lpFmt, vaList);
	va_end(vaList);

	//init security shit
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = &sd;

	//open file
	hFile = CreateFileW(VANQUISH_LOG, GENERIC_WRITE, FILE_SHARE_READ, &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (!hFile) return; //quit silently; (no c:\ access)

	//append
	SetFilePointer(hFile, 0, NULL, FILE_END);

	//write data
	WriteFile(hFile, lpVRTLogBuffer, dwSize, &dwActual, NULL);

	//FlushFileBuffers(hFile);
	CloseHandle(hFile);
}

LONG WINAPI VRTExceptionFilter(LPEXCEPTION_POINTERS lpEp)
{
	//determine first if it is our fault or not
	BOOL bUs = FALSE;
	DWORD dwAddr = 0;
    LPDWORD pFrame, pPrevFrame;
	MEMORY_BASIC_INFORMATION mbi;
	LPSTR lpStackInfo;
	LPVOID lpMemory;

	lpMemory = VRTAlloc(STACKINFO_SZ);
	if (!lpMemory)
		goto notus; //do default exception handling
	lpStackInfo = (LPSTR)lpMemory;
	lpStackInfo[0] = 0; //empty string

	//let's parse the stack...
	dwAddr = lpEp->ContextRecord->Eip;
    pFrame = (LPDWORD)lpEp->ContextRecord->Ebp;

	for(;;)
	{
		CHAR szModule[MAX_PATH];

		if (!VirtualQuery((LPCVOID)dwAddr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)))
			goto notus; //cannot determine if it is our fault

		if (!GetModuleFileName((HMODULE)mbi.AllocationBase, szModule, MAX_PATH))
			lpStackInfo += wsprintf(lpStackInfo, "???[0x%08x]: 0x%08x\r\n", szModule, (DWORD)mbi.AllocationBase, dwAddr);
		else
		{
			bUs = TRUE;
			lpStackInfo += wsprintf(lpStackInfo, "%s[0x%08x]: 0x%08x\r\n", szModule, (DWORD)mbi.AllocationBase, dwAddr);
		}

		dwAddr = pFrame[1];
		pPrevFrame = pFrame;
		pFrame = (LPDWORD)pFrame[0]; //precede to next higher frame on stack
        
		//frame pointer is always aligned
		if ((DWORD)pFrame & 3) break;

		//we are going backwards through stack => up in memory, not down
		if (pFrame <= pPrevFrame) break;

		//check if we can read two DWORDs
        if (IsBadReadPtr(pFrame, sizeof(DWORD) << 1)) break;
	}

notus:
	bUs = FALSE;
	if (bUs)
	{
		//write info to log
		VRTWriteLog(
			FALSE, 0, NULL,
			"Unhandled exception caught! Please forward this information to the author.\r\n"
			"Base: 0x%08x * Exception Adress: 0x%08x\r\n"
			"EAX: 0x%08x  EBX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x\r\nESI: 0x%08x  EDI: 0x%08x\r\n"
			"EBP: 0x%08x  ESP: 0x%08x EIP: 0x%08x\r\n"
			"---------STACK---------\r\n"
			"%s\r\n",
			(DWORD)hVanquishModule,
			(DWORD)lpEp->ExceptionRecord->ExceptionAddress,
			lpEp->ContextRecord->Eax,
			lpEp->ContextRecord->Ebx,
			lpEp->ContextRecord->Ecx,
			lpEp->ContextRecord->Edx,
			lpEp->ContextRecord->Esi,
			lpEp->ContextRecord->Edi,
			lpEp->ContextRecord->Ebp,
			lpEp->ContextRecord->Esp,
			lpEp->ContextRecord->Eip,
			lpMemory
		);

		if (lpMemory) VRTFree(lpMemory);
		return EXCEPTION_EXECUTE_HANDLER; //probably will terminate
	}
	else
	{
		if (lpMemory) VRTFree(lpMemory);
		if (lpOldTLEF)
			return lpOldTLEF(lpEp);
		else
			return EXCEPTION_CONTINUE_SEARCH;
	}
}

///////////////////////////////VRT FUNCTIONS////////////////////////////////////

VOID VRTInit()
{
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;

	//setup heap
	hHeap = HeapCreate(0, VRTHEAP_DEFAULT, 0);
	if (!HeapValidate(hHeap, 0, NULL))
	{
		//MessageBox(NULL, "Whoops! Your computer will now crash...", "BAU!", MB_OK);
		ExitProcess(0); //cannot validate/create any heap
	}

	//setup some space for logs
	lpVRTLogBuffer = (LPSTR)VRTAlloc(VRTLOGBUFFER_SZ);
	lpVRTAppName = (LPSTR)VRTAlloc(VRTAPPNAME_SZ);
	lpVRTLogDate = (LPSTR)VRTAlloc(VRTLOGDATE_SZ);

	//fail hard
	if (!(lpVRTLogBuffer && lpVRTAppName && lpVRTLogDate))
	{
		//MessageBox(NULL, "Whoops! Your computer will now crash (no log)...", "BAU!", MB_OK);
		ExitProcess(0);
	}

	//exception handling
	lpOldTLEF = SetUnhandledExceptionFilter(VRTExceptionFilter);

	//get app name
	GetModuleFileNameA(NULL, lpVRTAppName, VRTAPPNAME_SZ);

	//init security shit
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = &sd;

	//process/thread sync
	/*FRIGGIN'MUTEX -- Some strange things happen under SP2 here ;) (let me know if someone fixes this)
	hMutexVRT = CreateMutexW(&sa, FALSE, VRTLOG_MUTEX);
	if (hMutexVRT == NULL)
	{
		VRTWriteLog(FALSE, 0, NULL, "Fatal: Cannot initialize VRT process synchronization.\r\n");
		ExitProcess(0);
	}
	*/
}

VOID VRTDeInit()
{
	//close handles
	/*FRIGGIN'MUTEX
	CloseHandle(hMutexVRT);
	*/

	//destroy log buffers
	VRTFree(lpVRTLogDate);
	VRTFree(lpVRTAppName);
	VRTFree(lpVRTLogBuffer);

	//exception handling; restore OLD
	SetUnhandledExceptionFilter(lpOldTLEF);

	//bye bye heap
	HeapDestroy(hHeap);
}

LPVOID VRTAlloc(DWORD dwSize)
{
	//allocate heap
	return HeapAlloc(hHeap, 0, dwSize);
}

LPVOID VRTReAlloc(LPVOID lpOldMem, DWORD dwSize)
{
	//reallocate heap; check if valid first
	if (HeapValidate(hHeap, 0, lpOldMem))
		return HeapReAlloc(hHeap, 0, lpOldMem, dwSize);
	else
	{
		VRTWriteLog(FALSE, 0, NULL, "Tried to realloc an invalid memory block.\r\n");
		return NULL;
	}
}

VOID VRTFree(LPVOID lpMem)
{
	//free heap
	if (HeapValidate(hHeap, 0, lpMem))
		HeapFree(hHeap, 0, lpMem);
	else
		VRTWriteLog(FALSE, 0, NULL, "Tried to free an invalid memory block.\r\n");
}

LPVOID VRTCopyMemory(LPVOID dst, LPCVOID src, DWORD count)
{
	//dirty copy memory algorithm
	LPVOID ret = dst;

	while (count--)
	{
		*(char *)dst = *(char *)src;
		dst = (char *)dst + 1;
		src = (char *)src + 1;
	}

	return ret;
}

LPVOID VRTFillMemory(LPVOID dst, DWORD count, BYTE fill)
{
	//dirty fill memory algorithm
	void *start = dst;

	while (count--)
	{
		*(char *)dst = (char)fill;
		dst = (char *)dst + 1;
	}

	return start;
}

VOID VRTCommonExecutionBegin()
{
/*FRIGGIN'MUTEX
	//common execution subsystem
	DWORD dwWaitR;

	dwWaitR = WaitForSingleObject(hMutexVRT, WAITTIMEOUT);
	if (dwWaitR == WAIT_FAILED)
	{
		VRTWriteLog(FALSE, VRTSKIP_CEXESYNC, NULL, "VRT wait synchronization failed.\r\n");
	}
*/
}

VOID VRTCommonExecutionEnd()
{
/*FRIGGIN'MUTEX
	//common execution subsystem
	if (!ReleaseMutex(hMutexVRT))
	{
		VRTWriteLog(FALSE, VRTSKIP_CEXESYNC, NULL, "VRT release synchronization failed.\r\n");
	}
*/
}
