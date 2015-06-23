/* Based off crazylord's article, "Playing with Windows /dev/(k)mem" */


#include <stdio.h>
#include <windows.h>
#include "kmem.h"

/*
#define PSADD    0x8046A180 // win2k PsActiveProcessHead
#define PSADD     0x80545578  //XP
#define PSADD		0x8056a558	//XPSP2

#define BASEADD 0x804d4000  //XP
#define BASEADD 0x804000c8  //win2k
*/

// max process, to prevent easy crashing
#define MAX_PROCESS 50


typedef HMODULE (*PMODULE) ; 

typedef struct _MY_CG {
   PHYSICAL_ADDRESS     pAddress;
   PVOID                MappedAddress;
   PCALLGATE_DESCRIPTOR Desc;
   WORD                 Segment;
   WORD                 LastEntry;
} MY_CG, *PMY_CG;

    typedef struct gdtr {
        short Limit;
        short BaseLow;
        short BaseHigh;
    } Gdtr_t, *PGdtr_t;

#pragma pack(1)    // 2 works, too
typedef struct _IDTR
{
    WORD    wLimit;
	DWORD	Base;

} IDTR, *PIDTR;

#pragma pack()

typedef struct
{
	unsigned short LowOffset;
	unsigned short selector;
	unsigned char unused_lo;
	unsigned char segment_type:4;	//0x0E is an interrupt gate
	unsigned char system_segment_flag:1;
	unsigned char DPL:2;	// descriptor privilege level 
	unsigned char P:1; /* present */
	unsigned short HiOffset;
} IDTENTRY;

/*__declspec(dllimport) 
ULONG
DbgPrint(
    PCH Format,
    ...
    );*/

ULONG         Granularity;
PLIST_ENTRY   PsActiveProcessHead;// = (PLIST_ENTRY) PSADD;
PVOID	  PsInitialSystemProcess;
ULONG		  ListEntryOffset;
MY_CG         GdtMap;
MAPPING       CurMap;

PHYSICAL_ADDRESS (*MmGetPhysicalAddress) (PVOID BaseAddress);

BOOLEAN (*MmIsAddressValid) (PVOID VirtualAddress);

#define PROTECTED_POOL 0x80000000

