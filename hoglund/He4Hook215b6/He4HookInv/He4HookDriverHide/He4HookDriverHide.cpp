#define STRICT
#include "He4hookDriverHide.hpp"

He4HookDriverHide::He4HookDriverHide(const TCHAR *lpszDeviceFileName)
                 : NtDriverControlHide(lpszDeviceFileName), pRtlNtStatusToDosError(NULL),
                   Result(FALSE)
{
  if (!NtDriverControlHide::Result)
    return;

  GetNtProcAddresses();

  Result = TRUE;
}

He4HookDriverHide::~He4HookDriverHide()
{
}

BOOL He4HookDriverHide::GetNtProcAddresses()
{
  if (pRtlNtStatusToDosError == NULL)
  {
    HMODULE hNtDll = GetModuleHandle("ntdll.dll");
    if (hNtDll != NULL)
    {
      pRtlNtStatusToDosError = (RTLNTSTATUSTODOSERROR)GetProcAddress(hNtDll, "RtlNtStatusToDosError");
      if (IsBadCodePtr((FARPROC)pRtlNtStatusToDosError))
        pRtlNtStatusToDosError = NULL;
    }
  }

  return (BOOL)pRtlNtStatusToDosError;
}

DWORD He4HookDriverHide::ZwDispatchFunction(DWORD dwProcessId, DWORD dwThreadId, DWORD IoControlCode,
                                            PVOID InputBuffer, DWORD InputBufferLength,
                                            PVOID OutputBuffer, DWORD OutputBufferLength, 
                                            DWORD *lpBytesReturned)
{
  void**   pParameterStack = (void**) &dwProcessId;
  DWORD    dwRet = -1;
  DWORD    dwSericeId = (KE_SERVICE_TABLE_INDEX<<12) + HE4_SERVICE_INDEX_DISPATCH_FUNCTION;

  __asm
  {
    mov  eax, [dwSericeId]     
    mov  edx, pParameterStack
    int  2eh
    mov  [dwRet], eax
  }

  return dwRet;
}

BOOL He4HookDriverHide::SendCommand(USER_COMMAND *lpUserCommand)
{
  if (IsBadReadPtr(lpUserCommand, sizeof(USER_COMMAND)))
    return FALSE;

  if (
      lpUserCommand->m_dwInBufferSize != 0 &&
      IsBadReadPtr(lpUserCommand->m_lpInBuffer, lpUserCommand->m_dwInBufferSize)
     )
    return FALSE;

  if (
      lpUserCommand->m_dwOutBufferSize != 0 &&
      IsBadWritePtr(lpUserCommand->m_lpOutBuffer, lpUserCommand->m_dwOutBufferSize)
     )
    return FALSE;

  __try
  {
    DWORD NtStatus = ZwDispatchFunction(0, 0,
                                        lpUserCommand->m_dwCommand,
                                        lpUserCommand->m_lpInBuffer, lpUserCommand->m_dwInBufferSize,
                                        lpUserCommand->m_lpOutBuffer, lpUserCommand->m_dwOutBufferSize,
                                        &lpUserCommand->m_dwBytesReturned);
    if (GetNtProcAddresses())
    {
      SetLastError(pRtlNtStatusToDosError(NtStatus));
    }
    if (NtStatus)
    {
      DriverErrorMessage();
      return FALSE;
    }
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    _DebugMessage("BOOL He4HookDriverHide::SendCommand(USER_COMMAND *lpUserCommand)");
    return FALSE;
  }

  return TRUE;
}

DWORD He4HookDriverHide::GetVersion()
{
  USER_COMMAND UserCommand;
  DWORD        dwVer = 0;

  UserCommand.m_dwCommand = HE4_DEVICE_VERSION;
  UserCommand.m_lpOutBuffer = &dwVer;
  UserCommand.m_dwOutBufferSize = sizeof(DWORD);

  if (!SendCommand(&UserCommand)) 
    return 0;
  return dwVer;
}

DWORD He4HookDriverHide::HookFileSystem(DWORD dwHook)
{
  USER_COMMAND  UserCommand;
  DWORD         dwHookResult = 0x0;


  UserCommand.m_dwCommand = HE4_HOOK_FILE_SYSTEM;
  UserCommand.m_lpInBuffer = &dwHook;
  UserCommand.m_dwInBufferSize = sizeof(DWORD);
  UserCommand.m_lpOutBuffer = &dwHookResult;
  UserCommand.m_dwOutBufferSize = sizeof(DWORD);

  if (!SendCommand(&UserCommand))
    return (DWORD)-1;
  return dwHookResult;
}

