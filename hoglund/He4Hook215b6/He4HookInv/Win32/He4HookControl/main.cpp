#define STRICT
//#define UNICODE
//#define _UNICODE

#include <windows.h>
#include <conio.h>
#include <stdio.h>

#include "..\..\He4HookDriverHide\He4HookDriverHide.hpp"
#include "..\..\..\NtDllTest\NtProcessList.h"
//#include "..\He4HookControlDll\ShareMemory.h"
//#include "..\He4HookControlDll\He4Message.h"

#define HE4_SERVICE_NAME           "He4HookInv"
#define HE4_SERVICE_FILE_NAME      "He4HookInv.sys" 

void ConsoleErrorMessage(void);
void ShowDeviceCurrentVersion(void);
void ShowHelp(void);
void ShowProtectedFiles(He4HookDriverHide* pNtDriverControl);
void AddProcessToUnLockList(char* pszProcessFileName, He4HookDriverHide* pNtDriverControl);
void DeleteProcessFromUnLockList(char* pszProcessFileName, He4HookDriverHide* pNtDriverControl);
void ShowUnlockThreads(He4HookDriverHide* pNtDriverControl);
BOOL InstallNewDriver(He4HookDriverHide* pNtDriverControl);
BOOL EnableDebugPriv(VOID);
void ShowStatistic(He4HookDriverHide* pNtDriverControl);

DWORD GetVersion(DWORD *pdwVer);
DWORD GetLocalBase(DWORD *pdwBase);


He4HookDriverHide* pNtDriverControl = NULL;
BOOL               bForceLoadDriver = FALSE;
BOOL               bShowProtectedFiles = FALSE;
int                bHookFileSystem = -1;
BOOL               bLockSaveObjectsForAllThreads = FALSE;
BOOL               bClearSaveList = FALSE;
DWORD              dwFileAccessType = 0;
DWORD              dwProcessAccessType = HE4_UNLOCK_READ | HE4_UNLOCK_WRITE | HE4_UNLOCK_DELETE | HE4_UNLOCK_VISIBLE;
BOOL               bShowUnlockThreadInfo = FALSE;
BOOL               bShowStatistic = FALSE;

//
// -h              - Show help
// -i:n            - Install driver type: 
//                   n = 0  - open exist driver (default)
//                   n != 0 - load new image driver force.
// -s              - Show protected files
// -u:process_name - Add process to unlock list
// -l:process_name - Remove process from unlock list
// -cp:_string_    - Access type for process:
//                   R - read
//                   W - write
//                   D - delete
//                   V - visible
//                   (Example: -c:RV)
// -la             - Remove all processes from unlock list
// -hk:n           - Hook/Unhook file system:
//                   n = 1 - hook Zw*/Nt* func.\n\
//                   n = 2 - hook DRIVER_OBJECT\n\
// -a:             - Add file to save list
// -d:             - Delete file from save list
// -da             - Delete all files from save list
// -c:             - Access type
// -t              - Show unlock thread info
// -q              - Show statistic
//

//#pragma pack(push)
//#pragma pack(16)
//typedef struct _TEST_ALIGN
//{
//  char    m_Symbol;
//  __int64    m_Long;
//} TEST_ALIGN, *PTEST_ALIGN;
//#pragma pack(pop)

