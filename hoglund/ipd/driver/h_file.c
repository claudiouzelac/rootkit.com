/*
 * Copyright (C) 2000 by Pedestal Software, LLC
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* h_file.c */

#include "driver.h"
#include "h_file.h"
#include "util.h"

ZWCREATEFILE OldZwCreateFile;
ZWOPENFILE   OldZwOpenFile;

static RESTRICT_PARAM *RESTRICT=NULL; /* r/o after init */
WCHAR STR_DRIVERS_PATH[] = L"\\system32\\drivers";
WCHAR STR_ETC[] = L"etc";
WCHAR STR_DRIVERS[] = L"drivers";

/* hookFileInit()
 *
 * find the actual 'drivers' paths, eg:
 *      c:\winnt\system32\drivers
 *  and \Device\HarddiskVolume1\winnt\system32\drivers
 */
void hookFileInit() {
  ACCESS_MASK mask =
    GENERIC_WRITE|GENERIC_ALL|DELETE|FILE_WRITE_DATA|FILE_APPEND_DATA|
    FILE_DELETE_CHILD;
  UNICODE_STRING systemroot, systempartition;
  WCHAR systemrootbuf[_MAX_PATH+1],
    systempartitionbuf[_MAX_PATH+1];

  RESTRICT =
    ExAllocatePoolWithTag ( PagedPool,
			    sizeof(RESTRICT_PARAM) * 3,
			    CRASH_TAG);
  if (!RESTRICT) return;

  RESTRICT[0].obj =
    ExAllocatePoolWithTag( PagedPool, _MAX_PATH+1, CRASH_TAG);
  if (!RESTRICT[0].obj) return;
  RESTRICT[0].mask = mask;

  systemroot.Length = 0;
  systemroot.MaximumLength = _MAX_PATH*sizeof(WCHAR);
  systemroot.Buffer = systemrootbuf;
  regReadString(L"software\\microsoft\\windows nt\\currentversion",
		L"SystemRoot", &systemroot);
  wcscpy(RESTRICT[0].obj, L"\\??\\");
  uwcscat(RESTRICT[0].obj, &systemroot);
  wcscat(RESTRICT[0].obj, STR_DRIVERS_PATH);
  
  debugOutput(RESTRICT[0].obj);
  debugOutput(L"\n");

  if (systemroot.Length >= 3*sizeof(WCHAR)) {
    WCHAR *p;
    RESTRICT[1].obj =
      ExAllocatePoolWithTag( PagedPool, _MAX_PATH+1, CRASH_TAG);
    if (!RESTRICT[1].obj) return;
    RESTRICT[1].mask = mask;
    systempartition.Length = 0;
    systempartition.MaximumLength = _MAX_PATH*sizeof(WCHAR);
    systempartition.Buffer = systempartitionbuf;
    regReadString(L"system\\currentcontrolset\\control\\hivelist",
		  L"\\registry\\machine\\sam", &systempartition);
    p = systempartitionbuf + systempartition.Length/sizeof(WCHAR) - 1;
    do {
      while (p > systempartitionbuf && *p != L'\\') p--;
      if (p<=systempartitionbuf) break;
      if (pathcmp(p,systemrootbuf+2)==0) {
	uwcscpy(RESTRICT[1].obj, &systempartition);
	wcscat(RESTRICT[1].obj, STR_DRIVERS_PATH);
	p=NULL;
	break;
      }
      *p-- = L'\0';
    } while (1);
    if (p) RESTRICT[1].obj[0]=L'\0';

    debugOutput(RESTRICT[1].obj);
    debugOutput(L"\n");  
  }
  else RESTRICT[1].obj = NULL;

  RESTRICT[2].obj = NULL;
  RESTRICT[2].mask = 0;
}


void hookFileClose() {
  RESTRICT_PARAM *p = RESTRICT;
  if (!p) return;
  while (p->obj) {
    ExFreePool(p->obj);
    ++p;
  }
  ExFreePool(RESTRICT);
  RESTRICT=NULL;
}



NTSTATUS
NewZwOpenFile(
	      PHANDLE phFile,
	      ACCESS_MASK DesiredAccess,
	      POBJECT_ATTRIBUTES ObjectAttributes,
	      PIO_STATUS_BLOCK pIoStatusBlock,
	      ULONG ShareMode,
	      ULONG OpenMode
	      ) {
  int status;
#ifndef NOTRY
  __try {
#endif
    if (restrictEnabled()) {
      WCHAR buf[_MAX_PATH+1];
      ACCESS_MASK mask;
      int l = getFullPath(buf, sizeof(buf), ObjectAttributes);
      mask = isRestricted(RESTRICT, buf);
      if (mask>0) {
	if (pathContainsPart(buf, buf+l-1, STR_ETC, STR_DRIVERS)) {
	  mask=0;
	}
	else {
	  debugOutput(buf);
	  debugOutput(L": NewZwOpenFile\n");
	}
      }
      DesiredAccess &= ~mask;
    }
#ifndef NOTRY
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {
    debugOutput(L"NewZwOpenFile: EXCEPTION\n");
  }
#endif

  status=((ZWOPENFILE)(OldZwOpenFile)) (
					phFile,
					DesiredAccess,
					ObjectAttributes,
					pIoStatusBlock,
					ShareMode,
					OpenMode);
  return status;
}



NTSTATUS
NewZwCreateFile(
		PHANDLE FileHandle,
		ACCESS_MASK DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes,
		PIO_STATUS_BLOCK IoStatusBlock,
		PLARGE_INTEGER AllocationSize OPTIONAL,
		ULONG FileAttributes,
		ULONG ShareAccess,
		ULONG CreateDisposition,
		ULONG CreateOptions,
		PVOID EaBuffer OPTIONAL,
		ULONG EaLength) {
  int status; 
#ifndef NOTRY
  __try {
#endif
    if (restrictEnabled()) {
      WCHAR buf[_MAX_PATH+1];
      ACCESS_MASK mask;
      int l = getFullPath(buf, sizeof(buf), ObjectAttributes);
      mask = isRestricted(RESTRICT, buf);
      if (mask>0) {
	if (pathContainsPart(buf, buf+l-1, STR_ETC, STR_DRIVERS)) {
	  mask=0;
	}
	else {
	  debugOutput(buf);
	  debugOutput(L": NewZwCreateFile\n");
	}
      }
      DesiredAccess &= ~mask;
    }
#ifndef NOTRY
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {
    debugOutput(L"NewZwCreateFile: EXCEPTION\n");
  }
#endif
  
  status=((ZWCREATEFILE)(OldZwCreateFile)) (
					    FileHandle,
					    DesiredAccess,
					    ObjectAttributes,
					    IoStatusBlock,
					    AllocationSize,
					    FileAttributes,
					    ShareAccess,
					    CreateDisposition,
					    CreateOptions,
					    EaBuffer,
					    EaLength);
  return status;
}


