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

#include <stdlib.h>
#include <stdio.h>

#include <pcap.h>

void usage();

void dispatcher_handler(u_char *, 
	const struct pcap_pkthdr *, const u_char *);


void main(int argc, char **argv) {
	pcap_t *fp;
	char error[PCAP_ERRBUF_SIZE];
	struct timeval st_ts;
	u_int netmask;
	struct bpf_program fcode;
  
	/* Check the validity of the command line */
	if (argc != 2)
	{
		usage();
		return;
	}
		
	/* Open the output adapter */
	if((fp = pcap_open_live(argv[1], 100, 1, 1000, error) ) == NULL)
	{
		fprintf(stderr,"\nError opening adapter: %s\n", error);
		return;
	}

    /* Don't care about netmask, it won't be used for this filter */
    netmask=0xffffff; 

    //compile the filter
    if(pcap_compile(fp, &fcode, "tcp", 1, netmask) <0 ){
        fprintf(stderr,"\nUnable to compile the packet filter. Check the syntax.\n");
        /* Free the device list */
        return;
    }
    
    //set the filter
    if(pcap_setfilter(fp, &fcode)<0){
        fprintf(stderr,"\nError setting the filter.\n");
        /* Free the device list */
        return;
    }

	/* Put the interface in statstics mode */
	pcap_setmode(fp, MODE_STAT);

	printf("TCP traffic summary:\n");

	/* Start the main loop */
	pcap_loop(fp, 0, dispatcher_handler, (PUCHAR)&st_ts);

	return;
}

void dispatcher_handler(u_char *state, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	struct timeval *old_ts = (struct timeval *)state;
	u_int delay;
	LARGE_INTEGER Bps,Pps;
	struct tm *ltime;
	char timestr[16];

	/* Calculate the delay in microseconds from the last sample. */
	/* This value is obtained from the timestamp that the associated with the sample. */
	delay=(header->ts.tv_sec - old_ts->tv_sec) * 1000000 - old_ts->tv_usec + header->ts.tv_usec;
	/* Get the number of Bits per second */
	Bps.QuadPart=(((*(LONGLONG*)(pkt_data + 8)) * 8 * 1000000) / (delay));
	/*                                            ^      ^
                                                  |      |
                                                  |      | 
                                                  |      |
                         converts bytes in bits --       |
                                                         |
                    delay is expressed in microseconds --
	*/

	/* Get the number of Packets per second */
	Pps.QuadPart=(((*(LONGLONG*)(pkt_data)) * 1000000) / (delay));

	/* Convert the timestamp to readable format */
	ltime=localtime(&header->ts.tv_sec);
	strftime( timestr, sizeof timestr, "%H:%M:%S", ltime);

	/* Print timestamp*/
	printf("%s ", timestr);

	/* Print the samples */
	printf("BPS=%I64u ", Bps.QuadPart);
	printf("PPS=%I64u\n", Pps.QuadPart);

	//store current timestamp
	old_ts->tv_sec=header->ts.tv_sec;
	old_ts->tv_usec=header->ts.tv_usec;
}


void usage()
{
	
	printf("\nShows the TCP traffic load, in bits per second and packets per second.\nCopyright (C) 2002 Loris Degioanni.\n");
	printf("\nUsage:\n");
	printf("\t tcptop adapter\n");
	printf("\t You can use \"WinDump -D\" if you don't know the name of your adapters.\n");

	exit(0);
}