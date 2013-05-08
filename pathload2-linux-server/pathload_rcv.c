/*
This file is part of pathload.

Pathload is free software; you can redistribute it and/or modify
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

/*-------------------------------------------------
	pathload : an end-to-end available bandwidth
			   estimation tool
	Author   : Nachiket Deo          (nachiket.deo@gatech.edu)
               Partha Kanuparthy     (partha@cc.gatech.edu)
               Manish Jain           (jain@cc.gatech.edu)
               Constantinos Dovrolis (dovrolis@cc.gatech.edu )
    Release  : Ver 2.0
--------------------------------------------------*/

#define LOCAL
#include "pathload_gbls.h"
#include "pathload_rcv.h"
#include <stdlib.h>

int main(int argc, char *argv[])
{
	rcv_func(0);
	return 0;
}

//int main(l_int32 argc, char *argv[])
void rcv_func(int flag)
{
//	#ifndef	DECL
		struct utsname uts ;
		struct timeval tv1,tv2;
		l_int32 latency[30],ord_latency[30];
		l_int32 ctr_code;
		l_int32 trend, prev_trend = 0;
		l_int32 opt_len, mss,i;
		l_int32 ret_val ;
		l_int32 errflg=0;
		char ctr_buff[8], myname[50], buff[26];
		l_int32 ret=0;
		int done=0, terminated = 0;
		struct sigaction sigstruct ;
//		#define DECL
//	#endif

  while(1){	//outer while(1)
	slow=0;
	requested_delay = 0;
	interrupt_coalescence=0;
	bad_fleet_cs=0;
	num_stream = NUM_STREAM;
	stream_len = STREAM_LEN ;
	exp_flag = 1;
	num=0;
	snd_time_interval=0;
	counter = 0 ;
	converged_gmx_rmx = 0 ;
	converged_gmx_rmx = 0 ;
	converged_gmn_rmn = 0 ;
	converged_rmn_rmx_tm = 0 ;
	converged_gmn_rmn_tm = 0 ;
	converged_rmn_rmx_tm = 0 ;
	prev_actual_rate = 0;
	prev_req_rate = 0 ;
	cur_actual_rate = 0 ;
	cur_req_rate = 0 ;
	verbose=1;
	Verbose = 1;
	bw_resol=1;
	netlog=0;
	increase_stream_len=0;
	lower_bound=0;
	exp_fleet_id = 0;
	tr_max = 0.0;
	tr_min = 0.0;
	grey_max = 0;
	grey_min = 0;
	tr = 0;
	adr = 0;
	grey_flag = 0;
	retry_fleet_cnt_cs = 0;
	quiet = 1;
	prev_trend = 0;
	errflg=0;
	done = 0;
	terminated = 0;
	cleanedup = 0;
	bzero(result,512);
	bzero(result1,512);
	bzero(filename,128);
	char dt[32];
  /*
    measure max_pkt_sz (based on TCP MSS).
    this is not accurate because it does not take into
    account MTU of intermediate routers.
  */
  	strcpy(filename,"pathload2/");

	struct tm *now = NULL;
	time_t tval = 0;
	tval = time(NULL);
	now = localtime(&tval);

	sprintf(dt,"%d/",1900+now->tm_year);
	strcat(filename,dt);
	sprintf(dt,"%d/",now->tm_mon+1);
	strcat(filename,dt);
	sprintf(dt,"%d/",now->tm_mday);
	strcat(filename,dt);

	bzero(myname,50);
  	if ( gethostname(myname ,50 ) == -1 )
  	{
    		if ( uname(&uts) < 0 )
      			strcpy(myname , "UNKNOWN") ;
    		else
      			strcpy(myname , uts.nodename) ;
  	}
	strcat(filename,myname);
//	strcat(filename,"/");
//	strcpy(filename,(mkdir(filename,0777) == 0)
//		perror("mkdir");
	strcpy(myname,"mkdir -p ");
	strcat(myname,filename);
	system(myname);
//	exit(0);
	ret = init_TCP_connection();
	if(ret == -1)
	{
		continue;
	}

	ret = init_UDP_connection();
	if(ret == -1)
	{
		close(sock_tcp);
		continue;
	}

	version = 0;
	version= recv_ctr_mesg(sock_tcp, ctr_buff);
	if(version <= 0 || (version != OLD && version != NEW))
	{
		printf("\nAuthentication Failed.");
		close(sock_tcp);
		close(sock_udp);
		continue;
	}
	gettimeofday(&exp_start_time, NULL);
	sprintf(buff,"%ld%ld",exp_start_time.tv_sec,exp_start_time.tv_usec);
	strcat(filename,"_");
	char timestamp[64] = { 0 };
	strftime(timestamp, 32,"%Y%m%dT%T%Z", now);
	strcat(filename,timestamp);

	strcat(filename,".log");
	strncpy(buff, ctime(&(exp_start_time.tv_sec)), 24);
  	buff[24] = '\0';

	if((pathload_fp = fopen(filename,"a")) == NULL)
	{
		printf("\nProblem with opening File %d",errno);
		terminate_gracefully(exp_start_time,1);
	}
 	bzero(myname,50);
  	if ( gethostname(myname ,50 ) == -1 )
  	{
    		if ( uname(&uts) < 0 )
      			strcpy(myname , "UNKNOWN") ;
    		else
      			strcpy(myname , uts.nodename) ;
  	}
 	if (verbose || Verbose)
	{
   		printf("\n\nReceiver %s starts measurements at sender %s on %s \n", myname , hostname, buff);
		fflush(stdout);
		fprintf(pathload_fp,"\n\nReceiver %s starts measurements at sender %s on %s \n", myname , hostname, buff);
	}
	opt_len = sizeof(mss);
	if (getsockopt(sock_tcp, IPPROTO_TCP, TCP_MAXSEG, (char*)&mss,(socklen_t *) &opt_len)<0)
	{
	 perror("getsockopt(sock_tcp,IPPROTO_TCP,TCP_MAXSEG):");
	 fprintf(pathload_fp,"getsockopt(sock_tcp,IPPROTO_TCP,TCP_MAXSEG):");
	}

	rcv_max_pkt_sz = mss;
	if (rcv_max_pkt_sz == 0 || rcv_max_pkt_sz == 1448 )
		rcv_max_pkt_sz = 1472;   //* Make it Ethernet sized MTU *
	else
		rcv_max_pkt_sz = mss+12;

	//* receive sender max_pkt_sz *
	while(1)
	{
			snd_max_pkt_sz = recv_ctr_mesg(sock_tcp,ctr_buff );
			if(snd_max_pkt_sz == DISCONNECTED)
				break;
			else if(snd_max_pkt_sz == -1)
				continue;
			else
				break;
	}

	if ( snd_max_pkt_sz == -2 || snd_max_pkt_sz == DISCONNECTED)
	{
		printf("\nClient did not respond for 60 sec\n");
		fprintf(pathload_fp,"\nClient did not respond for 60 sec\n");
		fclose(pathload_fp);
		close(sock_tcp);
		close(sock_udp);
		continue;
	}
	if(Verbose) printf("\nClients max packet size = %ld\n",snd_max_pkt_sz);
	fprintf(pathload_fp,"\nClients max packet size = %ld\n",snd_max_pkt_sz);


	send_ctr_mesg(ctr_buff, rcv_max_pkt_sz);
	max_pkt_sz = (rcv_max_pkt_sz < snd_max_pkt_sz) ? rcv_max_pkt_sz:snd_max_pkt_sz ;

	if (Verbose)
		printf("  Maximum packet size          :: %ld bytes\n",max_pkt_sz);
	fprintf(pathload_fp,"  Maximum packet size          :: %ld bytes\n",max_pkt_sz);

	rcv_latency = (l_int32) recvfrom_latency(server_udp_addr);//rcv_udp_addr
	snd_latency = recv_ctr_mesg(sock_tcp, ctr_buff);
	if(snd_latency == DISCONNECTED)
	{
		close(sock_tcp);
		close(sock_udp);
 		fclose(pathload_fp);
		continue;
	}

	send_ctr_mesg(ctr_buff, rcv_latency) ;
	if (Verbose)
	{
		printf("  client latency            :: %ld usec\n",snd_latency);
		printf("  server latency            :: %ld usec\n",rcv_latency);
	}
  	fprintf(pathload_fp,"  send latency @sndr           :: %ld usec\n",snd_latency);
 	fprintf(pathload_fp,"  recv latency @rcvr           :: %ld usec\n",rcv_latency);
    min_time_interval=
      SCALE_FACTOR*((rcv_latency>snd_latency)?rcv_latency:snd_latency) ;
	min_time_interval = min_time_interval>MIN_TIME_INTERVAL?
      min_time_interval:MIN_TIME_INTERVAL;
	if (Verbose)
		printf("  Minimum packet spacing       :: %ld usec\n",min_time_interval );
  	fprintf(pathload_fp,"  Minimum packet spacing       :: %ld usec\n",min_time_interval );
	max_rate = (max_pkt_sz+28) * 8. / min_time_interval ;
	min_rate = (MIN_PKT_SZ+28) * 8./ MAX_TIME_INTERVAL ;
	if(Verbose)
		printf("  Max rate(max_pktsz/min_time) :: %.2fMbps\n",max_rate);
	fprintf(pathload_fp,"  Max rate(max_pktsz/min_time) :: %.2fMbps\n",max_rate);

	// Estimate ADR
	adr = get_adr() ;
	if ( bw_resol == 0 && adr != 0 )
		bw_resol = .02*adr ;
	else if (bw_resol == 0 )
		bw_resol = 2 ;
	if(Verbose)
		printf("  Grey bandwidth resolution    :: %.2f\n",grey_bw_resolution());
	fprintf(pathload_fp,"  Grey bandwidth resolution    :: %.2f\n",grey_bw_resolution());

	if (interrupt_coalescence)
	{
		bw_resol = .05*adr;
		if(verbose||Verbose)
		  printf("  Interrupt coalescion detected\n");
		fprintf(pathload_fp,"  Interrupt coalescion detected\n");
//		sprintf(result,"%s","Interrupt Coalescion Detected#");
	}
	if ( adr == 0 || adr > max_rate || adr < min_rate)
		tr = (max_rate+min_rate)/2.;
	else
		tr = adr ;

	//* eSTIMate the available bandwidth.
	transmission_rate = (l_uint32)rint(1000000 * tr);
	max_rate_flag = 0 ;
	min_rate_flag = 0 ;
	fflush(pathload_fp);

	sigemptyset(&sigstruct.sa_mask);
	sigstruct.sa_handler = sig_alrm ;
	sigstruct.sa_flags = 0 ;
	#ifdef SA_INTERRUPT
	sigstruct.sa_flags |= SA_INTERRUPT ;
	#endif
	sigaction(SIGALRM , &sigstruct,NULL );


  while (1)	//inner while(1)
  {
    if ( calc_param() == -1 )
    {
      terminated = terminate_gracefully(exp_start_time,0) ;
	  break;
    }
	gettimeofday(&exp_end_time, NULL);
	if(exp_fleet_id > 10 ||(time_to_us_delta(exp_start_time, exp_end_time) / 1000000) > 20.00)
	{
		terminate_gracefully(exp_start_time,0);
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
      ret_val = recv_ctr_mesg(sock_tcp, ctr_buff);
      if ((((ret_val & CTR_CODE) >> 31) == 1) &&
           ((ret_val & 0x7fffffff) == RECV_FLEET ))
        break ;
      else if ( (((ret_val & CTR_CODE) >> 31) == 1) &&
                 ((ret_val & 0x7fffffff) == FINISHED_STREAM ))
        ret_val = recv_ctr_mesg(sock_tcp, ctr_buff);
	  if(ret_val == DISCONNECTED)
	  break;
    }
	if(ret_val == DISCONNECTED)
	break;

	ret = recv_fleet();
	if(ret == DISCONNECTED)
		break;
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
            terminated = terminate_gracefully(exp_start_time,0);
	        break;
	    }
      }
    }
    else
    {
      get_sending_rate() ;
      trend = aggregate_trend_result();

      if ( trend == -1 && bad_fleet_cs && retry_fleet_cnt_cs >NUM_RETRY_CS )
      {
        terminated = terminate_gracefully(exp_start_time,0) ;
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
        terminated = terminate_gracefully(exp_start_time,0);
		break;
	 }
    }
	fflush(pathload_fp);
  }	//end inner while(1)

  if(ret_val == DISCONNECTED || ret == DISCONNECTED)
  continue;

