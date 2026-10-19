/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
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
extern int cpu_meas_enabled;
// structure to store data to compute cpu measurment
#if defined(__x86_64__) || defined(__i386__) || defined(__arm__) || defined(__aarch64__)
  typedef long long oai_cputime_t;
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

/**
 * \typedef time_stats_sorted_list_t
 * \brief sorted list of time stats to get med, q1, q2
 * it can be left disabled by leaving size equal to 0
 * \var size allocated size of the list
 * 0 is the sorted list is disabled
 * \var nb_elm number of elements in the list
 * \var list pointer to the list
 */
/*
 * Marker set only by init_time_stats_sorted_list().
 * It prevents an uninitialized time_stats_t from being mistaken for an
 * enabled sorted list when its size field contains garbage.
 */
#define TIME_STATS_SORTED_LIST_MAGIC 0x51A7BEEF

typedef struct {
  unsigned int size;
  unsigned int nb_elm;
  uint32_t magic;
  oai_cputime_t *list;
} time_stats_sorted_list_t;

/**
 * \brief initializes sorted list
 * if dst is already initialized then asserts
 * \param time_stats_sorted_list sorted list to be initialized
 * \param size size of the sorted list
 */
void init_time_stats_sorted_list(time_stats_sorted_list_t *list, unsigned int size);

/**
 * \brief free sorted list
 * if dst is already free then does nothing
 * \param time_stats_sorted_list sorted list to be freed
 */
void free_time_stats_sorted_list(time_stats_sorted_list_t *list);

/**
 * \brief returns true if the sorted list is enabled and false otherwise
 * \param time_stats_sorted_list sorted list to be tested
 */
int is_enabled_time_stats_sorted_list(const time_stats_sorted_list_t *list);

/**
 * \brief empties sorted list
 * if dst is not initialized then does nothing
 * \param time_stats_sorted_list sorted list to be emptied
 */
void reset_time_stats_sorted_list(time_stats_sorted_list_t *list);

/**
 * \brief inserts value sorted list
 * if dst is not initialized then does nothing
 * if dst is full then does nothing
 * \param time_stats_sorted_list sorted list to insert in
 * \param time time value to insert
 */
void insert_in_time_stats_sorted_list(time_stats_sorted_list_t *list, oai_cputime_t time);

/**
 * \brief copy sorted list src into dst, freeing and replacing dst
 * dst and src should be initialized, otherwise does nothing
 * \param dst destination sorted list
 * should be intitialized even with a dummy size 1 buffer to make sure that copying the list there is expected by the caller
 * \param src source sorted list
 */
void copy_time_stats_sorted_list(time_stats_sorted_list_t *dst, const time_stats_sorted_list_t *src);

/**
 * \brief inserts the content of sorted list src into dst
 * dst and src should be initialized, otherwise does nothing
 * if dst is not large enough to copy src then does nothing
 * \param dst destination sorted list
 * \param src source sorted list
 */
void merge_time_stats_sorted_list(time_stats_sorted_list_t *dst, const time_stats_sorted_list_t *src);

/**
 * \brief get the minimum from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param time_stats_sorted_list sorted list to query
 */
oai_cputime_t get_min(time_stats_sorted_list_t *list);

/**
 * \brief get the median from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param time_stats_sorted_list sorted list to query
 */
oai_cputime_t get_median(time_stats_sorted_list_t *list);

/**
 * \brief get the first quartile from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param time_stats_sorted_list sorted list to query
 */
oai_cputime_t get_q1(time_stats_sorted_list_t *list);

/**
 * \brief get the third quartile from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param time_stats_sorted_list sorted list to query
 */
oai_cputime_t get_q3(time_stats_sorted_list_t *list);

/**
 * \brief get the first decile from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param time_stats_sorted_list sorted list to query
 */
oai_cputime_t get_d1(time_stats_sorted_list_t *list);

/**
 * \brief get the nineth decile from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param time_stats_sorted_list sorted list to query
 */
oai_cputime_t get_d9(time_stats_sorted_list_t *list);

struct notifiedFIFO_elt_s;
typedef struct time_stats {
  oai_cputime_t in;                                /*!< \brief time at measure starting point */
  oai_cputime_t diff;                              /*!< \brief average difference between time at starting point and time at endpoint*/
  oai_cputime_t p_time;                            /*!< \brief absolute process duration */
  double diff_square;                              /*!< \brief process duration square */
  oai_cputime_t max;                               /*!< \brief maximum difference between time at starting point and time at endpoint*/
  int trials;                                      /*!< \brief number of start point - end point iterations */
  int meas_flag;                                   /*!< \brief 1: stop_meas not called (consecutive calls of start_meas) */
  char *meas_name;                                 /*!< \brief name to use when printing the measure (not used for PHY simulators)*/
  int meas_index;                                  /*!< \brief index of this measure in the measure array (not used for PHY simulators)*/
  struct notifiedFIFO_elt_s *tpoolmsg;             /*!< \brief message pushed to the cpu measurment queue to report a measure START or STOP */
  time_stats_msg_t *tstatptr;                      /*!< \brief pointer to the time_stats_msg_t data in the tpoolmsg, stored here for perf considerations */
  time_stats_sorted_list_t time_stats_sorted_list; /*!< \brief optional sorted list to get med, q1, q2 */
} time_stats_t;

