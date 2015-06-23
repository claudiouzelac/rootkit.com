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

#if !defined(AFX_LINECOLL_H__EE11D4A3_ED58_11D1_939A_000000000000__INCLUDED_)
#define AFX_LINECOLL_H__EE11D4A3_ED58_11D1_939A_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afx.h>
#include <afxtempl.h>

class LineCollection  
{
public:
	void clear();
	CString & operator =(CString &);
	int GetSize();
	int getLineCount(){return GetSize();};
	const char* line(int i){if (i<GetSize()) return (LPCTSTR) vect[i];else return NULL;};
	LineCollection(CString *s=NULL,int skip=0);
	LineCollection(CArchive &ar,int skip=0);
	virtual ~LineCollection();
	void Insert(CString& s){vect.SetSize(vect.GetSize()+1);vect[vect.GetSize()-1]=s;}
private:
	int m_Skip;
	CArray<CString,CString&> vect;
};

#endif // !defined(AFX_LINECOLL_H__EE11D4A3_ED58_11D1_939A_000000000000__INCLUDED_)
