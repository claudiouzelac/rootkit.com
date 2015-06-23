#ifndef __HE4_NDIS_BOOT_DRIVER
 #define __HE4_NDIS_BOOT_DRIVER
#define STRICT

#include "..\ntdll.h"

#include "..\NtBootDriverControlHide\NtBootDriverControlHide.hpp"

class He4NDISBootDriver : public NtBootDriverControlHide
{
  public:
   He4NDISBootDriver(const WCHAR *lpszDeviceFileName);
  ~He4NDISBootDriver();

   virtual BOOLEAN Install();

};

#endif // __HE4_NDIS_BOOT_DRIVER