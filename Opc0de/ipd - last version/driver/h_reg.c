/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
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
ZWSETVALUEKEY   OldZwSetValueKey;

static RESTRICT_PARAM RESTRICT[] = {
  // new services/drivers
  { L"\\Registry\\Machine\\System\\CurrentControlSet\\Services",
    51,
    KEY_CREATE_SUB_KEY|KEY_SET_VALUE|DELETE|GENERIC_WRITE|GENERIC_ALL },
  { L"\\Registry\\Machine\\System\\ControlSet001\\Services",
    47,
    KEY_CREATE_SUB_KEY|KEY_SET_VALUE|DELETE|GENERIC_WRITE|GENERIC_ALL },
  { L"\\Registry\\Machine\\System\\ControlSet002\\Services",
    47,
    KEY_CREATE_SUB_KEY|KEY_SET_VALUE|DELETE|GENERIC_WRITE|GENERIC_ALL },
  { L"\\Registry\\Machine\\System\\ControlSet003\\Services",
    47,
    KEY_CREATE_SUB_KEY|KEY_SET_VALUE|DELETE|GENERIC_WRITE|GENERIC_ALL },
  { NULL, 0, 0 }
};

static WCHAR *RESTRICT_VALS[] = {
  L"DependOnGroup",
  L"DependOnService",
  L"Description",
  L"DisplayName",
  L"ErrorControl",
  L"ImagePath",
  L"ObjectName",
  L"Start",
  L"Type",
  L"Group",
  L"Tag",
  L"FailureActions",
  L"FailureCommand",
  NULL };
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
  NTSTATUS status;
  WCHAR buf[1024];
#ifndef NOTRY
  __try {
#endif
    if (restrictEnabled()) {
      ACCESS_MASK mask;
      int l = getFullPath(buf, sizeof(buf), ObjectAttributes);
      mask = isRestricted(RESTRICT, buf, 1);
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
  status=(OldZwOpenKey)(phKey,
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
  NTSTATUS status;
  ACCESS_MASK mask=0;
#ifndef NOTRY
  __try {
#endif
    if (restrictEnabled()) {
      WCHAR buf[1024]; 
      getFullPath(buf, sizeof(buf), ObjectAttributes);
      mask = isRestricted(RESTRICT, buf, 1);
      DesiredAccess &= ~mask;
    }
#ifndef NOTRY
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {
    debugOutput(L"NewZwCreateKey: EXCEPTION\n");
  }
#endif

  status=(OldZwCreateKey)(phKey,
			  DesiredAccess,
			  ObjectAttributes,
			  TitleIndex,
			  Class,
			  CreateOptions,
			  pDisposition);
  return status;
}



NTSTATUS NewZwSetValueKey(
			  HANDLE  KeyHandle,
			  PUNICODE_STRING  ValueName,
			  ULONG  TitleIndex,
			  ULONG  Type,
			  PVOID  Data,
			  ULONG  DataSize
			  ) {
  NTSTATUS status;

  if (restrictEnabled()) {
    NTSTATUS rtn;
    PVOID Object;
    WCHAR buf[1024];

    rtn=
      ObReferenceObjectByHandle(KeyHandle,
				0,
				0,
				KernelMode,
				&Object,
				NULL);
    
    if (NT_SUCCESS(rtn)) {
      int bytes;
      rtn=ObQueryNameString(Object,
			    (PUNICODE_STRING)buf,
			    sizeof(buf),
			    &bytes);
      ObDereferenceObject(Object);
      if (NT_SUCCESS(rtn)) {
	int l = ((PUNICODE_STRING)buf)->Length/sizeof(WCHAR);
	WCHAR *p = ((PUNICODE_STRING)buf)->Buffer;
	if (isRestricted(RESTRICT, p, 1)) {
	  WCHAR **q = RESTRICT_VALS;
	  while (*q != NULL) {
	    if (_wcsicmp(*q, ValueName->Buffer)==0) {
	      debugOutput(L"Restrict: ");
	      debugOutput(p);
	      debugOutput(L" ");
	      debugOutput(*q);
	      debugOutput(L"\n");
	      return STATUS_ACCESS_DENIED;
	    }
	    ++q;
	  }
	}
      }
    }
  }


  status = (OldZwSetValueKey)(
			      KeyHandle,
			      ValueName,
			      TitleIndex,
			      Type,
			      Data,
			      DataSize);
  return status;
}
