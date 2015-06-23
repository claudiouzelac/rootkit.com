/******************************************************************************
  kdbg_IAT.c	: provides functions to dump IAT of a module
  *****************************************************************************
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

#include <process.h>
#include <windows.h>
#include <SHLWAPI.H>
#include "kdbg_IAT.h"

#include "kNTILib.h"		// Internal runtime
#include "../Engine/Hijacking/kHijackEng.h"

#define printf OutputString

//		OutputString : Send a formatted string to debugger output
//		todo : handle compatibility with Win 9x version
void _OutputString(char* frmstr,...) {
 char buf[1024];
 va_list vargs;
 va_start(vargs, frmstr);
 wvsprintfA(buf, frmstr, vargs); 
 va_end(vargs);
 OutputDebugString((char*)buf);
 return;
} 


// Check all imported functions for a given module (dwBase: base address of module)
// to see if the loader has done a good job (function address is correct)
int CheckAllImportedFunctionsForModule(DWORD dwBase)
{
    PIMAGE_DOS_HEADER pDOSHeader;
    PIMAGE_NT_HEADERS pNTHeaders;
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc;// Dll name chunk
    PIMAGE_THUNK_DATA pIAT;              // Functions address chunk
    PIMAGE_THUNK_DATA pINT;              // Functions names array
    PIMAGE_IMPORT_BY_NAME pImportName;   // Function Name 
    PIMAGE_THUNK_DATA pIteratingIAT;
    char* DllName=NULL;
    DWORD IAT_funcAddr=0;                // fonction entry point in IAT
    DWORD GPR_funcAddr=0;                // function entry point with GetProcAddress
    unsigned int cFuncs=0;
    int i=0;
    
    
    // Init
    pDOSHeader = (PIMAGE_DOS_HEADER) dwBase;
    // Check dos header signature
    if(pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
    { 
        OutputString("Wrong IMAGE_DOS_SIGNATURE\n") ;
        return 0;
    }
    
    pNTHeaders = MakePtr(PIMAGE_NT_HEADERS, pDOSHeader, pDOSHeader->e_lfanew);
    if(pNTHeaders->Signature != IMAGE_NT_SIGNATURE)
    { 
        OutputString("Wrong IMAGE_NT_SIGNATURE\n") ;
        return 0;
    }
    
    // Localize Import table
    pImportDesc = MakePtr(PIMAGE_IMPORT_DESCRIPTOR, dwBase, pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    if(pImportDesc == (PIMAGE_IMPORT_DESCRIPTOR)pNTHeaders) 
    {
        OutputString("Unable to locate import directory\n") ;
        return 0;
    }    
  
    OutputString("-------------------------------\n");
    // Dump imported DLLs names "winnt.h"
    while(pImportDesc->Name) 
	{    
	    
	    DllName = MakePtr(char*, dwBase, pImportDesc->Name);
	    OutputString("> %s\n", DllName);
	    
	    pINT = MakePtr(PIMAGE_THUNK_DATA, dwBase, pImportDesc->OriginalFirstThunk );
	    pIAT = MakePtr(PIMAGE_THUNK_DATA, dwBase, pImportDesc->FirstThunk );
	    cFuncs = 0;
	    
	    // Count how many entries there are in this IAT.  Array is 0 terminated
	    pIteratingIAT = pIAT;  
	    while ( pIteratingIAT->u1.Function )
	    {
             cFuncs++;
             pIteratingIAT++;
        }
    
	    if ( cFuncs == 0 ) 
		   continue;
    
    
	    // Scan through the IAT
	    pIteratingIAT = pIAT;	        
        while ( pIteratingIAT->u1.Function )
        {
            if ( !IMAGE_SNAP_BY_ORDINAL( pINT->u1.Ordinal ) )  // import by name
            {
                pImportName = MakePtr(PIMAGE_IMPORT_BY_NAME, dwBase, pINT->u1.AddressOfData);
                OutputString("  %s ",((char*)pImportName->Name));
                
//                for(i=0; i<(35 - strlen(((char*)pImportName->Name))); i++)
//                  OutputString(" ");
                
                IAT_funcAddr = ((DWORD)(pIteratingIAT->u1.Function));
                GPR_funcAddr = (DWORD) GetProcAddress(GetModuleHandle(DllName),
                ((char*)pImportName->Name) );
                // Show Address in IAT
                OutputString("\t 0x%X", IAT_funcAddr);

                // Show resolved address (using GetProcAddress)
                OutputString("\t 0x%X", GPR_funcAddr);
                
                if(IAT_funcAddr!=GPR_funcAddr)
                 OutputString(" <!>");
                OutputString("\n");

            }    
            
            pIteratingIAT++; // jump to next IAT entry
            pINT++;          // jump to next INT entry
        }

		pImportDesc++;  // jump to next dll
		printf("\n");
	}
    
    OutputString("-------------------------------\n");
    
    return 1;
}  


int DumpPartOfIAT(PIMAGE_IMPORT_DESCRIPTOR pImportDesc, PVOID pBaseLoadAddr)
{
	PIMAGE_THUNK_DATA pIAT;     // Ptr to import address table
    PIMAGE_THUNK_DATA pINT;     // Ptr to import names table
    PIMAGE_THUNK_DATA pIteratingIAT;
	PIMAGE_IMPORT_BY_NAME pImportName;
	unsigned cFuncs = 0;

	// If no import names table, we can't redirect this, so leave
    if ( pImportDesc->OriginalFirstThunk == 0 )
	{
        printf("No import.\n");
		return 0;
	}

    pIAT = MakePtr( PIMAGE_THUNK_DATA, pBaseLoadAddr, pImportDesc->FirstThunk );
    pINT = MakePtr( PIMAGE_THUNK_DATA, pBaseLoadAddr, pImportDesc->OriginalFirstThunk );

	// Count how many entries there are in this IAT.  Array is 0 terminated
    pIteratingIAT = pIAT;  
    while ( pIteratingIAT->u1.Function )
    {
        cFuncs++;
        pIteratingIAT++;
    }
    
	if ( cFuncs == 0 ) 
		return 0;
    
	// Scan through the IAT
    pIteratingIAT = pIAT;

    while ( pIteratingIAT->u1.Function )
    {
        if ( !IMAGE_SNAP_BY_ORDINAL( pINT->u1.Ordinal ) )  // import by name
        {
            pImportName = MakePtr( PIMAGE_IMPORT_BY_NAME, pBaseLoadAddr, pINT->u1.AddressOfData );
            printf("%s\n",((char*)pImportName->Name));
        }
	    pIteratingIAT++;    // Advance to next IAT entry
        pINT++;             // Advance to next INT entry
    }

	return 0;
}


void DumpIATOfModule(char* ModuleName)
{
	HMODULE hModEXE;
	PIMAGE_NT_HEADERS pNTHeader = 0;
	DWORD importRVA;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
	PIMAGE_IMPORT_DESCRIPTOR g_pFirstImportDesc;
	char* pszImportModuleName;
	

	printf("Dumping IAT of %s:\n", ModuleName);
	printf("_____________________________\n");

	hModEXE = (HMODULE) LoadLibrary(ModuleName);
	if(hModEXE==NULL)
	{
		printf("Cannot load module.\n");
		return;
	}

	if (((PIMAGE_DOS_HEADER)hModEXE)->e_magic != IMAGE_DOS_SIGNATURE )
	{
		printf("Wrong IMAGE_DOS_SIGNATURE\n");
		return;
	}

	pNTHeader =
		(PIMAGE_NT_HEADERS)
		(
			( (PBYTE)hModEXE  )
			+
			(((PIMAGE_DOS_HEADER)(hModEXE))->e_lfanew)
		);
	
	if ( pNTHeader->Signature != IMAGE_NT_SIGNATURE )
	{
		printf("Wrong IMAGE_NT_SIGNATURE\n");
		return;
	}
	
	importRVA = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	if ( !importRVA )
	{
		printf("Unable to locate import directory.\n");
		return;
	}

	pImportDesc = MakePtr( PIMAGE_IMPORT_DESCRIPTOR, hModEXE, importRVA );

	// Save imports address for later use
    g_pFirstImportDesc = pImportDesc;   

	// Iterate through each import descriptor, and redirect if appropriate
    while ( pImportDesc->FirstThunk )
	{
		pszImportModuleName = MakePtr( PSTR, hModEXE, pImportDesc->Name);
        printf("= %s relies on %s for: \n", ModuleName, pszImportModuleName);
		DumpPartOfIAT(pImportDesc, (PVOID)hModEXE);
		printf("_____________________________\n");
        pImportDesc++;  // Advance to next import descriptor
    }

	FreeLibrary(hModEXE);
	printf("End of IAT dump of %s.\n", ModuleName);
	printf("_____________________________\n");
}
