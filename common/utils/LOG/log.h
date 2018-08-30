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
#include "T.h"
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

#define MAX_LOG_TOTAL 1500 /*!< \brief the maximum length of a log */
/* @}*/

/** @defgroup _log_level Message levels defined by LOG
 *  @ingroup _macro
 *  @brief LOG defines 9 levels of messages for users. Importance of these levels decrease gradually from 0 to 8
 * @{*/

# define  OAILOG_ERR     0 /*!< \brief critical error conditions, impact on "must have" fuctinalities */
# define  OAILOG_FILE    1 /*!< \brief important informational messages, but everything OK  */
# define  OAILOG_WARNING 2 /*!< \brief warning conditions, shouldn't happen but doesn't impact "must have" functionalities */
# define  OAILOG_INFO    3 /*!< \brief informational messages most people don't need, shouldn't impact real-time behavior */
# define  OAILOG_DEBUG   4 /*!< \brief first level debug-level messages, for developers , may impact real-time behavior */
# define  OAILOG_TRACE   5 /*!< \brief  second level debug-level messages, for developers ,likely impact real-time behavior*/

#define NUM_LOG_LEVEL 6 /*!< \brief the number of message levels users have with LOG */
/* @}*/


/** @defgroup _log_format Defined log format
 *  @ingroup _macro
 *  @brief Macro of log formats defined by LOG
 * @{*/

/* .log_format = 0x13 uncolored standard messages
 * .log_format = 0x93 colored standard messages */
/* keep white space in first position; switching it to 0 allows colors to be disabled*/
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


#define FLAG_NOCOLOR   0x0001  /*!< \brief use colors in log messages, depending on level */
#define FLAG_THREAD    0x0008  /*!< \brief display thread name in log messages */
#define FLAG_LEVEL     0x0010  /*!< \brief display log level in log messages */
#define FLAG_FUNCT     0x0020
#define FLAG_FILE_LINE 0x0040
#define FLAG_TIME      0x0100

#define SET_LOG_OPTION(O)   g_log->flag = (g_log->flag | O)
#define CLEAR_LOG_OPTION(O) g_log->flag = (g_log->flag & (~O))

/** @defgroup macros to identify a debug entity
 *  @ingroup each macro is a bit mask where the unique bit set identifies an entity to be debugged
 *            it allows to dynamically activate or not blocks of code 
 *  @brief 
 * @{*/
#define DEBUG_PRACH        (1<<0)
#define DEBUG_RU           (1<<1)
#define DEBUG_UE_PHYPROC   (1<<2)
#define DEBUG_LTEESTIM     (1<<3)
#define DEBUG_CTRLSOCKET   (1<<10)
#define UE_TIMING          (1<<20)

#define SET_LOG_DEBUG(O)   g_log->debug_mask = (g_log->debug_mask | O)
#define CLEAR_LOG_DEBUG(O) g_log->debug_mask = (g_log->debug_mask & (~O))

#define SET_LOG_MATLAB(O)   g_log->matlab_mask = (g_log->matlab_mask | O)
#define CLEAR_LOG_MATLAB(O) g_log->matlab_mask = (g_log->matlab_mask & (~O))



typedef enum {
    MIN_LOG_COMPONENTS = 0,
    PHY = MIN_LOG_COMPONENTS,
    MAC,
    EMU,
    SIM,
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
    GNB_APP,
    NR_RRC,
    NR_MAC,
    NR_PHY,
    LOADER,
    MAX_LOG_PREDEF_COMPONENTS,
}
comp_name_t;

#define MAX_LOG_DYNALLOC_COMPONENTS 20
#define MAX_LOG_COMPONENTS (MAX_LOG_PREDEF_COMPONENTS + MAX_LOG_DYNALLOC_COMPONENTS)


typedef struct {
    char *name; /*!< \brief string name of item */
    int value;  /*!< \brief integer value of mapping */
} mapping;

typedef int(*log_write_func_t)(FILE *stream, const char *format, va_list ap );

typedef struct  {
    const char       *name;
    int              level;
    int              flag;
    int              interval;
    int              filelog;
    char             *filelog_name;
    FILE             *stream;
    log_write_func_t fwrite;
    /* SR: make the log buffer component relative */
    char             log_buffer[MAX_LOG_TOTAL];
} log_component_t;


