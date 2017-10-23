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

/*! \file UE_transport_IQ.c
 * \brief UE transport IQ sampels 
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

#define START_CMD 1
#define RRH_UE_PORT 51000
#define RRH_UE_DEST_IP "127.0.0.1"
#define PRINTF_PERIOD 3750

/******************************************************************************
 **                               FUNCTION PROTOTYPES                        **
 ******************************************************************************/
void *rrh_proc_UE_thread(void *);
void *rrh_UE_thread(void *);
void *rrh_UE_rx_thread(void *);
void *rrh_UE_tx_thread(void *);



openair0_timestamp	timestamp_UE_tx[4]= {0,0,0,0},timestamp_UE_rx[4]= {0,0,0,0};
openair0_timestamp 	nrt_UE_counter[4]= {0,0,0,0};

pthread_t		main_rrh_UE_thread,main_rrh_proc_UE_thread;
pthread_attr_t 		attr, attr_proc;
struct sched_param 	sched_param_rrh, sched_param_rrh_proc;
pthread_cond_t 		sync_UE_cond[4];
pthread_mutex_t 	sync_UE_mutex[4];

int32_t 		overflow_rx_buffer_UE[4]= {0,0,0,0};
int32_t 		nsamps_UE[4]= {0,0,0,0};
int32_t 		UE_tx_started=0,UE_rx_started=0;
int32_t 		RT_flag_UE=0, NRT_flag_UE=1;
int32_t 		counter_UE_rx[4]= {0,0,0,0};
int32_t 		counter_UE_tx[4]= {0,0,0,0};
int32_t			**tx_buffer_UE, **rx_buffer_UE;

int 			sync_UE_rx[4]= {-1,-1,-1,-1};
void 			*rrh_UE_thread_status;


void *rx_ue[2]; // FIXME hard coded array size; indexed by lte_frame_parms.nb_antennas_rx
void *tx_ue[2]; // FIXME hard coded array size; indexed by lte_frame_parms.nb_antennas_tx


void config_UE_mod( rrh_module_t *dev_ue, uint8_t RT_flag,uint8_t NRT_flag) {

  int 	  i;
  int 	  error_code_UE, error_code_proc_UE;
  
  RT_flag_UE=RT_flag;
  NRT_flag_UE=NRT_flag;
  
  pthread_attr_init(&attr);
  sched_param_rrh.sched_priority = sched_get_priority_max(SCHED_FIFO);
  pthread_attr_init(&attr_proc);
  sched_param_rrh_proc.sched_priority = sched_get_priority_max(SCHED_FIFO-1);
  
  pthread_attr_setschedparam(&attr,&sched_param_rrh);
  pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
  pthread_attr_setschedparam(&attr_proc,&sched_param_rrh_proc);
  pthread_attr_setschedpolicy(&attr_proc,SCHED_FIFO-1);
  
  
  for (i=0; i<4; i++) {
    pthread_mutex_init(&sync_UE_mutex[i],NULL);
    pthread_cond_init(&sync_UE_cond[i],NULL);
  }
  
  error_code_UE = pthread_create(&main_rrh_UE_thread, &attr, rrh_UE_thread, (void *)dev_ue);
  error_code_proc_UE = pthread_create(&main_rrh_proc_UE_thread, &attr_proc, rrh_proc_UE_thread, (void *)dev_ue);
  
  if (error_code_UE) {
    printf("Error while creating UE thread\n");
    exit(-1);
  }
  
  if (error_code_proc_UE) {
    printf("Error while creating UE proc thread\n");
    exit(-1);
  }
  
}


