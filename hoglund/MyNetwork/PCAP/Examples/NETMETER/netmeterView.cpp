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

#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>

#include "stdafx.h"
#include "netmeter.h"

#include "netmeterDoc.h"
#include "netmeterView.h"
#include "console.h"
#include "selectadapter.h"
#include "..\..\include\pcap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pcap_t *fp;
pcap_dumper_t *dumpfile;
struct _timeb OldTime;

/////////////////////////////////////////////////////////////////////////////
// CNetmeterView

IMPLEMENT_DYNCREATE(CNetmeterView, CView)

BEGIN_MESSAGE_MAP(CNetmeterView, CView)
	//{{AFX_MSG_MAP(CNetmeterView)
	ON_COMMAND(seladapter, OnSelAdapter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetmeterView construction/destruction

CNetmeterView::CNetmeterView()
{
	//init the values
	time=0;
	TimeSlice=1000;
	delta=5;
	lastval1=0;
	lastval2=0;
	Adapter="";
	BytesCaptured=0;
	gridpen.CreatePen(PS_SOLID,0,RGB(0,150,0));
	diagrampen1.CreatePen(PS_SOLID,0,RGB(255,255,0));
	diagrampen2.CreatePen(PS_SOLID,0,RGB(0,255,255));
	InitializeCriticalSection(&Crit);

	//begin the capture

	CString result;
	if ((result=StartCapture())!=""){
		AfxMessageBox("Error initializing the capture:\n"+result); 
		exit(0);
	}

}

CNetmeterView::~CNetmeterView()
{
//stop the capture and close libpcap 
	StopCapture();
}

BOOL CNetmeterView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// function that starts the capture

CString CNetmeterView::StartCapture()
{
	char *ebuf;
	char *myAdapter;


	CCapPars *pars=new CCapPars;

	pars->prg=this;

	ebuf=(char*)malloc(PCAP_ERRBUF_SIZE);

	if (Adapter==""){
		//If no adapter is defined choose the first
		myAdapter=(char*)pcap_lookupdev(ebuf);

		DWORD dwVersion=GetVersion();		//get the OS version
		DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
		if (dwVersion >= 0x80000000 && dwWindowsMajorVersion >= 4)// Windows '95
			Adapter=myAdapter;
		else //winNT
			Adapter=(WCHAR *)myAdapter;

		//open the adapter
		//note: the snaplen is 1 byte because we don't need the data
		//and the header of the packet, but only the length.
		//Snaplen is not set to 0 because with this value the filter
		//drops all the packet without copying them. Snaplen=0 can be 
		//used to count the packets with the minimun overhead on the system.

		if ( (fp= pcap_open_live((char*)myAdapter, 1, 1, 1000, ebuf) ) == NULL)
		{
			return "PCAP error: Error opening the adapter";
		}
	}
	else{
		if ( (fp= pcap_open_live(Adapter.GetBuffer(0), 1, 1, 1000, ebuf) ) == NULL)
		{
			return "PCAP error: Error opening the adapter";
		}
	}


	//this example shows a correct diagram only on 10Mbit Ethernets
	if(pcap_datalink(fp)!=1/*Ether10*/){
			AfxMessageBox("Warning: the nework adapter '"+Adapter+"' is not supported correctly by netmeter.\nNetmeter works correcly only with 10Mbit ethernet adapters.");
	}


	//set mode 1 (statistics mode).
	//When set in this mode, the capture driver DOES'T capture anything.
	//It only counts the packets and the number of bytes that satisfy the BPF filter,
	//and returns to the application this values every Mon.delay milliseconds.
	if(pcap_setmode(fp,MODE_STAT)==-1){
		pcap_close(fp);
		return "PCAP error setting count mode";
	}


	//We set a 256 bytes kernel buffer, big enough to contain the statistical summary created by the driver
	if(pcap_setbuff(fp, 256)<0){
		pcap_close(fp);
		return "Not enough memory to allocate the capture buffer\nTry to set a smaller buffer.";
	}

	pars->fp=fp;

	//start the capture thread
	thr=AfxBeginThread(MyThreadProc, pars);
	if(!thr) {
		pcap_close(fp);
		return "Error launching the capture thread.";
	}

	//get the time of the beginning of the capture
	_ftime( &OldTime );
	return "";


}

/////////////////////////////////////////////////////////////////////////////
// function that stops the capture

void CNetmeterView::StopCapture()
{

	HANDLE thread=thr->m_hThread;

	//kill the capture thread.
	BOOL res=TerminateThread(thread,0);
	
	if(res==false){
		AfxMessageBox("Fatal Error: cannot terminate the capture thread"); 
		exit(0);
	}
	//close the adapter
	pcap_close(fp);

	//wait the end of the capture thread
	WaitForSingleObject(thread,INFINITE);

}

/////////////////////////////////////////////////////////////////////////////
// Create a new blackboard

void CNetmeterView::CreateBoard(CDC* pDC,CDC *DrawBuff,RECT rett)
{
		int i;
		CDC dc;

		DrawBuff->CreateCompatibleDC(pDC);

		hBitmap=CreateCompatibleBitmap(*pDC,rett.right-rett.left,rett.bottom-rett.top);
		DrawBuff->SelectObject(hBitmap);

		DrawBuff->FillSolidRect(0,0,rett.right-rett.left,rett.bottom-rett.top,0);
		DrawBuff->SelectObject(gridpen);

		for(i=1;i<10;i++){
			DrawBuff->MoveTo(0,(rett.bottom-rett.top)*i/10);
			DrawBuff->LineTo(rett.right-rett.left,(rett.bottom-rett.top)*i/10);
		}
		for(i=1;i<=10;i++){
			DrawBuff->MoveTo((rett.right-rett.left)*i/10-1,0);
			DrawBuff->LineTo((rett.right-rett.left)*i/10-1,rett.bottom-rett.top);
		}

		DrawBuff->LineTo(rett.right-rett.left,rett.bottom-rett.top);

		DrawBoard(DrawBuff,rett,0,0);
		pDC->BitBlt( rett.left, rett.top, rett.right-rett.left, rett.bottom-rett.top, DrawBuff, 0, 0, SRCCOPY);

}

/////////////////////////////////////////////////////////////////////////////
// Scroll the blackboard and insert a new value

void CNetmeterView::DrawBoard(CDC* pDC,RECT rett,int height1,int height2)
{
	int i;
	int x,y;


	
	pDC->SelectObject(gridpen);
	pDC->BitBlt( 0, 0, rett.right-rett.left, rett.bottom-rett.top, pDC, delta, 0, SRCCOPY);
	pDC->FillSolidRect(rett.right-delta,0,rett.right,rett.bottom-rett.top,0);



	for(i=1;i<10;i++){
		pDC->MoveTo(rett.right-rett.left-delta,(rett.bottom-rett.top)*i/10);
		pDC->LineTo(rett.right-rett.left,(rett.bottom-rett.top)*i/10);
	}

	if(((time)*delta)/((rett.right-rett.left)/10) > ((time-1)*delta)/((rett.right-rett.left)/10))
	{
		pDC->MoveTo((rett.right-rett.left)*i/10-1,0);
		pDC->LineTo((rett.right-rett.left)*i/10-1,rett.bottom-rett.top);
	}

	// draw the line for the new PPS
	pDC->SelectObject(diagrampen2);

	x=rett.right-rett.left-delta-1;
	y=rett.bottom-rett.top-lastval2*(rett.bottom-rett.top)/100-1;
	pDC->MoveTo(x,y);
	pDC->SetPixelV(x,y,RGB(255,255,255));
	x=rett.right-rett.left-1;
	y=rett.bottom-rett.top-height2*(rett.bottom-rett.top)/100-1;
	pDC->LineTo(x,y);
	pDC->SetPixelV(x,y,RGB(255,255,255));
	lastval2=height2;

	// draw the line for the new BPS
	pDC->SelectObject(diagrampen1);

	x=rett.right-rett.left-delta-1;
	y=rett.bottom-rett.top-lastval1*(rett.bottom-rett.top)/100-1;
	pDC->MoveTo(x,y);
	pDC->SetPixelV(x,y,RGB(255,255,255));
	x=rett.right-rett.left-1;
	y=rett.bottom-rett.top-height1*(rett.bottom-rett.top)/100-1;
	pDC->LineTo(x,y);
	pDC->SetPixelV(x,y,RGB(255,255,255));
	lastval1=height1;
	
	//redraw
	if(time>0){
		Invalidate( FALSE );
		UpdateWindow();
	}
	time++;
}
/////////////////////////////////////////////////////////////////////////////
// CNetmeterView drawing

void CNetmeterView::OnDraw(CDC* pDC)
{
	CNetmeterDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CWnd* wind=pDC->GetWindow();
	wind->GetClientRect(&wrett);


	if(time==0){
		
		//write the name of the adapter in the title bar of the program's window
		CWnd *pater=this->GetParent();
		CString titlestr;
		CString tmpstr;

		tmpstr=Adapter.Right(12);
		if(tmpstr.GetLength()==Adapter.GetLength())
		 titlestr.Format("netmeter-%s",Adapter);
		else
		 titlestr.Format("netmeter-...%s",tmpstr);
		pater->SetWindowText(titlestr);
		
		CreateBoard(pDC,&DrawBuffer,wrett);
		time++;
	}

	if(time>0)pDC->BitBlt( wrett.left, wrett.top, wrett.right-wrett.left, wrett.bottom-wrett.top, &DrawBuffer, 0, 0, SRCCOPY);

}

/////////////////////////////////////////////////////////////////////////////
// CNetmeterView diagnostics

#ifdef _DEBUG
void CNetmeterView::AssertValid() const
{
	CView::AssertValid();
}

void CNetmeterView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNetmeterDoc* CNetmeterView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNetmeterDoc)));
	return (CNetmeterDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// Timer callback
/*
void CNetmeterView::OnTimer(UINT nIDEvent) 
{
struct _timeb CurrTime;
int DeltaTime;
int valuetoprint;

	_ftime( &CurrTime );
	DeltaTime=(CurrTime.time-OldTime.time)*1000+CurrTime.millitm-OldTime.millitm;
	if (DeltaTime==0)return;
	OldTime.time=CurrTime.time;
	OldTime.millitm=CurrTime.millitm;

	valuetoprint=((BytesCaptured*1000/DeltaTime)*100)/1250000;
	if(valuetoprint<0)valuetoprint=0;
	if(valuetoprint>100)valuetoprint=100;

	DrawBoard(&DrawBuffer,wrett, valuetoprint);

	EnterCriticalSection(&Crit);
	BytesCaptured=0;
	LeaveCriticalSection(&Crit);

	Invalidate( FALSE );
	UpdateWindow();
	CView::OnTimer(nIDEvent);
}
*/
/////////////////////////////////////////////////////////////////////////////
// "select the adapter" menu command


void CNetmeterView::OnSelAdapter() 
{
	CSelectAdapter dlg;

	//stop the current capture process
	StopCapture();

	//show the selectadapter dialog
	dlg.m_Adapter=Adapter;
	if (dlg.DoModal()==IDOK) Adapter=dlg.m_Adapter;
	
	//write the name of the adapter in the title bar of the program's window
	CWnd *pater=this->GetParent();
	CString titlestr;
	CString tmpstr;

	tmpstr=Adapter.Right(12);
	if(tmpstr.GetLength()==Adapter.GetLength())
	 titlestr.Format("netmeter-%s",Adapter);
	else
	 titlestr.Format("netmeter-...%s",tmpstr);
	pater->SetWindowText(titlestr);
	
	//start the new capture process
	StartCapture();
}
