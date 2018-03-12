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

/*! \file log.h
* \brief openair log generator (OLG) for
* \author Navid Nikaein
* \date 2009 - 2014
* \version 0.5
* @ingroup util

*/

#ifndef __LOG_H__
#    define __LOG_H__

/*--- INCLUDES ---------------------------------------------------------------*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup _LOG LOG Generator
 * @{*/
/* @}*/

/** @defgroup _macro Macro Definition
 *  @ingroup _LOG
 *  @brief these macros are used in the code of LOG
 * @{*/
/* @}*/

/** @defgroup _max_length Maximum Length of LOG
 *  @ingroup _macro
 *  @brief the macros that describe the maximum length of LOG
 * @{*/
#define MAX_LOG_ITEM 100 /*!< \brief the maximum length of a LOG item, what is LOG_ITEM ??? */
#define MAX_LOG_INFO 1000 /*!< \brief the maximum length of a log */
#define MAX_LOG_TOTAL 1500 /*!< \brief the maximum length of a log */
/* @}*/

/** @defgroup _log_level Message levels defined by LOG
 *  @ingroup _macro
 *  @brief LOG defines 9 levels of messages for users. Importance of these levels decrease gradually from 0 to 8
 * @{*/
# define  LOG_EMERG 0 /*!< \brief system is unusable */
# define  LOG_ALERT 1 /*!< \brief action must be taken immediately */
# define  LOG_CRIT  2 /*!< \brief critical conditions */
# define  LOG_ERR   3 /*!< \brief error conditions */
# define  LOG_WARNING 4 /*!< \brief warning conditions */
# define  LOG_NOTICE  5 /*!< \brief normal but significant condition */
# define  LOG_INFO  6 /*!< \brief informational */
# define  LOG_DEBUG 7 /*!< \brief debug-level messages */
# define  LOG_FILE        8 /*!< \brief message sequence chart -level  */
# define  LOG_TRACE 9 /*!< \brief trace-level messages */
#define NUM_LOG_LEVEL  10 /*!< \brief the number of message levels users have with LOG */
/* @}*/


/** @defgroup _log_format Defined log format
 *  @ingroup _macro
 *  @brief Macro of log formats defined by LOG
 * @{*/

/* .log_format = 0x13 uncolored standard messages
 * .log_format = 0x93 colored standard messages */

#define LOG_RED "\033[1;31m"  /*!< \brief VT100 sequence for bold red foreground */
#define LOG_GREEN "\033[32m"  /*!< \brief VT100 sequence for green foreground */
#define LOG_ORANGE "\033[93m"   /*!< \brief VT100 sequence for orange foreground */
#define LOG_BLUE "\033[34m" /*!< \brief VT100 sequence for blue foreground */
#define LOG_CYBL "\033[40;36m"  /*!< \brief VT100 sequence for cyan foreground on black background */
#define LOG_RESET "\033[0m" /*!< \brief VT100 sequence for reset (black) foreground */
/* @}*/


/** @defgroup _syslog_conf Macros for write in syslog.conf
 *  @ingroup _macro
 *  @brief Macros used to write lines (local/remote) in syslog.conf
 * @{*/
#define LOG_LOCAL      0x01
#define LOG_REMOTE     0x02

#define FLAG_COLOR     0x001  /*!< \brief defaults */
#define FLAG_PID       0x002  /*!< \brief defaults */
#define FLAG_COMP      0x004
#define FLAG_THREAD    0x008  /*!< \brief all : 255/511 */
#define FLAG_LEVEL     0x010
#define FLAG_FUNCT     0x020
#define FLAG_FILE_LINE 0x040
#define FLAG_TIME      0x100

#define LOG_NONE        0x00
#define LOG_LOW         0x5
#define LOG_MED         0x15
#define LOG_HIGH        0x35
#define LOG_FULL        0x75

#define OAI_OK 0    /*!< \brief all ok */
#define OAI_ERR 1   /*!< \brief generic error */
#define OAI_ERR_READ_ONLY 2 /*!< \brief tried to write to read-only item */
#define OAI_ERR_NOTFOUND 3  /*!< \brief something wasn't found */
/* @}*/


//static char *log_level_highlight_start[] = {LOG_RED, LOG_RED, LOG_RED, LOG_RED, LOG_BLUE, "", "", "", LOG_GREEN}; /*!< \brief Optional start-format strings for highlighting */

