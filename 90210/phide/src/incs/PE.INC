;
;                       . .: .:.. :.. .. .:.::. :. ..:
;                     <<-==ÜÛÛÛÛÛÜ=ÜÛÛÛÛÛÜ=ÜÛÛÛÛÛÜ===<
;                      .:: ÛÛÛ ÛÛÛ:ÛÛÛ ÛÛÛ.ÛÛÛ ÛÛÛ .:.
;                      . .:.ÜÜÜÛÛß.ßÛÛÛÛÛÛ.ÛÛÛÛÛÛÛ:..
;                       ...ÛÛÛÜÜÜÜ:ÜÜÜÜÛÛÛ:ÛÛÛ ÛÛÛ.::.
;                      >===ÛÛÛÛÛÛÛ=ÛÛÛÛÛÛß=ÛÛÛ ÛÛÛ=->>
;                     .: .:.. ..:. .: ..:.::. ::.. :.:.
;
;                              [29A INC files]
;                         Portable Executable format
;                            by Jacky Qwerty/29A
;
; Description
; ÄÄÄÄÄÄÄÄÄÄÄ
; This include file contains all the constantz and structurez needed to work
; with the PE (Portable Executable) format from inside ASM filez. For exclu-
; sive use with TASM(32), of course. MASM sucks.. :P
;
; Disclaimer
; ÄÄÄÄÄÄÄÄÄÄ
; This file was built up by Jacky Qwerty from 29A. The author is not respon-
; sible for any problemz caused due to use/misuse of this file.
;
;
; (c) 1997. No rightz reserved. Use without permision >8P.


; ÄÄ´ Based relocation type valuez ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_REL_BASED_ABSOLUTE      EQU   0
IMAGE_REL_BASED_HIGH          EQU   1
IMAGE_REL_BASED_LOW           EQU   2
IMAGE_REL_BASED_HIGHLOW       EQU   3
IMAGE_REL_BASED_HIGHADJ       EQU   4
IMAGE_REL_BASED_MIPS_JMPADDR  EQU   5

IMAGE_RELOCATION_DATA   RECORD      {
    RD_RelocType        :4
    RD_RelocOffset      :12
}

IMAGE_BASE_RELOCATION   STRUC
    BR_VirtualAddress   DD    ?
    BR_SizeOfBlock      DD    ?
;   BR_TypeOffset       IMAGE_RELOCATION_DATA 1 DUP (?)   ; Array of zero or more relocations (type + RVAs)
IMAGE_BASE_RELOCATION   ENDS

IMAGE_SIZEOF_BASE_RELOCATION  EQU   SIZE  IMAGE_BASE_RELOCATION   ; 8

IMAGE_IMPORT_BY_NAME    STRUC
    IBN_Hint            DW    ?
    IBN_Name            DB    1 DUP (?)   ; ASCIIZ function name (variable size)
IMAGE_IMPORT_BY_NAME    ENDS

IMAGE_ORDINAL_FLAG      EQU   80000000h

IMAGE_THUNK_DATA        STRUC
                        UNION
    TD_AddressOfData    DD    IMAGE_IMPORT_BY_NAME PTR ?    ; Ptr to IMAGE_IMPORT_BY_NAME structure
    TD_Ordinal          DD    ?                             ; Ordinal ORed with IMAGE_ORDINAL_FLAG
    TD_Function         DD    BYTE PTR ?  ; CODE PTR        ; Ptr to function (i.e. Function address after program load)
    TD_ForwarderString  DD    BYTE PTR ?                    ; Ptr to a forwarded API function.
                        ENDS
IMAGE_THUNK_DATA        ENDS

; ÄÄ´ Import format ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_IMPORT_DESCRIPTOR       STRUC
                              UNION
    ID_Characteristics        DD    ?                       ; 0 for terminating null import descriptor
    ID_OriginalFirstThunk     DD    IMAGE_THUNK_DATA PTR ?  ; RVA to original unbound IAT
                              ENDS
    ID_TimeDateStamp          DD    ?     ; 0 if not bound,
                                          ; -1 if bound, and real date\time stamp
                                          ;     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                          ; O.W. date/time stamp of DLL bound to (Old BIND)
    ID_ForwarderChain         DD    ?           ; -1 if no forwarders
    ID_Name                   DD    BYTE PTR ?              ; RVA to name of imported DLL
    ID_FirstThunk             DD    IMAGE_THUNK_DATA PTR ?  ; RVA to IAT (if bound this IAT has actual addresses)
