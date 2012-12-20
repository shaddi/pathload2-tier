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

//#include "stdafx.h"
#include "global.h"
#include "client.h"

l_int32 fleet_id_n ;
l_int32 fleet_id  ;
int sock_udp, sock_tcp, send_buff_sz, rcv_tcp_adrlen;
struct sockaddr_in snd_udp_addr, snd_tcp_addr, rcv_udp_addr, rcv_tcp_addr,server_tcp_addr,server_udp_addr, client_udp_addr;
l_int32 min_sleep_interval ; /* in usec */
l_int32 min_timer_intr ; /* in usec */
int gettimeofday_latency ;

//void order_dbl(double unord_arr[], double ord_arr[],int start, int num_elems);
void order_float(float unord_arr[], float ord_arr[],int start, int num_elems);
//void order_int(int unord_arr[], int ord_arr[], int num_elems);
int quiet ;
l_int32 exp_flag  ;
l_int32 grey_flag  ;
double tr,old_tr ;
double adr ;
double tr_max ;
double tr_min ;
double grey_max  , grey_min  ;
struct timeval first_time, second_time ;
l_int32 exp_fleet_id  ;
float bw_resol;
l_uint32 min_time_interval, snd_latency, rcv_latency ;
l_int32 repeat_fleet ;
l_int32 counter ; /* # of consecutive times act-rate != req-rate */
l_int32 converged_gmx_rmx ;
l_int32 converged_gmn_rmn ;
l_int32 converged_rmn_rmx ;
l_int32 converged_gmx_rmx_tm ;
l_int32 converged_gmn_rmn_tm ;
l_int32 converged_rmn_rmx_tm ;
double cur_actual_rate , cur_req_rate ;
double prev_actual_rate , prev_req_rate ;
double max_rate , min_rate ;
l_int32 max_rate_flag , min_rate_flag ; 
l_int32 bad_fleet_cs , bad_fleet_rate_mismatch ;
l_int32 retry_fleet_cnt_cs , retry_fleet_cnt_rate_mismatch ;
FILE *pathload_fp, *netlog_fp  ;
double pct_metric[50], pdt_metric[50];
l_int32 trend_idx ;
double snd_time_interval ;
l_int32 min_rsleep_time ; 
l_int32 num ; 
l_int32 slow;
l_int32 interrupt_coalescence;
l_int32 ic_flag ;
l_int32 netlog ;
char hostname[256];
struct hostent *host_rcv;
l_int32 repeat_1, repeat_2;
l_int32 lower_bound;
l_int32 increase_stream_len;
l_int32 requested_delay ;
struct timeval  exp_start_time ;
char result[512],result1[512];

/* Characteristics of Packet Stream */
l_int32 time_interval ;              /* in us */
l_uint32 transmission_rate ; /* in bps */
l_int32 cur_pkt_sz ;                 /* in bytes */
l_int32 max_pkt_sz ;                 /* in bytes */
l_int32 rcv_max_pkt_sz ;             /* in bytes */
l_int32 snd_max_pkt_sz ;             /* in bytes */
l_int32 num_stream ;
l_int32 stream_len ;                 /* in packets */

l_int32 verbose ;
l_int32 Verbose ;
l_int32 VVerbose ;

//LARGE_INTEGER freq;
//HANDLE hTimer;

//#include "resource.h"
//#include "guidlg.h"

//CGuiDlg *guidlg;


l_int32 num_servers;
char serverList[MAXDATASIZE][MAXDATASIZE]; 
l_int32 no_udp_sock,no_tcp_sock;
//struct WSAData wsa;