/*! \fn void *rrh_proc_UE_thread((void *)dev_ue)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
void *rrh_proc_UE_thread(void * arg) {
  
  int				antenna_index,i;
  openair0_timestamp 		truncated_timestamp, truncated_timestamp_final, last_hw_counter=0;
  struct			timespec time_req, time_rem;
  int16_t       		*txp,*rxp;
  unsigned int                  samples_per_frame=0;
  rrh_module_t 	*dev=(rrh_module_t *)arg;
  
  samples_per_frame= dev->eth_dev.openair0_cfg->samples_per_frame;
  AssertFatal(samples_per_frame <=0, "invalide samples_per_frame !%u\n",samples_per_frame);

  time_req.tv_sec = 0;
  time_req.tv_nsec = 1000;
  
  while (rrh_exit==0) {
    //wait until some data has been copied
    for (antenna_index=0; antenna_index<4; antenna_index++) {
      if (sync_UE_rx[antenna_index]==0) {
	if (!UE_tx_started) {
	  UE_tx_started=1;  //Set this flag to 1 to indicate that a UE started retrieving data
	  if (RT_flag_UE==1) {
	    last_hw_counter=hw_counter;
	  }
	} else {
	  if (RT_flag_UE==1) {
	    if (hw_counter > last_hw_counter+1) {
	      printf("L1");
	      //              goto end_copy_UE;
	    } else {
	      while (hw_counter < last_hw_counter+1)
		nanosleep(&time_req,&time_rem);
	    }
	  }
	}
	
	truncated_timestamp = timestamp_UE_tx[antenna_index]%(samples_per_frame);
	truncated_timestamp_final =  (timestamp_UE_tx[antenna_index]+nsamps_UE[antenna_index])%samples_per_frame;
	
	if ((truncated_timestamp + nsamps_UE[antenna_index]) > samples_per_frame) {
	  if ((timestamp_eNB_rx[antenna_index]%samples_per_frame < nsamps_UE[antenna_index]) && (eNB_rx_started==1)) {
	    overflow_rx_buffer_eNB[antenna_index]++;
	    printf("eNB Overflow[%d] : %d, timestamp : %d\n",antenna_index,overflow_rx_buffer_eNB[antenna_index],(int)truncated_timestamp);
	    
	    if (NRT_flag_UE==1) {
	      while ((timestamp_eNB_rx[antenna_index]%samples_per_frame) < nsamps_UE[antenna_index])
		nanosleep(&time_req,&time_rem);
	    }
	  }
	  
	  rxp = (int16_t*)&rx_buffer_eNB[antenna_index][truncated_timestamp];
	  txp = (int16_t*)&tx_buffer_UE[antenna_index][truncated_timestamp];
	  
	  for (i=0; i<(samples_per_frame<<1)-(truncated_timestamp<<1); i++) {
	    rxp[i] = txp[i]>>6;
	  }
	  
	  rxp = (int16_t*)&rx_buffer_eNB[antenna_index][0];
	  txp = (int16_t*)&tx_buffer_UE[antenna_index][0];
	  
	  for (i=0; i<nsamps_eNB[antenna_index]-(samples_per_frame)+(truncated_timestamp); i++) {
	    rxp[i] = txp[i]>>6;
	  }
	  
	} else {
	  if (((truncated_timestamp < (timestamp_eNB_rx[antenna_index]%samples_per_frame)) && 
	       (truncated_timestamp_final >  (timestamp_eNB_rx[antenna_index]%samples_per_frame))) && 
	      (eNB_rx_started==1)) {
	    overflow_rx_buffer_eNB[antenna_index]++;
	    printf("eNB Overflow[%d] : %d, timestamp : %d\n",antenna_index,overflow_rx_buffer_eNB[antenna_index],(int)truncated_timestamp);
	    
	    if (NRT_flag_UE==1) {
	      while (truncated_timestamp_final >  timestamp_eNB_rx[antenna_index]%samples_per_frame)
		nanosleep(&time_req,&time_rem);
	    }
	  }
	  
	  rxp = (int16_t*)&rx_buffer_eNB[antenna_index][truncated_timestamp];
	  txp = (int16_t*)&tx_buffer_UE[antenna_index][truncated_timestamp];
	  
	  for (i=0; i<(nsamps_eNB[antenna_index]); i++) {
	    rxp[i] =txp[i]>>6;
	  }
		  
	}
	//end_copy_UE :
	last_hw_counter=hw_counter;
	pthread_mutex_lock(&sync_UE_mutex[antenna_index]);
	sync_UE_rx[antenna_index]--;
	pthread_mutex_unlock(&sync_UE_mutex[antenna_index]);
	
      }
    }

  }
  
  return(0);
}

/*! \fn void *rrh_UE_thread(void *arg)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
void *rrh_UE_thread(void *arg) {
  
  rrh_module_t 	*dev=(rrh_module_t *)arg;

  struct sched_param 	sched_param_UE_rx, sched_param_UE_tx;
  int16_t		i,cmd;   //,nsamps,antenna_index;
  //struct timespec 	time_req_1us;
  pthread_t 		UE_rx_thread, UE_tx_thread;
  pthread_attr_t 	attr_UE_rx, attr_UE_tx;
  int 			error_code_UE_rx, error_code_UE_tx;
  void 			*tmp;
  unsigned int          samples_per_frame=0;
  
  samples_per_frame= dev->eth_dev.openair0_cfg->samples_per_frame;
  //time_req_1us.tv_sec = 0;
  //time_req_1us.tv_nsec = 1000;
  
  while (rrh_exit==0) {
    
    cmd=dev->eth_dev.trx_start_func(&dev->eth_dev);
    
    /* allocate memory for TX/RX buffers */
    rx_buffer_UE = (int32_t**)malloc16(dev->eth_dev.openair0_cfg->rx_num_channels*sizeof(int32_t*));
    tx_buffer_UE = (int32_t**)malloc16(dev->eth_dev.openair0_cfg->tx_num_channels*sizeof(int32_t*));
    
    for (i=0; i<dev->eth_dev.openair0_cfg->rx_num_channels; i++) {
      tmp=(void *)malloc(sizeof(int32_t)*(samples_per_frame+4));
      memset(tmp,0,sizeof(int32_t)*(samples_per_frame+4));
      rx_buffer_UE[i]=(tmp+4*sizeof(int32_t));  
    }
    
    for (i=0; i<dev->eth_dev.openair0_cfg->tx_num_channels; i++) {
      tmp=(void *)malloc(sizeof(int32_t)*(samples_per_frame+4));
      memset(tmp,0,sizeof(int32_t)*(samples_per_frame+4));
      tx_buffer_UE[i]=(tmp+4*sizeof(int32_t));  
    }
    
    printf("Client %s:%d is connected (DL_RB=%d) rt=%d|%d. \n" ,   dev->eth_dev.openair0_cfg->remote_addr,
	   dev->eth_dev.openair0_cfg->remote_port,
	   dev->eth_dev.openair0_cfg->num_rb_dl,
	   dev->eth_dev.openair0_cfg->rx_num_channels,
	   dev->eth_dev.openair0_cfg->tx_num_channels);
    
    if (cmd==START_CMD) {
      
      pthread_attr_init(&attr_UE_rx);
      pthread_attr_init(&attr_UE_tx);
      sched_param_UE_rx.sched_priority = sched_get_priority_max(SCHED_FIFO);
      sched_param_UE_tx.sched_priority = sched_get_priority_max(SCHED_FIFO);
      pthread_attr_setschedparam(&attr_UE_rx,&sched_param_UE_rx);
      pthread_attr_setschedparam(&attr_UE_tx,&sched_param_UE_tx);
      pthread_attr_setschedpolicy(&attr_UE_rx,SCHED_FIFO);
      pthread_attr_setschedpolicy(&attr_UE_tx,SCHED_FIFO);
      
      error_code_UE_rx = pthread_create(&UE_rx_thread, &attr_UE_rx, rrh_UE_rx_thread, (void *)&dev);
      error_code_UE_tx = pthread_create(&UE_tx_thread, &attr_UE_tx, rrh_UE_tx_thread, (void *)&dev);
      
      if (error_code_UE_rx) {
	printf("Error while creating UE RX thread\n");
	exit(-1);
      }

      if (error_code_UE_tx) {
	printf("Error while creating UE TX thread\n");
	exit(-1);
      }
      
      while (rrh_exit==0)
	sleep(1);
    }
  }
  
  
  rrh_UE_thread_status = 0;
  pthread_exit(&rrh_UE_thread_status);
  
  return(0);
}


