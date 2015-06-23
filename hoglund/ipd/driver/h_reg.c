/*
 * Copyright (C) 2000 by Pedestal Software, LLC
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

#include "driver.h"
#include "h_reg.h"
#include "util.h"

ZWOPENKEY	OldZwOpenKey;
ZWCREATEKEY	OldZwCreateKey;

#define REGSETVALUE 0x2
#define REGCREATEKEY 0x4

static RESTRICT_PARAM RESTRICT[] = {
  // new services/drivers
  { L"\\Registry\\Machine\\System\\CurrentControlSet\\Services",
    REGCREATEKEY|REGSETVALUE|DELETE|GENERIC_WRITE|GENERIC_ALL },
  { L"\\Registry\\Machine\\System\\ControlSet001\\Services",
    REGCREATEKEY|REGSETVALUE|DELETE|GENERIC_WRITE|GENERIC_ALL },
  { L"\\Registry\\Machine\\System\\ControlSet002\\Services",
    REGCREATEKEY|REGSETVALUE|DELETE|GENERIC_WRITE|GENERIC_ALL },
  { L"\\Registry\\Machine\\System\\ControlSet003\\Services",
    REGCREATEKEY|REGSETVALUE|DELETE|GENERIC_WRITE|GENERIC_ALL },
  { NULL, 0 }
};
WCHAR STR_PARAMETERS[] = L"Parameters";
WCHAR STR_SERVICES[] = L"Services";


void hookRegInit() {
}
void hookRegClose() {
}



NTSTATUS
NewZwOpenKey(
	     PHANDLE phKey,
	     ACCESS_MASK DesiredAccess,
	     POBJECT_ATTRIBUTES ObjectAttributes) {
  int status;
  WCHAR buf[1024];
#ifndef NOTRY
  __try {
#endif
    if (restrictEnabled()) {
      ACCESS_MASK mask;
      int l = getFullPath(buf, sizeof(buf), ObjectAttributes);
      mask = isRestricted(RESTRICT, buf);
      if (mask>0) {
	if (pathContainsPart(buf, buf+l-1, STR_PARAMETERS, STR_SERVICES)) {
	  mask &= ~REGSETVALUE;
	}
	debugOutput(buf);
	debugOutput(L": restrict1\n");
      }
      DesiredAccess &= ~mask;
    }
#ifndef NOTRY
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {
    swprintf(buf,L"NewZwOpenKey: EXCEPTION\n");
    debugOutput(buf);
  }
#endif
  
  /* open the key, as normal */
  status=((ZWOPENKEY)(OldZwOpenKey)) (
				      phKey,
				      DesiredAccess,
				      ObjectAttributes );  
  return status;
}



NTSTATUS NewZwCreateKey(
			PHANDLE phKey,
			ACCESS_MASK DesiredAccess,
			POBJECT_ATTRIBUTES ObjectAttributes,
			ULONG TitleIndex,
			PUNICODE_STRING Class,
			ULONG CreateOptions,
			PULONG pDisposition
			) {
  int status;
#ifndef NOTRY
  __try {
#endif
    if (restrictEnabled()) {
      WCHAR buf[1024]; 
      ACCESS_MASK mask;
      getFullPath(buf, sizeof(buf), ObjectAttributes);
      mask = isRestricted(RESTRICT, buf);
      if (mask>0) {
	//debugOutput(buf);
	//debugOutput(L": restrict2\n");
      }
      DesiredAccess &= ~mask;
    }
#ifndef NOTRY
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {
    debugOutput(L"NewZwCreateKey: EXCEPTION\n");
  }
#endif

  status=((ZWCREATEKEY)(OldZwCreateKey)) (
					  phKey,
					  DesiredAccess,
					  ObjectAttributes,
					  TitleIndex,
					  Class,
					  CreateOptions,
					  pDisposition);
  return status;
}



