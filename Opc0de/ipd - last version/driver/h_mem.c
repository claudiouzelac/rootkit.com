/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* h_mem.c */

#include "driver.h"
#include "util.h"
#include "h_file.h"
#include "h_mem.h"


void hookMemInit() {
}
void hookMemClose() {
}

ZWOPENSECTION OldZwOpenSection;
ZWMAPVIEWOFSECTION OldZwMapViewOfSection;
ZWCREATESYMBOLICLINKOBJECT OldZwCreateSymbolicLinkObject;


NTSTATUS NewZwOpenSection(
			  PHANDLE pHandle,
			  ACCESS_MASK DesiredAccess,
			  POBJECT_ATTRIBUTES ObjectAttributes
			  ) {
  NTSTATUS status;

  if (restrictEnabled()) {
    if (ObjectAttributes && ObjectAttributes->ObjectName &&
	ObjectAttributes->ObjectName->Length>0) {
      if (_wcsicmp(ObjectAttributes->ObjectName->Buffer,
		   L"\\Device\\PhysicalMemory")==0) {
	WCHAR buf[200];
	swprintf(buf, L"Blocking device/PhysicalMemory access, procid=0x%x\n",
		 PsGetCurrentProcessId());
	debugOutput(buf);
	return STATUS_ACCESS_DENIED;
      }
    }
  }
  status = (OldZwOpenSection)(pHandle, DesiredAccess, ObjectAttributes);
  return status;
}



NTSTATUS NewZwMapViewOfSection(
			       HANDLE SectionHandle,
			       HANDLE  ProcessHandle,
			       PVOID  *BaseAddress,
			       ULONG  ZeroBits,
			       ULONG  CommitSize,
			       PLARGE_INTEGER  SectionOffset,
			       PSIZE_T  ViewSize,
			       SECTION_INHERIT  InheritDisposition,
			       ULONG  AllocationType,
			       ULONG  Protect
			       ) {
  NTSTATUS status;
#ifdef DEBUG
  NTSTATUS rtn;
  PVOID Object;
  WCHAR buf[1024];

  rtn=
    ObReferenceObjectByHandle(SectionHandle,
			      0,
			      0,
			      KernelMode,
			      &Object,
			      NULL);
    
  if (rtn==STATUS_SUCCESS) {
    int bytes;
    rtn=ObQueryNameString(Object,
			  (PUNICODE_STRING)buf,
			  sizeof(buf),
			  &bytes);
    ObDereferenceObject(Object);
    if (rtn==STATUS_SUCCESS) {
      WCHAR *p = ((PUNICODE_STRING)buf)->Buffer;
      debugOutput(L"MapViewOfSection ");
      debugOutput(p);

      if ((Protect&PAGE_READWRITE) || 
	  (Protect&PAGE_EXECUTE_READWRITE)) {
	swprintf(buf,L" protect: 0x%lx\n", Protect);
	debugOutput(buf);
      }
      debugOutput(L"\n");
    }
  }
#endif

  status = (OldZwMapViewOfSection)(SectionHandle, ProcessHandle,
				   BaseAddress, ZeroBits,
				   CommitSize, SectionOffset,
				   ViewSize, InheritDisposition,
				   AllocationType,
				   Protect);
  return status;
}




NTSTATUS NewZwCreateSymbolicLinkObject(PHANDLE SymLinkHandle,
				       ACCESS_MASK DesiredAccess,
				       POBJECT_ATTRIBUTES ObjectAttributes,
				       PUNICODE_STRING ObjectName
				       ) {
  NTSTATUS status;

  if (restrictEnabled()) {
    if (ObjectName && ObjectName->Buffer && ObjectName->Length>0) {
#ifdef DEBUG
      debugOutput(L"Incoming link to: ");
      debugOutput(ObjectName->Buffer);
      debugOutput(L"\n");
#endif
      if (_wcsicmp(ObjectName->Buffer, L"\\Device\\PhysicalMemory")==0) {
	WCHAR buf[200];
	swprintf(buf, L"Blocking device/PhysicalMemory access, procid=0x%x\n",
		 PsGetCurrentProcessId());
	debugOutput(buf);
	return STATUS_ACCESS_DENIED;
      }
    }
  }
  status = (OldZwCreateSymbolicLinkObject)(SymLinkHandle, DesiredAccess, ObjectAttributes, ObjectName);
  return status;
}

