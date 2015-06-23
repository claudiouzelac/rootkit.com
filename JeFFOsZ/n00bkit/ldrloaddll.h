#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef NTSTATUS (WINAPI* LDRLOADDLL)(PWCHAR,ULONG,PUNICODE_STRING,PHANDLE);

#ifndef __LDRLOADDLL__
#define __LDRLOADDLL__

LDRLOADDLL   		OldLdrLoadDll;
NTSTATUS WINAPI		NewLdrLoadDll(PWCHAR,ULONG,PUNICODE_STRING,PHANDLE);

#endif
