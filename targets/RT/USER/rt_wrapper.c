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

/*! \file rt_wrapper.h
* \brief provides a wrapper for the timing function, runtime calculations for real-time opeartions depending on weather RTAI or DEADLINE_SCHEDULER kernels are used or not
* \author F. Kaltenberger and Navid Nikaein
* \date 2013
* \version 0.1
* \company Eurecom
* \email: florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
* \note
* \warning
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <sys/sysinfo.h>
#include "rt_wrapper.h"
#include <errno.h>

#include "openair1/PHY/defs_common.h"

static int latency_target_fd = -1;
static int32_t latency_target_value = 0;
/* Latency trick - taken from cyclictest.c
 * if the file /dev/cpu_dma_latency exists,
 * open it and write a zero into it. This will tell
 * the power management system not to transition to
 * a high cstate (in fact, the system acts like idle=poll)
 * When the fd to /dev/cpu_dma_latency is closed, the behavior
 * goes back to the system default.
 *
 * Documentation/power/pm_qos_interface.txt
 */
void set_latency_target(void) {
  struct stat s;
  int ret;

  if (stat("/dev/cpu_dma_latency", &s) == 0) {
    latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);

    if (latency_target_fd == -1)
      return;

    ret = write(latency_target_fd, &latency_target_value, 4);

    if (ret == 0) {
      printf("# error setting cpu_dma_latency to %d!: %s\n", latency_target_value, strerror(errno));
      close(latency_target_fd);
      return;
    }

    printf("# /dev/cpu_dma_latency set to %dus\n", latency_target_value);
  }
}


struct timespec interval, next, now, res;
clockid_t clock_id = CLOCK_MONOTONIC; //other options are CLOCK_MONOTONIC, CLOCK_REALTIME, CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID
RTIME rt_get_time_ns (void)
{
  clock_gettime(clock_id, &now);
  return(now.tv_sec*1e9+now.tv_nsec);
}

int rt_sleep_ns (RTIME x)
{
  int ret;
  clock_gettime(clock_id, &now);
  interval.tv_sec = x/((RTIME)1000000000);
  interval.tv_nsec = x%((RTIME)1000000000);
  //rt_printk("sleeping for %d sec and %d ns\n",interval.tv_sec,interval.tv_nsec);
  next = now;
  next.tv_sec += interval.tv_sec;
  next.tv_nsec += interval.tv_nsec;

  if (next.tv_nsec>=1000000000) {
    next.tv_nsec -= 1000000000;
    next.tv_sec++;
  }

  ret = clock_nanosleep(clock_id, TIMER_ABSTIME, &next, NULL);

  /*
  if (ret==EFAULT)
    rt_printk("rt_sleep_ns returned EFAULT (%d), reqested %d sec and %d ns\n",ret,next.tv_sec,next.tv_nsec);
  if (ret==EINVAL)
    rt_printk("rt_sleep_ns returned EINVAL (%d), reqested %d sec and %d ns\n",ret,next.tv_sec,next.tv_nsec);
  if (ret==EINTR)
    rt_printk("rt_sleep_ns returned EINTR (%d), reqested %d sec and %d ns\n",ret,next.tv_sec,next.tv_nsec);
  */

  return(ret);
}

void check_clock(void)
{
  if (clock_getres(clock_id, &res)) {
    printf("clock_getres failed");
  } else {
    printf("reported resolution = %lld ns\n", (long long int) ((int) 1e9 * res.tv_sec) + (long long int) res.tv_nsec);
  }
}

uint16_t cell_processing_dl[6]={10,15,24,42,80,112};
uint16_t platform_processing_dl=20; // upperbound for GPP, LXC, DOCKER and KVM
uint16_t user_processing_dl_a[6]={2,4,5,7,10,12};
uint16_t user_processing_dl_b[6]={10, 15, 25, 70, 110, 150};
uint16_t user_processing_dl_err[6]={20, 40, 60, 90, 120, 160};
uint16_t protocol_processing_dl[6]={150, 250, 350, 450, 650, 800}; // assumption: max MCS 27 --> gives an upper bound for the transport block size : to be measured 

uint16_t cell_processing_ul[6]={10,15,24,42,80,112};
uint16_t platform_processing_ul=30; // upperbound for GPP, LXC, DOCKER and KVM
uint16_t user_processing_ul_a[6]={5, 9, 12, 24, 33, 42};
uint16_t user_processing_ul_b[6]={20, 30, 40, 76, 140, 200};
uint16_t user_processing_ul_err[6]={15, 25, 32, 60, 80, 95};
uint16_t protocol_processing_ul[6]={100, 200, 300, 400, 550, 700}; // assumption: max MCS 16 --> gives an upper bound for the transport block size 

int fill_modeled_runtime_table(uint16_t runtime_phy_rx[29][6],
			       uint16_t runtime_phy_tx[29][6]){
  //double cpu_freq;
  //cpu_freq = get_cpu_freq_GHz();
  // target_dl_mcs
  // target_ul_mcs
  // frame_parms[0]->N_RB_DL
  int i,j;
  memset(runtime_phy_rx,0,sizeof(uint16_t)*29*6);
  memset(runtime_phy_tx,0,sizeof(uint16_t)*29*6);
  /* only the BBU/PHY procesing time */ 
  for (i=0;i<29;i++){
    for (j=0;j<6;j++){
      runtime_phy_rx[i][j] = cell_processing_ul[j] + platform_processing_ul + user_processing_ul_err[j] + user_processing_ul_a[j]*i+  user_processing_ul_b[j];
      runtime_phy_tx[i][j] = cell_processing_dl[j] + platform_processing_dl + user_processing_dl_err[j] + user_processing_dl_a[j]*i+  user_processing_dl_b[j];
    }
  }
  return 0;
}
 
