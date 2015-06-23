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

#if !defined(AFX_SELECTADAPTER_H__D41A3004_2B3D_11D0_9528_0020AF2A4474__INCLUDED_)
#define AFX_SELECTADAPTER_H__D41A3004_2B3D_11D0_9528_0020AF2A4474__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelectAdapter.h : header file
//

#include "linecoll.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectAdapter dialog

int ExecuteApp(CString & s);

class CSelectAdapter : public CDialog
{
// Construction
public:
	CSelectAdapter(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectAdapter)
	enum { IDD = IDD_ADAPTER };
	CStatic	m_CAdapter;
	CListCtrl	m_ListCtrl;
	//}}AFX_DATA
	CString m_Adapter;
	CString m_Cmd;
    CImageList m_ctlImage;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectAdapter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void Update(LineCollection &lc);
	void AddItem(int nItem,int nSubItem,LPCTSTR strItem,int nImageIndex=-1);
	void SelectItem(int i);

	// Generated message map functions
	//{{AFX_MSG(CSelectAdapter)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelectItem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void Ondblclickitem(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTADAPTER_H__D41A3004_2B3D_11D0_9528_0020AF2A4474__INCLUDED_)