BOOL He4HookDriverHide::LockSaveObjectsForAllThreads()
{
  USER_COMMAND       UserCommand;

  UserCommand.m_dwCommand = HE4_LOCK_SAVE_FILES_FOR_ALL_THREADS;

  if (!SendCommand(&UserCommand)) 
    return FALSE;
  return (BOOL) UserCommand.m_dwBytesReturned;
}

BOOL He4HookDriverHide::LockSaveObjects(DWORD dwClientId, BOOL bForProcess)
{
  USER_COMMAND       UserCommand;
  HE4_LOCK_SETTING   LockSetting;

  LockSetting.m_dwClientId = dwClientId;
  LockSetting.m_dwForProcess = (bForProcess == TRUE) ? 1 : 0;

  UserCommand.m_dwCommand = HE4_LOCK_SAVE_FILES;
  UserCommand.m_lpInBuffer = &LockSetting;
  UserCommand.m_dwInBufferSize = sizeof(HE4_LOCK_SETTING);
  UserCommand.m_lpOutBuffer = NULL;
  UserCommand.m_dwOutBufferSize = 0;
  UserCommand.m_dwBytesReturned = 0;
  if (!SendCommand(&UserCommand)) 
    return FALSE;
  return (BOOL) UserCommand.m_dwBytesReturned;
}

BOOL He4HookDriverHide::UnLockSaveObjects(DWORD dwUnlockFlags, DWORD dwClientId)
{
  USER_COMMAND         UserCommand;
  HE4_UNLOCK_SETTING   UnlockSetting;

  UnlockSetting.m_dwClientId = dwClientId;
  UnlockSetting.m_dwUnlockFlags = dwUnlockFlags;

  UserCommand.m_dwCommand = HE4_UNLOCK_SAVE_FILES;
  UserCommand.m_lpInBuffer = &UnlockSetting;
  UserCommand.m_dwInBufferSize = sizeof(HE4_UNLOCK_SETTING);
  UserCommand.m_lpOutBuffer = NULL;
  UserCommand.m_dwOutBufferSize = 0;
  UserCommand.m_dwBytesReturned = 0;
  if (!SendCommand(&UserCommand)) 
    return FALSE;
  return (BOOL) UserCommand.m_dwBytesReturned;
}

DWORD He4HookDriverHide::GetUnlockListSizeByBytes()
{
  USER_COMMAND UserCommand;
  DWORD        dwSize = 0;

  UserCommand.m_dwCommand = HE4_GET_SIZE_BY_BYTES_UNLOCK_LIST;
  UserCommand.m_lpOutBuffer = &dwSize;
  UserCommand.m_dwOutBufferSize = sizeof(DWORD);

  if (!SendCommand(&UserCommand))
    return 0;
  return dwSize;
}

BOOL He4HookDriverHide::GetUnlockList(PUNLOCK_CLIENT_INFO_SET pUnlockInfoSet)
{
  if (IsBadWritePtr(pUnlockInfoSet, SIZEOF_UNLOCK_CLIENT_INFO_SET) || IsBadReadPtr(pUnlockInfoSet, SIZEOF_UNLOCK_CLIENT_INFO_SET))
    return FALSE;

  USER_COMMAND UserCommand;

  UserCommand.m_dwCommand = HE4_GET_UNLOCK_LIST;
  UserCommand.m_lpOutBuffer = pUnlockInfoSet;
  UserCommand.m_dwOutBufferSize = pUnlockInfoSet->m_dwSize;
  UserCommand.m_dwBytesReturned = 0;
  if (SendCommand(&UserCommand)) 
  { 
    return (BOOL) UserCommand.m_dwBytesReturned;
  }

  return FALSE;
}

BOOL He4HookDriverHide::AddToSaveList(char* pszFileName, DWORD dwAccessType, char* pszChangedName)
{
  if (
      pszFileName == NULL ||
      ((dwAccessType & FILE_ACC_TYPE_EXCHANGE) && pszChangedName == NULL)
     )
    return FALSE;

  W32_FILEINFOSET  FileInfoSet;
  W32_FILEINFO     FileInfo;

  FileInfoSet.dwSize = 1;
  FileInfoSet.lpFileInfo = &FileInfo;

  FileInfo.lpszName = pszFileName;
  FileInfo.lpszChangedName = pszChangedName;
  FileInfo.dwAccessType = dwAccessType;

  return AddToSaveList(&FileInfoSet);
}

