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
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#ifndef __CONDITIONALS_H__
#define __CONDITIONALS_H__


/* CONDITIONAL COMPILE ############################################################# */
/* 
 defines cut down on checks/code space/memory usage - the memory usage can add up quick for large amounts
 of items that are to be filtered creating many instances of the _HIDDEN_DATA structure.
 Also cuts down on the checks if they are not needed inside PostDirectoryControl() which is a heavily
 called function.
 This allows versions to be compile that only filter on cretain class types, obsessive way to save memory.
 **** * ATLEAST ONE * **** of these ** MUST ** be defined 1, if not then why are we even bothering
*/


// Are we going to be compile for use with a user interface
#define ENABLE_USER_INTERFACE            1

// Conditional statements for compile support for IRP_MJ_XXX
#define FILTER_IRP_MJ_CREATE             1   // This is manditory if filtering IRP_MJ_CREATE
#define FILTER_IRP_MJ_DIRECTORY_CONTROL  1   // This is manditory if filtering IRP_MJ_DIRECTORY_CONTROL
#define FILTER_IRP_MJ_SET_INFORMATION    1   // This is manditory if filtering IRP_MJ_SET_INFORMATION

// If FILTER_IRP_MJ_DIRECTORY_CONTROL is set to 1 then any single or combination of the 3 below *MUST* be set to 1.
// If FILTER_IRP_MJ_DIRECTORY_CONTROL is set to 0 then the 3 below are *IGNORED*
#define FILTER_BY_NAME_INFORMATION    1
#define FILTER_BY_TIME                0
#define FILTER_BY_ATTRIBUTES          0

// If FILTER_BY_NAME_INFORMATION is set to 1 then any single or combination of the 5 below *MUST* be set to 1.
// If FILTER_BY_NAME_INFORMATION is set to 0 the 5 below are *IGNORED*
#define FILTER_BY_VOLUME           1
#define FILTER_BY_DIRECTORY        1
#define FILTER_BY_SHARE            1
#define FILTER_BY_STREAM           1
#define FILTER_BY_NAME             1
#define FILTER_BY_EXTENSION        0


#endif // End #ifndef __CONDITIONALS_H__