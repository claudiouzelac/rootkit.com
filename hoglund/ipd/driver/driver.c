/*
 * Copyright (C) 2000 by Pedestal Software, LLC
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* driver.c */

#include "driver.h"
#include "hook.h"
#include "util.h"


GLOBALS Globals;


NTSTATUS DriverEntry( IN PDRIVER_OBJECT driverObject,
		      IN PUNICODE_STRING registryPath ) {
#ifndef NOTRY
  __try {
#endif
    initDebugOutput();
    Globals.RESTRICT_STARTUP_TIMEOUT.LowPart = 0;
#ifndef NOUNLOAD
    driverObject->DriverUnload  = OnUnload; 
    Globals.RESTRICT_STARTUP_TIMEOUT.HighPart = 0;
#else
    Globals.RESTRICT_STARTUP_TIMEOUT.HighPart = 2;    
#endif

    Globals.registryPath = Uwcsdup(registryPath);
    if (Globals.registryPath == NULL)
      return STATUS_INSUFFICIENT_RESOURCES;

    KeQuerySystemTime(&Globals.DRIVERSTARTTIME);

    debugOutput(L"hooking system calls...\n");
    hookInit();
    hookFuncs();
    debugOutput(L"hook ok\n");

    return STATUS_SUCCESS;
#ifndef NOTRY
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {
    debugOutput(L"DriverEntry: EXCEPTION\n");
  }
  return(STATUS_UNSUCCESSFUL);
#endif
}



#ifndef NOUNLOAD
VOID OnUnload( IN PDRIVER_OBJECT DriverObject ) {
  debugOutput(L"unhooking functions\n");
  unhookFuncs();
  hookClose();
  if (Globals.registryPath) ExFreePool(Globals.registryPath);
  closeDebugOutput();
}
#endif




/*
 * debugging
 */

VOID initDebugOutput() {
}

VOID debugOutput(WCHAR *txt) {
#ifdef DEBUG
  if (!txt) return;
  DbgPrint("%S", txt);
#endif
}

VOID closeDebugOutput() {
}