BOOL He4HookDriverHide::AddToSaveList(PW32_FILEINFOSET lpFileInfoSetW32)
{
  USER_COMMAND    UserCommand;
  BOOL            bRes = FALSE;

  AddShortName(lpFileInfoSetW32);

  PFILEINFOSET lpFileInfoSet = CreateFileInfoSet(lpFileInfoSetW32);
  if (lpFileInfoSet)
  {
    bRes = AddToSaveList(lpFileInfoSet);
    delete[] (char*)lpFileInfoSet;
  }
  
  return bRes;
}

BOOL He4HookDriverHide::AddToSaveList(PFILEINFOSET lpFileInfoSet)
{
  USER_COMMAND    UserCommand;

  if (lpFileInfoSet)
  {
    UserCommand.m_dwCommand = HE4_ADD_TO_SAVE_LIST;
    UserCommand.m_lpInBuffer = lpFileInfoSet;
    UserCommand.m_dwInBufferSize = lpFileInfoSet->dwSize;
    UserCommand.m_dwBytesReturned = 0;
    if (SendCommand(&UserCommand)) 
      return (BOOL) UserCommand.m_dwBytesReturned;
  }

  return FALSE;
}

BOOL He4HookDriverHide::DelFromSaveList(char* pszFileName)
{
  if (pszFileName == NULL)
    return FALSE;

  W32_FILEINFOSET  FileInfoSet;
  W32_FILEINFO     FileInfo;

  FileInfoSet.dwSize = 1;
  FileInfoSet.lpFileInfo = &FileInfo;

  FileInfo.lpszName = pszFileName;

  return DelFromSaveList(&FileInfoSet);
}


BOOL He4HookDriverHide::DelFromSaveList(PW32_FILEINFOSET lpFileInfoSetW32)
{
  USER_COMMAND    UserCommand;

  UnLockSaveObjects();
  DelShortName(lpFileInfoSetW32);
  LockSaveObjects();

  PFILEINFOSET lpFileInfoSet = CreateFileInfoSet(lpFileInfoSetW32);

  if (lpFileInfoSet)
  {
    UserCommand.m_dwCommand = HE4_DEL_FROM_SAVE_LIST;
    UserCommand.m_lpInBuffer = lpFileInfoSet;
    UserCommand.m_dwInBufferSize = lpFileInfoSet->dwSize;
    UserCommand.m_dwBytesReturned = 0;
    if (SendCommand(&UserCommand)) 
    { 
      delete[] (char*)lpFileInfoSet;
      return (BOOL) UserCommand.m_dwBytesReturned;
    }
    delete[] (char*)lpFileInfoSet;
  }
  return FALSE;
}

BOOL He4HookDriverHide::ClearSaveList()
{
  USER_COMMAND    UserCommand;

  UserCommand.m_dwCommand = HE4_CLEAR_SAVE_LIST;
  UserCommand.m_dwBytesReturned = 0;
  SendCommand(&UserCommand);

  return (BOOL) UserCommand.m_dwBytesReturned;
}

DWORD He4HookDriverHide::GetSaveListSizeByBytes()
{
  USER_COMMAND UserCommand;
  DWORD        dwSize = 0;

  UserCommand.m_dwCommand = HE4_GET_SIZE_BY_BYTES_SAVE_LIST;
  UserCommand.m_lpOutBuffer = &dwSize;
  UserCommand.m_dwOutBufferSize = sizeof(DWORD);

  if (!SendCommand(&UserCommand))
    return 0;
  return dwSize;
}

BOOL He4HookDriverHide::GetSaveList(PFILEINFOSET pFileInfoSet)
{
  if (IsBadWritePtr(pFileInfoSet, SIZEOF_FILEINFOSET) || IsBadReadPtr(pFileInfoSet, SIZEOF_FILEINFOSET))
    return FALSE;

  USER_COMMAND UserCommand;

  UserCommand.m_dwCommand = HE4_GET_SAVE_LIST;
  UserCommand.m_lpOutBuffer = pFileInfoSet;
  UserCommand.m_dwOutBufferSize = pFileInfoSet->dwSize;
  UserCommand.m_dwBytesReturned = 0;
  if (SendCommand(&UserCommand)) 
  { 
    return (BOOL) UserCommand.m_dwBytesReturned;
  }

  return FALSE;
}

