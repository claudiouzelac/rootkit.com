#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef NTSTATUS (WINAPI* NTSAVEMERGEDKEYS)(HANDLE,HANDLE,HANDLE);

#ifndef __NTSAVEMERGEDKEYS__
#define __NTSAVEMERGEDKEYS__

NTSAVEMERGEDKEYS OldNtSaveMergedKeys;
NTSTATUS WINAPI NewNtSaveMergedKeys(HANDLE,HANDLE,HANDLE);

#endif 