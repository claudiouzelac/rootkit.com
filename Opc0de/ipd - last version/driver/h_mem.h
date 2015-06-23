/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* h_mem.h */

#ifndef H_MEM_H
#define H_MEM_H

void hookMemInit();
void hookMemClose();

#define HOOK_ZWOPENSECTION
//#define HOOK_ZWMAPVIEWOFSECTION
#define HOOK_ZWCREATESYMBOLICLINKOBJECT


typedef NTSTATUS (*ZWOPENSECTION)( 	
				  OUT PHANDLE, 
				  IN ACCESS_MASK, 
				  IN POBJECT_ATTRIBUTES 
				  );
extern ZWOPENSECTION OldZwOpenSection;

typedef NTSTATUS
(*ZWMAPVIEWOFSECTION)(
		      IN HANDLE  SectionHandle,
		      IN HANDLE  ProcessHandle,
		      IN OUT PVOID  *BaseAddress,
		      IN ULONG  ZeroBits,
		      IN ULONG  CommitSize,
		      IN OUT PLARGE_INTEGER  SectionOffset  OPTIONAL,
		      IN OUT PSIZE_T  ViewSize,
		      IN SECTION_INHERIT  InheritDisposition,
		      IN ULONG  AllocationType,
		      IN ULONG  Protect
		      );
extern ZWMAPVIEWOFSECTION OldZwMapViewOfSection;


typedef NTSTATUS (*ZWCREATESYMBOLICLINKOBJECT)(
					       OUT PHANDLE SymLinkHandle,
					       IN ACCESS_MASK DesiredAccess,
					       IN POBJECT_ATTRIBUTES ObjectAttributes,
					       IN PUNICODE_STRING ObjectName
					       );
extern ZWCREATESYMBOLICLINKOBJECT OldZwCreateSymbolicLinkObject;



NTSTATUS NewZwOpenSection(
			  PHANDLE pHandle,
			  ACCESS_MASK DesiredAccess,
			  POBJECT_ATTRIBUTES ObjectAttributes
			  );

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
			       );

NTSTATUS NewZwCreateSymbolicLinkObject(PHANDLE SymLinkHandle,
				       ACCESS_MASK DesiredAccess,
				       POBJECT_ATTRIBUTES ObjectAttributes,
				       PUNICODE_STRING ObjectName
				       );

#endif
