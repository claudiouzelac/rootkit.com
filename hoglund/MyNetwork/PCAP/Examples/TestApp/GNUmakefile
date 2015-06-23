# Makefile for cygwin gcc
# Nate Lawson <nate@rootlabs.com>

PCAP_PATH = ../../lib
CFLAGS = -g -O -mno-cygwin -DDBG

OBJS = Testapp.o
LIBS = -L ${PCAP_PATH} -lPacket

all: ${OBJS}
	${CC} ${CFLAGS} -o TestApp.exe ${OBJS} ${LIBS}

clean:
	rm -f ${OBJS} TestApp.exe

.c.o:
	${CC} ${CFLAGS} -c -o $*.o $<