// int runtime_upper_layers[6]; // values for different RBs
// int runtime_phy_rx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]
// int runtime_phy_tx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100]

// target_dl_mcs
  // target_ul_mcs
  // frame_parms[0]->N_RB_DL
  //runtime_upper_layers[6]; // values for different RBs
  // int runtime_phy_rx[29][6]; // SISO [MCS 0-28][RBs 0-5 : 6, 15, 25, 50, 75, 100] 
  // int runtime_phy_tx[29][6]

double get_runtime_tx(int tx_subframe, uint16_t runtime_phy_tx[29][6], uint32_t mcs, int N_RB_DL,double cpuf,int nb_tx_antenna){
   int i;	
   double runtime;
   //printf("cpuf =%lf  \n",cpuf);
   switch(N_RB_DL){
   case 6:
     i = 0;
     break;
   case 15:
     i = 1;
     break;
   case 25:
     i = 2;
     break;
   case 50:
     i = 3;
     break;
   case 75:
     i = 4;
     break;
   case 100:
     i = 5;
     break;
   default:
     i = 3;
     break;
   }			
   
   runtime = ( (3.2/cpuf)*(double)runtime_phy_tx[mcs][i] + (3.2/cpuf)*(double)protocol_processing_dl[i])/1000 ;
   printf("Setting tx %d runtime value (ms) = %lf\n",tx_subframe,runtime);
   
   return runtime;	
 }
 
double get_runtime_rx(int rx_subframe, uint16_t runtime_phy_rx[29][6], uint32_t mcs, int N_RB_DL,double cpuf,int nb_rx_antenna){
   int i;	
   double runtime;
   
   //printf("N_RB_DL=%d  cpuf =%lf  \n",N_RB_DL, cpuf);
   switch(N_RB_DL){
   case 6:
     i = 0;
     break;
   case 15:
     i = 1;
     break;
   case 25:
     i = 2;
     break;
   case 50:
     i = 3;
     break;
   case 75:
     i = 4;
     break;
   case 100:
     i = 5;
     break;
   default:
     i = 3;
     break;
   }			
   
   runtime = ((3.2/cpuf)*(double)runtime_phy_rx[mcs][i] + (3.2/cpuf)*(double)protocol_processing_ul[i])/1000 ;
   printf("Setting rx %d runtime value (ms) = %lf \n",rx_subframe, runtime);
   
   return runtime;	
}

#ifdef DEADLINE_SCHEDULER
int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags)
{

  return syscall(__NR_sched_setattr, pid, attr, flags);
}


int sched_getattr(pid_t pid,struct sched_attr *attr,unsigned int size, unsigned int flags)
{

  return syscall(__NR_sched_getattr, pid, attr, size, flags);
}

#endif

void thread_top_init(char *thread_name,
		     int affinity,
		     uint64_t runtime,
		     uint64_t deadline,
		     uint64_t period) {
  
  MSC_START_USE();

#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;

  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  = runtime;
  attr.sched_deadline = deadline;
  attr.sched_period   = period; 

  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] eNB tx thread: sched_setattr failed\n");
    fprintf(stderr,"sched_setattr Error = %s",strerror(errno));
    exit(1);
  }

#else //LOW_LATENCY
  int policy, s, j;
  struct sched_param sparam;
  char cpu_affinity[1024];
  cpu_set_t cpuset;

  /* Set affinity mask to include CPUs 1 to MAX_CPUS */
  /* CPU 0 is reserved for UHD threads */
  /* CPU 1 is reserved for all RX_TX threads */
  /* Enable CPU Affinity only if number of CPUs >2 */
  CPU_ZERO(&cpuset);

#ifdef CPU_AFFINITY
  if (get_nprocs() > 2)
  {
    if (affinity == 0)
      CPU_SET(0,&cpuset);
    else
      for (j = 1; j < get_nprocs(); j++)
        CPU_SET(j, &cpuset);
    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
      perror( "pthread_setaffinity_np");
      exit_fun("Error setting processor affinity");
    }
  }
#endif //CPU_AFFINITY

  /* Check the actual affinity mask assigned to the thread */
  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0) {
    perror( "pthread_getaffinity_np");
    exit_fun("Error getting processor affinity ");
  }
  memset(cpu_affinity,0,sizeof(cpu_affinity));
  for (j = 0; j < 1024; j++)
    if (CPU_ISSET(j, &cpuset)) {  
      char temp[1024];
      sprintf (temp, " CPU_%d", j);
      strcat(cpu_affinity, temp);
    }

  memset(&sparam, 0, sizeof(sparam));
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
  policy = SCHED_FIFO ; 
  
  s = pthread_setschedparam(pthread_self(), policy, &sparam);
  if (s != 0) {
    perror("pthread_setschedparam : ");
    exit_fun("Error setting thread priority");
  }
  
  s = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (s != 0) {
    perror("pthread_getschedparam : ");
    exit_fun("Error getting thread priority");
  }

  pthread_setname_np(pthread_self(), thread_name);

  LOG_I(HW, "[SCHED][eNB] %s started on CPU %d, sched_policy = %s , priority = %d, CPU Affinity=%s \n",thread_name,sched_getcpu(),
                   (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                   (policy == SCHED_RR)    ? "SCHED_RR" :
                   (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                   "???",
                   sparam.sched_priority, cpu_affinity );

#endif //LOW_LATENCY

  mlockall(MCL_CURRENT | MCL_FUTURE);

}


