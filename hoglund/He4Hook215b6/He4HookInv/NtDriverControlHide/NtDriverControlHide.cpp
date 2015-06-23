#define STRICT
#include "NtDriverControlHide.hpp"

NtDriverControlHide::NtDriverControlHide(const TCHAR *lpszDeviceFileName)
                   : Result(FALSE), m_lpszDeviceFileName(NULL)
{
  m_OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (!GetVersionEx(&m_OSVer))
  {
    DriverErrorMessage();
    return;
  }
  if (
      m_OSVer.dwPlatformId == VER_PLATFORM_WIN32s ||
      m_OSVer.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
     )
  {
    MessageBox(GetForegroundWindow(), 
               _T("Kernel mode driver supported only WinNT !"), _T("Information"), MB_OK);
    return;
  }

  if(!lpszDeviceFileName) return;

  m_lpszDeviceFileName = new TCHAR[lstrlen(lpszDeviceFileName)+1];
  if (m_lpszDeviceFileName) 
    lstrcpy(m_lpszDeviceFileName, lpszDeviceFileName);
  else
    return;
  Result = TRUE;
}

NtDriverControlHide::~NtDriverControlHide()
{
  if (m_lpszDeviceFileName)
    delete[] m_lpszDeviceFileName;
  m_lpszDeviceFileName = 0;
}

BOOL NtDriverControlHide::SendCommand(USER_COMMAND *lpUserCommand)
{
  return TRUE;
}

/****************************************************************************
*
*    FUNCTION: Start(IN SC_HANDLE)
*
*    PURPOSE: Starts the driver service.
*
****************************************************************************/
BOOL NtDriverControlHide::Start(TCHAR *lpszDeviceName, SC_HANDLE schSCManager)
{
  if (!lpszDeviceName || !schSCManager)
    return FALSE;

  SC_HANDLE  schService;
  BOOL       ret;

  schService = OpenService(schSCManager, lpszDeviceName, SERVICE_ALL_ACCESS);
  if (schService == NULL)
    return FALSE;

  ret = StartService(schService, 0, NULL) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING;

  CloseServiceHandle(schService);

  return ret;
}

/****************************************************************************
*
*    FUNCTION: Stop(IN SC_HANDLE)
*
*    PURPOSE: Has the configuration manager stop the driver (unload it)
*
****************************************************************************/
BOOL NtDriverControlHide::Stop(TCHAR *lpszDeviceName, SC_HANDLE schSCManager)
{
  if (!lpszDeviceName || !schSCManager)
    return FALSE;

  SC_HANDLE       schService;
  BOOL            ret;
  SERVICE_STATUS  serviceStatus;

  schService = OpenService(schSCManager, lpszDeviceName, SERVICE_ALL_ACCESS);
  if (schService == NULL)
    return FALSE;

  ret = ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus);

  CloseServiceHandle(schService);

  return ret;
}

/****************************************************************************
*
*    FUNCTION: Install(IN SC_HANDLE)
*
*    PURPOSE: Creates a driver service.
*
****************************************************************************/
BOOL NtDriverControlHide::Install()
{
  if (!m_lpszDeviceFileName) 
    return FALSE;
  
  SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!schSCManager)
  {
    DriverErrorMessage();
    return FALSE;
  }

  TCHAR szDeviceName[64];

  wsprintf(szDeviceName, TEXT("%08X"), GetTickCount());

  SC_HANDLE  schService;

  //
  // NOTE: This creates an entry for a standalone driver. If this
  //       is modified for use with a driver that requires a Tag,
  //       Group, and/or Dependencies, it may be necessary to
  //       query the registry for existing driver information
  //       (in order to determine a unique Tag, etc.).
  //

  schService = CreateService(schSCManager,          // SCManager database
                             szDeviceName,          // name of service
                             szDeviceName,          // name to display
                             SERVICE_ALL_ACCESS,    // desired access
                             SERVICE_KERNEL_DRIVER, // service type
                             SERVICE_DEMAND_START,  // start type
                             SERVICE_ERROR_IGNORE,  // error control type
                             m_lpszDeviceFileName,  // service's binary
                             NULL,                  // no load ordering group
                             NULL,                  // no tag identifier
                             NULL,                  // no dependencies
                             NULL,                  // LocalSystem account
                             NULL                   // no password
                             );
  if (schService == NULL)
    return FALSE;

  CloseServiceHandle(schService);

  Start(szDeviceName, schSCManager);
  Stop(szDeviceName, schSCManager);
  Remove(szDeviceName, schSCManager);

  CloseServiceHandle(schSCManager);

  return TRUE;
}

