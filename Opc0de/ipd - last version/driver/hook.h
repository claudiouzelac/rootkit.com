/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

#ifndef HOOK_H
#define HOOK_H


/*
 * System Service Descriptor Table, from Undocumented Windows NT
 */
typedef struct {
  PVOID *ServiceTableBase;
  PVOID ServiceCounterTable;
  unsigned int NumberOfServices;
  PVOID ParamTableBase;
} ServiceDescriptorTable;

__declspec(dllimport) ServiceDescriptorTable KeServiceDescriptorTable;


#define FUNCSTABLE KeServiceDescriptorTable.ServiceTableBase

ULONG getFuncIndex(void *fp, int ref);
VOID hookInit();
VOID hookClose();
VOID hookFuncs();
VOID unhookFuncs();

#endif
