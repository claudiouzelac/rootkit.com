/*
 *   Clandestine File System Driver
 *   Copyright (C) 2005 Jason Todd
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *   General header information  
 *
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#ifndef __CFSD_H__
#define __CFSD_H__

#include "base.h"


/* PROTOTYPES ###################################################################### */
NTSTATUS
DriverEntry( IN PDRIVER_OBJECT theDriverObject,
             IN PUNICODE_STRING theRegistryPath );

NTSTATUS 
cfsd_Unload( FLT_FILTER_UNLOAD_FLAGS theFlags );


NTSTATUS 
cfsd_InstanceSetup( IN PCFLT_RELATED_OBJECTS FltObjects,
                    IN FLT_INSTANCE_SETUP_FLAGS Flags,
                    IN DEVICE_TYPE VolumeDeviceType,
                    IN FLT_FILESYSTEM_TYPE VolumeFilesystemType );

VOID
cfsd_InstanceTeardownComplete( IN PCFLT_RELATED_OBJECTS  FltObjects,
                               IN FLT_INSTANCE_TEARDOWN_FLAGS  Reason );

#if ENABLE_USER_INTERFACE
NTSTATUS
cfsd_UserModeConnect( IN PFLT_PORT ClientPort,
                      IN PVOID ServerPortCookie,
                      IN PVOID ConnectionContext,
                      IN ULONG SizeOfContext,
                     OUT PVOID *ConnectionCookie );

VOID
cfsd_UserModeDisconnect( IN PVOID ConnectionCookie );

NTSTATUS
cfsd_UserModeCommunication( IN PVOID ConnectionCookie,
                            IN PVOID InputBuffer OPTIONAL,
                            IN ULONG InputBufferSize,
                            OUT PVOID OutputBuffer OPTIONAL,
                            IN ULONG OutputBufferSize,
                            OUT PULONG ReturnOutputBufferLength );
#endif // End #if ENABLE_USER_INTERFACE

#endif // End #ifndef __CFSD_H__