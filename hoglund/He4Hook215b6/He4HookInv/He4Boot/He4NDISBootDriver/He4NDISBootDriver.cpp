#define STRICT
#include "He4NDISBootDriver.hpp"

He4NDISBootDriver::He4NDISBootDriver(const WCHAR *lpszDeviceFileName)  
                 : NtBootDriverControlHide(lpszDeviceFileName)
{
  if(!NtBootDriverControlHide::Result) return;

  Result = TRUE;
}

He4NDISBootDriver::~He4NDISBootDriver()
{
}

BOOLEAN He4NDISBootDriver::Install()
{
  return NtBootDriverControlHide::Install(L"He4NDIS", 1, 2, 1);
}