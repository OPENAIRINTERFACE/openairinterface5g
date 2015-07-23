/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

   Contact Information
   OpenAirInterface Admin: openair_admin@eurecom.fr
   OpenAirInterface Tech : openair_tech@eurecom.fr
   OpenAirInterface Dev  : openair4g-devel@eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*! \file eNB_transport_IQ.c
 * \brief eNB transport IQ sampels 
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
//#undef LOWLATENCY
/******************************************************************************
 **                               FUNCTION PROTOTYPES                        **
 ******************************************************************************/
void *rrh_eNB_rx_thread(void *);
void *rrh_eNB_tx_thread(void *);
void *rrh_proc_eNB_thread(void *);
void *rrh_eNB_thread(void *);
void set_rt_period( openair0_config_t openair0_cfg);
void check_dev_config( rrh_module_t *mod_enb);


pthread_t	   main_rrh_eNB_thread;
pthread_attr_t     attr, attr_proc;
struct sched_param sched_param_rrh;

pthread_cond_t          sync_eNB_cond[4];
pthread_mutex_t         sync_eNB_mutex[4];
openair0_timestamp 	nrt_eNB_counter[4]= {0,0,0,0};
int32_t 	overflow_rx_buffer_eNB[4]= {0,0,0,0};
int32_t 	nsamps_eNB[4]= {0,0,0,0};
int32_t 	eNB_tx_started=0,eNB_rx_started=0;
int32_t 	counter_eNB_rx[4]= {0,0,0,0};
int32_t 	counter_eNB_tx[4]= {0,0,0,0};

uint8_t		RT_flag_eNB,NRT_flag_eNB;
int32_t		**tx_buffer_eNB, **rx_buffer_eNB;
void 		*rrh_eNB_thread_status;
void 		**rx_eNB; //was fixed to 2 ant
void 		**tx_eNB; //was fixed to 2 ant

int 	        	sync_eNB_rx[4]= {-1,-1,-1,-1};
openair0_timestamp	timestamp_eNB_tx[4]= {0,0,0,0},timestamp_eNB_rx[4]= {0,0,0,0};

unsigned int  rx_pos=0, next_rx_pos=0;
unsigned int  tx_pos=0, prev_tx_pos=0;


/*! \fn void create_eNB_trx_threads( rrh_module_t *dev_enb, uint8_t RT_flag,uint8_t NRT_flag)
 * \brief this function
 * \param[in]
 * \param[out]
 * \return
 * \note
 * @ingroup  _oai
 */