IMAGE_IMPORT_DESCRIPTOR       ENDS

IMAGE_SIZEOF_IMPORT_DESCRIPTOR      EQU   SIZE IMAGE_IMPORT_DESCRIPTOR

; ÄÄ´ Export format ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_EXPORT_DIRECTORY        STRUC
    ED_Characteristics        DD    ?
    ED_TimeDateStamp          DD    ?
    ED_MajorVersion           DW    ?
    ED_MinorVersion           DW    ?
    ED_Name                   DD    BYTE PTR ?  ; Ptr to name of exported DLL
                              UNION
    ED_Base                   DD    ?
    ED_BaseOrdinal            DD    ?
                              ENDS
    ED_NumberOfFunctions      DD    ?
                              UNION
    ED_NumberOfNames          DD    ?
    ED_NumberOfOrdinals       DD    ?
                              ENDS
    ED_AddressOfFunctions     DD    DWORD PTR ? ; Ptr to array of function addresses
    ED_AddressOfNames         DD    DWORD PTR ? ; Ptr to array of (function) name addresses
                              UNION
    ED_AddressOfNameOrdinals  DD    WORD PTR ?  ; Ptr to array of ordinals
    ED_AddressOfOrdinals      DD    WORD PTR ?  ;
                              ENDS
IMAGE_EXPORT_DIRECTORY        ENDS

IMAGE_SIZEOF_EXPORT_DIRECTORY EQU   SIZE IMAGE_EXPORT_DIRECTORY

; ÄÄ´ SH_Characteristics valuez ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

;MAGE_SCN_TYPE_REG               EQU  00000000h ; Reserved
;MAGE_SCN_TYPE_REGULAR           EQU  00000000h ; Reserved
;MAGE_SCN_TYPE_DSECT             EQU  00000001h ; Reserved
;MAGE_SCN_TYPE_DUMMY             EQU  00000001h ; Reserved
;MAGE_SCN_TYPE_NOLOAD            EQU  00000002h ; Reserved
;MAGE_SCN_TYPE_NO_LOAD           EQU  00000002h ; Reserved
;MAGE_SCN_TYPE_GROUP             EQU  00000004h ; Reserved. Used for 16-bit offset code
;MAGE_SCN_TYPE_GROUPED           EQU  00000004h ; Reserved. Used for 16-bit offset code
IMAGE_SCN_TYPE_NO_PAD            EQU  00000008h ; Reserved
;MAGE_SCN_TYPE_COPY              EQU  00000010h ; Reserved
IMAGE_SCN_CNT_CODE               EQU  00000020h ; Section contains code.
IMAGE_SCN_CNT_INITIALIZED_DATA   EQU  00000040h ; Section contains initialized data.
IMAGE_SCN_CNT_UNINITIALIZED_DATA EQU  00000080h ; Section contains uninitialized data.
IMAGE_SCN_LNK_OTHER              EQU  00000100h ; Reserved.
IMAGE_SCN_LNK_INFO               EQU  00000200h ; Section contains comments or some other type of information.
;MAGE_SCN_TYPE_OVER              EQU  00000400h ; Reserved. Section contains an overlay.
;MAGE_SCN_LNK_OVERLAY            EQU  00000400h ; Reserved. Section contains an overlay.
IMAGE_SCN_LNK_REMOVE             EQU  00000800h ; Section contents will not become part of image.
IMAGE_SCN_LNK_COMDAT             EQU  00001000h ; Section contents comdat.
;                                EQU  00002000h ; Reserved.
;MAGE_SCN_MEM_PROTECTED          EQU  00004000h ; Obsolete.
IMAGE_SCN_MEM_FARDATA            EQU  00008000h
;MAGE_SCN_MEM_SYSHEAP            EQU  00010000h ; Obsolete.
IMAGE_SCN_MEM_PURGEABLE          EQU  00020000h
IMAGE_SCN_MEM_16BIT              EQU  00020000h
IMAGE_SCN_MEM_LOCKED             EQU  00040000h
IMAGE_SCN_MEM_PRELOAD            EQU  00080000h
IMAGE_SCN_ALIGN_1BYTES           EQU  00100000h
IMAGE_SCN_ALIGN_2BYTES           EQU  00200000h
IMAGE_SCN_ALIGN_4BYTES           EQU  00300000h
IMAGE_SCN_ALIGN_8BYTES           EQU  00400000h
IMAGE_SCN_ALIGN_16BYTES          EQU  00500000h ; Default alignment if no others are specified.
IMAGE_SCN_ALIGN_32BYTES          EQU  00600000h
IMAGE_SCN_ALIGN_64BYTES          EQU  00700000h
;                                EQU  00800000h ; Unused.
IMAGE_SCN_LNK_NRELOC_OVFL        EQU  01000000h ; Section contains extended relocations.
IMAGE_SCN_MEM_DISCARDABLE        EQU  02000000h ; Section can be discarded.
IMAGE_SCN_MEM_NOT_CACHED         EQU  04000000h ; Section is not cachable.
IMAGE_SCN_MEM_NOT_PAGED          EQU  08000000h ; Section is not pageable.
IMAGE_SCN_MEM_SHARED             EQU  10000000h ; Section is shareable.
IMAGE_SCN_MEM_EXECUTE            EQU  20000000h ; Section is executable.
IMAGE_SCN_MEM_READ               EQU  40000000h ; Section is readable.
IMAGE_SCN_MEM_WRITE              EQU  80000000h ; Section is writeable.