typedef struct {
    log_component_t         log_component[MAX_LOG_COMPONENTS];
    char*                   level2string[NUM_LOG_LEVEL];
    int                     onlinelog;
    int                     flag;
    int                     filelog;
    char*                   filelog_name;
    uint64_t                debug_mask;
    uint64_t                matlab_mask;
} log_t;


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

int  set_log(int component, int level, int interval);
void set_glog(int level);

void set_glog_onlinelog(int enable);
void set_glog_filelog(int enable);
void set_component_filelog(int comp);

int  map_str_to_int(mapping *map, const char *str);
char *map_int_to_str(mapping *map, int val);
void logClean (void);
int  is_newline( char *str, int size);

int register_log_component(char *name, char *fext, int compidx);

/* @}*/

/*!\fn int32_t write_file_matlab(const char *fname, const char *vname, void *data, int length, int dec, char format);
\brief Write output file from signal data
@param fname output file name
@param vname  output vector name (for MATLAB/OCTAVE)
@param data   point to data
@param length length of data vector to output
@param dec    decimation level
@param format data format (0 = real 16-bit, 1 = complex 16-bit,2 real 32-bit, 3 complex 32-bit,4 = real 8-bit, 5 = complex 8-bit)
*/
int32_t write_file_matlab(const char *fname, const char *vname, void *data, int length, int dec, char format);

/*----------------macro definitions for reading log configuration from the config module */
#define CONFIG_STRING_LOG_PREFIX                           "log_config"

#define LOG_CONFIG_STRING_GLOBAL_LOG_LEVEL                 "global_log_level"
#define LOG_CONFIG_STRING_GLOBAL_LOG_ONLINE                "global_log_online"
#define LOG_CONFIG_STRING_GLOBAL_LOG_INFILE                "global_log_infile"
#define LOG_CONFIG_STRING_GLOBAL_LOG_OPTIONS               "global_log_options"

#define LOG_CONFIG_LEVEL_FORMAT                            "%s_log_level"
#define LOG_CONFIG_LOGFILE_FORMAT                          "%s_log_infile"
#define LOG_CONFIG_DEBUG_FORMAT                            "%s_debug"
#define LOG_CONFIG_MATLAB_FORMAT                           "%s_matlab"

#define LOG_CONFIG_HELP_OPTIONS      " list of comma separated options to enable log module behavior. Available options: \n"\
                                     " nocolor:   disable color usage in log messages\n"\
				     " level:     add log level indication in log messages\n"\
				     " thread:    add threads names in log messages\n"
				     


                   
/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                       LOG globalconfiguration parameters										                                                */
/*   optname                            help                                                paramflags       XXXptr	                   defXXXval				      type	 numelt	*/
/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define LOG_GLOBALPARAMS_DESC { \
{LOG_CONFIG_STRING_GLOBAL_LOG_LEVEL,    "Default log level for all componemts\n",              0,  	      strptr:(char **)&gloglevel,    defstrval:log_level_names[2].name,    TYPE_STRING,    0}, \
{LOG_CONFIG_STRING_GLOBAL_LOG_ONLINE,   "Default console output option, for all components\n", 0,  	      iptr:&(g_log->onlinelog),      defintval:1,                          TYPE_INT,       0}, \
{LOG_CONFIG_STRING_GLOBAL_LOG_OPTIONS,  LOG_CONFIG_HELP_OPTIONS,                               0,  	      strlistptr:NULL,               defstrlistval:NULL,                   TYPE_STRINGLIST,0} \
}

#define LOG_OPTIONS_IDX   2
/*----------------------------------------------------------------------------------*/
/** @defgroup _debugging debugging macros
 *  @ingroup _macro
 *  @brief Macro used to call logIt function with different message levels
 * @{*/

// debugging macros(g_log->log_component[component].interval?logRecord_mt(__FILE__, __FUNCTION__, __LINE__, component, level, format, ##args):(void)0)
#  if T_TRACER 
     /* per component, level dependant macros */
