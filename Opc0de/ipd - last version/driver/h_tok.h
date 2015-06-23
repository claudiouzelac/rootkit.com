/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* h_tok.h */

#ifndef H_TOK_H
#define H_TOK_H

void hookTokInit();
void hookTokClose();

#define HOOK_ZWADJUSTPRIVILEGESTOKEN
#define HOOK_ZWSETINFORMATIONTOKEN
#define HOOK_ZWCREATEPROCESS
#define HOOK_ZWOPENPROCESS

extern int EPROCESS_NAME_OFFSET;
extern ULONG processid_smss;
int isImageAllow(HANDLE procid);

typedef NTSTATUS(*ZWADJUSTPRIVILEGESTOKEN)
  (IN HANDLE hToken,
   IN BOOLEAN DisableAllPrivileges,
   IN PTOKEN_PRIVILEGES pNewPrivilegeSet,
   IN ULONG PreviousPrivilegeSetBufferLength OPTIONAL,
   PTOKEN_PRIVILEGES pPreviousPrivilegeSet OPTIONAL,
   PULONG PreviousPrivilegeSetReturnLength OPTIONAL
   );
extern ZWADJUSTPRIVILEGESTOKEN OldZwAdjustPrivilegesToken;

typedef NTSTATUS(*ZWSETINFORMATIONTOKEN)
  (IN HANDLE TokenHandle,
   IN TOKEN_INFORMATION_CLASS TokenInformationClass,
   IN PVOID TokenInformation,
   IN ULONG TokenInformationLength);
extern ZWSETINFORMATIONTOKEN OldZwSetInformationToken;

typedef NTSTATUS(*ZWCREATEPROCESS)
  (OUT PHANDLE ProcessHandle,
   IN ACCESS_MASK DesiredAccess,
   IN POBJECT_ATTRIBUTES ObjectAttributes,
   IN HANDLE InheritFromProcessHandle,
   IN BOOLEAN InheritHandles,
   IN HANDLE SectionHandle,
   IN HANDLE DebugPort,
   IN HANDLE ExceptionPort);
extern ZWCREATEPROCESS OldZwCreateProcess;

typedef NTSTATUS(*ZWOPENPROCESS)
  (OUT PHANDLE ProcessHandle,
   IN ACCESS_MASK DesiredAccess,
   IN POBJECT_ATTRIBUTES ObjectAttributes,
   IN PCLIENT_ID ClientId);
extern ZWOPENPROCESS OldZwOpenProcess;

					    
NTSTATUS
NewZwAdjustPrivilegesToken(
			   HANDLE hToken,
			   BOOLEAN DisableAllPrivileges,
			   PTOKEN_PRIVILEGES pNewPrivilegeSet,
			   ULONG PreviousPrivilegeSetBufferLength,
			   PTOKEN_PRIVILEGES pPreviousPrivilegeSet,
			   PULONG PreviousPrivilegeSetReturnLength
			   );

NTSTATUS
NewZwSetInformationToken(
			 IN HANDLE TokenHandle,
			 IN TOKEN_INFORMATION_CLASS TokenInformationClass,
			 IN PVOID TokenInformation,
			 IN ULONG TokenInformationLength);

NTSTATUS
NewZwCreateProcess(OUT PHANDLE ProcessHandle,
		   IN ACCESS_MASK DesiredAccess,
		   IN POBJECT_ATTRIBUTES ObjectAttributes,
		   IN HANDLE InheritFromProcessHandle,
		   IN BOOLEAN InheritHandles,
		   IN HANDLE SectionHandle OPTIONAL,
		   IN HANDLE DebugPort OPTIONAL,
		   IN HANDLE ExceptionPort OPTIONAL
		   );

NTSTATUS
NewZwOpenProcess(OUT PHANDLE ProcessHandle,
		 IN ACCESS_MASK DesiredAccess,
		 IN POBJECT_ATTRIBUTES ObjectAttributes,
		 IN PCLIENT_ID ClientId);
#endif
