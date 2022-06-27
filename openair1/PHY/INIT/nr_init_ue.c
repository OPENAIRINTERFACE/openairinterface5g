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

#include "phy_init.h"
#include "PHY/phy_extern_nr_ue.h"
#include "openair1/PHY/defs_RU.h"
#include "openair1/PHY/impl_defs_nr.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "assertions.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/NR_REFSIG/pss_nr.h"
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "PHY/MODULATION/nr_modulation.h"

#if 0
void phy_config_harq_ue(module_id_t Mod_id,
                        int CC_id,
                        uint8_t gNB_id,
                        uint16_t max_harq_tx) {
  int num_of_threads,num_of_code_words;
  PHY_VARS_NR_UE *phy_vars_ue = PHY_vars_UE_g[Mod_id][CC_id];

  for (num_of_threads=0; num_of_threads<RX_NB_TH_MAX; num_of_threads++)
    for (num_of_code_words=0; num_of_code_words<NR_MAX_NB_CODEWORDS; num_of_code_words++)
      phy_vars_ue->ulsch[num_of_threads][gNB_id][num_of_code_words]->Mlimit = max_harq_tx;
}
#endif

extern uint16_t beta_cqi[16];

/*! \brief Helper function to allocate memory for DLSCH data structures.
 * \param[out] pdsch Pointer to the LTE_UE_PDSCH structure to initialize.
 * \param[in] frame_parms LTE_DL_FRAME_PARMS structure.
 * \note This function is optimistic in that it expects malloc() to succeed.
 */
void phy_init_nr_ue_PDSCH(NR_UE_PDSCH *const pdsch,
                           const NR_DL_FRAME_PARMS *const fp) {

  AssertFatal( pdsch, "pdsch==0" );

  pdsch->llr128 = (int16_t **)malloc16_clear( sizeof(int16_t *) );
  // FIXME! no further allocation for (int16_t*)pdsch->llr128 !!! expect SIGSEGV
  // FK, 11-3-2015: this is only as a temporary pointer, no memory is stored there
  pdsch->rxdataF_ext            = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->rxdataF_uespec_pilots  = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->rxdataF_comp0          = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->rho                    = (int32_t ***)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t **) );
  pdsch->dl_ch_estimates        = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_ch_estimates_ext    = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_bf_ch_estimates     = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_bf_ch_estimates_ext = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_ch_mag0             = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_ch_magb0            = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_ch_magr0            = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->ptrs_phase_per_slot    = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->ptrs_re_per_slot       = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );
  // the allocated memory size is fixed:
  AssertFatal( fp->nb_antennas_rx <= 4, "nb_antennas_rx > 4" );//Extend the max number of UE Rx antennas to 4

  const size_t num = 7*2*fp->N_RB_DL*12;
  for (int i=0; i<fp->nb_antennas_rx; i++) {
    pdsch->rxdataF_ext[i]              = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
    pdsch->rxdataF_uespec_pilots[i]    = (int32_t *)malloc16_clear( sizeof(int32_t) * fp->N_RB_DL*12);
    pdsch->ptrs_phase_per_slot[i]      = (int32_t *)malloc16_clear( sizeof(int32_t) * 14 );
    pdsch->ptrs_re_per_slot[i]         = (int32_t *)malloc16_clear( sizeof(int32_t) * 14);
    pdsch->rho[i]                      = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*NR_MAX_NB_LAYERS*sizeof(int32_t *) );

    for (int j=0; j<NR_MAX_NB_LAYERS; j++) {
      const int idx = (j*fp->nb_antennas_rx)+i;
      for (int k=0; k<NR_MAX_NB_LAYERS; k++) {
        pdsch->rho[i][j*NR_MAX_NB_LAYERS+k] = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      }
      pdsch->rxdataF_comp0[idx]           = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_estimates[idx]         = (int32_t *)malloc16_clear( sizeof(int32_t) * fp->ofdm_symbol_size*7*2);
      pdsch->dl_ch_estimates_ext[idx]     = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_bf_ch_estimates[idx]      = (int32_t *)malloc16_clear( sizeof(int32_t) * fp->ofdm_symbol_size*7*2);
      pdsch->dl_bf_ch_estimates_ext[idx]  = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_mag0[idx]              = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_magb0[idx]             = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_magr0[idx]             = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
    }
  }
}

