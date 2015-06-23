/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* dutil.c */

#include "driver.h"
#include "util.h"

#ifdef DEBUG

void dumpbuf(void *p, int size) {
  int l,i;
  WCHAR buf[500];
  char c[5];
  for (l=0; l<size; l+=5) {
    for (i=0; i<5; i++) {
      c[i] = (*(((unsigned char *)p)+l+i));
      if (!isprint(c[i])) c[i]='.';
    }

    swprintf(buf,
	     L"%d 0x%0.2x 0x%0.2x 0x%0.2x 0x%0.2x 0x%0.2x %C%C%C%C%C\n",
	     l,
	     (*(((unsigned char *)p)+l)),
	     (*(((unsigned char *)p)+l+1)),
	     (*(((unsigned char *)p)+l+2)),
	     (*(((unsigned char *)p)+l+3)),
	     (*(((unsigned char *)p)+l+4)),
	     c[0], c[1], c[2], c[3], c[4]
	     );
    debugOutput(buf);
  }
}


void say_process_mask(ACCESS_MASK mask) {
  if ((mask & PROCESS_ALL_ACCESS) == PROCESS_ALL_ACCESS)
    debugOutput(L"   PROCESS_ALL_ACCESS\n");
  else {
    if (mask & PROCESS_TERMINATE) debugOutput(L"   TERMINATE\n");
    if (mask & PROCESS_CREATE_THREAD) debugOutput(L"   CREATE_THREAD\n");
    if (mask & PROCESS_SET_SESSIONID) debugOutput(L"   SET_SESSION_ID\n");
    if (mask & PROCESS_VM_OPERATION) debugOutput(L"   VM_OPERATION\n");
    if (mask & PROCESS_VM_READ) debugOutput(L"   VM_READ\n");
    if (mask & PROCESS_VM_WRITE) debugOutput(L"   VM_WRITE\n");
    if (mask & PROCESS_DUP_HANDLE) debugOutput(L"   DUP_HANDLE\n");
    if (mask & PROCESS_CREATE_PROCESS) debugOutput(L"   CREATE_PROCESS\n");
    if (mask & PROCESS_SET_QUOTA) debugOutput(L"   SET_QUOTA\n");
    if (mask & PROCESS_SET_INFORMATION) debugOutput(L"   SET_INFORMATION\n");
    if (mask & PROCESS_QUERY_INFORMATION)
      debugOutput(L"   QUERY_INFORMATION\n");
    if ((mask & STANDARD_RIGHTS_REQUIRED)==STANDARD_RIGHTS_REQUIRED)
      debugOutput(L"   STANDARD_RIGHTS_REQUIRED\n");
    else {
      if (mask & SYNCHRONIZE) debugOutput(L"   SYNCHRONIZE\n");
    }
  }
}


int dumpOA(WCHAR *tag, POBJECT_ATTRIBUTES OA) {
  WCHAR buf[1024];
  uwcscpy(buf, OA->ObjectName);
  debugOutput(tag);
  debugOutput(L": ");
  debugOutput(buf);
  debugOutput(L"\n");
  if (OA->Attributes & OBJ_INHERIT) debugOutput(L"     OBJ_INHERIT\n");
  if (OA->Attributes & OBJ_PERMANENT) debugOutput(L"     OBJ_PERMANENT\n");
  if (OA->Attributes & OBJ_EXCLUSIVE) debugOutput(L"     OBJ_EXCLUSIVE\n");
  if (OA->Attributes & OBJ_CASE_INSENSITIVE) debugOutput(L"     OBJ_CASE_INSENSITIVE\n");
  if (OA->Attributes & OBJ_OPENIF) debugOutput(L"    OBJ_OPENIF\n");
  if (OA->Attributes & OBJ_OPENLINK) debugOutput(L"     OBJ_OPENLINK\n");
  if (OA->Attributes & OBJ_KERNEL_HANDLE) debugOutput(L"    OBJ_KERNEL_HANDLE\n");

  debugOutput(L"     RootDirectory ");
  if (OA->RootDirectory) debugOutput(L" is not Null\n");
  else debugOutput(L" is Null\n");
  return 0;
}


int dumpCreateOptions(WCHAR *tag, ULONG opt) {
  debugOutput(tag);
  debugOutput(L"\n");
  if (opt & FILE_DIRECTORY_FILE) debugOutput(L"   FILE_DIRECTORY_FILE\n");
  if (opt & FILE_WRITE_THROUGH) debugOutput(L"   FILE_WRITE_THROUGH\n");
  if (opt & FILE_SEQUENTIAL_ONLY) debugOutput(L"   FILE_SEQUENTIAL_ONLY\n");
  if (opt & FILE_NO_INTERMEDIATE_BUFFERING) debugOutput(L"   FILE_NO_INTERMEDIATE_BUFFERING\n");
  if (opt & FILE_SYNCHRONOUS_IO_ALERT) debugOutput(L"   FILE_SYNCRONOUT_IO_ALERT\n");
  if (opt & FILE_SYNCHRONOUS_IO_NONALERT) debugOutput(L"   FILE_SYNCHRONOUS_IO_NONALERT\n");
  if (opt & FILE_NON_DIRECTORY_FILE) debugOutput(L"   FILE_NON_DIRECTORY_FILE\n");
  if (opt & FILE_CREATE_TREE_CONNECTION) debugOutput(L"   FILE_CREATE_TREE_CONNECTION\n");
  if (opt & FILE_COMPLETE_IF_OPLOCKED) debugOutput(L"   FILE_COMPLETE_IF_OPLOCKED\n");
  if (opt & FILE_NO_EA_KNOWLEDGE) debugOutput(L"   FILE_NO_EA_KNOWLEDGE\n");
  if (opt & FILE_OPEN_FOR_RECOVERY) debugOutput(L"   FILE_OPEN_FOR_RECOVERY\n");
  if (opt & FILE_RANDOM_ACCESS) debugOutput(L"   FILE_RANDOM_ACCESS\n");
  if (opt & FILE_DELETE_ON_CLOSE) debugOutput(L"   FILE_DELETE_ON_CLOSE\n");
  if (opt & FILE_OPEN_BY_FILE_ID) debugOutput(L"   FILE_OPEN_BY_FILE_ID\n");
  if (opt & FILE_OPEN_FOR_BACKUP_INTENT) debugOutput(L"   FILE_OPEN_FOR_BACKUP_INTENT\n");
  if (opt & FILE_NO_COMPRESSION) debugOutput(L"   FILE_NO_COMPRESSION\n");
  if (opt & FILE_RESERVE_OPFILTER) debugOutput(L"   FILE_RESERVE_OPFILTER\n");
  if (opt & FILE_OPEN_REPARSE_POINT) debugOutput(L"   FILE_OPEN_REPARSE_POINT\n");
  if (opt & FILE_OPEN_NO_RECALL) debugOutput(L"   FILE_OPEN_NO_RECALL\n");
  if (opt & FILE_OPEN_FOR_FREE_SPACE_QUERY) debugOutput(L"   FILE_OPEN_FOR_FREE_SPACE_QUERY\n");
  return 0;
}


#endif
