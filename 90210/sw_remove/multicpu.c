/*******************************************************************
 *                      Functions for MP Systems                   *
 ******************************************************************/

/*--
Module Name:
    multicpu.cpp

Abstract: Functions to suspend/resume all secondary processors. 


Revision History:

 Sten      11/05/2003
      Initial release

--*/
extern void __declspec(dllimport) HalInitializeProcessor(int, int);

#include "multicpu.h"

///////////////////////////////////////////////////////////////////
// PUBLIC DEFINITIONS
///////////////////////////////////////////////////////////////////

DWORD mp_OldHandler      = 0;
WORD  mp_OldSelector     = 0;
BYTE  mp_OldFlag         = 0;

DWORD mp_NumOfCPUs       = 1;       // default value is 1 processor
DWORD mp_PCR_VA          = 0xFFDFF000;
DWORD *mp_PCR_VA_array   = &mp_PCR_VA;

///////////////////////////////////////////////////////////////////
// STATIC DEFINITIONS
///////////////////////////////////////////////////////////////////

static DWORD mp_spinlock           = 0;

static DWORD mp_IPI_Delivered_Flag = 0;
static DWORD mp_LocalAPIC_ID       = 0xFFFFFFFF;

static WORD  mp_SavedDS            = 0x23;
static DWORD mp_APIC_Regs_VA       = 0xFFFE0000;


static DWORD mp_OldNMIHandler      = 0;
static WORD  mp_OldNMISelector     = 0;
static BYTE  mp_OldNMIFlag         = 0;

///////////////////////////////////////////////////////////////////
// mp_Init()
//
//  Initialize multicpu module
//
///////////////////////////////////////////////////////////////////
void __declspec(naked) mp_Init()
{
    __asm
    {          
                pushad
                mov     ax, ds          ; save data selector
                mov     mp_SavedDS, ax

                call    mp_AnalyzeHalInitProcessor
/*
                mov     eax, 2          ; hook NMI interrupt on all CPUs
                mov     dl, 8Eh
                mov     edi, offset mp_NMIHook
                mov     bx, cs
                call    mp_HookInterrupt

                mov     eax, mp_OldHandler
                mov     mp_OldNMIHandler, eax

                mov     bx, mp_OldSelector
                mov     mp_OldNMISelector, bx

                mov     al, mp_OldFlag
                mov     mp_OldNMIFlag, al
*/
                popad
                retn 
    }  
}
/*
///////////////////////////////////////////////////////////////////
// mp_Done()
//
//  Unhook all hooks installed by multicpu module
//
///////////////////////////////////////////////////////////////////

void __declspec(naked) mp_Done()
{
    __asm
    {
                cmp     mp_OldNMIHandler, 0
                jz      exit_done

                pushad
                mov     eax, 2                      ; restore old NMI handler on all CPUs
                mov     dl,  mp_OldNMIFlag          ; TO DO: restore whole IDT Entry
                mov     edi, mp_OldNMIHandler
                mov     bx,  mp_OldNMISelector
                call    mp_HookInterrupt
                popad

exit_done:
                retn 
    }       
}

///////////////////////////////////////////////////////////////////
// mp_NMIHook()
//
//  My own NMI handler
//
///////////////////////////////////////////////////////////////////
void __declspec(naked) mp_NMIHook()
{
    __asm
    {    
//                call    mp_INT02_IOAPIC      
                push    word ptr  cs:[mp_OldNMISelector]
                push    dword ptr cs:[mp_OldNMIHandler]                  
                retf
    }
}
*/
///////////////////////////////////////////////////////////////////
// mp_AnalyzeHalInitProcessor()
//
//  Returns number of active CPUs in the system.
//    -  Sets mp_NumOfCPUs to the number of active processors in the
//       system.
//    -  Sets mp_PCR_VA_array to the value extracted from the HAL 
//       code.  
//
///////////////////////////////////////////////////////////////////

DWORD __declspec(naked) mp_AnalyzeHalInitProcessor()
{
    __asm
    {
                pushad
                mov     ecx, 128
                mov     edi, offset HalInitializeProcessor
                mov     edi, [edi]
                cld

search_some_bytes:                      
                mov     al, 89h
                repne scasb
                jnz     short return_default
                mov     ax, [edi]
                cmp     ax, 850Ch
                jnz     short search_some_bytes
                mov     eax, [edi+2]
                or      eax, eax
                jns     short return_default
                add     edi, 80000h
                cmp     eax, edi
                ja      short return_default
                mov     mp_PCR_VA_array, eax
                mov     edi, eax
                xor     ecx, ecx

count_CPUs:                             
                mov     eax, [edi]
                or      eax, eax
                jz      short count_end
                add     edi, 4
                inc     ecx
                jmp     short count_CPUs
; ---------------------------------------------------------------------------

count_end:                              
                jecxz   short return_default
                mov     mp_NumOfCPUs, ecx

return_default:                         
                popad
                mov     eax, mp_NumOfCPUs
                retn
    }
}

