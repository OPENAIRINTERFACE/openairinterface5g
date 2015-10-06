/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
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
# include "UTIL/LOG/log.h"
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

#if defined(NAS_BUILT_IN_UE) && defined(NAS_UE)
# define LOG_TRACE(s, x, args...)                               \
do {                                                            \
    switch (s) {                                                \
        case ERROR:     LOG_E(NAS, " %s:%d  " x "\n", __FILE__, __LINE__, ##args); break;  \
        case WARNING:   LOG_W(NAS, " %s:%d  " x "\n", __FILE__, __LINE__, ##args); break;  \
        case INFO:      LOG_I(NAS, " %s:%d  " x "\n", __FILE__, __LINE__, ##args); break;  \
        default:        LOG_D(NAS, " %s:%d  " x "\n", __FILE__, __LINE__, ##args); break;  \
    }                                                           \
} while (0)

# define LOG_DUMP(dATA, lEN)                                                    \
do {                                                                            \
    char buffer[3*lEN + 1];                                                     \
    int i;                                                                      \
    for (i = 0; i < lEN; i++)                                                   \
        sprintf (&buffer[3*i], "%02x ", dATA[i]);                               \
    LOG_D(NAS, " Dump %d: %s\n", lEN, buffer);                                  \
} while (0)

# define LOG_FUNC_IN                                                            \
do {                                                                            \
    LOG_D(NAS, " %s:%d %*sEntering %s()\n", __FILE__, __LINE__, nas_log_func_indent, "", __FUNCTION__);   \
    nas_log_func_indent += 2;                                                   \
} while (0)

# define LOG_FUNC_OUT                                                           \
do {                                                                            \
    nas_log_func_indent -= 2;                                                   \
    LOG_D(NAS, " %s:%d %*sLeaving %s()\n", __FILE__, __LINE__, nas_log_func_indent, "", __FUNCTION__);    \
} while (0)

# define LOG_FUNC_RETURN(rETURNcODE)                                            \
do {                                                                            \
    nas_log_func_indent -= 2;                                                   \
    LOG_D(NAS, " %s:%d %*sLeaving %s(rc = %ld)\n", __FILE__, __LINE__, nas_log_func_indent, "",           \
          __FUNCTION__, (long) rETURNcODE);                                     \
    return (rETURNcODE);                                                        \
} while (0)

extern int nas_log_func_indent;

#else
# define LOG_TRACE log_data(__FILE__, __LINE__); log_trace
# define LOG_DUMP(a, b) log_dump((a),(b));

# define LOG_FUNC_IN LOG_TRACE(FUNC_IN, "Entering %s()", __FUNCTION__)
# define LOG_FUNC_OUT LOG_TRACE(FUNC_OUT, "Leaving %s()", __FUNCTION__)
# define LOG_FUNC_RETURN(rETURNcODE)                                            \
do {                                                                           \
    LOG_TRACE(FUNC_OUT, "Leaving %s(rc = %ld)", __FUNCTION__,                  \
    (long) rETURNcODE);                                                        \
    return (rETURNcODE);                                                       \
} while(0)

void nas_log_init(char filter);
void log_data(const char* filename, int line);
void log_trace(log_severity_t severity, const char* data, ...);
void log_dump(const char* data, int len);
#endif

#endif /* __NAS_LOG_H__*/
