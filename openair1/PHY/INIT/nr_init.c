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

#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"
#include "RadioResourceConfigCommonSIB.h"
#include "RadioResourceConfigDedicated.h"
#include "TDD-Config.h"
#include "LAYER2/MAC/mac_extern.h"
#include "MBSFN-SubframeConfigList.h"
#include "assertions.h"
#include <math.h>

#include "PHY/NR_REFSIG/nr_refsig.h" 
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "SCHED_NR/fapi_nr_l1.h"

extern uint32_t from_earfcn(int eutra_bandP,uint32_t dl_earfcn);
extern int32_t get_uldl_offset(int eutra_bandP);

int l1_north_init_gNB() {

  int i,j;

  if (RC.nb_nr_L1_inst > 0 && RC.nb_nr_L1_CC != NULL && RC.gNB != NULL)
  {
    AssertFatal(RC.nb_nr_L1_inst>0,"nb_nr_L1_inst=%d\n",RC.nb_nr_L1_inst);
    AssertFatal(RC.nb_nr_L1_CC!=NULL,"nb_nr_L1_CC is null\n");
    AssertFatal(RC.gNB!=NULL,"RC.gNB is null\n");

    LOG_I(PHY,"%s() RC.nb_nr_L1_inst:%d\n", __FUNCTION__, RC.nb_nr_L1_inst);

    for (i=0;i<RC.nb_nr_L1_inst;i++) {
      AssertFatal(RC.gNB[i]!=NULL,"RC.gNB[%d] is null\n",i);
      AssertFatal(RC.nb_nr_L1_CC[i]>0,"RC.nb_nr_L1_CC[%d]=%d\n",i,RC.nb_nr_L1_CC[i]);

      LOG_I(PHY,"%s() RC.nb_nr_L1_CC[%d]:%d\n", __FUNCTION__, i,  RC.nb_nr_L1_CC[i]);

      for (j=0;j<RC.nb_nr_L1_CC[i];j++) {
        AssertFatal(RC.gNB[i][j]!=NULL,"RC.gNB[%d][%d] is null\n",i,j);

        if ((RC.gNB[i][j]->if_inst =  NR_IF_Module_init(i))<0) return(-1); 

        LOG_I(PHY,"%s() RC.gNB[%d][%d] installing callbacks\n", __FUNCTION__, i,  j);

        RC.gNB[i][j]->if_inst->NR_PHY_config_req = nr_phy_config_request;
        RC.gNB[i][j]->if_inst->NR_Schedule_response = nr_schedule_response;
      }
    }
  }
  else
  {
    LOG_I(PHY,"%s() Not installing PHY callbacks - RC.nb_nr_L1_inst:%d RC.nb_nr_L1_CC:%p RC.gNB:%p\n", __FUNCTION__, RC.nb_nr_L1_inst, RC.nb_nr_L1_CC, RC.gNB);
  }
  return(0);
}


