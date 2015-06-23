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

#ifndef __BASE_H__
#define __BASE_H__

#include <fltKernel.h>
#include <windef.h>

#include "..\inc\conditionals.h"

/* MACROS ########################################################################## */
// Compatability with win2k
#if WINVER == 0x0500

#define ExFreePoolWithTag( a, b ) ExFreePool( (a) )

#ifndef FlagOn
#define FlagOn(_F,_SF)  ((_F) & (_SF))
#endif

#endif

// Debug macro
#define DBG_PRINT( _DbgFlags, _DbgFlagsMask, _string ) (FlagOn( _DbgFlags,(_DbgFlagsMask) ) ? DbgPrint _string:((void)0))


/* DEFINES ######################################################################### */
#define PRINT_TAG             "[CFSD]: "
#define PRINT_TAG_ENTRY       "[CFSD-ENTRY]: "
#define PRINT_TAG_DIRECTORY   "[CFSD-DIRECTORY]: "
#define PRINT_TAG_CREATE      "[CFSD-CREATE]: "
#define PRINT_TAG_SETINFO     "[CFSD-SETINFO]: "
#define PRINT_TAG_ATTACH      "[CFSD-ATTACH]: "
#define PRINT_TAG_DETACH      "[CFSD-DETACH]: "
#define PRINT_TAG_UNLOAD      "[CFSD-UNLOAD]: "
#define PRINT_TAG_USERMODE    "[CFSD-USERMODE]: "
#define PRINT_TAG_CNAME       "[CFSD-CMPNAME]: "
#define PRINT_TAG_CTIME       "[CFSD-CMPTIME]: "
#define PRINT_TAG_CATTRIBUTES "[CFSD-CMPATTRIBUTE]: "

// Debug masks
#define DBG_IRP_MJ_DIRECTORY           0x00000001
#define DBG_IRP_MJ_CREATE              0x00000002
#define DBG_IRP_MJ_SET_INFORMATION     0x00000004

// cfsd.c
#define DBG_ENTRY                      0x00000008
#define DBG_ATTACH_INSTANCE            0x00000010
#define DBG_DETACH_INSTANCE            0x00000020
#define DBG_UNLOAD                     0x00000040
#define DBG_USERMODE                   0x00000080

// CompareFilters.c
#define DBG_COMPARE_NAME               0x00000100
#define DBG_COMPARE_TIME               0x00000200
#define DBG_COMPARE_ATTRIBUTES         0x00000300


// Device Types Masks
#define MASK_FILE_DEVICE_UNKNOWN             0x00000001
#define MASK_FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000002
#define MASK_FILE_DEVICE_DFS_FILE_SYSTEM     0x00000004
#define MASK_FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define MASK_FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000010
#define MASK_FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define MASK_FILE_DEVICE_NULL                0x00000040
#define MASK_FILE_DEVICE_VIRTUAL_DISK        0x00000080

// File System Masks
#define MASK_FSTYPE_UNKNOWN      0x00000001
#define MASK_FSTYPE_RAW          0x00000002
#define MASK_FSTYPE_NTFS         0x00000004
#define MASK_FSTYPE_FAT          0x00000008
#define MASK_FSTYPE_CDFS         0x00000010
#define MASK_FSTYPE_UDFS         0x00000020
#define MASK_FSTYPE_LANMAN       0x00000040
#define MASK_FSTYPE_WEBDAV       0x00000080
#define MASK_FSTYPE_RDPDR        0x00000100
#define MASK_FSTYPE_NFS          0x00000200
#define MASK_FSTYPE_MS_NETWARE   0x00000400
#define MASK_FSTYPE_NETWARE      0x00000800
#define MASK_FSTYPE_BSUDF        0x00001000
#define MASK_FSTYPE_MUP          0x00002000
#define MASK_FSTYPE_RSFX         0x00004000
#define MASK_FSTYPE_ROXIO_UDF1   0x00008000
#define MASK_FSTYPE_ROXIO_UDF2   0x00010000
#define MASK_FSTYPE_ROXIO_UDF3   0x00020000


/* DATA STRUCTURES ################################################################# */

#if FILTER_BY_NAME_INFORMATION
typedef struct _NAME_INFORMATION_DATA {

#if FILTER_BY_VOLUME
   UNICODE_STRING    VolumeName;
#endif

#if FILTER_BY_DIRECTORY
   UNICODE_STRING    DirectoryName;
#endif

#if FILTER_BY_SHARE
   UNICODE_STRING    ShareName;
#endif

#if FILTER_BY_NAME
   UNICODE_STRING    Name;
#endif

#if FILTER_BY_EXTENSION
   UNICODE_STRING    Extension;
#endif

#if FILTER_BY_STREAM
   UNICODE_STRING    StreamName;
#endif

} NAME_INFORMATION_DATA, *PNAME_INFORMATION_DATA;
#endif


