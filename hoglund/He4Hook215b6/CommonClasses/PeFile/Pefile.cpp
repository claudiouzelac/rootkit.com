#include "pefile.h"

DWORD NativeGetProcAddress(DWORD hModule, char *lpszFunctionName)
{
  DWORD                    dwFunctionAddress = 0;
  DWORD                    i;
  DWORD                   *pdwFunctionAddress = 0;
  WORD                    *pwOrdinals;
  char                   **pszName;
  PIMAGE_DOS_HEADER        pDOSHeader = (PIMAGE_DOS_HEADER) hModule;
  PIMAGE_NT_HEADERS32      pNtHeader = 0;
  PIMAGE_DATA_DIRECTORY    pDataDir = 0;
  PIMAGE_EXPORT_DIRECTORY  pExportDir = 0;
     
  if (!hModule || !lpszFunctionName)
    return dwFunctionAddress;

  if (!_MmIsAddressValid((PVOID)hModule)) 
  {
    DbgPrintPe(("He4HookInv: NativeGetProcAddress: hModule ERROR !!!\n"));
    return dwFunctionAddress;
  }
  if (!_MmIsAddressValid((PVOID)lpszFunctionName)) 
  {
    DbgPrintPe(("He4HookInv: NativeGetProcAddress: lpszFunctionName ERROR !!!\n"));
    return dwFunctionAddress;
  }

  if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
  {
    DbgPrintPe(("He4HookInv: NativeGetProcAddress: IMAGE_DOS_SIGNATURE ERROR !!!\n"));
    return dwFunctionAddress;
  }

  pNtHeader = (PIMAGE_NT_HEADERS32)(((char *)pDOSHeader) + pDOSHeader->e_lfanew);
  if (!_MmIsAddressValid((PVOID)pNtHeader)) 
  {
    DbgPrintPe(("He4HookInv: NativeGetProcAddress: PIMAGE_NT_HEADERS32 ERROR !!!\n"));
    return dwFunctionAddress;
  }
  if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
  {
    DbgPrintPe(("He4HookInv: NativeGetProcAddress: IMAGE_NT_SIGNATURE ERROR !!!\n"));
    return dwFunctionAddress;
  }
    
  pDataDir = &pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
  if (!_MmIsAddressValid(pDataDir))
  {
    DbgPrintPe(("He4HookInv: NativeGetProcAddress: IMAGE_DIRECTORY_ENTRY_EXPORT ERROR !!!\n"));
    return dwFunctionAddress;
  }
  if (!pDataDir->VirtualAddress)
  {
    DbgPrintPe(("He4HookInv: NativeGetProcAddress: pDataDir->VirtualAddress ERROR !!!\n"));
    return dwFunctionAddress;
  }

  pExportDir = (PIMAGE_EXPORT_DIRECTORY) (pDataDir->VirtualAddress + hModule);
  if (!_MmIsAddressValid(pExportDir))
  {
    DbgPrintPe(("He4HookInv: NativeGetProcAddress: PIMAGE_EXPORT_DIRECTORY ERROR !!!\n"));
    return dwFunctionAddress;
  }

  pszName = (char**)(pExportDir->AddressOfNames + hModule);
  for (i=0; i<pExportDir->NumberOfNames; i++)
  {
    if (_MmIsAddressValid(pszName))
    {
      if (_MmIsAddressValid((PVOID)(*pszName+hModule)))
      {
        if (!__strcmpi(*pszName+hModule, lpszFunctionName))
          break;
      }
    }
    pszName++;
  }

  if (i >= pExportDir->NumberOfNames)
    return dwFunctionAddress;
//  i++;

  pwOrdinals = (WORD*)(pExportDir->AddressOfNameOrdinals + hModule);
  if (!_MmIsAddressValid(pwOrdinals))
    return dwFunctionAddress;
  pdwFunctionAddress = (DWORD*)(pExportDir->AddressOfFunctions + hModule);
  if (!_MmIsAddressValid(pdwFunctionAddress))
    return dwFunctionAddress;
  dwFunctionAddress = pdwFunctionAddress[(pwOrdinals[i]/* + pExportDir->Base*/)] + hModule;

  DbgPrintPe(("He4HookInv: NativeGetProcAddress: SUCCESS!!!\n"));
  return dwFunctionAddress;
}

VOID RelocBuffer(DWORD hModule, DWORD Start, DWORD End, RELO_HEADER *RelocTable, int RelocTableSize, DWORD relo)
{
  RELO_HEADER    *rel;
  unsigned short *reltbl;
  unsigned int    i;
//  int             nRel;
  DWORD           trva;

//  __asm { int 3h };
  for (rel=RelocTable; ((ULONG)rel) < ((ULONG)RelocTable+RelocTableSize); rel = (RELO_HEADER *)(((ULONG)rel) + rel->Size))
//  for (rel=RelocTable, nRel=0; nRel < RelocTableSize; rel = (RELO_HEADER *)(((int)rel) + rel->Size), nRel += rel->Size)
  {
    reltbl=(unsigned short*)(((int)rel)+0x8);
    for (i=0; i<(((rel->Size)-0x8)/2); i++)
    {
      trva = rel->VirtualAddress + (reltbl[i] & 0x0fff) + hModule;
      if ((trva < Start) || (trva >= End))
        continue;
      switch (reltbl[i]&0xf000)
      {
        case 0x0000:
             break;
        case 0x3000:
             *(DWORD*)(trva) += relo;
             break;
        default : /*(*(char *)0)=0;*/ 
//     rel=0;
             break;
      }
    }
  }     
}

PIMAGE_SECTION_HEADER GetSection(PIMAGE_SECTION_HEADER pFirstSection, char *lpszSectionName, int nNumberSections)
{
  int i;

  if (!pFirstSection || !lpszSectionName)
    return NULL;

  for (i=0; i<nNumberSections; i++)
  {
    if (!__strcmpi((char*)pFirstSection[i].Name, lpszSectionName))
      return &pFirstSection[i];
  }
  return NULL;
}

// This function locates module base by any addres in code (".text") section
ULONG LocateBase(void *CodePtr)
{
  char              *cptr;
  IMAGE_DOS_HEADER  *dh;
  char              *tpc;
  ULONG             *nh;
  unsigned int       tpa;

  cptr = (char *)CodePtr;

  __try
  {
    if (!_MmIsAddressValid(CodePtr)) 
      return 0;

    if (*(unsigned short *)CodePtr==0x25ff)
    { // this is jmp @[] instruction
      cptr = (char *)*(ULONG *)*(ULONG *)((char *)CodePtr+2);
    }
    for (tpa=(((ULONG)cptr)&0xfffff000);tpa!=0x1000;tpa-=0x1000)
    {
      //unsigned short *tps;
      tpc = (char *)tpa; //tps=(unsigned short*)tpa;
      if (_MmIsAddressValid((void*)tpc)) 
      {
        if (tpc[0]!='M')
          continue;
        if (tpc[1]!='Z')
          continue;
    
        dh = (IMAGE_DOS_HEADER *)tpa;
    
        nh = (ULONG*)(((char *)dh)+dh->e_lfanew);
        if (_MmIsAddressValid((void*)nh)) 
        {
          if (*nh != IMAGE_NT_SIGNATURE)
            continue;
        }

        return tpa;
      }
    }
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
  }

  return 0;
}