BOOL He4HookDriverHide::QueryStatistic(PHE4_STATISTIC_INFO pStatInfo)
{
  USER_COMMAND         UserCommand;

  UserCommand.m_dwCommand = HE4_QUERY_STATISTIC;
  UserCommand.m_lpInBuffer = NULL;
  UserCommand.m_dwInBufferSize = 0;
  UserCommand.m_lpOutBuffer = pStatInfo;
  UserCommand.m_dwOutBufferSize = sizeof(HE4_STATISTIC_INFO);
  UserCommand.m_dwBytesReturned = 0;
  if (!SendCommand(&UserCommand)) 
    return FALSE;
  return (BOOL) UserCommand.m_dwBytesReturned;
}

DWORD He4HookDriverHide::QueryUnload()
{
  USER_COMMAND    UserCommand;

  UserCommand.m_dwCommand = HE4_QUERY_UNLOAD;
  UserCommand.m_lpInBuffer = NULL;
  UserCommand.m_dwInBufferSize = 0;
  UserCommand.m_dwBytesReturned = 0;
  if (!SendCommand(&UserCommand)) 
    return (DWORD)-1;
  return UserCommand.m_dwBytesReturned;
}

BOOL He4HookDriverHide::AddKeysToSaveList(PW32_KEYINFOSET lpKeyInfoSetW32)
{
  USER_COMMAND    UserCommand;

  PKEYINFOSET lpKeyInfoSet = CreateKeyInfoSet(lpKeyInfoSetW32);

  if (lpKeyInfoSet)
  {
    UserCommand.m_dwCommand = HE4_ADD_KEYS_TO_SAVE_LIST;
    UserCommand.m_lpInBuffer = lpKeyInfoSet;
    UserCommand.m_dwInBufferSize = lpKeyInfoSet->dwSize;
    UserCommand.m_dwBytesReturned = 0;
    if (SendCommand(&UserCommand)) 
    {
      delete[] (char*)lpKeyInfoSet;
      return (BOOL) UserCommand.m_dwBytesReturned;
    }
    delete[] (char*)lpKeyInfoSet;
  }

  return FALSE;
}

BOOL He4HookDriverHide::DelKeysFromSaveList(PW32_KEYINFOSET lpKeyInfoSetW32)
{
  USER_COMMAND    UserCommand;

  PKEYINFOSET lpKeyInfoSet = CreateKeyInfoSet(lpKeyInfoSetW32);

  if (lpKeyInfoSet)
  {
    UserCommand.m_dwCommand = HE4_DEL_KEYS_FROM_SAVE_LIST;
    UserCommand.m_lpInBuffer = lpKeyInfoSet;
    UserCommand.m_dwInBufferSize = lpKeyInfoSet->dwSize;
    UserCommand.m_dwBytesReturned = 0;
    if (SendCommand(&UserCommand)) 
    {
      delete[] (char*)lpKeyInfoSet;
      return (BOOL) UserCommand.m_dwBytesReturned;
    }
    delete[] (char*)lpKeyInfoSet;
  }

  return FALSE;
}

BOOL He4HookDriverHide::HookRegistry()
{
  USER_COMMAND    UserCommand;

  UserCommand.m_dwCommand = HE4_HOOK_REGISTRY;
  UserCommand.m_lpInBuffer = NULL;
  UserCommand.m_dwInBufferSize = 0;
  UserCommand.m_lpOutBuffer = NULL;
  UserCommand.m_dwOutBufferSize = 0;
  UserCommand.m_dwBytesReturned = 0;
  if (!SendCommand(&UserCommand)) 
    return FALSE;
  return (BOOL) UserCommand.m_dwBytesReturned;
}

BOOL He4HookDriverHide::UnHookRegistry()
{
  USER_COMMAND    UserCommand;

  UserCommand.m_dwCommand = HE4_UNHOOK_REGISTRY;
  UserCommand.m_lpInBuffer = NULL;
  UserCommand.m_dwInBufferSize = 0;
  UserCommand.m_lpOutBuffer = NULL;
  UserCommand.m_dwOutBufferSize = 0;
  UserCommand.m_dwBytesReturned = 0;
  if (!SendCommand(&UserCommand)) 
    return FALSE;
  return (BOOL) UserCommand.m_dwBytesReturned;
}

