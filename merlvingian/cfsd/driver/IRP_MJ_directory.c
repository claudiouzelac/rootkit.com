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
 *   Dispatch/Completion functions for IRP_MJ_DIRECTORY_CONTROL
 *
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#include "IRP_MJ_directory.h"
#include "CompareFilters.h"
#include "base.h"


#if FILTER_IRP_MJ_DIRECTORY_CONTROL
/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 * This is equal to a dispatch function in the legacy model
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_PREOP_CALLBACK_STATUS
PreDirectoryControl( IN OUT PFLT_CALLBACK_DATA Data,
                     IN PCFLT_RELATED_OBJECTS FltObjects,
                     OUT PVOID *CompletionContext )

{

return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS 
PostDirectoryControl( IN OUT PFLT_CALLBACK_DATA Data,
                      IN PCFLT_RELATED_OBJECTS FltObjects,
                      IN PVOID CompletionContext,
                      IN FLT_POST_OPERATION_FLAGS Flags )

{
PVOID                        DirectoryBuffer = NULL;
FLT_POSTOP_CALLBACK_STATUS   ReturnStatus = FLT_POSTOP_FINISHED_PROCESSING;


     // If we are draining we have no reason to process. This flag could have been set
     // and we have been called before a lower filter driver of the filesystem has completed. So we have
     // no real way to know if our data is correct/valid/or even there, and we are unloading anyways so
     // just exit with no furthur processing. 
     // NOTE When Draining* DDK : "Data parameter points to a copy of the original callback 
     // data structure for the operation, not the original callback data structure"
     if ( FlagOn( FLTFL_POST_OPERATION_DRAINING, Flags ) )
	 {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "POST OPERATION DRAINING - Exiting fPostOperationDirectoryControl()\n") );
      return ReturnStatus;
     }

     //  If the operation failed or there is no data we have no reason to continue so exit
     // *NOTE* Seem to see this spam alot, need to look into why this calls alot
     if ( !NT_SUCCESS( Data->IoStatus.Status ) || ( Data->IoStatus.Information == 0 ) ) 
     {

#if DBG
      if ( !NT_SUCCESS( Data->IoStatus.Status ) )
      {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "IoStatus.Status = 0x%x - Exiting fPostOperationDirectoryControl()\n",Data->IoStatus.Status ) );
      } 
      if ( ( Data->IoStatus.Information == 0 ) )
      {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "Information = [%d] - Exiting fPostOperationDirectoryControl()\n",Data->IoStatus.Information ) );
      }
#endif

      return ReturnStatus;
     }

      // We attempt to get to a safe IRQL for processing. If we can not then we just fail and exit
      // without and processing. Odds are we are going to have a user buffer and need to call this 
      // anyways, odds also favor that our processing we want to do cannot be done at DPC
      if ( !FltDoCompletionProcessingWhenSafe( Data,                    // Data
                                               FltObjects,              // FltObjects
                                               CompletionContext,       // CompletionContext
                                               Flags,                   // Flags
                                               PostDirectoryControlSafe,// SafePostCallback
                                               &ReturnStatus ) )        // RetPostOperationsStatus
      {
       // If cannot get to a safe IRQL we just fail
       Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
       Data->IoStatus.Information = 0;
      }

return ReturnStatus;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
PostDirectoryControlSafe ( IN OUT PFLT_CALLBACK_DATA Data,
                           IN PCFLT_RELATED_OBJECTS FltObjects,
                           IN PVOID CompletionContext,
                           IN FLT_POST_OPERATION_FLAGS Flags )

