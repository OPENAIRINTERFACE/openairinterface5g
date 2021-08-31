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
#include "SCHED_UE/sched_UE.h"
#include "PHY/phy_extern_nr_ue.h"
//#include "SIMULATION/TOOLS/sim.h"
/*#include "RadioResourceConfigCommonSIB.h"
#include "RadioResourceConfigDedicated.h"
#include "TDD-Config.h"
#include "MBSFN-SubframeConfigList.h"*/
#include "openair1/PHY/defs_RU.h"
#include "openair1/PHY/impl_defs_nr.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "assertions.h"
#include <math.h>
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
//#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "PHY/CODING/nrPolar_tools/nr_polar_pbch_defs.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_REFSIG/pss_nr.h"
#include "PHY/NR_REFSIG/ul_ref_seq_nr.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_REFSIG/nr_refsig.h"

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
void phy_init_nr_ue__PDSCH(NR_UE_PDSCH *const pdsch,
                           const NR_DL_FRAME_PARMS *const fp) {
  AssertFatal( pdsch, "pdsch==0" );
  pdsch->pmi_ext = (uint8_t *)malloc16_clear( fp->N_RB_DL );
  pdsch->llr[0] = (int16_t *)malloc16_clear( (8*(3*8*6144))*sizeof(int16_t) );
  pdsch->layer_llr[0] = (int16_t *)malloc16_clear( (8*(3*8*6144))*sizeof(int16_t) );
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
  //pdsch->dl_ch_rho_ext          = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  //pdsch->dl_ch_rho2_ext         = (int32_t**)malloc16_clear( 8*sizeof(int32_t*) );
  pdsch->dl_ch_mag0             = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_ch_magb0            = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_ch_magr0            = (int32_t **)malloc16_clear( NR_MAX_NB_LAYERS*fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->ptrs_phase_per_slot    = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->ptrs_re_per_slot       = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );
  pdsch->dl_ch_ptrs_estimates_ext = (int32_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(int32_t *) );
  // the allocated memory size is fixed:
  AssertFatal( fp->nb_antennas_rx <= 4, "nb_antennas_rx > 4" );//Extend the max number of UE Rx antennas to 4

  const size_t num = 7*2*fp->N_RB_DL*12;
  for (int i=0; i<fp->nb_antennas_rx; i++) {
    pdsch->rxdataF_ext[i]              = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
    pdsch->rxdataF_uespec_pilots[i]    = (int32_t *)malloc16_clear( sizeof(int32_t) * fp->N_RB_DL*12);
    pdsch->ptrs_phase_per_slot[i]      = (int32_t *)malloc16_clear( sizeof(int32_t) * 14 );
    pdsch->ptrs_re_per_slot[i]         = (int32_t *)malloc16_clear( sizeof(int32_t) * 14);
    pdsch->dl_ch_ptrs_estimates_ext[i] = (int32_t *)malloc16_clear( sizeof(int32_t) * num);
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
      //pdsch->dl_ch_rho_ext[idx]           = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      //pdsch->dl_ch_rho2_ext[idx]          = (int32_t*)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_mag0[idx]              = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_magb0[idx]             = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
      pdsch->dl_ch_magr0[idx]             = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
    }
  }
}

void phy_init_nr_ue_PUSCH(NR_UE_PUSCH *const pusch,
                          const NR_DL_FRAME_PARMS *const fp) {
  AssertFatal( pusch, "pusch==0" );

  for (int i=0; i<NR_MAX_NB_LAYERS; i++) {
    pusch->txdataF_layers[i] = (int32_t *)malloc16_clear(NR_MAX_PUSCH_ENCODED_LENGTH*sizeof(int32_t));
  }
}

