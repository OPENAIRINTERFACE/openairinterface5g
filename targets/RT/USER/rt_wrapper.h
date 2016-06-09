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

   Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file rt_wrapper.h
* \brief provides a wrapper for the timing function for real-time opeartions depending on weather RTAI is used or not. It also implements an API for the SCHED_DEADLINE kernel scheduler.
* \author F. Kaltenberger and Navid Nikaein
* \date 2013
* \version 0.1
* \company Eurecom
* \email: florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
* \note
* \warning This code will be removed when a legacy libc API becomes available.
*/


void set_latency_target(void);

#ifndef RTAI
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

#define RTIME long long int

#define rt_printk printf

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

#else
#include <rtai_hal.h>
#include <rtai_lxrt.h>
#include <rtai_sem.h>
#include <rtai_msg.h>

int rt_sleep_ns(RTIME x);

#endif
