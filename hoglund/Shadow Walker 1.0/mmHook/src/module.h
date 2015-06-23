#ifndef __module_h__
#define __module_h__

typedef unsigned long DWORD;
typedef unsigned long *PDWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned char *PBYTE;

#define MAXIMUM_FILENAME_LENGTH 256

#define ERROR_RETRIEVING_MODULE_LIST 0
#define MODULE_NOT_FOUND			 -1

typedef enum _SYSTEM_INFORMATION_CLASS 
{
  SystemBasicInformation,
  SystemProcessorInformation,
  SystemPerformanceInformation,
  SystemTimeOfDayInformation,
  SystemNotImplemented1,
  SystemProcessesAndThreadsInformation,
  SystemCallCounts,
  SystemConfigurationInformation,
  SystemProcessorTimes,
  SystemGlobalFlag,
  SystemNotImplemented2,
  SystemModuleInformation,
  /* ... */
} SYSTEM_INFORMATION_CLASS;


typedef struct _MODULE_INFO 
{
    DWORD d_Reserved1;
    DWORD d_Reserved2;
    PVOID p_Base;
    DWORD d_Size;
    DWORD d_Flags;
    WORD  w_Index;
    WORD  w_Rank;
    WORD  w_LoadCount;
    WORD  w_NameOffset;
    BYTE  a_bPath [MAXIMUM_FILENAME_LENGTH]; 
} MODULE_INFO, *PMODULE_INFO, **PPMODULE_INFO;


typedef struct _MODULE_LIST
{
    int         d_Modules;
    MODULE_INFO a_Modules[0];
    
}MODULE_LIST, *PMODULE_LIST, **PPMODULE_LIST;


typedef struct _MODULE 
{
	DWORD Base;
	DWORD End;
} MODULE, *PMODULE;


extern "C"
{
	NTSTATUS ZwQuerySystemInformation (SYSTEM_INFORMATION_CLASS, PVOID, DWORD, PDWORD);
}


PMODULE_LIST GetListOfModules(PNTSTATUS);
BOOLEAN IsAddressInModule (ULONG, char *, PNTSTATUS);
void WhatModuleIsAddressIn(ULONG, char *, PNTSTATUS);
int GetModuleBase( char* ModuleName, MODULE* mod );

#endif