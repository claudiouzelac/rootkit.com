# Makefile for cygwin gcc
# Nate Lawson <nate@rootlabs.com>

PCAP_PATH = ../../lib
CFLAGS = -g -O -mno-cygwin -I ../../include

OBJS = Tg.o
LIBS = -L ${PCAP_PATH} -lPacket

all: ${OBJS}
	${CC} ${CFLAGS} -o tg.exe ${OBJS} ${LIBS}

clean:
	rm -f ${OBJS} tg.exe

.c.o:
	${CC} ${CFLAGS} -c -o $*.o $<
