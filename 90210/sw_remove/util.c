/*++
    Copyright  (c) 2002 Sten
    Contact information:
        mail: stenri@mail.ru

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 
Module Name:
    util.cpp

Abstract: Some helper commands. 

Revision History:

 Sten        05/06/2002
      Initial release

--*/

#include "util.h"

////////////////////////////////////////////////////////////////////////////
//
// Hooks interrupt via IDT patching
//
// Arguments:
//     IntNo - interrupt number to hook
//     Ptr   - pointer to interrupt function
// Return Value:
//     STATUS_SUCCESS - ?
//
////////////////////////////////////////////////////////////////////////////

void* SetInterruptHandler(int IntNo, void *HandlerPtr, BYTE flags)
{

   __asm
   {
          pushad

          mov    eax, IntNo
          mov    dl,  flags
          mov    edi, HandlerPtr 
          mov    bx,  cs

          cli                       ; TO DO: StopSecondCPUs()
          call   mp_HookInterrupt
          sti

          popad
   }

   return (void*)mp_OldHandler;
}

////////////////////////////////////////////////////////////////////////////
//
// Get interrupt vector via IDT
//
// Arguments:
//     IntNo - interrupt number
// Return Value:
//     Ptr to interrupt handler, -1 if error.
//
////////////////////////////////////////////////////////////////////////////

void* GetInterruptHandler(DWORD IntNo, DWORD IdtNum)
{
   DWORD IDTBase=0;
   DWORD OldIntr;
   WORD  *IdtEntry;

   IDTBase = mp_GetIDTBase(IdtNum);

   IdtEntry=(WORD*)(IDTBase+IntNo*8);

   OldIntr=IdtEntry[0]+(IdtEntry[3]<<16);

   return (void*)OldIntr;  
}

////////////////////////////////////////////////////////////////////////////
//
// GetPde 
//     Returns pointer to PDE for the given page
//     See MiGetPdeAddress in HAL.DLL
//
////////////////////////////////////////////////////////////////////////////

DWORD* GetPde(void *Address)
{
   return (DWORD*)(MmSystemPteBase + ( ((DWORD)Address>>20) & 0xFFC) + ((DWORD)MmSystemPteBase>>10));
} 

////////////////////////////////////////////////////////////////////////////
//
// GetPte 
//     Returns pointer to PTE entry for the given address.
//     If this is a 4Mbyte page - return PDE
//
////////////////////////////////////////////////////////////////////////////

DWORD* GetPte(void *Address)
{
   DWORD *p=GetPde(Address);

   if ((*p&0x81) == 0x81)  // PDE is 4MByte && present
            return p;

   return (DWORD*)(MmSystemPteBase + ( ((DWORD)Address>>10) & 0x3FFFFC) );
}

