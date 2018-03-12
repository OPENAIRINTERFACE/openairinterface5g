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
