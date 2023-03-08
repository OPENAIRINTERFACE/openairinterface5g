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

#include "executables/nr-softmodem-common.h"
#include "common/utils/nr/nr_common.h"
#include "common/ran_context.h"
#include "PHY/defs_gNB.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "PHY/INIT/nr_phy_init.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"
#include "openair1/PHY/MODULATION/nr_modulation.h"
#include "openair1/PHY/defs_RU.h"
#include "openair1/PHY/CODING/nrLDPC_extern.h"
#include "assertions.h"
#include <math.h>
#include <complex.h>
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "SCHED_NR/fapi_nr_l1.h"
#include "nfapi_nr_interface.h"

#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"


int l1_north_init_gNB() {

  if (RC.nb_nr_L1_inst > 0 &&  RC.gNB != NULL) {

    AssertFatal(RC.nb_nr_L1_inst>0,"nb_nr_L1_inst=%d\n",RC.nb_nr_L1_inst);
    AssertFatal(RC.gNB!=NULL,"RC.gNB is null\n");
    LOG_I(PHY,"%s() RC.nb_nr_L1_inst:%d\n", __FUNCTION__, RC.nb_nr_L1_inst);

    for (int i=0; i<RC.nb_nr_L1_inst; i++) {
      AssertFatal(RC.gNB[i]!=NULL,"RC.gNB[%d] is null\n",i);

      if ((RC.gNB[i]->if_inst =  NR_IF_Module_init(i))<0) return(-1);
      
      LOG_I(PHY,"%s() RC.gNB[%d] installing callbacks\n", __FUNCTION__, i);
      RC.gNB[i]->if_inst->NR_PHY_config_req = nr_phy_config_request;
      RC.gNB[i]->if_inst->NR_Schedule_response = nr_schedule_response;
    }
  } else {
    LOG_I(PHY,"%s() Not installing PHY callbacks - RC.nb_nr_L1_inst:%d RC.gNB:%p\n", __FUNCTION__, RC.nb_nr_L1_inst, RC.gNB);
  }

  return(0);
}

NR_gNB_PHY_STATS_t *get_phy_stats(PHY_VARS_gNB *gNB, uint16_t rnti)
{
  NR_gNB_PHY_STATS_t *stats;
  int first_free = -1;
  for (int i = 0; i < MAX_MOBILES_PER_GNB; i++) {
    stats = &gNB->phy_stats[i];
    if (stats->active && stats->rnti == rnti)
      return stats;
    else if (!stats->active && first_free == -1)
      first_free = i;
  }
  // new stats
  AssertFatal(first_free >= 0, "PHY statistics list is full\n");
  stats = &gNB->phy_stats[first_free];
  stats->active = true;
  stats->rnti = rnti;
  memset(&stats->dlsch_stats, 0, sizeof(stats->dlsch_stats));
  memset(&stats->ulsch_stats, 0, sizeof(stats->ulsch_stats));
  memset(&stats->uci_stats, 0, sizeof(stats->uci_stats));
  return stats;
}

void reset_active_stats(PHY_VARS_gNB *gNB, int frame)
{
  // disactivate PHY stats if UE is inactive for a given number of frames
  for (int i = 0; i < MAX_MOBILES_PER_GNB; i++) {
    NR_gNB_PHY_STATS_t *stats = &gNB->phy_stats[i];
    if (stats->active && (((frame - stats->frame + 1024) % 1024) > NUMBER_FRAMES_PHY_UE_INACTIVE))
      stats->active = false;
  }
}

// A global var to reduce the changes size
ldpc_interface_t ldpc_interface = {0}, ldpc_interface_offload = {0};