void create_eNB_trx_threads( rrh_module_t *mod_enb, uint8_t RT_flag, uint8_t NRT_flag){
  
  //int 	i;
  int 	error_code_eNB;
  
  RT_flag_eNB=RT_flag;
  NRT_flag_eNB=NRT_flag;
  
  pthread_attr_init(&attr);
  sched_param_rrh.sched_priority = sched_get_priority_max(SCHED_FIFO);
  pthread_attr_setschedparam(&attr,&sched_param_rrh);
  pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
  /*for (i=0; i<4; i++) {
    pthread_mutex_init(&sync_eNB_mutex[i],NULL);
    pthread_cond_init(&sync_eNB_cond[i],NULL);
    }*/
  
  /* handshake with client to exchange parameters */
  mod_enb->eth_dev.trx_start_func(&mod_enb->eth_dev);//change port  make it plus_id
  
  memcpy((void*)&mod_enb->devs->openair0_cfg,(void *)&mod_enb->eth_dev.openair0_cfg,sizeof(openair0_config_t));
  
  /* update certain parameters */
  if ( mod_enb->devs->type == EXMIMO_IF ) {
    if ( mod_enb->devs->openair0_cfg.num_rb_dl == 100 ) {
      mod_enb->devs->openair0_cfg.samples_per_packet = 2048;
      mod_enb->devs->openair0_cfg.tx_forward_nsamps = 175;
      mod_enb->devs->openair0_cfg.tx_delay = 8;
    }
    else if( mod_enb->devs->openair0_cfg.num_rb_dl == 50 ){
      mod_enb->devs->openair0_cfg.samples_per_packet = 2048;
      mod_enb->devs->openair0_cfg.tx_forward_nsamps = 95;
      mod_enb->devs->openair0_cfg.tx_delay = 5;
    }
    else if( mod_enb->devs->openair0_cfg.num_rb_dl == 25 ){
      mod_enb->devs->openair0_cfg.samples_per_packet = 1024;
      mod_enb->devs->openair0_cfg.tx_forward_nsamps = 70;
      mod_enb->devs->openair0_cfg.tx_delay = 6;
    }
    else if( mod_enb->devs->openair0_cfg.num_rb_dl == 6 ){
      mod_enb->devs->openair0_cfg.samples_per_packet = 256;
      mod_enb->devs->openair0_cfg.tx_forward_nsamps = 40;
      mod_enb->devs->openair0_cfg.tx_delay = 8;
    }
  }
  else if (mod_enb->devs->type == USRP_IF) {
    if ( mod_enb->devs->openair0_cfg.num_rb_dl == 100 ) {
      mod_enb->devs->openair0_cfg.samples_per_packet = 2048;
      mod_enb->devs->openair0_cfg.tx_forward_nsamps = 175;
      mod_enb->devs->openair0_cfg.tx_delay = 8;
    }
    else if( mod_enb->devs->openair0_cfg.num_rb_dl == 50 ) {
      mod_enb->devs->openair0_cfg.samples_per_packet = 2048;
      mod_enb->devs->openair0_cfg.tx_forward_nsamps = 95;
      mod_enb->devs->openair0_cfg.tx_delay = 5;
    }
    else if( mod_enb->devs->openair0_cfg.num_rb_dl == 25 ) {
      mod_enb->devs->openair0_cfg.samples_per_packet = 1024;
      mod_enb->devs->openair0_cfg.tx_forward_nsamps = 70;
      mod_enb->devs->openair0_cfg.tx_delay = 6;
    }
  else if( mod_enb->devs->openair0_cfg.num_rb_dl == 6 ) {
    mod_enb->devs->openair0_cfg.samples_per_packet = 256;
    mod_enb->devs->openair0_cfg.tx_forward_nsamps = 40;
    mod_enb->devs->openair0_cfg.tx_delay = 8;
  }
  }
  else if (mod_enb->devs->type == BLADERF_IF) {
    if ( mod_enb->devs->openair0_cfg.num_rb_dl == 100 ) {
      mod_enb->devs->openair0_cfg.samples_per_packet = 2048;
      mod_enb->devs->openair0_cfg.tx_forward_nsamps = 175;
      mod_enb->devs->openair0_cfg.tx_delay = 8;
   }
    else if( mod_enb->devs->openair0_cfg.num_rb_dl == 50 ){
      mod_enb->devs->openair0_cfg.samples_per_packet = 2048;
      mod_enb->devs->openair0_cfg.tx_forward_nsamps = 95;
      mod_enb->devs->openair0_cfg.tx_delay = 5;
    }
    else if( mod_enb->devs->openair0_cfg.num_rb_dl == 25 ){
      mod_enb->devs->openair0_cfg.samples_per_packet = 1024;
      mod_enb->devs->openair0_cfg.tx_forward_nsamps = 70;
      mod_enb->devs->openair0_cfg.tx_delay = 6;
    }
    else if( mod_enb->devs->openair0_cfg.num_rb_dl == 6 ){
      mod_enb->devs->openair0_cfg.samples_per_packet = 256;
     mod_enb->devs->openair0_cfg.tx_forward_nsamps = 40;
     mod_enb->devs->openair0_cfg.tx_delay = 8;
    }           
 }  
  
  /* check sanity of received configuration parameters and print */
  check_dev_config(mod_enb);
  
#ifndef ETHERNET
    /* initialize and apply configuration to associated RF device */
    if (openair0_device_init(mod_enb->devs, &mod_enb->devs->openair0_cfg)<0){
    LOG_E(RRH,"Exiting, cannot initialize RF device.\n");
    exit(-1);
    }
    else {
      LOG_I(RRH,"RF device has been successfully initialized.\n");
    }
    
#endif
  
  error_code_eNB = pthread_create(&main_rrh_eNB_thread, &attr, rrh_eNB_thread, (void *)mod_enb);
  if (error_code_eNB) {
    LOG_E(RRH,"Error while creating eNB thread\n");
    exit(-1);
  }
  
}

/*! \fn void *rrh_eNB_thread(void *arg)
 * \brief this function
 * \param[in]
 * \param[out]
 * \return
 * \note
 * @ingroup  _oai
 */
