// mfcgui.h : main header file for the MFCGUI application
//

#if !defined(AFX_MFCGUI_H__E37A0AC8_C85C_4D6E_B620_9F8DAB15E083__INCLUDED_)
#define AFX_MFCGUI_H__E37A0AC8_C85C_4D6E_B620_9F8DAB15E083__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMfcguiApp:
// See mfcgui.cpp for the implementation of this class
//

class CMfcguiApp : public CWinApp
{
public:
	CMfcguiApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMfcguiApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMfcguiApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MFCGUI_H__E37A0AC8_C85C_4D6E_B620_9F8DAB15E083__INCLUDED_)
