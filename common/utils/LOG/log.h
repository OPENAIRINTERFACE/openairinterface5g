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
#include <common/utils/utils.h>
/*----------------------------------------------------------------------------*/
#include <assert.h>
#ifdef NDEBUG
#warning assert is disabled
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup _max_length Maximum Length of LOG
 *  @ingroup _macro
 *  @brief the macros that describe the maximum length of LOG
 * @{*/

#define MAX_LOG_TOTAL 16384 /*!< \brief the maximum length of a log */
/** @}*/

/** @defgroup _log_level Message levels defined by LOG
 *  @ingroup _macro
 *  @brief LOG defines 9 levels of messages for users. Importance of these levels decrease gradually from 0 to 8
 * @{*/
# define  OAILOG_DISABLE -1 /*!< \brief disable all LOG messages, cannot be used in LOG macros, use only in LOG module */
# define  OAILOG_ERR      0 /*!< \brief critical error conditions, impact on "must have" functionalities */
# define  OAILOG_WARNING  1 /*!< \brief warning conditions, shouldn't happen but doesn't impact "must have" functionalities */
# define  OAILOG_ANALYSIS 2 /*!< \brief informational messages most people don't need, shouldn't impact real-time behavior */
# define  OAILOG_INFO     3 /*!< \brief informational messages most people don't need, shouldn't impact real-time behavior */
# define  OAILOG_DEBUG    4 /*!< \brief first level debug-level messages, for developers, may impact real-time behavior */
# define  OAILOG_TRACE    5 /*!< \brief second level debug-level messages, for developers, likely impact real-time behavior*/

#define NUM_LOG_LEVEL 6 /*!< \brief the number of message levels users have with LOG (OAILOG_DISABLE is not available to user as a level, so it is not included)*/
/** @}*/

#define SET_LOG_OPTION(O)   g_log->flag = (g_log->flag | O)
#define CLEAR_LOG_OPTION(O) g_log->flag = (g_log->flag & (~O))

/** @defgroup macros to identify a debug entity
 *  @ingroup each macro is a bit mask where the unique bit set identifies an entity to be debugged
 *            it allows to dynamically activate or not blocks of code. The  LOG_MASKMAP_INIT macro
 *            is used to map a character string name to each debug bit, it allows to set or clear
 *            the corresponding bit via the defined name, from the configuration or from the telnet
 *            server.
 *  @brief
 * @{*/
#define DEBUG_PRACH        (1<<0)
#define DEBUG_RU           (1<<1)
#define DEBUG_UE_PHYPROC   (1<<2)
#define DEBUG_LTEESTIM     (1<<3)
#define DEBUG_DLCELLSPEC   (1<<4)
#define DEBUG_ULSCH        (1<<5)
#define DEBUG_RRC          (1<<6)
#define DEBUG_PDCP         (1<<7)
#define DEBUG_DFT          (1<<8)
#define DEBUG_ASN1         (1<<9)
#define DEBUG_CTRLSOCKET   (1<<10)
#define DEBUG_SECURITY     (1<<11)
#define DEBUG_NAS          (1<<12)
#define DEBUG_RLC          (1<<13)
#define DEBUG_DLSCH_DECOD  (1<<14)
#define UE_TIMING          (1<<20)

#define SET_LOG_DEBUG(B)   g_log->debug_mask = (g_log->debug_mask | B)
#define CLEAR_LOG_DEBUG(B) g_log->debug_mask = (g_log->debug_mask & (~B))

#define SET_LOG_DUMP(B)   g_log->dump_mask = (g_log->dump_mask | B)
#define CLEAR_LOG_DUMP(B) g_log->dump_mask = (g_log->dump_mask & (~B))

#define FOREACH_COMP(COMP_DEF)  \
  COMP_DEF(PHY, log)            \
  COMP_DEF(MAC, log)            \
  COMP_DEF(EMU, log)            \
  COMP_DEF(SIM, txt)            \
  COMP_DEF(OMG, csv)            \
  COMP_DEF(OPT, log)            \
  COMP_DEF(OTG, log)            \
  COMP_DEF(OTG_LATENCY, dat)    \
  COMP_DEF(OTG_LATENCY_BG, dat) \
  COMP_DEF(OTG_GP, dat)         \
  COMP_DEF(OTG_GP_BG, dat)      \
  COMP_DEF(OTG_JITTER, dat)     \
  COMP_DEF(RLC, )               \
  COMP_DEF(PDCP, )              \
  COMP_DEF(RRC, )               \
  COMP_DEF(NAS, log)            \
  COMP_DEF(OIP, )               \
  COMP_DEF(CLI, )               \
  COMP_DEF(OCM, )               \
  COMP_DEF(GTPU, )              \
  COMP_DEF(SDAP, )              \
  COMP_DEF(SPGW, )              \
  COMP_DEF(S1AP, )              \
  COMP_DEF(F1AP, )              \
  COMP_DEF(E1AP, )              \
  COMP_DEF(SCTP, )              \
  COMP_DEF(HW, )                \
  COMP_DEF(OSA, )               \
  COMP_DEF(ENB_APP, log)        \
  COMP_DEF(MCE_APP, log)        \
  COMP_DEF(MME_APP, log)        \
  COMP_DEF(TMR, )               \
  COMP_DEF(USIM, log)           \
  COMP_DEF(F1U, )               \
  COMP_DEF(X2AP, )              \
  COMP_DEF(M2AP, )              \
  COMP_DEF(M3AP, )              \
  COMP_DEF(NGAP, )              \
  COMP_DEF(GNB_APP, log)        \
  COMP_DEF(NR_RRC, log)         \
  COMP_DEF(NR_MAC, log)         \
  COMP_DEF(NR_MAC_DCI, log)         \
  COMP_DEF(NR_PHY_DCI, log)         \
  COMP_DEF(NR_PHY, log)         \
  COMP_DEF(LOADER, log)         \
  COMP_DEF(ASN1, log)           \
  COMP_DEF(NFAPI_VNF, log)      \
  COMP_DEF(NFAPI_PNF, log)      \
  COMP_DEF(ITTI, log)           \
  COMP_DEF(UTIL, log)           \
  COMP_DEF(MAX_LOG_PREDEF_COMPONENTS, )

#define COMP_ENUM(comp, file_extension) comp,
typedef enum { FOREACH_COMP(COMP_ENUM) } comp_name_t;

#define COMP_TEXT(comp, file_extension) #comp,
static const char *const comp_name[] = {FOREACH_COMP(COMP_TEXT)};

#define COMP_EXTENSION(comp, file_extension) #file_extension,
static const char *const comp_extension[] = {FOREACH_COMP(COMP_EXTENSION)};

#define MAX_LOG_DYNALLOC_COMPONENTS 20
#define MAX_LOG_COMPONENTS (MAX_LOG_PREDEF_COMPONENTS + MAX_LOG_DYNALLOC_COMPONENTS)

typedef struct {
  char *name; /*!< \brief string name of item */
  int value;  /*!< \brief integer value of mapping */
} mapping;

typedef int(*log_vprint_func_t)(FILE *stream, const char *format, va_list ap );
typedef int(*log_print_func_t)(FILE *stream, const char *format, ... );
typedef struct {
  int savedlevel;
  char *filelog_name;
} log_component_back_t;

typedef struct  {
  const char        *name;
  int level;
  int filelog;
  FILE              *stream;
  log_vprint_func_t vprint;
  log_print_func_t print;
} log_component_t;


typedef struct {
  log_component_t         log_component[MAX_LOG_COMPONENTS];
  log_component_back_t log_rarely_used[MAX_LOG_COMPONENTS];
  char                    level2string[NUM_LOG_LEVEL];
  int                     flag;
  char                   *filelog_name;
  uint64_t                debug_mask;
  uint64_t                dump_mask;
} log_t;

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

/*----------------------------------------------------------------------------*/
int  logInit (void);
void logTerm (void);
int  isLogInitDone (void);
void logRecord_mt(const char *file, const char *func, int line,int comp, int level, const char *format, ...) __attribute__ ((format (printf, 6, 7)));
void vlogRecord_mt(const char *file, const char *func, int line, int comp, int level, const char *format, va_list args );
void log_dump(int component, void *buffer, int buffsize,int datatype, const char *format, ... );
int  set_log(int component, int level);
void set_glog(int level);

mapping * log_level_names_ptr(void);
mapping * log_option_names_ptr(void);
mapping * log_maskmap_ptr(void);
void set_glog_onlinelog(int enable);
void set_glog_filelog(int enable);
void set_component_filelog(int comp);
void close_component_filelog(int comp);
void set_component_consolelog(int comp);
int map_str_to_int(const mapping *map, const char *str);
char *map_int_to_str(const mapping *map, const int val);
void logClean (void);

int register_log_component(const char *name, const char *fext, int compidx);

int logInit_log_mem(char*);
void close_log_mem(void);

/** @}*/

/*!\fn int32_t write_file_matlab(const char *fname, const char *vname, void *data, int length, int dec, char format);
\brief Write output file from signal data
@param fname output file name
@param vname  output vector name (for MATLAB/OCTAVE)
@param data   point to data
@param length length of data vector to output
@param dec    decimation level
@param format data format (0 = real 16-bit, 1 = complex 16-bit,2 real 32-bit, 3 complex 32-bit,4 = real 8-bit, 5 = complex 8-bit)
@param multiVec create new file or append to existing (useful for writing multiple vectors to same file. Just call the function multiple times with same file name and with this parameter set to 1)
*/
#define MATLAB_RAW (1U<<31)
#define MATLAB_SHORT 0
#define MATLAB_CSHORT 1
#define MATLAB_INT 2
#define MATLAB_CINT 3
#define MATLAB_INT8 4
#define MATLAB_CINT8 5
#define MATLAB_LLONG 6
#define MATLAB_DOUBLE 7
#define MATLAB_CDOUBLE 8
#define MATLAB_UINT8 9
#define MATLEB_EREN1 10
#define MATLEB_EREN2 11
#define MATLEB_EREN3 12
#define MATLAB_CSHORT_BRACKET1 13
#define MATLAB_CSHORT_BRACKET2 14
#define MATLAB_CSHORT_BRACKET3 15
  
int32_t write_file_matlab(const char *fname, const char *vname, void *data, int length, int dec, unsigned int format, int multiVec);
#define write_output(a, b, c, d, e, f) write_file_matlab(a, b, c, d, e, f, 0)

/*----------------macro definitions for reading log configuration from the config module */
#define CONFIG_STRING_LOG_PREFIX                           "log_config"

#define LOG_CONFIG_STRING_GLOBAL_LOG_LEVEL                 "global_log_level"
#define LOG_CONFIG_STRING_GLOBAL_LOG_ONLINE                "global_log_online"
#define LOG_CONFIG_STRING_GLOBAL_LOG_INFILE                "global_log_infile"
#define LOG_CONFIG_STRING_GLOBAL_LOG_OPTIONS               "global_log_options"

#define LOG_CONFIG_LEVEL_FORMAT                            "%s_log_level"
#define LOG_CONFIG_LOGFILE_FORMAT                          "%s_log_infile"
#define LOG_CONFIG_DEBUG_FORMAT                            "%s_debug"
#define LOG_CONFIG_DUMP_FORMAT                             "%s_dump"

#define LOG_CONFIG_HELP_OPTIONS      " list of comma separated options to enable log module behavior. Available options: \n"\
  " nocolor:   disable color usage in log messages\n"\
  " level:     add log level indication in log messages\n"\
  " thread:    add threads names in log messages\n"

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*   LOG global configuration parameters                                                                                                                                                */
/*   optname                               help                                          paramflags         XXXptr               defXXXval                          type        numelt */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
// clang-format off
#define LOG_GLOBALPARAMS_DESC { \
  {LOG_CONFIG_STRING_GLOBAL_LOG_LEVEL,   "Default log level for all componemts\n",              0, .strptr=&gloglevel,         .defstrval=log_level_names[3].name, TYPE_STRING,     0}, \
  {LOG_CONFIG_STRING_GLOBAL_LOG_ONLINE,  "Default console output option, for all components\n", 0, .iptr=&(consolelog),        .defintval=1,                       TYPE_INT,        0}, \
  {LOG_CONFIG_STRING_GLOBAL_LOG_OPTIONS, LOG_CONFIG_HELP_OPTIONS,                               0, .strlistptr=NULL,           .defstrlistval=NULL,                TYPE_STRINGLIST, 0}, \
}
// clang-format on

