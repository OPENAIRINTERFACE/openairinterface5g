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

/** ethernet_lib : API to stream I/Q samples over standard ethernet (RRH component)
*
*  Authors: Raymond Knopp  <raymond.knopp@eurecom.fr>,  Riadh Ghaddab <riadh.ghaddab@eurecom.fr>
*
*  Changelog:
*  06.10.2014: Initial version
*/

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <pthread.h>
#include <time.h>

#include "common_lib.h"

#define BUF_LEN 4096+32
#define RRH_eNB_PORT 50000
#define RRH_eNB_DEST_IP "127.0.0.1"
#define RRH_UE_PORT 51000
#define RRH_UE_DEST_IP "127.0.0.1"

#define FRAME_MAX_SIZE 307200//76800
#define DEFAULT_PERIOD_NS 133333//200000

#define START_CMD 1
#define PRINTF_PERIOD 3750

typedef struct {
  int eNB_port;
  char eNB_dest_ip[20];
  int UE_port;
  char UE_dest_ip[20];
  struct timespec time_req;
} rrh_desc_t;

typedef struct {
  int32_t nsamps;
  int32_t antenna_index_eNB_rx;
  int32_t antenna_index_eNB_tx;
  int sockid_eNB;
  struct sockaddr clientaddr;
  socklen_t clientaddrlen;
} rrh_eNB_desc_t;

typedef struct {
  int32_t nsamps;
  int32_t antenna_index_UE_rx;
  int32_t antenna_index_UE_tx;
  int sockid_UE;
  struct sockaddr clientaddr;
  socklen_t clientaddrlen;
} rrh_UE_desc_t;

int rrh_exit=0;

int32_t tx_buffer_eNB[4][(1+(sizeof(openair0_timestamp)>>2))+FRAME_MAX_SIZE],rx_buffer_eNB[4][(1+(sizeof(openair0_timestamp)>>2))+FRAME_MAX_SIZE];
int32_t tx_buffer_UE[4][(1+(sizeof(openair0_timestamp)>>2))+FRAME_MAX_SIZE],rx_buffer_UE[4][(1+(sizeof(openair0_timestamp)>>2))+FRAME_MAX_SIZE];
void  *rrh_eNB_thread_status;
void  *rrh_UE_thread_status;
int32_t counter_UE_rx[4]= {0,0,0,0};
int32_t counter_UE_tx[4]= {0,0,0,0};
int32_t counter_eNB_rx[4]= {0,0,0,0};
int32_t counter_eNB_tx[4]= {0,0,0,0};
int32_t overflow_rx_buffer_UE[4]= {0,0,0,0};
int32_t overflow_rx_buffer_eNB[4]= {0,0,0,0};
int32_t nsamps_eNB[4]= {0,0,0,0};
int32_t nsamps_UE[4]= {0,0,0,0};
int32_t UE_tx_started=0,eNB_tx_started=0, UE_rx_started=0,eNB_rx_started=0;
int32_t RT_FLAG=0, NRT_FLAG=1;
openair0_timestamp nrt_eNB_counter[4]= {0,0,0,0};
openair0_timestamp nrt_UE_counter[4]= {0,0,0,0};

openair0_timestamp timestamp_eNB_tx[4],timestamp_eNB_rx[4]= {0,0,0,0};
openair0_timestamp timestamp_UE_tx[4],timestamp_UE_rx[4]= {0,0,0,0};
openair0_timestamp hw_counter=0;

pthread_cond_t sync_UE_cond[4];
pthread_mutex_t sync_UE_mutex[4];
pthread_cond_t sync_eNB_cond[4];
pthread_mutex_t sync_eNB_mutex[4];
pthread_mutex_t timer_mutex;

int sync_UE_rx[4]= {-1,-1,-1,-1};
int sync_eNB_rx[4]= {-1,-1,-1,-1};

//functions prototype
void timer_signal_handler(int);
void *timer_proc(void *);
void *rrh_proc_eNB_thread();
void *rrh_proc_UE_thread();
void *rrh_UE_thread(void *);
void *rrh_UE_rx_thread(void *);
void *rrh_UE_tx_thread(void *);
void *rrh_eNB_thread(void *);
void *rrh_eNB_rx_thread(void *);
void *rrh_eNB_tx_thread(void *);

void timer_signal_handler(int sig)
{

  if (sig == SIGALRM) {
    pthread_mutex_lock(&timer_mutex);
    hw_counter ++;
    //printf("[RRH] : hw_counter : %d\n",(int)hw_counter);
    pthread_mutex_unlock(&timer_mutex);
  }


}


void *timer_proc(void *arg)
{
  timer_t             timerid;    // timer ID for timer
  //struct sigevent     event;      // event to deliver
  struct itimerspec   *timer = (struct itimerspec*)arg;      // the timer data structure
  struct itimerspec  *old_value;


  //printf("Starting the timer\n");
  if (timer_create (CLOCK_REALTIME, NULL, &timerid) == -1) {
    fprintf (stderr, "couldn't create a timer\n");
    perror (NULL);
    exit (EXIT_FAILURE);
  }

  signal(SIGALRM, timer_signal_handler);
  // and start it!
  timer_settime (timerid, 0, timer, old_value);

  while (!rrh_exit) {
    sleep(1);
  }

  timer_delete(timerid);
  return (0);


}


