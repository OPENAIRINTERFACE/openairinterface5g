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

/*****************************************************************************
Source    nas_log.h

Version   0.1

Date    2012/02/28

Product   NAS stack

Subsystem Utilities

Author    Frederic Maurel

Description Usefull logging functions

*****************************************************************************/
#ifndef __NAS_LOG_H__
#define __NAS_LOG_H__

#if defined(NAS_BUILT_IN_UE) && defined(NAS_UE)
# include "common/utils/LOG/log.h"
# undef LOG_TRACE
#endif

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* -----------------------
 * Logging severity levels
 * -----------------------
 *  OFF : Disables logging trace utilities.
 *  DEBUG : Only used for debug purpose. Should be removed from the code.
 *  INFO  : Informational trace
 *  WARNING : The program displays the warning message and doesn't stop.
 *  ERROR : The program displays the error message and usually exits or
 *      runs appropriate procedure.
 *  FUNC  : Prints trace when entering/leaving to/from function. Usefull
 *      to display the function's calling tree information at runtime.
 *  ON  : Enables logging traces excepted FUNC.
 *  ALL : Turns on ALL logging traces.
 */
#define NAS_LOG_OFF 0x00  /* No trace       */
#define NAS_LOG_DEBUG 0x01  /* Debug trace        */
#define NAS_LOG_INFO  0x02  /* Informational trace      */
#define NAS_LOG_WARNING 0x04  /* Warning trace      */
#define NAS_LOG_ERROR 0x08  /* Error trace        */
#define NAS_LOG_FUNC  0x10  /* Entering/Leaving function trace  */
#define NAS_LOG_HEX 0x20  /* Dump trace       */

#define NAS_LOG_ON  0x0F  /* All traces excepted FUNC and HEX */
#define NAS_LOG_ALL 0xFF  /* All traces       */

/* Logging severity type */
typedef enum {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FUNC_IN,
  FUNC_OUT,
  LOG_SEVERITY_MAX
} log_severity_t;

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#ifdef LOG_E
# define LOG_TRACE(s, x, args...)                               \
do {                                                            \
    switch (s) {                                                \
        case ERROR:     LOG_E(NAS, " %s:%d  " x "\n", __FILE__, __LINE__, ##args); break;  \
        case WARNING:   LOG_W(NAS, " %s:%d  " x "\n", __FILE__, __LINE__, ##args); break;  \
        case INFO:      LOG_I(NAS, " %s:%d  " x "\n", __FILE__, __LINE__, ##args); break;  \
        default:        LOG_D(NAS, " %s:%d  " x "\n", __FILE__, __LINE__, ##args); break;  \
    }                                                           \
} while (0)

# define LOG_DUMP(dATA, lEN)   LOG_DUMPMSG(NAS, DEBUG_NAS,dATA, lEN, " Dump %d:\n", lEN)                                                 
# define LOG_FUNC_IN  LOG_ENTER(NAS)
# define LOG_FUNC_OUT  LOG_END(NAS)
# define LOG_FUNC_RETURN(rETURNcODE) LOG_RETURN(NAS,rETURNcODE)

extern int nas_log_func_indent;

#else
# define LOG_TRACE(s, x, args...)  
# define LOG_DUMP(dATA, lEN)   LOG_DUMPMSG(NAS, LOG_DUMP_CHAR,dATA, lEN,  " Dump %d:\n", lEN)

# define LOG_FUNC_IN 
# define LOG_FUNC_OUT 
# define LOG_FUNC_RETURN(rETURNcODE)  return  rETURNcODE                                  \

#endif

#endif /* __NAS_LOG_H__*/
