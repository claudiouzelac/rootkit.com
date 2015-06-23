#ifndef __PE_FILE_H
 #define __PE_FILE_H
//#define __PE_DEBUG

extern "C"
{
 #include "ntddk.h"
}

#include "../Include/KTypes.h"
#include "PeHeader.h"
#include "../Misc/Misc.h"

#ifdef __PE_DEBUG
#define DbgPrintPe(arg) DbgPrint arg
#else
#define DbgPrintPe(arg)
#endif //__PE_DEBUG


ULONG    LocateBase(void *CodePtr);
PIMAGE_SECTION_HEADER GetSection(PIMAGE_SECTION_HEADER pFirstSection, char *lpszSectionName, int nNumberSections);
VOID     RelocBuffer(DWORD hModule, DWORD Start, DWORD End, RELO_HEADER *RelocTable, int RelocTableSize, DWORD relo);
DWORD    NativeGetProcAddress(DWORD hModule, char *lpszFunctionName);


#endif //__PE_FILE_H