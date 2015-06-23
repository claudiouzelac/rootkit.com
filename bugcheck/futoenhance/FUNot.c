
#include <ntddk.h>

typedef struct _EX_PUSH_LOCK
{
   union
   {
        struct
        {
        ULONG Waiting:1;
        ULONG Exclusive:1;
        ULONG Shared:30;
        };
        ULONG Value;
        PVOID Ptr;
   };

} EX_PUSH_LOCK, *PEX_PUSH_LOCK;

typedef struct _HANDLE_TRACE_DB_ENTRY
{
   CLIENT_ID    ClientId;
   HANDLE       Handle;
   ULONG        Type;
   PVOID        StackTrace[16];

} HANDLE_TRACE_DB_ENTRY; *PHANDLE_TRACE_DB_ENTRY;

typedef struct _HANDLE_TRACE_DEBUG_INFO
{
   ULONG CurrentStackIndex;
   HANDLE_TRACE_DB_ENTRY TraceDb[4096];

} HANDLE_TRACE_DEBUG_INFO, *PHANDLE_TRACE_DEBUG_INFO;

typedef struct _HANDLE_TABLE_ENTRY
{
    union
    {
        ULONG_PTR   Flags:2;
        PVOID       Object;
    };
    ULONG  NextFree;
   
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE 
{
    PVOID TableCode;
    PEPROCESS QuotaProcess;
    PVOID UniqueProcessId;
    EX_PUSH_LOCK HandleTableLock [4];
    LIST_ENTRY HandleTableList;
    EX_PUSH_LOCK HandleContentionEvent;
    PHANDLE_TRACE_DEBUG_INFO DebugInfo;
    ULONG ExtraInfoPages;
    ULONG FirstFree;
    ULONG LastFree;
    ULONG NextHandleNeedingPool;
    ULONG HandleCount;
    ULONG Flags;
} HANDLE_TABLE, *PHANDLE_TABLE;

NTSTATUS
PsLookupProcessByProcessId
(
    HANDLE Pid,
    PEPROCESS *Process
);

NTSTATUS
PsLookupThreadByThreadId
(
    IN HANDLE ThreadId,
    OUT PETHREAD *Thread
);


PVOID
GetPspCidTable()
{
    PVOID pPspCidTable = NULL;
    ULONG i;
    UNICODE_STRING  usPsLookup;
    PUCHAR Buff;
    
    RtlInitUnicodeString( &usPsLookup, L"PsLookupProcessByProcessId" );
    Buff = MmGetSystemRoutineAddress( &usPsLookup );

    if( Buff != NULL )
    {    
        for( i = 0; i < 0x40; i++, Buff++ )
        {
            if( *(PUSHORT)(Buff) == 0x35ff && *((PUCHAR)Buff+6) == 0xe8 )
            {
                pPspCidTable = (PVOID)(*(PULONG)(Buff+2));
                break;
            }
        }
    }

    return pPspCidTable ? *(PVOID*)pPspCidTable : NULL;
}

#define MAX_CID 0x41DC
#define EPROC_CID_OFFSET  0x84
#define EPROC_NAME_OFFSET 0x174 

VOID
EnumPspCidTableAndPrintInUse()
{
    PHANDLE_TABLE PspCidTable = GetPspCidTable();
    PBOOLEAN pAllocTable;
    ULONG i;
    ULONG   entryIdx;
    PHANDLE_TABLE_ENTRY entry;
        
    if( PspCidTable == NULL )
    {
        DbgPrint("PspCidTable == NULL\n");
        return;   
    }
    
    //
    // Wuss out and only support 1 page worth of handles for now
    //
    ASSERT( PspCidTable->NextHandleNeedingPool == 0x800 );    
        
    pAllocTable  
    = (PBOOLEAN)
            ExAllocatePool( PagedPool, PspCidTable->NextHandleNeedingPool );
    
    if( pAllocTable  == NULL )
    {
        DbgPrint("pAllocTable == NULL\n");
        return;
    }
    
/*        
    for( i = 0; i < PspCidTable->NextHandleNeedingPool >> 2; i++ )
    {
        pAllocTable[i] = TRUE;   
    }
    
    // first and last entry always are marked false
    pAllocTable[0] = FALSE;
    pAllocTable[i-1] = FALSE;
     
    entryIdx = PspCidTable->FirstFree >> 2;
    
    do
    {
        pAllocTable[entryIdx] = FALSE;
        entry = &(((PHANDLE_TABLE_ENTRY)PspCidTable->TableCode)[entryIdx]);
        entryIdx = entry->NextFree >> 2;
        
    } while( entry->NextFree != 0 ); // && (entry->Flags & 0x3) == 0x0 )   
    
*/    
    
    for( i = 0; i < PspCidTable->NextHandleNeedingPool >> 2; i++ )
    {
        entry = &(((PHANDLE_TABLE_ENTRY)PspCidTable->TableCode)[i]);
        if( entry->Object == 0 && entry->NextFree != 0 )
        {
            pAllocTable[i] = FALSE;        
        }
        else
        {
            pAllocTable[i] = TRUE;    
        }
    }

    for( i = 0; i < PspCidTable->NextHandleNeedingPool >> 2; i++ )
    {   
        if( pAllocTable[i] == TRUE )
        {
            NTSTATUS ntStatus;
            HANDLE Cid = (HANDLE)(i << 2);
            PEPROCESS  Process;
            PETHREAD   Thread;
            
            ntStatus = PsLookupThreadByThreadId( Cid, &Thread );           
            
            if( NT_SUCCESS(ntStatus) )
            {
                ObDereferenceObject( Thread );
            }
            else
            {
                ntStatus = PsLookupProcessByProcessId( Cid, &Process );           
                
                if( NT_SUCCESS(ntStatus) )
                {
                    DbgPrint("RUNNING PID: 0x%04x NAME: %-16s\n"
                             , Cid, ((PUCHAR)Process+EPROC_NAME_OFFSET) );
                             
                    ObDereferenceObject( Process );
                }
                else
                {
                    entry = &(((PHANDLE_TABLE_ENTRY)PspCidTable->TableCode)[i]);
                    
                    if( entry->Object == NULL )
                    {
                        DbgPrint("Unable to open CID: 0x%04x\n", Cid);   
                    }
                }
            }
        }   
    }
    
    ExFreePool( pAllocTable );
}


VOID
FUNotUnload
(
    IN PDRIVER_OBJECT pDrvObj
)
{
    // NOTHING
    return;   
}

NTSTATUS
DriverEntry
(
    IN PDRIVER_OBJECT pDrvObj,
    IN PUNICODE_STRING pRegistry 
)
{
      
    __debugbreak(); 
  
    pDrvObj->DriverUnload = FUNotUnload;

    DbgPrint( "FUNot: DriverEntry\n" );
    
    EnumPspCidTableAndPrintInUse();
    
    return STATUS_SUCCESS;
}
