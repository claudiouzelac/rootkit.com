/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* h_file.c */

#include "driver.h"
#include "util.h"
#include "dutil.h"
#include "h_file.h"

ZWCREATEFILE OldZwCreateFile;
ZWOPENFILE   OldZwOpenFile;

int FILEIDCOUNT=0;
HANDLE *FILEHANDLES=NULL;


/* hookFileInit()
 *
 *  obtain a list of file id's for driver files
 */


void hookFileInit() {
  NTSTATUS status;
  KEY_FULL_INFORMATION *info = NULL;
  WCHAR *CCS = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services";
  HANDLE hreg = NULL;
  KEY_BASIC_INFORMATION *keyname = NULL;
  ULONG keybasicsize, resultlen;
  DWORD keycount=0;
  UNICODE_STRING value, tmp;
  WCHAR buf[_MAX_PATH+1];
  WCHAR *key;
  UNICODE_STRING systemroot;
  WCHAR systemrootbuf[_MAX_PATH+1];

  /*
   * get systemroot
   */
  systemrootbuf[0] = 0;
  systemroot.Length = 0;
  systemroot.MaximumLength = _MAX_PATH*sizeof(WCHAR);
  systemroot.Buffer = systemrootbuf;
  regReadString(L"software\\microsoft\\windows nt\\currentversion",
		L"SystemRoot", &systemroot);
  debugOutput(L"Systemroot=");
  debugOutput(systemroot.Buffer);
  debugOutput(L"\n");

  /*
   * get key info
   */
  status = getKeyFullInformation(&info, CCS);
  if (status != STATUS_SUCCESS) return;
  FILEHANDLES = (HANDLE *)
    ExAllocatePoolWithTag ( PagedPool, sizeof(HANDLE) * (info->SubKeys),
			    CRASH_TAG);
  if (!FILEHANDLES) return;

  keybasicsize = sizeof(KEY_BASIC_INFORMATION)+ sizeof(WCHAR) *
    (info->MaxNameLen + 1);
  ExFreePool(info);


  /*
   * loop over each subkey
   */
  hreg = openKey(CCS, KEY_ENUMERATE_SUB_KEYS);
  if (!hreg) return;
  
  keyname = (KEY_BASIC_INFORMATION *)
    ExAllocatePoolWithTag( PagedPool, keybasicsize, CRASH_TAG);
  if (!keyname) {
    ZwClose(hreg);
    return;
  }

  do {
    status = ZwEnumerateKey(hreg, keycount++, KeyBasicInformation,
			    keyname, keybasicsize, &resultlen);
    if (status != STATUS_SUCCESS) break;

    value.Length = 0;
    value.MaximumLength = _MAX_PATH*sizeof(WCHAR);
    value.Buffer = buf;

    key = (WCHAR *)
      ExAllocatePoolWithTag( PagedPool, wcslen(CCS)*sizeof(WCHAR)+
			     2*sizeof(WCHAR)+keyname->NameLength,
			     CRASH_TAG);
    if (!key) break;
    tmp.Length = keyname->NameLength;
    tmp.MaximumLength = keyname->NameLength;
    tmp.Buffer = keyname->Name;
    wcscpy(key, CCS);
    wcscat(key, L"\\");
    uwcscat(key, &tmp);
    debugOutput(L"regReadString1: ");
    debugOutput(key);
    debugOutput(L"\n");

    if (regReadString(key, L"type", &value) == 0) {
      debugOutput(L"regReadString1 type=");
      debugOutput(value.Buffer);
      debugOutput(L"\n");
      if (wcscmp(value.Buffer, L"1")==0 ||
	  wcscmp(value.Buffer, L"2")==0) {

	if (regReadString(key, L"ImagePath", &value) == 0) {
	  debugOutput(L"regReadString2 ImagePath=");
	  debugOutput(value.Buffer);
	  debugOutput(L"\n");
	  if (value.Length > 0) {
	    WCHAR *pathbuf;
	    UNICODE_STRING sr1, sr2;
	    RtlInitUnicodeString(&sr1, L"\\SystemRoot\\");
	    RtlInitUnicodeString(&sr2, L"%SystemRoot%");

	    pathbuf = (WCHAR *)
	      ExAllocatePool(PagedPool, systemroot.Length + value.Length + 
			     6*sizeof(WCHAR));
	    if (pathbuf) {
	      UNICODE_STRING path;
	      OBJECT_ATTRIBUTES OA;
	      IO_STATUS_BLOCK iostatus;

	      if (wcsncmp(value.Buffer, L"\\??\\", 4) == 0) {
		wcscpy(pathbuf, value.Buffer);
	      }
	      else if (RtlPrefixUnicodeString(&sr1, &value, TRUE)) {
		wcscpy(pathbuf, L"\\??\\");
		wcscat(pathbuf, systemrootbuf);
		wcscat(pathbuf, value.Buffer + 11);
	      }
	      else if (RtlPrefixUnicodeString(&sr2, &value, TRUE)) {
		wcscpy(pathbuf, L"\\??\\");
		wcscat(pathbuf, systemrootbuf);
		wcscat(pathbuf, value.Buffer + 12);
	      }
	      else {
		wcscpy(pathbuf, L"\\??\\");
		wcscat(pathbuf, systemrootbuf);
		wcscat(pathbuf, L"\\");
		wcscat(pathbuf, value.Buffer);
	      }
	      RtlInitUnicodeString(&path, pathbuf);
	      debugOutput(L"getFileId of path=");
	      debugOutput(pathbuf);
	      debugOutput(L"\n");

	      InitializeObjectAttributes(&OA, &path, OBJ_CASE_INSENSITIVE,
					 NULL, NULL);
	      if (ZwOpenFile(&(FILEHANDLES[FILEIDCOUNT]),
			     FILE_READ_DATA,
			     &OA,
			     &iostatus,
			     FILE_SHARE_READ,
			     FILE_NON_DIRECTORY_FILE) == STATUS_SUCCESS) {
		++FILEIDCOUNT;
	      }

	      ExFreePool(pathbuf);
	    }
	  }
	}
      }
    }
    ExFreePool(key);
  } while (status == STATUS_SUCCESS);
  
  ExFreePool(keyname);
  ZwClose(hreg);
}


void hookFileClose() {
  if (FILEHANDLES) {
    int i;
    for (i=0; i<FILEIDCOUNT; i++) {
      if (FILEHANDLES[i]) ZwClose(FILEHANDLES[i]);
    }
    ExFreePool(FILEHANDLES);
    FILEHANDLES = NULL;
  }
  FILEIDCOUNT = 0;
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
#ifdef DEBUG
  WCHAR buf[1024];
  //dumpOA(L"NewZwOpenFile", ObjectAttributes);
  if (getFullPath(buf, sizeof(buf), ObjectAttributes)) {
    debugOutput(L"NewZwOpenFile: ");
    debugOutput(buf);
    debugOutput(L"\n");
  }
#endif
  status=((ZWOPENFILE)(OldZwOpenFile))(
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
#ifdef DEBUG
  WCHAR buf[1024];
  //dumpOA(L"NewZwCreateFile", ObjectAttributes);
  if (getFullPath(buf, sizeof(buf), ObjectAttributes)) {
    debugOutput(L"NewZwOpenFile: ");
    debugOutput(buf);
    debugOutput(L"\n");
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