void phy_term_nr_ue__PDSCH(NR_UE_PDSCH* pdsch, const NR_DL_FRAME_PARMS *const fp)
{
  for (int i = 0; i < fp->nb_antennas_rx; i++) {
    for (int j = 0; j < NR_MAX_NB_LAYERS; j++) {
      const int idx = j * fp->nb_antennas_rx + i;
      for (int k = 0; k < NR_MAX_NB_LAYERS; k++)
        free_and_zero(pdsch->rho[i][j*NR_MAX_NB_LAYERS+k]);
      free_and_zero(pdsch->rxdataF_comp0[idx]);
      free_and_zero(pdsch->dl_ch_estimates[idx]);
      free_and_zero(pdsch->dl_ch_estimates_ext[idx]);
      free_and_zero(pdsch->dl_bf_ch_estimates[idx]);
      free_and_zero(pdsch->dl_bf_ch_estimates_ext[idx]);
      free_and_zero(pdsch->dl_ch_mag0[idx]);
      free_and_zero(pdsch->dl_ch_magb0[idx]);
      free_and_zero(pdsch->dl_ch_magr0[idx]);
    }
    free_and_zero(pdsch->rxdataF_ext[i]);
    free_and_zero(pdsch->rxdataF_uespec_pilots[i]);
    free_and_zero(pdsch->ptrs_phase_per_slot[i]);
    free_and_zero(pdsch->ptrs_re_per_slot[i]);
    free_and_zero(pdsch->rho[i]);
  }
  free_and_zero(pdsch->pmi_ext);
  int nb_codewords = NR_MAX_NB_LAYERS > 4 ? 2 : 1;
  for (int i=0; i<nb_codewords; i++)
    free_and_zero(pdsch->llr[i]);
  for (int i=0; i<NR_MAX_NB_LAYERS; i++)
    free_and_zero(pdsch->layer_llr[i]);
  free_and_zero(pdsch->llr128);
  free_and_zero(pdsch->rxdataF_ext);
  free_and_zero(pdsch->rxdataF_uespec_pilots);
  free_and_zero(pdsch->rxdataF_comp0);
  free_and_zero(pdsch->rho);
  free_and_zero(pdsch->dl_ch_estimates);
  free_and_zero(pdsch->dl_ch_estimates_ext);
  free_and_zero(pdsch->dl_bf_ch_estimates);
  free_and_zero(pdsch->dl_bf_ch_estimates_ext);
  free_and_zero(pdsch->dl_ch_mag0);
  free_and_zero(pdsch->dl_ch_magb0);
  free_and_zero(pdsch->dl_ch_magr0);
  free_and_zero(pdsch->ptrs_phase_per_slot);
  free_and_zero(pdsch->ptrs_re_per_slot);
}

