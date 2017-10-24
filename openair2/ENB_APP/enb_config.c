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
  enb_config.c
  -------------------
  AUTHOR  : Lionel GAUTHIER, navid nikaein, Laurent Winckel
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr, navid.nikaein@eurecom.fr
*/

#include <string.h>
#include <inttypes.h>

#include "log.h"
#include "log_extern.h"
#include "assertions.h"
#include "enb_config.h"
#include "UTIL/OTG/otg.h"
#include "UTIL/OTG/otg_externs.h"
#if defined(OAI_EMU)
# include "OCG.h"
# include "OCG_extern.h"
#endif
#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# if defined(ENABLE_USE_MME)
#   include "s1ap_eNB.h"
#   include "sctp_eNB_task.h"
# endif
#endif
#include "sctp_default_values.h"
#include "SystemInformationBlockType2.h"
#include "LAYER2/MAC/extern.h"
#include "PHY/extern.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"
#include "enb_paramdef.h"
#include "common/config/config_userapi.h"


int RCconfig_RRC(MessageDef *msg_p, uint32_t i, eNB_RRC_INST *rrc);
int RCconfig_S1(MessageDef *msg_p, uint32_t i);


static int enb_check_band_frequencies(char* lib_config_file_name_pP,
                                      int ind,
                                      int16_t band,
                                      uint32_t downlink_frequency,
                                      int32_t uplink_frequency_offset,
                                      lte_frame_type_t frame_type)
{
  int errors = 0;

  if (band > 0) {
    int band_index;

    for (band_index = 0; band_index < sizeof (eutra_bands) / sizeof (eutra_bands[0]); band_index++) {
      if (band == eutra_bands[band_index].band) {
        uint32_t uplink_frequency = downlink_frequency + uplink_frequency_offset;

        AssertError (eutra_bands[band_index].dl_min < downlink_frequency, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d downlink frequency %u too low (%u) for band %d!",
                     lib_config_file_name_pP, ind, downlink_frequency, eutra_bands[band_index].dl_min, band);
        AssertError (downlink_frequency < eutra_bands[band_index].dl_max, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d downlink frequency %u too high (%u) for band %d!",
                     lib_config_file_name_pP, ind, downlink_frequency, eutra_bands[band_index].dl_max, band);

        AssertError (eutra_bands[band_index].ul_min < uplink_frequency, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d uplink frequency %u too low (%u) for band %d!",
                     lib_config_file_name_pP, ind, uplink_frequency, eutra_bands[band_index].ul_min, band);
        AssertError (uplink_frequency < eutra_bands[band_index].ul_max, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d uplink frequency %u too high (%u) for band %d!",
                     lib_config_file_name_pP, ind, uplink_frequency, eutra_bands[band_index].ul_max, band);

        AssertError (eutra_bands[band_index].frame_type == frame_type, errors ++,
                     "Failed to parse eNB configuration file %s, enb %d invalid frame type (%d/%d) for band %d!",
                     lib_config_file_name_pP, ind, eutra_bands[band_index].frame_type, frame_type, band);
      }
    }
  }


  return errors;
}







/* --------------------------------------------------------*/
/* from here function to use configuration module          */
void RCconfig_RU(void) {
  
  int               j                             = 0;
  int               i                             = 0;

  
  paramdef_t RUParams[] = RUPARAMS_DESC;
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};


  config_getlist( &RUParamList,RUParams,sizeof(RUParams)/sizeof(paramdef_t), NULL);  

  
  if ( RUParamList.numelt > 0) {


    RC.ru = (RU_t**)malloc(RC.nb_RU*sizeof(RU_t*));
   



    RC.ru_mask=(1<<NB_RU) - 1;
    printf("Set RU mask to %lx\n",RC.ru_mask);

    for (j = 0; j < RC.nb_RU; j++) {

      RC.ru[j]                                    = (RU_t*)malloc(sizeof(RU_t));
      memset((void*)RC.ru[j],0,sizeof(RU_t));
      RC.ru[j]->idx                                 = j;

      RC.ru[j]->if_timing                           = synch_to_ext_device;
      if (RC.nb_L1_inst >0)
        RC.ru[j]->num_eNB                           = RUParamList.paramarray[j][RU_ENB_LIST_IDX].numelt;
      else
	    RC.ru[j]->num_eNB                           = 0;
      for (i=0;i<RC.ru[j]->num_eNB;i++) RC.ru[j]->eNB_list[i] = RC.eNB[RUParamList.paramarray[j][RU_ENB_LIST_IDX].iptr[i]][0];     


      if (strcmp(*(RUParamList.paramarray[j][RU_LOCAL_RF_IDX].strptr), "yes") == 0) {
	if ( !(config_isparamset(RUParamList.paramarray[j],RU_LOCAL_IF_NAME_IDX)) ) {
	  RC.ru[j]->if_south                        = LOCAL_RF;
	  RC.ru[j]->function                        = eNodeB_3GPP;
	  printf("Setting function for RU %d to eNodeB_3GPP\n",j);
        }
        else { 
          RC.ru[j]->eth_params.local_if_name            = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));    
          RC.ru[j]->eth_params.my_addr                  = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr)); 
          RC.ru[j]->eth_params.remote_addr              = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
          RC.ru[j]->eth_params.my_portc                 = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
          RC.ru[j]->eth_params.remote_portc             = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
          RC.ru[j]->eth_params.my_portd                 = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
          RC.ru[j]->eth_params.remote_portd             = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);

	  if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp") == 0) {
	    RC.ru[j]->if_south                        = LOCAL_RF;
	    RC.ru[j]->function                        = NGFI_RRU_IF5;
	    RC.ru[j]->eth_params.transp_preference    = ETH_UDP_MODE;
	    printf("Setting function for RU %d to NGFI_RRU_IF5 (udp)\n",j);
	  } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw") == 0) {
	    RC.ru[j]->if_south                        = LOCAL_RF;
	    RC.ru[j]->function                        = NGFI_RRU_IF5;
	    RC.ru[j]->eth_params.transp_preference    = ETH_RAW_MODE;
	    printf("Setting function for RU %d to NGFI_RRU_IF5 (raw)\n",j);
	  } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_if4p5") == 0) {
	    RC.ru[j]->if_south                        = LOCAL_RF;
	    RC.ru[j]->function                        = NGFI_RRU_IF4p5;
	    RC.ru[j]->eth_params.transp_preference    = ETH_UDP_IF4p5_MODE;
	    printf("Setting function for RU %d to NGFI_RRU_IF4p5 (udp)\n",j);
	  } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw_if4p5") == 0) {
	    RC.ru[j]->if_south                        = LOCAL_RF;
	    RC.ru[j]->function                        = NGFI_RRU_IF4p5;
	    RC.ru[j]->eth_params.transp_preference    = ETH_RAW_IF4p5_MODE;
	    printf("Setting function for RU %d to NGFI_RRU_IF4p5 (raw)\n",j);
	  }
	}
	RC.ru[j]->max_pdschReferenceSignalPower     = *(RUParamList.paramarray[j][RU_MAX_RS_EPRE_IDX].uptr);;
	RC.ru[j]->max_rxgain                        = *(RUParamList.paramarray[j][RU_MAX_RXGAIN_IDX].uptr);
	RC.ru[j]->num_bands                         = RUParamList.paramarray[j][RU_BAND_LIST_IDX].numelt;
	for (i=0;i<RC.ru[j]->num_bands;i++) RC.ru[j]->band[i] = RUParamList.paramarray[j][RU_BAND_LIST_IDX].iptr[i]; 
      } //strcmp(local_rf, "yes") == 0
      else {
	printf("RU %d: Transport %s\n",j,*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr));

        RC.ru[j]->eth_params.local_if_name	      = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));    
        RC.ru[j]->eth_params.my_addr		      = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr)); 
        RC.ru[j]->eth_params.remote_addr	      = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
        RC.ru[j]->eth_params.my_portc		      = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
        RC.ru[j]->eth_params.remote_portc	      = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
        RC.ru[j]->eth_params.my_portd		      = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
        RC.ru[j]->eth_params.remote_portd	      = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);
	if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp") == 0) {
	  RC.ru[j]->if_south                     = REMOTE_IF5;
	  RC.ru[j]->function                     = NGFI_RAU_IF5;
	  RC.ru[j]->eth_params.transp_preference = ETH_UDP_MODE;
	} else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw") == 0) {
	  RC.ru[j]->if_south                     = REMOTE_IF5;
	  RC.ru[j]->function                     = NGFI_RAU_IF5;
	  RC.ru[j]->eth_params.transp_preference = ETH_RAW_MODE;
	} else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_if4p5") == 0) {
	  RC.ru[j]->if_south                     = REMOTE_IF4p5;
	  RC.ru[j]->function                     = NGFI_RAU_IF4p5;
	  RC.ru[j]->eth_params.transp_preference = ETH_UDP_IF4p5_MODE;
	} else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw_if4p5") == 0) {
	  RC.ru[j]->if_south                     = REMOTE_IF4p5;
	  RC.ru[j]->function                     = NGFI_RAU_IF4p5;
	  RC.ru[j]->eth_params.transp_preference = ETH_RAW_IF4p5_MODE;
	} else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw_if5_mobipass") == 0) {
	  RC.ru[j]->if_south                     = REMOTE_IF5;
	  RC.ru[j]->function                     = NGFI_RAU_IF5;
	  RC.ru[j]->if_timing                    = synch_to_other;
	  RC.ru[j]->eth_params.transp_preference = ETH_RAW_IF5_MOBIPASS;
	}
	RC.ru[j]->att_tx                         = *(RUParamList.paramarray[j][RU_ATT_TX_IDX].uptr); 
	RC.ru[j]->att_rx                         = *(RUParamList.paramarray[j][RU_ATT_TX_IDX].uptr); 
      }  /* strcmp(local_rf, "yes") != 0 */

      RC.ru[j]->nb_tx                             = *(RUParamList.paramarray[j][RU_NB_TX_IDX].uptr);
      RC.ru[j]->nb_rx                             = *(RUParamList.paramarray[j][RU_NB_RX_IDX].uptr);
      
    }// j=0..num_rus
  } else {
    RC.nb_RU = 0;	    
  } // setting != NULL

  return;
  
}

void RCconfig_L1() {
  int               i,j;
  paramdef_t L1_Params[] = L1PARAMS_DESC;
  paramlist_def_t L1_ParamList = {CONFIG_STRING_L1_LIST,NULL,0};


  config_getlist( &L1_ParamList,L1_Params,sizeof(L1_Params)/sizeof(paramdef_t), NULL);    
  if (L1_ParamList.numelt > 0) {

    if (RC.eNB == NULL) {
      RC.eNB                               = (PHY_VARS_eNB ***)malloc((1+NUMBER_OF_eNB_MAX)*sizeof(PHY_VARS_eNB***));
      LOG_I(PHY,"RC.eNB = %p\n",RC.eNB);
      memset(RC.eNB,0,(1+NUMBER_OF_eNB_MAX)*sizeof(PHY_VARS_eNB***));
      RC.nb_L1_CC = malloc((1+RC.nb_L1_inst)*sizeof(int));
    }

    for (j = 0; j < RC.nb_L1_inst; j++) {
      RC.nb_L1_CC[j] = *(L1_ParamList.paramarray[j][L1_CC_IDX].uptr);


      if (RC.eNB[j] == NULL) {
	RC.eNB[j]                       = (PHY_VARS_eNB **)malloc((1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB**));
	LOG_I(PHY,"RC.eNB[%d] = %p\n",j,RC.eNB[j]);
	memset(RC.eNB[j],0,(1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB***));
      }


      for (i=0;i<RC.nb_L1_CC[j];i++) {
	if (RC.eNB[j][i] == NULL) {
	  RC.eNB[j][i] = (PHY_VARS_eNB *)malloc(sizeof(PHY_VARS_eNB));
	  memset((void*)RC.eNB[j][i],0,sizeof(PHY_VARS_eNB));
	  LOG_I(PHY,"RC.eNB[%d][%d] = %p\n",j,i,RC.eNB[j][i]);
	  RC.eNB[j][i]->Mod_id  = j;
	  RC.eNB[j][i]->CC_id   = i;
	}
      }

      if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_mac") == 0) {

      }
      else if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "nfapi") == 0) {
	RC.eNB[j][0]->eth_params_n.local_if_name            = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_IF_NAME_IDX].strptr));
	RC.eNB[j][0]->eth_params_n.my_addr                  = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_ADDRESS_IDX].strptr));
	RC.eNB[j][0]->eth_params_n.remote_addr              = strdup(*(L1_ParamList.paramarray[j][L1_REMOTE_N_ADDRESS_IDX].strptr));
	RC.eNB[j][0]->eth_params_n.my_portc                 = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTC_IDX].iptr);
	RC.eNB[j][0]->eth_params_n.remote_portc             = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTC_IDX].iptr);
	RC.eNB[j][0]->eth_params_n.my_portd                 = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTD_IDX].iptr);
	RC.eNB[j][0]->eth_params_n.remote_portd             = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTD_IDX].iptr);
	RC.eNB[j][0]->eth_params_n.transp_preference        = ETH_UDP_MODE;
      }
      
      else { // other midhaul
      }	
    }// j=0..num_inst
    printf("Initializing northbound interface for L1\n");
    l1_north_init_eNB();
  } else {
    LOG_I(PHY,"No " CONFIG_STRING_L1_LIST " configuration found");    
  }
}

void RCconfig_macrlc() {
  int               j;


  paramdef_t MacRLC_Params[] = MACRLCPARAMS_DESC;
  paramlist_def_t MacRLC_ParamList = {CONFIG_STRING_MACRLC_LIST,NULL,0};

  config_getlist( &MacRLC_ParamList,MacRLC_Params,sizeof(MacRLC_Params)/sizeof(paramdef_t), NULL);    
  

  if ( MacRLC_ParamList.numelt > 0) {

      RC.nb_macrlc_inst=MacRLC_ParamList.numelt;
      mac_top_init_eNB();
      for (j=0;j<RC.nb_macrlc_inst;j++) {

          if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_RRC") == 0) {
              // check number of instances is same as RRC/PDCP

          } else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "cudu") == 0) {
              RC.mac[j]->eth_params_n.local_if_name            = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_IF_NAME_IDX].strptr));
              RC.mac[j]->eth_params_n.my_addr                  = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_ADDRESS_IDX].strptr));
              RC.mac[j]->eth_params_n.remote_addr              = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_ADDRESS_IDX].strptr));
              RC.mac[j]->eth_params_n.my_portc                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTC_IDX].iptr);
              RC.mac[j]->eth_params_n.remote_portc             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTC_IDX].iptr);
              RC.mac[j]->eth_params_n.my_portd                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTD_IDX].iptr);
              RC.mac[j]->eth_params_n.remote_portd             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTD_IDX].iptr);;
              RC.mac[j]->eth_params_n.transp_preference        = ETH_UDP_MODE;
          } else { // other midhaul
              AssertFatal(1==0,"MACRLC %d: %s unknown northbound midhaul\n",j, *(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr));
          }

          if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr), "local_L1") == 0) {


          } else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr), "nfapi") == 0) {
              RC.mac[j]->eth_params_s.local_if_name            = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_IF_NAME_IDX].strptr));
              RC.mac[j]->eth_params_s.my_addr                  = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_ADDRESS_IDX].strptr));
              RC.mac[j]->eth_params_s.remote_addr              = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_ADDRESS_IDX].strptr));
              RC.mac[j]->eth_params_s.my_portc                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_PORTC_IDX].iptr);
              RC.mac[j]->eth_params_s.remote_portc             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTC_IDX].iptr);
              RC.mac[j]->eth_params_s.my_portd                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_PORTD_IDX].iptr);
              RC.mac[j]->eth_params_s.remote_portd             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTD_IDX].iptr);
              RC.mac[j]->eth_params_s.transp_preference        = ETH_UDP_MODE;
          } else { // other midhaul
              AssertFatal(1==0,"MACRLC %d: %s unknown southbound midhaul\n",j,*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr));
          }
      }// j=0..num_inst
  } else {// MacRLC_ParamList.numelt > 0
      AssertFatal (0,
                   "No " CONFIG_STRING_MACRLC_LIST " configuration found");
  }
}

int RCconfig_RRC(MessageDef *msg_p, uint32_t i, eNB_RRC_INST *rrc)
{
    int               num_enbs                      = 0;
    int               num_component_carriers        = 0;
    int               k                           = 0;
    int32_t           enb_id                        = 0;
    int               nb_cc                         = 0;
    int               parse_errors                  = 0;
    int               num_mme_address               = 0;
    int               num_otg_elements              = 0;

    int               j                             = 0;
    int               sched_info_idx                = 0;
    int               sys_info_idx                  = 0;


    char*             if_name_s                       = NULL;
    char*             ipv4_s                          = NULL;
    char*             ipv4_s_remote                   = NULL;

    char*             tr_s_preference                 = NULL;
    int     local_s_portc                   = 0;
    int     remote_s_portc                  = 0;
    int     local_s_portd                   = 0;
    int     remote_s_portd                  = 0;

    const char*       cell_type                     = NULL;
    const char*       tac                           = 0;
    const char*       enb_name                      = NULL;
    const char*       mcc                           = 0;
    const char*       mnc                           = 0;
    char*             frame_type                    = NULL;
    int               tdd_config                    = 0;
    int               tdd_config_s                  = 0;
    const char*       prefix_type                   = NULL;
    const char*       pbch_repetition               = NULL;
    int               eutra_band                    = 0;
    long long int     downlink_frequency            = 0;
    int               uplink_frequency_offset       = 0;
    int               Nid_cell                      = 0;
    int               Nid_cell_mbsfn                = 0;
    int               N_RB_DL                       = 0;
    int               nb_antenna_ports              = 0;
    int               nb_antennas_tx                = 0;
    int               nb_antennas_rx                = 0;
    int               tx_gain                       = 0;
    int               rx_gain                       = 0;
    int               prach_root                    = 0;
    int               prach_config_index            = 0;
    const char*       prach_high_speed              = NULL;
    int               prach_zero_correlation        = 0;
    int               prach_freq_offset             = 0;
    int               pucch_delta_shift             = 0;
    int               pucch_nRB_CQI                 = 0;
    int               pucch_nCS_AN                  = 0;
    int               pucch_n1_AN                   = 0;

    int               pdsch_referenceSignalPower    = 0;
    int               pdsch_p_b                     = 0;
    int               pusch_n_SB                    = 0;
    const char *      pusch_hoppingMode             = NULL;
    int               pusch_hoppingOffset           = 0;
    const char*       pusch_enable64QAM             = NULL;
    const char*       pusch_groupHoppingEnabled     = NULL;
    int               pusch_groupAssignment         = 0;
    const char*       pusch_sequenceHoppingEnabled  = NULL;
    int               pusch_nDMRS1                  = 0;
    const char*       phich_duration                = NULL;
    const char*       phich_resource                = NULL;
    const char*       srs_enable                    = NULL;
    int               srs_BandwidthConfig           = 0;
    int               srs_SubframeConfig            = 0;
    const char*       srs_ackNackST                 = NULL;
    const char*       srs_MaxUpPts                  = NULL;
    int               pusch_p0_Nominal              = 0;
    const char*       pusch_alpha                   = NULL;
    int               pucch_p0_Nominal              = 0;
    int               msg3_delta_Preamble           = 0;
    //int     ul_CyclicPrefixLength         = 0;
    const char*       pucch_deltaF_Format1          = NULL;
    //const char*     pucch_deltaF_Format1a         = NULL;
    const char*       pucch_deltaF_Format1b         = NULL;
    const char*       pucch_deltaF_Format2          = NULL;
    const char*       pucch_deltaF_Format2a         = NULL;
    const char*       pucch_deltaF_Format2b         = NULL;
    const char*       rach_numberOfRA_Preambles     = NULL;
    const char*       rach_preamblesGroupAConfig    = NULL;
    int               rach_sizeOfRA_PreamblesGroupA = 0;
    int               rach_messageSizeGroupA        = 0;
    const char*       rach_messagePowerOffsetGroupB = NULL;
    int               rach_powerRampingStep         = 0;
    int               rach_preambleInitialReceivedTargetPower    = 0;
    int               rach_preambleTransMax         = 0;
    int               rach_raResponseWindowSize     = 0;
    int               rach_macContentionResolutionTimer = 0;
    int               rach_maxHARQ_Msg3Tx           = 0;
    const char*       pcch_defaultPagingCycle       = NULL;

    const char*       pcch_nB                       = NULL;
    int               bcch_modificationPeriodCoeff  = 0;

    const char*       ue_TimersAndConstants_t300    = NULL;
    const char*       ue_TimersAndConstants_t301    = NULL;
    const char*       ue_TimersAndConstants_t310    = NULL;
    const char*       ue_TimersAndConstants_t311    = NULL;
    const char*       ue_TimersAndConstants_n310    = NULL;
    const char*       ue_TimersAndConstants_n311    = NULL;
    const char*       ue_TransmissionMode           = NULL;

    int           si_Narrowband_r13             = 0;
    int           si_TBS_r13                    = 0;

    int           systemInfoValueTagSi_r13      = 0;

    int           firstPreamble_r13                     = 0;
    int           lastPreamble_r13                      = 0;
    const char*   ra_ResponseWindowSize_r13             = NULL;
    const char*   mac_ContentionResolutionTimer_r13     = NULL;
    const char*   rar_HoppingConfig_r13                 = NULL;
    int           rsrp_range_br                         = 0;
    int           prach_config_index_br                 = 0;
    int           prach_freq_offset_br                  = 0;
    int           prach_StartingSubframe_r13            = 0;
    const char*   maxNumPreambleAttemptCE_r13           = NULL;
    const char*   numRepetitionPerPreambleAttempt_r13   = NULL;
    const char*   mpdcch_NumRepetition_RA_r13           = NULL;
    const char*   prach_HoppingConfig_r13               = NULL;
    int           *maxavailablenarrowband               = NULL;
    int           pucch_info_value                      = 0;

    int           paging_narrowbands_r13                = 0;
    const char*   mpdcch_numrepetition_paging_r13       = NULL;
    const char*   nb_v1310                              = NULL;


    const char*   pucch_NumRepetitionCE_Msg4_Level0_r13 = NULL;
    const char*   pucch_NumRepetitionCE_Msg4_Level1_r13 = NULL;
    const char*   pucch_NumRepetitionCE_Msg4_Level2_r13 = NULL;
    const char*   pucch_NumRepetitionCE_Msg4_Level3_r13 = NULL;

    const char*   sib2_mpdcch_pdsch_hoppingNB_r13                   = NULL;
    const char*   sib2_interval_DLHoppingConfigCommonModeA_r13      = NULL;
    const char*   sib2_interval_DLHoppingConfigCommonModeA_r13_val  = NULL;
    const char*   sib2_interval_DLHoppingConfigCommonModeB_r13      = NULL;
    const char*   sib2_interval_DLHoppingConfigCommonModeB_r13_val  = NULL;

    const char*   sib2_interval_ULHoppingConfigCommonModeA_r13      = NULL;
    const char*   sib2_interval_ULHoppingConfigCommonModeA_r13_val  = NULL;
    const char*   sib2_interval_ULHoppingConfigCommonModeB_r13      = NULL;
    const char*   sib2_interval_ULHoppingConfigCommonModeB_r13_val  = NULL;
    int           sib2_mpdcch_pdsch_hoppingOffset_r13               = 0;

    const char*   pdsch_maxNumRepetitionCEmodeA_r13                 = NULL;
    const char*   pdsch_maxNumRepetitionCEmodeB_r13                 = NULL;

    const char*   pusch_maxNumRepetitionCEmodeA_r13                 = 0;
    const char*   pusch_maxNumRepetitionCEmodeB_r13                 = 0;
    int           pusch_HoppingOffset_v1310                         = 0;

    int           hyperSFN_r13                                      = 0;
    int           eDRX_Allowed_r13                                  = 0;
    int           q_RxLevMinCE_r13                                  = 0;
    int           q_QualMinRSRQ_CE_r13                              = 0;
    const char*   si_WindowLength_BR_r13                            = NULL;
    const char*   si_RepetitionPattern_r13                          = NULL;
    int           startSymbolBR_r13                                 = 0;
    const char*   si_HoppingConfigCommon_r13                        = NULL;
    const char*   si_ValidityTime_r13                               = NULL;
    const char*   mpdcch_pdsch_HoppingNB_r13                        = NULL;
    int           interval_DLHoppingConfigCommonModeA_r13_val       = 0;
    int           interval_DLHoppingConfigCommonModeB_r13_val       = 0;
    int           mpdcch_pdsch_HoppingOffset_r13                    = 0;
    const char*   preambleTransMax_CE_r13                           = NULL;
    const char*   rach_numberOfRA_Preambles_br                      = NULL;

    int           prach_HoppingOffset_r13                           = 0;
    int           schedulingInfoSIB1_BR_r13                         = 0;
    uint64_t      fdd_DownlinkOrTddSubframeBitmapBR_val_r13         = 0;

    char* cellSelectionInfoCE_r13                                       = NULL;
    char* bandwidthReducedAccessRelatedInfo_r13                         = NULL;
    char* fdd_DownlinkOrTddSubframeBitmapBR_r13                         = NULL;
    char* fdd_UplinkSubframeBitmapBR_r13                                = NULL;
    char* freqHoppingParametersDL_r13                                   = NULL;
    char* interval_DLHoppingConfigCommonModeA_r13                       = NULL;
    char* interval_DLHoppingConfigCommonModeB_r13                       = NULL;
    const char* prach_ConfigCommon_v1310                                = NULL;
    const char* mpdcch_startSF_CSS_RA_r13                               = NULL;
    const char* mpdcch_startSF_CSS_RA_r13_val                           = NULL;

    int     srb1_timer_poll_retransmit    = 0;
    int     srb1_timer_reordering         = 0;
    int     srb1_timer_status_prohibit    = 0;
    int     srb1_poll_pdu                 = 0;
    int     srb1_poll_byte                = 0;
    int     srb1_max_retx_threshold       = 0;

    int     my_int;


    const char*       active_enb[MAX_ENB];
    char*             enb_interface_name_for_S1U    = NULL;
    char*             enb_ipv4_address_for_S1U      = NULL;
    int                enb_port_for_S1U              = 0;
    char*             enb_interface_name_for_S1_MME = NULL;
    char*             enb_ipv4_address_for_S1_MME   = NULL;
    char             *address                       = NULL;
    char             *cidr                          = NULL;
    char             *astring                       = NULL;
    char*             flexran_agent_interface_name  = NULL;
    char*             flexran_agent_ipv4_address    = NULL;
    int               flexran_agent_port            = 0;
    char*             flexran_agent_cache           = NULL;
    int               otg_ue_id                     = 0;
    char*             otg_app_type                  = NULL;
    char*             otg_bg_traffic                = NULL;
    char*             glog_level                    = NULL;
    char*             glog_verbosity                = NULL;
    char*             hw_log_level                  = NULL;
    char*             hw_log_verbosity              = NULL;
    char*             phy_log_level                 = NULL;
    char*             phy_log_verbosity             = NULL;
    char*             mac_log_level                 = NULL;
    char*             mac_log_verbosity             = NULL;
    char*             rlc_log_level                 = NULL;
    char*             rlc_log_verbosity             = NULL;
    char*             pdcp_log_level                = NULL;
    char*             pdcp_log_verbosity            = NULL;
    char*             rrc_log_level                 = NULL;
    char*             rrc_log_verbosity             = NULL;
    char*             udp_log_verbosity             = NULL;
    char*             osa_log_level                 = NULL;
    char*             osa_log_verbosity             = NULL;

    printf("[KOGO][TESTING] Start of RCConfig_RCC");

    // for no gcc warnings
    (void)my_int;
    paramdef_t ENBSParams[]            = ENBSPARAMS_DESC;

    paramdef_t ENBParams[]             = ENBPARAMS_DESC;
    paramlist_def_t ENBParamList       = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};

    paramdef_t CCsParams[]             = CCPARAMS_DESC;
    paramlist_def_t CCsParamList       = {ENB_CONFIG_STRING_COMPONENT_CARRIERS,NULL,0};

    paramdef_t brParams[]              = BRPARAMS_DESC;


    paramdef_t schedulingInfoBrParams[] = SI_INFO_BR_DESC;
    paramlist_def_t schedulingInfoBrParamList = {ENB_CONFIG_STRING_SCHEDULING_INFO_BR, NULL, 0};


    paramdef_t rachcelevelParams[]     = RACH_CE_LEVELINFOLIST_R13_DESC;
    paramlist_def_t rachcelevellist    = {ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13, NULL, 0};

    paramdef_t rsrprangeParams[]       = RSRP_RANGE_LIST_DESC;
    paramlist_def_t rsrprangelist      = {ENB_CONFIG_STRING_RSRP_RANGE_LIST, NULL, 0};


    paramdef_t prachParams[]           = PRACH_PARAMS_CE_R13_DESC;
    paramlist_def_t prachParamslist    = {ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13, NULL, 0};

    paramdef_t n1PUCCH_ANR13Params[]   = N1PUCCH_AN_INFOLIST_R13_DESC;
    paramlist_def_t n1PUCCHInfoList    = {ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13, NULL, 0};

    paramdef_t pcchv1310Params[]       = PCCH_CONFIG_V1310_DESC;

    paramdef_t sib2freqhoppingParams[] = SIB2_FREQ_HOPPING_R13_DESC;

    paramdef_t SRB1Params[]            = SRB1PARAMS_DESC;




    /* get global parameters, defined outside any section in the config file */

    config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
    num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
    AssertFatal (i<num_enbs,
                 "Failed to parse config file no %ith element in %s \n",i, ENB_CONFIG_STRING_ACTIVE_ENBS);