int phy_init_nr_gNB(PHY_VARS_gNB *gNB)
{
  // shortcuts
  NR_DL_FRAME_PARMS *const fp       = &gNB->frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  NR_gNB_COMMON *const common_vars  = &gNB->common_vars;
  NR_gNB_PRACH *const prach_vars   = &gNB->prach_vars;

  int i;
  int Ptx=cfg->carrier_config.num_tx_ant.value;
  int Prx=cfg->carrier_config.num_rx_ant.value;
  int max_ul_mimo_layers = 4;

  AssertFatal(Ptx>0 && Ptx<9,"Ptx %d is not supported\n",Ptx);
  AssertFatal(Prx>0 && Prx<9,"Prx %d is not supported\n",Prx);
  LOG_I(PHY,"[gNB %d] %s() About to wait for gNB to be configured\n", gNB->Mod_id, __FUNCTION__);

  while(gNB->configured == 0) usleep(10000);

  load_dftslib();

  crcTableInit();
  init_scrambling_luts();
  init_pucch2_luts();

  nr_init_fde(); // Init array for frequency equalization of transform precoding of PUSCH

  load_LDPClib(NULL, &ldpc_interface);

  if (gNB->ldpc_offload_flag)
    load_LDPClib("_t2", &ldpc_interface_offload);
  gNB->max_nb_pdsch = MAX_MOBILES_PER_GNB;
  init_delay_table(fp->ofdm_symbol_size, MAX_DELAY_COMP, NR_MAX_OFDM_SYMBOL_SIZE, fp->delay_table);

  // PBCH DMRS gold sequences generation
  nr_init_pbch_dmrs(gNB);
  //PDCCH DMRS init
  gNB->nr_gold_pdcch_dmrs = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t **));
  uint32_t ***pdcch_dmrs             = gNB->nr_gold_pdcch_dmrs;
  AssertFatal(pdcch_dmrs!=NULL, "NR init: pdcch_dmrs malloc failed\n");

  gNB->bad_pucch = 0;
  if (gNB->TX_AMP == 0)
    gNB->TX_AMP = AMP;
  // ceil(((NB_RB<<1)*3)/32) // 3 RE *2(QPSK)
  int pdcch_dmrs_init_length =  (((fp->N_RB_DL<<1)*3)>>5)+1;

  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    pdcch_dmrs[slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t *));
    AssertFatal(pdcch_dmrs[slot]!=NULL, "NR init: pdcch_dmrs for slot %d - malloc failed\n", slot);

    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      pdcch_dmrs[slot][symb] = (uint32_t *)malloc16(pdcch_dmrs_init_length*sizeof(uint32_t));
      LOG_D(PHY,"pdcch_dmrs[%d][%d] %p\n",slot,symb,pdcch_dmrs[slot][symb]);
      AssertFatal(pdcch_dmrs[slot][symb]!=NULL, "NR init: pdcch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);
    }
  }

  nr_generate_modulation_table();
  gNB->pdcch_gold_init = cfg->cell_config.phy_cell_id.value;
  nr_init_pdcch_dmrs(gNB, cfg->cell_config.phy_cell_id.value);
  nr_init_pbch_interleaver(gNB->nr_pbch_interleaver);

  //PDSCH DMRS init
  gNB->nr_gold_pdsch_dmrs = (uint32_t ****)malloc16(fp->slots_per_frame*sizeof(uint32_t ***));
  uint32_t ****pdsch_dmrs             = gNB->nr_gold_pdsch_dmrs;

  // ceil(((NB_RB*12(k)*2(QPSK)/32) // 3 RE *2(QPSK)
  const int pdsch_dmrs_init_length =  ((fp->N_RB_DL*24)>>5)+1;
  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    pdsch_dmrs[slot] = (uint32_t ***)malloc16(fp->symbols_per_slot*sizeof(uint32_t **));
    AssertFatal(pdsch_dmrs[slot]!=NULL, "NR init: pdsch_dmrs for slot %d - malloc failed\n", slot);

    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      pdsch_dmrs[slot][symb] = (uint32_t **)malloc16(NR_NB_NSCID*sizeof(uint32_t *));
      AssertFatal(pdsch_dmrs[slot][symb]!=NULL, "NR init: pdsch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);

      for (int q=0; q<NR_NB_NSCID; q++) {
        pdsch_dmrs[slot][symb][q] = (uint32_t *)malloc16(pdsch_dmrs_init_length*sizeof(uint32_t));
        AssertFatal(pdsch_dmrs[slot][symb][q]!=NULL, "NR init: pdsch_dmrs for slot %d symbol %d nscid %d - malloc failed\n", slot, symb, q);
      }
    }
  }


  for (int nscid = 0; nscid < NR_NB_NSCID; nscid++) {
    gNB->pdsch_gold_init[nscid] = cfg->cell_config.phy_cell_id.value;
    nr_init_pdsch_dmrs(gNB, nscid, cfg->cell_config.phy_cell_id.value);
  }

  //PUSCH DMRS init
  gNB->nr_gold_pusch_dmrs = (uint32_t ****)malloc16(NR_NB_NSCID*sizeof(uint32_t ***));

  uint32_t ****pusch_dmrs = gNB->nr_gold_pusch_dmrs;

  int pusch_dmrs_init_length =  ((fp->N_RB_UL*12)>>5)+1;
  for(int nscid=0; nscid<NR_NB_NSCID; nscid++) {
    pusch_dmrs[nscid] = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t **));
    AssertFatal(pusch_dmrs[nscid]!=NULL, "NR init: pusch_dmrs for nscid %d - malloc failed\n", nscid);

    for (int slot=0; slot<fp->slots_per_frame; slot++) {
      pusch_dmrs[nscid][slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t *));
      AssertFatal(pusch_dmrs[nscid][slot]!=NULL, "NR init: pusch_dmrs for slot %d - malloc failed\n", slot);

      for (int symb=0; symb<fp->symbols_per_slot; symb++) {
        pusch_dmrs[nscid][slot][symb] = (uint32_t *)malloc16(pusch_dmrs_init_length*sizeof(uint32_t));
        AssertFatal(pusch_dmrs[nscid][slot][symb]!=NULL, "NR init: pusch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);
      }
    }
  }

  for (int nscid=0; nscid<NR_NB_NSCID; nscid++) {
    gNB->pusch_gold_init[nscid] = cfg->cell_config.phy_cell_id.value;
    nr_gold_pusch(gNB, nscid, gNB->pusch_gold_init[nscid]);
  }

  // CSI RS init
  // ceil((NB_RB*8(max allocation per RB)*2(QPSK))/32)
  int csi_dmrs_init_length =  ((fp->N_RB_DL<<4)>>5)+1;
  gNB->nr_csi_info = (nr_csi_info_t *)malloc16_clear(sizeof(nr_csi_info_t));
  gNB->nr_csi_info->nr_gold_csi_rs = (uint32_t ***)malloc16(fp->slots_per_frame * sizeof(uint32_t **));
  AssertFatal(gNB->nr_csi_info->nr_gold_csi_rs != NULL, "NR init: csi reference signal malloc failed\n");
  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    gNB->nr_csi_info->nr_gold_csi_rs[slot] = (uint32_t **)malloc16(fp->symbols_per_slot * sizeof(uint32_t *));
    AssertFatal(gNB->nr_csi_info->nr_gold_csi_rs[slot] != NULL, "NR init: csi reference signal for slot %d - malloc failed\n", slot);
    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      gNB->nr_csi_info->nr_gold_csi_rs[slot][symb] = (uint32_t *)malloc16(csi_dmrs_init_length * sizeof(uint32_t));
      AssertFatal(gNB->nr_csi_info->nr_gold_csi_rs[slot][symb] != NULL, "NR init: csi reference signal for slot %d symbol %d - malloc failed\n", slot, symb);
    }
  }

  gNB->nr_csi_info->csi_gold_init = cfg->cell_config.phy_cell_id.value;
  nr_init_csi_rs(&gNB->frame_parms, gNB->nr_csi_info->nr_gold_csi_rs, cfg->cell_config.phy_cell_id.value);

  //PRS init
  nr_init_prs(gNB);

  generate_ul_reference_signal_sequences(SHRT_MAX);

  /* Generate low PAPR type 1 sequences for PUSCH DMRS, these are used if transform precoding is enabled.  */
  generate_lowpapr_typ1_refsig_sequences(SHRT_MAX);

  /// Transport init necessary for NR synchro
  init_nr_transport(gNB);

  gNB->nr_srs_info = (nr_srs_info_t **)malloc16_clear(gNB->max_nb_srs * sizeof(nr_srs_info_t*));
  for (int id = 0; id < gNB->max_nb_srs; id++) {
    gNB->nr_srs_info[id] = (nr_srs_info_t *)malloc16_clear(sizeof(nr_srs_info_t));
    gNB->nr_srs_info[id]->srs_generated_signal = (int32_t**)malloc16_clear(MAX_NUM_NR_SRS_AP*sizeof(int32_t*));
    for(int ap=0; ap<MAX_NUM_NR_SRS_AP; ap++) {
      gNB->nr_srs_info[id]->srs_generated_signal[ap] = (int32_t*)malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
    }
  }

  common_vars->txdataF = (c16_t **)malloc16(Ptx*sizeof(c16_t*));
  common_vars->rxdataF = (c16_t **)malloc16(Prx*sizeof(c16_t*));
  /* Do NOT allocate per-antenna txdataF/rxdataF: the gNB gets a pointer to the
   * RU to copy/recover freq-domain memory from there */
  common_vars->beam_id = (uint8_t **)malloc16(Ptx*sizeof(uint8_t*));

  for (i=0;i<Ptx;i++){
    common_vars->txdataF[i] = (c16_t*)malloc16_clear(fp->samples_per_frame_wCP*sizeof(c16_t)); // [hna] samples_per_frame without CP
    LOG_D(PHY,"[INIT] common_vars->txdataF[%d] = %p (%lu bytes)\n",
          i,common_vars->txdataF[i],
          fp->samples_per_frame_wCP*sizeof(c16_t));
    common_vars->beam_id[i] = (uint8_t*)malloc16_clear(fp->symbols_per_slot*fp->slots_per_frame*sizeof(uint8_t));
    memset(common_vars->beam_id[i],255,fp->symbols_per_slot*fp->slots_per_frame);
  }
  common_vars->debugBuff = (int32_t*)malloc16_clear(fp->samples_per_frame*sizeof(int32_t)*100);	
  common_vars->debugBuff_sample_offset = 0; 

  // PRACH
  prach_vars->prachF = (int16_t *)malloc16_clear( 1024*2*sizeof(int16_t) );
  prach_vars->rxsigF = (int16_t **)malloc16_clear(Prx*sizeof(int16_t*));
  prach_vars->prach_ifft       = (int32_t *)malloc16_clear(1024*2*sizeof(int32_t));

  init_prach_list(gNB);

  int N_RB_UL = cfg->carrier_config.ul_grid_size[cfg->ssb_config.scs_common.value].value;
  int n_buf = Prx*max_ul_mimo_layers;

  int nb_re_pusch = N_RB_UL * NR_NB_SC_PER_RB;
  int nb_re_pusch2 = (nb_re_pusch + 15) & ~15;

  gNB->pusch_vars = (NR_gNB_PUSCH *)malloc16_clear(gNB->max_nb_pusch * sizeof(NR_gNB_PUSCH));
  for (int ULSCH_id = 0; ULSCH_id < gNB->max_nb_pusch; ULSCH_id++) {
    NR_gNB_PUSCH *pusch = &gNB->pusch_vars[ULSCH_id];
    pusch->rxdataF_ext = (int32_t **)malloc16(Prx * sizeof(int32_t *));
    pusch->ul_ch_estimates = (int32_t **)malloc16(n_buf * sizeof(int32_t *));
    pusch->ul_ch_estimates_ext = (int32_t **)malloc16(n_buf * sizeof(int32_t *));
    pusch->ptrs_phase_per_slot = (int32_t **)malloc16(n_buf * sizeof(int32_t *));
    pusch->ul_ch_estimates_time = (int32_t **)malloc16(n_buf * sizeof(int32_t *));
    pusch->rxdataF_comp = (int32_t **)malloc16(n_buf * sizeof(int32_t *));
    pusch->llr_layers = (int16_t **)malloc16(max_ul_mimo_layers * sizeof(int32_t *));
    for (i = 0; i < Prx; i++) {
      pusch->rxdataF_ext[i] = (int32_t *)malloc16_clear(sizeof(int32_t) * nb_re_pusch2 * fp->symbols_per_slot);
    }
    for (i = 0; i < n_buf; i++) {
      pusch->ul_ch_estimates[i] = (int32_t *)malloc16_clear(sizeof(int32_t) * fp->ofdm_symbol_size * fp->symbols_per_slot);
      pusch->ul_ch_estimates_ext[i] = (int32_t *)malloc16_clear(sizeof(int32_t) * nb_re_pusch2 * fp->symbols_per_slot);
      pusch->ul_ch_estimates_time[i] = (int32_t *)malloc16_clear(sizeof(int32_t) * fp->ofdm_symbol_size);
      pusch->ptrs_phase_per_slot[i] = (int32_t *)malloc16_clear(sizeof(int32_t) * fp->symbols_per_slot); // symbols per slot
      pusch->rxdataF_comp[i] = (int32_t *)malloc16_clear(sizeof(int32_t) * nb_re_pusch2 * fp->symbols_per_slot);
    }

    for (i=0; i< max_ul_mimo_layers; i++) {
      pusch->llr_layers[i] = (int16_t *)malloc16_clear((8 * ((3 * 8 * 6144) + 12))
                                                       * sizeof(int16_t)); // [hna] 6144 is LTE and (8*((3*8*6144)+12)) is not clear
    }
    pusch->llr = (int16_t *)malloc16_clear((8 * ((3 * 8 * 6144) + 12))
                                           * sizeof(int16_t)); // [hna] 6144 is LTE and (8*((3*8*6144)+12)) is not clear
    pusch->ul_valid_re_per_slot = (int16_t *)malloc16_clear(sizeof(int16_t) * fp->symbols_per_slot);
  } // ulsch_id

  return (0);
}

