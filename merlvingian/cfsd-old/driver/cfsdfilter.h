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
 *   General header file - Compare functions are a conditional compile
 *
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#ifndef __CFSDFILTER_H__
#define __CFSDFILTER_H__

#include "cfsdcommon.h"


/* DEFINES ######################################################################## */
#define POOL_TAG_TEMPORARY_NAME     'NpmT'


// Mask types for file time checks - TMaskSet
#if FILTER_BY_TIME
#define MASK_CREATION_TIME      0x01
#define MASK_LAST_ACCESS_TIME   0x02
#define MASK_LAST_WRITE_TIME    0x04
#define MASK_CHANGE_TIME        0x06

// Compare time values
#define COMPARE_TIME_LESS_THAN       1
#define COMPARE_TIME_EQUAL           2 
#define COMPARE_TIME_GREATER_THAN    3
#endif

// Compare match options
#if FILTER_BY_ATTRIBUTES
#define COMPARE_MATCH_PASSTHROUGH    0
#define COMPARE_MATCH_ANY            1
#define COMPARE_MATCH_ALL_EXACT      2
#define COMPARE_MATCH_ALL_PARTIAL    3
#endif


/* PROTOTYPES ###################################################################### */
#if FILTER_BY_NAME
BOOLEAN
CompareFileName( WCHAR FName[1], 
                 ULONG FLength, 
                 PUNICODE_STRING MatchName );

#endif // End FILTER_BY_NAME

#if FILTER_BY_ATTRIBUTES
BOOLEAN
CompareFileAttributes( ULONG Attributes, 
                       ULONG MatchAttributes, 
                       UCHAR MType );

#endif // End FILTER_BY_ATTRIBUTES

#if FILTER_BY_TIME
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
                 LARGE_INTEGER MatchChangeTime );
#endif // End FILTER_BY_TIME


#endif