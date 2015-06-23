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

#ifndef _KNTINETWORKHIDE_H
#define _KNTINETWORKHIDE_H

//	oo Netstat & others similar TCP/UDP utils oo

// GetTcpTable
// Structures available at msdn.com
typedef struct   { //_MIB_TCPROW_
  DWORD dwState;
  DWORD dwLocalAddr;
  DWORD dwLocalPort;
  DWORD dwRemoteAddr;
  DWORD dwRemotePort;
} MIB_TCPROW_, *PMIB_TCPROW_;

typedef struct   { //_MIB_TCPTABLE
  DWORD dwNumEntries;
  MIB_TCPROW_ table[];
} MIB_TCPTABLE_, *PMIB_TCPTABLE_;


//AllocAndGetTcpExTableFromStack
// Undocumented extended information structures available 
// only on XP and higher
typedef struct {
  DWORD   dwState;        // state of the connection
  DWORD   dwLocalAddr;    // address on local computer
  DWORD   dwLocalPort;    // port number on local computer
  DWORD   dwRemoteAddr;   // address on remote computer
  DWORD   dwRemotePort;   // port number on remote computer
  DWORD	  dwProcessId;
} MIB_TCPEXROWEx, *PMIB_TCPEXROWEx;

typedef struct {
	DWORD			dwNumEntries;
	MIB_TCPEXROWEx	table[];
} MIB_TCPEXTABLEEx, *PMIB_TCPEXTABLEEx;


BOOL WINAPI MyCharToOemBuff(LPCTSTR lpszSrc, LPSTR lpszDst, DWORD cchDstLength);
DWORD WINAPI MyGetTcpTable(PMIB_TCPTABLE_ pTcpTable, PDWORD pdwSize, BOOL bOrder);
DWORD WINAPI MyAllocateAndGetTcpExTableFromStack(PMIB_TCPEXTABLEEx *pTcpTable,
				BOOL bOrder, HANDLE heap, DWORD zero, DWORD flags);
int GetProcessNamebyPid(DWORD pId, char* name);
BOOL WINAPI MyWriteFile(HANDLE hFile,LPCVOID lpBuffer,DWORD nNumberOfBytesToWrite,LPDWORD lpNumberOfBytesWritten,LPOVERLAPPED lpOverlapped);

BOOL WINAPI MyDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer,
  DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned,
  LPOVERLAPPED lpOverlapped);

#endif