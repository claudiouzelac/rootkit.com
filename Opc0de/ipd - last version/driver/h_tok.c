/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* h_tok.c */

#include "driver.h"
#include "util.h"
#include "dutil.h"
#include "h_tok.h"

ZWADJUSTPRIVILEGESTOKEN OldZwAdjustPrivilegesToken;
ZWSETINFORMATIONTOKEN OldZwSetInformationToken;
ZWCREATEPROCESS OldZwCreateProcess;
ZWOPENPROCESS OldZwOpenProcess;


static LUID LUID_RESTRICT[] = {
  { SE_DEBUG_PRIVILEGE, 0 },
  { SE_TCB_PRIVILEGE, 0 },
  { SE_CREATE_TOKEN_PRIVILEGE, 0 },
  { SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, 0 },
  { 0, 0 }
  // We don't necessarily want to disallow SE_LOAD_DRIVER_PRIVILEGE, but
  // do want stop new code from executing. h_reg+h_file should provide this.
};

static WCHAR *IMAGEALLOW[] = {
  L"lsass.exe",
  L"winlogon.exe",
  L"smss.exe",
  L"rpcss.exe",
  L"svchost.exe",
  L"system",
  L"services.exe",
  L"termsrv.exe",
  L"dfssvc.exe",
  L"tnslsnr80.exe", // required to run Oracle
  NULL
};


int EPROCESS_NAME_OFFSET=508; /* W2K no SP */
ULONG *processid, processid_smss=0;
ULONG numid=0;
 
 

void hookTokInit() {
  WCHAR buf[200];
  ULONG n=200;
  PSYSTEM_PROCESSES sp, p;
  NTSTATUS status;
  numid = 0;
  sp = ExAllocatePoolWithTag ( PagedPool,
			       sizeof(SYSTEM_PROCESSES)*n, CRASH_TAG);
  if (!sp) {
    debugOutput(L"Allocate paged pool failed, hookTokInit\n");
    return;
  }


  while (ZwQuerySystemInformation(SystemProcessesAndThreadsInformation,
				  sp,
				  n* sizeof(*sp),
				  0)
	 == STATUS_INFO_LENGTH_MISMATCH) {
    ExFreePool(sp);
    n*=2;
    sp = ExAllocatePoolWithTag ( PagedPool,
				 sizeof(SYSTEM_PROCESSES)*n, CRASH_TAG);
    if (!sp) {
      debugOutput(L"Allocate paged pool failed, hookTokInit\n");
      return;
    }
  }
  
  p = sp;
  /*
   * count ImageAllowed matches
   */
  while (p->NextEntryDelta != 0) {
    if (p->ProcessName.Length >0) {
      WCHAR **q = IMAGEALLOW;
      while (*q != NULL) {
	if (_wcsicmp(p->ProcessName.Buffer, *q)==0) ++numid;
	if (_wcsicmp(p->ProcessName.Buffer, L"smss.exe")==0)
	  processid_smss = p->ProcessId;
	++q;
      }
    }
    p = (PSYSTEM_PROCESSES)(((unsigned char *)p)+p->NextEntryDelta);
  }

  if (numid>0) {
    processid = (ULONG *)ExAllocatePoolWithTag ( PagedPool,
						 sizeof(ULONG)*numid,
						 CRASH_TAG);
    if (!processid) {
      debugOutput(L"Allocate paged pool failed, hookTokInit\n");
      numid=0;
      return;
    }

    p = sp;
    n=0;
    /*
     * save ImageAllowed process ids
     */
    while (p->NextEntryDelta != 0) {
      if (p->ProcessName.Length >0) {
	WCHAR **q = IMAGEALLOW;
	while (*q != NULL) {
	  if (_wcsicmp(p->ProcessName.Buffer, *q)==0)
	    processid[n++] = p->ProcessId;
	  ++q;
	}
      }
      p = (PSYSTEM_PROCESSES)(((unsigned char *)p)+p->NextEntryDelta);
    }    
  }    

  ExFreePool(sp);

  {
    PEPROCESS proc = PsGetCurrentProcess();
    if (proc) {
      int i;
      //dumpbuf(proc, 600);
      for (i=0;i<1024; i++) {
	if (_strnicmp(((char *)proc)+i, "system", 6)==0) {
	  EPROCESS_NAME_OFFSET = i;
	  break;
	}
      }
    }
  }
}

