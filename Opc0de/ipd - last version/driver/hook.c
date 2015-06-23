/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
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
#include "util.h"
#include "h_reg.h"
#include "h_file.h"
#include "h_tok.h"
#include "h_mem.h"
#include "h_sys.h"


VOID hookInit() {
  // call before hookFuncs
  hookFileInit();
  hookRegInit();
  hookTokInit();
  hookMemInit();
  hookSysInit();
}

VOID hookClose() {
  // call after unhookFuncs
  hookFileClose();
  hookRegClose();
  hookTokClose();
  hookMemClose();
  hookSysClose();
}


ULONG getFuncIndex(void *fp, int ref) {
  ULONG c=0;
  WCHAR buf[1024];
  if (fp && *((unsigned char *)fp) == 0xB8) {
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

  /* mem */
#ifdef HOOK_ZWOPENSECTION
  ent = getFuncIndex(ZwOpenSection,ref++);
  if (ent>0) {
    OldZwOpenSection=(ZWOPENSECTION)FUNCSTABLE[ent];
    (ZWOPENSECTION)FUNCSTABLE[ent]=NewZwOpenSection;
  }
#endif

#ifdef HOOK_ZWMAPVIEWOFSECTION
  ent = getFuncIndex(ZwMapViewOfSection,ref++);
  if (ent>0) {
    OldZwMapViewOfSection=(ZWMAPVIEWOFSECTION)FUNCSTABLE[ent];
    (ZWMAPVIEWOFSECTION)FUNCSTABLE[ent]=NewZwMapViewOfSection;
  }
#endif

#ifdef HOOK_ZWCREATESYMBOLICLINKOBJECT
  ent = getFuncIndex(FindFunc(FindNT(),"ZwCreateSymbolicLinkObject"), ref++);
  if (ent>0) {
    OldZwCreateSymbolicLinkObject=(ZWCREATESYMBOLICLINKOBJECT)FUNCSTABLE[ent];
    (ZWCREATESYMBOLICLINKOBJECT)FUNCSTABLE[ent]=NewZwCreateSymbolicLinkObject;
  }
#endif


  /* tok */
#ifdef HOOK_ZWADJUSTPRIVILEGESTOKEN
  ent = getFuncIndex(FindFunc(FindNT(),"NtAdjustPrivilegesToken"), ref++);
  if (ent>0) {
    OldZwAdjustPrivilegesToken = (ZWADJUSTPRIVILEGESTOKEN)FUNCSTABLE[ent];
    (ZWADJUSTPRIVILEGESTOKEN)FUNCSTABLE[ent] = NewZwAdjustPrivilegesToken;
  }
#endif

#ifdef HOOK_ZWSETINFORMATIONTOKEN
  ent = getFuncIndex(FindFunc(FindNT(),"ZwSetInformationToken"), ref++);
  if (ent>0) {
    OldZwSetInformationToken = (ZWSETINFORMATIONTOKEN)FUNCSTABLE[ent];
    (ZWSETINFORMATIONTOKEN)FUNCSTABLE[ent] = NewZwSetInformationToken;
  }
#endif

#ifdef HOOK_ZWCREATEPROCESS
  ent = getFuncIndex(FindFunc(FindNT(),"ZwCreateProcess"), ref++);
  if (ent>0) {
    OldZwCreateProcess = (ZWCREATEPROCESS)FUNCSTABLE[ent];
    (ZWCREATEPROCESS)FUNCSTABLE[ent] = NewZwCreateProcess;
  }
#endif

#ifdef HOOK_ZWOPENPROCESS
  ent = getFuncIndex(FindFunc(FindNT(),"ZwOpenProcess"), ref++);
  if (ent>0) {
    OldZwOpenProcess = (ZWOPENPROCESS)FUNCSTABLE[ent];
    (ZWOPENPROCESS)FUNCSTABLE[ent] =  NewZwOpenProcess;
  }
#endif

  /* registry */
#ifdef HOOK_ZWOPENKEY
  ent = getFuncIndex(ZwOpenKey,ref++);
  if (ent>0) {
    OldZwOpenKey=(ZWOPENKEY)FUNCSTABLE[ent];
    (ZWOPENKEY)FUNCSTABLE[ent]=NewZwOpenKey;
  }
#endif

#ifdef HOOK_ZWCREATEKEY
  ent = getFuncIndex(ZwCreateKey,ref++);
  if (ent>0) {
    OldZwCreateKey=(ZWCREATEKEY)FUNCSTABLE[ent];
    (ZWCREATEKEY)FUNCSTABLE[ent]=NewZwCreateKey;
  }
#endif

#ifdef HOOK_ZWSETVALUEKEY
  ent = getFuncIndex(ZwSetValueKey,ref++);
  if (ent>0) {
    OldZwSetValueKey=(ZWSETVALUEKEY)FUNCSTABLE[ent];
    (ZWSETVALUEKEY)FUNCSTABLE[ent]=NewZwSetValueKey;
  }
#endif

  /* files */
#ifdef HOOK_ZWCREATEFILE
  ent = getFuncIndex(ZwCreateFile,ref++);
  if (ent>0) {
    OldZwCreateFile=(ZWCREATEFILE)FUNCSTABLE[ent];
    (ZWCREATEFILE)FUNCSTABLE[ent]=NewZwCreateFile;
   }
#endif

#ifdef HOOK_ZWOPENFILE
  ent = getFuncIndex(ZwOpenFile,ref++);
  if (ent>0) {
    OldZwOpenFile=(ZWOPENFILE)FUNCSTABLE[ent];
    (ZWOPENFILE)FUNCSTABLE[ent]=NewZwOpenFile;
  }
#endif

  /* sys */
#ifdef HOOK_ZWSETSYSTEMINFORMATION
  ent = getFuncIndex(FindFunc(FindNT(),"ZwSetSystemInformation"), ref++);
  if (ent>0) {
    OldZwSetSystemInformation = (ZWSETSYSTEMINFORMATION)FUNCSTABLE[ent];
    (ZWSETSYSTEMINFORMATION)FUNCSTABLE[ent] = NewZwSetSystemInformation;
  }
#endif

  _asm sti;
}



VOID unhookFuncs( void ) {
  ULONG ent;
  int ref=0;
  _asm cli;

  /* mem */
#ifdef HOOK_ZWOPENSECTION
  ent = getFuncIndex(ZwOpenSection,ref++);
  if (ent>0) (ZWOPENSECTION)FUNCSTABLE[ent]=OldZwOpenSection;
#endif

#ifdef HOOK_ZWMAPVIEWOFSECTION
  ent = getFuncIndex(ZwMapViewOfSection,ref++);
  if (ent>0) (ZWMAPVIEWOFSECTION)FUNCSTABLE[ent]=OldZwMapViewOfSection;
#endif

#ifdef HOOK_ZWCREATESYMBOLICLINKOBJECT
  ent = getFuncIndex(FindFunc(FindNT(),"ZwCreateSymbolicLinkObject"), ref++);
  if (ent>0) (ZWCREATESYMBOLICLINKOBJECT)FUNCSTABLE[ent] =
	       OldZwCreateSymbolicLinkObject;
#endif

  /* tok */
#ifdef HOOK_ZWADJUSTPRIVILEGESTOKEN
  ent = getFuncIndex(FindFunc(FindNT(),"NtAdjustPrivilegesToken"), ref++);
  if (ent>0) (ZWADJUSTPRIVILEGESTOKEN)FUNCSTABLE[ent] =
	       OldZwAdjustPrivilegesToken;
#endif

#ifdef HOOK_ZWSETINFORMATIONTOKEN
  ent = getFuncIndex(FindFunc(FindNT(),"ZwSetInformationToken"), ref++);
  if (ent>0) (ZWSETINFORMATIONTOKEN)FUNCSTABLE[ent] =
	       OldZwSetInformationToken;
#endif

#ifdef HOOK_ZWCREATEPROCESS
  ent = getFuncIndex(FindFunc(FindNT(),"ZwCreateProcess"), ref++);
  if (ent>0) (ZWCREATEPROCESS)FUNCSTABLE[ent]=OldZwCreateProcess;
#endif

#ifdef HOOK_ZWOPENPROCESS
  ent = getFuncIndex(FindFunc(FindNT(),"ZwOpenProcess"), ref++);
  if (ent>0) (ZWOPENPROCESS)FUNCSTABLE[ent] =  OldZwOpenProcess;
#endif

  /* registry */
#ifdef HOOK_ZWOPENKEY
  ent = getFuncIndex(ZwOpenKey,ref++);
  if (ent>0) (ZWOPENKEY)FUNCSTABLE[ent]=OldZwOpenKey;
#endif

#ifdef HOOK_ZWCREATEKEY
  ent = getFuncIndex(ZwCreateKey,ref++);
  if (ent>0) (ZWCREATEKEY)FUNCSTABLE[ent]=OldZwCreateKey;
#endif

#ifdef HOOK_ZWSETVALUEKEY
  ent = getFuncIndex(ZwSetValueKey,ref++);
  if (ent>0) (ZWSETVALUEKEY)FUNCSTABLE[ent]=OldZwSetValueKey;
#endif

  /* files */
#ifdef HOOK_ZWOPENFILE
  ent = getFuncIndex(ZwOpenFile,ref++);
  if (ent>0) (ZWOPENFILE)FUNCSTABLE[ent]=OldZwOpenFile;
#endif

#ifdef HOOK_ZWCREATEFILE
  ent = getFuncIndex(ZwCreateFile,ref++);
  if (ent>0) (ZWCREATEFILE)FUNCSTABLE[ent]=OldZwCreateFile;
#endif

#ifdef HOOK_ZWSETSYSTEMINFORMATION
  ent = getFuncIndex(FindFunc(FindNT(),"ZwSetSystemInformation"), ref++);
  if (ent>0) (ZWSETSYSTEMINFORMATION)FUNCSTABLE[ent]=
	       OldZwSetSystemInformation;
#endif
	
  _asm sti;
}

