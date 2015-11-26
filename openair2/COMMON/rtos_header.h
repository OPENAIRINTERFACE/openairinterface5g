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
