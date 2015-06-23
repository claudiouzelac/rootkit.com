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
 *   Entry point for the driver, minifilter configuration, instance attachment validation
 *
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#include "cfsd.h"

#include "base.h"
#include "IRP_MJ_directory.h"
#include "IRP_MJ_create.h"
#include "IRP_MJ_setinformation.h"
#include "registry.h"


#include "..\inc\crossover.h"


/* #################################################################################

  DDK : "...Structure is used to register operation callback routines"

*/
CONST FLT_OPERATION_REGISTRATION cfsd_Callbacks[] = {

#if FILTER_IRP_MJ_DIRECTORY_CONTROL
    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      NULL,//PreDirectoryControl
      PostDirectoryControl },
#endif

#if FILTER_IRP_MJ_CREATE
    { IRP_MJ_CREATE,
      0,
      NULL,//PreCreate,
      PostCreate},
#endif

#if FILTER_IRP_MJ_SET_INFORMATION
    { IRP_MJ_SET_INFORMATION,
      0,
      NULL,//PreSetInformation,
      PostSetInformation},
#endif

    { IRP_MJ_OPERATION_END }
};

/* #################################################################################

   DDK : "...Structure is passed as a parameter to FltRegisterFilter()."

*/
CONST FLT_REGISTRATION cfsd_FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,//FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP, //  Flags 
    /* If FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP is set you cannot use 'net stop cfsd' to
       stop the driver, but fltmc unload cfsd will still be possible. To make the driver unloadable
       you must set the above flag and uncomment return STATUS_FLT_DO_NOT_DETACH; in cfsd_Unload()
     */
    NULL,                               //  ContextRegistration

    cfsd_Callbacks,                     //  OperationRegistration

    cfsd_Unload,                        //  FilterUnloadCallback

    cfsd_InstanceSetup,                 //  InstanceSetupCallback
    NULL,                               //  InstanceQueryTeardownCallback
    NULL,                               //  InstanceTeardownStartCallback
    cfsd_InstanceTeardownComplete,      //  InstanceTeardownCompleteCallback

    NULL,                               //  GenerateFileNameCallback
    NULL,                               //  NormalizeNameComponentCallback
    NULL                                //  NormalizeContextCleanupCallback

};
/* ################################################################################# */

#if ENABLE_USER_INTERFACE
USER_MODE_CONNECTION   gUserModeConnection;
#endif

PFLT_FILTER            gFilterPointer;




// kludge var
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
         ExFreePoolWithTag( gFileData, 'parC' );

         return Status;
        }

 DBG_PRINT( DbgOutput, DBG_USERMODE, (PRINT_TAG_USERMODE "Created communication server port 0x%X for usermode access\n", gUserModeConnection.ServerPort ));

     }

