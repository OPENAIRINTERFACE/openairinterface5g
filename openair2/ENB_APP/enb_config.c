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
#include "LAYER2/MAC/proto.h"
#include "PHY/extern.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"
#include "nfapi_vnf.h"
#include "nfapi_pnf.h"

#include "enb_paramdef.h"
#include "common/config/config_userapi.h"

extern uint16_t sf_ahead;

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

      printf("Creating RC.ru[%d]:%p\n", j, RC.ru[j]);

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
	RC.ru[j]->att_tx                            = *(RUParamList.paramarray[j][RU_ATT_TX_IDX].uptr);
	RC.ru[j]->att_rx                            = *(RUParamList.paramarray[j][RU_ATT_RX_IDX].uptr);
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
	RC.ru[j]->att_rx                         = *(RUParamList.paramarray[j][RU_ATT_RX_IDX].uptr);
      }  /* strcmp(local_rf, "yes") != 0 */

      RC.ru[j]->nb_tx                             = *(RUParamList.paramarray[j][RU_NB_TX_IDX].uptr);
      RC.ru[j]->nb_rx                             = *(RUParamList.paramarray[j][RU_NB_RX_IDX].uptr);
      
    }// j=0..num_rus
  } else {
    RC.nb_RU = 0;	    
  } // setting != NULL

  return;
  
}

void RCconfig_L1(void) {
  int               i,j;
  paramdef_t L1_Params[] = L1PARAMS_DESC;
  paramlist_def_t L1_ParamList = {CONFIG_STRING_L1_LIST,NULL,0};


  if (RC.eNB == NULL) {
    RC.eNB                       = (PHY_VARS_eNB ***)malloc((1+NUMBER_OF_eNB_MAX)*sizeof(PHY_VARS_eNB**));
    LOG_I(PHY,"RC.eNB = %p\n",RC.eNB);
    memset(RC.eNB,0,(1+NUMBER_OF_eNB_MAX)*sizeof(PHY_VARS_eNB**));
    RC.nb_L1_CC = malloc((1+RC.nb_L1_inst)*sizeof(int));
  }

  config_getlist( &L1_ParamList,L1_Params,sizeof(L1_Params)/sizeof(paramdef_t), NULL);
  if (L1_ParamList.numelt > 0) {

    for (j = 0; j < RC.nb_L1_inst; j++) {
      RC.nb_L1_CC[j] = *(L1_ParamList.paramarray[j][L1_CC_IDX].uptr);

      if (RC.eNB[j] == NULL) {
	RC.eNB[j]                       = (PHY_VARS_eNB **)malloc((1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB*));
	LOG_I(PHY,"RC.eNB[%d] = %p\n",j,RC.eNB[j]);
	memset(RC.eNB[j],0,(1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB*));
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

        sf_ahead = 4; // Need 4 subframe gap between RX and TX
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

        sf_ahead = 2; // Cannot cope with 4 subframes betweem RX and TX - set it to 2

        RC.nb_macrlc_inst = 1;  // This is used by mac_top_init_eNB()

        // This is used by init_eNB_afterRU()
        RC.nb_CC = (int *)malloc((1+RC.nb_inst)*sizeof(int));
        RC.nb_CC[0]=1;

        RC.nb_inst =1; // DJP - feptx_prec uses num_eNB but phy_init_RU uses nb_inst

        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_inst=1 this is because phy_init_RU() uses that to index and not RC.num_eNB - why the 2 similar variables?\n", __FUNCTION__);
        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_CC[0]=%d for init_eNB_afterRU()\n", __FUNCTION__, RC.nb_CC[0]);
        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_macrlc_inst:%d because used by mac_top_init_eNB()\n", __FUNCTION__, RC.nb_macrlc_inst);

        mac_top_init_eNB();

        configure_nfapi_pnf(RC.eNB[j][0]->eth_params_n.remote_addr, RC.eNB[j][0]->eth_params_n.remote_portc, RC.eNB[j][0]->eth_params_n.my_addr, RC.eNB[j][0]->eth_params_n.my_portd, RC.eNB[j][0]->eth_params_n     .remote_portd);
      }
      else { // other midhaul
      }	
    }// j=0..num_inst
    printf("Initializing northbound interface for L1\n");
    l1_north_init_eNB();
  } else {
    LOG_I(PHY,"No " CONFIG_STRING_L1_LIST " configuration found");    

    // DJP need to create some structures for VNF

    j = 0;

    RC.nb_L1_CC = malloc((1+RC.nb_L1_inst)*sizeof(int)); // DJP - 1 lot then???

    RC.nb_L1_CC[j]=1; // DJP - hmmm

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

        sf_ahead = 2; // Cannot cope with 4 subframes betweem RX and TX - set it to 2

        printf("**************** vnf_port:%d\n", RC.mac[j]->eth_params_s.my_portc);
        configure_nfapi_vnf(RC.mac[j]->eth_params_s.my_addr, RC.mac[j]->eth_params_s.my_portc);
        printf("**************** RETURNED FROM configure_nfapi_vnf() vnf_port:%d\n", RC.mac[j]->eth_params_s.my_portc);
      } else { // other midhaul
	AssertFatal(1==0,"MACRLC %d: %s unknown southbound midhaul\n",j,*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr));
      }	
    }// j=0..num_inst
  } else {// MacRLC_ParamList.numelt > 0
	  AssertFatal (0,
		       "No " CONFIG_STRING_MACRLC_LIST " configuration found");     
  }
}
	       
