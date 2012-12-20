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

/*------------------------------------------------------------
   pathload : an end-to-end available bandwidth 
              estimation tool
   Author   : Nachiket Deo          (nachiket.deo@gatech.edu)
   	      Partha Kanuparthy     (partha@cc.gatech.edu)
   	      Manish Jain           (jain@cc.gatech.edu)
              Constantinos Dovrolis (dovrolis@cc.gatech.edu )
   Release  : Ver 2.0
---------------------------------------------------------------*/


#include "pathload_gbls.h"
#include "pathload_rcv.h"


l_int32 recvfrom_latency(struct sockaddr_in rcv_udp_addr)
{
  char *random_data;
  float min_OSdelta[50], ord_min_OSdelta[50];
  l_int32 j ;
  struct timeval current_time, first_time ;

  if ( (random_data = malloc(max_pkt_sz*sizeof(char)) ) == NULL )
  {
    printf("ERROR : unable to malloc %d bytes \n",max_pkt_sz);
    exit(-1);
  }
  srandom(getpid()); /* Create random payload; does it matter? */
  for (j=0; j<max_pkt_sz-1; j++) random_data[j]=(char)(random()&0x000000ff);
  for (j=0; j<50; j++)
  {
    if ( sendto(sock_udp, random_data, max_pkt_sz, 0, 
         (struct sockaddr*)&rcv_udp_addr,sizeof(rcv_udp_addr)) == -1)
        perror("recvfrom_latency");
    gettimeofday(&first_time, NULL);
    recvfrom(sock_udp, random_data, max_pkt_sz, 0, NULL, NULL);
    gettimeofday(&current_time, NULL);
    min_OSdelta[j]= time_to_us_delta(first_time, current_time);
  }
  /* Use median  of measured latencies to avoid outliers */
  order_float(min_OSdelta, ord_min_OSdelta,0,50);
  free(random_data);
  return((l_int32)ord_min_OSdelta[25]);   
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

  if (Verbose)
    printf("  ADR [");
  fflush(stdout);
  fprintf(pathload_fp,"  ADR [");
  fflush(pathload_fp);
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
    fprintf(pathload_fp,".");
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
      bw_msr = ((28+max_pkt_sz) << 3) * (last-1) / delta;
      /* tell sender that it was agood train.*/
      ctr_code = GOOD_TRAIN | CTR_CODE ;
      send_ctr_mesg(ctr_buff, ctr_code ) ;
    }
    else
    {
      retry++ ;
      /* wait for atleast 10msec before requesting another train */
      last=train_len;
      while(!arrv_tv[last].tv_sec) --last;
      first=1 ; 
      while(!arrv_tv[first].tv_sec) ++first ; 
      delta = time_to_us_delta(arrv_tv[first], arrv_tv[last]);
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
  fputc(']',pathload_fp);
  while(--spacecnt>0)fputc(' ',pathload_fp);
  fprintf(pathload_fp,":: ");
  if ( !bad_train)
  {
    if(Verbose)
      printf("%.2fMbps\n", bw_msr ) ;
    fprintf(pathload_fp,"%.2fMbps\n", bw_msr ) ;
  }
  else
  {
    for ( i=0;i<num_bad_train;i++)
      if ( finite(bad_bw_msr[i]))
        sum += bad_bw_msr[i] ;
    bw_msr = sum/num_bad_train ; 
    if(Verbose)
      printf("%.2fMbps (I)\n", bw_msr ) ;
    fprintf(pathload_fp,"%.2fMbps (I)\n", bw_msr ) ;
  }
  return bw_msr ;
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
  int cliLen;
  char *pack_buf ;
  char ctr_buff[8];
/*#ifdef THRLIB
  thr_arg arg ;
  pthread_t tid;
  pthread_attr_t attr ;
#endif */
  exp_pack_id=0;

  if ( ( pack_buf = malloc(max_pkt_sz*sizeof(char))) == NULL ) 
  {
    printf("ERROR : unable to malloc %d bytes \n",max_pkt_sz);
	fclose(pathload_fp); 
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
			cliLen = sizeof(client_udp_addr);
		    if (recvfrom(sock_udp, pack_buf, max_pkt_sz, 0, (struct sockaddr *) &client_udp_addr, &cliLen) != -1)
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
		  ret_val = recv_ctr_mesg(sock_tcp,ctr_buff);
		  if(ret_val == DISCONNECTED)
		  break;
          if (ret_val != -1)
          {
            if ( (((ret_val & CTR_CODE) >> 31) == 1) && 
                  ((ret_val & 0x7fffffff) == FINISHED_TRAIN ) )
            {

				do {
            		fd_set readset1;
		            FD_ZERO(&readset1);
        		    FD_SET(sock_udp,&readset1);
		            select_tv.tv_sec=0;select_tv.tv_usec=10000;
        		    if (select(sock_udp+1,&readset1,NULL,NULL,&select_tv) > 0 )
            		{
		                if (FD_ISSET(sock_udp,&readset1) )
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
	  else
	  break;
  } while (1);
  if ( rcvd != train_len+1 )
  	bad_train=1;
  gettimeofday(&time[pack_id+1], NULL);
  sigstruct.sa_handler = SIG_DFL ;
  sigemptyset(&sigstruct.sa_mask);
  sigstruct.sa_flags = 0 ;
  sigaction(SIGUSR1 , &sigstruct,NULL );
  free(pack_buf);
  return bad_train ; 
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

/* 
   Receive N streams .
   After each stream, compute the loss rate.
   Mark a stream "lossy" , if losss rate in 
   that stream is more than a threshold.
*/
l_int32 recv_fleet()
{
  struct sigaction sigstruct;
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
  char *pkt_buf ;
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

  if ( (pkt_buf = malloc(cur_pkt_sz*sizeof(char)) ) == NULL )
  {
    printf("ERROR : unable to malloc %d bytes \n",cur_pkt_sz);
    exit(-1);
  }
  trend_idx=0;
  ic_flag = 0;
  if(verbose&&!Verbose)
    printf("Receiving Fleet %ld, Rate %.2fMbps\n",exp_fleet_id,tr);
  if(Verbose)
  {
    printf("\nReceiving Fleet %ld\n",exp_fleet_id);
    printf("  Fleet Parameter(req)  :: R=%.2fMbps, L=%ldB, K=%ldpackets, \
T=%ldusec\n",tr, cur_pkt_sz , stream_len,time_interval) ;
  }
  fprintf(pathload_fp,"\nReceiving Fleet %ld\n",exp_fleet_id);
  fprintf(pathload_fp,"  Fleet Parameter(req)  :: R=%.2fMbps, L=%ldB, \
K=%ldpackets, T=%ldusec\n",tr, cur_pkt_sz , stream_len,time_interval);
  
  if(Verbose)
    printf("  Lossrate per stream   :: ");
  fprintf(pathload_fp,"  Lossrate per stream   :: ");

  sigstruct.sa_handler = sig_sigusr1 ;
  sigemptyset(&sigstruct.sa_mask);
  sigstruct.sa_flags = 0 ;
  #ifdef SA_INTERRUPT
    sigstruct.sa_flags |= SA_INTERRUPT ;
  #endif
  sigaction(SIGUSR1 , &sigstruct,NULL );

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
		free(pkt_buf);
		terminate_gracefully(exp_start_time,1);
		return DISCONNECTED;
	  }
      else// if (select(sock_tcp+1,&readset,NULL,NULL,&select_tv) > 0 ) 
      {
        if (FD_ISSET(sock_udp,&readset) )
        {
		  ret = recv(sock_udp,pkt_buf,cur_pkt_sz,0);
		  if(ret <= 0)
		  {
	  //		printf("\nClient Disconnected\n");
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
        } // end of FD_ISSET

        if ( FD_ISSET(sock_tcp,&readset) )
        {
          /* check the control connection.*/
		  ret_val = recv_ctr_mesg(sock_tcp,ctr_buff);
		  if(ret_val == DISCONNECTED)
		  {
		    free(pkt_buf);
		  	return (DISCONNECTED);
		  }
          if (ret_val != -1 )
          {
            if ( (((ret_val & CTR_CODE) >> 31) == 1) && 
                  ((ret_val & 0x7fffffff) == FINISHED_STREAM ) )
            {
              while(1)
			  {
			    ret_val = recv_ctr_mesg(sock_tcp,ctr_buff );
				if(ret_val == DISCONNECTED)
				{
				  free(pkt_buf);
				  return(DISCONNECTED);
				}
				else if(ret_val == -1)
				continue;
				else
				break;
			  }
              if ( ret_val == stream_cnt )
              {
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
//	  printf(" %.2f ",owd[j]);
    }

    total_pkt_rcvd += pkt_rcvd ;
    finished_stream = 0 ;
    pkt_lost +=  stream_len  - exp_pkt_id ;
    pkt_loss_rate = (double )pkt_lost * 100. / stream_len ;
    if(Verbose)
      printf(":%.1f",pkt_loss_rate ) ;
    fprintf(pathload_fp,":%.1f",pkt_loss_rate ) ;
    exp_pkt_id = 0 ;
    stream_cnt++ ;
	fprintf(pathload_fp,"\nOWD :\n");
	for(j=0;j<stream_len;j++)
		fprintf(pathload_fp," %.2f ",owd[j]);
	fprintf(pathload_fp,"\nLossrate ");
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
      fprintf(pathload_fp,"\n  Fleet aborted due to high lossrate");
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
              printf("%d %f\n",p,owdfortd[p]);
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

  if ( Verbose ) printf("\n");
  fprintf(pathload_fp,"\n");

  if ( abort_fleet )
  {
     if(Verbose) printf("\tAborting fleet. Stream_cnt %d\n",stream_cnt);
	 fprintf(pathload_fp,"\tAborting fleet. Stream_cnt %d\n",stream_cnt);
     ctr_code = ABORT_FLEET | CTR_CODE;
     send_ctr_mesg(ctr_buff , ctr_code ) ;
     return_val = -1 ;
  }
  else
     print_contextswitch_info(num_sndr_cs,num_rcvr_cs,b2b_pkt_per_stream,stream_cnt);

  exp_fleet_id++ ;
//  for(j=0;j<stream_len;j++)
  //	fprintf(pathload_fp,"%.2f ",owd[j]);
  free(pkt_buf);
  return return_val  ;
}

void print_contextswitch_info(l_int32 num_sndr_cs[], l_int32 num_rcvr_cs[],l_int32 discard[],l_int32 stream_cnt)
{
    l_int32 j;

    if (Verbose)
      printf("  # of CS @ sndr        :: ");
    fprintf(pathload_fp,"  # of CS @ sndr        :: ");
    
    for(j=0;j<stream_cnt-1;j++)
    {
      if (Verbose) printf(":%2d",num_sndr_cs[j]);
      fprintf(pathload_fp,":%2d",num_sndr_cs[j]);
    }
    if ( Verbose ) printf("\n");
    fprintf(pathload_fp,"\n");
    if (Verbose)
      printf("  # of CS @ rcvr        :: ");
    fprintf(pathload_fp,"  # of CS @ rcvr        :: ");
    for(j=0;j<stream_cnt-1;j++)
    {
      if (Verbose) printf(":%2d",num_rcvr_cs[j]);
      fprintf(pathload_fp,":%2d",num_rcvr_cs[j]);
    }
    if ( Verbose ) printf("\n");
    fprintf(pathload_fp,"\n");
 
    if (Verbose)
      printf("  # of DS @ rcvr        :: ");
    fprintf(pathload_fp,"  # of DS @ rcvr        :: ");
    for(j=0;j<stream_cnt-1;j++)
    {
      if (Verbose) printf(":%2d",discard[j]);
      fprintf(pathload_fp,":%2d",discard[j]);
    }
     if ( Verbose ) printf("\n");
     fprintf(pathload_fp,"\n");
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
/*
void *ctrl_listen(void *arg)
{
  struct timeval select_tv;
  fd_set readset;
  l_int32 ret_val ;
  char ctr_buff[8]; 
  
/#ifdef THRLIB
  FD_ZERO(&readset);
  FD_SET(sock_tcp,&readset);
  select_tv.tv_sec=100;select_tv.tv_usec=0;
  if (select(sock_tcp+1,&readset,NULL,NULL,&select_tv) > 0 ) 
  {
    // check ... mesg received 
    if ( FD_ISSET(sock_tcp,&readset) )
    {
      // check the control connection.
      if (( ret_val = recv_ctr_mesg(sock_tcp,ctr_buff)) != -1 )
      {
        if ( (((ret_val & CTR_CODE) >> 31) == 1) && 
              ((ret_val & 0x7fffffff) == FINISHED_STREAM ) )
        {
          while((ret_val = recv_ctr_mesg(sock_tcp,ctr_buff ))== -1) ;
          if ( ret_val == ((thr_arg *)arg)->stream_cnt )
          {
            ((thr_arg *)arg)->finished_stream =1 ;
            pthread_kill(((thr_arg *)arg)->ptid,SIGUSR1);
            pthread_exit(NULL);
          }
        }
        else if ( (((ret_val & CTR_CODE) >> 31) == 1) &&
                  ((ret_val & 0x7fffffff) == FINISHED_TRAIN ) )
        {
          select_tv.tv_usec = 2000 ;
          select_tv.tv_sec = 0 ;
          select(1,NULL,NULL,NULL,&select_tv);
          ((thr_arg *)arg)->finished_stream =1 ;
          pthread_kill(((thr_arg *)arg)->ptid,SIGUSR1);
          pthread_exit(NULL);
        }
      }
    }
  }
#endif
  return NULL;
}*/

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

/* 
    Order an array of float using bubblesort 
*/
void order_float(float unord_arr[], float ord_arr[],l_int32 start, l_int32 num_elems)
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

/* 
    Order an array of l_int32 using bubblesort 
*/
void order_int(l_int32 unord_arr[], l_int32 ord_arr[], l_int32 num_elems)
{
    l_int32 i,j;
    l_int32 temp;
    for (i=0;i<num_elems;i++) ord_arr[i]=unord_arr[i];
    for (i=1;i<num_elems;i++) {
        for (j=i-1;j>=0;j--)
            if (ord_arr[j+1] < ord_arr[j]) {
                temp=ord_arr[j]; 
                ord_arr[j]=ord_arr[j+1]; 
                ord_arr[j+1]=temp;
            }
            else break;
    }
}

/*
    Send a message through the control stream
*/
void send_ctr_mesg(char *ctr_buff, l_int32 ctr_code)
{
//	printf("\nIn SND\n");
    l_int32 ctr_code_n = htonl(ctr_code);
    memcpy((void*)ctr_buff, &ctr_code_n, sizeof(l_int32));
    if (write(sock_tcp, ctr_buff, sizeof(l_int32)) != sizeof(l_int32))
    {
        fprintf(stderr, "send control message failed:\n");
	//	fclose(pathload_fp); 
	//	rcv_func(1);
    }
//	printf("\nSND Done\n");
}


/*
        Send a message through the control stream
*/
int send_ctr_mesg1(char *ctr_buff, l_int32 ctr_code)
{
    l_int32 ctr_code_n = htonl(ctr_code);
    memcpy((void*)ctr_buff, &ctr_code_n, sizeof(l_int32));
    if (write(sock_tcp, ctr_buff, sizeof(l_int32)) != sizeof(l_int32))
      return -1 ; 
    else return 0;
}


void send_result()
{
    if (write(sock_tcp, result, 512) != 512)
    {
        fprintf(stderr, "send result failed:\n");
		close(sock_tcp);
		close(sock_udp);
	//	rcv_func(1);
	//	fclose(pathload_fp); 
    }
}

int receive(l_int32 sock, char *buff, int size)
{
	int readsz = 0;
	int ret = 0;
	fd_set readset ;
   	struct timeval select_tv;

	while(readsz < size)
	{
	    	select_tv.tv_sec = 50 ; // if noctrl mesg for 50 sec, terminate 
		select_tv.tv_usec=0 ;
	    	FD_ZERO(&readset);
	    	FD_SET(sock,&readset);
		ret = recv(sock, (char *)buff + readsz, size - readsz, 0);

		if(ret < 0)
		{
			perror("receive: recvd Client disconnected\n");
			return ret;
		}
		if(ret == 0)
		{
			perror("receive: ret == 0\n");
			return ret;
		}
		readsz += ret;

		if (readsz >= size || select(sock+1,&readset,NULL,NULL,&select_tv) <= 0 )
		{
			break;
		}
	}
	return readsz;
}


/*
  Receive message from the control stream
*/
l_int32 recv_ctr_mesg(l_int32 ctr_strm, char *ctr_buff)
{
  l_int32 ctr_code,ret;
  struct timeval select_tv;
  fd_set readset;

  select_tv.tv_sec = 50;
  select_tv.tv_usec = 0;
  FD_ZERO(&readset);
  FD_SET(ctr_strm, &readset);
  if(select(ctr_strm+1,&readset,NULL,NULL,&select_tv) > 0)
  {
  
  	gettimeofday(&first_time,0);
  	ret = recv(ctr_strm, ctr_buff, sizeof(l_int32),0);
	if(ret < 0)
	{
		terminate_gracefully(first_time,1);	//first_time : dummy
		return(-1);
	}
	if(ret == 0)
	{
		printf("\nClient Disconnected");
		terminate_gracefully(first_time,1);	//first_time : dummy
		return(DISCONNECTED);
	}
  	memcpy(&ctr_code, ctr_buff, sizeof(l_int32));
  	return(ntohl(ctr_code));
  }
  else
  {
  	printf("Client not responding\n");
	terminate_gracefully(first_time,1);	//first_time : dummy
	return(-1);
  }
  return(0);
}


/*
  Receive result from the control stream
  */
l_int32 recv_result(l_int32 ctr_strm)
{
	l_int32 ret;
	struct timeval select_tv;
	fd_set readset;
	select_tv.tv_sec = 50;
	select_tv.tv_usec = 0;
	FD_ZERO(&readset);
	FD_SET(ctr_strm, &readset);
	memset(result1,0,512);
	if(select(ctr_strm+1,&readset,NULL,NULL,&select_tv)>0)
	{
		ret = recv(ctr_strm, result1, 512,0);
		if(ret < 0)
		{
			printf("ret = 0 in result");
			return(-1);
	    	}
		else if(ret<512)	printf("\nret < 512");
	    	else if(ret == 0)
	    	{
	        	printf("\nResult : Client Disconnected");
	        	terminate_gracefully(first_time,1); //first_time : dummy
	    	}
		result1[ret-1] = '\0';
	}
	else
	{
		printf("\nWaiting to receive result from Client");
	}
	return 0;
}
																			  


/*
  Compute the time difference in microseconds
  between two timeval measurements
*/
double time_to_us_delta(struct timeval tv1, struct timeval tv2)
{
  double time_us;
  time_us = (double)
    ((tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec));
  return time_us;
}

/*
  Compute the average of the set of measurements <data>.
*/
double get_avg(double data[], l_int32 num_values)
{
  l_int32 i;
  double sum_;
  sum_ = 0;
  for (i=0; i<num_values; i++) sum_ += data[i];
  return (sum_ / (double)num_values);
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

double grey_bw_resolution() 
{
  if ( adr )
    return (.05*adr<12?.05*adr:12) ;
  else 
    return min_rate ;
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
//printf("\ntr = %f  in radj_increasing\n",tr);
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
  {
	tr = old_tr * 2.;
  }
//printf("\ntr = %f  in radj_notrend\n",tr);
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
//      tr_min =  tr_max = tr ;		//Added tr_max = tr!!!
/*   if ( tr_min > tr_max && tr_max==0)
		tr_max = tr_min;*/

    if ( !converged_gmn_rmn_tm && !converged() ) 
      radj_notrend(NOTREND) ;
    else
      ret_val=-1 ; //return -1 ;
  }
  else if ( flag == GREY )
  {
//	printf("\nIn ( flag == GREY )\n");
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
//	printf("\nIn else \n");
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
  fprintf(pathload_fp,"  Rmin-Rmax             :: %.2f-%.2fMbps\n",tr_min,tr_max);
  fprintf(pathload_fp,"  Gmin-Gmax             :: %.2f-%.2fMbps\n",grey_min,grey_max);

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

/*
  discards owd of packets received when
  receiver was NOT running.
*/
l_int32 eliminate_rcvr_side_CS ( double rcvr_time_stamp[] , double owd[],double owdfortd[], l_int32 low,l_int32 high,l_int32 *num_rcvr_cs,l_int32 *tmp_b2b )
{
  l_int32 b2b_pkt[MAX_STREAM_LEN] ;
  l_int32 i,k=0 ;
  l_int32 len=0;
  l_int32 min_gap;

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
  return len ;
}

/* Adjust offset to zero again  */
void adjust_offset_to_zero(double owd[], l_int32 len)
{
    l_int32 owd_min = 0;
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

#define INCR    1
#define NOTR    2
#define DISCARD 3
#define UNCL    4
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
     fprintf(pathload_fp,"d");
     pct_trend[i] = DISCARD ;
   }
   else if ( pct_metric[i] > 1.1 * PCT_THRESHOLD )
   {
     if (Verbose)
       printf("I");
     fprintf(pathload_fp,"I");
     pct_trend[i] = INCR ;
   }
   else if ( pct_metric[i] < .9 * PCT_THRESHOLD )
   {
     if (Verbose)
       printf("N");
     fprintf(pathload_fp,"N");
     pct_trend[i] = NOTR ;
   }
   else if(pct_metric[i] <= PCT_THRESHOLD*1.1 && pct_metric[i] >= PCT_THRESHOLD*.9 )
   {
     if (Verbose)
       printf("U");
     fprintf(pathload_fp,"U");
     pct_trend[i] = UNCL ;
   }
 }
 if (Verbose)
   printf("\n");
 fprintf(pathload_fp,"\n");
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
        fprintf(pathload_fp,"d");
        pdt_trend[i] = DISCARD ;
    }
    else if ( pdt_metric[i] > 1.1 * PDT_THRESHOLD )
    {
 		if (Verbose)
        	printf("I");
        fprintf(pathload_fp,"I");
        pdt_trend[i] = INCR ;
    }
    else if ( pdt_metric[i] < .9 * PDT_THRESHOLD )
    {
	 	if (Verbose)
        	printf("N");
	    fprintf(pathload_fp,"N");
        pdt_trend[i] = NOTR ;
    }
    else if ( pdt_metric[i] <= PDT_THRESHOLD*1.1 && pdt_metric[i] >= PDT_THRESHOLD*.9 )
    {
	 	if (Verbose)
        	printf("U");
        fprintf(pathload_fp,"U");
        pdt_trend[i] = UNCL ;
    }
 }
 if (Verbose)
 	printf("\n");
 fprintf(pathload_fp,"\n");
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
  fprintf(pathload_fp,"  PCT metric/stream[%2d] :: ",trend_idx); 
  for (i=0; i < trend_idx;i++ )
  {
    if (Verbose)
      printf("%3.2f:",pct_metric[i]);
    fprintf(pathload_fp,"%3.2f:",pct_metric[i]);
  }
  if (Verbose)
    printf("\n"); 
  fprintf(pathload_fp,"\n"); 
  if (Verbose)
    printf("  PDT metric/stream[%2d] :: ",trend_idx); 
  fprintf(pathload_fp,"  PDT metric/stream[%2d] :: ",trend_idx); 
  for (i=0; i < trend_idx;i++ )
  {
    if (Verbose)
      printf("%3.2f:",pdt_metric[i]);
    fprintf(pathload_fp,"%3.2f:",pdt_metric[i]);
  }
  if (Verbose)
    printf("\n"); 
  fprintf(pathload_fp,"\n"); 
  if (Verbose)
    printf("  PCT Trend/stream [%2d] :: ",trend_idx); 
  fprintf(pathload_fp,"  PCT Trend/stream [%2d] :: ",trend_idx); 
  get_pct_trend(pct_metric,pct_trend,trend_idx);
  if (Verbose)
    printf("  PDT Trend/stream [%2d] :: ",trend_idx); 
  fprintf(pathload_fp,"  PDT Trend/stream [%2d] :: ",trend_idx); 
  get_pdt_trend(pdt_metric,pdt_trend,trend_idx);
  
  if (Verbose)
    printf("  Trend per stream [%2d] :: ",trend_idx); 
  fprintf(pathload_fp,"  Trend per stream [%2d] :: ",trend_idx); 
  for (i=0; i < trend_idx;i++ )
  {
    if ( pct_trend[i] == DISCARD || pdt_trend[i] == DISCARD )
    {
       if (Verbose)
         printf("d");
       fprintf(pathload_fp,"d");
       num_dscrd_strm++ ;
    }
    else if ( pct_trend[i] == INCR &&  pdt_trend[i] == INCR )
    {
       if (Verbose)
         printf("I");
       fprintf(pathload_fp,"I");
       i_cnt++;
    }
    else if ( pct_trend[i] == NOTR && pdt_trend[i] == NOTR )
    {
       if (Verbose)
         printf("N");
       fprintf(pathload_fp,"N");
       n_cnt++;
    }
    else if ( pct_trend[i] == INCR && pdt_trend[i] == UNCL )
    {
       if (Verbose)
         printf("I");
       fprintf(pathload_fp,"I");
       i_cnt++;
    }
    else if ( pct_trend[i] == NOTR && pdt_trend[i] == UNCL )
    {
       if (Verbose)
         printf("N");
       fprintf(pathload_fp,"N");
       n_cnt++;
    }
    else if ( pdt_trend[i] == INCR && pct_trend[i] == UNCL )
    {
       if (Verbose)
         printf("I");
       fprintf(pathload_fp,"I");
       i_cnt++;
    }
    else if ( pdt_trend[i] == NOTR && pct_trend[i] == UNCL )
    {
       if (Verbose)
         printf("N");
       fprintf(pathload_fp,"N");
       n_cnt++ ;
    }
    else
    {
       if (Verbose)
         printf("U");
       fprintf(pathload_fp,"U");
    }
    total++ ;
  }
  if (Verbose) printf("\n"); 
  fprintf(pathload_fp,"\n"); 

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
    fprintf(pathload_fp,"  Aggregate trend       :: INCREASING\n");
    return INCREASING ;
  }
  else if( (double)n_cnt/(total) >= AGGREGATE_THRESHOLD )
  {
    if (Verbose)
      printf("  Aggregate trend       :: NO TREND\n");
    fprintf(pathload_fp,"  Aggregate trend       :: NO TREND\n");
    return NOTREND ;
  }
  else 
  {
    if (Verbose)
      printf("  Aggregate trend       :: GREY\n");
    fprintf(pathload_fp,"  Aggregate trend       :: GREY\n");
    return GREY ;
  }
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

void get_sending_rate() 
{
 time_interval = snd_time_interval/num;
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
    printf("  Fleet Parameter(act)  :: R=%.2fMbps, L=%ldB, K=%ldpackets, T=%ldusec\n",cur_actual_rate, cur_pkt_sz , stream_len,time_interval);
  fprintf(pathload_fp,"  Fleet Parameter(act)  :: R=%.2fMbps, L=%ldB, K=%ldpackets, T=%ldusec\n",cur_actual_rate,cur_pkt_sz,stream_len,time_interval);
  snd_time_interval=0;
  num=0;
}

l_int32 terminate_gracefully(struct timeval exp_start_time,l_int32 disconnected)
{
  l_int32 ctr_code;
  char ctr_buff[8],buff[26],temp[80];
  struct timeval exp_end_time;
  double min=0,max=0 ;

  if(disconnected)
  {
  	close(sock_tcp);
	close(sock_udp);
	printf("\nTerminate : Client Disconnected\n");
	fprintf(pathload_fp,"\nClient Disconnected");
	fclose(pathload_fp); 
	cleanedup = 1;
	return DISCONNECTED;
  }
  ctr_code = TERMINATE | CTR_CODE;
  send_ctr_mesg(ctr_buff, ctr_code);
  gettimeofday(&exp_end_time, NULL);
  strncpy(buff, ctime(&(exp_end_time.tv_sec)), 24);
  buff[24] = '\0';
  if (verbose || Verbose)
    printf("\n\t*****  RESULT *****\n");
//  fprintf(pathload_fp,"\n\t*****  RESULT *****\n");
  
  if ( min_rate_flag )
  {
    if (verbose || Verbose)
    {
      printf("Avail-bw < minimum sending rate.\n");
      printf("Increase MAX_TIME_INTERVAL in pathload_rcv.h from 200000 usec to a higher value.\n");
//	  strcat(result,"Available bandwidth < minimum sending rate.#");
	  strcat(result,"The measured available bandwidth is less than the minimum possible probing rate.#");
//      strcat(result,"Increase MAX_TIME_INTERVAL in pathload_rcv.h from 200000 usec to a higher value.#");
    }
    fprintf(pathload_fp,"Avail-bw < minimum sending rate.\n");
    fprintf(pathload_fp,"Increase MAX_TIME_INTERVAL in pathload_rcv.h from 200000 usec to a higher value.\n");
  }
  else if ( max_rate_flag && !interrupt_coalescence )
  {
    if (verbose || Verbose)
    {
      	printf("Avail-bw > maximum sending rate.\n");
		printf("Measurement duration : %.2f sec \n", time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
		printf("Avail-bw is at least %.2f (Mbps)\n", max_rate);
	}
		strcat(result,"The measured available bandwidth is higher than the maximum possible probing rate.#");
		strcat(result,"Available bandwidth is at least ");
		sprintf(temp,"%.2f",max_rate);
		strcat(result,temp);
		strcat(result," (Mbps)#");
		strcat(result,"Measurement duration : ");
        sprintf(temp,"%.2f",time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
	    strcat(result,temp);
        strcat(result," sec");
    	fprintf(pathload_fp,"Avail-bw > maximum sending rate.\n");
    	fprintf(pathload_fp,"Avail-bw is at least %.2f (Mbps)\n", max_rate);
  }
  else if (bad_fleet_cs && !interrupt_coalescence)
  {
	if (verbose || Verbose)
    {
		printf("Measurement terminated due to frequent cotext-switching at server.\n");
		printf("Measurement duration : %.2f sec \n", time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
	}
	strcat(result,"Measurements terminated due to frequent context-switching at server#");
    strcat(result,"Measurement duration : ");
    sprintf(temp,"%.2f",time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
    strcat(result,temp);
    strcat(result," sec");

    fprintf(pathload_fp,"Measurement terminated due to frequent context-switching at server.\n");
	fprintf(pathload_fp,"Measurement duration : %.2f sec \n", time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
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
      if (verbose || Verbose)
      {
//	  	if(min>max) max = min;
		if(min==0)
		{
			if(max==0)
			{
				printf("Measurement terminated due to insufficient probing rate.\nPlease try again later");
			}
			else
			{
				printf("Available bandwidth is at most %.2f  (Mbps)\n",max);
			}
		}
		else if(max==0)
		{
			printf("Available bandwidth is at least %.2f (Mbps)\n",min);
		}
		else
		{
	        printf("Available bandwidth range : %.2f - %.2f (Mbps)\n", min, max);
    	    printf("Measurements finished at %s \n",  buff);
        	printf("Measurement duration : %.2f sec \n", time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
		}
      }
		if(min==0)
		{
			if(max==0)
			{
				strcat(result,"#Measurement terminated due to insufficient probing rate.#Please try again later#");
				fprintf(pathload_fp,"Measurement terminated due to insufficient probing rate.\nPlease try again later");
			}
			else
			{
				strcat(result,"#Available bandwidth is at most ");
				sprintf(temp,"%.2f",max);
				strcat(result,temp);
				strcat(result," (Mbps)#");
				fprintf(pathload_fp,"Available bandwidth is at most %.2f  (Mbps)\n",max);
		 }
		}
		else if(max==0)
		{
			strcat(result,"#Available bandwidth is at least ");
			sprintf(temp,"%.2f",min);
			strcat(result,temp);
			strcat(result," (Mbps)#");
			fprintf(pathload_fp,"Available bandwidth is at least %.2f (Mbps)\n",min);
		 }
		else
		{
			strcat(result,"Available bandwidth range : ");
			sprintf(temp,"%.2f",min);
			strcat(result,temp);
			strcat(result," - ");
			sprintf(temp,"%.2f",max);
			strcat(result,temp);
//			strcat(result," (Mbps)\nMeasurements finished at ");
//			strcat(result,buff);
			strcat(result,"(Mbps)#");
		    fprintf(pathload_fp,"Available bandwidth range : %.2f - %.2f (Mbps)\n", min, max);
		}
			strcat(result,"Measurement duration : ");
			sprintf(temp,"%.2f",time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
			strcat(result,temp);
			strcat(result," sec");
			fprintf(pathload_fp,"Measurements finished at %s \n",  buff);
		    fprintf(pathload_fp,"Measurement duration : %.2f sec \n", time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
    }
  }
  else 
  {
    if ( !interrupt_coalescence && ((converged_gmx_rmx_tm && converged_gmn_rmn_tm) || converged_rmn_rmx_tm ))
    {
      if (Verbose)
        printf("Actual probing rate != desired probing rate.\n");
		strcat(result,"Measurements terminated due to insufficient probing rate.#Please try again later.#");
      fprintf(pathload_fp,"Actual probing rate != desired probing rate.\n");
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
        printf("Exiting due to grey bw resolution\n");
//		strcat(result,"Exiting due to grey bw resolution#");
      fprintf(pathload_fp,"Exiting due to grey bw resolution\n");
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

    if (verbose||Verbose)
    {
      if ( lower_bound)
      {
        printf("Receiver NIC has interrupt coalescence enabled\n");
        printf("Available bandwidth is greater than %.2f (Mbps)\n", min);
      }
      else
	  {
//	  	if(min>max) max = min;
    	if(min==0)
		{
			if(max==0)
			{
				printf("Measurement terminated due to insufficient probing rate.\nPlease try again later");
			}
			else
			{
				printf("Available bandwidth is at most %.2f  (Mbps)\n",max);
			}
		}
		else if(max==0)
		{
			printf("Available bandwidth is at least %.2f (Mbps)\n",min);
		}
		
		else
		{
			printf("Available bandwidth range : %.2f - %.2f (Mbps)\n", min, max);
		}
	  }
      printf("Measurements finished at %s \n",  buff);
      printf("Measurement duration : %.2f sec \n",time_to_us_delta(exp_start_time,exp_end_time)/1000000);
    }
    if ( lower_bound)
    {
    	strcat(result,"Measurements terminated due to interrupt coalescence at the receiving network interface card.#");
		strcat(result,"Available bandwidth is greater than ");
		sprintf(temp,"%.2f",min);
		strcat(result,temp);
		strcat(result," (Mbps)#");
		fprintf(pathload_fp,"Receiver NIC has interrupt coalescence enabled\n");
      	fprintf(pathload_fp,"Available bandwidth is greater than %.2f (Mbps)\n", min);
    }
    else
	{
      if(min==0)
	  {
			if(max==0)
			{
				strcat(result,"#Measurement terminated due to insufficient probing rate.#Please try again later#");
				fprintf(pathload_fp,"#Measurement terminated due to insufficient probing rate.#Please try again later#");
			}
			else
			{
				strcat(result,"Available bandwidth is at most ");
				sprintf(temp,"%.2f",max);
				strcat(result,temp);
				strcat(result," (Mbps)#");
				fprintf(pathload_fp,"Available bandwidth is at most %.2f  (Mbps)\n",max);
		 	}
	  }
	  else if(max==0)
	  {
			strcat(result,"Available bandwidth is at least ");
			sprintf(temp,"%.2f",min);
			strcat(result,temp);
			strcat(result," (Mbps)#");
			fprintf(pathload_fp,"Available bandwidth is at least %.2f (Mbps)\n",min);
	  }
	  else
	  {
			strcat(result,"Available bandwidth range : ");
			sprintf(temp,"%.2f",min);
			strcat(result,temp);
			strcat(result," - ");
			sprintf(temp,"%.2f",max);
			strcat(result,temp);
			strcat(result," (Mbps)#");
			fprintf(pathload_fp,"Available bandwidth range : %.2f - %.2f (Mbps)\n", min, max);
	  }
	}
	strcat(result,"Measurement duration : ");
	sprintf(temp,"%.2f",time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
	strcat(result,temp);
	strcat(result," sec");

    fprintf(pathload_fp,"Measurements finished at %s \n",  buff);
    fprintf(pathload_fp,"Measurement duration : %.2f sec \n", time_to_us_delta(exp_start_time, exp_end_time) / 1000000);
  }
   fflush(pathload_fp);
   send_result();
   return 0; 
}


/* prl_int32 time */
/*void print_time(FILE *fp, l_int32 time)
{
  if( time<10){
    fprintf(fp,"0");
    fprintf(fp,"%1d",time);
  }
  else{
    fprintf(fp,"%2d",time);
  }
}*/

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
   if a approx-equal b, return 1
   else 0
*/
l_int32 equal(double a , double b)
{
  l_int32 maxdiff ;
  if ( a<b?a:b < 500 ) maxdiff = 2.5 ;
  else maxdiff = 5 ;
  if ( abs( a - b ) / b <= .02  && abs(a-b) < maxdiff )
    return 1 ;
  else
    return 0;
}

#include <sys/stat.h>


int init_TCP_connection()
{
	struct sockaddr_in server_tcp_addr,client_tcp_addr;
	int opt_len;
	server_tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_tcp_sock < 0)
	{ 
		printf("Invalid Socket\n");
        return (-1);
	}
 	opt_len=1;
   	if (setsockopt(server_tcp_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt_len, sizeof(opt_len)) < 0)
	{
		perror("setsockopt(SOL_SOCKET,SO_REUSEADDR):");
	}
	bzero((char *) &server_tcp_addr, sizeof(server_tcp_addr));
	
	server_tcp_addr.sin_family = AF_INET;
	server_tcp_addr.sin_addr.s_addr = INADDR_ANY;
	server_tcp_addr.sin_port = htons(TCPSND_PORT);

	if (bind(server_tcp_sock, (struct sockaddr *) &server_tcp_addr,sizeof(server_tcp_addr)) < 0) 
	{ 
		printf("Could not bind %d\n",errno);
        return(-1);
	}
	if(Verbose)
	{	
		printf("\nListening for TCP connection\n");
		fflush(stdout);
	}
	// Listen for Client to connect
	if (listen(server_tcp_sock,1) < 0)
	{
		printf("ERROR : Listen on server TCP Socket\n");
		//exit(-1);
		return -1;
	}
	
	// TCP Connection part
	int len = sizeof(server_tcp_addr);
	sock_tcp = accept(server_tcp_sock,(struct sockaddr *) &client_tcp_addr, &len);
	if (sock_tcp < 0) 
	{ 
		printf("ERROR : Accept on server TCP Socket\n");
//		exit(-1);
		return -1;
	}
	close(server_tcp_sock);
	if(Verbose)
	{
		printf("\nControl Stream established\n");
		fflush(stdout);
	}
	host_client=gethostbyaddr((char*)&(client_tcp_addr.sin_addr), sizeof(client_tcp_addr.sin_addr), AF_INET);
    	struct in_addr **addr_list;
	addr_list = (struct in_addr **)host_client->h_addr_list;
	strcpy(hostname, inet_ntoa(*addr_list[0]));
	strcat(filename,"/");
	strcat(filename,hostname);
//	strcat(filename,"-");
	if (fcntl(sock_tcp, F_SETFL, O_NONBLOCK) < 0)
	{
		perror("fcntl:");
		return -1;
//		exit(-1);
	}
	return 0;
}

int init_UDP_connection()
{
	l_int32 send_buff_sz;
	if ((sock_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	perror("socket");
	return (-1);
	}
	bzero((char *) &server_udp_addr, sizeof(server_udp_addr));
	server_udp_addr.sin_family = AF_INET; // host byte order
	server_udp_addr.sin_port = htons(UDPRCV_PORT); // short, network byte order
	server_udp_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	
	if (bind(sock_udp, (struct sockaddr *)&server_udp_addr,sizeof(struct sockaddr)) == -1) 
	{
		printf("ERROR: UDP bind");
		return (-1);
	}
	
	send_buff_sz = UDP_BUFFER_SZ;
	if (setsockopt(sock_udp, SOL_SOCKET, SO_SNDBUF, (char*)&send_buff_sz, sizeof(send_buff_sz)) < 0)
	{
		send_buff_sz/=2;
		if (setsockopt(sock_udp, SOL_SOCKET, SO_SNDBUF, (char*)&send_buff_sz, sizeof(send_buff_sz)) < 0)
		{
			perror("setsockopt(SOL_SOCKET,SO_SNDBUF):");
		}
	}
	send_buff_sz = UDP_BUFFER_SZ;
	if (setsockopt(sock_udp, SOL_SOCKET, SO_RCVBUF, (char*)&send_buff_sz, sizeof(send_buff_sz)) < 0)
	{
		send_buff_sz/=2;
		if (setsockopt(sock_udp, SOL_SOCKET, SO_RCVBUF, (char*)&send_buff_sz, sizeof(send_buff_sz)) < 0)
		{
			perror("setsockopt(SOL_SOCKET,SO_RCVBUF):");
		}
	}
	return 0;
}

/*
void UDPComm(l_int32 socket)
{
	int addr_len, numbytes;
	char buf[512];
	addr_len = sizeof(struct sockaddr);
	while(1)
	{
		//if ((numbytes=recvfrom(socket,buf, MAXBUFLEN-1, 0,(struct sockaddr *)&clnt_udp_addr, &addr_len)) == -1) {
		//perror("recvfrom");
		//exit(1);
		//}
		memset(buf,0,512);
		numbytes = recv(socket, buf, 512, 0);
		if (numbytes < 0) 
		{
			printf("Error reading from socket");
			exit(0);
		}
//		printf("got UDP packet from %s\n",inet_ntoa(clnt_udp_addr.sin_addr));
		printf("packet is %d bytes long\n",numbytes);
		buf[numbytes] = 0;
		printf("packet contains \"%s\"\n",buf);

		if(strcmp(buf,"goodbye\n")==0)		//If the client says goodbye, disconnect
		{
				break;
		}
	}
}
*/

#define NUM_SELECT_CALL 31
void min_sleeptime()
{
  struct timeval sleep_time, time[NUM_SELECT_CALL] ;
  l_int32 res[NUM_SELECT_CALL] , ord_res[NUM_SELECT_CALL] ;
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
#ifdef DEBUG
  printf("DEBUG :: min_sleep_interval %ld\n",min_sleep_interval);
  printf("DEBUG :: min_timer_intr %ld\n",min_timer_intr);
#endif

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
	
	fflush(stdout);
	if ( (pack_buf = (char *)malloc(max_pkt_sz*sizeof(char)) ) == NULL )
	{
		printf("ERROR : send_train : unable to malloc %d bytes \n",max_pkt_sz);
		fprintf(pathload_fp,"ERROR : send_train : unable to malloc %d bytes \n",max_pkt_sz);
		exit(-1);
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
			if(send(sock_udp, pack_buf, max_pkt_sz,0 ) ==-1)
			{
			}
		}

		select_tv.tv_sec=0;select_tv.tv_usec=1000;
		select(0,NULL,NULL,NULL,&select_tv);
		ctr_code = FINISHED_TRAIN | CTR_CODE ;
		send_ctr_mesg(ctr_buff, ctr_code );
		ret_val  = recv_ctr_mesg (sock_tcp,ctr_buff);
		if (ret_val == -1 || ret_val == DISCONNECTED)
		{
			return -1;
		}

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
	if(Verbose) printf("\nreverse ADR done\n");
	fflush(stdout);
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

  if ( (pkt_buf = malloc(cur_pkt_sz*sizeof(char)) ) == NULL )
  {
    printf("ERROR : send_fleet : unable to malloc %ld bytes \n",cur_pkt_sz);
	fprintf(pathload_fp,"ERROR : send_fleet : unable to malloc %ld bytes \n",cur_pkt_sz);
	fclose(pathload_fp);
    exit(-1);
  }
  srandom(getpid()); /* Create random payload; does it matter? */
  for (i=0; i<cur_pkt_sz-1; i++) pkt_buf[i]=(char)(random()&0x000000ff);
  pkt_id = 0 ;
  if ( !quiet)
    printf("Sending fleet %ld ",fleet_id);
	fprintf(pathload_fp,"Sending fleet %ld ",fleet_id);
  while ( stream_cnt < num_stream )
  {
    if ( !quiet) printf("#"); 
	fprintf(pathload_fp,"#");
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
	  //fprintf(stdout,"sending to %d size %d\n",sock_udp, cur_pkt_sz);
      if ( send(sock_udp, pkt_buf, cur_pkt_sz,0 ) == -1 ) 
	  {	
		fprintf(stderr, "sockudp: %d\n", sock_udp);
		perror("send"); 
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
    if ( send_ctr_mesg1(ctr_buff, ctr_code ) == -1 ) 
    {
      free(pkt_buf);
      perror("send_ctr_mesg : FINISHED_STREAM");
      return -1;
    }
    if ( send_ctr_mesg1(ctr_buff, stream_cnt ) == -1 )
    {
      free(pkt_buf);
      return -1;
    }

    /* Wait for continue/cancel message from receiver.*/
	ret_val = recv_ctr_mesg (sock_tcp,ctr_buff );
    if(ret_val == -1 || ret_val == DISCONNECTED)
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
	if(version == NEW)
    	recv_owdData();
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

/* 
	Receive OWD data
*/
l_int32 recv_owdData()		
{
	struct timeval select_tv;
	int numbytes,size,i;
	struct owdData data;
	char ctr_buff[8];
	fd_set readset ;
	select_tv.tv_sec = 50 ; // if noctrl mesg for 50 sec, terminate 
	select_tv.tv_usec=0 ;
	FD_ZERO(&readset);
	FD_SET(sock_tcp,&readset);
	size = recv_ctr_mesg(sock_tcp, ctr_buff);
	if(size <= 0) return -1;
	if ( select(sock_tcp+1,&readset,NULL,NULL,&select_tv) > 0 )
	{
		numbytes = receive(sock_tcp, (char *)&data, size);
	}
	else
	{
		printf("Waiting for the Server to respond\n");
		terminate_gracefully(select_tv, 1);
		return -1;
	}
	fprintf(pathload_fp,"\nfleetid = %d\n",ntohl(data.fleetid));
	size = (size - sizeof(data.fleetid))/sizeof(unsigned int);
	for(i = 0;i<size;i++)
		fprintf(pathload_fp," %d ",ntohl(data.owd[i]));
	return 0;
}

