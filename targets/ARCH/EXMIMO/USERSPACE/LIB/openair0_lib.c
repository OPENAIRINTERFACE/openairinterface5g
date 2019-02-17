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

/** openair0_lib : API to interface with ExpressMIMO-1&2 kernel driver
*
*  Authors: Matthias Ihmig <matthias.ihmig@mytum.de>, 2013
*           Raymond Knopp <raymond.knopp@eurecom.fr>
*
*  Changelog:
*  28.01.2013: Initial version
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h> 
#include <sys/types.h>
#include <sys/mman.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <syscall.h>

#include "openair0_lib.h"
#include "openair_device.h"
#include "common_lib.h"

#include <pthread.h>


#define max(a,b) ((a)>(b) ? (a) : (b))

//#define DEBUG_EXMIMO

exmimo_pci_interface_bot_virtual_t openair0_exmimo_pci[MAX_CARDS]; // contains userspace pointers for each card

char *bigshm_top[MAX_CARDS];

int openair0_fd;
int openair0_num_antennas[MAX_CARDS];
int openair0_num_detected_cards = 0;

unsigned int PAGE_SHIFT;

static uint32_t                      rf_local[4] =       {8255000,8255000,8255000,8255000}; // UE zepto
//{8254617, 8254617, 8254617, 8254617}; //eNB khalifa
//{8255067,8254810,8257340,8257340}; // eNB PETRONAS

static uint32_t                      rf_vcocal[4] =      {910,910,910,910};
static uint32_t                      rf_vcocal_850[4] =  {2015, 2015, 2015, 2015};
static uint32_t                      rf_rxdc[4] =        {32896,32896,32896,32896};



extern volatile int                    oai_exit;


void kill_watchdog(openair0_device *);
void create_watchdog(openair0_device *);
void rt_sleep(struct timespec *,long );

unsigned int log2_int( unsigned int x )
{
  unsigned int ans = 0 ;

  while( x>>=1 ) ans++;

  return ans ;
}

int openair0_open(void)
{
  exmimo_pci_interface_bot_virtual_t exmimo_pci_kvirt[MAX_CARDS];
  void *bigshm_top_kvirtptr[MAX_CARDS];

  int card;
  int ant;

  PAGE_SHIFT = log2_int( sysconf( _SC_PAGESIZE ) );


  if ((openair0_fd = open("/dev/openair0", O_RDWR,0)) <0) {
    return -1;
  }

  ioctl(openair0_fd, openair_GET_NUM_DETECTED_CARDS, &openair0_num_detected_cards);



  if ( openair0_num_detected_cards == 0 ) {
    fprintf(stderr, "No cards detected!\n");
    return -4;
  }

  ioctl(openair0_fd, openair_GET_BIGSHMTOPS_KVIRT, &bigshm_top_kvirtptr[0]);
  ioctl(openair0_fd, openair_GET_PCI_INTERFACE_BOTS_KVIRT, &exmimo_pci_kvirt[0]);

  //printf("bigshm_top_kvirtptr (MAX_CARDS %d): %p  %p  %p  %p\n", MAX_CARDS,bigshm_top_kvirtptr[0], bigshm_top_kvirtptr[1], bigshm_top_kvirtptr[2], bigshm_top_kvirtptr[3]);

  for( card=0; card < MAX_CARDS; card++)
    bigshm_top[card] = NULL;

  for( card=0; card < openair0_num_detected_cards; card++) {
    bigshm_top[card] = (char *)mmap( NULL,
                                     BIGSHM_SIZE_PAGES<<PAGE_SHIFT,
                                     PROT_READ|PROT_WRITE,
                                     MAP_SHARED, //|MAP_FIXED,//MAP_SHARED,
                                     openair0_fd,
                                     ( openair_mmap_BIGSHM | openair_mmap_Card(card) )<<PAGE_SHIFT);

    if (bigshm_top[card] == MAP_FAILED) {
      openair0_close();
      return -2;
    }

    // calculate userspace addresses
#if __x86_64
    openair0_exmimo_pci[card].firmware_block_ptr = (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[0].firmware_block_ptr - (int64_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].printk_buffer_ptr  = (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[0].printk_buffer_ptr  - (int64_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].exmimo_config_ptr  = (exmimo_config_t*) (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[0].exmimo_config_ptr  - (int64_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].exmimo_id_ptr      = (exmimo_id_t*)     (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[0].exmimo_id_ptr      - (int64_t)bigshm_top_kvirtptr[0]);
#else
    openair0_exmimo_pci[card].firmware_block_ptr = (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[0].firmware_block_ptr - (int32_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].printk_buffer_ptr  = (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[0].printk_buffer_ptr  - (int32_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].exmimo_config_ptr  = (exmimo_config_t*) (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[0].exmimo_config_ptr  - (int32_t)bigshm_top_kvirtptr[0]);
    openair0_exmimo_pci[card].exmimo_id_ptr      = (exmimo_id_t*)     (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[0].exmimo_id_ptr      - (int32_t)bigshm_top_kvirtptr[0]);
#endif

    /*
          printf("openair0_exmimo_pci.firmware_block_ptr (%p) =  bigshm_top(%p) + exmimo_pci_kvirt.firmware_block_ptr(%p) - bigshm_top_kvirtptr(%p)\n",
              openair0_exmimo_pci[card].firmware_block_ptr, bigshm_top, exmimo_pci_kvirt[card].firmware_block_ptr, bigshm_top_kvirtptr[card]);
          printf("card%d, openair0_exmimo_pci.exmimo_id_ptr      (%p) =  bigshm_top(%p) + exmimo_pci_kvirt.exmimo_id_ptr     (%p) - bigshm_top_kvirtptr(%p)\n",
              card, openair0_exmimo_pci[card].exmimo_id_ptr, bigshm_top[card], exmimo_pci_kvirt[card].exmimo_id_ptr, bigshm_top_kvirtptr[card]);
    */

    /*
    if (openair0_exmimo_pci[card].exmimo_id_ptr->board_swrev != BOARD_SWREV_CNTL2)
      {
        error("Software revision %d and firmware revision %d do not match, Please update either Software or Firmware",BOARD_SWREV_CNTL2,openair0_exmimo_pci[card].exmimo_id_ptr->board_swrev);
        return -5;
      }
    */

    if ( openair0_exmimo_pci[card].exmimo_id_ptr->board_exmimoversion == 1)
      openair0_num_antennas[card] = 2;

    if ( openair0_exmimo_pci[card].exmimo_id_ptr->board_exmimoversion == 2)
      openair0_num_antennas[card] = 4;


    for (ant=0; ant<openair0_num_antennas[card]; ant++) {
#if __x86_64__
      openair0_exmimo_pci[card].rxcnt_ptr[ant] = (unsigned int *) (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[card].rxcnt_ptr[ant] - (int64_t)bigshm_top_kvirtptr[card]);
      openair0_exmimo_pci[card].txcnt_ptr[ant] = (unsigned int *) (bigshm_top[card] +  (int64_t)exmimo_pci_kvirt[card].txcnt_ptr[ant] - (int64_t)bigshm_top_kvirtptr[card]);
#else
      openair0_exmimo_pci[card].rxcnt_ptr[ant] = (unsigned int *) (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[card].rxcnt_ptr[ant] - (int32_t)bigshm_top_kvirtptr[card]);
      openair0_exmimo_pci[card].txcnt_ptr[ant] = (unsigned int *) (bigshm_top[card] +  (int32_t)exmimo_pci_kvirt[card].txcnt_ptr[ant] - (int32_t)bigshm_top_kvirtptr[card]);
#endif
    }

    for (ant=0; ant<openair0_num_antennas[card]; ant++) {
      openair0_exmimo_pci[card].adc_head[ant] = mmap( NULL,
          ADAC_BUFFERSZ_PERCHAN_B,
          PROT_READ|PROT_WRITE,
          MAP_SHARED, //|MAP_FIXED,//MAP_SHARED,
          openair0_fd,
          ( openair_mmap_RX(ant) | openair_mmap_Card(card) )<<PAGE_SHIFT );

      openair0_exmimo_pci[card].dac_head[ant] = mmap( NULL,
          ADAC_BUFFERSZ_PERCHAN_B,
          PROT_READ|PROT_WRITE,
          MAP_SHARED, //|MAP_FIXED,//MAP_SHARED,
          openair0_fd,
          ( openair_mmap_TX(ant) | openair_mmap_Card(card) )<<PAGE_SHIFT );

      if (openair0_exmimo_pci[card].adc_head[ant] == MAP_FAILED || openair0_exmimo_pci[card].dac_head[ant] == MAP_FAILED) {
        openair0_close();
        return -3;
      }
    }

    //printf("p_exmimo_config = %p, p_exmimo_id = %p\n", openair0_exmimo_pci.exmimo_config_ptr, openair0_exmimo_pci.exmimo_id_ptr);

    printf("card %d: ExpressMIMO %d, HW Rev %d, SW Rev 0x%d, %d antennas\n", card, openair0_exmimo_pci[card].exmimo_id_ptr->board_exmimoversion,
           openair0_exmimo_pci[card].exmimo_id_ptr->board_hwrev, openair0_exmimo_pci[card].exmimo_id_ptr->board_swrev, openair0_num_antennas[card]);

  } // end for(card)

  return 0;
}