int main(int nCountArg, char* lpszArg[], char* lpszEnv[])
{
//  EnableDebugPriv();
//  CSharedMemory cSharedMemory;
//
//  if (cSharedMemory.Open(HE4_CONTROL_DLL_SHARED_MEMORY, 4096, 100))
//  {
//    char Msg[2048];
//    PHE4_CONTROL_MSG pMsg = (PHE4_CONTROL_MSG) Msg;
//    
//    pMsg->m_dwMessageId = 2;
//    strcpy((char*)pMsg->m_MessageBody, "D:\\tmp\\guard\\He4HookInv.sys");
//    pMsg->m_dwSizeByBytes = SIZE_OF_HE4_CONTROL_MSG_REAL + strlen((char*) pMsg->m_MessageBody) + sizeof(char);
//
//    cSharedMemory.Write(pMsg, pMsg->m_dwSizeByBytes, 500);
//
//    memset(Msg, 0, sizeof(Msg));
//
//    cSharedMemory.Write(pMsg, pMsg->m_dwSizeByBytes, 500);
//
//
//
//    cSharedMemory.Close();
//    printf("He4Dev@hotmal.com\n\n");
//  }
  
//  TEST_ALIGN  TestAlign;
//  PTEST_ALIGN pTestAlign = &TestAlign;
//  PCHAR pSymbol = (PCHAR)&(pTestAlign->m_Symbol);
//  PCHAR pLong = (PCHAR)&(pTestAlign->m_Long);
//  int nRest = pLong-pSymbol;

  printf("\nHe4HookControl v2.03 - control utility for He4HookInv\n");
  printf("Copyright (C) 2000 He4 developers team\n");
  printf("He4Dev@hotmail.com\n\n");

  ShowDeviceCurrentVersion();

  if (nCountArg <= 1)
  {
    ShowHelp();
    return 0;
  }

  for (int i=1; i<nCountArg; i++)
  {
    if (!stricmp(lpszArg[i], "-h"))
    {
      ShowHelp();
    }
    if (!strnicmp(lpszArg[i], "-i:", 3))
    {
      if (strlen(lpszArg[i]) > 3)
        bForceLoadDriver = (BOOL)atoi(lpszArg[i]+3);
    }
    if (!stricmp(lpszArg[i], "-s"))
    {
      bShowProtectedFiles = TRUE;
    }
    if (!strnicmp(lpszArg[i], "-hk:", 4))
    {
      if (strlen(lpszArg[i]) > 4)
        bHookFileSystem = atoi(lpszArg[i]+4);
    }
    if (!strnicmp(lpszArg[i], "-c:", 3))
    {
      if (strlen(lpszArg[i]) > 3)  
      {
        dwFileAccessType = 0;
        char* pszAcc = lpszArg[i]+3;
        for (; *pszAcc; pszAcc++)
        {
          if (*pszAcc == 'R' || *pszAcc == 'r')
            dwFileAccessType |= ACC_TYPE_READ;
          if (*pszAcc == 'W' || *pszAcc == 'w')
            dwFileAccessType |= ACC_TYPE_WRITE;
          if (*pszAcc == 'D' || *pszAcc == 'd')
            dwFileAccessType |= ACC_TYPE_DELETE;
          if (*pszAcc == 'V' || *pszAcc == 'v')
            dwFileAccessType |= ACC_TYPE_VISIBLE;
          if (*pszAcc == 'E' || *pszAcc == 'e')
            dwFileAccessType |= FILE_ACC_TYPE_EXCHANGE;
        }
      }
    }
    if (!strnicmp(lpszArg[i], "-cp:", 4))
    {
      if (strlen(lpszArg[i]) > 4)  
      {
//        dwFileAccessType = 0;
        char* pszAcc = lpszArg[i]+4;
        dwProcessAccessType = 0;
        for (; *pszAcc; pszAcc++)
        {
          if (*pszAcc == 'R' || *pszAcc == 'r')
            dwProcessAccessType |= HE4_UNLOCK_READ;
          if (*pszAcc == 'W' || *pszAcc == 'w')
            dwProcessAccessType |= HE4_UNLOCK_WRITE;
          if (*pszAcc == 'D' || *pszAcc == 'd')
            dwProcessAccessType |= HE4_UNLOCK_DELETE;
          if (*pszAcc == 'V' || *pszAcc == 'v')
            dwProcessAccessType |= HE4_UNLOCK_VISIBLE;
        }
      }
    }
    if (!stricmp(lpszArg[i], "-la"))
      bLockSaveObjectsForAllThreads = TRUE;
    if (!stricmp(lpszArg[i], "-da"))
      bClearSaveList = TRUE;
    if (!stricmp(lpszArg[i], "-t"))
      bShowUnlockThreadInfo = TRUE;
    if (!stricmp(lpszArg[i], "-q"))
      bShowStatistic = TRUE;
  }


  TCHAR szExeFileName[2048];
  TCHAR szDeviceFileName[2048];

  GetModuleFileName(NULL, szExeFileName, 2048);

  TCHAR drive[_MAX_DRIVE];   
  TCHAR dir[_MAX_DIR];
  TCHAR fname[_MAX_FNAME];   
  TCHAR ext[_MAX_EXT];

  _splitpath(szExeFileName, drive, dir, fname, ext);
  lstrcpy(szDeviceFileName, _T("\\??\\"));
  _makepath(szDeviceFileName+sizeof(_T("\\??\\"))-sizeof(TCHAR), drive, dir, HE4_SERVICE_FILE_NAME, _T(""));

  pNtDriverControl = new He4HookDriverHide(szDeviceFileName);
  if (pNtDriverControl == NULL)
  {
    printf("\nNo memory for create class He4HookDriverHide!!!\n");
    return -1;
  }

  if (pNtDriverControl->Result == FALSE)
  {
    delete pNtDriverControl;
    printf("\nCreate class He4HookDriverHide - ERROR!!!\n");
    return -1;
  }

  if (bForceLoadDriver)
  {
    if (InstallNewDriver(pNtDriverControl) == FALSE)
    {
      delete pNtDriverControl;
      printf("\nLoad new driver - ERROR!!!\n");
      return -1;
    }

    bShowProtectedFiles = TRUE;
    
    printf("\nNew version driver:\n");
    ShowDeviceCurrentVersion();
  }

  if (bHookFileSystem != -1)
  {
    DWORD dwDrivesMaskReal;
    if ((dwDrivesMaskReal = pNtDriverControl->HookFileSystem((DWORD)bHookFileSystem)) == (DWORD)-1)
    {
      if (bHookFileSystem)
        printf("Hook file system - ERROR!!!\n");
      else
        printf("Unhook file system - ERROR!!!\n");
    }
    else
    {
      if (bHookFileSystem)
        printf("File system - hooked\n");
      else
        printf("File system - unhooked\n");
    }
  }

  if (bLockSaveObjectsForAllThreads == TRUE)
    pNtDriverControl->LockSaveObjectsForAllThreads();

  if (bClearSaveList == TRUE)
    pNtDriverControl->ClearSaveList();
  
  for (i=1; i<nCountArg; i++)
  {
    if (!strnicmp(lpszArg[i], "-u:", 3))
    {
      if (strlen(lpszArg[i]) > 3)
      {
        AddProcessToUnLockList(lpszArg[i]+3, pNtDriverControl);
        bShowUnlockThreadInfo = TRUE;
      }
    }
    if (!strnicmp(lpszArg[i], "-l:", 3))
    {
      if (strlen(lpszArg[i]) > 3)
      {
        DeleteProcessFromUnLockList(lpszArg[i]+3, pNtDriverControl);
        bShowUnlockThreadInfo = TRUE;
      }
    }
    
    if (!strnicmp(lpszArg[i], "-a:", 3))
    {
      if (strlen(lpszArg[i]) > 3)
      {
        if (dwFileAccessType & FILE_ACC_TYPE_EXCHANGE)
        {
          char* pszFirstFile = strtok(lpszArg[i]+3, "=");
          if (pszFirstFile != NULL)
            pNtDriverControl->AddToSaveList(pszFirstFile, dwFileAccessType, strtok(NULL, "="));
        }
        else
          pNtDriverControl->AddToSaveList(lpszArg[i]+3, dwFileAccessType);
        bShowProtectedFiles = TRUE;
      }
    }
    if (!strnicmp(lpszArg[i], "-d:", 3))
    {
      if (strlen(lpszArg[i]) > 3)
      {
        pNtDriverControl->DelFromSaveList(lpszArg[i]+3);
        bShowProtectedFiles = TRUE;
      }
    }
  }

  if (bShowProtectedFiles)
  {
    printf("\nProtected files list:\n");
    ShowProtectedFiles(pNtDriverControl);
  }

  if (bShowUnlockThreadInfo)
  {
    ShowUnlockThreads(pNtDriverControl);
  }
  
  if (bShowStatistic)
  {
    ShowStatistic(pNtDriverControl);
  }


  delete pNtDriverControl;

  return 0;
}

