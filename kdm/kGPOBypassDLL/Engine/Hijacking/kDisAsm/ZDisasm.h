/******************************************************************************
  ZDisasm.c		: Z0MBiE DISASM ENGINE


   Z0MBiE's HomePage

			/\\|///
			| ~_  |
		   (- 0o -)
	_oOOo_ |/ (_) \| _oOOo_
	  ||    \ ==- /    ||
	   \\   /~~~~\    //
	http://z0mbie.cjb.net
	  z0mbie@i.am (PGP)


  ******************************************************************************/

#ifndef ZOMBIE_DISASM_ENGINE_H
#define ZOMBIE_DISASM_ENGINE_H

#include <windows.h>

void GetInstLenght(DWORD* myiptr0, DWORD* osizeptr);

#define C_ERROR         0xFFFFFFFF
#define C_PREFIX        0x00000001
#define C_66            0x00000002
#define C_67            0x00000004
#define C_DATA66        0x00000008
#define C_DATA1         0x00000010
#define C_DATA2         0x00000020
#define C_DATA4         0x00000040
#define C_MEM67         0x00000080
#define C_MEM1          0x00000100
#define C_MEM2          0x00000200
#define C_MEM4          0x00000400
#define C_MODRM         0x00000800
#define C_DATAW0        0x00001000
#define C_FUCKINGTEST   0x00002000
#define C_TABLE_0F      0x00004000

#endif