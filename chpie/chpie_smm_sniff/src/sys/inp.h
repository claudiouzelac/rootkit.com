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

typedef unsigned long int 
PCI_CONFIGURATION_PACKET, * PPCI_CONFIGURATION_PACKET;

typedef unsigned long int
USB_LEGACY_REGISTER, * PUSB_LEGACY_REGISTER;

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
	ULONG	StartIoFlag;
	PVOID	Vpb;
} R_DEVOBJ_EXTENSION, *PR_DEVOBJ_EXTENSION;

typedef struct _DEVICE_EXTENSION
{
	KEVENT   kill;
	PKTHREAD hThread;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
