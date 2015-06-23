// This driver collects a list of all running threads in the system and
// displays them to the user through DbgPrint-function. This proofs that
// it is feasible to collect very low level data of what threads are running
// in the system. These threads belong to some process and this information
// can be used to examine if a rootkit is hiding a process from the system.
//
// The idea to collect the running threads by hooking the SwapContext-function
// in ntoskrnl.exe was given to me by Jamie Butler in his slides about DKOM
// (Direct Kernel Object Manipulation) presented at Black Hat Europe 2004.
//
// I also would like to thank Jacob R. Lorch and Alan Jay Smith, whose MSDN-article
// "The VTrace Tool: Building a System Tracer for Windows NT and Windows 2000" gave
// me many ideas and examples. Especially, their code to scan kernel memory was very
// usefull.
//
// NOTICE: The driver modifies the code of the kernel and uses some techniques, like
// hardcoded offsets to kernel structures, which are OS version dependent. It is
// possible that this code will crash your operating system. I have tested it with
// Windows XP SP1/SP1A/SP2 and Windows 2003 Server and it has been very stable and the
// performance has been good.
//
// USAGE: Install the driver (I use InstDriver.exe available from Hoglund's vault
// at http://www.rootkit.com) and start it. When you stop the driver, it will display
// all the threads that have been active and are still alive. The driver uses DbgPrint
// call to display the information, so you must use a debugger or SysInternals DbgView
// to display it.
//
// TODO:
//	- move some of the headers and functions to separate files.
//
// Kimmo

#include <ntddk.h>
#include <ntimage.h>
#include "hash.h"
#include "xde.h"

typedef struct _LDR_MODULE {
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
} LDR_MODULE, *PLDR_MODULE;

typedef struct _BYTECODE
{
	BYTE *pAddress;
	SIZE_T size;
} BYTECODE, *PBYTECODE;

typedef struct _SEARCHDATA
{
	BYTE **pKnownAddresses;
	BYTECODE **pScanAddresses;
	BYTECODE *pSignature;
} SEARCHDATA, *PSEARCHDATA;

typedef struct _OFFSETS
{
	BYTE threadsProcess;
	BYTE CID;
	BYTE imageFilename;
	BYTE crossThreadFlags;
} OFFSETS, *POFFSETS;

#define JMP_SIZE 5
#define SIG_SIZE 20
#define HASHTABLE_SIZE 256
#define MAINTAG1 'NIAM'

// NOTICE: WinDbg gives offsets in BYTEs, we use DWORDS.
OFFSETS offsets;
// The beginning of the SwapContext function is stored here.
BYTE *pSwapContext = NULL;
// The trampoline function which executes the replaced code and
// passes control to the hooked function.
BYTECODE trampoline;
// Inline assembler does not support structures, so this points
// directly to the pCode of the _BYTECODE structure.
BYTE *pTrampoline = NULL;
// The hashtable where we store the data.
PHASHTABLE pHashTable = NULL;

// Array HAS TO BE NULL TERMINATED!!!
void DeallocatePointerArrayWithTag(DWORD *pArray, ULONG tag)
{
	int i;

	for (i = 0; pArray[i] != 0x00000000; i++)
	{
			ExFreePoolWithTag((PVOID)pArray[i], tag);
	}
}

// Gets the address of the PsLoadedModuleList from the _DBGKD_GET_VERSION64
// structure which is pointed by the KdVersionBlock field in the
// _KPCR structure. This information was provided by Opc0de and Alex Ionescu.
//
// OUT DWORD * A pointer to the beginning of the list.
//
DWORD *GetPsLoadedModuleList()
{
	DWORD *address = 0x00000000;

	__asm {
		mov eax, fs:[0x34];	 		// Get address of KdVersionBlock
		mov eax, [eax+0x18];	// Get address of PsLoadedModuleList
		mov address, eax;
	}

	return address;
}

