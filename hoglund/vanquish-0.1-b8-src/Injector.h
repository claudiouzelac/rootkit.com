/******************************************************************************\

	Vanquish Injector - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __INJECTOR__H__
#define __INJECTOR__H__

#define _WIN32_WINNT 0x0500
#include <windows.h>

#define BEGIN_DEBUGPRIVILEGE HANDLE hToken = NULL; PTOKEN_PRIVILEGES lptkpNew = NULL, lptkpOld = NULL; Vanquish_BeginDebugPrivilege(hToken, lptkpNew, lptkpOld);
#define END_DEBUGPRIVILEGE Vanquish_EndDebugPrivilege(hToken, lptkpOld);

BOOL Vanquish_BeginDebugPrivilege(HANDLE hToken, PTOKEN_PRIVILEGES lptkpNew, PTOKEN_PRIVILEGES lptkpOld);
void Vanquish_EndDebugPrivilege(HANDLE hToken, PTOKEN_PRIVILEGES lptkpOld);

LPVOID Vanquish_ModuleFunction(LPCTSTR lpszAPIFunction, HMODULE hModule);
BOOL Vanquish_PrepareInjector();
BOOL Vanquish_InjectDLLbyPID(DWORD pid);
BOOL Vanquish_InjectDLLbyHandle(HANDLE hTarget);

#endif //__INJECTOR__H__