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

#ifndef __TIME_MEAS_DEFS__H__
#define __TIME_MEAS_DEFS__H__

#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include "common/utils/threadPool/thread-pool.h"
// global var to enable openair performance profiler
extern int opp_enabled;
extern double cpu_freq_GHz  __attribute__ ((aligned(32)));;
// structure to store data to compute cpu measurment
#if defined(__x86_64__) || defined(__i386__)
  #define OAI_CPUTIME_TYPE long long
#elif defined(__arm__)
  #define OAI_CPUTIME_TYPE uint32_t
#else
  #error "building on unsupported CPU architecture"
#endif

#define TIMESTAT_MSGID_START       0     /*!< \brief send time at measure starting point */
#define TIMESTAT_MSGID_STOP        1     /*!< \brief send time at measure end  point */
#define TIMESTAT_MSGID_ENABLE      2     /*!< \brief enable measure point */
#define TIMESTAT_MSGID_DISABLE     3     /*!< \brief disable measure point */
#define TIMESTAT_MSGID_DISPLAY     10    /*!< \brief display measure */
#define TIMESTAT_MSGID_END         11    /*!< \brief stops the measure threads and free assocated resources */
typedef void(*meas_printfunc_t)(const char* format, ...);
typedef struct {
  int               msgid;                  /*!< \brief message id, as defined by TIMESTAT_MSGID_X macros */
  int               timestat_id;            /*!< \brief points to the time_stats_t entry in cpumeas table */
  OAI_CPUTIME_TYPE  ts;                     /*!< \brief time stamp */
  meas_printfunc_t  displayFunc;            /*!< \brief function to call when DISPLAY message is received*/
} time_stats_msg_t;


typedef struct {
  OAI_CPUTIME_TYPE in;      /*!< \brief time at measure starting point */
  OAI_CPUTIME_TYPE diff;     /*!< \brief average difference between time at starting point and time at endpoint*/
  OAI_CPUTIME_TYPE p_time; /*!< \brief absolute process duration */
  OAI_CPUTIME_TYPE diff_square; /*!< \brief process duration square */
  OAI_CPUTIME_TYPE max;      /*!< \brief maximum difference between time at starting point and time at endpoint*/
  int trials;                /*!< \brief number of start point - end point iterations */
  int meas_flag;             /*!< \brief 1: stop_meas not called (consecutive calls of start_meas) */
  char *meas_name;           /*!< \brief name to use when printing the measure (not used for PHY simulators)*/
  int meas_index;            /*!< \brief index of this measure in the measure array (not used for PHY simulators)*/
  int meas_enabled;         /*!< \brief per measure enablement flag. send_meas tests this flag, unused today in start_meas and stop_meas*/
  notifiedFIFO_elt_t *tpoolmsg; /*!< \brief message pushed to the cpu measurment queue to report a measure START or STOP */
  time_stats_msg_t *tstatptr;   /*!< \brief pointer to the time_stats_msg_t data in the tpoolmsg, stored here for perf considerations*/
} time_stats_t;
#define MEASURE_ENABLED(X)       (X->meas_enabled)




static inline void start_meas(time_stats_t *ts) __attribute__((always_inline));
static inline void stop_meas(time_stats_t *ts) __attribute__((always_inline));


void print_meas_now(time_stats_t *ts, const char *name, FILE *file_name);
void print_meas(time_stats_t *ts, const char *name, time_stats_t *total_exec_time, time_stats_t *sf_exec_time);
int print_meas_log(time_stats_t *ts, const char *name, time_stats_t *total_exec_time, time_stats_t *sf_exec_time, char *output);
double get_time_meas_us(time_stats_t *ts);
double get_cpu_freq_GHz(void);

#if defined(__i386__)
static inline unsigned long long rdtsc_oai(void) __attribute__((always_inline));
static inline unsigned long long rdtsc_oai(void) {
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
}
#elif defined(__x86_64__)
static inline unsigned long long rdtsc_oai(void) __attribute__((always_inline));
static inline unsigned long long rdtsc_oai(void) {
  unsigned long long a, d;
  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  return (d<<32) | a;
}

#elif defined(__arm__)
static inline uint32_t rdtsc_oai(void) __attribute__((always_inline));
static inline uint32_t rdtsc_oai(void) {
  uint32_t r = 0;
  asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(r) );
  return r;
}
#endif

#define CPUMEAS_DISABLE  0
#define CPUMEAS_ENABLE   1
#define CPUMEAS_GETSTATE 2
int cpumeas(int action);
static inline void start_meas(time_stats_t *ts) {
  if (opp_enabled) {
    if (ts->meas_flag==0) {
      ts->trials++;
      ts->in = rdtsc_oai();
      ts->meas_flag=1;
    } else {
      ts->in = rdtsc_oai();
    }
  }
}

static inline void stop_meas(time_stats_t *ts) {
  if (opp_enabled) {
    long long out = rdtsc_oai();
    ts->diff += (out-ts->in);
    /// process duration is the difference between two clock points
    ts->p_time = (out-ts->in);
    ts->diff_square += (out-ts->in)*(out-ts->in);

    if ((out-ts->in) > ts->max)
      ts->max = out-ts->in;

    ts->meas_flag=0;
  }
}

static inline void reset_meas(time_stats_t *ts) {
  ts->in=0;
  ts->diff=0;
  ts->p_time=0;
  ts->diff_square=0;
  ts->max=0;
  ts->trials=0;
  ts->meas_flag=0;
}

static inline void copy_meas(time_stats_t *dst_ts,time_stats_t *src_ts) {
  if (opp_enabled) {
    dst_ts->trials=src_ts->trials;
    dst_ts->diff=src_ts->diff;
    dst_ts->max=src_ts->max;
  }
}

extern notifiedFIFO_t measur_fifo;
#define CPUMEASUR_SECTION "cpumeasur"

#define CPUMEASUR_PARAMS_DESC { \
    {"max_cpumeasur",     "Max number of cpu measur entries",      0,       uptr:&max_cpumeasur,           defintval:100,         TYPE_UINT,   0},\
  }

  void init_meas(void);
  time_stats_t *register_meas(char *name);
  #define START_MEAS(X) send_meas(X, TIMESTAT_MSGID_START)
  #define STOP_MEAS(X)  send_meas(X, TIMESTAT_MSGID_STOP)
  static inline void send_meas(time_stats_t *ts, int msgid) {
    if (MEASURE_ENABLED(ts) ) {
      ts->tstatptr->timestat_id=ts->meas_index;
      ts->tstatptr->msgid = msgid ;
      ts->tstatptr->ts = rdtsc_oai();
      pushNotifiedFIFO(&measur_fifo, ts->tpoolmsg);
    }
  }
  void end_meas(void);

#endif
