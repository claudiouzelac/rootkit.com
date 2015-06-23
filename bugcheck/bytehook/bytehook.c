/*
 *  The following demonstrates a few things which attempt to evade some hook
 *  detection methods.
 *
 *  bugcheck.org 
 *  Feb 11, 2006
 *
 */ 

#include <ntddk.h>

#include "ntoskrnl.h"

////////////////////////////////////////////////////////////////////////////////
// INTERNALS FORWARDS
////////////////////////////////////////////////////////////////////////////////

NTSTATUS 
NTAPI
NtTerminateProcessHook
(
    IN  PHANDLE     ProcessHandle, OPTIONAL
    IN  NTSTATUS    ExitStatus
);


NTSTATUS
NTAPI
NtCreateFileHook
(
    OUT PHANDLE             FileHandle,
    IN  ACCESS_MASK         DesiredAccess,
    IN  POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN  PLARGE_INTEGER      AllocationSize, OPTIONAL
    IN  ULONG               FileAttributes,
    IN  ULONG               ShareAccess,
    IN  ULONG               CreateDisposition,
    IN  ULONG               CreateOptions,
    IN  PVOID               EaBuffer, OPTIONAL
    IN  ULONG               EaLength
);

NTSTATUS
NTAPI
NtOpenFileHook
(
    PHANDLE             FileHandle,
    ACCESS_MASK         DesiredAccess,
    POBJECT_ATTRIBUTES  ObjectAttributes,
    PIO_STATUS_BLOCK    IoStatusBlock,
    ULONG               ShareAccess,
    ULONG               OpenOptions
);


////////////////////////////////////////////////////////////////////////////////
// GLOBALS
////////////////////////////////////////////////////////////////////////////////

NT_TERMINATE_PROCESS    pNtTerminateProcess = NULL;
NT_CREATE_FILE          pNtCreateFile       = NULL;
NT_OPEN_FILE            pNtOpenFile         = NULL;

PVOID                   pSavedIntHandler    = NULL;

////////////////////////////////////////////////////////////////////////////////
// INTERNAL FUNCTIONS
////////////////////////////////////////////////////////////////////////////////


// ASM 32bit only
__declspec(naked)
PVOID
ByteHookIntHandler()
{ __asm{
    
    pop     eax
    add     esp,8
    sti
    
    cmp     eax,[pNtCreateFile]
    jnz     NotNtCreateFile
    jmp     [NtCreateFileHook]
NotNtCreateFile:

    cmp     eax,[pNtTerminateProcess]
    jnz     NotNtTerminateProcess
    jmp     [NtTerminateProcessHook]
NotNtTerminateProcess:
    
    jmp     [pSavedIntHandler]
}}

// ASM 32bit only
__declspec(naked)
PVOID
set_entry_idt
(
    IN  UCHAR   Vector,
    IN  PVOID   Address
)
{__asm{
    sub     esp,8
    sidt    [esp]
    mov     eax,[esp+2]
    add     esp,8
    mov     ecx,[esp+4]
    lea     eax,[eax+ecx*8]
    mov     ecx,[esp+8]
    cli
    mov     edx,[eax+6]
    shl     edx,0x10
    mov     dx,[eax+6]
    mov     [eax],cx
    shr     ecx,0x10
    mov     [eax+6],cx
    sti
    mov     eax,edx
    retn    8
}}


NTSTATUS 
NTAPI
NtTerminateProcessHook
(
    IN  PHANDLE     ProcessHandle, OPTIONAL
    IN  NTSTATUS    ExitStatus  
)
{
    
    KdPrint(( "[BYTEHOOK] Process: %p killing handle %p\n",
                PsGetCurrentProcess(), ProcessHandle ));
    
    return pNtTerminateProcess( ProcessHandle, 
                                ExitStatus );
}


