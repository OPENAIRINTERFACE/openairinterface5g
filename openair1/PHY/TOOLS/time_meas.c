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
#define _GNU_SOURCE
#include <stdio.h>
#include "time_meas.h"
#include <math.h>
#include <unistd.h>
#include <string.h>
#include "assertions.h"
#include <pthread.h>
#include "common/config/config_userapi.h"
#include <common/utils/threadPool/thread-pool.h>
// global var for openair performance profiler
int opp_enabled = 0;
double cpu_freq_GHz  __attribute__ ((aligned(32)));

double cpu_freq_GHz  __attribute__ ((aligned(32)))=0.0;
static uint32_t    max_cpumeasur;
static time_stats_t  **measur_table;
notifiedFIFO_t measur_fifo;
double get_cpu_freq_GHz(void)
{
  if (cpu_freq_GHz <1 ) {
  time_stats_t ts = {0};
  reset_meas(&ts);
  ts.trials++;
  ts.in = rdtsc_oai();
  sleep(1);
  ts.diff = (rdtsc_oai()-ts.in);
  cpu_freq_GHz = (double)ts.diff/1000000000;
  printf("CPU Freq is %f \n", cpu_freq_GHz);
  }
  return cpu_freq_GHz;
}

int cpumeas(int action)
{
  switch (action) {
    case CPUMEAS_ENABLE:
      opp_enabled = 1;
      break;

    case CPUMEAS_DISABLE:
      opp_enabled = 0;
      break;

    case CPUMEAS_GETSTATE:
    default:
      break;
  }

  return opp_enabled;
}

void print_meas_now(time_stats_t *ts,
                    const char *name,
                    FILE *file_name)
{
  if (opp_enabled) {
    //static double cpu_freq_GHz = 3.2;

    //if (cpu_freq_GHz == 0.0)
    //cpu_freq_GHz = get_cpu_freq_GHz(); // super slow
    if (ts->trials>0) {
      //fprintf(file_name,"Name %25s: Processing %15.3f ms for SF %d, diff_now %15.3f \n", name,(ts->p_time/(cpu_freq_GHz*1000000.0)),subframe,ts->p_time);
      fprintf(file_name,"%15.3f us, diff_now %15.3f \n",(ts->p_time/(cpu_freq_GHz*1000.0)),(double)ts->p_time);
    }
  }
}

void print_meas(time_stats_t *ts,
                const char *name,
                time_stats_t *total_exec_time,
                time_stats_t *sf_exec_time)
{
  if (opp_enabled) {
    static int first_time = 0;
    static double cpu_freq_GHz = 0.0;

    if (cpu_freq_GHz == 0.0)
      cpu_freq_GHz = get_cpu_freq_GHz();

    if (first_time == 0) {
      first_time=1;

      if ((total_exec_time == NULL) || (sf_exec_time== NULL))
        fprintf(stderr, "%25s  %25s  %25s  %25s %25s %6f\n","Name","Total","Per Trials",   "Num Trials","CPU_F_GHz", cpu_freq_GHz);
      else
        fprintf(stderr, "%25s  %25s  %25s  %20s %15s %6f\n","Name","Total","Average/Frame","Trials",    "CPU_F_GHz", cpu_freq_GHz);
    }

    if (ts->trials>0) {
      //printf("%20s: total: %10.3f ms, average: %10.3f us (%10d trials)\n", name, ts->diff/cpu_freq_GHz/1000000.0, ts->diff/ts->trials/cpu_freq_GHz/1000.0, ts->trials);
      if ((total_exec_time == NULL) || (sf_exec_time== NULL)) {
        fprintf(stderr, "%25s:  %15.3f us; %15d; %15.3f us;\n",
                name,
                (ts->diff/ts->trials/cpu_freq_GHz/1000.0),
                ts->trials,
                ts->max/cpu_freq_GHz/1000.0);
      } else {
        fprintf(stderr, "%25s:  %15.3f ms (%5.2f%%); %15.3f us (%5.2f%%); %15d;\n",
                name,
                (ts->diff/cpu_freq_GHz/1000000.0),
                ((ts->diff/cpu_freq_GHz/1000000.0)/(total_exec_time->diff/cpu_freq_GHz/1000000.0))*100,  // percentage
                (ts->diff/ts->trials/cpu_freq_GHz/1000.0),
                ((ts->diff/ts->trials/cpu_freq_GHz/1000.0)/(sf_exec_time->diff/sf_exec_time->trials/cpu_freq_GHz/1000.0))*100,  // percentage
                ts->trials);
      }
    }
  }
}