int openair0_close(void)
{
  int ant;
  int card;

  close(openair0_fd);

  for (card=0; card<openair0_num_detected_cards; card++) {
    if (bigshm_top[card] != NULL && bigshm_top[card] != MAP_FAILED)
      munmap(bigshm_top[card], BIGSHM_SIZE_PAGES<<PAGE_SHIFT);

    for (ant=0; ant<openair0_num_antennas[card]; ant++) {
      if (openair0_exmimo_pci[card].adc_head[ant] != NULL && openair0_exmimo_pci[card].adc_head[ant] != MAP_FAILED)
        munmap(openair0_exmimo_pci[card].adc_head[ant], ADAC_BUFFERSZ_PERCHAN_B);

      if (openair0_exmimo_pci[card].dac_head[ant] != NULL && openair0_exmimo_pci[card].dac_head[ant] != MAP_FAILED)
        munmap(openair0_exmimo_pci[card].dac_head[ant], ADAC_BUFFERSZ_PERCHAN_B);
    }
  }

  return 0;
}

int openair0_dump_config(int card)
{
  return ioctl(openair0_fd, openair_DUMP_CONFIG, card);
}

int openair0_get_frame(int card)
{
  return ioctl(openair0_fd, openair_GET_FRAME, card);
}

int openair0_start_rt_acquisition(int card)
{
  return ioctl(openair0_fd, openair_START_RT_ACQUISITION, card);
}