PVOID (NTAPI * ExAllocatePoolWithTag)(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

PVOID (NTAPI *ExAllocatePool)(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

VOID (NTAPI *ExFreePool)(
	IN PVOID P
    );

PVOID (NTAPI *ExFreePoolWithTag)(
    IN PVOID P,
    IN ULONG Tag
    );

VOID 
  (NTAPI *RtlMoveMemory)(
    IN VOID UNALIGNED  *Destination,
    IN CONST VOID UNALIGNED  *Source,
    IN SIZE_T  Length
    );


HANDLE (NTAPI *PsGetCurrentProcessId)( VOID );



ULONG MiniMmGetPhysicalAddress(ULONG virtualaddress)
    {
        if(virtualaddress<0x80000000||virtualaddress>=0xA0000000)
           return 0;
        return virtualaddress&0x1FFFF000;
    }


NTSTATUS          status;
OBJECT_ATTRIBUTES ObjectAttributes;
UNICODE_STRING       FileName;
UCHAR ImageName[16];

DWORD intgate, LowOffset, HiOffset;
PVOID ImageBase,intbase ;
IDTR MyIDTR;
IDTENTRY * Myidt;

PIMAGE_DOS_HEADER	header;
WORD mzheader;

DWORD NameOffset;
BOOLEAN ExportsRetrvd = FALSE;


VOID GetKrnlExports(void)
{

  HMODULE           hDll;
  hDll = LoadLibrary("ntoskrnl.exe");
//	hDll = GetModuleHandle("ntdll.dll");
 
//save offsets and use them later to get kernel addresses  
  if (hDll) {
   DbgPrint = NULL;

   DbgPrint = (PVOID) ( (DWORD)  GetProcAddress(hDll, "DbgPrint") - (DWORD) hDll);
   MmGetPhysicalAddress =  (DWORD)  GetProcAddress(hDll, "MmGetPhysicalAddress") - (DWORD) hDll;	  
   ExAllocatePoolWithTag = (DWORD)  GetProcAddress(hDll, "ExAllocatePoolWithTag") - (DWORD) hDll;
   ExFreePool = (DWORD)  GetProcAddress(hDll, "ExFreePool") - (DWORD) hDll;
   RtlMoveMemory = (DWORD)  GetProcAddress(hDll, "RtlMoveMemory") - (DWORD) hDll;
   KeServiceDescriptorTable = (DWORD)  GetProcAddress(hDll, "KeServiceDescriptorTable") - (DWORD) hDll;

   PsInitialSystemProcess = (DWORD)  GetProcAddress(hDll, "PsInitialSystemProcess") - (DWORD) hDll;
   MmIsAddressValid = (DWORD)  GetProcAddress(hDll, "MmIsAddressValid") - (DWORD) hDll;
   //KeServiceDescriptorTable = 0x805425c0;
   //MmGetPhysicalAddress = 0x805196fd; //xp 2 

   FreeLibrary(hDll); 
   }
  GetCallGate();
}

void AddImageBase()
{
   DbgPrint = (DWORD)ImageBase + (DWORD)DbgPrint;

   ExAllocatePoolWithTag = (DWORD)ImageBase + (DWORD)ExAllocatePoolWithTag;
   ExFreePool= (DWORD)ImageBase + (DWORD)ExFreePool;
   RtlMoveMemory= (DWORD)ImageBase + (DWORD)RtlMoveMemory;
   
   KeServiceDescriptorTable = (DWORD)ImageBase + (DWORD)KeServiceDescriptorTable;
   PsInitialSystemProcess = (DWORD)ImageBase + (DWORD)PsInitialSystemProcess;
   MmIsAddressValid = (DWORD)ImageBase + (DWORD)MmIsAddressValid;
   MmGetPhysicalAddress = (DWORD)ImageBase + (DWORD)MmGetPhysicalAddress;
  
}

PVOID GetImageBase()
{
	//Memory scan used from cavity infector and converted to C. 

	intbase =  MyIDTR.Base;

	HiOffset =  *(WORD*) ((int)intbase + 0x2a * 8  + 6);

	LowOffset = *(WORD*) ((int)intbase + 0x2a * 8) ;

	intgate = MAKE_DWORD(LowOffset,HiOffset);//HiOffset << 16 | LowOffset; // shift 16 bits to the left. Or the second value;

	ImageBase = (ULONG)intgate & 0xFFFFF000;//align to 4096 boundry
	
	while (*(WORD *)ImageBase != IMAGE_DOS_SIGNATURE && (ImageBase > 0x80000000) )
	{
		(ULONG)ImageBase -= 0x00001000;
	}

	//mzheader = *(WORD *)ImageBase;

	return ImageBase;
}


void __declspec(naked) Ring0Func() {
   
	
_asm {
      pushad
      pushf
      cli

     sidt MyIDTR    // we save base address of IDT so that we can access to it

}


if (!ExportsRetrvd){


	GetImageBase();//Set global ImageBase variable

	//	DbgPrint("ntoskrnl base: %.08x\n",ImageBase );
	

	//Must have ntoskrnl base address before getting routine addresses.
	
	AddImageBase(); //Set global kernel address variables
	
	NameOffset = getProcNameOffset(PsInitialSystemProcess); //Set ImageName offset
	//DbgPrint("NameOffset %.08x \n", NameOffset);

	PsActiveProcessHead = (PLIST_ENTRY)(*(DWORD*) PsInitialSystemProcess + ListEntryOffset);

	PsActiveProcessHead = PsActiveProcessHead->Blink;//Set Real ProcessHead
	//DbgPrint("PsActiveProcessHead %.08x \n",PsActiveProcessHead);



   ExportsRetrvd = TRUE;
}

//else do more stuff if you want

_asm {
	
	 
      mov esi, CurMap.vAddress
      push esi
      call MmGetPhysicalAddress
      mov CurMap.pAddress, eax  // save low part of LARGE_INTEGER
      mov [CurMap+4], edx       // save high part of LARGE_INTEGER


      popf
      popad
      retf
   }
}


int GetCallGate() {
   WORD   farcall[3];
   HANDLE Thread = GetCurrentThread();

   farcall[2] = GdtMap.Segment;

//_asm int 3

   if(!VirtualLock((PVOID) Ring0Func, 0x30)) {
      printf("error: unable to lock function\n");
      CurMap.pAddress.QuadPart = 1;
   } else {

      
	  SetThreadPriority(Thread, THREAD_PRIORITY_TIME_CRITICAL);
      Sleep(0);

      _asm call fword ptr [farcall]

	  Sleep(10);

      SetThreadPriority(Thread,THREAD_PRIORITY_NORMAL);
      VirtualUnlock((PVOID) Ring0Func, 0x30);
   }


   return(0);
}

// function which call the callgate
PHYSICAL_ADDRESS NewGetPhysicalAddress(PVOID vAddress) {
   WORD   farcall[3];
   HANDLE Thread = GetCurrentThread();

   farcall[2] = GdtMap.Segment;

   if(!VirtualLock((PVOID) Ring0Func, 0x30)) {
      printf("error: unable to lock function\n");
      CurMap.pAddress.QuadPart = 1;
   } else {

      CurMap.vAddress = vAddress; // ugly way to pass argument
      CurMap.Offset   = (DWORD) vAddress % Granularity;
     (DWORD) CurMap.vAddress -= CurMap.Offset;

//	 printf("vAddress %x \n",vAddress);
	 
      SetThreadPriority(Thread, THREAD_PRIORITY_TIME_CRITICAL);
      Sleep(0);

      _asm call fword ptr [farcall]

      SetThreadPriority(Thread,THREAD_PRIORITY_NORMAL);
      VirtualUnlock((PVOID) Ring0Func, 0x30);

	//  printf("physical address: 0x%.16x\n", CurMap.pAddress.QuadPart);
   }
   return(CurMap.pAddress);
}


PHYSICAL_ADDRESS GetPhysicalAddress(ULONG vAddress) {
   PHYSICAL_ADDRESS  add;

    if (vAddress < 0x80000000L || vAddress >= 0xA0000000L) {
      add.QuadPart = (ULONGLONG) vAddress & 0xFFFF000;
   } else {
      add.QuadPart = (ULONGLONG) vAddress & 0x1FFFF000;
   }
   return(add);
}

void UnmapMemory(PVOID MappedAddress) {
   NtUnmapViewOfSection((HANDLE) -1, MappedAddress);
}


int InstallCallgate(HANDLE Section, DWORD Function) {
   NTSTATUS             ntS;
   KGDTENTRY            gGdt;
   DWORD                Size;
   PCALLGATE_DESCRIPTOR CgDesc;

   _asm sgdt gGdt;


   printf("virtual address of GDT : 0x%.8x\n",
          MAKE_DWORD(gGdt.BaseLow, gGdt.BaseHigh));
   GdtMap.pAddress =
              GetPhysicalAddress(MAKE_DWORD(gGdt.BaseLow, gGdt.BaseHigh));
   printf("physical address of GDT: 0x%.16x\n", GdtMap.pAddress.QuadPart);
   
   Size = gGdt.LimitLow;
   ntS = NtMapViewOfSection(Section, (HANDLE) -1, &GdtMap.MappedAddress,
                            0L, Size, &GdtMap.pAddress, &Size, ViewShare,
                            0, PAGE_READWRITE);
   if (ntS != STATUS_SUCCESS || !GdtMap.MappedAddress) {
      printf("error: NtMapViewOfSection (code: %x)\n", ntS);
      return(0);
   }

   GdtMap.LastEntry = gGdt.LimitLow & 0xFFF8; // offset to last entry
   
	for(CgDesc = (PVOID) ((DWORD)GdtMap.MappedAddress+GdtMap.LastEntry),
       GdtMap.Desc=NULL;
       (DWORD) CgDesc > (DWORD) GdtMap.MappedAddress;
       CgDesc--) {
      
      //printf("present:%x, type:%x\n", CgDesc->present, CgDesc->type);
      if(CgDesc->present == 0){
         CgDesc->offset_0_15  = (WORD) (Function & 0xFFFF);
         CgDesc->selector     = 8;
         CgDesc->param_count  = 0; //1;
         CgDesc->some_bits    = 0;
         CgDesc->type         = 12;     // 32-bits callgate junior :>
         CgDesc->app_system   = 0;      // A system segment
         CgDesc->dpl          = 3;      // Ring 3 code can call
         CgDesc->present      = 1;
         CgDesc->offset_16_31 = (WORD) (Function >> 16);
         GdtMap.Desc = CgDesc;
         break;
      }

   }

   if (GdtMap.Desc == NULL) {
      printf("error: unable to find free entry for installing callgate\n");
      printf("       not normal by the way .. your box is strange =]\n");
   }

   GdtMap.Segment =
      ((WORD) ((DWORD) CgDesc - (DWORD) GdtMap.MappedAddress))|3;
   printf("Allocated segment      : %x\n", GdtMap.Segment);
   return(1);
}

int UninstallCallgate(HANDLE Section, DWORD Function) {
   PCALLGATE_DESCRIPTOR CgDesc;

   for(CgDesc = (PVOID) ((DWORD) GdtMap.MappedAddress+GdtMap.LastEntry);
       (DWORD) CgDesc > (DWORD) GdtMap.MappedAddress;
      CgDesc--) {
      
      if((CgDesc->offset_0_15 == (WORD) (Function & 0xFFFF))
         && CgDesc->offset_16_31 == (WORD) (Function >> 16)){
         memset(CgDesc, 0, sizeof(CALLGATE_DESCRIPTOR));
printf("callgateuninstalled\n");
         return(1);
      }
   }
   NtUnmapViewOfSection((HANDLE) -1, GdtMap.MappedAddress);

   return(0);
}

void UnmapVirtualMemory(PVOID vAddress) {
   NtUnmapViewOfSection((HANDLE) -1, vAddress);
}

PVOID MapVirtualMemory(HANDLE Section, PVOID vAddress, DWORD Size) {
   PHYSICAL_ADDRESS pAddress;
   NTSTATUS         ntS;
   DWORD            MappedSize;
   PVOID            MappedAddress=NULL;

   //printf("* vAddress: 0x%.8x\n", vAddress);
   pAddress = NewGetPhysicalAddress((PVOID) vAddress);
   //printf("* vAddress: 0x%.8x (after rounding, offset: 0x%x)\n",
   //       CurMap.vAddress, CurMap.Offset);
   //printf("* pAddress: 0x%.16x\n", pAddress);

   // check for error (1= impossible value)
   if (pAddress.QuadPart != 1) {
      Size += CurMap.Offset; // adjust mapping view
      MappedSize = Size;

      ntS = NtMapViewOfSection(Section, (HANDLE) -1, &MappedAddress,
                         0L, Size, &pAddress, &MappedSize, ViewShare,
                         0, SECTION_MAP_WRITE );
      if (ntS != STATUS_SUCCESS || !MappedSize) {
         printf(" error: NtMapViewOfSection, mapping 0x%.8x (code: %x)\n",
                vAddress, ntS);
         return(NULL);
      }
   } else
      MappedAddress = NULL;
  // printf("mapped %d bytes @ 0x%.8x (init Size: 0x%x bytes)\n",
   //      MappedSize, MappedAddress, Size);
		 
   return(MappedAddress);
}



void DisplayProcesses(HANDLE Section) {
   int i = 0;
   DWORD     Padding;
   PVOID CurProcess, NextProcess;
   PVOID     vCurEntry, vOldEntry, NewMappedAddress;
   PLIST_ENTRY PsCur;

   // first we map PsActiveProcessHead to get first entry
   vCurEntry = MapVirtualMemory(Section, PsActiveProcessHead, 4);
   if (!vCurEntry)
      return;
   PsCur = (PLIST_ENTRY) ((DWORD) vCurEntry + CurMap.Offset);
   
   // most of EPROCESS struct are located around 0xfc[e-f]00000
   // so we map 0x100000 bytes (~ 1mb) to avoid heavy mem mapping
   while (PsCur->Flink != PsActiveProcessHead && i<MAX_PROCESS) {

	   NextProcess = (PVOID) TO_EPROCESS(PsCur->Flink);
      //printf("==> Current process: %x\n", CurProcess);

      // we map 0x100000 bytes view so we store offset to EPROCESS
      Padding = TO_EPROCESS(PsCur->Flink) & 0xFFFFF;

      // check if the next struct is already mapped in memory
      if ((DWORD) vCurEntry<= (DWORD) NextProcess 
         && (DWORD)NextProcess+sizeof(EPROCESS)<(DWORD)vCurEntry+0x100000){
         // no need to remap
         // no remapping so we need to calculate the new address
         CurProcess = (PVOID) ((DWORD) NewMappedAddress + Padding);

      } else {
         CurProcess = NextProcess;
         // unmap old view and map a new one
         // calculate next base address to map
         vOldEntry = vCurEntry;
         vCurEntry = (PVOID) (TO_EPROCESS(PsCur->Flink) & 0xFFF00000);

         //printf("link: %x, process: %x, to_map: %x, padding: %x\n",
         //    PsCur->Flink, TO_EPROCESS(PsCur->Flink),
         //    vCurEntry, Padding);

         // unmap old view
         UnmapVirtualMemory(vOldEntry);
         vOldEntry = vCurEntry;
         // map new view
         vCurEntry = MapVirtualMemory(Section, vCurEntry, 0x100000);
         if (!vCurEntry)
            break;
         // adjust EPROCESS structure pointer
         CurProcess =
                 (PVOID) ((DWORD) vCurEntry + CurMap.Offset + Padding);
         // save mapped address
         NewMappedAddress = vCurEntry;
         // restore pointer from mapped addresses space 0x4**** to
         // the real virtual address 0xf*******
         vCurEntry = vOldEntry;
      }

      // reajust pointer to LIST_ENTRY struct
      PsCur = (DWORD)CurProcess + ListEntryOffset;//&CurProcess->ActiveProcessLinks;
	  
	  //0x0174 is offset to imagename in xp sp2 eprocess struct
		//NameOffset = getProcNameOffset((DWORD)CurProcess);

	    strncpy(ImageName,((DWORD)CurProcess + NameOffset),16);
		printf("==> ImageName: %s \n",ImageName);

      i++;
   }


   UnmapVirtualMemory(vCurEntry);
}




int main(int argc, char **argv) {
   SYSTEM_INFO       SysInfo;
   OBJECT_ATTRIBUTES ObAttributes;
   NTSTATUS          ntS;
   HANDLE            Section;
   PVOID			vTest;
   ULONG                FileNameLength;
   PHYSICAL_ADDRESS pAddress;

   INIT_UNICODE(ObString, L"\\Device\\PhysicalMemory");


 DisableProt(1);

   GetSystemInfo(&SysInfo);
   Granularity = SysInfo.dwAllocationGranularity;
   printf("Allocation granularity: %lu bytes\n", Granularity);
   InitializeObjectAttributes(&ObAttributes,
                              &ObString,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                              NULL,
                              NULL);


   ntS = NtOpenSection(&Section, SECTION_MAP_READ|SECTION_MAP_WRITE,
                       &ObAttributes);
   if (ntS != STATUS_SUCCESS) {
	   if (ntS == STATUS_ACCESS_DENIED){
		 printf("error: access denied to open \\Device\\PhysicalMemory for r/w\n");
	   }
      else
         printf("error: NtOpenSection (code: %x)\n", ntS);
	   
      goto cleanup;
   }



   if (!InstallCallgate(Section, (DWORD) Ring0Func))
      goto cleanup;

   memset(&CurMap, 0, sizeof(MAPPING));

  __try {
ListEntryOffset = FindSystemProcessActiveLink();
GetKrnlExports();//Get Krnl addresses we want for later use


//Might as well not go on with out the ImageName offset
if (NameOffset > 1)
{

	DisplayProcesses(Section);
}


   } __except(UninstallCallgate(Section, (DWORD) Ring0Func), 1) {
      printf("exception: trying to clean callgate...\n");

	  goto cleanup;
   }
	
   if (!UninstallCallgate(Section, (DWORD) Ring0Func))
      goto cleanup;

cleanup:
   if (Section)
      NtClose(Section);
	printf("callgatecleanup\n");

return 0;

}


DWORD FindSystemProcessActiveLink(void)
{
DWORD Offset;
OSVERSIONINFO info;

GetVersionEx(&info);


	if(info.dwMinorVersion == 0)//win2k
		Offset = 0xa0;
	else if(info.dwMinorVersion > 0)//xp
		Offset = 0x88;
	else
		return 0;

	return Offset;
}


DWORD getProcNameOffset(PVOID startPos)
{
    int i = 1;
	DWORD procNameOffset = 0;

	while (strcmp(*(DWORD *)startPos + i, "System") != 0 && i < PAGE_SIZE)
	{	
		i += 1;
	}
	return i;
}
