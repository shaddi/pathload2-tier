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
//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif
//#define hostnm "altair"
//#pragma warning(disable:4996)

int client_TCP_connection()
{
	sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_tcp < 0) 
	{
		printf("ERROR : Opening TCP socket on Client");
//		guidlg->m_cTextout.SetWindowTextW(L"error opening TCP socket");
		exit (0);
		//exit(-1);
	}
	no_tcp_sock = 0;

	memset((char *) &server_tcp_addr, 0, sizeof(server_tcp_addr));
	server_tcp_addr.sin_family = AF_INET;
	server_tcp_addr.sin_port = htons(TCPSND_PORT);

	if ((host_rcv = gethostbyname(hostname)) == 0)
	{
		/* check if the user gave ipaddr */
		if ( ( server_tcp_addr.sin_addr.s_addr = inet_addr(hostname) ) == -1 )
		{
		  fprintf(stderr,"%s: unknown host\n", hostname);
		  return -1;
		}
	}
	memcpy((void *)&(server_tcp_addr.sin_addr.s_addr), host_rcv->h_addr, host_rcv->h_length);

	if (connect(sock_tcp,(struct sockaddr *)&server_tcp_addr,sizeof(server_tcp_addr)) < 0) 
	{
		//printf("ERROR : TCP : Could not connect to server\n");
		//printf("Make sure that pathload runs at server\n");
		return -1;
		//exit(-1);
	}
	//printf("\nTCP Connection Established\n");
	return 0;
}

void client_cleanup()
{
	if(!no_tcp_sock)
	{
		if( close(sock_tcp) < 0 )
		{
			printf("ERROR : Close Client TCP socket");
		}
	}
	if(!no_udp_sock)
	{
		if( close(sock_udp) < 0 )
		{
			printf("ERROR : Close Client UDP socket");
		}
	}
	// Release WinSock DLL
//	WSACleanup();
}


int client_UDP_connection()
{
	memset(&server_udp_addr,0, sizeof(server_udp_addr));
	server_udp_addr.sin_family = AF_INET;
	server_udp_addr.sin_port = htons(SERVER_UDPRCV_PORT);

	if ((host_rcv = gethostbyname(hostname)) == 0)
	{
		/* check if the user gave ipaddr */
		if ( ( server_udp_addr.sin_addr.s_addr = inet_addr(hostname) ) == -1 )
		{
		  fprintf(stderr,"%s: unknown host\n", hostname);
		  return-1;
		}
	}
	memcpy((void *)&(server_udp_addr.sin_addr.s_addr), host_rcv->h_addr, host_rcv->h_length);

	sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_udp < 0) 
	{
		printf("ERROR : Opening UDP socket on Client");
//		guidlg->m_cTextout.SetWindowTextW(L"Error in opening UDP socket");
		return -1;
	}
	no_udp_sock = 0;
	
	fflush(stdout);
	bzero((char *) &client_udp_addr, sizeof(client_udp_addr));
	client_udp_addr.sin_family = AF_INET; // host byte order
	client_udp_addr.sin_port = htons(UDPRCV_PORT); // short, network byte order
	client_udp_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP

	if (bind(sock_udp, (struct sockaddr *)&client_udp_addr,sizeof(struct sockaddr)) == -1) 
	{
		printf("Ports already in use");
	//	guidlg->m_cTextout.SetWindowTextW(L"Ports already in use");
		return -1;
	}
	
	send_buff_sz = UDP_BUFFER_SZ;
	if (setsockopt(sock_udp, SOL_SOCKET, SO_SNDBUF, (char*)&send_buff_sz, sizeof(send_buff_sz)) < 0)
	{
		send_buff_sz/=2;
		if (setsockopt(sock_udp, SOL_SOCKET, SO_SNDBUF, (char*)&send_buff_sz, sizeof(send_buff_sz)) < 0)
		{
			perror("setsockopt(SOL_SOCKET,SO_SNDBUF):");
			return -1;
		}
	}

	if(connect(sock_udp, (struct sockaddr *)&server_udp_addr, sizeof(server_udp_addr)) <0)
	{
		printf("\nERROR : Could not connect to server\n");
		printf("\nMake sure that pathload runs at server\n");
		return -1;
	}
	//printf("\nUDP connected to server\n");
	//printf("\nUDP stream established\n");

	return 0;
}

/*
Send a message through the control stream
*/
int send_result()
{
	if (send(sock_tcp, result1, 512, 0) == -1)
	{
		//printf("\nSND Error");
		printf("\nConnection to the server lost");
		return -1 ; 
	}
	else 
	{
		return 0;
	}
}

/*
Send a message through the control stream
*/
int send_ctr_mesg(char *ctr_buff, l_int32 ctr_code)
{
	l_int32 ctr_code_n = htonl(ctr_code);
	memcpy((void*)ctr_buff, &ctr_code_n, sizeof(l_int32));
	if (send(sock_tcp, ctr_buff, sizeof(l_int32), 0) == -1)
	{
	//	guidlg->m_cTextout.SetWindowTextW(L"Connection to the server lost");
		printf("\nConnection to the server lost (187)\nMake sure Pathload is running at server\n");
		exit (0) ; 
	}
	else 
	{
		return 0;
	}
}

/* 
Receive a message from the control stream 
*/
l_int32 recv_result()
{
	struct timeval select_tv;
	int numbytes;
	fd_set readset ;
	fflush(stdout);
	select_tv.tv_sec = 50 ; // if noctrl mesg for 50 sec, terminate 
	select_tv.tv_usec=0 ;
	FD_ZERO(&readset);
	FD_SET(sock_tcp,&readset);
	memset(result,0,512);
	if ( select(sock_tcp+1,&readset,NULL,NULL,&select_tv) > 0 )
	{
		numbytes = recv(sock_tcp, result, 512, 0);
			
		if ( numbytes < 512 ) 
		{
			printf("recv result error : less than 512\n");	
			return -1;
		}
		if ( numbytes < 0 ) 
		{
			printf("recv result error\n");		
			return -1 ;
		}
		result[numbytes-1] = '\0';
	}
	else
	{
		printf("Waiting for the Server to respond\n");
		return -1;
	}
	return 0;
}


/* 
Receive a message from the control stream 
*/
l_int32 recv_ctr_mesg(char *ctr_buff)
{
	struct timeval select_tv;
	fd_set readset ;
	l_int32 ctr_code,ret;
	select_tv.tv_sec = 50 ; // if noctrl mesg for 50 sec, terminate 
	select_tv.tv_usec=0 ;
	FD_ZERO(&readset);
	FD_SET(sock_tcp,&readset);
	memset(ctr_buff,0,4);
	if ( select(sock_tcp+1,&readset,NULL,NULL,&select_tv) > 0 )
	{
		ret = recv(sock_tcp, ctr_buff, sizeof(l_int32), 0);
		if ( ret < 0 ) return -1 ;
		if (ret ==0)
		{
			printf("\nConnection to the server lost\r\nMake sure Pathload is running at server\n");
		//	guidlg->m_cTextout.SetWindowTextW(L"Connection to the server lost");
			exit (0);
		}
		memcpy(&ctr_code, ctr_buff, sizeof(l_int32));
		return(ntohl(ctr_code));
	}
	else
	{
		printf("\nWaiting for the Server to respond\n");
	//	guidlg->m_cTextout.SetWindowTextW(L"Waiting for the Server to respond");
		return -1;
	}
}



void min_sleeptime()
{
	struct timeval sleep_time, time[NUM_SELECT_CALL] ;
	int res[NUM_SELECT_CALL] , ord_res[NUM_SELECT_CALL] ;
	int i ;
	l_int32 tm ;
	gettimeofday(&time[0], NULL);
	for(i=1;i<NUM_SELECT_CALL;i++)
	{
		sleep_time.tv_sec=0;sleep_time.tv_usec=1;
		gettimeofday(&time[i], NULL);
		select(0,NULL,NULL,NULL,&sleep_time);
	}
	for(i=1;i<NUM_SELECT_CALL;i++)
	{
		res[i-1] = (time[i].tv_sec-time[i-1].tv_sec)*1000000+
			time[i].tv_usec-time[i-1].tv_usec ;
#ifdef DEBUG
		printf("DEBUG :: %.2f ",(time[i].tv_sec-time[i-1].tv_sec)*1000000.+
			time[i].tv_usec-time[i-1].tv_usec);
		printf("DEBUG :: %d \n",res[i-1]);
#endif
	}
	order_int(res,ord_res,NUM_SELECT_CALL-1);
	min_sleep_interval=(ord_res[NUM_SELECT_CALL/2]+ord_res[NUM_SELECT_CALL/2+1])/2;
	if(min_sleep_interval < 1) min_sleep_interval = 1;
	gettimeofday(&time[0], NULL);
	tm = min_sleep_interval+min_sleep_interval/4;
	for(i=1;i<NUM_SELECT_CALL;i++)
	{
		sleep_time.tv_sec=0;sleep_time.tv_usec=tm;
		gettimeofday(&time[i], NULL);
		select(0,NULL,NULL,NULL,&sleep_time);
	}
	for(i=1;i<NUM_SELECT_CALL;i++)
	{
		res[i-1] = (time[i].tv_sec-time[i-1].tv_sec)*1000000+
			time[i].tv_usec-time[i-1].tv_usec ;
#ifdef DEBUG
		printf("DEBUG :: %.2f ",(time[i].tv_sec-time[i-1].tv_sec)*1000000.+
			time[i].tv_usec-time[i-1].tv_usec);
		printf("DEBUG :: %d \n",res[i-1]);
#endif
	}
	order_int(res,ord_res,NUM_SELECT_CALL-1);
	min_timer_intr=(ord_res[NUM_SELECT_CALL/2]+ord_res[NUM_SELECT_CALL/2+1])/2-min_sleep_interval;
	if(min_timer_intr < 1) min_timer_intr = 1;
#ifdef DEBUG
	printf("DEBUG :: min_sleep_interval %ld\n",min_sleep_interval);
	printf("DEBUG :: min_timer_intr %ld\n",min_timer_intr);
#endif
}

/* 
Order an array of int using bubblesort 
*/
void order_int(int unord_arr[], int ord_arr[], int num_elems)
{
	int i,j;
	int temp;
	for (i=0;i<num_elems;i++) ord_arr[i]=unord_arr[i];
	for (i=1;i<num_elems;i++) {
		for (j=i-1;j>=0;j--)
			if (ord_arr[j+1] < ord_arr[j])
			{
				temp=ord_arr[j]; 
				ord_arr[j]=ord_arr[j+1]; 
				ord_arr[j+1]=temp;
			}
			else break;
	}
}

