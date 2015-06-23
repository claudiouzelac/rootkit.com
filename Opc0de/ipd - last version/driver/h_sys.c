/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* h_sys.c */

#include "driver.h"
#include "util.h"
#include "dutil.h"
#include "h_tok.h"
#include "h_sys.h"

ZWSETSYSTEMINFORMATION OldZwSetSystemInformation;

void hookSysInit() {
}
void hookSysClose() {
}


NTSTATUS NewZwSetSystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength) {
  NTSTATUS status;
  PEPROCESS proc;
  HANDLE procid;
  
  proc = PsGetCurrentProcess();
  procid = PsGetCurrentProcessId();

  if (restrictEnabled() &&
      (SystemInformationClass==SystemLoadAndCallImage ||
       SystemInformationClass==SystemLoadImage)
      && (ULONG)procid != processid_smss
      //&& !isImageAllow(procid)) {
      && _strnicmp(((char *)proc)+EPROCESS_NAME_OFFSET,
		   "csrss.exe", 9)!=0) {
#ifdef DEBUG
    WCHAR buf[512];
    debugOutput(L"NewZwSetSystemInformation: ");
    swprintf(buf, L"proc 0x%lx ", procid);
    debugOutput(buf);
    dumpbuf(((char *)proc)+EPROCESS_NAME_OFFSET, 15);
    if (SystemInformationClass==SystemLoadAndCallImage)
      debugOutput(L": LoadAndCallImage\n");
    else
      debugOutput(L": LoadImage\n");
#endif
    return STATUS_ACCESS_DENIED;
  }

#ifndef NOTRY
  __try {
#endif
    status = (OldZwSetSystemInformation)(SystemInformationClass,
					 SystemInformation,
					 SystemInformationLength);
    return status;
#ifndef NOTRY
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {
    debugOutput(L"ZwSetSystemInformation: EXCEPTION\n");
  }
  return(STATUS_UNSUCCESSFUL);
#endif
}

