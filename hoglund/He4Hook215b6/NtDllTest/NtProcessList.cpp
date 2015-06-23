#define STRICT
#include <windows.h>
#include "NtProcessList.h"

static BOOL bGetProcAddrsOK = FALSE;

static NTQUERYSYSTEMINFORMATION         pNtQuerySystemInformation = NULL;
static RTLCREATEQUERYDEBUGBUFFER        pRtlCreateQueryDebugBuffer = NULL;
static RTLDESTROYQUERYDEBUGBUFFER       pRtlDestroyQueryDebugBuffer = NULL;
static RTLQUERYPROCESSDEBUGINFORMATION  pRtlQueryProcessDebugInformation = NULL;
static NTQUERYINFORMATIONPROCESS        pNtQueryInformationProcess = NULL;
static NTOPENPROCESS                    pNtOpenProcess = NULL;
static NTCLOSE                          pNtClose = NULL;
static NTOPENDIRECTORYOBJECT            pNtOpenDirectoryObject = NULL;
static RTLINITUUNICODESTRING            pRtlInitUnicodeString = NULL;
static NTQUERYDIRECTORYOBJECT           pNtQueryDirectoryObject = NULL;
static NTOPENSYMBOLICLINKOBJECT         pNtOpenSymbolicLinkObject = NULL;
static NTQUERYSYMBOLICLINKOBJECT        pNtQuerySymbolicLinkObject = NULL;
static RTLGETFULLPATHNAME_U             pRtlGetFullPathName_U = NULL;
static NTQUERYOBJECT                    pNtQueryObject = NULL;
static NTSETSYSTEMINFORMATION           pNtSetSystemInformation = NULL;
static RTLADJUSTPRIVILEGE               pRtlAdjustPrivilege = NULL;

#define OS_VERSION_WIN32s               1
#define OS_VERSION_WIN9X                2
#define OS_VERSION_WINNT                3
#define OS_VERSION_WIN2K                4

static DWORD                            dwOsVersion = 0;

static int GetToken(WCHAR *lpInBuf, int dwInBufSize, WCHAR *lpOutBuf, int dwOutBufSize, WCHAR *lpDeliver, int nDeliverCount, int nNumber);

