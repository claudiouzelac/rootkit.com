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
 *   Calls for processing data matches
 *
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#include "CompareFilters.h"


/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
#if FILTER_BY_NAME
BOOLEAN
CompareFileName( WCHAR FName[1], 
                 ULONG FLength, 
                 NAME_INFORMATION_DATA MatchName,
                 PFLT_FILE_NAME_INFORMATION pNameInfo )
{

UNICODE_STRING FileName;

/*
KdPrint( ("Volume (%wZ)\n",&pNameInfo->Volume) );
KdPrint( ("Share (%wZ)\n",pNameInfo->Share ) );
KdPrint( ("Directory (%wZ)\n",&pNameInfo->ParentDir ) );
KdPrint( ("Stream (%wZ)\n",&pNameInfo->Stream) );
*/

   FileName.Length        = (USHORT) FLength;
   FileName.MaximumLength = (USHORT) FLength + sizeof( WCHAR );
   FileName.Buffer        = ExAllocatePoolWithTag( NonPagedPool,
                                                   FileName.MaximumLength,
                                                   POOL_TAG_TEMPORARY_NAME );

   // Zero it out agian out of paranoia
   RtlZeroMemory( FileName.Buffer, FileName.MaximumLength );

   RtlCopyMemory( FileName.Buffer,FName, FLength);

   // *NOTE* This compare must convert the matchname to UPPER CASE since case insensative is true
   if ( FsRtlIsNameInExpression( &MatchName.Name,
                                 &FileName,
                                 TRUE,
                                 NULL ) )
   {
 DBG_PRINT( DbgOutput, DBG_COMPARE_NAME, (PRINT_TAG_CNAME "# MATCH (%wZ) [%wZ]", &FileName, &MatchName.Name ) );

    ExFreePoolWithTag( FileName.Buffer, POOL_TAG_TEMPORARY_NAME );

    return TRUE;
   }

 DBG_PRINT( DbgOutput, DBG_COMPARE_NAME, (PRINT_TAG_CNAME "* NO-MATCH (%wZ) [%wZ]", &MatchName.Name, &FileName ) );

   ExFreePoolWithTag( FileName.Buffer, POOL_TAG_TEMPORARY_NAME );

 return FALSE;
}
#endif // End #if FILTER_BY_NAME

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
#if FILTER_BY_ATTRIBUTES
BOOLEAN
CompareFileAttributes( ULONG Attributes, 
                       ULONG MatchAttributes, 
                       UCHAR MType )
{

#if DBG
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "-- [FILE ATTRIBUTES] --\n") );
 if ( FlagOn( Attributes, FILE_ATTRIBUTE_DIRECTORY ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_DIRECTORY\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_READONLY ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_READONLY\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_HIDDEN ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_HIDDEN\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_SYSTEM ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_SYSTEM\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_ARCHIVE ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_ARCHIVE\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_NORMAL ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_NORMAL\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_TEMPORARY ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_TEMPORARY\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_SPARSE_FILE ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_SPARSE_FILE\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_REPARSE_POINT ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_REPARSE_POINT\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_COMPRESSED ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_COMPRESSED\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_OFFLINE ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_OFFLINE\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_ENCRYPTED ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_ENCRYPTED\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_DIRECTORY ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_DIRECTORY\n") );
 }

 if ( FlagOn( Attributes, FILE_ATTRIBUTE_DEVICE ) )
 {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "FILE_ATTRIBUTE_DEVICE\n") );
 }
#endif // End #if DBG


  switch ( MType )
  {
   case COMPARE_MATCH_PASSTHROUGH  : // No mask type defined
        {
         // If we have compiled in support for file attributes compares but do not use them on this
         // paticular compare we need to pass back success as to not fail any of the other compares
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "# CompareFileAttributes() NO CHECK - PASSTHROUGH\n") );
         return TRUE;
         break;
        }
   case COMPARE_MATCH_ANY : // ANY flag in mask matches 
       {
        if ( FlagOn( Attributes, MatchAttributes ) ) 
		{
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "# MATCH [0x%x] (0x%x) COMPARE_MATCH_ANY\n", Attributes, MatchAttributes ) );
		 return TRUE;
		}

 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "*NO MATCH [0x%x] (0x%x) COMPARE_MATCH_ANY\n", Attributes, MatchAttributes ) );

		break;
       }
   case COMPARE_MATCH_ALL_EXACT : // ALL the flags in mask match NO EXTRA FLAGS - EXACT MATCH
       {
        if ( Attributes == MatchAttributes ) 
		{
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "# MATCH [0x%x] (0x%x) COMPARE_MATCH_ALL_EXACT\n", Attributes, MatchAttributes ) );
	     return TRUE;
		}

 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "*NO MATCH [0x%x] (0x%x) COMPARE_MATCH_ALL_EXACT\n", Attributes, MatchAttributes ) );

		break;
	   }
   case COMPARE_MATCH_ALL_PARTIAL : // ALL the flags in mask must match but can have extra flags - PARTIAL MATCH
       {

        if ( ( Attributes & MatchAttributes ) == MatchAttributes ) 
	    {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "# MATCH [0x%x] (0x%x) COMPARE_MATCH_ALL_PARTIAL\n", Attributes, MatchAttributes ) );
		 return TRUE;
	    }

 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "*NO MATCH [0x%x] (0x%x) COMPARE_MATCH_ALL_PARTIAL\n", Attributes, MatchAttributes ) );

		break;
	   }
   default : // Fall through for code safety
	      {
 DBG_PRINT( DbgOutput, DBG_COMPARE_ATTRIBUTES, (PRINT_TAG_CATTRIBUTES "BAD MOJO\n") );
	        break;
	      }
  } // End switch ( MType )	  


 // Default to Failure with no matches
 return FALSE;
}
#endif // End #if FILTER_BY_ATTRIBUTES