int RCconfig_RRC(MessageDef *msg_p, uint32_t i, eNB_RRC_INST *rrc) {

  int               num_enbs                      = 0;
 
  int               num_component_carriers        = 0;
  int               j,k                           = 0;
  int32_t     enb_id                        = 0;
  int               nb_cc                         = 0;


  char*       frame_type                    = NULL;
  int32_t     tdd_config                    = 0;
  int32_t     tdd_config_s                  = 0;

  char*       prefix_type                   = NULL;
  char*       pbch_repetition               = NULL;
  
  int32_t     eutra_band                    = 0;
  long long int     downlink_frequency            = 0;
  int32_t     uplink_frequency_offset       = 0;
  int32_t     Nid_cell                      = 0;
  int32_t     Nid_cell_mbsfn                = 0;
  int32_t     N_RB_DL                       = 0;
  int32_t     nb_antenna_ports              = 0;

  int32_t     prach_root                    = 0;
  int32_t     prach_config_index            = 0;
  char*            prach_high_speed         = NULL;
  int32_t     prach_zero_correlation        = 0;
  int32_t     prach_freq_offset             = 0;
  int32_t     pucch_delta_shift             = 0;
  int32_t     pucch_nRB_CQI                 = 0;
  int32_t     pucch_nCS_AN                  = 0;
//#if !defined(Rel10) && !defined(Rel14)
  int32_t     pucch_n1_AN                   = 0;
//#endif
  int32_t     pdsch_referenceSignalPower    = 0;
  int32_t     pdsch_p_b                     = 0;
  int32_t     pusch_n_SB                    = 0;
  char *      pusch_hoppingMode             = NULL;
  int32_t     pusch_hoppingOffset           = 0;
  char*          pusch_enable64QAM          = NULL;
  char*          pusch_groupHoppingEnabled  = NULL;
  int32_t     pusch_groupAssignment         = 0;
  char*          pusch_sequenceHoppingEnabled = NULL;
  int32_t     pusch_nDMRS1                  = 0;
  char*       phich_duration                = NULL;
  char*       phich_resource                = NULL;
  char*       srs_enable                    = NULL;
  int32_t     srs_BandwidthConfig           = 0;
  int32_t     srs_SubframeConfig            = 0;
  char*       srs_ackNackST                 = NULL;
  char*       srs_MaxUpPts                  = NULL;
  int32_t     pusch_p0_Nominal              = 0;
  char*       pusch_alpha                   = NULL;
  int32_t     pucch_p0_Nominal              = 0;
  int32_t     msg3_delta_Preamble           = 0;
  //int32_t     ul_CyclicPrefixLength         = 0;
  char*       pucch_deltaF_Format1          = NULL;
  //const char*       pucch_deltaF_Format1a         = NULL;
  char*       pucch_deltaF_Format1b         = NULL;
  char*       pucch_deltaF_Format2          = NULL;
  char*       pucch_deltaF_Format2a         = NULL;
  char*       pucch_deltaF_Format2b         = NULL;
  int32_t     rach_numberOfRA_Preambles     = 0;
  char*       rach_preamblesGroupAConfig    = NULL;
  int32_t     rach_sizeOfRA_PreamblesGroupA = 0;
  int32_t     rach_messageSizeGroupA        = 0;
  char*       rach_messagePowerOffsetGroupB = NULL;
  int32_t     rach_powerRampingStep         = 0;
  int32_t     rach_preambleInitialReceivedTargetPower    = 0;
  int32_t     rach_preambleTransMax         = 0;
  int32_t     rach_raResponseWindowSize     = 10;
  int32_t     rach_macContentionResolutionTimer = 0;
  int32_t     rach_maxHARQ_Msg3Tx           = 0;
  int32_t     pcch_defaultPagingCycle       = 0;
  char*       pcch_nB                       = NULL;
  int32_t     bcch_modificationPeriodCoeff  = 0;
  int32_t     ue_TimersAndConstants_t300    = 0;
  int32_t     ue_TimersAndConstants_t301    = 0;
  int32_t     ue_TimersAndConstants_t310    = 0;
  int32_t     ue_TimersAndConstants_t311    = 0;
  int32_t     ue_TimersAndConstants_n310    = 0;
  int32_t     ue_TimersAndConstants_n311    = 0;
  int32_t     ue_TransmissionMode           = 0;

  int32_t     ue_multiple_max               = 0;

  int32_t     srb1_timer_poll_retransmit    = 0;
  int32_t     srb1_timer_reordering         = 0;
  int32_t     srb1_timer_status_prohibit    = 0;
  int32_t     srb1_poll_pdu                 = 0;
  int32_t     srb1_poll_byte                = 0;
  int32_t     srb1_max_retx_threshold       = 0;

  int32_t     my_int;


  
/* 
  char*             flexran_agent_interface_name      = NULL;
  char*             flexran_agent_ipv4_address        = NULL;
  int32_t     flexran_agent_port                = 0;
  char*             flexran_agent_cache               = NULL;
  int32_t     otg_ue_id                     = 0;
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
  char* 	    rlc_log_level		  = NULL;
  char* 	    rlc_log_verbosity		  = NULL;
  char* 	    pdcp_log_level		  = NULL;
  char* 	    pdcp_log_verbosity  	  = NULL;
  char* 	    rrc_log_level		  = NULL;
  char* 	    rrc_log_verbosity		  = NULL;
  char* 	    udp_log_verbosity		  = NULL;
  char* 	    osa_log_level		  = NULL;
  char* 	    osa_log_verbosity		  = NULL;
*/  

  
  // for no gcc warnings 
  (void)my_int;
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};  

  paramdef_t CCsParams[] = CCPARAMS_DESC;
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS,NULL,0};
  
  paramdef_t SRB1Params[] = SRB1PARAMS_DESC;  

  