#define LOG_OPTIONS_IDX   2


/*----------------------------------------------------------------------------------*/
/** @defgroup _debugging debugging macros
 *  @ingroup _macro
 *  @brief Macro used to call logIt function with different message levels
 * @{*/
#define LOG_DUMP_CHAR       0
#define LOG_DUMP_DOUBLE     1
// debugging macros
#define LOG_F  LOG_I           /* because  LOG_F was originaly to dump a message or buffer but is also used as a regular level...., to dump use LOG_DUMPMSG */

#  if T_TRACER
#include "T.h"
/* per component, level dependent macros */
#define LOG_E(c, x...)                                                    \
  do {                                                                    \
    T(T_LEGACY_##c##_ERROR, T_PRINTF(x));                                 \
    if (T_stdout) {                                                       \
      if (g_log->log_component[c].level >= OAILOG_ERR)                    \
        logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_ERR, x); \
    }                                                                     \
  } while (0)

#define LOG_W(c, x...)                                                        \
  do {                                                                        \
    T(T_LEGACY_##c##_WARNING, T_PRINTF(x));                                   \
    if (T_stdout) {                                                           \
      if (g_log->log_component[c].level >= OAILOG_WARNING)                    \
        logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_WARNING, x); \
    }                                                                         \
  } while (0)