int init_nr_ue_signal(PHY_VARS_NR_UE *ue, int nb_connected_gNB)
{
  // create shortcuts
  NR_DL_FRAME_PARMS *const fp            = &ue->frame_parms;
  NR_UE_COMMON *const common_vars        = &ue->common_vars;
  NR_UE_PBCH  **const pbch_vars          = ue->pbch_vars;
  NR_UE_PRACH **const prach_vars         = ue->prach_vars;
  NR_UE_CSI_IM **const csiim_vars        = ue->csiim_vars;
  NR_UE_CSI_RS **const csirs_vars        = ue->csirs_vars;
  NR_UE_SRS **const srs_vars             = ue->srs_vars;

  int i, j, slot, symb, gNB_id, th_id;

  LOG_I(PHY, "Initializing UE vars for gNB TXant %u, UE RXant %u\n", fp->nb_antennas_tx, fp->nb_antennas_rx);

  phy_init_nr_top(ue);
  // many memory allocation sizes are hard coded
  AssertFatal( fp->nb_antennas_rx <= 4, "hard coded allocation for ue_common_vars->dl_ch_estimates[gNB_id]" );
  AssertFatal( nb_connected_gNB <= NUMBER_OF_CONNECTED_gNB_MAX, "n_connected_gNB is too large" );
  // init phy_vars_ue

  for (i=0; i<4; i++) {
    ue->rx_gain_max[i] = 135;
    ue->rx_gain_med[i] = 128;
    ue->rx_gain_byp[i] = 120;
  }

  ue->n_connected_gNB = nb_connected_gNB;

  for(gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {
    ue->total_TBS[gNB_id] = 0;
    ue->total_TBS_last[gNB_id] = 0;
    ue->bitrate[gNB_id] = 0;
    ue->total_received_bits[gNB_id] = 0;

    ue->ul_time_alignment[gNB_id].apply_ta = 0;
    ue->ul_time_alignment[gNB_id].ta_frame = -1;
    ue->ul_time_alignment[gNB_id].ta_slot  = -1;
  }
  // init NR modulation lookup tables
  nr_generate_modulation_table();

  /////////////////////////PUCCH init/////////////////////////
  ///////////
  for (th_id = 0; th_id < RX_NB_TH_MAX; th_id++) {
    for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {
      ue->pucch_vars[th_id][gNB_id] = (NR_UE_PUCCH *)malloc16(sizeof(NR_UE_PUCCH));
      for (i=0; i<2; i++)
        ue->pucch_vars[th_id][gNB_id]->active[i] = false;
    }
  }

  ///////////
  ////////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////PUSCH DMRS init/////////////////////////
  ///////////

  // ceil(((NB_RB*6(k)*2(QPSK)/32) // 3 RE *2(QPSK)
  int pusch_dmrs_init_length =  ((fp->N_RB_UL*12)>>5)+1;
  ue->nr_gold_pusch_dmrs = (uint32_t ****)malloc16(fp->slots_per_frame*sizeof(uint32_t ***));
  uint32_t ****pusch_dmrs = ue->nr_gold_pusch_dmrs;

  for (slot=0; slot<fp->slots_per_frame; slot++) {
    pusch_dmrs[slot] = (uint32_t ***)malloc16(fp->symbols_per_slot*sizeof(uint32_t **));
    AssertFatal(pusch_dmrs[slot]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d - malloc failed\n", slot);

    for (symb=0; symb<fp->symbols_per_slot; symb++) {
      pusch_dmrs[slot][symb] = (uint32_t **)malloc16(NR_NB_NSCID*sizeof(uint32_t *));
      AssertFatal(pusch_dmrs[slot][symb]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);

      for (int q=0; q<NR_NB_NSCID; q++) {
        pusch_dmrs[slot][symb][q] = (uint32_t *)malloc16(pusch_dmrs_init_length*sizeof(uint32_t));
        AssertFatal(pusch_dmrs[slot][symb][q]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d symbol %d nscid %d - malloc failed\n", slot, symb, q);
      }
    }
  }

  ///////////
  ////////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////PUSCH PTRS init/////////////////////////
  ///////////

  //------------- config PTRS parameters--------------//
  // ptrs_Uplink_Config->timeDensity.ptrs_mcs1 = 2; // setting MCS values to 0 indicate abscence of time_density field in the configuration
  // ptrs_Uplink_Config->timeDensity.ptrs_mcs2 = 4;
  // ptrs_Uplink_Config->timeDensity.ptrs_mcs3 = 10;
  // ptrs_Uplink_Config->frequencyDensity.n_rb0 = 25;     // setting N_RB values to 0 indicate abscence of frequency_density field in the configuration
  // ptrs_Uplink_Config->frequencyDensity.n_rb1 = 75;
  // ptrs_Uplink_Config->resourceElementOffset = 0;
  //-------------------------------------------------//

  ///////////
  ////////////////////////////////////////////////////////////////////////////////////////////

  for (i=0; i<10; i++)
    ue->tx_power_dBm[i]=-127;

  // init TX buffers
  common_vars->txdata  = (int32_t **)malloc16( fp->nb_antennas_tx*sizeof(int32_t *) );
  common_vars->txdataF = (int32_t **)malloc16( fp->nb_antennas_tx*sizeof(int32_t *) );

  for (i=0; i<fp->nb_antennas_tx; i++) {
    common_vars->txdata[i]  = (int32_t *)malloc16_clear( fp->samples_per_subframe*10*sizeof(int32_t) );
    common_vars->txdataF[i] = (int32_t *)malloc16_clear( fp->samples_per_slot_wCP*sizeof(int32_t) );
  }

  // init RX buffers
  common_vars->rxdata   = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );

  for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
    common_vars->common_vars_rx_data_per_thread[th_id].rxdataF  = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
  }

  for (i=0; i<fp->nb_antennas_rx; i++) {
    common_vars->rxdata[i] = (int32_t *) malloc16_clear( (2*(fp->samples_per_frame)+2048)*sizeof(int32_t) );

    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      common_vars->common_vars_rx_data_per_thread[th_id].rxdataF[i] = (int32_t *)malloc16_clear( sizeof(int32_t)*(fp->samples_per_slot_wCP) );
    }
  }

  // ceil(((NB_RB<<1)*3)/32) // 3 RE *2(QPSK)
  int pdcch_dmrs_init_length =  (((fp->N_RB_DL<<1)*3)>>5)+1;
  //PDCCH DMRS init (gNB offset = 0)
  ue->nr_gold_pdcch[0] = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t **));
  uint32_t ***pdcch_dmrs = ue->nr_gold_pdcch[0];
  AssertFatal(pdcch_dmrs!=NULL, "NR init: pdcch_dmrs malloc failed\n");

  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    pdcch_dmrs[slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t *));
    AssertFatal(pdcch_dmrs[slot]!=NULL, "NR init: pdcch_dmrs for slot %d - malloc failed\n", slot);

    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      pdcch_dmrs[slot][symb] = (uint32_t *)malloc16(pdcch_dmrs_init_length*sizeof(uint32_t));
      AssertFatal(pdcch_dmrs[slot][symb]!=NULL, "NR init: pdcch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);
    }
  }

  // ceil(((NB_RB*6(k)*2(QPSK)/32) // 3 RE *2(QPSK)
  int pdsch_dmrs_init_length =  ((fp->N_RB_DL*12)>>5)+1;

  //PDSCH DMRS init (eNB offset = 0)
  ue->nr_gold_pdsch[0] = (uint32_t ****)malloc16(fp->slots_per_frame*sizeof(uint32_t ***));
  uint32_t ****pdsch_dmrs = ue->nr_gold_pdsch[0];

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

  // DLSCH
  for (gNB_id = 0; gNB_id < ue->n_connected_gNB+1; gNB_id++) {
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      ue->pdsch_vars[th_id][gNB_id] = (NR_UE_PDSCH *)malloc16_clear(sizeof(NR_UE_PDSCH));
    }
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      phy_init_nr_ue_PDSCH( ue->pdsch_vars[th_id][gNB_id], fp );
    }

    int nb_codewords = NR_MAX_NB_LAYERS > 4 ? 2 : 1;
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      for (i=0; i<nb_codewords; i++) {
        ue->pdsch_vars[th_id][gNB_id]->llr[i] = (int16_t *)malloc16_clear( (8*(3*8*8448))*sizeof(int16_t) );//Q_m = 8 bits/Sym, Code_Rate=3, Number of Segments =8, Circular Buffer K_cb = 8448
      }
      for (i=0; i<NR_MAX_NB_LAYERS; i++) {
        ue->pdsch_vars[th_id][gNB_id]->layer_llr[i] = (int16_t *)malloc16_clear( (8*(3*8*8448))*sizeof(int16_t) );//Q_m = 8 bits/Sym, Code_Rate=3, Number of Segments =8, Circular Buffer K_cb = 8448
      }
    }
  }

  for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      ue->pdcch_vars[th_id][gNB_id] = (NR_UE_PDCCH *)malloc16_clear(sizeof(NR_UE_PDCCH));
    }

    prach_vars[gNB_id] = (NR_UE_PRACH *)malloc16_clear(sizeof(NR_UE_PRACH));
    pbch_vars[gNB_id] = (NR_UE_PBCH *)malloc16_clear(sizeof(NR_UE_PBCH));
    csiim_vars[gNB_id] = (NR_UE_CSI_IM *)malloc16_clear(sizeof(NR_UE_CSI_IM));
    csirs_vars[gNB_id] = (NR_UE_CSI_RS *)malloc16_clear(sizeof(NR_UE_CSI_RS));
    srs_vars[gNB_id] = (NR_UE_SRS *)malloc16_clear(sizeof(NR_UE_SRS));

    csiim_vars[gNB_id]->active = false;
    csirs_vars[gNB_id]->active = false;
    srs_vars[gNB_id]->active = false;

    // ceil((NB_RB*8(max allocation per RB)*2(QPSK))/32)
    int csi_dmrs_init_length =  ((fp->N_RB_DL<<4)>>5)+1;
    ue->nr_csi_rs_info = (nr_csi_rs_info_t *)malloc16_clear(sizeof(nr_csi_rs_info_t));
    ue->nr_csi_rs_info->nr_gold_csi_rs = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t **));
    AssertFatal(ue->nr_csi_rs_info->nr_gold_csi_rs!=NULL, "NR init: csi reference signal malloc failed\n");
    for (int slot=0; slot<fp->slots_per_frame; slot++) {
      ue->nr_csi_rs_info->nr_gold_csi_rs[slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t *));
      AssertFatal(ue->nr_csi_rs_info->nr_gold_csi_rs[slot]!=NULL, "NR init: csi reference signal for slot %d - malloc failed\n", slot);
      for (int symb=0; symb<fp->symbols_per_slot; symb++) {
        ue->nr_csi_rs_info->nr_gold_csi_rs[slot][symb] = (uint32_t *)malloc16(csi_dmrs_init_length*sizeof(uint32_t));
        AssertFatal(ue->nr_csi_rs_info->nr_gold_csi_rs[slot][symb]!=NULL, "NR init: csi reference signal for slot %d symbol %d - malloc failed\n", slot, symb);
      }
    }
    ue->nr_csi_rs_info->noise_power = (uint32_t*)malloc16_clear(sizeof(uint32_t));
    ue->nr_csi_rs_info->csi_rs_generated_signal = (int32_t **)malloc16(NR_MAX_NB_PORTS * sizeof(int32_t *) );
    for (i=0; i<NR_MAX_NB_PORTS; i++) {
      ue->nr_csi_rs_info->csi_rs_generated_signal[i] = (int32_t *) malloc16_clear(fp->samples_per_frame_wCP * sizeof(int32_t));
    }
    ue->nr_csi_rs_info->csi_rs_received_signal = (int32_t **)malloc16(fp->nb_antennas_rx * sizeof(int32_t *) );
    ue->nr_csi_rs_info->csi_rs_ls_estimated_channel = (int32_t ***)malloc16(fp->nb_antennas_rx * sizeof(int32_t **) );
    ue->nr_csi_rs_info->csi_rs_estimated_channel_freq = (int32_t ***)malloc16(fp->nb_antennas_rx * sizeof(int32_t **) );
    for (i=0; i<fp->nb_antennas_rx; i++) {
      ue->nr_csi_rs_info->csi_rs_received_signal[i] = (int32_t *) malloc16_clear(fp->samples_per_frame_wCP * sizeof(int32_t));
      ue->nr_csi_rs_info->csi_rs_ls_estimated_channel[i] = (int32_t **) malloc16_clear(NR_MAX_NB_PORTS * sizeof(int32_t *));
      ue->nr_csi_rs_info->csi_rs_estimated_channel_freq[i] = (int32_t **) malloc16_clear(NR_MAX_NB_PORTS * sizeof(int32_t *));
      for (j=0; j<NR_MAX_NB_PORTS; j++) {
        ue->nr_csi_rs_info->csi_rs_ls_estimated_channel[i][j] = (int32_t *) malloc16_clear(fp->ofdm_symbol_size * sizeof(int32_t));
        ue->nr_csi_rs_info->csi_rs_estimated_channel_freq[i][j] = (int32_t *) malloc16_clear(fp->ofdm_symbol_size * sizeof(int32_t));
      }
    }

    ue->nr_srs_info = (nr_srs_info_t *)malloc16_clear(sizeof(nr_srs_info_t));
    ue->nr_srs_info->sc_list = (uint16_t *) malloc16_clear(6*fp->N_RB_UL*sizeof(uint16_t));
    ue->nr_srs_info->srs_generated_signal = (int32_t *) malloc16_clear( (2*(fp->samples_per_frame)+2048)*sizeof(int32_t) );
    ue->nr_srs_info->noise_power = (uint32_t*)malloc16_clear(sizeof(uint32_t));
    ue->nr_srs_info->srs_received_signal = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
    ue->nr_srs_info->srs_ls_estimated_channel = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
    ue->nr_srs_info->srs_estimated_channel_freq = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
    ue->nr_srs_info->srs_estimated_channel_time = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
    ue->nr_srs_info->srs_estimated_channel_time_shifted = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
    for (i=0; i<fp->nb_antennas_rx; i++) {
      ue->nr_srs_info->srs_received_signal[i] = (int32_t *) malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
      ue->nr_srs_info->srs_ls_estimated_channel[i] = (int32_t *) malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
      ue->nr_srs_info->srs_estimated_channel_freq[i] = (int32_t *) malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
      ue->nr_srs_info->srs_estimated_channel_time[i] = (int32_t *) malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
      ue->nr_srs_info->srs_estimated_channel_time_shifted[i] = (int32_t *) malloc16_clear(fp->ofdm_symbol_size*MAX_NUM_NR_SRS_SYMBOLS*sizeof(int32_t));
    }


    // RACH
    prach_vars[gNB_id]->prachF             = (int16_t *)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(fp->ofdm_symbol_size*12)) );
    prach_vars[gNB_id]->prach              = (int16_t *)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(fp->ofdm_symbol_size*12)) );

  }

  ue->sinr_CQI_dB = (double *) malloc16_clear( fp->N_RB_DL*12*sizeof(double) );
  ue->init_averaging = 1;

  // default value until overwritten by RRCConnectionReconfiguration
  if (fp->nb_antenna_ports_gNB==2)
    ue->pdsch_config_dedicated->p_a = dBm3;
  else
    ue->pdsch_config_dedicated->p_a = dB0;

  // enable MIB/SIB decoding by default
  ue->decode_MIB = 1;
  ue->decode_SIB = 1;

  init_nr_prach_tables(839);
  init_symbol_rotation(fp);
  return 0;
}