{

PVOID       SafeBuffer;
NTSTATUS    Status;
NTSTATUS    ReturnStatus  = FLT_POSTOP_FINISHED_PROCESSING;


 UNREFERENCED_PARAMETER( FltObjects );
 UNREFERENCED_PARAMETER( CompletionContext );
 UNREFERENCED_PARAMETER( Flags );
 
    // Lock the buffer so we can access it safely
    Status = FltLockUserBuffer( Data );

    if ( !NT_SUCCESS( Status ) ) 
    {
     // If we can't lock the buffer, fail the operation
     Data->IoStatus.Status = Status;
     Data->IoStatus.Information = 0;
    } 
    else 
    {
     // Get a system address for this buffer.
      SafeBuffer = MmGetSystemAddressForMdlSafe( Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                                                 NormalPagePriority );

      if ( SafeBuffer == NULL ) 
      {
       // If we couldn not get a SYSTEM buffer address fail and exit
       Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
       Data->IoStatus.Information = 0;
      } 
      else 
      {
        // Determine which IRP_MN_XXX we have and direct control accordingly
        if ( Data->Iopb->MinorFunction == IRP_MN_QUERY_DIRECTORY )
        {
         // Multiplex which FileInformationClass type we are going to be processing and direct it
         // to the proper processing function
         switch ( Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass )
         {
          case FileBothDirectoryInformation:
          { 
           ReturnStatus = ProcessFileBothDirectoryInformation( Data, SafeBuffer );
           break;
          }
          case FileDirectoryInformation:
          {
           ProcessFileDirectoryInformation( Data );
           break;
          }
          case FileFullDirectoryInformation:
          {
           ProcessFileFullDirectoryInformation( Data );
           break;
          }
          case FileIdBothDirectoryInformation:
          {
           ProcessFileIdBothDirectoryInformation( Data );
           break;
          }
          case FileIdFullDirectoryInformation:
          { 
           ProcessFileIdFullDirectoryInformation( Data );
           break;
          }
          case FileNamesInformation:
          {
           ReturnStatus = ProcessFileNamesInformation( FltObjects, Data, SafeBuffer );
           break;
          }
          case FileObjectIdInformation:
          { 
           ProcessFileObjectIdInformation( Data );
           break;
          }
          case FileReparsePointInformation:
          { 
           ProcessFileReparsePointInformation( Data );
           break;
          }
          default : // Default Security catch
          {
           KdPrint( (PRINT_TAG "BAD MOJO") );
           break;
          }
         } // End switch ( iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass )

        }
        else if ( Data->Iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY )
        {
         ProcessMNNotifyChangeDirectory( Data );
        }
        else // Default Security catch
        {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "BAD MOJO\n") );
        }

      } // End if ( SafeBuffer == NULL) 
    } // End if (!NT_SUCCESS(status)) 

 return ReturnStatus;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
ProcessMNNotifyChangeDirectory( IN OUT PFLT_CALLBACK_DATA Data )