void *rrh_proc_eNB_thread()
{

  //rrh_desc_t *rrh_desc = (rrh_desc_t *)arg;
  int antenna_index,i;
  openair0_timestamp truncated_timestamp, truncated_timestamp_final, last_hw_counter=0;
  struct timespec time_req, time_rem;
  int16_t *rxp,*txp;

  time_req.tv_sec = 0;
  time_req.tv_nsec = 1000;

  while (rrh_exit==0) {
    //wait until some data has been copied
    for (antenna_index=0; antenna_index<4; antenna_index++) {
      if (sync_eNB_rx[antenna_index]==0) {
        if (!eNB_tx_started) {
          eNB_tx_started=1; // set this flag to 1 to indicate that eNB started

          if (RT_FLAG==1) {
            last_hw_counter=hw_counter;
          }
        } else {
          if (RT_FLAG==1) {
            if (hw_counter > last_hw_counter+1) {
              printf("L");
              //      goto end_copy_eNB;
            } else {
              //printf("hw_counter : %llu, last_hw_counter : %llu\n",hw_counter,last_hw_counter);
              while (hw_counter < last_hw_counter+1)
                nanosleep(&time_req,&time_rem);
            }
          }
        }

        truncated_timestamp = timestamp_eNB_tx[antenna_index]%(FRAME_MAX_SIZE);
        truncated_timestamp_final = (timestamp_eNB_tx[antenna_index]+nsamps_eNB[antenna_index])%FRAME_MAX_SIZE;

        if ((truncated_timestamp + nsamps_eNB[antenna_index]) > FRAME_MAX_SIZE) {
          if ((timestamp_UE_rx[antenna_index]%FRAME_MAX_SIZE < nsamps_eNB[antenna_index]) && (UE_rx_started==1)) {
            overflow_rx_buffer_UE[antenna_index]++;
            printf("UE Overflow[%d] : %d, timestamp : %d\n",antenna_index,overflow_rx_buffer_UE[antenna_index],(int)truncated_timestamp);

            if (NRT_FLAG==1) {
              while ((timestamp_UE_rx[antenna_index]%FRAME_MAX_SIZE) < nsamps_eNB[antenna_index])
                nanosleep(&time_req,&time_rem);
            }
          }

          rxp = (int16_t*)&rx_buffer_UE[antenna_index][truncated_timestamp+(sizeof(openair0_timestamp)>>2)];
          txp = (int16_t*)&tx_buffer_eNB[antenna_index][truncated_timestamp+(sizeof(openair0_timestamp)>>2)];

          for (i=0; i<(FRAME_MAX_SIZE<<1)-(truncated_timestamp<<1); i++) {
            rxp[i] = txp[i]>>6;
          }

          rxp = (int16_t*)&rx_buffer_UE[antenna_index][(sizeof(openair0_timestamp)>>2)];
          txp = (int16_t*)&tx_buffer_eNB[antenna_index][0];

          for (i=0; i<nsamps_eNB[antenna_index]-(FRAME_MAX_SIZE)+(truncated_timestamp); i++) {
            rxp[i] = txp[i]>>6;
          }

          /*          memcpy(&rx_buffer_UE[antenna_index][truncated_timestamp + (sizeof(openair0_timestamp)>>2)],&tx_buffer_eNB[antenna_index][truncated_timestamp],(FRAME_MAX_SIZE<<2)-(truncated_timestamp<<2));
                memcpy(&rx_buffer_UE[antenna_index][(sizeof(openair0_timestamp)>>2)],&tx_buffer_eNB[antenna_index][0],(nsamps_eNB[antenna_index]<<2)-(FRAME_MAX_SIZE<<2)+(truncated_timestamp<<2));*/
        } else {
          if (((truncated_timestamp < (timestamp_UE_rx[antenna_index]%FRAME_MAX_SIZE)) && (truncated_timestamp_final >  (timestamp_UE_rx[antenna_index]%FRAME_MAX_SIZE))) && (UE_rx_started==1)) {
            overflow_rx_buffer_UE[antenna_index]++;
            printf("UE Overflow[%d] : %d, timestamp : %d\n",antenna_index,overflow_rx_buffer_UE[antenna_index],(int)truncated_timestamp);

            if (NRT_FLAG==1) {
              while (truncated_timestamp_final >  timestamp_UE_rx[antenna_index]%FRAME_MAX_SIZE)
                nanosleep(&time_req,&time_rem);
            }
          }

          rxp = (int16_t*)&rx_buffer_UE[antenna_index][truncated_timestamp+(sizeof(openair0_timestamp))];
          txp = (int16_t*)&tx_buffer_eNB[antenna_index][truncated_timestamp];

          for (i=0; i<(nsamps_eNB[antenna_index]); i++) {
            rxp[i] =txp[i]>>6;
          }

          /*
            memcpy(&rx_buffer_UE[antenna_index][truncated_timestamp + (sizeof(openair0_timestamp)>>2)],&tx_buffer_eNB[antenna_index][truncated_timestamp],(nsamps_eNB[antenna_index]<<2));*/
        }


        // end_copy_eNB :
        last_hw_counter=hw_counter;
        pthread_mutex_lock(&sync_eNB_mutex[antenna_index]);
        sync_eNB_rx[antenna_index]--;
        pthread_mutex_unlock(&sync_eNB_mutex[antenna_index]);

      }
    }

  }

  return(0);
}