int openair0_stop(int card)
{
  return ioctl(openair0_fd, openair_STOP, card);
}

int openair0_stop_without_reset(int card)
{
  return ioctl(openair0_fd, openair_STOP_WITHOUT_RESET, card);
}

#define MY_RF_MODE      (RXEN + TXEN + TXLPFNORM + TXLPFEN + TXLPF25 + RXLPFNORM + RXLPFEN + RXLPF25 + LNA1ON +LNAMax + RFBBNORM + DMAMODE_RX + DMAMODE_TX)
#define RF_MODE_BASE    (LNA1ON + RFBBNORM)

void rt_sleep(struct timespec *ts,long tv_nsec) {

  clock_gettime(CLOCK_MONOTONIC, ts);

  ts->tv_nsec += tv_nsec;

  if (ts->tv_nsec>=1000000000L) {
    ts->tv_nsec -= 1000000000L;
    ts->tv_sec++;
  }

  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ts, NULL);

}
static void *watchdog_thread(void *arg) {

  int policy, s, j;
  struct sched_param sparam;
  char cpu_affinity[1024];
  cpu_set_t cpuset;
  exmimo_state_t *exm=((openair0_device *)arg)->priv;
  openair0_config_t *cfg=&((openair0_device *)arg)->openair0_cfg[0];

  volatile unsigned int *daq_mbox = openair0_daq_cnt();
  unsigned int mbox,diff;
  int first_acquisition;
  struct timespec sleep_time,wait;

  int ret;


  wait.tv_sec=0;
  wait.tv_nsec=50000000L;

  /* Set affinity mask to include CPUs 1 to MAX_CPUS */
  /* CPU 0 is reserved for UHD threads */
  /* CPU 1 is reserved for all TX threads */
  /* Enable CPU Affinity only if number of CPUs >2 */
  CPU_ZERO(&cpuset);

#ifdef CPU_AFFINITY
  if (get_nprocs() > 2)
  {
    for (j = 1; j < get_nprocs(); j++)
        CPU_SET(j, &cpuset);
    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
      perror( "pthread_setaffinity_np");
      printf("Error setting processor affinity");
    }
  }
#endif //CPU_AFFINITY

  /* Check the actual affinity mask assigned to the thread */

  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0)
  {
    perror( "pthread_getaffinity_np");
    printf("Error getting processor affinity ");
  }
  memset(cpu_affinity,0,sizeof(cpu_affinity));
  for (j = 0; j < CPU_SETSIZE; j++)
     if (CPU_ISSET(j, &cpuset))
     {  
        char temp[1024];
        sprintf (temp, " CPU_%d", j);
        strcat(cpu_affinity, temp);
     }

  memset(&sparam, 0 , sizeof (sparam));
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
  policy = SCHED_FIFO ; 
  
  s = pthread_setschedparam(pthread_self(), policy, &sparam);
  if (s != 0)
     {
     perror("pthread_setschedparam : ");
     printf("Error setting thread priority");
     }
  s = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (s != 0)
   {
     perror("pthread_getschedparam : ");
     printf("Error getting thread priority");

   }

 printf("EXMIMO2 Watchdog TX thread started on CPU %d TID %ld, sched_policy = %s , priority = %d, CPU Affinity=%s \n",
	sched_getcpu(),
	syscall(__NR_gettid),
	(policy == SCHED_FIFO)  ? "SCHED_FIFO" :
	(policy == SCHED_RR)    ? "SCHED_RR" :
	(policy == SCHED_OTHER) ? "SCHED_OTHER" :
	"???",
	sparam.sched_priority, 
	cpu_affinity );


  mlockall(MCL_CURRENT | MCL_FUTURE);

  exm->watchdog_exit = 0;
  exm->ts = 0;
  exm->last_mbox = 0;
  
  if (cfg->sample_rate==30.72e6) {
    exm->samples_per_tick  = 15360;
    exm->samples_per_frame = 307200; 
  }
  else if (cfg->sample_rate==23.04e6) {
    exm->samples_per_tick = 11520;
    exm->samples_per_frame = 230400; 
  }
  else if (cfg->sample_rate==15.36e6) {
    exm->samples_per_tick = 7680;
    exm->samples_per_frame = 153600; 
  }
  else if (cfg->sample_rate==7.68e6) {
    exm->samples_per_tick = 3840;
    exm->samples_per_frame = 76800; 
  }
  else if (cfg->sample_rate==3.84e6) {
    exm->samples_per_tick = 1920;
    exm->samples_per_frame = 38400; 
  }
  else if (cfg->sample_rate==1.92e6) {
    exm->samples_per_tick = 960;
    exm->samples_per_frame = 19200; 
  }
  else {
    printf("Unknown sampling rate %f, exiting \n",cfg->sample_rate);
    exm->watchdog_exit=1;
  }

  first_acquisition=1;
  printf("Locking watchdog for first acquisition\n");
  ret = pthread_mutex_timedlock(&exm->watchdog_mutex,&wait);
  // main loop to keep up with DMA transfers from exmimo2

  unsigned long long cnt_diff0=0;
  while ((!oai_exit) && (!exm->watchdog_exit)) {

    if (exm->daq_state == running) {

      // grab time from MBOX
      mbox = daq_mbox[0];

      if (mbox<exm->last_mbox) { // wrap-around
	diff = 150 + mbox - exm->last_mbox;
      }
      else {
	diff = mbox - exm->last_mbox;
      }
      exm->last_mbox = mbox;

      if (first_acquisition==0)
      {
        ret = pthread_mutex_timedlock(&exm->watchdog_mutex,&wait);
        if(ret)
        {
//           exm->watchdog_exit = 1;
           printf("watchdog_thread pthread_mutex_timedlock error = %d\n", ret);
           continue;
        }
      }

      exm->ts += (diff*exm->samples_per_frame/150) ; 


      if ((exm->daq_state == running) &&
	  (diff > 16)&&
	  (first_acquisition==0))  {// we're too late so exit
	exm->watchdog_exit = 1;
        printf("exiting, too late to keep up - diff = %u\n", diff);
      }
      first_acquisition=0;

      if ((exm->daq_state == running) && 
	  (diff == 0)) {
	cnt_diff0++;
	if (cnt_diff0 == 10) {
	  exm->watchdog_exit = 1;
	  printf("exiting, HW stopped %llu\n", cnt_diff0);
	}
      }
      else
	cnt_diff0=0;

      if ((exm->daq_state == running) &&
	  (exm->wait_first_read==0) &&
	  (exm->ts - exm->last_ts_rx > exm->samples_per_frame)) {
	exm->watchdog_exit = 1;
	printf("RX Overflow, exiting (TS %llu, TS last read %llu)\n",
	       (long long unsigned int)exm->ts,(long long unsigned int)exm->last_ts_rx);
      }
      //      printf("ts %lu, last_ts_rx %lu, mbox %d, diff %d\n",exm->ts, exm->last_ts_rx,mbox,diff);
      
      if ( !ret )
      pthread_mutex_unlock(&exm->watchdog_mutex);
    }
    else {
      first_acquisition=1;
    }
    rt_sleep(&sleep_time,250000L);
  }
  
  oai_exit=1;
  printf("Exiting watchdog\n");
  return NULL;
}

