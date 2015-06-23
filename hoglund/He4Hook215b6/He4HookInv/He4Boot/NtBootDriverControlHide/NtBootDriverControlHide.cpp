#define STRICT
#include "NtBootDriverControlHide.hpp"

NtBootDriverControlHide::NtBootDriverControlHide(const WCHAR *lpszDeviceFileName) :
Result(FALSE), m_lpszDeviceFileName(NULL)
{
  if(!lpszDeviceFileName) return;

  m_lpszDeviceFileName = new WCHAR[(wcslen(lpszDeviceFileName))+1];
  if(m_lpszDeviceFileName) 
     wcscpy(m_lpszDeviceFileName, lpszDeviceFileName);
  else
     return;
  Result = TRUE;
}

NtBootDriverControlHide::~NtBootDriverControlHide()
{
  if(m_lpszDeviceFileName)  delete(m_lpszDeviceFileName);
  m_lpszDeviceFileName = 0;
}

BOOLEAN NtBootDriverControlHide::SendCommand(USER_COMMAND *lpUserCommand)
{
  return TRUE;
}

BOOLEAN NtBootDriverControlHide::Start(WCHAR *lpszDeviceName)
{
  if(!lpszDeviceName) return FALSE;

  UNICODE_STRING     str;
  NTSTATUS           NtStatus;
  BOOLEAN            bEnable;
  WCHAR             *lpwszFullServiceName;


  lpwszFullServiceName = QueryFullDeviceName(lpszDeviceName);
  if(!lpwszFullServiceName)
     return FALSE;

  RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &bEnable);

  RtlInitUnicodeString(&str, lpwszFullServiceName);
  NtStatus = ZwLoadDriver(&str);
  if(!NT_SUCCESS(NtStatus)) 
    {
     ReleaseFullDeviceName(lpwszFullServiceName);
     return FALSE;
    }
  ReleaseFullDeviceName(lpwszFullServiceName);

  return TRUE;
}

BOOLEAN NtBootDriverControlHide::Stop(WCHAR *lpszDeviceName)
{
  if(!lpszDeviceName) return FALSE;

  UNICODE_STRING     str;
  NTSTATUS           NtStatus;
  BOOLEAN            bEnable;
  WCHAR             *lpwszFullServiceName;

  lpwszFullServiceName = QueryFullDeviceName(lpszDeviceName);
  if(!lpwszFullServiceName)
     return FALSE;

  RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &bEnable);

  RtlInitUnicodeString(&str, lpwszFullServiceName);
  NtStatus = ZwUnloadDriver(&str);
  if(!NT_SUCCESS(NtStatus)) 
    {
     ReleaseFullDeviceName(lpwszFullServiceName);
     return FALSE;
    }
  ReleaseFullDeviceName(lpwszFullServiceName);

  return TRUE;
}

