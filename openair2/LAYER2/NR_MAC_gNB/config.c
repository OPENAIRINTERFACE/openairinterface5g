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

//int32_t **rxdata;
//int32_t **txdata;

nr_bandentry_t nr_bandtable[] = {
  {1,  1920000, 1980000, 2110000, 2170000, 20, 422000},
  {2,  1850000, 1910000, 1930000, 1990000, 20, 386000},
  {3,  1710000, 1785000, 1805000, 1880000, 20, 361000},
  {5,   824000,  849000,  869000,  894000, 20, 173800},
  {7,  2500000, 2570000, 2620000, 2690000, 20, 524000},
  {8,   880000,  915000,  925000,  960000, 20, 185000},
  {12,  698000,  716000,  728000,  746000, 20, 145800},
  {20,  832000,  862000,  791000,  821000, 20, 158200},
  {25, 1850000, 1915000, 1930000, 1995000, 20, 386000},
  {28,  703000,  758000,  758000,  813000, 20, 151600},
  {34, 2010000, 2025000, 2010000, 2025000, 20, 402000},
  {38, 2570000, 2620000, 2570000, 2630000, 20, 514000},
  {39, 1880000, 1920000, 1880000, 1920000, 20, 376000},
  {40, 2300000, 2400000, 2300000, 2400000, 20, 460000},
  {41, 2496000, 2690000, 2496000, 2690000,  3, 499200},
  {50, 1432000, 1517000, 1432000, 1517000, 20, 286400},
  {51, 1427000, 1432000, 1427000, 1432000, 20, 285400},
  {66, 1710000, 1780000, 2110000, 2200000, 20, 422000},
  {70, 1695000, 1710000, 1995000, 2020000, 20, 399000},
  {71,  663000,  698000,  617000,  652000, 20, 123400},
  {74, 1427000, 1470000, 1475000, 1518000, 20, 295000},
  {75,     000,     000, 1432000, 1517000, 20, 286400},
  {76,     000,     000, 1427000, 1432000, 20, 285400},
  {77, 3300000, 4200000, 3300000, 4200000,  1, 620000},
  {78, 3300000, 3800000, 3300000, 3800000,  1, 620000},
  {79, 4400000, 5000000, 4400000, 5000000,  2, 693334},
  {80, 1710000, 1785000,     000,     000, 20, 342000},
  {81,  860000,  915000,     000,     000, 20, 176000},
  {82,  832000,  862000,     000,     000, 20, 166400},
  {83,  703000,  748000,     000,     000, 20, 140600},
  {84, 1920000, 1980000,     000,     000, 20, 384000},
  {86, 1710000, 1785000,     000,     000, 20, 342000}
};

void get_band(uint32_t downlink_frequency,   uint8_t *current_band,   int32_t *current_offset, lte_frame_type_t *current_type) {

    int ind;
    int64_t dl_freq_khz = downlink_frequency/1000;
    for ( ind=0;
          ind < sizeof(nr_bandtable) / sizeof(nr_bandtable[0]);
          ind++) {

      *current_band = nr_bandtable[ind].band;
      LOG_I(PHY, "Scanning band %d, dl_min %"PRIu64", ul_min %"PRIu64"\n", ind, nr_bandtable[ind].dl_min,nr_bandtable[ind].ul_min);

      if ( nr_bandtable[ind].dl_min <= dl_freq_khz && nr_bandtable[ind].dl_max >= dl_freq_khz ) {
	*current_offset = (nr_bandtable[ind].ul_min - nr_bandtable[ind].dl_min)*1000;
	if (*current_offset == 0)
	  *current_type = TDD;
	else
	  *current_type = FDD;

	LOG_I( PHY, "DL frequency %"PRIu32": band %d, frame_type %d, UL frequency %"PRIu32"\n",
	       downlink_frequency, *current_band, *current_type, downlink_frequency+*current_offset);
        break;
      }
    }

    AssertFatal(ind != (sizeof(nr_bandtable) / sizeof(nr_bandtable[0])),
	    "Can't find EUTRA band for frequency %d\n", downlink_frequency);

}

uint32_t to_nrarfcn(int nr_bandP, uint64_t dl_CarrierFreq, uint32_t bw)
{

  uint64_t dl_CarrierFreq_by_1k = dl_CarrierFreq / 1000;
  int bw_kHz = bw / 1000;

  int i;

  LOG_I(MAC,"Searching for nr band %d DL Carrier frequency %llu bw %u\n",nr_bandP,(long long unsigned int)dl_CarrierFreq,bw);
  AssertFatal(nr_bandP < 86, "nr_band %d > 86\n", nr_bandP);
  for (i = 0; i < 30 && nr_bandtable[i].band != nr_bandP; i++);

  AssertFatal(dl_CarrierFreq_by_1k >= nr_bandtable[i].dl_min,
        "Band %d, bw %u : DL carrier frequency %llu kHz < %llu\n",
	      nr_bandP, bw, (long long unsigned int)dl_CarrierFreq_by_1k,
	      (long long unsigned int)nr_bandtable[i].dl_min);
  AssertFatal(dl_CarrierFreq_by_1k <=
        (nr_bandtable[i].dl_max - bw_kHz),
        "Band %d, dl_CarrierFreq %llu bw %u: DL carrier frequency %llu kHz > %llu\n",
	      nr_bandP, (long long unsigned int)dl_CarrierFreq,bw, (long long unsigned int)dl_CarrierFreq_by_1k,
	      (long long unsigned int)(nr_bandtable[i].dl_max - bw_kHz));
 
  int deltaFglobal;

  if (dl_CarrierFreq < 3e9) deltaFglobal = 5;
  else                      deltaFglobal = 15;

  // This is equation before Table 5.4.2.1-1 in 38101-1-f30
  // F_REF=F_REF_Offs + deltaF_Global(N_REF-NREF_REF_Offs)
  return (((dl_CarrierFreq_by_1k - nr_bandtable[i].dl_min)/deltaFglobal) +
	  nr_bandtable[i].N_OFFs_DL);
}