NTSTATUS
NTAPI
NtCreateFileHook
(
    OUT PHANDLE             FileHandle,
    IN  ACCESS_MASK         DesiredAccess,
    IN  POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT PIO_STATUS_BLOCK    IoStatusBlock,
    IN  PLARGE_INTEGER      AllocationSize,
    IN  ULONG               FileAttributes,
    IN  ULONG               ShareAccess,
    IN  ULONG               CreateDisposition,
    IN  ULONG               CreateOptions,
    IN  PVOID               EaBuffer,
    IN  ULONG               EaLength
)
{
    
    KdPrint(( "[BYTEHOOK] CreateFile: %wZ\n", ObjectAttributes ? \
                        ObjectAttributes->ObjectName : NULL  ));
    
    return pNtCreateFile(   FileHandle,
                            DesiredAccess,
                            ObjectAttributes,
                            IoStatusBlock,
                            AllocationSize,
                            FileAttributes,
                            ShareAccess,
                            CreateDisposition,
                            CreateOptions,
                            EaBuffer,
                            EaLength );
}

NTSTATUS
NTAPI
NtOpenFileHook
(
    PHANDLE             FileHandle,
    ACCESS_MASK         DesiredAccess,
    POBJECT_ATTRIBUTES  ObjectAttributes,
    PIO_STATUS_BLOCK    IoStatusBlock,
    ULONG               ShareAccess,
    ULONG               OpenOptions
)
{
    KdPrint(( "[BYTEHOOK] OpenFile:  %wZ\n", ObjectAttributes ? \
                        ObjectAttributes->ObjectName : NULL  ));
    
    return pNtOpenFile( FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        ShareAccess,
                        OpenOptions );
}


//
// Its probebly easier to just kill the mem protection bit
// in cr0 but thats not as safe/portable
//
BOOLEAN
RtlCopyBytesProtected
(
    IN  PVOID   DestBuffer,
    IN  PVOID   SrcBuffer,
    IN  ULONG   Length
)
{
    BOOLEAN     bRtn = FALSE;    
    PMDL        pMdl;
    PUCHAR      pBuffer;
    
    if( DestBuffer == NULL || SrcBuffer == NULL || Length == 0 ) return bRtn;
    
    pMdl = IoAllocateMdl( DestBuffer, Length, FALSE, FALSE, NULL );
    
    if( pMdl != NULL )
    {
        MmBuildMdlForNonPagedPool( pMdl );
        
        pBuffer = MmMapLockedPages( pMdl, KernelMode );
        
        if( pBuffer != NULL )
        {
            RtlCopyBytes( pBuffer, SrcBuffer, Length );
            bRtn = TRUE;
            
            MmUnmapLockedPages( pBuffer, pMdl );
        }
        
        IoFreeMdl( pMdl );
    }
    
    return bRtn;
}

//
// needed to do a one shot write at the USHORT vs the byte copy which is unsafe
//
BOOLEAN
RtlCopyUShortProtected
(
    IN  PVOID   DestBuffer,
    IN  USHORT  UShort
)
{
    BOOLEAN     bRtn = FALSE;    
    PMDL        pMdl;
    PUCHAR      pBuffer;
    
    if( DestBuffer == NULL ) return bRtn;
    
    pMdl = IoAllocateMdl( DestBuffer, 2, FALSE, FALSE, NULL );
    
    if( pMdl != NULL )
    {
        MmBuildMdlForNonPagedPool( pMdl );
        
        pBuffer = MmMapLockedPages( pMdl, KernelMode );
        
        if( pBuffer != NULL )
        {
            *(PUSHORT)pBuffer = UShort;
            bRtn = TRUE;
            
            MmUnmapLockedPages( pBuffer, pMdl );
        }
        
        IoFreeMdl( pMdl );
    }
    
    return bRtn;
}


//
// This is pretty cool. Could even update drivers IAT entry to use new
// value instead of using the saved ptr. That way this driver is excempt
// from its own hooks.
//