//static char *log_level_highlight_end[]   = {LOG_RESET, LOG_RESET, LOG_RESET, LOG_RESET, LOG_RESET, "", "", "", LOG_RESET};  /*!< \brief Optional end-format strings for highlighting */

typedef enum {
    MIN_LOG_COMPONENTS = 0,
    PHY = MIN_LOG_COMPONENTS,
    MAC,
    EMU,
    OCG,
    OMG,
    OPT,
    OTG,
    OTG_LATENCY,
    OTG_LATENCY_BG,
    OTG_GP,
    OTG_GP_BG,
    OTG_JITTER,
    RLC,
    PDCP,
    RRC,
    NAS,
    PERF,
    OIP,
    CLI,
    MSC,
    OCM,
    UDP_,
    GTPU,
    SPGW,
    S1AP,
    SCTP,
    HW,
    OSA,
    RAL_ENB,
    RAL_UE,
    ENB_APP,
    FLEXRAN_AGENT,
    TMR,
    USIM,
    LOCALIZE,
    RRH,
    X2AP,
    MAX_LOG_COMPONENTS,
}
comp_name_t;

//#define msg printf

typedef struct {
    char *name; /*!< \brief string name of item */
    int value;  /*!< \brief integer value of mapping */
} mapping;


typedef struct  {
    const char *name;
    int         level;
    int         flag;
    int         interval;
    int         fd;
    int         filelog;
    char       *filelog_name;

    /* SR: make the log buffer component relative */
    char        log_buffer[MAX_LOG_TOTAL];
} log_component_t;

typedef struct  {
    unsigned int remote_ip;
    unsigned int audit_ip;
    int  remote_level;
    int  facility;
    int  audit_facility;
    int  format;
} log_config_t;


typedef struct {
    log_component_t         log_component[MAX_LOG_COMPONENTS];
    log_config_t            config;
    char*                   level2string[NUM_LOG_LEVEL];
    int                     level;
    int                     onlinelog;
    int                     flag;
    int                     syslog;
    int                     filelog;
    char*                   filelog_name;
} log_t;

typedef struct LOG_params {
    const char *file;
    const char *func;
    int line;
    int comp;
    int level;
    const char *format;
    char l_buff_info [MAX_LOG_INFO];
    int len;
} LOG_params;

#if defined(ENABLE_ITTI)
typedef enum log_instance_type_e {
    LOG_INSTANCE_UNKNOWN,
    LOG_INSTANCE_ENB,
    LOG_INSTANCE_UE,
} log_instance_type_t;

void log_set_instance_type (log_instance_type_t instance);
#endif

#ifdef LOG_MAIN
log_t *g_log;
#else
#ifdef __cplusplus
   extern "C" {
#endif
extern log_t *g_log;
#ifdef __cplusplus
}
#endif
#endif
/*--- INCLUDES ---------------------------------------------------------------*/
#    include "log_if.h"
/*----------------------------------------------------------------------------*/
int  logInit (void);
void logRecord_mt(const char *file, const char *func, int line,int comp, int level, const char *format, ...) __attribute__ ((format (printf, 6, 7)));
void logRecord(const char *file, const char *func, int line,int comp, int level, const char *format, ...) __attribute__ ((format (printf, 6, 7)));
int  set_comp_log(int component, int level, int verbosity, int interval);
int  set_log(int component, int level, int interval);
void set_glog(int level, int verbosity);
void set_log_syslog(int enable);
void set_glog_onlinelog(int enable);
void set_glog_filelog(int enable);

void set_component_filelog(int comp);
int  map_str_to_int(mapping *map, const char *str);
char *map_int_to_str(mapping *map, int val);
void logClean (void);
int  is_newline( char *str, int size);
void *log_thread_function(void * list);

/** @defgroup _logIt logIt function
 *  @ingroup _macro
 *  @brief Macro used to call tr_log_full_ex with file, function and line information
 * @{*/
#ifdef LOG_NO_THREAD
#define logIt(component, level, format, args...) (g_log->log_component[component].interval?logRecord_mt(__FILE__, __FUNCTION__, __LINE__, component, level, format, ##args):(void)0)
#else //default
#define logIt(component, level, format, args...) (g_log->log_component[component].interval?logRecord(__FILE__, __FUNCTION__, __LINE__, component, level, format, ##args):(void)0)
#endif
/* @}*/

/*----------------macro definitions for reading log configuration from the config module */
#define CONFIG_STRING_LOG_PREFIX                           "log_config"