BOOLEAN NtBootDriverControlHide::Install(PWSTR lpwszDefaultDeviceName, ULONG ulType, ULONG ulStart, ULONG ulErrorControl, PWSTR lpwszObjectName)
{
  if(!m_lpszDeviceFileName || !lpwszDefaultDeviceName) return FALSE;

  
//  WCHAR              wszDeviceName[64];
//  PWSTR              lpwszDeviceName = 0;
  WCHAR             *lpwszFullServiceName;
  UNICODE_STRING     str, GroupValue;
  OBJECT_ATTRIBUTES  obj;
  ULONG              Disposition, dwData;
  HANDLE             hKey;
  NTSTATUS           NtStatus;

//  if(!wszDefaultDeviceName)
//    {
//     swprintf(wszDeviceName, L"%08X", (ULONG)NtCurrentTeb());
//     lpwszDeviceName = wszDeviceName;
//    }
//  else
//     lpwszDeviceName = wszDefaultDeviceName;
  
  lpwszFullServiceName = QueryFullDeviceName(lpwszDefaultDeviceName);
  if(!lpwszFullServiceName)
     return FALSE;

  RtlInitUnicodeString(&str, lpwszFullServiceName);
  InitializeObjectAttributes(&obj, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);
  NtStatus = NtCreateKey(&hKey, KEY_ALL_ACCESS, &obj, 0, NULL, REG_OPTION_VOLATILE, &Disposition);
  if(!NT_SUCCESS(NtStatus)) 
    {
     ReleaseFullDeviceName(lpwszFullServiceName);
     return FALSE;
    }
//***********************************************************//
  RtlInitUnicodeString(&str, L"Type");
  dwData = ulType;
  NtStatus = NtSetValueKey(hKey, &str, 0, REG_DWORD, &dwData, sizeof(ULONG));
  if(!NT_SUCCESS(NtStatus)) 
    {
     NtClose(hKey);
     ReleaseFullDeviceName(lpwszFullServiceName);
     return FALSE;
    }

//***********************************************************//
  RtlInitUnicodeString(&str, L"Start");
  dwData = ulStart;
  NtStatus = NtSetValueKey(hKey, &str, 0, REG_DWORD, &dwData, sizeof(ULONG));
  if(!NT_SUCCESS(NtStatus)) 
    {
     NtClose(hKey);
     ReleaseFullDeviceName(lpwszFullServiceName);
     return FALSE;
    }
//***********************************************************//
  RtlInitUnicodeString(&str, L"ErrorControl");
  dwData = ulErrorControl;
  NtStatus = NtSetValueKey(hKey, &str, 0, REG_DWORD, &dwData, sizeof(ULONG));
  if(!NT_SUCCESS(NtStatus)) 
    {
     NtClose(hKey);
     ReleaseFullDeviceName(lpwszFullServiceName);
     return FALSE;
    }
//***********************************************************//
  RtlInitUnicodeString(&str, L"ImagePath");
  RtlInitUnicodeString(&GroupValue, m_lpszDeviceFileName); 
  NtStatus = NtSetValueKey(hKey, &str, 0, REG_SZ, GroupValue.Buffer, GroupValue.Length+sizeof(WCHAR));
  if(!NT_SUCCESS(NtStatus)) 
    {
     NtClose(hKey);
     ReleaseFullDeviceName(lpwszFullServiceName);
     return FALSE;
    }
//***********************************************************//
  if (lpwszObjectName != NULL)
  {
    RtlInitUnicodeString(&str, L"ObjectName");
    RtlInitUnicodeString(&GroupValue, lpwszObjectName); 
    NtStatus = NtSetValueKey(hKey, &str, 0, REG_SZ, GroupValue.Buffer, GroupValue.Length+sizeof(WCHAR));
    if (!NT_SUCCESS(NtStatus)) 
    {
      NtClose(hKey);
      ReleaseFullDeviceName(lpwszFullServiceName);
      return FALSE;
    }
  }
//***********************************************************//
  NtClose(hKey);

//  Start(lpwszFullServiceName);
//  Stop(lpwszFullServiceName);
//  Remove(lpwszFullServiceName);

  ReleaseFullDeviceName(lpwszFullServiceName);

  return TRUE;
}

BOOLEAN NtBootDriverControlHide::Remove(WCHAR *lpszDeviceName)
{
  if(!lpszDeviceName) return FALSE;

  WCHAR             *lpwszFullServiceName;

  lpwszFullServiceName = QueryFullDeviceName(lpszDeviceName);
  if(!lpwszFullServiceName)
     return FALSE;

  BOOLEAN bRes = DeleteKey(lpwszFullServiceName);
  ReleaseFullDeviceName(lpwszFullServiceName);

  return bRes;
}