void ShowHelp(void)
{
  printf("\n\
   -h                - Show help\n\
   -i:n              - Install driver type: \n\
                       n = 0  - open exist driver (default)\n\
                       n != 0 - load new image driver force.\n\
   -s                - Show protected files\n\
   -u:process_name   - Add process to unlock list\n\
   -l:process_name   - Remove process from unlock list\n\
   -cp:_string_      - Access type for process:\n\
                       R - read\n\
                       W - write\n\
                       D - delete\n\
                       V - visible\n\
                       (Example: -c:RV)\n\
   -la               - Remove all processes from unlock list\n\
   -hk:n             - Hook/Unhook file system:\n\
                       n = 0  - unhook\n\
                       n = 1 - hook Zw*/Nt* func.\n\
                       n = 2 - hook DRIVER_OBJECT\n\
   -a:full_file_name - Add file to save list\n\
   -d:full_file_name - Delete file from save list\n\
   -da               - Delete all files from save list\n\
   -c:_string_       - Access type for file:\n\
                       R - read\n\
                       W - write\n\
                       D - delete\n\
                       V - visible\n\
                       E - exchange (hook method = 1 (-hk:1))\n\
                       (Example: -c:RV)\n\
   -t                - Show unlock thread info\n\
   -q                - Show statistic\n\
   Examples:\n\
   He4HookControl.exe -a:c:\\MyFile -c:RV\n\
   He4HookControl.exe -a:c:\\MyFile=c:\\MyFileNew -c:ERV\n"
        );
}

