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

#include "LineColl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LineCollection::LineCollection(CString *s,int skip)
{
	m_Skip=skip;
	if(s==NULL) return;
	*this=*s;
}

LineCollection::LineCollection(CArchive &ar,int skip)
{
	if (ar.IsStoring())
	{
		return;
	}
	else
	{
		m_Skip=skip;
		CString s;
		int i,j;
		vect.SetSize(0);
		for(i=0,j=0;ar.ReadString(s);i++)
		{
			if(m_Skip && s=="") continue;
			vect.SetSize(j+1);
			vect[j]=s;
			j++;
		}
	}
}

LineCollection::~LineCollection()
{
}

int LineCollection::GetSize()
{
	return vect.GetSize();
}

CString & LineCollection::operator =(CString &s)
{
	int i,l,n,t=0;
	l=s.GetLength();
	vect.SetSize(0);
	for(i=0,n=0;i<l;i++)
	{
		if ((s)[i]=='\n')
		{
			if(m_Skip && i>0 && (s)[i-1]=='\n') continue;
			vect.SetSize(t+1);
			vect[t]=(s).Mid(n,i-n-1);
			t++;
			n=i+1;
		}
	}
	return s;
}

void LineCollection::clear()
{
	vect.SetSize(0);
}

