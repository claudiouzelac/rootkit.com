#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef NTSTATUS (WINAPI* NTOPENPROCESS)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PCLIENT_ID);

#ifndef __NTOPENPROCESS__
#define __NTOPENPROCESS__

NTOPENPROCESS OldNtOpenProcess;
NTSTATUS WINAPI NewNtOpenProcess(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PCLIENT_ID);

#endif 