#define LOG_CONFIG_STRING_GLOBAL_LOG_LEVEL                 "global_log_level"
#define LOG_CONFIG_STRING_GLOBAL_LOG_VERBOSITY             "global_log_verbosity"
#define LOG_CONFIG_STRING_GLOBAL_LOG_ONLINE                "global_log_online"
#define LOG_CONFIG_STRING_GLOBAL_LOG_INFILE                "global_log_infile"

#define LOG_CONFIG_LEVEL_FORMAT                            "%s_log_level"
#define LOG_CONFIG_VERBOSITY_FORMAT                        "%s_log_verbosity"
#define LOG_CONFIG_LOGFILE_FORMAT                          "%s_log_infile"
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                       LOG globalconfiguration parameters										        */
/*   optname                              helpstr   paramflags    XXXptr	             defXXXval				      type	     numelt	*/
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define LOG_GLOBALPARAMS_DESC { \
{LOG_CONFIG_STRING_GLOBAL_LOG_LEVEL,    NULL,	    0,  	 strptr:(char **)&gloglevel, defstrval:log_level_names[2].name,       TYPE_STRING,  0}, \
{LOG_CONFIG_STRING_GLOBAL_LOG_VERBOSITY,NULL,	    0,  	 strptr:(char **)&glogverbo, defstrval:log_verbosity_names[2].name,   TYPE_STRING,  0}, \
{LOG_CONFIG_STRING_GLOBAL_LOG_ONLINE,   NULL,	    0,  	 iptr:&(g_log->onlinelog),   defintval:1,                             TYPE_INT,      0,              }, \
{LOG_CONFIG_STRING_GLOBAL_LOG_INFILE,   NULL,	    0,  	 iptr:&(g_log->filelog),     defintval:0,                             TYPE_INT,      0,              }, \
}
/*----------------------------------------------------------------------------------*/
/** @defgroup _debugging debugging macros
 *  @ingroup _macro
 *  @brief Macro used to call logIt function with different message levels
 * @{*/

// debugging macros
#  if T_TRACER
#    include "T.h"
#    define LOG_I(c, x...) T(T_LEGACY_ ## c ## _INFO, T_PRINTF(x))
#    define LOG_W(c, x...) T(T_LEGACY_ ## c ## _WARNING, T_PRINTF(x))
#    define LOG_E(c, x...) T(T_LEGACY_ ## c ## _ERROR, T_PRINTF(x))
#    define LOG_D(c, x...) T(T_LEGACY_ ## c ## _DEBUG, T_PRINTF(x))
#    define LOG_T(c, x...) T(T_LEGACY_ ## c ## _TRACE, T_PRINTF(x))
#    define LOG_G(c, x...) /* */
#    define LOG_A(c, x...) /* */
#    define LOG_C(c, x...) /* */
#    define LOG_N(c, x...) /* */
#    define LOG_F(c, x...) /* */
#  else /* T_TRACER */
#    if DISABLE_LOG_X
#        define LOG_I(c, x...) /* */
#        define LOG_W(c, x...) /* */
#        define LOG_E(c, x...) /* */
#        define LOG_D(c, x...) /* */
#        define LOG_T(c, x...) /* */
#        define LOG_G(c, x...) /* */
#        define LOG_A(c, x...) /* */
#        define LOG_C(c, x...) /* */
#        define LOG_N(c, x...) /* */
#        define LOG_F(c, x...) /* */
#    else  /*DISABLE_LOG_X*/
#        define LOG_G(c, x...) logIt(c, LOG_EMERG, x)
#        define LOG_A(c, x...) logIt(c, LOG_ALERT, x)
#        define LOG_C(c, x...) logIt(c, LOG_CRIT,  x)
#        define LOG_E(c, x...) logIt(c, LOG_ERR, x)
#        define LOG_W(c, x...) logIt(c, LOG_WARNING, x)
#        define LOG_N(c, x...) logIt(c, LOG_NOTICE, x)
#        define LOG_I(c, x...) logIt(c, LOG_INFO, x)
#        define LOG_D(c, x...) logIt(c, LOG_DEBUG, x)
#        define LOG_F(c, x...) logIt(c, LOG_FILE, x)  // log to a file, useful for the MSC chart generation
#        define LOG_T(c, x...) logIt(c, LOG_TRACE, x)
#    endif /*DISABLE_LOG_X*/
#  endif /* T_TRACER */
/* @}*/


/** @defgroup _useful_functions useful functions in LOG
 *  @ingroup _macro
 *  @brief Macro of some useful functions defined by LOG
 * @{*/