int print_meas_log(time_stats_t *ts,
                const char *name,
                time_stats_t *total_exec_time,
                time_stats_t *sf_exec_time,
                char *output)
{
  int stroff = 0;
  static int first_time = 0;
  static double cpu_freq_GHz = 0.0;

  if (cpu_freq_GHz == 0.0)
    cpu_freq_GHz = get_cpu_freq_GHz();

  if (first_time == 0) {
    first_time=1;

    if ((total_exec_time == NULL) || (sf_exec_time== NULL))
      stroff += sprintf(output, "%25s  %25s  %25s  %25s %25s %6f\n","Name","Total","Per Trials",   "Num Trials","CPU_F_GHz", cpu_freq_GHz);
    else
      stroff += sprintf(output+stroff, "%25s  %25s  %25s  %20s %15s %6f\n","Name","Total","Average/Frame","Trials",    "CPU_F_GHz", cpu_freq_GHz);
  }

  if (ts->trials>0) {
    //printf("%20s: total: %10.3f ms, average: %10.3f us (%10d trials)\n", name, ts->diff/cpu_freq_GHz/1000000.0, ts->diff/ts->trials/cpu_freq_GHz/1000.0, ts->trials);
    if ((total_exec_time == NULL) || (sf_exec_time== NULL)) {
      stroff += sprintf(output+stroff, "%25s:  %15.3f us; %15d; %15.3f us;\n",
              name,
              (ts->diff/ts->trials/cpu_freq_GHz/1000.0),
              ts->trials,
              ts->max/cpu_freq_GHz/1000.0);
    } else {
      stroff += sprintf(output+stroff, "%25s:  %15.3f ms (%5.2f%%); %15.3f us (%5.2f%%); %15d;\n",
              name,
              (ts->diff/cpu_freq_GHz/1000000.0),
              ((ts->diff/cpu_freq_GHz/1000000.0)/(total_exec_time->diff/cpu_freq_GHz/1000000.0))*100,  // percentage
              (ts->diff/ts->trials/cpu_freq_GHz/1000.0),
              ((ts->diff/ts->trials/cpu_freq_GHz/1000.0)/(sf_exec_time->diff/sf_exec_time->trials/cpu_freq_GHz/1000.0))*100,  // percentage
              ts->trials);
    }
  }
  return stroff;
}

double get_time_meas_us(time_stats_t *ts)
{
  static double cpu_freq_GHz = 0.0;

  if (cpu_freq_GHz == 0.0)
    cpu_freq_GHz = get_cpu_freq_GHz();

  if (ts->trials>0)
    return  (ts->diff/ts->trials/cpu_freq_GHz/1000.0);

  return 0;
}

/* function for the asynchronous measurment module: cpu stat are sent to a dedicated thread
 * which is in charge of computing the cpu time spent in a given function/algorithm...
 */

time_stats_t *register_meas(char *name)
{
  for (int i=0; i<max_cpumeasur; i++) {
    if (measur_table[i] == NULL) {
      measur_table[i] = (time_stats_t *)malloc(sizeof(time_stats_t));
      memset(measur_table[i] ,0,sizeof(time_stats_t));
      measur_table[i]->meas_name = strdup(name);
      measur_table[i]->meas_index = i;
      measur_table[i]->tpoolmsg =newNotifiedFIFO_elt(sizeof(time_stats_msg_t),0,NULL,NULL);
      measur_table[i]->tstatptr = (time_stats_msg_t *)NotifiedFifoData(measur_table[i]->tpoolmsg);
      return measur_table[i];
    }
  }
  return NULL;
}