l_int32 send_latency()
{
	char *pack_buf ;
	float min_OSdelta[50], ord_min_OSdelta[50];
	int i;
	socklen_t len ;
	int sock_udp ;
	struct timeval first_time ,current_time;
	struct sockaddr_in snd_udp_addr, rcv_udp_addr ;
	if ( max_pkt_sz == 0 ||  (pack_buf = (char *)malloc(max_pkt_sz*sizeof(char)) ) == NULL )
	{
		printf("ERROR : send_latency : unable to malloc %d bytes \n",max_pkt_sz);
	//	guidlg->m_cTextout.SetWindowTextW(L"Out of memory");
		exit (0);
	}
	if ((sock_udp=socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket(AF_INET,SOCK_DGRAM,0):");
	//	guidlg->m_cTextout.SetWindowTextW(L"Error creating socket");
		exit (0);
	}
	bzero((char*)&snd_udp_addr, sizeof(snd_udp_addr));
	snd_udp_addr.sin_family = AF_INET;
	snd_udp_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	snd_udp_addr.sin_port  = 0 ;
	if (bind(sock_udp, (struct sockaddr*)&snd_udp_addr, sizeof(snd_udp_addr)) < 0)
	{
		perror("bind(sock_udp):");
//		guidlg->m_cTextout.SetWindowTextW(L"Bind Error");
		exit (0);
	}

	len = sizeof(rcv_udp_addr);
	if (getsockname(sock_udp, (struct sockaddr *)&rcv_udp_addr, &len ) < 0 )
	{ 
		perror("getsockname");
//		guidlg->m_cTextout.SetWindowTextW(L"getsockname Error");
		exit (0);
	}

	if(connect(sock_udp,(struct sockaddr *)&rcv_udp_addr, sizeof(rcv_udp_addr)) < 0 )
	{
		perror("connect(sock_udp)");
//		guidlg->m_cTextout.SetWindowTextW(L"Connect Error");
		exit (0);
	}
	srand(getpid()); /* Create random payload; does it matter? */
	for (i=0; i<max_pkt_sz-1; i++) pack_buf[i]=(char)(rand()&0x000000ff);
	for (i=0; i<50; i++) 
	{
		gettimeofday(&first_time, NULL);
		if ( send(sock_udp, pack_buf, max_pkt_sz, 0) == -1 ) perror("sendto");
		gettimeofday(&current_time, NULL);
		recv(sock_udp, pack_buf, max_pkt_sz, 0); 
		min_OSdelta[i] = (float)time_to_us_delta(first_time, current_time);
	}
	/* Use median  of measured latencies to avoid outliers */
	order_float(min_OSdelta, ord_min_OSdelta,0, 50);
	if ( pack_buf != NULL ) free(pack_buf);
	return ((int)(ord_min_OSdelta[25])); 
}

/* 
Order an array of float using bubblesort 
*/
void order_float(float unord_arr[], float ord_arr[],int start, int num_elems)
{
	int i,j,k;
	float temp;
	for (i=start,k=0;i<start+num_elems;i++,k++) ord_arr[k]=unord_arr[i];
	for (i=1;i<num_elems;i++) {
		for (j=i-1;j>=0;j--)
			if (ord_arr[j+1] < ord_arr[j])
			{
				temp=ord_arr[j]; 
				ord_arr[j]=ord_arr[j+1]; 
				ord_arr[j+1]=temp;
			}
			else break;
	}
}

/*
Compute the time difference in microseconds between two timeval measurements
*/
double time_to_us_delta(struct timeval tv1, struct timeval tv2)
{
	double time_us;
	time_us= (double) ((tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec));
	return time_us;
}

int send_train() 
{
	struct timeval select_tv;
	char *pack_buf ;
	int train_id , train_id_n ; 
	int pack_id , pack_id_n ; 
	l_int32 ctr_code ;
	int ret_val ; 
	int i ;
	int train_len=0;
	char ctr_buff[8];

	if ( (pack_buf = (char *)malloc(max_pkt_sz*sizeof(char)) ) == NULL )
	{
		printf("ERROR : send_train : unable to malloc %d bytes \n",max_pkt_sz);
//		guidlg->m_cTextout.SetWindowTextW(L"Out of memory Error");
		exit (0);
	}

	train_id = 0 ; 
	srand(getpid()); /* Create random payload; does it matter? */
	for (i=0; i<max_pkt_sz-1; i++) pack_buf[i]=(char)(rand()&0x000000ff);
	while ( train_id < MAX_TRAIN )
	{
		if ( train_len == 5)
			train_len = 3;
		else 
			train_len = TRAIN_LEN - train_id*15;

		train_id_n = htonl(train_id) ;
		memcpy(pack_buf, &train_id_n, sizeof(l_int32));
		for (pack_id=0; pack_id <= train_len; pack_id++) 
		{
			pack_id_n = htonl(pack_id);
			memcpy(pack_buf+sizeof(l_int32), &pack_id_n, sizeof(l_int32));
			send(sock_udp, pack_buf, max_pkt_sz,0 ) ;
		}
		select_tv.tv_sec=0;select_tv.tv_usec=1000;
		select(0,NULL,NULL,NULL,&select_tv);
		ctr_code = FINISHED_TRAIN | CTR_CODE ;
		send_ctr_mesg(ctr_buff, ctr_code );
		if (( ret_val  = recv_ctr_mesg (ctr_buff)) == -1 )
			return -1;
		if ( (((ret_val & CTR_CODE) >> 31) == 1) && 
			((ret_val & 0x7fffffff) == BAD_TRAIN) )
		{
			train_id++ ;
			continue ;
		}
		else
		{
			free(pack_buf);
			return 0;
		}
	}
	free(pack_buf);
	return 0 ;
}


/*
Send fleet for avail-bw estimation. 
pkt_id and fleet_id start with 0.
*/
int send_fleet()
{
	struct timeval tmp1 , tmp2 ;
	struct timeval sleep_time ; 
	double  t1=0, t2 = 0 ;
	l_int32 ctr_code ;
	l_int32 pkt_id ;
	l_int32 pkt_cnt = 0 ;
	l_int32 stream_cnt = 0 ;
	l_int32 stream_id_n = 0 ;
	l_int32 usec_n , sec_n,pkt_id_n ;
	l_int32 sleep_tm_usec;
	int ret_val ;
	int stream_duration ;
	int diff ;
	int i ; 
	l_int32 tmp=0 ; 
	char *pkt_buf;
	char ctr_buff[8];

	if ( (pkt_buf = (char *)malloc(cur_pkt_sz*sizeof(char)) ) == NULL )
	{
		printf("ERROR : send_fleet : unable to malloc %d bytes \n",cur_pkt_sz);
//		guidlg->m_cTextout.SetWindowTextW(L"Out of memory Error");
		exit (0);
	}
	srand(getpid()); /* Create random payload; does it matter? */
	for (i=0; i<cur_pkt_sz-1; i++) pkt_buf[i]=(char)(rand()&0x000000ff);
	pkt_id = 0 ;
	t1 = (double)transmission_rate/1000000;
	printf("Sending fleet %d, Probing Rate %3.2f Mbps ", fleet_id,t1);
/*	CString str;
	str.Format(_T("Sending fleet %ld, Probing Rate %3.2f Mbps"), fleet_id,t1);
	guidlg->m_cTextout.SetWindowTextW(str);
	int pos = guidlg->m_cProgress.GetPos();
	if(pos <= 50 ){
		if(pos<30) pos += 10;
		else if(pos<40) pos += 5;
		else pos += 2;
	}
	else pos = 50;
	guidlg->m_cProgress.SetPos(pos);	*/
	while ( stream_cnt < num_stream )
	{
		if ( !quiet) printf("#");
		fflush(stdout);
		fleet_id_n = htonl(fleet_id) ;
		memcpy(pkt_buf, &fleet_id_n , sizeof(l_int32));
		stream_id_n = htonl(stream_cnt) ;
		memcpy(pkt_buf+sizeof(l_int32), &stream_id_n , sizeof(l_int32));
		gettimeofday(&tmp1 , NULL ) ;
		t1 = (double) tmp1.tv_sec * 1000000.0 +(double)tmp1.tv_usec ;
		for (pkt_cnt=0 ; pkt_cnt < stream_len ; pkt_cnt++)
		{
			pkt_id_n = htonl(pkt_cnt) ;
			memcpy(pkt_buf+2*sizeof(l_int32), &pkt_id_n , sizeof(l_int32));
			sec_n  = htonl ( tmp1.tv_sec ) ;
			memcpy(pkt_buf+3*sizeof(l_int32), &sec_n , sizeof(l_int32));
			usec_n  = htonl ( tmp1.tv_usec ) ;
			memcpy(pkt_buf+4*sizeof(l_int32), &usec_n , sizeof(l_int32));
			if ( send(sock_udp, pkt_buf, cur_pkt_sz,0 ) == -1 ) 
			{
				printf("send error"); 
				return -1;
			}
			gettimeofday(&tmp2, NULL) ;
			t2 = (double) tmp2.tv_sec * 1000000.0 +(double)tmp2.tv_usec ;
			tmp = (l_int32) (t2-t1);
			if ( pkt_cnt < ( stream_len - 1 ) )
			{
				l_int32 tm_remaining = time_interval - tmp;
				if ( tm_remaining > min_sleep_interval )
				{
					sleep_tm_usec = tm_remaining - (tm_remaining%min_timer_intr) 
						-min_timer_intr<200?2*min_timer_intr:min_timer_intr;
					sleep_time.tv_sec  = (int)(sleep_tm_usec / 1000000) ;
					sleep_time.tv_usec = sleep_tm_usec - sleep_time.tv_sec*1000000 ;
					select(1,NULL,NULL,NULL,&sleep_time);
				}
				gettimeofday(&tmp2,NULL) ;
				t2 = (double) tmp2.tv_sec * 1000000.0 +(double)tmp2.tv_usec ;
				diff = gettimeofday_latency>0?gettimeofday_latency-1:0;
				while((t2 - t1) < (time_interval-diff) )  
				{
					gettimeofday(&tmp2, NULL) ;
					t2 = (double) tmp2.tv_sec * 1000000.0 +(double)tmp2.tv_usec ;
				}
				tmp1 = tmp2 ;
				t1 = t2 ;
			}
		}
		/* Wait for 2000 usec and send End of 
		stream message along with streamid. */
		gettimeofday(&tmp2,NULL) ;
		t1 = (double) tmp2.tv_sec * 1000000.0 +(double)tmp2.tv_usec ;
		do
		{
			gettimeofday(&tmp2, NULL) ;
			t2 = (double) tmp2.tv_sec * 1000000.0 +(double)tmp2.tv_usec ;
		}while((t2 - t1) < 8000 ) ;
		ctr_code = FINISHED_STREAM | CTR_CODE ;
		if ( send_ctr_mesg(ctr_buff, ctr_code ) == -1 ) 
		{
			free(pkt_buf);
			perror("send_ctr_mesg : FINISHED_STREAM");
			return -1;
		}
		if ( send_ctr_mesg(ctr_buff, stream_cnt ) == -1 )
		{
			free(pkt_buf);
			return -1;
		}

		/* Wait for continue/cancel message from receiver.*/
		if( (ret_val = recv_ctr_mesg (ctr_buff )) == -1 )
		{
			free(pkt_buf);
			return -1;
		}
		
		if ( (((ret_val & CTR_CODE) >> 31) == 1) && 
			((ret_val & 0x7fffffff) == CONTINUE_STREAM) ) 
			stream_cnt++ ;
		else if ((((ret_val & CTR_CODE) >> 31) == 1 )&& 
			((ret_val & 0x7fffffff) == ABORT_FLEET) ) 
		{
			free(pkt_buf);
			return 0   ;
		}
		/* inter-stream latency is max (RTT,9*stream_duration)*/
		stream_duration = stream_len * time_interval ;
		if ( t2 - t1 < stream_duration * 9 )
		{
			/* release cpu if inter-stream gap is longer than min_sleep_time
			*/
			if ( t2 - t1 - stream_duration * 9 > min_sleep_interval )
			{
				sleep_tm_usec = time_interval - tmp - 
					((time_interval-tmp) % min_sleep_interval) - min_sleep_interval;
				sleep_time.tv_sec  = (int)(sleep_tm_usec / 1000000) ;
				sleep_time.tv_usec = sleep_tm_usec - sleep_time.tv_sec*1000000 ;
				select(1,NULL,NULL,NULL,&sleep_time);
				gettimeofday(&tmp2,NULL) ;
				t2 = (double) tmp2.tv_sec * 1000000.0 +(double)tmp2.tv_usec ;
			}
			/* busy wait for the remaining time */
			do
			{
				gettimeofday(&tmp2 , NULL ); 
				t2 = (double) tmp2.tv_sec * 1000000.0 +(double)tmp2.tv_usec ;
			}while((t2 - t1) < stream_duration * 9 ) ;
		}
		/* A hack for slow links */
		if ( stream_duration >= 500000 )
			break ;
	}
	free(pkt_buf);
	return 0 ;
}




double get_adr() 
{
	struct timeval select_tv,arrv_tv[MAX_STREAM_LEN] ; 
	double delta ;
	double bw_msr = 0;
	double bad_bw_msr[10] ;
	int num_bad_train=0 ; 
	int first = 1 ;
	double sum =0 ;
	l_int32 exp_train_id ; 
	l_int32 bad_train = 1; 
	l_int32 retry = 0 ;
	l_int32 ctr_code ; 
	l_int32 ctr_msg_rcvd ; 
	l_int32 train_len=0;
	l_int32 last=0,i;
	l_int32 spacecnt=24 ; 
	char ctr_buff[8];
	l_int32 num_burst;

	if (setsockopt(sock_udp, SOL_SOCKET, SO_RCVBUF, (char*)&send_buff_sz, sizeof(send_buff_sz)) < 0)
	{
		send_buff_sz/=2;
		if (setsockopt(sock_udp, SOL_SOCKET, SO_RCVBUF, (char*)&send_buff_sz, sizeof(send_buff_sz)) < 0)
		{
			perror("setsockopt(SOL_SOCKET,SO_RCVBUF):");
			return -1;
		}
	}

	if (Verbose)
		printf("  ADR [");
	fflush(stdout);
	/*fprintf(pathload_fp,"  ADR [");
	fflush(pathload_fp);*/
	ctr_code = SEND_TRAIN | CTR_CODE ;
	send_ctr_mesg(ctr_buff, ctr_code);
	exp_train_id = 0 ;
	for(i=0;i<100;i++)
	{
		arrv_tv[i].tv_sec=0;
		arrv_tv[i].tv_usec=0;
	}
	while ( retry < MAX_TRAIN && bad_train )
	{
		if ( train_len == 5)
			train_len = 3;
		else 
			train_len = TRAIN_LEN - exp_train_id*15;
		if (Verbose)
			printf(".");
		fflush(stdout);
		//fprintf(pathload_fp,".");
		spacecnt--;
		ctr_msg_rcvd = 0 ;
		bad_train = recv_train(exp_train_id, arrv_tv, train_len);
		/* Compute dispersion and bandwidth measurement */
		if (!bad_train) 
		{
			num_burst=0;
			interrupt_coalescence=check_intr_coalescence(arrv_tv,train_len,&num_burst);
			last=train_len;
			while(!arrv_tv[last].tv_sec) --last;
			delta = time_to_us_delta(arrv_tv[1], arrv_tv[last]);
			if(delta == 0.0) { printf("delta == 0!\n"); delta = 1.0; }
			bw_msr = ((28+max_pkt_sz) << 3) * (last-1) / delta;
			/* tell sender that it was agood train.*/
			ctr_code = GOOD_TRAIN | CTR_CODE ;
			send_ctr_mesg(ctr_buff, ctr_code ) ;
		}
		else
		{
			fflush(stdout);
			retry++ ;
			/* wait for atleast 10msec before requesting another train */
			last=train_len;
			while(!arrv_tv[last].tv_sec) --last;
			first=1 ; 
			while(!arrv_tv[first].tv_sec) ++first ; 
			delta = time_to_us_delta(arrv_tv[first], arrv_tv[last]);
			if(delta == 0.0) delta = 1.0;
			bad_bw_msr[num_bad_train++] = ((28+max_pkt_sz) << 3) * (last-first-1) / delta;
			select_tv.tv_sec=0;select_tv.tv_usec=10000;
			select(0,NULL,NULL,NULL,&select_tv);
			ctr_code = BAD_TRAIN | CTR_CODE ;
			send_ctr_mesg(ctr_buff, ctr_code ) ;
			exp_train_id++ ;
		}
	}

	if (Verbose)
	{ 
		i = spacecnt;
		putchar(']');
		while(--i>0)putchar(' ');
		printf(":: ");
	}
	/*fputc(']',pathload_fp);
	while(--spacecnt>0)fputc(' ',pathload_fp);
	fprintf(pathload_fp,":: ");*/
	if ( !bad_train)
	{
		if(Verbose)
			printf("%.2fMbps\n", bw_msr ) ;
		//fprintf(pathload_fp,"%.2fMbps\n", bw_msr ) ;
	}
	else
	{
		for ( i=0;i<num_bad_train;i++)
			if (finite(bad_bw_msr[i]))
				sum += bad_bw_msr[i] ;
		bw_msr = sum/num_bad_train ; 
		if(Verbose)
			printf("%.2fMbps (I)\n", bw_msr ) ;
		//fprintf(pathload_fp,"%.2fMbps (I)\n", bw_msr ) ;
	}
	return bw_msr ;
}

double grey_bw_resolution() 
{
	if ( adr )
		return (.05*adr<12?.05*adr:12) ;
	else 
		return min_rate ;
}

l_int32 check_intr_coalescence(struct timeval time[],l_int32 len, l_int32 *burst)
{
	double delta[MAX_STREAM_LEN];
	l_int32 b2b=0,tmp=0;
	l_int32 i;
	l_int32 min_gap;

	min_gap = MIN_TIME_INTERVAL > 3*rcv_latency ? MIN_TIME_INTERVAL : 3*rcv_latency ;
	for (i=2;i<len;i++)
	{
		delta[i] = time_to_us_delta(time[i-1],time[i]);
		if ( delta[i] <=  min_gap )
		{
			b2b++ ;
			tmp++;
		}
		else
		{
			if ( tmp >=3 )
			{
				(*burst)++;
				tmp=0;
			}
		}
	}

	//fprintf(stderr,"\tNumber of b2b %d, Number of burst %d\n",b2b,*burst);
	if ( b2b > .6*len )
	{
		return 1;
	}
	else return 0;
}

void sig_sigusr1()
{
  return;
}

void sig_alrm()
{
  terminate_gracefully(exp_start_time,0);
  exit(0);
}

/* Receive a complete packet train from the sender */
l_int32 recv_train( l_int32 exp_train_id, struct timeval *time,l_int32 train_len)
{
  struct sigaction sigstruct ;
  struct timeval current_time;
  struct timeval select_tv;
  fd_set readset;
  l_int32 ret_val ;
  l_int32 pack_id , exp_pack_id ;
  l_int32 bad_train = 0 ;
  l_int32 train_id  ;
  l_int32 rcvd=0;
  char *pack_buf ;
  char ctr_buff[8];

  exp_pack_id=0;

  if ( ( pack_buf = malloc(max_pkt_sz*sizeof(char))) == NULL )
  {
    printf("ERROR : unable to malloc %d bytes \n",max_pkt_sz);
    exit(-1);
  }
     
  sigstruct.sa_handler = sig_sigusr1 ;
  sigemptyset(&sigstruct.sa_mask);
  sigstruct.sa_flags = 0 ;
  #ifdef SA_INTERRUPT
    sigstruct.sa_flags |= SA_INTERRUPT ;
  #endif
  sigaction(SIGUSR1 , &sigstruct,NULL );

  do
  {
      int maxfd = (sock_tcp > sock_udp) ? sock_tcp : sock_udp;
      FD_ZERO(&readset);
      FD_SET(sock_tcp,&readset);
      FD_SET(sock_udp,&readset);
      select_tv.tv_sec=1000;select_tv.tv_usec=0;
      if (select(maxfd+1,&readset,NULL,NULL,&select_tv) > 0 )
      {
        if (FD_ISSET(sock_udp,&readset) )
        {
        	if (recvfrom(sock_udp, pack_buf, max_pkt_sz, 0, NULL, NULL) != -1)
        	{
          	gettimeofday(&current_time, NULL);
          	memcpy(&train_id, pack_buf, sizeof(l_int32));
          	train_id = ntohl(train_id) ;
          	memcpy(&pack_id, pack_buf+sizeof(l_int32), sizeof(l_int32));
          	pack_id=ntohl(pack_id);
          	if (train_id == exp_train_id && pack_id==exp_pack_id )
          	{
        			rcvd++;
        			time[pack_id] = current_time ;
        			exp_pack_id++;
          	}
          	else bad_train=1;
        	}
        } // end of FD_ISSET

        if ( FD_ISSET(sock_tcp,&readset) )
        {
          /* check the control connection.*/
          if (( ret_val = recv_ctr_mesg(ctr_buff)) != -1 )
          {
            if ( (((ret_val & CTR_CODE) >> 31) == 1) &&
                  ((ret_val & 0x7fffffff) == FINISHED_TRAIN ) )
            {
        		do 
				{
        		    fd_set readset1;
        		    FD_ZERO(&readset1);
        		    FD_SET(sock_udp,&readset1);
        		    select_tv.tv_sec=0;select_tv.tv_usec=1000;
        		    if (select(sock_udp+1,&readset1,NULL,NULL,&select_tv) > 0 )
        		    {
        		       	if (FD_ISSET(sock_udp,&readset1) )
        	        	{
//                    printf("PENDING UDP RECV\n");
							if (recvfrom(sock_udp, pack_buf, max_pkt_sz, 0, NULL, NULL) != -1)
                    		{
		                        gettimeofday(&current_time, NULL);
        		                memcpy(&train_id, pack_buf, sizeof(l_int32));
                		        train_id = ntohl(train_id) ;
                        		memcpy(&pack_id, pack_buf+sizeof(l_int32), sizeof(l_int32));
		                        pack_id=ntohl(pack_id);
        		                if (train_id == exp_train_id && pack_id==exp_pack_id )
                		        {
                        		    rcvd++;
		                            time[pack_id] = current_time ;
        		                    exp_pack_id++;
                		        }
                        		else bad_train=1;
                    		}
                		}
	                	else
    	            	{
        		            break;
                		}
            		}
            		else
            		{
	              	  break;
            		}
        		} while(1);   
        		break;
            }
          }
        }
      } // end of select
  } while (1);

  if ( rcvd != train_len+1 ) bad_train=1;
  gettimeofday(&time[pack_id+1], NULL);
  sigstruct.sa_handler = SIG_DFL ;
  sigemptyset(&sigstruct.sa_mask);
  sigstruct.sa_flags = 0 ;
  sigaction(SIGUSR1 , &sigstruct,NULL );
  free(pack_buf);
  return bad_train ;
}

void terminate_gracefully(struct timeval exp_start_time,l_int32 disconnected)
{
  l_int32 ctr_code;
  char ctr_buff[8],buff[26],temp[80];
  struct timeval exp_end_time;
  double min=0,max=0 ;

  if(disconnected)
  {
  	close(sock_tcp);
	close(sock_udp);
	printf("\nDisconnected from server\n");
//	guidlg->m_cTextout.SetWindowTextW(L"Connection to the server lost");
	exit (0);
  }
  ctr_code = TERMINATE | CTR_CODE;
  send_ctr_mesg(ctr_buff, ctr_code);
  gettimeofday(&exp_end_time, NULL);
  strncpy(buff, ctime(&(exp_end_time.tv_sec)), 24);
  buff[24] = '\0';
 
  if ( min_rate_flag )
  {
	  strcat(result1,"\nThe measured available bandwidth is less than the minimum possible probing rate.");
  }
  else if ( max_rate_flag && !interrupt_coalescence )
  {
	strcat(result1,"\nThe measured available bandwidth is higher than the maximum possible probing rate.");
      
  	if ( tr_min)
   	{
		strcat(result1,"\nAvailable bandwidth is at least ");
		sprintf(temp,"%.2f",max_rate);
		strcat(result1,temp);
		strcat(result1," (Mbps).");
		strcat(result1,"\nMeasurement duration : ");
		sprintf(temp,"%.2f",time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
		strcat(result1,temp);
		strcat(result1," sec.");
	}
  }
  else if (bad_fleet_cs && !interrupt_coalescence)
  {
    if (verbose || Verbose)
		printf("\nMeasurements terminated due to frequent context-switching at server.");
	strcat(result1,"\nMeasurements terminated due to frequent context-switching at server.");
    if ((tr_min&& tr_max) || (grey_min&&grey_max))
    {
    	if ( grey_min&& grey_max)
      	{
        	min = grey_min ; max = grey_max ;
			if(grey_min == grey_max)
			{
				min = tr_min; max = tr_max;
			}
      	}
      	else
      	{
        	min = tr_min ;max = tr_max ;
      	}
	//	if(min>max) max = min;
		if(min == 0)
		{
			if(max == 0)
			{
				strcat(result1,"\nMeasurements terminated due to insufficient probing rate.\nPlease try again later.");
			}
			else
			{
				strcat(result1,"\nAvailable bandwidth is at most ");
				sprintf(temp,"%.2f",max);
				strcat(result1,temp);
				strcat(result1," (Mbps).");
			}
		}
		else if(max == 0)
		{
			strcat(result1,"\nAvailable bandwidth is at least ");
			sprintf(temp,"%.2f",min);
			strcat(result1,temp);
			strcat(result1," (Mbps).");
		}
		else
		{
			strcat(result1,"\nAvailable bandwidth range : ");
			sprintf(temp,"%.2f",min);
			strcat(result1,temp);
			strcat(result1," - ");
			sprintf(temp,"%.2f.",max);
			strcat(result1,temp);
			strcat(result1,"\nMeasurement duration : ");
			sprintf(temp,"%.2f",time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
			strcat(result1,temp);
			strcat(result1," sec.");
		}
    }
  }
  else 
  {
	if ( !interrupt_coalescence && ((converged_gmx_rmx_tm && converged_gmn_rmn_tm) || converged_rmn_rmx_tm ))
    {
		strcat(result1,"\nMeasurements terminated due to insufficient probing rate.\nPlease try again later.");
      	if ( converged_rmn_rmx_tm )
      	{
        	min = tr_min ; max = tr_max;
      	}
      	else
      	{
        	min = grey_min ; max = grey_max ;
			if(grey_min == grey_max)
			{
				min = tr_min; max = tr_max;
			}
      	}
    }
    else if ( !interrupt_coalescence && converged_rmn_rmx )
    {
      min = tr_min ; max = tr_max ;
    }
    else if ( !interrupt_coalescence && converged_gmn_rmn && converged_gmx_rmx )
    {
    	if (Verbose)
        	printf("\nExiting due to grey bw resolution.");
	 // strcat(result1,"Exiting due to grey bw resolution#");
		min = grey_min ; max = grey_max;
	  	if(grey_min == grey_max)
	  	{
			min = tr_min; max = tr_max;
	  	}
    }
    else
    {
      min = tr_min ; max = tr_max;
    }

    if ( lower_bound)
    {
		strcat(result1,"\nMeasurements terminated due to interrupt coalescence at the receiving network interface card.\nAvailable bandwidth is greater than ");
		sprintf(temp,"%.2f",min);
		strcat(result1,temp);
		strcat(result1," (Mbps).");
	}
    else
	{
	//	if(min>max) max = min;
		if(min == 0)
		{
			if(max == 0)
			{
				strcat(result1,"\nMeasurements terminated due to insufficient probing rate.\nPlease try again later.");
			}
			else
			{
				strcat(result1,"\nAvailable bandwidth is at most ");
				sprintf(temp,"%.2f",max);
				strcat(result1,temp);
				strcat(result1," (Mbps).");
			}
		}
		else if(max == 0)
		{
			strcat(result1,"\nAvailable bandwidth is at least ");
			sprintf(temp,"%.2f",min);
			strcat(result1,temp);
			strcat(result1," (Mbps).");
		}
		else
		{
			strcat(result1,"\nAvailable bandwidth range : ");
			sprintf(temp,"%.2f",min);
			strcat(result1,temp);
			strcat(result1," - ");
			sprintf(temp,"%.2f",max);
			strcat(result1,temp);
			strcat(result1," (Mbps).");
		}
	}
	strcat(result1,"\nMeasurement duration : ");
	sprintf(temp,"%.2f",time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
	strcat(result1,temp);
	strcat(result1," sec.");
  }
  send_result();
}

/*
  calculates fleet param L,T .
  calc_param returns -1, if we have 
  reached to upper/lower limits of the 
  stream parameters like L,T .
  otherwise returns 0 .
*/
l_int32 calc_param()
{
  double tmp_tr ;
  l_int32 tmp ;
  l_int32 tmp_time_interval;
  if (tr < 150 )
  {
    time_interval  = 80>min_time_interval?80:min_time_interval ;
    cur_pkt_sz=rint(tr*time_interval/8.) - 28;
    if ( cur_pkt_sz < MIN_PKT_SZ )
    {
      cur_pkt_sz = MIN_PKT_SZ ;
      time_interval =rint ((cur_pkt_sz + 28)*8./tr) ;
      tr = ( cur_pkt_sz + 28 )*8. /time_interval ;
    }
    else if ( cur_pkt_sz > max_pkt_sz )
    {
      cur_pkt_sz = max_pkt_sz;
      time_interval = min_time_interval ;
      tmp_tr = ( cur_pkt_sz + 28 )*8. /time_interval ;
//      if ( equal(tr,tmp_tr))
          tr = tmp_tr;
//      else
//        return -1 ;
    }
  }
  else if ( tr < 600 )
  {
    tmp_tr = tr ;
    tmp_time_interval = rint(( max_pkt_sz + 28 )* 8 / tr) ; 
//    if ( cur_pkt_sz == max_pkt_sz && tmp_time_interval == time_interval )
//      return -1 ;
    time_interval = tmp_time_interval ; 
    tmp=rint(tr*time_interval/8.)-28;
    cur_pkt_sz=tmp<max_pkt_sz?tmp:max_pkt_sz;
    tr = ( cur_pkt_sz + 28 ) *8./time_interval ;
    if ((tr_min && (equal(tr,tr_min) || tr<tr_min)) 
        || (grey_max && tmp_tr>grey_max && (equal(tr,grey_max) || tr<grey_max))) 
    {
      do
      {
        --time_interval;
        cur_pkt_sz=rint(tr*time_interval/8.)-28;
      }while (cur_pkt_sz > max_pkt_sz);
      tr = ( cur_pkt_sz + 28 ) *8./time_interval ;
    }
  }
  else
  {
    cur_pkt_sz = max_pkt_sz ;
    time_interval = rint(( cur_pkt_sz + 28 )* 8 / tr) ; 
    tr = ( cur_pkt_sz + 28 ) *8./time_interval ;
    if ((tr_min && (equal(tr,tr_min) || tr<tr_min)) )
    {
      return -1 ;
    } 
    if( equal(tr,tr_max) )
    {
      tr_max = tr ;
      if ( grey_max )
      {
        converged_gmx_rmx_tm=1;
        if ( !converged_gmn_rmn && !converged_gmn_rmn_tm )
          radj_notrend(NOTREND);
        else return -1 ;
      }
      else 
      {
        converged_rmn_rmx=1;
        return -1 ;
      }
    }
  }
  return 0 ;
}

/*
   if a approx-equal b, return 1
   else 0
*/
l_int32 equal(double a , double b)
{
  double maxdiff ;
  if ( a<b?a:b < 500 ) maxdiff = 2.5 ;
  else maxdiff = 5.0 ;
  if ( abs( a - b ) / b <= .02  && abs(a-b) < maxdiff )
    return 1 ;
  else
    return 0;
}

/*
    Calculate next fleet rate
    when fleet showed NOTREND trend. 
*/
void radj_notrend(l_int32 flag) 
{
  if ( exp_flag )
     tr =  2*tr>max_rate?max_rate:2*tr ;
  else
  {
    if ( grey_min != 0 && grey_min <= tr_max )
    {
      if ( grey_min - tr_min <= grey_bw_resolution() )
      {
        converged_gmn_rmn = 1;
        radj_increasing() ;
      }
      else 
        tr =  (tr_min+grey_min)/2.<min_rate?min_rate:(tr_min+grey_min)/2. ;
    }
    else
      tr =  (tr_max + tr_min)/2.<min_rate?min_rate:(tr_min+tr_max)/2. ;
  }
  if(flag == GREY && tr == min_rate)
	tr = old_tr *2.;
}

/*
    Calculate next fleet rate
    when fleet showed INCREASING trend. 
*/
void radj_increasing() 
{
  if ( grey_max != 0 && grey_max >= tr_min )
  {
    if ( tr_max - grey_max <= grey_bw_resolution() )
    {
      converged_gmx_rmx = 1;
      exp_flag=0;
      if ( grey_min || tr_min )
        radj_notrend(INCREASING) ;
      else
      {
        if ( grey_min < grey_max )
          tr = grey_min/2. ;
        else
          tr = grey_max/2. ;
      }
    }
    else 
     tr = ( tr_max + grey_max)/2. ;
  }
  else
    tr =  (tr_max + tr_min)/2.<min_rate?min_rate:(tr_min+tr_max)/2. ;
}

/*
    dpending upon trend in fleet :-
    - update the state variables.
    - decide the next fleet rate
    return -1 when converged
*/
l_int32 rate_adjustment(l_int32 flag)
{
  l_int32 ret_val = 0 ;
  if( flag == INCREASING )
  {
    if ( max_rate_flag)
      max_rate_flag=0;
    if ( grey_max >= tr )
      grey_max = grey_min = 0 ;
    tr_max = tr ;
    if (!converged_gmx_rmx_tm ) 
    {
      if ( !converged() ) 
        radj_increasing() ;
      else
        ret_val=-1 ; //return -1; 
    }
    else
    {
      exp_flag = 0 ;
      if ( !converged() )
        radj_notrend(INCREASING) ;
    }
  }
  else if ( flag == NOTREND )
  {
    if ( grey_min < tr )
      grey_min = 0 ;
    if ( grey_max < tr )
      grey_max = grey_min = 0 ;
    if ( tr > tr_min )
	  tr_min = tr;
	/*if ( tr_min > tr_max )
	  tr_max = tr_min;*/

    //  tr_min =  tr_max = tr ;		//Added tr_max = tr!!!
    if ( !converged_gmn_rmn_tm && !converged() ) 
      radj_notrend(NOTREND) ;
    else
      ret_val=-1 ; //return -1 ;
  }
  else if ( flag == GREY )
  {
    if ( grey_max == 0 && grey_min == 0 )
      grey_max =  grey_min = tr ;
    if (tr==grey_max || tr>grey_max )
    {
      grey_max = tr ;
      if ( !converged_gmx_rmx_tm )
      {
        if ( !converged() )
          radj_greymax() ;
        else
          ret_val=-1 ; //return -1 ;
      }
      else
      {
        exp_flag = 0 ;
        if ( !converged() )
          radj_notrend(GREY) ;
        else
          ret_val=-1;
      }
    }
    else if ( tr < grey_min || grey_min == 0  ) 
    {
      grey_min = tr ;
      if ( !converged() )
        radj_greymin() ;
      else
        ret_val=-1 ; //return -1 ;
    }
  }

  if (Verbose)
  {
    printf("  Rmin-Rmax             :: %.2f-%.2fMbps\n",tr_min,tr_max);
    printf("  Gmin-Gmax             :: %.2f-%.2fMbps\n",grey_min,grey_max);
  }
//  fprintf(pathload_fp,"  Rmin-Rmax             :: %.2f-%.2fMbps\n",tr_min,tr_max);
//  fprintf(pathload_fp,"  Gmin-Gmax             :: %.2f-%.2fMbps\n",grey_min,grey_max);

  if ( ret_val == -1 )
    return -1 ;
  if ( tr >= max_rate )
    max_rate_flag++ ;

  if ( max_rate_flag > 1 )
     return -1 ;
  if ( min_rate_flag > 1 ) 
     return -1 ;
  transmission_rate = (l_int32) rint(1000000 * tr ) ;
  return 0 ;
}

/*
    test if Rmax and Rmin range is smaller than
    user specified bw resolution
    or
    if Gmin and Gmax range is smaller than grey
    bw resolution.
*/
l_int32 converged()
{
  int ret_val=0;
  if ( (converged_gmx_rmx_tm && converged_gmn_rmn_tm) || converged_rmn_rmx_tm  )
    ret_val=1;
  else if ( tr_max != 0 && tr_max != tr_min )
  {
    if ( tr_max - tr_min <= bw_resol )
    {
      converged_rmn_rmx=1;
      ret_val=1;
    }
    else if( tr_max - grey_max <= grey_bw_resolution() &&
              grey_min - tr_min <= grey_bw_resolution() )
    {
      converged_gmn_rmn = 1;
      converged_gmx_rmx = 1;
      ret_val=1;
    }
  }
  return ret_val ;
}

/*
    Calculate next fleet rate
    when fleet showed GREY trend. 
*/
void radj_greymax()
{
  if ( tr_max == 0 )
    tr = (tr+.5*tr)<max_rate?(tr+.5*tr):max_rate ;
  else if ( tr_max - grey_max <= grey_bw_resolution() )
  {
    converged_gmx_rmx = 1;
    radj_greymin() ;
  }
  else 
    tr = ( tr_max + grey_max)/2. ;
}

/*
    Calculate next fleet rate
    when fleet showed GREY trend. 
*/
void radj_greymin()
{
 if ( grey_min - tr_min <= grey_bw_resolution() )
 {
   converged_gmn_rmn = 1;
   radj_greymax() ;
 }
 else 
   tr = (tr_min+grey_min)/2.<min_rate?min_rate:(tr_min+grey_min)/2. ;
}

void get_sending_rate() 
{
 time_interval = (l_int32)(snd_time_interval/num);
 cur_req_rate = tr ;
 cur_actual_rate = (28 + cur_pkt_sz) * 8. / time_interval ;

 if( !equal(cur_req_rate, cur_actual_rate)  ) 
 {
   if( !grey_max && !grey_min )
   {
     if( tr_min && tr_max && (less_than(cur_actual_rate,tr_min)||equal(cur_actual_rate,tr_min)))
       converged_rmn_rmx_tm = 1;
     if( tr_min && tr_max && (less_than(tr_max,cur_actual_rate)||equal(tr_max,cur_actual_rate)))
       converged_rmn_rmx_tm = 1;
     
   }
   else if ( cur_req_rate < tr_max && cur_req_rate > grey_max )
   {
     if( !(less_than(cur_actual_rate,tr_max)&&grtr_than(cur_actual_rate,grey_max)) )
       converged_gmx_rmx_tm = 1;
   }
   else if ( cur_req_rate < grey_min && cur_req_rate > tr_min )
   {
     if( !(less_than(cur_actual_rate,grey_min) && grtr_than(cur_actual_rate,tr_min)) )
        converged_gmn_rmn_tm = 1;
   }
  }

  tr = cur_actual_rate ;
  transmission_rate = (l_int32) rint(1000000 * tr) ;
  if(Verbose)
    printf("  Fleet Parameter(act)  :: R=%.2fMbps, L=%dB, K=%dpackets, T=%dusec\n",cur_actual_rate, cur_pkt_sz , stream_len,time_interval);
//  fprintf(pathload_fp,"  Fleet Parameter(act)  :: R=%.2fMbps, L=%ldB, K=%ldpackets, T=%ldusec\n",cur_actual_rate,cur_pkt_sz,stream_len,time_interval);
  snd_time_interval=0;
  num=0;
}

l_int32 less_than(double a, double b)
{
  if ( !equal(a,b) && a < b)
    return 1;
  else 
    return 0;
}

l_int32 grtr_than(double a, double b)
{
  if ( !equal(a,b) && a > b)
    return 1;
  else 
    return 0;
}

/*
  returns : trend in fleet or -1 if more than 50% of stream were discarded 
*/
l_int32 aggregate_trend_result()
{
  l_int32 total=0 ,i_cnt = 0, n_cnt = 0;
  l_int32 num_dscrd_strm=0;
  l_int32 i=0;
  l_int32 pct_trend[TREND_ARRAY_LEN] , pdt_trend[TREND_ARRAY_LEN] ; 

  if (Verbose)
    printf("  PCT metric/stream[%2d] :: ",trend_idx); 
//  fprintf(pathload_fp,"  PCT metric/stream[%2d] :: ",trend_idx); 
  for (i=0; i < trend_idx;i++ )
  {
    if (Verbose)
      printf("%3.2f:",pct_metric[i]);
//    fprintf(pathload_fp,"%3.2f:",pct_metric[i]);
  }
  if (Verbose)
    printf("\n"); 
//  fprintf(pathload_fp,"\n"); 
  if (Verbose)
    printf("  PDT metric/stream[%2d] :: ",trend_idx); 
//  fprintf(pathload_fp,"  PDT metric/stream[%2d] :: ",trend_idx); 
  for (i=0; i < trend_idx;i++ )
  {
    if (Verbose)
      printf("%3.2f:",pdt_metric[i]);
//    fprintf(pathload_fp,"%3.2f:",pdt_metric[i]);
  }
  if (Verbose)
    printf("\n"); 
//  fprintf(pathload_fp,"\n"); 
  if (Verbose)
    printf("  PCT Trend/stream [%2d] :: ",trend_idx); 
//  fprintf(pathload_fp,"  PCT Trend/stream [%2d] :: ",trend_idx); 
  get_pct_trend(pct_metric,pct_trend,trend_idx);
  if (Verbose)
    printf("  PDT Trend/stream [%2d] :: ",trend_idx); 
//  fprintf(pathload_fp,"  PDT Trend/stream [%2d] :: ",trend_idx); 
  get_pdt_trend(pdt_metric,pdt_trend,trend_idx);
  
  if (Verbose)
    printf("  Trend per stream [%2d] :: ",trend_idx); 
//  fprintf(pathload_fp,"  Trend per stream [%2d] :: ",trend_idx); 
  for (i=0; i < trend_idx;i++ )
  {
    if ( pct_trend[i] == DISCARD || pdt_trend[i] == DISCARD )
    {
       if (Verbose)
         printf("d");
//       fprintf(pathload_fp,"d");
       num_dscrd_strm++ ;
    }
    else if ( pct_trend[i] == INCR &&  pdt_trend[i] == INCR )
    {
       if (Verbose)
         printf("I");
//       fprintf(pathload_fp,"I");
       i_cnt++;
    }
    else if ( pct_trend[i] == NOTR && pdt_trend[i] == NOTR )
    {
       if (Verbose)
         printf("N");
//       fprintf(pathload_fp,"N");
       n_cnt++;
    }
    else if ( pct_trend[i] == INCR && pdt_trend[i] == UNCL )
    {
       if (Verbose)
         printf("I");
//       fprintf(pathload_fp,"I");
       i_cnt++;
    }
    else if ( pct_trend[i] == NOTR && pdt_trend[i] == UNCL )
    {
       if (Verbose)
         printf("N");
//       fprintf(pathload_fp,"N");
       n_cnt++;
    }
    else if ( pdt_trend[i] == INCR && pct_trend[i] == UNCL )
    {
       if (Verbose)
         printf("I");
//       fprintf(pathload_fp,"I");
       i_cnt++;
    }
    else if ( pdt_trend[i] == NOTR && pct_trend[i] == UNCL )
    {
       if (Verbose)
         printf("N");
//       fprintf(pathload_fp,"N");
       n_cnt++ ;
    }
    else
    {
       if (Verbose)
         printf("U");
//       fprintf(pathload_fp,"U");
    }
    total++ ;
  }
  if (Verbose) printf("\n"); 
//  fprintf(pathload_fp,"\n"); 

  /* check whether number of usable streams is 
     atleast 50% of requested number of streams */
  total-=num_dscrd_strm ;
  if ( total < num_stream/2 && !slow && !interrupt_coalescence)
  {
    bad_fleet_cs = 1 ;
    retry_fleet_cnt_cs++  ;
    return -1 ;
  }
  else
  {
    bad_fleet_cs = 0 ;
    retry_fleet_cnt_cs=0;
  }

  if( (double)i_cnt/(total) >= AGGREGATE_THRESHOLD )
  {
    if (Verbose)
      printf("  Aggregate trend       :: INCREASING\n");
//    fprintf(pathload_fp,"  Aggregate trend       :: INCREASING\n");
    return INCREASING ;
  }
  else if( (double)n_cnt/(total) >= AGGREGATE_THRESHOLD )
  {
    if (Verbose)
      printf("  Aggregate trend       :: NO TREND\n");
//    fprintf(pathload_fp,"  Aggregate trend       :: NO TREND\n");
    return NOTREND ;
  }
  else 
  {
    if (Verbose)
      printf("  Aggregate trend       :: GREY\n");
//    fprintf(pathload_fp,"  Aggregate trend       :: GREY\n");
    return GREY ;
  }
}

void get_pct_trend(double pct_metric[], l_int32 pct_trend[], l_int32 pct_result_cnt )
{
 l_int32 i ;
 for (i=0; i < pct_result_cnt;i++ )
 {
   pct_trend[i] = UNCL ;
   if ( pct_metric[i] == -1  )
   {
     if (Verbose)
       printf("d");
//     fprintf(pathload_fp,"d");
     pct_trend[i] = DISCARD ;
   }
   else if ( pct_metric[i] > 1.1 * PCT_THRESHOLD )
   {
     if (Verbose)
       printf("I");
//     fprintf(pathload_fp,"I");
     pct_trend[i] = INCR ;
   }
   else if ( pct_metric[i] < .9 * PCT_THRESHOLD )
   {
     if (Verbose)
       printf("N");
//     fprintf(pathload_fp,"N");
     pct_trend[i] = NOTR ;
   }
   else if(pct_metric[i] <= PCT_THRESHOLD*1.1 && pct_metric[i] >= PCT_THRESHOLD*.9 )
   {
     if (Verbose)
       printf("U");
 //    fprintf(pathload_fp,"U");
     pct_trend[i] = UNCL ;
   }
 }
 if (Verbose)
   printf("\n");
// fprintf(pathload_fp,"\n");
}

void get_pdt_trend(double pdt_metric[], l_int32 pdt_trend[], l_int32 pdt_result_cnt )
{
 l_int32 i ;
 for (i=0; i < pdt_result_cnt;i++ )
 {
    if ( pdt_metric[i] == 2  )
    {
 if (Verbose)
        printf("d");
//        fprintf(pathload_fp,"d");
        pdt_trend[i] = DISCARD ;
    }
    else if ( pdt_metric[i] > 1.1 * PDT_THRESHOLD )
    {
 if (Verbose)
        printf("I");
//        fprintf(pathload_fp,"I");
        pdt_trend[i] = INCR ;
    }
    else if ( pdt_metric[i] < .9 * PDT_THRESHOLD )
    {
 if (Verbose)
        printf("N");
//        fprintf(pathload_fp,"N");
        pdt_trend[i] = NOTR ;
    }
    else if ( pdt_metric[i] <= PDT_THRESHOLD*1.1 && pdt_metric[i] >= PDT_THRESHOLD*.9 )
    {
 if (Verbose)
        printf("U");
//        fprintf(pathload_fp,"U");
        pdt_trend[i] = UNCL ;
    }
 }
 if (Verbose)
 printf("\n");
// fprintf(pathload_fp,"\n");
}






/* 
   Receive N streams .
   After each stream, compute the loss rate.
   Mark a stream "lossy" , if losss rate in 
   that stream is more than a threshold.
*/
l_int32 recv_fleet()
{
  struct sigaction sigstruct ;
  struct timeval snd_tv[MAX_STREAM_LEN], arrv_tv[MAX_STREAM_LEN];
  struct timeval current_time, first_time;
  double pkt_loss_rate ;
  double owd[MAX_STREAM_LEN] ;
  double snd_tm[MAX_STREAM_LEN] ;
  double arrv_tm[MAX_STREAM_LEN];
  l_int32 ctr_code ;
  l_int32 pkt_lost = 0 ;
  l_int32 stream_id_n , stream_id=0 ;
  l_int32 total_pkt_rcvd=0 ,pkt_rcvd = 0 ;
  l_int32 pkt_id = 0 ;
  l_int32 pkt_id_n = 0 ;
  l_int32 exp_pkt_id = 0 ;
  l_int32 stream_cnt = 0 ; /* 0->n*/
  l_int32 fleet_id  , fleet_id_n = 0 ;
  l_int32 lossy_stream = 0 ;
  l_int32 return_val = 0 ;
  l_int32 finished_stream = 0 ;
  l_int32 stream_duration ;
  l_int32 num_sndr_cs[20],num_rcvr_cs[20];
  char ctr_buff[8];
  double owdfortd[MAX_STREAM_LEN];
  l_int32 num_substream,substream[MAX_STREAM_LEN];
  l_int32 low,high,len,j;
  l_int32 b2b_pkt_per_stream[20];
  l_int32 tmp_b2b;
  l_int32 num_bursts;
  l_int32 abort_fleet=0;
  struct timeval select_tv;
  fd_set readset;
  l_int32 ret_val,ret ;
  char *pkt_buf ;

  sigstruct.sa_handler = sig_sigusr1 ;
  sigemptyset(&sigstruct.sa_mask);
  sigstruct.sa_flags = 0 ;
  #ifdef SA_INTERRUPT
    sigstruct.sa_flags |= SA_INTERRUPT ;
  #endif
  sigaction(SIGUSR1 , &sigstruct,NULL );

  if ( (pkt_buf = (char *)malloc(cur_pkt_sz*sizeof(char)) ) == NULL )
  {
    printf("ERROR : unable to malloc %d bytes \n",cur_pkt_sz);
//	guidlg->m_cTextout.SetWindowTextW(L"Out of memory");
    exit(0);
  }
  trend_idx=0;
  ic_flag = 0;
//	CString str;
//	str.Format(_T("Receiving fleet %ld, Probing Rate %3.2f Mbps"), exp_fleet_id,tr);
//	guidlg->m_cTextout.SetWindowTextW(str);
//	int pos = guidlg->m_cProgress.GetPos();
//	if(pos >= 50 ){
//		if(pos<80) pos += 10;
//		else if(pos<90) pos += 5;
//		else pos += 2;
//	}
//	else pos = 100;
//	guidlg->m_cProgress.SetPos(pos);
	printf("\nReceiving Fleet %d, Probing Rate %.2fMbps ",exp_fleet_id,tr);
  if(Verbose)
  {
//    printf("\nReceiving Fleet %d\n",exp_fleet_id);
    printf("\n  Fleet Parameter(req)  :: R=%.2fMbps, L=%dB, K=%dpackets, \
T=%dusec\n",tr, cur_pkt_sz , stream_len,time_interval) ;
  }
//  fprintf(pathload_fp,"\nReceiving Fleet %ld\n",exp_fleet_id);
//  fprintf(pathload_fp,"  Fleet Parameter(req)  :: R=%.2fMbps, L=%ldB, 
//  K=%ldpackets, T=%ldusec\n",tr, cur_pkt_sz , stream_len,time_interval);
  if(Verbose)
    printf("  Lossrate per stream   :: ");
//  fprintf(pathload_fp,"  Lossrate per stream   :: ");

  while ( stream_cnt < num_stream  )
  {
    pkt_lost = 0 ;
    first_time.tv_sec = 0 ;
    for (j=0; j < stream_len; j++ )
    {
      snd_tv[j].tv_sec=0 ; snd_tv[j].tv_usec=0 ;
      arrv_tv[j].tv_sec=0; arrv_tv[j].tv_usec=0;
    }

    /* Receive K packets of ith stream */
    while(1)
    {
      int maxfd = (sock_tcp > sock_udp) ? sock_tcp : sock_udp;
      FD_ZERO(&readset);
      FD_SET(sock_udp,&readset);
      FD_SET(sock_tcp,&readset);
      select_tv.tv_sec=1000;select_tv.tv_usec=0;
	  if(select(maxfd+1,&readset,NULL,NULL,&select_tv) <= 0)
	  {
	  	printf("\nTimeout\n");
		free(pkt_buf);
		terminate_gracefully(exp_start_time,1);
		return DISCONNECTED;
	  }
      else// if (select(sock_tcp+1,&readset,NULL,NULL,&select_tv) > 0 ) 
      {
        if (FD_ISSET(sock_udp,&readset) )
        {
			ret = recvfrom(sock_udp,pkt_buf,cur_pkt_sz,0,NULL,NULL);
//			TRACE("\nsize = %d ret %d\n",cur_pkt_sz, ret);
		  if(ret < 0)
		  {
	  		printf("\nClient Disconnected");
			free(pkt_buf);
			terminate_gracefully(exp_start_time,1);
			return DISCONNECTED;
		  }
		  else if(ret == 0)
		  {
			  fflush(stdout);
			  terminate_gracefully(exp_start_time,1);
			return DISCONNECTED;
		  }
		  else// if(ret > 0)
          {
            gettimeofday(&current_time,NULL);
	    //ioctl(sock_udp, SIOCGSTAMP, &current_time);
            memcpy(&fleet_id_n,pkt_buf , sizeof(l_int32));
            fleet_id = ntohl(fleet_id_n) ;
            memcpy(&stream_id_n,pkt_buf+sizeof(l_int32) , sizeof(l_int32));
			stream_id = ntohl(stream_id_n) ;
			memcpy(&pkt_id_n, pkt_buf+2*sizeof(l_int32), sizeof(l_int32));
			pkt_id = ntohl(pkt_id_n) ;
			if ( fleet_id == exp_fleet_id  && stream_id == stream_cnt && pkt_id >= exp_pkt_id )
			{
				if ( first_time.tv_sec == 0 )
					first_time = current_time;
			  arrv_tv[pkt_id] = current_time ;
			  memcpy(&(snd_tv[pkt_id].tv_sec) , pkt_buf+3*sizeof(l_int32), sizeof(l_int32));
			  memcpy(&(snd_tv[pkt_id].tv_usec), pkt_buf+4*sizeof(l_int32), sizeof(l_int32));
			  if ( pkt_id > exp_pkt_id ) /* reordered are considered as lost */
			  {
				pkt_lost += ( pkt_id - exp_pkt_id ) ;
				exp_pkt_id  = pkt_id ;
			  }
			  ++exp_pkt_id ;
			  ++pkt_rcvd;
			}
		  } // end of recvfrom
        } // end of FD_ISSET

        if ( FD_ISSET(sock_tcp,&readset) )
        {
          /* check the control connection.*/
          if (( ret_val = recv_ctr_mesg(ctr_buff)) != -1 )
          {
            if ( (((ret_val & CTR_CODE) >> 31) == 1) && 
                  ((ret_val & 0x7fffffff) == FINISHED_STREAM ) )
            {
				printf("#");
				fflush(stdout);
              while((ret_val = recv_ctr_mesg(ctr_buff ))== -1) ;
              if ( ret_val == stream_cnt )
              {
				do {
					fd_set readset1;
					FD_ZERO(&readset1);
					FD_SET(sock_udp,&readset1);
					select_tv.tv_sec=0;select_tv.tv_usec=1000;
					if (select(sock_udp+1,&readset1,NULL,NULL,&select_tv) > 0 ) 
					{
						if (FD_ISSET(sock_udp,&readset1) )
						{
				//			TRACE("FLEET : PENDING UDP RECV\n");
							ret = recvfrom(sock_udp,pkt_buf,cur_pkt_sz,0,NULL,NULL);
//							TRACE("\nsize = %d ret %d\n",cur_pkt_sz, ret);
							if(ret < 0)
							{
								printf("\nClient Disconnected\n");
								free(pkt_buf);
								terminate_gracefully(exp_start_time,1);
								return DISCONNECTED;
							}
							else if(ret == 0)
							{
				//				TRACE("ret = 0");
								fflush(stdout);
								free(pkt_buf);
								terminate_gracefully(exp_start_time,1);
								return DISCONNECTED;
							}
							else// if(ret > 0)
							{
								gettimeofday(&current_time,NULL);
								memcpy(&fleet_id_n,pkt_buf , sizeof(l_int32));
								fleet_id = ntohl(fleet_id_n) ;
								memcpy(&stream_id_n,pkt_buf+sizeof(l_int32) , sizeof(l_int32));
								stream_id = ntohl(stream_id_n) ;
								memcpy(&pkt_id_n, pkt_buf+2*sizeof(l_int32), sizeof(l_int32));
								pkt_id = ntohl(pkt_id_n) ;
								if ( fleet_id == exp_fleet_id  && stream_id == stream_cnt && pkt_id >= exp_pkt_id )
								{
									if ( first_time.tv_sec == 0 )
										first_time = current_time;
									arrv_tv[pkt_id] = current_time ;
									memcpy(&(snd_tv[pkt_id].tv_sec) , pkt_buf+3*sizeof(l_int32), sizeof(l_int32));
									memcpy(&(snd_tv[pkt_id].tv_usec), pkt_buf+4*sizeof(l_int32), sizeof(l_int32));
									if ( pkt_id > exp_pkt_id ) /* reordered are considered as lost */
									{
										pkt_lost += ( pkt_id - exp_pkt_id ) ;
										exp_pkt_id  = pkt_id ;
									}
									++exp_pkt_id ;
									++pkt_rcvd;
								}
							} // end of recvfrom
						}
					}
					else
					{
						break;
					}
				} while(1);

                break ; 
              }
            }
          }
        }
      } // end of select
    } 

    for (j=0; j < stream_len; j++ )
    {
      snd_tv[j].tv_sec = ntohl(snd_tv[j].tv_sec);
      snd_tv[j].tv_usec = ntohl(snd_tv[j].tv_usec);
      snd_tm[j]= snd_tv[j].tv_sec * 1000000.0 + snd_tv[j].tv_usec ;
      arrv_tm[j] = arrv_tv[j].tv_sec * 1000000.0 + arrv_tv[j].tv_usec ;
      owd[j] =  arrv_tm[j] - snd_tm[j]   ;
    }

    total_pkt_rcvd += pkt_rcvd ;
    finished_stream = 0 ;
    pkt_lost +=  stream_len  - exp_pkt_id ;
    pkt_loss_rate = (double )pkt_lost * 100. / stream_len ;
    if(Verbose)
      printf(":%.1f",pkt_loss_rate ) ;
//    fprintf(pathload_fp,":%.1f",pkt_loss_rate ) ;
    exp_pkt_id = 0 ;
    stream_cnt++ ;

    num_bursts=0;
    if ( interrupt_coalescence )
      ic_flag=check_intr_coalescence(arrv_tv,pkt_rcvd,&num_bursts); 
    
    if ( pkt_loss_rate < HIGH_LOSS_RATE && pkt_loss_rate >= MEDIUM_LOSS_RATE )
      lossy_stream++ ;
     
    if ( pkt_loss_rate >= HIGH_LOSS_RATE || ( stream_cnt >= num_stream
         && lossy_stream*100./stream_cnt >= MAX_LOSSY_STREAM_FRACTION ))
    {
      if ( increase_stream_len )
      {
        increase_stream_len=0;
        lower_bound=1;
      }

      if(Verbose)
        printf("\n  Fleet aborted due to high lossrate");
//      fprintf(pathload_fp,"\n  Fleet aborted due to high lossrate");
      abort_fleet=1;
      break ;
    }
    else
    {
      /* analyze trend in stream */
      num += get_sndr_time_interval(snd_tm,&snd_time_interval) ;
      adjust_offset_to_zero(owd, stream_len);  
      num_substream = eliminate_sndr_side_CS(snd_tm,substream);
      num_sndr_cs[stream_cnt-1] = num_substream ;
      substream[num_substream++]=stream_len-1;
      low=0;
      num_rcvr_cs[stream_cnt-1]=0;
      tmp_b2b=0;
      for (j=0;j<num_substream;j++)
      {
        high=substream[j]; 
        if ( ic_flag )
        {
          if ( num_bursts < 2 )
          {
            if ( ++repeat_1 == 3)
            {
              repeat_1=0;
              /* Abort fleet and try to find lower bound */
              abort_fleet=1;
              lower_bound=1;
              increase_stream_len=0;
              break ;
            }
          }
          else if ( num_bursts <= 5 )
          {
            if ( ++repeat_2 == 3)
            {
              repeat_2=0;
              /* Abort fleet and retry with longer stream length */
              abort_fleet=1;
              increase_stream_len=1;
              break ;
            }
          }
          else
          {
            increase_stream_len=0;
            len=eliminate_b2b_pkt_ic(arrv_tm,owd,owdfortd,low,high,&num_rcvr_cs[stream_cnt-1],&tmp_b2b);
            /*
            for(p=0;p<len;p++)
              TRACE("%d %f\n",p,owdfortd[p]);
            */
            pct_metric[trend_idx]=
              pairwise_comparision_test(owdfortd , 0 , len );
            pdt_metric[trend_idx]=
              pairwise_diff_test(owdfortd , 0, len );
            trend_idx+=1;
          }
        }
        else
        {
          len=eliminate_rcvr_side_CS(arrv_tm,owd,owdfortd,low,high,&num_rcvr_cs[stream_cnt-1],&tmp_b2b);
//printf("len: %d\n", len);
          if ( len > MIN_STREAM_LEN )
          {
            get_trend(owdfortd,len);
          }
        }
        low=high+1;
      }
      if ( abort_fleet )
        break;
      else
      {
        b2b_pkt_per_stream[stream_cnt-1] = tmp_b2b ;
        ctr_code = CONTINUE_STREAM | CTR_CODE;
        send_ctr_mesg(ctr_buff, ctr_code);
	send_owd(fleet_id, owd,stream_len);
      }
    }
    pkt_rcvd = 0 ;

    /* A hack for slow links */
    stream_duration = stream_len * time_interval ;
    if ( stream_duration >= 500000 )
    {
      slow=1;
      break ;
    }
  }  /*end of while (stream_cnt < num_stream ). */

  if ( abort_fleet )
  {
//    TRACE("\tAborting fleet. Stream_cnt %d\n",stream_cnt);
     ctr_code = ABORT_FLEET | CTR_CODE;
     send_ctr_mesg(ctr_buff , ctr_code ) ;
     return_val = -1 ;
  }
  else
     print_contextswitch_info(num_sndr_cs,num_rcvr_cs,b2b_pkt_per_stream,stream_cnt);

  exp_fleet_id++ ;
  free(pkt_buf);
  return return_val  ;
}


l_int32 get_sndr_time_interval(double snd_time[],double *sum)
{
  l_int32 k,j=0,new_j=0;
  double ordered[MAX_STREAM_LEN] ;
  double ltime_interval[MAX_STREAM_LEN] ;
  for ( k = 0; k < stream_len-1; k++ )
  {
    if ( snd_time[k] == 0 || snd_time[k+1] == 0 )
      continue;
    else 
      ltime_interval[j++] = snd_time[k+1] - snd_time[k] ;
  }
  order_dbl(ltime_interval, ordered , 0, j ) ;
  /* discard the top 15% as outliers  */
  new_j = j - rint(j*.15) ;
  for ( k = 0 ; k < new_j ; k++ )
    *sum += ordered[k] ;
  return new_j ;
}

/* Adjust offset to zero again  */
void adjust_offset_to_zero(double owd[], l_int32 len)
{
    double owd_min = 0;
    l_int32 i ; 
    for (i=0; i< len; i++) {
        if ( owd_min == 0 && owd[i] != 0 ) owd_min=owd[i];
        else if (owd_min != 0 && owd[i] != 0 && owd[i]<owd_min) owd_min=owd[i];
    }

    for (i=0; i< len; i++) {
        if ( owd[i] != 0 )
            owd[i] -= owd_min;
    }
}

/*
  splits stream iff sender sent packets more than
  time_interval+1000 usec apart.
*/
l_int32 eliminate_sndr_side_CS (double sndr_time_stamp[], l_int32 split_owd[])
{
  l_int32 j = 0,k=0;
  l_int32 cs_threshold;

  cs_threshold = 2*time_interval>time_interval+1000?2*time_interval:time_interval+1000;
  for ( k = 0 ; k < stream_len-1 ; k++ )
  {
    if ( sndr_time_stamp[k] == 0 || sndr_time_stamp[k+1] == 0 )
       continue;
    else if ((sndr_time_stamp[k+1]-sndr_time_stamp[k]) > cs_threshold)
      split_owd[j++] = k;
  }
  return j ;
}

/* eliminates packets received b2b due to IC */
l_int32 eliminate_b2b_pkt_ic ( double rcvr_time_stamp[] , double owd[],double owdfortd[], l_int32 low,l_int32 high,l_int32 *num_rcvr_cs,l_int32 *tmp_b2b )
{
  l_int32 b2b_pkt[MAX_STREAM_LEN] ;
  l_int32 i,k=0 ;
  l_int32 len=0;
  l_int32 min_gap;
  l_int32 tmp=0;

  min_gap = MIN_TIME_INTERVAL > 3*rcv_latency ? MIN_TIME_INTERVAL :3*rcv_latency ;
  for ( i = low ; i <= high  ; i++ )
  {
    if ( rcvr_time_stamp[i] == 0 || rcvr_time_stamp[i+1] == 0 )
      continue ;
    
    //fprintf(stderr,"i %d  owd %.2f dispersion %.2f",i, owd[i],rcvr_time_stamp[i+1]- rcvr_time_stamp[i]);
    if ((rcvr_time_stamp[i+1]- rcvr_time_stamp[i])< min_gap)
    {
      b2b_pkt[k++] = i ;
      tmp++;
      //fprintf(stderr," b\n");
    }
    else 
    {
      if ( tmp >= 3 )
      {
        //fprintf(stderr," j\n");
        tmp=0;
        owdfortd[len++] = owd[i];
      }
    }
  }
//  printf("\nLENGTH: %d\n", len);
  return len ;
}

/*
    PCT test to detect increasing trend in stream
*/
double pairwise_comparision_test (double array[] ,l_int32 start , l_int32 end)
{
  l_int32 improvement = 0 ,i ;
  double total ;

  if ( ( end - start  ) >= MIN_PARTITIONED_STREAM_LEN )
  {
    for ( i = start ; i < end - 1   ; i++ )
    {
      if ( array[i] < array[i+1] )
        improvement += 1 ;
    }
    total = ( end - start ) ;
    return ( (double)improvement/total ) ;
  }
  else
    return -1 ;
}

/*
    PDT test to detect increasing trend in stream
*/
double pairwise_diff_test(double array[] ,l_int32 start , l_int32 end)
{
  double y = 0 , y_abs = 0 ;
  l_int32 i ;
  if ( ( end - start  ) >= MIN_PARTITIONED_STREAM_LEN )
  {
    for ( i = start+1 ; i < end    ; i++ )
    {
      y += array[i] - array[i-1] ;
      y_abs += fabs(array[i] - array[i-1]) ;
    }
    return y/y_abs ;
  }
  else
    return 2. ;
}

/*
  discards owd of packets received when
  receiver was NOT running.
*/
l_int32 eliminate_rcvr_side_CS ( double rcvr_time_stamp[] , double owd[],double owdfortd[], l_int32 low,l_int32 high,l_int32 *num_rcvr_cs,l_int32 *tmp_b2b )
{
  l_int32 b2b_pkt[MAX_STREAM_LEN] ;
  l_int32 i,k=0 ;
  l_int32 len=0;
  double min_gap;

  min_gap = MIN_TIME_INTERVAL > 1.5*rcv_latency ? MIN_TIME_INTERVAL :2.5*rcv_latency ;
  for ( i = low ; i <= high  ; i++ )
  {
    if ( rcvr_time_stamp[i] == 0 || rcvr_time_stamp[i+1] == 0 )
      continue ;
    else if ((rcvr_time_stamp[i+1]- rcvr_time_stamp[i])> min_gap)
      owdfortd[len++] = owd[i];
    else 
      b2b_pkt[k++] = i ;
  }

  /* go through discarded list and count b2b discards as 1 CS instance */
  for (i=1;i<k;i++)
    if ( b2b_pkt[i]-b2b_pkt[i-1] != 1)
      (*num_rcvr_cs)++;
  *tmp_b2b += k;
  return len ;
}

void get_trend(double owdfortd[],l_int32 pkt_cnt )
{
  double median_owd[MAX_STREAM_LEN];
  l_int32 median_owd_len=0;
  double ordered[MAX_STREAM_LEN];
  l_int32 j,count,pkt_per_min;
  //pkt_per_min = 5 ; 
  pkt_per_min = (int)floor(sqrt((double)pkt_cnt));
  count = 0 ;
  for ( j = 0 ; j < pkt_cnt  ; j=j+pkt_per_min )
  {
    if ( j+pkt_per_min >= pkt_cnt )
      count = pkt_cnt - j ;
    else
      count = pkt_per_min;
    order_dbl(owdfortd , ordered ,j,count ) ;
    if ( count % 2 == 0 )
       median_owd[median_owd_len++] =  
          ( ordered[(int)(count*.5) -1] + ordered[(int)(count*0.5)] )/2 ;
    else
       median_owd[median_owd_len++] =  ordered[(int)(count*0.5)] ;
  }
  pct_metric[trend_idx]=
      pairwise_comparision_test(median_owd , 0 , median_owd_len );
  pdt_metric[trend_idx]=
      pairwise_diff_test(median_owd , 0, median_owd_len );
  trend_idx+=1;
}

/* 
  Order an array of doubles using bubblesort 
*/
void order_dbl(double unord_arr[], double ord_arr[],l_int32 start, l_int32 num_elems)
{
  l_int32 i,j,k;
  double temp;
  for (i=start,k=0;i<start+num_elems;i++,k++) ord_arr[k]=unord_arr[i];
  
  for (i=1;i<num_elems;i++) 
  {
    for (j=i-1;j>=0;j--)
      if (ord_arr[j+1] < ord_arr[j]) 
      {
        temp=ord_arr[j]; 
        ord_arr[j]=ord_arr[j+1]; 
        ord_arr[j+1]=temp;
      }
      else break;
  }
}

void print_contextswitch_info(l_int32 num_sndr_cs[], l_int32 num_rcvr_cs[],l_int32 discard[],l_int32 stream_cnt)
{
    l_int32 j;

    if (Verbose)
      printf("\n  # of CS @ sndr        :: ");
//    fprintf(pathload_fp,"  # of CS @ sndr        :: ");
    
    for(j=0;j<stream_cnt-1;j++)
    {
      if (Verbose) printf(":%2d",num_sndr_cs[j]);
//      fprintf(pathload_fp,":%2d",num_sndr_cs[j]);
    }
    if ( Verbose ) printf("\n");
//    fprintf(pathload_fp,"\n");
    if (Verbose)
      printf("  # of CS @ rcvr        :: ");
//    fprintf(pathload_fp,"  # of CS @ rcvr        :: ");
    for(j=0;j<stream_cnt-1;j++)
    {
      if (Verbose) printf(":%2d",num_rcvr_cs[j]);
//      fprintf(pathload_fp,":%2d",num_rcvr_cs[j]);
    }
    if ( Verbose ) printf("\n");
//    fprintf(pathload_fp,"\n");
 
    if (Verbose)
      printf("  # of DS @ rcvr        :: ");
//    fprintf(pathload_fp,"  # of DS @ rcvr        :: ");
    for(j=0;j<stream_cnt-1;j++)
    {
      if (Verbose) printf(":%2d",discard[j]);
//      fprintf(pathload_fp,":%2d",discard[j]);
    }
     if ( Verbose ) printf("\n");
//    fprintf(pathload_fp,"\n");
}

/*
 * Send a message through the control stream
 * */
l_int32 send_owd(l_int32 fleet_id, double owd[],l_int32 stream_len)
{
	struct owdData data;
	char ctr_buff[8];
	int i,size;
	data.fleetid = htonl(fleet_id);
	for(i=0;i<stream_len;i++) {
		data.owd[i]=htonl((unsigned int)owd[i]);
	}
	size =  sizeof(data.fleetid) + (stream_len * sizeof(unsigned int));
	send_ctr_mesg(ctr_buff,size);
	if (send(sock_tcp, (const char*)&data, size, 0) != size)
	{
		printf("\nOWD SND Error");
		printf("\nConnection to the server lost");
		return -1 ; 
	}
	else 
	{
		return 0;
	}
}