{


//FltNotifyFilterChangeDirectory()


/*

FILE_NOTIFY_CHANGE_FILE_NAME A file has been added, deleted, or renamed in this directory. 
FILE_NOTIFY_CHANGE_DIR_NAME A subdirectory has been created, removed, or renamed. 
FILE_NOTIFY_CHANGE_NAME This directory's name has changed. 
FILE_NOTIFY_CHANGE_ATTRIBUTES The value of an attribute of this file, such as last access time, has changed. 
FILE_NOTIFY_CHANGE_SIZE This file's size has changed. 
FILE_NOTIFY_CHANGE_LAST_WRITE This file's last modification time has changed. 
FILE_NOTIFY_CHANGE_LAST_ACCESS This file's last access time has changed. 
FILE_NOTIFY_CHANGE_CREATION This file's creation time has changed. 
FILE_NOTIFY_CHANGE_EA This file's extended attributes have been modified. 
FILE_NOTIFY_CHANGE_SECURITY This file's security information has changed. 
FILE_NOTIFY_CHANGE_STREAM_NAME A file stream has been added, deleted, or renamed in this directory. 
FILE_NOTIFY_CHANGE_STREAM_SIZE This file stream's size has changed. 
FILE_NOTIFY_CHANGE_STREAM_WRITE This file stream's data has changed. 


*/

return FLT_POSTOP_FINISHED_PROCESSING;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
ProcessFileBothDirectoryInformation( IN OUT PFLT_CALLBACK_DATA Data,
                                     IN OUT PVOID SafeBuffer )

{

/*
 ** Other possible flag settings in bitmask iopb->OperationFlags **

 SL_INDEX_SPECIFIED - Used for IRP_MJ_DIRECTORY_CONTROL, IRP_MJ_QUERY_EA, and IRP_MJ_SET_QUOTA.
 If this flag is set, the scan for directory, quota, or extended-attribute information should 
 begin at the entry in the list whose index is specified. 

 SL_OVERRIDE_VERIFY_VOLUME - Used for IRP_MJ_DIRECTORY_CONTROL, IRP_MJ_READ, and IRP_MJ_WRITE. 
 If this flag is set, the I/O operation should be performed even if the DO_VERIFY_VOLUME flag 
 is set on the volume's device object

 SL_RESTART_SCAN - Used for IRP_MJ_DIRECTORY_CONTROL, IRP_MJ_QUERY_EA, and IRP_MJ_SET_QUOTA. 
 If this flag is set, the scan for directory, quota, or extended-attribute information should 
 begin at the first entry in the directory or list. Otherwise, the scan should be resumed from the previous scan. 

 SL_WATCH_TREE - Used for IRP_MJ_DIRECTORY_CONTROL. If this flag is set, all subdirectories of this directory 
 should also be watched. Otherwise, only the directory itself is to be watched. 

 SL_RETURN_SINGLE_ENTRY Return only the first entry that is found.
*/


PFILE_BOTH_DIR_INFORMATION    DirectoryBuffer     = NULL;
PFILE_BOTH_DIR_INFORMATION    NextDirectoryBuffer = NULL;
PVOID                         TemporaryBuffer     = NULL;

ULONG   BufferPosition = 0;
ULONG   ModifiedLength = 0;

//////////////////////

PFLT_FILE_NAME_INFORMATION   nameInfo;
NTSTATUS                     Status;

    // Parse out our info for matching volume, path, share, and stream
    Status = FltGetFileNameInformation( Data,
                                        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                                        &nameInfo );





/////////////////////////////////////////



/*
KdPrint( (PRINT_TAG "Query Name (%wZ) DirectoryBuffer Size [%d]",Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName,
                              Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length) );
KdPrint( (PRINT_TAG "IOStatus Info %d\n", Data->IoStatus.Information) );
*/

    // Overlay our buffer onto a structure for processing
    DirectoryBuffer = ( PFILE_BOTH_DIR_INFORMATION ) SafeBuffer;

  do {
       // Incrament our buffer position to match the next entry
       BufferPosition += DirectoryBuffer->NextEntryOffset;

      // Check to see if this file matches our template
#if FILTER_BY_NAME
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "CompareFileName()\n") );

      if ( !CompareFileName( DirectoryBuffer->FileName, DirectoryBuffer->FileNameLength, gFileData->NameInfo, nameInfo) )
      {
       // If we failed to match then we are not interested in this entry so we incrament the loop
       NextDirectoryBuffer = DirectoryBuffer;
       DirectoryBuffer = ( PFILE_BOTH_DIR_INFORMATION )((PCHAR) DirectoryBuffer + DirectoryBuffer->NextEntryOffset );

       continue;
      }
#endif

#if FILTER_BY_TIME
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "CompareFileTme()\n") );

      if ( !CompareFileTime( gFileData->TimeMaskSet,
                             gFileData->TimeMaskType,
                             DirectoryBuffer->CreationTime,
                             DirectoryBuffer->LastAccessTime,
                             DirectoryBuffer->LastWriteTime,
                             DirectoryBuffer->ChangeTime,
                             gFileData->CreationTime, 
                             gFileData->LastAccessTime, 
                             gFileData->LastWriteTime, 
                             gFileData->ChangeTime ) ) 
      {
       // If we failed to match then we are not interested in this entry so we incrament the loop
       NextDirectoryBuffer = DirectoryBuffer;
       DirectoryBuffer = ( PFILE_BOTH_DIR_INFORMATION )((PCHAR) DirectoryBuffer + DirectoryBuffer->NextEntryOffset );

       continue;
      }
#endif
#if FILTER_BY_ATTRIBUTES
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "CompareFileAttributes()\n") );

      if ( !CompareFileAttributes( DirectoryBuffer->FileAttributes, gFileData->FileAttributes, gFileData->AttributesMaskType ) )
      {
       // If we failed to match then we are not interested in this entry so we incrament the loop
       NextDirectoryBuffer = DirectoryBuffer;
       DirectoryBuffer = ( PFILE_BOTH_DIR_INFORMATION )((PCHAR) DirectoryBuffer + DirectoryBuffer->NextEntryOffset );

       continue;
      }
#endif