// Gets the address of the entry in the PsLoadedModuleList
// when the name of the module equals to the one passed as
// an argument.
//
// IN pModName Name of the module we are looking for.
// OUT DWORD * A pointer to the entry of the corresponding module.
//
PLDR_MODULE GetModuleByName(PUNICODE_STRING pModName)
{
	PLDR_MODULE pModule = NULL;
	PUNICODE_STRING pTmpName = NULL;
	PLIST_ENTRY pEntry = NULL;
	PLIST_ENTRY pListHead = (PLIST_ENTRY)GetPsLoadedModuleList();

	if (pListHead == NULL)
	{
		DbgPrint("GetPsLoadedModuleList returned NULL!\n");
		return NULL;
	}

	for (pEntry = pListHead->Flink;
		 pEntry != NULL && pEntry != pListHead;
		 pEntry = pEntry->Flink)
	{
		pModule = (PLDR_MODULE)pEntry;
		pTmpName = &pModule->BaseDllName;
		if (RtlCompareUnicodeString(pModName, pTmpName, TRUE) == 0)
		{
			return pModule;
		}
	}

	return NULL;
}

// REMEMBER TO RELEASE THE RESOURCES!!!
PBYTECODE *GetPESectionsWithCharacteristics(DWORD *pModule, DWORD chrs)
{
	int i, cnt, numOfSections;
	PIMAGE_DOS_HEADER pDOSHeader = NULL;
	PIMAGE_NT_HEADERS pNTHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	PBYTECODE *pResult = NULL;
	PBYTECODE pTmp = NULL;

	pDOSHeader = (PIMAGE_DOS_HEADER)pModule;

    if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        DbgPrint("Not a valid IMAGE_DOS_SIGNATURE!\n");
		return NULL;
    }

	pNTHeader = (PIMAGE_NT_HEADERS)((BYTE *)pDOSHeader + pDOSHeader->e_lfanew);

    if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        DbgPrint("Not a valid IMAGE_NT_SIGNATURE!\n");
        return NULL;
    }

	numOfSections = pNTHeader->FileHeader.NumberOfSections;
	pResult = ExAllocatePoolWithTag(PagedPool, sizeof(PBYTECODE) * numOfSections, MAINTAG1);
	if (pResult == NULL)
	{
		DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
		return NULL;
	}
	RtlZeroMemory(pResult, sizeof(PBYTECODE) * numOfSections);

	cnt = 0;
	pSectionHeader = IMAGE_FIRST_SECTION(pNTHeader);
	for (i = 0; i < numOfSections; i++)
	{
#ifdef MY_DEBUG
		DbgPrint("Section 0x%x: VirtualAddress = 0x%x Size = 0x%x :", i, (BYTE *)pModule + pSectionHeader->VirtualAddress, pSectionHeader->Misc.VirtualSize);
		if (pSectionHeader->Characteristics & IMAGE_SCN_CNT_CODE)
			DbgPrint(" CD");
		if (pSectionHeader->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
			DbgPrint(" DC");
		if (pSectionHeader->Characteristics & IMAGE_SCN_MEM_NOT_PAGED)
			DbgPrint(" NP");
		if (pSectionHeader->Characteristics & IMAGE_SCN_MEM_READ)
			DbgPrint(" R");
		if (pSectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE)
			DbgPrint(" W");
		if (pSectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE)
			DbgPrint(" X");
		DbgPrint("\n");
#endif
		if ((pSectionHeader->Characteristics & chrs) == chrs)
		{
			pTmp = ExAllocatePoolWithTag(PagedPool, sizeof(BYTECODE), MAINTAG1);
			if (pTmp == NULL)
			{
				//TODO: Release the resources!!!
				DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
				return NULL;
			}
			pTmp->pAddress = (BYTE *)pModule + pSectionHeader->VirtualAddress;
			pTmp->size = pSectionHeader->Misc.VirtualSize;
			pResult[cnt] = pTmp;
			cnt++;
		}
		pSectionHeader++;
	}

	return pResult;
}

