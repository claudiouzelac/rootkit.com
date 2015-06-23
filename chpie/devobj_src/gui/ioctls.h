#pragma once

#include <winioctl.h>

#define IOCTL_REQUEST_DATA     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KINTERRUPT       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_REGISTER_EVENT   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct LIST_ENTRY
{
	int a;
	int b;
};

typedef struct _KINTERRUPT {
    short				Type;
    short      	   	    Size;
    LIST_ENTRY          InterruptListEntry; // LIST_ENTRY : 8bytes
    unsigned long       ServiceRoutine;
    unsigned long       ServiceContext;
    unsigned long       SpinLock;
    unsigned long       TickCount;
    unsigned long	    ActualLock;
    unsigned long       DispatchAddress;
    unsigned long		Vector;
    unsigned char       Irql;
    unsigned char       SynchronizeIrql;
    unsigned char       FloatingSave;
    unsigned char       Connected;
    char                Number;
    unsigned char       ShareVector;
    unsigned long       Mode;               // KINTERRUPT_MODE : 4bytes
    unsigned long       ServiceCount;
    unsigned long       DispatchCount;
    unsigned long       DispatchCode[106];
} KINTERRUPT, *PKINTERRUPT;
