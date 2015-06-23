#ifndef _I386_H
#define _I386_H
#pragma pack(1)
/* sidt instruction stores the base and limit of IDTR in this format */
typedef struct idtr {
        short Limit;
        unsigned int Base;
} Idtr_t, *PIdtr_t;

/* Decriptor Entry corresponding to interrupt gate */
typedef struct idtentry {
        unsigned short OffsetLow;
        unsigned short Selector;
        unsigned char Reserved;
        unsigned char Type:4;
        unsigned char Always0:1;
        unsigned char Dpl:2;
        unsigned char Present:1;
        unsigned short OffsetHigh;
} IdtEntry_t, *PIdtEntry_t;


typedef struct _KINTERRUPT {
    CSHORT		Type;
    CSHORT      	Size;
    LIST_ENTRY          InterruptListEntry;
    ULONG               ServiceRoutine;
    ULONG               ServiceContext;
    KSPIN_LOCK          SpinLock;
    ULONG               TickCount;
    PKSPIN_LOCK         ActualLock;
    PVOID               DispatchAddress;
    ULONG	        Vector;
    KIRQL               Irql;
    KIRQL               SynchronizeIrql;
    BOOLEAN             FloatingSave;
    BOOLEAN             Connected;
    CHAR                Number;
    UCHAR                ShareVector;
    KINTERRUPT_MODE     Mode;
    ULONG               ServiceCount;
    ULONG               DispatchCount;
    ULONG               DispatchCode[106];
} KINTERRUPT, *PKINTERRUPT;

#pragma pack()
#endif
