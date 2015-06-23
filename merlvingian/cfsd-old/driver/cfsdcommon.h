/*
 *   Clandestine File System Driver
 *   Copyright (C) 2005 Jason Todd
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
 *
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#ifndef __CFSDCOMMON_H__
#define __CFSDCOMMON_H__

#include <fltKernel.h>
#include <windef.h>

/* MACROS ########################################################################## */
// Compatability with win2k
#if WINVER == 0x0500

#define ExFreePoolWithTag( a, b ) ExFreePool( (a) )

#ifndef FlagOn
#define FlagOn(_F,_SF)  ((_F) & (_SF))
#endif

#endif

/* CONDITIONAL COMPILE ############################################################# */
/* 
 defines cut down on checks/code space/memory usage - the memory usage can add up quick for large amounts
 of items that are to be filtered creating many instances of the _HIDDEN_DATA structure.
 Also cuts down on the checks if they are not needed inside PostDirectoryControl() which is a heavily
 called function.
 This allows versions to be compile that only filter on cretain class types, obsessive way to save memory.
 **** * ATLEAST ONE * **** of these ** MUST ** be defined 1, if not then why are we even bothering
*/
#define FILTER_BY_NAME         1
#define FILTER_BY_TIME         0
#define FILTER_BY_ATTRIBUTES   0


// Are we going to be compile for use with a user interface
#define ENABLE_USER_INTERFACE  1

/* DEFINES ######################################################################### */
#define PRINT_TAG "[CFSD]: "

#define MAX_DEVICE_TYPE      12


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


// Device Types Masks
#define MASK_FILE_DEVICE_UNKNOWN             0x00000001
#define MASK_FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000002
#define MASK_FILE_DEVICE_DFS_FILE_SYSTEM     0x00000004
#define MASK_FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define MASK_FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000010
#define MASK_FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define MASK_FILE_DEVICE_NULL                0x00000040
#define MASK_FILE_DEVICE_VIRTUAL_DISK        0x00000080



/* DATA STRUCTURES ################################################################# */
// List later will be a linked list of files that wish to hide - but for now in testing it is single
typedef struct _HIDDEN_DATA {

#if FILTER_BY_NAME
   UNICODE_STRING    HFile;
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

   LARGE_INTEGER     HCreationTime;
   LARGE_INTEGER     HLastAccessTime;
   LARGE_INTEGER     HLastWriteTime;
   LARGE_INTEGER     HChangeTime;
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
   ULONG             HFileAttributes;
#endif

} HIDDEN_DATA, *PHIDDEN_DATA;


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




typedef struct _USER_MODE_CONNECTION {

  //  Listens for incoming connections
  PFLT_PORT        ServerPort;

  //  Client port for a connection to user-mode
  PFLT_PORT        ClientPort;

  //  User process that connected to the port
  PEPROCESS        UserProcess;


} USER_MODE_CONNECTION, *PUSER_MODE_CONNECTION;

/* GLOBAL VARIABLES ################################################################# */

PATTACH_REQUIREMENTS    gAttachRequirements;
PHIDDEN_DATA            gHiddenData;

extern USER_MODE_CONNECTION   gUserModeConnection;


#endif