/*

///////////////////////////////////////////////////////////////////
// mp_GetLocalAPIC_ID()
//
//  Return local APIC ID for current processor.
//
///////////////////////////////////////////////////////////////////

DWORD __declspec(naked) mp_GetLocalAPIC_ID()
{
    __asm
    {
                mov     eax, mp_APIC_Regs_VA
                mov     eax, [eax+20h]             ; Read Local APIC ID
                shr     eax, 24                    ; extract ID
                mov     mp_LocalAPIC_ID, eax
                retn
    }
}

///////////////////////////////////////////////////////////////////
// mp_ResumeSecondCPUs()
//
//  Resume all secondary processors.
//
///////////////////////////////////////////////////////////////////

DWORD __declspec(naked) mp_ResumeSecondCPUs()
{
    __asm
    {
                push    eax
                mov     eax, ss:mp_spinlock
                or      eax, eax
                jz      short resume_CPUs_exit
                cmp     ax, 1
                jbe     short resume_CPUs_exit

                dec     eax
                or      ax, ax
                jz      short clear_flags
                mov     mp_spinlock, eax
                jmp     short resume_CPUs_exit
; ---------------------------------------------------------------------------

clear_flags:
                mov     mp_LocalAPIC_ID, 0FFFFFFFFh
                mov     mp_spinlock, 0

resume_CPUs_exit:
                pop     eax
                retn
    }
}

///////////////////////////////////////////////////////////////////
// mp_StopSecondCPUs()
//
//  Stop all secondary processors.
//
///////////////////////////////////////////////////////////////////

DWORD __declspec(naked) mp_StopSecondCPUs()
{
    __asm
    {
                cmp     cs:mp_NumOfCPUs, 1
                ja      short do_stop_second_cpus
                retn
; ---------------------------------------------------------------------------

do_stop_second_cpus:
                pushad
                mov     ebp, esp
                push    ds
                mov     ds, cs:[mp_SavedDS]     
                call    mp_GetLocalAPIC_ID
                mov     edi, mp_APIC_Regs_VA

arquire_spinlock:
                lock bts     mp_spinlock, 31
                jnb     short stop_CPUs
                push    ecx
                mov     ecx, 20

nothing:
                loop    nothing
                pop     ecx
                cmp     mp_LocalAPIC_ID, eax
                jnz     short wait_CPUs_counter
                inc     mp_spinlock
                jmp     short exit_stop_second_CPUs
; ---------------------------------------------------------------------------

wait_CPUs_counter:
                cmp     mp_spinlock, 0
                jnz     short wait_CPUs_counter
                jmp     short arquire_spinlock
; ---------------------------------------------------------------------------

stop_CPUs:
                mov     mp_LocalAPIC_ID, eax
                inc     mp_spinlock
                mov     mp_IPI_Delivered_Flag, 1

send_IPI:
                test    dword ptr [edi+300h], 1000h ; read interupt command register
                jnz     short send_IPI              ; wait while delivery status = 0
                mov     eax, 11000100010011111111b
                           ; 11                   = Destination shorthand (all excluding self)
                           ;   00                 = Reserved 
                           ;     0                = Trigger Mode          (edge)
                           ;      1               = Level                 (must be 1)
                           ;       0              = Reserved                 
                           ;        0             = Delivery status       (ignored)           
                           ;         0            = Destanation Mode      (physical)               
                           ;          100         = Delivery Mode         (deliver an NMI)        
                           ;             11111111 = Vector                (ignored)

                mov     [edi+300h], eax            ; write command to ICR
                mov     ecx, 20000

wait_for_stop_CPUs_flag_cleared:
                push    ecx
                mov     ecx, 20

idle:
                loop    idle
                pop     ecx
                cmp     mp_IPI_Delivered_Flag, 0
                loopne  wait_for_stop_CPUs_flag_cleared
                mov     mp_IPI_Delivered_Flag, 0

exit_stop_second_CPUs:
                pop     ds
                popad
                retn
    }
}

///////////////////////////////////////////////////////////////////
// mp_INT02_IOAPIC()
//
//  Non-maskable interrupt hook.
//
///////////////////////////////////////////////////////////////////

DWORD __declspec(naked) mp_INT02_IOAPIC()
{
    __asm
    {
                cmp     cs:mp_NumOfCPUs, 1
                jmp     short local_ret
                jbe     short local_ret
                add     esp, 4

                jmp     short many_cpus
; ---------------------------------------------------------------------------

local_ret:
                retn
; ---------------------------------------------------------------------------

many_cpus:
                pushad
                mov     ebp, esp
                push    ds
                push    es
                mov     ds, cs:mp_SavedDS
                mov     es, cs:mp_SavedDS
                mov     mp_IPI_Delivered_Flag, 0   ; send signal to StopSecondCPUs
                                                   ; IPI was delivered

                mov     edi, mp_APIC_Regs_VA
                mov     eax, [edi+80h]             ; Task Priority Register
                push    eax
                mov     dword ptr [edi+80h], 0FFh  ; Disable all interrupts except those
                                                   ; delivered with NMI, SMI, INIT, ExtINT, start-up,
                                                   ; Init-deassert delivery mode.

wait_loop:
                cmp     mp_spinlock, 0
                jnz     short wait_loop

                pop     eax                        ; EAX = Old TPR
                mov     [edi+80h], eax             ; restore Task Priority Register
                pop     es
                pop     ds
                popad
                add     esp, 4
                iretd
    }
}
*/