void *rrh_eNB_thread(void *arg)
{
  rrh_module_t 	       	*dev=(rrh_module_t *)arg;
  pthread_t  	      	eNB_rx_thread, eNB_tx_thread;  
  int 	      		error_code_eNB_rx, error_code_eNB_tx;
  int32_t	        i,j;   		
  void 			*tmp;
  unsigned int          samples_per_frame=0;
  
  
  while (rrh_exit==0) {
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_TRX, 1 );
    
    
    if (dev->devs->type != NONE_IF) {
      set_rt_period(dev->eth_dev.openair0_cfg);
    }
    samples_per_frame = dev->eth_dev.openair0_cfg.samples_per_frame;    
    
    /* allocate memory for TX/RX buffers
       each antenna port has a TX and a RX buffer
       each TX and RX buffer is of (samples_per_frame + HEADER_SIZE) samples (size of samples is 4 bytes) */
    rx_buffer_eNB = (int32_t**)malloc16(dev->eth_dev.openair0_cfg.rx_num_channels*sizeof(int32_t*));
    tx_buffer_eNB = (int32_t**)malloc16(dev->eth_dev.openair0_cfg.tx_num_channels*sizeof(int32_t*));
    
    LOG_I(RRH,"rx ch %d %p and tx ch %d %p\n", 
	  dev->eth_dev.openair0_cfg.rx_num_channels,
	  rx_buffer_eNB,
	  dev->eth_dev.openair0_cfg.tx_num_channels,
	  tx_buffer_eNB);
    
    /* rx_buffer_eNB points to the beginning of data */
    for (i=0; i<dev->eth_dev.openair0_cfg.rx_num_channels; i++) {
      tmp=(void *)malloc(sizeof(int32_t)*(samples_per_frame + HEADER_SIZE));
      memset(tmp,0,sizeof(int32_t)*(samples_per_frame + HEADER_SIZE));
      rx_buffer_eNB[i]=( tmp + (HEADER_SIZE*sizeof(int32_t)) );  
      LOG_I(RRH," rx ch %d %p |%p\n",i,rx_buffer_eNB[i],tmp);
    }
    /* tx_buffer_eNB points to the beginning of data */
    for (i=0; i<dev->eth_dev.openair0_cfg.tx_num_channels; i++) {
      tmp=(void *)malloc(sizeof(int32_t)*(samples_per_frame + HEADER_SIZE));
      memset(tmp,0,sizeof(int32_t)*(samples_per_frame + HEADER_SIZE));
      tx_buffer_eNB[i]=( tmp + (HEADER_SIZE*sizeof(int32_t)) );  
      LOG_I(RRH," tx ch %d %p| %p \n", i,tx_buffer_eNB[i],tmp);
    }
    /* dummy initialization for TX/RX buffers */
    for (i=0; i<dev->eth_dev.openair0_cfg.rx_num_channels; i++) {
      for (j=0; j<samples_per_frame; j++) {
	rx_buffer_eNB[i][j]=32+i; 
      } 
    }
    /* dummy initialization for TX/RX buffers */
    for (i=0; i<dev->eth_dev.openair0_cfg.tx_num_channels; i++) {
      for (j=0; j<samples_per_frame; j++) {
	tx_buffer_eNB[i][j]=12+i; 
      } 
    }
    
  /* allocate TX/RX buffers pointers used in write/read operations */
    rx_eNB = (void**)malloc16(dev->eth_dev.openair0_cfg.rx_num_channels*sizeof(int32_t*));
    tx_eNB = (void**)malloc16(dev->eth_dev.openair0_cfg.tx_num_channels*sizeof(int32_t*));
    
    
#ifdef LOWLATENCY
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

    while (rrh_exit==0)
      sleep(1);
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_TRX,0 );
  }  //while (eNB_exit==0)

  rrh_eNB_thread_status = 0;
  pthread_exit(&rrh_eNB_thread_status);
  return(0);
}



/*! \fn void *rrh_eNB_rx_thread(void *arg)
 * \brief this function
 * \param[in]
 * \param[out]
 * \return
 * \note
 * @ingroup  _oai
 */
