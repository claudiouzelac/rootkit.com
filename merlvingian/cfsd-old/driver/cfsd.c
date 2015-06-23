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
 *   Entry point for the driver, minifilter configuration, instance attachment validation
 *
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#include "cfsd.h"

#include "cfsdcommon.h"
#include "cfsddirectory.h"
#include "cfsdregistry.h"

#include "..\inc\crossover.h"


/* #################################################################################

  DDK : "...Structure is used to register operation callback routines"

*/
CONST FLT_OPERATION_REGISTRATION cfsd_Callbacks[] = {

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      NULL,//PreDirectoryControl
      PostDirectoryControl },

    { IRP_MJ_OPERATION_END }
};

/* #################################################################################

   DDK : "...Structure is passed as a parameter to FltRegisterFilter()."

*/
CONST FLT_REGISTRATION cfsd_FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,//FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP //  Flags

    NULL,                               //  ContextRegistration

    cfsd_Callbacks,                     //  OperationRegistration

    cfsd_Unload,                        //  FilterUnloadCallback

    cfsd_InstanceSetup,                 //  InstanceSetupCallback
    NULL,                               //  InstanceQueryTeardownCallback
    NULL,                               //  InstanceTeardownStartCallback
    NULL,                               //  InstanceTeardownCompleteCallback

    NULL,                               //  GenerateFileNameCallback
    NULL,                               //  NormalizeNameComponentCallback
    NULL                                //  NormalizeContextCleanupCallback

};
/* ################################################################################# */


PFLT_FILTER            gFilterPointer;
USER_MODE_CONNECTION   gUserModeConnection;

UNICODE_STRING uFName;


/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS
DriverEntry( IN PDRIVER_OBJECT theDriverObject,
             IN PUNICODE_STRING theRegistryPath )

