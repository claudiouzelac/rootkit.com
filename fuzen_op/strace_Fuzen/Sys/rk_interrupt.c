#include "ntddk.h"
#include "stdarg.h"
#include "stdio.h"


#include "rk_interrupt.h"


DWORD KiRealSystemServiceISR_Ptr; /* the real interrupt 2E handler */
DWORD g_EDX; // Global set with the address of the function parameters so we do not have to pass them on the stack

/* _____________________________________________________________________________
 . Interrupt Hook - if you create other hooks you can copy this one to start.
 . _____________________________________________________________________________ */
__declspec(naked) MyKiSystemService()
/* thanks to mad russians */
{
	__asm{
		pushad
		pushfd
		push fs
		mov bx,0x30
		mov fs,bx
		push ds
		push es

		mov ebx, eax;

TestPID:
		call getPID;
		cmp  eax, gProcessID;
		jne  Finish;

TestSyscallLimit:
		cmp ebx, gSyscallLimit;
		jge  Finish;

PrintDebug:
		mov  g_EDX, edx;
		push ebx;
		call PrintDbg;

Finish:		
		pop es
		pop ds
		pop fs
		popfd
		popad

		jmp	KiRealSystemServiceISR_Ptr;
	}
}

void PrintDbg(ULONG syscall)
{
	char *fname = NULL;

	if ((long) syscall >= KeServiceDescriptorTable.NumberOfServices)
		DbgPrint("%s:%u just called Syscall: %u\n", (char *)PsGetCurrentProcess()+gProcessNameOffset, PsGetCurrentProcessId(), syscall);
	else
	{
		if (name_array != NULL)
		{
			DbgPrint("%s:%u just called Syscall: %s\n", (char *)PsGetCurrentProcess()+gProcessNameOffset, PsGetCurrentProcessId(), name_array+(syscall*FUNC_NAME_LEN));
			fname = name_array+(syscall*FUNC_NAME_LEN);
			if(strcmp(fname, "NtCreateFile") == 0)
				PrintNtCreateFile();
			else if(strcmp(fname, "NtCreateDirectoryObject") == 0)
				PrintNtCreateDirectoryObject();
			else if(strcmp(fname, "NtCreateEvent") == 0)
				PrintNtCreateEvent();
			else if(strcmp(fname, "NtCreateEventPair") == 0)
				PrintNtCreateEventPair();

		}
		else
			DbgPrint("%s:%u just called Syscall: %u\n", (char *)PsGetCurrentProcess()+gProcessNameOffset, PsGetCurrentProcessId(), syscall);
	}
}

void PrintNtCreateEvent()
{
/*	DWORD *d_access;

	d_access = g_EDX;
	d_access += 1;
*/
	PrintObjectAttributes(2);
//	if(*d_access != NULL)
//	{

//	}
}

void PrintNtCreateEventPair()
{
	PrintObjectAttributes(2);
}

void PrintNtCreateFile()
{
	PrintObjectAttributes(2);
}

void PrintNtCreateDirectoryObject()
{
	PrintObjectAttributes(2);
}

void PrintObjectAttributes(int edx_offset)
{
	DWORD *temp;
	POBJECT_ATTRIBUTES p_objAttr;
	ANSI_STRING objname;

	temp = g_EDX;
	temp += edx_offset;
	p_objAttr = (POBJECT_ATTRIBUTES) *temp;
	if (p_objAttr != NULL)
	{
		if(p_objAttr->ObjectName != NULL)
		{
			temp = &p_objAttr->ObjectName;
			if(NT_SUCCESS(RtlUnicodeStringToAnsiString(&objname, *temp, TRUE)))
			{
				DbgPrint("\tObject Name: %s\n",objname.Buffer);
				RtlFreeAnsiString( &objname );
			}
		}
		if (p_objAttr->Attributes != 0x00000000)
		{

			DbgPrint("\tAttributes:\n");
			if ((p_objAttr->Attributes & OBJ_INHERIT) == OBJ_INHERIT)
				DbgPrint("\t\tOBJ_INHERIT\n");
			if ((p_objAttr->Attributes & OBJ_PERMANENT) == OBJ_PERMANENT)
				DbgPrint("\t\tOBJ_PERMANENT\n");
			if ((p_objAttr->Attributes & OBJ_EXCLUSIVE) == OBJ_EXCLUSIVE)
				DbgPrint("\t\tOBJ_EXCLUSIVE\n");
			if ((p_objAttr->Attributes & OBJ_CASE_INSENSITIVE) == OBJ_CASE_INSENSITIVE)
				DbgPrint("\t\tOBJ_CASE_INSENSITIVE\n");
			if ((p_objAttr->Attributes & OBJ_OPENIF) == OBJ_OPENIF)
				DbgPrint("\t\tOBJ_OPENIF\n");
			if ((p_objAttr->Attributes & OBJ_OPENLINK) == OBJ_OPENLINK)
				DbgPrint("\t\tOBJ_OPENLINK\n");
			if ((p_objAttr->Attributes & OBJ_KERNEL_HANDLE) == OBJ_KERNEL_HANDLE)
				DbgPrint("\t\tOBJ_KERNEL_HANDLE\n");

		}
	}
}


ULONG getPID()
{
	return (ULONG) PsGetCurrentProcessId();
}
/* ________________________________________________________________________________
 . This function replaces the interrupt descriptor.  You can hook any interrupts
 . you choose from this function.  Just make sure you unhook them also!
 . 
 . they to discern between truth and suggestion 
 . they bid for your Id, for your fear of 
 . rejection.
 . ________________________________________________________________________________ */
int HookInterrupts()
{
	IDTINFO idt_info;
	IDTENTRY* idt_entries;
	IDTENTRY* int2e_entry;
	__asm{
		sidt idt_info;
	}

	idt_entries = (IDTENTRY*) MAKELONG(idt_info.LowIDTbase,idt_info.HiIDTbase);

	KiRealSystemServiceISR_Ptr = MAKELONG(idt_entries[NT_SYSTEM_SERVICE_INT].LowOffset,idt_entries[NT_SYSTEM_SERVICE_INT].HiOffset);

	/*******************************************************
	 * Note: we can patch ANY interrupt here
	 * the sky is the limit
	 *******************************************************/
	int2e_entry = &(idt_entries[NT_SYSTEM_SERVICE_INT]);

	__asm{
		cli;
		lea eax,MyKiSystemService;
		mov ebx, int2e_entry;
		mov [ebx],ax;
		shr eax,16
		mov [ebx+6],ax;

		lidt idt_info;
		sti;
	}
	return 0;
}

/* _______________________________________________________________________________
 . What is hooked must be unhooked ;-)
 . _______________________________________________________________________________ */
int UnhookInterrupts()
{
	IDTINFO idt_info;
	IDTENTRY* idt_entries;
	IDTENTRY* int2e_entry;

	__asm{
		sidt idt_info;
	}

	idt_entries = (IDTENTRY*) MAKELONG(idt_info.LowIDTbase,idt_info.HiIDTbase);

	int2e_entry = &(idt_entries[NT_SYSTEM_SERVICE_INT]);

	__asm{
		cli;
		mov eax,KiRealSystemServiceISR_Ptr;
		mov ebx, int2e_entry;
		mov [ebx],ax;
		shr eax,16
		mov [ebx+6],ax;


		lidt idt_info;
		sti;
	}
	return 0;
}
