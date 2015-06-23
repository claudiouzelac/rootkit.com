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
#include "dutil.h"
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
  if (Globals.RESTRICT_ENABLED) return 1;
  KeQuerySystemTime(&curtime);
  diff = RtlLargeIntegerSubtract(curtime, Globals.DRIVERSTARTTIME);
  
  if (RtlLargeIntegerGreaterThan(diff,
				 Globals.RESTRICT_STARTUP_TIMEOUT)) {
    Globals.RESTRICT_ENABLED = 1;
    return 1;
  }
  return 0;
}


ACCESS_MASK isRestricted(RESTRICT_PARAM *list, PWCHAR key, int levs) {
  RESTRICT_PARAM *p = list;
  int cmp,i;
  WCHAR *s;
  if (!p || !key) return 0;

  while (p->obj != NULL) {
    cmp = _wcsnicmp(p->obj, key, p->len);
    if (cmp==0) {
      if (key[p->len] == L'\0') return p->mask;
      i=levs;
      s = key + p->len;
      while (i>0) {
	s = wcschr(s + 1, L'\\');
	if (s == NULL) return p->mask;
	--i;
      }
    }
    ++p;
  }
  return 0;
}



int regReadString(PWCHAR key, PWCHAR val, PUNICODE_STRING result) {
  WCHAR *path;
  RTL_QUERY_REGISTRY_TABLE qtbl[2];
  NTSTATUS status;

  path =
    ExAllocatePoolWithTag ( PagedPool, (wcslen(key)+20)*sizeof(WCHAR),
			    CRASH_TAG);
  if (!path) return -1;

  result->Buffer[0] = L'\0';
  result->Length = 0;
  wcscpy(path, L"\\Registry\\Machine\\");
  if (wcsncmp(key, path, wcslen(path))==0)
    wcscpy(path, key);
  else
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
  ExFreePool(path);
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
  if (Context && ValueData) {
    PUNICODE_STRING str = (PUNICODE_STRING) Context;
    if (ValueType == REG_SZ || ValueType == REG_EXPAND_SZ) {
      if (ValueLength+sizeof(UNICODE_NULL) <= str->MaximumLength) {
	RtlMoveMemory(str->Buffer, ValueData, ValueLength);
	str->Buffer[ValueLength/sizeof(WCHAR)] = UNICODE_NULL;
	str->Length = (USHORT)ValueLength;
      }
    }
    else if (ValueType == REG_DWORD) {
      if (str->MaximumLength >= 11*sizeof(WCHAR)) {
	swprintf(str->Buffer, L"%x", *((DWORD *)ValueData));
	str->Length = wcslen(str->Buffer) * sizeof(WCHAR);
      }
    }
  }
  return STATUS_SUCCESS;
}

HANDLE openKey(WCHAR *key, ACCESS_MASK access) {
  OBJECT_ATTRIBUTES OA;
  UNICODE_STRING str;
  NTSTATUS status;
  HANDLE hreg;
  RtlInitUnicodeString(&str, key);
  InitializeObjectAttributes(&OA, &str, OBJ_CASE_INSENSITIVE, NULL, NULL);
			     
  status = ZwOpenKey(&hreg, access, &OA);
  if (status != STATUS_SUCCESS) return NULL;
  return hreg;
}

NTSTATUS getKeyFullInformation(KEY_FULL_INFORMATION **info, WCHAR *key) {
  HANDLE hreg;
  ULONG resultlen;
  NTSTATUS status;
  
  hreg = openKey(key, KEY_READ);
  if (!hreg) return STATUS_OBJECT_PATH_NOT_FOUND;

  resultlen = sizeof(KEY_FULL_INFORMATION) + 100*sizeof(WCHAR);
  *info = ExAllocatePoolWithTag ( PagedPool, resultlen, CRASH_TAG);
  if (!*info) {
    ZwClose(hreg);
    return STATUS_NO_MEMORY;
  }
  
  status = ZwQueryKey(hreg, KeyFullInformation, *info, resultlen, &resultlen);
  if (status == STATUS_BUFFER_OVERFLOW) {
    ExFreePool(*info);
    *info = ExAllocatePoolWithTag ( PagedPool, resultlen, CRASH_TAG);
    if (*info)
      status =
	ZwQueryKey(hreg, KeyFullInformation, *info, resultlen, &resultlen);
  }
  if (status != STATUS_SUCCESS && *info) {
    ExFreePool(*info);
    *info = NULL;
  }

  ZwClose(hreg);
  return status;
}   



