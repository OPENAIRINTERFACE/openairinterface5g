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

#include "nr_phy_init.h"
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
#include "openair2/COMMON/prs_nr_paramdef.h"
#include "SCHED_NR_UE/harq_nr.h"

void RCconfig_nrUE_prs(void *cfg)
{
  int j = 0, k = 0, gNB_id = 0;
  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  char str[7][100] = {{'\0'}}; int16_t n[7] = {0};
  PHY_VARS_NR_UE *ue  = (PHY_VARS_NR_UE *)cfg;
  prs_config_t *prs_config = NULL;

  paramlist_def_t gParamList = {CONFIG_STRING_PRS_LIST,NULL,0};
  paramdef_t gParams[] = PRS_GLOBAL_PARAMS_DESC;
  config_getlist(config_get_if(), &gParamList, gParams, sizeofArray(gParams), NULL);
  if (gParamList.numelt > 0)
  {
    ue->prs_active_gNBs = *(gParamList.paramarray[j][PRS_ACTIVE_GNBS_IDX].uptr);
  } else {
    LOG_I(PHY,"%s configuration NOT found..!! Skipped configuring UE for the PRS reception\n", CONFIG_STRING_PRS_CONFIG);
  }

  paramlist_def_t PRS_ParamList = {{0},NULL,0};
  for(int i = 0; i < ue->prs_active_gNBs; i++)
  {
    paramdef_t PRS_Params[] = PRS_PARAMS_DESC;
    sprintf(PRS_ParamList.listname, "%s%i", CONFIG_STRING_PRS_CONFIG, i);

    sprintf(aprefix, "%s.[%i]", CONFIG_STRING_PRS_LIST, 0);
    config_getlist(config_get_if(), &PRS_ParamList, PRS_Params, sizeofArray(PRS_Params), aprefix);

    if (PRS_ParamList.numelt > 0) {
      for (j = 0; j < PRS_ParamList.numelt; j++) {
        gNB_id = *(PRS_ParamList.paramarray[j][PRS_GNB_ID].uptr);
        if(gNB_id != i)  gNB_id = i; // force gNB_id to avoid mismatch

        memset(n,0,sizeof(n));
        ue->prs_vars[gNB_id]->NumPRSResources = *(PRS_ParamList.paramarray[j][NUM_PRS_RESOURCES].uptr);
        for (k = 0; k < ue->prs_vars[gNB_id]->NumPRSResources; k++)
        {
          prs_config = &ue->prs_vars[gNB_id]->prs_resource[k].prs_cfg;
          prs_config->PRSResourceSetPeriod[0]  = PRS_ParamList.paramarray[j][PRS_RESOURCE_SET_PERIOD_LIST].uptr[0];
          prs_config->PRSResourceSetPeriod[1]  = PRS_ParamList.paramarray[j][PRS_RESOURCE_SET_PERIOD_LIST].uptr[1];
          // per PRS resources parameters
          prs_config->SymbolStart              = PRS_ParamList.paramarray[j][PRS_SYMBOL_START_LIST].uptr[k];
          prs_config->NumPRSSymbols            = PRS_ParamList.paramarray[j][PRS_NUM_SYMBOLS_LIST].uptr[k];
          prs_config->REOffset                 = PRS_ParamList.paramarray[j][PRS_RE_OFFSET_LIST].uptr[k];
          prs_config->NPRSID                   = PRS_ParamList.paramarray[j][PRS_ID_LIST].uptr[k];
          prs_config->PRSResourceOffset        = PRS_ParamList.paramarray[j][PRS_RESOURCE_OFFSET_LIST].uptr[k];
          // Common parameters to all PRS resources
          prs_config->NumRB                    = *(PRS_ParamList.paramarray[j][PRS_NUM_RB].uptr);
          prs_config->RBOffset                 = *(PRS_ParamList.paramarray[j][PRS_RB_OFFSET].uptr);
          prs_config->CombSize                 = *(PRS_ParamList.paramarray[j][PRS_COMB_SIZE].uptr);
          prs_config->PRSResourceRepetition    = *(PRS_ParamList.paramarray[j][PRS_RESOURCE_REPETITION].uptr);
          prs_config->PRSResourceTimeGap       = *(PRS_ParamList.paramarray[j][PRS_RESOURCE_TIME_GAP].uptr);

          prs_config->MutingBitRepetition      = *(PRS_ParamList.paramarray[j][PRS_MUTING_BIT_REPETITION].uptr);
          for (int l = 0; l < PRS_ParamList.paramarray[j][PRS_MUTING_PATTERN1_LIST].numelt; l++)
          {
            prs_config->MutingPattern1[l]      = PRS_ParamList.paramarray[j][PRS_MUTING_PATTERN1_LIST].uptr[l];
            if (k == 0) // print only for 0th resource
              n[5] += snprintf(str[5]+n[5],sizeof(str[5]),"%d, ",prs_config->MutingPattern1[l]);
          }
          for (int l = 0; l < PRS_ParamList.paramarray[j][PRS_MUTING_PATTERN2_LIST].numelt; l++)
          {
            prs_config->MutingPattern2[l]      = PRS_ParamList.paramarray[j][PRS_MUTING_PATTERN2_LIST].uptr[l];
            if (k == 0) // print only for 0th resource
              n[6] += snprintf(str[6]+n[6],sizeof(str[6]),"%d, ",prs_config->MutingPattern2[l]);
          }

          // print to buffer
          n[0] += snprintf(str[0]+n[0],sizeof(str[0]),"%d, ",prs_config->SymbolStart);
          n[1] += snprintf(str[1]+n[1],sizeof(str[1]),"%d, ",prs_config->NumPRSSymbols);
          n[2] += snprintf(str[2]+n[2],sizeof(str[2]),"%d, ",prs_config->REOffset);
          n[3] += snprintf(str[3]+n[3],sizeof(str[3]),"%d, ",prs_config->PRSResourceOffset);
          n[4] += snprintf(str[4]+n[4],sizeof(str[4]),"%d, ",prs_config->NPRSID);
        } // for k

        prs_config = &ue->prs_vars[gNB_id]->prs_resource[0].prs_cfg;
        LOG_I(PHY, "-----------------------------------------\n");
        LOG_I(PHY, "PRS Config for gNB_id %d @ %p\n", gNB_id, prs_config);
        LOG_I(PHY, "-----------------------------------------\n");
        LOG_I(PHY, "NumPRSResources \t%d\n", ue->prs_vars[gNB_id]->NumPRSResources);
        LOG_I(PHY, "PRSResourceSetPeriod \t[%d, %d]\n", prs_config->PRSResourceSetPeriod[0], prs_config->PRSResourceSetPeriod[1]);
        LOG_I(PHY, "NumRB \t\t\t%d\n", prs_config->NumRB);
        LOG_I(PHY, "RBOffset \t\t%d\n", prs_config->RBOffset);
        LOG_I(PHY, "CombSize \t\t%d\n", prs_config->CombSize);
        LOG_I(PHY, "PRSResourceRepetition \t%d\n", prs_config->PRSResourceRepetition);
        LOG_I(PHY, "PRSResourceTimeGap \t%d\n", prs_config->PRSResourceTimeGap);
        LOG_I(PHY, "MutingBitRepetition \t%d\n", prs_config->MutingBitRepetition);
        LOG_I(PHY, "SymbolStart \t\t[%s\b\b]\n", str[0]);
        LOG_I(PHY, "NumPRSSymbols \t\t[%s\b\b]\n", str[1]);
        LOG_I(PHY, "REOffset \t\t[%s\b\b]\n", str[2]);
        LOG_I(PHY, "PRSResourceOffset \t[%s\b\b]\n", str[3]);
        LOG_I(PHY, "NPRS_ID \t\t[%s\b\b]\n", str[4]);
        LOG_I(PHY, "MutingPattern1 \t\t[%s\b\b]\n", str[5]);
        LOG_I(PHY, "MutingPattern2 \t\t[%s\b\b]\n", str[6]);
        LOG_I(PHY, "-----------------------------------------\n");
      }
    } else {
      LOG_I(PHY,"No %s configuration found..!!\n", PRS_ParamList.listname);
    }
  }
}