void create_watchdog(openair0_device *dev) {
  
  exmimo_state_t *priv = dev->priv;
  priv->watchdog_exit=0;
#ifndef DEADLINE_SCHEDULER
  priv->watchdog_sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);  
  pthread_attr_setschedparam(&priv->watchdog_attr,&priv->watchdog_sched_param);
  pthread_attr_setschedpolicy(&priv->watchdog_attr,SCHED_FIFO);
  pthread_create(&priv->watchdog,&priv->watchdog_attr,watchdog_thread,dev);
#else
  pthread_create(&priv->watchdog,NULL,watchdog_thread,dev);
#endif
  pthread_mutex_init(&priv->watchdog_mutex,NULL);


}

int trx_exmimo_start(openair0_device *device) {

  exmimo_state_t *exm=device->priv;

  printf("Starting ...\n");
  openair0_config(device->openair0_cfg,0);
  openair0_start_rt_acquisition(0);
  printf("Setting state to running\n");
  exm->daq_state = running;  
  exm->wait_first_read = 1;
  return(0);
}

int trx_exmimo_write(openair0_device *device,openair0_timestamp ptimestamp, void **buff, int nsamps, int cc, int flags) {

  
  return(nsamps);
}



int trx_exmimo_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc) {

  exmimo_state_t *exm=device->priv;
  openair0_config_t *cfg=&device->openair0_cfg[0];
  openair0_timestamp old_ts=0,ts,diff;
  struct timespec sleep_time;
  unsigned long tv_nsec;
  int i;
  int n,n1,n2,ntot,first_len;
  int ret;

  //  struct timespec wait;
  //  wait.tv_sec=0;
  //  wait.tv_nsec=50000000L;

  if (exm->watchdog_exit == 1)
    return(0);

  if (exm->daq_state == idle) {
    tv_nsec=(unsigned long)((double)(nsamps)*1e9/cfg->sample_rate);
    return(0);
  }

  ret = pthread_mutex_lock(&exm->watchdog_mutex);

  switch (ret) {
  case EINVAL:
#ifdef DEBUG_EXMIMO 
    printf("trx_exmimo_read: mutex_timedlock returned EINVAL\n");
#endif
    return(0);
    break;
  case ETIMEDOUT: 
#ifdef DEBUG_EXMIMO 
    printf("trx_exmimo_read: mutex_timedlock returned ETIMEDOUT\n");
#endif
    return(0);
    break;
  case EAGAIN: 
#ifdef DEBUG_EXMIMO 
    printf("trx_exmimo_read: mutex_timedlock returned EAGAIN\n");
#endif
    return(0);
    break;
  case EDEADLK: 
#ifdef DEBUG_EXMIMO 
    printf("trx_exmimo_read: mutex_timedlock returned EDEADLK\n");
#endif
    return(0);
    break;
  }

  ts = exm->ts;
  if (exm->wait_first_read==1) {
    exm->wait_first_read=0;
    exm->last_ts_rx = ts;
  }

  pthread_mutex_unlock(&exm->watchdog_mutex);
  //  dump_frame_parms(frame_parms[0]);
  
  if (nsamps > (exm->samples_per_frame>>1)) {
    n1 = nsamps>>1;
    n2 = nsamps-n1;
  }
  else {
    n1=nsamps;
    n2=0;
  }

#ifdef DEBUG_EXMIMO
  printf("Reading %d samples, ts %lu (%d), last_ts_rx %lu (%lu)\n",nsamps,ts,ts%exm->samples_per_frame,exm->last_ts_rx,exm->last_ts_rx+nsamps);
#endif
  for (n=n1,ntot=0;ntot<nsamps;n=n2) {
    while ((ts < exm->last_ts_rx + n) && 
	   (exm->watchdog_exit==0)) {
      
      diff = exm->last_ts_rx+n - ts; // difference in samples between current timestamp and last RX received sample
      // go to sleep until we should have enough samples (1024 for a bit more)
#ifdef DEBUG_EXMIMO
      printf("portion %d samples, ts %lu, last_ts_rx %lu (%lu) => sleeping %u us\n",n,ts,exm->last_ts_rx,exm->last_ts_rx+n,
	     (unsigned int)((double)(diff+1024)*1e6/cfg->sample_rate));
#endif
      tv_nsec=(unsigned long)((double)(diff+3840)*1e9/cfg->sample_rate);
      //    tv_nsec = 500000L;
      old_ts = ts;
      rt_sleep(&sleep_time,tv_nsec);
#ifdef DEBUG_EXMIMO
      printf("back\n");
#endif
      // get new timestamp, in case we have to sleep again
      pthread_mutex_lock(&exm->watchdog_mutex);
      ts = exm->ts;
      pthread_mutex_unlock(&exm->watchdog_mutex);
      if (old_ts == ts) {
	printf("ts stopped, returning\n");
	return(0);
      }
    }

  
  
    if (cfg->mmapped_dma == 0) {  // if buff is not the dma buffer, do a memcpy, otherwise do nothing
      for (i=0;i<cc;i++) {
#ifdef DEBUG_EXMIMO
	printf("copying to %p (%zu), from %llu\n",buff[i]+(ntot*sizeof(int)),ntot*sizeof(int),(exm->last_ts_rx % exm->samples_per_frame));
#endif
	if ((n+(exm->last_ts_rx%exm->samples_per_frame))<exm->samples_per_frame) {
	  memcpy(buff[i]+(ntot*sizeof(int)),
		 (void*)(openair0_exmimo_pci[0].adc_head[i]+(exm->last_ts_rx % exm->samples_per_frame)),
		 n*sizeof(int));
 	}
	else {
	  first_len =  (exm->samples_per_frame-(exm->last_ts_rx%exm->samples_per_frame));
#ifdef DEBUG_EXMIMO
	  printf("split: first_len %d, remainder %d\n",first_len,n-first_len);
#endif
	  memcpy(buff[i]+(ntot*sizeof(int)),
		 (void*)(openair0_exmimo_pci[0].adc_head[i]+(exm->last_ts_rx % exm->samples_per_frame)),
		 first_len*sizeof(int));
	  memcpy(buff[i]+(ntot+first_len)*sizeof(int),
		 (void*)openair0_exmimo_pci[0].adc_head[i],
		 (n-first_len)*sizeof(int));
	}
      }
    }
    pthread_mutex_lock(&exm->watchdog_mutex);
    exm->last_ts_rx += n;
    pthread_mutex_unlock(&exm->watchdog_mutex);    
    if (n==n1) {
      *ptimestamp=exm->last_ts_rx;
    }
    ntot+=n;
   }
  



  return(nsamps);
}