void free_measurtbl(void) {
  for (int i=0; i<max_cpumeasur; i++) {
    if (measur_table[i] != NULL) {
	  free(measur_table[i]->meas_name);
	  delNotifiedFIFO_elt(measur_table[i]->tpoolmsg);
	  free(measur_table[i]);
    }
  }
  //free the fifo...
}

void run_cpumeasur(void) {
    struct sched_param schedp;
    pthread_setname_np(pthread_self(), "measur");
    schedp.sched_priority=0;
    int rt=pthread_setschedparam(pthread_self(), SCHED_IDLE, &schedp);
    AssertFatal(rt==0, "couldn't set measur thread priority: %s\n",strerror(errno));
    initNotifiedFIFO(&measur_fifo);
    while(1) {
      notifiedFIFO_elt_t *msg = pullNotifiedFIFO(&measur_fifo);
      time_stats_msg_t *tsm = (time_stats_msg_t *)NotifiedFifoData(msg);
        switch(tsm->msgid) {
          case TIMESTAT_MSGID_START:
             measur_table[tsm->timestat_id]->in=tsm->ts;
             (measur_table[tsm->timestat_id]->trials)++;
          break;
          case TIMESTAT_MSGID_STOP:
    /// process duration is the difference between two clock points
             measur_table[tsm->timestat_id]->p_time = (tsm->ts - measur_table[tsm->timestat_id]->in);
             measur_table[tsm->timestat_id]->diff += measur_table[tsm->timestat_id]->p_time;
             if ( measur_table[tsm->timestat_id]->p_time > measur_table[tsm->timestat_id]->max )
               measur_table[tsm->timestat_id]->max = measur_table[tsm->timestat_id]->p_time;
          break;
          case TIMESTAT_MSGID_DISPLAY:
            {
            char aline[256];
            int start, stop;
             if (tsm->displayFunc != NULL) {
               if(tsm->timestat_id >= 0) {
                 start=tsm->timestat_id ;
                 stop=start+1;
               }
               else {
                  start=0;
                  stop=max_cpumeasur ;
               }
               for (int i=start ; i<stop ; i++) {
                 if (measur_table[i] != NULL) {
                   sprintf(aline,"%s: %15.3f us ",measur_table[i]->meas_name, measur_table[i]->trials==0?0:(  (measur_table[i]->trials/measur_table[i]->diff )/ cpu_freq_GHz /1000 ));
                   tsm->displayFunc(aline);
                   }
                }
             }
            }
          break;
          case TIMESTAT_MSGID_END:
            free_measurtbl();
            delNotifiedFIFO_elt(msg);
            pthread_exit(NULL);
          break;
          default:
          break;
      }
    delNotifiedFIFO_elt(msg);
    }
}


void init_meas(void) {
  pthread_t thid;
  paramdef_t cpumeasur_params[] = CPUMEASUR_PARAMS_DESC;
  int numparams=sizeof(cpumeasur_params)/sizeof(paramdef_t);
  int rt = config_get( cpumeasur_params,numparams,CPUMEASUR_SECTION);
  AssertFatal(rt >= 0, "cpumeasur configuration couldn't be performed");
  measur_table=calloc(max_cpumeasur,sizeof( time_stats_t *));
  AssertFatal(measur_table!=NULL, "couldn't allocate %u cpu measurements entries\n",max_cpumeasur);
  rt=pthread_create(&thid,NULL, (void *(*)(void *))run_cpumeasur, NULL);
  AssertFatal(rt==0, "couldn't create cpu measurment thread: %s\n",strerror(errno));
}

void end_meas(void) {
    notifiedFIFO_elt_t *nfe = newNotifiedFIFO_elt(sizeof(time_stats_msg_t),0,NULL,NULL);
	time_stats_msg_t *msg = (time_stats_msg_t *)NotifiedFifoData(nfe);
    msg->msgid = TIMESTAT_MSGID_END ;
    pushNotifiedFIFO(&measur_fifo, nfe);
}
