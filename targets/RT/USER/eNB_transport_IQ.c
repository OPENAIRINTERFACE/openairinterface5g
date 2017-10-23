/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file eNB_transport_IQ.c
 * \brief eNB transport IQ samples 
 * \author  Katerina Trilyraki, Navid Nikaein, Raymond Knopp
 * \date 2015
 * \version 0.1
 * \company Eurecom
 * \maintainer:  navid.nikaein@eurecom.fr
 * \note
 * \warning very experimental 
 */
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "common_lib.h"
#include "PHY/defs.h"
#include "rrh_gw.h"
#include "rrh_gw_externs.h"
#include "rt_wrapper.h"

#define PRINTF_PERIOD    3750
#define HEADER_SIZE      ((sizeof(int32_t) + sizeof(openair0_timestamp))>>2)

pthread_cond_t          sync_eNB_cond[4];
pthread_mutex_t         sync_eNB_mutex[4];
pthread_mutex_t         sync_trx_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t          sync_trx_cond=PTHREAD_COND_INITIALIZER;

openair0_timestamp 	nrt_eNB_counter[4]= {0,0,0,0};
int32_t 	overflow_rx_buffer_eNB[4]= {0,0,0,0};
int32_t 	nsamps_eNB[4]= {0,0,0,0};
int32_t 	eNB_tx_started=0,eNB_rx_started=0;
int32_t 	counter_eNB_rx[4]= {0,0,0,0};
int32_t 	counter_eNB_tx[4]= {0,0,0,0};
uint8_t		RT_flag_eNB,NRT_flag_eNB;
void 		*rrh_eNB_thread_status;
int 	        sync_eNB_rx[4]= {-1,-1,-1,-1};
unsigned int    sync_trx=0;

int32_t		**tx_buffer_eNB;
int32_t         **rx_buffer_eNB;
void 		**rx_eNB; //was fixed to 2 ant
void 		**tx_eNB; //was fixed to 2 ant

openair0_timestamp	timestamp_eNB_tx[4]= {0,0,0,0};// all antennas must have the same ts
openair0_timestamp      timestamp_eNB_rx[4]= {0,0,0,0};
openair0_timestamp	timestamp_rx=0,timestamp_tx=0;

unsigned int   rx_pos=0, next_rx_pos=0;
unsigned int   tx_pos=0, tx_pos_rf=0, prev_tx_pos=0;
unsigned int   rt_period=0;

struct itimerspec       timerspec;
pthread_mutex_t         timer_mutex;



/*! \fn void *rrh_eNB_rx_thread(void *arg)
 * \brief this function
 * \param[in]
 * \return none
 * \note
 * @ingroup  _oai
 */
void *rrh_eNB_rx_thread(void *);
/*! \fn void *rrh_eNB_tx_thread(void *arg)
 * \brief this function
 * \param[in]
 * \return none
 * \note
 * @ingroup  _oai
 */
void *rrh_eNB_tx_thread(void *);
/*! \fn void *rrh_eNB_thread(void *arg)
 * \brief this function
 * \param[in]
 * \return none
 * \note
 * @ingroup  _oai
 */
void *rrh_eNB_thread(void *);
/*! \fn  void check_dev_config( rrh_module_t *mod_enb)
 * \brief this function
 * \param[in] *mod_enb
 * \return none
 * \note
 * @ingroup  _oai
 */
static void check_dev_config( rrh_module_t *mod_enb);
/*! \fn void calc_rt_period_ns( openair0_config_t openair0_cfg)
 * \brief this function
 * \param[in] openair0_cfg
 * \return none
 * \note
 * @ingroup  _oai
 */
static void calc_rt_period_ns( openair0_config_t *openair0_cfg);