{
NTSTATUS              Status; 
PSECURITY_DESCRIPTOR  SecurityDescriptor;
OBJECT_ATTRIBUTES     ObjectAttributes;
UNICODE_STRING        uPortName;


    // Open the registry and read in all the setting we will use in kernel mode
    EnumerateRegistryValues( theRegistryPath );

   // DDK : "...Add itself to the global list of registered minifilters and to provide 
   //        the Filter Manager with a list of callback functions and other information 
   //        about the minifilter."
   Status = FltRegisterFilter( theDriverObject,
                               &cfsd_FilterRegistration,
                               &gFilterPointer );

    if ( NT_SUCCESS( Status ) )
    {

#if ENABLE_USER_INTERFACE

     Status  = FltBuildDefaultSecurityDescriptor( &SecurityDescriptor,
                                                  FLT_PORT_ALL_ACCESS );

     if ( NT_SUCCESS( Status ) ) 
     {

      RtlInitUnicodeString( &uPortName, USER_COMMUNICATION_PORT_NAME );

      InitializeObjectAttributes( &ObjectAttributes,
                                  &uPortName,
                                  OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                  NULL,
                                  SecurityDescriptor );

 KdPrint( (PRINT_TAG "Attempting to create Communication Port for user mode access\n") );

        Status = FltCreateCommunicationPort( gFilterPointer,                 // Filter
                                             &gUserModeConnection.ServerPort,// *ServerPort
                                             &ObjectAttributes,              // ObjectAttributes
                                             NULL,                           // ServerPortCookie
                                             cfsd_UserModeConnect,           // ConnectNotifyCallback
                                             cfsd_UserModeDisconnect,        // DisconnectNotifyCallback
                                             cfsd_UserModeCommunication,     // MessageNotifyCallback
                                             1 );                            // MaxConnections

        FltFreeSecurityDescriptor( SecurityDescriptor );

        // If we failed to create a communications port then we are going to fail the driver
        if ( !NT_SUCCESS( Status ) ) 
        {
 KdPrint( (PRINT_TAG "Failed FltCreateCommunicationPort() with NTSTATUS 0x%x\n",Status ) );

         // Release our hidden data memory
         ExFreePoolWithTag( gHiddenData, 'parC' );

         return Status;
        }

 KdPrint( (PRINT_TAG "Created ServerPort 0x%X\n", gUserModeConnection.ServerPort ) );

     }

#endif
     // DDK : "...Notifies the Filter Manager that the minifilter is ready to 
     //        begin attaching to volumes and filtering I/O requests"
     Status = FltStartFiltering( gFilterPointer );

     if ( !NT_SUCCESS( Status )) 
     {

#if ENABLE_USER_INTERFACE
      FltCloseCommunicationPort( gUserModeConnection.ServerPort );
#endif
      // If we failed FltStartFiltering() then we unregister ourself with the Filter Manager 
      // so that we no longer recieve calls to process I/O operations.
      FltUnregisterFilter( gFilterPointer );

      // Release our hidden data memory
      ExFreePoolWithTag( gHiddenData, 'parC' );
     }
    }

 return Status;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS
cfsd_Unload( FLT_FILTER_UNLOAD_FLAGS theFlags )

{
 UNREFERENCED_PARAMETER( theFlags );

#if ENABLE_USER_INTERFACE
 KdPrint( (PRINT_TAG "Closing ServerPort 0x%X\n",gUserModeConnection.ServerPort ) );

   FltCloseCommunicationPort( gUserModeConnection.ServerPort );
#endif

   // DDK : "...Unregister itself so that the Filter Manager no longer calls it to 
   //        process I/O operations. "
   FltUnregisterFilter( gFilterPointer );

   // Release our hidden data memory
   ExFreePoolWithTag( gHiddenData, 'parC' );

   // Release our attach method data memory
   ExFreePoolWithTag( gAttachRequirements, 'parC' );

 return STATUS_SUCCESS;
}


/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS
cfsd_InstanceSetup( IN PCFLT_RELATED_OBJECTS FltObjects,
                    IN FLT_INSTANCE_SETUP_FLAGS Flags,
                    IN DEVICE_TYPE VolumeDeviceType,
                    IN FLT_FILESYSTEM_TYPE VolumeFilesystemType )

{

USHORT     i;

 UNREFERENCED_PARAMETER( FltObjects );

// *************************************************************************************

 KdPrint( (PRINT_TAG "Instance Setup Flags 0x%x\n",Flags) );

    // Handle our instance setup under different situations and decide if we want
    // to attach under those circumstances
     if ( ( FlagOn( FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT, Flags ) ) &&
          ( FlagOn( FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT, gAttachRequirements->InstanceFlags ) ) )
     {
       // If FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME is set and we are not accepting it, return STATUS_FLT_DO_NOT_ATTACH
       if ( ( FlagOn( FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME, Flags ) ) &&
            ( !FlagOn( FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME, gAttachRequirements->InstanceFlags ) ) )
       {
 KdPrint( (PRINT_TAG "REFUSED FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT -> FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME\n") );
        return STATUS_FLT_DO_NOT_ATTACH;
       } // End if ( ( FlagOn( FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME, Flags ) ) &&
         //          ( !FlagOn( FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME, gAttachRequirements->InstanceFlags ) ) )

#if DBG
  if ( FlagOn( FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME, Flags ) )
  {
 KdPrint( (PRINT_TAG "ATTACHED - FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT -> FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME\n") );
  }
  else
  {
 KdPrint( (PRINT_TAG "ATTACHED - FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT\n") );
  } 
#endif 

     } // End if ( ( FlagOn( FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT, Flags ) ) &&
       //          ( FlagOn( FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT, gAttachRequirements->InstanceFlags ) ) )
     else if ( ( FlagOn( FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT, Flags ) ) &&
               ( FlagOn( FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT, gAttachRequirements->InstanceFlags ) ) )
     {
 KdPrint( (PRINT_TAG "fInstanceSetup() - FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT\n") );

     } // End if ( ( FlagOn( FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT, Flags ) ) &&
       //          ( FlagOn( FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT, gAttachRequirements->InstanceFlags ) ) )
     else
     {
 KdPrint( (PRINT_TAG "WILL NOT ATTACH - Unknown FLAGS in fInstanceSetup()\n") );
      return STATUS_FLT_DO_NOT_ATTACH;
     }

// *************************************************************************************


   switch ( VolumeDeviceType )
   {
    case FILE_DEVICE_UNKNOWN:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_UNKNOWN ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume Device type : FILE_DEVICE_UNKNOWN\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume Device type : FILE_DEVICE_UNKNOWN\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_CD_ROM_FILE_SYSTEM ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume Device type : FILE_DEVICE_CD_ROM_FILE_SYSTEM\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume Device type : FILE_DEVICE_CD_ROM_FILE_SYSTEM\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FILE_DEVICE_DFS_FILE_SYSTEM:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_DFS_FILE_SYSTEM ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume Device type : FILE_DEVICE_DFS_FILE_SYSTEM\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume Device type : FILE_DEVICE_DFS_FILE_SYSTEM\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FILE_DEVICE_DISK_FILE_SYSTEM:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_DISK_FILE_SYSTEM ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume Device type : FILE_DEVICE_DISK_FILE_SYSTEM\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume Device type : FILE_DEVICE_DISK_FILE_SYSTEM\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FILE_DEVICE_NETWORK_FILE_SYSTEM:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_NETWORK_FILE_SYSTEM ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume Device type : FILE_DEVICE_NETWORK_FILE_SYSTEM\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume Device type : FILE_DEVICE_NETWORK_FILE_SYSTEM\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }

    case FILE_DEVICE_TAPE_FILE_SYSTEM:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_TAPE_FILE_SYSTEM ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume Device type : FILE_DEVICE_TAPE_FILE_SYSTEM\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume Device type : FILE_DEVICE_TAPE_FILE_SYSTEM\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }

    case FILE_DEVICE_NULL:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_NULL ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume Device type : FILE_DEVICE_NULL\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume Device type : FILE_DEVICE_NULL\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }

    case FILE_DEVICE_VIRTUAL_DISK:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_VIRTUAL_DISK ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume Device type : FILE_DEVICE_VIRTUAL_DISK\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume Device type : FILE_DEVICE_VIRTUAL_DISK\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
        default :
               {
 KdPrint( (PRINT_TAG "REFUSED to ATTACHED to NONSUPPORTED Volume Device type : 0x%x\n",VolumeDeviceType ) );

                return STATUS_FLT_DO_NOT_ATTACH;
               }

   } // End switch ( VolumeDeviceType 

// *************************************************************************************


   switch ( VolumeFilesystemType )
   {
    case FLT_FSTYPE_UNKNOWN:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_UNKNOWN ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to File System type : FLT_FSTYPE_UNKNOWN\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_UNKNOWN\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_RAW:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_RAW ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_RAW\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_RAW\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_NTFS:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_NTFS ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_NTFS\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_NTFS\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_FAT:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_FAT ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_FAT\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_FAT\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_CDFS:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_CDFS ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_CDFS\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_CDFS\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_UDFS:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_UDFS ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_UDFS\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_UDFS\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_LANMAN:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_LANMAN ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_LANMAN\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_LANMAN\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_WEBDAV:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_WEBDAV ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_WEBDAV\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_WEBDAV\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }
         break;
        }
    case FLT_FSTYPE_RDPDR:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_RDPDR ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_RDPDR\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_RDPDR\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_NFS:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_NFS ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_NFS\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_NFS\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_MS_NETWARE:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_MS_NETWARE) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_MS_NETWARE\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_MS_NETWARE\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_NETWARE:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_NETWARE ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_NETWARE\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_NETWARE\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_BSUDF:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_BSUDF ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_BSUDF\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_BSUDF\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_MUP:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_MUP ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_MUP\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_MUP\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_RSFX:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_RSFX) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_RSFX\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_RSFX\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_ROXIO_UDF1:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_ROXIO_UDF1) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_ROXIO_UDF1\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_ROXIO_UDF1\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_ROXIO_UDF2:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_ROXIO_UDF2 ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_ROXIO_UDF2\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_ROXIO_UDF2\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_ROXIO_UDF3:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_ROXIO_UDF3 ) )
          {
 KdPrint( (PRINT_TAG "ATTACHED to Volume File System type : FLT_FSTYPE_ROXIO_UDF3\n" ) );
          }
          else
          {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_ROXIO_UDF3\n" ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    default :
           {
 KdPrint( (PRINT_TAG "REFUSED to ATTACH to NONSUPPORTED Volume File System type : 0x%x\n",VolumeFilesystemType ) );

            return STATUS_FLT_DO_NOT_ATTACH;
           }

   } // End switch ( VolumeFilesystemType )

