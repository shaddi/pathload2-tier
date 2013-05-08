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

//#define LOCAL
//#define _WIN32_WINNT 0x0500
//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif
//#include "stdafx.h"
#include "global.h"
#include "client.h"
//#include "../gui/resource.h"
//#include "../gui/guidlg.h"

//#pragma warning(disable:4996)

/*int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	LARGE_INTEGER t;
	QueryPerformanceCounter((LARGE_INTEGER *)&t);

	tv->tv_sec = (long)floor(1.0*t.QuadPart / freq.QuadPart);
	tv->tv_usec = (long)((1.0*t.QuadPart/freq.QuadPart - tv->tv_sec)*1.0e6);

	return TRUE;
}*/


l_int32 selectServer()
{
	char *selectorList[NUM_SELECT_SERVERS] = {"127.0.0.1"};
	int visited[NUM_SELECT_SERVERS];
	int num,i;
	l_int32 ctr_code;
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	char ctr_buff[8];
	struct hostent *he;
	struct sockaddr_in their_addr; // connector’s address information
	for(i = 0;i<NUM_SELECT_SERVERS;i++)
	{
		visited[i]= 0;
	}
	while(1)
	{
		int flag = 0;
		int i=0;
		for(i = 0;i<NUM_SELECT_SERVERS;i++)
		{
			if(visited[i]==0)
			{
				flag = 1;
				break;
			}
		}
		if(!flag) {
			printf("\nAll servers are busy. \nPlease try again later.");
		//	guidlg->m_cTextout.SetWindowTextW(L"Could not connect to selected M-lab server. \r\nPlease try again later.");
			return -1;
		}
		num = rand()%NUM_SELECT_SERVERS;
		if(visited[num] == 1)
			continue;
		visited[num] = 1;
		strcpy(hostname,selectorList[num]);
		if ((he=gethostbyname(hostname)) == NULL) { // get the host info
			perror("gethostbyname");
			continue;
		}
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("socket");
			continue;
		}
		their_addr.sin_family = AF_INET; // host byte order
		their_addr.sin_port = htons(SELECTPORT); // short, network byte order
		their_addr.sin_addr = *((struct in_addr *)he->h_addr);
		bzero((char *)&(their_addr.sin_zero),sizeof(their_addr.sin_zero)); // zero the rest of the struct
		if (connect(sockfd,(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
			perror("connect");
			printf("\nConnect Failed");
			continue;
			//exit(1);
		}
		if ((numbytes=recv(sockfd, ctr_buff, sizeof(l_int32), 0)) == -1) {
				perror("recv");
				close(sockfd);
				exit (0);
		}
		memcpy(&ctr_code, ctr_buff, sizeof(l_int32));
		num_servers = ntohl(ctr_code);
		printf("\nNo of servers available = %d",num_servers);
		for(i=0;i<num_servers;i++)
		{
			bzero(buf,MAXDATASIZE);
			if ((numbytes=recv(sockfd, ctr_buff, sizeof(l_int32), 0)) == -1) {
				close(sockfd);
				exit (0);
			}
			memcpy(&ctr_code, ctr_buff, sizeof(l_int32));
			int size = ntohl(ctr_code);
			if ((numbytes=recv(sockfd, buf, size, 0)) == -1) {
				printf("recv err ");
				close(sockfd);
				exit (0);
			}
			buf[size] = '\0';
			strcpy(serverList[i],buf);
			//serverList[i][size] = '\0';
		}
		close(sockfd);

		break;
	}//end while(1)
	return 0;
}

int main(l_int32 argc, char* argv[])
{
	struct timeval tv1,tv2,exp_end_time;
	l_uint32 rcv_latency ;
	l_int32 ctr_code,ret,c ;
	time_t localtm;
	int mss;
	int ret_val ;
	int iterate=0;
	int done=0;
	int latency[30],ord_latency[30];
	int i;
	char ctr_buff[8];
	// declarations for reverse measurements
	l_int32 trend, prev_trend = 0;
	Verbose = 0;
	localtm = time(NULL);
	//guidlg = (CGuiDlg *)t;
	slow=0;
	interrupt_coalescence=0;
	bad_fleet_cs=0;
	num_stream = NUM_STREAM;
	stream_len = STREAM_LEN ;
	exp_flag = 1;
	num=0;
	snd_time_interval=0;
	converged_gmx_rmx = 0 ;
	converged_gmn_rmn = 0 ;
	converged_rmn_rmx = 0 ;
	counter = 0 ;
	prev_actual_rate = 0;
	prev_req_rate = 0 ;
	cur_actual_rate = 0 ;
	cur_req_rate = 0 ;
	gettimeofday(&exp_start_time, NULL);
	verbose=0;
	bw_resol=1;
	netlog=0;
	increase_stream_len=0;
	lower_bound=0;
	bzero(result,512);
	bzero(result1,512);
	quiet=0;

	c=getopt(argc,argv,"vV");
	if(c=='v'|| c=='V')
	{
		verbose = 1;
		Verbose = 1;
	}
	//struct sigaction sigstruct;
//	WSAStartup(MAKEWORD(2,2),&wsa);
//	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	srand(exp_start_time.tv_usec);

/*	 Create a waitable timer.
    hTimer = CreateWaitableTimer(NULL, TRUE, L"WaitableTimer");
    if (NULL == hTimer)
    {
        printf("CreateWaitableTimer failed (%d)\n", GetLastError());
        return 1;

*/
	num_stream = NUM_STREAM ;
	min_sleeptime();

	/* gettimeofday latency */
	for(i=0;i<30;i++)
	{
		gettimeofday(&tv1,NULL);
		gettimeofday(&tv2,NULL);
		latency[i]=tv2.tv_sec*1000000+tv2.tv_usec-tv1.tv_sec*1000000-tv1.tv_usec;
	}
	order_int(latency,ord_latency,30);
	gettimeofday_latency = ord_latency[15];

	iterate = 1;
//	guidlg->m_cTextout.SetWindowTextW(L"Connecting to selected M-lab server.");
	printf("Connecting to selected M-lab server.");
	if(selectServer() == -1)
		return -1;
	char *visited = (char*)malloc(num_servers*sizeof(char));
	bzero(visited,num_servers);

	while(1)
	{
		int flag = 0;
		for(i = 0;i<num_servers;i++)
		{
			if(visited[i]==0)
			{
				flag = 1;
				break;
			}
		}
		if(!flag) {
			printf("\nAll servers are busy, yo.\nPlease try again later.\n");
	//		guidlg->m_cTextout.SetWindowTextW(L"Could not connect to server");
			free(visited);
			return -1;
		}
		num = rand()%num_servers;
		if(visited[num] == 1)
			continue;
		visited[num] = 1;
		strcpy(hostname,serverList[num]);
		printf("\nTrying to connect %s\n",hostname);
		if(client_TCP_connection()==-1)
			continue;
		break;
	}
	free(visited);
	if(i==num_servers)
	{
		printf("\nCould not connect to MLab server\nPlease try again later.");
	//	guidlg->m_cTextout.SetWindowTextW(L"Could not connect to MLab server\r\nPlease try again later.");
		return -1;
	}

//	strcpy(hostname,"");
//	client_TCP_connection();
//	printf("\nServer Available %s",hostname);
	send_ctr_mesg(ctr_buff, NEW);
	no_udp_sock = 0;
	if(client_UDP_connection() == -1)
		return -1;
//	CString str;
	printf("\nConnected to MLab Server: %s ",hostname);
//	str.Append((CString)hostname);
//	guidlg->m_cTextout.SetWindowTextW(str);
//
//	CGuiDlg *ct = (CGuiDlg *)t;
//	ct->m_cList.AddString(str);
//	ct->m_cList.PostMessageW(WM_VSCROLL, SB_BOTTOM, 0);
	printf("\nMeasuring Upstream Available Bandwidth\n");
//	ct->m_cList.AddString(L"Measuring Upstream Available Bandwidth");
//	ct->m_cList.PostMessageW(WM_VSCROLL, SB_BOTTOM, 0);
	fflush(stdout);
	do{

		mss = 0;
		snd_max_pkt_sz = mss;
		if (snd_max_pkt_sz == 0 || snd_max_pkt_sz== 1448)
			snd_max_pkt_sz = 1472;   /* Make it Ethernet sized MTU */
		else
			snd_max_pkt_sz = mss+12;
		if(verbose)
			printf("\nClient's max pkt size = %d",snd_max_pkt_sz);
		/* tell receiver our max packet sz */
		while(send_ctr_mesg(ctr_buff, snd_max_pkt_sz) == -1);
		/* receiver's maxp packet size */
		while ((rcv_max_pkt_sz = recv_ctr_mesg( ctr_buff)) == -1);
		if(verbose)
			printf("\nServer's max pkt size = %d",rcv_max_pkt_sz);
		max_pkt_sz = (rcv_max_pkt_sz < snd_max_pkt_sz) ? rcv_max_pkt_sz:snd_max_pkt_sz ;
		if(verbose)
			printf("\nMaximum packet size          :: %d bytes\n",max_pkt_sz);

		/* tell receiver our send latency */
		rcv_latency = (l_int32) send_latency();
//		printf("\nClient latency = %d",rcv_latency);
		send_ctr_mesg(ctr_buff, rcv_latency) ;
		while ((snd_latency = recv_ctr_mesg(ctr_buff)) == -1);
		if (Verbose)
		{
			printf("\n  server latency            :: %d usec\n",snd_latency);
			printf("\n  Client latency            :: %d usec\n",rcv_latency);
		}
//		printf("\nlatency done");

		// wait for receiver to start ADR measurement ****
		if((ret_val=recv_ctr_mesg(ctr_buff)) == -1 )break;
		if ( (((ret_val & CTR_CODE) >> 31) == 1) && ((ret_val & 0x7fffffff) == SEND_TRAIN ) )
		{
			if ( quiet)
				printf("\nEstimating ADR to initialize rate adjustment algorithm => ");
			fflush(stdout);
			if ( send_train() == -1 ) continue ;
			if ( quiet)
				printf("Done\n");
		}

		fleet_id=0;
		done=0;
		exp_fleet_id = 0;
		// Start avail-bw measurement
		while(!done)
		{
			if (( ret_val  = recv_ctr_mesg ( ctr_buff ) ) == -1 ) break;
			if((((ret_val & CTR_CODE) >> 31) == 1) &&((ret_val&0x7fffffff) == TERMINATE))
			{
				if(verbose)
					printf("\nTerminating current run.\n");
				recv_result();
				done=1;
			}
			else
			{
				transmission_rate = ret_val ;
				if ((cur_pkt_sz = recv_ctr_mesg( ctr_buff)) <= 0 )
					break;
				if ((stream_len = recv_ctr_mesg( ctr_buff))  <= 0 )
					break;
				if ((time_interval = recv_ctr_mesg( ctr_buff)) <= 0 )
					break;
				if ((ret_val = recv_ctr_mesg ( ctr_buff )) == -1 )
					break;
				// ret_val = SEND_FLEET
				ctr_code = RECV_FLEET | CTR_CODE ;
				if ( send_ctr_mesg(ctr_buff,  ctr_code  ) == -1 )
					break;
				ret_val = send_fleet();
				if(ret_val == -1)
					break;

				if ( !quiet) printf("\n");
				fleet_id++ ;
			}
		}

//	((CGuiDlg *)t)->m_cProgress.SetPos(50);

/*	Opp direction code here (Lin-Win)	*/
		printf("\nMeasuring Downstream Available Bandwidth");
//		ct->m_cList.AddString(L"Measuring Downstream Available Bandwidth");
//		ct->m_cList.PostMessageW(WM_VSCROLL, SB_BOTTOM, 0);
		gettimeofday(&exp_start_time, NULL);

		min_time_interval=
			SCALE_FACTOR*((rcv_latency>snd_latency)?rcv_latency:snd_latency) ;
		min_time_interval = min_time_interval>MIN_TIME_INTERVAL?min_time_interval:MIN_TIME_INTERVAL;
		if (Verbose)
			printf("\n  Minimum packet spacing       :: %d usec\n",min_time_interval );
		//fprintf(pathload_fp,"  Minimum packet spacing       :: %ld usec\n",min_time_interval );
		max_rate = (max_pkt_sz+28) * 8. / min_time_interval ;
		min_rate = (MIN_PKT_SZ+28) * 8./ MAX_TIME_INTERVAL ;
		if(Verbose)
			printf("  Max rate(max_pktsz/min_time) :: %.2fMbps\n",max_rate);

		// Estimate ADR
		adr = get_adr() ;
		/*if ( bw_resol == 0 && adr != 0 )
			bw_resol = (float)(.02*adr) ;
		else if (bw_resol == 0 )
			bw_resol = 2 ; */
		if(Verbose)
			printf("  Grey bandwidth resolution    :: %.2f\n",grey_bw_resolution());
		//fprintf(pathload_fp,"  Grey bandwidth resolution    :: %.2f\n",grey_bw_resolution());

		if (interrupt_coalescence)
		{
			bw_resol = (float)(.05*adr);
			if(verbose||Verbose)
				printf("\nInterrupt coalescion detected\n\n");
			//fprintf(pathload_fp,"  Interrupt coalescion detected\n");
		}

		if ( adr == 0 || adr > max_rate || adr < min_rate)
			tr = (max_rate+min_rate)/2.;
		else
			tr = adr ;
//		printf("\nReverse ADR Done\n");

		/* Estimate the reverse available bandwidth.*/
		transmission_rate = (l_uint32)rint(1000000 * tr);
		max_rate_flag = 0 ;
		min_rate_flag = 0 ;

		while (1)	//inner while(1)
		{
			if ( calc_param() == -1 )
			{
			  ctr_code = TERMINATE | CTR_CODE;
			  send_ctr_mesg(ctr_buff, ctr_code);
			  terminate_gracefully(exp_start_time,0) ;
			  break;
			}
			gettimeofday(&exp_end_time, NULL);
			if(exp_fleet_id > 10 || (time_to_us_delta(exp_start_time, exp_end_time) / 1000000) > 20.00)
			{
				terminate_gracefully(exp_start_time,0) ;
				break;
			}
			old_tr = tr;
			send_ctr_mesg(ctr_buff, transmission_rate);
			send_ctr_mesg(ctr_buff,cur_pkt_sz) ;
			if ( increase_stream_len )
			  stream_len=3*STREAM_LEN;
			else
			  stream_len = STREAM_LEN;
			send_ctr_mesg(ctr_buff,stream_len);
			send_ctr_mesg(ctr_buff,time_interval);
			ctr_code = SEND_FLEET | CTR_CODE ;
			send_ctr_mesg(ctr_buff, ctr_code);

			while (1)
			{
			  ret_val = recv_ctr_mesg(ctr_buff);
			  if ((((ret_val & CTR_CODE) >> 31) == 1) &&
				   ((ret_val & 0x7fffffff) == RECV_FLEET ))
				break ;
			  else if ( (((ret_val & CTR_CODE) >> 31) == 1) &&
						 ((ret_val & 0x7fffffff) == FINISHED_STREAM ))
				ret_val = recv_ctr_mesg(ctr_buff);
			}

			ret = recv_fleet();
			if(ret == DISCONNECTED)
			{
			//	guidlg->m_cTextout.SetWindowTextW(L"Broken connection to server.\r\nPlease try again later.");
				printf("Broken connection to server.\nPlease try again later.");
				return -1;
			}

			else if (ret == -1)
			{
			  if ( !increase_stream_len )
			  {
				trend = INCREASING;
				if ( exp_flag == 1 && prev_trend != 0 && prev_trend != trend)
				  exp_flag = 0;
				prev_trend = trend;
				if (rate_adjustment(INCREASING) == -1)
				{
					terminate_gracefully(exp_start_time,0);
					break;
				}
			  }
			}
			else
			{
			  get_sending_rate() ;
			  trend = aggregate_trend_result();
			  fflush(stdout);
			  if ( trend == -1 && bad_fleet_cs && retry_fleet_cnt_cs >NUM_RETRY_CS )
			  {
				terminate_gracefully(exp_start_time,0) ;
				break;
			  }
			  else if(( trend == -1 && bad_fleet_cs && retry_fleet_cnt_cs <= NUM_RETRY_CS )) //* repeat fleet with current rate.
				continue ;

			  if (trend != GREY)
			  {
				if (exp_flag == 1 && prev_trend != 0 && prev_trend != trend)
				  exp_flag = 0;
				prev_trend = trend;
			  }

			  if (rate_adjustment(trend) == -1){
				terminate_gracefully(exp_start_time,0);
				break;
			 }
			}
		 //   fflush(pathload_fp);
		}	//end inner while(1)

		fflush(stdout);

	/*	Opp direction code ends here (Lin-Win)	*/
		iterate = 0;
	}while(iterate);
	printf("\nMeasurement completed.");

//	ct->m_cList.AddString(L"Measurement completed.");
//	ct->m_cList.PostMessageW(WM_VSCROLL, SB_BOTTOM, 0);
	printf("\n\n\t*****  RESULT *****\n");
	printf("Upstream Measurement (towards the Internet)\n");
//	guidlg->m_cTextout.SetWindowTextW(L"Measurement completed.");

//	char delims[] = "#";
	char * temp = NULL;
	temp = strtok(result,"#");
	while(temp!=NULL)
	{
		printf("%s\n",temp);
		temp = strtok(NULL,"#");
	}

	printf("\nDownstream Measurement (from the Internet) %s\n",result1);
	printf("\nFor more information about available bandwidth measurement, \nplease see: http://www.pathrate.org\n");
//	ct->m_cList.AddString(L"");
//	ct->m_cList.AddString(L"Upstream Measurement");
//	char * tok;
//	tok = strtok (result,"#");
//	ct->m_cList.AddString(CString(tok));
//	while (tok != NULL)
//	{
//		tok = strtok (NULL, "#");
//		if(tok){
//		printf("%s\n",tok);
//		ct->m_cList.AddString(CString(tok));}
//	}
//	ct->m_cList.AddString(L"");
//	ct->m_cList.PostMessageW(WM_VSCROLL, SB_BOTTOM, 0);
//	ct->m_cList.AddString(L"Downstream Measurement");
//
//	tok = strtok (result1,"#");
//	ct->m_cList.AddString(CString(tok));
//	while (tok != NULL)
//	{
//		tok = strtok (NULL, "#");
//		if(tok){
//		printf("%s\n",tok);
//		ct->m_cList.AddString(CString(tok));}
//
//	}
//	ct->m_cList.AddString(L"");
//	ct->m_cList.PostMessageW(WM_VSCROLL, SB_BOTTOM, 0);
//	((CGuiDlg *)t)->m_cProgress.SetPos(100);

	return 0;
}