/*! \fn void *rrh_UE_rx_thread(void *arg)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
void *rrh_UE_rx_thread(void *arg) {

  rrh_module_t        *dev = (rrh_module_t *)arg;
  struct 	      timespec time0,time1,time2;
  struct 	      timespec time_req_1us, time_rem_1us;
  ssize_t	      bytes_sent;
  int 		      antenna_index, nsamps;
  int 		      trace_cnt=0;
  unsigned long long  max_rx_time=0, min_rx_time=133333, total_rx_time=0, average_rx_time=133333, s_period=0, trial=0;
  unsigned int        samples_per_frame=0;
  openair0_timestamp  last_hw_counter=0;
  
  antenna_index     = 0;
  nsamps	    = dev->eth_dev.openair0_cfg->samples_per_packet;
  samples_per_frame = dev->eth_dev.openair0_cfg->samples_per_frame;
  
  /* TODO: check if setting time0 has to be done here */
  clock_gettime(CLOCK_MONOTONIC,&time0);

  while (rrh_exit == 0) {
    if (!UE_rx_started) {
      UE_rx_started=1;  //Set this flag to 1 to indicate that a UE started retrieving data
      
      if (RT_flag_UE==1) {
	last_hw_counter=hw_counter;
      }
    } else {
      if (RT_flag_UE==1) {
	if (hw_counter > last_hw_counter+1) {
	  printf("L1");
	  //goto end_copy_UE;
	} else {
	  while (hw_counter < last_hw_counter+1)
	    nanosleep(&time_req_1us,&time_rem_1us);
	}
      }
    }
    
    clock_gettime(CLOCK_MONOTONIC,&time1);
    
    // send return
    
    if ((timestamp_UE_rx[antenna_index]%(samples_per_frame)+nsamps) > samples_per_frame) { // Wrap around if nsamps exceeds the buffer limit
      if (((timestamp_eNB_tx[antenna_index]%(samples_per_frame)) < ((timestamp_UE_rx[antenna_index]+nsamps)%(samples_per_frame))) && (eNB_tx_started==1)) {
	printf("UE underflow wraparound timestamp_UE_rx : %d, timestamp_eNB_tx : %d\n",
	       (int)(timestamp_UE_rx[antenna_index]%(samples_per_frame)),
	       (int)(timestamp_eNB_tx[antenna_index]%samples_per_frame));
	
	if (NRT_flag_UE==1) {
	  while ((timestamp_eNB_tx[antenna_index]%samples_per_frame) < ((timestamp_UE_rx[antenna_index]+nsamps)%(samples_per_frame)))
	    nanosleep(&time_req_1us,&time_rem_1us);
	}
      }
      
      rx_ue[antenna_index]=(void*)&rx_buffer_UE[antenna_index][timestamp_UE_rx[antenna_index]%(samples_per_frame)];
      if ((bytes_sent =dev->eth_dev.trx_write_func (dev,
						    timestamp_UE_rx[antenna_index],
						    rx_ue,
						    ( samples_per_frame- (timestamp_eNB_rx[antenna_index]%(samples_per_frame)) ),
					antenna_index,
						    0))<0)
	perror("RRH UE : sendto for RX");
      
      rx_ue[antenna_index]=(void*)&rx_buffer_UE[antenna_index][3];
      if ((bytes_sent  =dev->eth_dev.trx_write_func (dev,
						     timestamp_UE_rx[antenna_index],
						     rx_ue,
						     (nsamps - samples_per_frame + (timestamp_eNB_rx[antenna_index]%(samples_per_frame))),
						     antenna_index,
						     0))<0)
	perror("RRH UE : sendto for RX");
      
    } else {
      if (((timestamp_UE_rx[antenna_index]%samples_per_frame)< timestamp_eNB_tx[antenna_index]%samples_per_frame) && 
	  (((timestamp_UE_rx[antenna_index]+nsamps)%samples_per_frame) > (timestamp_eNB_tx[antenna_index]%samples_per_frame)) && 
	  (eNB_tx_started==1) ) {
	printf("UE underflow timestamp_UE_rx : %d, timestamp_eNB_tx : %d\n",
	       (int)(timestamp_UE_rx[antenna_index]%samples_per_frame),
	       (int)(timestamp_eNB_tx[antenna_index]%samples_per_frame));
	
	if (NRT_flag_UE==1) {
	  while (((timestamp_UE_rx[antenna_index]+nsamps)%samples_per_frame) > (timestamp_eNB_tx[antenna_index]%samples_per_frame)) {
	    nanosleep(&time_req_1us,&time_rem_1us);
	  }
	}
      }
      
      rx_ue[antenna_index]=(void*)&rx_buffer_UE[antenna_index][timestamp_UE_rx[antenna_index]%(samples_per_frame)];
      if ((bytes_sent = dev->eth_dev.trx_write_func (dev,
						     timestamp_UE_rx[antenna_index],
						     rx_ue,
						     nsamps,
						     antenna_index,
						     0))<0)
	perror("RRH UE thread: sendto for RX");
    }
    
    timestamp_UE_rx[antenna_index]+=nsamps;
    last_hw_counter=hw_counter;
    
    clock_gettime(CLOCK_MONOTONIC,&time2);
    
    if (trace_cnt++ > 10) {
      total_rx_time = (unsigned int)(time2.tv_nsec - time0.tv_nsec);
      
      if (total_rx_time < 0) total_rx_time=1000000000-total_rx_time;
      
      if ((total_rx_time > 0) && (total_rx_time < 1000000000)) {
	trial++;
	
	if (total_rx_time < min_rx_time)
	  min_rx_time = total_rx_time;
	
	if (total_rx_time > max_rx_time)
	  max_rx_time = total_rx_time;
	
	average_rx_time = (long long unsigned int)((average_rx_time*trial)+total_rx_time)/(trial+1);
      }
      
      if (s_period++ == PRINTF_PERIOD) {
	s_period=0;
	printf("Average UE RX time : %llu\tMax RX time : %llu\tMin RX time : %llu\n",average_rx_time,max_rx_time,min_rx_time);
	
      }
      
      //printf("RX: t1 %llu (time from last send), t2 %llu (sendto of %lu bytes) total time : %llu\n",(long long unsigned int)(time1.tv_nsec  - time0.tv_nsec), (long long unsigned int)(time2.tv_nsec - time1.tv_nsec),
      //   (nsamps<<2)+sizeof(openair0_timestamp),(long long unsigned int)(time2.tv_nsec - time0.tv_nsec));
      
    }
    
    memcpy(&time0,&time2,sizeof(struct timespec));
    
  }  //while (UE_exit==0)
  
  return(0);
  
}


