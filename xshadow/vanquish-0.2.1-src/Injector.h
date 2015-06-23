/******************************************************************************\

	Vanquish Injector - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __INJECTOR__H__
#define __INJECTOR__H__

#include <windows.h>

#define BEGIN_DEBUGPRIVILEGE HANDLE hToken = NULL; PTOKEN_PRIVILEGES lptkpNew = NULL, lptkpOld = NULL; Vanquish_BeginDebugPrivilege(hToken, lptkpNew, lptkpOld);
#define END_DEBUGPRIVILEGE Vanquish_EndDebugPrivilege(hToken, lptkpOld);

//flags for dwFlags in injector
#define VANQUISHINJECT_PRESUSPENDTHREAD 0x1000
#define VANQUISHINJECT_POSTRESUMETHREAD 0x2000
#define VANQUISHINJECT_UNLOADVANQUISH   0x4000

BOOL Vanquish_BeginDebugPrivilege(HANDLE hToken, PTOKEN_PRIVILEGES lptkpNew, PTOKEN_PRIVILEGES lptkpOld);
void Vanquish_EndDebugPrivilege(HANDLE hToken, PTOKEN_PRIVILEGES lptkpOld);

LPVOID Vanquish_ModuleFunction(LPCTSTR lpszAPIFunction, HMODULE hModule);
BOOL Vanquish_PrepareInjector(); //MUST BE CALLED FROM THE MAIN THREAD ONLY!!!
BOOL Vanquish_SafeInjectProcess(LPPROCESS_INFORMATION pi, DWORD dwFlags);
BOOL Vanquish_SafeInjectDLL(HANDLE hProcess, HANDLE hThread, DWORD dwFlags);

#endif //__INJECTOR__H__
