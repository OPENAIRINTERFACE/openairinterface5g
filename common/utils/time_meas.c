/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#define _GNU_SOURCE
#define BINARY_SEARCH
#include <stdio.h>
#include "time_meas.h"
#include <math.h>
#include <unistd.h>
#include <string.h>
#include "assertions.h"
#include <pthread.h>
#include "common/config/config_userapi.h"
#include "common/utils/threadPool/notified_fifo.h"
// global var for openair performance profiler
int cpu_meas_enabled = 0;

static uint32_t    max_cpumeasur;
static time_stats_t  **measur_table;
notifiedFIFO_t measur_fifo;

double get_std_dev(time_stats_t *ptr)
{
  return sqrt((double)ptr->diff_square * 1e-6 / ptr->trials - pow((double)ptr->diff / ptr->trials / 1000, 2));
}


void print_meas_now(time_stats_t *ts, const char *name, FILE *file_name)
{
  UNUSED(name);
  if (cpu_meas_enabled) {
    if (ts->trials > 0) {
      fprintf(file_name, "%15.3f us, diff_now %15.3f \n", (ts->p_time / 1000.0), (double)ts->p_time);
    }
  }
}

void print_meas_header(time_stats_t *total_exec_time,
                       time_stats_t *sf_exec_time)
{
  if ((total_exec_time == NULL) || (sf_exec_time== NULL))
    fprintf(stderr,
            "%25s   %18s  %18s  %18s  %15s  %18s  %18s  %18s  %18s  %18s  %18s\n",
            "Name",
            "Total",
            "Max",
            "Std",
            "Num Trials",
            "min",
            "d1",
            "q1",
            "median",
            "q3",
            "d9");
  else
    fprintf(stderr,
            "%25s   %18s  %18s  %15s\n",
            "Name",
            "Total",
            "Average/Frame",
            "Trials");
}

void print_meas(time_stats_t *ts,
                const char *name,
                time_stats_t *total_exec_time,
                time_stats_t *sf_exec_time)
{
  if (ts->trials>0) {
    if ((total_exec_time == NULL) || (sf_exec_time == NULL)) {
      if (is_enabled_time_stats_sorted_list(&ts->time_stats_sorted_list)) {
        fprintf(stderr,
                "%25s:  %15.3f us; %15.3f us; %15.3f us; %15d; %15.3f us; %15.3f us; %15.3f us; %15.3f us; %15.3f us; %15.3f us;\n",
                name,
                ts->diff / ts->trials / 1000.0,
                ts->max / 1000.0,
                get_std_dev(ts),
                ts->trials,
                get_min(&ts->time_stats_sorted_list) / 1000.0,
                get_d1(&ts->time_stats_sorted_list) / 1000.0,
                get_q1(&ts->time_stats_sorted_list) / 1000.0,
                get_median(&ts->time_stats_sorted_list) / 1000.0,
                get_q3(&ts->time_stats_sorted_list) / 1000.0,
                get_d9(&ts->time_stats_sorted_list) / 1000.0);
      } else {
        fprintf(stderr,
                "%25s:  %15.3f us; %15.3f us; %15.3f us; %15d;\n",
                name,
                ts->diff / ts->trials / 1000.0,
                ts->max / 1000.0,
                get_std_dev(ts),
                ts->trials);
      }
    } else {
      fprintf(stderr,
              "%30s:  %15.3f ms (%5.2f%%); %15.3f us (%5.2f%%); %15d;\n",
              name,
              ts->diff / 1000000.0,
              ((ts->diff / 1000000.0) / (total_exec_time->diff / 1000000.0)) * 100,  // percentage
              (ts->diff / ts->trials / 1000.0),
              ((ts->diff / ts->trials / 1000.0) / (sf_exec_time->diff / sf_exec_time->trials / 1000.0)) * 100,  // percentage
              ts->trials);
    }
  }
}