// This function returns an MDL to an nonpaged virtual memory area.
// 
// IN pVirtualAddress Virtual address to the start of the memory area.
// IN length Length of the memory area in bytes.
//
// OUT PMDL Mdl to the nonpaged virtual memory area.
//
PMDL GetMdlForNonPagedMemory(PVOID pVirtualAddress, SIZE_T length)
{
	PMDL pMdl;

	if (length >= (PAGE_SIZE * (65535 - sizeof(MDL)) / sizeof(ULONG_PTR)))
	{
		DbgPrint("Size parameter passed to IoAllocateMdl is too big!\n");
		return NULL;
	}

	pMdl = IoAllocateMdl((PVOID)pVirtualAddress, length, FALSE, FALSE, NULL);
	if (NULL == pMdl)
	{
		DbgPrint("IoAllocateMdl returned NULL!\n");
		return NULL;
	}

	MmBuildMdlForNonPagedPool(pMdl);

	return pMdl;
}

// This function returns an MDL to a paged virtual memory area while
// making sure the pages are not paged out to the disk.
// 
// IN pVirtualAddress Virtual address to the start of the memory area.
// IN length Length of the memory area in bytes.
// IN operation Desired mode of operation.
//
// OUT PMDL Mdl to the locked and nonpaged memory area.
//
PMDL GetMdlForPagedMemory(PVOID pVirtualAddress, SIZE_T length, LOCK_OPERATION operation)
{
	PMDL pMdl;

	if (length >= (PAGE_SIZE * (65535 - sizeof(MDL)) / sizeof(ULONG_PTR)))
	{
		DbgPrint("Size parameter passed to IoAllocateMdl is too big!\n");
		return NULL;
	}

	pMdl = IoAllocateMdl((PVOID)pVirtualAddress, length, FALSE, FALSE, NULL);
	if (NULL == pMdl)
	{
		DbgPrint("IoAllocateMdl returned NULL!\n");
		return NULL;
	}

	// Make sure the memory is not paged on the disk.
	try
	{
		MmProbeAndLockPages(pMdl, KernelMode, operation);
	}
	except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("MmProbeAndLockPages caused an exception!\n");
		IoFreeMdl(pMdl);
		return NULL;
	}

	return pMdl;
}

// This function searches the given signature from the non-paged kernel address space.
// It returs the virtual address of the location where the beginning of the
// signature was found the first time.
//
// IN pTarger Pointer to the BYTECODE structure that specifies the memory area to search.
// IN pSignature Pointer to the signature we want to search for.
// OUT BYTE * The address of the found signature, NULL if not found or error.
//
BYTE *SearchKernelMemoryForSignature(PBYTECODE pTarget, PBYTECODE pSignature)
{
	unsigned int i, limit, equals;
	PMDL pMdl = NULL;
	PVOID pAddress = NULL;
	BYTE *pTmpAddress = NULL;
	BYTE *pRetValue = NULL;

	pMdl = GetMdlForNonPagedMemory(pTarget->pAddress, pTarget->size);
	if (NULL == pMdl)
	{
		DbgPrint("GetMdlForSafeKernelMemoryArea returned NULL!\n");
		return NULL;
	}

	pAddress = MmGetSystemAddressForMdlSafe(pMdl, HighPagePriority);

	if (pAddress == NULL)
	{
		IoFreeMdl(pMdl);
		DbgPrint("MmGetSystemAddressForMdlSafe returned NULL!\n");
		return NULL;
	}

	pTmpAddress = (BYTE *)pAddress;
	limit = pTarget->size - pSignature->size;

	for (i = 0; i <= limit; i++)
	{
		equals = RtlCompareMemory(pTmpAddress, pSignature->pAddress, pSignature->size);
		if (equals == pSignature->size)
		{
			pRetValue = pTmpAddress;
			break;
		}
		pTmpAddress++;
	}

	IoFreeMdl(pMdl);

	return pRetValue;
}

