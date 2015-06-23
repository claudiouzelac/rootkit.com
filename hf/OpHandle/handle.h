#define IOCTL_GET_NAME_STRING \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_TOKEN_HANDLE \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define NT_DEVICE_NAME          L"\\Device\\HwndNameDriver"
#define DOS_DEVICE_NAME         L"\\DosDevices\\HwndNameDriver"
#define DEVICE_NAME		L"\\\\.\\HwndNameDriver"

typedef struct 
{
  ULONG pid;
  HANDLE hwnd;
} DIB_NAME_STRING;

typedef struct 
{
  HANDLE hwnd;
} DIB_TOKEN_HANDLE;

typedef struct 
{
  ULONG status;
} DOB_UNKNOWN;

typedef struct 
{
  ULONG status;
  ANSI_STRING name;
} DOB_NAME_STRING;

typedef struct 
{
  ULONG status;
  HANDLE hwnd;
} DOB_TOKEN_HANDLE;
