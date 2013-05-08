CC=gcc

CFLAGS=-g -O2 -pthread  -DHAVE_PTHREAD=1 -DHAVE_LIBM=1 -DHAVE_LIBNSL=1 -DSTDC_HEADERS=1 -DHAVE_FCNTL_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_UNISTD_H=1 -DTIME_WITH_SYS_TIME=1 -DSIZEOF_INT=4 -DSIZEOF_LONG=4 -DHAVE_STRFTIME=1 -DHAVE_GETHOSTNAME=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_SOCKET=1  -DTHRLIB
CPPFLAGS=
LIBS=-lnsl -lm
LDFLAGS=

ROBJS=pathload_rcv.o pathload_rcv_func.o
OBJS=$(ROBJS)

TARGETS=pathload_server selection_server

all:${TARGETS} tidy

pathload_server: $(ROBJS)
	 $(CC) $(ROBJS) -o pathload_server $(LIBS) $(LDFLAGS) $(CFLAGS)

selection_server:
	$(CC) -o selection_server serverSelect.c

pathload_rcv.o pathload_rcv_func.o: pathload_gbls.h pathload_rcv.h

tidy:
	 rm -f ${OBJS} config.cache config.log  config.status

clean:
	 rm -f ${OBJS} config.cache config.log  config.status ${TARGETS}

distclean:
	 rm -f ${OBJS} ${TARGETS} config.cache config.log makefile config.status