void AddProcessToUnLockList(char* pszProcessFileName, He4HookDriverHide* pNtDriverControl)
{
  if (pszProcessFileName == NULL || pNtDriverControl == NULL)
    return;

  NTSTATUS           NtStatus;
  PROCESS_INFO       ProcInfo[1024];
  PPROCESS_INFO      lpProcInfo;
  ULONG              LehgthReturned = 0;
//  ULONG              i;

  NtStatus = NtGetProcessList(ProcInfo, sizeof(ProcInfo), &LehgthReturned);
  if (NtStatus == STATUS_SUCCESS)
  {
    lpProcInfo = ProcInfo;
    EnableDebugPriv();
    char szProcName[1024];
    while (1)
    {
      if (lpProcInfo->ProcessName.Buffer != NULL)
      {
        WideCharToMultiByte(CP_OEMCP, 0, (LPCWSTR)lpProcInfo->ProcessName.Buffer, -1,
                            szProcName, sizeof(szProcName), NULL, NULL);
      
        if (stricmp(szProcName, pszProcessFileName) == 0)
        {
          //PTHREAD_INFO pThreadInfo = GetThreadInfoPtr(lpProcInfo);
          //for (i=0; i<lpProcInfo->dwThreadCount; i++)
          //{
          //  pNtDriverControl->UnLockSaveObjects(dwProcessAccessType, pThreadInfo[i].dwThreadID);
          //}
          pNtDriverControl->UnLockSaveObjects(dwProcessAccessType | HE4_UNLOCK_FOR_PROCESS, lpProcInfo->dwProcessID);
        }
      }
      if (lpProcInfo->dwOffset == 0) 
        break;
      lpProcInfo = (PPROCESS_INFO)((CHAR*)lpProcInfo + lpProcInfo->dwOffset);
    }
  }
}

void DeleteProcessFromUnLockList(char* pszProcessFileName, He4HookDriverHide* pNtDriverControl)
{
  if (pszProcessFileName == NULL || pNtDriverControl == NULL)
    return;

  NTSTATUS           NtStatus;
  PROCESS_INFO       ProcInfo[1024];
  PPROCESS_INFO      lpProcInfo;
  ULONG              LehgthReturned = 0;
//  ULONG              i;

  NtStatus = NtGetProcessList(ProcInfo, sizeof(ProcInfo), &LehgthReturned);
  if (NtStatus == STATUS_SUCCESS)
  {
    lpProcInfo = ProcInfo;
    EnableDebugPriv();
    char szProcName[1024];
    while (1)
    {
      if (lpProcInfo->ProcessName.Buffer != NULL)
      {
        WideCharToMultiByte(CP_OEMCP, 0, (LPCWSTR)lpProcInfo->ProcessName.Buffer, -1,
                            szProcName, sizeof(szProcName), NULL, NULL);
      
        if (stricmp(szProcName, pszProcessFileName) == 0)
        {
          //PTHREAD_INFO pThreadInfo = GetThreadInfoPtr(lpProcInfo);
          //for (i=0; i<lpProcInfo->dwThreadCount; i++)
          //{
          //  pNtDriverControl->LockSaveObjects(pThreadInfo[i].dwThreadID);
          //}
          pNtDriverControl->LockSaveObjects(lpProcInfo->dwProcessID, TRUE);
        }
      }
      if (lpProcInfo->dwOffset == 0) 
        break;
      lpProcInfo = (PPROCESS_INFO)((CHAR*)lpProcInfo + lpProcInfo->dwOffset);
    }
  }
}


