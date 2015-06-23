#define IOCTL_GET_TOKEN_HANDLE \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IMPERSONATE_PROCESS \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KILL_PROCESS \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PROCESS_TERMINATE         (0x0001)  

#define NT_DEVICE_NAME          L"\\Device\\ProcessMasterDrvDevice"
#define DOS_DEVICE_NAME         L"\\DosDevices\\ProcessMasterDrvDevice"
#define DEVICE_NAME		L"\\\\.\\ProcessMasterDrvDevice"


typedef struct 
{
  HANDLE hwnd;
} DIB_TOKEN_HANDLE;

typedef struct 
{
  HANDLE hCalling;
  HANDLE hTarget;
} DIB_IMPERSONATE_PROCESS;

typedef struct 
{
  ULONG dwProcessId;
} DIB_KILL_PROCESS;


typedef struct 
{
  ULONG status;
} DOB_UNKNOWN;

typedef struct 
{
  ULONG status;
  HANDLE hwnd;
} DOB_TOKEN_HANDLE;

typedef struct 
{
  ULONG status1;
  ULONG status2;
} DOB_IMPERSONATE_PROCESS;

typedef struct 
{
  ULONG status;
} DOB_KILL_PROCESS;

