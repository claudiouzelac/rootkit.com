/*
	IAT Hooking Module for Kernel Drivers (C) tibbar, 2005.
	Written by tibbar.  Contact tibbar@tibbar.org for more information.

	You many use this code freely in non-commercial applications, provided you give credit
	for any code used and notify the author.

	If you wish to use this in a commercial application you must contact the author for terms and
	conditions.

*/

#include "ntddk.h"

#include "hookiat.h"

#pragma comment(lib,"ntdll.lib")

PVOID g_OriginalRtlGenerate8dot3Name = NULL;
PVOID g_IATFunctionPointer = NULL;

typedef VOID (*RTLGENERATE8DOT3NAME) (
    IN PUNICODE_STRING              Name,
    IN BOOLEAN                      AllowExtendedCharacters,
    IN OUT PGENERATE_NAME_CONTEXT   Context,
    OUT PUNICODE_STRING             Name8dot3
);

PVOID FindDriverBase(char* driverName) /*i.e. driver.sys*/
{
	ULONG n,i;
	PULONG q;
	PSYSTEM_MODULE_INFORMATION p;
	PVOID driver;

	ZwQuerySystemInformation(SystemModuleInformation,&n, 0, &n);

	q = (PULONG)ExAllocatePool(PagedPool, n);
	ZwQuerySystemInformation(SystemModuleInformation,q, n * sizeof *q, 0);

	p = (PSYSTEM_MODULE_INFORMATION)(q + 1);
	driver = 0;
	for (i = 0; i < *q; i++)
	if (_stricmp(p[i].ImageName + p[i].ModuleNameOffset, driverName) == 0)  driver = p[i].Base;
	ExFreePool(q);
	return driver;
}


