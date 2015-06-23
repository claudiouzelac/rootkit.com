PCAP_PATH = ../../lib
CFLAGS = -g -O -mno-cygwin -I ../../include

OBJS = tcptop.o
LIBS = -L ${PCAP_PATH} -lwpcap

all: ${OBJS}
	${CC} ${CFLAGS} -o tcptop.exe ${OBJS} ${LIBS}

clean:
	rm -f ${OBJS} tcptop.exe

.c.o:
	${CC} ${CFLAGS} -c -o $*.o $<