/* ================================================================================= **
   This is a *FIRST* -or- *MIDDLE ENTRY* in the buffer, This check is first since it is more
   likely that we are processing the middle of a list then its end or a single entry
 * ================================================================================= **/
      if ( DirectoryBuffer->NextEntryOffset > 0 )
      {

#if DBG
 if ( NextDirectoryBuffer == NULL )
 {
#if FILTER_BY_NAME
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "REMOVING FIRST ENTRY - (%wZ)\n", &gFileData->NameInfo.Name ) );
#endif
 }
 else
 {
#if FILTER_BY_NAME
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "REMOVING MIDDLE ENTRY - (%wZ)\n", &gFileData->NameInfo.Name ) );
#endif
 }
#endif

        // This is the length from start of the entry we are going to remove to the end of the buffer
        ModifiedLength = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length - BufferPosition;
        // Allocate a temporary buffer to do our patch work in
        TemporaryBuffer = ExAllocatePoolWithTag( NonPagedPool,
                                                 ModifiedLength,
                                                 POOL_TAG_TEMPORARY_BUFFER );

        try 
        {
         // Copy the memory from the end of the entry we are going to remove to the end of the buffer
         RtlCopyMemory( TemporaryBuffer,
                        ((PBYTE)DirectoryBuffer + DirectoryBuffer->NextEntryOffset),
                        ModifiedLength );               

         // Zero out the trailing data after the removed entry because it feels clean
         RtlZeroMemory( DirectoryBuffer,
                        ModifiedLength+DirectoryBuffer->NextEntryOffset);
         // Patch our temporary buffer onto the real buffer removing the current entry.
         // Since DirectoryBuffer is a relative position we can always patch over it with
         // no starting offset. This allows us to patch first, and any other entry other 
         // then the last all in the same manner.
         RtlCopyMemory( DirectoryBuffer,
                        TemporaryBuffer,
                        ModifiedLength );
  
        } 
        except ( EXCEPTION_EXECUTE_HANDLER ) 
        {
         Data->IoStatus.Status = GetExceptionCode();
         Data->IoStatus.Information = 0;

 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "Failed FILE REMOVAL - IOStatus.Status 0x%x\n",Data->IoStatus.Status) );
        }
        // Free our patchwork buffer
        ExFreePoolWithTag( TemporaryBuffer, POOL_TAG_TEMPORARY_BUFFER );

        // DDK : "...for a callback data structure to indicate that it 
        //        has modified the contents of the structure."
        FltSetCallbackDataDirty( Data );
      }
/* ================================================================================= **
   SL_RETURN_SINGLE_ENTRY Flag set with only a single entry in buffer
 * ================================================================================= **/
      else if ( ( FlagOn ( SL_RETURN_SINGLE_ENTRY, Data->Iopb->OperationFlags ) ) &&
                ( NextDirectoryBuffer == NULL ) )
	  {
#if FILTER_BY_NAME
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "REMOVING SINGLE ENTRY - (%wZ)\n", &gFileData->NameInfo.Name ) );
#endif

/* 
   Normally you would re issue the irp if you were not returning any files. I belive there
   is a differnt way to do this in a minifilter and need to reseatch the subject more.
*/


/*
        // Security wipe just makes me feel better
        RtlZeroMemory( DirectoryBuffer, sizeof( FILE_BOTH_DIR_INFORMATION ) );

        Data->IoStatus.Information = 0;
 
        DirectoryBuffer = NULL;
        // 
        NextDirectoryBuffer = DirectoryBuffer;

        Data->IoStatus.Status =  STATUS_NO_MORE_FILES; // STATUS_NO_SUCH_FILE;

        continue;
*/
      }
/* ================================================================================= **
   This is the *LAST ENTRY* in the buffer
 * ================================================================================= **/
      else if ( DirectoryBuffer->NextEntryOffset == 0 )
      {
#if FILTER_BY_NAME
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "REMOVING LAST ENTRY - (%wZ)\n", &gFileData->NameInfo.Name ) );
#endif
        // Zero out entry as well just because it feels clean
        RtlZeroMemory( DirectoryBuffer,
                       ModifiedLength+DirectoryBuffer->NextEntryOffset);
        // Zero out the NextEntryOffset like this entry never existed
        NextDirectoryBuffer->NextEntryOffset = 0;
        // DDK : "...for a callback data structure to indicate that it 
        //        has modified the contents of the structure."
        FltSetCallbackDataDirty( Data );
      }

      NextDirectoryBuffer = DirectoryBuffer;
      DirectoryBuffer = ( PFILE_BOTH_DIR_INFORMATION )((PCHAR) DirectoryBuffer + DirectoryBuffer->NextEntryOffset );

     } while ( DirectoryBuffer != NextDirectoryBuffer );


    // Free
    FltReleaseFileNameInformation( nameInfo );