BOOLEAN NtBootDriverControlHide::DeleteKey(WCHAR *lpszKeyName)
{
  if(!lpszKeyName) return FALSE;

  UNICODE_STRING     str;
  HANDLE             hKey;
  OBJECT_ATTRIBUTES  obj;
  NTSTATUS           NtStatus;
  CHAR               Buf[512];
  ULONG              dwRealLen;

  RtlInitUnicodeString(&str, lpszKeyName);
  InitializeObjectAttributes(&obj, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);
  NtStatus = NtOpenKey(&hKey, KEY_ALL_ACCESS, &obj);
  if(!NT_SUCCESS(NtStatus)) 
    {
     return FALSE;
    }

  memset(Buf, 0, sizeof(Buf));
  NtStatus = NtQueryKey(hKey, KeyFullInformation, Buf, sizeof(Buf), &dwRealLen);
  if(NT_SUCCESS(NtStatus)) 
    {
     PKEY_FULL_INFORMATION pKeyFullInfo = (PKEY_FULL_INFORMATION) Buf;
     ULONG Values = pKeyFullInfo->Values;
     ULONG SubKeys = pKeyFullInfo->SubKeys;
     ULONG i;

     for(i=0; i<Values; i++)
        {
         memset(Buf, 0, sizeof(Buf));
         NtStatus = NtEnumerateValueKey(hKey, 0, KeyValueBasicInformation, Buf, sizeof(Buf), &dwRealLen);
         if(NT_SUCCESS(NtStatus))
           {
            PKEY_VALUE_BASIC_INFORMATION pValueBasicInfo = (PKEY_VALUE_BASIC_INFORMATION) Buf;
            RtlInitUnicodeString(&str, pValueBasicInfo->Name);
            ZwDeleteValueKey(hKey, &str);
           }
        }
     for(i=0; i<SubKeys; i++)
        {
         memset(Buf, 0, sizeof(Buf));
         NtStatus = NtEnumerateKey(hKey, 0, KeyBasicInformation, Buf, sizeof(Buf), &dwRealLen);
         if(NT_SUCCESS(NtStatus))
           {
            PKEY_BASIC_INFORMATION pKeyBasicInfo = (PKEY_BASIC_INFORMATION) Buf;
            WCHAR BufName[512];
            swprintf(BufName, L"%s%c%s", lpszKeyName, OBJ_NAME_PATH_SEPARATOR, pKeyBasicInfo->Name);
            DeleteKey(BufName);
           }
        }
    }

  ZwDeleteKey(hKey);
  NtClose(hKey);

  return TRUE;
}

PWSTR NtBootDriverControlHide::QueryFullDeviceName(PWSTR lpwszDeviceName)
{
  if(!lpwszDeviceName)
     return NULL;

  PWSTR lpwszFullServiceName = new WCHAR [wcslen(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\") + wcslen(lpwszDeviceName) + 1];
  if(!lpwszFullServiceName)
     return NULL;
  swprintf(lpwszFullServiceName, L"%s%s", L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\", lpwszDeviceName);

  return lpwszFullServiceName;
}


BOOLEAN NtBootDriverControlHide::ReleaseFullDeviceName(PWSTR lpwszFullDeviceName)
{
  if(!lpwszFullDeviceName) 
     return FALSE;

  delete[] lpwszFullDeviceName;

  return TRUE;
}
 
   void* _cdecl operator new(size_t size)
   { 
    NTSTATUS           NtStatus;
    ULONG              ulSize = size;
    PVOID              lpBuf = 0;

    NtStatus = NtAllocateVirtualMemory((HANDLE)-1, &lpBuf, 0, &ulSize, MEM_COMMIT, PAGE_READWRITE);
    if(!NT_SUCCESS(NtStatus)) 
       lpBuf = 0;

    return lpBuf; 
   };
   void  _cdecl operator delete(void* p)
   {
    NTSTATUS           NtStatus;
    ULONG              ulSize = 0;

    if(p)
       NtStatus = NtFreeVirtualMemory((HANDLE)-1, &p, &ulSize, MEM_RELEASE);
   };