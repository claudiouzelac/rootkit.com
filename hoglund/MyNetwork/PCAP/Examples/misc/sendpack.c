#include <stdlib.h>
#include <stdio.h>

#include <pcap.h>

void usage();

void main(int argc, char **argv) {
	pcap_t *fp;
	char error[PCAP_ERRBUF_SIZE];
	u_char packet[100];
	int i;

	/* Check the validity of the command line */
	if (argc != 2)
	{
		printf("usage: %s inerface", argv[0]);
		return;
	}

	/* Open the output adapter */
	if((fp = pcap_open_live(argv[1], 100, 1, 1000, error) ) == NULL)
	{
		fprintf(stderr,"\nError opening adapter: %s\n", error);
		return;
	}

	/* Supposing to be on ethernet, set mac destination to 1:1:1:1:1:1 */
	packet[0]=1;
	packet[1]=1;
	packet[2]=1;
	packet[3]=1;
	packet[4]=1;
	packet[5]=1;
	
	/* set mac source to 2:2:2:2:2:2 */
	packet[6]=2;
	packet[7]=2;
	packet[8]=2;
	packet[9]=2;
	packet[10]=2;
	packet[11]=2;
	
	/* Fill the rest of the packet */
	for(i=12;i<100;i++){
		packet[i]=i%256;
	}

	/* Send down the packet */
	pcap_sendpacket(fp,
		packet,
		100);

	return;
}