static inline void start_meas(time_stats_t *ts) __attribute__((always_inline));
static inline void stop_meas(time_stats_t *ts) __attribute__((always_inline));

/**
 * \brief get the standard deviation of a timer
 * \param ptr timer to query
 */
double get_std_dev(time_stats_t *ptr);
void print_meas_now(time_stats_t *ts, const char *name, FILE *file_name);
void print_meas(time_stats_t *ts, const char *name, time_stats_t *total_exec_time, time_stats_t *sf_exec_time);
size_t print_meas_log_header(time_stats_t *total_exec_time,
                             time_stats_t *sf_exec_time,
                             char *output,
                             size_t outputlen,
                             int cpu_meas_enabled);
size_t print_meas_log(time_stats_t *ts,
                      const char *name,
                      time_stats_t *total_exec_time,
                      time_stats_t *sf_exec_time,
                      char *output,
                      size_t outputlen);
double get_time_meas_us(time_stats_t *ts);

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

static inline long long clock_gettime_oai()
{
  struct timespec time;
#ifdef CLOCK_MONOTONIC_RAW
  // CLOCK_MONOTONIC_RAW only on linux
  // See clock_getres(2)
  clock_gettime(CLOCK_MONOTONIC_RAW, &time);
#else
  clock_gettime(CLOCK_REALTIME, &time);
#endif
  return 1000000000LL * time.tv_sec + time.tv_nsec;
}

#define CPUMEAS_DISABLE  0
#define CPUMEAS_ENABLE   1
#define CPUMEAS_GETSTATE 2
static inline int cpumeas(int action) {
  switch (action) {
    case CPUMEAS_ENABLE:
      cpu_meas_enabled = 1;
      break;

    case CPUMEAS_DISABLE:
      cpu_meas_enabled = 0;
      break;

    case CPUMEAS_GETSTATE:
    default:
      break;
  }

  return cpu_meas_enabled;
}

static inline void start_meas(time_stats_t *ts) {
  if (cpu_meas_enabled) {
    if (ts->meas_flag==0) {
      ts->trials++;
      ts->in = clock_gettime_oai();
      ts->meas_flag=1;
    } else {
      ts->in = clock_gettime_oai();
    }
    if ((ts->trials&16383)<10) ts->max=0;
  }
}

static inline void stop_meas(time_stats_t *ts) {
  if (cpu_meas_enabled) {
    long long out = clock_gettime_oai();
    if (ts->in) {
      ts->diff += (out - ts->in);
      /// process duration is the difference between two clock points
      ts->p_time = (out - ts->in);
      ts->diff_square += ((double)out - ts->in) * ((double)out - ts->in);

      if ((out - ts->in) > ts->max)
        ts->max = out - ts->in;

      insert_in_time_stats_sorted_list(&ts->time_stats_sorted_list, (out - ts->in));
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
  reset_time_stats_sorted_list(&ts->time_stats_sorted_list);
}

static inline void copy_meas(time_stats_t *dst_ts,time_stats_t *src_ts) {
  if (cpu_meas_enabled) {
    dst_ts->trials=src_ts->trials;
    dst_ts->diff=src_ts->diff;
    dst_ts->max=src_ts->max;
    copy_time_stats_sorted_list(&dst_ts->time_stats_sorted_list, &src_ts->time_stats_sorted_list);
  }
}

static inline void merge_meas(time_stats_t *dst_ts, const time_stats_t *src_ts)
{
  if (!cpu_meas_enabled)
    return;
  dst_ts->trials += src_ts->trials;
  dst_ts->diff += src_ts->diff;
  dst_ts->diff_square += src_ts->diff_square;
  if (src_ts->max > dst_ts->max)
    dst_ts->max = src_ts->max;
  if (is_enabled_time_stats_sorted_list(&src_ts->time_stats_sorted_list)) {
    merge_time_stats_sorted_list(&dst_ts->time_stats_sorted_list, &src_ts->time_stats_sorted_list);
  } else if (src_ts->trials == 1) {
    insert_in_time_stats_sorted_list(&dst_ts->time_stats_sorted_list, src_ts->max);
  }
}

#define TIME_STATS_ADVANCED_MODE 2

static inline void init_sorted_list_meas(time_stats_t *ts, unsigned int size)
{
  init_time_stats_sorted_list(&ts->time_stats_sorted_list, size);
}

static inline void free_sorted_list_meas(time_stats_t *ts)
{
  free_time_stats_sorted_list(&ts->time_stats_sorted_list);
}

#define CPUMEASUR_SECTION "cpumeasur"

// clang-format off
#define CPUMEASUR_PARAMS_DESC { \
  {"max_cpumeasur",     "Max number of cpu measur entries",      0,       .uptr=&max_cpumeasur,           .defintval=100,         TYPE_UINT,   0}, \
}
// clang-format on

void init_meas(void);
void end_meas(void);

#endif