IMAGE_SIZEOF_SHORT_NAME          EQU  8

; ÄÄ´ Section header format ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_SECTION_HEADER          STRUC
    SH_Name                   DB    IMAGE_SIZEOF_SHORT_NAME DUP (?)
                              UNION
    SH_PhysicalAddress        DD    BYTE PTR ?
    SH_VirtualSize            DD    ?
                              ENDS
    SH_VirtualAddress         DD    BYTE PTR ?
    SH_SizeOfRawData          DD    ?
    SH_PointerToRawData       DD    BYTE PTR ?
    SH_PointerToRelocations   DD    BYTE PTR ?
    SH_PointerToLinenumbers   DD    BYTE PTR ?
    SH_NumberOfRelocations    DW    ?
    SH_NumberOfLinenumbers    DW    ?
    SH_Characteristics        DD    ?
IMAGE_SECTION_HEADER          ENDS

IMAGE_SIZEOF_SECTION_HEADER   EQU   SIZE IMAGE_SECTION_HEADER

; ÄÄ´ OH_DataDirectory index valuez ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_DIRECTORY_ENTRY_EXPORT        EQU   0     ; Export Directory
IMAGE_DIRECTORY_ENTRY_IMPORT        EQU   1     ; Import Directory
IMAGE_DIRECTORY_ENTRY_RESOURCE      EQU   2     ; Resource Directory
IMAGE_DIRECTORY_ENTRY_EXCEPTION     EQU   3     ; Exception Directory
IMAGE_DIRECTORY_ENTRY_SECURITY      EQU   4     ; Security Directory
IMAGE_DIRECTORY_ENTRY_BASERELOC     EQU   5     ; Base Relocation Table
IMAGE_DIRECTORY_ENTRY_DEBUG         EQU   6     ; Debug Directory
IMAGE_DIRECTORY_ENTRY_COPYRIGHT     EQU   7     ; Description String
IMAGE_DIRECTORY_ENTRY_GLOBALPTR     EQU   8     ; Machine Value (MIPS GP)
IMAGE_DIRECTORY_ENTRY_TLS           EQU   9     ; TLS Directory
IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG   EQU   10    ; Load Configuration Directory
IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT  EQU   11    ; Bound Import Directory in headers
IMAGE_DIRECTORY_ENTRY_IAT           EQU   12    ; Import Address Table

IMAGE_NUMBEROF_DIRECTORY_ENTRIES    EQU   16

; ÄÄ´ OH_DataDirectory format ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_DATA_DIRECTORY    STRUC
    DD_VirtualAddress   DD    BYTE PTR ?
    DD_Size             DD    ?