void *rrh_proc_UE_thread()
{

  //rrh_desc_t *rrh_desc = (rrh_desc_t *)arg;
  int antenna_index,i;
  openair0_timestamp truncated_timestamp, truncated_timestamp_final, last_hw_counter=0;
  struct timespec time_req, time_rem;
  int16_t *txp,*rxp;

  time_req.tv_sec = 0;
  time_req.tv_nsec = 1000;


  while (rrh_exit==0) {
    //wait until some data has been copied
    for (antenna_index=0; antenna_index<4; antenna_index++) {
      if (sync_UE_rx[antenna_index]==0) {
        if (!UE_tx_started) {
          UE_tx_started=1;  //Set this flag to 1 to indicate that a UE started retrieving data

          if (RT_FLAG==1) {
            last_hw_counter=hw_counter;
          }
        } else {
          if (RT_FLAG==1) {
            if (hw_counter > last_hw_counter+1) {
              printf("L1");
              //              goto end_copy_UE;
            } else {
              while (hw_counter < last_hw_counter+1)
                nanosleep(&time_req,&time_rem);
            }
          }
        }

        truncated_timestamp = timestamp_UE_tx[antenna_index]%(FRAME_MAX_SIZE);
        truncated_timestamp_final =  (timestamp_UE_tx[antenna_index]+nsamps_UE[antenna_index])%FRAME_MAX_SIZE;

        if ((truncated_timestamp + nsamps_UE[antenna_index]) > FRAME_MAX_SIZE) {
          if ((timestamp_eNB_rx[antenna_index]%FRAME_MAX_SIZE < nsamps_UE[antenna_index]) && (eNB_rx_started==1)) {
            overflow_rx_buffer_eNB[antenna_index]++;
            printf("eNB Overflow[%d] : %d, timestamp : %d\n",antenna_index,overflow_rx_buffer_eNB[antenna_index],(int)truncated_timestamp);

            if (NRT_FLAG==1) {
              while ((timestamp_eNB_rx[antenna_index]%FRAME_MAX_SIZE) < nsamps_UE[antenna_index])
                nanosleep(&time_req,&time_rem);
            }
          }

          rxp = (int16_t*)&rx_buffer_eNB[antenna_index][truncated_timestamp+(sizeof(openair0_timestamp)>>2)];
          txp = (int16_t*)&tx_buffer_UE[antenna_index][truncated_timestamp+(sizeof(openair0_timestamp)>>2)];

          for (i=0; i<(FRAME_MAX_SIZE<<1)-(truncated_timestamp<<1); i++) {
            rxp[i] = txp[i]>>6;
          }

          rxp = (int16_t*)&rx_buffer_eNB[antenna_index][(sizeof(openair0_timestamp)>>2)];
          txp = (int16_t*)&tx_buffer_UE[antenna_index][0];

          for (i=0; i<nsamps_eNB[antenna_index]-(FRAME_MAX_SIZE)+(truncated_timestamp); i++) {
            rxp[i] = txp[i]>>6;
          }

          /*
                memcpy(&rx_buffer_eNB[antenna_index][truncated_timestamp + (sizeof(openair0_timestamp)>>2)],&tx_buffer_UE[antenna_index][truncated_timestamp],(FRAME_MAX_SIZE<<2)-(truncated_timestamp<<2));
                memcpy(&rx_buffer_eNB[antenna_index][(sizeof(openair0_timestamp)>>2)],&tx_buffer_UE[antenna_index][0],(nsamps_UE[antenna_index]<<2)-(FRAME_MAX_SIZE<<2)+(truncated_timestamp<<2));*/

        } else {
          if (((truncated_timestamp < (timestamp_eNB_rx[antenna_index]%FRAME_MAX_SIZE)) && (truncated_timestamp_final >  (timestamp_eNB_rx[antenna_index]%FRAME_MAX_SIZE))) && (eNB_rx_started==1)) {
            overflow_rx_buffer_eNB[antenna_index]++;
            printf("eNB Overflow[%d] : %d, timestamp : %d\n",antenna_index,overflow_rx_buffer_eNB[antenna_index],(int)truncated_timestamp);

            if (NRT_FLAG==1) {
              while (truncated_timestamp_final >  timestamp_eNB_rx[antenna_index]%FRAME_MAX_SIZE)
                nanosleep(&time_req,&time_rem);
            }
          }

          rxp = (int16_t*)&rx_buffer_eNB[antenna_index][truncated_timestamp+(sizeof(openair0_timestamp))];
          txp = (int16_t*)&tx_buffer_UE[antenna_index][truncated_timestamp];

          for (i=0; i<(nsamps_eNB[antenna_index]); i++) {
            rxp[i] =txp[i]>>6;
          }

          /*          memcpy(&rx_buffer_eNB[antenna_index][truncated_timestamp+ (sizeof(openair0_timestamp)>>2)],&tx_buffer_UE[antenna_index][truncated_timestamp],(nsamps_UE[antenna_index]<<2));*/
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


void *rrh_UE_rx_thread(void *arg)
{

  struct sockaddr clientaddr;
  socklen_t clientaddrlen;
  rrh_UE_desc_t *rrh_UE_rx_desc = (rrh_UE_desc_t *)arg;
  struct timespec time0,time1,time2;
  struct timespec time_req_1us, time_rem_1us;
  ssize_t bytes_sent;
  int antenna_index, nsamps;
  int trace_cnt=0;
  int sockid;
  char str[INET_ADDRSTRLEN];
  unsigned long long max_rx_time=0, min_rx_time=133333, total_rx_time=0, average_rx_time=133333, s_period=0, trial=0;

  openair0_timestamp temp, last_hw_counter=0;

  antenna_index =  rrh_UE_rx_desc->antenna_index_UE_rx;
  nsamps = rrh_UE_rx_desc->nsamps;
  sockid = rrh_UE_rx_desc->sockid_UE;


  bzero((void*)&clientaddr,sizeof(struct sockaddr));
  clientaddrlen = sizeof(struct sockaddr);
  //printf("Waiting for eNB ...\n");
  //printf("[RRH eNB RX thread] received parameters : nsamps : %d, antenna_index : %d\n",nsamps,antenna_index);
  clientaddr = rrh_UE_rx_desc->clientaddr;
  clientaddrlen = rrh_UE_rx_desc->clientaddrlen;

  inet_ntop(AF_INET, &(((struct sockaddr_in*)&clientaddr)->sin_addr), str, INET_ADDRSTRLEN);
  printf("Received UE RX request for antenna %d, nsamps %d (from %s:%d)\n",antenna_index,nsamps,str,
         ntohs(((struct sockaddr_in*)&clientaddr)->sin_port));

  while (rrh_exit == 0) {
    if (!UE_rx_started) {
      UE_rx_started=1;  //Set this flag to 1 to indicate that a UE started retrieving data

      if (RT_FLAG==1) {
        last_hw_counter=hw_counter;
      }
    } else {
      if (RT_FLAG==1) {
        if (hw_counter > last_hw_counter+1) {
          printf("L1");
          //              goto end_copy_UE;
        } else {
          while (hw_counter < last_hw_counter+1)
            nanosleep(&time_req_1us,&time_rem_1us);
        }
      }
    }

    clock_gettime(CLOCK_MONOTONIC,&time1);

    // send return
    temp=*(openair0_timestamp*)&rx_buffer_UE[antenna_index][timestamp_UE_rx[antenna_index]%(FRAME_MAX_SIZE)];
    *(openair0_timestamp*)&rx_buffer_UE[antenna_index][timestamp_UE_rx[antenna_index]%(FRAME_MAX_SIZE)]=timestamp_UE_rx[antenna_index];

    if ((timestamp_UE_rx[antenna_index]%(FRAME_MAX_SIZE)+nsamps) > FRAME_MAX_SIZE) { // Wrap around if nsamps exceeds the buffer limit
      if (((timestamp_eNB_tx[antenna_index]%(FRAME_MAX_SIZE)) < ((timestamp_UE_rx[antenna_index]+nsamps)%(FRAME_MAX_SIZE))) && (eNB_tx_started==1)) {
        printf("UE underflow wraparound timestamp_UE_rx : %d, timestamp_eNB_tx : %d\n",(int)(timestamp_UE_rx[antenna_index]%(FRAME_MAX_SIZE)),(int)(timestamp_eNB_tx[antenna_index]%FRAME_MAX_SIZE));

        if (NRT_FLAG==1) {
          while ((timestamp_eNB_tx[antenna_index]%FRAME_MAX_SIZE) < ((timestamp_UE_rx[antenna_index]+nsamps)%(FRAME_MAX_SIZE)))
            nanosleep(&time_req_1us,&time_rem_1us);
        }
      }

      if ((bytes_sent = sendto(sockid,
                               &rx_buffer_UE[antenna_index][timestamp_UE_rx[antenna_index]%(FRAME_MAX_SIZE)],
                               ((FRAME_MAX_SIZE)<<2) - ((timestamp_UE_rx[antenna_index]%(FRAME_MAX_SIZE))<<2) + sizeof(openair0_timestamp),
                               0,
                               (struct sockaddr*)&clientaddr,
                               sizeof(struct sockaddr)))<0)
        perror("RRH UE : sendto for RX");

      if ((bytes_sent = sendto(sockid,
                               &rx_buffer_UE[antenna_index][0],
                               (nsamps<<2) - ((FRAME_MAX_SIZE)<<2) + ((timestamp_UE_rx[antenna_index]%(FRAME_MAX_SIZE))<<2),
                               0,
                               (struct sockaddr*)&clientaddr,
                               sizeof(struct sockaddr)))<0)
        perror("RRH UE : sendto for RX");
    } else {
      if (((timestamp_UE_rx[antenna_index]%FRAME_MAX_SIZE)< timestamp_eNB_tx[antenna_index]%FRAME_MAX_SIZE)
          && (((timestamp_UE_rx[antenna_index]+nsamps)%FRAME_MAX_SIZE) > (timestamp_eNB_tx[antenna_index]%FRAME_MAX_SIZE)) && (eNB_tx_started==1) ) {
        printf("UE underflow timestamp_UE_rx : %d, timestamp_eNB_tx : %d\n",(int)(timestamp_UE_rx[antenna_index]%FRAME_MAX_SIZE),(int)(timestamp_eNB_tx[antenna_index]%FRAME_MAX_SIZE));

        if (NRT_FLAG==1) {
          while (((timestamp_UE_rx[antenna_index]+nsamps)%FRAME_MAX_SIZE) > (timestamp_eNB_tx[antenna_index]%FRAME_MAX_SIZE)) {
            nanosleep(&time_req_1us,&time_rem_1us);
          }
        }
      }


      if ((bytes_sent = sendto(sockid,
                               &rx_buffer_UE[antenna_index][timestamp_UE_rx[antenna_index]%(FRAME_MAX_SIZE)],
                               (nsamps<<2)+sizeof(openair0_timestamp),
                               0,
                               (struct sockaddr*)&clientaddr,
                               sizeof(struct sockaddr)))<0)
        perror("RRH UE thread: sendto for RX");
    }

    //printf("bytes_sent %d(timestamp_UE_rx[%d] %d)\n",(int)bytes_sent,antenna_index,(int)timestamp_UE_rx[antenna_index]);
    *(openair0_timestamp*)&rx_buffer_UE[antenna_index][timestamp_UE_rx[antenna_index]%(FRAME_MAX_SIZE)]=temp;
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
        printf("Average UE RX time : %lu\tMax RX time : %lu\tMin RX time : %lu\n",average_rx_time,max_rx_time,min_rx_time);

      }

      //printf("RX: t1 %llu (time from last send), t2 %llu (sendto of %lu bytes) total time : %llu\n",(long long unsigned int)(time1.tv_nsec  - time0.tv_nsec), (long long unsigned int)(time2.tv_nsec - time1.tv_nsec),
      //   (nsamps<<2)+sizeof(openair0_timestamp),(long long unsigned int)(time2.tv_nsec - time0.tv_nsec));

    }

    memcpy(&time0,&time2,sizeof(struct timespec));


  }  //while (rrh_exit==0)

  return(0);

}


void *rrh_UE_tx_thread(void *arg)
{
  struct sockaddr clientaddr;
  socklen_t clientaddrlen;
  struct timespec time0a,time0,time1,time2;
  rrh_UE_desc_t *rrh_UE_tx_desc = (rrh_UE_desc_t *)arg;
  //  struct timespec time_rem;
  struct timespec time_req_1us, time_rem_1us;
  ssize_t bytes_received;
  int antenna_index, nsamps;
  int8_t buf[BUF_LEN];
  int trace_cnt=0;
  int sockid;

  bzero((void*)&clientaddr,sizeof(struct sockaddr));
  clientaddrlen = sizeof(struct sockaddr);

  antenna_index =  rrh_UE_tx_desc->antenna_index_UE_tx;
  nsamps = rrh_UE_tx_desc->nsamps;
  sockid = rrh_UE_tx_desc->sockid_UE;

  while (rrh_exit == 0) {

    clock_gettime(CLOCK_MONOTONIC,&time0a);

    bytes_received = recvfrom(sockid,buf,BUF_LEN,0,&clientaddr,&clientaddrlen);
    timestamp_eNB_tx[antenna_index] = *(openair0_timestamp*)(buf+4);

    clock_gettime(CLOCK_MONOTONIC,&time1);

    if (NRT_FLAG==1) {
      nrt_UE_counter[antenna_index]++;
    }

    //printf("Received UE TX request for antenna %d, nsamps %d, timestamp %d bytes_received %d\n",antenna_index,nsamps,(int)timestamp_UE_tx[antenna_index],(int)bytes_received);
    if ((timestamp_UE_tx[antenna_index]%(FRAME_MAX_SIZE)+nsamps) > FRAME_MAX_SIZE) { // Wrap around if nsamps exceeds the buffer limit
      memcpy(&tx_buffer_UE[antenna_index][timestamp_UE_tx[antenna_index]%(FRAME_MAX_SIZE)],buf+sizeof(openair0_timestamp)+2*sizeof(int16_t),
             (FRAME_MAX_SIZE<<2)-((timestamp_UE_tx[antenna_index]%(FRAME_MAX_SIZE))<<2));
      memcpy(&tx_buffer_UE[antenna_index][0],buf+sizeof(openair0_timestamp)+2*sizeof(int16_t)+(FRAME_MAX_SIZE*4)-((timestamp_UE_tx[antenna_index]%(FRAME_MAX_SIZE))<<2),
             (nsamps<<2)-((FRAME_MAX_SIZE-(timestamp_UE_tx[antenna_index]%(FRAME_MAX_SIZE)))<<2));
      //printf("Received UE TX samples for antenna %d, nsamps %d (%d)\n",antenna_index,nsamps,(int)(bytes_received>>2));
    } else {
      memcpy(&tx_buffer_UE[antenna_index][timestamp_UE_tx[antenna_index]%(FRAME_MAX_SIZE)],buf+sizeof(openair0_timestamp)+2*sizeof(int16_t),(nsamps<<2));
    }

    //printf("Received UE TX samples for antenna %d, nsamps %d (%d)\n",antenna_index,nsamps,(int)(bytes_received>>2));

    while (sync_UE_rx[antenna_index]==0)
      nanosleep(&time_req_1us,&time_rem_1us);

    pthread_mutex_lock(&sync_UE_mutex[antenna_index]);
    sync_UE_rx[antenna_index]++;

    if (!sync_UE_rx[antenna_index]) {
      counter_UE_tx[antenna_index]=(counter_UE_tx[antenna_index]+nsamps)%FRAME_MAX_SIZE;
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



void *rrh_UE_thread(void *arg)
{

  int sockid=-1;
  struct sockaddr_in serveraddr;
  struct sockaddr clientaddr;
  socklen_t clientaddrlen;
  rrh_desc_t *rrh_desc = (rrh_desc_t *)arg;
  char str[INET_ADDRSTRLEN];
  //int8_t msg_header[4+sizeof(openair0_timestamp)];
  int8_t buf[BUF_LEN];
  int16_t cmd;   //,nsamps,antenna_index;
  ssize_t bytes_received;
  //  struct timespec time_rem;
  //  ssize_t bytes_sent;
  //  openair0_timestamp temp;
  //  openair0_timestamp last_hw_counter=0;
  struct timespec time_req_1us, time_rem_1us;

  rrh_UE_desc_t rrh_UE_desc;
  pthread_t UE_rx_thread, UE_tx_thread;
  pthread_attr_t attr_UE_rx, attr_UE_tx;
  struct sched_param sched_param_UE_rx, sched_param_UE_tx;
  int error_code_UE_rx, error_code_UE_tx;

  time_req_1us.tv_sec = 0;
  time_req_1us.tv_nsec = 1000;

  while (rrh_exit==0) {


    sockid=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

    if (sockid==-1) {
      perror("Cannot create UE socket: ");
      rrh_exit=1;
    }


    bzero((char *)&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(rrh_desc->UE_port);
    inet_pton(AF_INET,rrh_desc->UE_dest_ip,&serveraddr.sin_addr.s_addr);

    inet_ntop(AF_INET, &(serveraddr.sin_addr), str, INET_ADDRSTRLEN);
    printf("Binding to UE socket for %s:%d\n",str,ntohs(serveraddr.sin_port));

    if (bind(sockid,(struct sockaddr *)&serveraddr,sizeof(serveraddr))<0) {
      perror("Cannot bind to UE socket: ");
      rrh_exit = 1;
    }

    // now wait for commands from eNB

    // get header info
    bzero((void*)&clientaddr,sizeof(struct sockaddr));
    clientaddrlen = sizeof(struct sockaddr);
    //printf("Waiting for UE ...\n");

    bytes_received = recvfrom(sockid,buf,BUF_LEN,0,&clientaddr,&clientaddrlen);
    cmd = buf[0];

    rrh_UE_desc.antenna_index_UE_rx = cmd>>1;
    rrh_UE_desc.antenna_index_UE_tx = cmd>>1;
    //rrh_eNB_desc.timestamp_eNB_tx[rrh_eNB_desc.antenna_index_eNB_tx] = *(openair0_timestamp*)(buf+4);  //we don't need timestamp when receiving the first command
    rrh_UE_desc.nsamps = *(int16_t *)(buf+2);
    rrh_UE_desc.sockid_UE = sockid;
    rrh_UE_desc.clientaddr = clientaddr;
    rrh_UE_desc.clientaddrlen = clientaddrlen;

    cmd = cmd&1;
    inet_ntop(AF_INET, &(((struct sockaddr_in*)&clientaddr)->sin_addr), str, INET_ADDRSTRLEN);

    if (cmd==START_CMD) {

      pthread_attr_init(&attr_UE_rx);
      pthread_attr_init(&attr_UE_tx);
      sched_param_UE_rx.sched_priority = sched_get_priority_max(SCHED_FIFO);
      sched_param_UE_tx.sched_priority = sched_get_priority_max(SCHED_FIFO);
      pthread_attr_setschedparam(&attr_UE_rx,&sched_param_UE_rx);
      pthread_attr_setschedparam(&attr_UE_tx,&sched_param_UE_tx);
      pthread_attr_setschedpolicy(&attr_UE_rx,SCHED_FIFO);
      pthread_attr_setschedpolicy(&attr_UE_tx,SCHED_FIFO);

      error_code_UE_rx = pthread_create(&UE_rx_thread, &attr_UE_rx, rrh_UE_rx_thread, (void *)&rrh_UE_desc);
      error_code_UE_tx = pthread_create(&UE_tx_thread, &attr_UE_tx, rrh_UE_tx_thread, (void *)&rrh_UE_desc);

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

  close(sockid);

  rrh_UE_thread_status = 0;
  pthread_exit(&rrh_UE_thread_status);

  return(0);
}



void *rrh_eNB_rx_thread(void *arg)
{

  struct sockaddr clientaddr;
  socklen_t clientaddrlen;
  rrh_eNB_desc_t *rrh_eNB_rx_desc = (rrh_eNB_desc_t *)arg;
  struct timespec time0,time1,time2;
  struct timespec time_req_1us, time_rem_1us;
  ssize_t bytes_sent;
  int antenna_index, nsamps;
  int trace_cnt=0;
  int sockid;
  char str[INET_ADDRSTRLEN];
  unsigned long long max_rx_time=0, min_rx_time=133333, total_rx_time=0, average_rx_time=133333, s_period=0, trial=0;

  openair0_timestamp temp, last_hw_counter=0;

  antenna_index =  rrh_eNB_rx_desc->antenna_index_eNB_rx;
  nsamps = rrh_eNB_rx_desc->nsamps;
  sockid = rrh_eNB_rx_desc->sockid_eNB;


  bzero((void*)&clientaddr,sizeof(struct sockaddr));
  clientaddrlen = sizeof(struct sockaddr);
  //printf("Waiting for eNB ...\n");
  //printf("[RRH eNB RX thread] received parameters : nsamps : %d, antenna_index : %d\n",nsamps,antenna_index);
  clientaddr = rrh_eNB_rx_desc->clientaddr;
  clientaddrlen = rrh_eNB_rx_desc->clientaddrlen;

  inet_ntop(AF_INET, &(((struct sockaddr_in*)&clientaddr)->sin_addr), str, INET_ADDRSTRLEN);
  printf("Received eNB RX request for antenna %d, nsamps %d (from %s:%d)\n",antenna_index,nsamps,str,
         ntohs(((struct sockaddr_in*)&clientaddr)->sin_port));

  while (rrh_exit == 0) {
    //printf("Received eNB RX request for antenna %d, nsamps %d (from %s:%d)\n",antenna_index,nsamps,str,
    //         ntohs(((struct sockaddr_in*)&clientaddr)->sin_port));

    if (!eNB_rx_started) {
      eNB_rx_started=1; // set this flag to 1 to indicate that eNB started

      if (RT_FLAG==1) {
        last_hw_counter=hw_counter;
      }
    } else {
      if (RT_FLAG==1) {
        if (hw_counter > last_hw_counter+1) {
          printf("L");
          //      goto end_copy_eNB;
        } else {
          while (hw_counter < last_hw_counter+1)
            nanosleep(&time_req_1us,&time_rem_1us);
        }
      }
    }

    clock_gettime(CLOCK_MONOTONIC,&time1);

    // send return
    temp=*(openair0_timestamp*)&rx_buffer_eNB[antenna_index][timestamp_eNB_rx[antenna_index]%(FRAME_MAX_SIZE)];
    *(openair0_timestamp*)&rx_buffer_eNB[antenna_index][timestamp_eNB_rx[antenna_index]%(FRAME_MAX_SIZE)]=timestamp_eNB_rx[antenna_index];

    if ((timestamp_eNB_rx[antenna_index]%(FRAME_MAX_SIZE)+nsamps) > FRAME_MAX_SIZE) { // Wrap around if nsamps exceeds the buffer limit
      if ((timestamp_UE_tx[antenna_index]%FRAME_MAX_SIZE < ((timestamp_eNB_rx[antenna_index]+nsamps)%FRAME_MAX_SIZE)) && (UE_tx_started==1)) {
        printf("eNB underflow\n");

        if (NRT_FLAG==1) {
          while ((timestamp_UE_tx[antenna_index]%FRAME_MAX_SIZE) < nsamps)
            nanosleep(&time_req_1us,&time_rem_1us);
        }
      }


      if ((bytes_sent = sendto(sockid,
                               &rx_buffer_eNB[antenna_index][timestamp_eNB_rx[antenna_index]%(FRAME_MAX_SIZE)],
                               ((FRAME_MAX_SIZE)<<2) - ((timestamp_eNB_rx[antenna_index]%(FRAME_MAX_SIZE))<<2) + sizeof(openair0_timestamp),
                               0,
                               (struct sockaddr*)&clientaddr,
                               sizeof(struct sockaddr)))<0)
        perror("RRH eNB : sendto for RX");

      if ((bytes_sent = sendto(sockid,
                               &rx_buffer_eNB[antenna_index][0],
                               (nsamps<<2) - ((FRAME_MAX_SIZE)<<2) + ((timestamp_eNB_rx[antenna_index]%(FRAME_MAX_SIZE))<<2),
                               0,
                               (struct sockaddr*)&clientaddr,
                               sizeof(struct sockaddr)))<0)
        perror("RRH eNB : sendto for RX");
    } else {
      if (((timestamp_eNB_rx[antenna_index]%FRAME_MAX_SIZE)< timestamp_UE_tx[antenna_index]%FRAME_MAX_SIZE)
          && (((timestamp_eNB_rx[antenna_index]+nsamps)%FRAME_MAX_SIZE) > (timestamp_UE_tx[antenna_index]%FRAME_MAX_SIZE)) && (UE_tx_started==1)) {
        printf("eNB underflow\n");

        if (NRT_FLAG==1) {
          while (((timestamp_eNB_rx[antenna_index]+nsamps)%FRAME_MAX_SIZE) > (timestamp_UE_tx[antenna_index]%FRAME_MAX_SIZE))
            nanosleep(&time_req_1us,&time_rem_1us);
        }
      }

      if ((bytes_sent = sendto(sockid,
                               &rx_buffer_eNB[antenna_index][timestamp_eNB_rx[antenna_index]%(FRAME_MAX_SIZE)],
                               (nsamps<<2)+sizeof(openair0_timestamp),
                               0,
                               (struct sockaddr*)&clientaddr,
                               sizeof(struct sockaddr)))<0)
        perror("RRH eNB : sendto for RX");

    }

    //printf("bytes_sent %d(timestamp_eNB_rx[%d] %d)\n",(int)bytes_sent,antenna_index,(int)timestamp_eNB_rx[antenna_index]);


    *(openair0_timestamp*)&rx_buffer_eNB[antenna_index][timestamp_eNB_rx[antenna_index]%(FRAME_MAX_SIZE)]=temp;
    timestamp_eNB_rx[antenna_index]+=nsamps;
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
        printf("Average eNB RX time : %lu\tMax RX time : %lu\tMin RX time : %lu\n",average_rx_time,max_rx_time,min_rx_time);

      }

      //printf("RX: t1 %llu (time from last send), t2 %llu (sendto of %lu bytes) total time : %llu\n",(long long unsigned int)(time1.tv_nsec  - time0.tv_nsec), (long long unsigned int)(time2.tv_nsec - time1.tv_nsec),
      //   (nsamps<<2)+sizeof(openair0_timestamp),(long long unsigned int)(time2.tv_nsec - time0.tv_nsec));

    }

    memcpy(&time0,&time2,sizeof(struct timespec));


  }  //while (rrh_exit==0)

  return(0);

}


void *rrh_eNB_tx_thread(void *arg)
{

  struct sockaddr clientaddr;
  socklen_t clientaddrlen;
  struct timespec time0a,time0,time1,time2;
  rrh_eNB_desc_t *rrh_eNB_tx_desc = (rrh_eNB_desc_t *)arg;
  //  struct timespec time_rem;
  struct timespec time_req_1us, time_rem_1us;
  ssize_t bytes_received;
  int antenna_index, nsamps;
  int8_t buf[BUF_LEN];
  int trace_cnt=0;
  int sockid;

  bzero((void*)&clientaddr,sizeof(struct sockaddr));
  clientaddrlen = sizeof(struct sockaddr);
  //printf("Waiting for eNB ...\n");

  antenna_index =  rrh_eNB_tx_desc->antenna_index_eNB_tx;
  nsamps = rrh_eNB_tx_desc->nsamps;
  sockid = rrh_eNB_tx_desc->sockid_eNB;

  while (rrh_exit == 0) {

    clock_gettime(CLOCK_MONOTONIC,&time0a);

    bytes_received = recvfrom(sockid,buf,BUF_LEN,0,&clientaddr,&clientaddrlen);
    timestamp_eNB_tx[antenna_index] = *(openair0_timestamp*)(buf+4);

    clock_gettime(CLOCK_MONOTONIC,&time1);

    if (NRT_FLAG==1) {
      nrt_eNB_counter[antenna_index]++;
    }

    //printf("Received eNB TX request for antenna %d, nsamps %d, timestamp %d bytes_received %d\n",antenna_index,nsamps,(int)timestamp_eNB_tx[antenna_index],(int)bytes_received);

    if ((timestamp_eNB_tx[antenna_index]%(FRAME_MAX_SIZE)+nsamps) > FRAME_MAX_SIZE) { // Wrap around if nsamps exceeds the buffer limit
      memcpy(&tx_buffer_eNB[antenna_index][timestamp_eNB_tx[antenna_index]%(FRAME_MAX_SIZE)],buf+sizeof(openair0_timestamp)+2*sizeof(int16_t),
             ((FRAME_MAX_SIZE)<<2)-((timestamp_eNB_tx[antenna_index]%(FRAME_MAX_SIZE))<<2));
      //printf("Done first part size : %d\n",(int32_t)(((FRAME_MAX_SIZE)<<2)-((timestamp_eNB_tx[antenna_index]%(FRAME_MAX_SIZE))<<2)));
      memcpy(&tx_buffer_eNB[antenna_index][0],buf+sizeof(openair0_timestamp)+2*sizeof(int16_t) + ((FRAME_MAX_SIZE)<<2) -((timestamp_eNB_tx[antenna_index]%(FRAME_MAX_SIZE))<<2),
             (nsamps<<2)-((FRAME_MAX_SIZE)<<2)+((timestamp_eNB_tx[antenna_index]%(FRAME_MAX_SIZE))<<2));
      //printf("Received eNB TX samples for antenna %d, nsamps %d (%d)\n",antenna_index,nsamps,(int)(bytes_received>>2));
    } else {
      memcpy(&tx_buffer_eNB[antenna_index][timestamp_eNB_tx[antenna_index]%(FRAME_MAX_SIZE)],buf+sizeof(openair0_timestamp)+2*sizeof(int16_t),nsamps<<2);
    }

    while (sync_eNB_rx[antenna_index]==0)
      nanosleep(&time_req_1us,&time_rem_1us);

    //printf("launching eNB proc\n");
    pthread_mutex_lock(&sync_eNB_mutex[antenna_index]);
    sync_eNB_rx[antenna_index]++;

    if (!sync_eNB_rx[antenna_index]) {
      counter_eNB_tx[antenna_index]=(counter_eNB_tx[antenna_index]+nsamps)%FRAME_MAX_SIZE;
      nsamps_eNB[antenna_index]=nsamps;
    } else {
      printf("rrh_eNB_proc thread is busy, will exit\n");
      exit(-1);
    }

    pthread_mutex_unlock(&sync_eNB_mutex[antenna_index]);

    clock_gettime(CLOCK_MONOTONIC,&time2);

    //if (trace_cnt++ < 10)
    //  printf("TX: t0 %llu (time from previous run) t1 %llu (time of memcpy TX samples), t2 %llu (total time)\n",(long long unsigned int)(time1.tv_nsec  - time0.tv_nsec),
    //   (long long unsigned int)(time2.tv_nsec  - time1.tv_nsec),
    //   (long long unsigned int)(time2.tv_nsec - time0.tv_nsec));

    memcpy(&time0,&time2,sizeof(struct timespec));



  }

  return(0);

}

void *rrh_eNB_thread(void *arg)
{

  int sockid=-1;
  struct sockaddr_in serveraddr;
  struct sockaddr clientaddr;
  socklen_t clientaddrlen;
  rrh_desc_t *rrh_desc = (rrh_desc_t *)arg;
  char str[INET_ADDRSTRLEN];
  //int8_t msg_header[4+sizeof(openair0_timestamp)];
  int8_t buf[BUF_LEN];
  int16_t cmd;   //,nsamps,antenna_index;
  ssize_t bytes_received;
  //ssize_t bytes_sent;
  //openair0_timestamp temp, last_hw_counter=0;
  //struct timespec time_rem;
  struct timespec time_req_1us, time_rem_1us;
  //struct timespec time0a,time0,time1,time2;
  //int trace_cnt=0;
  rrh_eNB_desc_t rrh_eNB_desc;
  pthread_t eNB_rx_thread, eNB_tx_thread;
  pthread_attr_t attr_eNB_rx, attr_eNB_tx;
  struct sched_param sched_param_eNB_rx, sched_param_eNB_tx;
  int error_code_eNB_rx, error_code_eNB_tx;

  time_req_1us.tv_sec = 0;
  time_req_1us.tv_nsec = 1000;


  while (rrh_exit==0) {


    sockid=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

    if (sockid==-1) {
      perror("Cannot create eNB socket: ");
      rrh_exit=1;
    }


    bzero((char *)&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(rrh_desc->eNB_port);
    inet_pton(AF_INET,rrh_desc->eNB_dest_ip,&serveraddr.sin_addr.s_addr);

    inet_ntop(AF_INET, &(serveraddr.sin_addr), str, INET_ADDRSTRLEN);
    printf("Binding to eNB socket for %s:%d\n",str,ntohs(serveraddr.sin_port));

    if (bind(sockid,(struct sockaddr *)&serveraddr,sizeof(serveraddr))<0) {
      perror("Cannot bind to eNB socket: ");
      rrh_exit = 1;
    }

    // now wait for commands from eNB

    // get header info
    bzero((void*)&clientaddr,sizeof(struct sockaddr));
    clientaddrlen = sizeof(struct sockaddr);
    //printf("Waiting for eNB ...\n");



    bytes_received = recvfrom(sockid,buf,BUF_LEN,0,&clientaddr,&clientaddrlen);
    cmd = buf[0];
    rrh_eNB_desc.antenna_index_eNB_rx = cmd>>1;
    rrh_eNB_desc.antenna_index_eNB_tx = cmd>>1;
    //rrh_eNB_desc.timestamp_eNB_tx[rrh_eNB_desc.antenna_index_eNB_tx] = *(openair0_timestamp*)(buf+4);  //we don't need timestamp when receiving the first command
    rrh_eNB_desc.nsamps = *(int16_t *)(buf+2);
    rrh_eNB_desc.sockid_eNB = sockid;
    rrh_eNB_desc.clientaddr = clientaddr;
    rrh_eNB_desc.clientaddrlen = clientaddrlen;

    //cmd = cmd&1;
    cmd = cmd|1;//in order to make cmd evalution dummy (the first message from lte to rrh has changed, see: @ethernet_lib.c trx_start_func has been substituted by trx_request_func )
    inet_ntop(AF_INET, &(((struct sockaddr_in*)&clientaddr)->sin_addr), str, INET_ADDRSTRLEN);
    
    if (cmd==START_CMD) {

      pthread_attr_init(&attr_eNB_rx);
      pthread_attr_init(&attr_eNB_tx);
      sched_param_eNB_rx.sched_priority = sched_get_priority_max(SCHED_FIFO);
      sched_param_eNB_tx.sched_priority = sched_get_priority_max(SCHED_FIFO);
      pthread_attr_setschedparam(&attr_eNB_rx,&sched_param_eNB_rx);
      pthread_attr_setschedparam(&attr_eNB_tx,&sched_param_eNB_tx);
      pthread_attr_setschedpolicy(&attr_eNB_rx,SCHED_FIFO);
      pthread_attr_setschedpolicy(&attr_eNB_tx,SCHED_FIFO);

      error_code_eNB_rx = pthread_create(&eNB_rx_thread, &attr_eNB_rx, rrh_eNB_rx_thread, (void *)&rrh_eNB_desc);
      error_code_eNB_tx = pthread_create(&eNB_tx_thread, &attr_eNB_tx, rrh_eNB_tx_thread, (void *)&rrh_eNB_desc);

      if (error_code_eNB_rx) {
        printf("Error while creating eNB RX thread\n");
        exit(-1);
      }

      if (error_code_eNB_tx) {
        printf("Error while creating eNB TX thread\n");
        exit(-1);
      }

      while (rrh_exit==0)
        sleep(1);

    }


  }  //while (rrh_exit==0)

  close(sockid);

  rrh_eNB_thread_status = 0;
  pthread_exit(&rrh_eNB_thread_status);

  return(0);
}


void signal_handler(int sig)
{
  void *array[10];
  size_t size;

  if (sig==SIGSEGV) {
    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, 2);
    exit(-1);
  } else {
    printf("trying to exit gracefully...\n");
    rrh_exit = 1;
  }
}

int main(int argc, char **argv)
{

  pthread_t main_rrh_eNB_thread, main_rrh_UE_thread, main_rrh_proc_eNB_thread, main_rrh_proc_UE_thread, main_timer_proc_thread;
  pthread_attr_t attr, attr_proc, attr_timer;
  struct sched_param sched_param_rrh, sched_param_rrh_proc, sched_param_timer;
  int error_code_eNB, error_code_UE, error_code_proc_eNB, error_code_proc_UE, error_code_timer;
  int i;
  int opt;
  int nsecs=0, rt_period=0;
  rrh_desc_t rrh;
  struct itimerspec timer;


  rrh.time_req.tv_sec = 0;
  rrh.time_req.tv_nsec = 0;

  rrh.eNB_port = RRH_eNB_PORT;
  strcpy(rrh.eNB_dest_ip,RRH_eNB_DEST_IP);
  rrh.UE_port = RRH_UE_PORT;
  strcpy(rrh.UE_dest_ip ,RRH_UE_DEST_IP);

  nsecs = 0;

  while ((opt = getopt(argc, argv, "t:rE:U:")) != -1) {
    switch (opt) {
    case 't':
      rt_period = atoi(optarg);
      RT_FLAG=1;
      NRT_FLAG=0;
      break;

    case 'r':
      rt_period = DEFAULT_PERIOD_NS;
      RT_FLAG=1;
      NRT_FLAG=0;
      break;

    case 'E':
      strcpy(rrh.eNB_dest_ip,optarg);
      break;

    case 'U':
      strcpy(rrh.UE_dest_ip,optarg);
      break;

    default: /* '?' */
      fprintf(stderr, "Usage: %s [-d nsecs]\n", argv[0]);
      exit(-1);
    }
  }

  //          if (optind >= argc) {
  //              fprintf(stderr, "Expected argument after options\n");
  //              exit(EXIT_FAILURE);
  //          }

  // setup the timer (1s delay, 1s reload)
  timer.it_value.tv_sec = rt_period/1000000000;
  timer.it_value.tv_nsec = rt_period%1000000000;
  timer.it_interval.tv_sec = rt_period/1000000000;
  timer.it_interval.tv_nsec = rt_period%1000000000;


  // to make a graceful exit when ctrl-c is pressed
  signal(SIGSEGV, signal_handler);
  signal(SIGINT, signal_handler);


  pthread_attr_init(&attr);
  sched_param_rrh.sched_priority = sched_get_priority_max(SCHED_FIFO);
  pthread_attr_init(&attr_proc);
  sched_param_rrh_proc.sched_priority = sched_get_priority_max(SCHED_FIFO-1);
  pthread_attr_init(&attr_timer);
  sched_param_timer.sched_priority = sched_get_priority_max(SCHED_FIFO-2);

  pthread_attr_setschedparam(&attr,&sched_param_rrh);
  pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
  pthread_attr_setschedparam(&attr_proc,&sched_param_rrh_proc);
  pthread_attr_setschedpolicy(&attr_proc,SCHED_FIFO-1);
  pthread_attr_setschedparam(&attr_timer,&sched_param_timer);
  pthread_attr_setschedpolicy(&attr_timer,SCHED_FIFO-2);

  for (i=0; i<4; i++) {
    pthread_mutex_init(&sync_eNB_mutex[i],NULL);
    pthread_cond_init(&sync_eNB_cond[i],NULL);
    pthread_mutex_init(&sync_UE_mutex[i],NULL);
    pthread_cond_init(&sync_UE_cond[i],NULL);
  }

  pthread_mutex_init(&timer_mutex,NULL);

  error_code_eNB = pthread_create(&main_rrh_eNB_thread, &attr, rrh_eNB_thread, (void *)&rrh);
  error_code_UE = pthread_create(&main_rrh_UE_thread, &attr, rrh_UE_thread, (void *)&rrh);
  error_code_proc_UE = pthread_create(&main_rrh_proc_UE_thread, &attr_proc, rrh_proc_UE_thread, NULL);
  error_code_proc_eNB = pthread_create(&main_rrh_proc_eNB_thread, &attr_proc, rrh_proc_eNB_thread, NULL);
  error_code_timer = pthread_create(&main_timer_proc_thread, &attr_timer, timer_proc, (void *)&timer);

  if (error_code_eNB) {
    printf("Error while creating eNB thread\n");
    exit(-1);
  }

  if (error_code_UE) {
    printf("Error while creating UE thread\n");
    exit(-1);
  }

  if (error_code_proc_UE) {
    printf("Error while creating UE proc thread\n");
    exit(-1);
  }

  if (error_code_proc_eNB) {
    printf("Error while creating eNB proc thread\n");
    exit(-1);
  }

  if (error_code_timer) {
    printf("Error while creating timer proc thread\n");
    exit(-1);
  }

  printf("TYPE <CTRL-C> TO TERMINATE\n");

  while (rrh_exit==0)
    sleep(1);


  //pthread_join(main_rrh_eNB_thread,&rrh_eNB_thread_status);
  return 0;
}
