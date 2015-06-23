;
;                       . .: .:.. :.. .. .:.::. :. ..:
;                     <<-==млллллм=млллллм=млллллм===<
;                      .:: ллл ллл:ллл ллл.ллл ллл .:.
;                      . .:.мммллп.плллллл.ллллллл:..
;                       ...лллмммм:ммммллл:ллл ллл.::.
;                      >===ллллллл=ллллллп=ллл ллл=->>
;                     .: .:.. ..:. .: ..:.::. ::.. :.:.
;
;                              [29A INC files]
;                        DOS EXE MZ executable format
;                            by Jacky Qwerty/29A
;
; Description
; ФФФФФФФФФФФ
; This include file contains all the constantz and structurez needed to work
; with the DOS EXE MZ executable format inside ASM filez. For use with TASM,
; of course (also with TASM32). MASM sucks.. :P
;
; Disclaimer
; ФФФФФФФФФФ
; This file was built up by Jacky Qwerty from 29A. The author is not respon-
; sible for any problemz caused due to use/misuse of this file.
;
;
; (c) 1997. No rightz reserved. Use without permision >8P.


; ФФД MZ_magic value УФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФФ

IMAGE_DOS_SIGNATURE     EQU   5A4Dh ;'MZ'

IMAGE_DOS_HEADER  STRUC
    MZ_magic      DW    ?           ; Magic number
    MZ_cblp       DW    ?           ; Bytes on last page of file
    MZ_cp         DW    ?           ; Pages in file
    MZ_crlc       DW    ?           ; Relocations
    MZ_cparhdr    DW    ?           ; Size of header in paragraphs
    MZ_minalloc   DW    ?           ; Minimum extra paragraphs needed
    MZ_maxalloc   DW    ?           ; Maximum extra paragraphs needed
    MZ_ss         DW    ?           ; Initial (relative) SS value
    MZ_sp         DW    ?           ; Initial SP value
    MZ_csum       DW    ?           ; Checksum
    MZ_ip         DW    ?           ; Initial IP value
    MZ_cs         DW    ?           ; Initial (relative) CS value
    MZ_lfarlc     DW    ?           ; File address of relocation table
    MZ_ovno       DW    ?           ; Overlay number
    MZ_res        DW    4 DUP (?)   ; Reserved words
    MZ_oemid      DW    ?           ; OEM identifier (for e_oeminfo)
    MZ_oeminfo    DW    ?           ; OEM information; e_oemid specific
    MZ_res2       DW    10 DUP (?)  ; Reserved words
    MZ_lfanew     DD    ?           ; File address of new exe header
IMAGE_DOS_HEADER  ENDS

IMAGE_SIZEOF_DOS_HEADER EQU   SIZE  IMAGE_DOS_HEADER
