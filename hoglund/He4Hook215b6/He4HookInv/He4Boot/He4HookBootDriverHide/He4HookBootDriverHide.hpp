#ifndef __HE4HOOK_BOOT_DRIVER_HIDE
 #define __HE4HOOK_BOOT_DRIVER_HIDE
#define STRICT

#include "..\ntdll.h"

#include "..\NtBootDriverControlHide\NtBootDriverControlHide.hpp"

#define PETHREAD PVOID
#include "..\..\He4Command.h"

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

class He4HookBootDriverHide : public NtBootDriverControlHide
{
  public:
   He4HookBootDriverHide(const WCHAR *lpszDeviceFileName);
  ~He4HookBootDriverHide();

   virtual BOOLEAN Install();

   DWORD        GetVersion();
   DWORD        NativeGetVersion(DWORD *lpdwVersion);
             
   BOOLEAN      LockSaveFiles();
   BOOLEAN      UnLockSaveFiles(DWORD dwUnlockFlags = HE4_UNLOCK_READ | HE4_UNLOCK_WRITE | HE4_UNLOCK_DELETE | HE4_UNLOCK_VISIBLE);
             
   BOOLEAN      HookFileSystem(DWORD dwHook);  // 0 - unhook, 1 - hook Nt*/Zw*/Io*, 2 - hook DRIVER_OBJECT; 
   BOOLEAN      AddToSaveList(PW32_FILEINFOSET lpKeyInfoSetW32);
   BOOLEAN      DelFromSaveList(PW32_FILEINFOSET lpKeyInfoSetW32);
             
   BOOLEAN      HookRegistry();
   BOOLEAN      UnHookRegistry();
   BOOLEAN      AddKeysToSaveList(PW32_KEYINFOSET lpKeyInfoSetW32);
   BOOLEAN      DelKeysFromSaveList(PW32_KEYINFOSET lpKeyInfoSetW32);
             
   DWORD        QueryUnload();

   DWORD        ZwDispatchFunction(DWORD dwProcessId, DWORD dwThreadId, DWORD IoControlCode,
                                   PVOID InputBuffer, DWORD InputBufferLength,
                                   PVOID OutputBuffer, DWORD OutputBufferLength, 
                                   DWORD *lpBytesReturned);
   virtual BOOLEAN SendCommand(USER_COMMAND *lpUserCommand);

   PFILEINFOSET CreateFileInfoSet(PW32_FILEINFOSET lpFileInfoSetW32);
   PKEYINFOSET  CreateKeyInfoSet(PW32_KEYINFOSET lpKeyInfoSetW32);
};

#endif //