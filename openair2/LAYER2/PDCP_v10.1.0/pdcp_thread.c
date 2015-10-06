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

/*! \file pdcp_thread.c
 * \brief
 * \author F. Kaltenberger
 * \date 2013
 * \version 0.1
 * \company Eurecom
 * \email: florian.kaltenberger@eurecom.fr
 * \note
 * \warning
 */
#include <pthread.h>
//#include <inttypes.h>

#include "pdcp.h"
#include "PHY/extern.h" //for PHY_vars
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "msc.h"

#define OPENAIR_THREAD_STACK_SIZE    8192
#define OPENAIR_THREAD_PRIORITY        255

extern int  oai_exit;
extern char UE_flag;

pthread_t       pdcp_thread;
pthread_attr_t  pdcp_thread_attr;
pthread_mutex_t pdcp_mutex;
pthread_cond_t  pdcp_cond;
int             pdcp_instance_cnt;

static void *pdcp_thread_main(void* param);

static void *pdcp_thread_main(void* param)
{
  uint8_t eNB_flag = !UE_flag;

  LOG_I(PDCP,"This is pdcp_thread eNB_flag = %d\n",eNB_flag);
  MSC_START_USE();

  while (!oai_exit) {

    if (pthread_mutex_lock(&pdcp_mutex) != 0) {
      LOG_E(PDCP,"Error locking mutex.\n");
    } else {
      while (pdcp_instance_cnt < 0) {
        pthread_cond_wait(&pdcp_cond,&pdcp_mutex);
      }

      if (pthread_mutex_unlock(&pdcp_mutex) != 0) {
        LOG_E(PDCP,"Error unlocking mutex.\n");
      }
    }

    if (oai_exit) break;

    if (eNB_flag) {
      pdcp_run(PHY_vars_eNB_g[0]->frame, eNB_flag, PHY_vars_eNB_g[0]->Mod_id, 0);
      LOG_D(PDCP,"Calling pdcp_run (eNB) for frame %d\n",PHY_vars_eNB_g[0]->frame);
    } else  {
      pdcp_run(PHY_vars_UE_g[0]->frame, eNB_flag, 0, PHY_vars_UE_g[0]->Mod_id);
      LOG_D(PDCP,"Calling pdcp_run (UE) for frame %d\n",PHY_vars_UE_g[0]->frame);
    }

    if (pthread_mutex_lock(&pdcp_mutex) != 0) {
      LOG_E(PDCP,"Error locking mutex.\n");
    } else {
      pdcp_instance_cnt--;

      if (pthread_mutex_unlock(&pdcp_mutex) != 0) {
        LOG_E(PDCP,"Error unlocking mutex.\n");
      }
    }
  }

  return(NULL);
}


int init_pdcp_thread(void)
{

  int    error_code;
  struct sched_param p;

  pthread_attr_init (&pdcp_thread_attr);
  pthread_attr_setstacksize(&pdcp_thread_attr,OPENAIR_THREAD_STACK_SIZE);
  //attr_dlsch_threads.priority = 1;

  p.sched_priority = OPENAIR_THREAD_PRIORITY;
  pthread_attr_setschedparam  (&pdcp_thread_attr, &p);
#ifndef RTAI_ISNT_POSIX
  pthread_attr_setschedpolicy (&pdcp_thread_attr, SCHED_FIFO);
#endif
  pthread_mutex_init(&pdcp_mutex,NULL);
  pthread_cond_init(&pdcp_cond,NULL);

  pdcp_instance_cnt = -1;
  LOG_I(PDCP,"Allocating PDCP thread\n");
  error_code = pthread_create(&pdcp_thread,
                              &pdcp_thread_attr,
                              pdcp_thread_main,
                              (void*)NULL);

  if (error_code!= 0) {
    LOG_I(PDCP,"Could not allocate PDCP thread, error %d\n",error_code);
    return(error_code);
  } else {
    LOG_I(PDCP,"Allocate PDCP thread successful\n");
    pthread_setname_np( pdcp_thread, "PDCP" );
  }

  return(0);
}


void cleanup_pdcp_thread(void)
{
  void *status_p = NULL;

  LOG_I(PDCP,"Scheduling PDCP thread to exit\n");

  pdcp_instance_cnt = 0;

  if (pthread_cond_signal(&pdcp_cond) != 0) {
    LOG_I(PDCP,"ERROR pthread_cond_signal\n");
  } else {
    LOG_I(PDCP,"Signalled PDCP thread to exit\n");
  }

  pthread_join(pdcp_thread,&status_p);
  LOG_I(PDCP,"PDCP thread exited\n");
  pthread_cond_destroy(&pdcp_cond);
  pthread_mutex_destroy(&pdcp_mutex);
}