return FLT_POSTOP_FINISHED_PROCESSING;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
ProcessFileNamesInformation( IN PCFLT_RELATED_OBJECTS FltObjects,
							 IN OUT PFLT_CALLBACK_DATA Data,
                             IN OUT PVOID SafeBuffer )

{

UNICODE_STRING MN;
PFILE_NAMES_INFORMATION DirectoryBuffer = NULL;
PFILE_NAMES_INFORMATION NextDirectoryBuffer = NULL;

ULONG   ModifiedLength  = 0;
PVOID   TemporaryBuffer = NULL;

/////////////
//PFLT_FILE_NAME_INFORMATION   nameInfo = NULL;
//////////////
/*

// Response to ZwQueryDirectoryFile()
DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!^^^^^FileNamesInformation\n") );



DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "Query Name (%wZ) DirectoryBuffer Size [%d]",Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName,
                            Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length) );
DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "IOStatus Info %d\n", Data->IoStatus.Information) );


    DirectoryBuffer = ( PFILE_NAMES_INFORMATION ) SafeBuffer;


if ( FlagOn ( SL_RETURN_SINGLE_ENTRY, Data->Iopb->OperationFlags ) )
{

      // Check to see if this file matches our template
      if ( CompareFileName( DirectoryBuffer->FileName, DirectoryBuffer->FileNameLength, gFileData->NameInfo, nameInfo ) )
       {


    if ( DirectoryBuffer->NextEntryOffset == 0 )
    {


// Security wipe just makes me feel better
RtlZeroMemory( DirectoryBuffer, Data->Iopb->Parameters.DirectoryControl.QueryDirectory.Length );


//Data->IoStatus.Information = 0;
//Data->IoStatus.Status      =  STATUS_NO_SUCH_FILE; //STATUS_UNSUCCESSFUL; // STATUS_NO_MORE_FILES;

//DirectoryBuffer->FileNameLength = 0;
//RtlZeroMemory( DirectoryBuffer->FileName, DirectoryBuffer->FileNameLength);

  FltSetCallbackDataDirty( Data );

//FltReissueSynchronousIo( FltObjects->Instance, Data );


    }

else
{
DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "***************************** MORE OFFSET\n") );
}

} // End if ( FlagOn ( SL_RETURN_SINGLE_ENTRY, Data->Iopb->OperationFlags ) )


       } // End if ( CompareFileName( DirectoryBuffer2, &gFileData->Name ) )

*/
return FLT_POSTOP_FINISHED_PROCESSING;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
ProcessFileDirectoryInformation( IN OUT PFLT_CALLBACK_DATA Data )

{

DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "^^^^^^^^FileDirectoryInformation\n") );

return FLT_POSTOP_FINISHED_PROCESSING;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
ProcessFileFullDirectoryInformation( IN OUT PFLT_CALLBACK_DATA Data )

{

DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "^^^^^^^^FileFullDirectoryInformation\n") );

return FLT_POSTOP_FINISHED_PROCESSING;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
ProcessFileIdBothDirectoryInformation( IN OUT PFLT_CALLBACK_DATA Data )

{

DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "^^^^^^^^FileIdBothDirectoryInformation\n") );

return FLT_POSTOP_FINISHED_PROCESSING;
}


/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
ProcessFileIdFullDirectoryInformation( IN OUT PFLT_CALLBACK_DATA Data )

{

DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "^^^^^^^^FileIdFullDirectoryInformation\n") );

return FLT_POSTOP_FINISHED_PROCESSING;
}


/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
ProcessFileObjectIdInformation( IN OUT PFLT_CALLBACK_DATA Data )

{

DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "^^^^^^^^FileObjectIdInformation\n") );

return FLT_POSTOP_FINISHED_PROCESSING;
}


/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
ProcessFileReparsePointInformation( IN OUT PFLT_CALLBACK_DATA Data )

{

DBG_PRINT( DbgOutput, DBG_IRP_MJ_DIRECTORY, (PRINT_TAG_DIRECTORY "^^^^^^^^FileDReparsePointInformation\n") );

return FLT_POSTOP_FINISHED_PROCESSING;
}

#endif