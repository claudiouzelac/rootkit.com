TRUE            = 1
FALSE           = 0
NULL            = 0
extrn           NtOpenSection:proc
extrn           NtMapViewOfSection:proc
extrn           NtUnmapViewOfSection:proc
extrn		NtQuerySystemInformation:proc
extrn           GetSecurityInfo:proc
extrn           SetEntriesInAclW:proc
extrn           SetSecurityInfo:proc
extrn           LocalFree:proc
extrn           CloseHandle:proc
extrn           LoadLibraryExA:proc
extrn           GetSystemInfo:proc
extrn		VirtualAlloc:proc
extrn		VirtualFree:proc
extrn		lstrcmpiA:proc
extrn		GetProcAddress:proc
extrn		VirtualProtect:proc
extrn		VirtualLock:proc
extrn		VirtualUnlock:proc
extrn		GetVersionExA:proc

EP_ET_OFFSETS_dw	struc
	dwEP_ActiveProcessLinks	dw	?
	dwEP_UniqueProcessId	dw	?
	dwEP_ImageFileName	dw	?
	dwEP_SAPCI_ImageFileName	dw	?
	dwEP_Pcb_ThreadListHead	dw	?	; in KPROCESS
	dwET_Cid_UniqueProcess	dw	?
	dwET_Tcb_ThreadListEntry	dw	?	; in KTHREAD	
EP_ET_OFFSETS_dw	ends

EP_ET_OFFSETS_dd	struc
	EP_ActiveProcessLinks	dd	?
	EP_UniqueProcessId	dd	?
	EP_ImageFileName	dd	?
	EP_SAPCI_ImageFileName	dd	?	; for xp and above
	EP_Pcb_ThreadListHead	dd	?	; in KPROCESS
	ET_Cid_UniqueProcess	dd	?
	ET_Tcb_ThreadListEntry	dd	?	; in KTHREAD
EP_ET_OFFSETS_dd	ends

R0_DATA	struc
	szNewImageFileName	db	16 dup (?)
; some required EPROCESS and ETHREAD offsets
	offs			EP_ET_OFFSETS_dd	<>
	pMmIsAddressValid	dd	?
R0_DATA	ends

SystemModuleInformation	=11


SYSTEM_MODULE_INFORMATION	struc 
	smi_Reserved	dd	2 dup (?)
	smi_Base		dd	?
	smi_Size		dd	?
	smi_Flags		dd	?
	smi_Index		dw	?
	smi_Unknown		dw	?
	smi_LoadCount	dw	?
	smi_ModuleNameOffset	dw	?
	smi_ImageName	db	256 dup (?)
SYSTEM_MODULE_INFORMATION	ends

OSVERSIONINFOA STRUCT
  dwOSVersionInfoSize   DWORD      ?
  dwMajorVersion        DWORD      ?
  dwMinorVersion        DWORD      ?
  dwBuildNumber         DWORD      ?
  dwPlatformId          DWORD      ?
  szCSDVersion          BYTE 128 dup (?)
OSVERSIONINFOA ENDS

OSVERSIONINFO  equ  <OSVERSIONINFOA>

;TRUSTEE struc
;	pMultipleTrustee	dd	?
;	MultipleTrusteeOperation	dd	?
;	TrusteeForm		dd	?
;	TrusteeType		dd	?
;	ptstrName		dd	?
;TRUSTEE ends

EXPLICIT_ACCESS	struc
	grfAccessPermissions	dd	?
	grfAccessMode		dd	?
	grfInheritance		dd	?
;	Trustee			TRUSTEE	<>
	pMultipleTrustee	dd	?
	MultipleTrusteeOperation	dd	?
	TrusteeForm		dd	?
	TrusteeType		dd	?
	ptstrName		dd	?
EXPLICIT_ACCESS	ends

OBJECT_ATTRIBUTES	struc
	oaLength		dd	?
	oaRootDirectory		dd	?
	oaObjectName		dd	?
	oaAttributes		dd	?
	oaSecurityDescriptor	dd	?
	oaSecurityQualityOfService	dd	?
OBJECT_ATTRIBUTES	ends

UNICODE_STRING	struc
	usLength		dw	?
	usMaximumLength		dw	?
	usBuffer		dd	?