BOOLEAN
UninstallHotpatch
(
    IN OUT PVOID *pOriginalAddress
)
{
    BOOLEAN bRtn;
    
    if( pOriginalAddress == NULL
     || *pOriginalAddress == NULL ) return FALSE;
    
    ASSERT( *((PUSHORT)*pOriginalAddress)-1 != 0xff8b );
    
    if( *(PUSHORT)*pOriginalAddress != 0xff8b
     && RtlCopyUShortProtected( ((PUCHAR)*pOriginalAddress)-2, 0xff8b ) )
    {    
        ((ULONG_PTR)*pOriginalAddress) -= 2;
        bRtn = TRUE;
    }
    else
    {
        bRtn = FALSE;  
    }    

    return bRtn;
}


BOOLEAN
InstallHotpatch
(
    IN OUT PVOID *pOriginalAddress, 
    IN     PVOID HookAddress
)
{
    BOOLEAN     bRtn;
    ULONG_PTR   offset;
    
    if( pOriginalAddress == NULL 
     || *pOriginalAddress == NULL
     || HookAddress == NULL ) return FALSE;
    
    ASSERT( *(PUSHORT)*pOriginalAddress == 0xff8b );
    
    offset = (ULONG_PTR)HookAddress -(ULONG_PTR)*pOriginalAddress;
    
    if( *(PUSHORT)*pOriginalAddress == 0xff8b 
     && RtlCopyBytesProtected( ((PUCHAR)*pOriginalAddress)-5, "\xe9", 1 )
     && RtlCopyBytesProtected( ((PUCHAR)*pOriginalAddress)-4, &offset,
                                    sizeof(ULONG) ) )
    {
        ((ULONG_PTR)*pOriginalAddress) += 2;
        
        if( RtlCopyUShortProtected( (PUCHAR)*pOriginalAddress-2, 0xf9eb ) )
        {    
            bRtn = TRUE;
        }
        else
        {
            ((ULONG_PTR)*pOriginalAddress) -= 2;
            bRtn = FALSE;  
        }    
    }

    return bRtn;
}


BOOLEAN
UninstallOneByteHooks()
{
    BOOLEAN     bRtn = TRUE;
    LONG        i;
    KAFFINITY   savedAffinity;
    KAFFINITY   affinity;
    PVOID       pAddr = *(NT_CREATE_PROCESS*)(*KeServiceDescriptorTable+0x2f );

    if( pNtTerminateProcess  != NULL )
    {
        *((PUCHAR)pNtTerminateProcess-2) = 0x8b;
    }
    
    if( pNtCreateFile != NULL )
    {   
        *((PUCHAR)pNtCreateFile-2) = 0x8b;
    }

    if( pSavedIntHandler != NULL )
    {
    //
    // save this threads affinity mask before playing with it
    // HACK: should use ZwQueryInformationProcess() =)
    //
    
    savedAffinity = *(KAFFINITY*)((PUCHAR)PsGetCurrentThread()+0x124);
    
    //
    // Hook IDT For all processors
    //
    
    for( i = 0, affinity = 1; i < KeNumberProcessors; i++, affinity <<= 1 )
    {
        KeSetAffinityThread( (PKTHREAD)PsGetCurrentThread(), affinity );
        // should return the same for every iteration
        (VOID)set_entry_idt( 0xff, pSavedIntHandler );
    }
    
    //
    // restore original thread affinity mask
    //
        
    *(KAFFINITY*)((PUCHAR)PsGetCurrentThread()+0x124) = savedAffinity;
    
    pSavedIntHandler = NULL;
    
    }
    
    // VERY SMALL race condition here
    RtlCopyBytesProtected( (PUCHAR)pAddr-5, "\x90\x90\x90\x90\x90", 5 );
       
    return bRtn;
}

VOID 
InstallGlobalIntHandler()
{
    PVOID       pAddr = *(NT_CREATE_PROCESS*)(*KeServiceDescriptorTable+0x2f );
    ULONG_PTR   offset;

#ifndef SVV22

    if( *((PUCHAR)pAddr-5) == 0x90 || *((PUCHAR)pAddr-5) == 0xcc )
    {
        offset = (ULONG_PTR)ByteHookIntHandler - (ULONG_PTR)pAddr;
        
        if( ! RtlCopyBytesProtected( (PUCHAR)pAddr-5, "\xe9", 1 )
        ||  ! RtlCopyBytesProtected( (PUCHAR)pAddr-4, &offset, sizeof(ULONG) ));
        {
            // nil
        } 
    }
    
    // should return the same for every iteration
    pSavedIntHandler = set_entry_idt( 0xff, (PUCHAR)pAddr-5 );
    
#else

    // should return the same for every iteration
    pSavedIntHandler = set_entry_idt( 0xff, ByteHookIntHandler );
    
#endif //SVV22
    
}