//
// для этой ф-ии m_lpszDeviceFileName должно начинаться с "\\??\\"
//
BOOL NtDriverControlHide::LoadAndCallImage()
{
  if (!m_lpszDeviceFileName) 
    return FALSE;

  typedef long NTSTATUS;

  #define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
  #define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
  #define STATUS_NO_SUCH_DEVICE            ((NTSTATUS)0xC000000EL)

  typedef struct _UNICODE_STRING 
  {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
  } UNICODE_STRING;
  typedef UNICODE_STRING *PUNICODE_STRING;

  typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE
  {
    UNICODE_STRING ModuleName;
  } SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE;

  typedef enum _SYSTEM_INFORMATION_CLASS
  {
    _SystemLoadAndCallImageInformation = 38
  } SYSTEM_INFORMATION_CLASS;

  //RtlInitUnicodeString
  typedef void (__stdcall *RTLINITUUNICODESTRING)(PUNICODE_STRING DestinationString, PCWSTR SourceString);
  //NtSetSystemInformation
  typedef long (__stdcall *NTSETSYSTEMINFORMATION)(SYSTEM_INFORMATION_CLASS SysInfoClass, PVOID pArray, ULONG dwSizeArray);
  //RtlAdjustPrivilege
  typedef long (__stdcall *RTLADJUSTPRIVILEGE)(ULONG Privilege, BOOLEAN Enable, BOOLEAN CurrentThread, BOOLEAN* Enabled);

  BOOLEAN           OldState = FALSE;
  SYSTEM_LOAD_AND_CALL_IMAGE LoadImageInfo;
  PWSTR             pwszNtFileName;
  int               nNtFileNameBufferLen = lstrlen(m_lpszDeviceFileName)+sizeof(WCHAR);
  NTSTATUS          NtStatus, NtStatusSetPriv;
  BOOL              bRes = FALSE;
  RTLINITUUNICODESTRING    pRtlInitUnicodeString = NULL;
  NTSETSYSTEMINFORMATION   pNtSetSystemInformation = NULL;
  RTLADJUSTPRIVILEGE       pRtlAdjustPrivilege = NULL;
  HMODULE                  hNtDll;


  hNtDll = GetModuleHandle("ntdll.dll");
  if (!hNtDll)
    return FALSE;

  pRtlInitUnicodeString = (RTLINITUUNICODESTRING)GetProcAddress(hNtDll, "RtlInitUnicodeString");
  if (IsBadCodePtr((FARPROC)pRtlInitUnicodeString))
    return FALSE;

  pNtSetSystemInformation = (NTSETSYSTEMINFORMATION)GetProcAddress(hNtDll, "NtSetSystemInformation");
  if (IsBadCodePtr((FARPROC)pNtSetSystemInformation))
    return FALSE;

  pRtlAdjustPrivilege = (RTLADJUSTPRIVILEGE)GetProcAddress(hNtDll, "RtlAdjustPrivilege");
  if (IsBadCodePtr((FARPROC)pRtlAdjustPrivilege))
    return FALSE;

  pwszNtFileName = new WCHAR[nNtFileNameBufferLen];
  if (pwszNtFileName == NULL)
    return bRes;

  memset(&LoadImageInfo, 0, sizeof(LoadImageInfo));

  if (sizeof(TCHAR) == 1)
  {
    MultiByteToWideChar(CP_ACP, 0, m_lpszDeviceFileName, -1, pwszNtFileName, lstrlen(m_lpszDeviceFileName)+sizeof(TCHAR));
  }
  else
  {
    memcpy(pwszNtFileName, m_lpszDeviceFileName, lstrlen(m_lpszDeviceFileName)*sizeof(TCHAR)+sizeof(TCHAR));
  }

  pRtlInitUnicodeString(&LoadImageInfo.ModuleName, pwszNtFileName);

  #define SE_LOAD_DRIVER_PRIVILEGE          (10L)

  NtStatusSetPriv = pRtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &OldState);

  NtStatus = pNtSetSystemInformation(_SystemLoadAndCallImageInformation, &LoadImageInfo, sizeof(LoadImageInfo));
  if (NtStatus == STATUS_SUCCESS || NtStatus == STATUS_UNSUCCESSFUL)
    bRes = TRUE;

  if (NtStatusSetPriv == STATUS_SUCCESS)
    pRtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, OldState, FALSE, &OldState);

  delete[] pwszNtFileName;

  return bRes;
}

/****************************************************************************
*
*    FUNCTION: Remove(IN SC_HANDLE)
*
*    PURPOSE: Deletes the driver service.
*
****************************************************************************/
BOOL NtDriverControlHide::Remove(TCHAR *lpszDeviceName, SC_HANDLE schSCManager)
{
  if(!lpszDeviceName || !schSCManager) return FALSE;

  SC_HANDLE  schService;
  BOOL       ret;

  schService = OpenService(schSCManager,
                           lpszDeviceName,
                           SERVICE_ALL_ACCESS);

  if (schService == NULL)
    return FALSE;

  ret = DeleteService(schService);

  CloseServiceHandle(schService);

  return ret;
}