// This function writes the given data to the given non-paged kernel memory location.
// It makes sure that no other instance can access it in any way until we have finished
// our job.
//
// IN pDestination Pointer to the kernel memory where we want to write.
// IN pSource Pointer to the data we want to write.
// IN length Length of data we want to write in bytes.
//
// OUT NTSTATUS return code.
//
NTSTATUS WriteKernelMemory(BYTE *pDestination, BYTE *pSource, SIZE_T length)
{
	KSPIN_LOCK spinLock;
	KLOCK_QUEUE_HANDLE lockHandle;
	PMDL pMdl;
	PVOID pAddress;

	pMdl = GetMdlForNonPagedMemory(pDestination, length);
	if (NULL == pMdl)
	{
		DbgPrint("GetMdlForSafeKernelMemoryArea returned NULL!\n");
		return STATUS_UNSUCCESSFUL;
	}

	pAddress = MmGetSystemAddressForMdlSafe(pMdl, HighPagePriority);

	if (pAddress == NULL)
	{
		IoFreeMdl(pMdl);
		DbgPrint("MmGetSystemAddressForMdlSafe returned NULL!\n");
		return STATUS_UNSUCCESSFUL;
	}

	KeInitializeSpinLock(&spinLock);
	// Only supported on XP and later. For Windows 2000 compatibility you can
	// use the older, less efficient and less reliable KeAcquireSpinLock function.
	KeAcquireInStackQueuedSpinLock (&spinLock, &lockHandle);
	// We have the spinlock, so we can safely overwrite the kernel memory.
	RtlCopyMemory(pAddress, pSource, length);
	KeReleaseInStackQueuedSpinLock(&lockHandle);

	IoFreeMdl(pMdl);

	return STATUS_SUCCESS;
}

// This function processes and stores the data our hook collects.
//
// IN	pEthread	Pointer to the _ETHREAD structure of the thread.
//
void __stdcall ProcessData(DWORD *pEthread)
{
	// NOTICE: WinDbg gives offsets in BYTEs, we use DWORDS
	DWORD *pEprocess = (DWORD *)*(pEthread + offsets.threadsProcess);
	DWORD *pCid = (DWORD *)(pEthread+offsets.CID);
	DWORD key;
	DATA data;
	
	data.processID = 0x0;
	data.threadID = 0x0;
	data.imageName = "NONE";

	key = (DWORD)pEthread;
	
	if (pCid != NULL)
	{
		data.processID = *pCid;
		data.threadID = *(pCid + 0x1);
	}
	
	if (pEprocess != NULL)
	{
		data.imageName = (BYTE *)(pEprocess+offsets.imageFilename);
	}
	// The thread is terminated so remove it from the hashtable.
	if (*(pEthread + offsets.crossThreadFlags) & 1)
	{
		Remove(key, pHashTable);
	}
	else
	{
		Insert(key, &data, pHashTable);
	}
}

// This function implements the hook we are placing on the SwapContext
// function. It is specified as of naked type, so we can handle our own
// epilog and prolog and get ultimate control.
//
void __declspec(naked) DetourFunction()
{
	__asm {
		// Save parameters we will overwrite. We save all data to play it safe.
		pushad
		pushfd
		// Disable interrupts. Assume single processor machine.
//		cli
		// EDI holds the thread whose context we will switch out.
		push edi
		call ProcessData
		// ESI holds the thread whose context we will switch in.
		push esi
		call ProcessData
		// Enable interrupts.
//		sti
		// Restore the saved state.
		popfd
		popad

		// Jump to the trampoline function.
		jmp dword ptr pTrampoline
	}
}