BOOL He4HookDriverHide::AddShortName(PW32_FILEINFOSET lpFileInfoSetW32)
{
  if (!lpFileInfoSetW32)
    return FALSE;

  USER_COMMAND    UserCommand;
  W32_FILEINFOSET FileInfoSet;
  W32_FILEINFO    FileInfo;
  char           *lpszShortName = 0;
  char           *lpwszShortChangedName = 0;
  DWORD           dwRes;

  FileInfoSet.dwSize = 1;
  FileInfoSet.lpFileInfo = &FileInfo;

  //UserCommand.m_dwCommand = HE4_ADD_TO_SAVE_LIST;
  //UserCommand.m_lpInBuffer = &FileInfoSet;
  //UserCommand.m_dwInBufferSize = sizeof(FILEINFOSET);

  for (int i=0; i<(int)lpFileInfoSetW32->dwSize; i++)
  {
    if (!lpFileInfoSetW32->lpFileInfo[i].lpszName)
      continue;
    if ((lpFileInfoSetW32->lpFileInfo[i].dwAccessType & FILE_ACC_TYPE_EXCHANGE) && (!lpFileInfoSetW32->lpFileInfo[i].lpszChangedName))
      continue;

    lpszShortName = new char[lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszName)+sizeof(char)];
    if (!lpszShortName)
      continue;
//    lstrcpy(&lpszShortName[1], lpFileInfoSetW32->lpFileInfo[i].lpszName);
//    lpszShortName[0] = 'f';
    dwRes = GetShortPathName(lpFileInfoSetW32->lpFileInfo[i].lpszName, lpszShortName, lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszName)+sizeof(char));
    if (!dwRes)
    {
      delete[] lpszShortName;
      lpszShortName = 0; 
      continue;
    }
//    memcpy(&lpszShortName[0], &lpszShortName[1], lstrlen(lpszShortName));
    if (lstrlen(lpszShortName) == lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszName))
    {
      delete[] lpszShortName;
      lpszShortName = 0; 
      continue;
    }
    if (lpFileInfoSetW32->lpFileInfo[i].dwAccessType & FILE_ACC_TYPE_EXCHANGE)
    {
      lpwszShortChangedName = new char[lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)+sizeof(char)];
      if (!lpwszShortChangedName)
      {
        delete[] lpszShortName;
        lpszShortName = 0; 
        continue;
      }
      dwRes = GetShortPathName((char*)lpFileInfoSetW32->lpFileInfo[i].lpszChangedName, lpwszShortChangedName, lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)+sizeof(char));
      if (!dwRes)
      {
        delete[] lpszShortName;
        lpszShortName = 0; 
        delete[] lpwszShortChangedName;
        lpwszShortChangedName = 0; 
        continue;
      }
    }
    
    FileInfo.lpszName = lpszShortName;         
    FileInfo.lpszChangedName = lpwszShortChangedName; 
    FileInfo.dwAccessType = lpFileInfoSetW32->lpFileInfo[i].dwAccessType;

    PFILEINFOSET pFileInfo = CreateFileInfoSet(&FileInfoSet);
    if (pFileInfo)
    {
      UserCommand.m_dwCommand = HE4_ADD_TO_SAVE_LIST;
      UserCommand.m_lpInBuffer = pFileInfo;
      UserCommand.m_dwInBufferSize = pFileInfo->dwSize;
      UserCommand.m_dwBytesReturned = 0;
      SendCommand(&UserCommand);
      delete[] (char*)pFileInfo;
    }

    delete[] lpszShortName;
    lpszShortName = 0; 
    delete[] lpwszShortChangedName;
    lpwszShortChangedName = 0; 
  }

  return TRUE;
}

