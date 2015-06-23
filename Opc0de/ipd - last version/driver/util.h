/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* util.h */

#ifndef UTIL_H
#define UTIL_H

#include <ntddk.h>

int uwcscat(PWCHAR buf, PUNICODE_STRING str);
VOID uwcscpy(PWCHAR buf, PUNICODE_STRING str);
WCHAR *uwcsdup(PUNICODE_STRING src); /* paged alloc */
PUNICODE_STRING Uwcsdup(PUNICODE_STRING src); /* paged alloc */

int pathcmp(PWCHAR p1, PWCHAR p2);
int pathContainsPart(WCHAR *pathstart, WCHAR *pathend, WCHAR *item,
		     WCHAR *stop);

int getFullPath(PWCHAR buf, USHORT bufsize,
		POBJECT_ATTRIBUTES oa);

int regReadString(PWCHAR key, PWCHAR val, PUNICODE_STRING result);
NTSTATUS regReadStringCB(
			 IN PWSTR ValueName,
			 IN ULONG ValueType,
			 IN PVOID ValueData,
			 IN ULONG ValueLength,
			 IN PVOID Context,
			 IN PVOID EntryContext);

HANDLE openKey(WCHAR *key, ACCESS_MASK access);
NTSTATUS getKeyFullInformation(KEY_FULL_INFORMATION **info, WCHAR *key);

NTSTATUS getFileId(PUNICODE_STRING file, PLARGE_INTEGER result);
NTSTATUS getFileIdOA(POBJECT_ATTRIBUTES OA, PLARGE_INTEGER result);
NTSTATUS getFileIdHandle(HANDLE hfile, PLARGE_INTEGER result);
    

typedef struct RESTRICT_PARAM {
  WCHAR *obj;
  int len;
  ACCESS_MASK mask;
} RESTRICT_PARAM;

int restrictEnabled();
ACCESS_MASK isRestricted(RESTRICT_PARAM *list, PWCHAR key, int levs);


/* Dynamically binding to ntdll.dll
 * Taken directly from Windows NT/2000 Native API Reference
 */
PVOID FindNT();
PVOID FindFunc(PVOID Base, PCSTR Name);
  
#endif