BOOL EnableDebugPriv(VOID)
{
  HANDLE hToken;
  LUID DebugValue;
  TOKEN_PRIVILEGES tkp;


  //
  // Retrieve a handle of the access token
  //
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
  {
     printf("   OpenProcessToken failed");
     return FALSE;
  }

  //
  // Enable the SE_DEBUG_NAME privilege
  //
  if (!LookupPrivilegeValue((LPSTR) NULL, SE_DEBUG_NAME, &DebugValue)) 
  {
     //printf("LookupPrivilegeValue failed with - \n");
     printf("   LookupPrivilegeValue failed");
     return FALSE;
  }

  tkp.PrivilegeCount = 1;
  tkp.Privileges[0].Luid = DebugValue;
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL);

  //
  // The return value of AdjustTokenPrivileges can't be tested
  //
  if (GetLastError() != ERROR_SUCCESS) 
  {
     //printf("AdjustTokenPrivileges failed with -\n");
     ConsoleErrorMessage();
     return FALSE;
  }

  return TRUE;
}


void ShowProtectedFiles(He4HookDriverHide* pNtDriverControl)
{
  if (pNtDriverControl == NULL)
    return;

  DWORD dwSizeListByBytes = pNtDriverControl->GetSaveListSizeByBytes();
  if (dwSizeListByBytes)
  {
    FILEINFOSET *fis = (FILEINFOSET*) new char[dwSizeListByBytes+sizeof(DWORD)];
    if (fis)
    {
      fis->dwSize = dwSizeListByBytes+sizeof(DWORD);
      if (pNtDriverControl->GetSaveList(fis))
      {
        FILEINFO* fi;
        fi = fis->FileInfo;
        while (fis->dwSize >= SIZEOF_FILEINFOSET)
        {
          if (fi->dwSizeAnsiName > 1)
          {
            char  szAccessType[10];
            char* pszAccessType = szAccessType;
            if (fi->dwAccessType & ACC_TYPE_READ)
              *pszAccessType++ = 'R';
            if (fi->dwAccessType & ACC_TYPE_WRITE)
              *pszAccessType++ = 'W';
            if (fi->dwAccessType & ACC_TYPE_DELETE)
              *pszAccessType++ = 'D';
            if (fi->dwAccessType & ACC_TYPE_VISIBLE)
              *pszAccessType++ = 'V';
            if (fi->dwAccessType & FILE_ACC_TYPE_EXCHANGE)
              *pszAccessType++ = 'E';
            if (pszAccessType == szAccessType)
              *pszAccessType++ = '0';
            *pszAccessType = 0;
            
            if (fi->dwAccessType & FILE_ACC_TYPE_EXCHANGE)
            {
              printf("%s (%s) => ", fi->szNames + fi->dwOffsetToAnsiName, szAccessType);
              wprintf(L"%s \n", fi->szNames + fi->dwOffsetToUniChangedName);
            }
            else
              printf("%s (%s)\n", fi->szNames + fi->dwOffsetToAnsiName, szAccessType);

          }
          if (fis->dwSize >= (SIZEOF_FILEINFO_REAL + fi->dwSizeAllNamesArea))
            fis->dwSize -= (SIZEOF_FILEINFO_REAL + fi->dwSizeAllNamesArea);
          else
            fis->dwSize = 0;
          fi = (FILEINFO*)((char*)fi + SIZEOF_FILEINFO_REAL + fi->dwSizeAllNamesArea);
        }
      }

      delete[] (char*)fis;
    }
  }
}