BOOL He4HookDriverHide::DelShortName(PW32_FILEINFOSET lpFileInfoSetW32)
{
  if (!lpFileInfoSetW32)
    return FALSE;

  USER_COMMAND    UserCommand;
  W32_FILEINFOSET FileInfoSet;
  W32_FILEINFO    FileInfo;
  char           *lpszShortName = 0;
  char           *lpwszShortChangedName = 0;
  DWORD           dwRes;

  FileInfoSet.dwSize = 1;
  FileInfoSet.lpFileInfo = &FileInfo;

  //UserCommand.m_dwCommand = HE4_DEL_FROM_SAVE_LIST;
  //UserCommand.m_lpInBuffer = &FileInfoSet;
  //UserCommand.m_dwInBufferSize = sizeof(FILEINFOSET);

  for (int i=0; i<(int)lpFileInfoSetW32->dwSize; i++)
  {
    if (!lpFileInfoSetW32->lpFileInfo[i].lpszName)
      continue;
    if ((lpFileInfoSetW32->lpFileInfo[i].dwAccessType & FILE_ACC_TYPE_EXCHANGE) && (!lpFileInfoSetW32->lpFileInfo[i].lpszChangedName))
      continue;

    lpszShortName = new char[lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszName)+sizeof(char)];
    if (!lpszShortName)
      continue;
//    lstrcpy(&lpszShortName[1], lpFileInfoSetW32->lpFileInfo[i].lpszName);
//    lpszShortName[0] = 'c';
    dwRes = GetShortPathName(lpFileInfoSetW32->lpFileInfo[i].lpszName, lpszShortName, lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszName)+sizeof(char));
    if (!dwRes)
    {
      delete[] lpszShortName;
      lpszShortName = 0; 
      continue;
    }
//    memcpy(&lpszShortName[0], &lpszShortName[1], lstrlen(lpszShortName));
    if (lstrlen(lpszShortName) == lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszName))
    {
      delete[] lpszShortName;
      lpszShortName = 0; 
      continue;
    }
    if (lpFileInfoSetW32->lpFileInfo[i].dwAccessType & FILE_ACC_TYPE_EXCHANGE)
    {
      lpwszShortChangedName = new char[lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)+sizeof(char)];
      if (!lpwszShortChangedName)
      {
        delete[] lpszShortName;
        lpszShortName = 0; 
        continue;
      }
      dwRes = GetShortPathName((char*)lpFileInfoSetW32->lpFileInfo[i].lpszChangedName, lpwszShortChangedName, lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)+sizeof(char));
      if (!dwRes)
      {
        delete[] lpszShortName;
        lpszShortName = 0; 
        delete[] lpwszShortChangedName;
        lpwszShortChangedName = 0; 
        continue;
       }
    }
    
    FileInfo.lpszName = lpszShortName;         
    FileInfo.lpszChangedName = lpwszShortChangedName; 
    FileInfo.dwAccessType = lpFileInfoSetW32->lpFileInfo[i].dwAccessType;

    PFILEINFOSET pFileInfo = CreateFileInfoSet(&FileInfoSet);
    if (pFileInfo)
    {
      UserCommand.m_dwCommand = HE4_DEL_FROM_SAVE_LIST;
      UserCommand.m_lpInBuffer = pFileInfo;
      UserCommand.m_dwInBufferSize = pFileInfo->dwSize;
      UserCommand.m_dwBytesReturned = 0;
      SendCommand(&UserCommand);
      delete[] (char*)pFileInfo;
    }
    
    delete[] lpszShortName;
    lpszShortName = 0; 
    delete[] lpwszShortChangedName;
    lpwszShortChangedName = 0; 
  }

  return TRUE;
}