BOOLEAN
InstallOneByteHooks()
{
    BOOLEAN     bRtn = TRUE;
    PULONG      pTable;
    LONG        i;
    KAFFINITY   savedAffinity;
    KAFFINITY   affinity;
    
    //
    // XPSP2 specific
    //
    
    pNtTerminateProcess = *(NT_TERMINATE_PROCESS*)
                                            (*KeServiceDescriptorTable + 0x101);
    pNtCreateFile       = *(NT_CREATE_FILE*)(*KeServiceDescriptorTable + 0x25 );
    
    //
    // should be mov edi,edi
    //
    
    ASSERT( (*(PUSHORT)pNtTerminateProcess) == 0xff8b );
    ASSERT( (*(PUSHORT)pNtCreateFile) == 0xff8b );
    
    //
    // save this threads affinity mask before playing with it
    // HACK: should use ZwQueryInformationProcess() =)
    //
    
    savedAffinity = *(KAFFINITY*)((PUCHAR)PsGetCurrentThread()+0x124);
    
    //
    // Hook IDT For all processors
    //
    
    for( i = 0, affinity = 1; i < KeNumberProcessors; i++, affinity <<= 1 )
    {
        KeSetAffinityThread( (PKTHREAD)PsGetCurrentThread(), affinity );
        InstallGlobalIntHandler();
    }
    
    //
    // restore original thread affinity mask
    //
        
    *(KAFFINITY*)((PUCHAR)PsGetCurrentThread()+0x124) = savedAffinity;
        
    //
    // A little bit of a fixup to skip the int 0xFF which will be
    // the original function minus the nop and as well as the 
    // return address in the trap frame which we use.
    //
    (PUCHAR)pNtTerminateProcess = (PUCHAR)pNtTerminateProcess + 2;
    (PUCHAR)pNtCreateFile       = (PUCHAR)pNtCreateFile + 2;

    //
    // Overwrite mod edi,edi with 0xCD, the 'int' opcode making int 0xFF
    //

    *((PUCHAR)pNtTerminateProcess-2)= 0xCD;
    *((PUCHAR)pNtCreateFile-2)      = 0xCD;
    
    return bRtn;
}



////////////////////////////////////////////////////////////////////////////////
// COMMON DRIVER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
VOID
DriverUnload
(
    IN PDRIVER_OBJECT pDrvObj
)
{
    
#ifndef SVV22    

    if( ! UninstallHotpatch( (PVOID*)&pNtOpenFile ) )
    {
        KdPrint(( "[BYTEHOOK] Unable to uninstall hotpatch for NtOpenFile.\n" ));
    }
    
#endif //SVV22
    
    (VOID)UninstallOneByteHooks();
    
    return;   
}

NTSTATUS
DriverEntry
(
    IN PDRIVER_OBJECT   pDrvObj,
    IN PUNICODE_STRING  pRegistry 
)
{
    PDEVICE_OBJECT  pDeviceObj = NULL;
      
    // __debugbreak();
  
    pDrvObj->DriverUnload = DriverUnload;

    KdPrint(( "[BYTEHOOK]: DriverEntry\n" ));

#ifndef SVV22

    //
    // Install hotpatch
    //
    pNtOpenFile = *(NT_OPEN_FILE*)(*KeServiceDescriptorTable + 0x74 );
    
    if( ! InstallHotpatch( (PVOID*)&pNtOpenFile, NtOpenFileHook ) )
    {
        KdPrint(( "[BYTEHOOK] Unable to install hotpatch for NtOpenFile.\n" ));
    }
    
#endif //SVV22
    
    InstallOneByteHooks();
    
    
    return STATUS_SUCCESS;
}