void ShowUnlockThreads(He4HookDriverHide* pNtDriverControl)
{
  BOOL bThread;

  if (pNtDriverControl == NULL)
    return;

  DWORD dwSizeListByBytes = pNtDriverControl->GetUnlockListSizeByBytes();
  if (dwSizeListByBytes)
  {
    PUNLOCK_CLIENT_INFO_SET tis = (PUNLOCK_CLIENT_INFO_SET) new char[dwSizeListByBytes+sizeof(DWORD)];
    if (tis)
    {
      tis->m_dwSize = dwSizeListByBytes+sizeof(DWORD);
      if (pNtDriverControl->GetUnlockList(tis))
      {
        UNLOCK_CLIENT_INFO* ti;
        ti = tis->m_CI;
        while (tis->m_dwSize >= SIZEOF_UNLOCK_CLIENT_INFO_SET)
        {
          char  szAccessType[10];
          char* pszAccessType = szAccessType;
          if (ti->m_dwUnlockFlags & ACC_TYPE_READ)
            *pszAccessType++ = 'R';
          if (ti->m_dwUnlockFlags & ACC_TYPE_WRITE)
            *pszAccessType++ = 'W';
          if (ti->m_dwUnlockFlags & ACC_TYPE_DELETE)
            *pszAccessType++ = 'D';
          if (ti->m_dwUnlockFlags & ACC_TYPE_VISIBLE)
            *pszAccessType++ = 'V';
          if (pszAccessType == szAccessType)
            *pszAccessType++ = '0';
          *pszAccessType = 0;
          
          if (ti->m_dwUnlockFlags & HE4_UNLOCK_FOR_PROCESS)
            bThread = FALSE;
          else
            bThread = TRUE;
          printf("Client Id = %0x (%s) (%s)\n", ti->m_dwClientId, (bThread != TRUE ? "Process" : "Thread"), szAccessType);

          if (tis->m_dwSize >= sizeof(UNLOCK_CLIENT_INFO))
            tis->m_dwSize -= sizeof(UNLOCK_CLIENT_INFO);
          else
            tis->m_dwSize = 0;
          ti++;
        }
      }

      delete[] (char*)tis;
    }
  }
}

void ConsoleErrorMessage(void)
{
  LPTSTR MsgBuf;
//  char   lpMultiByteStr[1024];

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                  (LPTSTR) &MsgBuf,
                  0,
                  NULL
                );
  //MessageBox(GetForegroundWindow(), MsgBuf, _T("Error"), MB_OK);
//  WideCharToMultiByte(CP_OEMCP, 0, (LPCWSTR)MsgBuf, -1,
//                      lpMultiByteStr, sizeof(lpMultiByteStr), NULL, NULL);
//  printf("%s\n", lpMultiByteStr);
  printf("%s\n", (char*)MsgBuf);
  LocalFree(MsgBuf);
}

DWORD GetVersion(DWORD *pdwVer)
{
  void** pParameterStack = (void **) &pdwVer;
  DWORD  dwRet = -1;
  DWORD  dwSericeId = (KE_SERVICE_TABLE_INDEX<<12) + HE4_SERVICE_INDEX_GET_VERSION; //00002000h

  __asm 
  { 
    mov  eax, [dwSericeId]
    mov  edx, pParameterStack
    int  2eh
    mov  [dwRet], eax
  }
  return dwRet;
}

DWORD GetLocalBase(DWORD *pdwBase)
{
  void** pParameterStack = (void **) &pdwBase;
  DWORD  dwRet = -1;
  DWORD  dwSericeId = (KE_SERVICE_TABLE_INDEX<<12) + HE4_SERVICE_INDEX_GET_LOCAL_BASE; //00002001h

  __asm 
  { 
    mov  eax, [dwSericeId]
    mov  edx, pParameterStack
    int  2eh
    mov  [dwRet], eax
  }
  return dwRet;
}

void ShowDeviceCurrentVersion(void)
{
  DWORD dwVersion = 0;
  DWORD dwBase = 0;

  GetVersion(&dwVersion);
  GetLocalBase(&dwBase);

  if (dwVersion != 0 && dwBase != 0)
    printf("He4HooInv device installed - \n     Version: %08X\n     Base: %08X\n", dwVersion, dwBase);
  else
    printf("He4HooInv device not installed\n");
}

