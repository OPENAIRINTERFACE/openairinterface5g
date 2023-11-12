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
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"


#include "radio/COMMON/common_lib.h"
#include "radio/ETHERNET/if_defs.h"

#include "PHY/phy_vars.h"
#include "PHY/phy_extern.h"
#include "PHY/TOOLS/phy_scope_interface.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/INIT/phy_init.h"
#include "openair2/ENB_APP/enb_paramdef.h"
#include "system.h"
#include "nfapi/oai_integration/vendor_ext.h"

#include <executables/softmodem-common.h>
#include <executables/thread-common.h>

static int DEFBANDS[] = {7};
static int DEFENBS[] = {0};
static int DEFBFW[] = {0x00007fff};

static int DEFRUTPCORES[] = {2,4,6,8};
THREAD_STRUCT thread_struct;

pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.
int config_sync_var=-1;

int oai_exit = 0;
uint16_t sf_ahead = 4;
RU_t ru_m;


extern void init_RU0(RU_t *ru,int ru_id,char *rf_config_file, int send_dmrssync);
extern void init_RU_proc(RU_t *ru);
extern void kill_RU_proc(RU_t *ru);
extern void set_function_spec_param(RU_t *ru);

int32_t uplink_frequency_offset[MAX_NUM_CCs][4];


void nfapi_setmode(nfapi_mode_t nfapi_mode) { return; }
void exit_function(const char *file, const char *function, const int line, const char *s, const int assert) {

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

  pthread_mutex_destroy(ru_m.ru_mutex);
  pthread_cond_destroy(ru_m.ru_cond);
  if (assert) {
    abort();
  } else {
    sleep(1); // allow lte-softmodem threads to exit first
    exit(EXIT_SUCCESS);
  }
}