void term_nr_ue_signal(PHY_VARS_NR_UE *ue, int nb_connected_gNB)
{
  const NR_DL_FRAME_PARMS* fp = &ue->frame_parms;
  phy_term_nr_top();

  for (int th_id = 0; th_id < RX_NB_TH_MAX; th_id++) {
    for (int gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {
      free_and_zero(ue->pucch_vars[th_id][gNB_id]);
    }
  }

  for (int slot = 0; slot < fp->slots_per_frame; slot++) {
    for (int symb = 0; symb < fp->symbols_per_slot; symb++) {
      for (int q=0; q<NR_NB_NSCID; q++)
        free_and_zero(ue->nr_gold_pusch_dmrs[slot][symb][q]);
      free_and_zero(ue->nr_gold_pusch_dmrs[slot][symb]);
    }
    free_and_zero(ue->nr_gold_pusch_dmrs[slot]);
  }
  free_and_zero(ue->nr_gold_pusch_dmrs);

  NR_UE_COMMON* common_vars = &ue->common_vars;

  for (int i = 0; i < fp->nb_antennas_tx; i++) {
    free_and_zero(common_vars->txdata[i]);
    free_and_zero(common_vars->txdataF[i]);
  }

  free_and_zero(common_vars->txdata);
  free_and_zero(common_vars->txdataF);

  for (int i = 0; i < fp->nb_antennas_rx; i++) {
    free_and_zero(common_vars->rxdata[i]);
    for (int th_id = 0; th_id < RX_NB_TH_MAX; th_id++)
      free_and_zero(common_vars->common_vars_rx_data_per_thread[th_id].rxdataF[i]);
  }
  for (int th_id = 0; th_id < RX_NB_TH_MAX; th_id++) {
    free_and_zero(common_vars->common_vars_rx_data_per_thread[th_id].rxdataF);
  }
  free_and_zero(common_vars->rxdata);

  for (int slot = 0; slot < fp->slots_per_frame; slot++) {
    for (int symb = 0; symb < fp->symbols_per_slot; symb++)
      free_and_zero(ue->nr_gold_pdcch[0][slot][symb]);
    free_and_zero(ue->nr_gold_pdcch[0][slot]);
  }
  free_and_zero(ue->nr_gold_pdcch[0]);

  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      for (int q=0; q<NR_NB_NSCID; q++)
        free_and_zero(ue->nr_gold_pdsch[0][slot][symb][q]);
      free_and_zero(ue->nr_gold_pdsch[0][slot][symb]);
    }
    free_and_zero(ue->nr_gold_pdsch[0][slot]);
  }
  free_and_zero(ue->nr_gold_pdsch[0]);

  for (int gNB_id = 0; gNB_id < ue->n_connected_gNB+1; gNB_id++) {

    // PDSCH
    for (int th_id = 0; th_id < RX_NB_TH_MAX; th_id++) {
      free_and_zero(ue->pdsch_vars[th_id][gNB_id]->llr_shifts);
      free_and_zero(ue->pdsch_vars[th_id][gNB_id]->llr128_2ndstream);
      phy_term_nr_ue__PDSCH(ue->pdsch_vars[th_id][gNB_id], fp);
      free_and_zero(ue->pdsch_vars[th_id][gNB_id]);
    }
  }
  
  for (int gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {

    for (int th_id = 0; th_id < RX_NB_TH_MAX; th_id++) {

      free_and_zero(ue->pdcch_vars[th_id][gNB_id]);
    }

    for (int i=0; i<NR_MAX_NB_PORTS; i++) {
      free_and_zero(ue->nr_csi_rs_info->csi_rs_generated_signal[i]);
    }
    for (int i = 0; i < fp->nb_antennas_rx; i++) {
      free_and_zero(ue->nr_csi_rs_info->csi_rs_received_signal[i]);
      for (int j=0; j<NR_MAX_NB_PORTS; j++) {
        free_and_zero(ue->nr_csi_rs_info->csi_rs_ls_estimated_channel[i][j]);
        free_and_zero(ue->nr_csi_rs_info->csi_rs_estimated_channel_freq[i][j]);
      }
      free_and_zero(ue->nr_csi_rs_info->csi_rs_ls_estimated_channel[i]);
      free_and_zero(ue->nr_csi_rs_info->csi_rs_estimated_channel_freq[i]);
    }
    for (int slot=0; slot<fp->slots_per_frame; slot++) {
      for (int symb=0; symb<fp->symbols_per_slot; symb++) {
        free_and_zero(ue->nr_csi_rs_info->nr_gold_csi_rs[slot][symb]);
      }
      free_and_zero(ue->nr_csi_rs_info->nr_gold_csi_rs[slot]);
    }
    free_and_zero(ue->nr_csi_rs_info->noise_power);
    free_and_zero(ue->nr_csi_rs_info->nr_gold_csi_rs);
    free_and_zero(ue->nr_csi_rs_info->csi_rs_generated_signal);
    free_and_zero(ue->nr_csi_rs_info->csi_rs_received_signal);
    free_and_zero(ue->nr_csi_rs_info->csi_rs_ls_estimated_channel);
    free_and_zero(ue->nr_csi_rs_info->csi_rs_estimated_channel_freq);
    free_and_zero(ue->nr_csi_rs_info);

    for (int i = 0; i < fp->nb_antennas_rx; i++) {
      free_and_zero(ue->nr_srs_info->srs_received_signal[i]);
      free_and_zero(ue->nr_srs_info->srs_ls_estimated_channel[i]);
      free_and_zero(ue->nr_srs_info->srs_estimated_channel_freq[i]);
      free_and_zero(ue->nr_srs_info->srs_estimated_channel_time[i]);
      free_and_zero(ue->nr_srs_info->srs_estimated_channel_time_shifted[i]);
    }
    free_and_zero(ue->nr_srs_info->sc_list);
    free_and_zero(ue->nr_srs_info->srs_generated_signal);
    free_and_zero(ue->nr_srs_info->noise_power);
    free_and_zero(ue->nr_srs_info->srs_received_signal);
    free_and_zero(ue->nr_srs_info->srs_ls_estimated_channel);
    free_and_zero(ue->nr_srs_info->srs_estimated_channel_freq);
    free_and_zero(ue->nr_srs_info->srs_estimated_channel_time);
    free_and_zero(ue->nr_srs_info->srs_estimated_channel_time_shifted);
    free_and_zero(ue->nr_srs_info);

    free_and_zero(ue->csiim_vars[gNB_id]);
    free_and_zero(ue->csirs_vars[gNB_id]);
    free_and_zero(ue->srs_vars[gNB_id]);

    free_and_zero(ue->pbch_vars[gNB_id]);

    free_and_zero(ue->prach_vars[gNB_id]->prachF);
    free_and_zero(ue->prach_vars[gNB_id]->prach);
    free_and_zero(ue->prach_vars[gNB_id]);
  }

  free_and_zero(ue->sinr_CQI_dB);
}

