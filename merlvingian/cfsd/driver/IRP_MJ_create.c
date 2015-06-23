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

#include "IRP_MJ_create.h"

#if FILTER_IRP_MJ_CREATE
/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_PREOP_CALLBACK_STATUS
PreCreate( IN OUT PFLT_CALLBACK_DATA Data,
           IN PCFLT_RELATED_OBJECTS FltObjects,
           OUT PVOID *CompletionContext )

{

 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PreCreate() CALLED\n") );

 return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
FLT_POSTOP_CALLBACK_STATUS
PostCreate( IN OUT PFLT_CALLBACK_DATA Data,
            IN PCFLT_RELATED_OBJECTS FltObjects,
            IN PVOID CompletionContext,
            IN FLT_POST_OPERATION_FLAGS Flags )

{

PFLT_FILE_NAME_INFORMATION    FNameInfo;
NTSTATUS                      Status;
BOOLEAN                       FailedMatch = TRUE;
FILE_DISPOSITION_INFORMATION  FDI;


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

//////////////////////////////////////


try {


#if FILTER_BY_VOLUME
if ( gProtectedData->NameInfo.VolumeName.Buffer != NULL )
{
   if ( FsRtlIsNameInExpression( &gProtectedData->NameInfo.VolumeName,
                                 &FNameInfo->Volume,
                                 TRUE,
                                 NULL ) )
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Volume # NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.VolumeName, &FNameInfo->Volume ) );
    FailedMatch = FALSE;
   }
   else
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Volume *NO NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.VolumeName, &FNameInfo->Volume ) );
   
    FailedMatch = TRUE;
    leave;
   }
}
#endif


#if FILTER_BY_DIRECTORY
if ( gProtectedData->NameInfo.DirectoryName.Buffer != NULL )
{
   if ( FsRtlIsNameInExpression( &gProtectedData->NameInfo.DirectoryName,
                                 &FNameInfo->ParentDir,
                                 TRUE,
                                 NULL ) )
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Directory # NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.DirectoryName, &FNameInfo->ParentDir) );
    FailedMatch = FALSE;
   }
   else
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Directory *NO NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.DirectoryName, &FNameInfo->ParentDir) );
   
    FailedMatch = TRUE;
    leave;
   }
}
#endif


#if FILTER_BY_SHARE
if ( gProtectedData->NameInfo.ShareName.Buffer != NULL )
{
   if ( FsRtlIsNameInExpression( &gProtectedData->NameInfo.ShareName,
                                 &FNameInfo->Share,
                                 TRUE,
                                 NULL ) )
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Share # NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.ShareName, &FNameInfo->Share ) );
    FailedMatch = FALSE;
   }
   else
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Share *NO NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.ShareName, &FNameInfo->Share ) );
   
    FailedMatch = TRUE;
    leave;
   }
}
#endif


#if FILTER_BY_STREAM
if ( gProtectedData->NameInfo.StreamName.Buffer != NULL )
{
   if ( FsRtlIsNameInExpression( &gProtectedData->NameInfo.StreamName,
                                 &FNameInfo->Stream,
                                 TRUE,
                                 NULL ) )
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Stream # NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.StreamName, &FNameInfo->Stream ) );
    FailedMatch = FALSE;
   }
   else
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Stream *NO NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.StreamName, &FNameInfo->Stream ) );
   
    FailedMatch = TRUE;
    leave;
   }
}
#endif


#if FILTER_BY_NAME
if ( gProtectedData->NameInfo.Name.Buffer != NULL )
{
   if ( FsRtlIsNameInExpression( &gProtectedData->NameInfo.Name,
                                 &FNameInfo->FinalComponent,
                                 TRUE,
                                 NULL ) )
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Name # NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.Name, &FNameInfo->FinalComponent) );
    FailedMatch = FALSE;
   }
   else
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Name *NO NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.Name, &FNameInfo->FinalComponent) );
   
    FailedMatch = TRUE;
    leave;
   }
}
#endif


#if FILTER_BY_EXTENSION
if ( gProtectedData->NameInfo.Extension.Buffer != NULL )
{
   if ( FsRtlIsNameInExpression( &gProtectedData->NameInfo.Extension,
                                 &FNameInfo->Extension,
                                 TRUE,
                                 NULL ) )
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Extension # NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.Extension, &FNameInfo->Extension ) );
    FailedMatch = FALSE;
   }
   else
   {
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "PostCreate()-Extension *NO NAME MATCH [%wZ] (%wZ)\n",&gProtectedData->NameInfo.Extension, &FNameInfo->Extension ) );
   
    FailedMatch = TRUE;
    leave;
   }
}
#endif

}
finally
{


  if ( !FailedMatch )
  {




if ( FltObjects->FileObject->WriteAccess ) 
{
 KdPrint( ("WriteAccess\n") );

  if ( Data->IoStatus.Information == FILE_CREATED )
  {
   FDI.DeleteFile = TRUE;

   FltSetInformationFile( FltObjects->Instance,
                          FltObjects->FileObject,
                          &FDI,
                          sizeof( FILE_DISPOSITION_INFORMATION ),
                          FileDispositionInformation );
  }

}
else if ( FltObjects->FileObject->ReadAccess ) 
{
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "ReadAccess\n") );
}
else if ( FltObjects->FileObject->DeleteAccess ) 
{
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "DeleteAccess\n") );
}
else if ( FltObjects->FileObject->SharedWrite ) 
{
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "SharedWrite\n") );
}
else if ( FltObjects->FileObject->SharedRead) 
{
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "SharedRead\n") );
}
else if ( FltObjects->FileObject->SharedDelete ) 
{
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "SharedDelete\n") );
}


    FltCancelFileOpen( FltObjects->Instance, 
                       FltObjects->FileObject );

    Data->IoStatus.Status = STATUS_ACCESS_DENIED;
    Data->IoStatus.Information = 0;
 
 DBG_PRINT( DbgOutput, DBG_IRP_MJ_CREATE, (PRINT_TAG_CREATE "REFUSED IRP_MJ_CREATE (%wZ)", &FNameInfo->Name) );


  } // End if ( !FailedMatch )


} // End Finally



//////////////////////////////////






    // Free
    FltReleaseFileNameInformation( FNameInfo );

 return FLT_POSTOP_FINISHED_PROCESSING;
}

#endif // End #if FILTER_IRP_MJ_CREATE