uint64_t from_nrarfcn(int nr_bandP, uint32_t dl_nrarfcn)
{

  int i;
  int deltaFglobal;

  if (nr_bandP < 77 || nr_bandP > 79) deltaFglobal = 5;
  else                                deltaFglobal = 15;
  
  AssertFatal(nr_bandP < 87, "nr_band %d > 86\n", nr_bandP);
  for (i = 0; i < 31 && nr_bandtable[i].band != nr_bandP; i++);
  AssertFatal(dl_nrarfcn>=nr_bandtable[i].N_OFFs_DL,"dl_nrarfcn %u < N_OFFs_DL %llu\n",dl_nrarfcn, (long long unsigned int)nr_bandtable[i].N_OFFs_DL);
 
  return 1000*(nr_bandtable[i].dl_min + (dl_nrarfcn - nr_bandtable[i].N_OFFs_DL) * deltaFglobal);
	  
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
  
  cfg->rf_config.dl_subcarrierspacing.value  = subCarrierSpacingCommon;

  cfg->rf_config.dl_subcarrierspacing.tl.tag = NFAPI_NR_RF_CONFIG_DL_SUBCARRIERSPACING_TAG;
  cfg->num_tlv++;
  
  cfg->rf_config.ul_subcarrierspacing.value  = subCarrierSpacingCommon;
  cfg->rf_config.ul_subcarrierspacing.tl.tag = NFAPI_NR_RF_CONFIG_UL_SUBCARRIERSPACING_TAG;
  cfg->num_tlv++;

  cfg->sch_config.ssb_subcarrier_offset.value = ssb_SubcarrierOffset;
  cfg->sch_config.ssb_subcarrier_offset.tl.tag = NFAPI_NR_SCH_CONFIG_SSB_SUBCARRIER_OFFSET_TAG;
  cfg->num_tlv++; 
}

void config_common(int Mod_idP, 
                   int CC_idP,
		   int cellid,
                   int nr_bandP,
                   uint64_t ssb_pattern,
		   uint64_t dl_CarrierFreqP,
                   uint32_t dl_BandwidthP
                  ){

  nfapi_nr_config_request_t *cfg = &RC.nrmac[Mod_idP]->config[CC_idP];

  int mu = 1;

  cfg->sch_config.physical_cell_id.value = cellid;
  cfg->sch_config.ssb_scg_position_in_burst.value = ssb_pattern;

  // FDD
  cfg->subframe_config.duplex_mode.value                          = 1;
  cfg->subframe_config.duplex_mode.tl.tag = NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG;
  cfg->num_tlv++;
  
  /// In NR DL and UL will be different band
  cfg->nfapi_config.rf_bands.number_rf_bands       = 1;
  cfg->nfapi_config.rf_bands.rf_band[0]            = nr_bandP;  
  cfg->nfapi_config.rf_bands.tl.tag = NFAPI_PHY_RF_BANDS_TAG;
  cfg->num_tlv++;

  cfg->nfapi_config.nrarfcn.value                   = to_nrarfcn(nr_bandP,dl_CarrierFreqP,dl_BandwidthP*180000*(1+mu));
  cfg->nfapi_config.nrarfcn.tl.tag = NFAPI_NR_NFAPI_NRARFCN_TAG;
  cfg->num_tlv++;

  cfg->subframe_config.numerology_index_mu.value = mu;
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
			   int cellid,
                           int p_gNB,
                           int nr_bandP,
			   uint64_t ssb_pattern,
                           uint64_t dl_CarrierFreqP,
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
#if (NR_RRC_VERSION >= MAKE_VERSION(15, 3, 0))
               mib->message.choice.mib->pdcch_ConfigSIB1.controlResourceSetZero * 16 + mib->message.choice.mib->pdcch_ConfigSIB1.searchSpaceZero,
#else
               mib->message.choice.mib->pdcch_ConfigSIB1,
#endif
               mib->message.choice.mib->cellBarred,
               mib->message.choice.mib->intraFreqReselection
               );
  }// END if( mib != NULL )


  if( servingcellconfigcommon != NULL ){
    config_common(Mod_idP, 
                  CC_idP,
		  cellid,
                  nr_bandP,
		  ssb_pattern,
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