void term_nr_ue_transport(PHY_VARS_NR_UE *ue)
{
  const int N_RB_DL = ue->frame_parms.N_RB_DL;
  for (int i = 0; i < NUMBER_OF_CONNECTED_gNB_MAX; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < RX_NB_TH_MAX; k++) {
        free_nr_ue_dlsch(&ue->dlsch[k][i][j], N_RB_DL);
        if (j==0)
          free_nr_ue_ulsch(&ue->ulsch[k][i], N_RB_DL, &ue->frame_parms);
      }
    }

    free_nr_ue_dlsch(&ue->dlsch_SI[i], N_RB_DL);
    free_nr_ue_dlsch(&ue->dlsch_ra[i], N_RB_DL);
  }

  free_nr_ue_dlsch(&ue->dlsch_MCH[0], N_RB_DL);
}

void init_nr_ue_transport(PHY_VARS_NR_UE *ue) {

  int num_codeword = NR_MAX_NB_LAYERS > 4? 2:1;

  for (int i = 0; i < NUMBER_OF_CONNECTED_gNB_MAX; i++) {
    for (int j=0; j<num_codeword; j++) {
      for (int k=0; k<RX_NB_TH_MAX; k++) {
        AssertFatal((ue->dlsch[k][i][j]  = new_nr_ue_dlsch(1,NR_MAX_DLSCH_HARQ_PROCESSES,NSOFT,ue->max_ldpc_iterations,ue->frame_parms.N_RB_DL))!=NULL,"Can't get ue dlsch structures\n");
        LOG_D(PHY,"dlsch[%d][%d][%d] => %p\n",k,i,j,ue->dlsch[k][i][j]);
        if (j==0) {
          AssertFatal((ue->ulsch[k][i] = new_nr_ue_ulsch(ue->frame_parms.N_RB_UL, NR_MAX_ULSCH_HARQ_PROCESSES,&ue->frame_parms))!=NULL,"Can't get ue ulsch structures\n");
          LOG_D(PHY,"ulsch[%d][%d] => %p\n",k,i,ue->ulsch[k][i]);
        }
      }
    }

    ue->dlsch_SI[i]  = new_nr_ue_dlsch(1,1,NSOFT,ue->max_ldpc_iterations,ue->frame_parms.N_RB_DL);
    ue->dlsch_ra[i]  = new_nr_ue_dlsch(1,1,NSOFT,ue->max_ldpc_iterations,ue->frame_parms.N_RB_DL);
    ue->transmission_mode[i] = ue->frame_parms.nb_antenna_ports_gNB==1 ? 1 : 2;
  }

  //ue->frame_parms.pucch_config_common.deltaPUCCH_Shift = 1;
  ue->dlsch_MCH[0]  = new_nr_ue_dlsch(1,NR_MAX_DLSCH_HARQ_PROCESSES,NSOFT,MAX_LDPC_ITERATIONS_MBSFN,ue->frame_parms.N_RB_DL);

  for(int i=0; i<5; i++)
    ue->dl_stats[i] = 0;
}


