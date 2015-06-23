/*
 * Copyright (c) 1999, 2000
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if !defined(AFX_CapDll_H__03FA9206_C8EA_11D2_B729_0048540133F7__INCLUDED_)
#define AFX_CapDll_H__03FA9206_C8EA_11D2_B729_0048540133F7__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#define MaxCapPars 10

/////////////////////////////////////////////////////////////////////////////
// CCapDll:
// See CapDll.cpp for the implementation of this class
//
#ifdef _EXPORTING   
#define CLASS_DECLSPEC __declspec(dllexport)
#else
#define CLASS_DECLSPEC __declspec(dllimport)
#endif

void CLASS_DECLSPEC InitCapDll(const char* INI);

class CLASS_DECLSPEC CCapDll
{
public:
	CCapDll();
	const char* GetFileName();
	char* SetFileName(const char* fn);
	const char* GetAdapter();
	const char* GetPath();
	const char* GetFilter();
	char* SetPath(const char * p);
	char* SetAdapter(const char* ad);
	char* SetFilter(const char* ad);
	int CaptureDialog(const char* Adapter,const char* P, CWnd* mw);
	int ChooseAdapter(const char* Adapter, CWnd* mw);
	const char* Capture(const char* file, CWnd* mw);
	~CCapDll();
private:
	char* Path;
	char* Adapter;
	char* FileName;
	char* Filter;
	int bufdim;
	int ncapture;
	int snaplen;	
	int promisquous;
	void LoadCmds();
	CString RetrieveValue(CString keyval);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CapDll_H__03FA9206_C8EA_11D2_B729_0048540133F7__INCLUDED_)
