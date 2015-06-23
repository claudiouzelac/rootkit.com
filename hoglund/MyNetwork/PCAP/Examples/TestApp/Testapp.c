/*
 * Copyright (c) 1999 - 2002
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

#include <stdio.h>
#include <conio.h>


#include "..\..\Include\packet32.h"
#include "..\..\Include\ntddndis.h"

#define Max_Num_Adapter 10

// Prototypes

void PrintPackets(LPPACKET lpPacket);

char        AdapterList[Max_Num_Adapter][1024];

int main()
{

	//define a pointer to an ADAPTER structure

	LPADAPTER  lpAdapter = 0;

	//define a pointer to a PACKET structure

	LPPACKET   lpPacket;

	int        i;
	DWORD      dwErrorCode;

	DWORD dwVersion;
	DWORD dwWindowsMajorVersion;

	//unicode strings (winnt)
	WCHAR		AdapterName[8192]; // string that contains a list of the network adapters
	WCHAR		*temp,*temp1;

	//ascii strings (win95)
	char		AdapterNamea[8192]; // string that contains a list of the network adapters
	char		*tempa,*temp1a;


	int			AdapterNum=0,Open;
	ULONG		AdapterLength;
	
	char buffer[256000];  // buffer to hold the data coming from the driver

	struct bpf_stat stat;
	
	//
	// Obtain the name of the adapters installed on this machine
	//
	printf("Packet.dll test application. Library version:%s\n", PacketGetVersion());

	printf("Adapters installed:\n");
	i=0;	

	// the data returned by PacketGetAdapterNames is different in Win95 and in WinNT.
	// We have to check the os on which we are running
	dwVersion=GetVersion();
	dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	if (!(dwVersion >= 0x80000000 && dwWindowsMajorVersion >= 4))
	{  // Windows NT
		AdapterLength = sizeof(AdapterName);

		if(PacketGetAdapterNames(AdapterName,&AdapterLength)==FALSE){
			printf("Unable to retrieve the list of the adapters!\n");
			return -1;
		}
		temp=AdapterName;
		temp1=AdapterName;
		while ((*temp!='\0')||(*(temp-1)!='\0'))
		{
			if (*temp=='\0') 
			{
				memcpy(AdapterList[i],temp1,(temp-temp1)*2);
				temp1=temp+1;
				i++;
		}
	
		temp++;
		}
	  
		AdapterNum=i;
		for (i=0;i<AdapterNum;i++)
			wprintf(L"\n%d- %s\n",i+1,AdapterList[i]);
		printf("\n");
		
	}

	else	//windows 95
	{
		AdapterLength = sizeof(AdapterNamea);

		if(PacketGetAdapterNames(AdapterNamea,&AdapterLength)==FALSE){
			printf("Unable to retrieve the list of the adapters!\n");
			return -1;
		}
		tempa=AdapterNamea;
		temp1a=AdapterNamea;

		while ((*tempa!='\0')||(*(tempa-1)!='\0'))
		{
			if (*tempa=='\0') 
			{
				memcpy(AdapterList[i],temp1a,tempa-temp1a);
				temp1a=tempa+1;
				i++;
			}
			tempa++;
		}
		  
		AdapterNum=i;
		for (i=0;i<AdapterNum;i++)
			printf("\n%d- %s\n",i+1,AdapterList[i]);
		printf("\n");

	}

	do 
	{
		printf("Select the number of the adapter to open : ");
		scanf("%d",&Open);
		if (Open>AdapterNum) printf("\nThe number must be smaller than %d",AdapterNum); 
	} while (Open>AdapterNum);
	

	
	
	lpAdapter =   PacketOpenAdapter(AdapterList[Open-1]);
	
	if (!lpAdapter || (lpAdapter->hFile == INVALID_HANDLE_VALUE))
	{
		dwErrorCode=GetLastError();
		printf("Unable to open the adapter, Error Code : %lx\n",dwErrorCode); 

		return -1;
	}	

	// set the network adapter in promiscuous mode
	
	if(PacketSetHwFilter(lpAdapter,NDIS_PACKET_TYPE_PROMISCUOUS)==FALSE){
			printf("Warning: unable to set promiscuous mode!\n");
	}

	// set a 512K buffer in the driver
	if(PacketSetBuff(lpAdapter,512000)==FALSE){
			printf("Unable to set the kernel buffer!\n");
			return -1;
	}

	// set a 1 second read timeout
	if(PacketSetReadTimeout(lpAdapter,1000)==FALSE){
			printf("Warning: unable to set the read tiemout!\n");
	}

	//allocate and initialize a packet structure that will be used to
	//receive the packets.
	if((lpPacket = PacketAllocatePacket())==NULL){
		printf("\nError: failed to allocate the LPPACKET structure.");
		return (-1);
	}
	PacketInitPacket(lpPacket,(char*)buffer,256000);
	
	//main capture loop
	while(!kbhit())
	{
	    // capture the packets
		if(PacketReceivePacket(lpAdapter,lpPacket,TRUE)==FALSE){
			printf("Error: PacketReceivePacket failed");
			return (-1);
		}

		PrintPackets(lpPacket);
	}


	//print the capture statistics
	if(PacketGetStats(lpAdapter,&stat)==FALSE){
			printf("Warning: unable to get stats from the kernel!\n");
	}
	else
		printf("\n\n%d packets received.\n%d Packets lost",stat.bs_recv,stat.bs_drop);

	PacketFreePacket(lpPacket);
	
	// close the adapter and exit

	PacketCloseAdapter(lpAdapter);
	return (0);
}

// this function prints the content of a block of packets received from the driver

void PrintPackets(LPPACKET lpPacket)
{

	ULONG	i, j, ulLines, ulen, ulBytesReceived;
	char	*pChar, *pLine, *base;
	char	*buf;
	u_int off=0;
	u_int tlen,tlen1;
	struct bpf_hdr *hdr;
	
		ulBytesReceived = lpPacket->ulBytesReceived;


		buf = lpPacket->Buffer;

		off=0;

		while(off<ulBytesReceived){	
			if(kbhit())return;
		hdr=(struct bpf_hdr *)(buf+off);
		tlen1=hdr->bh_datalen;
		tlen=hdr->bh_caplen;
		printf("Packet length, captured portion: %ld, %ld\n", tlen1, tlen);
		off+=hdr->bh_hdrlen;

		ulLines = (tlen + 15) / 16;

		pChar =(char*)(buf+off);
		base=pChar;
		off=Packet_WORDALIGN(off+tlen);
		
		for ( i=0; i<ulLines; i++ )
		{

			pLine =pChar;

			printf( "%08lx : ", pChar-base );

			ulen=tlen;
			ulen = ( ulen > 16 ) ? 16 : ulen;
			tlen -= ulen;

			for ( j=0; j<ulen; j++ )
				printf( "%02x ", *(BYTE *)pChar++ );

			if ( ulen < 16 )
				printf( "%*s", (16-ulen)*3, " " );

			pChar = pLine;

			for ( j=0; j<ulen; j++, pChar++ )
				printf( "%c", isprint( *pChar ) ? *pChar : '.' );

			printf( "\n" );
		} 

		printf( "\n" );
		}
} 

 