// *************************************************************************************

#if FILTER_IRP_MJ_CREATE
typedef struct _PROTECTED_DATA {

   NAME_INFORMATION_DATA    NameInfo;


} PROTECTED_DATA, *PPROTECTED_DATA;
#endif

// *************************************************************************************

// List later will be a linked list of files that wish to hide - but for now in testing it is single
//#if FILTER_IRP_MJ_DIRECTORY_CONTROL
typedef struct _FILE_INFORMATION {

#if FILTER_BY_NAME_INFORMATION
   NAME_INFORMATION_DATA   NameInfo;
#endif 

#if FILTER_BY_TIME
/*
#define MASK_CREATION_TIME      0x01
#define MASK_LAST_ACCESS_TIME   0x02
#define MASK_LAST_WRITE_TIME    0x04
#define MASK_CHANGE_TIME        0x06
*/
   UCHAR             TimeMaskSet;
/*
#define COMPARE_TIME_LESS_THAN       1
#define COMPARE_TIME_EQUAL           2 
#define COMPARE_TIME_GREATER_THAN    3
*/
   UCHAR             TimeMaskType;

   LARGE_INTEGER     CreationTime;
   LARGE_INTEGER     LastAccessTime;
   LARGE_INTEGER     LastWriteTime;
   LARGE_INTEGER     ChangeTime;
#endif

#if FILTER_BY_ATTRIBUTES
/*
#define COMPARE_MATCH_PASSTHROUGH    0
#define COMPARE_MATCH_ANY            1
#define COMPARE_MATCH_ALL_EXACT      2
#define COMPARE_MATCH_ALL_PARTIAL    3
*/
   UCHAR             AttributesMaskType; 
/*
FILE_ATTRIBUTE_READONLY
FILE_ATTRIBUTE_HIDDEN
FILE_ATTRIBUTE_SYSTEM
FILE_ATTRIBUTE_ARCHIVE
FILE_ATTRIBUTE_NORMAL
FILE_ATTRIBUTE_TEMPORARY
FILE_ATTRIBUTE_SPARSE_FILE
FILE_ATTRIBUTE_REPARSE_POINT
FILE_ATTRIBUTE_COMPRESSED
FILE_ATTRIBUTE_OFFLINE
FILE_ATTRIBUTE_NOT_CONTEXT_INDEXED
FILE_ATTRIBUTE_ENCRYPTED
FILE_ATTRIBUTE_DIRECTORY
FILE_ATTRIBUTE_DEVICE
*/
   ULONG             FileAttributes;
#endif

} FILE_INFORMATION, *PFILE_INFORMATION;

//#endif // End #if FILTER_IRP_MJ_DIRECTORY_CONTROL

// *************************************************************************************

typedef struct _ATTACH_REQUIREMENTS {

/*
FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT  1
FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT     2
FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME  4
*/
  // What flags do we need to see in order consider this for mount
  FLT_INSTANCE_SETUP_FLAGS      InstanceFlags;

  // Array of device types that we will attach too
  DEVICE_TYPE                   InstanceVolumeDeviceTypes;

  // Array of filesystem types we will attach too
  DWORD                         InstancedFileSystemTypes;

} ATTACH_REQUIREMENTS, *PATTACH_REQUIREMENTS;


// *************************************************************************************

#if ENABLE_USER_INTERFACE
typedef struct _USER_MODE_CONNECTION {

  //  Listens for incoming connections
  PFLT_PORT        ServerPort;

  //  Client port for a connection to user-mode
  PFLT_PORT        ClientPort;

  //  User process that connected to the port
  PEPROCESS        UserProcess;


} USER_MODE_CONNECTION, *PUSER_MODE_CONNECTION;
#endif

/* GLOBAL VARIABLES ################################################################# */

PATTACH_REQUIREMENTS    gAttachRequirements;
PFILE_INFORMATION       gFileData;

#if FILTER_IRP_MJ_CREATE
PPROTECTED_DATA         gProtectedData;
#endif

#if DBG
DWORD      DbgOutput;
#endif

#if ENABLE_USER_INTERFACE
extern USER_MODE_CONNECTION   gUserModeConnection;
#endif

#endif