int init_nr_ue_signal(PHY_VARS_NR_UE *ue,
                      int nb_connected_gNB,
                      uint8_t abstraction_flag) {
  // create shortcuts
  NR_DL_FRAME_PARMS *const fp            = &ue->frame_parms;
  NR_UE_COMMON *const common_vars        = &ue->common_vars;
  NR_UE_PBCH  **const pbch_vars          = ue->pbch_vars;
  NR_UE_PRACH **const prach_vars         = ue->prach_vars;
  int i,j,k,l,slot,symb,q;
  int gNB_id;
  int th_id;
  uint32_t ****pusch_dmrs;
  uint16_t N_n_scid[2] = {0,1}; // [HOTFIX] This is a temporary implementation of scramblingID0 and scramblingID1 which are given by DMRS-UplinkConfig
  int n_scid;
  abstraction_flag = 0;
  LOG_I(PHY, "Initializing UE vars (abstraction %u) for gNB TXant %u, UE RXant %u\n", abstraction_flag, fp->nb_antennas_tx, fp->nb_antennas_rx);
  //LOG_D(PHY,"[MSC_NEW][FRAME 00000][PHY_UE][MOD %02u][]\n", ue->Mod_id+NB_gNB_INST);
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

  /////////////////////////PUSCH init/////////////////////////
  ///////////
  for (th_id = 0; th_id < RX_NB_TH_MAX; th_id++) {
    for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {
      ue->pusch_vars[th_id][gNB_id] = (NR_UE_PUSCH *)malloc16(sizeof(NR_UE_PUSCH));
      phy_init_nr_ue_PUSCH( ue->pusch_vars[th_id][gNB_id], fp );
    }
  }

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
  ue->nr_gold_pusch_dmrs = (uint32_t ****)malloc16(fp->slots_per_frame*sizeof(uint32_t ***));
  pusch_dmrs             = ue->nr_gold_pusch_dmrs;
  n_scid = 0; // This quantity is indicated by higher layer parameter dmrs-SeqInitialization

  for (slot=0; slot<fp->slots_per_frame; slot++) {
    pusch_dmrs[slot] = (uint32_t ***)malloc16(fp->symbols_per_slot*sizeof(uint32_t **));
    AssertFatal(pusch_dmrs[slot]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d - malloc failed\n", slot);

    for (symb=0; symb<fp->symbols_per_slot; symb++) {
      pusch_dmrs[slot][symb] = (uint32_t **)malloc16(NR_MAX_NB_CODEWORDS*sizeof(uint32_t *));
      AssertFatal(pusch_dmrs[slot][symb]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);

      for (q=0; q<NR_MAX_NB_CODEWORDS; q++) {
        pusch_dmrs[slot][symb][q] = (uint32_t *)malloc16(NR_MAX_PDSCH_DMRS_INIT_LENGTH_DWORD*sizeof(uint32_t));
        AssertFatal(pusch_dmrs[slot][symb][q]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d symbol %d codeword %d - malloc failed\n", slot, symb, q);
      }
    }
  }

  nr_init_pusch_dmrs(ue, N_n_scid, n_scid);
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

  if (abstraction_flag == 0) {
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
  }

  //PDCCH DMRS init (gNB offset = 0)
  ue->nr_gold_pdcch[0] = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t **));
  uint32_t ***pdcch_dmrs = ue->nr_gold_pdcch[0];
  AssertFatal(pdcch_dmrs!=NULL, "NR init: pdcch_dmrs malloc failed\n");

  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    pdcch_dmrs[slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t *));
    AssertFatal(pdcch_dmrs[slot]!=NULL, "NR init: pdcch_dmrs for slot %d - malloc failed\n", slot);

    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      pdcch_dmrs[slot][symb] = (uint32_t *)malloc16(NR_MAX_PDCCH_DMRS_INIT_LENGTH_DWORD*sizeof(uint32_t));
      AssertFatal(pdcch_dmrs[slot][symb]!=NULL, "NR init: pdcch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);
    }
  }

  ue->scramblingID_pdcch = fp->Nid_cell;
  nr_gold_pdcch(ue,fp->Nid_cell);

  //PDSCH DMRS init (eNB offset = 0)
  ue->nr_gold_pdsch[0] = (uint32_t ****)malloc16(fp->slots_per_frame*sizeof(uint32_t ***));
  uint32_t ****pdsch_dmrs = ue->nr_gold_pdsch[0];

  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    pdsch_dmrs[slot] = (uint32_t ***)malloc16(fp->symbols_per_slot*sizeof(uint32_t **));
    AssertFatal(pdsch_dmrs[slot]!=NULL, "NR init: pdsch_dmrs for slot %d - malloc failed\n", slot);

    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      pdsch_dmrs[slot][symb] = (uint32_t **)malloc16(NR_MAX_NB_CODEWORDS*sizeof(uint32_t *));
      AssertFatal(pdsch_dmrs[slot][symb]!=NULL, "NR init: pdsch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);

      for (int q=0; q<NR_MAX_NB_CODEWORDS; q++) {
        pdsch_dmrs[slot][symb][q] = (uint32_t *)malloc16(NR_MAX_PDSCH_DMRS_INIT_LENGTH_DWORD*sizeof(uint32_t));
        AssertFatal(pdsch_dmrs[slot][symb][q]!=NULL, "NR init: pdsch_dmrs for slot %d symbol %d codeword %d - malloc failed\n", slot, symb, q);
      }
    }
  }

  // initializing the scrambling IDs for PDSCH DMRS
  for (int i=0; i<2; i++)
    ue->scramblingID[i]=fp->Nid_cell;

  nr_gold_pdsch(ue,ue->scramblingID);

  // DLSCH
  for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      ue->pdsch_vars[th_id][gNB_id] = (NR_UE_PDSCH *)malloc16_clear(sizeof(NR_UE_PDSCH));
    }

    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      ue->pdcch_vars[th_id][gNB_id] = (NR_UE_PDCCH *)malloc16_clear(sizeof(NR_UE_PDCCH));
    }

    prach_vars[gNB_id] = (NR_UE_PRACH *)malloc16_clear(sizeof(NR_UE_PRACH));
    pbch_vars[gNB_id] = (NR_UE_PBCH *)malloc16_clear(sizeof(NR_UE_PBCH));

    if (abstraction_flag == 0) {
      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        phy_init_nr_ue__PDSCH( ue->pdsch_vars[th_id][gNB_id], fp );
      }

      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        ue->pdsch_vars[th_id][gNB_id]->llr_shifts          = (uint8_t *)malloc16_clear(7*2*fp->N_RB_DL*12);
        ue->pdsch_vars[th_id][gNB_id]->llr_shifts_p        = ue->pdsch_vars[0][gNB_id]->llr_shifts;
        ue->pdsch_vars[th_id][gNB_id]->llr[1]              = (int16_t *)malloc16_clear( (8*(3*8*8448))*sizeof(int16_t) );
        ue->pdsch_vars[th_id][gNB_id]->layer_llr[1]        = (int16_t *)malloc16_clear( (8*(3*8*8448))*sizeof(int16_t) );
        ue->pdsch_vars[th_id][gNB_id]->llr128_2ndstream    = (int16_t **)malloc16_clear( sizeof(int16_t *) );
      }


      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        ue->pdsch_vars[th_id][gNB_id]->dl_ch_rho2_ext      = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
      }

      for (i=0; i<fp->nb_antennas_rx; i++)
        for (j=0; j<4; j++) {
          const int idx = (j*fp->nb_antennas_rx)+i;
          const size_t num = 7*2*fp->N_RB_DL*12+4;

          for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
            ue->pdsch_vars[th_id][gNB_id]->dl_ch_rho2_ext[idx] = (int32_t *)malloc16_clear( sizeof(int32_t) * num );
          }
        }

      //const size_t num = 7*2*fp->N_RB_DL*12+4;
      for (k=0; k<8; k++) { //harq_pid
        for (l=0; l<8; l++) { //round
          for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
            ue->pdsch_vars[th_id][gNB_id]->rxdataF_comp1[k][l] = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
            ue->pdsch_vars[th_id][gNB_id]->dl_ch_rho_ext[k][l] = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
            ue->pdsch_vars[th_id][gNB_id]->dl_ch_mag1[k][l]    = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
            ue->pdsch_vars[th_id][gNB_id]->dl_ch_magb1[k][l]   = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
          }

          for (int i=0; i<fp->nb_antennas_rx; i++)
            for (int j=0; j<4; j++) { //frame_parms->nb_antennas_tx; j++)
              const int idx = (j*fp->nb_antennas_rx)+i;

              for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
                ue->pdsch_vars[th_id][gNB_id]->dl_ch_rho_ext[k][l][idx] = (int32_t *)malloc16_clear(7*2*sizeof(int32_t)*(fp->N_RB_DL*12));
                ue->pdsch_vars[th_id][gNB_id]->rxdataF_comp1[k][l][idx] = (int32_t *)malloc16_clear(7*2*sizeof(int32_t)*(fp->N_RB_DL*12));
                ue->pdsch_vars[th_id][gNB_id]->dl_ch_mag1[k][l][idx]    = (int32_t *)malloc16_clear(7*2*sizeof(int32_t)*(fp->N_RB_DL*12));
                ue->pdsch_vars[th_id][gNB_id]->dl_ch_magb1[k][l][idx]   = (int32_t *)malloc16_clear(7*2*sizeof(int32_t)*(fp->N_RB_DL*12));
              }
            }
        }
      }

      // 100 PRBs * 12 REs/PRB * 4 PDCCH SYMBOLS * 2 LLRs/RE
      for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
        ue->pdcch_vars[th_id][gNB_id]->llr                 = (int16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
        ue->pdcch_vars[th_id][gNB_id]->llr16               = (int16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
        ue->pdcch_vars[th_id][gNB_id]->wbar                = (int16_t *)malloc16_clear( 2*4*100*12*sizeof(uint16_t) );
        ue->pdcch_vars[th_id][gNB_id]->e_rx                = (int16_t *)malloc16_clear( 4*2*100*12 );
        ue->pdcch_vars[th_id][gNB_id]->rxdataF_comp        = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
        ue->pdcch_vars[th_id][gNB_id]->dl_ch_rho_ext       = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
        ue->pdcch_vars[th_id][gNB_id]->rho                 = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
        ue->pdcch_vars[th_id][gNB_id]->rxdataF_ext         = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
        ue->pdcch_vars[th_id][gNB_id]->dl_ch_estimates_ext = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
        // Channel estimates
        ue->pdcch_vars[th_id][gNB_id]->dl_ch_estimates      = (int32_t **)malloc16_clear(4*fp->nb_antennas_rx*sizeof(int32_t *));
        ue->pdcch_vars[th_id][gNB_id]->dl_ch_estimates_time = (int32_t **)malloc16_clear(4*fp->nb_antennas_rx*sizeof(int32_t *));

        for (i=0; i<fp->nb_antennas_rx; i++) {
          ue->pdcch_vars[th_id][gNB_id]->rho[i] = (int32_t *)malloc16_clear( sizeof(int32_t)*(100*12*4));

          for (j=0; j<4; j++) {
            int idx = (j*fp->nb_antennas_rx)+i;
            ue->pdcch_vars[th_id][gNB_id]->dl_ch_estimates[idx] = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->symbols_per_slot*(fp->ofdm_symbol_size+LTE_CE_FILTER_LENGTH) );
            ue->pdcch_vars[th_id][gNB_id]->dl_ch_estimates_time[idx] = (int32_t *)malloc16_clear( sizeof(int32_t)*fp->ofdm_symbol_size*2 );
            //  size_t num = 7*2*fp->N_RB_DL*12;
            size_t num = 4*273*12;  // 4 symbols, 100 PRBs, 12 REs per PRB
            ue->pdcch_vars[th_id][gNB_id]->rxdataF_comp[idx]        = (int32_t *)malloc16_clear(sizeof(int32_t) * num);
            ue->pdcch_vars[th_id][gNB_id]->dl_ch_rho_ext[idx]       = (int32_t *)malloc16_clear(sizeof(int32_t) * num);
            ue->pdcch_vars[th_id][gNB_id]->rxdataF_ext[idx]         = (int32_t *)malloc16_clear(sizeof(int32_t) * num);
            ue->pdcch_vars[th_id][gNB_id]->dl_ch_estimates_ext[idx] = (int32_t *)malloc16_clear(sizeof(int32_t) * num);
          }
        }
      }

      // PBCH
      pbch_vars[gNB_id]->rxdataF_ext         = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );
      pbch_vars[gNB_id]->rxdataF_comp        = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
      pbch_vars[gNB_id]->dl_ch_estimates     = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
      pbch_vars[gNB_id]->dl_ch_estimates_ext = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
      pbch_vars[gNB_id]->dl_ch_estimates_time = (int32_t **)malloc16_clear( 4*fp->nb_antennas_rx*sizeof(int32_t *) );
      pbch_vars[gNB_id]->llr                 = (int16_t *)malloc16_clear( 1920 ); //
      prach_vars[gNB_id]->prachF             = (int16_t *)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(fp->ofdm_symbol_size*12)) );
      prach_vars[gNB_id]->prach              = (int16_t *)malloc16_clear( sizeof(int)*(7*2*sizeof(int)*(fp->ofdm_symbol_size*12)) );

      for (i=0; i<fp->nb_antennas_rx; i++) {
        pbch_vars[gNB_id]->rxdataF_ext[i]    = (int32_t *)malloc16_clear( sizeof(int32_t)*20*12*4 );

        for (j=0; j<4; j++) {//fp->nb_antennas_tx;j++) {
          int idx = (j*fp->nb_antennas_rx)+i;
          pbch_vars[gNB_id]->rxdataF_comp[idx]        = (int32_t *)malloc16_clear( sizeof(int32_t)*20*12*4 );
          pbch_vars[gNB_id]->dl_ch_estimates[idx]     = (int32_t *)malloc16_clear( sizeof(int32_t)*7*2*sizeof(int)*(fp->ofdm_symbol_size) );
          pbch_vars[gNB_id]->dl_ch_estimates_time[idx]= (int32_t *)malloc16_clear( sizeof(int32_t)*7*2*sizeof(int)*(fp->ofdm_symbol_size) );
          pbch_vars[gNB_id]->dl_ch_estimates_ext[idx] = (int32_t *)malloc16_clear( sizeof(int32_t)*20*12*4 );
        }
      }
    }

    pbch_vars[gNB_id]->decoded_output = (uint8_t *)malloc16_clear(64);
  }

  // initialization for the last instance of pdsch_vars (used for MU-MIMO)
  for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
    ue->pdsch_vars[th_id][gNB_id] = (NR_UE_PDSCH *)malloc16_clear(sizeof(NR_UE_PDSCH));
  }

  if (abstraction_flag == 0) {
    for (th_id=0; th_id<RX_NB_TH_MAX; th_id++) {
      //phy_init_lte_ue__PDSCH( ue->pdsch_vars[th_id][gNB_id], fp );
      ue->pdsch_vars[th_id][gNB_id]->llr[1] = (int16_t *)malloc16_clear((8*(3*8*8448))*sizeof(int16_t));
      ue->pdsch_vars[th_id][gNB_id]->layer_llr[1] = (int16_t *)malloc16_clear((8*(3*8*8448))*sizeof(int16_t));
    }
  } else { //abstraction == 1
    ue->sinr_dB = (double *) malloc16_clear( fp->N_RB_DL*12*sizeof(double) );
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

  return 0;
}