/* get global parameters, defined outside any section in the config file */
  
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL); 
  num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  AssertFatal (i<num_enbs,
   	       "Failed to parse config file no %ith element in %s \n",i, ENB_CONFIG_STRING_ACTIVE_ENBS);
  
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
  

  
  if (num_enbs>0) {
    // Output a list of all eNBs.
    config_getlist( &ENBParamList,ENBParams,sizeof(ENBParams)/sizeof(paramdef_t),NULL); 
    
    
      
      
      if (ENBParamList.paramarray[i][ENB_ENB_ID_IDX].uptr == NULL) {
	// Calculate a default eNB ID
# if defined(ENABLE_USE_MME)
	uint32_t hash;
	
	hash = s1ap_generate_eNB_id ();
	enb_id = i + (hash & 0xFFFF8);
# else
	enb_id = i;
# endif
      } else {
          enb_id = *(ENBParamList.paramarray[i][ENB_ENB_ID_IDX].uptr);
      }

      
      printf("RRC %d: Southbound Transport %s\n",i,*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr));
	    
      if (strcmp(*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr), "local_mac") == 0) {


      }
      else if (strcmp(*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr), "cudu") == 0) {
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

     
   
   
 

      for (k=0; k <num_enbs ; k++) {
	if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[k], *(ENBParamList.paramarray[i][ENB_ENB_NAME_IDX].strptr) )== 0) {
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
	  sprintf(enbpath,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k),
	  config_getlist( &CCsParamList,NULL,0,enbpath); 
	  
	  LOG_I(RRC,"num component carriers %d \n", num_component_carriers);  
	  if ( CCsParamList.numelt> 0) {
	    char ccspath[MAX_OPTNAME_SIZE*2 + 16];
	    

	    

	    
	    //enb_properties_loc.properties[enb_properties_loc_index]->nb_cc = num_component_carriers;


	    for (j = 0; j < CCsParamList.numelt ;j++) { 

	      sprintf(ccspath,"%s.%s.[%i]",enbpath,ENB_CONFIG_STRING_COMPONENT_CARRIERS,j);
	      config_get( CCsParams,sizeof(CCsParams)/sizeof(paramdef_t),ccspath);	      


	      //printf("Component carrier %d\n",component_carrier);	     

		    

	      nb_cc++;
	      /*
		if (strcmp(cc_node_function, "eNodeB_3GPP") == 0) {
		enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = eNodeB_3GPP;
		} else if (strcmp(cc_node_function, "eNodeB_3GPP_BBU") == 0) {
		enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = eNodeB_3GPP_BBU;
		} else if (strcmp(cc_node_function, "NGFI_RCC_IF4p5") == 0) {
		enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = NGFI_RCC_IF4p5;
		} else if (strcmp(cc_node_function, "NGFI_RAU_IF4p5") == 0) {
		enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = NGFI_RAU_IF4p5;
		} else if (strcmp(cc_node_function, "NGFI_RRU_IF4p5") == 0) {
		enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = NGFI_RRU_IF4p5;
		} else if (strcmp(cc_node_function, "NGFI_RRU_IF5") == 0) {
		enb_properties_loc.properties[enb_properties_loc_index]->cc_node_function[j] = NGFI_RRU_IF5;
		} else {
		AssertError (0, parse_errors ++,
		"Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for node_function choice: eNodeB_3GPP or eNodeB_3GPP_BBU or NGFI_IF4_RCC or NGFI_IF4_RRU or NGFI_IF5_RRU !\n",
		lib_config_file_name_pP, i, cc_node_function);
		}
		
		if (strcmp(cc_node_timing, "synch_to_ext_device") == 0) {
		enb_properties_loc.properties[enb_properties_loc_index]->cc_node_timing[j] = synch_to_ext_device;
		} else if (strcmp(cc_node_timing, "synch_to_other") == 0) {
		enb_properties_loc.properties[enb_properties_loc_index]->cc_node_timing[j] = synch_to_other;
		} else {
		AssertError (0, parse_errors ++,
		"Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for node_function choice: SYNCH_TO_DEVICE or SYNCH_TO_OTHER !\n",
		lib_config_file_name_pP, i, cc_node_timing);
		}
		
		if ((cc_node_synch_ref >= -1) && (cc_node_synch_ref < num_component_carriers)) {  
		enb_properties_loc.properties[enb_properties_loc_index]->cc_node_synch_ref[j] = (int16_t) cc_node_synch_ref; 
		} else {
		AssertError (0, parse_errors ++,
		"Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for node_synch_ref choice: valid CC_id or -1 !\n",
		lib_config_file_name_pP, i, cc_node_synch_ref);
		}
	      */
	      
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
		RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = NORMAL;
	      } else  if (strcmp(prefix_type, "EXTENDED") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = EXTENDED;
	      } else {
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
			     RC.config_file_name, i, prefix_type);
	      }
#ifdef Rel14
	      if (!pbch_repetition)
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d define %s: TRUE,FALSE!\n",
			     RC.config_file_name, i, ENB_CONFIG_STRING_PBCH_REPETITION);
	      else if (strcmp(pbch_repetition, "TRUE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).pbch_repetition[j] = 1;
	      } else  if (strcmp(pbch_repetition, "FALSE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).pbch_repetition[j] = 0;
	      } else {
		AssertFatal (0,
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
	      
	      if (strcmp(frame_type, "FDD") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).frame_type[j] = FDD;
	      } else  if (strcmp(frame_type, "TDD") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).frame_type[j] = TDD;
	      } else {
		AssertFatal (0,
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
		RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = NORMAL;
	      } else  if (strcmp(prefix_type, "EXTENDED") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = EXTENDED;
	      } else {
		AssertFatal (0,
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
	      
	      
	      RRC_CONFIGURATION_REQ (msg_p).prach_root[j] =  prach_root;
	      
	      if ((prach_root <0) || (prach_root > 1023))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_root choice: 0..1023 !\n",
			     RC.config_file_name, i, prach_root);
	      
	      RRC_CONFIGURATION_REQ (msg_p).prach_config_index[j] = prach_config_index;
	      
	      if ((prach_config_index <0) || (prach_config_index > 63))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
			     RC.config_file_name, i, prach_config_index);
	      
	      if (!prach_high_speed)
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
			     RC.config_file_name, i, ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
	      else if (strcmp(prach_high_speed, "ENABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).prach_high_speed[j] = TRUE;
	      } else if (strcmp(prach_high_speed, "DISABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).prach_high_speed[j] = FALSE;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
			     RC.config_file_name, i, prach_high_speed);
	      
	      RRC_CONFIGURATION_REQ (msg_p).prach_zero_correlation[j] =prach_zero_correlation;
	      
	      if ((prach_zero_correlation <0) || (prach_zero_correlation > 15))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
			     RC.config_file_name, i, prach_zero_correlation);
	      
	      RRC_CONFIGURATION_REQ (msg_p).prach_freq_offset[j] = prach_freq_offset;
#ifndef UE_EXPANSION
	      if ((prach_freq_offset <0) || (prach_freq_offset > 94))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
			     RC.config_file_name, i, prach_freq_offset);
#else
        if ((N_RB_DL == 25) && (prach_freq_offset != 2))
          AssertFatal (0,
              "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 2(N_RB_DL %d)!\n",
              RC.config_file_name, i, prach_freq_offset,N_RB_DL);
        if (((N_RB_DL == 50) || (N_RB_DL == 100)) && (prach_freq_offset < 3))
          AssertFatal (0,
              "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 3,4(N_RB_DL %d)!\n",
              RC.config_file_name, i, prach_freq_offset,N_RB_DL);
#endif
	      
	      RRC_CONFIGURATION_REQ (msg_p).pucch_delta_shift[j] = pucch_delta_shift-1;
	      
	      if ((pucch_delta_shift <1) || (pucch_delta_shift > 3))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
			     RC.config_file_name, i, pucch_delta_shift);
	      
		RRC_CONFIGURATION_REQ (msg_p).pucch_nRB_CQI[j] = pucch_nRB_CQI;

		if ((pucch_nRB_CQI <0) || (pucch_nRB_CQI > 98))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
			       RC.config_file_name, i, pucch_nRB_CQI);

		RRC_CONFIGURATION_REQ (msg_p).pucch_nCS_AN[j] = pucch_nCS_AN;

		if ((pucch_nCS_AN <0) || (pucch_nCS_AN > 7))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
			       RC.config_file_name, i, pucch_nCS_AN);

//#if !defined(Rel10) && !defined(Rel14)
		RRC_CONFIGURATION_REQ (msg_p).pucch_n1_AN[j] = pucch_n1_AN;

		if ((pucch_n1_AN <0) || (pucch_n1_AN > 2047))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_n1_AN choice: 0..2047!\n",
			       RC.config_file_name, i, pucch_n1_AN);

//#endif
		RRC_CONFIGURATION_REQ (msg_p).pdsch_referenceSignalPower[j] = pdsch_referenceSignalPower;

		if ((pdsch_referenceSignalPower <-60) || (pdsch_referenceSignalPower > 50))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
			       RC.config_file_name, i, pdsch_referenceSignalPower);

		RRC_CONFIGURATION_REQ (msg_p).pdsch_p_b[j] = pdsch_p_b;

		if ((pdsch_p_b <0) || (pdsch_p_b > 3))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
			       RC.config_file_name, i, pdsch_p_b);

		RRC_CONFIGURATION_REQ (msg_p).pusch_n_SB[j] = pusch_n_SB;

		if ((pusch_n_SB <1) || (pusch_n_SB > 4))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
			       RC.config_file_name, i, pusch_n_SB);

		if (!pusch_hoppingMode)
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d define %s: interSubframe,intraAndInterSubframe!\n",
			       RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);
		else if (strcmp(pusch_hoppingMode,"interSubFrame")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_hoppingMode[j] = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
		}  else if (strcmp(pusch_hoppingMode,"intraAndInterSubFrame")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_hoppingMode[j] = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
			       RC.config_file_name, i, pusch_hoppingMode);

		RRC_CONFIGURATION_REQ (msg_p).pusch_hoppingOffset[j] = pusch_hoppingOffset;

		if ((pusch_hoppingOffset<0) || (pusch_hoppingOffset>98))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
			       RC.config_file_name, i, pusch_hoppingMode);

		if (!pusch_enable64QAM)
		  AssertFatal (0, 
			       "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
			       RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
		else if (strcmp(pusch_enable64QAM, "ENABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_enable64QAM[j] = TRUE;
		}  else if (strcmp(pusch_enable64QAM, "DISABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_enable64QAM[j] = FALSE;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
			       RC.config_file_name, i, pusch_enable64QAM);

		if (!pusch_groupHoppingEnabled)
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
			       RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
		else if (strcmp(pusch_groupHoppingEnabled, "ENABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_groupHoppingEnabled[j] = TRUE;
		}  else if (strcmp(pusch_groupHoppingEnabled, "DISABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_groupHoppingEnabled[j] = FALSE;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
			       RC.config_file_name, i, pusch_groupHoppingEnabled);


		RRC_CONFIGURATION_REQ (msg_p).pusch_groupAssignment[j] = pusch_groupAssignment;

		if ((pusch_groupAssignment<0)||(pusch_groupAssignment>29))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
			       RC.config_file_name, i, pusch_groupAssignment);

		if (!pusch_sequenceHoppingEnabled)
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
			       RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
		else if (strcmp(pusch_sequenceHoppingEnabled, "ENABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_sequenceHoppingEnabled[j] = TRUE;
		}  else if (strcmp(pusch_sequenceHoppingEnabled, "DISABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_sequenceHoppingEnabled[j] = FALSE;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
			       RC.config_file_name, i, pusch_sequenceHoppingEnabled);

		RRC_CONFIGURATION_REQ (msg_p).pusch_nDMRS1[j] = pusch_nDMRS1;  //cyclic_shift in RRC!

		if ((pusch_nDMRS1 <0) || (pusch_nDMRS1>7))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
			       RC.config_file_name, i, pusch_nDMRS1);

		if (strcmp(phich_duration,"NORMAL")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).phich_duration[j] = PHICH_Config__phich_Duration_normal;
		} else if (strcmp(phich_duration,"EXTENDED")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).phich_duration[j] = PHICH_Config__phich_Duration_extended;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
			       RC.config_file_name, i, phich_duration);

		if (strcmp(phich_resource,"ONESIXTH")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).phich_resource[j] = PHICH_Config__phich_Resource_oneSixth ;
		} else if (strcmp(phich_resource,"HALF")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).phich_resource[j] = PHICH_Config__phich_Resource_half;
		} else if (strcmp(phich_resource,"ONE")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).phich_resource[j] = PHICH_Config__phich_Resource_one;
		} else if (strcmp(phich_resource,"TWO")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).phich_resource[j] = PHICH_Config__phich_Resource_two;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
			       RC.config_file_name, i, phich_resource);

		printf("phich.resource %ld (%s), phich.duration %ld (%s)\n",
		       RRC_CONFIGURATION_REQ (msg_p).phich_resource[j],phich_resource,
		       RRC_CONFIGURATION_REQ (msg_p).phich_duration[j],phich_duration);

		if (strcmp(srs_enable, "ENABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).srs_enable[j] = TRUE;
		} else if (strcmp(srs_enable, "DISABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).srs_enable[j] = FALSE;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
			       RC.config_file_name, i, srs_enable);

		if (RRC_CONFIGURATION_REQ (msg_p).srs_enable[j] == TRUE) {

		  RRC_CONFIGURATION_REQ (msg_p).srs_BandwidthConfig[j] = srs_BandwidthConfig;

		  if ((srs_BandwidthConfig < 0) || (srs_BandwidthConfig >7))
		    AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value %d for srs_BandwidthConfig choice: 0...7\n",
				 RC.config_file_name, i, srs_BandwidthConfig);

		  RRC_CONFIGURATION_REQ (msg_p).srs_SubframeConfig[j] = srs_SubframeConfig;

		  if ((srs_SubframeConfig<0) || (srs_SubframeConfig>15))
		    AssertFatal (0,
				 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
				 RC.config_file_name, i, srs_SubframeConfig);

		  if (strcmp(srs_ackNackST, "ENABLE") == 0) {
		    RRC_CONFIGURATION_REQ (msg_p).srs_ackNackST[j] = TRUE;
		  } else if (strcmp(srs_ackNackST, "DISABLE") == 0) {
		    RRC_CONFIGURATION_REQ (msg_p).srs_ackNackST[j] = FALSE;
		  } else
		    AssertFatal (0,
				 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
				 RC.config_file_name, i, srs_ackNackST);

		  if (strcmp(srs_MaxUpPts, "ENABLE") == 0) {
		    RRC_CONFIGURATION_REQ (msg_p).srs_MaxUpPts[j] = TRUE;
		  } else if (strcmp(srs_MaxUpPts, "DISABLE") == 0) {
		    RRC_CONFIGURATION_REQ (msg_p).srs_MaxUpPts[j] = FALSE;
		  } else
		    AssertFatal (0,
				 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
				 RC.config_file_name, i, srs_MaxUpPts);
		}

		RRC_CONFIGURATION_REQ (msg_p).pusch_p0_Nominal[j] = pusch_p0_Nominal;

		if ((pusch_p0_Nominal<-126) || (pusch_p0_Nominal>24))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
			       RC.config_file_name, i, pusch_p0_Nominal);

#ifndef Rel14
		if (strcmp(pusch_alpha,"AL0")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = UplinkPowerControlCommon__alpha_al0;
		} else if (strcmp(pusch_alpha,"AL04")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = UplinkPowerControlCommon__alpha_al04;
		} else if (strcmp(pusch_alpha,"AL05")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = UplinkPowerControlCommon__alpha_al05;
		} else if (strcmp(pusch_alpha,"AL06")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = UplinkPowerControlCommon__alpha_al06;
		} else if (strcmp(pusch_alpha,"AL07")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = UplinkPowerControlCommon__alpha_al07;
		} else if (strcmp(pusch_alpha,"AL08")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = UplinkPowerControlCommon__alpha_al08;
		} else if (strcmp(pusch_alpha,"AL09")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = UplinkPowerControlCommon__alpha_al09;
		} else if (strcmp(pusch_alpha,"AL1")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = UplinkPowerControlCommon__alpha_al1;
		} 
#else
		if (strcmp(pusch_alpha,"AL0")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = Alpha_r12_al0;
		} else if (strcmp(pusch_alpha,"AL04")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = Alpha_r12_al04;
		} else if (strcmp(pusch_alpha,"AL05")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = Alpha_r12_al05;
		} else if (strcmp(pusch_alpha,"AL06")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = Alpha_r12_al06;
		} else if (strcmp(pusch_alpha,"AL07")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = Alpha_r12_al07;
		} else if (strcmp(pusch_alpha,"AL08")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = Alpha_r12_al08;
		} else if (strcmp(pusch_alpha,"AL09")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = Alpha_r12_al09;
		} else if (strcmp(pusch_alpha,"AL1")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pusch_alpha[j] = Alpha_r12_al1;
		} 
