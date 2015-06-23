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
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#include "registry.h"
#include "base.h"

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
BOOLEAN
QueryRegistryDWORDKey( PUNICODE_STRING RegistryPath,
                       PWSTR KeyName,
                       REG_KEY_TYPES RegAttachType )

{
HANDLE                        hkey;
NTSTATUS                      Status;
OBJECT_ATTRIBUTES             ObjectAttributes;
UNICODE_STRING                AttachKey;
PKEY_VALUE_FULL_INFORMATION   keyValue;
ULONG                         Length;
ULONG                         ResultLength;


    // DDK : "Macro initializes the opaque OBJECT_ATTRIBUTES structure, which specifies 
    //        the properties of an object handle to routines that open handles."
    InitializeObjectAttributes ( &ObjectAttributes,
                                 RegistryPath,
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
 KdPrint( (PRINT_TAG "ZwOpenKey() Failure in QueryRegistryDWORDKey()\n") );
	 return FALSE;
	}

    //
    RtlInitUnicodeString( &AttachKey, KeyName );


    // Amount of memory we will need to allocate
    Length = sizeof( KEY_VALUE_FULL_INFORMATION ) + AttachKey.Length * sizeof( WCHAR ) + sizeof( ULONG );
    // Allocate temporary memory for our key
    keyValue = ExAllocatePoolWithTag( NonPagedPool,
                                      Length,
                                      POOL_TAG_REGISTRY_ATTACHMETHOD );
    // Query our key for our value
    Status = ZwQueryValueKey( hkey,                   // KeyHandle 
                              &AttachKey,             // ValueName
                              KeyValueFullInformation,// KeyValueInformationClass
                              keyValue,               // KeyValueInformation
                              Length,                 // Length
                              &ResultLength );        // ResultLength
    // If we have a failure use the default value
	if ( !NT_SUCCESS( Status ) )
	{
 KdPrint( (PRINT_TAG "ZwQueryValueKey() EnumerateRegistryAttachMethod()\n") );
	 return FALSE;
	}


    switch ( RegAttachType )
    {
     case AttachMethods:
         {
           gAttachRequirements->InstanceFlags = *( (PULONG)(((PCHAR)keyValue) + keyValue->DataOffset ));

 KdPrint( (PRINT_TAG "Processing %wZ\\%ws 0x%x\n",RegistryPath, KeyName ,gAttachRequirements->InstanceFlags ) );
           break;
         }
     case VolumeDeviceTypes:
         {
           gAttachRequirements->InstanceVolumeDeviceTypes = *( (PULONG)(((PCHAR)keyValue) + keyValue->DataOffset ));

 KdPrint( (PRINT_TAG "Processing %wZ\\%ws 0x%x\n",RegistryPath, KeyName ,gAttachRequirements->InstanceVolumeDeviceTypes ) );
           break;
         }
     case FileSystems:
         {
           gAttachRequirements->InstancedFileSystemTypes = *( (PULONG)(((PCHAR)keyValue) + keyValue->DataOffset ));

 KdPrint( (PRINT_TAG "Processing %wZ\\%ws 0x%x\n",RegistryPath, KeyName ,gAttachRequirements->InstancedFileSystemTypes ) );
           break;
         }
     case DebugMask:
         {
           DbgOutput = *( (PULONG)(((PCHAR)keyValue) + keyValue->DataOffset ));

 KdPrint( (PRINT_TAG "Processing %wZ\\%ws 0x%x\n",RegistryPath, KeyName ,DbgOutput ) );
           break;
         }
     default :
            {
 KdPrint( ("BAD MOJO\n") );
             break;
            }
    }

   // Free key from memory
   ExFreePoolWithTag( keyValue, POOL_TAG_REGISTRY_ATTACHMETHOD );
   // Close the regkey
   ZwClose( hkey );
 
 return TRUE;
}
/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
BOOLEAN
EnumerateRegistryValues( PUNICODE_STRING RegistryPath )
{

#if FILTER_IRP_MJ_CREATE
gProtectedData = ExAllocatePoolWithTag( NonPagedPool, 
                                        sizeof( PROTECTED_DATA ),
                                       'parC' ); 


RtlZeroMemory( &gProtectedData->NameInfo, sizeof( NAME_INFORMATION_DATA ) );

#if FILTER_BY_NAME
RtlInitUnicodeString( &gProtectedData->NameInfo.Name, L"TESTME.TXT" );
#endif

#if FILTER_BY_DIRECTORY
//RtlInitUnicodeString( &gProtectedData->NameInfo.DirectoryName, L"*CFSD*" );
#endif

#if FILTER_BY_VOLUME
//RtlInitUnicodeString( &gProtectedData->NameInfo.VolumeName, L"HARDDISK" );
#endif


#if FILTER_BY_SHARE

#endif


// This is a dangerous way to filter
#if FILTER_BY_EXTENSION

#endif

#if FILTER_BY_STREAM

#endif



#endif

gFileData = ExAllocatePoolWithTag( NonPagedPool, 
                                     sizeof( FILE_INFORMATION ),
                                     'parC' ); 

#if FILTER_BY_NAME        
RtlInitUnicodeString( &gFileData->NameInfo.Name, L"TESTME.TXT" );

// Convert the registry entry here UCASE for later comparisons
//RtlUpcaseUnicodeString();

#endif

#if FILTER_BY_ATTRIBUTES     
gFileData->AttributesMaskType = COMPARE_MATCH_ALL_PARTIAL;
gFileData->FileAttributes    = /*FILE_ATTRIBUTE_COMPRESSED |*/ FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE;

#endif

#if FILTER_BY_TIME
gFileData->TimeMaskSet              = MASK_CREATION_TIME;
gFileData->TimeMaskType             = COMPARE_TIME_EQUAL;

gFileData->CreationTime.LowPart    = 399602980;
gFileData->CreationTime.HighPart   = 29745695;
gFileData->LastAccessTime.LowPart  = 0;
gFileData->LastAccessTime.HighPart = 0;
gFileData->LastWriteTime.LowPart   = 0;
gFileData->LastWriteTime.HighPart  = 0;
gFileData->ChangeTime.LowPart      = 0;
gFileData->ChangeTime.HighPart     = 0;
#endif


    gAttachRequirements = ExAllocatePoolWithTag( NonPagedPool, 
                                                 sizeof( ATTACH_REQUIREMENTS ),
                                                 'parC' ); 

    // Read the debug reg key first so we can start
    QueryRegistryDWORDKey( RegistryPath , REGKEY_DEBUG, DebugMask );

    QueryRegistryDWORDKey( RegistryPath , REGKEY_ATTACHMETHODS, AttachMethods );

    QueryRegistryDWORDKey( RegistryPath , REGKEY_FILESYSTEMS, FileSystems );

    QueryRegistryDWORDKey( RegistryPath , REGKEY_VOLUMEDEVICETYPES, VolumeDeviceTypes );

 return TRUE;
}