void config_BBU_mod( rrh_module_t *mod_enb, uint8_t RT_flag, uint8_t NRT_flag) {
  
  int 	             error_code_eNB;
  pthread_t	     main_rrh_eNB_thread;
  pthread_attr_t     attr;
  struct sched_param sched_param_rrh;
  
  RT_flag_eNB=RT_flag;
  NRT_flag_eNB=NRT_flag;
  
  /* init socket and have handshake-like msg with client to exchange parameters */
  mod_enb->eth_dev.trx_start_func(&mod_enb->eth_dev);//change port  make it plus_id

  mod_enb->devs->openair0_cfg = mod_enb->eth_dev.openair0_cfg;

  /* check sanity of configuration parameters and print */
  check_dev_config(mod_enb);  
  if (rf_config_file[0] == '\0')  
    mod_enb->devs->openair0_cfg->configFilename = NULL;
  else
    mod_enb->devs->openair0_cfg->configFilename = rf_config_file;
  /* initialize and configure the RF device */
  if (openair0_device_load(mod_enb->devs, mod_enb->devs->openair0_cfg)<0) {
    LOG_E(RRH,"Exiting, cannot initialize RF device.\n");
    exit(-1);
  } else {   
    if (mod_enb->devs->type != NONE_DEV) {
      /* start RF device */
      if (mod_enb->devs->type == EXMIMO_DEV) {
	//call start function for exmino
      } else {

	if (mod_enb->devs->trx_start_func(mod_enb->devs)!=0)
	  LOG_E(RRH,"Unable to initiate RF device.\n");
	else
	  LOG_I(RRH,"RF device has been initiated.\n");
      }
      
    }
  }  
  
  /* create main eNB module thread
     main_rrh_eNB_thread allocates memory 
     for TX/RX buffers and creates TX/RX
     threads for every eNB module */ 
  pthread_attr_init(&attr);
  sched_param_rrh.sched_priority = sched_get_priority_max(SCHED_FIFO);
  pthread_attr_setschedparam(&attr,&sched_param_rrh);
  pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
  error_code_eNB = pthread_create(&main_rrh_eNB_thread, &attr, rrh_eNB_thread, (void *)mod_enb);

  if (error_code_eNB) {
    LOG_E(RRH,"Error while creating eNB thread\n");
    exit(-1);
  }
  
}


