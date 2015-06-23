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

#define LINE_LEN 16

main(int argc, char **argv) {
	
	pcap_if_t *alldevs, *d;
	pcap_t *fp;
	u_int inum, i=0;
	char errbuf[PCAP_ERRBUF_SIZE];
	int res;
	struct pcap_pkthdr *header;
	u_char *pkt_data;

	printf("pktdump_ex: prints the packets of the network using WinPcap.\n");
	printf("\t Usage: pktdump_ex [-n adapter] | [-f file_name]\n\n");

	if(argc < 3){

		/* The user didn't provide a packet source: Retrieve the device list */
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
		if ( (fp= pcap_open_live(d->name, 100, 1, 20, errbuf) ) == NULL)
		{
			fprintf(stderr,"\nError opening adapter\n");
			return -1;
		}
	}
	else{
		
		/* The user provided a packet source: open it */
		switch (argv[1] [1])
		{
			
		case 'n':
			{
				/* Open a physical device */
				if ( (fp= pcap_open_live(argv[2], 100, 1, 20, errbuf) ) == NULL)
				{
					fprintf(stderr,"\nError opening adapter\n");
					return -1;
				}
			};
			break;
			
		case 'f':
			{
				/* Open a capture file */
				if ( (fp = pcap_open_offline(argv[2], errbuf) ) == NULL)
				{
					fprintf(stderr,"\nError opening dump file\n");
					return -1;
				}
			};
			break;
		}
	}

	/* Read the packets */
	while((res = pcap_next_ex( fp, &header, &pkt_data)) >= 0){

		if(res == 0)
			/* Timeout elapsed */
			continue;

		/* print pkt timestamp and pkt len */
		printf("%ld:%ld (%ld)\n", header->ts.tv_sec, header->ts.tv_usec, header->len);			
		
		/* Print the packet */
		for (i=1; (i < header->caplen + 1 ) ; i++)
		{
			printf("%.2x ", pkt_data[i-1]);
			if ( (i % LINE_LEN) == 0) printf("\n");
		}
		
		printf("\n\n");		
	}

	if(res == -1){
		printf("Error reading the packets: %s\n", pcap_geterr(fp));
		return -1;
	}

	return 0;
}
