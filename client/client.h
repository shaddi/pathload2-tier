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

#ifdef LOCAL
#define EXTERN
#else
#define EXTERN extern
#endif
#define NUM_SELECT_CALL 31
#define SCALE_FACTOR                2
#define MIN_TIME_INTERVAL           7         /* microsecond */
#define MAX_TIME_INTERVAL           200000    /* microsecond */


#define SCALE_FACTOR                2
#define NOTREND                     1
#define INCREASING                  2
#define GREY                        3

#define MEDIUM_LOSS_RATE            3
#define HIGH_LOSS_RATE              15
#define MIN_PARTITIONED_STREAM_LEN  5       /* number of packet */
#define MIN_STREAM_LEN              36      /* # of packets */
#define PCT_THRESHOLD               .55
#define PDT_THRESHOLD               .4
#define AGGREGATE_THRESHOLD         .6

#define NUM_RETRY_RATE_MISMATCH     0
#define NUM_RETRY_CS                1 
#define MAX_LOSSY_STREAM_FRACTION   50

#define DISCONNECTED				-99

#define INCR    1
#define NOTR    2
#define DISCARD 3
#define UNCL    4

#define SELECTPORT 55000 // the port client will be connecting to
#define MAXDATASIZE 25 // max number of bytes we can get at once
#define NUM_SELECT_SERVERS 1

EXTERN int send_fleet() ;
EXTERN int send_train();
EXTERN int send_ctr_mesg(char *ctr_buff, l_int32 ctr_code);
EXTERN l_int32 recv_ctr_mesg( char *ctr_buff);
EXTERN l_int32 send_latency() ;
EXTERN void min_sleeptime() ;
EXTERN l_int32 send_result();
EXTERN void order_int(int unord_arr[], int ord_arr[], int num_elems);
EXTERN double time_to_us_delta(struct timeval tv1, struct timeval tv2);
EXTERN l_int32 fleet_id_n ;
EXTERN l_int32 fleet_id  ;
EXTERN int sock_udp, sock_tcp, send_buff_sz, rcv_tcp_adrlen;
EXTERN struct sockaddr_in snd_udp_addr, snd_tcp_addr, rcv_udp_addr, rcv_tcp_addr,server_tcp_addr,server_udp_addr, client_udp_addr;
EXTERN l_int32 min_sleep_interval ; /* in usec */
EXTERN l_int32 min_timer_intr ; /* in usec */
EXTERN int gettimeofday_latency ;

EXTERN l_int32 num_servers;
EXTERN char serverList[MAXDATASIZE][MAXDATASIZE]; 

//EXTERN void order_dbl(double unord_arr[], double ord_arr[],int start, int num_elems);
EXTERN void order_float(float unord_arr[], float ord_arr[],int start, int num_elems);
//EXTERN void order_int(int unord_arr[], int ord_arr[], int num_elems);
EXTERN void help() ;
EXTERN int quiet ;

EXTERN l_int32 client_TCP_connection();
EXTERN void TCPecho();
EXTERN void client_cleanup();
EXTERN l_int32 client_UDP_connection();
EXTERN void UDPComm(l_int32 socket);
EXTERN l_int32 send_owd(l_int32 fleet_id, double owd[],l_int32 stream_len);

EXTERN l_int32 exp_flag  ;
EXTERN l_int32 grey_flag  ;
EXTERN double tr,old_tr ;
EXTERN double adr ;
EXTERN double tr_max ;
EXTERN double tr_min ;
EXTERN double grey_max  , grey_min  ;
EXTERN struct timeval first_time, second_time ;
EXTERN l_int32 exp_fleet_id  ;
EXTERN float bw_resol;
EXTERN l_uint32 min_time_interval, snd_latency, rcv_latency ;
EXTERN l_int32 repeat_fleet ;
EXTERN l_int32 counter ; /* # of consecutive times act-rate != req-rate */
EXTERN l_int32 converged_gmx_rmx ;
EXTERN l_int32 converged_gmn_rmn ;
EXTERN l_int32 converged_rmn_rmx ;
EXTERN l_int32 converged_gmx_rmx_tm ;
EXTERN l_int32 converged_gmn_rmn_tm ;
EXTERN l_int32 converged_rmn_rmx_tm ;
EXTERN double cur_actual_rate , cur_req_rate ;
EXTERN double prev_actual_rate , prev_req_rate ;
EXTERN double max_rate , min_rate ;
EXTERN l_int32 max_rate_flag , min_rate_flag ; 
EXTERN l_int32 bad_fleet_cs , bad_fleet_rate_mismatch ;
EXTERN l_int32 retry_fleet_cnt_cs , retry_fleet_cnt_rate_mismatch ;
EXTERN FILE *pathload_fp, *netlog_fp  ;
EXTERN double pct_metric[50], pdt_metric[50];
EXTERN l_int32 trend_idx ;
EXTERN double snd_time_interval ;
EXTERN l_int32 min_rsleep_time ; 
EXTERN l_int32 num ; 
EXTERN l_int32 slow;
EXTERN l_int32 interrupt_coalescence;
EXTERN l_int32 ic_flag ;
EXTERN l_int32 netlog ;
EXTERN char hostname[256];
EXTERN struct hostent *host_rcv;
EXTERN l_int32 repeat_1, repeat_2;
EXTERN l_int32 lower_bound;
EXTERN l_int32 increase_stream_len;
EXTERN struct timeval  exp_start_time ;


