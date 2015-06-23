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

#if !defined(AFX_NETMETERVIEW_H__3BF6CD2B_6C2B_11D3_B76F_0080C8DF82B3__INCLUDED_)
#define AFX_NETMETERVIEW_H__3BF6CD2B_6C2B_11D3_B76F_0080C8DF82B3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CNetmeterView : public CView
{
protected: // create from serialization only
	CRITICAL_SECTION Crit;
	CWinThread* thr;
	CString  StartCapture();
	void StopCapture();
	CNetmeterView();
	DECLARE_DYNCREATE(CNetmeterView)
// Attributes
public:
	CPen gridpen;
	CPen diagrampen1;
	CPen diagrampen2;
	UINT BytesCaptured;
	int lastval1,lastval2;
	CDC DrawBuffer;
	int time;
	int delta;
	int TimeSlice;
	CString Adapter;
	RECT wrett;
	HBITMAP hBitmap;
	CNetmeterDoc* GetDocument();
	void DrawBoard(CDC* pDC,RECT rett,int height1,int height2);
	void CreateBoard(CDC* pDC,CDC *DrawBuff,RECT rett);
// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetmeterView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNetmeterView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CNetmeterView)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSelAdapter();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in netmeterView.cpp
inline CNetmeterDoc* CNetmeterView::GetDocument()
   { return (CNetmeterDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETMETERVIEW_H__3BF6CD2B_6C2B_11D3_B76F_0080C8DF82B3__INCLUDED_)