#define LOG_A(c, x...)                                                         \
  do {                                                                         \
    T(T_LEGACY_##c##_INFO, T_PRINTF(x));                                       \
    if (T_stdout) {                                                            \
      if (g_log->log_component[c].level >= OAILOG_ANALYSIS)                    \
        logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_ANALYSIS, x); \
    }                                                                          \
  } while (0)

#define LOG_I(c, x...)                                                     \
  do {                                                                     \
    T(T_LEGACY_##c##_INFO, T_PRINTF(x));                                   \
    if (T_stdout) {                                                        \
      if (g_log->log_component[c].level >= OAILOG_INFO)                    \
        logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_INFO, x); \
    }                                                                      \
  } while (0)

#define LOG_D(c, x...)                                                      \
  do {                                                                      \
    T(T_LEGACY_##c##_DEBUG, T_PRINTF(x));                                   \
    if (T_stdout) {                                                         \
      if (g_log->log_component[c].level >= OAILOG_DEBUG)                    \
        logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_DEBUG, x); \
    }                                                                       \
  } while (0)

#define LOG_DDUMP(c, b, s, f, x...)                      \
  do {                                                   \
    T(T_LEGACY_##c##_DEBUG, T_PRINTF(x));                \
    if (T_stdout) {                                      \
      if (g_log->log_component[c].level >= OAILOG_DEBUG) \
        log_dump(c, b, s, f, x);                         \
    }                                                    \
  } while (0)

#define LOG_T(c, x...)                                                      \
  do {                                                                      \
    T(T_LEGACY_##c##_TRACE, T_PRINTF(x));                                   \
    if (T_stdout) {                                                         \
      if (g_log->log_component[c].level >= OAILOG_TRACE)                    \
        logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_TRACE, x); \
    }                                                                       \
  } while (0)

#define VLOG(c, l, f, args)                                             \
  do {                                                                  \
    if (T_stdout) {                                                     \
      if (g_log->log_component[c].level >= l)                           \
        vlogRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, l, f, args); \
    }                                                                   \
  } while (0)