void phy_free_nr_gNB(PHY_VARS_gNB *gNB)
{
  NR_DL_FRAME_PARMS* const fp = &gNB->frame_parms;
  const int Ptx = gNB->gNB_config.carrier_config.num_tx_ant.value;
  const int Prx = gNB->gNB_config.carrier_config.num_rx_ant.value;
  const int max_ul_mimo_layers = 4; // taken from phy_init_nr_gNB()
  const int n_buf = Prx * max_ul_mimo_layers;

  PHY_MEASUREMENTS_gNB *meas = &gNB->measurements;
  free_and_zero(meas->n0_subband_power);
  free_and_zero(meas->n0_subband_power_dB);

  uint32_t ***pdcch_dmrs = gNB->nr_gold_pdcch_dmrs;
  for (int slot = 0; slot < fp->slots_per_frame; slot++) {
    for (int symb = 0; symb < fp->symbols_per_slot; symb++)
      free_and_zero(pdcch_dmrs[slot][symb]);
    free_and_zero(pdcch_dmrs[slot]);
  }
  free_and_zero(pdcch_dmrs);

  uint32_t ****pdsch_dmrs = gNB->nr_gold_pdsch_dmrs;
  for (int slot = 0; slot < fp->slots_per_frame; slot++) {
    for (int symb = 0; symb < fp->symbols_per_slot; symb++) {
      for (int q = 0; q < NR_NB_NSCID; q++)
        free_and_zero(pdsch_dmrs[slot][symb][q]);
      free_and_zero(pdsch_dmrs[slot][symb]);
    }
    free_and_zero(pdsch_dmrs[slot]);
  }
  free_and_zero(gNB->nr_gold_pdsch_dmrs);

  uint32_t ****pusch_dmrs = gNB->nr_gold_pusch_dmrs;
  for(int nscid = 0; nscid < 2; nscid++) {
    for (int slot = 0; slot < fp->slots_per_frame; slot++) {
      for (int symb = 0; symb < fp->symbols_per_slot; symb++)
        free_and_zero(pusch_dmrs[nscid][slot][symb]);
      free_and_zero(pusch_dmrs[nscid][slot]);
    }
    free_and_zero(pusch_dmrs[nscid]);
  }
  free_and_zero(pusch_dmrs);

  uint32_t ***nr_gold_csi_rs = gNB->nr_csi_info->nr_gold_csi_rs;
  for (int slot = 0; slot < fp->slots_per_frame; slot++) {
    for (int symb = 0; symb < fp->symbols_per_slot; symb++)
      free_and_zero(nr_gold_csi_rs[slot][symb]);
    free_and_zero(nr_gold_csi_rs[slot]);
  }
  free_and_zero(nr_gold_csi_rs);
  free_and_zero(gNB->nr_csi_info);

  for (int id = 0; id < gNB->max_nb_srs; id++) {
    for(int i=0; i<MAX_NUM_NR_SRS_AP; i++) {
      free_and_zero(gNB->nr_srs_info[id]->srs_generated_signal[i]);
    }
    free_and_zero(gNB->nr_srs_info[id]->srs_generated_signal);
    free_and_zero(gNB->nr_srs_info[id]);
  }
  free_and_zero(gNB->nr_srs_info);

  free_ul_reference_signal_sequences();
  free_gnb_lowpapr_sequences();

  reset_nr_transport(gNB);

  NR_gNB_COMMON * common_vars = &gNB->common_vars;
  for (int i = 0; i < Ptx; i++) {
    free_and_zero(common_vars->txdataF[i]);
    free_and_zero(common_vars->beam_id[i]);
  }

  for (int rsc=0; rsc < gNB->prs_vars.NumPRSResources; rsc++) {
    for (int slot=0; slot<fp->slots_per_frame; slot++) {
      for (int symb=0; symb<fp->symbols_per_slot; symb++) {
        free_and_zero(gNB->nr_gold_prs[rsc][slot][symb]);
      }
      free_and_zero(gNB->nr_gold_prs[rsc][slot]);
    }
    free_and_zero(gNB->nr_gold_prs[rsc]);
  }
  free_and_zero(gNB->nr_gold_prs);

  /* Do NOT free per-antenna txdataF/rxdataF: the gNB gets a pointer to the
   * RU's txdataF/rxdataF, and the RU will free that */
  free_and_zero(common_vars->txdataF);
  free_and_zero(common_vars->rxdataF);
  free_and_zero(common_vars->beam_id);

  free_and_zero(common_vars->debugBuff);

  NR_gNB_PRACH* prach_vars = &gNB->prach_vars;
  free_and_zero(prach_vars->prachF);
  free_and_zero(prach_vars->rxsigF);
  free_and_zero(prach_vars->prach_ifft);

  for (int ULSCH_id = 0; ULSCH_id < gNB->max_nb_pusch; ULSCH_id++) {
    NR_gNB_PUSCH *pusch_vars = &gNB->pusch_vars[ULSCH_id];
    for (int i=0; i< max_ul_mimo_layers; i++)
      free_and_zero(pusch_vars->llr_layers[i]);
    for (int i = 0; i < Prx; i++) {
      free_and_zero(pusch_vars->rxdataF_ext[i]);
    }
    for (int i = 0; i < n_buf; i++) {
      free_and_zero(pusch_vars->ul_ch_estimates[i]);
      free_and_zero(pusch_vars->ul_ch_estimates_ext[i]);
      free_and_zero(pusch_vars->ul_ch_estimates_time[i]);
      free_and_zero(pusch_vars->ptrs_phase_per_slot[i]);
      free_and_zero(pusch_vars->rxdataF_comp[i]);
    }
    free_and_zero(pusch_vars->llr_layers);
    free_and_zero(pusch_vars->rxdataF_ext);
    free_and_zero(pusch_vars->ul_ch_estimates);
    free_and_zero(pusch_vars->ul_ch_estimates_ext);
    free_and_zero(pusch_vars->ptrs_phase_per_slot);
    free_and_zero(pusch_vars->ul_ch_estimates_time);
    free_and_zero(pusch_vars->ul_valid_re_per_slot);
    free_and_zero(pusch_vars->rxdataF_comp);

    free_and_zero(pusch_vars->llr);
  } // ULSCH_id
  free(gNB->pusch_vars);
}