size_t print_meas_log_header(time_stats_t *total_exec_time,
                             time_stats_t *sf_exec_time,
                             char *output,
                             size_t outputlen,
                             int cpu_meas_enabled)
{
  const char *begin = output;
  const char *end = output + outputlen;

  if ((total_exec_time == NULL) || (sf_exec_time== NULL))
    if(cpu_meas_enabled == TIME_STATS_ADVANCED_MODE)
      output += snprintf(output,
                         end - output,
                         "%25s   %18s  %18s  %18s  %15s  %18s  %18s  %18s  %18s  %18s  %18s\n",
                         "Name",
                         "Total",
                         "Max",
                         "Std",
                         "Num Trials",
                         "min",
                         "d1",
                         "q1",
                         "median",
                         "q3",
                         "d9");
    else
      output += snprintf(output,
                         end - output,
                         "%25s   %18s  %18s  %18s  %15s\n",
                         "Name",
                         "Total",
                         "Max",
                         "Std",
                         "Num Trials");
  else
    output += snprintf(output,
                       end - output,
                       "%25s   %18s  %18s  %15s\n",
                       "Name",
                       "Total",
                       "Average/Frame",
                       "Trials");

  return output - begin;
}

size_t print_meas_log(time_stats_t *ts,
                      const char *name,
                      time_stats_t *total_exec_time,
                      time_stats_t *sf_exec_time,
                      char *output,
                      size_t outputlen)
{
  const char *begin = output;
  const char *end = output + outputlen;

  if (ts->trials > 0) {
    if ((total_exec_time == NULL) || (sf_exec_time == NULL)) {
      if (is_enabled_time_stats_sorted_list(&ts->time_stats_sorted_list)) {
        output += snprintf(output,
                           end - output,
                           "%25s:  %15.3f us; %15.3f us; %15.3f us; %15d; %15.3f us; %15.3f us; %15.3f us; %15.3f us; %15.3f us; %15.3f us;\n",
                           name,
                           ts->diff / ts->trials / 1000.0,
                           ts->max / 1000.0,
                           get_std_dev(ts),
                           ts->trials,
                           get_min(&ts->time_stats_sorted_list) / 1000.0,
                           get_d1(&ts->time_stats_sorted_list) / 1000.0,
                           get_q1(&ts->time_stats_sorted_list) / 1000.0,
                           get_median(&ts->time_stats_sorted_list) / 1000.0,
                           get_q3(&ts->time_stats_sorted_list) / 1000.0,
                           get_d9(&ts->time_stats_sorted_list) / 1000.0);
      } else {
        output += snprintf(output,
                           end - output,
                           "%25s:  %15.3f us; %15.3f us; %15.3f us; %15d;\n",
                           name,
                           ts->diff / ts->trials / 1000.0,
                           ts->max / 1000.0,
                           get_std_dev(ts),
                           ts->trials);
      }
    } else {
      output += snprintf(output,
                         end - output,
                         "%25s:  %15.3f ms (%5.2f%%); %15.3f us (%5.2f%%); %15d;\n",
                         name,
                         ts->diff / 1000000.0,
                         ((ts->diff / 1000000.0) / (total_exec_time->diff / 1000000.0)) * 100,  // percentage
                         ts->diff / ts->trials / 1000.0,
                         ((ts->diff / ts->trials / 1000.0) / (sf_exec_time->diff / sf_exec_time->trials / 1000.0)) * 100,  // percentage
                         ts->trials);
    }
  }
  return output - begin;
}

