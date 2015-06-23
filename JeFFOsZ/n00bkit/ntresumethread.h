#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "ntdll.h"

typedef NTSTATUS (WINAPI* NTRESUMETHREAD)(HANDLE,PDWORD);
typedef NTSTATUS (WINAPI* LDRINITIALIZETHUNK)(DWORD,DWORD,DWORD);

#ifndef __NTRESUMETHREAD__
#define __NTRESUMETHREAD__

NTRESUMETHREAD		OldNtResumeThread;
NTSTATUS WINAPI		NewNtResumeThread(HANDLE,PDWORD);
LDRINITIALIZETHUNK	OldLdrInitializeThunk;
NTSTATUS WINAPI		NewLdrInitializeThunk(DWORD,DWORD,DWORD);
DWORD				dwLdrInitializeThunkSize;

#endif
