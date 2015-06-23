#ifndef __NT_BOOT_DRIVER_CONTROL_HIDE
 #define __NT_BOOT_DRIVER_CONTROL_HIDE
#define STRICT

#include "..\ntdll.h"

typedef  struct _USER_COMMAND
               { DWORD     m_dwCommand;
                 LPVOID    m_lpInBuffer;
                 DWORD     m_dwInBufferSize;
                 LPVOID    m_lpOutBuffer;
                 DWORD     m_dwOutBufferSize;
                 DWORD     m_dwBytesReturned;
                
                 _USER_COMMAND(void)
                 {
                  memset(this, 0, sizeof(_USER_COMMAND));
                 }
               } USER_COMMAND, *PUSER_COMMAND;

class NtBootDriverControlHide
{
  public:
   BOOLEAN         Result;
   WCHAR          *m_lpszDeviceFileName;

   NtBootDriverControlHide(const WCHAR *lpszDeviceFileName);
  ~NtBootDriverControlHide();

   virtual BOOLEAN Install(PWSTR lpwszDefaultDeviceName, ULONG ulType = 1, ULONG ulStart = 1, ULONG ulErrorControl = 1, PWSTR lpwszObjectName = NULL);
   virtual BOOLEAN SendCommand(USER_COMMAND *lpUserCommand);

  protected:
   BOOLEAN         Start(WCHAR *lpszDeviceName);
   BOOLEAN         Stop(WCHAR *lpszDeviceName);
   BOOLEAN         Remove(WCHAR *lpszDeviceName);
   BOOLEAN         DeleteKey(WCHAR *lpszKeyName);

   PWSTR           QueryFullDeviceName(PWSTR lpwszDeviceName);
   BOOLEAN         ReleaseFullDeviceName(PWSTR lpwszFullDeviceName);

  public:
};

   void* _cdecl operator new(size_t size);
   void  _cdecl operator delete(void* p);

#endif