void trx_exmimo_end(openair0_device *device) {

  exmimo_state_t *exm=device->priv;

  exm->daq_state = idle;
  openair0_stop(0);
 
}

int trx_exmimo_get_stats(openair0_device* device) {

  return(0);

}

int trx_exmimo_reset_stats(openair0_device* device) {

  return(0);

}

int trx_exmimo_stop(openair0_device* device) {

  exmimo_state_t *exm=device->priv;

  printf("Stopping ...\n");
  exm->daq_state = idle;  
  openair0_stop(0);

  return(0);

}

int trx_exmimo_set_freq(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config) {

  openair0_set_frequencies(device,openair0_cfg,0);
  return(0);
}

int trx_exmimo_set_gains(openair0_device* device, openair0_config_t *openair0_cfg) {

  return(0);

}

void kill_watchdog(openair0_device *device) {

  exmimo_state_t *exm=(exmimo_state_t *)device->priv;
  exm->watchdog_exit=1;

}

int device_init(openair0_device *device, openair0_config_t *openair0_cfg) {

  // Initialize card
  //  exmimo_config_t         *p_exmimo_config;
  exmimo_id_t             *p_exmimo_id;
  int ret;
  exmimo_state_t *exm = (exmimo_state_t *)malloc(sizeof(exmimo_state_t));
  int card,ant;

  ret = openair0_open();
 
  if ( ret != 0 ) {
    if (ret == -1)
      printf("Error opening /dev/openair0");

    if (ret == -2)
      printf("Error mapping bigshm");

    if (ret == -3)
      printf("Error mapping RX or TX buffer");
    free(exm);
    return(ret);
  }

  if (openair0_num_detected_cards>MAX_CARDS) {
    printf ("Detected %d number of cards, but MAX_CARDS=%d\n", openair0_num_detected_cards, MAX_CARDS);
  } else {
    printf ("Detected %d number of cards, %d number of antennas.\n", openair0_num_detected_cards, openair0_num_antennas[0]);
  }

  for (card=0; card<openair0_num_detected_cards; card++) {
    //  p_exmimo_config = openair0_exmimo_pci[0].exmimo_config_ptr;
    p_exmimo_id     = openair0_exmimo_pci[card].exmimo_id_ptr;

    printf("Card %d: ExpressMIMO %d, HW Rev %d, SW Rev 0x%d\n", card, p_exmimo_id->board_exmimoversion, p_exmimo_id->board_hwrev, p_exmimo_id->board_swrev);

    // check if the software matches firmware
    if (p_exmimo_id->board_swrev!=BOARD_SWREV_CNTL2) {
      printf("Software revision %d and firmware revision %d do not match. Please update either the firmware or the software!\n",BOARD_SWREV_CNTL2,p_exmimo_id->board_swrev);
      free(exm);
      return(-1);
    }
  }

  device->type             = EXMIMO_DEV; 

  // Add stuff that was in lte-softmodem here

  //
  device->trx_start_func = trx_exmimo_start;
  device->trx_end_func   = trx_exmimo_end;
  device->trx_read_func  = trx_exmimo_read;
  device->trx_write_func = trx_exmimo_write;
  device->trx_get_stats_func   = trx_exmimo_get_stats;
  device->trx_reset_stats_func = trx_exmimo_reset_stats;
  device->trx_stop_func        = trx_exmimo_stop;
  device->trx_set_freq_func    = trx_exmimo_set_freq;
  device->trx_set_gains_func   = trx_exmimo_set_gains;
  device->openair0_cfg = openair0_cfg;
  device->priv = (void *)exm;


  printf("EXMIMO2: Getting addresses for memory-mapped DMA\n");
  for (card=0; card<openair0_num_detected_cards; card++) {
    for (ant=0; ant<4; ant++) {
      openair0_cfg[card].rxbase[ant] = (int32_t*)openair0_exmimo_pci[card].adc_head[ant];
      openair0_cfg[card].txbase[ant] = (int32_t*)openair0_exmimo_pci[card].dac_head[ant];
    }
    openair0_cfg[card].mmapped_dma = 1;
  }

  create_watchdog(device);

  return(0);
}

