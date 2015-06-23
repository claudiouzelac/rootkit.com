/*
 * Copyright (C) 2000 by Pedestal Software, LLC
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
    

typedef struct RESTRICT_PARAM {
  WCHAR *obj;
  ACCESS_MASK mask;
} RESTRICT_PARAM;

int restrictEnabled();
ACCESS_MASK isRestricted(RESTRICT_PARAM *list, PWCHAR key);

  
#endif
