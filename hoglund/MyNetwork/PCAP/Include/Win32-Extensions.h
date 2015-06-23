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

/* Definitions */

/*!
  \brief A queue of raw packets that will be sent to the network with pcap_sendqueue_transmit().
*/
struct pcap_send_queue{
	u_int maxlen;		///< Maximum size of the the queue, in bytes. This variable contains the size of the buffer field.
	u_int len;			///< Current size of the queue, in bytes.
	char *buffer;		///< Buffer containing the packets to be sent.
};

typedef struct pcap_send_queue pcap_send_queue;

#define		BPF_MEM_EX_IMM	0xc0
#define		BPF_MEM_EX_IND	0xe0

/*used for ST*/
#define		BPF_MEM_EX		0xc0
#define		BPF_TME					0x08

#define		BPF_LOOKUP				0x90   
#define		BPF_EXECUTE				0xa0
#define		BPF_INIT				0xb0
#define		BPF_VALIDATE			0xc0
#define		BPF_SET_ACTIVE			0xd0
#define		BPF_RESET				0xe0
#define		BPF_SET_MEMORY			0x80
#define		BPF_GET_REGISTER_VALUE	0x70
#define		BPF_SET_REGISTER_VALUE	0x60
#define		BPF_SET_WORKING			0x50
#define		BPF_SET_ACTIVE_READ		0x40
#define		BPF_SET_AUTODELETION	0x30
#define		BPF_SEPARATION			0xff

/* Prototypes */
pcap_send_queue* pcap_sendqueue_alloc(u_int memsize);

void pcap_sendqueue_destroy(pcap_send_queue* queue);

int pcap_sendqueue_queue(pcap_send_queue* queue, const struct pcap_pkthdr *pkt_header, const u_char *pkt_data);

u_int pcap_sendqueue_transmit(pcap_t *p, pcap_send_queue* queue, int sync);

HANDLE pcap_getevent(pcap_t *p);

#ifdef REMOTE
	struct pcap_stat *pcap_stats_ex(pcap_t *p);
#else
	int pcap_stats_ex(pcap_t *p, struct pcap_stat *ps);
#endif

int pcap_setuserbuffer(pcap_t *p, int size);

int pcap_live_dump(pcap_t *p, char *filename, int maxsize, int maxpacks);

int pcap_live_dump_ended(pcap_t *p, int sync);
