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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
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
  long long diff_now;
  long long p_time; /*!< \brief absolute process duration */
  long long diff_square; /*!< \brief process duration square */
  long long max;
  int trials;
} time_stats_t;
#elif defined(__arm__)
typedef struct {
  uint32_t in;
  uint32_t diff_now;
  uint32_t diff;
  uint32_t p_time; /*!< \brief absolute process duration */
  uint32_t diff_square; /*!< \brief process duration square */
  uint32_t max;
  int trials;
} time_stats_t;

#endif
static inline void start_meas(time_stats_t *ts) __attribute__((always_inline));
static inline void stop_meas(time_stats_t *ts) __attribute__((always_inline));


void print_meas_now(time_stats_t *ts, const char* name, int subframe, FILE* file_name);
void print_meas(time_stats_t *ts, const char* name, time_stats_t * total_exec_time, time_stats_t * sf_exec_time);
double get_time_meas_us(time_stats_t *ts);
double get_cpu_freq_GHz(void);

#if defined(__i386__)
static inline unsigned long long rdtsc_oai(void) __attribute__((always_inline));
static inline unsigned long long rdtsc_oai(void)
{
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
}
#elif defined(__x86_64__)
static inline unsigned long long rdtsc_oai(void) __attribute__((always_inline));
static inline unsigned long long rdtsc_oai(void)
{
  unsigned long long a, d;
  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  return (d<<32) | a;
}

#elif defined(__arm__)
static inline uint32_t rdtsc_oai(void) __attribute__((always_inline));
static inline uint32_t rdtsc_oai(void)
{
  uint32_t r = 0;
  asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(r) );
  return r;
}
#endif

static inline void start_meas(time_stats_t *ts)
{

  if (opp_enabled) {

      ts->trials++;
      ts->in = rdtsc_oai();
  }
}

static inline void stop_meas(time_stats_t *ts)
{

  if (opp_enabled) {
    long long out = rdtsc_oai();
    
    ts->diff_now = (out-ts->in);
    
    ts->diff_now = (out-ts->in);
    ts->diff += (out-ts->in);
    /// process duration is the difference between two clock points
    ts->p_time = (out-ts->in);
    ts->diff_square += (out-ts->in)*(out-ts->in);
    
    if ((out-ts->in) > ts->max)
      ts->max = out-ts->in;
    
  }
}

static inline void reset_meas(time_stats_t *ts) {

  ts->trials=0;
  ts->diff=0;
  ts->diff_now=0;
  ts->p_time=0;
  ts->diff_square=0;
  ts->max=0;
  
}

static inline void copy_meas(time_stats_t *dst_ts,time_stats_t *src_ts)
{

  if (opp_enabled) {
    dst_ts->trials=src_ts->trials;
    dst_ts->diff=src_ts->diff;
    dst_ts->max=src_ts->max;
  }
}
#endif
