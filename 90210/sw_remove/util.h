#ifndef _UTIL_
#define _UTIL_

#include <ntddk.h>
#include "multicpu.h"

static DWORD MmSystemPteBase=0xc0000000;

void* SetInterruptHandler(int IntNo, void *HandlerPtr, BYTE flags);
void* GetInterruptHandler(DWORD IntNo, DWORD IdtNum);
DWORD* GetPde(void *Address);
DWORD* GetPte(void *Address);

#endif