IMAGE_DATA_DIRECTORY    ENDS

IMAGE_DIRECTORY_ENTRIES STRUC
    DE_Export           IMAGE_DATA_DIRECTORY    ?
    DE_Import           IMAGE_DATA_DIRECTORY    ?
    DE_Resource         IMAGE_DATA_DIRECTORY    ?
    DE_Exception        IMAGE_DATA_DIRECTORY    ?
    DE_Security         IMAGE_DATA_DIRECTORY    ?
    DE_BaseReloc        IMAGE_DATA_DIRECTORY    ?
    DE_Debug            IMAGE_DATA_DIRECTORY    ?
    DE_Copyright        IMAGE_DATA_DIRECTORY    ?
    DE_GlobalPtr        IMAGE_DATA_DIRECTORY    ?
    DE_TLS              IMAGE_DATA_DIRECTORY    ?
    DE_LoadConfig       IMAGE_DATA_DIRECTORY    ?
    DE_BoundImport      IMAGE_DATA_DIRECTORY    ?
    DE_IAT              IMAGE_DATA_DIRECTORY    ?
IMAGE_DIRECTORY_ENTRIES ENDS

; ÄÄ´ OH_LoaderFlags valuez ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

;MAGE_LOADER_FLAGS_BREAK_ON_LOAD    EQU   00000001h
;MAGE_LOADER_FLAGS_DEBUG_ON_LOAD    EQU   00000002h

; ÄÄ´ OH_DllCharacteristics valuez ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

;MAGE_LIBRARY_PROCESS_INIT    EQU   1     ; Dll has a process initialization routine
;MAGE_LIBRARY_PROCESS_TERM    EQU   2     ; Dll has a thread termination routine
;MAGE_LIBRARY_THREAD_INIT     EQU   4     ; Dll has a thread initialization routine
;MAGE_LIBRARY_THREAD_TERM     EQU   8     ; Dll has a thread termination routine

; ÄÄ´ OH_Subsystem Valuez ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_SUBSYSTEM_UNKNOWN       EQU   0     ; Unknown subsystem
IMAGE_SUBSYSTEM_NATIVE        EQU   1     ; Image doesn't require a subsystem
IMAGE_SUBSYSTEM_WINDOWS_GUI   EQU   2     ; Image runs in the Windows GUI subsystem
IMAGE_SUBSYSTEM_WINDOWS_CUI   EQU   3     ; Image runs in the Windows character subsystem
IMAGE_SUBSYSTEM_OS2_CUI       EQU   5     ; Image runs in the OS/2 character subsystem
IMAGE_SUBSYSTEM_POSIX_CUI     EQU   7     ; Image run  in the Posix character subsystem

; ÄÄ´ OH_Magic value ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_NT_OPTIONAL_HDR_MAGIC   EQU   10Bh

; ÄÄ´ Optional header format ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_OPTIONAL_HEADER               STRUC
 ; Standard fields:
    OH_Magic                        DW    ?
    OH_MajorLinkerVersion           DB    ?
    OH_MinorLinkerVersion           DB    ?
    OH_SizeOfCode                   DD    ?
    OH_SizeOfInitializedData        DD    ?
    OH_SizeOfUninitializedData      DD    ?
    OH_AddressOfEntryPoint          DD    BYTE PTR ?
    OH_BaseOfCode                   DD    BYTE PTR ?
    OH_BaseOfData                   DD    BYTE PTR ?
 ; NT additional fields:
    OH_ImageBase                    DD    BYTE PTR ?
    OH_SectionAlignment             DD    ?
    OH_FileAlignment                DD    ?
    OH_MajorOperatingSystemVersion  DW    ?
    OH_MinorOperatingSystemVersion  DW    ?
    OH_MajorImageVersion            DW    ?
    OH_MinorImageVersion            DW    ?
    OH_MajorSubsystemVersion        DW    ?
    OH_MinorSubsystemVersion        DW    ?
    OH_Reserved1                    DD    ?
    OH_SizeOfImage                  DD    ?
    OH_SizeOfHeaders                DD    ?
    OH_CheckSum                     DD    ?
    OH_Subsystem                    DW    ?
    OH_DllCharacteristics           DW    ?
    OH_SizeOfStackReserve           DD    ?
    OH_SizeOfStackCommit            DD    ?
    OH_SizeOfHeapReserve            DD    ?
    OH_SizeOfHeapCommit             DD    ?
    OH_LoaderFlags                  DD    ?
    OH_NumberOfRvaAndSizes          DD    ?
                                    UNION
    OH_DataDirectory                IMAGE_DATA_DIRECTORY    \
                                      IMAGE_NUMBEROF_DIRECTORY_ENTRIES \
                                      DUP (?)
    OH_DirectoryEntries             IMAGE_DIRECTORY_ENTRIES ?
                                    ENDS