UNICODE_STRING	ends

SYSTEM_INFO STRUCT
	wProcessorArchitecture	dw	?
	wReserved		dw	?
	dwPageSize		dd	?
	lpMinimumApplicationAddress	dd	?
	lpMaximumApplicationAddress	dd	?
	dwActiveProcessorMask	dd	?
	dwNumberOfProcessors	dd	?
	dwProcessorType		dd	?
	dwAllocationGranularity	dd	?
	wProcessorLevel		dw	?
	wProcessorRevision	dw	?
SYSTEM_INFO ENDS

MAXIMUM_SUPPORTED_EXTENSION equ 512
SIZE_OF_80387_REGISTERS equ 80

FLOATING_SAVE_AREA STRUCT
 ControlWord dd ?
 StatusWord dd ?
 TagWord dd ?
 ErrorOffset dd ?
 ErrorSelector dd ?
 DataOffset dd ?
 DataSelector dd ?
 RegisterArea BYTE SIZE_OF_80387_REGISTERS dup(?)
 Cr0NpxState dd ?
FLOATING_SAVE_AREA ENDS

CONTEXT_i386 equ 10000h
CONTEXT_i486 equ 10000h
CONTEXT_CONTROL equ CONTEXT_i386 OR 00000001h
CONTEXT_INTEGER equ CONTEXT_i386 OR 00000002h
CONTEXT_EXTENDED_REGISTERS equ CONTEXT_i386 OR 0000002h
CONTEXT_SEGMENTS equ CONTEXT_i386 OR 00000004h
CONTEXT_FLOATING_POINT equ CONTEXT_i386 OR 00000008h
CONTEXT_DEBUG_REGISTERS equ CONTEXT_i386 OR 00000010h
CONTEXT_FULL equ CONTEXT_CONTROL OR CONTEXT_INTEGER OR CONTEXT_SEGMENTS


CONTEXT STRUCT
 ContextFlags dd ?
 iDr0 dd ?
 iDr1 dd ?
 iDr2 dd ?
 iDr3 dd ?
 iDr6 dd ?
 iDr7 dd ?
 FloatSave FLOATING_SAVE_AREA <>
 regGs dd ?
 regFs dd ?
 regEs dd ?
 regDs dd ?
 regEdi dd ?
 regEsi dd ?
 regEbx dd ?
 regEdx dd ?
 regEcx dd ?
 regEax dd ?
 regEbp dd ?
 regEip dd ?
 regCs dd ?
 regFlag dd ?
 regEsp dd ?
 regSs dd ?
 ExtendedRegisters db MAXIMUM_SUPPORTED_EXTENSION dup(?)
CONTEXT ENDS

STATUS_INFO_LENGTH_MISMATCH	=0C0000004h
MEM_COMMIT			=1000h
MEM_RELEASE			=8000h

SECTION_MAP_WRITE		=2
GRANT_ACCESS			=1
NO_INHERITANCE			=0
NO_MULTIPLE_TRUSTEE		=0
TRUSTEE_IS_NAME			=1
TRUSTEE_IS_USER			=1
OBJ_CASE_INSENSITIVE		=40h
WRITE_DAC			=40000h
READ_CONTROL			=20000h
SE_KERNEL_OBJECT		=6
DACL_SECURITY_INFORMATION	=4
PAGE_READWRITE			=4
ViewShare			=1
DONT_RESOLVE_DLL_REFERENCES	=1
PAGE_EXECUTE_READWRITE		=40h
STANDARD_RIGHTS_REQUIRED	=0F0000h


unicode                 macro page,string,zero
                        irpc c,<string>
                        db '&c', page
                        endm
                        ifnb <zero>
                        dw zero
                        endif
endm

; (c) Jacky Qwerty
Pushad_struc            struc
        Pushad_edi      dd      ?
        Pushad_esi      dd      ?
        Pushad_ebp      dd      ?
        Pushad_esp      dd      ?
        Pushad_ebx      dd      ?
        Pushad_edx      dd      ?
        Pushad_ecx      dd      ?
        Pushad_eax      dd      ?
Pushad_struc            ends

cPushad         equ     size Pushad_struc
