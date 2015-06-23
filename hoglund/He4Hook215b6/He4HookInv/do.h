typedef struct _DEVICE_OBJECT {
    CSHORT Type;                                                      // 0x0000
    USHORT Size;                                                      // 0x0002
    LONG ReferenceCount;                                              // 0x0004
    struct _DRIVER_OBJECT *DriverObject;                              // 0x0008
    struct _DEVICE_OBJECT *NextDevice;                                // 0x000C
    struct _DEVICE_OBJECT *AttachedDevice;                            // 0x0010
    struct _IRP *CurrentIrp;                                          // 0x0014
    PIO_TIMER Timer;                                                  // 0x0018
    ULONG Flags;                         // See above:  DO_...        // 0x001C
    ULONG Characteristics;               // See ntioapi:  FILE_...    // 0x0020
    PVPB Vpb;                                                         // 0x0024
    PVOID DeviceExtension;                                            // 0x0028
    DEVICE_TYPE DeviceType;                                           // 0x002C
    CCHAR StackSize;                                                  // 0x0030
    union {
        LIST_ENTRY ListEntry;
        WAIT_CONTEXT_BLOCK Wcb;
    } Queue;
    ULONG AlignmentRequirement;
    KDEVICE_QUEUE DeviceQueue;
    KDPC Dpc;

    //
    //  The following field is for exclusive use by the filesystem to keep
    //  track of the number of Fsp threads currently using the device
    //

    ULONG ActiveThreadCount;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    KEVENT DeviceLock;

    USHORT SectorSize;                                                // 0x00ac
    USHORT Spare1;                                                    // 0x00ae

    struct _DEVOBJ_EXTENSION  *DeviceObjectExtension;                 // 0x00B0
    PVOID  Reserved;
} DEVICE_OBJECT;


for driverobject

NTSTATUS NTOSKRNL
    ObCreateObject
     (
	KPROCESSOR_MODE bMode,           // kernel / user = 0
	POBJECT_TYPE Type,               // Типовой объект = IoDriverObjectType
	POBJECT_ATTRIBUTES Attributes,   // Аттрибуты {0x18, 0, {\Driver\Name}, 0x10, 0, 0}
	BOOLEAN bObjectMode,             // Тип объекта kernel/user = 0
	DWORD Reserved,                  // не используется функцией = 0
	DWORD BodySize,                  // размер тела объекта = 0xbc
	DWORD PagedPoolQuota OPTIONAL,   // если 0 = 0
	DWORD NonPagedPoolQuota OPTIONAL,// то наследуется = 0
	PVOID* pObjectBody               // возвращаемый указатель на тело.
     );


NTSTATUS NTOSKRNL
	ObInsertObject(
		PVOID pObject,                      //Тело
		PACCESS_STATE pAccessState OPTIONAL,  // = 0
		ACCESS_MASK Access,                   // = 1
		DWORD RefCounterDelta OPTIONAL,   //0- default (т.е. 1) = 0
		PVOID  OUT *ObjectExist OPTIONAL, //Если уже существует = 0
		PHANDLE OUT Handle                //хэндл
	      	);