static void get_options(configmodule_interface_t *cfg)
{
  CONFIG_SETRTFLAG(CONFIG_NOEXITONHELP);
  get_common_options(cfg, SOFTMODEM_ENB_BIT);
  CONFIG_CLEARRTFLAG(CONFIG_NOEXITONHELP);

  // RCConfig();
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

configmodule_interface_t *uniqCfg = NULL;

int main ( int argc, char **argv )
{
  if ((uniqCfg = load_configmodule(argc, argv, 0)) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }

  logInit();
  printf("Reading in command-line options\n");
  get_options(uniqCfg);

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

  paramdef_t RUParams[] = RUPARAMS_DESC;
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};
  config_getlist(config_get_if(), &RUParamList, RUParams, sizeofArray(RUParams), NULL);

  int j=0;
  uint64_t ru_mask=1;
  pthread_mutex_t ru_mutex; pthread_mutex_init(&ru_mutex,NULL);
  pthread_cond_t ru_cond; pthread_cond_init(&ru_cond,NULL);

  ru->ru_mask= &ru_mask;
  ru->ru_mutex = &ru_mutex;
  ru->ru_cond = &ru_cond;

  ru->if_timing = synch_to_ext_device;
  ru->num_eNB = 0;
  ru->has_ctrl_prt = 1;
  if (config_isparamset(RUParamList.paramarray[j], RU_SDR_ADDRS)) {
    ru->openair0_cfg.sdr_addrs = strdup(*(RUParamList.paramarray[j][RU_SDR_ADDRS].strptr));
  }

  if (config_isparamset(RUParamList.paramarray[j], RU_SDR_CLK_SRC)) {
    if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "internal") == 0) {
      ru->openair0_cfg.clock_source = internal;
      LOG_D(PHY, "RU clock source set as internal\n");
    } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "external") == 0) {
      ru->openair0_cfg.clock_source = external;
      LOG_D(PHY, "RU clock source set as external\n");
    } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "gpsdo") == 0) {
      ru->openair0_cfg.clock_source = gpsdo;
      LOG_D(PHY, "RU clock source set as gpsdo\n");
    } else {
      LOG_E(PHY, "Erroneous RU clock source in the provided configuration file: '%s'\n", *(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr));
    }
  }
  else {
    ru->openair0_cfg.clock_source = unset;
  }

  if (config_isparamset(RUParamList.paramarray[j], RU_SDR_TME_SRC)) {
    if (strcmp(*(RUParamList.paramarray[j][RU_SDR_TME_SRC].strptr), "internal") == 0) {
      ru->openair0_cfg.time_source = internal;
      LOG_D(PHY, "RU time source set as internal\n");
    } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_TME_SRC].strptr), "external") == 0) {
      ru->openair0_cfg.time_source = external;
      LOG_D(PHY, "RU time source set as external\n");
    } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_TME_SRC].strptr), "gpsdo") == 0) {
      ru->openair0_cfg.time_source = gpsdo;
      LOG_D(PHY, "RU time source set as gpsdo\n");
    } else {
      LOG_E(PHY, "Erroneous RU time source in the provided configuration file: '%s'\n", *(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr));
    }
  }
  else {
    ru->openair0_cfg.time_source = unset;
  }      

  if (strcmp(*(RUParamList.paramarray[j][RU_LOCAL_RF_IDX].strptr), "yes") == 0) {
    if ( !(config_isparamset(RUParamList.paramarray[j],RU_LOCAL_IF_NAME_IDX)) ) {
      AssertFatal(1==0,"IF_NAME is required\n");
    } else {
      ru->eth_params.local_if_name            = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));
      ru->eth_params.my_addr                  = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr));
      ru->eth_params.remote_addr              = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
      ru->eth_params.my_portd                 = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
      ru->eth_params.remote_portd             = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);

      // Check if control port set
      if  (!(config_isparamset(RUParamList.paramarray[j],RU_REMOTE_PORTC_IDX)) ) {
	printf("Removing control port for RU %d\n",j);
	ru->has_ctrl_prt            = 0;
      } else {
	ru->eth_params.my_portc                 = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
	ru->eth_params.remote_portc             = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
	printf(" Control port %u \n",ru->eth_params.my_portc);
      }

      if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp") == 0) {
	ru->if_south                        = LOCAL_RF;
	ru->function                        = NGFI_RRU_IF5;
	ru->eth_params.transp_preference    = ETH_UDP_MODE;
	printf("Setting function for RU %d to NGFI_RRU_IF5 (udp)\n",j);
      } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw") == 0) {
	ru->if_south                        = LOCAL_RF;
	ru->function                        = NGFI_RRU_IF5;
	ru->eth_params.transp_preference    = ETH_RAW_MODE;
	printf("Setting function for RU %d to NGFI_RRU_IF5 (raw)\n",j);
      } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_if4p5") == 0) {
	ru->if_south                        = LOCAL_RF;
	ru->function                        = NGFI_RRU_IF4p5;
	ru->eth_params.transp_preference    = ETH_UDP_IF4p5_MODE;
	printf("Setting function for RU %d to NGFI_RRU_IF4p5 (udp)\n",j);
      } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw_if4p5") == 0) {
	ru->if_south                        = LOCAL_RF;
	ru->function                        = NGFI_RRU_IF4p5;
	ru->eth_params.transp_preference    = ETH_RAW_IF4p5_MODE;
	printf("Setting function for RU %d to NGFI_RRU_IF4p5 (raw)\n",j);
      }

      printf("RU %d is_slave=%s\n",j,*(RUParamList.paramarray[j][RU_IS_SLAVE_IDX].strptr));

      if (strcmp(*(RUParamList.paramarray[j][RU_IS_SLAVE_IDX].strptr), "yes") == 0) ru->is_slave=1;
      else ru->is_slave=0;

      printf("RU %d ota_sync_enabled=%s\n",j,*(RUParamList.paramarray[j][RU_OTA_SYNC_ENABLE_IDX].strptr));

      if (strcmp(*(RUParamList.paramarray[j][RU_OTA_SYNC_ENABLE_IDX].strptr), "yes") == 0) ru->ota_sync_enable=1;
      else ru->ota_sync_enable=0;
    }

    ru->max_pdschReferenceSignalPower     = *(RUParamList.paramarray[j][RU_MAX_RS_EPRE_IDX].uptr);;
    ru->max_rxgain                        = *(RUParamList.paramarray[j][RU_MAX_RXGAIN_IDX].uptr);
    ru->num_bands                         = RUParamList.paramarray[j][RU_BAND_LIST_IDX].numelt;
    /* sf_extension is in unit of samples for 30.72MHz here, has to be scaled later */
    ru->sf_extension                      = *(RUParamList.paramarray[j][RU_SF_EXTENSION_IDX].uptr);

    for (int i=0; i<ru->num_bands; i++) ru->band[i] = RUParamList.paramarray[j][RU_BAND_LIST_IDX].iptr[i];
  } //strcmp(local_rf, "yes") == 0
  else {
    printf("RU %d: Transport %s\n",j,*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr));
    ru->eth_params.local_if_name      = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));
    ru->eth_params.my_addr            = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr));
    ru->eth_params.remote_addr        = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
    ru->eth_params.my_portc           = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
    ru->eth_params.remote_portc       = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
    ru->eth_params.my_portd           = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
    ru->eth_params.remote_portd       = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);

    if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp") == 0) {
      ru->if_south                     = REMOTE_IF5;
      ru->function                     = NGFI_RAU_IF5;
      ru->eth_params.transp_preference = ETH_UDP_MODE;
    } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw") == 0) {
      ru->if_south                     = REMOTE_IF5;
      ru->function                     = NGFI_RAU_IF5;
      ru->eth_params.transp_preference = ETH_RAW_MODE;
    } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_if4p5") == 0) {
      ru->if_south                     = REMOTE_IF4p5;
      ru->function                     = NGFI_RAU_IF4p5;
      ru->eth_params.transp_preference = ETH_UDP_IF4p5_MODE;
    } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw_if4p5") == 0) {
      ru->if_south                     = REMOTE_IF4p5;
      ru->function                     = NGFI_RAU_IF4p5;
      ru->eth_params.transp_preference = ETH_RAW_IF4p5_MODE;
    }

    if (strcmp(*(RUParamList.paramarray[j][RU_IS_SLAVE_IDX].strptr), "yes") == 0) ru->is_slave=1;
    else ru->is_slave=0;
  }  /* strcmp(local_rf, "yes") != 0 */
      
  ru->nb_tx                             = *(RUParamList.paramarray[j][RU_NB_TX_IDX].uptr);
  ru->nb_rx                             = *(RUParamList.paramarray[j][RU_NB_RX_IDX].uptr);
  ru->att_tx                            = *(RUParamList.paramarray[j][RU_ATT_TX_IDX].uptr);
  ru->att_rx                            = *(RUParamList.paramarray[j][RU_ATT_RX_IDX].uptr);


  set_worker_conf("WORKER_ENABLE");

  mlockall(MCL_CURRENT | MCL_FUTURE);
  pthread_cond_init(&sync_cond,NULL);
  pthread_mutex_init(&sync_mutex, NULL);
 
  init_RU0(ru,0,get_softmodem_params()->rf_config_file,get_softmodem_params()->send_dmrs_sync);
  ru->rf_map.card=0;
  ru->rf_map.chain=(get_softmodem_params()->chain_offset);

  LOG_I(PHY, "Initializing RRU descriptor : (%s,%s,%d)\n", ru_if_types[ru->if_south], NB_timing[ru->if_timing], ru->function);
  set_function_spec_param(ru);
  LOG_I(PHY, "Starting ru_thread , is_slave %d, send_dmrs %d\n", ru->is_slave, ru->generate_dmrs_sync);
  init_RU_proc(ru);

  pthread_mutex_lock(&ru_mutex);
  while (ru_mask>0) 
    pthread_cond_wait(&ru_cond,&ru_mutex);
  pthread_mutex_unlock(&ru_mutex);
  LOG_I(PHY,"RU configured, unlocking threads\n"); 
  config_sync_var=0;
  pthread_mutex_lock(&sync_mutex);
  sync_var=0;
  pthread_cond_broadcast(&sync_cond);
  pthread_mutex_unlock(&sync_mutex);
 
  while (oai_exit==0) sleep(1);
  // stop threads
      
  kill_RU_proc(ru);
  phy_free_RU(ru);
      
  free_lte_top();
  end_configmodule(uniqCfg);

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
