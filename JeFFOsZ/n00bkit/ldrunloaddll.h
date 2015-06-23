#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef NTSTATUS (WINAPI* LDRUNLOADDLL)(HANDLE);

#ifndef __LDRUNLOADDLL__
#define __LDRUNLOADDLL__

LDRUNLOADDLL   		OldLdrUnloadDll;
NTSTATUS WINAPI		NewLdrUnloadDll(HANDLE);

#endif