double get_time_meas_us(time_stats_t *ts)
{
  if (ts->trials > 0)
    return  (ts->diff / ts->trials / 1000.0);

  return 0;
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
                   sprintf(aline,"%s: %15.3f us ",measur_table[i]->meas_name, measur_table[i]->trials==0?0:(  (float) measur_table[i]->diff/measur_table[i]->trials  /1000 ));
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
  int numparams = sizeofArray(cpumeasur_params);
  int rt = config_get(config_get_if(), cpumeasur_params, numparams, CPUMEASUR_SECTION);
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

/**
 * \brief initializes sorted list
 * if dst is already initialized then asserts
 * \param list sorted list to be initialized
 * \param size size of the sorted list
 */
void init_time_stats_sorted_list(time_stats_sorted_list_t *list, unsigned int size)
{
  if (list == NULL)
    return;

  AssertFatal(list->magic != TIME_STATS_SORTED_LIST_MAGIC,
              "Calling init_time_stats_sorted_list on initialized sorted list\n");

  list->size = 0;
  list->nb_elm = 0;
  list->list = NULL;
  list->magic = 0;

  if (size == 0)
    return;

  list->list = calloc(size, sizeof(oai_cputime_t));
  AssertFatal(list->list != NULL, "Could not allocate sorted list for time stats\n");

  list->size = size;
  list->nb_elm = 0;
  list->magic = TIME_STATS_SORTED_LIST_MAGIC;
}

/**
 * \brief free sorted list
 * if dst is already free then does nothing
 * \param list sorted list to be freed
 */
void free_time_stats_sorted_list(time_stats_sorted_list_t *list)
{
  if (list == NULL)
    return;

  if (list->magic == TIME_STATS_SORTED_LIST_MAGIC && list->list != NULL)
    free(list->list);

  list->size = 0;
  list->nb_elm = 0;
  list->magic = 0;
  list->list = NULL;
}

/**
 * \brief returns true if the sorted list is enabled and false otherwise
 * \param list sorted list to be tested
 */
int is_enabled_time_stats_sorted_list(const time_stats_sorted_list_t *list)
{
  return list != NULL
         && list->magic == TIME_STATS_SORTED_LIST_MAGIC
         && list->size > 0
         && list->list != NULL
         && list->nb_elm <= list->size;
}

/**
 * \brief empties sorted list
 * if dst is not initialized then does nothing
 * \param list sorted list to be emptied
 */
void reset_time_stats_sorted_list(time_stats_sorted_list_t *list)
{
  if (!is_enabled_time_stats_sorted_list(list))
    return;

  list->nb_elm = 0;
}

#ifdef BINARY_SEARCH
/**
 * \brief searches an index in sorted list dst
 * between low_bound and high_bound to insert value
 * \param value value to search an index for
 * \param dst destination sorted list
 * \param low_bound lower bound for binary search
 * \param high_bound higher bound for binary search
 */
static inline unsigned int binary_search(oai_cputime_t value, time_stats_sorted_list_t *dst, unsigned int low_bound, unsigned int high_bound)
{
  unsigned int low = low_bound;
  unsigned int high = high_bound;
  bool converged = false;
  unsigned int i;
  while (!converged) {
    i = (high + low) / 2;
    if (i > 0 && dst->list[i - 1] > value) {
      high = i - 1;
    } else if (i < dst->nb_elm && dst->list[i] < value) {
      low = i + 1;
    } else {
      converged = true;
    }
  }
  return i;
}
#endif

/**
 * \brief inserts value sorted list
 * if dst is not initialized then does nothing
 * if dst is full then does nothing
 * \param list sorted list to insert in
 * \param time time value to insert
 */
void insert_in_time_stats_sorted_list(time_stats_sorted_list_t *list, oai_cputime_t time)
{
  if (!is_enabled_time_stats_sorted_list(list))
    return;

  if (list->nb_elm < list->size) {
      unsigned int i = 0;
#ifdef BINARY_SEARCH
      i = binary_search(time, list, 0, list->nb_elm);
#else
      for (; i < list->nb_elm && list->list[i] < time; i++);
#endif
      // dst and src may overlap => use memmove rather than memcpy
      memmove(&list->list[i+1], &list->list[i], (list->nb_elm - i) * sizeof(oai_cputime_t));
      list->list[i] = time;
      list->nb_elm++;
  }
}

/**
 * \brief copy sorted list src into dst, freeing and replacing dst
 * dst and src should be initialized, otherwise does nothing
 * \param dst destination sorted list
 * should be intitialized even with a dummy size 1 buffer to make sure that copying the list there is expected by the caller
 * \param src source sorted list
 */
void copy_time_stats_sorted_list(time_stats_sorted_list_t *dst, const time_stats_sorted_list_t *src)
{
  if (!is_enabled_time_stats_sorted_list(dst) || !is_enabled_time_stats_sorted_list(src))
    return;

  if (dst->size != src->size) {
    free_time_stats_sorted_list(dst);
    init_time_stats_sorted_list(dst, src->size);
  }

  if (!is_enabled_time_stats_sorted_list(dst))
    return;

  memcpy(dst->list, src->list, src->nb_elm * sizeof(oai_cputime_t));
  dst->nb_elm = src->nb_elm;
}

/**
 * \brief inserts the content of sorted list src into dst
 * dst and src should be initialized, otherwise does nothing
 * if dst is not large enough to copy src then does nothing
 * \param dst destination sorted list
 * \param src source sorted list
 */
void merge_time_stats_sorted_list(time_stats_sorted_list_t *dst, const time_stats_sorted_list_t *src)
{
  if (is_enabled_time_stats_sorted_list(dst) && is_enabled_time_stats_sorted_list(src)) {
    if ((dst->size - dst->nb_elm) >= src->nb_elm) {
      unsigned int j = 0;
      for (unsigned int i = 0; i < src->nb_elm; i++) {
#ifdef BINARY_SEARCH
        j = binary_search(src->list[i], dst, j + 1, dst->nb_elm);
#else
        for (; j < dst->nb_elm && dst->list[j] < src->list[i]; j++);
#endif
        // dst and src may overlap => use memmove rather than memcpy
        memmove(&dst->list[j+1], &dst->list[j], (dst->nb_elm - j) * sizeof(oai_cputime_t));
        dst->list[j] = src->list[i];
        dst->nb_elm++;
        j++;
      }
    }
  }
}

/**
 * \brief get the minimum from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param list sorted list to query
 */
oai_cputime_t get_min(time_stats_sorted_list_t *list)
{
  if (is_enabled_time_stats_sorted_list(list) && list->nb_elm > 0)
    return list->list[0];

  return -1;
}

/**
 * \brief get the median from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param list sorted list to query
 */
oai_cputime_t get_median(time_stats_sorted_list_t *list)
{
  if (is_enabled_time_stats_sorted_list(list) && list->nb_elm > 0)
    return list->list[list->nb_elm / 2];

  return -1;
}

/**
 * \brief get the first quartile from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param list sorted list to query
 */
oai_cputime_t get_q1(time_stats_sorted_list_t *list)
{
  if (is_enabled_time_stats_sorted_list(list) && list->nb_elm > 0)
    return list->list[list->nb_elm / 4];

  return -1;
}

/**
 * \brief get the third quartile from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param list sorted list to query
 */
oai_cputime_t get_q3(time_stats_sorted_list_t *list)
{
  if (is_enabled_time_stats_sorted_list(list) && list->nb_elm > 0)
    return list->list[3 * list->nb_elm / 4];

  return -1;
}

/**
 * \brief get the first decile from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param list sorted list to query
 */
oai_cputime_t get_d1(time_stats_sorted_list_t *list)
{
  if (is_enabled_time_stats_sorted_list(list) && list->nb_elm > 0)
    return list->list[list->nb_elm / 10];

  return -1;
}

/**
 * \brief get the nineth decile from a sorted list
 * if the sorted list is not initialized or empty then returns -1
 * \param list sorted list to query
 */
oai_cputime_t get_d9(time_stats_sorted_list_t *list)
{
  if (is_enabled_time_stats_sorted_list(list) && list->nb_elm > 0)
    return list->list[9 * list->nb_elm / 10];

  return -1;
}
