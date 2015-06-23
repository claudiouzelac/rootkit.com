;//////////////////////////////////////////////////////////////////////////////
;//
;// .:[kutil.inc]:.
;//
;// Useful kernel functions to hook SSDT
;//
;// by Edgar Minglini Barbosa (Opc0de)
;//////////////////////////////////////////////////////////////////////////////
;//
;// Example:
;// --------------------------------------------
;// Hook the SSDT Shadow with function ID = 0x225
;//
;// invoke GetKeServiceDescriptorTable
;// invoke HookSSDTfunction, 0225h, addr NewSetWindowsHookFunction
;// mov originalSetWindowsHook, eax
;//
;//////////////////////////////////////////////////////////////////////////////


;------------------------------------------------------------------------------
; Prototypes
;------------------------------------------------------------------------------
EnablePP				PROTO
DisablePP				PROTO
GetKeServiceDescriptorTable		PROTO
HookSSDTfunction			PROTO :DWORD, :DWORD

.code
;------------------------------------------------------------------------------
; Desativa a proteção de página WP (Write Protection)
;------------------------------------------------------------------------------
DisablePP proc
	push eax
	mov eax, CR0
	or eax, NOT 0FFFEFFFFh
	mov CR0, eax
	pop eax
	ret
DisablePP endp

;------------------------------------------------------------------------------
; Ativa a proteção de página WP (Write Protection)
;------------------------------------------------------------------------------
EnablePP proc
	push eax
	mov eax, CR0
	and eax, 0FFFEFFFFh
	mov CR0, eax
	pop eax
	ret
EnablePP endp

;------------------------------------------------------------------------------
; Retorna em EAX o endereço da tabela KeServiceDescriptorTable
;------------------------------------------------------------------------------
GetKeServiceDescriptorTable proc
	mov eax, [KeServiceDescriptorTable]
	mov eax, [eax]
	ret
GetKeServiceDescriptorTable endp


;------------------------------------------------------------------------------
; Cria o hook da função ServiceID localizada dentro da
; tabela KeServiceDescriptorTable
;------------------------------------------------------------------------------
; ServiceId: número do serviço
; HookFunctionAddr: endereço da função de hook
;
; Retorna em EAX o endereço da função original
;------------------------------------------------------------------------------
HookSSDTfunction proc uses ebx ecx edx , ServiceID:DWORD, HookFunctionAddr:DWORD
	invoke GetKeServiceDescriptorTable
	mov eax, [eax]
	mov ebx, ServiceID
	shl ebx, 2
	mov ecx, [eax+ebx]
	invoke DisablePP
	cli
	mov edx, HookFunctionAddr
	mov dword ptr [eax+ebx], edx
	invoke EnablePP
	sti	
	mov eax, ecx
	ret
HookSSDTfunction endp