/* macro used to dump a buffer or a message as in openair2/RRC/LTE/RRC_eNB.c, replaces LOG_F macro */
#define LOG_DUMPMSG(c, f, b, s, x...)      \
  do {                                     \
    if (g_log->dump_mask & f)              \
      log_dump(c, b, s, LOG_DUMP_CHAR, x); \
  } while (0)

/* bitmask dependent macros, to isolate debugging code */
#define LOG_DEBUGFLAG(D) (g_log->debug_mask & D)

/* bitmask dependent macros, to generate debug file such as matlab file or message dump */
#define LOG_DUMPFLAG(D) (g_log->dump_mask & D)

#define LOG_M(file, vector, data, len, dec, format)             \
  do {                                                          \
    write_file_matlab(file, vector, data, len, dec, format, 0); \
  } while (0)

/* define variable only used in LOG macro's */
#define LOG_VAR(A, B) A B

#else /* no T_TRACER */

#define LOG_E(c, x...)                                                  \
  do {                                                                  \
    if (g_log->log_component[c].level >= OAILOG_ERR)                    \
      logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_ERR, x); \
  } while (0)

#define LOG_W(c, x...)                                                      \
  do {                                                                      \
    if (g_log->log_component[c].level >= OAILOG_WARNING)                    \
      logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_WARNING, x); \
  } while (0)