int phy_init_nr_gNB(PHY_VARS_gNB *gNB,
                     unsigned char is_secondary_gNB,
                     unsigned char abstraction_flag)
{

  // shortcuts
  NR_DL_FRAME_PARMS* const fp       = &gNB->frame_parms;
  nfapi_nr_config_request_t* cfg       = &gNB->gNB_config;
  NR_gNB_COMMON* const common_vars  = &gNB->common_vars;
  LTE_eNB_PUSCH** const pusch_vars   = gNB->pusch_vars;
  LTE_eNB_SRS* const srs_vars        = gNB->srs_vars;
  LTE_eNB_PRACH* const prach_vars    = &gNB->prach_vars;

  int i, UE_id; 

  LOG_I(PHY,"[gNB %d] %s() About to wait for gNB to be configured", gNB->Mod_id, __FUNCTION__);

  gNB->total_dlsch_bitrate = 0;
  gNB->total_transmitted_bits = 0;
  gNB->total_system_throughput = 0;
  gNB->check_for_MUMIMO_transmissions=0;
 
  while(gNB->configured == 0) usleep(10000);
/*
  LOG_I(PHY,"[gNB %"PRIu8"] Initializing DL_FRAME_PARMS : N_RB_DL %"PRIu8", PHICH Resource %d, PHICH Duration %d nb_antennas_tx:%u nb_antennas_rx:%u PRACH[rootSequenceIndex:%u prach_Config_enabled:%u configIndex:%u highSpeed:%u zeroCorrelationZoneConfig:%u freqOffset:%u]\n",
        gNB->Mod_id,
        fp->N_RB_DL,fp->phich_config_common.phich_resource,
        fp->phich_config_common.phich_duration,
        fp->nb_antennas_tx, fp->nb_antennas_rx,
        fp->prach_config_common.rootSequenceIndex,
        fp->prach_config_common.prach_Config_enabled,
        fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
        fp->prach_config_common.prach_ConfigInfo.highSpeedFlag,
        fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig,
        fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset
        );*/
  LOG_D(PHY,"[MSC_NEW][FRAME 00000][PHY_gNB][MOD %02"PRIu8"][]\n", gNB->Mod_id);

  // PBCH DMRS gold sequences generation
  nr_init_pbch_dmrs(gNB);
  // Polar encoder init for PBCH
  nr_polar_init(&gNB->nrPolar_params,
        NR_POLAR_PBCH_MESSAGE_TYPE,
				NR_POLAR_PBCH_PAYLOAD_BITS,
				NR_POLAR_PBCH_AGGREGATION_LEVEL);

  //PDCCH DMRS init
  gNB->nr_gold_pdcch_dmrs = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t**));
  uint32_t ***pdcch_dmrs             = gNB->nr_gold_pdcch_dmrs;
  AssertFatal(pdcch_dmrs!=NULL, "NR init: pdcch_dmrs malloc failed\n");
  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    pdcch_dmrs[slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t*));
    AssertFatal(pdcch_dmrs[slot]!=NULL, "NR init: pdcch_dmrs for slot %d - malloc failed\n", slot);
    for (int symb=0; symb<fp->symbols_per_slot; symb++){
      pdcch_dmrs[slot][symb] = (uint32_t *)malloc16(NR_MAX_PDCCH_DMRS_INIT_LENGTH_DWORD*sizeof(uint32_t));
      AssertFatal(pdcch_dmrs[slot][symb]!=NULL, "NR init: pdcch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);
    }
  }

  nr_init_pdcch_dmrs(gNB, cfg->sch_config.physical_cell_id.value);