/*! \fn void *rrh_UE_tx_thread(void *arg)
* \brief this function
* \param[in]
* \param[out]
* \return
* \note
* @ingroup  _oai
*/
void *rrh_UE_tx_thread(void *arg) {
  
  struct timespec 	time0a,time0,time1,time2;
  struct timespec 	time_req_1us, time_rem_1us;
  
  
  rrh_module_t    	*dev = (rrh_module_t *)arg;
  ssize_t 		bytes_received;
  int 			antenna_index, nsamps;
  unsigned int          samples_per_frame=0;
  
  antenna_index    = 0;
  nsamps 	   = dev->eth_dev.openair0_cfg->samples_per_packet;
  samples_per_frame = dev->eth_dev.openair0_cfg->samples_per_frame;
  
  
  while (rrh_exit == 0) {
    
    clock_gettime(CLOCK_MONOTONIC,&time0a);
    
    bytes_received = dev->eth_dev.trx_read_func(dev,
						&timestamp_UE_tx[antenna_index],
						tx_ue,
						nsamps,
						antenna_index);
    
    clock_gettime(CLOCK_MONOTONIC,&time1);
    
    if (NRT_flag_UE==1) {
      nrt_UE_counter[antenna_index]++;
    }
    printf(" first part size: %d   second part size: %"PRId64" idx=%"PRId64"\n",
	   (int32_t)(((samples_per_frame)<<2)-((timestamp_UE_tx[antenna_index]%(samples_per_frame))<<2)),
	   (nsamps<<2)-((samples_per_frame)<<2)+((timestamp_UE_tx[antenna_index]%(samples_per_frame))<<2),
	   timestamp_UE_tx[antenna_index]%(samples_per_frame));
    if ((timestamp_UE_tx[antenna_index]%(samples_per_frame)+nsamps) > samples_per_frame) { // Wrap around if nsamps exceeds the buffer limit
      memcpy(&tx_buffer_UE[antenna_index][timestamp_UE_tx[antenna_index]%(samples_per_frame)],(void*)(tx_ue[antenna_index]),
	     (samples_per_frame<<2)-((timestamp_UE_tx[antenna_index]%(samples_per_frame))<<2));
      
      memcpy(&tx_buffer_UE[antenna_index][0], (void*)(tx_ue[antenna_index]+(samples_per_frame*4)-((timestamp_UE_tx[antenna_index]%(samples_per_frame))<<2)),
             (nsamps<<2)-((samples_per_frame-(timestamp_UE_tx[antenna_index]%(samples_per_frame)))<<2));
      //printf("Received UE TX samples for antenna %d, nsamps %d (%d)\n",antenna_index,nsamps,(int)(bytes_received>>2));
    } else {
      memcpy(&tx_buffer_UE[antenna_index][timestamp_UE_tx[antenna_index]%(samples_per_frame)], (void*)(tx_ue[antenna_index]),(nsamps<<2));
    }
    
    while (sync_UE_rx[antenna_index]==0)
      nanosleep(&time_req_1us,&time_rem_1us);
    
    pthread_mutex_lock(&sync_UE_mutex[antenna_index]);
    sync_UE_rx[antenna_index]++;
    
    if (!sync_UE_rx[antenna_index]) {
      counter_UE_tx[antenna_index]=(counter_UE_tx[antenna_index]+nsamps)%samples_per_frame;
      nsamps_UE[antenna_index]=nsamps;
    } else {
      printf("rrh_eNB_proc thread is busy, will exit\n");
      exit(-1);
    }
    
    pthread_mutex_unlock(&sync_UE_mutex[antenna_index]);
    
    clock_gettime(CLOCK_MONOTONIC,&time2);
    
    memcpy(&time0,&time2,sizeof(struct timespec));
    
  }
  
  return(0);
  
}