#define LOG_A(c, x...)                                                       \
  do {                                                                       \
    if (g_log->log_component[c].level >= OAILOG_ANALYSIS)                    \
      logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_ANALYSIS, x); \
  } while (0)

#define LOG_I(c, x...)                                                   \
  do {                                                                   \
    if (g_log->log_component[c].level >= OAILOG_INFO)                    \
      logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_INFO, x); \
  } while (0)

#define LOG_D(c, x...)                                                    \
  do {                                                                    \
    if (g_log->log_component[c].level >= OAILOG_DEBUG)                    \
      logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_DEBUG, x); \
  } while (0)

#define LOG_DDUMP(c, b, s, f, x...)                    \
  do {                                                 \
    if (g_log->log_component[c].level >= OAILOG_DEBUG) \
      log_dump(c, b, s, f, x);                         \
  } while (0)

#define LOG_T(c, x...)                                                    \
  do {                                                                    \
    if (g_log->log_component[c].level >= OAILOG_TRACE)                    \
      logRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, OAILOG_TRACE, x); \
  } while (0)

#define VLOG(c, l, f, args)                                           \
  do {                                                                \
    if (g_log->log_component[c].level >= l)                           \
      vlogRecord_mt(__FILE__, __FUNCTION__, __LINE__, c, l, f, args); \
  } while (0)

#define nfapi_log(FILE, FNC, LN, COMP, LVL, FMT...)
#define LOG_DEBUGFLAG(D) (g_log->dump_mask & D)
#define LOG_DUMPFLAG(D) (g_log->debug_mask & D)
#define LOG_DUMPMSG(c, f, b, s, x...)      \
  do {                                     \
    if (g_log->dump_mask & f)              \
      log_dump(c, b, s, LOG_DUMP_CHAR, x); \
  } while (0) /* */

#define LOG_M(file, vector, data, len, dec, format)             \
  do {                                                          \
    write_file_matlab(file, vector, data, len, dec, format, 0); \
  } while (0)

#define LOG_VAR(A, B) A B
#define T_ACTIVE(a) (0)

#endif /* T_TRACER */

/* avoid warnings for variables only used in LOG macro's but set outside debug section */
#define GCC_NOTUSED   __attribute__((unused))
#define LOG_USEDINLOG_VAR(A,B) GCC_NOTUSED A B

/* unfiltered macros, useful for simulators or messages at init time, before log is configured */
#define LOG_UM(file, vector, data, len, dec, format) do { write_file_matlab(file, vector, data, len, dec, format, 0);} while(0)
#define LOG_UI(c, x...) do {logRecord_mt(__FILE__, __FUNCTION__, __LINE__,c, OAILOG_INFO, x) ; } while(0)
#define LOG_UDUMPMSG(c, b, s, f, x...) do { log_dump(c, b, s, f, x)  ;}   while (0)  /* */
#    define LOG_MM(file, vector, data, len, dec, format) do { write_file_matlab(file, vector, data, len, dec, format, 1);} while(0)
/** @}*/

/** @defgroup _useful_functions useful functions in LOG
 *  @ingroup _macro
 *  @brief Macro of some useful functions defined by LOG
 * @{*/
#define LOG_ENTER(c) do {LOG_T(c, "Entering %s\n",__FUNCTION__);}while(0) /*!< \brief Macro to log a message with severity DEBUG when entering a function */
#define LOG_END(c) do {LOG_T(c, "End of  %s\n",__FUNCTION__);}while(0) /*!< \brief Macro to log a message with severity DEBUG when entering a function */
#define LOG_EXIT(c)  do { LOG_END(c); return;}while(0)  /*!< \brief Macro to log a message with severity TRACE when exiting a function */
#define LOG_RETURN(c,r) do {LOG_T(c,"Leaving %s (rc = %08lx)\n", __FUNCTION__ , (unsigned long)(r) );return(r);}while(0)  /*!< \brief Macro to log a function exit, including integer value, then to return a value to the calling function */

/** @}*/

#ifdef __cplusplus
}
#endif

#endif
