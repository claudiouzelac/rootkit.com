/*
 * Copyright (C) 2000-2003 by Pedestal Software, Inc.
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* h_sys.h */

#ifndef H_SYS_H
#define H_SYS_H

void hookSysInit();
void hookSysClose();

#define HOOK_ZWSETSYSTEMINFORMATION


#define SystemLoadAndCallImage 38
#define SystemLoadImage 26

typedef NTSTATUS (*ZWSETSYSTEMINFORMATION)( 	
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength);
extern ZWSETSYSTEMINFORMATION OldZwSetSystemInformation;



NTSTATUS NewZwSetSystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength);

#endif