//Adding nr_schedule_handler
void install_nr_schedule_handlers(NR_IF_Module_t *if_inst)
{
  if_inst->NR_PHY_config_req = nr_phy_config_request;
  if_inst->NR_Schedule_response = nr_schedule_response;
}
/*
void install_schedule_handlers(IF_Module_t *if_inst)
{
  if_inst->PHY_config_req = phy_config_request;
  if_inst->schedule_response = schedule_response;
}*/

/// this function is a temporary addition for NR configuration


void nr_phy_config_request_sim(PHY_VARS_gNB *gNB,
                               int N_RB_DL,
                               int N_RB_UL,
                               int mu,
                               int Nid_cell,
                               uint64_t position_in_burst)
{
  NR_DL_FRAME_PARMS *fp                                   = &gNB->frame_parms;
  nfapi_nr_config_request_scf_t *gNB_config               = &gNB->gNB_config;
  //overwrite for new NR parameters

  uint64_t rev_burst=0;
  for (int i=0; i<64; i++)
    rev_burst |= (((position_in_burst>>(63-i))&0x01)<<i);

  gNB_config->cell_config.phy_cell_id.value             = Nid_cell;
  gNB_config->ssb_config.scs_common.value               = mu;
  gNB_config->ssb_table.ssb_subcarrier_offset.value     = 0;
  gNB_config->ssb_table.ssb_offset_point_a.value        = (N_RB_DL-20)>>1;
  gNB_config->ssb_table.ssb_mask_list[1].ssb_mask.value = (rev_burst)&(0xFFFFFFFF);
  gNB_config->ssb_table.ssb_mask_list[0].ssb_mask.value = (rev_burst>>32)&(0xFFFFFFFF);
  gNB_config->cell_config.frame_duplex_type.value       = TDD;
  gNB_config->ssb_table.ssb_period.value		= 1; //10ms
  gNB_config->carrier_config.dl_grid_size[mu].value     = N_RB_DL;
  gNB_config->carrier_config.ul_grid_size[mu].value     = N_RB_UL;
  gNB_config->carrier_config.num_tx_ant.value           = fp->nb_antennas_tx;
  gNB_config->carrier_config.num_rx_ant.value           = fp->nb_antennas_rx;

  gNB_config->tdd_table.tdd_period.value = 0;
  //gNB_config->subframe_config.dl_cyclic_prefix_type.value = (fp->Ncp == NORMAL) ? NFAPI_CP_NORMAL : NFAPI_CP_EXTENDED;

  if (mu==0) {
    fp->dl_CarrierFreq = 2600000000;//from_nrarfcn(gNB_config->nfapi_config.rf_bands.rf_band[0],gNB_config->nfapi_config.nrarfcn.value);
    fp->ul_CarrierFreq = 2600000000;//fp->dl_CarrierFreq - (get_uldl_offset(gNB_config->nfapi_config.rf_bands.rf_band[0])*100000);
    fp->nr_band = 38;
    //  fp->threequarter_fs= 0;
  } else if (mu==1) {
    fp->dl_CarrierFreq = 3600000000;//from_nrarfcn(gNB_config->nfapi_config.rf_bands.rf_band[0],gNB_config->nfapi_config.nrarfcn.value);
    fp->ul_CarrierFreq = 3600000000;//fp->dl_CarrierFreq - (get_uldl_offset(gNB_config->nfapi_config.rf_bands.rf_band[0])*100000);
    fp->nr_band = 78;
    //  fp->threequarter_fs= 0;
  } else if (mu==3) {
    fp->dl_CarrierFreq = 27524520000;//from_nrarfcn(gNB_config->nfapi_config.rf_bands.rf_band[0],gNB_config->nfapi_config.nrarfcn.value);
    fp->ul_CarrierFreq = 27524520000;//fp->dl_CarrierFreq - (get_uldl_offset(gNB_config->nfapi_config.rf_bands.rf_band[0])*100000);
    fp->nr_band = 261;
    //  fp->threequarter_fs= 0;
  }

  fp->threequarter_fs = 0;
  int bw_index = get_supported_band_index(mu, fp->nr_band, N_RB_DL);
  gNB_config->carrier_config.dl_bandwidth.value = get_supported_bw_mhz(fp->nr_band > 256 ? FR2 : FR1, bw_index);

  nr_init_frame_parms(gNB_config, fp);

  fp->ofdm_offset_divisor = UINT_MAX;
  init_symbol_rotation(fp);
  init_timeshift_rotation(fp);

  gNB->configured    = 1;
  LOG_I(PHY,"gNB configured\n");
}