// This function searches the kernel memory for the start address of the
// SwapContext function. We will first try to check some well known locations
// and if it fails we will search the whole memory area.
//
// IN	data	Pointer to structure of type _SEARCHDATA.
// OUT	BYTE pointer to the start address of the SwapContext function.
//
BYTE *FindSwapContextAddress(PSEARCHDATA data)
{
	unsigned int i, limit;
	BYTE **pKnownAddresses = data->pKnownAddresses;
	PBYTECODE *pScanAddresses = data->pScanAddresses;
	PBYTECODE pSignature = data->pSignature;
	PBYTECODE pSection = NULL;
	BYTECODE tmpSection;
	BYTE *pAddress = NULL;
	int bytesMatched = 0;

	for (i = 0; pKnownAddresses[i] != 0x00000000; i++)
	{
		tmpSection.pAddress = pKnownAddresses[i];
		tmpSection.size = pSignature->size;
		pAddress = SearchKernelMemoryForSignature(&tmpSection, pSignature);
		if (pAddress != NULL)
		{
			return pAddress;
		}

	}

	// We have to find it by force. This is very slow method
	// and could be optimized a lot. However, this is only run
	// once, so we prefer stability more than speed.
#ifdef MY_DEBUG
	DbgPrint("Doing a manual memory scan!\n");
#endif
	for (i = 0; pScanAddresses[i] != 0x00000000; i++)
	{
		pSection = pScanAddresses[i];
#ifdef MY_DEBUG
		DbgPrint("KERNEL MEMORY SCAN: address = 0x%x , size = 0x%x\n", pSection->pAddress, pSection->size);
#endif
		pAddress = SearchKernelMemoryForSignature(pSection, pSignature);
		if (pAddress != NULL)
		{
			return pAddress;
		}
	}

	return NULL;
}
// This function hooks the original SwapContext function which makes
// it possible to execute our code before any other code is executed.
//
// OUT NTSTATUS return value.
//
NTSTATUS InstallSwapContextHook()
{
	NTSTATUS rc;
	int length = 0;
	int totalLength = 0;
	struct xde_instr instr;
	BYTE *pJmpCode = NULL;
	long displacement = 0;

	// Disassemble the code to get how many bytes we have to replace.
	// We use XDE v1.01 by Z0MBie (http://z0mbie.host.sk/).
	while (totalLength < 5)
	{
		length = xde_disasm(pSwapContext + totalLength, &instr);
		if (length == 0)
		{
			DbgPrint("xde_disasm returned 0!\n");
			return STATUS_UNSUCCESSFUL;
		}
		totalLength += length;
	}
	
	DbgPrint("Hook will replace the first %d bytes.\n", totalLength);

	// Allocate the required bytes for the trampoline function.
	pTrampoline = trampoline.pAddress = ExAllocatePoolWithTag(NonPagedPool, totalLength + 5, MAINTAG1);
	if (trampoline.pAddress == NULL)
	{
		DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
		return STATUS_UNSUCCESSFUL;
	} 

	DbgPrint("Trampoline is at 0x%x\n", pTrampoline);

	// This tells how many bytes we replaced from the original function.
	trampoline.size = totalLength;
	RtlCopyMemory(trampoline.pAddress, pSwapContext, totalLength);

	// We are using JMP rel32 instruction to jump to the rest of the
	// swapcontext function, so we first calculate the 32bit displacement
	// and then create the five byte JMP instruction.
	displacement = (pSwapContext + totalLength) - (trampoline.pAddress + totalLength + JMP_SIZE);
	pJmpCode = trampoline.pAddress + totalLength;
	*pJmpCode = 0xe9;
	RtlCopyMemory(pJmpCode+1, &displacement, 4);

	// Allocate the required bytes for the jmp code to the detour function.
	pJmpCode = ExAllocatePoolWithTag(NonPagedPool, totalLength, MAINTAG1);
	if (pJmpCode == NULL)
	{
		DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
		return STATUS_UNSUCCESSFUL;
	}

	// Initialize the jmp-code with NOPs.
	RtlFillMemory(pJmpCode, totalLength, 0x90);
	
	// We are using JMP rel32 instruction to jump to our hook function,
	// so we first calculate the 32bit displacement and then create the
	// five byte JMP instruction.
	displacement = ((BYTE *)&DetourFunction) - (pSwapContext + JMP_SIZE);
	*pJmpCode = 0xe9;
	RtlCopyMemory(pJmpCode+1, &displacement, 4);

	rc = WriteKernelMemory(pSwapContext, pJmpCode, totalLength);
	ExFreePoolWithTag(pJmpCode, MAINTAG1);

	return rc;
}

// This function removes our hook by restoring the bytes we have replaced
// from the original SwapContext function.
//
// OUT NTSTATUS return value.
//
NTSTATUS UninstallSwapContextHook()
{
	return WriteKernelMemory(pSwapContext, trampoline.pAddress, trampoline.size);
}

