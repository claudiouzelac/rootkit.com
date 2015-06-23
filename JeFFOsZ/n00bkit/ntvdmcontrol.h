#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef NTSTATUS (WINAPI* NTVDMCONTROL)(ULONG,PVOID);

#ifndef __NTVDMCONTROL__
#define __NTVDMCONTROL__

NTVDMCONTROL OldNtVdmControl;
NTSTATUS WINAPI NewNtVdmControl(ULONG,PVOID);

#endif