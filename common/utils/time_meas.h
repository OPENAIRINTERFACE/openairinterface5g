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
// global var to enable openair performance profiler
extern int opp_enabled;
extern double cpu_freq_GHz  __attribute__ ((aligned(32)));;
// structure to store data to compute cpu measurment
#if defined(__x86_64__) || defined(__i386__)
  typedef long long oai_cputime_t;
#elif defined(__arm__) || defined(__aarch64__)
  typedef uint32_t oai_cputime_t;
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
  oai_cputime_t  ts;                        /*!< \brief time stamp */
  meas_printfunc_t  displayFunc;            /*!< \brief function to call when DISPLAY message is received*/
} time_stats_msg_t;

struct notifiedFIFO_elt_s;
typedef struct time_stats {
  oai_cputime_t in;          /*!< \brief time at measure starting point */
  oai_cputime_t diff;        /*!< \brief average difference between time at starting point and time at endpoint*/
  oai_cputime_t p_time;      /*!< \brief absolute process duration */
  double diff_square;        /*!< \brief process duration square */
  oai_cputime_t max;         /*!< \brief maximum difference between time at starting point and time at endpoint*/
  int trials;                /*!< \brief number of start point - end point iterations */
  int meas_flag;             /*!< \brief 1: stop_meas not called (consecutive calls of start_meas) */
  char *meas_name;           /*!< \brief name to use when printing the measure (not used for PHY simulators)*/
  int meas_index;            /*!< \brief index of this measure in the measure array (not used for PHY simulators)*/
  int meas_enabled;         /*!< \brief per measure enablement flag. send_meas tests this flag, unused today in start_meas and stop_meas*/
  struct notifiedFIFO_elt_s *tpoolmsg; /*!< \brief message pushed to the cpu measurment queue to report a measure START or STOP */
  time_stats_msg_t *tstatptr;   /*!< \brief pointer to the time_stats_msg_t data in the tpoolmsg, stored here for perf considerations*/
} time_stats_t;
#define MEASURE_ENABLED(X)       (X->meas_enabled)




static inline void start_meas(time_stats_t *ts) __attribute__((always_inline));
static inline void stop_meas(time_stats_t *ts) __attribute__((always_inline));


void print_meas_now(time_stats_t *ts, const char *name, FILE *file_name);
void print_meas(time_stats_t *ts, const char *name, time_stats_t *total_exec_time, time_stats_t *sf_exec_time);
size_t print_meas_log(time_stats_t *ts,
                      const char *name,
                      time_stats_t *total_exec_time,
                      time_stats_t *sf_exec_time,
                      char *output,
                      size_t outputlen);
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
#elif defined(__aarch64__)
static inline uint64_t rdtsc_oai(void) __attribute__((always_inline));
static inline uint64_t rdtsc_oai(void)
{
	  uint64_t r = 0;
	    asm volatile("mrs %0, cntvct_el0" : "=r"(r));
	      return r;
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
static inline int cpumeas(int action) {
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

static inline void start_meas(time_stats_t *ts) {
  if (opp_enabled) {
    if (ts->meas_flag==0) {
      ts->trials++;
      ts->in = rdtsc_oai();
      ts->meas_flag=1;
    } else {
      ts->in = rdtsc_oai();
    }
    if ((ts->trials&16383)<10) ts->max=0;
  }
}

static inline void stop_meas(time_stats_t *ts) {
  if (opp_enabled) {
    long long out = rdtsc_oai();
    if (ts->in) {
      ts->diff += (out - ts->in);
      /// process duration is the difference between two clock points
      ts->p_time = (out - ts->in);
      ts->diff_square += ((double)out - ts->in) * ((double)out - ts->in);

      if ((out - ts->in) > ts->max)
        ts->max = out - ts->in;

      ts->meas_flag = 0;
    }
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

static inline void merge_meas(time_stats_t *dst_ts, const time_stats_t *src_ts)
{
  if (!opp_enabled)
    return;
  dst_ts->trials += src_ts->trials;
  dst_ts->diff += src_ts->diff;
  if (src_ts->max > dst_ts->max)
    dst_ts->max = src_ts->max;
}

#define CPUMEASUR_SECTION "cpumeasur"

// clang-format off
#define CPUMEASUR_PARAMS_DESC { \
  {"max_cpumeasur",     "Max number of cpu measur entries",      0,       .uptr=&max_cpumeasur,           .defintval=100,         TYPE_UINT,   0}, \
}
// clang-format on

void init_meas(void);
time_stats_t *register_meas(char *name);
#define START_MEAS(X) send_meas(X, TIMESTAT_MSGID_START)
#define STOP_MEAS(X)  send_meas(X, TIMESTAT_MSGID_STOP)
void send_meas(time_stats_t *ts, int msgid);
void end_meas(void);

#define timeIt(a)                                           \
  {                                                         \
    uint64_t deb = rdtsc_oai();                             \
    a;                                                      \
    LOG_W(UTIL, #a ": %llu\n", (rdtsc_oai() - deb) / 3000); \
  }
#endif
