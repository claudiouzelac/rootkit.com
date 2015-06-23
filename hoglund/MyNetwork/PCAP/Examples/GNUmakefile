# Makefile for cygwin gcc
# Nate Lawson <nate@rootlabs.com>

SUBDIRS = iflist kdump pktdump_ex sendcap tcptop UDPdump TG TestApp pcap_filter

all clean install uninstall: ${SUBDIRS}
	for subdir in ${SUBDIRS}; do \
		echo "Entering $$subdir"; \
		(cd $$subdir && ${MAKE} $@) \
	done;