#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)


    if (strcasecmp( *(ENBSParams[ENB_ASN1_VERBOSITY_IDX].strptr), ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0) {
        asn_debug      = 0;
        asn1_xer_print = 0;
    }else if (strcasecmp( *(ENBSParams[ENB_ASN1_VERBOSITY_IDX].strptr), ENB_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0) {
        asn_debug      = 1;
        asn1_xer_print = 1;
    } else if (strcasecmp(*(ENBSParams[ENB_ASN1_VERBOSITY_IDX].strptr) , ENB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0) {
        asn_debug      = 1;
        asn1_xer_print = 2;
    } else {
        asn_debug      = 0;
        asn1_xer_print = 0;
    }


#endif



    if (num_enbs>0)
    {
        // Output a list of all eNBs.
        config_getlist( &ENBParamList,ENBParams,sizeof(ENBParams)/sizeof(paramdef_t),NULL);




        if (ENBParamList.paramarray[i][ENB_ENB_ID_IDX].uptr == NULL)
        {
            // Calculate a default eNB ID
# if defined(ENABLE_USE_MME)
            uint32_t hash;

            hash = s1ap_generate_eNB_id ();
            enb_id = i + (hash & 0xFFFF8);
# else
            enb_id = i;
# endif
        }
        else {
            enb_id = *(ENBParamList.paramarray[i][ENB_ENB_ID_IDX].uptr);
        }


        printf("RRC %d: Southbound Transport %s\n",i,*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr));

        if (strcmp(*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr), "local_mac") == 0) {


        }
        else if (strcmp(*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr), "cudu") == 0)
        {
            rrc->eth_params_s.local_if_name            = strdup(*(ENBParamList.paramarray[i][ENB_LOCAL_S_IF_NAME_IDX].strptr));
            rrc->eth_params_s.my_addr                  = strdup(*(ENBParamList.paramarray[i][ENB_LOCAL_S_ADDRESS_IDX].strptr));
            rrc->eth_params_s.remote_addr              = strdup(*(ENBParamList.paramarray[i][ENB_REMOTE_S_ADDRESS_IDX].strptr));
            rrc->eth_params_s.my_portc                 = *(ENBParamList.paramarray[i][ENB_LOCAL_S_PORTC_IDX].uptr);
            rrc->eth_params_s.remote_portc             = *(ENBParamList.paramarray[i][ENB_REMOTE_S_PORTC_IDX].uptr);
            rrc->eth_params_s.my_portd                 = *(ENBParamList.paramarray[i][ENB_LOCAL_S_PORTD_IDX].uptr);
            rrc->eth_params_s.remote_portd             = *(ENBParamList.paramarray[i][ENB_REMOTE_S_PORTD_IDX].uptr);
            rrc->eth_params_s.transp_preference        = ETH_UDP_MODE;
        }

        else { // other midhaul
        }

        // search if in active list






        for (k=0; k < num_enbs ; k++)
        {
            if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[k], *(ENBParamList.paramarray[i][ENB_ENB_NAME_IDX].strptr) ) == 0)
            {
                char enbpath[MAX_OPTNAME_SIZE + 8];


                RRC_CONFIGURATION_REQ (msg_p).cell_identity = enb_id;

                /*
        if (strcmp(*(ENBParamList.paramarray[i][ENB_CELL_TYPE_IDX].strptr), "CELL_MACRO_ENB") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cell_type = CELL_MACRO_ENB;
        } else  if (strcmp(cell_type, "CELL_HOME_ENB") == 0) {
        enb_properties_loc.properties[enb_properties_loc_index]->cell_type = CELL_HOME_ENB;
        } else {
        AssertFatal (0,
        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
        lib_config_file_name_pP, i, cell_type);
        }

        enb_properties_loc.properties[enb_properties_loc_index]->eNB_name         = strdup(enb_name);
      */
                RRC_CONFIGURATION_REQ (msg_p).tac              = (uint16_t)atoi( *(ENBParamList.paramarray[i][ENB_TRACKING_AREA_CODE_IDX].strptr) );
                RRC_CONFIGURATION_REQ (msg_p).mcc              = (uint16_t)atoi( *(ENBParamList.paramarray[i][ENB_MOBILE_COUNTRY_CODE_IDX].strptr) );
                RRC_CONFIGURATION_REQ (msg_p).mnc              = (uint16_t)atoi( *(ENBParamList.paramarray[i][ENB_MOBILE_NETWORK_CODE_IDX].strptr) );
                RRC_CONFIGURATION_REQ (msg_p).mnc_digit_length = strlen(*(ENBParamList.paramarray[i][ENB_MOBILE_NETWORK_CODE_IDX].strptr));
                AssertFatal((RRC_CONFIGURATION_REQ (msg_p).mnc_digit_length == 2) ||
                            (RRC_CONFIGURATION_REQ (msg_p).mnc_digit_length == 3),
                            "BAD MNC DIGIT LENGTH %d",
                            RRC_CONFIGURATION_REQ (msg_p).mnc_digit_length);


                // Parse optional physical parameters
                sprintf(enbpath,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
                config_getlist(&CCsParamList, NULL, 0, enbpath);

                printf("[KOGO][TESTING]: CC List count: %d \n", CCsParamList.numelt);
                LOG_I(RRC,"num component carriers %d \n", num_component_carriers);
                if ( CCsParamList.numelt> 0) {

                    char ccspath[MAX_OPTNAME_SIZE*2 + 16];





                    //enb_properties_loc.properties[enb_properties_loc_index]->nb_cc = num_component_carriers;


                    for (j = 0; j < CCsParamList.numelt ;j++) {

                        sprintf(ccspath,"%s.%s.[%i]", enbpath, ENB_CONFIG_STRING_COMPONENT_CARRIERS, j);
                        config_get( CCsParams,sizeof(CCsParams)/sizeof(paramdef_t),ccspath);


                        //printf("Component carrier %d\n",component_carrier);

                        printf("[KOGO][TESTING]: tdd_config: %d \n",tdd_config);

                        nb_cc++;
                        RRC_CONFIGURATION_REQ (msg_p).tdd_config[j] = tdd_config;

                        AssertFatal (tdd_config <= TDD_Config__subframeAssignment_sa6,
                                     "Failed to parse eNB configuration file %s, enb %d illegal tdd_config %d (should be 0-%d)!",
                                     RC.config_file_name, i, tdd_config, TDD_Config__subframeAssignment_sa6);

                        RRC_CONFIGURATION_REQ (msg_p).tdd_config_s[j] = tdd_config_s;
                        AssertFatal (tdd_config_s <= TDD_Config__specialSubframePatterns_ssp8,
                                     "Failed to parse eNB configuration file %s, enb %d illegal tdd_config_s %d (should be 0-%d)!",
                                     RC.config_file_name, i, tdd_config_s, TDD_Config__specialSubframePatterns_ssp8);

                        if (!prefix_type)
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d define %s: NORMAL,EXTENDED!\n",
                                         RC.config_file_name, i, ENB_CONFIG_STRING_PREFIX_TYPE);
                        else if (strcmp(prefix_type, "NORMAL") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).prefix_type[j] = NORMAL;
                        }
                        else  if (strcmp(prefix_type, "EXTENDED") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).prefix_type[j] = EXTENDED;
                        }
                        else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
                                        RC.config_file_name, i, prefix_type);
                        }
#ifdef Rel14
                        if (!pbch_repetition)
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d define %s: TRUE,FALSE!\n",
                                        RC.config_file_name, i, ENB_CONFIG_STRING_PBCH_REPETITION);
                        else if (strcmp(pbch_repetition, "TRUE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).pbch_repetition[j] = 1;
                        }
                        else  if (strcmp(pbch_repetition, "FALSE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).pbch_repetition[j] = 0;
                        }
                        else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pbch_repetition choice: TRUE or FALSE !\n",
                                        RC.config_file_name, i, pbch_repetition);
                        }
#endif

                        RRC_CONFIGURATION_REQ (msg_p).eutra_band[j] = eutra_band;
                        RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j] = (uint32_t) downlink_frequency;
                        RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j] = (unsigned int) uplink_frequency_offset;
                        RRC_CONFIGURATION_REQ (msg_p).Nid_cell[j]= Nid_cell;

                        if (Nid_cell>503) {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for Nid_cell choice: 0...503 !\n",
                                         RC.config_file_name, i, Nid_cell);
                        }

                        RRC_CONFIGURATION_REQ (msg_p).N_RB_DL[j]= N_RB_DL;

                        if ((N_RB_DL!=6) && (N_RB_DL!=15) && (N_RB_DL!=25) && (N_RB_DL!=50) && (N_RB_DL!=75) && (N_RB_DL!=100)) {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 6,15,25,50,75,100 !\n",
                                         RC.config_file_name, i, N_RB_DL);
                        }

                        printf("[KOGO][DBUGGING] frame type = %s \n", frame_type);

                        if (strcmp(frame_type, "FDD") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).frame_type[j] = FDD;
                        }
                        else  if (strcmp(frame_type, "TDD") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).frame_type[j] = TDD;
                        }
                        else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for frame_type choice: FDD or TDD !\n",
                                        RC.config_file_name, i, frame_type);
                        }


                        RRC_CONFIGURATION_REQ (msg_p).tdd_config[j] = tdd_config;
                        AssertFatal (tdd_config <= TDD_Config__subframeAssignment_sa6,
                                     "Failed to parse eNB configuration file %s, enb %d illegal tdd_config %d (should be 0-%d)!",
                                     RC.config_file_name, i, tdd_config, TDD_Config__subframeAssignment_sa6);


                        RRC_CONFIGURATION_REQ (msg_p).tdd_config_s[j] = tdd_config_s;
                        AssertFatal (tdd_config_s <= TDD_Config__specialSubframePatterns_ssp8,
                                     "Failed to parse eNB configuration file %s, enb %d illegal tdd_config_s %d (should be 0-%d)!",
                                     RC.config_file_name, i, tdd_config_s, TDD_Config__specialSubframePatterns_ssp8);



                        if (!prefix_type)
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d define %s: NORMAL,EXTENDED!\n",
                                         RC.config_file_name, i, ENB_CONFIG_STRING_PREFIX_TYPE);
                        else if (strcmp(prefix_type, "NORMAL") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).prefix_type[j] = NORMAL;
                        }
                        else  if (strcmp(prefix_type, "EXTENDED") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).prefix_type[j] = EXTENDED;
                        }
                        else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
                                        RC.config_file_name, i, prefix_type);
                        }



                        RRC_CONFIGURATION_REQ (msg_p).eutra_band[j] = eutra_band;
                        // printf( "\teutra band:\t%d\n",RRC_CONFIGURATION_REQ (msg_p).eutra_band);



                        RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j] = (uint32_t) downlink_frequency;
                        //printf( "\tdownlink freq:\t%u\n",RRC_CONFIGURATION_REQ (msg_p).downlink_frequency);


                        RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j] = (unsigned int) uplink_frequency_offset;

                        if (enb_check_band_frequencies(RC.config_file_name,
                                                       j,
                                                       RRC_CONFIGURATION_REQ (msg_p).eutra_band[j],
                                                       RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j],
                                                       RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j],
                                                       RRC_CONFIGURATION_REQ (msg_p).frame_type[j])) {
                            AssertFatal(0, "error calling enb_check_band_frequencies\n");
                        }

                        if ((nb_antenna_ports <1) || (nb_antenna_ports > 2))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for nb_antenna_ports choice: 1..2 !\n",
                                         RC.config_file_name, i, nb_antenna_ports);

                        RRC_CONFIGURATION_REQ (msg_p).nb_antenna_ports[j] = nb_antenna_ports;


                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_root =  prach_root;

                        if ((prach_root <0) || (prach_root > 1023))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_root choice: 0..1023 !\n",
                                         RC.config_file_name, i, prach_root);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_config_index = prach_config_index;

                        if ((prach_config_index <0) || (prach_config_index > 63))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
                                         RC.config_file_name, i, prach_config_index);

                        if (!prach_high_speed)
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                                         RC.config_file_name, i, ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
                        else if (strcmp(prach_high_speed, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].prach_high_speed = TRUE;
                        }
                        else if (strcmp(prach_high_speed, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].prach_high_speed = FALSE;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
                                        RC.config_file_name, i, prach_high_speed);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_zero_correlation =prach_zero_correlation;

                        if ((prach_zero_correlation <0) || (prach_zero_correlation > 15))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
                                         RC.config_file_name, i, prach_zero_correlation);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_freq_offset = prach_freq_offset;

                        if ((prach_freq_offset <0) || (prach_freq_offset > 94))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
                                         RC.config_file_name, i, prach_freq_offset);


                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_delta_shift = pucch_delta_shift-1;

                        if ((pucch_delta_shift <1) || (pucch_delta_shift > 3))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
                                         RC.config_file_name, i, pucch_delta_shift);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_nRB_CQI = pucch_nRB_CQI;

                        if ((pucch_nRB_CQI <0) || (pucch_nRB_CQI > 98))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
                                         RC.config_file_name, i, pucch_nRB_CQI);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_nCS_AN = pucch_nCS_AN;

                        if ((pucch_nCS_AN <0) || (pucch_nCS_AN > 7))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
                                         RC.config_file_name, i, pucch_nCS_AN);

#if !defined(Rel10) && !defined(Rel14)
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_n1_AN = pucch_n1_AN;

                        if ((pucch_n1_AN <0) || (pucch_n1_AN > 2047))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_n1_AN choice: 0..2047!\n",
                                         RC.config_file_name, i, pucch_n1_AN);

#endif
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pdsch_referenceSignalPower = pdsch_referenceSignalPower;

                        if ((pdsch_referenceSignalPower <-60) || (pdsch_referenceSignalPower > 50))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
                                         RC.config_file_name, i, pdsch_referenceSignalPower);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pdsch_p_b = pdsch_p_b;

                        if ((pdsch_p_b <0) || (pdsch_p_b > 3))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
                                         RC.config_file_name, i, pdsch_p_b);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_n_SB = pusch_n_SB;

                        if ((pusch_n_SB <1) || (pusch_n_SB > 4))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
                                         RC.config_file_name, i, pusch_n_SB);

                        if (!pusch_hoppingMode)
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d define %s: interSubframe,intraAndInterSubframe!\n",
                                         RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);
                        else if (strcmp(pusch_hoppingMode, "interSubFrame") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
                        }
                        else if (strcmp(pusch_hoppingMode, "intraAndInterSubFrame") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
                                        RC.config_file_name, i, pusch_hoppingMode);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingOffset = pusch_hoppingOffset;

                        if ((pusch_hoppingOffset<0) || (pusch_hoppingOffset>98))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
                                         RC.config_file_name, i, pusch_hoppingMode);

                        if (!pusch_enable64QAM)
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                                         RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
                        else if (strcmp(pusch_enable64QAM, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_enable64QAM = TRUE;
                        }
                        else if (strcmp(pusch_enable64QAM, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_enable64QAM = FALSE;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
                                        RC.config_file_name, i, pusch_enable64QAM);

                        if (!pusch_groupHoppingEnabled)
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                                        RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
                        else if (strcmp(pusch_groupHoppingEnabled, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_groupHoppingEnabled = TRUE;
                        }
                        else if (strcmp(pusch_groupHoppingEnabled, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_groupHoppingEnabled = FALSE;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
                                        RC.config_file_name, i, pusch_groupHoppingEnabled);


                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupAssignment = pusch_groupAssignment;

                        if ((pusch_groupAssignment<0)||(pusch_groupAssignment>29))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
                                         RC.config_file_name, i, pusch_groupAssignment);

                        if (!pusch_sequenceHoppingEnabled)
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                                         RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
                        else if (strcmp(pusch_sequenceHoppingEnabled, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_sequenceHoppingEnabled = TRUE;
                        }
                        else if (strcmp(pusch_sequenceHoppingEnabled, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_sequenceHoppingEnabled = FALSE;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
                                        RC.config_file_name, i, pusch_sequenceHoppingEnabled);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_nDMRS1 = pusch_nDMRS1;  //cyclic_shift in RRC!

                        if ((pusch_nDMRS1 <0) || (pusch_nDMRS1>7))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
                                         RC.config_file_name, i, pusch_nDMRS1);

                        if (strcmp(phich_duration, "NORMAL") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_duration = PHICH_Config__phich_Duration_normal;
                        }
                        else if (strcmp(phich_duration, "EXTENDED") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_duration = PHICH_Config__phich_Duration_extended;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
                                        RC.config_file_name, i, phich_duration);

                        if (strcmp(phich_resource, "ONESIXTH") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_resource = PHICH_Config__phich_Resource_oneSixth;
                        }
                        else if (strcmp(phich_resource, "HALF") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_resource = PHICH_Config__phich_Resource_half;
                        }
                        else if (strcmp(phich_resource, "ONE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_resource = PHICH_Config__phich_Resource_one;
                        }
                        else if (strcmp(phich_resource, "TWO") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_resource = PHICH_Config__phich_Resource_two;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
                                        RC.config_file_name, i, phich_resource);

                        printf("phich.resource %d (%s), phich.duration %d (%s)\n",
                               (int)RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_resource, phich_resource,
                               (int)RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].phich_duration, phich_duration);

                        if (strcmp(srs_enable, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_enable = TRUE;
                        }
                        else if (strcmp(srs_enable, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_enable = FALSE;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                                        RC.config_file_name, i, srs_enable);

                        if (RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable == TRUE) {

                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_BandwidthConfig = srs_BandwidthConfig;

                            if ((srs_BandwidthConfig < 0) || (srs_BandwidthConfig >7))
                                AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value %d for srs_BandwidthConfig choice: 0...7\n",
                                             RC.config_file_name, i, srs_BandwidthConfig);

                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_SubframeConfig = srs_SubframeConfig;

                            if ((srs_SubframeConfig<0) || (srs_SubframeConfig>15))
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
                                             RC.config_file_name, i, srs_SubframeConfig);

                            if (strcmp(srs_ackNackST, "ENABLE") == 0) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_ackNackST = TRUE;
                            }
                            else if (strcmp(srs_ackNackST, "DISABLE") == 0) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_ackNackST = FALSE;
                            }
                            else
                                AssertFatal(0,
                                            "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                                            RC.config_file_name, i, srs_ackNackST);

                            if (strcmp(srs_MaxUpPts, "ENABLE") == 0) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_MaxUpPts = TRUE;
                            }
                            else if (strcmp(srs_MaxUpPts, "DISABLE") == 0) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].srs_MaxUpPts = FALSE;
                            }
                            else
                                AssertFatal(0,
                                            "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
                                            RC.config_file_name, i, srs_MaxUpPts);
                        }

                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_p0_Nominal = pusch_p0_Nominal;

                        if ((pusch_p0_Nominal < -126) || (pusch_p0_Nominal > 24))
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
                                        RC.config_file_name, i, pusch_p0_Nominal);

