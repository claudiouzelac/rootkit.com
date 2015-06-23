/******************************************************************************
  Author		: Kdm (Kodmaker@syshell.org)
  WebSite		: http://www.syshell.org

  Copyright (C) 2003,2004 Kdm
  *****************************************************************************
  This file is part of NtIllusion.

  NtIllusion is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  NtIllusion is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NtIllusion; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  ******************************************************************************/

#ifndef _PEB_HEADER
#define _PEB_HEADER

#define PEB_ADDRESS						0x7ffdf000
#define PEB_LDR_DATA_OFFSET				0x0C    // RVA to _PEB_LDR_DATA (ProcessModuleInfo)

#define LDR_DATA_IMAGE_BASE				0x18	// MODULE_ITEM.ImageBase
#define LDR_DATA_PATHFILENAME_OFFSET	0x24	// MODULE_ITEM.PathFileName
#define LDR_DATA_FILENAME_OFFSET		0x2C	// MODULE_ITEM.FileName


#pragma pack(4)
typedef struct _UNICODE_STRING 
{
	USHORT  Length;
	USHORT  MaximumLength;
	PWSTR  Buffer;
	} UNICODE_STRING, *PUNICODE_STRING;
#pragma pack()
	

typedef struct _PEB {
	/*000*/ BOOLEAN              InheritedAddressSpace;
	/*001*/ BOOLEAN              ReadImageFileExecOptions;
	/*002*/ BOOLEAN              BeingDebugged;
	/*003*/ BOOL                 SpareBool; // alloc size
	/*004*/ HANDLE               Mutant;
	/*008*/ PVOID                SectionBaseAddress;
	/*00C   PPROCESS_MODULE_INFO*/PVOID ProcessModuleInfo;
	/*010   PPROCESS_PARAMETERS*/ PVOID ProcessParameters;
	/*...*/
/*1E8*/ } PEB, *PPEB;

// At PEB+0x0C (PEB+PEB_LDR_DATA_OFFSET) : PEB_LDR_DATA (ProcessModuleInfo)
#pragma pack(4)
typedef struct _PEB_LDR_DATA {
	/*000*/  ULONG Length;
	/*004*/  BOOLEAN Initialized;
	/*008*/  PVOID SsHandle;
	/*00C*/  LIST_ENTRY ModuleListLoadOrder;
	/*014*/  LIST_ENTRY ModuleListMemoryOrder;
	/*018*/  LIST_ENTRY ModuleListInitOrder;
	/*020*/ } PEB_LDR_DATA, *PPEB_LDR_DATA;
#pragma pack()
    
// At PEB_LDR_DATA->ModuleListLoadOrder
typedef struct _MODULE_ITEM {
	/*000*/ LIST_ENTRY     ModuleListLoadOrder;
	/*008*/ LIST_ENTRY     ModuleListMemoryOrder;
	/*010*/ LIST_ENTRY     ModuleListInitOrder;
	/*018*/ DWORD          ImageBase;
	/*01C*/ DWORD          EntryPoint;
	/*020*/ DWORD          ImageSize;
	/*024*/ UNICODE_STRING PathFileName;
	/*02C*/ UNICODE_STRING FileName;
	/*034*/ ULONG          ModuleFlags;
	/*038*/ WORD           LoadCount;
	/*03A*/ WORD           Fill;
	/*03C*/ DWORD          dw3c;
	/*040*/ DWORD          dw40;
	/*044*/ DWORD          TimeDateStamp;
	/*048*/ } MODULE_ITEM, *PMODULE_ITEM;

// Winnt.h :
/*typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;
*/


#endif
