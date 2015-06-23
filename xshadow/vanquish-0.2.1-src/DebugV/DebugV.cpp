/******************************************************************************\

	Vanquish DebugV - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

//------------------------------DebugV testing grounds------------------------//

#include <windows.h>
#include <tlhelp32.h>

void __declspec(naked) blah()
{
	__asm pushad
	__asm push 0x0012fab4 //patched dynamically
	__asm mov esi, 0x77e7d961 //patched dynamically
	__asm call esi
	__asm mov esi, 0x11223344
	__asm add esi, eax
	__asm call esi
	__asm popad
	__asm ret
}

extern char ** __argv;

LPCSTR HIDE_STRA = "vanquish";
const DWORD HIDE_STRCN = 8;
DWORD pos0(LPCSTR str1)
{
	LPSTR cp = (LPSTR)str1;
	LPSTR s1, s2;

	if (str1 == NULL) return MAXDWORD;
	//here we should use strlen(str1) but it's faster this way
	if (IsBadReadPtr(str1, HIDE_STRCN)) return MAXDWORD;

	while (*cp)
	{
		s1 = cp;
		s2 = (LPSTR)HIDE_STRA;

		while (*s1 && *s2 && !(CharUpperA((LPSTR)*s1) - CharUpperA((LPSTR)*s2)))
				s1++, s2++;

		if (!*s2) return (DWORD)(cp - str1);

		cp++;
	}

	return MAXDWORD;
}

//the WinMain thing...
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	HMODULE hv = LoadLibraryA(".\\VANQUISH.DLL");
	Sleep(2000);


	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW);

	//system("C:\\WINDOWS\\NOTEPAD.EXE");
	CreateProcessW(L"C:\\WINDOWS\\NOTEPAD.EXE", L"C:\\WINDOWS\\NOTEPAD.EXE", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	for (;;)
	{
		SleepEx(1000, TRUE);
	}

	return 0;

	/*
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;
	GetThreadContext(GetCurrentThread(), &ctx);

	FreeLibrary((HMODULE)0xfac13bed);
	*/

	/*
	PROCESSENTRY32 pe;
	THREADENTRY32 te;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD, 0);

	pe.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hSnap, &pe))
	{
		MessageBox(NULL, pe.szExeFile, "proc", MB_OK);
		//>>>>>>>>>>> pe.th32ProcessID
		pe.dwSize = sizeof(PROCESSENTRY32);
		while (Process32Next(hSnap, &pe))
		{
			MessageBox(NULL, pe.szExeFile, "proc", MB_OK);
			pe.dwSize = sizeof(PROCESSENTRY32);
		}
	}

	te.dwSize = sizeof(THREADENTRY32);
	if (Thread32First(hSnap, &te))
	{
		te.dwSize = sizeof(THREADENTRY32);
		//>te.th32ThreadID< together with >te.th32OwnerProcessID<
		while (Thread32Next(hSnap, &te))
		{
			te.dwSize = sizeof(THREADENTRY32);
		}
	}

	CloseHandle(hSnap);
	*/

	/*
	char txt[256];
	GetModuleFileNameW(hv, (LPWSTR)txt, 256);
	*/

	/*
	ENUM_SERVICE_STATUSW ess[600];
	DWORD dwNeed, dwRet, dwHan, i;
	LPDWORD lpdwHan = NULL;
	WCHAR blah[8192];

	SC_HANDLE sc = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	dwHan = 0;
	blah[0] = 0;
	if (!EnumServicesStatusW(sc, SERVICE_WIN32, SERVICE_STATE_ALL, ess, sizeof(ENUM_SERVICE_STATUSW) << 1, &dwNeed, &dwRet, &dwHan))
	{
		if (GetLastError() != ERROR_MORE_DATA) return 0;
	}

	for(i = 0; i < dwRet; i++)
	{
		wcscat(blah, ess[i].lpServiceName);
		wcscat(blah, L"|");
	}
	MessageBoxW(NULL, blah, L"SERVICES", MB_OK);
	CloseServiceHandle(sc);
	*/

	/*
	HANDLE hUser;
	LogonUserW(L"Test", NULL, L"password", LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &hUser);
	CloseHandle(hUser);
	*/

	/*
	HKEY hKey;
	if (RegOpenKeyW(HKEY_CLASSES_ROOT, L"Access.Application.11\\shell\\New\\command\\", &hKey) != ERROR_SUCCESS)
	{
		MessageBoxA(NULL, "Blah", "TZAPA", MB_OK);
		return 1;
	}

	DWORD i, dwSize;
	WCHAR lpBuf[1024];
	i = 0;
	dwSize = 512;
	while (RegEnumValueW(hKey, i, lpBuf+1, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		lpBuf[0] = '>';
		lpBuf[lstrlenW(lpBuf) + 1] = 0;
		lpBuf[lstrlenW(lpBuf)] = '<';
		MessageBoxW(NULL, lpBuf, L"Enum", MB_OK);
		i++;
		dwSize = 512;
	}

	RegCloseKey(hKey);
	*/

	/*
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	memset(&si,0,sizeof(STARTUPINFO));
	si.cb  = sizeof(STARTUPINFOA);
	si.wShowWindow = SW_SHOW;
	//..\\DebugP\\Debug\\DebugP.exe
	//CreateProcessW(L"C:\\regnav.exe", L"C:\\regnav.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	CreateProcessA(__argv[1], __argv[1], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	WaitForSingleObject(pi.hThread, INFINITE);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	*/

	return 0;
}
