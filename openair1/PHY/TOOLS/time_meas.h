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
double cpu_freq_GHz;
#if defined(__x86_64__) || defined(__i386__)

typedef struct {

  long long in;
  long long diff;
  long long p_time; /*!< \brief absolute process duration */
  long long diff_square; /*!< \brief process duration square */
  long long max;
  int trials;
  int meas_flag;
} time_stats_t;
#elif defined(__arm__)
typedef struct {
  uint32_t in;
  uint32_t diff;
  uint32_t p_time; /*!< \brief absolute process duration */
  uint32_t diff_square; /*!< \brief process duration square */
  uint32_t max;
  int trials;
} time_stats_t;

#endif
static inline void start_meas(time_stats_t *ts) __attribute__((always_inline));
static inline void stop_meas(time_stats_t *ts) __attribute__((always_inline));


void print_meas_now(time_stats_t *ts, const char *name, FILE *file_name);
void print_meas(time_stats_t *ts, const char *name, time_stats_t *total_exec_time, time_stats_t *sf_exec_time);
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
  ts->trials=0;
  ts->diff=0;
  ts->p_time=0;
  ts->diff_square=0;
  ts->max=0;
  ts->meas_flag=0;
}

static inline void copy_meas(time_stats_t *dst_ts,time_stats_t *src_ts) {
  if (opp_enabled) {
    dst_ts->trials=src_ts->trials;
    dst_ts->diff=src_ts->diff;
    dst_ts->max=src_ts->max;
  }
}
#endif
