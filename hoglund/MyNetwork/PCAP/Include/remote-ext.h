/*
 * Copyright (c) 2002 - 2003
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Politecnico di Torino nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */


#ifndef __REMOTE_EXT_H__
#define __REMOTE_EXT_H__



// Definition for Microsoft Visual Studio
#if _MSC_VER > 1000
#pragma once
#endif


/*!
	\file remote-ext.h

	The goal of this file it to include most of the new definitions that should be
	placed into the pcap.h file.

	It includes all new definitions (structures and functions like pcap_open().
    Some of the functions are not really a remote feature, but, right now, 
	they are placed here.
*/



// All this stuff is public
/*! \addtogroup remote_struct
	\{
*/




/*!
	\brief Defines the maximum buffer size in which address, port, interface names are kept.

	In case the adapter name or such is larger than this value, it is truncated.
	This is not used by the user; however it must be aware that an hostname / interface
	name longer than this value will be truncated.
*/
#define PCAP_BUF_SIZE 1024



/*!
	\brief Internal representation of the type of source in use (null, file, 
	remote/local interface).

	This indicates a file, i.e. the user want to open a capture from a local file.
*/
#define PCAP_SRC_FILE 2
/*!
	\brief Internal representation of the type of source in use (null, file, 
	remote/local interface).

	This indicates a local interface, i.e. the user want to open a capture from 
	a local interface. This does not involve the RPCAP protocol.
*/
#define PCAP_SRC_IFLOCAL 3
/*!
	\brief Internal representation of the type of source in use (null, file, 
	remote/local interface).

	This indicates a remote interface, i.e. the user want to open a capture from 
	an interface on a remote host. This does involve the RPCAP protocol.
*/
#define PCAP_SRC_IFREMOTE 4




/*!
	\brief String that will be used to determine the type of source in use (null, file,
	remote/local interface).

	This string will be prepended to the interface name in order to create a string
	that contains all the information required to open the source.

	This string indicates that the user wants to open a capture from a local file.
*/
#define PCAP_SRC_FILE_KEY "file://"
/*!
	\brief String that will be used to determine the type of source in use (null, file,
	remote/local interface).

	This string will be prepended to the interface name in order to create a string
	that contains all the information required to open the source.

	This string indicates that the user wants to open a capture from a network interface.
	This string does not necessarily involve the use of the RPCAP protocol. If the
	interface required resides on the local host, the RPCAP protocol is not involved
	and the local functions are used.
*/
#define PCAP_SRC_IF_KEY "rpcap://"






// Definitions needed by the new pcap_open()

	//! pcap_open(): selects promiscuous mode
#define PCAP_OPENFLAG_PROMISCUOUS		1
	//! pcap_open(): selects who has to open the data connection(remote capture)
#define PCAP_OPENFLAG_SERVEROPEN_DP		2
	//! pcap_open(): selects if the data connection has to be on top of UDP
#define PCAP_OPENFLAG_UDP_DP			4





/*!

	\brief This structure keeps the information needed to autheticate
	the user on a remote machine.
	
	The remote machine can either grant or refuse the access according 
	to the information provided.
	In case the NULL authentication is required, both 'username' and
	'password' can be NULL pointers.
	
	This structure is meaningless if the source is not a remote interface;
	in that case, the functions which requires such a structure can accept
	a NULL pointer as well.
*/
struct pcap_rmtauth
{
	/*!
		\brief Type of the authentication required.

		In order to provide maximum flexibility, we can support different types
		of authentication based on the value of this 'type' variable. The currently 
		supported authentication mathods are:
		- RPCAP_RMTAUTH_NULL: if the user does not provide an authentication method
		(this could enough if, for example, the RPCAP daemon allows connections 
		from trusted hosts only)
		- RPCAP_RMTAUTH_PWD: if the user is willing to provide a valid 
		username/password to authenticate itself on the remote machine. Username/
		password must be valid on the remote machine.

	*/
	int type;
	/*!
		\brief Zero-terminated string containing the username that has to be 
		used on the remote machine for authentication.
		
		This field is meaningless in case of the RPCAP_RMTAUTH_NULL authentication
		and it can be NULL.
	*/
	char *username;
	/*!
		\brief Zero-terminated string containing the password that has to be 
		used on the remote machine for authentication.
		
		This field is meaningless in case of the RPCAP_RMTAUTH_NULL authentication
		and it can be NULL.
	*/
	char *password;
};



/*!
	\brief It defines the NULL authentication.

	This value has to be used within the 'type' member of the pcap_rmtauth structure.
	The 'NULL' authentication has to be equal to 'zero', so that old applications
	can just put every field of struct pcap_rmtauth to zero, and it does work.
*/
#define RPCAP_RMTAUTH_NULL 0
/*!
	\brief It defines the username/password authentication.

	With this type of authentication, the RPCAP protocol will use the username/
	password provided to authenticate the user on the remote machine. If the
	authentication is successful (and the user has the right to open network devices)
	the RPCAP connection will continue; otherwise it will be dropped.

	This value has to be used within the 'type' member of the pcap_rmtauth structure.
*/
#define RPCAP_RMTAUTH_PWD 1

//! Maximum lenght of an host name (needed for the RPCAP active mode)
#define RPCAP_HOSTLIST_SIZE 1024


/*!
	\}
*/ // end of public documentation


// Exported functions
pcap_t *pcap_open(const char *source, int snaplen, int flags, int read_timeout, struct pcap_rmtauth *auth, char *errbuf);
int pcap_createsrcstr(char *source, int type, const char *host, const char *port, const char *name, char *errbuf);
int pcap_parsesrcstr(const char *source, int *type, char *host, char *port, char *name, char *errbuf);
int pcap_findalldevs_ex(char *host, char *port, SOCKET sockctrl, struct pcap_rmtauth *auth, pcap_if_t **alldevs, char *errbuf);
int pcap_remoteact_accept(const char *address, const char *port, const char *hostlist, char *connectinghost, struct pcap_rmtauth *auth, char *errbuf);
int pcap_remoteact_list(char *hostlist, char sep, int size, char *errbuf);
int pcap_remoteact_close(const char *host, char *errbuf);
void pcap_remoteact_cleanup();


#endif

