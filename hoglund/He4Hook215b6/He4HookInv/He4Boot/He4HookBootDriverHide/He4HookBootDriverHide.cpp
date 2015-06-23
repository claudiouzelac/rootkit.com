#define STRICT
#include "He4HookBootDriverHide.hpp"

He4HookBootDriverHide::He4HookBootDriverHide(const WCHAR *lpszDeviceFileName) : 
                       NtBootDriverControlHide(lpszDeviceFileName)
{
  if (!NtBootDriverControlHide::Result)
    return;

  Result = TRUE;
}

He4HookBootDriverHide::~He4HookBootDriverHide()
{
}

DWORD He4HookBootDriverHide::ZwDispatchFunction(DWORD dwProcessId, DWORD dwThreadId, DWORD IoControlCode,
                                                PVOID InputBuffer, DWORD InputBufferLength,
                                                PVOID OutputBuffer, DWORD OutputBufferLength, 
                                                DWORD *lpBytesReturned)
{
  void   **lpParameterStack = (void**) &dwProcessId;
  DWORD    dwRet = -1;
  DWORD    dwSericeId = (KE_SERVICE_TABLE_INDEX<<12) + 2;

  __asm
  {
    mov  eax, [dwSericeId]     
    mov  edx, lpParameterStack
    int  2eh
    mov  [dwRet], eax
  }

  return dwRet;
}

BOOLEAN He4HookBootDriverHide::SendCommand(USER_COMMAND *lpUserCommand)
{
  if (!lpUserCommand)
    return FALSE;

  __try
  {
    DWORD NtStatus = ZwDispatchFunction(0, 0,
                                        lpUserCommand->m_dwCommand,
                                        lpUserCommand->m_lpInBuffer, lpUserCommand->m_dwInBufferSize,
                                        lpUserCommand->m_lpOutBuffer, lpUserCommand->m_dwOutBufferSize,
                                        &lpUserCommand->m_dwBytesReturned);
    if (NtStatus)
    {
      return FALSE;
    }
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    return FALSE;
  }
  return TRUE;
}

DWORD He4HookBootDriverHide::GetVersion()
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

BOOLEAN He4HookBootDriverHide::HookFileSystem(DWORD dwHook)
{
  USER_COMMAND  UserCommand;
  DWORD         dwHookResult = 0x0;


  UserCommand.m_dwCommand = HE4_HOOK_FILE_SYSTEM;
  UserCommand.m_lpInBuffer = &dwHook;
  UserCommand.m_dwInBufferSize = sizeof(DWORD);
  UserCommand.m_lpOutBuffer = &dwHookResult;
  UserCommand.m_dwOutBufferSize = sizeof(DWORD);

  if (!SendCommand(&UserCommand))
    return FALSE;
  return (BOOLEAN) UserCommand.m_dwBytesReturned;
}

BOOLEAN He4HookBootDriverHide::LockSaveFiles()
{
  USER_COMMAND    UserCommand;

  UserCommand.m_dwCommand = HE4_LOCK_SAVE_FILES;
  UserCommand.m_lpInBuffer = NULL;
  UserCommand.m_dwInBufferSize = 0;
  UserCommand.m_lpOutBuffer = NULL;
  UserCommand.m_dwOutBufferSize = 0;
  UserCommand.m_dwBytesReturned = 0;
  if(!SendCommand(&UserCommand)) 
     return FALSE;
  return (BOOLEAN) UserCommand.m_dwBytesReturned;
}

BOOLEAN He4HookBootDriverHide::UnLockSaveFiles(DWORD dwUnlockFlags)
{
  USER_COMMAND         UserCommand;
  HE4_UNLOCK_SETTING   UnlockSetting;

  UnlockSetting.m_dwUnlockFlags = dwUnlockFlags;

  UserCommand.m_dwCommand = HE4_UNLOCK_SAVE_FILES;
  UserCommand.m_lpInBuffer = &UnlockSetting;
  UserCommand.m_dwInBufferSize = sizeof(HE4_UNLOCK_SETTING);
  UserCommand.m_lpOutBuffer = NULL;
  UserCommand.m_dwOutBufferSize = 0;
  UserCommand.m_dwBytesReturned = 0;
  if (!SendCommand(&UserCommand)) 
    return FALSE;
  return (BOOLEAN) UserCommand.m_dwBytesReturned;
}

