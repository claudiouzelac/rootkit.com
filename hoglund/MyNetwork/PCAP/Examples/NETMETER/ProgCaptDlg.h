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

#if !defined(AFX_PROGCAPTDLG_H__D79DA3C4_3B52_11D2_9482_0020AF2A4474__INCLUDED_)
#define AFX_PROGCAPTDLG_H__D79DA3C4_3B52_11D2_9482_0020AF2A4474__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ProgCaptDlg.h : header file
//
#include "..\..\include\pcap-c.h"

class CCapPars;

/////////////////////////////////////////////////////////////////////////////
// CProgCaptDlg dialog

class CProgCaptDlg : public CDialog
{
// Construction
public:
	CProgCaptDlg(CWnd* pParent = NULL);   // standard constructor
	CCapPars* m_Main;
	int npc;
	int init;
	pcap_t *fp;
	pcap_dumper_t *dumpfile;


// Dialog Data
	//{{AFX_DATA(CProgCaptDlg)
	enum { IDD = IDD_CAP_BUSY };
	CStatic	m_Static;
	CProgressCtrl	m_Progress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProgCaptDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int p;
	int v;

	// Generated message map functions
	//{{AFX_MSG(CProgCaptDlg)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROGCAPTDLG_H__D79DA3C4_3B52_11D2_9482_0020AF2A4474__INCLUDED_)