void *rrh_eNB_thread(void *arg) {

  rrh_module_t 	       	*dev=(rrh_module_t *)arg;
  pthread_t  	      	eNB_rx_thread, eNB_tx_thread;  
  int 	      		error_code_eNB_rx, error_code_eNB_tx;
  int32_t	        i,j;   		
  void 			*tmp;
  unsigned int          samples_per_frame=0;
  
  samples_per_frame = dev->eth_dev.openair0_cfg->samples_per_frame;    

  while (rrh_exit==0) {
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_TRX, 1 );    
    
    /* calculate packet period */
    calc_rt_period_ns(dev->eth_dev.openair0_cfg);
        
    /* allocate memory for TX/RX buffers
       each antenna port has a TX and a RX buffer
       each TX and RX buffer is of (samples_per_frame + HEADER_SIZE) samples (size of samples is 4 bytes) */
    rx_buffer_eNB = (int32_t**)malloc16(dev->eth_dev.openair0_cfg->rx_num_channels*sizeof(int32_t*));
    tx_buffer_eNB = (int32_t**)malloc16(dev->eth_dev.openair0_cfg->tx_num_channels*sizeof(int32_t*));    
    LOG_D(RRH,"rx_buffer_eNB address =%p tx_buffer_eNB address =%p  \n",rx_buffer_eNB,tx_buffer_eNB);
    
    /* rx_buffer_eNB points to the beginning of data */
    for (i=0; i<dev->eth_dev.openair0_cfg->rx_num_channels; i++) {
      tmp=(void *)malloc16(sizeof(int32_t)*(samples_per_frame + 32));
      memset(tmp,0,sizeof(int32_t)*(samples_per_frame + 32));
      rx_buffer_eNB[i]=( tmp + (32*sizeof(int32_t)) );  
      LOG_D(RRH,"i=%d rx_buffer_eNB[i]=%p tmp= %p\n",i,rx_buffer_eNB[i],tmp);
    }
    /* tx_buffer_eNB points to the beginning of data */
    for (i=0; i<dev->eth_dev.openair0_cfg->tx_num_channels; i++) {
      tmp=(void *)malloc16(sizeof(int32_t)*(samples_per_frame + 32));
      memset(tmp,0,sizeof(int32_t)*(samples_per_frame + 32));
      tx_buffer_eNB[i]=( tmp + (32*sizeof(int32_t)) );  
      LOG_D(RRH,"i= %d tx_buffer_eNB[i]=%p tmp= %p \n",i,tx_buffer_eNB[i],tmp);
    }
    /* dummy initialization for TX/RX buffers */
    for (i=0; i<dev->eth_dev.openair0_cfg->rx_num_channels; i++) {
      for (j=0; j<samples_per_frame; j++) {
	rx_buffer_eNB[i][j]=32+i; 
      } 
    }
    /* dummy initialization for TX/RX buffers */
    for (i=0; i<dev->eth_dev.openair0_cfg->tx_num_channels; i++) {
      for (j=0; j<samples_per_frame; j++) {
	tx_buffer_eNB[i][j]=12+i; 
      } 
    }    
    /* allocate TX/RX buffers pointers used in write/read operations */
    rx_eNB = (void**)malloc16(dev->eth_dev.openair0_cfg->rx_num_channels*sizeof(int32_t*));
    tx_eNB = (void**)malloc16(dev->eth_dev.openair0_cfg->tx_num_channels*sizeof(int32_t*));

    /* init mutexes */    
    for (i=0; i<dev->eth_dev.openair0_cfg->tx_num_channels; i++) {
      pthread_mutex_init(&sync_eNB_mutex[i],NULL);
      pthread_cond_init(&sync_eNB_cond[i],NULL);
    }
    /* init mutexes */    
    pthread_mutex_init(&sync_trx_mutex,NULL);

    /* create eNB module's TX/RX threads */    
#ifdef DEADLINE_SCHEDULER
    error_code_eNB_rx = pthread_create(&eNB_rx_thread, NULL, rrh_eNB_rx_thread, (void *)dev);
    error_code_eNB_tx = pthread_create(&eNB_tx_thread, NULL, rrh_eNB_tx_thread, (void *)dev);
    LOG_I(RRH,"[eNB][SCHED] deadline scheduling applied to eNB TX/RX threads\n");	
#else
    pthread_attr_t	attr_eNB_rx, attr_eNB_tx;
    struct sched_param 	sched_param_eNB_rx, sched_param_eNB_tx;

    pthread_attr_init(&attr_eNB_rx);
    pthread_attr_init(&attr_eNB_tx);	
    sched_param_eNB_rx.sched_priority = sched_get_priority_max(SCHED_FIFO);
    sched_param_eNB_tx.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&attr_eNB_rx,&sched_param_eNB_rx);
    pthread_attr_setschedparam(&attr_eNB_tx,&sched_param_eNB_tx);
    pthread_attr_setschedpolicy(&attr_eNB_rx,SCHED_FIFO);
    pthread_attr_setschedpolicy(&attr_eNB_tx,SCHED_FIFO);
    
    error_code_eNB_rx = pthread_create(&eNB_rx_thread, &attr_eNB_rx, rrh_eNB_rx_thread, (void *)dev);
    error_code_eNB_tx = pthread_create(&eNB_tx_thread, &attr_eNB_tx, rrh_eNB_tx_thread, (void *)dev);
    LOG_I(RRH,"[eNB][SCHED] FIFO scheduling applied to eNB TX/RX threads\n");		
#endif

    if (error_code_eNB_rx) {
      LOG_E(RRH,"[eNB] Error while creating eNB RX thread\n");
      exit(-1);
    }
    if (error_code_eNB_tx) {
      LOG_E(RRH,"[eNB] Error while creating eNB TX thread\n");
      exit(-1);
    }
   
    /* create timer thread; when no RF device is present a software clock is generated */    
    if (dev->devs->type == NONE_DEV) {

      int 			error_code_timer;
      pthread_t 		main_timer_proc_thread;
      
      LOG_I(RRH,"Creating timer thread with rt period  %d ns.\n",rt_period);
      
      /* setup the timer to generate an interrupt:
	 -for the first time in (sample_per_packet/sample_rate) ns
	 -and then every (sample_per_packet/sample_rate) ns */
      timerspec.it_value.tv_sec =     rt_period/1000000000;
      timerspec.it_value.tv_nsec =    rt_period%1000000000;
      timerspec.it_interval.tv_sec =  rt_period/1000000000;
      timerspec.it_interval.tv_nsec = rt_period%1000000000;
      
      
#ifdef DEADLINE_SCHEDULER
      error_code_timer = pthread_create(&main_timer_proc_thread, NULL, timer_proc, (void *)&timerspec);
      LOG_I(RRH,"[eNB][SCHED] deadline scheduling applied to timer thread \n");
#else 
      pthread_attr_t attr_timer;
      struct sched_param sched_param_timer;
      
      pthread_attr_init(&attr_timer);
      sched_param_timer.sched_priority = sched_get_priority_max(SCHED_FIFO-1);
      pthread_attr_setschedparam(&attr_timer,&sched_param_timer);
      pthread_attr_setschedpolicy(&attr_timer,SCHED_FIFO-1);
      
      pthread_mutex_init(&timer_mutex,NULL);
      
      error_code_timer = pthread_create(&main_timer_proc_thread, &attr_timer, timer_proc, (void *)&timerspec);
      LOG_I(RRH,"[eNB][SCHED] FIFO scheduling applied to timer thread \n");   	
#endif	
      
      if (error_code_timer) {
	LOG_E(RRH,"Error while creating timer proc thread\n");
	exit(-1);
      }
      
    }
    
    while (rrh_exit==0)
      sleep(1);
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_TRX,0 );
  }  
  
  rrh_eNB_thread_status = 0;
  pthread_exit(&rrh_eNB_thread_status);
  return(0);
}