BOOL InstallNewDriver(He4HookDriverHide* pNtDriverControl)
{
  if (pNtDriverControl == NULL)
    return FALSE;

  DWORD dwVersionOld = 0;
  DWORD dwBaseOld = 0;

  GetVersion(&dwVersionOld);
  GetLocalBase(&dwBaseOld);

  FILEINFOSET* fis = NULL;
  BOOL         bGetSaveList = FALSE;
  DWORD        dwSizeListByBytes = pNtDriverControl->GetSaveListSizeByBytes();
  if (dwSizeListByBytes)
  {
    fis = (FILEINFOSET*) new char[dwSizeListByBytes+sizeof(DWORD)];
    if (fis)
    {
      fis->dwSize = dwSizeListByBytes+sizeof(DWORD);
      bGetSaveList = pNtDriverControl->GetSaveList(fis);
    }
  }

  //if (pNtDriverControl->Install() == FALSE) 
  if (pNtDriverControl->LoadAndCallImage() == FALSE)
  {
    if (fis != NULL)
      delete[] (char*)fis;
    return FALSE;
  }

  DWORD dwVersionNew = 0;
  DWORD dwBaseNew = 0;

  GetVersion(&dwVersionNew);
  GetLocalBase(&dwBaseNew);
  if (dwVersionNew == dwVersionOld && dwBaseNew == dwBaseOld)
  {
    if (fis != NULL)
      delete[] (char*)fis;
    return FALSE;
  }

  if (bGetSaveList == TRUE)
    pNtDriverControl->AddToSaveList(fis);

  if (fis != NULL)
    delete[] (char*)fis;

  return TRUE;
}

void ShowStatistic(He4HookDriverHide* pNtDriverControl)
{
  if (pNtDriverControl == NULL)
    return;

  HE4_STATISTIC_INFO StatInfo;
  PHEAP_INFO_SET     pHeapInfoSet;

  if (pNtDriverControl->QueryStatistic(&StatInfo) == TRUE)
  {
    pHeapInfoSet = &(StatInfo.m_HeapInfoSet);

    printf("\n m_DefaultHeapInfo:\n   SystemMemoryUsage = %u\n   HeapMemoryUsage = %u",
           pHeapInfoSet->m_DefaultHeapInfo.m_dwSystemMemoryUsage,
           pHeapInfoSet->m_DefaultHeapInfo.m_dwHeapMemoryUsage);

    printf("\n UnlockListHeapInfo:\n   SystemMemoryUsage = %u\n   HeapMemoryUsage = %u",
           pHeapInfoSet->m_UnlockListHeapInfo.m_dwSystemMemoryUsage,
           pHeapInfoSet->m_UnlockListHeapInfo.m_dwHeapMemoryUsage);
                                                             
    printf("\n FSDefaultHeapInfo:\n   SystemMemoryUsage = %u\n   HeapMemoryUsage = %u",
           pHeapInfoSet->m_FSDefaultHeapInfo.m_dwSystemMemoryUsage,
           pHeapInfoSet->m_FSDefaultHeapInfo.m_dwHeapMemoryUsage);
                                                             
    printf("\n SOFileListHeapInfo:\n   SystemMemoryUsage = %u\n   HeapMemoryUsage = %u",
           pHeapInfoSet->m_SOFileListHeapInfo.m_dwSystemMemoryUsage, 
           pHeapInfoSet->m_SOFileListHeapInfo.m_dwHeapMemoryUsage);
                                                             
    printf("\n LLDefaultHeapInfo:\n   SystemMemoryUsage = %u\n   HeapMemoryUsage = %u",
           pHeapInfoSet->m_LLDefaultHeapInfo.m_dwSystemMemoryUsage,
           pHeapInfoSet->m_LLDefaultHeapInfo.m_dwHeapMemoryUsage);
                                                             
    printf("\n MiscDefaultHeapInfo:\n   SystemMemoryUsage = %u\n   HeapMemoryUsage = %u",
           pHeapInfoSet->m_MiscDefaultHeapInfo.m_dwSystemMemoryUsage,
           pHeapInfoSet->m_MiscDefaultHeapInfo.m_dwHeapMemoryUsage);

    printf("\n DHDefaultHeapInfo:\n   SystemMemoryUsage = %u\n   HeapMemoryUsage = %u",
           pHeapInfoSet->m_DHDefaultHeapInfo.m_dwSystemMemoryUsage,
           pHeapInfoSet->m_DHDefaultHeapInfo.m_dwHeapMemoryUsage);

//    printf("\n BTreeDefaultHeapInfo:\n   SystemMemoryUsage = %u\n   HeapMemoryUsage = %u",
//           pHeapInfoSet->m_BTreeDefaultHeapInfo.m_dwSystemMemoryUsage,
//           pHeapInfoSet->m_BTreeDefaultHeapInfo.m_dwHeapMemoryUsage);
  }
}