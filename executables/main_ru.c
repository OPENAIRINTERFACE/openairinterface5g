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

/*! \file oairu.c
 * \brief Top-level threads for radio-unit
 * \author R. Knopp
 * \date 2020
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */


#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>




#include "assertions.h"
#include "PHY/types.h"

#include "PHY/defs_RU.h"
#include "common/ran_context.h"
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"


#include "../../ARCH/COMMON/common_lib.h"
#include "../../ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"


#include "PHY/phy_vars.h"
#include "SCHED/sched_common_vars.h"
#include "PHY/TOOLS/phy_scope_interface.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/INIT/phy_init.h"

#include "system.h"

#include <executables/split_headers.h>
#include <targets/RT/USER/lte-softmodem.h>

pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.
int config_sync_var=-1;

volatile int             oai_exit = 0;
uint16_t sf_ahead = 4;
RU_t ru_m;

void init_RU0(RU_t *ru,int ru_id,char *rf_config_file, int send_dmrssync);

void exit_function(const char *file, const char *function, const int line, const char *s) {

  if (s != NULL) {
    printf("%s:%d %s() Exiting OAI softmodem: %s\n",file,line, function, s);
  }
  close_log_mem();
  oai_exit = 1;

  if (ru_m.rfdevice.trx_end_func) {
      ru_m.rfdevice.trx_end_func(&ru_m.rfdevice);
      ru_m.rfdevice.trx_end_func = NULL;
  }

  if (ru_m.ifdevice.trx_end_func) {
      ru_m.ifdevice.trx_end_func(&ru_m.ifdevice);
      ru_m.ifdevice.trx_end_func = NULL;
  }
  
  sleep(1); //allow lte-softmodem threads to exit first
  exit(1);
}


static void get_options(void) {
  CONFIG_SETRTFLAG(CONFIG_NOEXITONHELP);
  get_common_options(SOFTMODEM_ENB_BIT );
  CONFIG_CLEARRTFLAG(CONFIG_NOEXITONHELP);

  //RCConfig();
  
}





extern void  phy_free_RU(RU_t *);

nfapi_mode_t nfapi_getmode(void) {
  return(NFAPI_MODE_PNF);
}

void oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind) {

  AssertFatal(1==0,"This is bad ... please check why we get here\n");
}

void wait_eNBs(void){ return; }

uint64_t                 downlink_frequency[MAX_NUM_CCs][4];

int main ( int argc, char **argv )
{

  if ( load_configmodule(argc,argv,0) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }

  logInit();
  printf("Reading in command-line options\n");
  get_options ();

  if (CONFIG_ISFLAGSET(CONFIG_ABORT) ) {
    fprintf(stderr,"Getting configuration failed\n");
    exit(-1);
  }

#if T_TRACER
  T_Config_Init();
#endif
  printf("configuring for RRU\n");

#ifndef PACKAGE_VERSION
#  define PACKAGE_VERSION "UNKNOWN-EXPERIMENTAL"
#endif
  LOG_I(HW, "Version: %s\n", PACKAGE_VERSION);

  /* Read configuration */

  printf("About to Init RU threads\n");
  

  RU_t *ru=&ru_m;
  init_RU0(ru,0,get_softmodem_params()->rf_config_file,get_softmodem_params()->send_dmrs_sync);
  ru->rf_map.card=0;
  ru->rf_map.chain=(get_softmodem_params()->chain_offset);
  

  config_sync_var=0;
  // once all RUs are ready intiailize the rest of the eNBs ((dependence on final RU parameters after configuration)
  printf("ALL RUs ready - init eNBs\n");
    
  while (oai_exit==0) sleep(1);
  // stop threads

  kill_RU_proc(ru);
  phy_free_RU(ru);

  free_lte_top();
  end_configmodule();

  if (ru->rfdevice.trx_end_func) {
    ru->rfdevice.trx_end_func(&ru->rfdevice);
    ru->rfdevice.trx_end_func = NULL;
  }

  if (ru->ifdevice.trx_end_func) {
    ru->ifdevice.trx_end_func(&ru->ifdevice);
    ru->ifdevice.trx_end_func = NULL;
  }

  

  logClean();
  printf("Bye.\n");
  return 0;
}