EXTERN char* getaddr();
EXTERN l_int32 recvfrom_latency();
EXTERN double  get_adr() ;
EXTERN l_int32 check_intr_coalescence(struct timeval time[],l_int32, l_int32 * );
EXTERN l_int32 recv_train(l_int32 , struct timeval *, l_int32 );
EXTERN double grey_bw_resolution() ;
//EXTERN int rint(float x);
//EXTERN int rint(double x);
//EXTERN void sig_alrm(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
EXTERN void terminate_gracefully(struct timeval exp_start_time,l_int32 disconnected);
EXTERN l_int32 calc_param();
EXTERN l_int32 equal(double a , double b);
EXTERN void radj_notrend(l_int32 flag) ;
EXTERN void radj_increasing() ;
EXTERN l_int32 rate_adjustment(l_int32 flag);
EXTERN l_int32 converged();
EXTERN void radj_greymax();
EXTERN void radj_greymin();
EXTERN void get_sending_rate();
EXTERN l_int32 less_than(double a, double b);
EXTERN l_int32 grtr_than(double a, double b);
EXTERN void get_pdt_trend(double pdt_metric[], l_int32 pdt_trend[], l_int32 pdt_result_cnt );
EXTERN void get_pct_trend(double pct_metric[], l_int32 pct_trend[], l_int32 pct_result_cnt );
EXTERN l_int32 aggregate_trend_result();
EXTERN l_int32 recv_fleet();
EXTERN l_int32 get_sndr_time_interval(double snd_time[],double *sum);
EXTERN void adjust_offset_to_zero(double owd[], l_int32 len);
EXTERN l_int32 eliminate_sndr_side_CS (double sndr_time_stamp[], l_int32 split_owd[]);
EXTERN l_int32 eliminate_b2b_pkt_ic ( double rcvr_time_stamp[] , double owd[],double owdfortd[], l_int32 low,l_int32 high,l_int32 *num_rcvr_cs,l_int32 *tmp_b2b );
EXTERN double pairwise_comparision_test (double array[] ,l_int32 start , l_int32 end);
EXTERN double pairwise_diff_test(double array[] ,l_int32 start , l_int32 end);
EXTERN l_int32 eliminate_rcvr_side_CS ( double rcvr_time_stamp[] , double owd[],double owdfortd[], l_int32 low,l_int32 high,l_int32 *num_rcvr_cs,l_int32 *tmp_b2b );
EXTERN void get_trend(double owdfortd[],l_int32 pkt_cnt );
EXTERN void order_dbl(double unord_arr[], double ord_arr[],l_int32 start, l_int32 num_elems);
EXTERN void print_contextswitch_info(l_int32 num_sndr_cs[], l_int32 num_rcvr_cs[],l_int32 discard[],l_int32 stream_cnt);
EXTERN l_int32 recv_result();
EXTERN char result[512],result1[512];
EXTERN void sig_sigusr1();
EXTERN void sig_alrm();
//int main123(void *t);

//#include "../gui/resource.h"
//#include "../gui/guidlg.h"
//EXTERN CGuiDlg *guidlg;
//EXTERN WSADATA wsa;
EXTERN l_int32 no_udp_sock,no_tcp_sock;
struct owdData
{
	unsigned int fleetid;
	unsigned int owd[MAX_STREAM_LEN] ;
};

struct owdData1
{
	unsigned int fleetid;
	unsigned int* owd;
};

