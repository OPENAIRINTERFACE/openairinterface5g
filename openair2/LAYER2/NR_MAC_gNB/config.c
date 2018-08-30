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

/*! \file config.c
 * \brief gNB configuration performed by RRC or as a consequence of RRC procedures
 * \author  Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \version 0.1
 * \company Eurecom, NTUST
 * \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
 * @ingroup _mac

 */

#include "COMMON/platform_types.h"
#include "COMMON/platform_constants.h"
#include "common/ran_context.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "NR_BCCH-BCH-Message.h"
#include "NR_ServingCellConfigCommon.h"

#include "LAYER2/NR_MAC_gNB/mac_proto.h"

#include "NR_MIB.h"

extern RAN_CONTEXT_t RC;
//extern int l2_init_gNB(void);
extern void mac_top_init_gNB(void);
extern uint8_t nfapi_mode;

int32_t **rxdata;
int32_t **txdata;

typedef struct eutra_bandentry_s {
  int16_t band;
  uint32_t ul_min;
  uint32_t ul_max;
  uint32_t dl_min;
  uint32_t dl_max;
  uint32_t N_OFFs_DL;
} eutra_bandentry_t;

typedef struct band_info_s {
  int nbands;
  eutra_bandentry_t band_info[100];
} band_info_t;

static const eutra_bandentry_t eutra_bandtable[] = {
  {1, 19200, 19800, 21100, 21700, 0},
  {2, 18500, 19100, 19300, 19900, 6000},
  {3, 17100, 17850, 18050, 18800, 12000},
  {4, 17100, 17550, 21100, 21550, 19500},
  {5, 8240, 8490, 8690, 8940, 24000},
  {6, 8300, 8400, 8750, 8850, 26500},
  {7, 25000, 25700, 26200, 26900, 27500},
  {8, 8800, 9150, 9250, 9600, 34500},
  {9, 17499, 17849, 18449, 18799, 38000},
  {10, 17100, 17700, 21100, 21700, 41500},
  {11, 14279, 14529, 14759, 15009, 47500},
  {12, 6980, 7160, 7280, 7460, 50100},
  {13, 7770, 7870, 7460, 7560, 51800},
  {14, 7880, 7980, 7580, 7680, 52800},
  {17, 7040, 7160, 7340, 7460, 57300},
  {18, 8150, 9650, 8600, 10100, 58500},
  {19, 8300, 8450, 8750, 8900, 60000},
  {20, 8320, 8620, 7910, 8210, 61500},
  {21, 14479, 14629, 14959, 15109, 64500},
  {22, 34100, 34900, 35100, 35900, 66000},
  {23, 20000, 20200, 21800, 22000, 75000},
  {24, 16126, 16605, 15250, 15590, 77000},
  {25, 18500, 19150, 19300, 19950, 80400},
  {26, 8140, 8490, 8590, 8940, 86900},
  {27, 8070, 8240, 8520, 8690, 90400},
  {28, 7030, 7580, 7580, 8130, 92100},
  {29, 0, 0, 7170, 7280, 96600},
  {30, 23050, 23250, 23500, 23600, 97700},
  {31, 45250, 34900, 46250, 35900, 98700},
  {32, 0, 0, 14520, 14960, 99200},
  {33, 19000, 19200, 19000, 19200, 36000},
  {34, 20100, 20250, 20100, 20250, 36200},
  {35, 18500, 19100, 18500, 19100, 36350},
  {36, 19300, 19900, 19300, 19900, 36950},
  {37, 19100, 19300, 19100, 19300, 37550},
  {38, 25700, 26200, 25700, 26300, 37750},
  {39, 18800, 19200, 18800, 19200, 38250},
  {40, 23000, 24000, 23000, 24000, 38650},
  {41, 24960, 26900, 24960, 26900, 39650},
  {42, 34000, 36000, 34000, 36000, 41590},
  {43, 36000, 38000, 36000, 38000, 43590},
  {44, 7030, 8030, 7030, 8030, 45590},
  {45, 14470, 14670, 14470, 14670, 46590},
  {46, 51500, 59250, 51500, 59250, 46790},
  {65, 19200, 20100, 21100, 22000, 65536},
  {66, 17100, 18000, 21100, 22000, 66436},
  {67, 0, 0, 7380, 7580, 67336},
  {68, 6980, 7280, 7530, 7830, 67536}
};


uint32_t nr_to_earfcn(int eutra_bandP, uint32_t dl_CarrierFreq, uint32_t bw)
{

  uint32_t dl_CarrierFreq_by_100k = dl_CarrierFreq / 100000;
  int bw_by_100 = bw / 100;

  int i;

  AssertFatal(eutra_bandP < 69, "eutra_band %d > 68\n", eutra_bandP);
  for (i = 0; i < 69 && eutra_bandtable[i].band != eutra_bandP; i++);

  AssertFatal(dl_CarrierFreq_by_100k >= eutra_bandtable[i].dl_min,
        "Band %d, bw %u : DL carrier frequency %u Hz < %u\n",
        eutra_bandP, bw, dl_CarrierFreq,
        eutra_bandtable[i].dl_min);
  AssertFatal(dl_CarrierFreq_by_100k <=
        (eutra_bandtable[i].dl_max - bw_by_100),
        "Band %d, bw %u: DL carrier frequency %u Hz > %d\n",
        eutra_bandP, bw, dl_CarrierFreq,
        eutra_bandtable[i].dl_max - bw_by_100);


  return (dl_CarrierFreq_by_100k - eutra_bandtable[i].dl_min +
    (eutra_bandtable[i].N_OFFs_DL / 10));
}


