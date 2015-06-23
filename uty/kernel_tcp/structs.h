#ifndef U_STRUCTS
#define U_STRUCTS

#include <ntddk.h>
#include "TcpIpHdr.h"


//--------------------------------------------------------------------
////struct
#pragma pack(push)
#pragma pack(1)
typedef struct _HOOK_CONTEXT_STRUCT
{
	//runtime code
	UCHAR    code1_0x58; //0x58 | pop  eax      | pop caller IP from stack to eax
	UCHAR    code2_0x68; //0x68 | push IMM      | push our hook context address
	struct _HOOK_CONTEXT_STRUCT *m_pHookContext;//point this 
	UCHAR    code3_0x50; //0x50 | push eax		| push caller IP from eax to stack 
	UCHAR    code4_0xE9; //0xE9 | jmp HookProc  | jump our hook proc
	ULONG   m_pHookProcOffset;

	//our context data

	PVOID    m_pOriginalProc;
	PVOID    m_pHookProc;
	PVOID    m_pBindAdaptHandle;
	PVOID    m_pProtocolContent;
	PVOID   *m_ppOriginPtr;

	struct _HOOK_CONTEXT_STRUCT *m_pHookNext;
	
}HOOK_CONTEXT_STRUCT;
#pragma pack(pop)
//--------------------------------------------------------------------


#define MAX_PACKET_SIZE 1600  //whAt?


#endif //#ifndef U_FUKTDI