/* Receive from RF and transmit to RRH */

void *rrh_eNB_rx_thread(void *arg) {

  /* measurement related vars */
  struct timespec time0,time1,time2;
  unsigned long long max_rx_time=0, min_rx_time=rt_period, total_rx_time=0, average_rx_time=rt_period, s_period=0, trial=0;
  int trace_cnt=0;

  struct timespec time_req_1us, time_rem_1us;
  rrh_module_t *dev = (rrh_module_t *)arg;
  ssize_t bytes_sent;
  int i=0 ,pck_rx=0, s_cnt=0;
  openair0_timestamp last_hw_counter=0;  //volatile int64_t
  unsigned int samples_per_frame=0,samples_per_subframe=0, spp_rf=0, spp_eth=0;
  uint8_t loopback=0,measurements=0;
  unsigned int subframe=0;
  unsigned int frame=0;

  time_req_1us.tv_sec = 0;
  time_req_1us.tv_nsec =1000;  //time_req_1us.tv_nsec = (int)rt_period/2;--->granularity issue
  spp_eth =  dev->eth_dev.openair0_cfg->samples_per_packet;
  spp_rf  =  dev->devs->openair0_cfg->samples_per_packet;

  samples_per_frame = dev->eth_dev.openair0_cfg->samples_per_frame;
  samples_per_subframe = (unsigned int)samples_per_frame/10;
  loopback = dev->loopback;
  measurements = dev->measurements;
  next_rx_pos = spp_eth;

#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;
  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  = (0.8 * 100) * 10000;//4 * 10000;
  attr.sched_deadline = (0.9 * 100) * 10000;//rt_period-2000;
  attr.sched_period   = 1 * 1000000;//rt_period;
  
  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] eNB RX thread: sched_setattr failed (run with sudo)\n");
    exit(-1);
  }
