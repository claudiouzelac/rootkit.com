/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* h_reg.h */
#ifndef H_REG_H
#define H_REG_H

//#define HOOK_ZWOPENKEY
//#define HOOK_ZWCREATEKEY
#define HOOK_ZWSETVALUEKEY

void hookRegInit();
void hookRegClose();

typedef NTSTATUS (*ZWOPENKEY)( 	
			      IN PHANDLE, 
			      IN OUT ACCESS_MASK, 
			      IN POBJECT_ATTRIBUTES 
			      );
extern ZWOPENKEY OldZwOpenKey;


typedef NTSTATUS (*ZWCREATEKEY)(
				OUT PHANDLE, 
				IN ACCESS_MASK,
				IN POBJECT_ATTRIBUTES, 
				IN ULONG,
				IN PUNICODE_STRING, 
				IN ULONG, 
				OUT PULONG 
				);
extern ZWCREATEKEY OldZwCreateKey;


typedef NTSTATUS (*ZWSETVALUEKEY)(
				  IN HANDLE  KeyHandle,
				  IN PUNICODE_STRING  ValueName,
				  IN ULONG  TitleIndex  OPTIONAL,
				  IN ULONG  Type,
				  IN PVOID  Data,
				  IN ULONG  DataSize);
extern ZWSETVALUEKEY OldZwSetValueKey;
  


NTSTATUS NewZwOpenKey(
		      PHANDLE phKey,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributes
		      );


NTSTATUS NewZwCreateKey(
			PHANDLE phKey,
			ACCESS_MASK DesiredAccess,
			POBJECT_ATTRIBUTES ObjectAttributes,
			ULONG TitleIndex,
			PUNICODE_STRING Class,
			ULONG CreateOptions,
			PULONG pDisposition
			);

NTSTATUS NewZwSetValueKey(
			  HANDLE  KeyHandle,
			  PUNICODE_STRING  ValueName,
			  ULONG  TitleIndex,
			  ULONG  Type,
			  PVOID  Data,
			  ULONG  DataSize
			  );

#endif
