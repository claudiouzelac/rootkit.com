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
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#ifndef __CROSSOVER_H__
#define __CROSSOVER_H__


#define USER_COMMUNICATION_PORT_NAME     L"\\ComServerPort"










// Enum list of possible user mode commands
typedef enum _USER_MODE_COMMANDS {

    ucSetFileName,
    ucSetFileTime,
    ucSetFileAttributes,

    ucGetFileName,
    ucGetFileTime,
    ucGetFileAttributes,

    ucDisplayList

} USER_MODE_COMMANDS;



typedef struct _COMMAND_BLOCK {

    USER_MODE_COMMANDS    UserCommand;

    UCHAR                 UserBuffer[];

} COMMAND_BLOCK, *PCOMMAND_BLOCK;


#endif