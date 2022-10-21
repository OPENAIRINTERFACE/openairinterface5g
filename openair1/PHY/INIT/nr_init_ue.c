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
#include "openair2/COMMON/prs_nr_paramdef.h"


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

void RCconfig_nrUE_prs(void *cfg)
{
  int j = 0, k = 0, gNB_id = 0;
  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  char str[7][100] = {'\0'}; int16_t n[7] = {0};
  PHY_VARS_NR_UE *ue  = (PHY_VARS_NR_UE *)cfg;
  prs_config_t *prs_config = NULL;

  paramlist_def_t gParamList = {CONFIG_STRING_PRS_LIST,NULL,0};
  paramdef_t gParams[] = PRS_GLOBAL_PARAMS_DESC;
  config_getlist( &gParamList,gParams,sizeof(gParams)/sizeof(paramdef_t), NULL);
  if (gParamList.numelt > 0)
  {
    ue->prs_active_gNBs = *(gParamList.paramarray[j][PRS_ACTIVE_GNBS_IDX].uptr);
  }
  else
  {
    LOG_E(PHY,"%s configuration NOT found..!! Skipped configuring UE for the PRS reception\n", CONFIG_STRING_PRS_CONFIG);
  }

  paramlist_def_t PRS_ParamList = {{0},NULL,0};
  for(int i = 0; i < ue->prs_active_gNBs; i++)
  {
    paramdef_t PRS_Params[] = PRS_PARAMS_DESC;
    sprintf(PRS_ParamList.listname, "%s%i", CONFIG_STRING_PRS_CONFIG, i);

    sprintf(aprefix, "%s.[%i]", CONFIG_STRING_PRS_LIST, 0);
    config_getlist( &PRS_ParamList,PRS_Params,sizeof(PRS_Params)/sizeof(paramdef_t), aprefix);

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
    }
    else
    {
      LOG_E(PHY,"No %s configuration found..!!\n", PRS_ParamList.listname);
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
    prs_vars[idx]   = (NR_UE_PRS *)malloc16_clear(sizeof(NR_UE_PRS));
    // PRS channel estimates

    for(int k = 0; k < NR_MAX_PRS_RESOURCES_PER_SET; k++)
    {
      prs_vars[idx]->prs_resource[k].prs_meas = (prs_meas_t **)malloc16_clear( fp->nb_antennas_rx*sizeof(prs_meas_t *) );
      AssertFatal((prs_vars[idx]->prs_resource[k].prs_meas!=NULL), "%s: PRS measurements malloc failed for gNB_id %d\n", __FUNCTION__, idx);

      for (int j=0; j<fp->nb_antennas_rx; j++) {
        prs_vars[idx]->prs_resource[k].prs_meas[j] = (prs_meas_t *)malloc16_clear(sizeof(prs_meas_t) );
        AssertFatal((prs_vars[idx]->prs_resource[k].prs_meas[j]!=NULL), "%s: PRS measurements malloc failed for gNB_id %d, rx_ant %d\n", __FUNCTION__, idx, j);
      }
    }
  }

  // load the config file params
  RCconfig_nrUE_prs(ue);

  //PRS sequence init
  ue->nr_gold_prs = (uint32_t *****)malloc16(ue->prs_active_gNBs*sizeof(uint32_t ****));
  uint32_t *****prs = ue->nr_gold_prs;
  AssertFatal(prs!=NULL, "%s: positioning reference signal malloc failed\n", __FUNCTION__);
  for (int gnb = 0; gnb < ue->prs_active_gNBs; gnb++) {
    prs[gnb] = (uint32_t ****)malloc16(ue->prs_vars[gnb]->NumPRSResources*sizeof(uint32_t ***));
    AssertFatal(prs[gnb]!=NULL, "%s: positioning reference signal for gnb %d - malloc failed\n", __FUNCTION__, gnb);

    for (int rsc = 0; rsc < ue->prs_vars[gnb]->NumPRSResources; rsc++) {
      prs[gnb][rsc] = (uint32_t ***)malloc16(fp->slots_per_frame*sizeof(uint32_t **));
      AssertFatal(prs[gnb][rsc]!=NULL, "%s: positioning reference signal for gnb %d rsc %d- malloc failed\n", __FUNCTION__, gnb, rsc);

      for (int slot=0; slot<fp->slots_per_frame; slot++) {
        prs[gnb][rsc][slot] = (uint32_t **)malloc16(fp->symbols_per_slot*sizeof(uint32_t *));
        AssertFatal(prs[gnb][rsc][slot]!=NULL, "%s: positioning reference signal for gnb %d rsc %d slot %d - malloc failed\n", __FUNCTION__, gnb, rsc, slot);

        for (int symb=0; symb<fp->symbols_per_slot; symb++) {
          prs[gnb][rsc][slot][symb] = (uint32_t *)malloc16(NR_MAX_PRS_INIT_LENGTH_DWORD*sizeof(uint32_t));
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
  NR_UE_PBCH  **const pbch_vars          = ue->pbch_vars;
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

    ue->ul_time_alignment[gNB_id].apply_ta = 0;
    ue->ul_time_alignment[gNB_id].ta_frame = -1;
    ue->ul_time_alignment[gNB_id].ta_slot  = -1;
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
  common_vars->rxdataF  = (int32_t **)malloc16( fp->nb_antennas_rx*sizeof(int32_t *) );

  for (i=0; i<fp->nb_antennas_rx; i++) {
    common_vars->rxdata[i] = (int32_t *) malloc16_clear( (2*(fp->samples_per_frame)+fp->ofdm_symbol_size)*sizeof(int32_t) );
    common_vars->rxdataF[i] = (int32_t *)malloc16_clear( sizeof(int32_t)*(fp->samples_per_slot_wCP) );
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
    ue->pdsch_vars[gNB_id] = (NR_UE_PDSCH *)malloc16_clear(sizeof(NR_UE_PDSCH));
    phy_init_nr_ue_PDSCH( ue->pdsch_vars[gNB_id], fp );

    int nb_codewords = NR_MAX_NB_LAYERS > 4 ? 2 : 1;
    for (i=0; i<nb_codewords; i++) {
      ue->pdsch_vars[gNB_id]->llr[i] = (int16_t *)malloc16_clear( (8*(3*8*8448))*sizeof(int16_t) );//Q_m = 8 bits/Sym, Code_Rate=3, Number of Segments =8, Circular Buffer K_cb = 8448
    }
    for (i=0; i<NR_MAX_NB_LAYERS; i++) {
      ue->pdsch_vars[gNB_id]->layer_llr[i] = (int16_t *)malloc16_clear( (8*(3*8*8448))*sizeof(int16_t) );//Q_m = 8 bits/Sym, Code_Rate=3, Number of Segments =8, Circular Buffer K_cb = 8448
    }
  }

  for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {
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
    ue->nr_csi_info = (nr_csi_info_t *)malloc16_clear(sizeof(nr_csi_info_t));
    ue->nr_csi_info->nr_gold_csi_rs = (uint32_t ***)malloc16(fp->slots_per_frame * sizeof(uint32_t **));
    AssertFatal(ue->nr_csi_info->nr_gold_csi_rs != NULL, "NR init: csi reference signal malloc failed\n");
    for (int slot=0; slot<fp->slots_per_frame; slot++) {
      ue->nr_csi_info->nr_gold_csi_rs[slot] = (uint32_t **)malloc16(fp->symbols_per_slot * sizeof(uint32_t *));
      AssertFatal(ue->nr_csi_info->nr_gold_csi_rs[slot] != NULL, "NR init: csi reference signal for slot %d - malloc failed\n", slot);
      for (int symb=0; symb<fp->symbols_per_slot; symb++) {
        ue->nr_csi_info->nr_gold_csi_rs[slot][symb] = (uint32_t *)malloc16(csi_dmrs_init_length * sizeof(uint32_t));
        AssertFatal(ue->nr_csi_info->nr_gold_csi_rs[slot][symb] != NULL, "NR init: csi reference signal for slot %d symbol %d - malloc failed\n", slot, symb);
      }
    }
    ue->nr_csi_info->csi_rs_generated_signal = (int32_t **)malloc16(NR_MAX_NB_PORTS * sizeof(int32_t *) );
    for (i=0; i<NR_MAX_NB_PORTS; i++) {
      ue->nr_csi_info->csi_rs_generated_signal[i] = (int32_t *) malloc16_clear(fp->samples_per_frame_wCP * sizeof(int32_t));
    }

    ue->nr_srs_info = (nr_srs_info_t *)malloc16_clear(sizeof(nr_srs_info_t));

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
    free_and_zero(common_vars->txdata[i]);
    free_and_zero(common_vars->txdataF[i]);
  }

  free_and_zero(common_vars->txdata);
  free_and_zero(common_vars->txdataF);

  for (int i = 0; i < fp->nb_antennas_rx; i++) {
    free_and_zero(common_vars->rxdata[i]);
    free_and_zero(common_vars->rxdataF[i]);
  }
  free_and_zero(common_vars->rxdataF);
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
    free_and_zero(ue->pdsch_vars[gNB_id]->llr_shifts);
    free_and_zero(ue->pdsch_vars[gNB_id]->llr128_2ndstream);
    phy_term_nr_ue__PDSCH(ue->pdsch_vars[gNB_id], fp);
    free_and_zero(ue->pdsch_vars[gNB_id]);
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

    free_and_zero(ue->pbch_vars[gNB_id]);
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
  free_and_zero(ue->sinr_CQI_dB);
}

void term_nr_ue_transport(PHY_VARS_NR_UE *ue)
{
  const int N_RB_DL = ue->frame_parms.N_RB_DL;
  for (int i = 0; i < NUMBER_OF_CONNECTED_gNB_MAX; i++) {
    for (int j = 0; j < 2; j++) {
      free_nr_ue_dlsch(&ue->dlsch[i][j], N_RB_DL);
      if (j==0)
        free_nr_ue_ulsch(&ue->ulsch[i], N_RB_DL, &ue->frame_parms);
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
      AssertFatal((ue->dlsch[i][j]  = new_nr_ue_dlsch(1,NR_MAX_DLSCH_HARQ_PROCESSES,NSOFT,ue->max_ldpc_iterations,ue->frame_parms.N_RB_DL))!=NULL,"Can't get ue dlsch structures\n");
      LOG_D(PHY,"dlsch[%d][%d] => %p\n",i,j,ue->dlsch[i][j]);
      if (j==0) {
        AssertFatal((ue->ulsch[i] = new_nr_ue_ulsch(ue->frame_parms.N_RB_UL, NR_MAX_ULSCH_HARQ_PROCESSES,&ue->frame_parms))!=NULL,"Can't get ue ulsch structures\n");
        LOG_D(PHY,"ulsch[%d] => %p\n",i,ue->ulsch[i]);
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