/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
#if FILTER_BY_TIME
BOOLEAN
CompareThisTime( UCHAR TMS, LARGE_INTEGER Value, LARGE_INTEGER MatchValue )
{

  switch ( TMS )
  {
  case COMPARE_TIME_EQUAL : // EXACT MATCH
      {
       if ( ( Value.LowPart == MatchValue.LowPart ) &&
            ( Value.HighPart == MatchValue.HighPart ) )
       {
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "# MATCH EQUALL [L:%d] [H:%d] (L:%d) (H:%d)\n",Value.LowPart, Value.HighPart, MatchValue.LowPart, MatchValue.HighPart ) );
        return TRUE;
       }

 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "* NO MATCH EQUALL [L:%d] [H:%d] (L:%d) (H:%d)\n",Value.LowPart, Value.HighPart, MatchValue.LowPart, MatchValue.HighPart ) );

       break;
      }
  case COMPARE_TIME_LESS_THAN : // LESS THAN
      {
       if ( ( Value.LowPart < MatchValue.LowPart ) &&
            ( Value.HighPart < MatchValue.HighPart ) )
       {
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "# MATCH LESS THAN [L:%d] [H:%d] (L:%d) (H:%d)\n",Value.LowPart, Value.HighPart, MatchValue.LowPart, MatchValue.HighPart ) );
        return TRUE;
       }

 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "* NO MATCH LESS THAN [L:%d] [H:%d] (L:%d) (H:%d)\n",Value.LowPart, Value.HighPart, MatchValue.LowPart, MatchValue.HighPart ) );
       break;
      }
  case COMPARE_TIME_GREATER_THAN : // GREATER THAN
      {
       if ( ( Value.LowPart > MatchValue.LowPart ) &&
            ( Value.HighPart > MatchValue.HighPart ) )
       {
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "# MATCH GREATER THAN [L:%d] [H:%d] (L:%d) (H:%d)\n",Value.LowPart, Value.HighPart, MatchValue.LowPart, MatchValue.HighPart ) );
        return TRUE;
       }
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "* NO MATCH GREATER THAN [L:%d] [H:%d] (L:%d) (H:%d)\n",Value.LowPart, Value.HighPart, MatchValue.LowPart, MatchValue.HighPart ) );

       break;
      }
  default :
         {
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "BAD MOJO\n") );
          break;
         }
  } // End switch ( TMS )

 return FALSE;
}
/* 
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 *
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= **
 */
BOOLEAN
CompareFileTime( UCHAR TMaskSet,
                 UCHAR TMaskType,
                 LARGE_INTEGER CreationTime,
                 LARGE_INTEGER LastAccessTime,
                 LARGE_INTEGER LastWriteTime,
                 LARGE_INTEGER ChangeTime,
                 LARGE_INTEGER MatchCreationTime,
                 LARGE_INTEGER MatchLastAccessTime,
                 LARGE_INTEGER MatchLastWriteTime,
                 LARGE_INTEGER MatchChangeTime )
{

 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "-- [FILE TIMES] --\n") );
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "CreationTime H:%d L:%d\n",CreationTime.HighPart,CreationTime.LowPart) );
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "LastAccessTime H:%d L:%d\n",LastAccessTime.HighPart,LastAccessTime.LowPart) );
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "LastWriteTime H:%d L:%d\n",LastWriteTime.HighPart,LastWriteTime.LowPart) );
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "ChangeTime H:%d L:%d\n",ChangeTime.HighPart,ChangeTime.LowPart) );


   if ( (TMaskSet == 0) || ( TMaskType == 0 ) )
   {
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "# CompareFileTime() NO CHECK - PASSTHROUGH\n") );
    return TRUE;
   }

   if ( FlagOn( MASK_CREATION_TIME, TMaskSet ) )
   {
    if ( !CompareThisTime( TMaskType, CreationTime, MatchCreationTime ) )
    {
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "* NO MATCH CompareFileTime() MASK_CREATION_TIME\n") );
     return FALSE;
    }
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "# MATCH CompareFileTime() MASK_CREATION_TIME\n") );

   }

   if ( FlagOn( MASK_LAST_ACCESS_TIME, TMaskSet ) )
   {
    if ( !CompareThisTime( TMaskType, LastAccessTime, MatchLastAccessTime ) )
    {
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "* NO MATCH CompareFileTime() MASK_LAST_ACCESS_TIME\n") );
     return FALSE;
    }
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "# MATCH CompareFileTime() MASK_LAST_ACCESS_TIME\n") );

   }

   if ( FlagOn( MASK_LAST_WRITE_TIME, TMaskSet ) )
   {
    if ( !CompareThisTime( TMaskType, LastWriteTime, MatchLastWriteTime ) )
    {
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "* NO MATCH CompareFileTime() MASK_LAST_WRITE_TIME\n") );
     return FALSE;
    }
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "# MATCH CompareFileTime() MASK_LAST_WRITE_TIME\n") );

   }

   if ( FlagOn( MASK_CHANGE_TIME, TMaskSet ) )
   {
    if ( !CompareThisTime( TMaskType, ChangeTime, MatchChangeTime ) )
    {
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "* NO MATCH CompareFileTime() MASK_CHANGE_TIME\n") );
     return FALSE;
    }
 DBG_PRINT( DbgOutput, DBG_COMPARE_TIME, (PRINT_TAG_CTIME "# MATCH CompareFileTime() MASK_CHANGE_TIME\n") );

   }

// Default Action here is if we did not encounter a failure at any point then succeded
 return TRUE;
}
#endif // End #if FILTER_BY_TIME