void nr_phy_config_request(NR_PHY_Config_t *phy_config)
{
  uint8_t Mod_id = phy_config->Mod_id;
  uint8_t short_sequence, num_sequences, rootSequenceIndex, fd_occasion;
  NR_DL_FRAME_PARMS *fp = &RC.gNB[Mod_id]->frame_parms;
  nfapi_nr_config_request_scf_t *gNB_config = &RC.gNB[Mod_id]->gNB_config;

  memcpy((void*)gNB_config,phy_config->cfg,sizeof(*phy_config->cfg));

  uint64_t dl_bw_khz = (12*gNB_config->carrier_config.dl_grid_size[gNB_config->ssb_config.scs_common.value].value)*(15<<gNB_config->ssb_config.scs_common.value);
  fp->dl_CarrierFreq = ((dl_bw_khz>>1) + gNB_config->carrier_config.dl_frequency.value)*1000 ;
  
  uint64_t ul_bw_khz = (12*gNB_config->carrier_config.ul_grid_size[gNB_config->ssb_config.scs_common.value].value)*(15<<gNB_config->ssb_config.scs_common.value);
  fp->ul_CarrierFreq = ((ul_bw_khz>>1) + gNB_config->carrier_config.uplink_frequency.value)*1000 ;

  int32_t dlul_offset = fp->ul_CarrierFreq - fp->dl_CarrierFreq;
  fp->nr_band = get_band(fp->dl_CarrierFreq, dlul_offset);

  LOG_I(PHY, "DL frequency %lu Hz, UL frequency %lu Hz: band %d, uldl offset %d Hz\n", fp->dl_CarrierFreq, fp->ul_CarrierFreq, fp->nr_band, dlul_offset);

  fp->threequarter_fs = openair0_cfg[0].threequarter_fs;
  LOG_A(PHY,"Configuring MIB for instance %d, : (Nid_cell %d,DL freq %llu, UL freq %llu)\n",
        Mod_id,
        gNB_config->cell_config.phy_cell_id.value,
        (unsigned long long)fp->dl_CarrierFreq,
        (unsigned long long)fp->ul_CarrierFreq);

  nr_init_frame_parms(gNB_config, fp);
  

  if (RC.gNB[Mod_id]->configured == 1) {
    LOG_E(PHY,"Already gNB already configured, do nothing\n");
    return;
  }

  fd_occasion = 0;
  nfapi_nr_prach_config_t *prach_config = &gNB_config->prach_config;
  short_sequence = prach_config->prach_sequence_length.value;
//  for(fd_occasion = 0; fd_occasion <= prach_config->num_prach_fd_occasions.value ; fd_occasion) { // TODO Need to handle for msg1-fdm > 1
  num_sequences = prach_config->num_prach_fd_occasions_list[fd_occasion].num_root_sequences.value;
  rootSequenceIndex = prach_config->num_prach_fd_occasions_list[fd_occasion].prach_root_sequence_index.value;

  compute_nr_prach_seq(short_sequence, num_sequences, rootSequenceIndex, RC.gNB[Mod_id]->X_u);
//  }
  RC.gNB[Mod_id]->configured     = 1;

  fp->ofdm_offset_divisor = RC.gNB[Mod_id]->ofdm_offset_divisor;
  init_symbol_rotation(fp);
  init_timeshift_rotation(fp);

  LOG_I(PHY,"gNB %d configured\n",Mod_id);
}