DWORD GetIATPointerRVAFromBase(char* lpFunctionName, char* lpFunctionLibrary, PUNICODE_STRING pDriverName, DWORD* pThunk, DWORD* pRVA) 
{
	HANDLE hThread, hSection, hFile, hMod;
	SECTION_IMAGE_INFORMATION sii;
	BOOLEAN foundIt;
	IMAGE_DOS_HEADER * dosheader;
	IMAGE_OPTIONAL_HEADER * opthdr;
	IMAGE_IMPORT_DESCRIPTOR * pDataEntryAddress;
	IMAGE_THUNK_DATA* thunk;
	char *pszModName;
	DWORD firstThunkList;
	DWORD* test;
	BOOLEAN isOrdinal;
	int x=0;

	PVOID BaseAddress = NULL;
	SIZE_T size=0;

	OBJECT_ATTRIBUTES oa = {sizeof oa, 0, pDriverName, OBJ_CASE_INSENSITIVE};

	IO_STATUS_BLOCK iosb;

	//_asm int 3;
	ZwOpenFile(&hFile, FILE_EXECUTE | SYNCHRONIZE, &oa, &iosb, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);

	oa.ObjectName = 0;

	ZwCreateSection(&hSection, SECTION_ALL_ACCESS, &oa, 0,PAGE_EXECUTE, SEC_IMAGE, hFile);
	
	ZwMapViewOfSection(hSection, NtCurrentProcess(), &BaseAddress, 0, 1000, 0, &size, (SECTION_INHERIT)1, MEM_TOP_DOWN, PAGE_READWRITE); 
	
	ZwClose(hFile);
	
	hMod = BaseAddress;
	
	dosheader = (IMAGE_DOS_HEADER *)hMod;
	
	opthdr =(IMAGE_OPTIONAL_HEADER *) ((BYTE*)hMod+dosheader->e_lfanew+24);
////////////////////
	pDataEntryAddress = (IMAGE_IMPORT_DESCRIPTOR *)((BYTE*)dosheader+ opthdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	if( NULL == pDataEntryAddress) return 0;
	
	
	
	// 1st thing to do is loop through the list of imported modules until we find pHookedFunctionLibrary
	//DbgBreakPoint();
	while (pDataEntryAddress->FirstThunk)
	{		
		pszModName = (PSTR) ((PBYTE) hMod + pDataEntryAddress->Name);
		//isSameLibrary = RtlEqualString((PSTRING) pszModName, (PSTRING) lpFunctionLibrary, TRUE);
	
		if (_stricmp(pszModName, lpFunctionLibrary) == 0 ) 
		{
			foundIt = TRUE;
			break;
		}
		pDataEntryAddress++;
	}

	if(foundIt==FALSE){return 0;}


	// if we found the module: pHookedFunctionLibrary
	// then we now should look for the function: pHookedFunction

	thunk = (IMAGE_THUNK_DATA*)( (BYTE*)hMod + pDataEntryAddress->OriginalFirstThunk);

	firstThunkList = (DWORD)((PBYTE)hMod + pDataEntryAddress->FirstThunk);

	
	foundIt = FALSE;
	//DbgBreakPoint();
	while(thunk->u1.Function)
	{
		// note that first we must determine if function is imported by name or by ordinal
		// this is determined by the last 8 bits of the DWORD IMAGE_THUNK_DATA.  If it's zero then import by name
		// if non zero then the 1st 24bits are the ordinal
		//DbgBreakPoint();
		isOrdinal = 0;
		if(thunk->u1.Function >= 0x01000000) isOrdinal = TRUE;
		///////////////////////START OF FUNCTION NAME IMPORT///////////////////////////////
		if(!isOrdinal) // import by name
		{
			char* functionName = (char*)( (BYTE*)hMod 
											+ (DWORD)thunk->u1.AddressOfData + 2 );
			//isSameFunction = RtlEqualString((PSTRING) functionName, (PSTRING) lpFunctionName, TRUE);
			if (_stricmp(functionName, lpFunctionName) == 0 ) 
			{
				
				//test = (DWORD*)( (BYTE*)hMod   // here we want a pointer to where the address in IAT lives
				//								+ pDataEntryAddress->FirstThunk) + x;
				//test = /*(DWORD*)( (BYTE*)hMod )+*/ (DWORD*)(pDataEntryAddress->FirstThunk) + x;
				*pThunk = pDataEntryAddress->FirstThunk;
				*pRVA = x;
				ZwClose(hSection);
				return 1;
			}
		}
		if(isOrdinal)
		{
			ZwClose(hSection);
			return (DWORD) NULL;
		}

		x++;
		thunk++;
		firstThunkList++;
	}
	if(foundIt==FALSE)
	{
		ZwClose(hSection);
		return 0;
	}
	ZwClose(hSection);
	return 0;
}
 
 


VOID
NTAPI
MyRtlGenerate8dot3Name (
    IN PUNICODE_STRING              Name,
    IN BOOLEAN                      AllowExtendedCharacters,
    IN OUT PGENERATE_NAME_CONTEXT   Context,
    OUT PUNICODE_STRING             Name8dot3
)
{
	DbgPrint("MyRtlGenerate8dot3Name called!\n");
	((RTLGENERATE8DOT3NAME)(g_OriginalRtlGenerate8dot3Name))(Name,AllowExtendedCharacters, Context,
															Name8dot3);
}


VOID Unload(PDRIVER_OBJECT DriverObject)
{
	if(NULL == g_IATFunctionPointer || NULL == g_OriginalRtlGenerate8dot3Name) return;
	_asm
	{
		CLI					//dissable interrupt
		MOV	EAX, CR0		//move CR0 register into EAX
		AND EAX, NOT 10000H //disable WP bit 
		MOV	CR0, EAX		//write register back
	}
	*(PVOID*)g_IATFunctionPointer = g_OriginalRtlGenerate8dot3Name;
	_asm 
	{
		MOV	EAX, CR0		//move CR0 register into EAX
		OR	EAX, 10000H		//enable WP bit 	
		MOV	CR0, EAX		//write register back		
		STI					//enable interrupt
	}
}



NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING str)
{
	DWORD didItWork = 0;
	DWORD RVA, Thunk;
	UNICODE_STRING driverName;
	PVOID base = NULL;
	char functionName[] = "RtlGenerate8dot3Name";
	char libraryName[] = "ntoskrnl.exe";
	//find the base address of target driver
	base = FindDriverBase("ntfs.sys");
	//DbgBreakPoint();
	if(NULL==base)
	{
		DbgPrint("base not found");
		return STATUS_SUCCESS;
	}

	RtlInitUnicodeString(&driverName, L"\\Device\\HarddiskVolume1\\Windows\\System32\\drivers\\ntfs.sys");
	didItWork = GetIATPointerRVAFromBase(functionName, libraryName, &driverName, &Thunk, &RVA );
	
	if(0==didItWork)
	{
		DbgPrint("IATPointerRVA not found");
		return STATUS_SUCCESS;
	}

	g_IATFunctionPointer = (DWORD*)( (BYTE*)base + Thunk ) + RVA;

	if(NULL==g_IATFunctionPointer)
	{
		DbgPrint("IATFunctionPointer not found");
		return STATUS_SUCCESS;
	}

	g_OriginalRtlGenerate8dot3Name = *(PVOID*)g_IATFunctionPointer;
	DbgBreakPoint();
	_asm
	{
		CLI					//dissable interrupt
		MOV	EAX, CR0		//move CR0 register into EAX
		AND EAX, NOT 10000H //disable WP bit 
		MOV	CR0, EAX		//write register back
	}

	*(PVOID*)g_IATFunctionPointer = MyRtlGenerate8dot3Name;
	_asm 
	{
		MOV	EAX, CR0		//move CR0 register into EAX
		OR	EAX, 10000H		//enable WP bit 	
		MOV	CR0, EAX		//write register back		
		STI					//enable interrupt
	}
	if (DriverObject) DriverObject->DriverUnload = Unload;
	return DriverObject ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

