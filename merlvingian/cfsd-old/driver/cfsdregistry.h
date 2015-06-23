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
 *   General header file.
 *
 *
 *   REFERENCE for this code :
 *    swapbuffers.c - DDK
 *    OSR ListServer Discussion Groups - http://www.osronline.com/page.cfm?name=search
 *
 */

#ifndef __CFSDREGISTRY_H__
#define __CFSDREGISTRY_H__

#include <fltKernel.h>
#include "cfsdfilter.h"

/* DEFINES ######################################################################### */
#define POOL_TAG_REGISTRY_HFILE_KEY     'geRk'
#define POOL_TAG_REGISTRY_HFILE         'geRf'
#define POOL_TAG_REGISTRY_ATTACHMETHOD  'geRa'



#define REGKEY_ATTACHMETHODS         L"AttachMethods"
#define REGKEY_VOLUMEDEVICETYPES     L"VolumeDeviceTypes"
#define REGKEY_FILESYSTEMS           L"FileSystems"


typedef enum _REG_KEY_TYPES {

  AttachMethods,
  VolumeDeviceTypes,
  FileSystems

} REG_KEY_TYPES;


/* PROTOTYPES ###################################################################### */
BOOLEAN 
EnumerateRegistryValues( PUNICODE_STRING  RegistryPath );

#endif