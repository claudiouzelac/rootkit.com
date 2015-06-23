#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef NTSTATUS (WINAPI* NTQUERYSYSTEMINFORMATION)(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);

#ifndef __NTQUERYSYSTEMINFORMATION__
#define __NTQUERYSYSTEMINFORMATION__

NTQUERYSYSTEMINFORMATION OldNtQuerySystemInformation;
NTSTATUS WINAPI NewNtQuerySystemInformation(ULONG,PVOID,ULONG,PULONG);

#endif
