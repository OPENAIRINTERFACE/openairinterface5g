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

/* Header file for making PLATON work on both RTLinux and RTAI using pthreads
*  Copyright Frank Wartena, 2004
*  Version: 0.1 January 28th, 2004 ## First version for scheduling test
*     0.2 February 20th, 2004 ## All PLATON functions added
*  It is mostly a conversion from RTLinux to RTAI, because PLATON was written
*  for RTLinux.
*  Not all necessary conversions can be done in this header file, other
*  adjustments in the code are also necessary.
*/

#ifndef _RT_COMPAT_H_
#define _RT_COMPAT_H_

#ifdef RTAI_ENABLED
//the conversions from RTLinux to RTAI

//variable for containing times
#define hrtime_t RTIME

//function for making a thread periodic
#define pthread_make_periodic_np(x,y,z) rt_task_make_periodic(x,rt_get_time(),nano2count(z))

//function for printing to the kernel log
#define rtl_printf printk

//function for determining the current cpu
#define rtl_getcpuid hard_cpu_id

//function for initialising a mutex
#define pthread_mutex_init pthread_mutex_init_rt

//function for initialising a mutex attribute
#define pthread_mutexattr_init pthread_mutexattr_init_rt

//function for setting a mutex protocol, does not exist in RTAI
#define pthread_mutexattr_setprotocol(x,y);

//function for locking a mutex
#define pthread_mutex_lock pthread_mutex_lock_rt

//function for unlocking a mutex
#define pthread_mutex_unlock pthread_mutex_unlock_rt

//function for destroying a mutex
#define pthread_mutex_destroy pthread_mutex_destroy_rt

//function for getting the current time
#define clock_gethrtime(x) rt_get_time_ns()
#define gethrtime          rt_get_time_ns

//function for initialising a condition
#define pthread_cond_init pthread_cond_init_rt

//function for waiting on a condition
#define pthread_cond_wait pthread_cond_wait_rt

//function for signalling a condition
#define pthread_cond_signal pthread_cond_signal_rt

//function for destroying a condition
#define pthread_cond_destroy pthread_cond_destroy_rt

//function for waking up a thread
#define pthread_wakeup_np rt_task_resume

//function for getting the current thread
#define pthread_self rt_whoami

//function for initialising an attribute
#define pthread_attr_init pthread_attr_init_rt

//function for setting a schedparam
#define pthread_attr_setschedparam pthread_attr_setschedparam_rt

//funcion for setting a schedpolicy
#define pthread_attr_setschedpolicy pthread_attr_setschedpolicy_rt

//function for creating a thread
#define pthread_create pthread_create_rt

//function for cancelling a thread
#define pthread_cancel pthread_cancel_rt

//function for deleting a thread, in RTAI both by calling cancel
#define pthread_delete_np pthread_cancel_rt

//testing if a cpuid exists
#define rtl_cpu_exists(x) x<NR_RT_CPUS

//function for flushing a fifo does not exist in RTAI
#define rtf_flush(x);

//function for setting the stacksize
#define pthread_attr_setstacksize pthread_attr_setstacksize_rt

//function for sleeping the indicated amount in nanoseconds
//#define rtl_delay(x) rt_sleep(nano2count(x))
//#define udelay(x) rt_sleep(nano2count(1000*x))
//#define usleep(x) rt_sleep(nano2count(1000*x))

#define pthread_exit pthread_exit_rt

//function for indicating that a thread uses the FPU
#define pthread_setfp_np rt_task_use_fpu

//function for freeing a soft irq
#define rtl_free_soft_irq(x) rt_free_linux_irq(x,NULL)

//function for triggering a soft irq
#define rtl_global_pend_irq rt_pend_linux_irq

//function for setting the thread scheduling, does not exist in RTAI
#define pthread_setschedparam(x,y,z);

#endif /* RTAI_ENABLED */

#endif /* _RT_COMPAT_H_ */
