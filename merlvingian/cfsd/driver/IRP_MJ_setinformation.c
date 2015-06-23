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

#include "IRP_MJ_setinformation.h"

#if FILTER_IRP_MJ_SET_INFORMATION

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_PREOP_CALLBACK_STATUS
PreSetInformation( IN OUT PFLT_CALLBACK_DATA Data,
                   IN PCFLT_RELATED_OBJECTS FltObjects,
                   OUT PVOID *CompletionContext )

{

 return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
PostSetInformation( IN OUT PFLT_CALLBACK_DATA Data,
                    IN PCFLT_RELATED_OBJECTS FltObjects,
                    IN PVOID CompletionContext,
                    IN FLT_POST_OPERATION_FLAGS Flags )

{

PFLT_FILE_NAME_INFORMATION    FNameInfo;
NTSTATUS                      Status;


FILE_RENAME_INFORMATION       FRI;


    // If we have an error then just exit
    if ( !NT_SUCCESS( Data->IoStatus.Status ) || ( STATUS_REPARSE == Data->IoStatus.Status ) ) 
    {
     return FLT_POSTOP_FINISHED_PROCESSING;
    }

    Status = FltGetFileNameInformation( Data,
                                        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
                                        &FNameInfo );

    // If we could not get the name information then exit
    if (!NT_SUCCESS( Status )) 
    {
     return FLT_POSTOP_FINISHED_PROCESSING;
    }

    FltParseFileNameInformation( FNameInfo );


    switch ( Data->Iopb->Parameters.SetFileInformation.FileInformationClass )
    {
     case FileRenameInformation:
         {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_SET_INFORMATION, (PRINT_TAG_SETINFO "PostSetInfo()-RENAME [%wZ] \n", &FNameInfo->Name ) );
          break;
         }
     case FileAllocationInformation:
         {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_SET_INFORMATION, (PRINT_TAG_SETINFO "PostSetInfo()-ALLOCATION [%wZ] \n", &FNameInfo->Name ) );
          break;
         }
     case FileBasicInformation:
         {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_SET_INFORMATION, (PRINT_TAG_SETINFO "PostSetInfo()-BASIC[%wZ] \n", &FNameInfo->Name ) );
          break;
         }
     case FileDispositionInformation:
         {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_SET_INFORMATION, (PRINT_TAG_SETINFO "PostSetInfo()-DISPOSITION [%wZ] \n", &FNameInfo->Name ) );
          break;
         }
     case FileEndOfFileInformation:
         {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_SET_INFORMATION, (PRINT_TAG_SETINFO "PostSetInfo()-EOF [%wZ] \n", &FNameInfo->Name ) );
          break;
         }
     case FileLinkInformation:
         {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_SET_INFORMATION, (PRINT_TAG_SETINFO "PostSetInfo()-LINK [%wZ] \n", &FNameInfo->Name ) );
          break;
         }
     case FilePositionInformation:
         {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_SET_INFORMATION, (PRINT_TAG_SETINFO "PostSetInfo()-POSITION [%wZ] \n", &FNameInfo->Name ) );
          break;
         }
     case FileValidDataLengthInformation:
         {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_SET_INFORMATION, (PRINT_TAG_SETINFO "PostSetInfo()-VALIDATEDATALENGTH [%wZ] \n", &FNameInfo->Name ) );
          break;
         }
     default :
            {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_SET_INFORMATION, (PRINT_TAG_SETINFO "BAD MOJO\n") );
             break;
            }
 
    } // End switch ( Data->Iopb->Parameters.SetFileInformation.FileInformationClass )


    // Free
    FltReleaseFileNameInformation( FNameInfo );


 return FLT_POSTOP_FINISHED_PROCESSING;
}







#endif