NTSTATUS getFileId(PUNICODE_STRING file, PLARGE_INTEGER result) {
  OBJECT_ATTRIBUTES OA;
  InitializeObjectAttributes(&OA, file, OBJ_CASE_INSENSITIVE,
			     NULL, NULL);
  return getFileIdOA(&OA, result);
}

NTSTATUS getFileIdOA(POBJECT_ATTRIBUTES OA, PLARGE_INTEGER result) {
  WCHAR buf[1024];
  NTSTATUS status;  
  HANDLE hfile;
  IO_STATUS_BLOCK iostatus;

  UNICODE_STRING t1, t2, t3, obj;
  int ok=0;

  if (!getFullPath(buf, sizeof(buf), OA)) {
    uwcscpy(buf, OA->ObjectName);
  }
  debugOutput(L"getFileIdOA object=");
  debugOutput(buf);
  debugOutput(L"\n");

  RtlInitUnicodeString(&obj, buf);
  RtlInitUnicodeString(&t1, L"\\device\\harddisk");
  RtlInitUnicodeString(&t2, L"\\??\\");
  RtlInitUnicodeString(&t3, L"\\device\\lanmanredirector");

  if (RtlPrefixUnicodeString(&t1, &obj, TRUE)) ok = 1;
  else if (RtlPrefixUnicodeString(&t2, &obj, TRUE)) {
    if (obj.Length > 5*sizeof(WCHAR) && obj.Buffer[5]==L':') ok=1;
  }
  else if (RtlPrefixUnicodeString(&t3, &obj, TRUE)) {
    ok=1;
  }
  if (!ok) return STATUS_OBJECT_NAME_INVALID;


  status = ZwOpenFile(&hfile,
		      0,
		      OA,
		      &iostatus,
		      0,
		      0);

  if (status == STATUS_SUCCESS) {
    status = getFileIdHandle(hfile, result);
    ZwClose(hfile);
  }
  return status;
}


NTSTATUS getFileIdHandle(HANDLE hfile, PLARGE_INTEGER result) {
  IO_STATUS_BLOCK iostatus;
  FILE_INTERNAL_INFORMATION info;
  NTSTATUS status;
  status = 
    ZwQueryInformationFile(hfile, &iostatus, &info, sizeof(info),
			   6 /* FileInternalInformation*/ );
  if (status == STATUS_SUCCESS) {
    *result = info.FileId;
  }
  return status;
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





/* Dynamically binding to ntdll.dll
 * From Windows NT/2000 Native API Reference
 */
PVOID FindNT() {
  ULONG n, *q;
  PSYSTEM_MODULE_INFORMATION p;
  PVOID ntdll=NULL;

  ZwQuerySystemInformation(SystemModuleInformation,
			   &n, 0, &n);
  q = (ULONG *) ExAllocatePool(PagedPool, n);
  ZwQuerySystemInformation(SystemModuleInformation, q, n*sizeof(*q), 0);
  p = (PSYSTEM_MODULE_INFORMATION)(q+1);
  
  for (n=0; n<*q; n++) {
    if (_stricmp(p[n].ImageName + p[n].ModuleNameOffset, "ntdll.dll")==0)
      ntdll = p[n].Base;
  }
  ExFreePool(q);
  return ntdll;
}


PVOID FindFunc(PVOID Base, PCSTR Name) {
  PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER) Base;
  PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(((char *)Base) + dos->e_lfanew);
  PIMAGE_DATA_DIRECTORY expdir = nt->OptionalHeader.DataDirectory +
    IMAGE_DIRECTORY_ENTRY_EXPORT;
  ULONG size = expdir->Size;
  ULONG addr = expdir->VirtualAddress;
  PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)(((char *)(Base))+
							      addr);
  PULONG functions = (PULONG)(((char *)Base) + exports->AddressOfFunctions);
  PSHORT ordinals = (PSHORT)(((char *)Base)+ exports->AddressOfNameOrdinals);
  PULONG names = (PULONG)(((char *)Base)+ exports->AddressOfNames);
  
  PVOID func=NULL;

  ULONG i;

  for (i=0; i<exports->NumberOfNames; i++) {
    ULONG ord = ordinals[i];
    if (functions[ord] < addr || functions[ord]>=addr+size) {
      if (strcmp(((char *)Base) + names[i], Name)==0) 
	func = ((char *)Base)+functions[ord];
    }
  }

  return func;
}
