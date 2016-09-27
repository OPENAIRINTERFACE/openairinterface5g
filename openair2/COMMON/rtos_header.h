/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

#ifndef _RTOS_HEADER_H_
#    define _RTOS_HEADER_H_
#    if defined(RTAI) && !defined(USER_MODE)
//       CONVERSION BETWEEN POSIX PTHREAD AND RTAI FUNCTIONS
/*
#        define pthread_mutex_init             pthread_mutex_init_rt
#        define pthread_mutexattr_init         pthread_mutexattr_init_rt
#        define pthread_mutex_lock             pthread_mutex_lock_rt
#        define pthread_mutex_unlock           pthread_mutex_unlock_rt
#        define pthread_mutex_destroy          pthread_mutex_destroy_rt
#        define pthread_cond_init              pthread_cond_init_rt
#        define pthread_cond_wait              pthread_cond_wait_rt
#        define pthread_cond_signal            pthread_cond_signal_rt
#        define pthread_cond_destroy           pthread_cond_destroy_rt
#        define pthread_attr_init              pthread_attr_init_rt
#        define pthread_attr_setschedparam     pthread_attr_setschedparam_rt
#        define pthread_create                 pthread_create_rt
#        define pthread_cancel                 pthread_cancel_rt
#        define pthread_delete_np              pthread_cancel_rt
#        define pthread_attr_setstacksize      pthread_attr_setstacksize_rt
#        define pthread_self                   rt_whoami
*/
#        include <asm/rtai.h>
#        include <rtai.h>
#        include <rtai_posix.h>
#        include <rtai_fifos.h>
#        include <rtai_sched.h>
#        ifdef CONFIG_PROC_FS
#            include <rtai_proc_fs.h>
#        endif
#    else
#        ifdef USER_MODE
#            include <stdio.h>
#            include <stdlib.h>
#            include <string.h>
#            include <math.h>
#            include <pthread.h>
#            include <assert.h>
#            define rtf_get    read
#            define rtf_put    write
#        endif
#    endif
#endif