void *rrh_eNB_rx_thread(void *arg){

  /* measuremnt related vars */
  struct timespec time0,time1,time2;
  unsigned long long max_rx_time=0, min_rx_time=rt_period, total_rx_time=0, average_rx_time=rt_period, s_period=0, trial=0;
  int trace_cnt=0;

  struct timespec time_req_1us, time_rem_1us;
  rrh_module_t *dev = (rrh_module_t *)arg;
  ssize_t bytes_sent;
  int i, spp ,pck_rx=0;
  openair0_vtimestamp last_hw_counter=0;  //volatile int64_t
  unsigned int samples_per_frame=0;
  uint8_t loopback=0,measurements=0;

  //RTIME sleep_ns=1000;
  time_req_1us.tv_sec = 0;
  time_req_1us.tv_nsec =1000;  //time_req_1us.tv_nsec = (int)rt_period/2;--->granularity issue
  spp =  dev->eth_dev.openair0_cfg.samples_per_packet;
  samples_per_frame = dev->eth_dev.openair0_cfg.samples_per_frame;
  loopback = dev->loopback;
  measurements = dev->measurements;
  next_rx_pos = spp;

#ifdef LOWLATENCY
  struct sched_attr attr;
  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  = (0.1   *  100) * 10000; // 
  attr.sched_deadline = rt_period;// 0.1   *  1000000; // 
  attr.sched_period   = rt_period; //0.1   *  1000000; // each TX/RX thread has 

  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] eNB RX thread: sched_setattr failed (run with sudo)\n");
    exit(-1);
  }
#endif

  while (rrh_exit == 0) {

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_RX, 1 );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RX_HWCNT, hw_counter );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RX_LHWCNT, last_hw_counter );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RX_PCK, pck_rx );

    for (i=0; i<dev->eth_dev.openair0_cfg.rx_num_channels; i++){
      if (!eNB_rx_started) {
	eNB_rx_started=1; // set this flag to 1 to indicate that eNB started
	if (RT_flag_eNB==1) {
	  last_hw_counter=hw_counter;
	}
      } else {
	if (RT_flag_eNB==1) {
	  if (hw_counter > last_hw_counter+1) {
	    printf("LR");
	  } else {
	    while (hw_counter < last_hw_counter+1){
	      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_RX_SLEEP, 1 );
	      nanosleep(&time_req_1us,&time_rem_1us);
	      //rt_sleep_ns(sleep_ns);
	      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_RX_SLEEP, 0 );
	    } 
	  }
	}
      }

      if (measurements == 1 )
	clock_gettime(CLOCK_MONOTONIC,&time1);
      
      /* LOG_I(RRH,"send for%d at %d with %d |%d|%d| \n",i,rx_pos,timestamp_eNB_rx[i],((timestamp_eNB_rx[i]+spp)%samples_per_frame),next_rx_pos );
			
      if ((timestamp_UE_tx[i]%samples_per_frame < next_rx_pos) && (UE_tx_started==1)) {
	printf("eNB underflow\n");
	if (NRT_flag_eNB==1) {
	  while ((timestamp_UE_tx[i]%samples_per_frame) < spp)
          nanosleep(&time_req_1us,&time_rem_1us);
	}
      }	
      if (((rx_pos)< timestamp_UE_tx[i]%samples_per_frame) && (next_rx_pos > (timestamp_UE_tx[i]%samples_per_frame)) && (UE_tx_started==1)) {
	printf("eNB underflow\n");
	if (NRT_flag_eNB==1) {
	  while (next_rx_pos > (timestamp_UE_tx[i]%samples_per_frame))
          nanosleep(&time_req_1us,&time_rem_1us);
	}
	}*/

      if (loopback == 1 ) {
	if (sync_eNB_rx[i]==0) {
	  rx_eNB[i] = (void*)&tx_buffer_eNB[i][tx_pos];
	  LOG_I(RRH,"tx_buffer_eNB[i][tx_pos]=%d ,tx_pos=%d\n",tx_buffer_eNB[i][tx_pos],tx_pos);			
	}
	else{
	  rx_eNB[i] = (void*)&rx_buffer_eNB[i][rx_pos];
	  LOG_I(RRH,"rx_buffer_eNB[i][rx_pos]=%d ,rx_pos=%d\n",rx_buffer_eNB[i][rx_pos],rx_pos);	
	}
      }

      rx_eNB[i] = (void*)&rx_buffer_eNB[i][rx_pos];

      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RXCNT, rx_pos );
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_RX_TS, timestamp_eNB_rx[i]&0xffffffff );
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 ); 
      
      //LOG_D(RRH," rx_eNB[i]=%p rx_buffer_eNB[i][rx_pos]=%p ,rx_pos=%d, i=%d ts=%d\n",rx_eNB[i],&rx_buffer_eNB[i][rx_pos],rx_pos,i,timestamp_eNB_rx[i]);	 
      if ((bytes_sent = dev->eth_dev.trx_write_func (&dev->eth_dev,
						     timestamp_eNB_rx[i],
						     rx_eNB,
						     spp,
						     i,
						     0))<0){
	perror("RRH eNB : sendto for RX");
      }
    
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );

      timestamp_eNB_rx[i]+=spp;
      last_hw_counter=hw_counter;

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
	      LOG_I(RRH,"Max value %d update at rx_position %d \n",total_rx_time,timestamp_eNB_rx[i]);
	    }
	    average_rx_time = (long long unsigned int)((average_rx_time*trial)+total_rx_time)/(trial+1);
	  }
	  if (s_period++ == PRINTF_PERIOD) {
	    s_period=0;
	    LOG_I(RRH,"Average eNB RX time : %lu\tMax RX time : %lu\tMin RX time : %lu\n",average_rx_time,max_rx_time,min_rx_time);
	  }
	}
	
	memcpy(&time0,&time2,sizeof(struct timespec));
      }

      if (loopback == 1 ){
	pthread_mutex_lock(&sync_eNB_mutex[i]);
	sync_eNB_rx[i]--;
	pthread_mutex_unlock(&sync_eNB_mutex[i]);
      }
      
    }//for each antenna
    
    rx_pos += spp;    
    pck_rx++;
    next_rx_pos=(rx_pos+spp);

    if (next_rx_pos >= samples_per_frame)
      next_rx_pos -= samples_per_frame;  
    if (rx_pos >= samples_per_frame)
      rx_pos -= samples_per_frame;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_RX, 0 );
  }  //while (eNB_exit==0)
  return(0);
}