/*
  lte_gold(fp,gNB->lte_gold_table,fp->Nid_cell);
  generate_pcfich_reg_mapping(fp);
  generate_phich_reg_mapping(fp);*/

  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    gNB->first_run_timing_advance[UE_id] =
      1; ///This flag used to be static. With multiple gNBs this does no longer work, hence we put it in the structure. However it has to be initialized with 1, which is performed here.

    // clear whole structure
    bzero( &gNB->UE_stats[UE_id], sizeof(LTE_eNB_UE_stats) );
    
    gNB->physicalConfigDedicated[UE_id] = NULL;
  }
  
  gNB->first_run_I0_measurements = 1; ///This flag used to be static. With multiple gNBs this does no longer work, hence we put it in the structure. However it has to be initialized with 1, which is performed here.


  common_vars->rxdata  = (int32_t **)NULL;
  common_vars->txdataF = (int32_t **)malloc16(NB_ANTENNA_PORTS_ENB*sizeof(int32_t*));
  common_vars->rxdataF = (int32_t **)malloc16(64*sizeof(int32_t*));
  
  LOG_D(PHY,"[INIT] NB_ANTENNA_PORTS_ENB:%d fp->nb_antenna_ports_gNB:%d\n", NB_ANTENNA_PORTS_ENB, cfg->rf_config.tx_antenna_ports.value);

  for (i=0; i<NB_ANTENNA_PORTS_ENB; i++) {
    if (i<cfg->rf_config.tx_antenna_ports.value || i==5) {
      common_vars->txdataF[i] = (int32_t*)malloc16_clear(fp->samples_per_frame_wCP*sizeof(int32_t) );
      
      LOG_D(PHY,"[INIT] common_vars->txdataF[%d] = %p (%lu bytes)\n",
	    i,common_vars->txdataF[i],
	    fp->samples_per_frame_wCP*sizeof(int32_t));
    }
  }  
  
  
  // Channel estimates for SRS
  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    
    srs_vars[UE_id].srs_ch_estimates      = (int32_t**)malloc16( 64*sizeof(int32_t*) );
    srs_vars[UE_id].srs_ch_estimates_time = (int32_t**)malloc16( 64*sizeof(int32_t*) );
    
    for (i=0; i<64; i++) {
      srs_vars[UE_id].srs_ch_estimates[i]      = (int32_t*)malloc16_clear( sizeof(int32_t)*fp->ofdm_symbol_size );
      srs_vars[UE_id].srs_ch_estimates_time[i] = (int32_t*)malloc16_clear( sizeof(int32_t)*fp->ofdm_symbol_size*2 );
    }
  } //UE_id


  /*generate_ul_ref_sigs_rx();
  
  init_ulsch_power_LUT();*/

  // SRS
  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    srs_vars[UE_id].srs = (int32_t*)malloc16_clear(2*fp->ofdm_symbol_size*sizeof(int32_t));
  }

  // PRACH
  prach_vars->prachF = (int16_t*)malloc16_clear( 1024*2*sizeof(int16_t) );

  // assume maximum of 64 RX antennas for PRACH receiver
  prach_vars->prach_ifft[0]    = (int32_t**)malloc16_clear(64*sizeof(int32_t*)); 
  for (i=0;i<64;i++) prach_vars->prach_ifft[0][i]    = (int32_t*)malloc16_clear(1024*2*sizeof(int32_t)); 

  prach_vars->rxsigF[0]        = (int16_t**)malloc16_clear(64*sizeof(int16_t*));

  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    
    //FIXME
    pusch_vars[UE_id] = (LTE_eNB_PUSCH*)malloc16_clear( NUMBER_OF_UE_MAX*sizeof(LTE_eNB_PUSCH) );
    
    pusch_vars[UE_id]->rxdataF_ext      = (int32_t**)malloc16( 2*sizeof(int32_t*) );
    pusch_vars[UE_id]->rxdataF_ext2     = (int32_t**)malloc16( 2*sizeof(int32_t*) );
    pusch_vars[UE_id]->drs_ch_estimates = (int32_t**)malloc16( 2*sizeof(int32_t*) );
    pusch_vars[UE_id]->drs_ch_estimates_time = (int32_t**)malloc16( 2*sizeof(int32_t*) );
    pusch_vars[UE_id]->rxdataF_comp     = (int32_t**)malloc16( 2*sizeof(int32_t*) );
    pusch_vars[UE_id]->ul_ch_mag  = (int32_t**)malloc16( 2*sizeof(int32_t*) );
    pusch_vars[UE_id]->ul_ch_magb = (int32_t**)malloc16( 2*sizeof(int32_t*) );
    
    for (i=0; i<2; i++) {
      // RK 2 times because of output format of FFT!
      // FIXME We should get rid of this
      pusch_vars[UE_id]->rxdataF_ext[i]      = (int32_t*)malloc16_clear( sizeof(int32_t)*cfg->rf_config.ul_carrier_bandwidth.value*12*fp->symbols_per_slot );
      pusch_vars[UE_id]->rxdataF_ext2[i]     = (int32_t*)malloc16_clear( sizeof(int32_t)*cfg->rf_config.ul_carrier_bandwidth.value*12*fp->symbols_per_slot );
      pusch_vars[UE_id]->drs_ch_estimates[i] = (int32_t*)malloc16_clear( sizeof(int32_t)*cfg->rf_config.ul_carrier_bandwidth.value*12*fp->symbols_per_slot );
      pusch_vars[UE_id]->drs_ch_estimates_time[i] = (int32_t*)malloc16_clear( 2*sizeof(int32_t)*fp->ofdm_symbol_size );
      pusch_vars[UE_id]->rxdataF_comp[i]     = (int32_t*)malloc16_clear( sizeof(int32_t)*cfg->rf_config.ul_carrier_bandwidth.value*12*fp->symbols_per_slot );
      pusch_vars[UE_id]->ul_ch_mag[i]  = (int32_t*)malloc16_clear( fp->symbols_per_slot*sizeof(int32_t)*cfg->rf_config.ul_carrier_bandwidth.value*12 );
      pusch_vars[UE_id]->ul_ch_magb[i] = (int32_t*)malloc16_clear( fp->symbols_per_slot*sizeof(int32_t)*cfg->rf_config.ul_carrier_bandwidth.value*12 );
      }
    
    pusch_vars[UE_id]->llr = (int16_t*)malloc16_clear( (8*((3*8*6144)+12))*sizeof(int16_t) );
  } //UE_id

    
  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++)
    gNB->UE_stats_ptr[UE_id] = &gNB->UE_stats[UE_id];
  
  gNB->pdsch_config_dedicated->p_a = dB0; //defaul value until overwritten by RRCConnectionReconfiguration
  

  return (0);

}
/*
void phy_config_request(PHY_Config_t *phy_config) {

  uint8_t Mod_id              = phy_config->Mod_id;
  int CC_id                   = phy_config->CC_id;
  nfapi_nr_config_request_t *cfg = phy_config->cfg;

  NR_DL_FRAME_PARMS *fp;
  PHICH_RESOURCE_t phich_resource_table[4]={oneSixth,half,one,two};
  int                 eutra_band     = cfg->nfapi_config.rf_bands.rf_band[0];  
  int                 dl_Bandwidth   = cfg->rf_config.dl_carrier_bandwidth.value;
  int                 ul_Bandwidth   = cfg->rf_config.ul_carrier_bandwidth.value;
  int                 Nid_cell       = cfg->sch_config.physical_cell_id.value;
  int                 Ncp            = cfg->subframe_config.dl_cyclic_prefix_type.value;
  int                 p_eNB          = cfg->rf_config.tx_antenna_ports.value;
  uint32_t            dl_CarrierFreq = cfg->nfapi_config.earfcn.value;

  LOG_I(PHY,"Configuring MIB for instance %d, CCid %d : (band %d,N_RB_DL %d, N_RB_UL %d, Nid_cell %d,gNB_tx_antenna_ports %d,Ncp %d,DL freq %u)\n",
	Mod_id, CC_id, eutra_band, dl_Bandwidth, ul_Bandwidth, Nid_cell, p_eNB,Ncp,dl_CarrierFreq	);

  AssertFatal(RC.gNB != NULL, "PHY instance pointer doesn't exist\n");
  AssertFatal(RC.gNB[Mod_id] != NULL, "PHY instance %d doesn't exist\n",Mod_id);
  AssertFatal(RC.gNB[Mod_id][CC_id] != NULL, "PHY instance %d, CCid %d doesn't exist\n",Mod_id,CC_id);


  if (RC.gNB[Mod_id][CC_id]->configured == 1)
  {
    LOG_E(PHY,"Already eNB already configured, do nothing\n");
    return;
  }

  RC.gNB[Mod_id][CC_id]->mac_enabled     = 1;

  fp = &RC.gNB[Mod_id][CC_id]->frame_parms;

  fp->threequarter_fs                    = 0;

  nr_init_frame_parms(fp,1);

  RC.gNB[Mod_id][CC_id]->configured                                   = 1;
  LOG_I(PHY,"gNB %d/%d configured\n",Mod_id,CC_id);
}*/