#ifndef Rel14
                        if (strcmp(pusch_alpha, "AL0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al0;
                        }
                        else if (strcmp(pusch_alpha, "AL04") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al04;
                        }
                        else if (strcmp(pusch_alpha, "AL05") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al05;
                        }
                        else if (strcmp(pusch_alpha, "AL06") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al06;
                        }
                        else if (strcmp(pusch_alpha, "AL07") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al07;
                        }
                        else if (strcmp(pusch_alpha, "AL08") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al08;
                        }
                        else if (strcmp(pusch_alpha, "AL09") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al09;
                        }
                        else if (strcmp(pusch_alpha, "AL1") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = UplinkPowerControlCommon__alpha_al1;
                        }
#else
                        if (strcmp(pusch_alpha, "AL0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al0;
                        }
                        else if (strcmp(pusch_alpha, "AL04") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al04;
                        }
                        else if (strcmp(pusch_alpha, "AL05") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al05;
                        }
                        else if (strcmp(pusch_alpha, "AL06") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al06;
                        }
                        else if (strcmp(pusch_alpha, "AL07") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al07;
                        }
                        else if (strcmp(pusch_alpha, "AL08") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al08;
                        }
                        else if (strcmp(pusch_alpha, "AL09") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al09;
                        }
                        else if (strcmp(pusch_alpha, "AL1") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pusch_alpha = Alpha_r12_al1;
                        }
#endif
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
                                        RC.config_file_name, i, pusch_alpha);

                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_p0_Nominal = pucch_p0_Nominal;

                        if ((pucch_p0_Nominal < -127) || (pucch_p0_Nominal > -96))
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
                                        RC.config_file_name, i, pucch_p0_Nominal);

                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].msg3_delta_Preamble = msg3_delta_Preamble;

                        if ((msg3_delta_Preamble < -1) || (msg3_delta_Preamble > 6))
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
                                        RC.config_file_name, i, msg3_delta_Preamble);


                        if (strcmp(pucch_deltaF_Format1, "deltaF_2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1 = DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
                        }
                        else if (strcmp(pucch_deltaF_Format1, "deltaF0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
                        }
                        else if (strcmp(pucch_deltaF_Format1, "deltaF2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
                                        RC.config_file_name, i, pucch_deltaF_Format1);

                        if (strcmp(pucch_deltaF_Format1b, "deltaF1") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
                        }
                        else if (strcmp(pucch_deltaF_Format1b, "deltaF3") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
                        }
                        else if (strcmp(pucch_deltaF_Format1b, "deltaF5") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
                                        RC.config_file_name, i, pucch_deltaF_Format1b);


                        if (strcmp(pucch_deltaF_Format2, "deltaF_2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
                        }
                        else if (strcmp(pucch_deltaF_Format2, "deltaF0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
                        }
                        else if (strcmp(pucch_deltaF_Format2, "deltaF1") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
                        }
                        else if (strcmp(pucch_deltaF_Format2, "deltaF2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
                                        RC.config_file_name, i, pucch_deltaF_Format2);

                        if (strcmp(pucch_deltaF_Format2a, "deltaF_2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
                        }
                        else if (strcmp(pucch_deltaF_Format2a, "deltaF0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
                        }
                        else if (strcmp(pucch_deltaF_Format2a, "deltaF2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
                                        RC.config_file_name, i, pucch_deltaF_Format2a);

                        if (strcmp(pucch_deltaF_Format2b, "deltaF_2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
                        }
                        else if (strcmp(pucch_deltaF_Format2b, "deltaF0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
                        }
                        else if (strcmp(pucch_deltaF_Format2b, "deltaF2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
                                        RC.config_file_name, i, pucch_deltaF_Format2b);


                        //              27   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n4  = 0,
                        //              28   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n8  = 1,
                        //              29   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n12 = 2,
                        //              30   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n16 = 3,
                        //              31   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n20 = 4,
                        //              32   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n24 = 5,
                        //              33   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n28 = 6,
                        //              34   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n32 = 7,
                        //              35   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n36 = 8,
                        //              36   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n40 = 9,
                        //              37   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n44 = 10,
                        //              38   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n48 = 11,
                        //              39   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n52 = 12,
                        //              40   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n56 = 13,
                        //              41   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n60 = 14,
                        //              42   RACH_ConfigCommon__preambleInfo__numberOfRA_Preambles_n64 = 15


                        if (!strcmp(rach_numberOfRA_Preambles, "n4")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (4 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n8")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (8 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n12")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (12 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n16")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (16 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n20")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (20 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n24")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (24 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n28")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (28 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n32")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (32 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n36")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (36 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n40")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (40 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n44")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (44 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n48")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (48 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n52")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (52 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n56")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (56 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n60")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (60 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n64")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles = (64 / 4) - 1;
                        } else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
                                        RC.config_file_name, i, rach_numberOfRA_Preambles);

                        }

                        printf("[DEBUGGIN][KOGO][TESTING]: number of RA Preambles: %s - %d\n",
                               rach_numberOfRA_Preambles,  RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles);

                        if (strcmp(rach_preamblesGroupAConfig, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_preamblesGroupAConfig= TRUE;



                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_sizeOfRA_PreamblesGroupA= (rach_sizeOfRA_PreamblesGroupA / 4) - 1;

                            switch (rach_messageSizeGroupA) {
                            case 56:
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b56;
                                break;

                            case 144:
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b144;
                                break;

                            case 208:
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b208;
                                break;

                            case 256:
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b256;
                                break;

                            default:
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
                                             RC.config_file_name, i, rach_messageSizeGroupA);
                                break;
                            }

                            if (strcmp(rach_messagePowerOffsetGroupB,"minusinfinity")==0) {
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
                            }

                            else if (strcmp(rach_messagePowerOffsetGroupB,"dB0")==0) {
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
                            }

                            else if (strcmp(rach_messagePowerOffsetGroupB,"dB5")==0) {
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
                            }

                            else if (strcmp(rach_messagePowerOffsetGroupB,"dB8")==0) {
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
                            }

                            else if (strcmp(rach_messagePowerOffsetGroupB,"dB10")==0) {
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
                            }

                            else if (strcmp(rach_messagePowerOffsetGroupB,"dB12")==0) {
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
                            }

                            else if (strcmp(rach_messagePowerOffsetGroupB,"dB15")==0) {
                                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
                            }

                            else if (strcmp(rach_messagePowerOffsetGroupB,"dB18")==0) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
                            }
                            else
                                AssertFatal(0,
                                            "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
                                            RC.config_file_name, i, rach_messagePowerOffsetGroupB);

                        }
                        else if (strcmp(rach_preamblesGroupAConfig, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].rach_preamblesGroupAConfig= FALSE;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
                                        RC.config_file_name, i, rach_preamblesGroupAConfig);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleInitialReceivedTargetPower= (rach_preambleInitialReceivedTargetPower+120)/2;

                        if ((rach_preambleInitialReceivedTargetPower<-120) || (rach_preambleInitialReceivedTargetPower>-90) || ((rach_preambleInitialReceivedTargetPower&1)!=0))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
                                         RC.config_file_name, i, rach_preambleInitialReceivedTargetPower);


                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_powerRampingStep= rach_powerRampingStep/2;

                        if ((rach_powerRampingStep<0) || (rach_powerRampingStep>6) || ((rach_powerRampingStep&1)!=0))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
                                         RC.config_file_name, i, rach_powerRampingStep);



                        switch (rach_preambleTransMax) {
#ifndef Rel14
                        case 3:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n3;
                            break;

                        case 4:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n4;
                            break;

                        case 5:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n5;
                            break;

                        case 6:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n6;
                            break;

                        case 7:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n7;
                            break;

                        case 8:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n8;
                            break;

                        case 10:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10;
                            break;

                        case 20:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n20;
                            break;

                        case 50:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n50;
                            break;

                        case 100:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n100;
                            break;

                        case 200:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n200;
                            break;

#else

                        case 3:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n3;
                            break;

                        case 4:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n4;
                            break;

                        case 5:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n5;
                            break;

                        case 6:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n6;
                            break;

                        case 7:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n7;
                            break;

                        case 8:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n8;
                            break;

                        case 10:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n10;
                            break;

                        case 20:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n20;
                            break;

                        case 50:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n50;
                            break;

                        case 100:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n100;
                            break;

                        case 200:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n200;
                            break;
#endif

                        default:
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
                                         RC.config_file_name, i, rach_preambleTransMax);
                            break;
                        }

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_raResponseWindowSize=  (rach_raResponseWindowSize==10)?7:rach_raResponseWindowSize-2;

                        if ((rach_raResponseWindowSize<0)||(rach_raResponseWindowSize==9)||(rach_raResponseWindowSize>10))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
                                         RC.config_file_name, i, rach_preambleTransMax);


                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_macContentionResolutionTimer= (rach_macContentionResolutionTimer/8)-1;

                        if ((rach_macContentionResolutionTimer<8) || (rach_macContentionResolutionTimer>64) || ((rach_macContentionResolutionTimer&7)!=0))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
                                         RC.config_file_name, i, rach_preambleTransMax);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_maxHARQ_Msg3Tx= rach_maxHARQ_Msg3Tx;

                        if ((rach_maxHARQ_Msg3Tx<0) || (rach_maxHARQ_Msg3Tx>8))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
                                         RC.config_file_name, i, rach_preambleTransMax);


                        if (!strcmp(pcch_defaultPagingCycle, "rf32")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf32;
                        } else if (!strcmp(pcch_defaultPagingCycle, "rf64")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf64;
                        } else if (!strcmp(pcch_defaultPagingCycle, "rf128")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf128;
                        } else if (!strcmp(pcch_defaultPagingCycle, "rf256")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf256;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
                                         RC.config_file_name, i, pcch_defaultPagingCycle);
                        }

                        printf("[DEBUGGING][KOGO][CHAR*] : pcch_defaultPagingCycle_br = %d\n", RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle);

                        if (strcmp(pcch_nB, "fourT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_fourT;
                        }
                        else if (strcmp(pcch_nB, "twoT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_twoT;
                        }
                        else if (strcmp(pcch_nB, "oneT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_oneT;
                        }
                        else if (strcmp(pcch_nB, "halfT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_halfT;
                        }
                        else if (strcmp(pcch_nB, "quarterT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_quarterT;
                        }
                        else if (strcmp(pcch_nB, "oneEighthT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_oneEighthT;
                        }
                        else if (strcmp(pcch_nB, "oneSixteenthT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_oneSixteenthT;
                        }
                        else if (strcmp(pcch_nB, "oneThirtySecondT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig[j].pcch_nB = PCCH_Config__nB_oneThirtySecondT;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
                                        RC.config_file_name, i, pcch_defaultPagingCycle);



                        switch (bcch_modificationPeriodCoeff) {
                        case 2:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n2;
                            break;

                        case 4:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n4;
                            break;

                        case 8:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n8;
                            break;

                        case 16:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff= BCCH_Config__modificationPeriodCoeff_n16;
                            break;

                        default:
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
                                         RC.config_file_name, i, bcch_modificationPeriodCoeff);

                            break;
                        }

                        if (!strcmp(ue_TimersAndConstants_t300, "ms100")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms100;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms200")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms200;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms300")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms300;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms400")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms400;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms600")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms600;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms1000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms1000;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms1500")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms1500;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms2000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms2000;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t300 unknown value !!",
                                         RC.config_file_name, i, ue_TimersAndConstants_t300);
                        }

                        printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_t300 : %s - %d\n", ue_TimersAndConstants_t300, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300);

                        if (!strcmp(ue_TimersAndConstants_t301, "ms100")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms100;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms200")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms200;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms300")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms300;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms400")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms400;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms600")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms600;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms1000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms1000;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms1500")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms1500;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms2000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms2000;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t301 !!",
                                         RC.config_file_name, i, ue_TimersAndConstants_t301);
                        }

                        printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_t301 : %s - %d\n", ue_TimersAndConstants_t301, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301);

                        if (!strcmp(ue_TimersAndConstants_t310, "ms0")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms0;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms50")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms50;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms100")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms100;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms200")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms200;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms500")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms500;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms1000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms1000;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms2000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms2000;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t310 !!",
                                         RC.config_file_name, i, ue_TimersAndConstants_t310);
                        }

                        printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_t310 : %s - %d\n", ue_TimersAndConstants_t310, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310);


                        if (!strcmp(ue_TimersAndConstants_t311, "ms1000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms1000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms3000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms3000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms5000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms5000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms10000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms10000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms15000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms15000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms20000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms20000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms30000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms30000;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 !!",
                                         RC.config_file_name, i, ue_TimersAndConstants_t311);
                        }

                        printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_t311 : %s - %d\n", ue_TimersAndConstants_t311, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311);


                        if (!strcmp(ue_TimersAndConstants_n310, "n1")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n1;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n2")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n2;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n3")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n3;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n4")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n4;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n6")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n6;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n8")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n8;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n10")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n10;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n20")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n20;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_n310 !!",
                                         RC.config_file_name, i, ue_TimersAndConstants_n310);
                        }

                        printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_n310 : %s - %d\n", ue_TimersAndConstants_n310, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310);


                        if (!strcmp(ue_TimersAndConstants_n311, "n1")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n1;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n2")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n2;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n3")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n3;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n4")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n4;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n5")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n5;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n6")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n6;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n8")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n8;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n10")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n10;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_n311!!",
                                         RC.config_file_name, i, ue_TimersAndConstants_n311);
                        }

                        printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_n311 : %s - %d\n", ue_TimersAndConstants_n311, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311);


                        if (!strcmp(ue_TransmissionMode, "tm1")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm1;
                        } else if (!strcmp(ue_TransmissionMode, "tm2")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm2;
                        } else if (!strcmp(ue_TransmissionMode, "tm3")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm3;
                        } else if (!strcmp(ue_TransmissionMode, "tm4")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm4;
                        } else if (!strcmp(ue_TransmissionMode, "tm5")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm5;
                        } else if (!strcmp(ue_TransmissionMode, "tm6")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm6;
                        } else if (!strcmp(ue_TransmissionMode, "tm7")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm7;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode !!",
                                         RC.config_file_name, i, ue_TransmissionMode);
                        }

                        printf("[DEBUGGING][KOGO][CHAR*]: ue_TransmissionMode : %s - %d\n", ue_TransmissionMode, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode);



#ifdef Rel14
                        char brparamspath[MAX_OPTNAME_SIZE*2 + 16];
                        sprintf(brparamspath,"%s.%s", ccspath, ENB_CONFIG_STRING_COMPONENT_BR_PARAMETERS);
                        config_get( brParams, sizeof(brParams)/sizeof(paramdef_t), brparamspath);

                        int cnt_pucch_NumRepetitionCE = 0;


                        RRC_CONFIGURATION_REQ(msg_p).schedulingInfoSIB1_BR_r13[j] = schedulingInfoSIB1_BR_r13;


                        if (!strcmp(cellSelectionInfoCE_r13, "ENABLE")) {
                            RRC_CONFIGURATION_REQ(msg_p).cellSelectionInfoCE_r13[j] = TRUE;
                            RRC_CONFIGURATION_REQ(msg_p).q_RxLevMinCE_r13[j]= q_RxLevMinCE_r13;
//                            RRC_CONFIGURATION_REQ(msg_p).q_QualMinRSRQ_CE_r13[j]= calloc(1, sizeof(long));
//                            *RRC_CONFIGURATION_REQ(msg_p).q_QualMinRSRQ_CE_r13[j]= q_QualMinRSRQ_CE_r13;
                        } else {
                            RRC_CONFIGURATION_REQ(msg_p).cellSelectionInfoCE_r13[j] = FALSE;
                        }


                        printf("[DEBUGGING][KOGO]: bandwidthReducedAccessRelatedInfo_r13 = %s\n", bandwidthReducedAccessRelatedInfo_r13);

                        if (!strcmp(bandwidthReducedAccessRelatedInfo_r13, "ENABLE")) {
                            RRC_CONFIGURATION_REQ(msg_p).bandwidthReducedAccessRelatedInfo_r13[j] = TRUE;


                            printf("[DEBUGGING][KOGO][TANY]: bandwidthReducedAccessRelatedInfo_r13 = %s\n", bandwidthReducedAccessRelatedInfo_r13);

                            if (!strcmp(si_WindowLength_BR_r13, "ms20")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 0;
                            } else if (!strcmp(si_WindowLength_BR_r13, "ms40")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 1;
                            } else if (!strcmp(si_WindowLength_BR_r13, "ms60")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 2;
                            } else if (!strcmp(si_WindowLength_BR_r13, "ms80")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 3;
                            } else if (!strcmp(si_WindowLength_BR_r13, "ms120")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 4;
                            } else if (!strcmp(si_WindowLength_BR_r13, "ms160")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 5;
                            } else if (!strcmp(si_WindowLength_BR_r13, "ms200")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 6;
                            } else if (!strcmp(si_WindowLength_BR_r13, "spare")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 7;
                            }


                            if (!strcmp(si_RepetitionPattern_r13, "everyRF")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[j] = 0;
                            } else if (!strcmp(si_RepetitionPattern_r13, "every2ndRF")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[j] = 1;
                            } else if (!strcmp(si_RepetitionPattern_r13, "every4thRF")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[j] = 2;
                            } else if (!strcmp(si_RepetitionPattern_r13, "every8thRF")) {
                                RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[j] = 3;
                            }

                        } else {
                            RRC_CONFIGURATION_REQ(msg_p).bandwidthReducedAccessRelatedInfo_r13[j] = FALSE;
                        }

                        char schedulingInfoBrPath[MAX_OPTNAME_SIZE * 2];
                        config_getlist(&schedulingInfoBrParamList, NULL, 0, brparamspath);
                        RRC_CONFIGURATION_REQ (msg_p).scheduling_info_br_size[j] = schedulingInfoBrParamList.numelt;
                        int siInfoindex;
                        for (siInfoindex = 0; siInfoindex < schedulingInfoBrParamList.numelt; siInfoindex++) {
                            sprintf(schedulingInfoBrPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_SCHEDULING_INFO_LIST, siInfoindex);
                            config_get(schedulingInfoBrParams, sizeof(schedulingInfoBrParams) / sizeof(paramdef_t), schedulingInfoBrPath);
                            RRC_CONFIGURATION_REQ (msg_p).si_Narrowband_r13[j][siInfoindex] = si_Narrowband_r13;
                            RRC_CONFIGURATION_REQ (msg_p).si_TBS_r13[j][siInfoindex] = si_TBS_r13;
                            printf("[DEBUGGING][KOGO] si_narrowband_r13 = %d\n", si_Narrowband_r13);
                            printf("[DEBUGGING][KOGO] si_TBS_r13 = %d\n", si_TBS_r13);
                        }



//                        RRC_CONFIGURATION_REQ (msg_p).system_info_value_tag_SI_size[j] = 0;

                        // kogo -- recheck
                        RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[j] = CALLOC(1, sizeof(BOOLEAN_t));
                        if (!strcmp(fdd_DownlinkOrTddSubframeBitmapBR_r13, "subframePattern40-r13")) {
                            *RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[j] = FALSE;
                            RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_val_r13[j] = fdd_DownlinkOrTddSubframeBitmapBR_val_r13;
                         } else {
                            *RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[j] = TRUE;
                            RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_val_r13[j] = fdd_DownlinkOrTddSubframeBitmapBR_val_r13;
                        }
                        printf("[DEBUGGING][KOGO] fdd_DownlinkOrTddSubframeBitmapBR_r13 = %s\n", fdd_DownlinkOrTddSubframeBitmapBR_r13);
                        printf("[DEBUGGING][KOGO] fdd_DownlinkOrTddSubframeBitmapBR_val_r13 = %x\n", fdd_DownlinkOrTddSubframeBitmapBR_val_r13);

                        RRC_CONFIGURATION_REQ(msg_p).startSymbolBR_r13[j] = startSymbolBR_r13;
                        printf("[DEBUGGING][KOGO] startSymbolBR_r13 = %d\n", startSymbolBR_r13);


                        if (!strcmp(si_HoppingConfigCommon_r13, "off")) {
                            RRC_CONFIGURATION_REQ(msg_p).si_HoppingConfigCommon_r13[j] = 1;
                        } else if (!strcmp(si_HoppingConfigCommon_r13, "on")) {
                            RRC_CONFIGURATION_REQ(msg_p).si_HoppingConfigCommon_r13[j] = 0;
                        }


                        RRC_CONFIGURATION_REQ(msg_p).si_ValidityTime_r13[j] = calloc(1, sizeof(long));
                        if (!strcmp(si_ValidityTime_r13, "true")) {
                            *RRC_CONFIGURATION_REQ(msg_p).si_ValidityTime_r13[j] = 0;
                        } else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d  si_ValidityTime_r13 unknown value!\n",
                                        RC.config_file_name, i);
                        }
                        printf("DEBUGGING][KOGO][CHAR*]: si_ValidityTime_r13: %s - %d\n", si_ValidityTime_r13, *RRC_CONFIGURATION_REQ(msg_p).si_ValidityTime_r13[j]);

                        if (!strcmp(freqHoppingParametersDL_r13, "ENABLE"))
                        {
                            RRC_CONFIGURATION_REQ(msg_p).freqHoppingParametersDL_r13[j] = TRUE;

                            if (!strcmp(interval_DLHoppingConfigCommonModeA_r13, "interval-TDD-r13"))
                                RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13[j] = FALSE;
                            else
                                RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13[j] = TRUE;
                            RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13_val[j] = interval_DLHoppingConfigCommonModeA_r13_val;

                            if (!strcmp(interval_DLHoppingConfigCommonModeB_r13, "interval-TDD-r13"))
                                RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13[j] = FALSE;
                            else
                                RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13[j] = TRUE;
                            RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13_val[j] = interval_DLHoppingConfigCommonModeB_r13_val;

                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[j] = calloc(1, sizeof(long));
                            if (!strcmp(mpdcch_pdsch_HoppingNB_r13, "nb2")) {
                                *RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[j] = 0;
                            } else if (!strcmp(mpdcch_pdsch_HoppingNB_r13, "nb4")) {
                                *RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[j] = 1;
                            } else {
                                AssertFatal(0,
                                            "Failed to parse eNB configuration file %s, enb %d  mpdcch_pdsch_HoppingNB_r13 unknown value!\n",
                                            RC.config_file_name, i);
                            }

                            // kogo -- recheck -- optional
                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingOffset_r13[j] = calloc(1, sizeof(long));
                            *RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingOffset_r13[j] = mpdcch_pdsch_HoppingOffset_r13;

                        }
                        else
                        {
                            RRC_CONFIGURATION_REQ(msg_p).freqHoppingParametersDL_r13[j] = FALSE;
                        }

                        /** ------------------------------SIB23------------------------------------------ */



                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = calloc(1, sizeof(long));
                        if (!strcmp(preambleTransMax_CE_r13, "n3")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 0;
                        } else if (!strcmp(preambleTransMax_CE_r13, "n4")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 1;
                        } else if (!strcmp(preambleTransMax_CE_r13, "n5")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 2;
                        } else if (!strcmp(preambleTransMax_CE_r13, "n6")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 3;
                        } else if (!strcmp(preambleTransMax_CE_r13, "n7")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 4;
                        } else if (!strcmp(preambleTransMax_CE_r13, "n8")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 5;
                        } else if (!strcmp(preambleTransMax_CE_r13, "n10")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 6;
                        } else if (!strcmp(preambleTransMax_CE_r13, "n20")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 7;
                        } else if (!strcmp(preambleTransMax_CE_r13, "n50")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 8;
                        } else if (!strcmp(preambleTransMax_CE_r13, "n100")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 9;
                        } else if (!strcmp(preambleTransMax_CE_r13, "n200")) {
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].preambleTransMax_CE_r13 = 10;
                        } else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d  preambleTransMax_CE_r13 unknown value!\n",
                                        RC.config_file_name, i);
                        }

                        printf("[DEBUGGING][KOGO]: preambleTransMax_CE_r13 = %s\n", preambleTransMax_CE_r13);


                        if (!strcmp(rach_numberOfRA_Preambles, "n4")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (4 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n8")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (8 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n12")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (12 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n16")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (16 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n20")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (20 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n24")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (24 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n28")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (28 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n32")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (32 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n36")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (36 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n40")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (40 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n44")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (44 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n48")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (48 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n52")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (52 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n56")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (56 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n60")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (60 / 4) - 1;
                        } else if (!strcmp(rach_numberOfRA_Preambles, "n64")) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (64 / 4) - 1;
                        } else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
                                        RC.config_file_name, i, rach_numberOfRA_Preambles);

                        }

                        printf("[DEBUGGING][KOGO]: rach_numberOfRA_Preambles = %s\n", rach_numberOfRA_Preambles);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_powerRampingStep = rach_powerRampingStep / 2;

                        printf("[DEBUGGING][KOGO]: rach_powerRampingStep = %d\n", rach_powerRampingStep);


                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleInitialReceivedTargetPower = (rach_preambleInitialReceivedTargetPower + 120) / 2;

                        if ((rach_preambleInitialReceivedTargetPower<-120) || (rach_preambleInitialReceivedTargetPower>-90) || ((rach_preambleInitialReceivedTargetPower&1)!=0))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
                                         RC.config_file_name, i, rach_preambleInitialReceivedTargetPower);


                        printf("[DEBUGGING][KOGO]: rach_preambleInitialReceivedTargetPower = %d\n", rach_preambleInitialReceivedTargetPower);

                        switch (rach_preambleTransMax) {
                        case 3:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n3;
                            break;
                        case 4:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n4;
                            break;
                        case 5:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n5;
                            break;
                        case 6:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n6;
                            break;
                        case 7:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n7;
                            break;
                        case 8:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n8;
                            break;
                        case 10:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n10;
                            break;
                        case 20:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n20;
                            break;
                        case 50:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n50;
                            break;
                        case 100:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n100;
                            break;
                        case 200:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax = PreambleTransMax_n200;
                            break;
                        default:
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
                                         RC.config_file_name, i, rach_preambleTransMax);
                            break;
                        }


                        printf("[DEBUGGING][KOGO]: rach_preambleTransMax = %d\n", rach_preambleTransMax);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_raResponseWindowSize=  (rach_raResponseWindowSize==10)?7:rach_raResponseWindowSize-2;

                        if ((rach_raResponseWindowSize<0)||(rach_raResponseWindowSize==9)||(rach_raResponseWindowSize>10))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
                                         RC.config_file_name, i, rach_preambleTransMax);

                        printf("[DEBUGGING][KOGO]: rach_raResponseWindowSize = %d\n", rach_raResponseWindowSize);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_macContentionResolutionTimer= (rach_macContentionResolutionTimer/8)-1;

                        if ((rach_macContentionResolutionTimer<8) || (rach_macContentionResolutionTimer>64) || ((rach_macContentionResolutionTimer&7)!=0))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
                                         RC.config_file_name, i, rach_preambleTransMax);

                        printf("[DEBUGGING][KOGO]: rach_macContentionResolutionTimer = %d\n", rach_macContentionResolutionTimer);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_maxHARQ_Msg3Tx= rach_maxHARQ_Msg3Tx;

                        if ((rach_maxHARQ_Msg3Tx<0) || (rach_maxHARQ_Msg3Tx>8))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
                                         RC.config_file_name, i, rach_preambleTransMax);

                        printf("[DEBUGGING][KOGO]: rach_maxHARQ_Msg3Tx = %d\n", rach_maxHARQ_Msg3Tx);

                        char rachCELevelInfoListPath[MAX_OPTNAME_SIZE * 2];
                        config_getlist(&rachcelevellist, NULL, 0, brparamspath);
                        RRC_CONFIGURATION_REQ (msg_p).rach_CE_LevelInfoList_r13_size[j] = rachcelevellist.numelt;
                        int rachCEInfoIndex;
                        for (rachCEInfoIndex = 0; rachCEInfoIndex < rachcelevellist.numelt; rachCEInfoIndex++) {
                            sprintf(rachCELevelInfoListPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13, rachCEInfoIndex);
                            config_get(rachcelevelParams, sizeof(rachcelevelParams) / sizeof(paramdef_t), rachCELevelInfoListPath);

                            RRC_CONFIGURATION_REQ (msg_p).firstPreamble_r13[j][rachCEInfoIndex] = firstPreamble_r13;
                            RRC_CONFIGURATION_REQ (msg_p).lastPreamble_r13[j][rachCEInfoIndex]  = lastPreamble_r13;

                            printf("DEBUGGING][KOGO]: firstPreamble_r13: %d\n", firstPreamble_r13);
                            printf("DEBUGGING][KOGO]: lastPreamble_r13: %d\n", lastPreamble_r13);



                            if (!strcmp(ra_ResponseWindowSize_r13, "sf20")) {
                                RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[j][rachCEInfoIndex] = 0;
                            } else if (!strcmp(ra_ResponseWindowSize_r13, "sf50")) {
                                RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[j][rachCEInfoIndex] = 1;
                            } else if (!strcmp(ra_ResponseWindowSize_r13, "sf80")) {
                                RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[j][rachCEInfoIndex] = 2;
                            } else if (!strcmp(ra_ResponseWindowSize_r13, "sf120")) {
                                RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[j][rachCEInfoIndex] = 3;
                            } else if (!strcmp(ra_ResponseWindowSize_r13, "sf180")) {
                                RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[j][rachCEInfoIndex] = 4;
                            } else if (!strcmp(ra_ResponseWindowSize_r13, "sf240")) {
                                RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[j][rachCEInfoIndex] = 5;
                            } else if (!strcmp(ra_ResponseWindowSize_r13, "sf320")) {
                                RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[j][rachCEInfoIndex] = 6;
                            } else if (!strcmp(ra_ResponseWindowSize_r13, "sf400")) {
                                RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[j][rachCEInfoIndex] = 7;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, ra_ResponseWindowSize_r13 unknown value!\n",
                                             RC.config_file_name);
                            }

                            if (!strcmp(mac_ContentionResolutionTimer_r13, "sf80")) {
                                RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[j][rachCEInfoIndex] = 0;
                            } else if (!strcmp(mac_ContentionResolutionTimer_r13, "sf100")) {
                                RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[j][rachCEInfoIndex] = 1;
                            } else if (!strcmp(mac_ContentionResolutionTimer_r13, "sf120")) {
                                RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[j][rachCEInfoIndex] = 2;
                            } else if (!strcmp(mac_ContentionResolutionTimer_r13, "sf160")) {
                                RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[j][rachCEInfoIndex] = 3;
                            } else if (!strcmp(mac_ContentionResolutionTimer_r13, "sf200")) {
                                RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[j][rachCEInfoIndex] = 4;
                            } else if (!strcmp(mac_ContentionResolutionTimer_r13, "sf240")) {
                                RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[j][rachCEInfoIndex] = 5;
                            } else if (!strcmp(mac_ContentionResolutionTimer_r13, "sf480")) {
                                RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[j][rachCEInfoIndex] = 6;
                            } else if (!strcmp(mac_ContentionResolutionTimer_r13, "sf960")) {
                                RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[j][rachCEInfoIndex] = 7;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, mac_ContentionResolutionTimer_r13 unknown value!\n",
                                             RC.config_file_name);
                            }

                            if (!strcmp(rar_HoppingConfig_r13, "on")) {
                                RRC_CONFIGURATION_REQ (msg_p).rar_HoppingConfig_r13[j][rachCEInfoIndex] = 0;
                            } else if (!strcmp(rar_HoppingConfig_r13, "off")) {
                                RRC_CONFIGURATION_REQ (msg_p).rar_HoppingConfig_r13[j][rachCEInfoIndex] = 1;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, rar_HoppingConfig_r13 unknown value!\n",
                                             RC.config_file_name);
                            }

                        } // end for loop (rach ce level info)

                        printf("DEBUGGING][KOGO]: ra_ResponseWindowSize_r13: %s\n", ra_ResponseWindowSize_r13);
                        printf("DEBUGGING][KOGO]: mac_ContentionResolutionTimer_r13: %s\n", mac_ContentionResolutionTimer_r13);
                        printf("DEBUGGING][KOGO]: rar_HoppingConfig_r13: %s\n", rar_HoppingConfig_r13);



                        /**  BCCH CONFIG */
                        switch (bcch_modificationPeriodCoeff) {
                        case 2:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n2;
                            break;
                        case 4:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n4;
                            break;
                        case 8:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n8;
                            break;
                        case 16:
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n16;
                            break;
                        default:
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
                                         RC.config_file_name, i, bcch_modificationPeriodCoeff);

                            break;
                        }

                        /**  PCCH CONFIG */
                        if (!strcmp(pcch_defaultPagingCycle, "rf32")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf32;
                        } else if (!strcmp(pcch_defaultPagingCycle, "rf64")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf64;
                        } else if (!strcmp(pcch_defaultPagingCycle, "rf128")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf128;
                        } else if (!strcmp(pcch_defaultPagingCycle, "rf256")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf256;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
                                         RC.config_file_name, i, pcch_defaultPagingCycle);
                        }

                        printf("[DEBUGGING][KOGO]: pcch_defaultPagingCycle_br = %d\n", RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle);

                        if (strcmp(pcch_nB, "fourT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_fourT;
                        }
                        else if (strcmp(pcch_nB, "twoT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_twoT;
                        }
                        else if (strcmp(pcch_nB, "oneT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneT;
                        }
                        else if (strcmp(pcch_nB, "halfT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_halfT;
                        }
                        else if (strcmp(pcch_nB, "quarterT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_quarterT;
                        }
                        else if (strcmp(pcch_nB, "oneEighthT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneEighthT;
                        }
                        else if (strcmp(pcch_nB, "oneSixteenthT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneSixteenthT;
                        }
                        else if (strcmp(pcch_nB, "oneThirtySecondT") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneThirtySecondT;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
                                        RC.config_file_name, i, pcch_defaultPagingCycle);


                        /** PRACH CONFIG */

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_root =  prach_root;
                        if ((prach_root <0) || (prach_root > 1023))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_root choice: 0..1023 !\n",
                                         RC.config_file_name, i, prach_root);

                        printf("DEBUGGING][KOGO]: prach_root: %d\n", prach_root);



                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_config_index = prach_config_index;
                        if ((prach_config_index <0) || (prach_config_index > 63))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
                                         RC.config_file_name, i, prach_config_index);

                        printf("DEBUGGING][KOGO]: prach_config_index: %d\n", prach_config_index);



                        if (!prach_high_speed)
                            AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                                         RC.config_file_name, i, ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
                        else if (strcmp(prach_high_speed, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_high_speed = TRUE;
                        }
                        else if (strcmp(prach_high_speed, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_high_speed = FALSE;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
                                        RC.config_file_name, i, prach_high_speed);


                        printf("DEBUGGING][KOGO]: prach_high_speed: %s\n", prach_high_speed);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_zero_correlation = prach_zero_correlation;
                        if ((prach_zero_correlation <0) || (prach_zero_correlation > 15))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
                                         RC.config_file_name, i, prach_zero_correlation);

                        printf("DEBUGGING][KOGO]: prach_zero_correlation: %d\n", prach_zero_correlation);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_freq_offset = prach_freq_offset;
                        if ((prach_freq_offset <0) || (prach_freq_offset > 94))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
                                         RC.config_file_name, i, prach_freq_offset);

                        printf("DEBUGGING][KOGO]: prach_freq_offset: %d\n", prach_freq_offset);


                        /** PDSCH Config Common */

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pdsch_referenceSignalPower = pdsch_referenceSignalPower;
                        if ((pdsch_referenceSignalPower <-60) || (pdsch_referenceSignalPower > 50))
                            AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
                                         RC.config_file_name, i, pdsch_referenceSignalPower);

                        printf("DEBUGGING][KOGO]: pdsch_referenceSignalPower: %d\n", pdsch_referenceSignalPower);


                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pdsch_p_b = pdsch_p_b;
                        if ((pdsch_p_b <0) || (pdsch_p_b > 3))
                            AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
                                         RC.config_file_name, i, pdsch_p_b);

                        printf("DEBUGGING][KOGO]: pdsch_p_b: %d\n", pdsch_p_b);




                        /** PUSCH Config Common */

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_n_SB = pusch_n_SB;
                        if ((pusch_n_SB <1) || (pusch_n_SB > 4))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
                                         RC.config_file_name, i, pusch_n_SB);

                        printf("DEBUGGING][KOGO]: pusch_n_SB: %d\n", pusch_n_SB);



                        if (!pusch_hoppingMode)
                            AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d define %s: interSubframe,intraAndInterSubframe!\n",
                                         RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);
                        else if (strcmp(pusch_hoppingMode, "interSubFrame") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
                        }
                        else if (strcmp(pusch_hoppingMode, "intraAndInterSubFrame") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
                                        RC.config_file_name, i, pusch_hoppingMode);

                        printf("DEBUGGING][KOGO]: pusch_hoppingMode: %s\n", pusch_hoppingMode);



                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_hoppingOffset = pusch_hoppingOffset;
                        if ((pusch_hoppingOffset<0) || (pusch_hoppingOffset>98))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
                                         RC.config_file_name, i, pusch_hoppingMode);

                        printf("DEBUGGING][KOGO]: pusch_hoppingOffset: %d\n", pusch_hoppingOffset);


                        if (!pusch_enable64QAM)
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                                         RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
                        else if (strcmp(pusch_enable64QAM, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_enable64QAM = TRUE;
                        }
                        else if (strcmp(pusch_enable64QAM, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_enable64QAM = FALSE;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
                                        RC.config_file_name, i, pusch_enable64QAM);

                        printf("DEBUGGING][KOGO]: pusch_enable64QAM: %s\n", pusch_enable64QAM);


                        if (!pusch_groupHoppingEnabled)
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                                        RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
                        else if (strcmp(pusch_groupHoppingEnabled, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_groupHoppingEnabled = TRUE;
                        }
                        else if (strcmp(pusch_groupHoppingEnabled, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_groupHoppingEnabled = FALSE;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
                                        RC.config_file_name, i, pusch_groupHoppingEnabled);

                        printf("DEBUGGING][KOGO]: pusch_groupHoppingEnabled: %s\n", pusch_groupHoppingEnabled);



                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_groupAssignment = pusch_groupAssignment;
                        if ((pusch_groupAssignment<0)||(pusch_groupAssignment>29))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
                                         RC.config_file_name, i, pusch_groupAssignment);

                        printf("DEBUGGING][KOGO]: pusch_groupAssignment: %d\n", pusch_groupAssignment);


                        if (!pusch_sequenceHoppingEnabled)
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                                         RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
                        else if (strcmp(pusch_sequenceHoppingEnabled, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_sequenceHoppingEnabled = TRUE;
                        }
                        else if (strcmp(pusch_sequenceHoppingEnabled, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_sequenceHoppingEnabled = FALSE;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
                                        RC.config_file_name, i, pusch_sequenceHoppingEnabled);

                        printf("DEBUGGING][KOGO]: pusch_sequenceHoppingEnabled: %s\n", pusch_sequenceHoppingEnabled);




                        /** PUCCH Config Common */
                        /** TO BE CONTINUED */

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_delta_shift = pucch_delta_shift-1;

                        if ((pucch_delta_shift <1) || (pucch_delta_shift > 3))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
                                         RC.config_file_name, i, pucch_delta_shift);


                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_nRB_CQI = pucch_nRB_CQI;

                        if ((pucch_nRB_CQI <0) || (pucch_nRB_CQI > 98))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
                                         RC.config_file_name, i, pucch_nRB_CQI);

                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_nCS_AN = pucch_nCS_AN;

                        if ((pucch_nCS_AN <0) || (pucch_nCS_AN > 7))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
                                         RC.config_file_name, i, pucch_nCS_AN);


                           /**
                                 check if pucch_n1_AN is in necessary the BR parameters
                            */



                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_p0_Nominal = pusch_p0_Nominal;

                        if ((pusch_p0_Nominal < -126) || (pusch_p0_Nominal > 24))
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
                                        RC.config_file_name, i, pusch_p0_Nominal);

                        if (strcmp(pusch_alpha, "AL0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al0;
                        } else if (strcmp(pusch_alpha, "AL04") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al04;
                        } else if (strcmp(pusch_alpha, "AL05") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al05;
                        } else if (strcmp(pusch_alpha, "AL06") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al06;
                        } else if (strcmp(pusch_alpha, "AL07") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al07;
                        } else if (strcmp(pusch_alpha, "AL08") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al08;
                        } else if (strcmp(pusch_alpha, "AL09") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al09;
                        } else if (strcmp(pusch_alpha, "AL1") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pusch_alpha = Alpha_r12_al1;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
                                        RC.config_file_name, i, pusch_alpha);

                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_p0_Nominal = pucch_p0_Nominal;
                        if ((pucch_p0_Nominal < -127) || (pucch_p0_Nominal > -96))
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
                                        RC.config_file_name, i, pucch_p0_Nominal);

                        if (strcmp(pucch_deltaF_Format1, "deltaF_2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1 = DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
                        }
                        else if (strcmp(pucch_deltaF_Format1, "deltaF0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
                        }
                        else if (strcmp(pucch_deltaF_Format1, "deltaF2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
                                        RC.config_file_name, i, pucch_deltaF_Format1);

                        if (strcmp(pucch_deltaF_Format1b, "deltaF1") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
                        }
                        else if (strcmp(pucch_deltaF_Format1b, "deltaF3") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
                        }
                        else if (strcmp(pucch_deltaF_Format1b, "deltaF5") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
                                        RC.config_file_name, i, pucch_deltaF_Format1b);


                        if (strcmp(pucch_deltaF_Format2, "deltaF_2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
                        }
                        else if (strcmp(pucch_deltaF_Format2, "deltaF0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
                        }
                        else if (strcmp(pucch_deltaF_Format2, "deltaF1") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
                        }
                        else if (strcmp(pucch_deltaF_Format2, "deltaF2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
                                        RC.config_file_name, i, pucch_deltaF_Format2);

                        if (strcmp(pucch_deltaF_Format2a, "deltaF_2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
                        }
                        else if (strcmp(pucch_deltaF_Format2a, "deltaF0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
                        }
                        else if (strcmp(pucch_deltaF_Format2a, "deltaF2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
                                        RC.config_file_name, i, pucch_deltaF_Format2a);

                        if (strcmp(pucch_deltaF_Format2b, "deltaF_2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
                        }
                        else if (strcmp(pucch_deltaF_Format2b, "deltaF0") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
                        }
                        else if (strcmp(pucch_deltaF_Format2b, "deltaF2") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
                                        RC.config_file_name, i, pucch_deltaF_Format2b);



                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].msg3_delta_Preamble = msg3_delta_Preamble;

                        if ((msg3_delta_Preamble < -1) || (msg3_delta_Preamble > 6))
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
                                        RC.config_file_name, i, msg3_delta_Preamble);




                        if (!strcmp(prach_ConfigCommon_v1310, "ENABLE"))
                        {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_ConfigCommon_v1310 = TRUE;

                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13 = calloc(1, sizeof(BOOLEAN_t));

                            if (!strcmp(mpdcch_startSF_CSS_RA_r13, "tdd-r13")) {
                                *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13 = FALSE;
                            } else {
                                *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13 = TRUE;
                            }

                            if (!strcmp(mpdcch_startSF_CSS_RA_r13_val, "v1")) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13_val = 0;
                            } else if (!strcmp(mpdcch_startSF_CSS_RA_r13_val, "v1dot5")) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13_val = 1;
                            } else if (!strcmp(mpdcch_startSF_CSS_RA_r13_val, "v2")) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13_val = 2;
                            } else if (!strcmp(mpdcch_startSF_CSS_RA_r13_val, "v2dot5")) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13_val = 3;
                            } else if (!strcmp(mpdcch_startSF_CSS_RA_r13_val, "v4")) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13_val = 4;
                            } else if (!strcmp(mpdcch_startSF_CSS_RA_r13_val, "v5")) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13_val = 5;
                            } else if (!strcmp(mpdcch_startSF_CSS_RA_r13_val, "v8")) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13_val = 6;
                            } else if (!strcmp(mpdcch_startSF_CSS_RA_r13_val, "10")) {
                                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].mpdcch_startSF_CSS_RA_r13_val = 7;
                            } else {
                                AssertFatal(0,
                                            "Failed to parse eNB configuration file %s, enb %d mpdcch_startSF_CSS_RA_r13_val! Unknown Value !!\n",
                                            RC.config_file_name, i);
                            }
                            printf("[DEBUGGING][KOGO] mpdcch_startSF_CSS_RA_r13_val %s\n", mpdcch_startSF_CSS_RA_r13_val);

                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_HoppingOffset_r13 = calloc(1, sizeof(long));
                            *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_HoppingOffset_r13 = prach_HoppingOffset_r13;
                        } else {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].prach_ConfigCommon_v1310 = FALSE;
                        }


                        RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeA_r13[j] = CALLOC(1, sizeof(long));
                        if (!strcmp(pdsch_maxNumRepetitionCEmodeA_r13, "r16")) {
                            *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeA_r13[j] = 0;
                        } else if (!strcmp(pdsch_maxNumRepetitionCEmodeA_r13, "r32")) {
                            *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeA_r13[j] = 1;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, pdsch_maxNumRepetitionCEmodeA_r13 unknown value!\n",
                                         RC.config_file_name);
                        }
                        printf("DEBUGGING][KOGO]: pdsch_maxNumRepetitionCEmodeA_r13: %s\n", pdsch_maxNumRepetitionCEmodeA_r13);


                        RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[j] = CALLOC(1, sizeof(long));
                        if (!strcmp(pusch_maxNumRepetitionCEmodeA_r13, "r8")) {
                            *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[j] =  0;
                        } else if (!strcmp(pusch_maxNumRepetitionCEmodeA_r13, "r16")) {
                            *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[j] =  1;
                        } else if (!strcmp(pusch_maxNumRepetitionCEmodeA_r13, "r32")) {
                            *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[j] =  2;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, pusch_maxNumRepetitionCEmodeA_r13 unknown value!\n",
                                         RC.config_file_name);
                        }
                        printf("DEBUGGING][KOGO]: pusch_maxNumRepetitionCEmodeA_r13: %s\n", pusch_maxNumRepetitionCEmodeA_r13);


                        char rsrpRangeListPath[MAX_OPTNAME_SIZE * 2];
                        config_getlist(&rsrprangelist, NULL, 0, brparamspath);
                        RRC_CONFIGURATION_REQ (msg_p).rsrp_range_list_size[j] = rsrprangelist.numelt;


                        int rsrprangeindex;
                        for (rsrprangeindex = 0; rsrprangeindex < rsrprangelist.numelt; rsrprangeindex++) {
                            sprintf(rsrpRangeListPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_RSRP_RANGE_LIST, rsrprangeindex);
                            config_get(rsrprangeParams, sizeof(rsrprangeParams) / sizeof(paramdef_t), rsrpRangeListPath);
                            RRC_CONFIGURATION_REQ (msg_p).rsrp_range[j][rsrprangeindex] = rsrp_range_br;
                            printf("[DEBUGGING][KOGO] : rsrp range br = %d\n", rsrp_range_br);
                        }


                        char prachparameterscePath[MAX_OPTNAME_SIZE * 2];
                        config_getlist(&prachParamslist, NULL, 0, brparamspath);
                        RRC_CONFIGURATION_REQ (msg_p).prach_parameters_list_size[j] = prachParamslist.numelt;

                        int prachparamsindex;
                        for (prachparamsindex = 0; prachparamsindex < prachParamslist.numelt; prachparamsindex++) {
                            sprintf(prachparameterscePath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13, prachparamsindex);
                            config_get(prachParams, sizeof(prachParams) / sizeof(paramdef_t), prachparameterscePath);

                            RRC_CONFIGURATION_REQ (msg_p).prach_config_index[j][prachparamsindex]                  = prach_config_index_br;
                            RRC_CONFIGURATION_REQ (msg_p).prach_freq_offset[j][prachparamsindex]                   = prach_freq_offset_br;

                            RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[j][prachparamsindex] = calloc(1, sizeof(long));
                            *RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[j][prachparamsindex] = prach_StartingSubframe_r13;

                            RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = calloc(1, sizeof(long));
                            if (!strcmp(maxNumPreambleAttemptCE_r13, "n3")) {
                                *RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = 0;
                            } else if (!strcmp(maxNumPreambleAttemptCE_r13, "n4")) {
                                *RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = 1;
                            } else if (!strcmp(maxNumPreambleAttemptCE_r13, "n5")) {
                                *RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = 2;
                            } else if (!strcmp(maxNumPreambleAttemptCE_r13, "n6")) {
                                *RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = 3;
                            } else if (!strcmp(maxNumPreambleAttemptCE_r13, "n7")) {
                                *RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = 4;
                            } else if (!strcmp(maxNumPreambleAttemptCE_r13, "n8")) {
                                *RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = 5;
                            } else if (!strcmp(maxNumPreambleAttemptCE_r13, "n10")) {
                                *RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = 6;
                            } else if (!strcmp(maxNumPreambleAttemptCE_r13, "spare1")) {
                                *RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = 7;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, maxNumPreambleAttemptCE_r13 unknown value !! %d!\n",
                                             RC.config_file_name, nb_cc++);
                            }
                            printf("[DEBUGGING][KOGO]: maxNumPreambleAttemptCE_r13: %s\n", maxNumPreambleAttemptCE_r13);


                            if (!strcmp(numRepetitionPerPreambleAttempt_r13, "n1")) {
                                RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[j][prachparamsindex] = 0;
                            } else if (!strcmp(numRepetitionPerPreambleAttempt_r13, "n2")) {
                                RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[j][prachparamsindex] = 1;
                            } else if (!strcmp(numRepetitionPerPreambleAttempt_r13, "n4")) {
                                RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[j][prachparamsindex] = 2;
                            } else if (!strcmp(numRepetitionPerPreambleAttempt_r13, "n8")) {
                                RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[j][prachparamsindex] = 3;
                            } else if (!strcmp(numRepetitionPerPreambleAttempt_r13, "n16")) {
                                RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[j][prachparamsindex] = 4;
                            } else if (!strcmp(numRepetitionPerPreambleAttempt_r13, "n32")) {
                                RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[j][prachparamsindex] = 5;
                            } else if (!strcmp(numRepetitionPerPreambleAttempt_r13, "n64")) {
                                RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[j][prachparamsindex] = 6;
                            } else if (!strcmp(numRepetitionPerPreambleAttempt_r13, "n128")) {
                                RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[j][prachparamsindex] = 7;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, numRepetitionPerPreambleAttempt_r13 unknown value !! %d!\n",
                                             RC.config_file_name, nb_cc++);
                            }

                            printf("[DEBUGGING][KOGO]: numRepetitionPerPreambleAttempt_r13: %s\n", numRepetitionPerPreambleAttempt_r13);


                            if (!strcmp(mpdcch_NumRepetition_RA_r13, "r1")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 0;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r2")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 1;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r4")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 2;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r8")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 3;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r16")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 4;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r32")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 5;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r64")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 6;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r128")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 7;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r256")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 8;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r512")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 9;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r1024")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 10;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "r2048")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 11;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "spare4")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 12;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "spare3")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 13;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "spare2")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 14;
                            } else if (!strcmp(mpdcch_NumRepetition_RA_r13, "spare1")) {
                                RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = 15;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, mpdcch_NumRepetition_RA_r13 unknown value !! %d!\n",
                                             RC.config_file_name, nb_cc++);
                            }

                            printf("[DEBUGGING][KOGO]: mpdcch_NumRepetition_RA_r13: %s\n", mpdcch_NumRepetition_RA_r13);

                            if (!strcmp(prach_HoppingConfig_r13, "off")) {
                                RRC_CONFIGURATION_REQ (msg_p).prach_HoppingConfig_r13[j][prachparamsindex] = 1;
                            } else if (!strcmp(prach_HoppingConfig_r13, "on")) {
                                RRC_CONFIGURATION_REQ (msg_p).prach_HoppingConfig_r13[j][prachparamsindex] = 0;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, prach_HoppingConfig_r13 unknown value !! %d!\n",
                                             RC.config_file_name, nb_cc++);
                            }

                            printf("[DEBUGGING][KOGO]: prach_HoppingConfig_r13: %s\n", prach_HoppingConfig_r13);



                            int maxavailablenarrowband_count = prachParams[7].numelt;
                            printf("[DEBUGGING][KOGO]: maxavailablenarrowband count = %d\n", prachParams[7].numelt);
                            RRC_CONFIGURATION_REQ (msg_p).max_available_narrow_band_size[j][prachparamsindex] = maxavailablenarrowband_count;
                            int narrow_band_index;
                            for (narrow_band_index = 0; narrow_band_index < maxavailablenarrowband_count; narrow_band_index++)
                            {
                                RRC_CONFIGURATION_REQ (msg_p).max_available_narrow_band[j][prachparamsindex][narrow_band_index] = prachParams[7].iptr[narrow_band_index];
                                printf("[DEBUGGING][KOGO]: maxavailablenarrowband[index] = %d\n", prachParams[7].iptr[narrow_band_index]);
                            }


                        }

                        char n1PUCCHInfoParamsPath[MAX_OPTNAME_SIZE * 2];
                        config_getlist(&n1PUCCHInfoList, NULL, 0, brparamspath);
                        RRC_CONFIGURATION_REQ (msg_p).pucch_info_value_size[j] = n1PUCCHInfoList.numelt;

                        int n1PUCCHinfolistindex;
                        for (n1PUCCHinfolistindex = 0; n1PUCCHinfolistindex < n1PUCCHInfoList.numelt; n1PUCCHinfolistindex++) {
                            sprintf(n1PUCCHInfoParamsPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13, n1PUCCHinfolistindex);
                            config_get(n1PUCCH_ANR13Params, sizeof(n1PUCCH_ANR13Params) / sizeof(paramdef_t), n1PUCCHInfoParamsPath);
                            RRC_CONFIGURATION_REQ (msg_p).pucch_info_value[j][n1PUCCHinfolistindex] = pucch_info_value;
                            printf("[DEBUGGING][KOGO]: pucch_info_value: %d\n", pucch_info_value);
                        }

                        /**  UE Timers And Constants */


                        if (!strcmp(ue_TimersAndConstants_t300, "ms100")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms100;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms200")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms200;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms300")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms300;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms400")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms400;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms600")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms600;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms1000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms1000;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms1500")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms1500;
                        } else if (!strcmp(ue_TimersAndConstants_t300, "ms2000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms2000;
                        } else {
                            AssertFatal (0,
                                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t300 unknown value !!",
                                RC.config_file_name, i, ue_TimersAndConstants_t300);
                        }

                        printf("[DEBUGGING][KOGO]: ue_TimersAndConstants_t300 : %s\n", ue_TimersAndConstants_t300);

                        if (!strcmp(ue_TimersAndConstants_t301, "ms100")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms100;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms200")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms200;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms300")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms300;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms400")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms400;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms600")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms600;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms1000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms1000;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms1500")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms1500;
                        } else if (!strcmp(ue_TimersAndConstants_t301, "ms2000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms2000;
                        } else {
                            AssertFatal (0,
                                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t301 !!",
                                RC.config_file_name, i, ue_TimersAndConstants_t301);
                        }

                        printf("[DEBUGGING][KOGO]: ue_TimersAndConstants_t301 : %s\n", ue_TimersAndConstants_t301);

                        if (!strcmp(ue_TimersAndConstants_t310, "ms0")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms0;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms50")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms50;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms100")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms100;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms200")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms200;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms500")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms500;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms1000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms1000;
                        } else if (!strcmp(ue_TimersAndConstants_t310, "ms2000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms2000;
                        } else {
                            AssertFatal (0,
                                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t310 !!",
                                RC.config_file_name, i, ue_TimersAndConstants_t310);
                        }

                        printf("[DEBUGGING][KOGO]: ue_TimersAndConstants_t310 : %s\n", ue_TimersAndConstants_t310);

                        if (!strcmp(ue_TimersAndConstants_t311, "ms1000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms1000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms3000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms3000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms5000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms5000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms10000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms10000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms15000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms15000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms20000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms20000;
                        } else if (!strcmp(ue_TimersAndConstants_t311, "ms30000")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms30000;
                        } else {
                            AssertFatal (0,
                                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 !!",
                                RC.config_file_name, i, ue_TimersAndConstants_t311);
                        }

                        printf("[DEBUGGING][KOGO]: ue_TimersAndConstants_t311 : %s\n", ue_TimersAndConstants_t311);

                        if (!strcmp(ue_TimersAndConstants_n310, "n1")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n1;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n2")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n2;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n3")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n3;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n4")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n4;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n6")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n6;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n8")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n8;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n10")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n10;
                        } else if (!strcmp(ue_TimersAndConstants_n310, "n20")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n20;
                        } else {
                            AssertFatal (0,
                                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_n310 !!",
                                RC.config_file_name, i, ue_TimersAndConstants_n310);
                        }

                        printf("[DEBUGGING][KOGO]: ue_TimersAndConstants_n310 : %s\n", ue_TimersAndConstants_n310);

                        if (!strcmp(ue_TimersAndConstants_n311, "n1")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n1;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n2")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n2;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n3")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n3;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n4")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n4;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n5")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n5;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n6")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n6;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n8")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n8;
                        } else if (!strcmp(ue_TimersAndConstants_n311, "n10")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n10;
                        } else {
                            AssertFatal (0,
                                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_n311!!",
                                RC.config_file_name, i, ue_TimersAndConstants_n311);
                        }

                        printf("[DEBUGGING][KOGO]: ue_TimersAndConstants_n311 : %s\n", ue_TimersAndConstants_n311);


                        if (!strcmp(ue_TransmissionMode, "tm1")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm1;
                        } else if (!strcmp(ue_TransmissionMode, "tm2")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm2;
                        } else if (!strcmp(ue_TransmissionMode, "tm3")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm3;
                        } else if (!strcmp(ue_TransmissionMode, "tm4")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm4;
                        } else if (!strcmp(ue_TransmissionMode, "tm5")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm5;
                        } else if (!strcmp(ue_TransmissionMode, "tm6")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm6;
                        } else if (!strcmp(ue_TransmissionMode, "tm7")) {
                            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm7;
                        } else {
                            AssertFatal (0,
                                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode !!",
                                RC.config_file_name, i, ue_TransmissionMode);
                        }

                        printf("[DEBUGGING][KOGO]: ue_TransmissionMode : %s\n", ue_TransmissionMode);

                        char PCCHConfigv1310Path[MAX_OPTNAME_SIZE*2 + 16];
                        sprintf(PCCHConfigv1310Path, "%s.%s", brparamspath, ENB_CONFIG_STRING_PCCH_CONFIG_V1310);
                        config_get(pcchv1310Params, sizeof(pcchv1310Params)/sizeof(paramdef_t), PCCHConfigv1310Path);



                        /** PCCH CONFIG V1310 */

                        RRC_CONFIGURATION_REQ(msg_p).pcch_config_v1310[j] = TRUE;
                        RRC_CONFIGURATION_REQ(msg_p).paging_narrowbands_r13[j] = paging_narrowbands_r13;
                        printf("[DEBUGGING][KOGO]: paging_narrowbands_r13 = %d\n", paging_narrowbands_r13);

                        if (!strcmp(mpdcch_numrepetition_paging_r13, "r1")) {
                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[j] = 0;
                        } else if (!strcmp(mpdcch_numrepetition_paging_r13, "r2")) {
                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[j] = 1;
                        } else if (!strcmp(mpdcch_numrepetition_paging_r13, "r4")) {
                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[j] = 2;
                        } else if (!strcmp(mpdcch_numrepetition_paging_r13, "r8")) {
                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[j] = 3;
                        } else if (!strcmp(mpdcch_numrepetition_paging_r13, "r16")) {
                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[j] = 4;
                        } else if (!strcmp(mpdcch_numrepetition_paging_r13, "r32")) {
                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[j] = 5;
                        } else if (!strcmp(mpdcch_numrepetition_paging_r13, "r64")) {
                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[j] = 6;
                        } else if (!strcmp(mpdcch_numrepetition_paging_r13, "r128")) {
                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[j] = 7;
                        } else if (!strcmp(mpdcch_numrepetition_paging_r13, "r256")) {
                            RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[j] = 8;
                        } else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, nb_v1310, unknown value !\n",
                                        RC.config_file_name);
                        }

                        printf("[DEBUGGING][KOGO] mpdcch_numrepetition_paging_r13: %s\n", mpdcch_numrepetition_paging_r13);

//                        RRC_CONFIGURATION_REQ(msg_p).nb_v1310[j] = CALLOC(1, sizeof(long));
//                        if (!strcmp(nb_v1310, "one64thT")) {
//                            *RRC_CONFIGURATION_REQ(msg_p).nb_v1310[j] = 0;
//                        } else if (!strcmp(nb_v1310, "one128thT")) {
//                            *RRC_CONFIGURATION_REQ(msg_p).nb_v1310[j] = 1;
//                        } else if (!strcmp(nb_v1310, "one256thT")) {
//                            *RRC_CONFIGURATION_REQ(msg_p).nb_v1310[j] = 2;
//                        } else {
//                            AssertFatal(0,
//                                        "Failed to parse eNB configuration file %s, nb_v1310, unknown value !\n",
//                                        RC.config_file_name);
//                        }
//                        printf("[DEBUGGING][KOGO][CHAR*] nb_v1310: %s - %d\n", nb_v1310,  *RRC_CONFIGURATION_REQ(msg_p).nb_v1310[j]);


                        RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[j] = CALLOC(1, sizeof(long));
                        // ++cnt; // check this ,, the conter is up above
                        if (!strcmp(pucch_NumRepetitionCE_Msg4_Level0_r13, "n1")) {
                            *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[j] = 0;
                        } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level0_r13, "n2")) {
                            *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[j] = 1;
                        } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level0_r13, "n4")) {
                            *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[j] = 2;
                        } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level0_r13, "n8")) {
                            *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[j] = 3;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, pucch_NumRepetitionCE_Msg4_Level0_r13 unknown value!\n",
                                         RC.config_file_name);
                        }
                        printf("DEBUGGING][KOGO]: pucch_NumRepetitionCE_Msg4_Level0_r13: %s\n", pucch_NumRepetitionCE_Msg4_Level0_r13);



                        /** SIB2 FREQ HOPPING PARAMETERS R13 */
                        RRC_CONFIGURATION_REQ(msg_p).sib2_freq_hoppingParameters_r13_exists[j] = TRUE;

                        char sib2FreqHoppingParametersR13Path[MAX_OPTNAME_SIZE*2 + 16];
                        sprintf(sib2FreqHoppingParametersR13Path, "%s.%s", brparamspath, ENB_CONFIG_STRING_SIB2_FREQ_HOPPINGPARAMETERS_R13);
                        config_get(sib2freqhoppingParams, sizeof(sib2freqhoppingParams)/sizeof(paramdef_t), sib2FreqHoppingParametersR13Path);


                        RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13[j] = CALLOC(1, sizeof(long));
                        if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13, "FDD")) {
                            *RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13[j] = 0;
                            if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13_val, "int1")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[j] = 0;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13_val, "int2")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[j] = 1;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13_val, "int4")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[j] = 2;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13_val, "int8")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[j] = 3;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, sib2_interval_ULHoppingConfigCommonModeA_r13_val unknown value !!\n",
                                             RC.config_file_name);
                            }
                        } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13, "TDD")) {
                            *RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13[j] = 1;
                            if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13_val, "int1")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[j] = 0;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13_val, "int5")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[j] = 1;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13_val, "int10")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[j] = 2;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13_val, "int20")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[j] = 3;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, sib2_interval_ULHoppingConfigCommonModeA_r13_val unknown value !!\n",
                                             RC.config_file_name);
                            }
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, sib2_interval_ULHoppingConfigCommonModeA_r13 unknown value !!\n",
                                         RC.config_file_name);
                        }

                        printf("[DEBUGGING][KOGO]: sib2_interval_ULHoppingConfigCommonModeA_r13: %s\n", sib2_interval_ULHoppingConfigCommonModeA_r13);
                        printf("[DEBUGGING][KOGO]: sib2_interval_ULHoppingConfigCommonModeA_r13_val: %s\n", sib2_interval_ULHoppingConfigCommonModeA_r13_val);



                        if (strcmp(rach_preamblesGroupAConfig, "ENABLE") == 0)
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_preamblesGroupAConfig= TRUE;



                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_nDMRS1 = pusch_nDMRS1;  //cyclic_shift in RRC!

                        printf("[DEBUGGING][KOGO]: pusch_nDMRS1 = %d\n", pusch_nDMRS1);

                        if ((pusch_nDMRS1 <0) || (pusch_nDMRS1>7))
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
                                         RC.config_file_name, i, pusch_nDMRS1);


                        if (strcmp(phich_duration, "NORMAL") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_duration = PHICH_Config__phich_Duration_normal;
                        }
                        else if (strcmp(phich_duration, "EXTENDED") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_duration = PHICH_Config__phich_Duration_extended;
                        }
                        else
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
                                        RC.config_file_name, i, phich_duration);


                        printf("[DEBUGGING][KOGO]: phich_duration = %s\n", phich_duration);

                        if (strcmp(phich_resource, "ONESIXTH") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_resource = PHICH_Config__phich_Resource_oneSixth;
                        }
                        else if (strcmp(phich_resource, "HALF") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_resource = PHICH_Config__phich_Resource_half;
                        }
                        else if (strcmp(phich_resource, "ONE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_resource = PHICH_Config__phich_Resource_one;
                        }
                        else if (strcmp(phich_resource, "TWO") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_resource = PHICH_Config__phich_Resource_two;
                        }
                        else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
                                        RC.config_file_name, i, phich_resource);
                        }

                        printf("[DEBUGGING][KOGO]: phich_resource = %s\n", phich_resource);


                        printf("phich.resource %d (%s), phich.duration %d (%s)\n",
                               RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_resource, phich_resource,
                               RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].phich_duration, phich_duration);

                        if (strcmp(srs_enable, "ENABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_enable = TRUE;
                        }
                        else if (strcmp(srs_enable, "DISABLE") == 0) {
                            RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_enable = FALSE;
                        }
                        else {
                            AssertFatal(0,
                                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                                        RC.config_file_name, i, srs_enable);
                        }

                        printf("[DEBUGGING][KOGO]: srs_enable = %s\n", srs_enable);






                        /*system_info_value_tag_SI_list = config_setting_get_member(setting_br13, ENB_CONFIG_STRING_SYSTEM_INFO_VALUE_TAG_LIST);
                  int num_system_info;
                  if (system_info_value_tag_SI_list != NULL)
                  {
                      num_system_info = config_setting_length(system_info_value_tag_SI_list);
                      for (sys_info_idx = 0; sys_info_idx < num_system_info; ++sys_info_idx)
                      {
                          system_info_value_tag_SI = config_setting_get_elem(system_info_value_tag_SI_list, sys_info_idx);
                          if ( !(config_setting_lookup_int(system_info_value_tag_SI, ENB_CONFIG_STRING_SYSTEM_INFO_VALUE_TAG_SI_R13, &systemInfoValueTagSi_r13)) )
                          {
                              AssertFatal (0, "Failed to parse eNB configuration file %s, system info value tag %d!\n", RC.config_file_name, nb_cc++);
                          }
                          RRC_CONFIGURATION_REQ (msg_p).systemInfoValueTagSi_r13[j][sys_info_idx] = systemInfoValueTagSi_r13;
                      }
                  }
                  else
                  {
                      num_system_info = 0;
                  }
                  RRC_CONFIGURATION_REQ (msg_p).system_info_value_tag_SI_size[j] = num_system_info;


                      if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_FREQHOPPINGPARAMETERSDL, &freqHoppingParametersDL_r13) && !strcmp(freqHoppingParametersDL_r13, "ENABLE"))
                      {
                          RRC_CONFIGURATION_REQ(msg_p).freqHoppingParametersDL_r13[j] = TRUE;
                          if (!config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEB, &interval_DLHoppingConfigCommonModeA_r13) ||
                                  !config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEB_VAL, &interval_DLHoppingConfigCommonModeA_r13_val) ||
                                  !config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEB, &interval_DLHoppingConfigCommonModeB_r13) ||
                                  !config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_INTERVAL_DLHOPPINGCONFIGCOMMONMODEB_VAL, &interval_DLHoppingConfigCommonModeB_r13_val))
                          {
                              AssertFatal(0,
                                          "Failed to parse eNB configuration file %s, enb %d  si_WindowLength_BR_r13, si_RepetitionPattern_r13, fdd_DownlinkOrTddSubframeBitmapBR_r13, fdd_UplinkSubframeBitmapBR_r13!\n",
                                          RC.config_file_name, i);

                          }






                  // SIB23 parameters ---------------------------------------------------------------------------------------------------------------------------





                      if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PDSCH_MAX_NUM_REPETITION_CE_MODE_B_R13, &pdsch_maxNumRepetitionCEmodeB_r13)) {
                          RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeB_r13[j] = CALLOC(1, sizeof(long));
                          if (!strcmp(pdsch_maxNumRepetitionCEmodeB_r13, "r192")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeB_r13[j] = 0;
                          } else if (!strcmp(pdsch_maxNumRepetitionCEmodeB_r13, "r256")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeB_r13[j] = 1;
                          } else if (!strcmp(pdsch_maxNumRepetitionCEmodeB_r13, "r384")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeB_r13[j] = 2;
                          } else if (!strcmp(pdsch_maxNumRepetitionCEmodeB_r13, "r512")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeB_r13[j] = 3;
                          } else if (!strcmp(pdsch_maxNumRepetitionCEmodeB_r13, "r768")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeB_r13[j] = 4;
                          } else if (!strcmp(pdsch_maxNumRepetitionCEmodeB_r13, "r1024")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeB_r13[j] = 5;
                          } else if (!strcmp(pdsch_maxNumRepetitionCEmodeB_r13, "r1536")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeB_r13[j] = 6;
                          } else if (!strcmp(pdsch_maxNumRepetitionCEmodeB_r13, "r2048")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeB_r13[j] = 7;
                          } else {
                              AssertFatal (0,
                                           "Failed to parse eNB configuration file %s, pdsch_maxNumRepetitionCEmodeB_r13 unknown value!\n",
                                           RC.config_file_name);
                          }
                          printf("DEBUGGING][KOGO][CHAR*]: pdsch_maxNumRepetitionCEmodeB_r13: %s - %d\n", pdsch_maxNumRepetitionCEmodeB_r13, *RRC_CONFIGURATION_REQ (msg_p).pdsch_maxNumRepetitionCEmodeB_r13[j]);
                      }


                      if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_MAX_NUM_REPETITION_CE_MODE_B_R13, &pusch_maxNumRepetitionCEmodeB_r13)) {
                          RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeB_r13[j] = CALLOC(1, sizeof(long));
                          if (!strcmp(pusch_maxNumRepetitionCEmodeB_r13, "r192")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeB_r13[j] =  0;
                          } else if (!strcmp(pusch_maxNumRepetitionCEmodeB_r13, "r256")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeB_r13[j] =  1;
                          } else if (!strcmp(pusch_maxNumRepetitionCEmodeB_r13, "r384")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeB_r13[j] =  2;
                          } else if (!strcmp(pusch_maxNumRepetitionCEmodeB_r13, "r512")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeB_r13[j] =  3;
                          } else if (!strcmp(pusch_maxNumRepetitionCEmodeB_r13, "r768")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeB_r13[j] =  4;
                          } else if (!strcmp(pusch_maxNumRepetitionCEmodeB_r13, "r1024")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeB_r13[j] =  5;
                          } else if (!strcmp(pusch_maxNumRepetitionCEmodeB_r13, "r1536")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeB_r13[j] =  6;
                          } else if (!strcmp(pusch_maxNumRepetitionCEmodeB_r13, "r2048")) {
                              *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeB_r13[j] =  7;
                          }  else {
                              AssertFatal (0,
                                           "Failed to parse eNB configuration file %s, pusch_maxNumRepetitionCEmodeB_r13 unknown value!\n",
                                           RC.config_file_name);
                          }
                          printf("DEBUGGING][KOGO][CHAR*]: pusch_maxNumRepetitionCEmodeB_r13: %s - %d\n", pusch_maxNumRepetitionCEmodeB_r13, *RRC_CONFIGURATION_REQ (msg_p).pusch_maxNumRepetitionCEmodeB_r13[j]);
                      }


                      if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_HOPPING_OFFSET_V1310, &pusch_HoppingOffset_v1310)) {
                             RRC_CONFIGURATION_REQ(msg_p).pusch_HoppingOffset_v1310[j] = CALLOC(1, sizeof(long));
                             *RRC_CONFIGURATION_REQ(msg_p).pusch_HoppingOffset_v1310[j] = pusch_HoppingOffset_v1310;
                             printf("DEBUGGING][KOGO][CHAR*]: pusch_HoppingOffset_v1310 %d\n", *RRC_CONFIGURATION_REQ (msg_p).pusch_HoppingOffset_v1310[j]);
                      }


                      if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL1, &pucch_NumRepetitionCE_Msg4_Level1_r13))
                      {
                          RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level1_r13[j] = CALLOC(1, sizeof(long));
                          ++cnt;
                          if (!strcmp(pucch_NumRepetitionCE_Msg4_Level1_r13, "n1")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level1_r13[j] = 0;
                          } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level1_r13, "n2")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level1_r13[j] = 1;
                          } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level1_r13, "n4")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level1_r13[j] = 2;
                          } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level1_r13, "n8")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level1_r13[j] = 3;
                          } else {
                              AssertFatal (0,
                                           "Failed to parse eNB configuration file %s, pucch_NumRepetitionCE_Msg4_Level1_r13 unknown value!\n",
                                           RC.config_file_name);

                          }
                          printf("DEBUGGING][KOGO][CHAR*]: pucch_NumRepetitionCE_Msg4_Level0_r13: %s - %d\n", pucch_NumRepetitionCE_Msg4_Level1_r13, *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level1_r13[j]);
                      }
                      else
                      {
                          RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level1_r13[j] = NULL;;
                      }


                      if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL2, &pucch_NumRepetitionCE_Msg4_Level2_r13))
                      {
                          RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level2_r13[j] = CALLOC(1, sizeof(long));
                          ++cnt;
                          if (!strcmp(pucch_NumRepetitionCE_Msg4_Level2_r13, "n4")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level2_r13[j] = 0;
                          } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level2_r13, "n8")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level2_r13[j] = 1;
                          } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level2_r13, "n16")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level2_r13[j] = 2;
                          } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level2_r13, "n32")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level2_r13[j] = 3;
                          } else {
                              AssertFatal (0,
                                           "Failed to parse eNB configuration file %s, pucch_NumRepetitionCE_Msg4_Level2_r13 unknown value!\n",
                                           RC.config_file_name);
                          }
                          printf("DEBUGGING][KOGO][CHAR*]: pucch_NumRepetitionCE_Msg4_Level2_r13: %s - %d\n", pucch_NumRepetitionCE_Msg4_Level2_r13, *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level2_r13[j]);
                      }
                      else
                      {
                          RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level2_r13[j] = NULL;
                      }


                      if (config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_NUM_REPETITION_CE_MSG4_LEVEL3, &pucch_NumRepetitionCE_Msg4_Level3_r13))
                      {
                          RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level3_r13[j] = CALLOC(1, sizeof(long));
                          ++cnt;
                          if (!strcmp(pucch_NumRepetitionCE_Msg4_Level3_r13, "n4")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level3_r13[j] = 0;
                          } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level3_r13, "n8")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level3_r13[j] = 1;
                          } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level3_r13, "n16")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level3_r13[j] = 2;
                          } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level3_r13, "n32")) {
                              *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level3_r13[j] = 3;
                          } else {
                              AssertFatal (0,
                                           "Failed to parse eNB configuration file %s, pucch_NumRepetitionCE_Msg4_Level3_r13 unknown value!\n",
                                           RC.config_file_name);
                          }
                          printf("DEBUGGING][KOGO][CHAR*]: pucch_NumRepetitionCE_Msg4_Level3_r13: %s - %d\n", pucch_NumRepetitionCE_Msg4_Level3_r13, *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level3_r13[j]);
                      }
                      else
                      {
                          RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level3_r13[j] = NULL;;
                      }


                      prach_parameters_ce_r13_list = config_setting_get_member(setting_br13, ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13);
                      int num_prach_parameters_ce_r13 = config_setting_length(prach_parameters_ce_r13_list);
                      RRC_CONFIGURATION_REQ (msg_p).prach_parameters_list_size[j] = num_prach_parameters_ce_r13;
                      int prach_parameters_index;
                      for (prach_parameters_index = 0; prach_parameters_index < num_prach_parameters_ce_r13; ++prach_parameters_index)
                      {
                          prach_parameters_ce_r13 = config_setting_get_elem(prach_parameters_ce_r13_list, prach_parameters_index);
                          if (!     (config_setting_lookup_int(prach_parameters_ce_r13, ENB_CONFIG_STRING_PRACH_CONFIG_INDEX_BR, &prach_config_index_br)
                                     && config_setting_lookup_int(prach_parameters_ce_r13, ENB_CONFIG_STRING_PRACH_FREQ_OFFSET_BR, &prach_freq_offset_br)
                                     && config_setting_lookup_string(prach_parameters_ce_r13, ENB_CONFIG_STRING_NUM_REPETITION_PREAMBLE_ATTEMPT_R13, &numRepetitionPerPreambleAttempt_r13)
                                     && config_setting_lookup_string(prach_parameters_ce_r13, ENB_CONFIG_STRING_MPDCCH_NUM_REPETITION_RA_R13, &mpdcch_NumRepetition_RA_r13)
                                     && config_setting_lookup_string(prach_parameters_ce_r13, ENB_CONFIG_STRING_PRACH_HOPPING_CONFIG_R13, &prach_HoppingConfig_r13) )
                                  )
                          {
                              AssertFatal (0,
                                           "Failed to parse eNB configuration file %s, prach_parameters_ce_r13_list %d!\n",
                                           RC.config_file_name, nb_cc++);
                          }

                          printf("[DEBUGGING][KOGO] : prach hopping config = %d\n", prach_HoppingConfig_r13);



                          max_available_narrow_band_list = config_setting_get_member(prach_parameters_ce_r13, ENB_CONFIG_STRING_MAX_AVAILABLE_NARROW_BAND);
                          int num_available_narrow_bands = config_setting_length(max_available_narrow_band_list);
                          RRC_CONFIGURATION_REQ (msg_p).max_available_narrow_band_size[j][prach_parameters_index] = num_available_narrow_bands;
                          int narrow_band_index;
                          for (narrow_band_index = 0; narrow_band_index < num_available_narrow_bands; narrow_band_index++)
                          {
                              max_available_narrow_band = config_setting_get_elem(max_available_narrow_band_list, narrow_band_index);
                              RRC_CONFIGURATION_REQ (msg_p).max_available_narrow_band[j][prach_parameters_index][narrow_band_index] = config_setting_get_int(max_available_narrow_band);
                          }

                      }


                n1_pucch_AN_info_r13_list = config_setting_get_member(setting_br13, ENB_CONFIG_STRING_N1_PUCCH_AN_INFO_LIST);

                int num_pucch_an_info = config_setting_length(n1_pucch_AN_info_r13_list);
                AssertFatal(cnt == num_pucch_an_info, "Num Repetition Count should be equal to pucch info count !!!");
                RRC_CONFIGURATION_REQ (msg_p).pucch_info_value_size[j] = num_pucch_an_info;
                int pucch_info_idx;
                for (pucch_info_idx = 0; pucch_info_idx < num_pucch_an_info; ++pucch_info_idx)
                {
                  n1_pucch_AN_info_r13 = config_setting_get_elem(n1_pucch_AN_info_r13_list, pucch_info_idx);
                  if (! (config_setting_lookup_int(n1_pucch_AN_info_r13, ENB_CONFIG_STRING_PUCCH_INFO_VALUE, &pucch_info_value)) )
                  {
                    AssertFatal (0,
                        "Failed to parse eNB configuration file %s, n1_pucch_AN_info_list_r13_list %d!\n",
                        RC.config_file_name, nb_cc++);
                  }

                  RRC_CONFIGURATION_REQ (msg_p).pucch_info_value[j][pucch_info_idx] = pucch_info_value;
                }

                puts("---------------------------------------------------------------------------------------------------");

                setting_freq_hoppingParameters_r13 = config_setting_get_member(setting_br13, ENB_CONFIG_STRING_FREQ_HOPPING_PARAMETERS_R13);
                if (setting_freq_hoppingParameters_r13 != NULL)
                {
                    RRC_CONFIGURATION_REQ(msg_p).sib2_freq_hoppingParameters_r13_exists[j] = TRUE;
                    if (config_setting_lookup_string(setting_freq_hoppingParameters_r13, ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_NB_R13, &sib2_mpdcch_pdsch_hoppingNB_r13))
                    {

                        RRC_CONFIGURATION_REQ(msg_p).sib2_mpdcch_pdsch_hoppingNB_r13[j] = CALLOC(1, sizeof(long));
                        if (!strcmp(sib2_mpdcch_pdsch_hoppingNB_r13, "nb2")) {
                            *RRC_CONFIGURATION_REQ(msg_p).sib2_mpdcch_pdsch_hoppingNB_r13[j] = 0;
                        } else if (!strcmp(sib2_mpdcch_pdsch_hoppingNB_r13, "nb4")) {
                            *RRC_CONFIGURATION_REQ(msg_p).sib2_mpdcch_pdsch_hoppingNB_r13[j] = 1;
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, sib2_mpdcch_pdsch_hoppingNB_r13 unknown value !!\n",
                                         RC.config_file_name);
                        }
                        printf("[DEBUGGING][KOGO][CHAR*]: sib2_mpdcch_pdsch_hoppingNB_r13: %s - %d\n", sib2_mpdcch_pdsch_hoppingNB_r13, *RRC_CONFIGURATION_REQ (msg_p).sib2_mpdcch_pdsch_hoppingNB_r13[j]);
                    }
                    else
                    {
                       RRC_CONFIGURATION_REQ(msg_p).sib2_mpdcch_pdsch_hoppingNB_r13[j] = NULL;
                    }


                    if (config_setting_lookup_string(setting_freq_hoppingParameters_r13, ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_A_R13, &sib2_interval_DLHoppingConfigCommonModeA_r13))
                    {

                        RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13[j] = CALLOC(1, sizeof(long));
                        config_setting_lookup_string(setting_freq_hoppingParameters_r13, ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_A_R13_VAL, &sib2_interval_DLHoppingConfigCommonModeA_r13_val);
                        if (!strcmp(sib2_interval_DLHoppingConfigCommonModeA_r13, "FDD")) {
                            *RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13[j] = 0;
                            if (!strcmp(sib2_interval_DLHoppingConfigCommonModeA_r13_val, "int1")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13_val[j] = 0;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeA_r13_val, "int2")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13_val[j] = 1;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeA_r13_val, "int4")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13_val[j] = 2;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeA_r13_val, "int8")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13_val[j] = 3;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, sib2_interval_DLHoppingConfigCommonModeA_r13_val unknown value !!\n",
                                             RC.config_file_name);
                            }
                        } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeA_r13, "TDD")) {
                            *RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13[j] = 1;
                            if (!strcmp(sib2_interval_DLHoppingConfigCommonModeA_r13_val, "int1")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13_val[j] = 0;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeA_r13_val, "int5")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13_val[j] = 1;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeA_r13_val, "int10")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13_val[j] = 2;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeA_r13_val, "int20")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13_val[j] = 3;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, sib2_interval_DLHoppingConfigCommonModeA_r13_val unknown value !!\n",
                                             RC.config_file_name);
                            }
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, sib2_interval_DLHoppingConfigCommonModeA_r13 unknown value !!\n",
                                         RC.config_file_name);
                        }
                        printf("[DEBUGGING][KOGO][CHAR*]: sib2_interval_DLHoppingConfigCommonModeA_r13: %s - %d\n", sib2_interval_DLHoppingConfigCommonModeA_r13, *RRC_CONFIGURATION_REQ (msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13[j]);
                        printf("[DEBUGGING][KOGO][CHAR*]: sib2_interval_DLHoppingConfigCommonModeA_r13_val: %s - %d\n", sib2_interval_DLHoppingConfigCommonModeA_r13_val, RRC_CONFIGURATION_REQ (msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13_val[j]);

                    }
                    else
                    {
                         RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeA_r13[j] = NULL;
                    }


                    if (config_setting_lookup_string(setting_freq_hoppingParameters_r13, ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_B_R13, &sib2_interval_DLHoppingConfigCommonModeB_r13))
                    {

                        RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13[j] = CALLOC(1, sizeof(long));
                        config_setting_lookup_string(setting_freq_hoppingParameters_r13, ENB_CONFIG_STRING_INTERVAL_DL_HOPPING_CONFIG_COMMON_MODE_B_R13_VAL, &sib2_interval_DLHoppingConfigCommonModeB_r13_val);
                        if (!strcmp(sib2_interval_DLHoppingConfigCommonModeB_r13, "FDD")) {
                            *RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13[j] = 0;
                            if (!strcmp(sib2_interval_DLHoppingConfigCommonModeB_r13_val, "int2")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13_val[j] = 0;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeB_r13_val, "int4")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13_val[j] = 1;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeB_r13_val, "int8")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13_val[j] = 2;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeB_r13_val, "int16")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13_val[j] = 3;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, sib2_interval_DLHoppingConfigCommonModeB_r13_val unknown value !!\n",
                                             RC.config_file_name);
                            }
                        } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeB_r13, "TDD")) {
                            *RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13[j] = 1;
                            if (!strcmp(sib2_interval_DLHoppingConfigCommonModeB_r13_val, "int5")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13_val[j] = 0;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeB_r13_val, "int10")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13_val[j] = 1;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeB_r13_val, "int20")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13_val[j] = 2;
                            } else if (!strcmp(sib2_interval_DLHoppingConfigCommonModeB_r13_val, "int40")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13_val[j] = 3;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, sib2_interval_DLHoppingConfigCommonModeB_r13_val unknown value !!\n",
                                             RC.config_file_name);
                            }
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, sib2_interval_DLHoppingConfigCommonModeB_r13 unknown value !!\n",
                                         RC.config_file_name);
                        }
                        printf("[DEBUGGING][KOGO][CHAR*]: sib2_interval_DLHoppingConfigCommonModeB_r13: %s - %d\n", sib2_interval_DLHoppingConfigCommonModeB_r13, *RRC_CONFIGURATION_REQ (msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13[j]);
                        printf("[DEBUGGING][KOGO][CHAR*]: sib2_interval_DLHoppingConfigCommonModeB_r13_val: %s - %d\n", sib2_interval_DLHoppingConfigCommonModeB_r13_val, RRC_CONFIGURATION_REQ (msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13_val[j]);
                    }
                    else
                    {
                         RRC_CONFIGURATION_REQ(msg_p).sib2_interval_DLHoppingConfigCommonModeB_r13[j] = NULL;
                    }






                    if (config_setting_lookup_string(setting_freq_hoppingParameters_r13, ENB_CONFIG_STRING_INTERVAL_UL_HOPPING_CONFIG_COMMON_MODE_B_R13, &sib2_interval_ULHoppingConfigCommonModeB_r13))
                    {

                        RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13[j] = CALLOC(1, sizeof(long));
                        config_setting_lookup_string(setting_freq_hoppingParameters_r13, ENB_CONFIG_STRING_INTERVAL_UL_HOPPING_CONFIG_COMMON_MODE_B_R13_VAL, &sib2_interval_ULHoppingConfigCommonModeB_r13_val);
                        if (!strcmp(sib2_interval_ULHoppingConfigCommonModeB_r13, "FDD")) {
                            *RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13[j] = 0;
                            if (!strcmp(sib2_interval_ULHoppingConfigCommonModeB_r13_val, "int2")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13_val[j] = 0;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeB_r13_val, "int4")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13_val[j] = 1;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeB_r13_val, "int8")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13_val[j] = 2;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeB_r13_val, "int16")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13_val[j] = 3;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, sib2_interval_ULHoppingConfigCommonModeB_r13_val unknown value !!\n",
                                             RC.config_file_name);
                            }
                        } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13, "TDD")) {
                            *RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13[j] = 1;
                            if (!strcmp(sib2_interval_ULHoppingConfigCommonModeB_r13_val, "int5")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13_val[j] = 0;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeB_r13_val, "int10")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13_val[j] = 1;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeB_r13_val, "int20")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13_val[j] = 2;
                            } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeB_r13_val, "int40")) {
                                RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13_val[j] = 3;
                            } else {
                                AssertFatal (0,
                                             "Failed to parse eNB configuration file %s, sib2_interval_ULHoppingConfigCommonModeB_r13_val unknown value !!\n",
                                             RC.config_file_name);
                            }
                        } else {
                            AssertFatal (0,
                                         "Failed to parse eNB configuration file %s, sib2_interval_ULHoppingConfigCommonModeA_r13 unknown value !!\n",
                                         RC.config_file_name);
                        }
                        printf("[DEBUGGING][KOGO][CHAR*]: sib2_interval_ULHoppingConfigCommonModeB_r13: %s - %d\n", sib2_interval_ULHoppingConfigCommonModeB_r13, *RRC_CONFIGURATION_REQ (msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13[j]);
                    }
                    else
                    {
                         RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13[j] = NULL;
                    }
                    printf("[DEBUGGING][KOGO][CHAR*]: sib2_interval_ULHoppingConfigCommonModeB_r13_val: %s - %d\n", sib2_interval_ULHoppingConfigCommonModeB_r13_val, RRC_CONFIGURATION_REQ (msg_p).sib2_interval_ULHoppingConfigCommonModeB_r13_val[j]);




                    if (config_setting_lookup_int(setting_freq_hoppingParameters_r13, ENB_CONFIG_STRING_MPDCCH_PDSCH_HOPPING_OFFSET_R13, &sib2_mpdcch_pdsch_hoppingOffset_r13))
                    {

                        RRC_CONFIGURATION_REQ(msg_p).sib2_mpdcch_pdsch_hoppingOffset_r13[j] = CALLOC(1, sizeof(long));
                        *RRC_CONFIGURATION_REQ(msg_p).sib2_mpdcch_pdsch_hoppingOffset_r13[j] = sib2_mpdcch_pdsch_hoppingOffset_r13;
                    }
                    else
                    {
                       RRC_CONFIGURATION_REQ(msg_p).sib2_mpdcch_pdsch_hoppingOffset_r13[j] = NULL;
                    }


                }
                else
                {
                    RRC_CONFIGURATION_REQ(msg_p).sib2_freq_hoppingParameters_r13_exists[j] = FALSE;
                }


                // Rel8 RadioResourceConfigCommon Parameters
                if (!(config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_FRAME_TYPE, &frame_type)

                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PRACH_ROOT, &prach_root)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PRACH_CONFIG_INDEX, &prach_config_index)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PRACH_HIGH_SPEED, &prach_high_speed)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PRACH_ZERO_CORRELATION, &prach_zero_correlation)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PRACH_FREQ_OFFSET, &prach_freq_offset)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTA_SHIFT, &pucch_delta_shift)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUCCH_NRB_CQI, &pucch_nRB_CQI)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUCCH_NCS_AN, &pucch_nCS_AN)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PDSCH_RS_EPRE, &pdsch_referenceSignalPower)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PDSCH_PB, &pdsch_p_b)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_N_SB, &pusch_n_SB)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE, &pusch_hoppingMode)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_HOPPINGOFFSET, &pusch_hoppingOffset)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM, &pusch_enable64QAM)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN, &pusch_groupHoppingEnabled)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_GROUP_ASSIGNMENT, &pusch_groupAssignment)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN, &pusch_sequenceHoppingEnabled)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_NDMRS1, &pusch_nDMRS1)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PHICH_DURATION, &phich_duration)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PHICH_RESOURCE, &phich_resource)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_SRS_ENABLE, &srs_enable)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUSCH_PO_NOMINAL, &pusch_p0_Nominal)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUSCH_ALPHA, &pusch_alpha)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_PUCCH_PO_NOMINAL, &pucch_p0_Nominal)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_MSG3_DELTA_PREAMBLE, &msg3_delta_Preamble)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1, &pucch_deltaF_Format1)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT1b, &pucch_deltaF_Format1b)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2, &pucch_deltaF_Format2)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2A, &pucch_deltaF_Format2a)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PUCCH_DELTAF_FORMAT2B, &pucch_deltaF_Format2b)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_RACH_NUM_RA_PREAMBLES, &rach_numberOfRA_Preambles)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_RACH_PREAMBLESGROUPACONFIG, &rach_preamblesGroupAConfig)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_POWERRAMPINGSTEP, &rach_powerRampingStep)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_PREAMBLEINITIALRECEIVEDTARGETPOWER, &rach_preambleInitialReceivedTargetPower)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_PREAMBLETRANSMAX, &rach_preambleTransMax)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_RARESPONSEWINDOWSIZE, &rach_raResponseWindowSize)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_MACCONTENTIONRESOLUTIONTIMER, &rach_macContentionResolutionTimer)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_MAXHARQMSG3TX, &rach_maxHARQ_Msg3Tx)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PCCH_DEFAULT_PAGING_CYCLE,  &pcch_defaultPagingCycle)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_PCCH_NB,  &pcch_nB)
                      && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_BCCH_MODIFICATIONPERIODCOEFF,  &bcch_modificationPeriodCoeff)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_UETIMERS_T300,  &ue_TimersAndConstants_t300)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_UETIMERS_T301,  &ue_TimersAndConstants_t301)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_UETIMERS_T310,  &ue_TimersAndConstants_t310)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_UETIMERS_T311,  &ue_TimersAndConstants_t311)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_UETIMERS_N310,  &ue_TimersAndConstants_n310)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_UETIMERS_N311,  &ue_TimersAndConstants_n311)
                      && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_UE_TRANSMISSION_MODE,  &ue_TransmissionMode)
                      )) {
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, Component Carrier %d!\n",
                                 RC.config_file_name, nb_cc++);
                    continue; // FIXME this prevents segfaults below, not sure what happens after function exit
                }

                if (RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_enable == TRUE) {
                    if (!(config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_SRS_BANDWIDTH_CONFIG, &srs_BandwidthConfig)
                          && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_SRS_SUBFRAME_CONFIG, &srs_SubframeConfig)
                          && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_SRS_ACKNACKST_CONFIG, &srs_ackNackST)
                          && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_SRS_MAXUPPTS, &srs_MaxUpPts)
                          ))
                        AssertFatal(0,
                                    "Failed to parse eNB configuration file %s, enb %d unknown values for srs_BandwidthConfig, srs_SubframeConfig, srs_ackNackST, srs_MaxUpPts\n",
                                    RC.config_file_name, i);

                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_BandwidthConfig = srs_BandwidthConfig;

                    if ((srs_BandwidthConfig < 0) || (srs_BandwidthConfig >7))
                        AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value %d for srs_BandwidthConfig choice: 0...7\n",
                                     RC.config_file_name, i, srs_BandwidthConfig);

                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_SubframeConfig = srs_SubframeConfig;

                    if ((srs_SubframeConfig<0) || (srs_SubframeConfig>15))
                        AssertFatal (0,
                                     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
                                     RC.config_file_name, i, srs_SubframeConfig);

                    if (strcmp(srs_ackNackST, "ENABLE") == 0) {
                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_ackNackST = TRUE;
                    }
                    else if (strcmp(srs_ackNackST, "DISABLE") == 0) {
                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_ackNackST = FALSE;
                    }
                    else
                        AssertFatal(0,
                                    "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                                    RC.config_file_name, i, srs_ackNackST);

                    if (strcmp(srs_MaxUpPts, "ENABLE") == 0) {
                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_MaxUpPts = TRUE;
                    }
                    else if (strcmp(srs_MaxUpPts, "DISABLE") == 0) {
                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].srs_MaxUpPts = FALSE;
                    }
                    else
                        AssertFatal(0,
                                    "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
                                    RC.config_file_name, i, srs_MaxUpPts);
                }





                if (!strcmp(rach_numberOfRA_Preambles, "n4")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (4 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n8")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (8 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n12")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (12 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n16")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (16 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n20")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (20 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n24")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (24 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n28")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (28 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n32")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (32 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n36")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (36 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n40")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (40 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n44")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (44 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n48")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (48 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n52")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (52 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n56")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (56 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n60")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (60 / 4) - 1;
                } else if (!strcmp(rach_numberOfRA_Preambles, "n64")) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (64 / 4) - 1;
                } else {
                    AssertFatal(0,
                                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
                                RC.config_file_name, i, rach_numberOfRA_Preambles);

                }


                printf("[DEBUGGING][KOGO][CHAR*] : rach_numberOfRA_Preambles_br = %d\n", RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles);

//                RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles = (rach_numberOfRA_Preambles_br/ 4) - 1;


                if (strcmp(rach_preamblesGroupAConfig, "ENABLE") == 0) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_preamblesGroupAConfig= TRUE;

                    if (!(config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_SIZEOFRA_PREAMBLESGROUPA, &rach_sizeOfRA_PreamblesGroupA)
                          && config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_RACH_MESSAGESIZEGROUPA, &rach_messageSizeGroupA)
                          && config_setting_lookup_string(setting_br13, ENB_CONFIG_STRING_RACH_MESSAGEPOWEROFFSETGROUPB, &rach_messagePowerOffsetGroupB)))
                        AssertFatal (0,
                                     "Failed to parse eNB configuration file %s, enb %d  rach_sizeOfRA_PreamblesGroupA, messageSizeGroupA,messagePowerOffsetGroupB!\n",
                                     RC.config_file_name, i);

                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_sizeOfRA_PreamblesGroupA= (rach_sizeOfRA_PreamblesGroupA/4)-1;

//                    if ((rach_numberOfRA_Preambles <4) || (rach_numberOfRA_Preambles>60) || ((rach_numberOfRA_Preambles&3)!=0))
//                        AssertFatal (0,
//                                     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_sizeOfRA_PreamblesGroupA choice: 4,8,12,...,60!\n",
//                                     RC.config_file_name, i, rach_sizeOfRA_PreamblesGroupA);


                    switch (rach_messageSizeGroupA) {
                    case 56:
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b56;
                        break;

                    case 144:
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b144;
                        break;

                    case 208:
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b208;
                        break;

                    case 256:
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b256;
                        break;

                    default:
                        AssertFatal (0,
                                     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
                                     RC.config_file_name, i, rach_messageSizeGroupA);
                        break;
                    }

                    if (strcmp(rach_messagePowerOffsetGroupB,"minusinfinity")==0) {
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
                    }

                    else if (strcmp(rach_messagePowerOffsetGroupB,"dB0")==0) {
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
                    }

                    else if (strcmp(rach_messagePowerOffsetGroupB,"dB5")==0) {
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
                    }

                    else if (strcmp(rach_messagePowerOffsetGroupB,"dB8")==0) {
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
                    }

                    else if (strcmp(rach_messagePowerOffsetGroupB,"dB10")==0) {
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
                    }

                    else if (strcmp(rach_messagePowerOffsetGroupB,"dB12")==0) {
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
                    }

                    else if (strcmp(rach_messagePowerOffsetGroupB,"dB15")==0) {
                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
                    }

                    else if (strcmp(rach_messagePowerOffsetGroupB,"dB18")==0) {
                        RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
                    }
                    else
                        AssertFatal(0,
                                    "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
                                    RC.config_file_name, i, rach_messagePowerOffsetGroupB);

                }
                else if (strcmp(rach_preamblesGroupAConfig, "DISABLE") == 0) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].rach_preamblesGroupAConfig= FALSE;
                }
                else
                    AssertFatal(0,
                                "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
                                RC.config_file_name, i, rach_preamblesGroupAConfig);

                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleInitialReceivedTargetPower= (rach_preambleInitialReceivedTargetPower+120)/2;

                if ((rach_preambleInitialReceivedTargetPower<-120) || (rach_preambleInitialReceivedTargetPower>-90) || ((rach_preambleInitialReceivedTargetPower&1)!=0))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
                                 RC.config_file_name, i, rach_preambleInitialReceivedTargetPower);


                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_powerRampingStep = rach_powerRampingStep / 2;

                if ((rach_powerRampingStep<0) || (rach_powerRampingStep>6) || ((rach_powerRampingStep&1)!=0))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
                                 RC.config_file_name, i, rach_powerRampingStep);



                switch (rach_preambleTransMax) {

                case 3:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n3;
                    break;

                case 4:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n4;
                    break;

                case 5:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n5;
                    break;

                case 6:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n6;
                    break;

                case 7:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n7;
                    break;

                case 8:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n8;
                    break;

                case 10:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n10;
                    break;

                case 20:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n20;
                    break;

                case 50:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n50;
                    break;

                case 100:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n100;
                    break;

                case 200:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n200;
                    break;


                default:
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
                                 RC.config_file_name, i, rach_preambleTransMax);
                    break;
                }

                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_raResponseWindowSize=  (rach_raResponseWindowSize==10)?7:rach_raResponseWindowSize-2;

                if ((rach_raResponseWindowSize<0)||(rach_raResponseWindowSize==9)||(rach_raResponseWindowSize>10))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
                                 RC.config_file_name, i, rach_preambleTransMax);


                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_macContentionResolutionTimer= (rach_macContentionResolutionTimer/8)-1;

                if ((rach_macContentionResolutionTimer<8) || (rach_macContentionResolutionTimer>64) || ((rach_macContentionResolutionTimer&7)!=0))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
                                 RC.config_file_name, i, rach_preambleTransMax);

                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_maxHARQ_Msg3Tx= rach_maxHARQ_Msg3Tx;

                if ((rach_maxHARQ_Msg3Tx<0) || (rach_maxHARQ_Msg3Tx>8))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
                                 RC.config_file_name, i, rach_preambleTransMax);


                if (!strcmp(pcch_defaultPagingCycle, "rf32")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf32;
                } else if (!strcmp(pcch_defaultPagingCycle, "rf64")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf64;
                } else if (!strcmp(pcch_defaultPagingCycle, "rf128")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf128;
                } else if (!strcmp(pcch_defaultPagingCycle, "rf256")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle = PCCH_Config__defaultPagingCycle_rf256;
                } else {
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
                                 RC.config_file_name, i, pcch_defaultPagingCycle);
                }

                printf("[DEBUGGING][KOGO][CHAR*] : pcch_defaultPagingCycle_br = %d\n", RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle);

                if (strcmp(pcch_nB, "fourT") == 0) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_fourT;
                }
                else if (strcmp(pcch_nB, "twoT") == 0) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_twoT;
                }
                else if (strcmp(pcch_nB, "oneT") == 0) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneT;
                }
                else if (strcmp(pcch_nB, "halfT") == 0) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_halfT;
                }
                else if (strcmp(pcch_nB, "quarterT") == 0) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_quarterT;
                }
                else if (strcmp(pcch_nB, "oneEighthT") == 0) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneEighthT;
                }
                else if (strcmp(pcch_nB, "oneSixteenthT") == 0) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneSixteenthT;
                }
                else if (strcmp(pcch_nB, "oneThirtySecondT") == 0) {
                    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneThirtySecondT;
                }
                else
                    AssertFatal(0,
                                "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
                                RC.config_file_name, i, pcch_defaultPagingCycle);



                switch (bcch_modificationPeriodCoeff) {
                case 2:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n2;
                    break;

                case 4:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n4;
                    break;

                case 8:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n8;
                    break;

                case 16:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff = BCCH_Config__modificationPeriodCoeff_n16;
                    break;

                default:
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
                                 RC.config_file_name, i, bcch_modificationPeriodCoeff);

                    break;
                }


                if (!strcmp(ue_TimersAndConstants_t300, "ms100")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms100;
                } else if (!strcmp(ue_TimersAndConstants_t300, "ms200")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms200;
                } else if (!strcmp(ue_TimersAndConstants_t300, "ms300")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms300;
                } else if (!strcmp(ue_TimersAndConstants_t300, "ms400")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms400;
                } else if (!strcmp(ue_TimersAndConstants_t300, "ms600")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms600;
                } else if (!strcmp(ue_TimersAndConstants_t300, "ms1000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms1000;
                } else if (!strcmp(ue_TimersAndConstants_t300, "ms1500")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms1500;
                } else if (!strcmp(ue_TimersAndConstants_t300, "ms2000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300 = UE_TimersAndConstants__t300_ms2000;
                } else {
                    AssertFatal (0,
                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t300 unknown value !!",
                        RC.config_file_name, i, ue_TimersAndConstants_t300);
                }

                printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_t300 : %s - %d\n", ue_TimersAndConstants_t300, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300);

                if (!strcmp(ue_TimersAndConstants_t301, "ms100")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms100;
                } else if (!strcmp(ue_TimersAndConstants_t301, "ms200")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms200;
                } else if (!strcmp(ue_TimersAndConstants_t301, "ms300")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms300;
                } else if (!strcmp(ue_TimersAndConstants_t301, "ms400")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms400;
                } else if (!strcmp(ue_TimersAndConstants_t301, "ms600")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms600;
                } else if (!strcmp(ue_TimersAndConstants_t301, "ms1000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms1000;
                } else if (!strcmp(ue_TimersAndConstants_t301, "ms1500")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms1500;
                } else if (!strcmp(ue_TimersAndConstants_t301, "ms2000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= UE_TimersAndConstants__t301_ms2000;
                } else {
                    AssertFatal (0,
                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t301 !!",
                        RC.config_file_name, i, ue_TimersAndConstants_t301);
                }

                printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_t301 : %s - %d\n", ue_TimersAndConstants_t301, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301);

                if (!strcmp(ue_TimersAndConstants_t310, "ms0")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms0;
                } else if (!strcmp(ue_TimersAndConstants_t310, "ms50")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms50;
                } else if (!strcmp(ue_TimersAndConstants_t310, "ms100")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms100;
                } else if (!strcmp(ue_TimersAndConstants_t310, "ms200")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms200;
                } else if (!strcmp(ue_TimersAndConstants_t310, "ms500")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms500;
                } else if (!strcmp(ue_TimersAndConstants_t310, "ms1000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms1000;
                } else if (!strcmp(ue_TimersAndConstants_t310, "ms2000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310 = UE_TimersAndConstants__t310_ms2000;
                } else {
                    AssertFatal (0,
                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t310 !!",
                        RC.config_file_name, i, ue_TimersAndConstants_t310);
                }
                printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_t310 : %s - %d\n", ue_TimersAndConstants_t310, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310);

                if (!strcmp(ue_TimersAndConstants_t311, "ms1000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms1000;
                } else if (!strcmp(ue_TimersAndConstants_t311, "ms3000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms3000;
                } else if (!strcmp(ue_TimersAndConstants_t311, "ms5000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms5000;
                } else if (!strcmp(ue_TimersAndConstants_t311, "ms10000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms10000;
                } else if (!strcmp(ue_TimersAndConstants_t311, "ms15000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms15000;
                } else if (!strcmp(ue_TimersAndConstants_t311, "ms20000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms20000;
                } else if (!strcmp(ue_TimersAndConstants_t311, "ms30000")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311 = UE_TimersAndConstants__t311_ms30000;
                } else {
                    AssertFatal (0,
                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 !!",
                        RC.config_file_name, i, ue_TimersAndConstants_t311);
                }

                printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_t311 : %s - %d\n", ue_TimersAndConstants_t311, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311);


                if (!strcmp(ue_TimersAndConstants_n310, "n1")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n1;
                } else if (!strcmp(ue_TimersAndConstants_n310, "n2")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n2;
                } else if (!strcmp(ue_TimersAndConstants_n310, "n3")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n3;
                } else if (!strcmp(ue_TimersAndConstants_n310, "n4")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n4;
                } else if (!strcmp(ue_TimersAndConstants_n310, "n6")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n6;
                } else if (!strcmp(ue_TimersAndConstants_n310, "n8")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n8;
                } else if (!strcmp(ue_TimersAndConstants_n310, "n10")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n10;
                } else if (!strcmp(ue_TimersAndConstants_n310, "n20")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310 = UE_TimersAndConstants__n310_n20;
                } else {
                    AssertFatal (0,
                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_n310 !!",
                        RC.config_file_name, i, ue_TimersAndConstants_n310);
                }

                printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_n310 : %s - %d\n", ue_TimersAndConstants_n310, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310);


                if (!strcmp(ue_TimersAndConstants_n311, "n1")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n1;
                } else if (!strcmp(ue_TimersAndConstants_n311, "n2")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n2;
                } else if (!strcmp(ue_TimersAndConstants_n311, "n3")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n3;
                } else if (!strcmp(ue_TimersAndConstants_n311, "n4")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n4;
                } else if (!strcmp(ue_TimersAndConstants_n311, "n5")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n5;
                } else if (!strcmp(ue_TimersAndConstants_n311, "n6")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n6;
                } else if (!strcmp(ue_TimersAndConstants_n311, "n8")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n8;
                } else if (!strcmp(ue_TimersAndConstants_n311, "n10")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311 = UE_TimersAndConstants__n311_n10;
                } else {
                    AssertFatal (0,
                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_n311!!",
                        RC.config_file_name, i, ue_TimersAndConstants_n311);
                }

                printf("[DEBUGGING][KOGO][CHAR*]: ue_TimersAndConstants_n311 : %s - %d\n", ue_TimersAndConstants_n311, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311);


                if (!strcmp(ue_TransmissionMode, "tm1")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm1;
                } else if (!strcmp(ue_TransmissionMode, "tm2")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm2;
                } else if (!strcmp(ue_TransmissionMode, "tm3")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm3;
                } else if (!strcmp(ue_TransmissionMode, "tm4")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm4;
                } else if (!strcmp(ue_TransmissionMode, "tm5")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm5;
                } else if (!strcmp(ue_TransmissionMode, "tm6")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm6;
                } else if (!strcmp(ue_TransmissionMode, "tm7")) {
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode = AntennaInfoDedicated__transmissionMode_tm7;
                } else {
                    AssertFatal (0,
                        "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode !!",
                        RC.config_file_name, i, ue_TransmissionMode);
                }

                printf("[DEBUGGING][KOGO][CHAR*]: ue_TransmissionMode : %s - %d\n", ue_TransmissionMode, RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode);



                if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_HYPERSFN, &hyperSFN_r13))
                {
                    RRC_CONFIGURATION_REQ(msg_p).hyperSFN_r13[j]= calloc(1, sizeof(uint16_t));
                    *RRC_CONFIGURATION_REQ(msg_p).hyperSFN_r13[j]= (uint16_t)hyperSFN_r13;
                }
                else
                    RRC_CONFIGURATION_REQ(msg_p).hyperSFN_r13[j]= NULL;

                if (config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_EDRX_ALLOWED, &eDRX_Allowed_r13))
                {
                    RRC_CONFIGURATION_REQ(msg_p).eDRX_Allowed_r13[j]= calloc(1, sizeof(long));
                    *RRC_CONFIGURATION_REQ(msg_p).eDRX_Allowed_r13[j]= eDRX_Allowed_r13;
                }
                else
                    RRC_CONFIGURATION_REQ(msg_p).eDRX_Allowed_r13[j]= NULL;


                bool fdd_uplink_exist = config_setting_lookup_int(setting_br13, ENB_CONFIG_STRING_FDD_ULSUBFRAMEBITMAPBR, &fdd_UplinkSubframeBitmapBR_r13);
                if (fdd_uplink_exist)
                {
                    RRC_CONFIGURATION_REQ(msg_p).fdd_UplinkSubframeBitmapBR_r13[j] = CALLOC(1, sizeof(BOOLEAN_t));
                    *RRC_CONFIGURATION_REQ(msg_p).fdd_UplinkSubframeBitmapBR_r13[j] = fdd_UplinkSubframeBitmapBR_r13;
                }
                else
                {
                    RRC_CONFIGURATION_REQ(msg_p).fdd_UplinkSubframeBitmapBR_r13[j] = NULL;;
                }

                  }

                  else
                  RRC_CONFIGURATION_REQ(msg_p).bandwidthReducedAccessRelatedInfo_r13[j] = FALSE;


                /////SIB2 Parameters


              }
              else
              {
                  RRC_CONFIGURATION_REQ(msg_p).schedulingInfoSIB1_BR_r13[j] = 0;
              }*/


#endif
                        //sprintf(brpath,"%s.%s.[%i].%s",enbpath,ENB_CONFIG_STRING_COMPONENT_CARRIERS,ENB_CONFIG_STRING_BR_PARAMETERS,j);
                        //config_get( CCsParams,sizeof(CCsParams)/sizeof(paramdef_t),ccspath);

                    }
                }
                char srb1path[MAX_OPTNAME_SIZE*2 + 8];
                sprintf(srb1path,"%s.%s",enbpath,ENB_CONFIG_STRING_SRB1);
                int npar = config_get( SRB1Params,sizeof(SRB1Params)/sizeof(paramdef_t), srb1path);
                if (npar == sizeof(SRB1Params)/sizeof(paramdef_t)) {
                    switch (srb1_max_retx_threshold) {
                    case 1:
                        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t1;
                        break;

                    case 2:
                        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t2;
                        break;

                    case 3:
                        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t3;
                        break;

                    case 4:
                        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t4;
                        break;

                    case 6:
                        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t6;
                        break;

                    case 8:
                        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t8;
                        break;

                    case 16:
                        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t16;
                        break;

                    case 32:
                        rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t32;
                        break;

                    default:
                        AssertFatal (0,
                                     "Bad config value when parsing eNB configuration file %s, enb %d  srb1_max_retx_threshold %u!\n",
                                     RC.config_file_name, i, srb1_max_retx_threshold);
                    }


                    switch (srb1_poll_pdu) {
                    case 4:
                        rrc->srb1_poll_pdu = PollPDU_p4;
                        break;

                    case 8:
                        rrc->srb1_poll_pdu = PollPDU_p8;
                        break;

                    case 16:
                        rrc->srb1_poll_pdu = PollPDU_p16;
                        break;

                    case 32:
                        rrc->srb1_poll_pdu = PollPDU_p32;
                        break;

                    case 64:
                        rrc->srb1_poll_pdu = PollPDU_p64;
                        break;

                    case 128:
                        rrc->srb1_poll_pdu = PollPDU_p128;
                        break;

                    case 256:
                        rrc->srb1_poll_pdu = PollPDU_p256;
                        break;

                    default:
                        if (srb1_poll_pdu >= 10000)
                            rrc->srb1_poll_pdu = PollPDU_pInfinity;
                        else
                            AssertFatal (0,
                                         "Bad config value when parsing eNB configuration file %s, enb %d  srb1_poll_pdu %u!\n",
                                         RC.config_file_name, i, srb1_poll_pdu);
                    }

                    rrc->srb1_poll_byte             = srb1_poll_byte;

                    switch (srb1_poll_byte) {
                    case 25:
                        rrc->srb1_poll_byte = PollByte_kB25;
                        break;

                    case 50:
                        rrc->srb1_poll_byte = PollByte_kB50;
                        break;

                    case 75:
                        rrc->srb1_poll_byte = PollByte_kB75;
                        break;

                    case 100:
                        rrc->srb1_poll_byte = PollByte_kB100;
                        break;

                    case 125:
                        rrc->srb1_poll_byte = PollByte_kB125;
                        break;

                    case 250:
                        rrc->srb1_poll_byte = PollByte_kB250;
                        break;

                    case 375:
                        rrc->srb1_poll_byte = PollByte_kB375;
                        break;

                    case 500:
                        rrc->srb1_poll_byte = PollByte_kB500;
                        break;

                    case 750:
                        rrc->srb1_poll_byte = PollByte_kB750;
                        break;

                    case 1000:
                        rrc->srb1_poll_byte = PollByte_kB1000;
                        break;

                    case 1250:
                        rrc->srb1_poll_byte = PollByte_kB1250;
                        break;

                    case 1500:
                        rrc->srb1_poll_byte = PollByte_kB1500;
                        break;

                    case 2000:
                        rrc->srb1_poll_byte = PollByte_kB2000;
                        break;

                    case 3000:
                        rrc->srb1_poll_byte = PollByte_kB3000;
                        break;

                    default:
                        if (srb1_poll_byte >= 10000)
                            rrc->srb1_poll_byte = PollByte_kBinfinity;
                        else
                            AssertFatal (0,
                                         "Bad config value when parsing eNB configuration file %s, enb %d  srb1_poll_byte %u!\n",
                                         RC.config_file_name, i, srb1_poll_byte);
                    }

                    if (srb1_timer_poll_retransmit <= 250) {
                        rrc->srb1_timer_poll_retransmit = (srb1_timer_poll_retransmit - 5)/5;
                    } else if (srb1_timer_poll_retransmit <= 500) {
                        rrc->srb1_timer_poll_retransmit = (srb1_timer_poll_retransmit - 300)/50 + 50;
                    } else {
                        AssertFatal (0,
                                     "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_poll_retransmit %u!\n",
                                     RC.config_file_name, i, srb1_timer_poll_retransmit);
                    }

                    if (srb1_timer_status_prohibit <= 250) {
                        rrc->srb1_timer_status_prohibit = srb1_timer_status_prohibit/5;
                    } else if ((srb1_timer_poll_retransmit >= 300) && (srb1_timer_poll_retransmit <= 500)) {
                        rrc->srb1_timer_status_prohibit = (srb1_timer_status_prohibit - 300)/50 + 51;
                    } else {
                        AssertFatal (0,
                                     "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_status_prohibit %u!\n",
                                     RC.config_file_name, i, srb1_timer_status_prohibit);
                    }

                    switch (srb1_timer_reordering) {
                    case 0:
                        rrc->srb1_timer_reordering = T_Reordering_ms0;
                        break;

                    case 5:
                        rrc->srb1_timer_reordering = T_Reordering_ms5;
                        break;

                    case 10:
                        rrc->srb1_timer_reordering = T_Reordering_ms10;
                        break;

                    case 15:
                        rrc->srb1_timer_reordering = T_Reordering_ms15;
                        break;

                    case 20:
                        rrc->srb1_timer_reordering = T_Reordering_ms20;
                        break;

                    case 25:
                        rrc->srb1_timer_reordering = T_Reordering_ms25;
                        break;

                    case 30:
                        rrc->srb1_timer_reordering = T_Reordering_ms30;
                        break;

                    case 35:
                        rrc->srb1_timer_reordering = T_Reordering_ms35;
                        break;

                    case 40:
                        rrc->srb1_timer_reordering = T_Reordering_ms40;
                        break;

                    case 45:
                        rrc->srb1_timer_reordering = T_Reordering_ms45;
                        break;

                    case 50:
                        rrc->srb1_timer_reordering = T_Reordering_ms50;
                        break;

                    case 55:
                        rrc->srb1_timer_reordering = T_Reordering_ms55;
                        break;

                    case 60:
                        rrc->srb1_timer_reordering = T_Reordering_ms60;
                        break;

                    case 65:
                        rrc->srb1_timer_reordering = T_Reordering_ms65;
                        break;

                    case 70:
                        rrc->srb1_timer_reordering = T_Reordering_ms70;
                        break;

                    case 75:
                        rrc->srb1_timer_reordering = T_Reordering_ms75;
                        break;

                    case 80:
                        rrc->srb1_timer_reordering = T_Reordering_ms80;
                        break;

                    case 85:
                        rrc->srb1_timer_reordering = T_Reordering_ms85;
                        break;

                    case 90:
                        rrc->srb1_timer_reordering = T_Reordering_ms90;
                        break;

                    case 95:
                        rrc->srb1_timer_reordering = T_Reordering_ms95;
                        break;

                    case 100:
                        rrc->srb1_timer_reordering = T_Reordering_ms100;
                        break;

                    case 110:
                        rrc->srb1_timer_reordering = T_Reordering_ms110;
                        break;

                    case 120:
                        rrc->srb1_timer_reordering = T_Reordering_ms120;
                        break;

                    case 130:
                        rrc->srb1_timer_reordering = T_Reordering_ms130;
                        break;

                    case 140:
                        rrc->srb1_timer_reordering = T_Reordering_ms140;
                        break;

                    case 150:
                        rrc->srb1_timer_reordering = T_Reordering_ms150;
                        break;

                    case 160:
                        rrc->srb1_timer_reordering = T_Reordering_ms160;
                        break;

                    case 170:
                        rrc->srb1_timer_reordering = T_Reordering_ms170;
                        break;

                    case 180:
                        rrc->srb1_timer_reordering = T_Reordering_ms180;
                        break;

                    case 190:
                        rrc->srb1_timer_reordering = T_Reordering_ms190;
                        break;

                    case 200:
                        rrc->srb1_timer_reordering = T_Reordering_ms200;
                        break;

                    default:
                        AssertFatal (0,
                                     "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_reordering %u!\n",
                                     RC.config_file_name, i, srb1_timer_reordering);
                    }

                } else {
                    rrc->srb1_timer_poll_retransmit = T_PollRetransmit_ms80;
                    rrc->srb1_timer_reordering      = T_Reordering_ms35;
                    rrc->srb1_timer_status_prohibit = T_StatusProhibit_ms0;
                    rrc->srb1_poll_pdu              = PollPDU_p4;
                    rrc->srb1_poll_byte             = PollByte_kBinfinity;
                    rrc->srb1_max_retx_threshold    = UL_AM_RLC__maxRetxThreshold_t8;
                }
                /*
        // Network Controller
        subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_NETWORK_CONTROLLER_CONFIG);

        if (subsetting != NULL) {
          if (  (
             config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_FLEXRAN_AGENT_INTERFACE_NAME,
                           (const char **)&flexran_agent_interface_name)
             && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_FLEXRAN_AGENT_IPV4_ADDRESS,
                              (const char **)&flexran_agent_ipv4_address)
             && config_setting_lookup_int(subsetting, ENB_CONFIG_STRING_FLEXRAN_AGENT_PORT,
                          &flexran_agent_port)
             && config_setting_lookup_string( subsetting, ENB_CONFIG_STRING_FLEXRAN_AGENT_CACHE,
                              (const char **)&flexran_agent_cache)
             )
            ) {
        enb_properties_loc.properties[enb_properties_loc_index]->flexran_agent_interface_name = strdup(flexran_agent_interface_name);
        cidr = flexran_agent_ipv4_address;
        address = strtok(cidr, "/");
        //enb_properties_loc.properties[enb_properties_loc_index]->flexran_agent_ipv4_address = strdup(address);
        if (address) {
          IPV4_STR_ADDR_TO_INT_NWBO (address, enb_properties_loc.properties[enb_properties_loc_index]->flexran_agent_ipv4_address, "BAD IP ADDRESS FORMAT FOR eNB Agent !\n" );
        }

        enb_properties_loc.properties[enb_properties_loc_index]->flexran_agent_port = flexran_agent_port;
        enb_properties_loc.properties[enb_properties_loc_index]->flexran_agent_cache = strdup(flexran_agent_cache);
          }
        }
        */

                /*
        // OTG _CONFIG
        setting_otg = config_setting_get_member (setting_enb, ENB_CONF_STRING_OTG_CONFIG);

        if (setting_otg != NULL) {
          num_otg_elements  = config_setting_length(setting_otg);
          printf("num otg elements %d \n", num_otg_elements);
          enb_properties_loc.properties[enb_properties_loc_index]->num_otg_elements = 0;

          for (j = 0; j < num_otg_elements; j++) {
        subsetting_otg=config_setting_get_elem(setting_otg, j);

        if (config_setting_lookup_int(subsetting_otg, ENB_CONF_STRING_OTG_UE_ID, &otg_ue_id)) {
          enb_properties_loc.properties[enb_properties_loc_index]->otg_ue_id[j] = otg_ue_id;
        } else {
          enb_properties_loc.properties[enb_properties_loc_index]->otg_ue_id[j] = 1;
        }

        if (config_setting_lookup_string(subsetting_otg, ENB_CONF_STRING_OTG_APP_TYPE, (const char **)&otg_app_type)) {
          if ((enb_properties_loc.properties[enb_properties_loc_index]->otg_app_type[j] = map_str_to_int(otg_app_type_names,otg_app_type))== -1) {
            enb_properties_loc.properties[enb_properties_loc_index]->otg_app_type[j] = BCBR;
          }
        } else {
          enb_properties_loc.properties[enb_properties_loc_index]->otg_app_type[j] = NO_PREDEFINED_TRAFFIC; // 0
        }

        if (config_setting_lookup_string(subsetting_otg, ENB_CONF_STRING_OTG_BG_TRAFFIC, (const char **)&otg_bg_traffic)) {

          if ((enb_properties_loc.properties[enb_properties_loc_index]->otg_bg_traffic[j] = map_str_to_int(switch_names,otg_bg_traffic)) == -1) {
            enb_properties_loc.properties[enb_properties_loc_index]->otg_bg_traffic[j]=0;
          }
        } else {
          enb_properties_loc.properties[enb_properties_loc_index]->otg_bg_traffic[j] = 0;
          printf("otg bg %s\n", otg_bg_traffic);
        }

        enb_properties_loc.properties[enb_properties_loc_index]->num_otg_elements+=1;

          }
        }

        // log_config
        subsetting = config_setting_get_member (setting_enb, ENB_CONFIG_STRING_LOG_CONFIG);

        if (subsetting != NULL) {
          // global
          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GLOBAL_LOG_LEVEL, (const char **)  &glog_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->glog_level = map_str_to_int(log_level_names, glog_level)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->glog_level = LOG_INFO;
        }

        //printf( "\tGlobal log level :\t%s->%d\n",glog_level, enb_properties_loc.properties[enb_properties_loc_index]->glog_level);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->glog_level = LOG_INFO;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GLOBAL_LOG_VERBOSITY,(const char **)  &glog_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->glog_verbosity = map_str_to_int(log_verbosity_names, glog_verbosity)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->glog_verbosity = LOG_MED;
        }

        //printf( "\tGlobal log verbosity:\t%s->%d\n",glog_verbosity, enb_properties_loc.properties[enb_properties_loc_index]->glog_verbosity);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->glog_verbosity = LOG_MED;
          }

          // HW
          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_HW_LOG_LEVEL, (const char **) &hw_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->hw_log_level = map_str_to_int(log_level_names,hw_log_level)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->hw_log_level = LOG_INFO;
        }

        //printf( "\tHW log level :\t%s->%d\n",hw_log_level,enb_properties_loc.properties[enb_properties_loc_index]->hw_log_level);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->hw_log_level = LOG_INFO;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_HW_LOG_VERBOSITY, (const char **) &hw_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->hw_log_verbosity = map_str_to_int(log_verbosity_names,hw_log_verbosity)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->hw_log_verbosity = LOG_MED;
        }

        //printf( "\tHW log verbosity:\t%s->%d\n",hw_log_verbosity, enb_properties_loc.properties[enb_properties_loc_index]->hw_log_verbosity);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->hw_log_verbosity = LOG_MED;
          }

          // phy
          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PHY_LOG_LEVEL,(const char **) &phy_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->phy_log_level = map_str_to_int(log_level_names,phy_log_level)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->phy_log_level = LOG_INFO;
        }

        //printf( "\tPHY log level :\t%s->%d\n",phy_log_level,enb_properties_loc.properties[enb_properties_loc_index]->phy_log_level);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->phy_log_level = LOG_INFO;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PHY_LOG_VERBOSITY, (const char **)&phy_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->phy_log_verbosity = map_str_to_int(log_verbosity_names,phy_log_verbosity)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->phy_log_verbosity = LOG_MED;
        }

        //printf( "\tPHY log verbosity:\t%s->%d\n",phy_log_level,enb_properties_loc.properties[enb_properties_loc_index]->phy_log_verbosity);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->phy_log_verbosity = LOG_MED;
          }

          //mac
          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_MAC_LOG_LEVEL, (const char **)&mac_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->mac_log_level = map_str_to_int(log_level_names,mac_log_level)) == -1 ) {
          enb_properties_loc.properties[enb_properties_loc_index]->mac_log_level = LOG_INFO;
        }

        //printf( "\tMAC log level :\t%s->%d\n",mac_log_level,enb_properties_loc.properties[enb_properties_loc_index]->mac_log_level);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->mac_log_level = LOG_INFO;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_MAC_LOG_VERBOSITY, (const char **)&mac_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->mac_log_verbosity = map_str_to_int(log_verbosity_names,mac_log_verbosity)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->mac_log_verbosity = LOG_MED;
        }

        //printf( "\tMAC log verbosity:\t%s->%d\n",mac_log_verbosity,enb_properties_loc.properties[enb_properties_loc_index]->mac_log_verbosity);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->mac_log_verbosity = LOG_MED;
          }

          //rlc
          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RLC_LOG_LEVEL, (const char **)&rlc_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_level = map_str_to_int(log_level_names,rlc_log_level)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_level = LOG_INFO;
        }

        //printf( "\tRLC log level :\t%s->%d\n",rlc_log_level, enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_level);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_level = LOG_INFO;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RLC_LOG_VERBOSITY, (const char **)&rlc_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_verbosity = map_str_to_int(log_verbosity_names,rlc_log_verbosity)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_verbosity = LOG_MED;
        }

        //printf( "\tRLC log verbosity:\t%s->%d\n",rlc_log_verbosity, enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_verbosity);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_verbosity = LOG_MED;
          }

          //pdcp
          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PDCP_LOG_LEVEL, (const char **)&pdcp_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_level = map_str_to_int(log_level_names,pdcp_log_level)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_level = LOG_INFO;
        }

        //printf( "\tPDCP log level :\t%s->%d\n",pdcp_log_level, enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_level);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_level = LOG_INFO;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_PDCP_LOG_VERBOSITY, (const char **)&pdcp_log_verbosity)) {
        enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_verbosity = map_str_to_int(log_verbosity_names,pdcp_log_verbosity);
        //printf( "\tPDCP log verbosity:\t%s->%d\n",pdcp_log_verbosity, enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_verbosity);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_verbosity = LOG_MED;
          }

          //rrc
          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RRC_LOG_LEVEL, (const char **)&rrc_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_level = map_str_to_int(log_level_names,rrc_log_level)) == -1 ) {
          enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_level = LOG_INFO;
        }

        //printf( "\tRRC log level :\t%s->%d\n",rrc_log_level,enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_level);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_level = LOG_INFO;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_RRC_LOG_VERBOSITY, (const char **)&rrc_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_verbosity = map_str_to_int(log_verbosity_names,rrc_log_verbosity)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_verbosity = LOG_MED;
        }

        //printf( "\tRRC log verbosity:\t%s->%d\n",rrc_log_verbosity,enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_verbosity);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_verbosity = LOG_MED;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GTPU_LOG_LEVEL, (const char **)&gtpu_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_level = map_str_to_int(log_level_names,gtpu_log_level)) == -1 ) {
          enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_level = LOG_INFO;
        }

        //printf( "\tGTPU log level :\t%s->%d\n",gtpu_log_level,enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_level);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_level = LOG_INFO;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_GTPU_LOG_VERBOSITY, (const char **)&gtpu_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity = map_str_to_int(log_verbosity_names,gtpu_log_verbosity)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity = LOG_MED;
        }

        //printf( "\tGTPU log verbosity:\t%s->%d\n",gtpu_log_verbosity,enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity = LOG_MED;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_UDP_LOG_LEVEL, (const char **)&udp_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->udp_log_level = map_str_to_int(log_level_names,udp_log_level)) == -1 ) {
          enb_properties_loc.properties[enb_properties_loc_index]->udp_log_level = LOG_INFO;
        }

        //printf( "\tUDP log level :\t%s->%d\n",udp_log_level,enb_properties_loc.properties[enb_properties_loc_index]->udp_log_level);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->udp_log_level = LOG_INFO;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_UDP_LOG_VERBOSITY, (const char **)&udp_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->udp_log_verbosity = map_str_to_int(log_verbosity_names,udp_log_verbosity)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->udp_log_verbosity = LOG_MED;
        }

        //printf( "\tUDP log verbosity:\t%s->%d\n",udp_log_verbosity,enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->udp_log_verbosity = LOG_MED;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_OSA_LOG_LEVEL, (const char **)&osa_log_level)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->osa_log_level = map_str_to_int(log_level_names,osa_log_level)) == -1 ) {
          enb_properties_loc.properties[enb_properties_loc_index]->osa_log_level = LOG_INFO;
        }

        //printf( "\tOSA log level :\t%s->%d\n",osa_log_level,enb_properties_loc.properties[enb_properties_loc_index]->osa_log_level);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->osa_log_level = LOG_INFO;
          }

          if (config_setting_lookup_string(subsetting, ENB_CONFIG_STRING_OSA_LOG_VERBOSITY, (const char **)&osa_log_verbosity)) {
        if ((enb_properties_loc.properties[enb_properties_loc_index]->osa_log_verbosity = map_str_to_int(log_verbosity_names,osa_log_verbosity)) == -1) {
          enb_properties_loc.properties[enb_properties_loc_index]->osa_log_verbosity = LOG_MED;
        }

        //printf( "\tOSA log verbosity:\t%s->%d\n",osa_log_verbosity,enb_properties_loc.properties[enb_properties_loc_index]->gosa_log_verbosity);
          } else {
        enb_properties_loc.properties[enb_properties_loc_index]->osa_log_verbosity = LOG_MED;
          }

        } else { // not configuration is given
          enb_properties_loc.properties[enb_properties_loc_index]->glog_level         = LOG_INFO;
          enb_properties_loc.properties[enb_properties_loc_index]->glog_verbosity     = LOG_MED;
          enb_properties_loc.properties[enb_properties_loc_index]->hw_log_level       = LOG_INFO;
          enb_properties_loc.properties[enb_properties_loc_index]->hw_log_verbosity   = LOG_MED;
          enb_properties_loc.properties[enb_properties_loc_index]->phy_log_level      = LOG_INFO;
          enb_properties_loc.properties[enb_properties_loc_index]->phy_log_verbosity  = LOG_MED;
          enb_properties_loc.properties[enb_properties_loc_index]->mac_log_level      = LOG_INFO;
          enb_properties_loc.properties[enb_properties_loc_index]->mac_log_verbosity  = LOG_MED;
          enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_level      = LOG_INFO;
          enb_properties_loc.properties[enb_properties_loc_index]->rlc_log_verbosity  = LOG_MED;
          enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_level     = LOG_INFO;
          enb_properties_loc.properties[enb_properties_loc_index]->pdcp_log_verbosity = LOG_MED;
          enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_level      = LOG_INFO;
          enb_properties_loc.properties[enb_properties_loc_index]->rrc_log_verbosity  = LOG_MED;
          enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_level     = LOG_INFO;
          enb_properties_loc.properties[enb_properties_loc_index]->gtpu_log_verbosity = LOG_MED;
          enb_properties_loc.properties[enb_properties_loc_index]->udp_log_level      = LOG_INFO;
          enb_properties_loc.properties[enb_properties_loc_index]->udp_log_verbosity  = LOG_MED;
          enb_properties_loc.properties[enb_properties_loc_index]->osa_log_level      = LOG_INFO;
          enb_properties_loc.properties[enb_properties_loc_index]->osa_log_verbosity  = LOG_MED;
        }
        */
                break;
            }

        }
    }
    return 0;
}

int RCconfig_gtpu(void ) {

  int               num_enbs                      = 0;



  char*             enb_interface_name_for_S1U    = NULL;
  char*             enb_ipv4_address_for_S1U      = NULL;
  uint32_t          enb_port_for_S1U              = 0;
  char             *address                       = NULL;
  char             *cidr                          = NULL;
  char gtpupath[MAX_OPTNAME_SIZE*2 + 8];
	  

  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  
  paramdef_t GTPUParams[]  = GTPUPARAMS_DESC;
  LOG_I(GTPU,"Configuring GTPu\n");

/* get number of active eNodeBs */
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL); 
  num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  AssertFatal (num_enbs >0,
   	       "Failed to parse config file no active eNodeBs in %s \n", ENB_CONFIG_STRING_ACTIVE_ENBS);


  sprintf(gtpupath,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,0,ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  config_get( GTPUParams,sizeof(GTPUParams)/sizeof(paramdef_t),gtpupath);    



	  cidr = enb_ipv4_address_for_S1U;
	  address = strtok(cidr, "/");
	  
	  if (address) {
	    IPV4_STR_ADDR_TO_INT_NWBO ( address, RC.gtpv1u_data_g->enb_ip_address_for_S1u_S12_S4_up, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );

	    LOG_I(GTPU,"Configuring GTPu address : %s -> %x\n",address,RC.gtpv1u_data_g->enb_ip_address_for_S1u_S12_S4_up);

	  }

	  RC.gtpv1u_data_g->enb_port_for_S1u_S12_S4_up = enb_port_for_S1U;
return 0;
}