IMAGE_OPTIONAL_HEADER               ENDS

IMAGE_SIZEOF_STD_OPTIONAL_HEADER    EQU   28d
IMAGE_SIZEOF_NT_OPTIONAL_HEADER     EQU   SIZE IMAGE_OPTIONAL_HEADER

; ÄÄ´ FH_Characteristics valuez ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_FILE_RELOCS_STRIPPED     EQU 0001h ; Relocation info stripped from file
IMAGE_FILE_EXECUTABLE_IMAGE    EQU 0002h ; File is executable  (i.e. no unresolved external references)
IMAGE_FILE_LINE_NUMS_STRIPPED  EQU 0004h ; Line numbers stripped from file
IMAGE_FILE_LOCAL_SYMS_STRIPPED EQU 0008h ; Local symbols stripped from file
;MAGE_FILE_MINIMAL_OBJECT      EQU 0010h ; Reserved
;MAGE_FILE_UPDATE_OBJECT       EQU 0020h ; Reserved
;MAGE_FILE_16BIT_MACHINE       EQU 0040h ; 16 bit word machine
IMAGE_FILE_BYTES_REVERSED_LO   EQU 0080h ; Bytes of machine word are reversed
IMAGE_FILE_32BIT_MACHINE       EQU 0100h ; 32 bit word machine
IMAGE_FILE_DEBUG_STRIPPED      EQU 0200h ; Debugging info stripped from file in .DBG file
;MAGE_FILE_PATCH               EQU 0400h ; Reserved
IMAGE_FILE_SYSTEM              EQU 1000h ; System File
IMAGE_FILE_DLL                 EQU 2000h ; File is a DLL
IMAGE_FILE_BYTES_REVERSED_HI   EQU 8000h ; Bytes of machine word are reversed

; ÄÄ´ FH_Machine valuez ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_FILE_MACHINE_UNKNOWN    EQU   0
IMAGE_FILE_MACHINE_I386       EQU   14Ch ; Intel 386
IMAGE_FILE_MACHINE_R3000      EQU   162h ; MIPS L-endian, 0160h B-endian
IMAGE_FILE_MACHINE_R4000      EQU   166h ; MIPS L-endian
IMAGE_FILE_MACHINE_R10000     EQU   168h ; MIPS L-endian
IMAGE_FILE_MACHINE_ALPHA      EQU   184h ; Alpha_AXP
IMAGE_FILE_MACHINE_POWERPC    EQU   1F0h ; IBM PowerPC L-Endian

IMAGE_FILE_HEADER             STRUC
    FH_Machine                DW    ?
    FH_NumberOfSections       DW    ?
    FH_TimeDateStamp          DD    ?
    FH_PointerToSymbolTable   DD    BYTE PTR ?
    FH_NumberOfSymbols        DD    ?
    FH_SizeOfOptionalHeader   DW    ?
    FH_Characteristics        DW    ?
IMAGE_FILE_HEADER             ENDS

IMAGE_SIZEOF_FILE_HEADER      EQU   SIZE IMAGE_FILE_HEADER

; ÄÄ´ NT_Signature value ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IMAGE_NT_SIGNATURE      EQU   00004550h   ; 'PE',0,0

IMAGE_NT_HEADERS        STRUC
    NT_Signature        DD                      ?
    NT_FileHeader       IMAGE_FILE_HEADER       ?
    NT_OptionalHeader   IMAGE_OPTIONAL_HEADER   ?
IMAGE_NT_HEADERS        ENDS
