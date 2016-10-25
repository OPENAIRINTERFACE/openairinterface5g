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

/*! \file log_if.h
* \brief log interface
* \author Navid Nikaein
* \date 2009 - 2014
* \version 0.3
* \warning This component can be runned only in user-space
* @ingroup routing
*/
#ifndef __LOG_IF_H__
#    define __LOG_IF_H__


/*--- INCLUDES ---------------------------------------------------------------*/
#    include "log.h"
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

#    ifdef COMPONENT_LOG
#        ifdef COMPONENT_LOG_IF
#            define private_log_if(x) x
#            define friend_log_if(x) x
#            define public_log_if(x) x
#        else
#            define private_log_if(x)
#            define friend_log_if(x) extern x
#            define public_log_if(x) extern x
#        endif
#    else
#        define private_log_if(x)
#        define friend_log_if(x)
#        define public_log_if(x) extern x
#    endif

/** @defgroup _log_if Interfaces of LOG
 * @{*/

//public_log_if( log_t *g_log;)

public_log_if( int logInit (void);)
public_log_if( void logRecord_mt(const char *file, const char *func, int line,int comp, int level, const char *format, ...);)
public_log_if( void logRecord(const char *file, const char *func, int line,int comp, int level, const char *format, ...);)
public_log_if( int set_comp_log(int component, int level, int verbosity, int interval);)
public_log_if( int  set_log(int component, int level, int interval);)
public_log_if( void set_glog(int level, int verbosity);)
public_log_if( void set_log_syslog(int enable);)
public_log_if( void set_log_onlinelog(int enable);)
public_log_if( void set_log_filelog(int enable);)
public_log_if( void set_component_filelog(int comp);)
public_log_if( int  map_str_to_int(mapping *map, const char *str);)
public_log_if( char *map_int_to_str(mapping *map, int val);)
public_log_if( void logClean (void); )
public_log_if( int is_newline( char *str, int size);)
public_log_if( void *log_thread_function(void * list);)

/* @}*/

#ifdef __cplusplus
}
#endif

#endif

