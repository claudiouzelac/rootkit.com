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
 *   General header file 
 *
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#ifndef __IRP_MJ_DIRECTORY_H__
#define __IRP_MJ_DIRECTORY_H__

#include "base.h"

#if FILTER_IRP_MJ_DIRECTORY_CONTROL

/* DEFINES ######################################################################### */
#define POOL_TAG_TEMPORARY_BUFFER   'BpmT'


/* PROTOTYPES ###################################################################### */
FLT_PREOP_CALLBACK_STATUS
PreDirectoryControl( IN OUT PFLT_CALLBACK_DATA Data,
                     IN PCFLT_RELATED_OBJECTS FltObjects,
                     OUT PVOID *CompletionContext );

FLT_POSTOP_CALLBACK_STATUS 
PostDirectoryControl( IN OUT PFLT_CALLBACK_DATA Data,
                      IN PCFLT_RELATED_OBJECTS FltObjects,
                      IN PVOID CompletionContext,
                      IN FLT_POST_OPERATION_FLAGS Flags );


FLT_POSTOP_CALLBACK_STATUS
PostDirectoryControlSafe ( IN OUT PFLT_CALLBACK_DATA Data,
                           IN PCFLT_RELATED_OBJECTS FltObjects,
                           IN PVOID CompletionContext,
                           IN FLT_POST_OPERATION_FLAGS Flags );



FLT_POSTOP_CALLBACK_STATUS
ProcessMNNotifyChangeDirectory( IN OUT PFLT_CALLBACK_DATA Data );




FLT_POSTOP_CALLBACK_STATUS
ProcessFileBothDirectoryInformation( IN OUT PFLT_CALLBACK_DATA Data,
                                     IN OUT PVOID SafeBuffer );

FLT_POSTOP_CALLBACK_STATUS
ProcessFileNamesInformation( IN PCFLT_RELATED_OBJECTS FltObjects,
							 IN OUT PFLT_CALLBACK_DATA Data,
                             IN OUT PVOID SafeBuffer );

FLT_POSTOP_CALLBACK_STATUS
ProcessFileDirectoryInformation( IN OUT PFLT_CALLBACK_DATA Data );

FLT_POSTOP_CALLBACK_STATUS
ProcessFileFullDirectoryInformation( IN OUT PFLT_CALLBACK_DATA Data );

FLT_POSTOP_CALLBACK_STATUS
ProcessFileIdBothDirectoryInformation( IN OUT PFLT_CALLBACK_DATA Data );

FLT_POSTOP_CALLBACK_STATUS
ProcessFileIdFullDirectoryInformation( IN OUT PFLT_CALLBACK_DATA Data );

FLT_POSTOP_CALLBACK_STATUS
ProcessFileObjectIdInformation( IN OUT PFLT_CALLBACK_DATA Data );

FLT_POSTOP_CALLBACK_STATUS
ProcessFileReparsePointInformation( IN OUT PFLT_CALLBACK_DATA Data );

#endif // End #if FILTER_IRP_MJ_DIRECTORY_CONTROL

#endif // End #ifndef __IRP_MJ_DIRECTORY_H__