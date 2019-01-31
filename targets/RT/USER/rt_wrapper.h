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
* \brief provides a wrapper for the timing function for real-time opeartions. It also implements an API for the SCHED_DEADLINE kernel scheduler.
* \author F. Kaltenberger and Navid Nikaein
* \date 2013
* \version 0.1
* \company Eurecom
* \email: florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
* \note
* \warning This code will be removed when a legacy libc API becomes available.
*/

#ifndef _RT_WRAPPER_H_
#define _RT_WRAPPER_H_

#define _GNU_SOURCE
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <syscall.h>
#include <math.h>
#include <sched.h> 
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <sys/sysinfo.h>

#include "common/utils/LOG/log_extern.h"
#include "msc.h"

#define RTIME long long int

#define rt_printk printf

void set_latency_target(void);

RTIME rt_get_time_ns (void);

int rt_sleep_ns (RTIME x);

void check_clock(void);

int fill_modeled_runtime_table(uint16_t runtime_phy_rx[29][6],uint16_t runtime_phy_tx[29][6]);

double get_runtime_tx(int tx_subframe, uint16_t runtime_phy_tx[29][6],uint32_t mcs, int N_RB_DL,double cpuf,int nb_tx_antenna);

double get_runtime_rx(int rx_subframe, uint16_t runtime_phy_rx[29][6], uint32_t mcs, int N_RB_DL,double cpuf,int nb_rx_antenna);
/**
 * see https://www.kernel.org/doc/Documentation/scheduler/sched-deadline.txt  or
 * http://www.blaess.fr/christophe/2014/04/05/utiliser-un-appel-systeme-inconnu-de-la-libc/
 */
#ifdef DEADLINE_SCHEDULER

#define gettid() syscall(__NR_gettid)

#define SCHED_DEADLINE  6

/* XXX use the proper syscall numbers */
#ifdef __x86_64__
#define __NR_sched_setattr   314
#define __NR_sched_getattr   315
#endif

#ifdef __i386__
#define __NR_sched_setattr   351
#define __NR_sched_getattr   352
#endif

struct sched_attr {
  __u32 size;

  __u32 sched_policy;
  __u64 sched_flags;

  /* SCHED_NORMAL, SCHED_BATCH */
  __s32 sched_nice;

  /* SCHED_FIFO, SCHED_RR */
  __u32 sched_priority;

  /* SCHED_DEADLINE (nsec) */
  __u64 sched_runtime;
  __u64 sched_deadline;
  __u64 sched_period;
};

int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags);

int sched_getattr(pid_t pid,struct sched_attr *attr,unsigned int size, unsigned int flags);

#endif

#define gettid() syscall(__NR_gettid) // for gettid


void thread_top_init(char *thread_name,
		     int affinity,
		     uint64_t runtime,
		     uint64_t deadline,
		     uint64_t period);

#endif