void init_nr_prs_ue_vars(PHY_VARS_NR_UE *ue)
{
  NR_UE_PRS   **const prs_vars = ue->prs_vars;
  NR_DL_FRAME_PARMS *const fp  = &ue->frame_parms;

  // PRS vars init
  for(int idx = 0; idx < NR_MAX_PRS_COMB_SIZE; idx++)
  {
    prs_vars[idx] = malloc16_clear(sizeof(NR_UE_PRS));
    // PRS channel estimates

    for(int k = 0; k < NR_MAX_PRS_RESOURCES_PER_SET; k++)
    {
      prs_vars[idx]->prs_resource[k].prs_meas = malloc16_clear(fp->nb_antennas_rx * sizeof(prs_meas_t *));
      AssertFatal((prs_vars[idx]->prs_resource[k].prs_meas!=NULL), "%s: PRS measurements malloc failed for gNB_id %d\n", __FUNCTION__, idx);

      for (int j=0; j<fp->nb_antennas_rx; j++) {
        prs_vars[idx]->prs_resource[k].prs_meas[j] = malloc16_clear(sizeof(prs_meas_t));
        AssertFatal((prs_vars[idx]->prs_resource[k].prs_meas[j]!=NULL), "%s: PRS measurements malloc failed for gNB_id %d, rx_ant %d\n", __FUNCTION__, idx, j);
      }
    }
  }

  // load the config file params
  RCconfig_nrUE_prs(ue);

  //PRS sequence init
  ue->nr_gold_prs = malloc16(ue->prs_active_gNBs * sizeof(uint32_t ****));
  uint32_t *****prs = ue->nr_gold_prs;
  AssertFatal(prs!=NULL, "%s: positioning reference signal malloc failed\n", __FUNCTION__);
  for (int gnb = 0; gnb < ue->prs_active_gNBs; gnb++) {
    prs[gnb] = malloc16(ue->prs_vars[gnb]->NumPRSResources * sizeof(uint32_t ***));
    AssertFatal(prs[gnb]!=NULL, "%s: positioning reference signal for gnb %d - malloc failed\n", __FUNCTION__, gnb);

    for (int rsc = 0; rsc < ue->prs_vars[gnb]->NumPRSResources; rsc++) {
      prs[gnb][rsc] = malloc16(fp->slots_per_frame * sizeof(uint32_t **));
      AssertFatal(prs[gnb][rsc]!=NULL, "%s: positioning reference signal for gnb %d rsc %d- malloc failed\n", __FUNCTION__, gnb, rsc);

      for (int slot=0; slot<fp->slots_per_frame; slot++) {
        prs[gnb][rsc][slot] = malloc16(fp->symbols_per_slot * sizeof(uint32_t *));
        AssertFatal(prs[gnb][rsc][slot]!=NULL, "%s: positioning reference signal for gnb %d rsc %d slot %d - malloc failed\n", __FUNCTION__, gnb, rsc, slot);

        for (int symb=0; symb<fp->symbols_per_slot; symb++) {
          prs[gnb][rsc][slot][symb] = malloc16(NR_MAX_PRS_INIT_LENGTH_DWORD * sizeof(uint32_t));
          AssertFatal(prs[gnb][rsc][slot][symb]!=NULL, "%s: positioning reference signal for gnb %d rsc %d slot %d symbol %d - malloc failed\n", __FUNCTION__, gnb, rsc, slot, symb);
        } // for symb
      } // for slot
    } // for rsc
  } // for gnb

  init_nr_gold_prs(ue);
}

