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