#endif // End #if ENABLE_USER_INTERFACE

     // DDK : "...Notifies the Filter Manager that the minifilter is ready to 
     //        begin attaching to volumes and filtering I/O requests"
     Status = FltStartFiltering( gFilterPointer );

     if ( !NT_SUCCESS( Status )) 
     {

#if ENABLE_USER_INTERFACE
      FltCloseCommunicationPort( gUserModeConnection.ServerPort );
#endif // End #if ENABLE_USER_INTERFACE

      // If we failed FltStartFiltering() then we unregister ourself with the Filter Manager 
      // so that we no longer recieve calls to process I/O operations.
      FltUnregisterFilter( gFilterPointer );

      // Release our hidden data memory
      ExFreePoolWithTag( gFileData, 'parC' );
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


// *** ENABLE THIS IS YOU DO NOT WANT THE DRIVER TO EVER BE UNLOADED and 
// SET FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP in FLT_REGISTRATION ALSO ***
//return STATUS_FLT_DO_NOT_DETACH;


#if ENABLE_USER_INTERFACE
 DBG_PRINT( DbgOutput, DBG_USERMODE, (PRINT_TAG_USERMODE "Closing ServerPort 0x%X\n",gUserModeConnection.ServerPort ) );

   FltCloseCommunicationPort( gUserModeConnection.ServerPort );
#endif // End #if ENABLE_USER_INTERFACE


   // DDK : "...Unregister itself so that the Filter Manager no longer calls it to 
   //        process I/O operations. "
   FltUnregisterFilter( gFilterPointer );

   // Release our hidden data memory
   ExFreePoolWithTag( gFileData, 'parC' );

#if FILTER_IRP_MJ_CREATE
   // Release our attach method data memory
   ExFreePoolWithTag( gProtectedData, 'parC' );
#endif

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

#if DBG
UCHAR                    VPBuffer[sizeof(FLT_VOLUME_PROPERTIES)+512];
PFLT_VOLUME_PROPERTIES   VolumeProperties = (PFLT_VOLUME_PROPERTIES)VPBuffer;
PDEVICE_OBJECT           theDeviceObject = NULL;
ULONG                    ReturnedLength;
NTSTATUS                 Status;
UNICODE_STRING           DosName;


     Status = FltGetVolumeProperties( FltObjects->Volume,
                                      VolumeProperties,
                                      sizeof( VPBuffer ),
                                      &ReturnedLength );

     if ( !NT_SUCCESS( Status ) ) 
     {
     }

     // Zero it so we can show a NULL if no DOS name is found
     RtlZeroMemory( &DosName, sizeof( UNICODE_STRING ) );

     Status = FltGetDiskDeviceObject( FltObjects->Volume, &theDeviceObject );

     if ( NT_SUCCESS( Status ) ) 
     {
      Status = IoVolumeDeviceToDosName( theDeviceObject, &DosName );
     }
     else
     {  
     }

#endif // End #if DBG

 UNREFERENCED_PARAMETER( FltObjects );

// *************************************************************************************


 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "** [ ATTACHMENT REQUEST 0x%X ] **", Flags ) );

    // Handle our instance setup under different situations and decide if we want
    // to attach under those circumstances
     if ( ( FlagOn( FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT, Flags ) ) &&
          ( FlagOn( FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT, gAttachRequirements->InstanceFlags ) ) )
     {
       // If FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME is set and we are not accepting it, return STATUS_FLT_DO_NOT_ATTACH
       if ( ( FlagOn( FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME, Flags ) ) &&
            ( !FlagOn( FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME, gAttachRequirements->InstanceFlags ) ) )
       {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED attach method - FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT -> FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME\n") );
        return STATUS_FLT_DO_NOT_ATTACH;
       } // End if ( ( FlagOn( FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME, Flags ) ) &&
         //          ( !FlagOn( FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME, gAttachRequirements->InstanceFlags ) ) )

#if DBG
  if ( FlagOn( FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME, Flags ) )
  {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ACCEPTED attach method - FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT -> FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME\n") );
  }
  else
  {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ACCEPTED attach method - FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT\n") );
  } 
#endif // End #if DBG

     } // End if ( ( FlagOn( FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT, Flags ) ) &&
       //          ( FlagOn( FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT, gAttachRequirements->InstanceFlags ) ) )
     else if ( ( FlagOn( FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT, Flags ) ) &&
               ( FlagOn( FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT, gAttachRequirements->InstanceFlags ) ) )
     {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ACCEPTED attach method - FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT\n") );

     } // End if ( ( FlagOn( FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT, Flags ) ) &&
       //          ( FlagOn( FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT, gAttachRequirements->InstanceFlags ) ) )
     else
     {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED ATTACHMENT - Unknown FLAGS in fInstanceSetup()\n") );
      return STATUS_FLT_DO_NOT_ATTACH;
     }