#endif
		else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
			       RC.config_file_name, i, pusch_alpha);

		RRC_CONFIGURATION_REQ (msg_p).pucch_p0_Nominal[j] = pucch_p0_Nominal;

		if ((pucch_p0_Nominal<-127) || (pucch_p0_Nominal>-96))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
			       RC.config_file_name, i, pucch_p0_Nominal);

		RRC_CONFIGURATION_REQ (msg_p).msg3_delta_Preamble[j] = msg3_delta_Preamble;

		if ((msg3_delta_Preamble<-1) || (msg3_delta_Preamble>6))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
			       RC.config_file_name, i, msg3_delta_Preamble);


		if (strcmp(pucch_deltaF_Format1,"deltaF_2")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format1[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
		} else if (strcmp(pucch_deltaF_Format1,"deltaF0")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format1[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
		} else if (strcmp(pucch_deltaF_Format1,"deltaF2")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format1[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
			       RC.config_file_name, i, pucch_deltaF_Format1);

		if (strcmp(pucch_deltaF_Format1b,"deltaF1")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format1b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
		} else if (strcmp(pucch_deltaF_Format1b,"deltaF3")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format1b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
		} else if (strcmp(pucch_deltaF_Format1b,"deltaF5")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format1b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
			       RC.config_file_name, i, pucch_deltaF_Format1b);


		if (strcmp(pucch_deltaF_Format2,"deltaF_2")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format2[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
		} else if (strcmp(pucch_deltaF_Format2,"deltaF0")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format2[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
		} else if (strcmp(pucch_deltaF_Format2,"deltaF1")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format2[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
		} else if (strcmp(pucch_deltaF_Format2,"deltaF2")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format2[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
			       RC.config_file_name, i, pucch_deltaF_Format2);

		if (strcmp(pucch_deltaF_Format2a,"deltaF_2")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format2a[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
		} else if (strcmp(pucch_deltaF_Format2a,"deltaF0")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format2a[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
		} else if (strcmp(pucch_deltaF_Format2a,"deltaF2")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format2a[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
			       RC.config_file_name, i, pucch_deltaF_Format2a);

		if (strcmp(pucch_deltaF_Format2b,"deltaF_2")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format2b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
		} else if (strcmp(pucch_deltaF_Format2b,"deltaF0")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format2b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
		} else if (strcmp(pucch_deltaF_Format2b,"deltaF2")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).pucch_deltaF_Format2b[j] = DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
			       RC.config_file_name, i, pucch_deltaF_Format2b);




		RRC_CONFIGURATION_REQ (msg_p).rach_numberOfRA_Preambles[j] = (rach_numberOfRA_Preambles/4)-1;

		if ((rach_numberOfRA_Preambles <4) || (rach_numberOfRA_Preambles>64) || ((rach_numberOfRA_Preambles&3)!=0))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
			       RC.config_file_name, i, rach_numberOfRA_Preambles);

		if (strcmp(rach_preamblesGroupAConfig, "ENABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).rach_preamblesGroupAConfig[j] = TRUE;


		  RRC_CONFIGURATION_REQ (msg_p).rach_sizeOfRA_PreamblesGroupA[j] = (rach_sizeOfRA_PreamblesGroupA/4)-1;

		  if ((rach_numberOfRA_Preambles <4) || (rach_numberOfRA_Preambles>60) || ((rach_numberOfRA_Preambles&3)!=0))
		    AssertFatal (0,
				 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_sizeOfRA_PreamblesGroupA choice: 4,8,12,...,60!\n",
				 RC.config_file_name, i, rach_sizeOfRA_PreamblesGroupA);


		  switch (rach_messageSizeGroupA) {
		  case 56:
		    RRC_CONFIGURATION_REQ (msg_p).rach_messageSizeGroupA[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b56;
		    break;

		  case 144:
		    RRC_CONFIGURATION_REQ (msg_p).rach_messageSizeGroupA[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b144;
		    break;

		  case 208:
		    RRC_CONFIGURATION_REQ (msg_p).rach_messageSizeGroupA[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b208;
		    break;

		  case 256:
		    RRC_CONFIGURATION_REQ (msg_p).rach_messageSizeGroupA[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b256;
		    break;

		  default:
		    AssertFatal (0,
				 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
				 RC.config_file_name, i, rach_messageSizeGroupA);
		    break;
		  }

		  if (strcmp(rach_messagePowerOffsetGroupB,"minusinfinity")==0) {
		    RRC_CONFIGURATION_REQ (msg_p).rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
		  }

		  else if (strcmp(rach_messagePowerOffsetGroupB,"dB0")==0) {
		    RRC_CONFIGURATION_REQ (msg_p).rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
		  }

		  else if (strcmp(rach_messagePowerOffsetGroupB,"dB5")==0) {
		    RRC_CONFIGURATION_REQ (msg_p).rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
		  }

		  else if (strcmp(rach_messagePowerOffsetGroupB,"dB8")==0) {
		    RRC_CONFIGURATION_REQ (msg_p).rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
		  }

		  else if (strcmp(rach_messagePowerOffsetGroupB,"dB10")==0) {
		    RRC_CONFIGURATION_REQ (msg_p).rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
		  }

		  else if (strcmp(rach_messagePowerOffsetGroupB,"dB12")==0) {
		    RRC_CONFIGURATION_REQ (msg_p).rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
		  }

		  else if (strcmp(rach_messagePowerOffsetGroupB,"dB15")==0) {
		    RRC_CONFIGURATION_REQ (msg_p).rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
		  }

		  else if (strcmp(rach_messagePowerOffsetGroupB,"dB18")==0) {
		    RRC_CONFIGURATION_REQ (msg_p).rach_messagePowerOffsetGroupB[j] = RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
		  } else
		    AssertFatal (0,
				 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
				 RC.config_file_name, i, rach_messagePowerOffsetGroupB);

		} else if (strcmp(rach_preamblesGroupAConfig, "DISABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).rach_preamblesGroupAConfig[j] = FALSE;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
			       RC.config_file_name, i, rach_preamblesGroupAConfig);

		RRC_CONFIGURATION_REQ (msg_p).rach_preambleInitialReceivedTargetPower[j] = (rach_preambleInitialReceivedTargetPower+120)/2;

		if ((rach_preambleInitialReceivedTargetPower<-120) || (rach_preambleInitialReceivedTargetPower>-90) || ((rach_preambleInitialReceivedTargetPower&1)!=0))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
			       RC.config_file_name, i, rach_preambleInitialReceivedTargetPower);


		RRC_CONFIGURATION_REQ (msg_p).rach_powerRampingStep[j] = rach_powerRampingStep/2;

		if ((rach_powerRampingStep<0) || (rach_powerRampingStep>6) || ((rach_powerRampingStep&1)!=0))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
			       RC.config_file_name, i, rach_powerRampingStep);



		switch (rach_preambleTransMax) {
#ifndef Rel14
		case 3:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n3;
		  break;

		case 4:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n4;
		  break;

		case 5:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n5;
		  break;

		case 6:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n6;
		  break;

		case 7:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n7;
		  break;

		case 8:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n8;
		  break;

		case 10:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10;
		  break;

		case 20:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n20;
		  break;

		case 50:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n50;
		  break;

		case 100:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n100;
		  break;

		case 200:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n200;
		  break;

#else

		case 3:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n3;
		  break;

		case 4:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n4;
		  break;

		case 5:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n5;
		  break;

		case 6:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n6;
		  break;

		case 7:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n7;
		  break;

		case 8:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n8;
		  break;

		case 10:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n10;
		  break;

		case 20:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n20;
		  break;

		case 50:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n50;
		  break;

		case 100:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n100;
		  break;

		case 200:
		  RRC_CONFIGURATION_REQ (msg_p).rach_preambleTransMax[j] =  PreambleTransMax_n200;
		  break;
#endif

		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
			       RC.config_file_name, i, rach_preambleTransMax);
		  break;
		}

		RRC_CONFIGURATION_REQ (msg_p).rach_raResponseWindowSize[j] =  (rach_raResponseWindowSize==10)?7:rach_raResponseWindowSize-2;

		if ((rach_raResponseWindowSize<0)||(rach_raResponseWindowSize==9)||(rach_raResponseWindowSize>10))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
			       RC.config_file_name, i, rach_preambleTransMax);


		RRC_CONFIGURATION_REQ (msg_p).rach_macContentionResolutionTimer[j] = (rach_macContentionResolutionTimer/8)-1;

		if ((rach_macContentionResolutionTimer<8) || (rach_macContentionResolutionTimer>64) || ((rach_macContentionResolutionTimer&7)!=0))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
			       RC.config_file_name, i, rach_preambleTransMax);

		RRC_CONFIGURATION_REQ (msg_p).rach_maxHARQ_Msg3Tx[j] = rach_maxHARQ_Msg3Tx;

		if ((rach_maxHARQ_Msg3Tx<0) || (rach_maxHARQ_Msg3Tx>8))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
			       RC.config_file_name, i, rach_preambleTransMax);


		switch (pcch_defaultPagingCycle) {
		case 32:
		  RRC_CONFIGURATION_REQ (msg_p).pcch_defaultPagingCycle[j] = PCCH_Config__defaultPagingCycle_rf32;
		  break;

		case 64:
		  RRC_CONFIGURATION_REQ (msg_p).pcch_defaultPagingCycle[j] = PCCH_Config__defaultPagingCycle_rf64;
		  break;

		case 128:
		  RRC_CONFIGURATION_REQ (msg_p).pcch_defaultPagingCycle[j] = PCCH_Config__defaultPagingCycle_rf128;
		  break;

		case 256:
		  RRC_CONFIGURATION_REQ (msg_p).pcch_defaultPagingCycle[j] = PCCH_Config__defaultPagingCycle_rf256;
		  break;

		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
			       RC.config_file_name, i, pcch_defaultPagingCycle);
		  break;
		}

		if (strcmp(pcch_nB, "fourT") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pcch_nB[j] = PCCH_Config__nB_fourT;
		} else if (strcmp(pcch_nB, "twoT") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pcch_nB[j] = PCCH_Config__nB_twoT;
		} else if (strcmp(pcch_nB, "oneT") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pcch_nB[j] = PCCH_Config__nB_oneT;
		} else if (strcmp(pcch_nB, "halfT") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pcch_nB[j] = PCCH_Config__nB_halfT;
		} else if (strcmp(pcch_nB, "quarterT") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pcch_nB[j] = PCCH_Config__nB_quarterT;
		} else if (strcmp(pcch_nB, "oneEighthT") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pcch_nB[j] = PCCH_Config__nB_oneEighthT;
		} else if (strcmp(pcch_nB, "oneSixteenthT") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pcch_nB[j] = PCCH_Config__nB_oneSixteenthT;
		} else if (strcmp(pcch_nB, "oneThirtySecondT") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).pcch_nB[j] = PCCH_Config__nB_oneThirtySecondT;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
			       RC.config_file_name, i, pcch_defaultPagingCycle);



		switch (bcch_modificationPeriodCoeff) {
		case 2:
		  RRC_CONFIGURATION_REQ (msg_p).bcch_modificationPeriodCoeff[j] = BCCH_Config__modificationPeriodCoeff_n2;
		  break;

		case 4:
		  RRC_CONFIGURATION_REQ (msg_p).bcch_modificationPeriodCoeff[j] = BCCH_Config__modificationPeriodCoeff_n4;
		  break;

		case 8:
		  RRC_CONFIGURATION_REQ (msg_p).bcch_modificationPeriodCoeff[j] = BCCH_Config__modificationPeriodCoeff_n8;
		  break;

		case 16:
		  RRC_CONFIGURATION_REQ (msg_p).bcch_modificationPeriodCoeff[j] = BCCH_Config__modificationPeriodCoeff_n16;
		  break;

		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
			       RC.config_file_name, i, bcch_modificationPeriodCoeff);

		  break;
		}


		switch (ue_TimersAndConstants_t300) {
		case 100:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms100;
		  break;

		case 200:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms200;
		  break;

		case 300:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms300;
		  break;

		case 400:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms400;
		  break;

		case 600:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms600;
		  break;

		case 1000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms1000;
		  break;

		case 1500:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms1500;
		  break;

		case 2000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t300[j] = UE_TimersAndConstants__t300_ms2000;
		  break;

		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t300 choice: 100,200,300,400,600,1000,1500,2000 ",
			       RC.config_file_name, i, ue_TimersAndConstants_t300);
		  break;

		}

		switch (ue_TimersAndConstants_t301) {
		case 100:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms100;
		  break;

		case 200:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms200;
		  break;

		case 300:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms300;
		  break;

		case 400:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms400;
		  break;

		case 600:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms600;
		  break;

		case 1000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms1000;
		  break;

		case 1500:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms1500;
		  break;

		case 2000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t301[j] = UE_TimersAndConstants__t301_ms2000;
		  break;

		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t301 choice: 100,200,300,400,600,1000,1500,2000 ",
			       RC.config_file_name, i, ue_TimersAndConstants_t301);
		  break;

		}

		switch (ue_TimersAndConstants_t310) {
		case 0:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms0;
		  break;

		case 50:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms50;
		  break;

		case 100:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms100;
		  break;

		case 200:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms200;
		  break;

		case 500:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms500;
		  break;

		case 1000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms1000;
		  break;

		case 2000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t310[j] = UE_TimersAndConstants__t310_ms2000;
		  break;

		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t310 choice: 0,50,100,200,500,1000,1500,2000 ",
			       RC.config_file_name, i, ue_TimersAndConstants_t310);
		  break;

		}

		switch (ue_TimersAndConstants_t311) {
		case 1000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms1000;
		  break;

		case 3110:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms3000;
		  break;

		case 5000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms5000;
		  break;

		case 10000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms10000;
		  break;

		case 15000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms15000;
		  break;

		case 20000:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms20000;
		  break;

		case 31100:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_t311[j] = UE_TimersAndConstants__t311_ms30000;
		  break;

		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 choice: 1000,3000,5000,10000,150000,20000,30000",
			       RC.config_file_name, i, ue_TimersAndConstants_t311);
		  break;

		}

		switch (ue_TimersAndConstants_n310) {
		case 1:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n1;
		  break;

		case 2:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n2;
		  break;

		case 3:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n3;
		  break;

		case 4:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n4;
		  break;

		case 6:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n6;
		  break;

		case 8:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n8;
		  break;

		case 10:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n10;
		  break;

		case 20:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n310[j] = UE_TimersAndConstants__n310_n20;
		  break;

		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_n310 choice: 1,2,3,4,6,6,8,10,20",
			       RC.config_file_name, i, ue_TimersAndConstants_n311);
		  break;

		}

		switch (ue_TimersAndConstants_n311) {
		case 1:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n1;
		  break;

		case 2:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n2;
		  break;

		case 3:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n3;
		  break;

		case 4:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n4;
		  break;

		case 5:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n5;
		  break;

		case 6:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n6;
		  break;

		case 8:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n8;
		  break;

		case 10:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TimersAndConstants_n311[j] = UE_TimersAndConstants__n311_n10;
		  break;

		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TimersAndConstants_t311 choice: 1,2,3,4,5,6,8,10",
			       RC.config_file_name, i, ue_TimersAndConstants_t311);
		  break;

		}

		switch (ue_TransmissionMode) {
		case 1:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm1;
		  break;
		case 2:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm2;
		  break;
		case 3:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm3;
		  break;
		case 4:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm4;
		  break;
		case 5:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm5;
		  break;
		case 6:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm6;
		  break;
		case 7:
		  RRC_CONFIGURATION_REQ (msg_p).ue_TransmissionMode[j] = AntennaInfoDedicated__transmissionMode_tm7;
		  break;
		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode choice: 1,2,3,4,5,6,7",
			       RC.config_file_name, i, ue_TransmissionMode);
		  break;
		}

