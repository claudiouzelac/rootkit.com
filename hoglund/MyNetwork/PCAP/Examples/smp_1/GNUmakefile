PCAP_PATH = ../../lib
CFLAGS = -g -O -mno-cygwin -I ../../include

OBJS = smp_1.o
LIBS = -L ${PCAP_PATH} -lwpcap

all: ${OBJS}
	${CC} ${CFLAGS} -o smp_1.exe ${OBJS} ${LIBS}

clean:
	rm -f ${OBJS} smp_1.exe

.c.o:
	${CC} ${CFLAGS} -c -o $*.o $<