#    define LOG_I(c, x...) do { if (T_stdout) { if( g_log->log_component[c].level >= OAILOG_INFO   ) logRecord_mt(__FILE__, __FUNCTION__, __LINE__,c, OAILOG_INFO, x)    ;} else { T(T_LEGACY_ ## c ## _INFO, T_PRINTF(x))    ;}} while (0) 
#    define LOG_W(c, x...) do { if (T_stdout) { if( g_log->log_component[c].level >= OAILOG_WARNING) logRecord_mt(__FILE__, __FUNCTION__, __LINE__,c, OAILOG_WARNING, x) ;} else { T(T_LEGACY_ ## c ## _WARNING, T_PRINTF(x)) ;}} while (0) 
#    define LOG_E(c, x...) do { if (T_stdout) { if( g_log->log_component[c].level >= OAILOG_ERR    ) logRecord_mt(__FILE__, __FUNCTION__, __LINE__,c, OAILOG_ERR, x)     ;} else { T(T_LEGACY_ ## c ## _ERROR, T_PRINTF(x))   ;}} while (0) 
#    define LOG_D(c, x...) do { if (T_stdout) { if( g_log->log_component[c].level >= OAILOG_DEBUG  ) logRecord_mt(__FILE__, __FUNCTION__, __LINE__,c, OAILOG_DEBUG, x)   ;} else { T(T_LEGACY_ ## c ## _DEBUG, T_PRINTF(x))   ;}} while (0) 
#    define LOG_T(c, x...) do { if (T_stdout) { if( g_log->log_component[c].level >= OAILOG_TRACE  ) logRecord_mt(__FILE__, __FUNCTION__, __LINE__,c, OAILOG_TRACE, x)   ;} else { T(T_LEGACY_ ## c ## _TRACE, T_PRINTF(x))   ;}} while (0) 
#    define LOG_F(c, x...) do { if (T_stdout) { if( g_log->log_component[c].level >= OAILOG_FILE   ) logRecord_mt(__FILE__, __FUNCTION__, __LINE__,c, OAILOG_FILE, x)  ;}}   while (0)  /* */
#    define nfapi_log(FILE, FNC, LN, COMP, LVL, F...)  do { if (T_stdout) { logRecord_mt(__FILE__, __FUNCTION__, __LINE__,COMP, LVL, F)  ;}}   while (0)  /* */
     /* bitmask dependant macros, to isolate debugging code */
#    define LOG_DEBUG_BEGIN(D) if (g_log->debug_mask & D) {
#    define LOG_DEBUG_END   }
     /* bitmask dependant macros, to generate matlab files */
#    define LOG_M_BEGIN(D) if (g_log->matlab_mask & D) {
#    define LOG_M_END   }
#    define LOG_M(file, vector, data, len, dec, format) do { write_file_matlab(file, vector, data, len, dec, format);} while(0)/* */
     /* define variable only used in LOG macro's */
#    define LOG_VAR(A,B) A B
#  else /* T_TRACER: remove all debugging and tracing messages, except errors */
#    define LOG_I(c, x...) /* */
#    define LOG_W(c, x...) /* */
#    define LOG_E(c, x...) /* */
#    define LOG_D(c, x...) /* */
#    define LOG_T(c, x...) /* */
#    define LOG_F(c, x...) /* */
#    define nfapi_log(FILE, FNC, LN, COMP, LVL, FMT...) 
#    define LOG_DEBUG_BEGIN(D) if (0) {
#    define LOG_DEBUG_END   }
#    define LOG_M_BEGIN(D) if (0) {
#    define LOG_M_END   }
#    define LOG_M(file, vector, data, len, dec, format) 
#    define LOG_VAR(A,B)
#  endif /* T_TRACER */
/* avoid warnings for variables only used in LOG macro's but set outside debug section */
#define LOG_USEDINLOG_VAR(A,B) __attribute__((unused)) A B 

/* unfiltered macros, usefull for simulators or messages at init time, before log is configured */
#define LOG_UM(file, vector, data, len, dec, format) do { write_file_matlab(file, vector, data, len, dec, format);} while(0)

#define LOG_UI(c, x...) do {logRecord_mt(__FILE__, __FUNCTION__, __LINE__,c, OAILOG_INFO, x) ; } while(0)
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
  uint32_t a, d;
  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  return (((uint64_t)d)<<32) | ((uint64_t)a);
}

#define DEBUG_REALTIME 1
#if DEBUG_REALTIME

extern double cpuf;

static inline uint64_t checkTCPU(int timeout, char * file, int line) {
    static __thread uint64_t  lastCPUTime=0;
    static __thread uint64_t  last=0;
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
    static __thread unsigned long long last=0;
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


