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

/*
 *                               fifo_printf.c
 *                             -------------------
 *  AUTHOR  : Lionel GAUTHIER
 *  COMPANY : EURECOM
 *  EMAIL   : Lionel.Gauthier@eurecom.fr
 *
 */
#include "rtos_header.h"
#include "platform.h"
#include "protocol_vars_extern.h"

#include <asm/page.h>
#include <asm/system.h>
#include <stdarg.h>


#include "fifo_printf.h"

#ifndef FIFO_PRINTF
//-----------------------------------------------------------------------------
int fifo_printf_null (const char *fmt, ...)
{
  return 0;
}
#else
static uint8_t       fifo_printed_l1[FIFO_PRINTF_MAX_STRING_SIZE];
static uint8_t       fifo_printed_l2[FIFO_PRINTF_MAX_STRING_SIZE];
//-----------------------------------------------------------------------------
void
fifo_printf_init (void)
{
  //-----------------------------------------------------------------------------
  printk ("[TRACE] INIT\n");
  rtf_create (FIFO_PRINTF_L1_NO, FIFO_PRINTF_SIZE);
  rtf_create (FIFO_PRINTF_L2_NO, FIFO_PRINTF_SIZE);
}

//-----------------------------------------------------------------------------
void
fifo_printf_clean_up (void)
{
  //-----------------------------------------------------------------------------
  rtf_destroy (FIFO_PRINTF_L1_NO);
  rtf_destroy (FIFO_PRINTF_L2_NO);
}

//-----------------------------------------------------------------------------
int
fifo_printf (const char *fmt, ...)
{
  //-----------------------------------------------------------------------------
  int             i;
  va_list         args;

  va_start (args, fmt);

  if (pthread_self () == &task_l1l) {
    i = vsprintf (fifo_printed_l1, fmt, args);
    va_end (args);

    /* perhaps we should discard old data instead */
    if (i > FIFO_PRINTF_MAX_STRING_SIZE) {
      rt_printk ("[WCDMA] FIFO_PRINTF WROTE OUTSIDE ITS MEMORY BOUNDARY : ERRORS WILL OCCUR\n");
    }

    if (i <= 0) {
      return 0;
    }

    rtf_put (FIFO_PRINTF_L1_NO, fifo_printed_l1, i);

    return i;
  }

  if (pthread_self () == &task_l2) {
    i = vsprintf (fifo_printed_l2, fmt, args);
    va_end (args);

    /* perhaps we should discard old data instead */
    if (i > FIFO_PRINTF_MAX_STRING_SIZE) {
      rt_printk ("[WCDMA] FIFO_PRINTF WROTE OUTSIDE ITS MEMORY BOUNDARY : ERRORS WILL OCCUR\n");
    }

    if (i <= 0) {
      return 0;
    }

    rtf_put (FIFO_PRINTF_L2_NO, fifo_printed_l2, i);

    return i;
  }

  return 0;
}
#endif
