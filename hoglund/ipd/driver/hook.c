/*
 * Copyright (C) 2000 by Pedestal Software, LLC
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* hook.c */

#include "driver.h"
#include "hook.h"
#include "h_reg.h"
#include "h_file.h"


VOID hookInit() {
  // call before hookFuncs
  hookFileInit();
  hookRegInit();
}

VOID hookClose() {
  // call after unhookFuncs
  hookFileClose();
  hookRegClose();
}


ULONG getFuncIndex(void *fp, int ref) {
  ULONG c=0;
  WCHAR buf[1024];
  debugOutput(L"getFuncIndex\n");
  if (*((unsigned char *)fp) == 0xB8) {
    c = *((ULONG *)((unsigned char *)fp+1));
  }
  swprintf(buf,L"getFuncIndex: ref=%d c=%lx\n",ref,c);
  debugOutput(buf);
  return c;
}


VOID hookFuncs( void ) {
  WCHAR buf[1024];
  ULONG ent;
  int ref=0;
  swprintf(buf,L"hookFuncs. Number of services: %d\n",
	   KeServiceDescriptorTable.NumberOfServices);
  debugOutput(buf);

  _asm cli;

  /* registry */
  ent = getFuncIndex(ZwOpenKey,ref++);
  if (ent>0) {
    OldZwOpenKey=(ZWOPENKEY)FUNCSTABLE[ent];
    (ZWOPENKEY)FUNCSTABLE[ent]=NewZwOpenKey;
  }

  ent = getFuncIndex(ZwCreateKey,ref++);
  if (ent>0) {
    OldZwCreateKey=(ZWCREATEKEY)FUNCSTABLE[ent];
    (ZWCREATEKEY)FUNCSTABLE[ent]=NewZwCreateKey;
  }

  /* files */
  ent = getFuncIndex(ZwOpenFile,ref++);
  if (ent>0) {
    OldZwOpenFile=(ZWOPENFILE)FUNCSTABLE[ent];
    (ZWOPENFILE)FUNCSTABLE[ent]=NewZwOpenFile;
  }

  ent = getFuncIndex(ZwCreateFile,ref++);
  if (ent>0) {
    OldZwCreateFile=(ZWCREATEFILE)FUNCSTABLE[ent];
    (ZWCREATEFILE)FUNCSTABLE[ent]=NewZwCreateFile;
   }

  _asm sti;
}



VOID unhookFuncs( void ) {
  ULONG ent;
  int ref=0;
  _asm cli;

  /* registry */
  ent = getFuncIndex(ZwOpenKey,ref++);
  if (ent>0) (ZWOPENKEY)FUNCSTABLE[ent]=OldZwOpenKey;

  ent = getFuncIndex(ZwCreateKey,ref++);
  if (ent>0) (ZWCREATEKEY)FUNCSTABLE[ent]=OldZwCreateKey;

  /* files */
  ent = getFuncIndex(ZwOpenFile,ref++);
  if (ent>0) (ZWOPENFILE)FUNCSTABLE[ent]=OldZwOpenFile;

  ent = getFuncIndex(ZwCreateFile,ref++);
  if (ent>0) (ZWCREATEFILE)FUNCSTABLE[ent]=OldZwCreateFile;
	
  _asm sti;
}