int init_nr_ue_signal(PHY_VARS_NR_UE *ue, int nb_connected_gNB)
{
  // create shortcuts
  NR_DL_FRAME_PARMS *const fp            = &ue->frame_parms;
  NR_UE_COMMON *const common_vars        = &ue->common_vars;
  NR_UE_PRACH **const prach_vars         = ue->prach_vars;
  NR_UE_CSI_IM **const csiim_vars        = ue->csiim_vars;
  NR_UE_CSI_RS **const csirs_vars        = ue->csirs_vars;
  NR_UE_SRS **const srs_vars             = ue->srs_vars;

  int i, slot, symb, gNB_id;

  LOG_I(PHY, "Initializing UE vars for gNB TXant %u, UE RXant %u\n", fp->nb_antennas_tx, fp->nb_antennas_rx);

  phy_init_nr_top(ue);
  // many memory allocation sizes are hard coded
  AssertFatal( fp->nb_antennas_rx <= 4, "hard coded allocation for ue_common_vars->dl_ch_estimates[gNB_id]" );
  AssertFatal( nb_connected_gNB <= NUMBER_OF_CONNECTED_gNB_MAX, "n_connected_gNB is too large" );
  // init phy_vars_ue

  for (i=0; i<fp->Lmax; i++)
    ue->measurements.ssb_rsrp_dBm[i] = INT_MIN;

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

  }
  // init NR modulation lookup tables
  nr_generate_modulation_table();

  ///////////
  ////////////////////////////////////////////////////////////////////////////////////////////

  ///////////////////////// PRS init /////////////////////////
  ///////////

  init_nr_prs_ue_vars(ue);

  ///////////
  ////////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////PUSCH DMRS init/////////////////////////
  ///////////

  // ceil(((NB_RB*6(k)*2(QPSK)/32) // 3 RE *2(QPSK)
  int pusch_dmrs_init_length =  ((fp->N_RB_UL*12)>>5)+1;
  ue->nr_gold_pusch_dmrs = malloc16(fp->slots_per_frame * sizeof(uint32_t ***));
  uint32_t ****pusch_dmrs = ue->nr_gold_pusch_dmrs;

  for (slot=0; slot<fp->slots_per_frame; slot++) {
    pusch_dmrs[slot] = malloc16(fp->symbols_per_slot * sizeof(uint32_t **));
    AssertFatal(pusch_dmrs[slot]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d - malloc failed\n", slot);

    for (symb=0; symb<fp->symbols_per_slot; symb++) {
      pusch_dmrs[slot][symb] = malloc16(NR_NB_NSCID * sizeof(uint32_t *));
      AssertFatal(pusch_dmrs[slot][symb]!=NULL, "init_nr_ue_signal: pusch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);

      for (int q=0; q<NR_NB_NSCID; q++) {
        pusch_dmrs[slot][symb][q] = malloc16(pusch_dmrs_init_length * sizeof(uint32_t));
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
  common_vars->txData = malloc16(fp->nb_antennas_tx * sizeof(c16_t *));

  for (i=0; i<fp->nb_antennas_tx; i++) {
    common_vars->txData[i] = malloc16_clear((fp->samples_per_frame) * sizeof(c16_t));
  }

  // init RX buffers
  common_vars->rxdata = malloc16(fp->nb_antennas_rx * sizeof(c16_t *));

  for (i=0; i<fp->nb_antennas_rx; i++) {
    common_vars->rxdata[i] = malloc16_clear((2 * (fp->samples_per_frame) + fp->ofdm_symbol_size) * sizeof(c16_t));
  }

  // ceil(((NB_RB<<1)*3)/32) // 3 RE *2(QPSK)
  int pdcch_dmrs_init_length =  (((fp->N_RB_DL<<1)*3)>>5)+1;
  //PDCCH DMRS init (gNB offset = 0)
  ue->nr_gold_pdcch[0] = malloc16(fp->slots_per_frame * sizeof(uint32_t **));
  uint32_t ***pdcch_dmrs = ue->nr_gold_pdcch[0];
  AssertFatal(pdcch_dmrs!=NULL, "NR init: pdcch_dmrs malloc failed\n");

  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    pdcch_dmrs[slot] = malloc16(fp->symbols_per_slot * sizeof(uint32_t *));
    AssertFatal(pdcch_dmrs[slot]!=NULL, "NR init: pdcch_dmrs for slot %d - malloc failed\n", slot);

    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      pdcch_dmrs[slot][symb] = malloc16(pdcch_dmrs_init_length * sizeof(uint32_t));
      AssertFatal(pdcch_dmrs[slot][symb]!=NULL, "NR init: pdcch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);
    }
  }

  // ceil(((NB_RB*6(k)*2(QPSK)/32) // 3 RE *2(QPSK)
  int pdsch_dmrs_init_length =  ((fp->N_RB_DL*12)>>5)+1;

  //PDSCH DMRS init (eNB offset = 0)
  ue->nr_gold_pdsch[0] = malloc16(fp->slots_per_frame * sizeof(uint32_t ***));
  uint32_t ****pdsch_dmrs = ue->nr_gold_pdsch[0];

  for (int slot=0; slot<fp->slots_per_frame; slot++) {
    pdsch_dmrs[slot] = malloc16(fp->symbols_per_slot * sizeof(uint32_t **));
    AssertFatal(pdsch_dmrs[slot]!=NULL, "NR init: pdsch_dmrs for slot %d - malloc failed\n", slot);

    for (int symb=0; symb<fp->symbols_per_slot; symb++) {
      pdsch_dmrs[slot][symb] = malloc16(NR_NB_NSCID * sizeof(uint32_t *));
      AssertFatal(pdsch_dmrs[slot][symb]!=NULL, "NR init: pdsch_dmrs for slot %d symbol %d - malloc failed\n", slot, symb);

      for (int q=0; q<NR_NB_NSCID; q++) {
        pdsch_dmrs[slot][symb][q] = malloc16(pdsch_dmrs_init_length * sizeof(uint32_t));
        AssertFatal(pdsch_dmrs[slot][symb][q]!=NULL, "NR init: pdsch_dmrs for slot %d symbol %d nscid %d - malloc failed\n", slot, symb, q);
      }
    }
  }

  // DLSCH
  for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {
    prach_vars[gNB_id] = malloc16_clear(sizeof(NR_UE_PRACH));
    csiim_vars[gNB_id] = malloc16_clear(sizeof(NR_UE_CSI_IM));
    csirs_vars[gNB_id] = malloc16_clear(sizeof(NR_UE_CSI_RS));
    srs_vars[gNB_id] = malloc16_clear(sizeof(NR_UE_SRS));

    csiim_vars[gNB_id]->active = false;
    csirs_vars[gNB_id]->active = false;
    srs_vars[gNB_id]->active = false;

    // ceil((NB_RB*8(max allocation per RB)*2(QPSK))/32)
    int csi_dmrs_init_length =  ((fp->N_RB_DL<<4)>>5)+1;
    ue->nr_csi_info = malloc16_clear(sizeof(nr_csi_info_t));
    ue->nr_csi_info->nr_gold_csi_rs = malloc16(fp->slots_per_frame * sizeof(uint32_t **));
    AssertFatal(ue->nr_csi_info->nr_gold_csi_rs != NULL, "NR init: csi reference signal malloc failed\n");
    for (int slot=0; slot<fp->slots_per_frame; slot++) {
      ue->nr_csi_info->nr_gold_csi_rs[slot] = malloc16(fp->symbols_per_slot * sizeof(uint32_t *));
      AssertFatal(ue->nr_csi_info->nr_gold_csi_rs[slot] != NULL, "NR init: csi reference signal for slot %d - malloc failed\n", slot);
      for (int symb=0; symb<fp->symbols_per_slot; symb++) {
        ue->nr_csi_info->nr_gold_csi_rs[slot][symb] = malloc16(csi_dmrs_init_length * sizeof(uint32_t));
        AssertFatal(ue->nr_csi_info->nr_gold_csi_rs[slot][symb] != NULL, "NR init: csi reference signal for slot %d symbol %d - malloc failed\n", slot, symb);
      }
    }
    ue->nr_csi_info->csi_rs_generated_signal = malloc16(NR_MAX_NB_PORTS * sizeof(int32_t *));
    for (i=0; i<NR_MAX_NB_PORTS; i++) {
      ue->nr_csi_info->csi_rs_generated_signal[i] = malloc16_clear(fp->samples_per_frame_wCP * sizeof(int32_t));
    }

    ue->nr_srs_info = malloc16_clear(sizeof(nr_srs_info_t));
  }

  ue->init_averaging = 1;

  // enable MIB/SIB decoding by default
  ue->decode_MIB = 1;
  ue->decode_SIB = 1;

  init_nr_prach_tables(839);
  init_symbol_rotation(fp);
  init_timeshift_rotation(fp);

  return 0;
}

void term_nr_ue_signal(PHY_VARS_NR_UE *ue, int nb_connected_gNB)
{
  const NR_DL_FRAME_PARMS* fp = &ue->frame_parms;
  phy_term_nr_top();

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
    free_and_zero(common_vars->txData[i]);
  }

  free_and_zero(common_vars->txData);

  for (int i = 0; i < fp->nb_antennas_rx; i++) {
    free_and_zero(common_vars->rxdata[i]);
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
  }

  for (int gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {

    for (int i=0; i<NR_MAX_NB_PORTS; i++) {
      free_and_zero(ue->nr_csi_info->csi_rs_generated_signal[i]);
    }
    free_and_zero(ue->nr_csi_info->csi_rs_generated_signal);
    for (int slot=0; slot<fp->slots_per_frame; slot++) {
      for (int symb=0; symb<fp->symbols_per_slot; symb++) {
        free_and_zero(ue->nr_csi_info->nr_gold_csi_rs[slot][symb]);
      }
      free_and_zero(ue->nr_csi_info->nr_gold_csi_rs[slot]);
    }
    free_and_zero(ue->nr_csi_info->nr_gold_csi_rs);
    free_and_zero(ue->nr_csi_info);

    free_and_zero(ue->nr_srs_info);

    free_and_zero(ue->csiim_vars[gNB_id]);
    free_and_zero(ue->csirs_vars[gNB_id]);
    free_and_zero(ue->srs_vars[gNB_id]);

    free_and_zero(ue->prach_vars[gNB_id]);
  }

  for (int gnb = 0; gnb < ue->prs_active_gNBs; gnb++)
  {
    for (int rsc = 0; rsc < ue->prs_vars[gnb]->NumPRSResources; rsc++)
    {
      for (int slot=0; slot<fp->slots_per_frame; slot++)
      {
        for (int symb=0; symb<fp->symbols_per_slot; symb++)
        {
          free_and_zero(ue->nr_gold_prs[gnb][rsc][slot][symb]);
        }
        free_and_zero(ue->nr_gold_prs[gnb][rsc][slot]);
      }
      free_and_zero(ue->nr_gold_prs[gnb][rsc]);
    }
    free_and_zero(ue->nr_gold_prs[gnb]);
  }
  free_and_zero(ue->nr_gold_prs);

  for(int idx = 0; idx < NR_MAX_PRS_COMB_SIZE; idx++)
  {
    for(int k = 0; k < NR_MAX_PRS_RESOURCES_PER_SET; k++)
    {
      for (int j=0; j<fp->nb_antennas_rx; j++)
      {
        free_and_zero(ue->prs_vars[idx]->prs_resource[k].prs_meas[j]);
      }
      free_and_zero(ue->prs_vars[idx]->prs_resource[k].prs_meas);
    }

    free_and_zero(ue->prs_vars[idx]);
  }
}

void free_nr_ue_dl_harq(NR_DL_UE_HARQ_t harq_list[2][NR_MAX_DLSCH_HARQ_PROCESSES], int number_of_processes, int num_rb) {

  uint16_t a_segments = MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER*NR_MAX_NB_LAYERS;
  if (num_rb != 273) {
    a_segments = a_segments*num_rb;
    a_segments = (a_segments/273)+1;
  }

  for (int j=0; j < 2; j++) {
    for (int i=0; i<number_of_processes; i++) {

      for (int r=0; r<a_segments; r++) {
        free_and_zero(harq_list[j][i].c[r]);
        free_and_zero(harq_list[j][i].d[r]);
      }
      free_and_zero(harq_list[j][i].c);
      free_and_zero(harq_list[j][i].d);
    }
  }
}

void free_nr_ue_ul_harq(NR_UL_UE_HARQ_t harq_list[NR_MAX_ULSCH_HARQ_PROCESSES], int number_of_processes, int num_rb, int num_ant_tx) {

  int max_layers = (num_ant_tx < NR_MAX_NB_LAYERS) ? num_ant_tx : NR_MAX_NB_LAYERS;
  uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER*max_layers;  //number of segments to be allocated

  if (num_rb != 273) {
    a_segments = a_segments*num_rb;
    a_segments = a_segments/273 +1;
  }

  for (int i = 0; i < number_of_processes; i++) {
    free_and_zero(harq_list[i].a);
    free_and_zero(harq_list[i].b);
    for (int r = 0; r < a_segments; r++) {
      free_and_zero(harq_list[i].c[r]);
      free_and_zero(harq_list[i].d[r]);
    }
    free_and_zero(harq_list[i].c);
    free_and_zero(harq_list[i].d);
    free_and_zero(harq_list[i].e);
    free_and_zero(harq_list[i].f);
  }
}

void term_nr_ue_transport(PHY_VARS_NR_UE *ue)
{
  const int N_RB_DL = ue->frame_parms.N_RB_DL;
  const int N_RB_UL = ue->frame_parms.N_RB_UL;
  free_nr_ue_dl_harq(ue->dl_harq_processes, NR_MAX_DLSCH_HARQ_PROCESSES, N_RB_DL);
  free_nr_ue_ul_harq(ue->ul_harq_processes, NR_MAX_ULSCH_HARQ_PROCESSES, N_RB_UL, ue->frame_parms.nb_antennas_tx);
}

void nr_init_dl_harq_processes(NR_DL_UE_HARQ_t harq_list[2][NR_MAX_DLSCH_HARQ_PROCESSES], int number_of_processes, int num_rb) {

  int a_segments = MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER*NR_MAX_NB_LAYERS;  //number of segments to be allocated
  if (num_rb != 273) {
    a_segments = a_segments*num_rb;
    a_segments = (a_segments/273)+1;
  }

  for (int j=0; j<2; j++) {
    for (int i=0; i<number_of_processes; i++) {
      memset(harq_list[j] + i, 0, sizeof(NR_DL_UE_HARQ_t));
      init_downlink_harq_status(harq_list[j] + i);

      harq_list[j][i].c = malloc16(a_segments*sizeof(uint8_t *));
      harq_list[j][i].d = malloc16(a_segments*sizeof(int16_t *));
      const int sz=5*8448*sizeof(int16_t);
      init_abort(&harq_list[j][i].abort_decode);
      for (int r=0; r<a_segments; r++) {
        harq_list[j][i].c[r] = malloc16_clear(1056);
        harq_list[j][i].d[r] = malloc16_clear(sz);
      }
      harq_list[j][i].status  = 0;
      harq_list[j][i].DLround = 0;
    }
  }
}

void nr_init_ul_harq_processes(NR_UL_UE_HARQ_t harq_list[NR_MAX_ULSCH_HARQ_PROCESSES], int number_of_processes, int num_rb, int num_ant_tx) {

  int max_layers = (num_ant_tx < NR_MAX_NB_LAYERS) ? num_ant_tx : NR_MAX_NB_LAYERS;
  uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER*max_layers;  //number of segments to be allocated

  if (num_rb != 273) {
    a_segments = a_segments*num_rb;
    a_segments = a_segments/273 +1;
  }

  uint32_t ulsch_bytes = a_segments*1056;  // allocated bytes per segment

  for (int i = 0; i < number_of_processes; i++) {

    memset(harq_list + i, 0, sizeof(NR_UL_UE_HARQ_t));

    harq_list[i].a = malloc16(ulsch_bytes);
    DevAssert(harq_list[i].a);
    bzero(harq_list[i].a, ulsch_bytes);

    harq_list[i].b = malloc16(ulsch_bytes);
    DevAssert(harq_list[i].b);
    bzero(harq_list[i].b, ulsch_bytes);

    harq_list[i].c = malloc16(a_segments*sizeof(uint8_t *));
    harq_list[i].d = malloc16(a_segments*sizeof(uint16_t *));
    for (int r = 0; r < a_segments; r++) {
      harq_list[i].c[r] = malloc16(8448);
      DevAssert(harq_list[i].c[r]);
      bzero(harq_list[i].c[r],8448);

      harq_list[i].d[r] = malloc16(68*384); //max size for coded output
      DevAssert(harq_list[i].d[r]);
      bzero(harq_list[i].d[r],(68*384));
    }

    harq_list[i].e = malloc16(14*num_rb*12*16);
    DevAssert(harq_list[i].e);
    bzero(harq_list[i].e,14*num_rb*12*16);

    harq_list[i].f = malloc16(14*num_rb*12*16);
    DevAssert(harq_list[i].f);
    bzero(harq_list[i].f,14*num_rb*12*16);

    harq_list[i].round = 0;
  }
}

void init_nr_ue_transport(PHY_VARS_NR_UE *ue) {

  nr_init_dl_harq_processes(ue->dl_harq_processes, NR_MAX_DLSCH_HARQ_PROCESSES, ue->frame_parms.N_RB_DL);
  nr_init_ul_harq_processes(ue->ul_harq_processes, NR_MAX_ULSCH_HARQ_PROCESSES, ue->frame_parms.N_RB_UL, ue->frame_parms.nb_antennas_tx);

  for(int i=0; i<5; i++)
    ue->dl_stats[i] = 0;
}

void clean_UE_harq(PHY_VARS_NR_UE *UE)
{
  for (int harq_pid = 0; harq_pid < NR_MAX_DLSCH_HARQ_PROCESSES; harq_pid++) {
    for (int i = 0; i < 2; i++) {
      NR_DL_UE_HARQ_t *dl_harq_process = &UE->dl_harq_processes[i][harq_pid];
      init_downlink_harq_status(dl_harq_process);
    }
  }
  for (int harq_pid = 0; harq_pid < NR_MAX_ULSCH_HARQ_PROCESSES; harq_pid++) {
    NR_UL_UE_HARQ_t *ul_harq_process = &UE->ul_harq_processes[harq_pid];
    ul_harq_process->tx_status = NEW_TRANSMISSION_HARQ;
    ul_harq_process->status = SCH_IDLE;
    ul_harq_process->round = 0;
  }
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
    ue->ta_frame = -1;
    ue->ta_slot = -1;

    LOG_I(PHY,"UE %d Setting N_TA_offset to %d samples (factor %f, UL Freq %lu, N_RB %d, mu %d)\n", ue->Mod_id, ue->N_TA_offset, factor, fp->ul_CarrierFreq, fp->N_RB_DL, fp->numerology_index);
  }
}

void phy_init_nr_top(PHY_VARS_NR_UE *ue) {
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  init_delay_table(frame_parms->ofdm_symbol_size, MAX_DELAY_COMP, NR_MAX_OFDM_SYMBOL_SIZE, frame_parms->delay_table);
  crcTableInit();
  init_scrambling_luts();
  load_dftslib();
  init_context_synchro_nr(frame_parms);
  generate_ul_reference_signal_sequences(SHRT_MAX);
}

void phy_term_nr_top(void)
{
  free_ul_reference_signal_sequences();
  free_context_synchro_nr();
}
