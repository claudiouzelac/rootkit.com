////////////////////////////////////////////////////////////////////
// 
//	Header file for injector.c. Contains definitions used for 
//  injecting data and loading a dll in a remote process.
//  Written by JeFFOsZ
//
////////////////////////////////////////////////////////////////////

#define NTSTATUS LONG
#define NTAPI __stdcall 
#define NTSYSAPI DECLSPEC_IMPORT
#define ACCESS_MASK DWORD;

typedef struct _UNICODE_STRING 
{  
	USHORT Length;  
	USHORT MaximumLength;  
	PWSTR Buffer;
} 
UNICODE_STRING, *PUNICODE_STRING;

typedef NTSTATUS (NTAPI* LDRLOADDLL)(PWCHAR,ULONG,PUNICODE_STRING,PHANDLE);

typedef struct _RemoteProcessData
{
	LDRLOADDLL pLdrLoadDll;
	PWCHAR PathToFile;
	ULONG Flags;
	UNICODE_STRING ModuleFileName;
	HANDLE ModuleHandle;
} RemoteProcessData;

// Get debug privileges
BOOL GetDebugPriv(PTOKEN_PRIVILEGES); 

// Restores original privileges
BOOL RestorePrivileges(TOKEN_PRIVILEGES);

// Checks for NT and returns version numbers when true
BOOL IsWinNt(PDWORD,PDWORD);

// Injects data into a process and returns the address of the data
LPVOID InjectData(HANDLE,LPVOID,ULONG); 

// Loads a dll in a process. (uses kernel32.LoadLibraryA). 
// Use LoadDllInProcessEx instead !
// BOOL LoadDllInProcess(DWORD,char*); 

// Loads a dll in a process (uses ntdll.LdrLoadDll) and returns the
// ModuleHandle.
DWORD LoadDllInProcessEx(DWORD,char*);