/*
 * real _DEVOBJ_EXTENSION structure
 * 
   +0x000 Type             : Int2B
   +0x002 Size             : Uint2B
   +0x004 DeviceObject     : Ptr32 _DEVICE_OBJECT
   +0x008 PowerFlags       : Uint4B
   +0x00c Dope             : Ptr32 _DEVICE_OBJECT_POWER_EXTENSION
   +0x010 ExtensionFlags   : Uint4B
   +0x014 DeviceNode       : Ptr32 Void
   +0x018 AttachedTo       : Ptr32 _DEVICE_OBJECT
   +0x01c StartIoCount     : Int4B
   +0x020 StartIoKey       : Int4B
   +0x024 StartIoFlags     : Uint4B
   +0x028 Vpb              : Ptr32 _VPB
 */
typedef struct _R_DEVOBJ_EXTENSION
{
	CSHORT Type;
	USHORT Size;
	PDEVICE_OBJECT DeviceObject;
	ULONG   PowerFlags;
	PVOID Dope;
	ULONG	ExtensionFlags;
	PVOID	DeviceNode;
	PDEVICE_OBJECT AttachedTo;
	ULONG	StartIoCount;
	ULONG	StartIoKey;
	PVOID	Vpb;
} R_DEVOBJ_EXTENSION, *PR_DEVOBJ_EXTENSION;
//
// Define the common portion of the keyboard/mouse device extension.
//
typedef struct _PORT_KEYBOARD_EXTENSION {
    //
    // Pointer back to the this extension's device object.
    //
    PDEVICE_OBJECT      Self;
 
    PKINTERRUPT InterruptObject;

} PORT_KEYBOARD_EXTENSION, *PPORT_KEYBOARD_EXTENSION;

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