unsigned int             rxg_max[4] =    {128,128,128,126};
unsigned int             rxg_med[4] =    {122,123,123,120};
unsigned int             rxg_byp[4] =    {116,117,116,116};
unsigned int             nf_max[4] =    {7,9,16,12};
unsigned int             nf_med[4] =    {12,13,22,17};
unsigned int             nf_byp[4] =    {15,20,29,23};

int openair0_config(openair0_config_t *openair0_cfg, int UE_flag)
{
  int ret;
  int ant, card;
  int resampling_factor=2;
  int rx_filter=RXLPF25, tx_filter=TXLPF25;
  int ACTIVE_RF=0;
  int i;

  exmimo_config_t         *p_exmimo_config;
  exmimo_id_t             *p_exmimo_id;

  if (!openair0_cfg) {
    printf("Error, openair0_cfg is null!!\n");
    return(-1);
  }

  for (card=0; card<openair0_num_detected_cards; card++) {

    p_exmimo_config = openair0_exmimo_pci[card].exmimo_config_ptr;
    p_exmimo_id     = openair0_exmimo_pci[card].exmimo_id_ptr;

    if (p_exmimo_id->board_swrev>=9)
      p_exmimo_config->framing.eNB_flag   = 0;
    else
      p_exmimo_config->framing.eNB_flag   = !UE_flag;

    if (openair0_num_detected_cards==1)
      p_exmimo_config->framing.multicard_syncmode=SYNCMODE_FREE;
    else if (card==0)
      p_exmimo_config->framing.multicard_syncmode=SYNCMODE_MASTER;
    else
      p_exmimo_config->framing.multicard_syncmode=SYNCMODE_SLAVE;

    /* device specific */
    openair0_cfg[card].iq_txshift = 4;//shift
    openair0_cfg[card].iq_rxrescale = 15;//rescale iqs


    if (openair0_cfg[card].sample_rate==30.72e6) {
      resampling_factor = 0;
      rx_filter = RXLPF10;
      tx_filter = TXLPF10;
    } else if (openair0_cfg[card].sample_rate==15.36e6) {
      resampling_factor = 1;
      rx_filter = RXLPF5;
      tx_filter = TXLPF5;
    } else if (openair0_cfg[card].sample_rate==7.68e6) {
      resampling_factor = 2;
      rx_filter = RXLPF25;
      tx_filter = TXLPF25;
    } else {
      printf("Sampling rate not supported, using default 7.68MHz");
      resampling_factor = 2;
      rx_filter = RXLPF25;
      tx_filter = TXLPF25;

    }

#if (BOARD_SWREV_CNTL2>=0x0A)

    for (ant=0; ant<4; ant++)
      p_exmimo_config->framing.resampling_factor[ant] = resampling_factor;

#else
    p_exmimo_config->framing.resampling_factor = resampling_factor;
#endif

    for (ant=0; ant<4; ant++) {
      p_exmimo_config->rf.rf_freq_rx[ant] = 0;
      p_exmimo_config->rf.rf_freq_tx[ant] = 0;
      p_exmimo_config->rf.rf_mode[ant] = 0;
      p_exmimo_config->rf.rx_gain[ant][0] = 0;
      p_exmimo_config->rf.tx_gain[ant][0] = 0;

      openair0_cfg[card].rxbase[ant] = (int32_t*)openair0_exmimo_pci[card].adc_head[ant];
      openair0_cfg[card].txbase[ant] = (int32_t*)openair0_exmimo_pci[card].dac_head[ant];

      if (openair0_cfg[card].rx_freq[ant] || openair0_cfg[card].tx_freq[ant]) {
	ACTIVE_RF += (1<<ant)<<5;
        p_exmimo_config->rf.rf_mode[ant] = RF_MODE_BASE;
        p_exmimo_config->rf.do_autocal[ant] = 1;//openair0_cfg[card].autocal[ant];
	printf("card %d, antenna %d, autocal %d\n",card,ant,p_exmimo_config->rf.do_autocal[ant]);
      }

      if (openair0_cfg[card].tx_freq[ant]>0) {
        p_exmimo_config->rf.rf_mode[ant] += (TXEN + DMAMODE_TX + TXLPFNORM + TXLPFEN + tx_filter);
        p_exmimo_config->rf.rf_freq_tx[ant] = (unsigned int)openair0_cfg[card].tx_freq[ant];
        p_exmimo_config->rf.tx_gain[ant][0] = (unsigned int)openair0_cfg[card].tx_gain[ant];
        printf("openair0 : programming card %d TX antenna %d (freq %u, gain %d)\n",card,ant,p_exmimo_config->rf.rf_freq_tx[ant],p_exmimo_config->rf.tx_gain[ant][0]);

	printf("Setting TX buffer to all-RX\n");

	for (i=0;i<307200;i++) {
	  ((uint32_t*)openair0_exmimo_pci[card].dac_head[ant])[i] = 0x00010001;
	}
      }

      if (openair0_cfg[card].rx_freq[ant]>0) {
        p_exmimo_config->rf.rf_mode[ant] += (RXEN + DMAMODE_RX + RXLPFNORM + RXLPFEN + rx_filter);
        p_exmimo_config->rf.rf_freq_rx[ant] = (unsigned int)openair0_cfg[card].rx_freq[ant];

        switch (openair0_cfg[card].rxg_mode[ant]) {
        default:
        case max_gain:
          p_exmimo_config->rf.rf_mode[ant] += LNAMax;
	  if (rxg_max[ant] >= (int)openair0_cfg[card].rx_gain[ant]) {
	    p_exmimo_config->rf.rx_gain[ant][0] = 30 - (rxg_max[ant] - (int)openair0_cfg[card].rx_gain[ant]); //was measured at rxgain=30;
	  }
	  else {
	    printf("openair0: RX RF gain too high, reduce by %u dB\n", (int)openair0_cfg[card].rx_gain[ant]-rxg_max[ant]);
	    exit(-1);
	  }
          break;

        case med_gain:
          p_exmimo_config->rf.rf_mode[ant] += LNAMed;
	  if (rxg_med[ant] >= (int)openair0_cfg[card].rx_gain[ant]) {
	    p_exmimo_config->rf.rx_gain[ant][0] = 30 - (rxg_med[ant] - (int)openair0_cfg[card].rx_gain[ant]); //was measured at rxgain=30;
	  }
	  else {
	    printf("openair0: RX RF gain too high, reduce by %u dB\n", (int)openair0_cfg[card].rx_gain[ant]-rxg_med[ant]);
	    exit(-1);
	  }
          break;

        case byp_gain:
          p_exmimo_config->rf.rf_mode[ant] += LNAByp;
	  if (rxg_byp[ant] >= (int)openair0_cfg[card].rx_gain[ant]) {
	    p_exmimo_config->rf.rx_gain[ant][0] = 30 - (rxg_byp[ant] - (int)openair0_cfg[card].rx_gain[ant]); //was measured at rxgain=30;
	  }
	  else {
	    printf("openair0: RX RF gain too high, reduce by %u dB\n", (int)openair0_cfg[card].rx_gain[ant]-rxg_byp[ant]);
	    exit(-1);
	  }
          break;
        }
        printf("openair0 : programming card %d RX antenna %d (freq %u, gain %d)\n",card,ant,p_exmimo_config->rf.rf_freq_rx[ant],p_exmimo_config->rf.rx_gain[ant][0]);

      } else {
        p_exmimo_config->rf.rf_mode[ant] = 0;
        p_exmimo_config->rf.do_autocal[ant] = 0;
      }

      p_exmimo_config->rf.rf_local[ant]   = rf_local[ant];
      p_exmimo_config->rf.rf_rxdc[ant]    = rf_rxdc[ant];

      if (( p_exmimo_config->rf.rf_freq_tx[ant] >= 790000000) && ( p_exmimo_config->rf.rf_freq_tx[ant] <= 865000000)) {
        p_exmimo_config->rf.rf_vcocal[ant]  = rf_vcocal_850[ant];
        p_exmimo_config->rf.rffe_band_mode[ant] = DD_TDD;
      } else if (( p_exmimo_config->rf.rf_freq_tx[ant] >= 1900000000) && ( p_exmimo_config->rf.rf_freq_tx[ant] <= 2000000000)) {
        p_exmimo_config->rf.rf_vcocal[ant]  = rf_vcocal[ant];
        p_exmimo_config->rf.rffe_band_mode[ant] = B19G_TDD;
      } else {
        p_exmimo_config->rf.rf_vcocal[ant]  = rf_vcocal[ant];
        p_exmimo_config->rf.rffe_band_mode[ant] = 0;
      }
    }

    if (openair0_cfg[card].duplex_mode==duplex_mode_FDD) {
      p_exmimo_config->framing.tdd_config = DUPLEXMODE_FDD;// + TXRXSWITCH_LSB + TXRXSWITCH_LSB + ACTIVE_RF+ ACTIVE_RF;
      printf("!!!!!setting FDD (tdd_config=%d)\n",p_exmimo_config->framing.tdd_config);
    } 
    else {
      p_exmimo_config->framing.tdd_config = DUPLEXMODE_TDD + TXRXSWITCH_LSB + ACTIVE_RF;
      printf("!!!!!setting TDD (tdd_config=%d)\n",p_exmimo_config->framing.tdd_config);
    }

    ret = ioctl(openair0_fd, openair_DUMP_CONFIG, card);

    if (ret!=0)
      return(-1);

  }

  return(0);
}