#define LOG_ENTER(c) do {LOG_T(c, "Entering\n");}while(0) /*!< \brief Macro to log a message with severity DEBUG when entering a function */
#define LOG_EXIT(c) do {LOG_T(c,"Exiting\n"); return;}while(0)  /*!< \brief Macro to log a message with severity TRACE when exiting a function */
#define LOG_RETURN(c,x) do {uint32_t __rv;__rv=(unsigned int)(x);LOG_T(c,"Returning %08x\n", __rv);return((typeof(x))__rv);}while(0)  /*!< \brief Macro to log a function exit, including integer value, then to return a value to the calling function */
/* @}*/

static __inline__ uint64_t rdtsc(void) {
  uint64_t a, d;
  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  return (d<<32) | a;
}

#define DEBUG_REALTIME 1
#if DEBUG_REALTIME

extern double cpuf;

static inline uint64_t checkTCPU(int timeout, char * file, int line) {
    static uint64_t __thread lastCPUTime=0;
    static uint64_t __thread last=0;
    uint64_t cur=rdtsc();
    struct timespec CPUt;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &CPUt);
    uint64_t CPUTime=CPUt.tv_sec*1000*1000+CPUt.tv_nsec/1000;
    double microCycles=(double)(cpuf*1000);
    int duration=(int)((cur-last)/microCycles);
    if ( last!=0 && duration > timeout ) {
      //struct timespec ts;
      //clock_gettime(CLOCK_MONOTONIC, &ts);
      printf("%s:%d lte-ue delay %d (exceed %d), CPU for this period: %lld\n", file, line,
               duration, timeout, (long long)CPUTime-lastCPUTime );
    }
    last=cur;
    lastCPUTime=CPUTime;
    return cur;
}

static inline unsigned long long checkT(int timeout, char * file, int line) {
    static unsigned long long __thread last=0;
    unsigned long long cur=rdtsc();
    int microCycles=(int)(cpuf*1000);
    int duration=(int)((cur-last)/microCycles);
    if ( last!=0 && duration > timeout )
        printf("%s:%d lte-ue delay %d (exceed %d)\n", file, line,
               duration, timeout);
    last=cur;
    return cur;
}

typedef struct m {
    uint64_t iterations;
    uint64_t sum;
    uint64_t maxArray[11];
} Meas;

static inline void printMeas(char * txt, Meas *M, int period) {
    if (M->iterations%period == 0 ) {
        char txt2[512];
        sprintf(txt2,"%s avg=%" PRIu64 " iterations=%" PRIu64 " max=%" 
                PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 "\n",
                txt,
                M->sum/M->iterations,
                M->iterations,
                M->maxArray[1],M->maxArray[2], M->maxArray[3],M->maxArray[4], M->maxArray[5], 
                M->maxArray[6],M->maxArray[7], M->maxArray[8],M->maxArray[9],M->maxArray[10]);
#if DISABLE_LOG_X
        printf("%s",txt2);
#else
        LOG_W(PHY, "%s",txt2);
#endif
    }
}

static inline int cmpint(const void* a, const void* b) {
    uint64_t* aa=(uint64_t*)a;
    uint64_t* bb=(uint64_t*)b;
    return (int)(*aa-*bb);
}

static inline void updateTimes(uint64_t start, Meas *M, int period, char * txt) {
    if (start!=0) {
        uint64_t end=rdtsc();
        long long diff=(end-start)/(cpuf*1000);
        M->maxArray[0]=diff;
        M->sum+=diff;
        M->iterations++;
        qsort(M->maxArray, 11, sizeof(uint64_t), cmpint);
        printMeas(txt,M,period);
    }
}

#define check(a) do { checkT(a,__FILE__,__LINE__); } while (0)
#define checkcpu(a) do { checkTCPU(a,__FILE__,__LINE__); } while (0)
#define initRefTimes(a) static __thread Meas a= {0}
#define pickTime(a) uint64_t a=rdtsc()
#define readTime(a) a
#define initStaticTime(a) static __thread uint64_t a={0}
#define pickStaticTime(a) do { a=rdtsc(); } while (0)

#else
#define check(a) do {} while (0)
#define checkcpu(a) do {} while (0)
#define initRefTimes(a) do {} while (0)
#define initStaticTime(a) do {} while (0)
#define pickTime(a) do {} while (0)
#define readTime(a) 0
#define pickStaticTime(a) do {} while (0)
#define updateTimes(a,b,c,d) do {} while (0)
#define printMeas(a,b,c) do {} while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif


