#ifndef __HE4HOOK_DRIVER_HIDE
 #define __HE4HOOK_DRIVER_HIDE
#define STRICT

//#ifndef UNICODE
// #define UNICODE 1
//#endif

//#ifndef _UNICODE
// #define _UNICODE 1
//#endif

#include <windows.h>

#include "..\NtDriverControlHide\NtDriverControlHide.hpp"

#define PETHREAD PVOID
#include "..\He4Command.h"

typedef struct _W32_FILEINFO
{
  char  *lpszName;
  char  *lpszChangedName;
  DWORD  dwAccessType;

  _W32_FILEINFO()
  {
   memset(this, 0, sizeof(_W32_FILEINFO));
  }
} W32_FILEINFO, *PW32_FILEINFO;

typedef struct _W32_FILEINFOSET
{
  DWORD          dwSize;    // number W32_FILEINFO
  PW32_FILEINFO  lpFileInfo;

  _W32_FILEINFOSET()
  {
   memset(this, 0, sizeof(_W32_FILEINFOSET));
  }
} W32_FILEINFOSET, *PW32_FILEINFOSET;


typedef struct _W32_KEYINFO
{
  char  *lpszName;
  DWORD  dwType;

  _W32_KEYINFO()
  {
   memset(this, 0, sizeof(_W32_KEYINFO));
  }
} W32_KEYINFO, *PW32_KEYINFO;

typedef struct _W32_KEYINFOSET
{
  DWORD          dwSize;    // number W32_FILEINFO
  PW32_KEYINFO   lpKeyInfo;

  _W32_KEYINFOSET()
  {
   memset(this, 0, sizeof(_W32_KEYINFOSET));
  }
} W32_KEYINFOSET, *PW32_KEYINFOSET;


typedef DWORD (__stdcall *RTLNTSTATUSTODOSERROR)(DWORD NtStatus);


class He4HookDriverHide : public NtDriverControlHide
{
  public:
   BOOL         Result;

   He4HookDriverHide(const TCHAR *lpszDeviceFileName);
  ~He4HookDriverHide();

   DWORD        GetVersion();
             
   BOOL         LockSaveObjectsForAllThreads();
   BOOL         LockSaveObjects(DWORD dwClientId = HE4_UNLOCK_CURRENT_CLIENT, BOOL bForProcess = FALSE);
   BOOL         UnLockSaveObjects(DWORD dwUnlockFlags = HE4_UNLOCK_READ | HE4_UNLOCK_WRITE | HE4_UNLOCK_DELETE | HE4_UNLOCK_VISIBLE, DWORD dwClientId = HE4_UNLOCK_CURRENT_CLIENT);
   DWORD        GetUnlockListSizeByBytes();
   BOOL         GetUnlockList(PUNLOCK_CLIENT_INFO_SET pUnlockInfoSet);
             
   DWORD        HookFileSystem(DWORD dwHook); // 0 - unhook, 1 - hook Nt*/Zw*/Io*, 2 - hook DRIVER_OBJECT; result - (-1) - Error
   BOOL         AddToSaveList(char* pszFileName, DWORD dwAccessType, char* pszChangedName = NULL);
   BOOL         AddToSaveList(PW32_FILEINFOSET lpKeyInfoSetW32);
   BOOL         AddToSaveList(PFILEINFOSET lpFileInfoSet);
   BOOL         DelFromSaveList(char* pszFileName);
   BOOL         DelFromSaveList(PW32_FILEINFOSET lpKeyInfoSetW32);
   BOOL         ClearSaveList();
   DWORD        GetSaveListSizeByBytes();
   BOOL         GetSaveList(PFILEINFOSET pFileInfoSet);

   BOOL         QueryStatistic(PHE4_STATISTIC_INFO pStatInfo);
       
   BOOL         HookRegistry();
   BOOL         UnHookRegistry();
   BOOL         AddKeysToSaveList(PW32_KEYINFOSET lpKeyInfoSetW32);
   BOOL         DelKeysFromSaveList(PW32_KEYINFOSET lpKeyInfoSetW32);
             
   DWORD        QueryUnload();

   DWORD        ZwDispatchFunction(DWORD dwProcessId, DWORD dwThreadId, DWORD IoControlCode,
                                   PVOID InputBuffer, DWORD InputBufferLength,
                                   PVOID OutputBuffer, DWORD OutputBufferLength, 
                                   DWORD *lpBytesReturned);
   virtual BOOL SendCommand(USER_COMMAND *lpUserCommand);

  protected:
   BOOL         AddShortName(PW32_FILEINFOSET lpFileInfoSet);
   BOOL         DelShortName(PW32_FILEINFOSET lpFileInfoSet);

   PFILEINFOSET CreateFileInfoSet(PW32_FILEINFOSET lpFileInfoSetW32);
   PKEYINFOSET  CreateKeyInfoSet(PW32_KEYINFOSET lpKeyInfoSetW32);

   BOOL         GetNtProcAddresses();

   RTLNTSTATUSTODOSERROR pRtlNtStatusToDosError;
};

#endif //