void init_DLSCH_struct(PHY_VARS_gNB *gNB, processingData_L1tx_t *msg)
{
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  uint16_t grid_size = cfg->carrier_config.dl_grid_size[fp->numerology_index].value;
  msg->num_pdsch_slot = 0;

  msg->dlsch = malloc16(gNB->max_nb_pdsch * sizeof(NR_gNB_DLSCH_t *));
  int num_cw = NR_MAX_NB_LAYERS > 4? 2:1;
  for (int i = 0; i < gNB->max_nb_pdsch; i++) {
    LOG_D(PHY, "Allocating Transport Channel Buffers for DLSCH %d/%d\n", i, gNB->max_nb_pdsch);
    msg->dlsch[i] = (NR_gNB_DLSCH_t *)malloc16(num_cw * sizeof(NR_gNB_DLSCH_t));
    for (int j = 0; j < num_cw; j++) {
      msg->dlsch[i][j] = new_gNB_dlsch(fp, grid_size);
    }
  }
}

void reset_DLSCH_struct(const PHY_VARS_gNB *gNB, processingData_L1tx_t *msg)
{
  const NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  const nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  const uint16_t grid_size = cfg->carrier_config.dl_grid_size[fp->numerology_index].value;
  int num_cw = NR_MAX_NB_LAYERS > 4? 2:1;
  for (int i = 0; i < gNB->max_nb_pdsch; i++) {
    for (int j = 0; j < num_cw; j++) {
      free_gNB_dlsch(&msg->dlsch[i][j], grid_size, fp);
    }
    free(msg->dlsch[i]);
  }
  free(msg->dlsch);
}