NTSTATUS OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	NTSTATUS rc;

	DbgPrint("OnUnload called\n");

	rc = UninstallSwapContextHook();

	if (STATUS_SUCCESS == rc)
	{
		DbgPrint("UninstallSwapContextHook succeeded.\n");
	}
	else
	{
		DbgPrint("UninstallSwapContextHook failed!\n");
	}

	// Show the collected data and release all resources.
	DumpTable(pHashTable);
	DestroyTable(pHashTable);
	ExFreePoolWithTag(pTrampoline, MAINTAG1);

	return rc;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS rc;
	SEARCHDATA searchData;
	DWORD characteristics;
	PUNICODE_STRING pModName = NULL;
	PLDR_MODULE pNtoskrnl = NULL;
	BYTE **pKnownAddresses = NULL;
	BYTECODE **pScanAddresses = NULL;
	DWORD *pSigCode = NULL;
	PBYTECODE pSignature;
	RTL_OSVERSIONINFOW osvi;
#ifdef MY_DEBUG
	__asm int 3;
#endif

	DbgPrint("DriverEntry called.\n");

	pModName = ExAllocatePoolWithTag(PagedPool, sizeof(UNICODE_STRING), MAINTAG1);
	if (pModName == NULL)
	{
		DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
		return STATUS_UNSUCCESSFUL;
	}

	RtlInitUnicodeString(pModName, L"ntoskrnl.exe");
	pNtoskrnl = GetModuleByName(pModName);

	if (pNtoskrnl == NULL)
	{
		DbgPrint("GetModuleByName returned NULL!\n");
		ExFreePoolWithTag(pModName, MAINTAG1);
		return STATUS_UNSUCCESSFUL;
	}

	ExFreePoolWithTag(pModName, MAINTAG1);

	RtlZeroMemory(&osvi, sizeof(RTL_OSVERSIONINFOW));
	osvi.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

	// Initialize the OS specific data.
	if (STATUS_SUCCESS == RtlGetVersion(&osvi))
	{
		// Windows XP.
		if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
		{
			DbgPrint("Microsoft Windows XP\n");
			offsets.threadsProcess = 0x88;
			offsets.CID = 0x7b;
			offsets.imageFilename = 0x5d;
			offsets.crossThreadFlags = 0x92;

			// NOTICE: This array has to be NULL terminated!
			// NOTICE: Each address is FOUR bytes!
			pKnownAddresses = ExAllocatePoolWithTag(PagedPool, 12, MAINTAG1);
			if (pKnownAddresses == NULL)
			{
				DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
				return STATUS_UNSUCCESSFUL;
			}
			// Well-known addresses of SwapContext at Windows XP.
//			pKnownAddresses[0] = (BYTE *)0x804dbeb9;	// SP2
//			pKnownAddresses[1] = (BYTE *)0x804f1b83;	// SP1
//			pKnownAddresses[2] = (BYTE *)0x80531500;	// SP1
//			pKnownAddresses[3] = (BYTE *)0x00000000;
			pKnownAddresses[0] = (BYTE *)0x00000000;

			pSignature = ExAllocatePoolWithTag(PagedPool, sizeof(BYTECODE), MAINTAG1);
			if (pSignature == NULL)
			{
				DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
				ExFreePoolWithTag(pKnownAddresses, MAINTAG1);
				return STATUS_UNSUCCESSFUL;
			}

			pSigCode = ExAllocatePoolWithTag(PagedPool, SIG_SIZE, MAINTAG1);
			if (pSigCode == NULL)
			{
				DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
				ExFreePoolWithTag(pKnownAddresses, MAINTAG1);
				ExFreePoolWithTag(pSignature, MAINTAG1);
				return STATUS_UNSUCCESSFUL;
			}
			// The first SIG_SIZE bytes of the SwapContext.
			pSigCode[0] = 0xc626c90a;
			pSigCode[1] = 0x9c022d46;
			pSigCode[2] = 0xbb830b8b;
			pSigCode[3] = 0x00000994;
//			pSigCode[3] = 0xaaaaa994;
			pSigCode[4] = 0x850f5100;
			pSignature->pAddress = (BYTE *)pSigCode;
			pSignature->size = SIG_SIZE;
		}
		// Windows 2003 Server.
		else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
		{
			DbgPrint("Microsoft Windows Server 2003 family\n");
			offsets.threadsProcess = 0x8a;
			offsets.CID = 0x7d;
			offsets.imageFilename = 0x55;
			offsets.crossThreadFlags = 0x94;

			// NOTICE: This array has to be NULL terminated!
			// NOTICE: Each address is FOUR bytes!
			pKnownAddresses = ExAllocatePoolWithTag(PagedPool, 8, MAINTAG1);
			if (pKnownAddresses == NULL)
			{
				DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
				return STATUS_UNSUCCESSFUL;
			}
			// Well-known addresses of SwapContext.
//			pKnownAddresses[0] = (BYTE *)0x804e4971;
//			pKnownAddresses[1] = (BYTE *)0x00000000;
			pKnownAddresses[0] = (BYTE *)0x00000000;

			pSignature = ExAllocatePoolWithTag(PagedPool, sizeof(BYTECODE), MAINTAG1);
			if (pSignature == NULL)
			{
				DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
				ExFreePoolWithTag(pKnownAddresses, MAINTAG1);
				return STATUS_UNSUCCESSFUL;
			}

			pSigCode = ExAllocatePoolWithTag(PagedPool, SIG_SIZE, MAINTAG1);
			if (pSigCode == NULL)
			{
				DbgPrint("ExAllocatePoolWithTag returned NULL!\n");
				ExFreePoolWithTag(pKnownAddresses, MAINTAG1);
				ExFreePoolWithTag(pSignature, MAINTAG1);
				return STATUS_UNSUCCESSFUL;
			}
			// The first SIG_SIZE bytes of the SwapContext.
			pSigCode[0] = 0x1043ff51;
			pSigCode[1] = 0x7b8333ff;
			pSigCode[2] = 0x850f0008;
			pSigCode[3] = 0x000000e9;
			pSigCode[4] = 0x8bc5200f;
			pSignature->pAddress = (BYTE *)pSigCode;
			pSignature->size = SIG_SIZE;
		}
		else
		{
			DbgPrint("Unsupported OS version!\n");
			return STATUS_UNSUCCESSFUL;
		}
		characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_NOT_PAGED;
		pScanAddresses = GetPESectionsWithCharacteristics((DWORD *)pNtoskrnl->BaseAddress, characteristics);

		if (pScanAddresses == NULL)
		{
			DbgPrint("GetPESectionsWithCharacteristics() FAILED!\n");
			ExFreePoolWithTag(pKnownAddresses, MAINTAG1);
			ExFreePoolWithTag(pSignature, MAINTAG1);
			return STATUS_UNSUCCESSFUL;
		}

		searchData.pKnownAddresses = pKnownAddresses;
		searchData.pScanAddresses = pScanAddresses;
		searchData.pSignature = pSignature;
	}
	else
	{
		DbgPrint("RtlGetVersion failed!\n");
		return STATUS_UNSUCCESSFUL;
	}

	DriverObject->DriverUnload  = OnUnload;

	pHashTable = InitializeTable(HASHTABLE_SIZE);
	if (pHashTable == NULL)
	{
		DbgPrint("InitializeTable failed!\n");
		return STATUS_UNSUCCESSFUL;
	}
    
	pSwapContext = FindSwapContextAddress(&searchData);
	ExFreePoolWithTag(pKnownAddresses, MAINTAG1);
	ExFreePoolWithTag(pSigCode, MAINTAG1);
	ExFreePoolWithTag(pSignature, MAINTAG1);
	DeallocatePointerArrayWithTag((DWORD *)pScanAddresses, MAINTAG1);
	ExFreePoolWithTag(pScanAddresses, MAINTAG1);
	if (NULL == pSwapContext)
	{
		DbgPrint("FindSwapContextAddress returned NULL!\n");
		return STATUS_UNSUCCESSFUL;
	}
	DbgPrint("SwapContext found at 0x%x\n", pSwapContext);

	rc = InstallSwapContextHook();
	if (STATUS_SUCCESS == rc)
	{
		DbgPrint("InstallSwapContextHook succeeded.\n");
		DbgPrint("DetourFunction is at 0x%x\n", DetourFunction);
	}
	else
	{
		DbgPrint("InstallSwapContextHook failed!\n");
	}

	return rc;
}