#endif

  while (rrh_exit == 0) {    
    while (rx_pos <(1 + subframe)*samples_per_subframe) {
      //LOG_D(RRH,"starting a new send:%d  %d\n",sync_trx,frame);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_RX, 1 );
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_HW_FRAME_RX, frame);
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_HW_SUBFRAME_RX, subframe );  
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RX_PCK, pck_rx );
      LOG_D(RRH,"pack=%d    rx_pos=%d    subframe=%d frame=%d\n ",pck_rx, rx_pos, subframe,frame);
      
      if (dev->devs->type == NONE_DEV) {	
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RX_HWCNT, hw_counter );
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RX_LHWCNT, last_hw_counter );
	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_CNT, s_cnt );
	if (!eNB_rx_started) {
	  eNB_rx_started=1; // set this flag to 1 to indicate that eNB started
	  if (RT_flag_eNB==1) {
	    last_hw_counter=hw_counter;//get current counter
	  }
	} else {
	  if (RT_flag_eNB==1) {
	    if (hw_counter > last_hw_counter+1) {
	      printf("LR");
	    } else {
	      while ((hw_counter < last_hw_counter+1)) {
		VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_RX_SLEEP, 1 );
		nanosleep(&time_req_1us,&time_rem_1us);	//rt_sleep_ns(sleep_ns);
		s_cnt++;	
		VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_RX_SLEEP, 0 );
	      } 
	    }
	  }
	}
      }
      

      if (measurements == 1 ) clock_gettime(CLOCK_MONOTONIC,&time1);      
    
      if (loopback == 1 ) {
	if (sync_eNB_rx[i]==0) {
	  rx_eNB[i] = (void*)&tx_buffer_eNB[i][tx_pos];
	  LOG_I(RRH,"tx_buffer_eNB[i][tx_pos]=%d ,tx_pos=%d\n",tx_buffer_eNB[i][tx_pos],tx_pos);			
	} else {
	  rx_eNB[i] = (void*)&rx_buffer_eNB[i][rx_pos];
	  LOG_I(RRH,"rx_buffer_eNB[i][rx_pos]=%d ,rx_pos=%d\n",rx_buffer_eNB[i][rx_pos],rx_pos);	
	}
       }
       
       for (i=0; i<dev->eth_dev.openair0_cfg->rx_num_channels; i++) {
	 rx_eNB[i] = (void*)&rx_buffer_eNB[i][rx_pos];
	 LOG_D(RRH," rx_eNB[i]=%p rx_buffer_eNB[i][rx_pos]=%p ,rx_pos=%d, i=%d ts=%d\n",rx_eNB[i],&rx_buffer_eNB[i][rx_pos],rx_pos,i,timestamp_rx);	 
       }  
       VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RXCNT, rx_pos );
       if (dev->devs->type != NONE_DEV) {
	 VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ_RF, 1 ); 
	 /* Read operation to RF device (RX)*/
	 if ( dev->devs->trx_read_func (dev->devs,
					&timestamp_rx,
					rx_eNB,
					spp_rf,
					dev->devs->openair0_cfg->rx_num_channels
					)<0) {
	   perror("RRH eNB : USRP read");
	 }
	 VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ_RF, 0 );	
       }
       VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RX_TS, timestamp_rx&0xffffffff );

       VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 ); 
       if ((bytes_sent = dev->eth_dev.trx_write_func (&dev->eth_dev,
						      timestamp_rx,
						      rx_eNB,
						      spp_eth,
						      dev->eth_dev.openair0_cfg->rx_num_channels,
						      0))<0) {
	 perror("RRH eNB : ETHERNET write");
       }    
       VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );

       /* when there is no RF timestamp is updated by number of samples */
       if (dev->devs->type == NONE_DEV) {
	 timestamp_rx+=spp_eth;
	 last_hw_counter=hw_counter;
       }
       
       if (measurements == 1 ) {

	 clock_gettime(CLOCK_MONOTONIC,&time2);
	 
	 if (trace_cnt++ > 10) {
	   total_rx_time = (unsigned int)(time2.tv_nsec - time0.tv_nsec);
	   if (total_rx_time < 0)
	     total_rx_time=1000000000-total_rx_time;
	   
	   if ((total_rx_time > 0) && (total_rx_time < 1000000000)) {
	     trial++;
	     if (total_rx_time < min_rx_time)
	       min_rx_time = total_rx_time;
	     if (total_rx_time > max_rx_time){
	       max_rx_time = total_rx_time;
	       LOG_I(RRH,"Max value %d update at rx_position %d \n",total_rx_time,timestamp_rx);
	     }
	     average_rx_time = (long long unsigned int)((average_rx_time*trial)+total_rx_time)/(trial+1);
	   }
	   if (s_period++ == PRINTF_PERIOD) {
	     s_period=0;
	     LOG_I(RRH,"Average eNB RX time : %lu ns\tMax RX time : %lu ns\tMin RXX time : %lu ns\n",average_rx_time,max_rx_time,min_rx_time);
	   }
	 }
	 
	 memcpy(&time0,&time2,sizeof(struct timespec));
       }
       
       if (loopback == 1 ) {
	 pthread_mutex_lock(&sync_eNB_mutex[i]);
	 sync_eNB_rx[i]--;
	 pthread_mutex_unlock(&sync_eNB_mutex[i]);
       }
       
       rx_pos += spp_eth;    
       pck_rx++;
       next_rx_pos=(rx_pos+spp_eth);
       
       VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_RX, 0 );
       /*
       if (frame>50) {
	 pthread_mutex_lock(&sync_trx_mutex);
	 while (sync_trx) {
	   pthread_cond_wait(&sync_trx_cond,&sync_trx_mutex);
	 }
	 sync_trx=1;
	 LOG_D(RRH,"out of while send:%d  %d\n",sync_trx,frame);
	 pthread_cond_signal(&sync_trx_cond);
	 pthread_mutex_unlock(&sync_trx_mutex);
	 }*/
    } // while 
    
    subframe++;
    s_cnt=0;
    
    /* wrap around rx buffer index */
    if (next_rx_pos >= samples_per_frame)
      next_rx_pos -= samples_per_frame;  
    if (rx_pos >= samples_per_frame)
      rx_pos -= samples_per_frame;
    /* wrap around subframe number */       
    if (subframe == 10 ) {
      subframe = 0; 
      frame++;
    }   
   
    
  }  //while (eNB_exit==0)
  return 0;
}

