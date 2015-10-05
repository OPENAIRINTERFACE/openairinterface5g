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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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
