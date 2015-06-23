/*
 * Copyright (C) 2000 by Pedestal Software, LLC
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <ntddk.h>
#include "ntproto.h"

#define NOTRY
#define NOUNLOAD
//#define DEBUG
#define CRASH_TAG 101

#ifndef NOUNLOAD
VOID OnUnload(IN PDRIVER_OBJECT DriverObject );
#endif

typedef struct GLOBALS {
  LARGE_INTEGER DRIVERSTARTTIME;
  LARGE_INTEGER RESTRICT_STARTUP_TIMEOUT;
  PUNICODE_STRING registryPath;
} GLOBALS;
extern GLOBALS Globals;


/*
 * debugging
 */
extern VOID initDebugOutput();
extern VOID debugOutput(WCHAR *txt);
extern VOID closeDebugOutput();


#endif