void phy_free_nr_gNB(PHY_VARS_gNB *gNB)
{
//  NR_DL_FRAME_PARMS* const fp       = &gNB->frame_parms;
  nfapi_nr_config_request_t *cfg       = &gNB->gNB_config;
  NR_gNB_COMMON* const common_vars  = &gNB->common_vars;
  LTE_eNB_PUSCH** const pusch_vars   = gNB->pusch_vars;
  LTE_eNB_SRS* const srs_vars        = gNB->srs_vars;
  LTE_eNB_PRACH* const prach_vars    = &gNB->prach_vars;
  uint32_t ***pdcch_dmrs             = gNB->nr_gold_pdcch_dmrs;

  int i, UE_id;

  for (i = 0; i < NB_ANTENNA_PORTS_ENB; i++) {
    if (i < cfg->rf_config.tx_antenna_ports.value || i == 5) {
      free_and_zero(common_vars->txdataF[i]);
      /* rxdataF[i] is not allocated -> don't free */
    }
  }
  free_and_zero(common_vars->txdataF);
  free_and_zero(common_vars->rxdataF);

  // PDCCH DMRS sequences
  free_and_zero(pdcch_dmrs);

  // Channel estimates for SRS
  for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
    for (i=0; i<64; i++) {
      free_and_zero(srs_vars[UE_id].srs_ch_estimates[i]);
      free_and_zero(srs_vars[UE_id].srs_ch_estimates_time[i]);
    }
    free_and_zero(srs_vars[UE_id].srs_ch_estimates);
    free_and_zero(srs_vars[UE_id].srs_ch_estimates_time);
  } //UE_id

  free_ul_ref_sigs();

  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) free_and_zero(srs_vars[UE_id].srs);

  free_and_zero(prach_vars->prachF);

  for (i = 0; i < 64; i++) free_and_zero(prach_vars->prach_ifft[0][i]);
  free_and_zero(prach_vars->prach_ifft[0]);

  free_and_zero(prach_vars->rxsigF[0]);

  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    for (i = 0; i < 2; i++) {
      free_and_zero(pusch_vars[UE_id]->rxdataF_ext[i]);
      free_and_zero(pusch_vars[UE_id]->rxdataF_ext2[i]);
      free_and_zero(pusch_vars[UE_id]->drs_ch_estimates[i]);
      free_and_zero(pusch_vars[UE_id]->drs_ch_estimates_time[i]);
      free_and_zero(pusch_vars[UE_id]->rxdataF_comp[i]);
      free_and_zero(pusch_vars[UE_id]->ul_ch_mag[i]);
      free_and_zero(pusch_vars[UE_id]->ul_ch_magb[i]);
    }
    free_and_zero(pusch_vars[UE_id]->rxdataF_ext);
    free_and_zero(pusch_vars[UE_id]->rxdataF_ext2);
    free_and_zero(pusch_vars[UE_id]->drs_ch_estimates);
    free_and_zero(pusch_vars[UE_id]->drs_ch_estimates_time);
    free_and_zero(pusch_vars[UE_id]->rxdataF_comp);
    free_and_zero(pusch_vars[UE_id]->ul_ch_mag);
    free_and_zero(pusch_vars[UE_id]->ul_ch_magb);
    free_and_zero(pusch_vars[UE_id]->llr);
    free_and_zero(pusch_vars[UE_id]);
  } //UE_id

  for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) gNB->UE_stats_ptr[UE_id] = NULL;
}
/*
void install_schedule_handlers(IF_Module_t *if_inst)
{
  if_inst->PHY_config_req = phy_config_request;
  if_inst->schedule_response = schedule_response;
}*/

/// this function is a temporary addition for NR configuration

/*void nr_phy_config_request(PHY_VARS_gNB *gNB)
{
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  nfapi_nr_config_request_t *gNB_config = &gNB->gNB_config;

  //overwrite for new NR parameters
  gNB_config->nfapi_config.rf_bands.rf_band[0] = 22;
  gNB_config->nfapi_config.earfcn.value = 6600;
  gNB_config->subframe_config.numerology_index_mu.value = 1;
  gNB_config->subframe_config.duplex_mode.value = FDD;
  gNB_config->rf_config.tx_antenna_ports.value = 1;
  gNB_config->rf_config.dl_carrier_bandwidth.value = 106;
  gNB_config->rf_config.ul_carrier_bandwidth.value = 106;
  gNB_config->sch_config.half_frame_index.value = 0;
  gNB_config->sch_config.ssb_subcarrier_offset.value = 0;
  gNB_config->sch_config.n_ssb_crb.value = 86;
  gNB_config->sch_config.ssb_subcarrier_offset.value = 0;


  gNB->mac_enabled     = 1;

  fp->dl_CarrierFreq = from_earfcn(gNB_config->nfapi_config.rf_bands.rf_band[0],gNB_config->nfapi_config.earfcn.value);
  fp->ul_CarrierFreq = fp->dl_CarrierFreq - (get_uldl_offset(gNB_config->nfapi_config.rf_bands.rf_band[0])*100000);
  fp->threequarter_fs                    = 0;

  nr_init_frame_parms(gNB_config, fp);

  gNB->configured                                   = 1;
  LOG_I(PHY,"gNB configured\n");
}*/