BOOLEAN He4HookBootDriverHide::AddToSaveList(PW32_FILEINFOSET lpFileInfoSetW32)
{
  USER_COMMAND    UserCommand;

  PFILEINFOSET lpFileInfoSet = CreateFileInfoSet(lpFileInfoSetW32);
  if (lpFileInfoSet)
  {
    UserCommand.m_dwCommand = HE4_ADD_TO_SAVE_LIST;
    UserCommand.m_lpInBuffer = lpFileInfoSet;
    UserCommand.m_dwInBufferSize = lpFileInfoSet->dwSize;
    UserCommand.m_dwBytesReturned = 0;
    if (SendCommand(&UserCommand)) 
    {
      delete[] (char*)lpFileInfoSet;
      return (BOOLEAN) UserCommand.m_dwBytesReturned;
    }
    delete[] (char*)lpFileInfoSet;
  }
  
  return FALSE;
}

BOOLEAN He4HookBootDriverHide::DelFromSaveList(PW32_FILEINFOSET lpFileInfoSetW32)
{
  USER_COMMAND    UserCommand;

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
      return (BOOLEAN) UserCommand.m_dwBytesReturned;
    }
    delete[] (char*)lpFileInfoSet;
  }
  return FALSE;
}

DWORD He4HookBootDriverHide::QueryUnload()
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

BOOLEAN He4HookBootDriverHide::AddKeysToSaveList(PW32_KEYINFOSET lpKeyInfoSetW32)
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
      return (BOOLEAN) UserCommand.m_dwBytesReturned;
    }
    delete[] (char*)lpKeyInfoSet;
  }
  return FALSE;
}

BOOLEAN He4HookBootDriverHide::DelKeysFromSaveList(PW32_KEYINFOSET lpKeyInfoSetW32)
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
      return (BOOLEAN) UserCommand.m_dwBytesReturned;
    }
    delete[] (char*)lpKeyInfoSet;
  }

  return FALSE;
}

BOOLEAN He4HookBootDriverHide::HookRegistry()
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
  return (BOOLEAN) UserCommand.m_dwBytesReturned;
}

BOOLEAN He4HookBootDriverHide::UnHookRegistry()
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
  return (BOOLEAN) UserCommand.m_dwBytesReturned;
}

DWORD He4HookBootDriverHide::NativeGetVersion(DWORD *lpdwVersion)
{
  void **lpParameterStack = (void **) &lpdwVersion;
  DWORD  dwRet = -1;
  DWORD  dwSericeId = (KE_SERVICE_TABLE_INDEX<<12) + 0;

  __asm
  {
    mov  eax, [dwSericeId]     
    mov  edx, lpParameterStack
    int  2eh
    mov  [dwRet], eax
  }

  return dwRet;
}

BOOLEAN He4HookBootDriverHide::Install()
{
  DWORD   dwVer = 0;
  BOOLEAN bRes;
  WCHAR   wszDeviceName[64];
  DWORD   NtStatus = NativeGetVersion(&dwVer);

  if (NT_SUCCESS(NtStatus) && dwVer == HE4_HOOK_INV_VERSION) 
    return TRUE;
  if (NT_SUCCESS(NtStatus))
    return FALSE;

  swprintf(wszDeviceName, L"%08X", (ULONG)NtCurrentTeb());
  bRes = NtBootDriverControlHide::Install(wszDeviceName);
  bRes &= Start(wszDeviceName);
  Stop(wszDeviceName);
  Remove(wszDeviceName);

  return bRes;
}


