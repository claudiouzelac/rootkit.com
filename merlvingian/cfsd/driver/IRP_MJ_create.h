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
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#ifndef __IRP_MJ_CREATE_H__
#define __IRP_MJ_CREATE_H__

#include "base.h"

#if FILTER_IRP_MJ_CREATE

FLT_PREOP_CALLBACK_STATUS
PreCreate( IN OUT PFLT_CALLBACK_DATA Data,
           IN PCFLT_RELATED_OBJECTS FltObjects,
           OUT PVOID *CompletionContext );

FLT_POSTOP_CALLBACK_STATUS
PostCreate( IN OUT PFLT_CALLBACK_DATA Data,
            IN PCFLT_RELATED_OBJECTS FltObjects,
            IN PVOID CompletionContext,
            IN FLT_POST_OPERATION_FLAGS Flags );

#endif // End #if FILTER_IRP_MJ_CREATE

#endif // End #ifndef __IRP_MJ_CREATE_H__