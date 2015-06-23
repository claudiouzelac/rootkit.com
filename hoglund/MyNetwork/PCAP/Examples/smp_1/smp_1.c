/*
 * Copyright (c) 2002 - 2003
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

#include <stdlib.h>
#include <stdio.h>

#include <pcap.h>

//#define LINE_LEN 16

main(int argc, char **argv) {
	
	pcap_if_t *alldevs, *d;
	pcap_t *fp;
	u_int inum, i=0;
	char errbuf[PCAP_ERRBUF_SIZE];
	int res;
	struct pcap_pkthdr *header;
	u_char *pkt_data;
	struct pcap_pkthdr old;

	char a[11];

	printf("SMP_1. (Copyright 2003 Gianluca Varenni - NetGroup, Politecnico di Torino)\n");
	printf("\nThis program tests the WinPcap kernel driver on SMP machines.\n");
	printf("The program tests that timestamps on the captured packets are consistent,\n");
	printf("and that the caplen is equal to the packet length.\n");
	printf("If there is an error, it will print out a message saying \"Inconsistent XXX\"\n");

	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
		exit(1);
	}
		
	/* Print the list */
	for(d=alldevs; d; d=d->next)
	{
		printf("%d. %s", ++i, d->name);
		if (d->description)
			printf(" (%s)\n", d->description);
		else
			printf(" (No description available)\n");
	}
		
	if(i==0)
	{
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
		return -1;
	}
		
	printf("Enter the interface number (1-%d):",i);
	scanf("%d", &inum);
		
	if(inum < 1 || inum > i)
	{
		printf("\nInterface number out of range.\n");
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return -1;
	}
		
	/* Jump to the selected adapter */
	for(d=alldevs, i=0; i< inum-1 ;d=d->next, i++);
	
	/* Open the device */
	if ( (fp= pcap_open_live(d->name, 65535, 1, 1000, errbuf) ) == NULL)
	{
		fprintf(stderr,"\nError opening adapter\n");
		return -1;
	}


	old.ts.tv_sec=0;
	old.ts.tv_usec=0;


	/* Read the packets */
	while((res = pcap_next_ex( fp, &header, &pkt_data)) >= 0){

		if(res == 0)
			continue;

		//check that caplen is equal to packet length
		if (header->caplen!=header->len)
			printf("Inconsistent header: CapLen %d\t Len %d\n",header->caplen,header->len);

		//check that timestamps always grow
		if ( old.ts.tv_sec > header->ts.tv_sec || (old.ts.tv_sec == header->ts.tv_sec  && old.ts.tv_usec > header->ts.tv_usec))
			printf("Inconsistent Timestamps! Old was %d.%.06d - New is %d.%.06d\n",old.ts.tv_sec,old.ts.tv_usec, header->ts.tv_sec,header->ts.tv_usec);

		old=*header;

	}

	if(res == -1){
		printf("Error reading the packets: %s\n", pcap_geterr(fp));
		return -1;
	}

	scanf("%s",a);

	return 0;
}