///////////////////////////////////////////////////////////////////
// mp_HookIDTEntry()
//
//  Hook interrupt in given IDT.
//
// INPUT:
//         ; DL  = Int flags byte
//         ; BX  = Selector
//         ; EAX = vector to hook
//         ; ESI = pointer to IDTBase
//         ; EDI = new interrupt dispatcher virtual address
///////////////////////////////////////////////////////////////////
DWORD __declspec(naked) mp_HookIDTEntry()
{
    __asm
    {
           pushad
           mov   ecx, eax
           lea   esi, [esi+eax*8]                  ; ESI = IDTEntry

           push  ecx
           movzx eax, word ptr [esi]               ; EAX = lo(OldIntr)
           mov   ecx, [esi+4]                      ; ECX = hi(OldIntr)
           and   ecx, 0FFFF0000h
           or    eax, ecx                          ; EAX = OldIntr
           pop   ecx

           mov   dword ptr [mp_OldHandler], eax    ; Save old NMI handler
//           cmp   ecx, 2
//           jnz   short not_NMI

           mov   cx, [esi+2]
           mov   word ptr [mp_OldSelector], cx
           mov   cl, [esi+5]     
           mov   byte ptr [mp_OldFlag], cl
           mov   byte ptr [esi+5], 8Eh             ; Set DPL=0, IntGate32
 
// not_NMI:
           mov   [esi], di
           shr   edi, 16                           ; EDI >>= 16
           mov   [esi+6], di
           and   byte ptr [esi+5], 9Fh             ; Set IntGate32 ??
           mov   [esi+5], dl
           mov   [esi+2], bx                       ; write selector
 
           popad
           retn
    }
}

///////////////////////////////////////////////////////////////////
// mp_HookInterrupt()
//
//  Hook interrupt on all processors in the system.
//
// INPUT:
//         ; DL  = Int flags byte
//         ; BX  = Selector
//         ; EAX = vector to hook
//         ; EDI = new interrupt dispatcher virtual address
///////////////////////////////////////////////////////////////////
static DWORD saved_CR0;

DWORD __declspec(naked) mp_HookInterrupt()
{
#define  var_IDT     -8 // (qword ptr -8)

    __asm
    {
                pushad                   
                                        
                mov     ebp, esp
                sub     esp, 8

                push    eax
                mov     eax, cr0
                mov     ds:saved_CR0, eax
                and     eax, 0FFFEFFFFh            ; Clear write protect bit
                mov     cr0, eax
                pop     eax
                or      eax, eax
                jz      short exit_hook_idt
                cmp     mp_NumOfCPUs, 1
                jbe     short hook_on_single_cpu   ; if multiprocessor system
                mov     ecx, mp_NumOfCPUs          ; hook interrupt on all
                mov     esi, mp_PCR_VA_array       ; processors
                mov     dh, 0

hook_idt_entry_loop:                              
                push    esi
                mov     esi, [esi]                 ; ESI points to KPCR
                mov     esi, [esi+38h]             ; ESI = IDTBase
                call    mp_HookIDTEntry
                pop     esi
                add     esi, 4
                inc     dh
                loop    hook_idt_entry_loop
                jmp     short exit_hook_idt
; ---------------------------------------------------------------------------

hook_on_single_cpu:                               ; hook interrupt on single
                sidt    [ebp+var_IDT]             ; cpu system
                mov     esi, dword ptr [ebp+var_IDT+2]
                call    mp_HookIDTEntry

exit_hook_idt:                          
                                        
                mov     eax, ds:saved_CR0         ; restore previous WP bit
                mov     cr0, eax
                mov     esp, ebp
                popad
                retn
    }
}


///////////////////////////////////////////////////////////////////
// mp_GetIDTBase
//
//  Get IDT Base address
//
///////////////////////////////////////////////////////////////////
DWORD __declspec(naked) mp_GetIDTBase(DWORD IdtNum)
{
    __asm
    {
                push    ebp
                mov     ebp, esp
                
                mov     eax, IdtNum
                cmp     eax, mp_NumOfCPUs
                jae     invalid_param

                push    esi
                mov     esi, mp_PCR_VA_array
                mov     eax, [esi+eax*4] ; EAX points to KPCR
                mov     eax, [eax+38h]             ; EAX = IDTBase
                pop     esi

                pop     ebp
                retn    4

invalid_param:
                xor     eax, eax
                pop     ebp
                retn    4
    }
}