void init_N_TA_offset(PHY_VARS_NR_UE *ue){

  NR_DL_FRAME_PARMS *fp = &ue->frame_parms;

  if (fp->frame_type == FDD) {
    ue->N_TA_offset = 0;
  } else {
    int N_TA_offset = fp->ul_CarrierFreq < 6e9 ? 400 : 431; // reference samples  for 25600Tc @ 30.72 Ms/s for FR1, same @ 61.44 Ms/s for FR2

    double factor = 1.0;
    switch (fp->numerology_index) {
      case 0: //15 kHz scs
        AssertFatal(N_TA_offset == 400, "scs_common 15kHz only for FR1\n");
        factor = fp->samples_per_subframe / 30720.0;
        break;
      case 1: //30 kHz sc
        AssertFatal(N_TA_offset == 400, "scs_common 30kHz only for FR1\n");
        factor = fp->samples_per_subframe / 30720.0;
        break;
      case 2: //60 kHz scs
        AssertFatal(1==0, "scs_common should not be 60 kHz\n");
        break;
      case 3: //120 kHz scs
        AssertFatal(N_TA_offset == 431, "scs_common 120kHz only for FR2\n");
        factor = fp->samples_per_subframe / 61440.0;
        break;
      case 4: //240 kHz scs
        AssertFatal(N_TA_offset == 431, "scs_common 240kHz only for FR2\n");
        factor = fp->samples_per_subframe / 61440.0;
        break;
      default:
        AssertFatal(1==0, "Invalid scs_common!\n");
    }

    ue->N_TA_offset = (int)(N_TA_offset * factor);

    LOG_I(PHY,"UE %d Setting N_TA_offset to %d samples (factor %f, UL Freq %lu, N_RB %d, mu %d)\n", ue->Mod_id, ue->N_TA_offset, factor, fp->ul_CarrierFreq, fp->N_RB_DL, fp->numerology_index);
  }
}

void phy_init_nr_top(PHY_VARS_NR_UE *ue) {
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  crcTableInit();
  init_scrambling_luts();
  load_dftslib();
  init_context_synchro_nr(frame_parms);
  generate_ul_reference_signal_sequences(SHRT_MAX);
  // Polar encoder init for PBCH
  //lte_sync_time_init(frame_parms);
  //generate_ul_ref_sigs();
  //generate_ul_ref_sigs_rx();
  //generate_64qam_table();
  //generate_16qam_table();
  //generate_RIV_tables();
  //init_unscrambling_lut();
  //set_taus_seed(1328);
}

void phy_term_nr_top(void)
{
  free_ul_reference_signal_sequences();
  free_context_synchro_nr();
}