/* Receive from eNB and transmit to RF */

void *rrh_eNB_tx_thread(void *arg) {

  struct timespec time0,time1,time2;

  rrh_module_t *dev = (rrh_module_t *)arg;
  struct timespec time_req_1us, time_rem_1us;
  ssize_t bytes_received;
  int i;
  //openair0_timestamp last_hw_counter=0;
  unsigned int samples_per_frame=0,samples_per_subframe=0;
  unsigned int  spp_rf=0, spp_eth=0;
  uint8_t loopback=0,measurements=0;
  unsigned int subframe=0,frame=0;
  unsigned int pck_tx=0;
  
#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;
  unsigned int flags = 0;
  
  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;
  
  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  = (0.8 * 100) * 10000;
  attr.sched_deadline = (0.9 * 100) * 10000;
  attr.sched_period   = 1 * 1000000;
  
  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] eNB TX thread: sched_setattr failed\n");
    exit(-1);
  }
#endif	
  
  time_req_1us.tv_sec = 1;
  time_req_1us.tv_nsec = 0;
  spp_eth = dev->eth_dev.openair0_cfg->samples_per_packet;
  spp_rf =  dev->devs->openair0_cfg->samples_per_packet;
  samples_per_frame = dev->eth_dev.openair0_cfg->samples_per_frame;
  samples_per_subframe = (unsigned int)samples_per_frame/10;
  tx_pos=0;

  loopback = dev->loopback;
  measurements = dev->measurements;
  
  while (rrh_exit == 0) {     
    while (tx_pos < (1 + subframe)*samples_per_subframe) {
      
      //LOG_D(RRH,"bef lock read:%d  %d\n",sync_trx,frame);
      //pthread_mutex_lock(&sync_trx_mutex);
      
      //while (!sync_trx) {
      //LOG_D(RRH,"in sync read:%d  %d\n",sync_trx,frame);
      //pthread_cond_wait(&sync_trx_cond,&sync_trx_mutex);
      //}
      //LOG_D(RRH,"out of while read:%d  %d\n",sync_trx,frame);
      
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_TX, 1 );
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_HW_FRAME, frame);
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_HW_SUBFRAME, subframe );
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TX_PCK, pck_tx );
          
      if (measurements == 1 ) 	clock_gettime(CLOCK_MONOTONIC,&time1); 
      for (i=0; i<dev->eth_dev.openair0_cfg->tx_num_channels; i++) tx_eNB[i] = (void*)&tx_buffer_eNB[i][tx_pos];		
      
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TXCNT, tx_pos );
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );
      
      /* Read operation to ETHERNET device */
      if (( bytes_received = dev->eth_dev.trx_read_func(&dev->eth_dev,
							&timestamp_tx,
							tx_eNB,
							spp_eth,
							dev->eth_dev.openair0_cfg->tx_num_channels))<0) {
	perror("RRH eNB : ETHERNET read");
      }		
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );	
      
      if (dev->devs->type != NONE_DEV) {  
	LOG_D(RRH," tx_buffer_eNB[i][tx_pos]=%x t_buffer_eNB[i][tx_pos+1]=%x t_buffer_eNB[i][tx_pos+2]=%x \n",tx_buffer_eNB[0][tx_pos],tx_buffer_eNB[0][tx_pos+1],tx_buffer_eNB[0][tx_pos+2]);	 
	VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE_RF, 1 );    
	/* Write operation to RF device (TX)*/
	if ( dev->devs->trx_write_func (dev->devs,
					timestamp_tx,
					tx_eNB,
					spp_rf,
					dev->devs->openair0_cfg->tx_num_channels,
					1)<0){
	  perror("RRH eNB : USRP write");
	}
	VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE_RF, 0 );
      }
      
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TX_TS, timestamp_tx&0xffffffff ); 
      
            
      //if (dev->devs->type == NONE_DEV)	last_hw_counter=hw_counter;
    
    
      if (loopback ==1 ) { 
	while (sync_eNB_rx[i]==0)
	  nanosleep(&time_req_1us,&time_rem_1us);
	
	pthread_mutex_lock(&sync_eNB_mutex[i]);
	sync_eNB_rx[i]++;
	pthread_mutex_unlock(&sync_eNB_mutex[i]);
      }      
      
      if (measurements == 1 ) {
	clock_gettime(CLOCK_MONOTONIC,&time2);
	memcpy(&time0,&time2,sizeof(struct timespec));
      }   
      
      prev_tx_pos=tx_pos;
      tx_pos += spp_eth;
      pck_tx++;   
      
      //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_TX, 0 );
      //sync_trx=0;
      //pthread_cond_signal(&sync_trx_cond);
      //pthread_mutex_unlock(&sync_trx_mutex);
    }

    /* wrap around tx buffer index */
    if (tx_pos >= samples_per_frame)
      tx_pos -= samples_per_frame;    
    /* wrap around subframe number */ 
    subframe++;
    if (subframe == 10 ) {
      subframe = 0; // the radio frame is complete, start over
      frame++;
    }
    
  } //while (eNB_exit==0)   
  return 0;
}