BOOL GetNtProcAddresses(void)
{
  if(!bGetProcAddrsOK)
    {
     HMODULE hNtDll = GetModuleHandle("ntdll.dll");
     if(!hNtDll)
        return bGetProcAddrsOK;

     pNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(hNtDll, "NtQuerySystemInformation");
     if(IsBadCodePtr((FARPROC)pNtQuerySystemInformation))
        return bGetProcAddrsOK;
     
     pRtlCreateQueryDebugBuffer = (RTLCREATEQUERYDEBUGBUFFER)GetProcAddress(hNtDll, "RtlCreateQueryDebugBuffer");
     if(IsBadCodePtr((FARPROC)pRtlCreateQueryDebugBuffer))
        return bGetProcAddrsOK;

     pRtlDestroyQueryDebugBuffer = (RTLDESTROYQUERYDEBUGBUFFER)GetProcAddress(hNtDll, "RtlDestroyQueryDebugBuffer");
     if(IsBadCodePtr((FARPROC)pRtlDestroyQueryDebugBuffer))
        return bGetProcAddrsOK;

     pRtlQueryProcessDebugInformation = (RTLQUERYPROCESSDEBUGINFORMATION)GetProcAddress(hNtDll, "RtlQueryProcessDebugInformation");
     if(IsBadCodePtr((FARPROC)pRtlQueryProcessDebugInformation))
        return bGetProcAddrsOK;

     pNtQueryInformationProcess = (NTQUERYINFORMATIONPROCESS)GetProcAddress(hNtDll, "NtQueryInformationProcess");
     if(IsBadCodePtr((FARPROC)pNtQueryInformationProcess))
        return bGetProcAddrsOK;

     pNtOpenProcess = (NTOPENPROCESS)GetProcAddress(hNtDll, "NtOpenProcess");
     if(IsBadCodePtr((FARPROC)pNtOpenProcess))
        return bGetProcAddrsOK;

     pNtClose = (NTCLOSE)GetProcAddress(hNtDll, "NtClose");
     if(IsBadCodePtr((FARPROC)pNtClose))
        return bGetProcAddrsOK;

     pNtOpenDirectoryObject = (NTOPENDIRECTORYOBJECT)GetProcAddress(hNtDll, "NtOpenDirectoryObject");
     if(IsBadCodePtr((FARPROC)pNtOpenDirectoryObject))
        return bGetProcAddrsOK;

     pRtlInitUnicodeString = (RTLINITUUNICODESTRING)GetProcAddress(hNtDll, "RtlInitUnicodeString");
     if(IsBadCodePtr((FARPROC)pRtlInitUnicodeString))
        return bGetProcAddrsOK;

     pNtQueryDirectoryObject = (NTQUERYDIRECTORYOBJECT)GetProcAddress(hNtDll, "NtQueryDirectoryObject");
     if(IsBadCodePtr((FARPROC)pNtQueryDirectoryObject))
        return bGetProcAddrsOK;

     pNtOpenSymbolicLinkObject = (NTOPENSYMBOLICLINKOBJECT)GetProcAddress(hNtDll, "NtOpenSymbolicLinkObject");
     if(IsBadCodePtr((FARPROC)pNtOpenSymbolicLinkObject))
        return bGetProcAddrsOK;

     pNtQuerySymbolicLinkObject = (NTQUERYSYMBOLICLINKOBJECT)GetProcAddress(hNtDll, "NtQuerySymbolicLinkObject");
     if(IsBadCodePtr((FARPROC)pNtQuerySymbolicLinkObject))
        return bGetProcAddrsOK;

     pRtlGetFullPathName_U = (RTLGETFULLPATHNAME_U)GetProcAddress(hNtDll, "RtlGetFullPathName_U");
     if(IsBadCodePtr((FARPROC)pRtlGetFullPathName_U))
        return bGetProcAddrsOK;

     pNtQueryObject = (NTQUERYOBJECT)GetProcAddress(hNtDll, "NtQueryObject");
     if(IsBadCodePtr((FARPROC)pNtQueryObject))
        return bGetProcAddrsOK;

     pNtSetSystemInformation = (NTSETSYSTEMINFORMATION)GetProcAddress(hNtDll, "NtSetSystemInformation");
     if(IsBadCodePtr((FARPROC)pNtSetSystemInformation))
        return bGetProcAddrsOK;

     pRtlAdjustPrivilege = (RTLADJUSTPRIVILEGE)GetProcAddress(hNtDll, "RtlAdjustPrivilege");
     if(IsBadCodePtr((FARPROC)pRtlAdjustPrivilege))
        return bGetProcAddrsOK;

     bGetProcAddrsOK = TRUE;
    }

  return bGetProcAddrsOK;
}

NTSTATUS NtGetProcessList(PPROCESS_INFO lpProcessInfo, DWORD dwProcessInfoSize, DWORD *lpLehgthReturned)
{
  DWORD                      dwLehgthReturned;
  NTSTATUS                   NtStatus;

  if(!lpProcessInfo || !dwProcessInfoSize) 
     return STATUS_NO_MEMORY;
  if(IsBadWritePtr(lpProcessInfo, dwProcessInfoSize))
     return STATUS_NO_MEMORY;

  if(!GetNtProcAddresses())
     return STATUS_ILLEGAL_FUNCTION;

  NtStatus = pNtQuerySystemInformation(SystemProcessesInformation, lpProcessInfo, dwProcessInfoSize, &dwLehgthReturned);
  if(NtStatus == STATUS_SUCCESS)
    { 
     if(lpLehgthReturned)
       {
        if(!IsBadWritePtr(lpLehgthReturned, sizeof(DWORD)))
           *lpLehgthReturned = dwLehgthReturned;
       }
    }

  return NtStatus;
}


PPROCESS_DEBUG_INFORMATION NtRtlCreateQueryDebugBuffer(void)
{
  PPROCESS_DEBUG_INFORMATION lpDebugBuffer;

  if(!GetNtProcAddresses())
     return NULL;

  lpDebugBuffer = pRtlCreateQueryDebugBuffer(0, 0);

  return lpDebugBuffer;
}

NTSTATUS NtRtlDestroyQueryDebugBuffer(PPROCESS_DEBUG_INFORMATION lpDebugBuffer)
{
  if(!GetNtProcAddresses())
     return STATUS_ILLEGAL_FUNCTION;

  return pRtlDestroyQueryDebugBuffer(lpDebugBuffer);
}

NTSTATUS NtRtlQueryProcessDebugInformation(PPROCESS_DEBUG_INFORMATION lpDebugBuffer, DWORD dwProcessId)
{
  if(!GetNtProcAddresses())
     return STATUS_ILLEGAL_FUNCTION;

  return pRtlQueryProcessDebugInformation(dwProcessId, 1, lpDebugBuffer);
}

DWORD NtGetProcessCommandLine(PWSTR lpwszCommandLine, DWORD dwSizeByByte, DWORD dwProcesId, DWORD dwThreadId)
{
  if(IsBadWritePtr(lpwszCommandLine, dwSizeByByte))
    {
     return 0;
    }
  ZeroMemory(lpwszCommandLine, dwSizeByByte);

  if(!GetNtProcAddresses())
     return 0;

  HANDLE             hProcess;
  NTSTATUS NtStatus;
  OBJECT_ATTRIBUTES  ObjAttr;

  InitializeObjectAttributes(&ObjAttr, NULL, 0, NULL, NULL);

  CLIENT_ID    ClientId;
  ClientId.UniqueProcess = (HANDLE)dwProcesId;
  ClientId.UniqueThread = (HANDLE)dwThreadId;

  NtStatus = pNtOpenProcess(&hProcess, /*0x02004000 | STANDARD_RIGHTS_ALL*/-1, &ObjAttr, &ClientId);//&ClientId
  if(NtStatus != STATUS_SUCCESS)
     return 0;

  if(!hProcess) 
     return 0;
  
  PROCESS_BASIC_INFORMATION  ProcessInfo;
  ULONG                      ReturnLength;
  DWORD                      dwSizeCmdLine = 0;
  NtStatus = pNtQueryInformationProcess(hProcess, ProcessBasicInformation, (PVOID)&ProcessInfo, sizeof(PROCESS_BASIC_INFORMATION), &ReturnLength);
  if(NtStatus == STATUS_SUCCESS)
    {
     PEB peb;
     DWORD dwRealReaded;

     BOOL bRes = ReadProcessMemory(hProcess, ProcessInfo.PebBaseAddress, &peb, sizeof(PEB), &dwRealReaded);
     if(bRes && dwRealReaded == sizeof(PEB))
       {
        PROCESS_PARAMETRS    pi;
        bRes = ReadProcessMemory(hProcess, peb.pi, &pi, sizeof(PROCESS_PARAMETRS), &dwRealReaded);
        if(bRes)
           ReadProcessMemory(hProcess, pi.CommandLine.Buffer, lpwszCommandLine, pi.CommandLine.Length, &dwSizeCmdLine);
       }
    }

  NtStatus = pNtClose(hProcess);
  return dwSizeCmdLine;
}

DWORD GetSymbolicLinkObject(PWSTR pwszDirectoryObject, PWSTR pwszSymLink, PVOID pwszSymLinkObject, DWORD dwSizeOfSymLinkObjectByBytes)
{
  if(!pwszDirectoryObject || !pwszSymLink)
     return 0;

  if(IsBadWritePtr(pwszSymLinkObject, dwSizeOfSymLinkObjectByBytes))
     return 0;
  ZeroMemory(pwszSymLinkObject, dwSizeOfSymLinkObjectByBytes);

  if(!GetNtProcAddresses())
     return 0;

  ULONG               dwSizeSymLinkObj = 0;
  UNICODE_STRING      dirString;
  OBJECT_ATTRIBUTES   dirAttr;
  HANDLE              hDir;

  pRtlInitUnicodeString(&dirString, pwszDirectoryObject);
  InitializeObjectAttributes(&dirAttr, &dirString, OBJ_CASE_INSENSITIVE, NULL, NULL);
  NTSTATUS NtStatus = pNtOpenDirectoryObject(&hDir, DIRECTORY_QUERY, &dirAttr);
  if(NtStatus == STATUS_SUCCESS)
    {
     char Buf[1024];
     POBJECT_NAMETYPE_INFO pObjName = (POBJECT_NAMETYPE_INFO) Buf;

     if(QueryObjectNameType(Buf, sizeof(Buf), hDir, pwszSymLink))
       {
        if(!wcsncmp(pObjName->ObjectType.Buffer, L"SymbolicLink", pObjName->ObjectType.Length/sizeof(WCHAR)))
          {
           HANDLE hSymLink;
           WCHAR  *SymLinkName = new WCHAR[wcslen(pwszSymLink)+wcslen(pwszDirectoryObject)+wcslen(L"\\")+1];
           if(SymLinkName)
             {
              wcscpy(SymLinkName, pwszDirectoryObject);
              wcscat(SymLinkName, L"\\");
              wcscat(SymLinkName, pwszSymLink);


              UNICODE_STRING      SymLinkString;
              pRtlInitUnicodeString(&SymLinkString, SymLinkName);
              InitializeObjectAttributes(&dirAttr, &SymLinkString, OBJ_CASE_INSENSITIVE, NULL, NULL);
              if(pNtOpenSymbolicLinkObject(&hSymLink, SYMBOLIC_LINK_QUERY, &dirAttr) == STATUS_SUCCESS)
                {
                 //dwSizeOfSymLinkObjectByBytes = 0;
                 SymLinkString.Buffer = (PWSTR)pwszSymLinkObject;
                 SymLinkString.Length = 0;
                 SymLinkString.MaximumLength = (WORD)dwSizeOfSymLinkObjectByBytes;

                 if(pNtQuerySymbolicLinkObject(hSymLink, &SymLinkString, &dwSizeSymLinkObj) != STATUS_SUCCESS)
                   {
                    dwSizeSymLinkObj = 0;
                   }
                 pNtClose(hSymLink);
                }
              delete[] SymLinkName;
             }
          }
       }
     pNtClose(hDir);
    }

  return dwSizeSymLinkObj;
}

BOOL QueryObjectNameType(PVOID pObjTypeInfo, DWORD dwSizeOfObjTypeInfo, HANDLE hDir, PWSTR pwszObjectName)
{
  char  Buf[2048];
  POBJECT_NAMETYPE_INFO pObjName = (POBJECT_NAMETYPE_INFO) Buf;
  DWORD ObjectIndex = 0, LengthReturned = 0;
  DWORD Index = 0;
  BOOL  bFirst = TRUE;
  
  if(!pObjTypeInfo || !pwszObjectName)
     return FALSE;
  if(!GetNtProcAddresses())
     return FALSE;
  
  while(pNtQueryDirectoryObject(hDir, Buf, sizeof(Buf), ObjectArray, bFirst, &ObjectIndex, &LengthReturned) >= 0)
    {
     bFirst = FALSE;
     for(int i=0; Index<ObjectIndex; Index++, i++)
       {
        if(!wcsnicmp(pObjName[i].ObjectName.Buffer, pwszObjectName, pObjName[i].ObjectName.Length/sizeof(WCHAR)))
          {
           if(pNtQueryDirectoryObject(hDir, pObjTypeInfo, dwSizeOfObjTypeInfo, ObjectByOne, bFirst, &Index, &LengthReturned) == STATUS_SUCCESS)
              return TRUE;
           return FALSE;
          }
       }
    }

  return FALSE;
}

