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
/*!\brief SCHED external variables */

#ifndef __SCHED_EXTERN_H__
#define __SCHED_EXTERN_H__

#ifndef USER_MODE
#define __NO_VERSION__


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>

#include <asm/io.h>
#include <asm/bitops.h>

#include <asm/uaccess.h>
#include <asm/segment.h>
#include <asm/page.h>




#ifdef RTAI_ENABLED
#include <rtai.h>
//#include <rtai_posix.h>
#include <rtai_fifos.h>
#include <rtai_sched.h>
#include <rtai_sem.h>
//#include "rt_compat.h"

#else
#include <unistd.h>
#endif

#endif  /* USER_MODE */

#include "defs.h"
//#include "dlc_engine.h"

extern int openair_sched_status;

//extern int exit_PHY;
//extern int exit_PHY_ack;

extern int synch_wait_cnt;

extern OPENAIR_DAQ_VARS openair_daq_vars;

extern int16_t hundred_times_delta_TF[100];
extern uint16_t hundred_times_log10_NPRB[100];
/*
#ifdef EMOS
extern fifo_dump_emos_UE emos_dump_UE;
extern fifo_dump_emos_eNB emos_dump_eNB;
#endif
*/

#endif /*__SCHED_EXTERN_H__ */
