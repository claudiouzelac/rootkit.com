#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef NTSTATUS (WINAPI* NTREADVIRTUALMEMORY)(HANDLE,PVOID,PVOID,ULONG,PULONG);

#ifndef __NTREADVIRTUALMEMORY__
#define __NTREADVIRTUALMEMORY__

NTREADVIRTUALMEMORY OldNtReadVirtualMemory;
NTSTATUS WINAPI NewNtReadVirtualMemory(HANDLE,PVOID,PVOID,ULONG,PULONG);

#endif 