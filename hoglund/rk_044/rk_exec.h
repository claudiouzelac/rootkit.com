#ifndef _EXEC_H_
#define _EXEC_H_

#define SEC_IMAGE         0x1000000     

typedef enum _SECTION_INFORMATION_CLASS{
	SectionBasicInformation,
	SectionImageInformation
}SECTION_INFORMATION_CLASS;

typedef struct _SECTION_IMAGE_INFORMATION{
	PVOID	EntryPoint;
	ULONG	Unknown1;
	ULONG	StackReserve;
	ULONG	StackCommit;
	ULONG	SubSystem;
	USHORT	MinorSubSystemVersion;
	USHORT	MajorSubSystemVersion;
	ULONG	Unknown2;
	ULONG	Characteristics;
	USHORT	ImageNumber;
	BOOLEAN	Executable;
	UCHAR	Unknown3;
	ULONG	Unknown4[3];
}SECTION_IMAGE_INFORMATION,*PSECTION_IMAGE_INFORMATION;

typedef struct _USER_STACK{
	PVOID	FixedStackBase;
	PVOID	FixedStackLimit;
	PVOID	ExpandableStackBase;
	PVOID	ExpandableStackLimit;
	PVOID	ExpandableStackBottom;
}USER_STACK,*PUSER_STACK;


typedef struct _PROCESS_PARAMETERS{
	ULONG	AllocationSize;
	ULONG	Size;
	ULONG	Flags;
	ULONG	Reserved;
	LONG	Console;
	ULONG	ProcessGroup;
	HANDLE	hStdInput;
	HANDLE	hStdOutput;
	HANDLE	hStdError;
	UNICODE_STRING	CurrentDirectoryName;
	HANDLE			CurrentDirectoryHandle;
	UNICODE_STRING	DllPath;
	UNICODE_STRING	ImageFile;
	UNICODE_STRING	CommandLine;
	PWSTR	Environment;
	ULONG	dwX;
	ULONG	dwY;
	ULONG	dwXSize;
	ULONG	dwYSize;
	ULONG	dwXCountChars;
	ULONG	dwYCountChars;
	ULONG	dwFillAttribute;
	ULONG	dwFlags;
	ULONG	wShowWindow;
	UNICODE_STRING	WindowTitle;
	UNICODE_STRING	Desktop;
	UNICODE_STRING	Reserved2;
	UNICODE_STRING	Reserved3;
}PROCESS_PARAMETERS,*PPROCESS_PARAMETERS;


typedef struct _PORT_MESSAGE{
	USHORT	DataSize;
	USHORT	MessageSize;
	USHORT	MessageType;
	USHORT	VirtualRangesOffset;
	CLIENT_ID	ClientId;
	ULONG	MessageId;
	ULONG	SectionSize;
}PORT_MESSAGE,*PPORT_MESSAGE;


typedef struct _CSRSS_MESSAGE{
	ULONG	Unknwon1;
	ULONG	Opcode;
	ULONG	Status;
	ULONG	Unknwon2;
}CSRSS_MESSAGE,*PCSRSS_MESSAGE;

typedef struct _PROCESS_INFORMATION {
    HANDLE	hProcess;
    HANDLE	hThread;
    ULONG	dwProcessId;
    ULONG	dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;


// The promise function...
int exec(PUNICODE_STRING ImageName);

extern int ObOpenObjectByPointer();

#endif