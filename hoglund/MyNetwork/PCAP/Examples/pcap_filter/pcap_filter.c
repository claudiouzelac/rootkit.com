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

#define MAX_PRINT 80
#define MAX_LINE 16


void dispatcher_handler(u_char *, 
	const struct pcap_pkthdr *, const u_char *);
void usage();

void main(int argc, char **argv) {
  pcap_t *fp;
  char error[PCAP_ERRBUF_SIZE];
  char *device=NULL;
  char *ifilename=NULL;
  char *ofilename=NULL;
  char *filter=NULL;
  int i=0;
  pcap_dumper_t *dumpfile;
  struct bpf_program fcode;
  bpf_u_int32 SubNet,NetMask;
  
  if (argc == 1)
  {
	  usage();
	  return;
  }
  
  for(i=1;i<argc;i+=2){
	  
	  switch (argv[i] [1])
	  {
		  
	  case 'i':
		  {
			  device=argv[i+1];
		  };
		  break;
		  
	  case 'f':
		  {
			  ifilename=argv[i+1];
		  };
		  break;
		  
	  case 'o':
		  {
			  ofilename=argv[i+1];
		  };
		  break;
		  
	  case 'p':
		  {
			  filter=argv[i+1];
		  };
		  break;
		  
		  
	  }
	  
  }
  
  //open a capture from the network
  if (device != NULL){
	  if ( (fp= pcap_open_live(device, 1514, 1, 20, error) ) == NULL)
	  {
		  fprintf(stderr,"\nUnable to open the adapter.\n");
		  return;
	  }
  }
  //open a capture from file
  else if (ifilename != NULL){
	  if ( (fp = pcap_open_offline(ifilename, NULL) ) == NULL)
	  {
		  fprintf(stderr,"\nUnable to find input file.\n");
		  return;
	  }
  }
  else usage();
  
  if(filter!=NULL){
	  
	  //obtain the subnet
	  if(device!=NULL){
		  if(pcap_lookupnet(device, &SubNet, &NetMask, error)<0){
			  fprintf(stderr,"\nUnable to obtain the netmask.\n");
			  return;
		  }
	  }
	  else NetMask=0xffffff; //If reading from file, we suppose to be in a C class network
	  
	  //compile the filter
	  if(pcap_compile(fp, &fcode, filter, 1, NetMask)<0){
		  fprintf(stderr,"\nError compiling filter: wrong syntax.\n");
		  return;
	  }
	  
	  //set the filter
	  if(pcap_setfilter(fp, &fcode)<0){
		  fprintf(stderr,"\nError setting the filter\n");
		  return;
	  }
	  
  }
  
  //open the dump file
  if (ofilename != NULL){
	  dumpfile=pcap_dump_open(fp, ofilename);
	  if(dumpfile==NULL){
		  fprintf(stderr,"\nError opening output file\n");
		  return;
	  }
  }
  else usage();
  
  //start the capture
  pcap_loop(fp, 0, dispatcher_handler, (unsigned char *)dumpfile);
  
}


//Callback function called by libpcap for every incoming packet
void dispatcher_handler(u_char *dumpfile,
						const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	u_int i=0;
	
	//save the packet on the dump file
	pcap_dump(dumpfile,header,pkt_data);

}


void usage()
{
	
	printf("\npf - generic packet filter.\nWritten by Loris Degioanni (loris@netgroup-serv.polito.it).");
    printf("\nUsage:\npf [-i interface] | [-f input_file_name] -o output_file_name -p packet_filter\n\n");
	exit(0);
}