/*! \fn void *rrh_eNB_tx_thread(void *arg)
 * \brief this function
 * \param[in]
 * \param[out]
 * \return
 * \note
 * @ingroup  _oai
 */
void *rrh_eNB_tx_thread(void *arg){

  struct timespec time0a,time0,time1,time2;

  rrh_module_t *dev = (rrh_module_t *)arg;
  struct timespec time_req_1us, time_rem_1us;
  ssize_t bytes_received;
  int spp,i;
  openair0_timestamp last_hw_counter=0;
  unsigned int samples_per_frame=0;
  uint8_t loopback=0,measurements=0;

#ifdef LOWLATENCY
  struct sched_attr attr;
  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  = (0.1  *  100) * 10000; // 
  attr.sched_deadline = rt_period;//0.1  *  1000000; // 
  attr.sched_period   = rt_period;//0.1  *  1000000; // each TX/RX thread has 

  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] eNB TX thread: sched_setattr failed\n");
    exit(-1);
  }
#endif	

  tx_pos=0;
  time_req_1us.tv_sec = 0;
  time_req_1us.tv_nsec = 1000;
  spp = dev->eth_dev.openair0_cfg.samples_per_packet;
  samples_per_frame = dev->eth_dev.openair0_cfg.samples_per_frame;
  loopback = dev->loopback;
  measurements = dev->measurements;
  
  while (rrh_exit == 0) {

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_TX, 1 );		
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TX_HWCNT, hw_counter );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TX_LHWCNT, last_hw_counter );

    if (measurements == 1 )
      clock_gettime(CLOCK_MONOTONIC,&time0a);

    for (i=0; i<dev->eth_dev.openair0_cfg.tx_num_channels; i++){
      if (!eNB_tx_started) {
	eNB_tx_started=1; // set this flag to 1 to indicate that eNB started
	if (RT_flag_eNB==1) {
	last_hw_counter=hw_counter;
	}
      } else {
	if (RT_flag_eNB==1) {
	  if (hw_counter > last_hw_counter+1) {
	    printf("LT");
	  } else {
	    while (hw_counter < last_hw_counter+1){
	      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_TX_SLEEP, 1 );
	      nanosleep(&time_req_1us,&time_rem_1us);
	      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_TX_SLEEP, 0 );
	    }
	  }
	}
      }

      if (measurements == 1 ) 
	clock_gettime(CLOCK_MONOTONIC,&time1);

      tx_eNB[i] = (void*)&tx_buffer_eNB[i][tx_pos];			
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TXCNT, tx_pos );
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );
      bytes_received = dev->eth_dev.trx_read_func(&dev->eth_dev,
						&timestamp_eNB_tx[i],
						tx_eNB,
						spp,
						i);		
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );	
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TX_TS, timestamp_eNB_tx[i]&0xffffffff );
      if (NRT_flag_eNB==1) {
	nrt_eNB_counter[i]++;
      }
      prev_tx_pos=tx_pos;
      tx_pos += spp;   

      if (tx_pos >= samples_per_frame)
	tx_pos -= samples_per_frame;
      
      last_hw_counter=hw_counter;
	
      if (loopback ==1 ) { 
	while (sync_eNB_rx[i]==0)
	  nanosleep(&time_req_1us,&time_rem_1us);
	
	pthread_mutex_lock(&sync_eNB_mutex[i]);
	sync_eNB_rx[i]++;
	pthread_mutex_unlock(&sync_eNB_mutex[i]);
      }
      
    }
    if (measurements == 1 ) {
      clock_gettime(CLOCK_MONOTONIC,&time2);
      memcpy(&time0,&time2,sizeof(struct timespec));
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_TX, 0 );
  }
  return(0);
}