#ifdef UE_EXPANSION
        RRC_CONFIGURATION_REQ (msg_p).ue_multiple_max[j] = ue_multiple_max;

        switch (N_RB_DL) {
        case 25:
          if ((ue_multiple_max < 1) || (ue_multiple_max > 4))
            AssertFatal (0,
                     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_multiple_max choice: 1..4!\n",
                     RC.config_file_name, i, ue_multiple_max);
          break;
        case 50:
          if ((ue_multiple_max < 1) || (ue_multiple_max > 6))
            AssertFatal (0,
                     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_multiple_max choice: 1..6!\n",
                     RC.config_file_name, i, ue_multiple_max);
          break;
        case 100:
          if ((ue_multiple_max < 1) || (ue_multiple_max > 10))
            AssertFatal (0,
                     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_multiple_max choice: 1..10!\n",
                     RC.config_file_name, i, ue_multiple_max);
          break;
        default:
          AssertFatal (0,
                   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 25,50,100 !\n",
                   RC.config_file_name, i, N_RB_DL);
          break;
        }
#endif
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
 
  printf("Getting ENBSParams\n");
 
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL); 
  RC.nb_inst = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
 
  if (RC.nb_inst > 0) {
    RC.nb_CC = (int *)malloc((1+RC.nb_inst)*sizeof(int));
    for (int i=0;i<RC.nb_inst;i++) {
      sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,i);
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
