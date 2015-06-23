/*
 * Copyright (C) 2000 by Pedestal Software, LLC
 * All Rights Reserved. Use at your own risk!
 * Email: support@pedestalsoftware.com
 *
 * This code is provided as Open Source with restrictions. See the readme
 * file that came with the distribution for details.
 *
 */

/* ntproto.h */

#ifndef NTPROTO_H
#define NTPROTO_H


NTSYSAPI
NTSTATUS
NTAPI ObQueryNameString( PVOID Object, PUNICODE_STRING Name,
			 ULONG MaximumLength, PULONG ActualLength );

#endif


