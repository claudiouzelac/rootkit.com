/******************************************************************************
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

#ifndef _ENTRY_POINT_HOOKING_FUNCTIONS_H
#define _ENTRY_POINT_HOOKING_FUNCTIONS_H

#include <windows.h>

#define  SIZEOFJUMP		5
#define  ASMNOP			0x90

typedef unsigned char byte;

void OutputString(char* frmstr,...);
int WakeUpProcess(DWORD pid);
int WINAPI HookApi(char* DllName, char* FuncName, DWORD ReplacementFunc, FARPROC* pFunc);
void InsertByte(DWORD Addr, unsigned char Byte);
void InsertDword(DWORD Addr, DWORD dWord);
void GenJmp(DWORD To, DWORD From);
int ForgeHook(DWORD pAddr, DWORD pAddrToJump, byte **Buffer);


#endif