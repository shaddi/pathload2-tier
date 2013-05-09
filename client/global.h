/*
 This file is part of pathload.

 pathload is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 pathload is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with pathload; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*-------------------------------------------------------------
   pathload : an end-to-end available bandwidth
              estimation tool
   Author   : Nachiket Deo          (nachiket.deo@gatech.edu)
              Partha Kanuparthy     (partha@cc.gatech.edu)
              Manish Jain           (jain@cc.gatech.edu)
              Constantinos Dovrolis (dovrolis@cc.gatech.edu )
   Release  : Ver 2.0
---------------------------------------------------------------*/

typedef int l_int32 ;
typedef unsigned int l_uint32 ;

#ifdef LOCAL
#define EXTERN
#else
#define EXTERN extern
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>

#define bzero(a, b) memset(a, 0x0, b)
  /* Code numbers sent from receiver to sender using the TCP control stream */
#define  CTR_CODE           0x80000000
#define  SEND_FLEET         0x00000001
#define  RECV_FLEET         0x00000002
#define  CONTINUE_STREAM    0x00000003
#define  FINISHED_STREAM    0x00000007
#define  TERMINATE          0x00000005
#define  ABORT_FLEET        0x00000006
#define  SEND_TRAIN         0x00000008 
#define  FINISHED_TRAIN     0x00000009
#define  BAD_TRAIN          0x0000000a
#define  GOOD_TRAIN         0x0000000b
#define	 AUTH				100
#define NEW				101

/* Port numbers (UDP for receiver, TCP for sender) */
#define UDPRCV_PORT         55001
#define SERVER_UDPRCV_PORT  55001
#define TCPSND_PORT         55002
#define UDP_BUFFER_SZ       400000    /* bytes */
#define TREND_ARRAY_LEN     50 


#define NUM_STREAM          12
#define MAX_STREAM_LEN      400
#define STREAM_LEN          100       /* # of packets */
#define MIN_PKT_SZ          200       /* bytes */

#define MAX_TRAIN           5
#define TRAIN_LEN           50

/* Characteristics of Packet Stream */
EXTERN l_int32 time_interval ;              /* in us */
EXTERN l_uint32 transmission_rate ; /* in bps */
EXTERN l_int32 cur_pkt_sz ;                 /* in bytes */
EXTERN l_int32 max_pkt_sz ;                 /* in bytes */
EXTERN l_int32 rcv_max_pkt_sz ;             /* in bytes */
EXTERN l_int32 snd_max_pkt_sz ;             /* in bytes */
EXTERN l_int32 num_stream ;
EXTERN l_int32 stream_len ;                 /* in packets */

EXTERN l_int32 verbose ;
EXTERN l_int32 Verbose ;
EXTERN l_int32 VVerbose ;

//EXTERN LARGE_INTEGER freq;
//int gettimeofday(struct timeval *tv, struct timezone *tz);

//EXTERN HANDLE hTimer;

#define SIGUSR1 10