void config_nr_mib(int Mod_idP, 
                int CC_idP,
                int p_gNBP,
                int subCarrierSpacingCommon, 
                uint32_t ssb_SubcarrierOffset,
                int dmrs_TypeA_Position,
                uint32_t pdcch_ConfigSIB1,
                int cellBarred,
                int intraFreqReselection
                ){
  nfapi_nr_config_request_t *cfg = &RC.nrmac[Mod_idP]->config[CC_idP];

  cfg->num_tlv=0;
  
  cfg->rf_config.tx_antenna_ports.value            = p_gNBP;
  cfg->rf_config.tx_antenna_ports.tl.tag = NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG;
  cfg->num_tlv++;
  
  cfg->sch_config.ssb_subcarrier_offset.value = ssb_SubcarrierOffset;

  
}

void config_common(int Mod_idP, 
                   int CC_idP,
                   int eutra_bandP,
                   int dl_CarrierFreqP,
                   int dl_BandwidthP
                  ){

  nfapi_nr_config_request_t *cfg = &RC.nrmac[Mod_idP]->config[CC_idP];

  // FDD
  cfg->subframe_config.duplex_mode.value                          = 1;
  cfg->subframe_config.duplex_mode.tl.tag = NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG;
  cfg->num_tlv++;
  
  /// In NR DL and UL will be different band
  cfg->nfapi_config.rf_bands.number_rf_bands       = 1;
  cfg->nfapi_config.rf_bands.rf_band[0]            = eutra_bandP;  
  cfg->nfapi_config.rf_bands.tl.tag = NFAPI_PHY_RF_BANDS_TAG;
  cfg->num_tlv++;

  cfg->nfapi_config.earfcn.value                   = nr_to_earfcn(eutra_bandP,dl_CarrierFreqP,dl_BandwidthP*180/100);
  cfg->nfapi_config.earfcn.tl.tag = NFAPI_NFAPI_EARFCN_TAG;
  cfg->num_tlv++;

  cfg->subframe_config.numerology_index_mu.value = 1;
  //cfg->subframe_config.tl.tag = 
  //cfg->num_tlv++;

  cfg->rf_config.dl_carrier_bandwidth.value    = dl_BandwidthP;
  cfg->rf_config.dl_carrier_bandwidth.tl.tag   = NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG; //temporary
  cfg->num_tlv++;
  LOG_I(PHY,"%s() dl_BandwidthP:%d\n", __FUNCTION__, dl_BandwidthP);

  cfg->rf_config.ul_carrier_bandwidth.value    = dl_BandwidthP;
  cfg->rf_config.ul_carrier_bandwidth.tl.tag   = NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG;  //temporary
  cfg->num_tlv++;

  //cfg->sch_config.half_frame_index.value = 0; Fix in PHY
  //cfg->sch_config.n_ssb_crb.value = 86;       Fix in PHY

}

/*void config_servingcellconfigcommon(){

}*/

int rrc_mac_config_req_gNB(module_id_t Mod_idP, 
                           int CC_idP,
                           int p_gNB,
                           int eutra_bandP,
                           int dl_CarrierFreqP,
                           int dl_BandwidthP,
                           NR_BCCH_BCH_Message_t *mib,
                           NR_ServingCellConfigCommon_t *servingcellconfigcommon
                           ){


  if( mib != NULL ){
    config_nr_mib(Mod_idP, 
               CC_idP,
               p_gNB, 
               mib->message.choice.mib->subCarrierSpacingCommon,
               mib->message.choice.mib->ssb_SubcarrierOffset,
               mib->message.choice.mib->dmrs_TypeA_Position,
               mib->message.choice.mib->pdcch_ConfigSIB1,
               mib->message.choice.mib->cellBarred,
               mib->message.choice.mib->intraFreqReselection
               );
  }// END if( mib != NULL )

  if( servingcellconfigcommon != NULL ){
    config_common(Mod_idP, 
                  CC_idP,
                  eutra_bandP,
                  dl_CarrierFreqP,
                  dl_BandwidthP
                  );  
  }//END if( servingcellconfigcommon != NULL )



  LOG_E(MAC, "%s() %s:%d RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req:%p\n", __FUNCTION__, __FILE__, __LINE__, RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req);

  // if in nFAPI mode 
  if ( (nfapi_mode == 1 || nfapi_mode == 2) && (RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req == NULL) ){
    while(RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req == NULL) {
      // DJP AssertFatal(RC.nrmac[Mod_idP]->if_inst->PHY_config_req != NULL,"if_inst->phy_config_request is null\n");
      usleep(100 * 1000);
      printf("Waiting for PHY_config_req\n");
    }
  }

  if (servingcellconfigcommon != NULL){
    NR_PHY_Config_t phycfg;
    phycfg.Mod_id = Mod_idP;
    phycfg.CC_id  = CC_idP;
    phycfg.cfg    = &RC.nrmac[Mod_idP]->config[CC_idP];
      
    if (RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req) RC.nrmac[Mod_idP]->if_inst->NR_PHY_config_req(&phycfg); 
      
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RRC_MAC_CONFIG, VCD_FUNCTION_OUT);
  }
    
  return(0);

}// END rrc_mac_config_req_gNB