void nr_phy_config_request(NR_PHY_Config_t *phy_config)
{

  uint8_t Mod_id                  = phy_config->Mod_id;
  int     CC_id                   = phy_config->CC_id;

  NR_DL_FRAME_PARMS         *fp         = &RC.gNB[Mod_id][CC_id]->frame_parms;
  nfapi_nr_config_request_t *gNB_config = &RC.gNB[Mod_id][CC_id]->gNB_config;


  gNB_config->nfapi_config.rf_bands.rf_band[0]          = phy_config->cfg->nfapi_config.rf_bands.rf_band[0]; //22
  gNB_config->nfapi_config.earfcn.value                 = phy_config->cfg->nfapi_config.earfcn.value; //6600
  gNB_config->subframe_config.numerology_index_mu.value = phy_config->cfg->subframe_config.numerology_index_mu.value;//1
  gNB_config->rf_config.tx_antenna_ports.value          = phy_config->cfg->rf_config.tx_antenna_ports.value; //1
  gNB_config->rf_config.dl_carrier_bandwidth.value      = phy_config->cfg->rf_config.dl_carrier_bandwidth.value;//106;
  gNB_config->rf_config.ul_carrier_bandwidth.value      = phy_config->cfg->rf_config.ul_carrier_bandwidth.value;//106;
  gNB_config->sch_config.half_frame_index.value         = 0;
  gNB_config->sch_config.ssb_subcarrier_offset.value    = phy_config->cfg->sch_config.ssb_subcarrier_offset.value;//0;
  gNB_config->sch_config.n_ssb_crb.value                = 86;
  gNB_config->sch_config.physical_cell_id.value         = phy_config->cfg->sch_config.physical_cell_id.value;

  if (phy_config->cfg->subframe_config.duplex_mode.value == 0) {
    gNB_config->subframe_config.duplex_mode.value    = TDD;
  }
  else {
    gNB_config->subframe_config.duplex_mode.value    = FDD;
  }

  RC.gNB[Mod_id][CC_id]->mac_enabled     = 1;

  fp->dl_CarrierFreq = from_earfcn(gNB_config->nfapi_config.rf_bands.rf_band[0],gNB_config->nfapi_config.earfcn.value);
  fp->ul_CarrierFreq = fp->dl_CarrierFreq - (get_uldl_offset(gNB_config->nfapi_config.rf_bands.rf_band[0])*100000);
  fp->threequarter_fs                    = 0;

  LOG_I(PHY,"Configuring MIB for instance %d, CCid %d : (band %d,N_RB_DL %d, N_RB_UL %d, Nid_cell %d,gNB_tx_antenna_ports %d,DL freq %u)\n",
  Mod_id, 
  CC_id, 
  gNB_config->nfapi_config.rf_bands.rf_band[0], 
  gNB_config->rf_config.dl_carrier_bandwidth.value, 
  gNB_config->rf_config.ul_carrier_bandwidth.value, 
  gNB_config->sch_config.physical_cell_id.value, 
  gNB_config->rf_config.tx_antenna_ports.value,
  fp->dl_CarrierFreq );

  nr_init_frame_parms(gNB_config, fp);

  if (RC.gNB[Mod_id][CC_id]->configured == 1){
    LOG_E(PHY,"Already gNB already configured, do nothing\n");
    return;
  }

  RC.gNB[Mod_id][CC_id]->configured     = 1;
  LOG_I(PHY,"gNB %d/%d configured\n",Mod_id,CC_id);



}
