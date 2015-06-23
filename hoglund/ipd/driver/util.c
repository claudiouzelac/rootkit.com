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
#include "util.h"


int uwcscat(PWCHAR buf, PUNICODE_STRING str) {
  WCHAR *p, *q;
  USHORT l;
  int elen=0;
  if (!str) return 0;
  p = buf;
  q = str->Buffer;
  l=(str->Length)/sizeof(WCHAR);
  if (!str->Buffer) return 0;
  while (*p++ != L'\0');
  --p;
  while(l-->0 && *q!=L'\0') { *p++ = *q++; ++elen; }
  *p = L'\0';
  return elen;
}

VOID uwcscpy(PWCHAR buf, PUNICODE_STRING str) {
  *buf=L'\0';
  uwcscat(buf, str);
}


WCHAR *uwcsdup(PUNICODE_STRING src) {
  WCHAR *rtn;
  if (!src) return NULL;

  rtn =
    ExAllocatePoolWithTag ( PagedPool,
			    src->Length+sizeof(UNICODE_NULL),
			    CRASH_TAG);
  if (!rtn) {
    debugOutput(L"Allocate paged pool failed, uwcsdup\n");
  }
  else {
    RtlMoveMemory(rtn, src->Buffer, src->Length);
    *(rtn+src->Length/sizeof(WCHAR)) = UNICODE_NULL;
  }
  return rtn;
}


PUNICODE_STRING Uwcsdup(PUNICODE_STRING src) {
  PUNICODE_STRING rtn;
  if (!src) return NULL;

  rtn =
    ExAllocatePoolWithTag ( PagedPool,
			    sizeof(UNICODE_STRING)+src->Length+
			    sizeof(UNICODE_NULL),
			    CRASH_TAG);
  if (!rtn) {
    debugOutput(L"Allocate paged pool failed, Uwcsdup\n");
  }
  else {
    rtn->Length = src->Length;
    rtn->MaximumLength = src->Length + sizeof(UNICODE_NULL);
    rtn->Buffer = (WCHAR *)(((unsigned char *)rtn)+
			    sizeof(UNICODE_STRING));
    RtlMoveMemory(rtn->Buffer, src->Buffer, src->Length);
    *(rtn->Buffer+src->Length/sizeof(WCHAR)) = UNICODE_NULL;
  }
  return rtn;
}



int pathcmp(PWCHAR p1, PWCHAR p2) {
  WCHAR *p = p1, *q = p2;
  while(*p != L'\0' && *q != L'\0' &&
	towlower(*p++) == towlower(*q++));
  if (*p==L'\0' && *q!=L'\0') return 1; // partial path, p2 contains p1
  if (*p!=L'\0' && *q==L'\0') return 2; // partial path, p1 contains p2
  if (*p==L'\0') return 0; // exact match
  return -1; // no match or partial match
}


int getFullPath(PWCHAR buf, USHORT bufsize,
		POBJECT_ATTRIBUTES oa) {
  NTSTATUS rtn;
  PVOID Object;
  int curlen=0;

  buf[0]=L'\0';
  if (!oa) return 0;

  if (oa->RootDirectory != NULL) {
    rtn=
      ObReferenceObjectByHandle(oa->RootDirectory,
				0,
				0,
				KernelMode,
				&Object,
				NULL);

    if (rtn==STATUS_SUCCESS) {
      int bytes;
      rtn=ObQueryNameString(Object,
			    (PUNICODE_STRING)buf,
			    bufsize,
			    &bytes);
      ObDereferenceObject(Object);
      if (rtn==STATUS_SUCCESS) {
	WCHAR *p = ((PUNICODE_STRING)buf)->Buffer, *q=buf;
	USHORT len = (((PUNICODE_STRING)buf)->Length)/sizeof(WCHAR);
	if ((len+2)*sizeof(WCHAR)<bufsize) {
	  while (len-->0 && *p!=L'\0') {
	    *q++ = *p++;
	    ++curlen;
	  }
	  *q++=OBJ_NAME_PATH_SEPARATOR;
	  ++curlen;
	}
	*q = L'\0';
      }
    }
  }

  if (oa->ObjectName &&
      oa->Length+(curlen+1)*sizeof(WCHAR) < bufsize) {
    curlen += uwcscat(buf+curlen, oa->ObjectName);
  }
  else *buf = L'\0';
  return curlen;
}



int restrictEnabled() {
  LARGE_INTEGER curtime, diff;
  KeQuerySystemTime(&curtime);
  diff = RtlLargeIntegerSubtract(curtime, Globals.DRIVERSTARTTIME);
  
  if (RtlLargeIntegerGreaterThan(diff,
				 Globals.RESTRICT_STARTUP_TIMEOUT))
    return 1;
  return 0;
}


ACCESS_MASK isRestricted(RESTRICT_PARAM *list, PWCHAR key) {
  RESTRICT_PARAM *p = list;
  int cmp;
  if (!p || !key) return 0;

  while (p->obj != NULL) {
    cmp = pathcmp(p->obj, key);
    if (cmp==0 || cmp==1) {
      return p->mask;
    }
    ++p;
  }
  return 0;
}



int regReadString(PWCHAR key, PWCHAR val, PUNICODE_STRING result) {
  WCHAR path[1024];
  RTL_QUERY_REGISTRY_TABLE qtbl[2];
  NTSTATUS status;

  result->Buffer[0] = L'\0';
  wcscpy(path, L"\\Registry\\Machine\\");
  wcscat(path, key);

  qtbl[0].QueryRoutine = regReadStringCB;
  qtbl[0].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
  qtbl[0].Name = val;
  qtbl[0].EntryContext = NULL;
  qtbl[0].DefaultType = REG_NONE;
  qtbl[0].DefaultData = NULL;
  qtbl[0].DefaultLength = 0;

  memset(&(qtbl[1]), 0, sizeof(qtbl[1]));
  status = 
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
			   path,
			   qtbl,
			   result,
			   NULL);
  if (status == STATUS_SUCCESS) return 0;
  return -1;
}

NTSTATUS regReadStringCB(
			 IN PWSTR ValueName,
			 IN ULONG ValueType,
			 IN PVOID ValueData,
			 IN ULONG ValueLength,
			 IN PVOID Context,
			 IN PVOID EntryContext) {
  if (Context && ValueData &&
      (ValueType == REG_SZ || ValueType == REG_EXPAND_SZ)) {
    PUNICODE_STRING str = (PUNICODE_STRING) Context;
    if (ValueLength+sizeof(UNICODE_NULL) <= str->MaximumLength) {
      RtlMoveMemory(str->Buffer, ValueData, ValueLength);
      str->Buffer[ValueLength/sizeof(WCHAR)] = UNICODE_NULL;
      str->Length = (USHORT)ValueLength;
    }
  }
  return STATUS_SUCCESS;
}



int pathContainsPart(WCHAR *pathstart, WCHAR *pathend, WCHAR *item,
		     WCHAR *stop) {
  WCHAR *p = pathend, *q=NULL;
  do {
    while (p > pathstart && *p != L'\\') --p;
    if (p<=pathstart) { if (q) *q=L'\\'; break; }
    if (pathcmp(p+1,item)==0) { if (q) *q=L'\\'; return 1; }
    if (pathcmp(p+1,stop)==0) { if (q) *q=L'\\'; return 0; }
    if (q!=NULL) *q=L'\\';
    q=p;
    *p-- = L'\0';
  } while (1);
  return 0;
}