void hookTokClose() {
  if (numid>0 && processid) ExFreePool(processid);
}


int isImageAllow(HANDLE procid) {
  int i;
  if (numid==0) return TRUE;  
  for (i=0; i<numid; i++) {
    if ((ULONG)procid == processid[i]) return TRUE;
  } 
  return FALSE;
}  


static NTSTATUS checkTokRestrictions(PTOKEN_PRIVILEGES pNewPrivilegeSet) {
  if (restrictEnabled() && pNewPrivilegeSet) {
    ULONG i;
    LUID *p;
    HANDLE procid;
    PEPROCESS proc;
    procid = PsGetCurrentProcessId();
    proc = PsGetCurrentProcess();

    for (i=0; i<numid; i++) {
      if ((ULONG)procid == processid[i]) break;
    }
    
    if (i>=numid &&
	_strnicmp(((char *)proc)+EPROCESS_NAME_OFFSET,
		  "winlogon.exe", 12)!=0) {

      for (i=0; i<pNewPrivilegeSet->PrivilegeCount; i++) {
	p=LUID_RESTRICT;
	while(p->LowPart != 0 || p->HighPart != 0) {
	  if (p->LowPart == pNewPrivilegeSet->Privileges[i].Luid.LowPart &&
	      p->HighPart == pNewPrivilegeSet->Privileges[i].Luid.HighPart) {
	    WCHAR buf[1024];
	    swprintf(buf,L"AdjustPrivileges: i=%d, Low=%d procid=0x%x restrict\n",
		     i,
		     pNewPrivilegeSet->Privileges[i].Luid.LowPart,
		     procid);
	    debugOutput(buf);
	    return STATUS_ACCESS_DENIED;
	  }
	  ++p;
	}
      }
    }
  }

#ifdef DEBUG
  {
    WCHAR buf[1024];
    ULONG i;
    HANDLE procid;
    procid = PsGetCurrentProcessId();
    for (i=0; i<pNewPrivilegeSet->PrivilegeCount; i++) {
      swprintf(buf, L"AdjustPrivileges: permit Low=%d procid=0x%x\n",
	       pNewPrivilegeSet->Privileges[i].Luid.LowPart,
	       procid);
      debugOutput(buf);
    }
  }
#endif

  return STATUS_SUCCESS;
}


NTSTATUS
NewZwAdjustPrivilegesToken(
			   HANDLE hToken,
			   BOOLEAN DisableAllPrivileges,
			   PTOKEN_PRIVILEGES pNewPrivilegeSet,
			   ULONG PreviousPrivilegeSetBufferLength,
			   PTOKEN_PRIVILEGES pPreviousPrivilegeSet,
			   PULONG PreviousPrivilegeSetReturnLength
			   ) {
  NTSTATUS status;

  status = checkTokRestrictions(pNewPrivilegeSet);
  if (status != STATUS_SUCCESS) return status;

  status =
    (OldZwAdjustPrivilegesToken)(hToken,
				 DisableAllPrivileges,
				 pNewPrivilegeSet,
				 PreviousPrivilegeSetBufferLength,
				 pPreviousPrivilegeSet,
				 PreviousPrivilegeSetReturnLength
				 );
  return status;					
}