void init_nr_transport(PHY_VARS_gNB *gNB)
{

  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  const nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;
  LOG_I(PHY, "Initialise nr transport\n");

  int nb_slots_per_period = cfg->cell_config.frame_duplex_type.value ?
                            fp->slots_per_frame / get_nb_periods_per_frame(cfg->tdd_table.tdd_period.value) :
                            fp->slots_per_frame;
  int nb_ul_slots_period = 0;
  if (cfg->cell_config.frame_duplex_type.value) {
    for(int i=0; i<nb_slots_per_period; i++) {
      for(int j=0; j<NR_NUMBER_OF_SYMBOLS_PER_SLOT; j++) {
        if(cfg->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[j].slot_config.value == 1) { // UL symbol
          nb_ul_slots_period++;
          break;
        }  
      }
    }
  }
  else
    nb_ul_slots_period = fp->slots_per_frame;

  int buffer_ul_slots; // the UL channels are scheduled sl_ahead before they are transmitted
  int slot_ahead = gNB->if_inst ? gNB->if_inst->sl_ahead : 6;
  if (slot_ahead > nb_slots_per_period)
    buffer_ul_slots = nb_ul_slots_period + (slot_ahead - nb_slots_per_period);
  else
    buffer_ul_slots = (nb_ul_slots_period < slot_ahead) ? nb_ul_slots_period : slot_ahead;

  gNB->max_nb_pucch = buffer_ul_slots ? MAX_MOBILES_PER_GNB * buffer_ul_slots : 1;
  gNB->max_nb_pusch = buffer_ul_slots ? MAX_MOBILES_PER_GNB * buffer_ul_slots : 1;
  gNB->max_nb_srs = buffer_ul_slots << 1; // assuming at most 2 SRS per slot

  gNB->pucch = (NR_gNB_PUCCH_t *)malloc16(gNB->max_nb_pucch * sizeof(NR_gNB_PUCCH_t));
  for (int i = 0; i < gNB->max_nb_pucch; i++) {
    memset(&gNB->pucch[i], 0, sizeof(gNB->pucch[i]));
  }

  gNB->srs = (NR_gNB_SRS_t *)malloc16(gNB->max_nb_srs * sizeof(NR_gNB_SRS_t));
  for (int i = 0; i < gNB->max_nb_srs; i++)
    gNB->srs[i].active = 0;

  gNB->ulsch = (NR_gNB_ULSCH_t *)malloc16(gNB->max_nb_pusch * sizeof(NR_gNB_ULSCH_t));
  for (int i = 0; i < gNB->max_nb_pusch; i++) {
    LOG_D(PHY, "Allocating Transport Channel Buffers for ULSCH %d/%d\n", i, gNB->max_nb_pusch);
    gNB->ulsch[i] = new_gNB_ulsch(gNB->max_ldpc_iterations, fp->N_RB_UL);
  }

  gNB->rx_total_gain_dB=130;

  //fp->pucch_config_common.deltaPUCCH_Shift = 1;
}

void reset_nr_transport(PHY_VARS_gNB *gNB)
{
  const NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;

  free(gNB->pucch);
  free(gNB->srs);

  for (int i = 0; i < gNB->max_nb_pusch; i++)
    free_gNB_ulsch(&gNB->ulsch[i], fp->N_RB_UL);
  free(gNB->ulsch);
}
