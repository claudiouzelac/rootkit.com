// mfcguiDlg.h : header file
//

#if !defined(AFX_MFCGUIDLG_H__877E2A3D_E841_4EFF_AC29_E35CD88FE851__INCLUDED_)
#define AFX_MFCGUIDLG_H__877E2A3D_E841_4EFF_AC29_E35CD88FE851__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMfcguiDlg dialog

class CMfcguiDlg : public CDialog
{
// Construction
public:
	CMfcguiDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMfcguiDlg)
	enum { IDD = IDD_MFCGUI_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMfcguiDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMfcguiDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLoadDriver();
	afx_msg void OnUnloadDriver();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MFCGUIDLG_H__877E2A3D_E841_4EFF_AC29_E35CD88FE851__INCLUDED_)