int openair0_reconfig(openair0_config_t *openair0_cfg)
{

  int ant, card;

  exmimo_config_t         *p_exmimo_config;
  //  exmimo_id_t             *p_exmimo_id;

  if (!openair0_cfg) {
    printf("Error, openair0_cfg is null!!\n");
    return(-1);
  }

#ifdef DEBUG_EXMIMO
  printf("Reconfiguration of gains/frequencies\n");
#endif
  for (card=0; card<openair0_num_detected_cards; card++) {

    p_exmimo_config = openair0_exmimo_pci[card].exmimo_config_ptr;
    //    p_exmimo_id     = openair0_exmimo_pci[card].exmimo_id_ptr;

    for (ant=0; ant<4; ant++) {
      if (openair0_cfg[card].tx_freq[ant]) {
        p_exmimo_config->rf.rf_freq_tx[ant] = (unsigned int)openair0_cfg[card].tx_freq[ant];
        p_exmimo_config->rf.tx_gain[ant][0] = (unsigned int)openair0_cfg[card].tx_gain[ant];
#ifdef DEBUG_EXMIMO
        printf("openair0 -  %d : programming TX antenna %d (freq %u, gain %d)\n",card,ant,p_exmimo_config->rf.rf_freq_tx[ant],p_exmimo_config->rf.tx_gain[ant][0]);
#endif
      }


      if (openair0_cfg[card].rx_freq[ant]) {
        p_exmimo_config->rf.rf_freq_rx[ant] = (unsigned int)openair0_cfg[card].rx_freq[ant];
        p_exmimo_config->rf.rx_gain[ant][0] = (unsigned int)openair0_cfg[card].rx_gain[ant];
#ifdef DEBUG_EXMIMO
        printf("openair0 -  %d : programming RX antenna %d (freq %u, gain %d)\n",card,ant,p_exmimo_config->rf.rf_freq_rx[ant],p_exmimo_config->rf.rx_gain[ant][0]);
#endif
        switch (openair0_cfg[card].rxg_mode[ant]) {
        default:
        case max_gain:
          p_exmimo_config->rf.rf_mode[ant] = (p_exmimo_config->rf.rf_mode[ant]&(~LNAGAINMASK))|LNAMax;
          break;

        case med_gain:
          p_exmimo_config->rf.rf_mode[ant] = (p_exmimo_config->rf.rf_mode[ant]&(~LNAGAINMASK))|LNAMed;
          break;

        case byp_gain:
          p_exmimo_config->rf.rf_mode[ant] = (p_exmimo_config->rf.rf_mode[ant]&(~LNAGAINMASK))|LNAByp;
          break;
        }
      }
    }
  }

  return(0);
}

int openair0_set_frequencies(openair0_device* device, openair0_config_t *openair0_cfg,int exmimo_dump_config) {

  if (exmimo_dump_config > 0) {
    // do a full configuration
    openair0_config(openair0_cfg,0);
  }
  else {  // just change the frequencies in pci descriptor
    openair0_reconfig(openair0_cfg);
  }
  return(0);
  
}

int openair0_set_gains(openair0_device* device, openair0_config_t *openair0_cfg){
  return(0);
}

unsigned int *openair0_daq_cnt(void) {

  return((unsigned int *)openair0_exmimo_pci[0].rxcnt_ptr[0]);

}
