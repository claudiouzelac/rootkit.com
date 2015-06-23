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

#include "stdafx.h"
#include "resource.h"
#include "capdll.h"
#include "console.h"

HANDLE in,out,err;

DWORD d;
CCapPars* pObject;
CRITICAL_SECTION Crit;

// callback routine called by libpcap for every incoming packet
void dispatcher_handler(u_char *pParam,const struct pcap_pkthdr *header, const u_char *pkt_data)
{
UINT delay;
LARGE_INTEGER Bps,Pps;
CCapPars* pObject;

	pObject=(CCapPars*)pParam;
	//Calculate the delay in microseconds from the last sample.
	//This value is obtained from the timestamp that the capture driver
	//associates to the sample.
	delay=(header->ts.tv_sec-pObject->lasttime.tv_sec)*1000000-pObject->lasttime.tv_usec+header->ts.tv_usec;
	//get the number of Bits per second
	Bps.QuadPart=(((LONGLONG)(*(LONGLONG*)(pkt_data+8))*80)/(delay));
	//get the number of Packets per second
	Pps.QuadPart=(((LONGLONG)(*(LONGLONG*)(pkt_data))*100000000)/((LONGLONG)delay*14880));

	//store current timestamp
	pObject->lasttime.tv_sec=header->ts.tv_sec;
	pObject->lasttime.tv_usec=header->ts.tv_usec;

	pObject->prg->DrawBoard(&(pObject->prg->DrawBuffer),pObject->prg->wrett,Bps.LowPart,Pps.LowPart);
}

//main thread procedure: launches the capture and wait
UINT MyThreadProc( LPVOID pParam )
{   
	int i;

    if (pParam == NULL)
    return -1;    // illegal parameter
	pObject=(CCapPars*)pParam;

	//reset the timer
	pObject->lasttime.tv_sec=0;
	pObject->lasttime.tv_usec=0;

	//start the capture loop
	i = pcap_loop(pObject->fp, 0, dispatcher_handler, (PUCHAR)pParam);

	Sleep(INFINITE);

	return 0;
}