//needs to be fixed 
void set_rt_period( openair0_config_t openair0_cfg){

  rt_period= (double)(openair0_cfg.samples_per_packet/(openair0_cfg.samples_per_frame/10.0)*1000000);
  AssertFatal(rt_period > 0, "Invalid rt period !%u\n", rt_period);
  //only in case of  NRT with emulated UE
  //create_timer_thread();  
}


void check_dev_config( rrh_module_t *mod_enb) {
  
  
  AssertFatal( (mod_enb->devs->openair0_cfg.num_rb_dl==100 || mod_enb->devs->openair0_cfg.num_rb_dl==50 || mod_enb->devs->openair0_cfg.num_rb_dl==25 || mod_enb->devs->openair0_cfg.num_rb_dl==6) , "Invalid number of resource blocks! %d\n", mod_enb->devs->openair0_cfg.num_rb_dl);
 AssertFatal( mod_enb->devs->openair0_cfg.samples_per_frame  > 0 ,  "Invalid number of samples per frame! %d\n",mod_enb->devs->openair0_cfg.samples_per_frame); 
 AssertFatal( mod_enb->devs->openair0_cfg.sample_rate        > 0.0, "Invalid sample rate! %f\n", mod_enb->devs->openair0_cfg.sample_rate);
 AssertFatal( mod_enb->devs->openair0_cfg.samples_per_packet > 0 ,  "Invalid number of samples per packet! %d\n",mod_enb->devs->openair0_cfg.samples_per_packet);
 AssertFatal( mod_enb->devs->openair0_cfg.rx_num_channels    > 0 ,  "Invalid number of RX antennas! %d\n", mod_enb->devs->openair0_cfg.rx_num_channels); 
 AssertFatal( mod_enb->devs->openair0_cfg.tx_num_channels    > 0 ,  "Invalid number of TX antennas! %d\n", mod_enb->devs->openair0_cfg.tx_num_channels);
 AssertFatal( mod_enb->devs->openair0_cfg.rx_freq[0]         > 0.0 ,"Invalid RX frequency! %f\n", mod_enb->devs->openair0_cfg.rx_freq[0]); 
 AssertFatal( mod_enb->devs->openair0_cfg.tx_freq[0]         > 0.0 ,"Invalid TX frequency! %f\n", mod_enb->devs->openair0_cfg.tx_freq[0]);
 AssertFatal( mod_enb->devs->openair0_cfg.rx_gain[0]         > 0.0 ,"Invalid RX gain! %f\n", mod_enb->devs->openair0_cfg.rx_gain[0]); 
 AssertFatal( mod_enb->devs->openair0_cfg.tx_gain[0]         > 0.0 ,"Invalid TX gain! %f\n", mod_enb->devs->openair0_cfg.tx_gain[0]);
 AssertFatal( mod_enb->devs->openair0_cfg.rx_bw              > 0.0 ,"Invalid RX bw! %f\n", mod_enb->devs->openair0_cfg.rx_bw); 
 AssertFatal( mod_enb->devs->openair0_cfg.tx_bw              > 0.0 ,"Invalid RX bw! %f\n", mod_enb->devs->openair0_cfg.tx_bw);
 // AssertFatal( mod_enb->devs->openair0_cfg.autocal[0]         > 0 ,  "Invalid auto calibration choice! %d\n", mod_enb->devs->openair0_cfg.autocal[0]);
 
 printf("\n---------------------RF device configuration parameters---------------------\n");
 
 printf("\tMod_id=%d\n \tlog level=%d\n \tDL_RB=%d\n \tsamples_per_frame=%d\n \tsample_rate=%f\n \tsamples_per_packet=%d\n \ttx_delay=%d\n \ttx_forward_nsamps=%d\n \trx_num_channels=%d\n \ttx_num_channels=%d\n \trx_freq_0=%f\n \ttx_freq_0=%f\n \trx_freq_1=%f\n \ttx_freq_1=%f\n \trx_freq_2=%f\n \ttx_freq_2=%f\n \trx_freq_3=%f\n \ttx_freq_3=%f\n \trxg_mode=%d\n \trx_gain_0=%f\n \ttx_gain_0=%f\n  \trx_gain_1=%f\n \ttx_gain_1=%f\n  \trx_gain_2=%f\n \ttx_gain_2=%f\n  \trx_gain_3=%f\n \ttx_gain_3=%f\n \trx_gain_offset_2=%f\n \ttx_gain_offset_3=%f\n  \trx_bw=%f\n \ttx_bw=%f\n \tautocal=%d\n \trem_addr %s:%d\n \tmy_addr %s:%d\n",	
	mod_enb->devs->openair0_cfg.Mod_id,
	mod_enb->devs->openair0_cfg.log_level,
	mod_enb->devs->openair0_cfg.num_rb_dl,
	mod_enb->devs->openair0_cfg.samples_per_frame,
	mod_enb->devs->openair0_cfg.sample_rate,
	mod_enb->devs->openair0_cfg.samples_per_packet,
	mod_enb->devs->openair0_cfg.tx_delay,
	mod_enb->devs->openair0_cfg.tx_forward_nsamps,
	mod_enb->devs->openair0_cfg.rx_num_channels,
	mod_enb->devs->openair0_cfg.tx_num_channels,
	mod_enb->devs->openair0_cfg.rx_freq[0],
	mod_enb->devs->openair0_cfg.tx_freq[0],
	mod_enb->devs->openair0_cfg.rx_freq[1],
	mod_enb->devs->openair0_cfg.tx_freq[1],
	mod_enb->devs->openair0_cfg.rx_freq[2],
	mod_enb->devs->openair0_cfg.tx_freq[2],
	mod_enb->devs->openair0_cfg.rx_freq[3],
	mod_enb->devs->openair0_cfg.tx_freq[3],
	mod_enb->devs->openair0_cfg.rxg_mode[0],
	mod_enb->devs->openair0_cfg.rx_gain[0],
	mod_enb->devs->openair0_cfg.tx_gain[0],
	mod_enb->devs->openair0_cfg.rx_gain[1],
	mod_enb->devs->openair0_cfg.tx_gain[1],
	mod_enb->devs->openair0_cfg.rx_gain[2],
	mod_enb->devs->openair0_cfg.tx_gain[2],
	mod_enb->devs->openair0_cfg.rx_gain[3],
	mod_enb->devs->openair0_cfg.tx_gain[3],
	//mod_enb->devs->openair0_cfg.rx_gain_offset[0],
	//mod_enb->devs->openair0_cfg.rx_gain_offset[1],
	mod_enb->devs->openair0_cfg.rx_gain_offset[2],
	mod_enb->devs->openair0_cfg.rx_gain_offset[3],
	mod_enb->devs->openair0_cfg.rx_bw,
	mod_enb->devs->openair0_cfg.tx_bw,
	mod_enb->devs->openair0_cfg.autocal[0],
	mod_enb->devs->openair0_cfg.remote_ip,
	mod_enb->devs->openair0_cfg.remote_port,
	mod_enb->devs->openair0_cfg.my_ip,
	mod_enb->devs->openair0_cfg.my_port  
	);
 
 printf("----------------------------------------------------------------------------\n");
 
 
}