NTSTATUS
NewZwSetInformationToken(
			 IN HANDLE TokenHandle,
			 IN TOKEN_INFORMATION_CLASS TokenInformationClass,
			 IN PVOID TokenInformation,
			 IN ULONG TokenInformationLength) {
  NTSTATUS status;

  if (TokenInformationClass == TokenPrivileges) {
    status = checkTokRestrictions((PTOKEN_PRIVILEGES) TokenInformation);
    if (status != STATUS_SUCCESS) return status;
  }

  status = (OldZwSetInformationToken)(TokenHandle,
				      TokenInformationClass,
				      TokenInformation,
				      TokenInformationLength);
  return status;
}





NTSTATUS
NewZwCreateProcess(OUT PHANDLE ProcessHandle,
		   IN ACCESS_MASK DesiredAccess,
		   IN POBJECT_ATTRIBUTES ObjectAttributes,
		   IN HANDLE InheritFromProcessHandle,
		   IN BOOLEAN InheritHandles,
		   IN HANDLE SectionHandle OPTIONAL,
		   IN HANDLE DebugPort OPTIONAL,
		   IN HANDLE ExceptionPort OPTIONAL
		   ) {
  NTSTATUS status;

  status = (OldZwCreateProcess)(ProcessHandle,
				DesiredAccess,
				ObjectAttributes,
				InheritFromProcessHandle,
				InheritHandles,
				SectionHandle,
				DebugPort,
				ExceptionPort);

  if (restrictEnabled() && status == STATUS_SUCCESS) {
    NTSTATUS rtn;
    PEPROCESS proc;

    rtn=
      ObReferenceObjectByHandle(*ProcessHandle,
				0,
				0,
				KernelMode,
				&proc,
				NULL);
    
    if (rtn==STATUS_SUCCESS) {
      HANDLE procid = PsGetCurrentProcessId();

      if (_strnicmp(((char *)proc)+EPROCESS_NAME_OFFSET,
		    "winlogon.exe", 12)==0 &&
	  (ULONG)procid != processid_smss) {
	WCHAR buf[500];
	swprintf(buf,L"CreateProcess: blocking winlogon parent=0x%x\n",procid);
	debugOutput(buf);
	ObDereferenceObject(proc);
	ZwClose(*ProcessHandle);
	return STATUS_ACCESS_DENIED;
      }
      if (_strnicmp(((char *)proc)+EPROCESS_NAME_OFFSET,
		    "csrss.exe", 9)==0 &&
	  (ULONG)procid != processid_smss) {
	WCHAR buf[500];
	swprintf(buf,L"CreateProcess: blocking csrss parent=0x%x\n",procid);
	debugOutput(buf);
	ObDereferenceObject(proc);
	ZwClose(*ProcessHandle);
	return STATUS_ACCESS_DENIED;
      }
      ObDereferenceObject(proc);
    }
  }

  return status;
}


NTSTATUS
NewZwOpenProcess(OUT PHANDLE ProcessHandle,
		 IN ACCESS_MASK DesiredAccess,
		 IN POBJECT_ATTRIBUTES ObjectAttributes,
		 IN PCLIENT_ID ClientId) {
  NTSTATUS status;
  HANDLE procid;
  ULONG i;
  ACCESS_MASK restrict = PROCESS_CREATE_THREAD|PROCESS_VM_WRITE;
  WCHAR buf[1024];

  if (restrictEnabled() && (DesiredAccess & restrict)) {
    procid = PsGetCurrentProcessId();

    for (i=0; i<numid; i++) {
      if ((ULONG)procid == processid[i]) break;
    }
    if (i>=numid) {
      swprintf(buf,L"OpenProcess restriction mask=0x%x procid=0x%x\n",
	       DesiredAccess, procid);
      debugOutput(buf);
#ifdef DEBUG
      say_process_mask(DesiredAccess);
#endif
      DesiredAccess &= ~restrict;
    }
  }

  status = (OldZwOpenProcess)(ProcessHandle,
			      DesiredAccess,
			      ObjectAttributes,
			      ClientId);
  return status;
}