void init_nr_ue_transport(PHY_VARS_NR_UE *ue,
                          int abstraction_flag) {
  for (int i = 0; i < NUMBER_OF_CONNECTED_gNB_MAX; i++) {
    for (int j=0; j<2; j++) {
      for (int k=0; k<RX_NB_TH_MAX; k++) {
        AssertFatal((ue->dlsch[k][i][j]  = new_nr_ue_dlsch(1,NR_MAX_DLSCH_HARQ_PROCESSES,NSOFT,MAX_LDPC_ITERATIONS,ue->frame_parms.N_RB_DL, abstraction_flag))!=NULL,"Can't get ue dlsch structures\n");
        LOG_D(PHY,"dlsch[%d][%d][%d] => %p\n",k,i,j,ue->dlsch[k][i][j]);
        AssertFatal((ue->ulsch[k][i][j]  = new_nr_ue_ulsch(ue->frame_parms.N_RB_UL, NR_MAX_ULSCH_HARQ_PROCESSES, abstraction_flag))!=NULL,"Can't get ue ulsch structures\n");
        LOG_D(PHY,"ulsch[%d][%d][%d] => %p\n",k,i,j,ue->ulsch[k][i][j]);
      }
    }

    ue->dlsch_SI[i]  = new_nr_ue_dlsch(1,1,NSOFT,MAX_LDPC_ITERATIONS,ue->frame_parms.N_RB_DL, abstraction_flag);
    ue->dlsch_ra[i]  = new_nr_ue_dlsch(1,1,NSOFT,MAX_LDPC_ITERATIONS,ue->frame_parms.N_RB_DL, abstraction_flag);
    ue->transmission_mode[i] = ue->frame_parms.nb_antenna_ports_gNB==1 ? 1 : 2;
  }

  //ue->frame_parms.pucch_config_common.deltaPUCCH_Shift = 1;
  ue->dlsch_MCH[0]  = new_nr_ue_dlsch(1,NR_MAX_DLSCH_HARQ_PROCESSES,NSOFT,MAX_LDPC_ITERATIONS_MBSFN,ue->frame_parms.N_RB_DL,0);

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
  //init_scrambling_lut();
  //set_taus_seed(1328);
}
