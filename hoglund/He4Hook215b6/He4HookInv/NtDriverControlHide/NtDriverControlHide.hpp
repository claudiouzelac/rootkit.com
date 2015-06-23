#ifndef __NT_DRIVER_CONTROL_HIDE
 #define __NT_DRIVER_CONTROL_HIDE
#define STRICT

//#ifndef UNICODE
// #define UNICODE 1
//#endif

//#ifndef _UNICODE
// #define _UNICODE 1
//#endif

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <stdio.h>

typedef  struct _USER_COMMAND
{
  DWORD     m_dwCommand;
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

class NtDriverControlHide
{
  public:
   BOOL           Result;
   OSVERSIONINFO  m_OSVer;
   TCHAR         *m_lpszDeviceFileName;

   NtDriverControlHide(const TCHAR *lpszDeviceFileName);
  ~NtDriverControlHide();

   BOOL           Install();
   //
   // для этой ф-ии m_lpszDeviceFileName должно начинаться с "\\??\\"
   // 
   BOOL           LoadAndCallImage();
   virtual BOOL   SendCommand(USER_COMMAND *lpUserCommand);

  private:
   BOOL           Start(TCHAR *lpszDeviceName, SC_HANDLE schSCManager);
   BOOL           Stop(TCHAR *lpszDeviceName, SC_HANDLE schSCManager);
   BOOL           Remove(TCHAR *lpszDeviceName, SC_HANDLE schSCManager);
};

void __inline DriverErrorMessage(void)
{
  TCHAR *MsgBuf;

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &MsgBuf,
                0,
                NULL
               );
  
  #ifndef _CONSOLE
  MessageBox(GetForegroundWindow(), MsgBuf, _T("Error"), MB_OK);
  #else
  printf("%s\n", (char*)MsgBuf);
  #endif //_CONSOLE
  LocalFree(MsgBuf);
}

#ifndef _DebugMessage
 #ifdef _DEBUG
 #define  _DebugMessage(FunctionName)   { _CrtDbgReport(0, __FILE__, __LINE__, NULL, #FunctionName); }
 #else
 #define  _DebugMessage(FunctionName)   { ; }
 #endif
#endif //_DebugMessage(FunctionName)

#endif