int RCconfig_S1(MessageDef *msg_p, uint32_t i) {

  int               j,k                           = 0;
  
  
  int enb_id;

  int32_t     my_int;



  const char*       active_enb[MAX_ENB];


  char             *address                       = NULL;
  char             *cidr                          = NULL;


  // for no gcc warnings 

  (void)my_int;

  memset((char*)active_enb,     0 , MAX_ENB * sizeof(char*));

  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};

/* get global parameters, defined outside any section in the config file */
  
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL); 
#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)
    if (strcasecmp( *(ENBSParams[ENB_ASN1_VERBOSITY_IDX].strptr), ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0) {
      asn_debug      = 0;
      asn1_xer_print = 0;
    } else if (strcasecmp( *(ENBSParams[ENB_ASN1_VERBOSITY_IDX].strptr), ENB_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0) {
      asn_debug      = 1;
      asn1_xer_print = 1;
    } else if (strcasecmp(*(ENBSParams[ENB_ASN1_VERBOSITY_IDX].strptr) , ENB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0) {
      asn_debug      = 1;
      asn1_xer_print = 2;
    } else {
      asn_debug      = 0;
      asn1_xer_print = 0;
    }

#endif

    AssertFatal (i<ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt,
		 "Failed to parse config file %s, %uth attribute %s \n",
		 RC.config_file_name, i, ENB_CONFIG_STRING_ACTIVE_ENBS);
    
  
  if (ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt>0) {
    // Output a list of all eNBs.
       config_getlist( &ENBParamList,ENBParams,sizeof(ENBParams)/sizeof(paramdef_t),NULL); 
    
    
      
      
    
    if (ENBParamList.numelt > 0) {
      for (k = 0; k < ENBParamList.numelt; k++) {
	if (ENBParamList.paramarray[k][ENB_ENB_ID_IDX].uptr == NULL) {
	  // Calculate a default eNB ID

# if defined(ENABLE_USE_MME)
	  uint32_t hash;
	  
	  hash = s1ap_generate_eNB_id ();
	  enb_id = k + (hash & 0xFFFF8);
# else
	  enb_id = k;
# endif
	} else {
          enb_id = *(ENBParamList.paramarray[k][ENB_ENB_ID_IDX].uptr);
        }
	
	
	// search if in active list
	for (j=0; j < ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt; j++) {
	  if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[j], *(ENBParamList.paramarray[k][ENB_ENB_NAME_IDX].strptr)) == 0) {
             paramdef_t S1Params[]  = S1PARAMS_DESC;
             paramlist_def_t S1ParamList = {ENB_CONFIG_STRING_MME_IP_ADDRESS,NULL,0};

             paramdef_t SCTPParams[]  = SCTPPARAMS_DESC;
             paramdef_t NETParams[]  =  NETPARAMS_DESC;
             char aprefix[MAX_OPTNAME_SIZE*2 + 8];
	    
	    S1AP_REGISTER_ENB_REQ (msg_p).eNB_id = enb_id;
	    
	    if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_MACRO_ENB") == 0) {
	      S1AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_MACRO_ENB;
	    } else  if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_HOME_ENB") == 0) {
	      S1AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_HOME_ENB;
	    } else {
	      AssertFatal (0,
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
			   RC.config_file_name, i, *(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr));
	    }
	    
	    S1AP_REGISTER_ENB_REQ (msg_p).eNB_name         = strdup(*(ENBParamList.paramarray[k][ENB_ENB_NAME_IDX].strptr));
	    S1AP_REGISTER_ENB_REQ (msg_p).tac              = (uint16_t)atoi(*(ENBParamList.paramarray[k][ENB_TRACKING_AREA_CODE_IDX].strptr));
	    S1AP_REGISTER_ENB_REQ (msg_p).mcc              = (uint16_t)atoi(*(ENBParamList.paramarray[k][ENB_MOBILE_COUNTRY_CODE_IDX].strptr));
	    S1AP_REGISTER_ENB_REQ (msg_p).mnc              = (uint16_t)atoi(*(ENBParamList.paramarray[k][ENB_MOBILE_NETWORK_CODE_IDX].strptr));
	    S1AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length = strlen(*(ENBParamList.paramarray[k][ENB_MOBILE_NETWORK_CODE_IDX].strptr));
	    S1AP_REGISTER_ENB_REQ (msg_p).default_drx      = 0;
	    AssertFatal((S1AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length == 2) ||
			(S1AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length == 3),
			"BAD MNC DIGIT LENGTH %d",
			S1AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length);
	    
	    sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
            config_getlist( &S1ParamList,S1Params,sizeof(S1Params)/sizeof(paramdef_t),aprefix); 
	    
	    S1AP_REGISTER_ENB_REQ (msg_p).nb_mme = 0;

	    for (int l = 0; l < S1ParamList.numelt; l++) {

	      S1AP_REGISTER_ENB_REQ (msg_p).nb_mme += 1;

	      strcpy(S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv4_address,*(S1ParamList.paramarray[l][ENB_MME_IPV4_ADDRESS_IDX].strptr));
	      strcpy(S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv6_address,*(S1ParamList.paramarray[l][ENB_MME_IPV6_ADDRESS_IDX].strptr));

	      if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_ACTIVE_IDX].strptr), "yes") == 0) {
#if defined(ENABLE_USE_MME)
		EPC_MODE_ENABLED = 1;
#endif
	      } 
	      if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv4") == 0) {
		S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[j].ipv4 = 1;
	      } else if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv6") == 0) {
		S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[j].ipv6 = 1;
	      } else if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_PREFERENCE_IDX].strptr), "no") == 0) {
		S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[j].ipv4 = 1;
		S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[j].ipv6 = 1;
	      }
	    }

	  
	    // SCTP SETTING
	    S1AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = SCTP_OUT_STREAMS;
	    S1AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams  = SCTP_IN_STREAMS;
