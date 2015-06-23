/******************************************************************************\

	Vanquish Utils - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include "Utils.h"
#include <time.h>
#include <stdio.h>

//the log
LPCSTR VANQUSIH_LOG = "c:\\vanquish.log";

///////////////////////////////LOG FUNCTIONS////////////////////////////////////

int Vanquish_Exception(LPCSTR lpFile, DWORD dwLine, DWORD dwExcep, LPEXCEPTION_POINTERS lpEp)
{
	char lpBuf[1024];

	_snprintf(
		lpBuf,
		1024,
		"Unhandled exception in vanquish.dll! Please forward this information to the author.\n"
		"Line: %u ExcCode: 0x%08x\nAdress: 0x%08x CPU registers follow:\n"
		"EAX: 0x%08x  EBX: 0x%08x  ECX: 0x%08x  EDX: 0x%08x\nESI: 0x%08x  EDI: 0x%08x\n"
		"EBP: 0x%08x\nESP: 0x%08x\nEIP: 0x%08x",
		dwLine,
		dwExcep,
		lpEp->ExceptionRecord->ExceptionAddress,
		lpEp->ContextRecord->Eax,
		lpEp->ContextRecord->Ebx,
		lpEp->ContextRecord->Ecx,
		lpEp->ContextRecord->Edx,
		lpEp->ContextRecord->Esi,
		lpEp->ContextRecord->Edi,
		lpEp->ContextRecord->Ebp,
		lpEp->ContextRecord->Esp,
		lpEp->ContextRecord->Eip
	);

	Vanquish_Dump2Log(lpBuf);
	return 1;
}

void Vanquish_DumpDWORD(DWORD x)
{
	char xx[20];
	_snprintf(xx, 20, "0x%08x", x);
	Vanquish_Dump(xx);
}

void Vanquish_Dump(LPCSTR what)
{
	FILE *f = fopen(VANQUSIH_LOG, "at");
	fprintf(f, "%s\n", what);
	fclose(f);
}

void Vanquish_DumpW(LPCWSTR what)
{
	FILE *f = fopen(VANQUSIH_LOG, "at");
	fprintf(f, "%S\n", what);
	fclose(f);
}

void Vanquish_Dump2Log(LPCSTR what)
{
	FILE *f = fopen(VANQUSIH_LOG, "at");
	struct tm *newtime;
	time_t aclock;
	time(&aclock);
	newtime = localtime(&aclock);
	fprintf(f, "-------------------Time: %s\n%s\n\n", asctime(newtime), what);
	fclose(f);
}

void Vanquish_DumpWithErrorCode(LPCSTR what, DWORD pre_error)
{
	void *winerr;
	FILE *f = fopen(VANQUSIH_LOG, "at");
	if (pre_error == 0)
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&winerr, 0, NULL);
	else
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, pre_error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&winerr, 0, NULL);
	fprintf(f, "ERROR: 0x%08x; IN VANQUISH: %s; DESCR: %s\n", GetLastError(), what, winerr);
	LocalFree(winerr);
	fclose(f);
}