static void calc_rt_period_ns( openair0_config_t *openair0_cfg) {

  rt_period= (double)(openair0_cfg->samples_per_packet/(openair0_cfg->samples_per_frame/10.0)*1000000);
  AssertFatal(rt_period > 0, "Invalid rt period !%u\n", rt_period);
  LOG_I(RRH,"[eNB] Real time period is set to %u ns\n", rt_period);	
}


static void check_dev_config( rrh_module_t *mod_enb) {
    
 AssertFatal( (mod_enb->devs->openair0_cfg->num_rb_dl==100 || mod_enb->devs->openair0_cfg->num_rb_dl==50 || mod_enb->devs->openair0_cfg->num_rb_dl==25 || mod_enb->devs->openair0_cfg->num_rb_dl==6) , "Invalid number of resource blocks! %d\n", mod_enb->devs->openair0_cfg->num_rb_dl);
 AssertFatal( mod_enb->devs->openair0_cfg->samples_per_frame  > 0 ,  "Invalid number of samples per frame! %d\n",mod_enb->devs->openair0_cfg->samples_per_frame); 
 AssertFatal( mod_enb->devs->openair0_cfg->sample_rate        > 0.0, "Invalid sample rate! %f\n", mod_enb->devs->openair0_cfg->sample_rate);
 AssertFatal( mod_enb->devs->openair0_cfg->samples_per_packet > 0 ,  "Invalid number of samples per packet! %d\n",mod_enb->devs->openair0_cfg->samples_per_packet);
 AssertFatal( mod_enb->devs->openair0_cfg->rx_num_channels    > 0 ,  "Invalid number of RX antennas! %d\n", mod_enb->devs->openair0_cfg->rx_num_channels); 
 AssertFatal( mod_enb->devs->openair0_cfg->tx_num_channels    > 0 ,  "Invalid number of TX antennas! %d\n", mod_enb->devs->openair0_cfg->tx_num_channels);
 AssertFatal( mod_enb->devs->openair0_cfg->rx_freq[0]         > 0.0 ,"Invalid RX frequency! %f\n", mod_enb->devs->openair0_cfg->rx_freq[0]); 
 AssertFatal( mod_enb->devs->openair0_cfg->tx_freq[0]         > 0.0 ,"Invalid TX frequency! %f\n", mod_enb->devs->openair0_cfg->tx_freq[0]);
 AssertFatal( mod_enb->devs->openair0_cfg->rx_gain[0]         > 0.0 ,"Invalid RX gain! %f\n", mod_enb->devs->openair0_cfg->rx_gain[0]); 
 AssertFatal( mod_enb->devs->openair0_cfg->tx_gain[0]         > 0.0 ,"Invalid TX gain! %f\n", mod_enb->devs->openair0_cfg->tx_gain[0]);
 AssertFatal( mod_enb->devs->openair0_cfg->rx_bw              > 0.0 ,"Invalid RX bw! %f\n", mod_enb->devs->openair0_cfg->rx_bw); 
 AssertFatal( mod_enb->devs->openair0_cfg->tx_bw              > 0.0 ,"Invalid RX bw! %f\n", mod_enb->devs->openair0_cfg->tx_bw);
 AssertFatal( mod_enb->devs->openair0_cfg->autocal[0]         > 0 ,  "Invalid auto calibration choice! %d\n", mod_enb->devs->openair0_cfg->autocal[0]);
 
 printf("\n---------------------RF device configuration parameters---------------------\n");
 
 printf("\tMod_id=%d\n \tlog level=%d\n \tDL_RB=%d\n \tsamples_per_frame=%d\n \tsample_rate=%f\n \tsamples_per_packet=%d\n \ttx_sample_advance=%d\n \trx_num_channels=%d\n \ttx_num_channels=%d\n \trx_freq_0=%f\n \ttx_freq_0=%f\n \trx_freq_1=%f\n \ttx_freq_1=%f\n \trx_freq_2=%f\n \ttx_freq_2=%f\n \trx_freq_3=%f\n \ttx_freq_3=%f\n \trxg_mode=%d\n \trx_gain_0=%f\n \ttx_gain_0=%f\n  \trx_gain_1=%f\n \ttx_gain_1=%f\n  \trx_gain_2=%f\n \ttx_gain_2=%f\n  \trx_gain_3=%f\n \ttx_gain_3=%f\n \trx_gain_offset_2=%f\n \ttx_gain_offset_3=%f\n  \trx_bw=%f\n \ttx_bw=%f\n \tautocal=%d\n",	
	mod_enb->devs->openair0_cfg->Mod_id,
	mod_enb->devs->openair0_cfg->log_level,
	mod_enb->devs->openair0_cfg->num_rb_dl,
	mod_enb->devs->openair0_cfg->samples_per_frame,
	mod_enb->devs->openair0_cfg->sample_rate,
	mod_enb->devs->openair0_cfg->samples_per_packet,
	mod_enb->devs->openair0_cfg->tx_sample_advance,
	mod_enb->devs->openair0_cfg->rx_num_channels,
	mod_enb->devs->openair0_cfg->tx_num_channels,
	mod_enb->devs->openair0_cfg->rx_freq[0],
	mod_enb->devs->openair0_cfg->tx_freq[0],
	mod_enb->devs->openair0_cfg->rx_freq[1],
	mod_enb->devs->openair0_cfg->tx_freq[1],
	mod_enb->devs->openair0_cfg->rx_freq[2],
	mod_enb->devs->openair0_cfg->tx_freq[2],
	mod_enb->devs->openair0_cfg->rx_freq[3],
	mod_enb->devs->openair0_cfg->tx_freq[3],
	mod_enb->devs->openair0_cfg->rxg_mode[0],
	mod_enb->devs->openair0_cfg->tx_gain[0],
	mod_enb->devs->openair0_cfg->tx_gain[0],
	mod_enb->devs->openair0_cfg->rx_gain[1],
	mod_enb->devs->openair0_cfg->tx_gain[1],
	mod_enb->devs->openair0_cfg->rx_gain[2],
	mod_enb->devs->openair0_cfg->tx_gain[2],
	mod_enb->devs->openair0_cfg->rx_gain[3],
	mod_enb->devs->openair0_cfg->tx_gain[3],
	//mod_enb->devs->openair0_cfg->rx_gain_offset[0],
	//mod_enb->devs->openair0_cfg->rx_gain_offset[1],
	mod_enb->devs->openair0_cfg->rx_gain_offset[2],
	mod_enb->devs->openair0_cfg->rx_gain_offset[3],
	mod_enb->devs->openair0_cfg->rx_bw,
	mod_enb->devs->openair0_cfg->tx_bw,
	mod_enb->devs->openair0_cfg->autocal[0]  
	);
 
 printf("----------------------------------------------------------------------------\n");
 
 
}