# if defined(ENABLE_USE_MME)
	    sprintf(aprefix,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_SCTP_CONFIG);
            config_get( SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix); 
            S1AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[ENB_SCTP_INSTREAMS_IDX].uptr);
            S1AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[ENB_SCTP_OUTSTREAMS_IDX].uptr);
#endif

            sprintf(aprefix,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
	    // NETWORK_INTERFACES
            config_get( NETParams,sizeof(NETParams)/sizeof(paramdef_t),aprefix); 

		//		S1AP_REGISTER_ENB_REQ (msg_p).enb_interface_name_for_S1U = strdup(enb_interface_name_for_S1U);
		cidr = *(NETParams[ENB_IPV4_ADDRESS_FOR_S1_MME_IDX].strptr);
		address = strtok(cidr, "/");

		S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv6 = 0;
		S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv4 = 1;

		strcpy(S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv4_address, address);

		/*
		in_addr_t  ipv4_address;

				if (address) {
		  IPV4_STR_ADDR_TO_INT_NWBO ( address, ipv4_address, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );
		}
		strcpy(S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv4_address, inet_ntoa(ipv4_address));
		//		S1AP_REGISTER_ENB_REQ (msg_p).enb_port_for_S1U = enb_port_for_S1U;


		S1AP_REGISTER_ENB_REQ (msg_p).enb_interface_name_for_S1_MME = strdup(enb_interface_name_for_S1_MME);
		cidr = enb_ipv4_address_for_S1_MME;
		address = strtok(cidr, "/");
		
		if (address) {
		  IPV4_STR_ADDR_TO_INT_NWBO ( address, S1AP_REGISTER_ENB_REQ(msg_p).enb_ipv4_address_for_S1_MME, "BAD IP ADDRESS FORMAT FOR eNB S1_MME !\n" );
		}
*/
	  



	    break;
	  }
	}
      }
    }
  }
return 0;
}

void RCConfig(void) {

  paramlist_def_t MACRLCParamList = {CONFIG_STRING_MACRLC_LIST,NULL,0};
  paramlist_def_t L1ParamList = {CONFIG_STRING_L1_LIST,NULL,0};
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS,NULL,0};
  
  char aprefix[MAX_OPTNAME_SIZE*2 + 8];  
  


/* get global parameters, defined outside any section in the config file */
  
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL); 
  RC.nb_inst = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
 
  if (RC.nb_inst > 0) {
    RC.nb_CC = (int *)malloc((1+RC.nb_inst)*sizeof(int));
    for (int i=0;i<RC.nb_inst;i++) {
      sprintf(aprefix, "%s.[%i]", ENB_CONFIG_STRING_ENB_LIST, i);
      config_getlist( &CCsParamList,NULL,0, aprefix);
      RC.nb_CC[i]		 = CCsParamList.numelt;
    }
  }

    // Get num MACRLC instances
    
 
    config_getlist( &MACRLCParamList,NULL,0, NULL);
    RC.nb_macrlc_inst  = MACRLCParamList.numelt;
    // Get num L1 instances
    config_getlist( &L1ParamList,NULL,0, NULL);
    RC.nb_L1_inst = L1ParamList.numelt;

    // Get num RU instances
    config_getlist( &RUParamList,NULL,0, NULL);  
    RC.nb_RU     = RUParamList.numelt; 
 

}