//------------------------Start the reverse measurements
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


	if(connect(sock_udp, (struct sockaddr *)&client_udp_addr, sizeof(client_udp_addr)) <0)
	{
		printf("\nERROR : Could not connect to client\n");
		printf("\nMake sure that pathload runs at client\n");
		fprintf(pathload_fp,"\nERROR : Could not connect to client\n");
		fprintf(pathload_fp,"\nMake sure that pathload runs at client\n");
		fclose(pathload_fp);
		terminate_gracefully(tv1, 1);
		continue;
	}

	if((ret_val=recv_ctr_mesg(sock_tcp, ctr_buff)) == -1 )break;
	else if(ret_val==DISCONNECTED)break;

    if ( (((ret_val & CTR_CODE) >> 31) == 1) && ((ret_val & 0x7fffffff) == SEND_TRAIN ) )
    {
      if ( !quiet)
        printf("Estimating ADR to initialize rate adjustment algorithm => ");
//		fprintf(pathload_fp,"Estimating ADR to initialize rate adjustment algorithm => ");

      if ( send_train() == -1 ) continue ;
      if ( !quiet)
        printf("ADR Done\n");
		fprintf(pathload_fp,"ADR Done\n");
		fflush(stdout);
    }

	fleet_id=0;
    done=0;
    /* Start avail-bw measurement */
    while(!done)
    {
      if (( ret_val  = recv_ctr_mesg (sock_tcp, ctr_buff ) ) == -1 ) break ;
	  else if(ret_val==DISCONNECTED)break;

      if((((ret_val & CTR_CODE) >> 31) == 1) &&((ret_val&0x7fffffff) == TERMINATE))
      {
        if ( !quiet)
          printf("Terminating current run.\n");
		fprintf(pathload_fp,"Terminating current run.\n");
		  recv_result(sock_tcp);
        done=1;
      }
      else
      {
        transmission_rate = ret_val ;
        if ((cur_pkt_sz = recv_ctr_mesg(sock_tcp, ctr_buff)) <= 0 )break;
        if ((stream_len = recv_ctr_mesg(sock_tcp, ctr_buff))  <= 0 )break;
        if ((time_interval = recv_ctr_mesg(sock_tcp, ctr_buff)) <= 0 )break;
        if ((ret_val = recv_ctr_mesg (sock_tcp, ctr_buff )) == -1 )break;
		if(ret_val == DISCONNECTED)
		break;

        /* ret_val = SENd_FLEET */
        ctr_code = RECV_FLEET | CTR_CODE ;
        if ( send_ctr_mesg1(ctr_buff,  ctr_code  ) == -1 ) break;

        if(send_fleet()==-1) break ;
        if ( !quiet) printf("\n");
		fprintf(pathload_fp,"\n");
        fleet_id++ ;
      }
    }
	if(cleanedup == 0)
	{
		if(Verbose) printf("Downstream Measurement : %s",result1);
		close(sock_tcp);
		close(sock_udp);
		gettimeofday(&exp_end_time, NULL);
	    strncpy(buff, ctime(&(exp_end_time.tv_sec)), 24);
    	if(Verbose) printf("\nEnd of measurement: at %s\n", buff);
		if(cleanedup == 0){
			fprintf(pathload_fp,"%s",result1);
			fprintf(pathload_fp,"\nEnd of measurement: at %s\n", buff);
			fclose(pathload_fp);
		}
	}
	cleanedup = 0;
  }	//end outer while(1)
}