DWORD DosPathNameToNtPathName(PWSTR pwszDosPath, PWSTR pwszNtPath, DWORD dwSizeNtPathByBytes)
{
  DWORD                 dwSizeOfNtPath = 0;
  DWORD                 dwSizeOfDosPath;
  PWSTR                 pwszCurrentDir, pwszPtrCurDir;
  PWSTR                 pwszPtrDosPath;
  UNICODE_STRING        dirString;
  OBJECT_ATTRIBUTES     dirAttr;
  HANDLE                hDir;
  NTSTATUS              NtStatus;
  char                  Buf[512];
  POBJECT_NAMETYPE_INFO pObjName = (POBJECT_NAMETYPE_INFO) Buf;
  DWORD                 dwRestDosPath, dwFreeCurDir, dwSizeToken;
  DWORD                 _dwSizeNtPathByBytes = dwSizeNtPathByBytes;
  HANDLE                hSymLink;
  UNICODE_STRING        SymLinkString;
  OBJECT_ATTRIBUTES     SymAttr;
  DWORD                 dwSizeSymLinkObj = 0;
  PWSTR                 pwszNewNtPath;
  
  if(!GetNtProcAddresses())
     return dwSizeOfNtPath;
  
  if(IsBadWritePtr(pwszNtPath, dwSizeNtPathByBytes))
     return dwSizeOfNtPath;
  memset(pwszNtPath, 0, dwSizeNtPathByBytes);

  dwSizeOfDosPath = wcslen(pwszDosPath);
  if(dwSizeOfDosPath <= 1)
     return dwSizeOfNtPath;

  pwszCurrentDir = new WCHAR[(sizeof(L"\\??\\")/sizeof(WCHAR))+dwSizeOfDosPath+1];
  if(!pwszCurrentDir)
     return dwSizeOfNtPath;
  memset(pwszCurrentDir, 0, sizeof(WCHAR)*(dwSizeOfDosPath+1));

  if(pwszDosPath[1] == L':')
     wcscpy(pwszCurrentDir, L"\\??");
  else
     wcscpy(pwszCurrentDir, L"\\");

  wcscpy(pwszNtPath, L"\\");

  pRtlInitUnicodeString(&dirString, pwszCurrentDir);
  InitializeObjectAttributes(&dirAttr, &dirString, OBJ_CASE_INSENSITIVE, NULL, NULL);
  NtStatus = pNtOpenDirectoryObject(&hDir, DIRECTORY_QUERY, &dirAttr);
  if(NtStatus == STATUS_SUCCESS)
    {
     if(pwszDosPath[1] == L':')
        wcscat(pwszCurrentDir, L"\\");
     pwszPtrCurDir = pwszCurrentDir + wcslen(pwszCurrentDir);
     pwszPtrDosPath = pwszDosPath;
     if(pwszPtrDosPath[0] == L'\\')
        pwszPtrDosPath++;
     dwFreeCurDir = ((sizeof(L"\\??\\")/sizeof(WCHAR))+dwSizeOfDosPath+1) - (pwszPtrCurDir - pwszCurrentDir);
     dwRestDosPath = dwSizeOfDosPath - (pwszPtrDosPath - pwszDosPath);
     
     while((dwSizeToken = GetToken(pwszPtrDosPath, dwRestDosPath,  pwszPtrCurDir, dwFreeCurDir, L"\\", 1, 0)))
       {
        dwFreeCurDir -= dwSizeToken;
        if(dwFreeCurDir != 0)
           dwFreeCurDir--;//sizeof(L'\\');
        dwRestDosPath -= dwSizeToken;
        if(dwRestDosPath != 0)
           dwRestDosPath--;//sizeof(L'\\');

        if(QueryObjectNameType(Buf, sizeof(Buf), hDir, pwszPtrCurDir))
          {
           if(!wcsncmp(pObjName->ObjectType.Buffer, L"Directory", pObjName->ObjectType.Length/sizeof(WCHAR)))
             {
              if(_dwSizeNtPathByBytes < ((wcslen(pObjName->ObjectName.Buffer)+2)*sizeof(WCHAR)))
                {
                 dwSizeOfNtPath = 0;
                 break;
                }
              wcscat(pwszNtPath, pObjName->ObjectName.Buffer);
              wcscat(pwszNtPath, L"\\");
              _dwSizeNtPathByBytes -= ((wcslen(pObjName->ObjectName.Buffer)+1)*sizeof(WCHAR));

              pNtClose(hDir);
              pRtlInitUnicodeString(&dirString, pwszCurrentDir);
              InitializeObjectAttributes(&dirAttr, &dirString, OBJ_CASE_INSENSITIVE, NULL, NULL);
              NtStatus = pNtOpenDirectoryObject(&hDir, DIRECTORY_QUERY, &dirAttr);
              if(NtStatus != STATUS_SUCCESS)
                {
                 dwSizeOfNtPath = 0;
                 break;
                }
             }
           else
             {
              if(!wcsncmp(pObjName->ObjectType.Buffer, L"SymbolicLink", pObjName->ObjectType.Length/sizeof(WCHAR)))
                {
                 dwSizeOfNtPath = 0;

                 pRtlInitUnicodeString(&SymLinkString, pObjName->ObjectName.Buffer);
                 InitializeObjectAttributes(&SymAttr, &SymLinkString, OBJ_CASE_INSENSITIVE, hDir, NULL);
                 if(pNtOpenSymbolicLinkObject(&hSymLink, SYMBOLIC_LINK_QUERY, &SymAttr) == STATUS_SUCCESS)
                   {
                    SymLinkString.Buffer = (PWSTR)pwszNtPath;
                    SymLinkString.Length = 0;
                    SymLinkString.MaximumLength = (WORD)dwSizeNtPathByBytes - wcslen(pwszPtrDosPath)*sizeof(WCHAR);

                    if(pNtQuerySymbolicLinkObject(hSymLink, &SymLinkString, &dwSizeSymLinkObj) == STATUS_SUCCESS)
                      {
                       if(pwszNtPath[0] == L'\\')
                         {
                          wcscat(pwszNtPath, pwszPtrDosPath+dwSizeToken);
                          pwszNewNtPath = new WCHAR[dwSizeNtPathByBytes/sizeof(WCHAR)];
                          if(pwszNewNtPath)
                            {
                             dwSizeOfNtPath = DosPathNameToNtPathName(pwszNtPath, pwszNewNtPath, dwSizeNtPathByBytes);
                             if(dwSizeOfNtPath)
                               {
                                wcscpy(pwszNtPath, pwszNewNtPath);
                                delete[] pwszNewNtPath;
                               }
                            }
                         }
                      }
                    pNtClose(hSymLink);
                   }
                 break;
                }
              else
                {
                 wcscat(pwszNtPath, pwszPtrDosPath);
                 dwSizeOfNtPath = wcslen(pwszNtPath);
                 break;
                }
             }
          }
        else
          {
           dwSizeOfNtPath = 0;
           break;
          }

        wcscat(pwszPtrCurDir, L"\\");
        pwszPtrCurDir += dwSizeToken + 1; // add wcslen(L"\\");
        pwszPtrDosPath += dwSizeToken + 1;
       }
     pNtClose(hDir);
    }

  delete[] pwszCurrentDir;

  return dwSizeOfNtPath;
}