// *************************************************************************************


   switch ( VolumeDeviceType )
   {
    case FILE_DEVICE_UNKNOWN:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_UNKNOWN ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_CD_ROM_FILE_SYSTEM ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume  <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FILE_DEVICE_DFS_FILE_SYSTEM:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_DFS_FILE_SYSTEM ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FILE_DEVICE_DISK_FILE_SYSTEM:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_DISK_FILE_SYSTEM ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FILE_DEVICE_NETWORK_FILE_SYSTEM:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_NETWORK_FILE_SYSTEM ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }

    case FILE_DEVICE_TAPE_FILE_SYSTEM:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_TAPE_FILE_SYSTEM ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }

    case FILE_DEVICE_NULL:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_NULL ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }

    case FILE_DEVICE_VIRTUAL_DISK:
        {
          if ( FlagOn( gAttachRequirements->InstanceVolumeDeviceTypes, MASK_FILE_DEVICE_VIRTUAL_DISK ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume <%wZ> [%wZ] (%wZ)\n", &DosName, &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
        default :
               {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACHED to NONSUPPORTED Volume Device type : 0x%x\n",VolumeDeviceType ) );

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
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to File System type : FLT_FSTYPE_UNKNOWN [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_UNKNOWN [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_RAW:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_RAW ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_RAW [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_RAW [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_NTFS:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_NTFS ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_NTFS [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_NTFS [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_FAT:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_FAT ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_FAT [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_FAT [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_CDFS:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_CDFS ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_CDFS [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_CDFS [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_UDFS:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_UDFS ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_UDFS [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_UDFS [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_LANMAN:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_LANMAN ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_LANMAN [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_LANMAN [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_WEBDAV:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_WEBDAV ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_WEBDAV [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_WEBDAV [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }
         break;
        }
    case FLT_FSTYPE_RDPDR:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_RDPDR ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_RDPDR [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_RDPDR [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_NFS:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_NFS ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_NFS [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_NFS [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_MS_NETWARE:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_MS_NETWARE) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_MS_NETWARE [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_MS_NETWARE [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_NETWARE:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_NETWARE ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_NETWARE [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_NETWARE [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_BSUDF:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_BSUDF ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_BSUDF [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_BSUDF [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_MUP:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_MUP ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_MUP [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_MUP [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_RSFX:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_RSFX) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_RSFX [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_RSFX [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_ROXIO_UDF1:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_ROXIO_UDF1) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_ROXIO_UDF1 [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_ROXIO_UDF1 [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_ROXIO_UDF2:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_ROXIO_UDF2 ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_ROXIO_UDF2 [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_ROXIO_UDF2 [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    case FLT_FSTYPE_ROXIO_UDF3:
        {

          if ( FlagOn( gAttachRequirements->InstancedFileSystemTypes, MASK_FSTYPE_ROXIO_UDF3 ) )
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "ATTACHED to Volume File System type : FLT_FSTYPE_ROXIO_UDF3 [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
          }
          else
          {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to Volume File System type : FLT_FSTYPE_ROXIO_UDF3 [%wZ]\n",&VolumeProperties->FileSystemDriverName ) );
           return STATUS_FLT_DO_NOT_ATTACH;
          }

         break;
        }
    default :
           {
 DBG_PRINT( DbgOutput, DBG_ATTACH_INSTANCE, (PRINT_TAG_ATTACH "REFUSED to ATTACH to NONSUPPORTED Volume File System type : 0x%x\n",VolumeFilesystemType ) );

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
VOID
cfsd_InstanceTeardownComplete( IN PCFLT_RELATED_OBJECTS  FltObjects,
                               IN FLT_INSTANCE_TEARDOWN_FLAGS  Reason )

{

#if DBG

UCHAR                    VPBuffer[sizeof(FLT_VOLUME_PROPERTIES)+512];
PFLT_VOLUME_PROPERTIES   VolumeProperties = (PFLT_VOLUME_PROPERTIES)VPBuffer;
ULONG                    ReturnedLength;
NTSTATUS                 Status;


 DBG_PRINT( DbgOutput, DBG_DETACH_INSTANCE, (PRINT_TAG_DETACH "## [ DETACHMENT REQUEST 0x%X ] ##", Reason ) );

    Status = FltGetVolumeProperties( FltObjects->Volume,
                                     VolumeProperties,
                                     sizeof( VPBuffer ),
                                     &ReturnedLength );

    if ( !NT_SUCCESS( Status ) ) 
    {
    }

    switch ( Reason )
    { 
     case FLTFL_INSTANCE_TEARDOWN_FILTER_UNLOAD:
         {
 DBG_PRINT( DbgOutput, DBG_DETACH_INSTANCE, (PRINT_TAG_DETACH "DETACHED - FLTFL_INSTANCE_TEARDOWN_FILTER_UNLOAD\n") );
          break;
         }
     case FLTFL_INSTANCE_TEARDOWN_INTERNAL_ERROR:
         {
 DBG_PRINT( DbgOutput, DBG_DETACH_INSTANCE, (PRINT_TAG_DETACH "DETACHED - FLTFL_INSTANCE_TEARDOWN_INTERNAL_ERROR\n") );

          break;
         }
     case FLTFL_INSTANCE_TEARDOWN_MANDATORY_FILTER_UNLOAD:
         {
 DBG_PRINT( DbgOutput, DBG_DETACH_INSTANCE, (PRINT_TAG_DETACH "DETACHED - FLTFL_INSTANCE_TEARDOWN_MANDATORY_FILTER_UNLOAD\n") );

          break;
         }
     case FLTFL_INSTANCE_TEARDOWN_MANUAL:
         {
 DBG_PRINT( DbgOutput, DBG_DETACH_INSTANCE, (PRINT_TAG_DETACH "DETACHED - FLTFL_INSTANCE_TEARDOWN_MANUAL\n") );

        break;
         }
     case FLTFL_INSTANCE_TEARDOWN_VOLUME_DISMOUNT:
         {
 DBG_PRINT( DbgOutput, DBG_DETACH_INSTANCE, (PRINT_TAG_DETACH "DETACHED - FLTFL_INSTANCE_TEARDOWN_VOLUME_DISMOUNT\n") );

          break;
         }
     default : // security
         {
          break;
         }
    } // End switch ( Reason )

 DBG_PRINT( DbgOutput, DBG_DETACH_INSTANCE, (PRINT_TAG_DETACH "Detaching [%wZ] (%wZ) %wZ\n", &VolumeProperties->RealDeviceName, &VolumeProperties->FileSystemDeviceName, &VolumeProperties->FileSystemDriverName ) );

#endif // End #if DBG

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

 DBG_PRINT( DbgOutput, DBG_USERMODE, (PRINT_TAG_USERMODE "Created ClientPort 0x%X in Process 0x%X\n", gUserModeConnection.ClientPort, gUserModeConnection.UserProcess ) );

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


 DBG_PRINT( DbgOutput, DBG_USERMODE, (PRINT_TAG_USERMODE "Closed ClientPort 0x%X in Process 0x%X\n", gUserModeConnection.ClientPort, gUserModeConnection.UserProcess ) );

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

#if FILTER_BY_NAME

 DBG_PRINT( DbgOutput, DBG_USERMODE, (PRINT_TAG_USERMODE "File Name from USER MODE to hide (%s) [%d]\n", InputBuffer, InputBufferSize ) );

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

    gFileData->NameInfo.Name = uFName;

// By not calling this we are going to leak memory everytime user mode makes a file name change. This is just a kludge
// to let user mode experiement with file names
//	RtlFreeUnicodeString(&uFName);

  }
  except( EXCEPTION_EXECUTE_HANDLER ) 
  {
   return GetExceptionCode();
  }

#endif // End #if FILTER_BY_NAME


 return STATUS_SUCCESS;
}

#endif // End #if ENABLE_USER_INTERFACE
