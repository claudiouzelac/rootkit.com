/*
 * Copyright (C) 2000 by Pedestal Software, LLC
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* h_file.h */
#ifndef H_FILE_H
#define H_FILE_H

void hookFileInit();
void hookFileClose();

typedef NTSTATUS (*ZWCREATEFILE)(
				 PHANDLE FileHandle,
				 ACCESS_MASK DesiredAccess,
				 POBJECT_ATTRIBUTES ObjectAttributes,
				 PIO_STATUS_BLOCK IoStatusBlock,
				 PLARGE_INTEGER AllocationSize OPTIONAL,
				 ULONG FileAttributes,
				 ULONG ShareAccess,
				 ULONG CreateDisposition,
				 ULONG CreateOptions,
				 PVOID EaBuffer OPTIONAL,
				 ULONG EaLength
				 );
extern ZWCREATEFILE OldZwCreateFile;

typedef NTSTATUS (*ZWOPENFILE)(
			       PHANDLE phFile,
			       ACCESS_MASK DesiredAccess,
			       POBJECT_ATTRIBUTES ObjectAttributes,
			       PIO_STATUS_BLOCK pIoStatusBlock,
			       ULONG ShareMode,
			       ULONG OpenMode
			       );
extern ZWOPENFILE OldZwOpenFile;



NTSTATUS NewZwCreateFile(
			 PHANDLE FileHandle,
			 ACCESS_MASK DesiredAccess,
			 POBJECT_ATTRIBUTES ObjectAttributes,
			 PIO_STATUS_BLOCK IoStatusBlock,
			 PLARGE_INTEGER AllocationSize OPTIONAL,
			 ULONG FileAttributes,
			 ULONG ShareAccess,
			 ULONG CreateDisposition,
			 ULONG CreateOptions,
			 PVOID EaBuffer OPTIONAL,
			 ULONG EaLength
			 );

NTSTATUS NewZwOpenFile(
		       PHANDLE phFile,
		       ACCESS_MASK DesiredAccess,
		       POBJECT_ATTRIBUTES ObjectAttributes,
		       PIO_STATUS_BLOCK pIoStatusBlock,
		       ULONG ShareMode,
		       ULONG OpenMode
		       );

#endif
