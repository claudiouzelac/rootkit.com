#ifndef __idtHook_h__
#define __idtHook_h__

//IDT gate descriptor - see Intel System Developer's Manual Vol. 3 for details
struct IDT_GATE
{
	unsigned OffsetLo	:16;   //bits 15....0
	unsigned Selector	:16;   //segment selector
	unsigned Reserved	:13;   //bits we don't need
	unsigned Dpl		:2;	   //descriptor privilege level
	unsigned Present	:1;	   //segment present flag
	unsigned OffsetHi	:16;   //bits 32...16
}; //end struct


struct IDT_INFO
{
	unsigned short wReserved;
	unsigned short wLimit;	//size of the table
	IDT_GATE* pIdt;			//base address of the table
};//end struct

//@@@@@@@@@@@@@@@@@@@@@@
// PROTOTYPES
//@@@@@@@@@@@@@@@@@@@@@@
int HookInt( unsigned long* pOldHandler, unsigned long NewHandler, int IntNumber ); //interrupt hook
int UnhookInt( unsigned long OldHandler,  int IntNumber );

#endif