int GetToken(WCHAR *lpInBuf, int dwInBufSize, WCHAR *lpOutBuf, int dwOutBufSize, WCHAR *lpDeliver, int nDeliverCount, int nNumber)
{
  if(!lpOutBuf) return 0;
  lpOutBuf[0] = 0;
//  memset(lpOutBuf, 0, sizeof(WCHAR)*dwOutBufSize);
  if(!lpInBuf) return 0;
  if(!lpDeliver) return 0;

  int   Count = 0, k;
  BOOL  bFlagExit;
  for(int i=0; i<dwInBufSize; i++)
     {
      for(k=0; k<nDeliverCount; k++)
         {
          if(lpInBuf[i] == lpDeliver[k])
            {
             if(i != 0) Count++;
             if(Count == nNumber) i++;
             break;
            }
         }
      if(Count == nNumber)
        {
         bFlagExit = FALSE;
         for(int j=0; j<dwOutBufSize; j++)
            {
             for(k=0; k<nDeliverCount; k++)
                {
                 if(lpInBuf[i] == lpDeliver[k])
                   {
                    bFlagExit = TRUE;
                    break;
                   }
                }
             if(bFlagExit) break;
             lpOutBuf[j] = lpInBuf[i];
             i++;
             if(i >= dwInBufSize) 
               {
                j++;
                break;
               }
            }
         lpOutBuf[j] = 0;
         return j;
        }
     }
  return 0;
}