PFILEINFOSET He4HookDriverHide::CreateFileInfoSet(PW32_FILEINFOSET lpFileInfoSetW32)
{
  if(!lpFileInfoSetW32)
     return FALSE;

  DWORD dwSizeOfArea = SIZEOF_FILEINFOSET - SIZEOF_FILEINFO;

  for (int i=0; i<(int)lpFileInfoSetW32->dwSize; i++)
  {
    if (lpFileInfoSetW32->lpFileInfo[i].lpszName)
    {
      dwSizeOfArea += SIZEOF_FILEINFO - sizeof(char);
      dwSizeOfArea += lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszName) + sizeof(char);
      if (lpFileInfoSetW32->lpFileInfo[i].dwAccessType & FILE_ACC_TYPE_EXCHANGE)
      {
        if (!lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)
        {
          lpFileInfoSetW32->lpFileInfo[i].dwAccessType &= ~FILE_ACC_TYPE_EXCHANGE;
        }
        else
        {
          dwSizeOfArea += lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszChangedName) + sizeof(char);
        }
      }
      else
      {
        lpFileInfoSetW32->lpFileInfo[i].lpszChangedName = NULL;
      }
    }
  }

  PFILEINFOSET pFileInfoSet = NULL;
  if (dwSizeOfArea > SIZEOF_FILEINFOSET - SIZEOF_FILEINFO)
  {
    pFileInfoSet = (PFILEINFOSET ) new char[dwSizeOfArea];
    if (pFileInfoSet)
    {
      memset(pFileInfoSet, 0, dwSizeOfArea);
      pFileInfoSet->dwSize = dwSizeOfArea;
      PFILEINFO pFileInfo = &pFileInfoSet->FileInfo[0];
      DWORD     dwSizeNames;
      for (int i=0; i<(int)lpFileInfoSetW32->dwSize; i++)
      {
        if (lpFileInfoSetW32->lpFileInfo[i].lpszName)
        {
          pFileInfo->dwAccessType = lpFileInfoSetW32->lpFileInfo[i].dwAccessType;

          dwSizeNames = lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszName)+sizeof(char);
          if (lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)
            dwSizeNames += lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)+sizeof(char);

          pFileInfo->dwSizeAllNamesArea = dwSizeNames;

          pFileInfo->dwOffsetToAnsiName = 0;
          pFileInfo->dwSizeAnsiName = lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszName)+sizeof(char);

          lstrcpy(pFileInfo->szNames+pFileInfo->dwOffsetToAnsiName, lpFileInfoSetW32->lpFileInfo[i].lpszName);

          if (lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)
          {
            pFileInfo->dwOffsetToAnsiChangedName = pFileInfo->dwOffsetToAnsiName + pFileInfo->dwSizeAnsiName;
            pFileInfo->dwSizeAnsiChangedName = lstrlen(lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)+sizeof(char);
            lstrcpy(pFileInfo->szNames+pFileInfo->dwOffsetToAnsiChangedName, lpFileInfoSetW32->lpFileInfo[i].lpszChangedName);
          }

          pFileInfo = (PFILEINFO) ((PCHAR)pFileInfo + dwSizeNames + (SIZEOF_FILEINFO-sizeof(char)));
        }
      }
    }
  }

  return pFileInfoSet;
}

PKEYINFOSET He4HookDriverHide::CreateKeyInfoSet(PW32_KEYINFOSET lpKeyInfoSetW32)
{
  if (!lpKeyInfoSetW32)
    return FALSE;

  DWORD dwSizeOfArea = SIZEOF_KEYINFOSET - SIZEOF_KEYINFO;

  for (int i=0; i<(int)lpKeyInfoSetW32->dwSize; i++)
  {
    if (lpKeyInfoSetW32->lpKeyInfo[i].lpszName)
    {
      dwSizeOfArea += SIZEOF_KEYINFO - sizeof(char);
      dwSizeOfArea += lstrlen(lpKeyInfoSetW32->lpKeyInfo[i].lpszName) + sizeof(char);
    }
  }

  PKEYINFOSET pKeyInfoSet = NULL;
  if (dwSizeOfArea > SIZEOF_KEYINFOSET - SIZEOF_KEYINFO)
  {
    pKeyInfoSet = (PKEYINFOSET) new char[dwSizeOfArea];
    if (pKeyInfoSet)
    {
      memset(pKeyInfoSet, 0, dwSizeOfArea);
      pKeyInfoSet->dwSize = dwSizeOfArea;
      PKEYINFO pKeyInfo = &pKeyInfoSet->KeyInfo[0];
      DWORD     dwSizeNames;
      for (int i=0; i<(int)lpKeyInfoSetW32->dwSize; i++)
      {
        if (lpKeyInfoSetW32->lpKeyInfo[i].lpszName)
        {
          pKeyInfo->dwType = lpKeyInfoSetW32->lpKeyInfo[i].dwType;

          dwSizeNames = lstrlen(lpKeyInfoSetW32->lpKeyInfo[i].lpszName)+sizeof(char);

          pKeyInfo->dwSizeName = dwSizeNames;

          lstrcpy(pKeyInfo->szName, lpKeyInfoSetW32->lpKeyInfo[i].lpszName);

          pKeyInfo = (PKEYINFO) ((PCHAR)pKeyInfo + dwSizeNames + (SIZEOF_KEYINFO-sizeof(char)));
        }
      }
    }
  }

  return pKeyInfoSet;
}