// *************************************************************************************

 return STATUS_SUCCESS;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
#if ENABLE_USER_INTERFACE
NTSTATUS
cfsd_UserModeConnect( IN PFLT_PORT ClientPort,
                      IN PVOID ServerPortCookie,
                      IN PVOID ConnectionContext,
                      IN ULONG SizeOfContext,
                      OUT PVOID *ConnectionCookie )

{

 UNREFERENCED_PARAMETER( ServerPortCookie );
 UNREFERENCED_PARAMETER( ConnectionContext );
 UNREFERENCED_PARAMETER( SizeOfContext);
 UNREFERENCED_PARAMETER( ConnectionCookie );

   gUserModeConnection.UserProcess = PsGetCurrentProcess();
   gUserModeConnection.ClientPort  = ClientPort;

 KdPrint( (PRINT_TAG "Created ClientPort 0x%X in Process 0x%X\n", gUserModeConnection.ClientPort, gUserModeConnection.UserProcess ) );

 return STATUS_SUCCESS;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
VOID
cfsd_UserModeDisconnect( IN PVOID ConnectionCookie )

{

 UNREFERENCED_PARAMETER( ConnectionCookie );

 KdPrint( (PRINT_TAG "Closed ClientPort 0x%X in Process 0x%X\n", gUserModeConnection.ClientPort, gUserModeConnection.UserProcess ) );

   // Close our handle to the connection
   FltCloseClientPort( gFilterPointer, &gUserModeConnection.ClientPort );
   // Reset our UserProcess field
   gUserModeConnection.UserProcess = NULL;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
NTSTATUS
cfsd_UserModeCommunication( IN PVOID ConnectionCookie,
                            IN PVOID InputBuffer  OPTIONAL,
                            IN ULONG InputBufferSize,
                            OUT PVOID OutputBuffer OPTIONAL,
                            IN ULONG OutputBufferSize,
                            OUT PULONG ReturnOutputBufferLength )

{
STRING		   FNameString;


 KdPrint( (PRINT_TAG "File Name from USER MODE to hide (%s) [%d]\n", InputBuffer, InputBufferSize ) );

/*
!!!!!!!!!!!!!!!
ALL TEMP CODE SO YOU CAN TEST THE DRIVER FROM USER MODE WITHOUT HAVING TO RECOMPILE TO CHANGE A FILE NAME
!!!!!!!!!!!!!!
*/

  try 
  {
   RtlInitAnsiString( &FNameString, InputBuffer );

    RtlAnsiStringToUnicodeString( &uFName, &FNameString, TRUE );

    RtlUpcaseUnicodeString( &uFName, &uFName, FALSE );

    gHiddenData->HFile = uFName;

// By not calling this we are going to leak memory everytime user mode makes a file name change. This is just a kludge
// to let user mode experiement with file names
//	RtlFreeUnicodeString(&uFName);

  }
  except( EXCEPTION_EXECUTE_HANDLER ) 
  {
   return GetExceptionCode();
  }

 return STATUS_SUCCESS;
}




#endif