//
// В win2k структура PROCESS_INFO увеличилась, для того чтобы
// код не надо было компилить под win2k и winnt4 отдельно написана
// эта функция. Указатель на THREAD_INFO получать только !!!! через нее.
//
PTHREAD_INFO GetThreadInfoPtr(PPROCESS_INFO pProcessInfo)
{
  if (dwOsVersion == 0)
    dwOsVersion = GetOsVersion();

  PTHREAD_INFO pThreadInfo = NULL;

  switch (dwOsVersion)
  {
    case OS_VERSION_WIN2K:
         pThreadInfo = (PTHREAD_INFO) ((char*)pProcessInfo + sizeof(PROCESS_INFO) + sizeof(DWORD)*12);
         break;
    default: //OS_VERSION_WINNT
         pThreadInfo = (PTHREAD_INFO) ((char*)pProcessInfo + sizeof(PROCESS_INFO));
         break;
  }


  //return (PTHREAD_INFO) (((char*)((char*)pProcessInfo + pProcessInfo->dwOffset)) - (pProcessInfo->dwThreadCount*sizeof(THREAD_INFO)));
  return pThreadInfo;
}


DWORD GetOsVersion(void)
{
  DWORD           dwOsVersion = 0;
  OSVERSIONINFO   OsVersionInfo = {0};
  OSVERSIONINFOEX OsVersionInfoEx = {0};
  OSVERSIONINFO*  pOsVersionInfo = NULL;

  OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  OsVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  
  if (!GetVersionEx(&OsVersionInfo))
  {
    if (GetVersionEx((OSVERSIONINFO*)&OsVersionInfoEx))
      pOsVersionInfo = (OSVERSIONINFO*)&OsVersionInfoEx;
  }
  else
  {
    pOsVersionInfo = &OsVersionInfo;
  }

  if (pOsVersionInfo != NULL)
  {
    switch (pOsVersionInfo->dwPlatformId)
    {
      case VER_PLATFORM_WIN32_WINDOWS:
           dwOsVersion = OS_VERSION_WIN9X;
           break;
      case VER_PLATFORM_WIN32s:
           dwOsVersion = OS_VERSION_WIN32s;
           break;
      case VER_PLATFORM_WIN32_NT:
           if (pOsVersionInfo->dwMajorVersion > 4)
             dwOsVersion = OS_VERSION_WIN2K;
           else
             dwOsVersion = OS_VERSION_WINNT;
           break;
    }
  }

  return dwOsVersion;
}

BOOL LoadDevice(PWSTR pwszDeviceFileName)
{
  if (pwszDeviceFileName == NULL)
    return FALSE;
  if (!GetNtProcAddresses())
    return FALSE;

  BOOLEAN OldState = FALSE;
  //SYSTEM_LOAD_IMAGE LoadImageInfo;
  SYSTEM_LOAD_AND_CALL_IMAGE LoadImageInfo;
  PWSTR             pwszNtFileName;
  int               nNtFileNameBufferLen = wcslen(pwszDeviceFileName)+sizeof(WCHAR)+1024;
  NTSTATUS          NtStatus, NtStatusSetPriv;
  BOOL              bRes = FALSE;

  pwszNtFileName = new WCHAR[nNtFileNameBufferLen];
  if (pwszNtFileName == NULL)
    return bRes;

  //memset(&LoadImageInfo, 0, sizeof(SYSTEM_LOAD_IMAGE));
  memset(&LoadImageInfo, 0, sizeof(LoadImageInfo));
  wcscpy(pwszNtFileName, pwszDeviceFileName);
  //if (DosPathNameToNtPathName(pwszDeviceFileName, pwszNtFileName, nNtFileNameBufferLen) == 0)
  //{
  //  delete[] pwszNtFileName;
  //  return bRes;
  //}

  pRtlInitUnicodeString(&LoadImageInfo.ModuleName, pwszNtFileName);

  NtStatusSetPriv = pRtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &OldState);

  //NTSTATUS NtStatus = pNtSetSystemInformation(SystemLoadImageInformation, &LoadImageInfo, sizeof(LoadImageInfo));
  NtStatus = pNtSetSystemInformation(SystemLoadAndCallImageInformation, &LoadImageInfo, sizeof(LoadImageInfo));
  if (NtStatus == STATUS_SUCCESS)
    bRes = TRUE;

  if (NtStatusSetPriv == STATUS_SUCCESS)
    pRtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, OldState, FALSE, &OldState);

  delete[] pwszNtFileName;

  return bRes;
}