PFILEINFOSET He4HookBootDriverHide::CreateFileInfoSet(PW32_FILEINFOSET lpFileInfoSetW32)
{
  if (!lpFileInfoSetW32)
    return FALSE;

  DWORD dwSizeOfArea = SIZEOF_FILEINFOSET - SIZEOF_FILEINFO;

  for (int i=0; i<(int)lpFileInfoSetW32->dwSize; i++)
  {
    if (lpFileInfoSetW32->lpFileInfo[i].lpszName)
    {
      dwSizeOfArea += SIZEOF_FILEINFO - sizeof(char);
      dwSizeOfArea += strlen(lpFileInfoSetW32->lpFileInfo[i].lpszName) + sizeof(char);
      if (lpFileInfoSetW32->lpFileInfo[i].dwAccessType & FILE_ACC_TYPE_EXCHANGE)
      {
        if (!lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)
        {
          lpFileInfoSetW32->lpFileInfo[i].dwAccessType &= ~FILE_ACC_TYPE_EXCHANGE;
        }
        else
        {
          dwSizeOfArea += strlen(lpFileInfoSetW32->lpFileInfo[i].lpszChangedName) + sizeof(char);
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
    pFileInfoSet = (PFILEINFOSET) new char[dwSizeOfArea];
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

          dwSizeNames = strlen(lpFileInfoSetW32->lpFileInfo[i].lpszName)+sizeof(char);
          if (lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)
            dwSizeNames += strlen(lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)+sizeof(char);

          pFileInfo->dwSizeAllNamesArea = dwSizeNames;

          pFileInfo->dwOffsetToAnsiName = 0;
          pFileInfo->dwSizeAnsiName = strlen(lpFileInfoSetW32->lpFileInfo[i].lpszName)+sizeof(char);

          strcpy(pFileInfo->szNames+pFileInfo->dwOffsetToAnsiName, lpFileInfoSetW32->lpFileInfo[i].lpszName);

          if (lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)
          {
            pFileInfo->dwOffsetToAnsiChangedName = pFileInfo->dwOffsetToAnsiName + pFileInfo->dwSizeAnsiName;
            pFileInfo->dwSizeAnsiChangedName = strlen(lpFileInfoSetW32->lpFileInfo[i].lpszChangedName)+sizeof(char);
            strcpy(pFileInfo->szNames+pFileInfo->dwOffsetToAnsiChangedName, lpFileInfoSetW32->lpFileInfo[i].lpszChangedName);
          }

          pFileInfo = (PFILEINFO) ((PCHAR)pFileInfo + dwSizeNames + (SIZEOF_FILEINFO-sizeof(char)));
        }
      }
    }
  }

  return pFileInfoSet;
}

PKEYINFOSET He4HookBootDriverHide::CreateKeyInfoSet(PW32_KEYINFOSET lpKeyInfoSetW32)
{
  if (!lpKeyInfoSetW32)
    return FALSE;

  DWORD dwSizeOfArea = SIZEOF_KEYINFOSET - SIZEOF_KEYINFO;

  for (int i=0; i<(int)lpKeyInfoSetW32->dwSize; i++)
  {
    if (lpKeyInfoSetW32->lpKeyInfo[i].lpszName)
    {
      dwSizeOfArea += SIZEOF_KEYINFO - sizeof(char);
      dwSizeOfArea += strlen(lpKeyInfoSetW32->lpKeyInfo[i].lpszName) + sizeof(char);
    }
  }

  PKEYINFOSET pKeyInfoSet = NULL;
  if (dwSizeOfArea > SIZEOF_KEYINFOSET - SIZEOF_KEYINFO)
  {
    pKeyInfoSet = (PKEYINFOSET ) new char[dwSizeOfArea];
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

          dwSizeNames = strlen(lpKeyInfoSetW32->lpKeyInfo[i].lpszName)+sizeof(char);

          pKeyInfo->dwSizeName = dwSizeNames;

          strcpy(pKeyInfo->szName, lpKeyInfoSetW32->lpKeyInfo[i].lpszName);

          pKeyInfo = (PKEYINFO) ((PCHAR)pKeyInfo + dwSizeNames + (SIZEOF_KEYINFO-sizeof(char)));
        }
      }
    }
  }

  return pKeyInfoSet;
}