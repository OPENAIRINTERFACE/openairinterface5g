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

/*! \file gNB_scheduler_primitives.c
 * \brief primitives used by gNB for BCH, RACH, ULSCH, DLSCH scheduling
 * \author  Raymond Knopp, Guy De Souza
 * \date 2018, 2019
 * \email: knopp@eurecom.fr, desouza@eurecom.fr
 * \version 1.0
 * \company Eurecom
 * @ingroup _mac

 */

#include <softmodem-common.h>
#include "assertions.h"

#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"

#include "NR_MAC_gNB/mac_proto.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/nr/nr_common.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LTE/rrc_extern.h"
#include "RRC/NR/nr_rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

#include "intertask_interface.h"

#include "T.h"
#include "NR_PDCCH-ConfigCommon.h"
#include "NR_ControlResourceSet.h"
#include "NR_SearchSpace.h"

#include "nfapi_nr_interface.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_gNB_SCHEDULER 1

#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;

  // Note the 2 scs values in the table names represent resp. scs_common and pdcch_scs
/// LUT for the number of symbols in the coreset indexed by coreset index (4 MSB rmsi_pdcch_config)
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_15_15[15] = {2,2,2,3,3,3,1,1,2,2,3,3,1,2,3};
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_15_30[14] = {2,2,2,2,3,3,3,3,1,1,2,2,3,3};
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_30_15_b40Mhz[9] = {1,1,2,2,3,3,1,2,3};
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_30_15_a40Mhz[9] = {1,2,3,1,1,2,2,3,3};
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_30_30_b40Mhz[16] = {2,2,2,2,2,3,3,3,3,3,1,1,1,2,2,2}; // below 40Mhz bw
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_30_30_a40Mhz[10] = {2,2,3,3,1,1,2,2,3,3}; // above 40Mhz bw
uint8_t nr_coreset_nsymb_pdcch_type_0_scs_120_60[12] = {1,1,2,2,3,3,1,2,1,1,1,1};

/// LUT for the number of RBs in the coreset indexed by coreset index
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_15_15[15] = {0,2,4,0,2,4,12,16,12,16,12,16,38,38,38};
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_15_30[14] = {5,6,7,8,5,6,7,8,18,20,18,20,18,20};
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_30_15_b40Mhz[9] = {2,6,2,6,2,6,28,28,28};
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_30_15_a40Mhz[9] = {4,4,4,0,56,0,56,0,56};
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_30_30_b40Mhz[16] = {0,1,2,3,4,0,1,2,3,4,12,14,16,12,14,16};
uint8_t nr_coreset_rb_offset_pdcch_type_0_scs_30_30_a40Mhz[10] = {0,4,0,4,0,28,0,28,0,28};
int8_t  nr_coreset_rb_offset_pdcch_type_0_scs_120_60[12] = {0,8,0,8,0,8,28,28,-1,49,-1,97};
int8_t  nr_coreset_rb_offset_pdcch_type_0_scs_120_120[8] = {0,4,14,14,-1,24,-1,48};
int8_t  nr_coreset_rb_offset_pdcch_type_0_scs_240_120[8] = {0,8,0,8,-1,25,-1,49};

/// LUT for monitoring occasions param O indexed by ss index (4 LSB rmsi_pdcch_config)
  // Note: scaling is used to avoid decimal values for O and M, original values commented
uint8_t nr_ss_param_O_type_0_mux1_FR1[16] = {0,0,2,2,5,5,7,7,0,5,0,0,2,2,5,5};
uint8_t nr_ss_param_O_type_0_mux1_FR2[14] = {0,0,5,5,5,5,0,5,5,15,15,15,0,5}; //{0,0,2.5,2.5,5,5,0,2.5,5,7.5,7.5,7.5,0,5}
uint8_t nr_ss_scale_O_mux1_FR2[14] = {0,0,1,1,0,0,0,1,0,1,1,1,0,0};

/// LUT for number of SS sets per slot indexed by ss index
uint8_t nr_ss_sets_per_slot_type_0_FR1[16] = {1,2,1,2,1,2,1,2,1,1,1,1,1,1,1,1};
uint8_t nr_ss_sets_per_slot_type_0_FR2[14] = {1,2,1,2,1,2,2,2,2,1,2,2,1,1};

/// LUT for monitoring occasions param M indexed by ss index
uint8_t nr_ss_param_M_type_0_mux1_FR1[16] = {1,1,1,1,1,1,1,1,2,2,1,1,1,1,1,1}; //{1,0.5,1,0.5,1,0.5,1,0.5,2,2,1,1,1,1,1,1}
uint8_t nr_ss_scale_M_mux1_FR1[16] = {0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0};
uint8_t nr_ss_param_M_type_0_mux1_FR2[14] = {1,1,1,1,1,1,1,1,1,1,1,1,2,2}; //{1,0.5,1,0.5,1,0.5,0.5,0.5,0.5,1,0.5,0.5,2,2}
uint8_t nr_ss_scale_M_mux1_FR2[14] = {0,1,0,1,0,1,1,1,1,0,1,1,0,0};

/// LUT for SS first symbol index indexed by ss index
uint8_t nr_ss_first_symb_idx_type_0_mux1_FR1[8] = {0,0,1,2,1,2,1,2};
  // Mux pattern type 2
uint8_t nr_ss_first_symb_idx_scs_120_60_mux2[4] = {0,1,6,7};
uint8_t nr_ss_first_symb_idx_scs_240_120_set1_mux2[6] = {0,1,2,3,0,1};
  // Mux pattern type 3
uint8_t nr_ss_first_symb_idx_scs_120_120_mux3[4] = {4,8,2,6};

/// Search space max values indexed by scs
uint8_t nr_max_number_of_candidates_per_slot[4] = {44, 36, 22, 20};
uint8_t nr_max_number_of_cces_per_slot[4] = {56, 56, 48, 32};

// CQI TABLES
// Table 1 (38.214 5.2.2.1-2)
uint16_t cqi_table1[16][2] = {{0,0},{2,78},{2,120},{2,193},{2,308},{2,449},{2,602},{4,378},
                              {4,490},{4,616},{6,466},{6,567},{6,666},{6,772},{6,873},{6,948}};

// Table 2 (38.214 5.2.2.1-3)
uint16_t cqi_table2[16][2] = {{0,0},{2,78},{2,193},{2,449},{4,378},{4,490},{4,616},{6,466},
                              {6,567},{6,666},{6,772},{6,873},{8,711},{8,797},{8,885},{8,948}};

// Table 2 (38.214 5.2.2.1-4)
uint16_t cqi_table3[16][2] = {{0,0},{2,30},{2,50},{2,78},{2,120},{2,193},{2,308},{2,449},
                              {2,602},{4,378},{4,490},{4,616},{6,466},{6,567},{6,666},{6,772}};


static inline uint8_t get_max_candidates(uint8_t scs) {
  AssertFatal(scs<4, "Invalid PDCCH subcarrier spacing %d\n", scs);
  return (nr_max_number_of_candidates_per_slot[scs]);
}

static inline uint8_t get_max_cces(uint8_t scs) {
  AssertFatal(scs<4, "Invalid PDCCH subcarrier spacing %d\n", scs);
  return (nr_max_number_of_cces_per_slot[scs]);
}

uint8_t set_dl_nrOfLayers(NR_UE_sched_ctrl_t *sched_ctrl) {

  // TODO check this but it should be enough for now
  // if there is not csi report RI is 0 from initialization
  return (sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.ri + 1);

}

void set_dl_mcs(NR_sched_pdsch_t *sched_pdsch,
                NR_UE_sched_ctrl_t *sched_ctrl,
                uint8_t *target_mcs,
                uint8_t mcs_table_idx) {

  if (sched_ctrl->set_mcs) {
    // TODO for wideband case and multiple TB
    int cqi_idx = sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.wb_cqi_1tb;
    uint16_t target_coderate,target_qm;
    if (cqi_idx>0) {
      int cqi_table = sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.cqi_table;
      AssertFatal(cqi_table == mcs_table_idx, "Indices of MCS tables don't correspond\n");
      switch (cqi_table) {
        case 0:
          target_qm = cqi_table1[cqi_idx][0];
          target_coderate = cqi_table1[cqi_idx][1];
          break;
        case 1:
          target_qm = cqi_table2[cqi_idx][0];
          target_coderate = cqi_table2[cqi_idx][1];
          break;
        case 2:
          target_qm = cqi_table3[cqi_idx][0];
          target_coderate = cqi_table3[cqi_idx][1];
          break;
        default:
          AssertFatal(1==0,"Invalid cqi table index %d\n",cqi_table);
      }
      int max_mcs = 28;
      int R,Qm;
      if (mcs_table_idx == 1)
        max_mcs = 27;
      for (int i=0; i<=max_mcs; i++) {
        R = nr_get_code_rate_dl(i, mcs_table_idx);
        Qm = nr_get_Qm_dl(i, mcs_table_idx);
        if ((Qm == target_qm) && (target_coderate <= R)) {
          *target_mcs = i;
          break;
        }
      }
    }
    sched_ctrl->set_mcs = FALSE;
  }
}

void set_dl_dmrs_ports(NR_pdsch_semi_static_t *ps) {

  //TODO first basic implementation of dmrs port selection
  //     only vaild for a single codeword
  //     for now it assumes a selection of Nl consecutive dmrs ports
  //     and a single front loaded symbol
  //     dmrs_ports_id is the index of Tables 7.3.1.2.2-1/2/3/4
  //     number of front loaded symbols need to be consistent with maxLength
  //     when a more complete implementation is done

  switch (ps->nrOfLayers) {
    case 1:
      ps->dmrs_ports_id = 0;
      ps->numDmrsCdmGrpsNoData = 1;
      ps->frontloaded_symb = 1;
      break;
    case 2:
      ps->dmrs_ports_id = 2;
      ps->numDmrsCdmGrpsNoData = 1;
      ps->frontloaded_symb = 1;
      break;
    case 3:
      ps->dmrs_ports_id = 9;
      ps->numDmrsCdmGrpsNoData = 2;
      ps->frontloaded_symb = 1;
      break;
    case 4:
      ps->dmrs_ports_id = 10;
      ps->numDmrsCdmGrpsNoData = 2;
      ps->frontloaded_symb = 1;
      break;
    default:
      AssertFatal(1==0,"Number of layers %d\n not supported or not valid\n",ps->nrOfLayers);
  }
}

NR_ControlResourceSet_t *get_coreset(module_id_t module_idP,
                                     NR_ServingCellConfigCommon_t *scc,
                                     void *bwp,
                                     NR_SearchSpace_t *ss,
                                     NR_SearchSpace__searchSpaceType_PR ss_type) {
  NR_ControlResourceSetId_t coreset_id = *ss->controlResourceSetId;
  if (ss_type == NR_SearchSpace__searchSpaceType_PR_common) { // common search space
    NR_ControlResourceSet_t *coreset;
    if(coreset_id == 0) {
      coreset =  RC.nrmac[module_idP]->sched_ctrlCommon->coreset; // this is coreset 0
    } else if (bwp) {
      coreset = ((NR_BWP_Downlink_t*)bwp)->bwp_Common->pdcch_ConfigCommon->choice.setup->commonControlResourceSet;
    } else if (scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonControlResourceSet) {
      coreset = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonControlResourceSet;
    } else {
      coreset = NULL;
    }

    if (coreset) AssertFatal(coreset_id == coreset->controlResourceSetId,
			     "ID of common ss coreset does not correspond to id set in the "
			     "search space\n");
    return coreset;
  } else {
    const int n = ((NR_BWP_DownlinkDedicated_t*)bwp)->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.count;
    for (int i = 0; i < n; i++) {
      NR_ControlResourceSet_t *coreset =
          ((NR_BWP_DownlinkDedicated_t*)bwp)->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.array[i];
      if (coreset_id == coreset->controlResourceSetId) {
        return coreset;
      }
    }
    AssertFatal(0, "Couldn't find coreset with id %ld\n", coreset_id);
  }
}

NR_SearchSpace_t *get_searchspace(NR_ServingCellConfigCommon_t *scc,
				  NR_BWP_DownlinkDedicated_t *bwp_Dedicated,
				  NR_SearchSpace__searchSpaceType_PR target_ss) {

  const int n = bwp_Dedicated ?
    bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count:
    scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList->list.count;
  for (int i=0;i<n;i++) {
    NR_SearchSpace_t *ss = bwp_Dedicated ?
      bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.array[i]:
      scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList->list.array[i];
    AssertFatal(ss->controlResourceSetId != NULL, "ss->controlResourceSetId is null\n");
    AssertFatal(ss->searchSpaceType != NULL, "ss->searchSpaceType is null\n");
    if (ss->searchSpaceType->present == target_ss) {
      return ss;
    }
  }
  AssertFatal(0, "Couldn't find an adequate searchspace bwp_Dedicated %p\n",bwp_Dedicated);
}

int allocate_nr_CCEs(gNB_MAC_INST *nr_mac,
                     NR_BWP_Downlink_t *bwp,
                     NR_ControlResourceSet_t *coreset,
                     int aggregation,
                     uint16_t Y,
                     int m,
                     int nr_of_candidates) {

  int coreset_id = coreset->controlResourceSetId;

  int *cce_list;
  if(bwp == NULL) {
    cce_list = nr_mac->cce_list[0][coreset_id];
  } else {
    cce_list = nr_mac->cce_list[bwp->bwp_Id][coreset_id];
  }

  int n_rb=0;
  for (int i=0;i<6;i++)
    for (int j=0;j<8;j++) {
      n_rb+=((coreset->frequencyDomainResources.buf[i]>>j)&1);
    }
  n_rb*=6;

  uint16_t N_reg = n_rb * coreset->duration;
  uint16_t n_CI = 0;

  const uint16_t N_cce = N_reg / NR_NB_REG_PER_CCE;
  const uint16_t M_s_max = nr_of_candidates;

  LOG_D(PHY,"allocate_NR_CCes : bwp_id %d, coreset_id %d : N_cce %d, m %d, nr_of_candidates %d, Y %d\n",
        (int)(bwp ? bwp->bwp_Id : 0),coreset_id,N_cce,m,nr_of_candidates, Y);
  //PDCCH candidate index m in CORESET exceeds the maximum number of PDCCH candidates
  if(m >= nr_of_candidates)
    return -1;

  int first_cce = aggregation * (( Y + CEILIDIV((m*N_cce),(aggregation*M_s_max)) + n_CI ) % CEILIDIV(N_cce,aggregation));

  for (int i=0;i<aggregation;i++)
    if (cce_list[first_cce+i] != 0) return(-1);
  
  for (int i=0;i<aggregation;i++) cce_list[first_cce+i] = 1;

  return(first_cce);

}

bool nr_find_nb_rb(uint16_t Qm,
                   uint16_t R,
                   uint8_t nrOfLayers,
                   uint16_t nb_symb_sch,
                   uint16_t nb_dmrs_prb,
                   uint32_t bytes,
                   uint16_t nb_rb_max,
                   uint32_t *tbs,
                   uint16_t *nb_rb)
{
  /* is the maximum (not even) enough? */
  *nb_rb = nb_rb_max;
  *tbs = nr_compute_tbs(Qm, R, *nb_rb, nb_symb_sch, nb_dmrs_prb, 0, 0, nrOfLayers) >> 3;
  /* check whether it does not fit, or whether it exactly fits. Some algorithms
   * might depend on the return value! */
  if (bytes > *tbs)
    return false;
  if (bytes == *tbs)
    return true;

  /* is the minimum enough? */
  *nb_rb = 5;
  *tbs = nr_compute_tbs(Qm, R, *nb_rb, nb_symb_sch, nb_dmrs_prb, 0, 0, nrOfLayers) >> 3;
  if (bytes <= *tbs)
    return true;

  /* perform binary search to allocate all bytes within a TBS up to nb_rb_max
   * RBs */
  int hi = nb_rb_max;
  int lo = 1;
  for (int p = (hi + lo) / 2; lo + 1 < hi; p = (hi + lo) / 2) {
    const uint32_t TBS = nr_compute_tbs(Qm, R, p, nb_symb_sch, nb_dmrs_prb, 0, 0, nrOfLayers) >> 3;
    if (bytes == TBS) {
      hi = p;
      break;
    } else if (bytes < TBS) {
      hi = p;
    } else {
      lo = p;
    }
  }
  *nb_rb = hi;
  *tbs = nr_compute_tbs(Qm, R, *nb_rb, nb_symb_sch, nb_dmrs_prb, 0, 0, nrOfLayers) >> 3;
  /* return whether we could allocate all bytes and stay below nb_rb_max */
  return *tbs >= bytes && *nb_rb <= nb_rb_max;
}

void nr_set_pdsch_semi_static(const NR_ServingCellConfigCommon_t *scc,
                              const NR_CellGroupConfig_t *secondaryCellGroup,
                              const NR_BWP_Downlink_t *bwp,
                              const NR_BWP_DownlinkDedicated_t *bwpd0,
                              int tda,
                              uint8_t layers,
                              NR_UE_sched_ctrl_t *sched_ctrl,
                              NR_pdsch_semi_static_t *ps)
{
  bool reset_dmrs = false;

  NR_BWP_DownlinkDedicated_t *bwpd;

  if (bwp && bwp->bwp_Dedicated) {
    bwpd = bwp->bwp_Dedicated;
  } else {
    bwpd = (NR_BWP_DownlinkDedicated_t*)bwpd0;
  }

  if (bwpd->pdsch_Config &&
      bwpd->pdsch_Config->choice.setup &&
      bwpd->pdsch_Config->choice.setup->mcs_Table) {
    if (*bwpd->pdsch_Config->choice.setup->mcs_Table == 0)
      ps->mcsTableIdx = 1;
    else
      ps->mcsTableIdx = 2;
  }
  else ps->mcsTableIdx = 0;

  NR_PDSCH_Config_t *pdsch_Config;
  if (bwp &&
      bwp->bwp_Dedicated &&
      bwp->bwp_Dedicated->pdsch_Config &&
      bwp->bwp_Dedicated->pdsch_Config->choice.setup)
    pdsch_Config = bwp->bwp_Dedicated->pdsch_Config->choice.setup;
  else
    pdsch_Config = NULL;

  if (ps->time_domain_allocation != tda) {
    reset_dmrs = true;
    ps->time_domain_allocation = tda;
    const struct NR_PDSCH_TimeDomainResourceAllocationList *tdaList = bwp ?
      bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList :
      scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    AssertFatal(tda < tdaList->list.count, "time_domain_allocation %d>=%d\n", tda, tdaList->list.count);
    ps->mapping_type = tdaList->list.array[tda]->mappingType;
    if (pdsch_Config) {
      if (ps->mapping_type == NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeB)
        ps->dmrsConfigType = pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->dmrs_Type == NULL ? 0 : 1;
      else
        ps->dmrsConfigType = pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NULL ? 0 : 1;
    }
    else
      ps->dmrsConfigType = NFAPI_NR_DMRS_TYPE1;
    const int startSymbolAndLength = tdaList->list.array[tda]->startSymbolAndLength;
    SLIV2SL(startSymbolAndLength, &ps->startSymbolIndex, &ps->nrOfSymbols);
  }

  const long f = sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats;
  int dci_format;
  if (sched_ctrl->search_space) {
    dci_format = f ? NR_DL_DCI_FORMAT_1_1 : NR_DL_DCI_FORMAT_1_0;
  }
  else {
    dci_format = NR_DL_DCI_FORMAT_1_0;
  }
  if (dci_format == NR_DL_DCI_FORMAT_1_0) {
    if (ps->nrOfSymbols == 2)
      ps->numDmrsCdmGrpsNoData = 1;
    else
      ps->numDmrsCdmGrpsNoData = 2;
    ps->dmrs_ports_id = 0;
    ps->frontloaded_symb = 1;
    ps->nrOfLayers = 1;
  }
  else {
    if (ps->nrOfLayers != layers) {
      reset_dmrs = true;
      ps->nrOfLayers = layers;
      set_dl_dmrs_ports(ps);
    }
  }

  ps->N_PRB_DMRS = ps->numDmrsCdmGrpsNoData * (ps->dmrsConfigType == NFAPI_NR_DMRS_TYPE1 ? 6 : 4);
  if (reset_dmrs) {
    ps->dl_dmrs_symb_pos = fill_dmrs_mask(bwpd ? bwpd->pdsch_Config->choice.setup : NULL, scc->dmrs_TypeA_Position, ps->nrOfSymbols, ps->startSymbolIndex, ps->mapping_type, ps->frontloaded_symb);
    ps->N_DMRS_SLOT = get_num_dmrs(ps->dl_dmrs_symb_pos);
  }
  LOG_D(NR_MAC,"bwpd0 %p, bwpd %p : Filling dmrs info, ps->N_PRB_DMRS %d, ps->dl_dmrs_symb_pos %x, ps->N_DMRS_SLOT %d\n",bwpd0,bwpd,ps->N_PRB_DMRS,ps->dl_dmrs_symb_pos,ps->N_DMRS_SLOT);
}

void nr_set_pusch_semi_static(const NR_ServingCellConfigCommon_t *scc,
                              const NR_BWP_Uplink_t *ubwp,
			                        const NR_BWP_UplinkDedicated_t *ubwpd,
                              long dci_format,
                              int tda,
                              uint8_t num_dmrs_cdm_grps_no_data,
                              NR_pusch_semi_static_t *ps)
{
  ps->dci_format = dci_format;
  ps->time_domain_allocation = tda;

  const struct NR_PUSCH_TimeDomainResourceAllocationList *tdaList =
    ubwp?
    ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList:
    scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList ;
  const int startSymbolAndLength = tdaList->list.array[tda]->startSymbolAndLength;
  SLIV2SL(startSymbolAndLength,
          &ps->startSymbolIndex,
          &ps->nrOfSymbols);

  ps->pusch_Config = ubwp?ubwp->bwp_Dedicated->pusch_Config->choice.setup:(ubwpd ? ubwpd->pusch_Config->choice.setup : NULL);
  if (ps->pusch_Config == NULL || !ps->pusch_Config->transformPrecoder)
    ps->transform_precoding = !scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder;
  else
    ps->transform_precoding = *ps->pusch_Config->transformPrecoder;
  const int target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
  if (ps->transform_precoding)
    ps->mcs_table = get_pusch_mcs_table(ps->pusch_Config ? ps->pusch_Config->mcs_Table : NULL,
                                    0,
                                    ps->dci_format,
                                    NR_RNTI_C,
                                    target_ss,
                                    false);
  else {
    ps->mcs_table = get_pusch_mcs_table(ps->pusch_Config ? ps->pusch_Config->mcs_TableTransformPrecoder : NULL,
                                    1,
                                    ps->dci_format,
                                    NR_RNTI_C,
                                    target_ss,
                                    false);
    num_dmrs_cdm_grps_no_data = 2; // in case of transform precoding - no Data sent in DMRS symbol
  }

  ps->num_dmrs_cdm_grps_no_data = num_dmrs_cdm_grps_no_data;

  /* DMRS calculations */
  ps->mapping_type = tdaList->list.array[tda]->mappingType;
  ps->NR_DMRS_UplinkConfig = ps->pusch_Config ?
    (ps->mapping_type == NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeA ?
     ps->pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup :
     ps->pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup) : NULL;
  ps->dmrs_config_type = ps->NR_DMRS_UplinkConfig ? ((ps->NR_DMRS_UplinkConfig->dmrs_Type == NULL ? 0 : 1)) : 0;
  const pusch_dmrs_AdditionalPosition_t additional_pos =
						     ps->NR_DMRS_UplinkConfig ? (ps->NR_DMRS_UplinkConfig->dmrs_AdditionalPosition == NULL
										 ? 2
										 : (*ps->NR_DMRS_UplinkConfig->dmrs_AdditionalPosition ==
										    NR_DMRS_UplinkConfig__dmrs_AdditionalPosition_pos3
										    ? 3
										    : *ps->NR_DMRS_UplinkConfig->dmrs_AdditionalPosition)):2;
  const pusch_maxLength_t pusch_maxLength =
    ps->NR_DMRS_UplinkConfig ? (ps->NR_DMRS_UplinkConfig->maxLength == NULL ? 1 : 2) : 1;
  ps->ul_dmrs_symb_pos = get_l_prime(ps->nrOfSymbols,
                                            ps->mapping_type,
                                            additional_pos,
                                            pusch_maxLength,
                                            ps->startSymbolIndex,
                                            scc->dmrs_TypeA_Position);
  uint8_t num_dmrs_symb = 0;
  for(int i = ps->startSymbolIndex; i < ps->startSymbolIndex + ps->nrOfSymbols; i++)
    num_dmrs_symb += (ps->ul_dmrs_symb_pos >> i) & 1;
  ps->num_dmrs_symb = num_dmrs_symb;
  ps->N_PRB_DMRS = ps->dmrs_config_type == 0
      ? num_dmrs_cdm_grps_no_data * 6
      : num_dmrs_cdm_grps_no_data * 4;
}

void nr_configure_css_dci_initial(nfapi_nr_dl_tti_pdcch_pdu_rel15_t* pdcch_pdu,
				  nr_scs_e scs_common,
				  nr_scs_e pdcch_scs,
				  nr_frequency_range_e freq_range,
				  uint8_t rmsi_pdcch_config,
				  uint8_t ssb_idx,
				  uint8_t k_ssb,
				  uint16_t sfn_ssb,
				  uint8_t n_ssb, /*slot index overlapping the corresponding SSB index*/
				  uint16_t nb_slots_per_frame,
				  uint16_t N_RB)
{
  //  uint8_t O, M;
  //  uint8_t ss_idx = rmsi_pdcch_config&0xf;
  //  uint8_t cset_idx = (rmsi_pdcch_config>>4)&0xf;
  //  uint8_t mu = scs_common;
  //  uint8_t O_scale=0, M_scale=0; // used to decide if the values of O and M need to be divided by 2

  AssertFatal(1==0,"todo\n");
  /*
  /// Coreset params
  switch(scs_common) {

    case kHz15:

      switch(pdcch_scs) {
        case kHz15:
          AssertFatal(cset_idx<15,"Coreset index %d reserved for scs kHz15/kHz15\n", cset_idx);
          pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
          pdcch_pdu->n_rb = (cset_idx < 6)? 24 : (cset_idx < 12)? 48 : 96;
          pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_15_15[cset_idx];
          pdcch_pdu->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_15_15[cset_idx];
        break;

        case kHz30:
          AssertFatal(cset_idx<14,"Coreset index %d reserved for scs kHz15/kHz30\n", cset_idx);
          pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
          pdcch_pdu->n_rb = (cset_idx < 8)? 24 : 48;
          pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_15_30[cset_idx];
          pdcch_pdu->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_15_15[cset_idx];
        break;

        default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);

      }
      break;

    case kHz30:

      if (N_RB < 106) { // Minimum 40Mhz bandwidth not satisfied
        switch(pdcch_scs) {
          case kHz15:
            AssertFatal(cset_idx<9,"Coreset index %d reserved for scs kHz30/kHz15\n", cset_idx);
            pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_pdu->n_rb = (cset_idx < 10)? 48 : 96;
            pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_15_b40Mhz[cset_idx];
            pdcch_pdu->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_30_15_b40Mhz[cset_idx];
          break;

          case kHz30:
            pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_pdu->n_rb = (cset_idx < 6)? 24 : 48;
            pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_30_b40Mhz[cset_idx];
            pdcch_pdu->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_30_30_b40Mhz[cset_idx];
          break;

          default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
        }
      }

      else { // above 40Mhz
        switch(pdcch_scs) {
          case kHz15:
            AssertFatal(cset_idx<9,"Coreset index %d reserved for scs kHz30/kHz15\n", cset_idx);
            pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_pdu->n_rb = (cset_idx < 3)? 48 : 96;
            pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_15_a40Mhz[cset_idx];
            pdcch_pdu->rb_offset = nr_coreset_rb_offset_pdcch_type_0_scs_30_15_a40Mhz[cset_idx];
          break;

          case kHz30:
            AssertFatal(cset_idx<10,"Coreset index %d reserved for scs kHz30/kHz30\n", cset_idx);
            pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
            pdcch_pdu->n_rb = (cset_idx < 4)? 24 : 48;
            pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_30_30_a40Mhz[cset_idx];
            pdcch_pdu->rb_offset =  nr_coreset_rb_offset_pdcch_type_0_scs_30_30_a40Mhz[cset_idx];
          break;

          default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
        }
      }
      break;

    case kHz120:
      switch(pdcch_scs) {
        case kHz60:
          AssertFatal(cset_idx<12,"Coreset index %d reserved for scs kHz120/kHz60\n", cset_idx);
          pdcch_pdu->mux_pattern = (cset_idx < 8)?NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 : NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2;
          pdcch_pdu->n_rb = (cset_idx < 6)? 48 : (cset_idx < 8)? 96 : (cset_idx < 10)? 48 : 96;
          pdcch_pdu->n_symb = nr_coreset_nsymb_pdcch_type_0_scs_120_60[cset_idx];
          pdcch_pdu->rb_offset = (nr_coreset_rb_offset_pdcch_type_0_scs_120_60[cset_idx]>0)?nr_coreset_rb_offset_pdcch_type_0_scs_120_60[cset_idx] :
          (k_ssb == 0)? -41 : -42;
        break;

        case kHz120:
          AssertFatal(cset_idx<8,"Coreset index %d reserved for scs kHz120/kHz120\n", cset_idx);
          pdcch_pdu->mux_pattern = (cset_idx < 4)?NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 : NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE3;
          pdcch_pdu->n_rb = (cset_idx < 2)? 24 : (cset_idx < 4)? 48 : (cset_idx < 6)? 24 : 48;
          pdcch_pdu->n_symb = (cset_idx == 2)? 1 : 2;
          pdcch_pdu->rb_offset = (nr_coreset_rb_offset_pdcch_type_0_scs_120_120[cset_idx]>0)? nr_coreset_rb_offset_pdcch_type_0_scs_120_120[cset_idx] :
          (k_ssb == 0)? -20 : -21;
        break;

        default:
            AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
      }
    break;

    case kHz240:
    switch(pdcch_scs) {
      case kHz60:
        AssertFatal(cset_idx<4,"Coreset index %d reserved for scs kHz240/kHz60\n", cset_idx);
        pdcch_pdu->mux_pattern = NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1;
        pdcch_pdu->n_rb = 96;
        pdcch_pdu->n_symb = (cset_idx < 2)? 1 : 2;
        pdcch_pdu->rb_offset = (cset_idx&1)? 16 : 0;
      break;

      case kHz120:
        AssertFatal(cset_idx<8,"Coreset index %d reserved for scs kHz240/kHz120\n", cset_idx);
        pdcch_pdu->mux_pattern = (cset_idx < 4)? NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1 : NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2;
        pdcch_pdu->n_rb = (cset_idx < 4)? 48 : (cset_idx < 6)? 24 : 48;
        pdcch_pdu->n_symb = ((cset_idx==2)||(cset_idx==3))? 2 : 1;
        pdcch_pdu->rb_offset = (nr_coreset_rb_offset_pdcch_type_0_scs_240_120[cset_idx]>0)? nr_coreset_rb_offset_pdcch_type_0_scs_240_120[cset_idx] :
        (k_ssb == 0)? -41 : -42;
      break;

      default:
          AssertFatal(1==0,"Invalid scs_common/pdcch_scs combination %d/%d \n", scs_common, pdcch_scs);
    }
    break;

  default:
    AssertFatal(1==0,"Invalid common subcarrier spacing %d\n", scs_common);

  }

  /// Search space params
  switch(pdcch_pdu->mux_pattern) {

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1:
      if (freq_range == nr_FR1) {
        O = nr_ss_param_O_type_0_mux1_FR1[ss_idx];
        pdcch_pdu->nb_ss_sets_per_slot = nr_ss_sets_per_slot_type_0_FR1[ss_idx];
        M = nr_ss_param_M_type_0_mux1_FR1[ss_idx];
        M_scale = nr_ss_scale_M_mux1_FR1[ss_idx];
        pdcch_pdu->first_symbol = (ss_idx < 8)? ( (ssb_idx&1)? pdcch_pdu->n_symb : 0 ) : nr_ss_first_symb_idx_type_0_mux1_FR1[ss_idx - 8];
      }

      else {
        AssertFatal(ss_idx<14 ,"Invalid search space index for multiplexing type 1 and FR2 %d\n", ss_idx);
        O = nr_ss_param_O_type_0_mux1_FR2[ss_idx];
        O_scale = nr_ss_scale_O_mux1_FR2[ss_idx];
        pdcch_pdu->nb_ss_sets_per_slot = nr_ss_sets_per_slot_type_0_FR2[ss_idx];
        M = nr_ss_param_M_type_0_mux1_FR2[ss_idx];
        M_scale = nr_ss_scale_M_mux1_FR2[ss_idx];
        pdcch_pdu->first_symbol = (ss_idx < 12)? ( (ss_idx&1)? 7 : 0 ) : 0;
      }
      pdcch_pdu->nb_slots = 2;
      pdcch_pdu->sfn_mod2 = (CEILIDIV( (((O<<mu)>>O_scale) + ((ssb_idx*M)>>M_scale)), nb_slots_per_frame ) & 1)? 1 : 0;
      pdcch_pdu->first_slot = (((O<<mu)>>O_scale) + ((ssb_idx*M)>>M_scale)) % nb_slots_per_frame;

    break;

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2:
      AssertFatal( ((scs_common==kHz120)&&(pdcch_scs==kHz60)) || ((scs_common==kHz240)&&(pdcch_scs==kHz120)),
      "Invalid scs_common/pdcch_scs combination %d/%d for Mux type 2\n", scs_common, pdcch_scs );
      AssertFatal(ss_idx==0, "Search space index %d reserved for scs_common/pdcch_scs combination %d/%d", ss_idx, scs_common, pdcch_scs);

      pdcch_pdu->nb_slots = 1;

      if ((scs_common==kHz120)&&(pdcch_scs==kHz60)) {
        pdcch_pdu->first_symbol = nr_ss_first_symb_idx_scs_120_60_mux2[ssb_idx&3];
        // Missing in pdcch_pdu sfn_C and n_C here and in else case
      }
      else {
        pdcch_pdu->first_symbol = ((ssb_idx&7)==4)?12 : ((ssb_idx&7)==4)?13 : nr_ss_first_symb_idx_scs_240_120_set1_mux2[ssb_idx&7]; //???
      }

    break;

    case NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE3:
      AssertFatal( (scs_common==kHz120)&&(pdcch_scs==kHz120),
      "Invalid scs_common/pdcch_scs combination %d/%d for Mux type 3\n", scs_common, pdcch_scs );
      AssertFatal(ss_idx==0, "Search space index %d reserved for scs_common/pdcch_scs combination %d/%d", ss_idx, scs_common, pdcch_scs);

      pdcch_pdu->first_symbol = nr_ss_first_symb_idx_scs_120_120_mux3[ssb_idx&3];

    break;

    default:
      AssertFatal(1==0, "Invalid SSB and coreset multiplexing pattern %d\n", pdcch_pdu->mux_pattern);
  }
  pdcch_pdu->config_type = NFAPI_NR_CSET_CONFIG_MIB_SIB1;
  pdcch_pdu->cr_mapping_type = NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED;
  pdcch_pdu->precoder_granularity = NFAPI_NR_CSET_SAME_AS_REG_BUNDLE;
  pdcch_pdu->reg_bundle_size = 6;
  pdcch_pdu->interleaver_size = 2;
  // set initial banwidth part to full bandwidth
  pdcch_pdu->n_RB_BWP = N_RB;

  */

}

void config_uldci(const NR_BWP_Uplink_t *ubwp,
		              const NR_BWP_UplinkDedicated_t *ubwpd,
                  const NR_ServingCellConfigCommon_t *scc,
                  const nfapi_nr_pusch_pdu_t *pusch_pdu,
                  dci_pdu_rel15_t *dci_pdu_rel15,
                  int dci_format,
                  int time_domain_assignment,
                  uint8_t tpc,
                  int n_ubwp,
                  int bwp_id) {

  const int bw = NRRIV2BW(ubwp ?
			  ubwp->bwp_Common->genericParameters.locationAndBandwidth :
			  scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  dci_pdu_rel15->frequency_domain_assignment.val =
      PRBalloc_to_locationandbandwidth0(pusch_pdu->rb_size, pusch_pdu->rb_start, bw);
  dci_pdu_rel15->time_domain_assignment.val = time_domain_assignment;
  dci_pdu_rel15->frequency_hopping_flag.val = pusch_pdu->frequency_hopping;
  dci_pdu_rel15->mcs = pusch_pdu->mcs_index;
  dci_pdu_rel15->ndi = pusch_pdu->pusch_data.new_data_indicator;
  dci_pdu_rel15->rv = pusch_pdu->pusch_data.rv_index;
  dci_pdu_rel15->harq_pid = pusch_pdu->pusch_data.harq_process_id;
  dci_pdu_rel15->tpc = tpc;
  const NR_BWP_UplinkDedicated_t *ubwpd2 = (ubwp) ? ubwp->bwp_Dedicated : ubwpd;

  if (ubwpd2) AssertFatal(ubwpd2->pusch_Config->choice.setup->resourceAllocation == NR_PUSCH_Config__resourceAllocation_resourceAllocationType1,
			"Only frequency resource allocation type 1 is currently supported\n");
  switch (dci_format) {
    case NR_UL_DCI_FORMAT_0_0:
      dci_pdu_rel15->format_indicator = 0;
      break;
    case NR_UL_DCI_FORMAT_0_1:
      LOG_D(NR_MAC,"Configuring DCI Format 0_1\n");
      dci_pdu_rel15->dai[0].val = 0; //TODO
      // bwp indicator as per table 7.3.1.1.2-1 in 38.212
      dci_pdu_rel15->bwp_indicator.val = n_ubwp < 4 ? bwp_id : bwp_id - 1;
      // SRS resource indicator
      if (ubwpd2 &&
          ubwpd2->pusch_Config &&
          ubwpd2->pusch_Config->choice.setup &&
          ubwpd2->pusch_Config->choice.setup->txConfig != NULL) {
        AssertFatal(*ubwpd2->pusch_Config->choice.setup->txConfig == NR_PUSCH_Config__txConfig_codebook,
                    "Non Codebook configuration non supported\n");
        dci_pdu_rel15->srs_resource_indicator.val = 0; // taking resource 0 for SRS
      }
      // Antenna Ports
      dci_pdu_rel15->antenna_ports.val = 0; // TODO for now it is hardcoded, it should depends on cdm group no data and rank
      // DMRS sequence initialization
      dci_pdu_rel15->dmrs_sequence_initialization.val = pusch_pdu->scid;
      break;
    default :
      AssertFatal(0, "Valid UL formats are 0_0 and 0_1\n");
  }

  LOG_D(NR_MAC,
        "%s() ULDCI type 0 payload: freq_alloc %d, time_alloc %d, freq_hop_flag %d, mcs %d tpc %d ndi %d rv %d\n",
        __func__,
        dci_pdu_rel15->frequency_domain_assignment.val,
        dci_pdu_rel15->time_domain_assignment.val,
        dci_pdu_rel15->frequency_hopping_flag.val,
        dci_pdu_rel15->mcs,
        dci_pdu_rel15->tpc,
        dci_pdu_rel15->ndi,
        dci_pdu_rel15->rv);
}

const int default_pucch_fmt[]       = {0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1};
const int default_pucch_firstsymb[] = {12,12,12,10,10,10,10,4,4,4,4,0,0,0,0,0};
const int default_pucch_numbsymb[]  = {2,2,2,2,4,4,4,4,10,10,10,10,14,14,14,14,14};
const int default_pucch_prboffset[] = {0,0,3,0,0,2,4,0,0,2,4,0,0,2,4,-1};
const int default_pucch_csset[]     = {2,3,3,2,4,4,4,2,4,4,4,2,4,4,4,4};

int nr_get_default_pucch_res(int pucch_ResourceCommon) {

  AssertFatal(pucch_ResourceCommon>=0 && pucch_ResourceCommon < 16, "illegal pucch_ResourceCommon %d\n",pucch_ResourceCommon);

  return(default_pucch_csset[pucch_ResourceCommon]);
}

void nr_configure_pdcch(gNB_MAC_INST *gNB_mac,
                        nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu,
                        NR_SearchSpace_t *ss,
                        NR_ControlResourceSet_t *coreset,
                        NR_ServingCellConfigCommon_t *scc,
                        NR_BWP_t *bwp,
                        NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config) {

  int sps;
  AssertFatal(*ss->controlResourceSetId == coreset->controlResourceSetId,
              "coreset id in SS %ld does not correspond to the one in coreset %ld",
              *ss->controlResourceSetId, coreset->controlResourceSetId);

  if (bwp) { // This is not for SIB1
    if(coreset->controlResourceSetId == 0){
      pdcch_pdu->BWPSize  = gNB_mac->cset0_bwp_size;
      pdcch_pdu->BWPStart = gNB_mac->cset0_bwp_start;
    }
    else {
      pdcch_pdu->BWPSize  = NRRIV2BW(bwp->locationAndBandwidth, MAX_BWP_SIZE);
      pdcch_pdu->BWPStart = NRRIV2PRBOFFSET(bwp->locationAndBandwidth, MAX_BWP_SIZE);
    }
    pdcch_pdu->SubcarrierSpacing = bwp->subcarrierSpacing;
    pdcch_pdu->CyclicPrefix = (bwp->cyclicPrefix==NULL) ? 0 : *bwp->cyclicPrefix;

    //AssertFatal(pdcch_scs==kHz15, "PDCCH SCS above 15kHz not allowed if a symbol above 2 is monitored");
    sps = bwp->cyclicPrefix == NULL ? 14 : 12;
  }
  else {
    AssertFatal(type0_PDCCH_CSS_config!=NULL,"type0_PDCCH_CSS_config is null,bwp %p\n",bwp);
    pdcch_pdu->BWPSize = type0_PDCCH_CSS_config->num_rbs;
    pdcch_pdu->BWPStart = type0_PDCCH_CSS_config->cset_start_rb;
    pdcch_pdu->SubcarrierSpacing = type0_PDCCH_CSS_config->scs_pdcch;
    pdcch_pdu->CyclicPrefix = 0;
    sps = 14;
  }

  AssertFatal(ss->monitoringSymbolsWithinSlot!=NULL,"ss->monitoringSymbolsWithinSlot is null\n");
  AssertFatal(ss->monitoringSymbolsWithinSlot->buf!=NULL,"ss->monitoringSymbolsWithinSlot->buf is null\n");
    
  // for SPS=14 8 MSBs in positions 13 downto 6
  uint16_t monitoringSymbolsWithinSlot = (ss->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) |
                                         (ss->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));

  for (int i=0; i<sps; i++) {
    if ((monitoringSymbolsWithinSlot>>(sps-1-i))&1) {
      pdcch_pdu->StartSymbolIndex=i;
      break;
    }
  }

  pdcch_pdu->DurationSymbols  = coreset->duration;

  for (int i=0;i<6;i++)
    pdcch_pdu->FreqDomainResource[i] = coreset->frequencyDomainResources.buf[i];

  LOG_D(MAC,"Coreset : BWPstart %d, BWPsize %d, SCS %d, freq %x, , duration %d,  \n",pdcch_pdu->BWPStart,pdcch_pdu->BWPSize,(int)pdcch_pdu->SubcarrierSpacing,(int)coreset->frequencyDomainResources.buf[0],(int)coreset->duration);

  //cce-REG-MappingType
  pdcch_pdu->CceRegMappingType = coreset->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved?
    NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED : NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED;

  if (pdcch_pdu->CceRegMappingType == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
    pdcch_pdu->RegBundleSize = (coreset->cce_REG_MappingType.choice.interleaved->reg_BundleSize ==
                                NR_ControlResourceSet__cce_REG_MappingType__interleaved__reg_BundleSize_n6) ? 6 : (2+coreset->cce_REG_MappingType.choice.interleaved->reg_BundleSize);
    pdcch_pdu->InterleaverSize = (coreset->cce_REG_MappingType.choice.interleaved->interleaverSize ==
                                  NR_ControlResourceSet__cce_REG_MappingType__interleaved__interleaverSize_n6) ? 6 : (2+coreset->cce_REG_MappingType.choice.interleaved->interleaverSize);
    AssertFatal(scc->physCellId != NULL,"scc->physCellId is null\n");
    pdcch_pdu->ShiftIndex = coreset->cce_REG_MappingType.choice.interleaved->shiftIndex != NULL ? *coreset->cce_REG_MappingType.choice.interleaved->shiftIndex : *scc->physCellId;
  }
  else {
    pdcch_pdu->RegBundleSize = 0;
    pdcch_pdu->InterleaverSize = 0;
    pdcch_pdu->ShiftIndex = 0;
  }

  if(coreset->controlResourceSetId == 0) {
    if(bwp == NULL)
      pdcch_pdu->CoreSetType = NFAPI_NR_CSET_CONFIG_MIB_SIB1;
    else
      pdcch_pdu->CoreSetType = NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG_CSET_0;
  } else{
    pdcch_pdu->CoreSetType = NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG;
  }

  //precoderGranularity
  pdcch_pdu->precoderGranularity = coreset->precoderGranularity;
}


// This function configures pucch pdu fapi structure
void nr_configure_pucch(nfapi_nr_pucch_pdu_t* pucch_pdu,
                        NR_ServingCellConfigCommon_t *scc,
                        NR_CellGroupConfig_t *CellGroup,
                        NR_BWP_Uplink_t *bwp,
                        NR_BWP_UplinkDedicated_t *bwpd,
                        uint16_t rnti,
                        uint8_t pucch_resource,
                        uint16_t O_csi,
                        uint16_t O_ack,
                        uint8_t O_sr,
                        int r_pucch) {

  NR_PUCCH_Config_t *pucch_Config;
  NR_PUCCH_Resource_t *pucchres;
  NR_PUCCH_ResourceSet_t *pucchresset;
  NR_PUCCH_FormatConfig_t *pucchfmt;
  NR_PUCCH_ResourceId_t *resource_id = NULL;

  long *id0 = NULL;
  int n_list, n_set;
  uint16_t N2,N3;
  int res_found = 0;

  pucch_pdu->bit_len_harq = O_ack;
  pucch_pdu->bit_len_csi_part1 = O_csi;

  uint16_t O_uci = O_csi + O_ack;

  NR_PUSCH_Config_t *pusch_Config = bwp ? bwp->bwp_Dedicated->pusch_Config->choice.setup : bwpd->pusch_Config->choice.setup;
  long *pusch_id = pusch_Config ? pusch_Config->dataScramblingIdentityPUSCH : NULL;

  if (pusch_Config && pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA != NULL)
    id0 = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup->transformPrecodingDisabled->scramblingID0;
  else if (pusch_Config && pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB != NULL)
    id0 = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->transformPrecodingDisabled->scramblingID0;
  else id0 = scc->physCellId;

  NR_PUCCH_ConfigCommon_t *pucch_ConfigCommon = bwp ?
    bwp->bwp_Common->pucch_ConfigCommon->choice.setup :
    scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup;
  // hop flags and hopping id are valid for any BWP
  switch (pucch_ConfigCommon->pucch_GroupHopping){
  case 0 :
    // if neither, both disabled
    pucch_pdu->group_hop_flag = 0;
    pucch_pdu->sequence_hop_flag = 0;
    break;
  case 1 :
    // if enable, group enabled
    pucch_pdu->group_hop_flag = 1;
    pucch_pdu->sequence_hop_flag = 0;
    break;
  case 2 :
    // if disable, sequence disabled
    pucch_pdu->group_hop_flag = 0;
    pucch_pdu->sequence_hop_flag = 1;
    break;
  default:
    AssertFatal(1==0,"Group hopping flag %ld undefined (0,1,2) \n", pucch_ConfigCommon->pucch_GroupHopping);
  }

  if (pucch_ConfigCommon->hoppingId != NULL)
    pucch_pdu->hopping_id = *pucch_ConfigCommon->hoppingId;
  else
    pucch_pdu->hopping_id = *scc->physCellId;
  NR_BWP_t *genericParameters = bwp ?
    &bwp->bwp_Common->genericParameters:
    &scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;
  pucch_pdu->bwp_size  = NRRIV2BW(genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
  pucch_pdu->bwp_start = NRRIV2PRBOFFSET(genericParameters->locationAndBandwidth,MAX_BWP_SIZE);
  pucch_pdu->subcarrier_spacing = genericParameters->subcarrierSpacing;
  pucch_pdu->cyclic_prefix = (genericParameters->cyclicPrefix==NULL) ? 0 : *genericParameters->cyclicPrefix;
  if (r_pucch<0 || bwp ){
      LOG_D(NR_MAC,"pucch_acknak: Filling dedicated configuration for PUCCH\n");
   // we have either a dedicated BWP or Dedicated PUCCH configuration on InitialBWP
      AssertFatal(bwp!=NULL || bwpd!=NULL,"We need one dedicated configuration for a BWP (neither additional or initial BWP has a dedicated configuration)\n");
      pucch_Config = bwp ?
	  bwp->bwp_Dedicated->pucch_Config->choice.setup:
	  bwpd->pucch_Config->choice.setup;

      AssertFatal(pucch_Config->resourceSetToAddModList!=NULL,
		    "PUCCH resourceSetToAddModList is null\n");

      n_set = pucch_Config->resourceSetToAddModList->list.count;
      AssertFatal(n_set>0,"PUCCH resourceSetToAddModList is empty\n");

      LOG_D(NR_MAC, "UCI n_set= %d\n", n_set);

      N2 = 2;
	// procedure to select pucch resource id from resource sets according to
	// number of uci bits and pucch resource indicator pucch_resource
	// ( see table 9.2.3.2 in 38.213)
      for (int i=0; i<n_set; i++) {
	pucchresset = pucch_Config->resourceSetToAddModList->list.array[i];
	n_list = pucchresset->resourceList.list.count;
	if (pucchresset->pucch_ResourceSetId == 0 && O_uci<3) {
	  if (pucch_resource < n_list)
            resource_id = pucchresset->resourceList.list.array[pucch_resource];
          else
            AssertFatal(1==0,"Couldn't fine pucch resource indicator %d in PUCCH resource set %d for %d UCI bits",pucch_resource,i,O_uci);
        }
        if (pucchresset->pucch_ResourceSetId == 1 && O_uci>2) {
#if (NR_RRC_VERSION >= MAKE_VERSION(16, 0, 0))
        N3 = pucchresset->maxPayloadSize!= NULL ?  *pucchresset->maxPayloadSize : 1706;
#else
        N3 = pucchresset->maxPayloadMinus1!= NULL ?  *pucchresset->maxPayloadMinus1 : 1706;
#endif
        if (N2<O_uci && N3>O_uci) {
          if (pucch_resource < n_list)
            resource_id = pucchresset->resourceList.list.array[pucch_resource];
          else
            AssertFatal(1==0,"Couldn't fine pucch resource indicator %d in PUCCH resource set %d for %d UCI bits",pucch_resource,i,O_uci);
        }
        else N2 = N3;
      }
    }

    AssertFatal(resource_id!=NULL,"Couldn-t find any matching PUCCH resource in the PUCCH resource sets");

    AssertFatal(pucch_Config->resourceToAddModList!=NULL,
                "PUCCH resourceToAddModList is null\n");

    n_list = pucch_Config->resourceToAddModList->list.count;
    AssertFatal(n_list>0,"PUCCH resourceToAddModList is empty\n");

    // going through the list of PUCCH resources to find the one indexed by resource_id
    for (int i=0; i<n_list; i++) {
      pucchres = pucch_Config->resourceToAddModList->list.array[i];
      if (pucchres->pucch_ResourceId == *resource_id) {
        res_found = 1;
        pucch_pdu->prb_start = pucchres->startingPRB;
        pucch_pdu->rnti = rnti;
        // FIXME why there is only one frequency hopping flag
        // what about inter slot frequency hopping?
        pucch_pdu->freq_hop_flag = pucchres->intraSlotFrequencyHopping!= NULL ?  1 : 0;
        pucch_pdu->second_hop_prb = pucchres->secondHopPRB!= NULL ?  *pucchres->secondHopPRB : 0;
        switch(pucchres->format.present) {
          case NR_PUCCH_Resource__format_PR_format0 :
            pucch_pdu->format_type = 0;
            pucch_pdu->initial_cyclic_shift = pucchres->format.choice.format0->initialCyclicShift;
            pucch_pdu->nr_of_symbols = pucchres->format.choice.format0->nrofSymbols;
            pucch_pdu->start_symbol_index = pucchres->format.choice.format0->startingSymbolIndex;
            pucch_pdu->sr_flag = O_sr;
            pucch_pdu->prb_size = 1;
            break;
          case NR_PUCCH_Resource__format_PR_format1 :
            pucch_pdu->format_type = 1;
            pucch_pdu->initial_cyclic_shift = pucchres->format.choice.format1->initialCyclicShift;
            pucch_pdu->nr_of_symbols = pucchres->format.choice.format1->nrofSymbols;
            pucch_pdu->start_symbol_index = pucchres->format.choice.format1->startingSymbolIndex;
            pucch_pdu->time_domain_occ_idx = pucchres->format.choice.format1->timeDomainOCC;
            pucch_pdu->sr_flag = O_sr;
            pucch_pdu->prb_size = 1;
            break;
          case NR_PUCCH_Resource__format_PR_format2 :
            pucch_pdu->format_type = 2;
            pucch_pdu->nr_of_symbols = pucchres->format.choice.format2->nrofSymbols;
            pucch_pdu->start_symbol_index = pucchres->format.choice.format2->startingSymbolIndex;
            pucch_pdu->data_scrambling_id = pusch_id!= NULL ? *pusch_id : *scc->physCellId;
            pucch_pdu->dmrs_scrambling_id = id0!= NULL ? *id0 : *scc->physCellId;
            pucch_pdu->prb_size = compute_pucch_prb_size(2,pucchres->format.choice.format2->nrofPRBs,
                                                         O_uci+O_sr,O_csi,pucch_Config->format2->choice.setup->maxCodeRate,
                                                         2,pucchres->format.choice.format2->nrofSymbols,8);
            pucch_pdu->bit_len_csi_part1 = O_csi;
            break;
          case NR_PUCCH_Resource__format_PR_format3 :
            pucch_pdu->format_type = 3;
            pucch_pdu->nr_of_symbols = pucchres->format.choice.format3->nrofSymbols;
            pucch_pdu->start_symbol_index = pucchres->format.choice.format3->startingSymbolIndex;
            pucch_pdu->data_scrambling_id = pusch_id!= NULL ? *pusch_id : *scc->physCellId;
            if (pucch_Config->format3 == NULL) {
              pucch_pdu->pi_2bpsk = 0;
              pucch_pdu->add_dmrs_flag = 0;
            }
            else {
              pucchfmt = pucch_Config->format3->choice.setup;
              pucch_pdu->pi_2bpsk = pucchfmt->pi2BPSK!= NULL ?  1 : 0;
              pucch_pdu->add_dmrs_flag = pucchfmt->additionalDMRS!= NULL ?  1 : 0;
            }
            int f3_dmrs_symbols;
            if (pucchres->format.choice.format3->nrofSymbols==4)
              f3_dmrs_symbols = 1<<pucch_pdu->freq_hop_flag;
            else {
              if(pucchres->format.choice.format3->nrofSymbols<10)
                f3_dmrs_symbols = 2;
              else
                f3_dmrs_symbols = 2<<pucch_pdu->add_dmrs_flag;
            }
            pucch_pdu->prb_size = compute_pucch_prb_size(3,pucchres->format.choice.format3->nrofPRBs,
                                                         O_uci+O_sr,O_csi,pucch_Config->format3->choice.setup->maxCodeRate,
                                                         2-pucch_pdu->pi_2bpsk,pucchres->format.choice.format3->nrofSymbols-f3_dmrs_symbols,12);
            pucch_pdu->bit_len_csi_part1 = O_csi;
            break;
          case NR_PUCCH_Resource__format_PR_format4 :
            pucch_pdu->format_type = 4;
            pucch_pdu->nr_of_symbols = pucchres->format.choice.format4->nrofSymbols;
            pucch_pdu->start_symbol_index = pucchres->format.choice.format4->startingSymbolIndex;
            pucch_pdu->pre_dft_occ_len = pucchres->format.choice.format4->occ_Length;
            pucch_pdu->pre_dft_occ_idx = pucchres->format.choice.format4->occ_Index;
            pucch_pdu->data_scrambling_id = pusch_id!= NULL ? *pusch_id : *scc->physCellId;
            if (pucch_Config->format3 == NULL) {
              pucch_pdu->pi_2bpsk = 0;
              pucch_pdu->add_dmrs_flag = 0;
            }
            else {
              pucchfmt = pucch_Config->format3->choice.setup;
              pucch_pdu->pi_2bpsk = pucchfmt->pi2BPSK!= NULL ?  1 : 0;
              pucch_pdu->add_dmrs_flag = pucchfmt->additionalDMRS!= NULL ?  1 : 0;
            }
            pucch_pdu->bit_len_csi_part1 = O_csi;
            break;
          default :
            AssertFatal(1==0,"Undefined PUCCH format \n");
        }
      }
    }
    AssertFatal(res_found==1,"No PUCCH resource found corresponding to id %ld\n",*resource_id);
  }
  else { // this is the default PUCCH configuration, PUCCH format 0 or 1
    LOG_D(NR_MAC,"pucch_acknak: Filling default PUCCH configuration from Tables (r_pucch %d, bwp %p)\n",r_pucch,bwp);
    int rsetindex = *scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup->pucch_ResourceCommon;
    int prboffset = r_pucch/default_pucch_csset[rsetindex];
    int prboffsetm8 = (r_pucch-8)/default_pucch_csset[rsetindex];
    pucch_pdu->prb_start = (r_pucch>>3)==0 ?
                           default_pucch_prboffset[rsetindex] + prboffset:
                           pucch_pdu->bwp_size-1-default_pucch_prboffset[rsetindex]-prboffsetm8;
    pucch_pdu->rnti = rnti;
    pucch_pdu->freq_hop_flag = 1;
    pucch_pdu->second_hop_prb = (r_pucch>>3)==0?
                                pucch_pdu->bwp_size-1-default_pucch_prboffset[rsetindex]-prboffset:
                                default_pucch_prboffset[rsetindex] + prboffsetm8;
    pucch_pdu->format_type = default_pucch_fmt[rsetindex];
    pucch_pdu->initial_cyclic_shift = r_pucch%default_pucch_csset[rsetindex];
    if (rsetindex==3||rsetindex==7||rsetindex==11) pucch_pdu->initial_cyclic_shift*=6;
    else if (rsetindex==1||rsetindex==2) pucch_pdu->initial_cyclic_shift*=3;
    else pucch_pdu->initial_cyclic_shift*=4;
    pucch_pdu->nr_of_symbols = default_pucch_numbsymb[rsetindex];
    pucch_pdu->start_symbol_index = default_pucch_firstsymb[rsetindex];
    if (pucch_pdu->format_type == 1) pucch_pdu->time_domain_occ_idx = 0; // check this!!
    pucch_pdu->sr_flag = O_sr;
    pucch_pdu->prb_size=1;
  }
}


void prepare_dci(const NR_CellGroupConfig_t *CellGroup,
                 dci_pdu_rel15_t *dci_pdu_rel15,
                 nr_dci_format_t format,
                 int bwp_id) {

  AssertFatal(CellGroup!=NULL,"CellGroup shouldn't be null here\n");

  const NR_BWP_DownlinkDedicated_t *bwpd = NULL;
  const NR_PDSCH_Config_t *pdsch_Config = NULL;
  const NR_PDCCH_Config_t *pdcch_Config = NULL;

  if (bwp_id>0) {
    bwpd=CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1]->bwp_Dedicated;
  }
  else {
    bwpd=CellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP;
  }
  pdsch_Config = (bwpd->pdsch_Config) ? bwpd->pdsch_Config->choice.setup : NULL;
  pdcch_Config = (bwpd->pdcch_Config) ? bwpd->pdcch_Config->choice.setup : NULL;


  switch(format) {
    case NR_UL_DCI_FORMAT_0_1:
      // format indicator
      dci_pdu_rel15->format_indicator = 0;
      // carrier indicator
      if (CellGroup->spCellConfig->spCellConfigDedicated->crossCarrierSchedulingConfig != NULL)
        AssertFatal(1==0,"Cross Carrier Scheduling Config currently not supported\n");
      // supplementary uplink
      if (CellGroup->spCellConfig->spCellConfigDedicated->supplementaryUplink != NULL)
        AssertFatal(1==0,"Supplementary Uplink currently not supported\n");
      // SRS request
      dci_pdu_rel15->srs_request.val = 0;
      dci_pdu_rel15->ulsch_indicator = 1;
      break;
    case NR_DL_DCI_FORMAT_1_1:
      // format indicator
      dci_pdu_rel15->format_indicator = 1;
      // carrier indicator
      if (CellGroup->spCellConfig->spCellConfigDedicated->crossCarrierSchedulingConfig != NULL)
        AssertFatal(1==0,"Cross Carrier Scheduling Config currently not supported\n");
      //vrb to prb mapping
      if (pdsch_Config->vrb_ToPRB_Interleaver==NULL)
        dci_pdu_rel15->vrb_to_prb_mapping.val = 0;
      else
        dci_pdu_rel15->vrb_to_prb_mapping.val = 1;
      //bundling size indicator
      if (pdsch_Config->prb_BundlingType.present == NR_PDSCH_Config__prb_BundlingType_PR_dynamicBundling)
        AssertFatal(1==0,"Dynamic PRB bundling type currently not supported\n");
      //rate matching indicator
      uint16_t msb = (pdsch_Config->rateMatchPatternGroup1==NULL)?0:1;
      uint16_t lsb = (pdsch_Config->rateMatchPatternGroup2==NULL)?0:1;
      dci_pdu_rel15->rate_matching_indicator.val = lsb | (msb<<1);
      // aperiodic ZP CSI-RS trigger
      if (pdsch_Config->aperiodic_ZP_CSI_RS_ResourceSetsToAddModList != NULL)
        AssertFatal(1==0,"Aperiodic ZP CSI-RS currently not supported\n");
      // transmission configuration indication
      if (pdcch_Config->controlResourceSetToAddModList->list.array[0]->tci_PresentInDCI != NULL)
        AssertFatal(1==0,"TCI in DCI currently not supported\n");
      //srs resource set
      if (CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->carrierSwitching!=NULL) {
        NR_SRS_CarrierSwitching_t *cs = CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->carrierSwitching->choice.setup;
        if (cs->srs_TPC_PDCCH_Group!=NULL){
          switch(cs->srs_TPC_PDCCH_Group->present) {
            case NR_SRS_CarrierSwitching__srs_TPC_PDCCH_Group_PR_NOTHING:
              dci_pdu_rel15->srs_request.val = 0;
              break;
            case NR_SRS_CarrierSwitching__srs_TPC_PDCCH_Group_PR_typeA:
              AssertFatal(1==0,"SRS TPC PRCCH group type A currently not supported\n");
              break;
            case NR_SRS_CarrierSwitching__srs_TPC_PDCCH_Group_PR_typeB:
              AssertFatal(1==0,"SRS TPC PRCCH group type B currently not supported\n");
              break;
          }
        }
        else
          dci_pdu_rel15->srs_request.val = 0;
      }
      else
        dci_pdu_rel15->srs_request.val = 0;
    // CBGTI and CBGFI
    if (CellGroup->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig &&
        CellGroup->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup &&
        CellGroup->spCellConfig->spCellConfigDedicated->pdsch_ServingCellConfig->choice.setup->codeBlockGroupTransmission != NULL)
      AssertFatal(1==0,"CBG transmission currently not supported\n");
    break;
  default :
    AssertFatal(1==0,"Prepare dci currently only implemented for 1_1 and 0_1 \n");
  }
}


void fill_dci_pdu_rel15(const NR_ServingCellConfigCommon_t *scc,
                        const NR_CellGroupConfig_t *CellGroup,
                        nfapi_nr_dl_dci_pdu_t *pdcch_dci_pdu,
                        dci_pdu_rel15_t *dci_pdu_rel15,
                        int dci_format,
                        int rnti_type,
                        int N_RB,
                        int bwp_id) {
  uint8_t fsize = 0, pos = 0;

  uint64_t *dci_pdu = (uint64_t *)pdcch_dci_pdu->Payload;
  *dci_pdu=0;
  int dci_size = nr_dci_size(scc->downlinkConfigCommon->initialDownlinkBWP,scc->uplinkConfigCommon->initialUplinkBWP, CellGroup, dci_pdu_rel15, dci_format, rnti_type, N_RB, bwp_id);
  pdcch_dci_pdu->PayloadSizeBits = dci_size;
  AssertFatal(dci_size <= 64, "DCI sizes above 64 bits not yet supported");
  if (dci_format == NR_DL_DCI_FORMAT_1_1 || dci_format == NR_UL_DCI_FORMAT_0_1)
    prepare_dci(CellGroup, dci_pdu_rel15, dci_format, bwp_id);

  /// Payload generation
  switch (dci_format) {
  case NR_DL_DCI_FORMAT_1_0:
    switch (rnti_type) {
    case NR_RNTI_RA:
      // Freq domain assignment
      fsize = (int)ceil(log2((N_RB * (N_RB + 1)) >> 1));
      pos = fsize;
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->frequency_domain_assignment.val & ((1 << fsize) - 1)) << (dci_size - pos));
      LOG_D(NR_MAC,
            "RA_RNTI, size %d frequency-domain assignment %d (%d bits) N_RB_BWP %d=> %d (0x%lx)\n",
            dci_size,dci_pdu_rel15->frequency_domain_assignment.val,
            fsize,
            N_RB,
            dci_size - pos,
            *dci_pdu);
      // Time domain assignment
      pos += 4;
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment.val & 0xf) << (dci_size - pos));
      LOG_D(NR_MAC,
            "time-domain assignment %d  (4 bits)=> %d (0x%lx)\n",
            dci_pdu_rel15->time_domain_assignment.val,
            dci_size - pos,
            *dci_pdu);
      // VRB to PRB mapping
      pos++;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping.val & 0x1) << (dci_size - pos);
      LOG_D(NR_MAC,
            "vrb to prb mapping %d  (1 bits)=> %d (0x%lx)\n",
            dci_pdu_rel15->vrb_to_prb_mapping.val,
            dci_size - pos,
            *dci_pdu);
      // MCS
      pos += 5;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->mcs & 0x1f) << (dci_size - pos);
      LOG_D(NR_MAC, "mcs %d  (5 bits)=> %d (0x%lx)\n", dci_pdu_rel15->mcs, dci_size - pos, *dci_pdu);
      // TB scaling
      pos += 2;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->tb_scaling & 0x3) << (dci_size - pos);
      LOG_D(NR_MAC, "tb_scaling %d  (2 bits)=> %d (0x%lx)\n", dci_pdu_rel15->tb_scaling, dci_size - pos, *dci_pdu);
      break;

    case NR_RNTI_C:
      // indicating a DL DCI format 1bit
      pos++;
      *dci_pdu |= ((uint64_t)1) << (dci_size - pos);
      LOG_D(NR_MAC,
            "DCI1_0 (size %d): Format indicator %d (%d bits) N_RB_BWP %d => %d (0x%lx)\n",
            dci_size,
            dci_pdu_rel15->format_indicator,
            1,
            N_RB,
            dci_size - pos,
            *dci_pdu);
      // Freq domain assignment (275rb >> fsize = 16)
      fsize = (int)ceil(log2((N_RB * (N_RB + 1)) >> 1));
      pos += fsize;
      *dci_pdu |= (((uint64_t)dci_pdu_rel15->frequency_domain_assignment.val & ((1 << fsize) - 1)) << (dci_size - pos));
      LOG_D(NR_MAC,
            "Freq domain assignment %d (%d bits)=> %d (0x%lx)\n",
            dci_pdu_rel15->frequency_domain_assignment.val,
            fsize,
            dci_size - pos,
            *dci_pdu);
      uint16_t is_ra = 1;
      for (int i = 0; i < fsize; i++) {
        if (!((dci_pdu_rel15->frequency_domain_assignment.val >> i) & 1)) {
          is_ra = 0;
          break;
        }
      }
      if (is_ra) { // fsize are all 1  38.212 p86
        // ra_preamble_index 6 bits
        pos += 6;
        *dci_pdu |= ((dci_pdu_rel15->ra_preamble_index & 0x3f) << (dci_size - pos));
        // UL/SUL indicator  1 bit
        pos++;
        *dci_pdu |= (dci_pdu_rel15->ul_sul_indicator.val & 1) << (dci_size - pos);
        // SS/PBCH index  6 bits
        pos += 6;
        *dci_pdu |= ((dci_pdu_rel15->ss_pbch_index & 0x3f) << (dci_size - pos));
        //  prach_mask_index  4 bits
        pos += 4;
        *dci_pdu |= ((dci_pdu_rel15->prach_mask_index & 0xf) << (dci_size - pos));
      } else {
        // Time domain assignment 4bit
        pos += 4;
        *dci_pdu |= ((dci_pdu_rel15->time_domain_assignment.val & 0xf) << (dci_size - pos));
        LOG_D(NR_MAC,
              "Time domain assignment %d (%d bits)=> %d (0x%lx)\n",
              dci_pdu_rel15->time_domain_assignment.val,
              4,
              dci_size - pos,
              *dci_pdu);
        // VRB to PRB mapping  1bit
        pos++;
        *dci_pdu |= (dci_pdu_rel15->vrb_to_prb_mapping.val & 1) << (dci_size - pos);
        LOG_D(NR_MAC,
              "VRB to PRB %d (%d bits)=> %d (0x%lx)\n",
              dci_pdu_rel15->vrb_to_prb_mapping.val,
              1,
              dci_size - pos,
              *dci_pdu);
        // MCS 5bit  //bit over 32, so dci_pdu ++
        pos += 5;
        *dci_pdu |= (dci_pdu_rel15->mcs & 0x1f) << (dci_size - pos);
        LOG_D(NR_MAC, "MCS %d (%d bits)=> %d (0x%lx)\n", dci_pdu_rel15->mcs, 5, dci_size - pos, *dci_pdu);
        // New data indicator 1bit
        pos++;
        *dci_pdu |= (dci_pdu_rel15->ndi & 1) << (dci_size - pos);
        LOG_D(NR_MAC, "NDI %d (%d bits)=> %d (0x%lx)\n", dci_pdu_rel15->ndi, 1, dci_size - pos, *dci_pdu);
        // Redundancy version  2bit
        pos += 2;
        *dci_pdu |= (dci_pdu_rel15->rv & 0x3) << (dci_size - pos);
        LOG_D(NR_MAC, "RV %d (%d bits)=> %d (0x%lx)\n", dci_pdu_rel15->rv, 2, dci_size - pos, *dci_pdu);
        // HARQ process number  4bit
        pos += 4;
        *dci_pdu |= ((dci_pdu_rel15->harq_pid & 0xf) << (dci_size - pos));
        LOG_D(NR_MAC, "HARQ_PID %d (%d bits)=> %d (0x%lx)\n", dci_pdu_rel15->harq_pid, 4, dci_size - pos, *dci_pdu);
        // Downlink assignment index  2bit
        pos += 2;
        *dci_pdu |= ((dci_pdu_rel15->dai[0].val & 3) << (dci_size - pos));
        LOG_D(NR_MAC, "DAI %d (%d bits)=> %d (0x%lx)\n", dci_pdu_rel15->dai[0].val, 2, dci_size - pos, *dci_pdu);
        // TPC command for scheduled PUCCH  2bit
        pos += 2;
        *dci_pdu |= ((dci_pdu_rel15->tpc & 3) << (dci_size - pos));
        LOG_D(NR_MAC, "TPC %d (%d bits)=> %d (0x%lx)\n", dci_pdu_rel15->tpc, 2, dci_size - pos, *dci_pdu);
        // PUCCH resource indicator  3bit
        pos += 3;
        *dci_pdu |= ((dci_pdu_rel15->pucch_resource_indicator & 0x7) << (dci_size - pos));
        LOG_D(NR_MAC,
              "PUCCH RI %d (%d bits)=> %d (0x%lx)\n",
              dci_pdu_rel15->pucch_resource_indicator,
              3,
              dci_size - pos,
              *dci_pdu);
        // PDSCH-to-HARQ_feedback timing indicator 3bit
        pos += 3;
        *dci_pdu |= ((dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val & 0x7) << (dci_size - pos));
        LOG_D(NR_MAC,
              "PDSCH to HARQ TI %d (%d bits)=> %d (0x%lx)\n",
              dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val,
              3,
              dci_size - pos,
              *dci_pdu);
      } // end else
      break;

    case NR_RNTI_P:
      // Short Messages Indicator – 2 bits
      for (int i = 0; i < 2; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->short_messages_indicator >> (1 - i)) & 1) << (dci_size - pos++);
      // Short Messages – 8 bits
      for (int i = 0; i < 8; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->short_messages >> (7 - i)) & 1) << (dci_size - pos++);
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil(log2((N_RB * (N_RB + 1)) >> 1));
      for (int i = 0; i < fsize; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->frequency_domain_assignment.val >> (fsize - i - 1)) & 1) << (dci_size - pos++);
      // Time domain assignment 4 bit
      for (int i = 0; i < 4; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment.val >> (3 - i)) & 1) << (dci_size - pos++);
      // VRB to PRB mapping 1 bit
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping.val & 1) << (dci_size - pos++);
      // MCS 5 bit
      for (int i = 0; i < 5; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs >> (4 - i)) & 1) << (dci_size - pos++);
      // TB scaling 2 bit
      for (int i = 0; i < 2; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->tb_scaling >> (1 - i)) & 1) << (dci_size - pos++);
      break;

    case NR_RNTI_SI:
      pos = 1;
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil(log2((N_RB * (N_RB + 1)) >> 1));
      LOG_D(NR_MAC, "fsize = %i\n", fsize);
      for (int i = 0; i < fsize; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->frequency_domain_assignment.val >> (fsize - i - 1)) & 1) << (dci_size - pos++);
      LOG_D(NR_MAC, "dci_pdu_rel15->frequency_domain_assignment.val = %i\n", dci_pdu_rel15->frequency_domain_assignment.val);
      // Time domain assignment 4 bit
      for (int i = 0; i < 4; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment.val >> (3 - i)) & 1) << (dci_size - pos++);
      LOG_D(NR_MAC, "dci_pdu_rel15->time_domain_assignment.val = %i\n", dci_pdu_rel15->time_domain_assignment.val);
      // VRB to PRB mapping 1 bit
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping.val & 1) << (dci_size - pos++);
      LOG_D(NR_MAC, "dci_pdu_rel15->vrb_to_prb_mapping.val = %i\n", dci_pdu_rel15->vrb_to_prb_mapping.val);
      // MCS 5bit  //bit over 32, so dci_pdu ++
      for (int i = 0; i < 5; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs >> (4 - i)) & 1) << (dci_size - pos++);
      LOG_D(NR_MAC, "dci_pdu_rel15->mcs = %i\n", dci_pdu_rel15->mcs);
      // Redundancy version  2bit
      for (int i = 0; i < 2; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->rv >> (1 - i)) & 1) << (dci_size - pos++);
      LOG_D(NR_MAC, "dci_pdu_rel15->rv = %i\n", dci_pdu_rel15->rv);
      // System information indicator 1bit
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->system_info_indicator&1)<<(dci_size-pos++);
      LOG_D(NR_MAC, "dci_pdu_rel15->system_info_indicator = %i\n", dci_pdu_rel15->system_info_indicator);
      break;

    case NR_RNTI_TC:
      pos = 1;
      // indicating a DL DCI format 1bit
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->format_indicator & 1) << (dci_size - pos++);
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil(log2((N_RB * (N_RB + 1)) >> 1));
      for (int i = 0; i < fsize; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->frequency_domain_assignment.val >> (fsize - i - 1)) & 1) << (dci_size - pos++);
      // Time domain assignment 4 bit
      for (int i = 0; i < 4; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->time_domain_assignment.val >> (3 - i)) & 1) << (dci_size - pos++);
      // VRB to PRB mapping 1 bit
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping.val & 1) << (dci_size - pos++);
      // MCS 5bit  //bit over 32, so dci_pdu ++
      for (int i = 0; i < 5; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->mcs >> (4 - i)) & 1) << (dci_size - pos++);
      // New data indicator 1bit
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->ndi & 1) << (dci_size - pos++);
      // Redundancy version  2bit
      for (int i = 0; i < 2; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->rv >> (1 - i)) & 1) << (dci_size - pos++);
      // HARQ process number  4bit
      for (int i = 0; i < 4; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->harq_pid >> (3 - i)) & 1) << (dci_size - pos++);
      // Downlink assignment index – 2 bits
      for (int i = 0; i < 2; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->dai[0].val >> (1 - i)) & 1) << (dci_size - pos++);
      // TPC command for scheduled PUCCH – 2 bits
      for (int i = 0; i < 2; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->tpc >> (1 - i)) & 1) << (dci_size - pos++);
      // PUCCH resource indicator – 3 bits
      for (int i = 0; i < 3; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->pucch_resource_indicator >> (2 - i)) & 1) << (dci_size - pos++);
      // PDSCH-to-HARQ_feedback timing indicator – 3 bits
      for (int i = 0; i < 3; i++)
        *dci_pdu |= (((uint64_t)dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val >> (2 - i)) & 1) << (dci_size - pos++);

      LOG_D(NR_MAC,"N_RB = %i\n", N_RB);
      LOG_D(NR_MAC,"dci_size = %i\n", dci_size);
      LOG_D(NR_MAC,"fsize = %i\n", fsize);
      LOG_D(NR_MAC,"dci_pdu_rel15->format_indicator = %i\n", dci_pdu_rel15->format_indicator);
      LOG_D(NR_MAC,"dci_pdu_rel15->frequency_domain_assignment.val = %i\n", dci_pdu_rel15->frequency_domain_assignment.val);
      LOG_D(NR_MAC,"dci_pdu_rel15->time_domain_assignment.val = %i\n", dci_pdu_rel15->time_domain_assignment.val);
      LOG_D(NR_MAC,"dci_pdu_rel15->vrb_to_prb_mapping.val = %i\n", dci_pdu_rel15->vrb_to_prb_mapping.val);
      LOG_D(NR_MAC,"dci_pdu_rel15->mcs = %i\n", dci_pdu_rel15->mcs);
      LOG_D(NR_MAC,"dci_pdu_rel15->rv = %i\n", dci_pdu_rel15->rv);
      LOG_D(NR_MAC,"dci_pdu_rel15->harq_pid = %i\n", dci_pdu_rel15->harq_pid);
      LOG_D(NR_MAC,"dci_pdu_rel15->dai[0].val = %i\n", dci_pdu_rel15->dai[0].val);
      LOG_D(NR_MAC,"dci_pdu_rel15->tpc = %i\n", dci_pdu_rel15->tpc);
      LOG_D(NR_MAC,"dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val = %i\n", dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val);

      break;
    }
    break;

  case NR_UL_DCI_FORMAT_0_0:
    switch (rnti_type) {
    case NR_RNTI_C:
      LOG_D(NR_MAC,"Filling format 0_0 DCI for CRNTI (size %d bits, format ind %d)\n",dci_size,dci_pdu_rel15->format_indicator);
      // indicating a UL DCI format 1bit
      pos=1;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->format_indicator & 1) << (dci_size - pos);
      // Freq domain assignment  max 16 bit
      fsize = (int)ceil(log2((N_RB * (N_RB + 1)) >> 1));
      pos+=fsize;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->frequency_domain_assignment.val & ((1 << fsize) - 1)) << (dci_size - pos);
      // Time domain assignment 4bit
      pos += 4;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->time_domain_assignment.val & ((1 << 4) - 1)) << (dci_size - pos);
      // Frequency hopping flag – 1 bit
      pos++;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->frequency_hopping_flag.val & 1) << (dci_size - pos);
      // MCS  5 bit
      pos+=5;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->mcs & 0x1f) << (dci_size - pos);
      // New data indicator 1bit
      pos++;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->ndi & 1) << (dci_size - pos);
      // Redundancy version  2bit
      pos+=2;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->rv & 0x3) << (dci_size - pos);
      // HARQ process number  4bit
      pos+=4;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->harq_pid & 0xf) << (dci_size - pos);
      // TPC command for scheduled PUSCH – 2 bits
      pos+=2;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->tpc & 0x3) << (dci_size - pos);
      // Padding bits
      for (int a = pos; a < 32; a++)
        *dci_pdu |= ((uint64_t)dci_pdu_rel15->padding & 1) << (dci_size - pos++);
      // UL/SUL indicator – 1 bit
      /* commented for now (RK): need to get this from BWP descriptor
      if (cfg->pucch_config.pucch_GroupHopping.value)
        *dci_pdu |=
      ((uint64_t)dci_pdu_rel15->ul_sul_indicator.val&1)<<(dci_size-pos++);
        */

        LOG_D(NR_MAC,"N_RB = %i\n", N_RB);
        LOG_D(NR_MAC,"dci_size = %i\n", dci_size);
        LOG_D(NR_MAC,"fsize = %i\n", fsize);
        LOG_D(NR_MAC,"dci_pdu_rel15->frequency_domain_assignment.val = %i\n", dci_pdu_rel15->frequency_domain_assignment.val);
        LOG_D(NR_MAC,"dci_pdu_rel15->time_domain_assignment.val = %i\n", dci_pdu_rel15->time_domain_assignment.val);
        LOG_D(NR_MAC,"dci_pdu_rel15->frequency_hopping_flag.val = %i\n", dci_pdu_rel15->frequency_hopping_flag.val);
        LOG_D(NR_MAC,"dci_pdu_rel15->mcs = %i\n", dci_pdu_rel15->mcs);
        LOG_D(NR_MAC,"dci_pdu_rel15->ndi = %i\n", dci_pdu_rel15->ndi);
        LOG_D(NR_MAC,"dci_pdu_rel15->rv = %i\n", dci_pdu_rel15->rv);
        LOG_D(NR_MAC,"dci_pdu_rel15->harq_pid = %i\n", dci_pdu_rel15->harq_pid);
        LOG_D(NR_MAC,"dci_pdu_rel15->tpc = %i\n", dci_pdu_rel15->tpc);
        LOG_D(NR_MAC,"dci_pdu_rel15->padding = %i\n", dci_pdu_rel15->padding);
      break;

    case NFAPI_NR_RNTI_TC:
      // indicating a UL DCI format 1bit
      pos=1;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->format_indicator & 1) << (dci_size - pos);
      // Freq domain assignment  max 16 bit
      fsize = (int)ceil(log2((N_RB * (N_RB + 1)) >> 1));
      pos+=fsize;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->frequency_domain_assignment.val & ((1 << fsize) - 1)) << (dci_size - pos);
      // Time domain assignment 4bit
      pos += 4;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->time_domain_assignment.val & ((1 << 4) - 1)) << (dci_size - pos);
      // Frequency hopping flag – 1 bit
      pos++;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->frequency_hopping_flag.val & 1) << (dci_size - pos);
      // MCS  5 bit
      pos+=5;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->mcs & 0x1f) << (dci_size - pos);
      // New data indicator 1bit
      pos++;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->ndi & 1) << (dci_size - pos);
      // Redundancy version  2bit
      pos+=2;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->rv & 0x3) << (dci_size - pos);
      // HARQ process number  4bit
      pos+=4;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->harq_pid & 0xf) << (dci_size - pos);
      // Padding bits
      for (int a = pos; a < 32; a++)
        *dci_pdu |= ((uint64_t)dci_pdu_rel15->padding & 1) << (dci_size - pos++);
      // UL/SUL indicator – 1 bit
      /* commented for now (RK): need to get this from BWP descriptor
      if (cfg->pucch_config.pucch_GroupHopping.value)
        *dci_pdu |=
      ((uint64_t)dci_pdu_rel15->ul_sul_indicator.val&1)<<(dci_size-pos++);
        */
      LOG_D(NR_MAC,"N_RB = %i\n", N_RB);
      LOG_D(NR_MAC,"dci_size = %i\n", dci_size);
      LOG_D(NR_MAC,"fsize = %i\n", fsize);
      LOG_D(NR_MAC,"dci_pdu_rel15->frequency_domain_assignment.val = %i\n", dci_pdu_rel15->frequency_domain_assignment.val);
      LOG_D(NR_MAC,"dci_pdu_rel15->time_domain_assignment.val = %i\n", dci_pdu_rel15->time_domain_assignment.val);
      LOG_D(NR_MAC,"dci_pdu_rel15->frequency_hopping_flag.val = %i\n", dci_pdu_rel15->frequency_hopping_flag.val);
      LOG_D(NR_MAC,"dci_pdu_rel15->mcs = %i\n", dci_pdu_rel15->mcs);
      LOG_D(NR_MAC,"dci_pdu_rel15->ndi = %i\n", dci_pdu_rel15->ndi);
      LOG_D(NR_MAC,"dci_pdu_rel15->rv = %i\n", dci_pdu_rel15->rv);
      LOG_D(NR_MAC,"dci_pdu_rel15->harq_pid = %i\n", dci_pdu_rel15->harq_pid);
      LOG_D(NR_MAC,"dci_pdu_rel15->tpc = %i\n", dci_pdu_rel15->tpc);
      LOG_D(NR_MAC,"dci_pdu_rel15->padding = %i\n", dci_pdu_rel15->padding);

      break;
    }
    break;

  case NR_UL_DCI_FORMAT_0_1:
    switch (rnti_type) {
    case NR_RNTI_C:
      LOG_D(NR_MAC,"Filling NR_UL_DCI_FORMAT_0_1 size %d format indicator %d\n",dci_size,dci_pdu_rel15->format_indicator);
      // Indicating a DL DCI format 1bit
      pos = 1;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->format_indicator & 0x1) << (dci_size - pos);
      // Carrier indicator
      pos += dci_pdu_rel15->carrier_indicator.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->carrier_indicator.val & ((1 << dci_pdu_rel15->carrier_indicator.nbits) - 1)) << (dci_size - pos);
      // UL/SUL Indicator
      pos += dci_pdu_rel15->ul_sul_indicator.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->ul_sul_indicator.val & ((1 << dci_pdu_rel15->ul_sul_indicator.nbits) - 1)) << (dci_size - pos);
      // BWP indicator
      pos += dci_pdu_rel15->bwp_indicator.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->bwp_indicator.val & ((1 << dci_pdu_rel15->bwp_indicator.nbits) - 1)) << (dci_size - pos);
      // Frequency domain resource assignment
      pos += dci_pdu_rel15->frequency_domain_assignment.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->frequency_domain_assignment.val & ((1 << dci_pdu_rel15->frequency_domain_assignment.nbits) - 1)) << (dci_size - pos);
      // Time domain resource assignment
      pos += dci_pdu_rel15->time_domain_assignment.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->time_domain_assignment.val & ((1 << dci_pdu_rel15->time_domain_assignment.nbits) - 1)) << (dci_size - pos);
      // Frequency hopping
      pos += dci_pdu_rel15->frequency_hopping_flag.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->frequency_hopping_flag.val & ((1 << dci_pdu_rel15->frequency_hopping_flag.nbits) - 1)) << (dci_size - pos);
      // MCS 5bit
      pos += 5;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->mcs & 0x1f) << (dci_size - pos);
      // New data indicator 1bit
      pos += 1;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->ndi & 0x1) << (dci_size - pos);
      // Redundancy version  2bit
      pos += 2;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->rv & 0x3) << (dci_size - pos);
      // HARQ process number  4bit
      pos += 4;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->harq_pid & 0xf) << (dci_size - pos);
      // 1st Downlink assignment index
      pos += dci_pdu_rel15->dai[0].nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->dai[0].val & ((1 << dci_pdu_rel15->dai[0].nbits) - 1)) << (dci_size - pos);
      // 2nd Downlink assignment index
      pos += dci_pdu_rel15->dai[1].nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->dai[1].val & ((1 << dci_pdu_rel15->dai[1].nbits) - 1)) << (dci_size - pos);
      // TPC command for scheduled PUSCH  2bit
      pos += 2;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->tpc & 0x3) << (dci_size - pos);
      // SRS resource indicator
      pos += dci_pdu_rel15->srs_resource_indicator.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->srs_resource_indicator.val & ((1 << dci_pdu_rel15->srs_resource_indicator.nbits) - 1)) << (dci_size - pos);
      // Precoding info and n. of layers
      pos += dci_pdu_rel15->precoding_information.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->precoding_information.val & ((1 << dci_pdu_rel15->precoding_information.nbits) - 1)) << (dci_size - pos);
      // Antenna ports
      pos += dci_pdu_rel15->antenna_ports.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->antenna_ports.val & ((1 << dci_pdu_rel15->antenna_ports.nbits) - 1)) << (dci_size - pos);
      // SRS request
      pos += dci_pdu_rel15->srs_request.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->srs_request.val & ((1 << dci_pdu_rel15->srs_request.nbits) - 1)) << (dci_size - pos);
      // CSI request
      pos += dci_pdu_rel15->csi_request.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->csi_request.val & ((1 << dci_pdu_rel15->csi_request.nbits) - 1)) << (dci_size - pos);
      // CBG transmission information
      pos += dci_pdu_rel15->cbgti.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->cbgti.val & ((1 << dci_pdu_rel15->cbgti.nbits) - 1)) << (dci_size - pos);
      // PTRS DMRS association
      pos += dci_pdu_rel15->ptrs_dmrs_association.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->ptrs_dmrs_association.val & ((1 << dci_pdu_rel15->ptrs_dmrs_association.nbits) - 1)) << (dci_size - pos);
      // Beta offset indicator
      pos += dci_pdu_rel15->beta_offset_indicator.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->beta_offset_indicator.val & ((1 << dci_pdu_rel15->beta_offset_indicator.nbits) - 1)) << (dci_size - pos);
      // DMRS sequence initialization
      pos += dci_pdu_rel15->dmrs_sequence_initialization.nbits;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->dmrs_sequence_initialization.val & ((1 << dci_pdu_rel15->dmrs_sequence_initialization.nbits) - 1)) << (dci_size - pos);
      // UL-SCH indicator
      pos += 1;
      *dci_pdu |= ((uint64_t)dci_pdu_rel15->ulsch_indicator & 0x1) << (dci_size - pos);
      break;
    }
    break;

  case NR_DL_DCI_FORMAT_1_1:
    // Indicating a DL DCI format 1bit
    LOG_D(NR_MAC,"Filling Format 1_1 DCI of size %d\n",dci_size);
    pos = 1;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->format_indicator & 0x1) << (dci_size - pos);
    // Carrier indicator
    pos += dci_pdu_rel15->carrier_indicator.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->carrier_indicator.val & ((1 << dci_pdu_rel15->carrier_indicator.nbits) - 1)) << (dci_size - pos);
    // BWP indicator
    pos += dci_pdu_rel15->bwp_indicator.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->bwp_indicator.val & ((1 << dci_pdu_rel15->bwp_indicator.nbits) - 1)) << (dci_size - pos);
    // Frequency domain resource assignment
    pos += dci_pdu_rel15->frequency_domain_assignment.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->frequency_domain_assignment.val & ((1 << dci_pdu_rel15->frequency_domain_assignment.nbits) - 1)) << (dci_size - pos);
    // Time domain resource assignment
    pos += dci_pdu_rel15->time_domain_assignment.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->time_domain_assignment.val & ((1 << dci_pdu_rel15->time_domain_assignment.nbits) - 1)) << (dci_size - pos);
    // VRB-to-PRB mapping
    pos += dci_pdu_rel15->vrb_to_prb_mapping.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->vrb_to_prb_mapping.val & ((1 << dci_pdu_rel15->vrb_to_prb_mapping.nbits) - 1)) << (dci_size - pos);
    // PRB bundling size indicator
    pos += dci_pdu_rel15->prb_bundling_size_indicator.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->prb_bundling_size_indicator.val & ((1 << dci_pdu_rel15->prb_bundling_size_indicator.nbits) - 1)) << (dci_size - pos);
    // Rate matching indicator
    pos += dci_pdu_rel15->rate_matching_indicator.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->rate_matching_indicator.val & ((1 << dci_pdu_rel15->rate_matching_indicator.nbits) - 1)) << (dci_size - pos);
    // ZP CSI-RS trigger
    pos += dci_pdu_rel15->zp_csi_rs_trigger.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->zp_csi_rs_trigger.val & ((1 << dci_pdu_rel15->zp_csi_rs_trigger.nbits) - 1)) << (dci_size - pos);
    // TB1
    // MCS 5bit
    pos += 5;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->mcs & 0x1f) << (dci_size - pos);
    // New data indicator 1bit
    pos += 1;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->ndi & 0x1) << (dci_size - pos);
    // Redundancy version  2bit
    pos += 2;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->rv & 0x3) << (dci_size - pos);
    // TB2
    // MCS 5bit
    pos += dci_pdu_rel15->mcs2.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->mcs2.val & ((1 << dci_pdu_rel15->mcs2.nbits) - 1)) << (dci_size - pos);
    // New data indicator 1bit
    pos += dci_pdu_rel15->ndi2.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->ndi2.val & ((1 << dci_pdu_rel15->ndi2.nbits) - 1)) << (dci_size - pos);
    // Redundancy version  2bit
    pos += dci_pdu_rel15->rv2.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->rv2.val & ((1 << dci_pdu_rel15->rv2.nbits) - 1)) << (dci_size - pos);
    // HARQ process number  4bit
    pos += 4;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->harq_pid & 0xf) << (dci_size - pos);
    // Downlink assignment index
    pos += dci_pdu_rel15->dai[0].nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->dai[0].val & ((1 << dci_pdu_rel15->dai[0].nbits) - 1)) << (dci_size - pos);
    // TPC command for scheduled PUCCH  2bit
    pos += 2;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->tpc & 0x3) << (dci_size - pos);
    // PUCCH resource indicator  3bit
    pos += 3;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->pucch_resource_indicator & 0x7) << (dci_size - pos);
    // PDSCH-to-HARQ_feedback timing indicator
    pos += dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.val & ((1 << dci_pdu_rel15->pdsch_to_harq_feedback_timing_indicator.nbits) - 1)) << (dci_size - pos);
    // Antenna ports
    pos += dci_pdu_rel15->antenna_ports.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->antenna_ports.val & ((1 << dci_pdu_rel15->antenna_ports.nbits) - 1)) << (dci_size - pos);
    // TCI
    pos += dci_pdu_rel15->transmission_configuration_indication.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->transmission_configuration_indication.val & ((1 << dci_pdu_rel15->transmission_configuration_indication.nbits) - 1)) << (dci_size - pos);
    // SRS request
    pos += dci_pdu_rel15->srs_request.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->srs_request.val & ((1 << dci_pdu_rel15->srs_request.nbits) - 1)) << (dci_size - pos);
    // CBG transmission information
    pos += dci_pdu_rel15->cbgti.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->cbgti.val & ((1 << dci_pdu_rel15->cbgti.nbits) - 1)) << (dci_size - pos);
    // CBG flushing out information
    pos += dci_pdu_rel15->cbgfi.nbits;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->cbgfi.val & ((1 << dci_pdu_rel15->cbgfi.nbits) - 1)) << (dci_size - pos);
    // DMRS sequence init
    pos += 1;
    *dci_pdu |= ((uint64_t)dci_pdu_rel15->dmrs_sequence_initialization.val & 0x1) << (dci_size - pos);
  }
  LOG_D(NR_MAC, "DCI has %d bits and the payload is %lx\n", dci_size, *dci_pdu);
}

int get_spf(nfapi_nr_config_request_scf_t *cfg) {

  int mu = cfg->ssb_config.scs_common.value;
  AssertFatal(mu>=0&&mu<4,"Illegal scs %d\n",mu);

  return(10 * (1<<mu));
} 

int to_absslot(nfapi_nr_config_request_scf_t *cfg,int frame,int slot) {

  return(get_spf(cfg)*frame) + slot; 

}

int extract_startSymbol(int startSymbolAndLength) {
  int tmp = startSymbolAndLength/14;
  int tmp2 = startSymbolAndLength%14;

  if (tmp > 0 && tmp < (14-tmp2)) return(tmp2);
  else                            return(13-tmp2);
}

int extract_length(int startSymbolAndLength) {
  int tmp = startSymbolAndLength/14;
  int tmp2 = startSymbolAndLength%14;

  if (tmp > 0 && tmp < (14-tmp2)) return(tmp);
  else                            return(15-tmp2);
}

/*
 * Dump the UL or DL UE_info into LOG_T(MAC)
 */
void dump_nr_list(NR_list_t *listP)
{
  for (int j = listP->head; j >= 0; j = listP->next[j])
    LOG_T(NR_MAC, "NR list node %d => %d\n", j, listP->next[j]);
}

/*
 * Create a new NR_list
 */
void create_nr_list(NR_list_t *list, int len)
{
  list->head = -1;
  list->next = calloc(len, sizeof(*list->next));
  AssertFatal(list, "cannot calloc() memory for NR_list_t->next\n");
  for (int i = 0; i < len; ++i)
    list->next[i] = -1;
  list->tail = -1;
  list->len = len;
}

/*
 * Destroy an NR_list
 */
void destroy_nr_list(NR_list_t *list)
{
  free(list->next);
}

/*
 * Add an ID to an NR_list at the end, traversing the whole list. Note:
 * add_tail_nr_list() is a faster alternative, but this implementation ensures
 * we do not add an existing ID.
 */
void add_nr_list(NR_list_t *listP, int id)
{
  int *cur = &listP->head;
  while (*cur >= 0) {
    AssertFatal(*cur != id, "id %d already in NR_UE_list!\n", id);
    cur = &listP->next[*cur];
  }
  *cur = id;
  if (listP->next[id] < 0)
    listP->tail = id;
}

/*
 * Remove an ID from an NR_list
 */
void remove_nr_list(NR_list_t *listP, int id)
{
  int *cur = &listP->head;
  int *prev = &listP->head;
  while (*cur != -1 && *cur != id) {
    prev = cur;
    cur = &listP->next[*cur];
  }
  AssertFatal(*cur != -1, "ID %d not found in UE_list\n", id);
  int *next = &listP->next[*cur];
  *cur = listP->next[*cur];
  *next = -1;
  listP->tail = *prev >= 0 && listP->next[*prev] >= 0 ? listP->tail : *prev;
}

/*
 * Add an ID to the tail of the NR_list in O(1). Note that there is
 * corresponding remove_tail_nr_list(), as we cannot set the tail backwards and
 * therefore need to go through the whole list (use remove_nr_list())
 */
void add_tail_nr_list(NR_list_t *listP, int id)
{
  int *last = listP->tail < 0 ? &listP->head : &listP->next[listP->tail];
  *last = id;
  listP->next[id] = -1;
  listP->tail = id;
}

/*
 * Add an ID to the front of the NR_list in O(1)
 */
void add_front_nr_list(NR_list_t *listP, int id)
{
  const int ohead = listP->head;
  listP->head = id;
  listP->next[id] = ohead;
  if (listP->tail < 0)
    listP->tail = id;
}

/*
 * Remove an ID from the front of the NR_list in O(1)
 */
void remove_front_nr_list(NR_list_t *listP)
{
  AssertFatal(listP->head >= 0, "Nothing to remove\n");
  const int ohead = listP->head;
  listP->head = listP->next[ohead];
  listP->next[ohead] = -1;
  if (listP->head < 0)
    listP->tail = -1;
}

int find_nr_UE_id(module_id_t mod_idP, rnti_t rntiP)
//------------------------------------------------------------------------------
{
  int UE_id;
  NR_UE_info_t *UE_info = &RC.nrmac[mod_idP]->UE_info;

  for (UE_id = 0; UE_id < MAX_MOBILES_PER_GNB; UE_id++) {
    if (UE_info->active[UE_id]) {
      if (UE_info->rnti[UE_id] == rntiP) {
        return UE_id;
      }
    }
  }

  return -1;
}

void set_Y(int Y[3][160], rnti_t rnti) {
  const int A[3] = {39827, 39829, 39839};
  const int D = 65537;

  Y[0][0] = (A[0] * rnti) % D;
  Y[1][0] = (A[1] * rnti) % D;
  Y[2][0] = (A[2] * rnti) % D;

  for (int s = 1; s < 160; s++) {
    Y[0][s] = (A[0] * Y[0][s - 1]) % D;
    Y[1][s] = (A[1] * Y[1][s - 1]) % D;
    Y[2][s] = (A[2] * Y[2][s - 1]) % D;
  }
}

int find_nr_RA_id(module_id_t mod_idP, int CC_idP, rnti_t rntiP) {
//------------------------------------------------------------------------------
  int RA_id;
  RA_t *ra = (RA_t *) &RC.nrmac[mod_idP]->common_channels[CC_idP].ra[0];

  for (RA_id = 0; RA_id < NB_RA_PROC_MAX; RA_id++) {
    LOG_D(NR_MAC, "Checking RA_id %d for %x : state %d\n",
          RA_id,
          rntiP,
          ra[RA_id].state);

    if (ra[RA_id].state != IDLE && ra[RA_id].rnti == rntiP)
      return RA_id;
  }

  return -1;
}

int get_nrofHARQ_ProcessesForPDSCH(e_NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH n)
{
  switch (n) {
  case NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH_n2:
    return 2;
  case NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH_n4:
    return 4;
  case NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH_n6:
    return 6;
  case NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH_n10:
    return 10;
  case NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH_n12:
    return 12;
  case NR_PDSCH_ServingCellConfig__nrofHARQ_ProcessesForPDSCH_n16:
    return 16;
  default:
    return 8;
  }
}

int get_dl_bwp_id(const NR_ServingCellConfig_t *servingCellConfig)
{
  if (servingCellConfig->firstActiveDownlinkBWP_Id)
    return *servingCellConfig->firstActiveDownlinkBWP_Id;
  else if (servingCellConfig->defaultDownlinkBWP_Id)
    return *servingCellConfig->defaultDownlinkBWP_Id;
  else
    return 1;
}

int get_ul_bwp_id(const NR_ServingCellConfig_t *servingCellConfig)
{
  if (servingCellConfig->uplinkConfig && servingCellConfig->uplinkConfig->firstActiveUplinkBWP_Id)
    return *servingCellConfig->uplinkConfig->firstActiveUplinkBWP_Id;
  else
    return 1;
}

//------------------------------------------------------------------------------
int add_new_nr_ue(module_id_t mod_idP, rnti_t rntiP, NR_CellGroupConfig_t *CellGroup)
{
  NR_ServingCellConfigCommon_t *scc = RC.nrmac[mod_idP]->common_channels[0].ServingCellConfigCommon;
  NR_UE_info_t *UE_info = &RC.nrmac[mod_idP]->UE_info;
  LOG_I(NR_MAC, "[gNB %d] Adding UE with rnti %x (num_UEs %d)\n",
        mod_idP,
        rntiP,
        UE_info->num_UEs);
  dump_nr_list(&UE_info->list);

  for (int i = 0; i < MAX_MOBILES_PER_GNB; i++) {
    if (UE_info->active[i]) {
      LOG_D(NR_MAC,"UE %x is active, skipping\n",rntiP);
      continue;
    }
    int UE_id = i;
    UE_info->num_UEs++;
    UE_info->active[UE_id] = true;
    if (CellGroup) UE_info->Msg4_ACKed[UE_id] = true;
    else           UE_info->Msg4_ACKed[UE_id] = false;
    UE_info->rnti[UE_id] = rntiP;
    UE_info->CellGroup[UE_id] = CellGroup;
    add_nr_list(&UE_info->list, UE_id);
    memset(&UE_info->mac_stats[UE_id], 0, sizeof(NR_mac_stats_t));
    set_Y(UE_info->Y[UE_id], rntiP);
    if (CellGroup && CellGroup->spCellConfig && CellGroup->spCellConfig && CellGroup->spCellConfig->spCellConfigDedicated)
      compute_csi_bitlen (CellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup, UE_info, UE_id, mod_idP);
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    memset(sched_ctrl, 0, sizeof(*sched_ctrl));
    sched_ctrl->set_mcs = TRUE;
    sched_ctrl->lcid_mask = 0;
    if (!get_softmodem_params()->phy_test && !get_softmodem_params()->do_ra && !get_softmodem_params()->sa) {
      sched_ctrl->lcid_mask = 1<<DL_SCH_LCID_DTCH;
    }
    sched_ctrl->ta_frame = 0;
    sched_ctrl->ta_update = 31;
    sched_ctrl->ta_apply = false;
    sched_ctrl->ul_rssi = 0;
    sched_ctrl->pucch_consecutive_dtx_cnt = 0;
    sched_ctrl->pusch_consecutive_dtx_cnt = 0;
    sched_ctrl->ul_failure                = 0;
    /* set illegal time domain allocation to force recomputation of all fields */
    sched_ctrl->pdsch_semi_static.time_domain_allocation = -1;
    sched_ctrl->pusch_semi_static.time_domain_allocation = -1;
    const NR_ServingCellConfig_t *servingCellConfig = CellGroup ? CellGroup->spCellConfig->spCellConfigDedicated : NULL;

    /* Set default BWPs */
    const struct NR_ServingCellConfig__downlinkBWP_ToAddModList *bwpList = servingCellConfig ? servingCellConfig->downlinkBWP_ToAddModList : NULL;
    if (bwpList) AssertFatal(bwpList->list.count == 1,
			     "downlinkBWP_ToAddModList has %d BWP!\n",
			     bwpList->list.count);
    const int bwp_id = 1;
    sched_ctrl->active_bwp = bwpList ? bwpList->list.array[bwp_id - 1] : NULL;
    const int target_ss = sched_ctrl->active_bwp ? NR_SearchSpace__searchSpaceType_PR_ue_Specific : NR_SearchSpace__searchSpaceType_PR_common;
    sched_ctrl->search_space = get_searchspace(scc,
                                               sched_ctrl->active_bwp ? sched_ctrl->active_bwp->bwp_Dedicated : NULL,
                                               target_ss);
    sched_ctrl->coreset = get_coreset(mod_idP, scc,
                                      sched_ctrl->active_bwp ? (void*)sched_ctrl->active_bwp->bwp_Dedicated : NULL,
                                      sched_ctrl->search_space, target_ss);
    const struct NR_UplinkConfig__uplinkBWP_ToAddModList *ubwpList = servingCellConfig ? servingCellConfig->uplinkConfig->uplinkBWP_ToAddModList : NULL;
    if (ubwpList) AssertFatal(ubwpList->list.count == 1,
			      "uplinkBWP_ToAddModList has %d BWP!\n",
			      ubwpList->list.count);
    sched_ctrl->active_ubwp = ubwpList ? ubwpList->list.array[bwp_id - 1] : NULL;

    /* get Number of HARQ processes for this UE */
    if (servingCellConfig) AssertFatal(servingCellConfig->pdsch_ServingCellConfig->present == NR_SetupRelease_PDSCH_ServingCellConfig_PR_setup,
				       "no pdsch-ServingCellConfig found for UE %d\n",
				       UE_id);
    const NR_PDSCH_ServingCellConfig_t *pdsch = servingCellConfig ? servingCellConfig->pdsch_ServingCellConfig->choice.setup : NULL;
    const int nrofHARQ = pdsch ? (pdsch->nrofHARQ_ProcessesForPDSCH ?
				  get_nrofHARQ_ProcessesForPDSCH(*pdsch->nrofHARQ_ProcessesForPDSCH) : 8) : 8;
    // add all available DL HARQ processes for this UE
    create_nr_list(&sched_ctrl->available_dl_harq, nrofHARQ);
    for (int harq = 0; harq < nrofHARQ; harq++)
      add_tail_nr_list(&sched_ctrl->available_dl_harq, harq);
    create_nr_list(&sched_ctrl->feedback_dl_harq, nrofHARQ);
    create_nr_list(&sched_ctrl->retrans_dl_harq, nrofHARQ);

    // add all available UL HARQ processes for this UE
    create_nr_list(&sched_ctrl->available_ul_harq, 16);
    for (int harq = 0; harq < 16; harq++)
      add_tail_nr_list(&sched_ctrl->available_ul_harq, harq);
    create_nr_list(&sched_ctrl->feedback_ul_harq, 16);
    create_nr_list(&sched_ctrl->retrans_ul_harq, 16);
    LOG_D(NR_MAC, "[gNB %d] Add NR UE_id %d : rnti %x\n",
          mod_idP,
          UE_id,
          rntiP);
    dump_nr_list(&UE_info->list);
    return (UE_id);
  }

  // printf("MAC: cannot add new UE for rnti %x\n", rntiP);
  LOG_E(NR_MAC, "error in add_new_ue(), could not find space in UE_info, Dumping UE list\n");
  dump_nr_list(&UE_info->list);
  return -1;
}

/* hack data to remove UE in the phy */
int rnti_to_remove[10];
volatile int rnti_to_remove_count;
pthread_mutex_t rnti_to_remove_mutex = PTHREAD_MUTEX_INITIALIZER;

void mac_remove_nr_ue(module_id_t mod_id, rnti_t rnti)
{
  int UE_id;
  int i;
  int cc_id;
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;

  for (i = 0; i < MAX_MOBILES_PER_GNB; i++) {

    if (UE_info->active[i] != TRUE)
      continue;
    if (UE_info->rnti[i] != rnti)
      continue;

    /* UE found, remove it */
    UE_id = i;

    UE_info->num_UEs--;
    UE_info->active[UE_id] = FALSE;
    UE_info->rnti[UE_id] = 0;
    remove_nr_list(&UE_info->list, UE_id);
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    destroy_nr_list(&sched_ctrl->available_dl_harq);
    destroy_nr_list(&sched_ctrl->feedback_dl_harq);
    destroy_nr_list(&sched_ctrl->retrans_dl_harq);
    destroy_nr_list(&sched_ctrl->available_ul_harq);
    destroy_nr_list(&sched_ctrl->feedback_ul_harq);
    destroy_nr_list(&sched_ctrl->retrans_ul_harq);
    LOG_I(NR_MAC, "[gNB %d] Remove NR UE_id %d : rnti %x\n",
          mod_id,
          UE_id,
          rnti);

    /* hack to remove UE in the phy */
    if (pthread_mutex_lock(&rnti_to_remove_mutex)) exit(1);
    if (rnti_to_remove_count == 10) exit(1);
    rnti_to_remove[rnti_to_remove_count] = rnti;
    LOG_W(NR_MAC, "to remove in mac rnti_to_remove[%d] = 0x%04x\n", rnti_to_remove_count, rnti);
    rnti_to_remove_count++;
    if (pthread_mutex_unlock(&rnti_to_remove_mutex)) exit(1);
  }

  /* clear RA process(es?) associated to the UE */
  for (cc_id = 0; cc_id < NFAPI_CC_MAX; cc_id++) {
    NR_COMMON_channels_t *cc = &RC.nrmac[mod_id]->common_channels[cc_id];
    for (i = 0; i < NR_NB_RA_PROC_MAX; i++) {
      if (cc->ra[i].rnti == rnti) {
        LOG_D(NR_MAC, "free RA process %d for rnti %d\n", i, rnti);
        /* is it enough? */
        cc->ra[i].cfra  = false;
        cc->ra[i].rnti  = 0;
        cc->ra[i].crnti = 0;
      }
    }
  }
}

void nr_mac_remove_ra_rnti(module_id_t mod_id, rnti_t rnti) {
  // Hack to remove UE in the phy (following the same procedure as in function mac_remove_nr_ue)
  if (pthread_mutex_lock(&rnti_to_remove_mutex)) exit(1);
  if (rnti_to_remove_count == 10) exit(1);
  rnti_to_remove[rnti_to_remove_count] = rnti;
  LOG_W(NR_MAC, "to remove in mac rnti_to_remove[%d] = 0x%04x\n", rnti_to_remove_count, rnti);
  rnti_to_remove_count++;
  if (pthread_mutex_unlock(&rnti_to_remove_mutex)) exit(1);
}

uint8_t nr_get_tpc(int target, uint8_t cqi, int incr) {
  // al values passed to this function are x10
  int snrx10 = (cqi*5) - 640;
  if (snrx10 > target + incr) return 0; // decrease 1dB
  if (snrx10 < target - (3*incr)) return 3; // increase 3dB
  if (snrx10 < target - incr) return 2; // increase 1dB
  LOG_D(NR_MAC,"tpc : target %d, snrx10 %d\n",target,snrx10);
  return 1; // no change
}


void get_pdsch_to_harq_feedback(int Mod_idP,
                                int UE_id,
                                int bwp_id,
                                NR_SearchSpace__searchSpaceType_PR ss_type,
                                int *max_fb_time,
                                uint8_t *pdsch_to_harq_feedback) {

  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  NR_CellGroupConfig_t *CellGroup = UE_info->CellGroup[UE_id];
  NR_BWP_DownlinkDedicated_t *bwpd=NULL;
  NR_BWP_UplinkDedicated_t *ubwpd=NULL;

  if (ss_type == NR_SearchSpace__searchSpaceType_PR_ue_Specific) {
    AssertFatal(CellGroup!=NULL,"Cellgroup is not defined for UE_id %d\n",UE_id);
    AssertFatal(CellGroup->spCellConfig!=NULL,"Cellgroup->spCellConfig is null\n");
    AssertFatal(CellGroup->spCellConfig->spCellConfigDedicated!=NULL,"CellGroup->spCellConfig->spCellConfigDedicated is null\n");
  }
  if (bwp_id>0) {
    AssertFatal(CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList!=NULL,
                "CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList is null\n");
    AssertFatal(CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList!=NULL,
                "CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList is null\n");
    AssertFatal(CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count >= bwp_id,
                "CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count %d < bwp_id %d\n",
 	 	CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count,bwp_id);
    AssertFatal(CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count >= bwp_id,
                "CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count %d < bwp_id %d\n",
                CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.count,bwp_id);

    bwpd = CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1]->bwp_Dedicated;
    ubwpd = CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_id-1]->bwp_Dedicated;
  }
  else if (CellGroup) { // this is an initialBWP
    AssertFatal((bwpd=CellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP)!=NULL,
                "CellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP is null\n");
    AssertFatal((ubwpd=CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP)!=NULL,
                "CellGroup->spCellConfig->spCellConfigDedicated->uplnikConfig->initialUplinkBWP is null\n");
  }
  NR_SearchSpace_t *ss=NULL;

  // common search type uses DCI format 1_0
  if (ss_type == NR_SearchSpace__searchSpaceType_PR_common) {
    for (int i=0; i<8; i++) {
      pdsch_to_harq_feedback[i] = i+1;
      if(pdsch_to_harq_feedback[i]>*max_fb_time)
        *max_fb_time = pdsch_to_harq_feedback[i];
    }
  }
  else {

    // searching for a ue specific search space
    int found=0;
    AssertFatal(bwpd->pdcch_Config!=NULL,"bwpd->pdcch_Config is null\n");
    AssertFatal(bwpd->pdcch_Config->choice.setup->searchSpacesToAddModList!=NULL,
                "bwpd->pdcch_Config->choice.setup->searchSpacesToAddModList is null\n");
    for (int i=0;i<bwpd->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count;i++) {
      ss=bwpd->pdcch_Config->choice.setup->searchSpacesToAddModList->list.array[i];
      AssertFatal(ss->controlResourceSetId != NULL,"ss->controlResourceSetId is null\n");
      AssertFatal(ss->searchSpaceType != NULL,"ss->searchSpaceType is null\n");
      if (ss->searchSpaceType->present == ss_type) {
       found=1;
       break;
      }
    }
    AssertFatal(found==1,"Couldn't find a ue specific searchspace\n");


    if (ss->searchSpaceType->choice.ue_Specific->dci_Formats == NR_SearchSpace__searchSpaceType__ue_Specific__dci_Formats_formats0_0_And_1_0) {
      for (int i=0; i<8; i++) {
        pdsch_to_harq_feedback[i] = i+1;
        if(pdsch_to_harq_feedback[i]>*max_fb_time)
          *max_fb_time = pdsch_to_harq_feedback[i];
      }
    }
    else {
      AssertFatal(ubwpd!=NULL,"ubwpd shouldn't be null here\n");
      if(ubwpd->pucch_Config->choice.setup->dl_DataToUL_ACK != NULL) {
        for (int i=0; i<8; i++) {
          pdsch_to_harq_feedback[i] = *ubwpd->pucch_Config->choice.setup->dl_DataToUL_ACK->list.array[i];
          if(pdsch_to_harq_feedback[i]>*max_fb_time)
            *max_fb_time = pdsch_to_harq_feedback[i];
        }
      }
      else
        AssertFatal(0==1,"There is no allocated dl_DataToUL_ACK for pdsch to harq feedback\n");
    }
  }
}


void nr_csirs_scheduling(int Mod_idP,
                         frame_t frame,
                         sub_frame_t slot,
                         int n_slots_frame){

  int CC_id = 0;
  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  NR_list_t *UE_list = &UE_info->list;
  gNB_MAC_INST *gNB_mac = RC.nrmac[Mod_idP];
  uint16_t *vrb_map = gNB_mac->common_channels[CC_id].vrb_map;

  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {

    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    NR_CellGroupConfig_t *CellGroup = UE_info->CellGroup[UE_id];

    if (!CellGroup || !CellGroup->spCellConfig || !CellGroup->spCellConfig->spCellConfigDedicated ||
	      !CellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig) continue;

    NR_CSI_MeasConfig_t *csi_measconfig = CellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup;

    if (csi_measconfig->nzp_CSI_RS_ResourceToAddModList != NULL) {

      NR_NZP_CSI_RS_Resource_t *nzpcsi;
      int period, offset;

      nfapi_nr_dl_tti_request_body_t *dl_req = &gNB_mac->DL_req[CC_id].dl_tti_request_body;
      NR_BWP_Downlink_t *bwp=CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[sched_ctrl->active_bwp->bwp_Id-1];

      for (int id = 0; id < csi_measconfig->nzp_CSI_RS_ResourceToAddModList->list.count; id++){
        nzpcsi = csi_measconfig->nzp_CSI_RS_ResourceToAddModList->list.array[id];
        NR_CSI_RS_ResourceMapping_t  resourceMapping = nzpcsi->resourceMapping;
        csi_period_offset(NULL,nzpcsi,&period,&offset);

        if((frame*n_slots_frame+slot-offset)%period == 0) {

          LOG_I(MAC,"Scheduling CSI-RS in frame %d slot %d\n",frame,slot);

          nfapi_nr_dl_tti_request_pdu_t *dl_tti_csirs_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
          memset((void*)dl_tti_csirs_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
          dl_tti_csirs_pdu->PDUType = NFAPI_NR_DL_TTI_CSI_RS_PDU_TYPE;
          dl_tti_csirs_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_csi_rs_pdu));

          nfapi_nr_dl_tti_csi_rs_pdu_rel15_t *csirs_pdu_rel15 = &dl_tti_csirs_pdu->csi_rs_pdu.csi_rs_pdu_rel15;

          csirs_pdu_rel15->bwp_size  = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
          csirs_pdu_rel15->bwp_start = NRRIV2PRBOFFSET(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
          csirs_pdu_rel15->subcarrier_spacing = bwp->bwp_Common->genericParameters.subcarrierSpacing;
          if (bwp->bwp_Common->genericParameters.cyclicPrefix)
            csirs_pdu_rel15->cyclic_prefix = *bwp->bwp_Common->genericParameters.cyclicPrefix;
          else
            csirs_pdu_rel15->cyclic_prefix = 0;

          csirs_pdu_rel15->start_rb = resourceMapping.freqBand.startingRB;
          csirs_pdu_rel15->nr_of_rbs = resourceMapping.freqBand.nrofRBs;
          csirs_pdu_rel15->csi_type = 1; // NZP-CSI-RS
          csirs_pdu_rel15->symb_l0 = resourceMapping.firstOFDMSymbolInTimeDomain;
          if (resourceMapping.firstOFDMSymbolInTimeDomain2)
            csirs_pdu_rel15->symb_l1 = *resourceMapping.firstOFDMSymbolInTimeDomain2;
          csirs_pdu_rel15->cdm_type = resourceMapping.cdm_Type;
          csirs_pdu_rel15->freq_density = resourceMapping.density.present;
          if ((resourceMapping.density.present == NR_CSI_RS_ResourceMapping__density_PR_dot5)
              && (resourceMapping.density.choice.dot5 == NR_CSI_RS_ResourceMapping__density__dot5_evenPRBs))
            csirs_pdu_rel15->freq_density--;
          csirs_pdu_rel15->scramb_id = nzpcsi->scramblingID;
          csirs_pdu_rel15->power_control_offset = nzpcsi->powerControlOffset + 8;
          if (nzpcsi->powerControlOffsetSS)
            csirs_pdu_rel15->power_control_offset_ss = *nzpcsi->powerControlOffsetSS;
          else
            csirs_pdu_rel15->power_control_offset_ss = 1; // 0 dB
          switch(resourceMapping.frequencyDomainAllocation.present){
            case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row1:
              csirs_pdu_rel15->row = 1;
              csirs_pdu_rel15->freq_domain = ((resourceMapping.frequencyDomainAllocation.choice.row1.buf[0])>>4)&0x0f;
              for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                vrb_map[rb+csirs_pdu_rel15->bwp_start] |= (1 << csirs_pdu_rel15->symb_l0);
              break;
            case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row2:
              csirs_pdu_rel15->row = 2;
              csirs_pdu_rel15->freq_domain = (((resourceMapping.frequencyDomainAllocation.choice.row2.buf[1]>>4)&0x0f) |
                                             ((resourceMapping.frequencyDomainAllocation.choice.row2.buf[0]<<8)&0xff0));
              for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                vrb_map[rb+csirs_pdu_rel15->bwp_start] |= (1 << csirs_pdu_rel15->symb_l0);
              break;
            case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row4:
              csirs_pdu_rel15->row = 4;
              csirs_pdu_rel15->freq_domain = ((resourceMapping.frequencyDomainAllocation.choice.row4.buf[0])>>5)&0x07;
              for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                vrb_map[rb+csirs_pdu_rel15->bwp_start] |= (1 << csirs_pdu_rel15->symb_l0);
              break;
            case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_other:
              csirs_pdu_rel15->freq_domain = ((resourceMapping.frequencyDomainAllocation.choice.other.buf[0])>>2)&0x3f;
              // determining the row of table 7.4.1.5.3-1 in 38.211
              switch(resourceMapping.nrofPorts){
                case NR_CSI_RS_ResourceMapping__nrofPorts_p1:
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p2:
                  csirs_pdu_rel15->row = 3;
                  for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                    vrb_map[rb+csirs_pdu_rel15->bwp_start] |= (1 << csirs_pdu_rel15->symb_l0);
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p4:
                  csirs_pdu_rel15->row = 5;
                  for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                    vrb_map[rb+csirs_pdu_rel15->bwp_start] |= ((1 << csirs_pdu_rel15->symb_l0) | (2 << csirs_pdu_rel15->symb_l0));
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p8:
                  if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm4_FD2_TD2) {
                    csirs_pdu_rel15->row = 8;
                    for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                      vrb_map[rb+csirs_pdu_rel15->bwp_start] |= ((1 << csirs_pdu_rel15->symb_l0) | (2 << csirs_pdu_rel15->symb_l0));
                  }
                  else{
                    int num_k = 0;
                    for (int k=0; k<6; k++)
                      num_k+=(((csirs_pdu_rel15->freq_domain)>>k)&0x01);
                    if(num_k==4) {
                      csirs_pdu_rel15->row = 6;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb+csirs_pdu_rel15->bwp_start] |= (1 << csirs_pdu_rel15->symb_l0);
                    }
                    else {
                      csirs_pdu_rel15->row = 7;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb+csirs_pdu_rel15->bwp_start] |= ((1 << csirs_pdu_rel15->symb_l0) | (2 << csirs_pdu_rel15->symb_l0));
                    }
                  }
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p12:
                  if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm4_FD2_TD2) {
                    csirs_pdu_rel15->row = 10;
                    for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                      vrb_map[rb+csirs_pdu_rel15->bwp_start] |= ((1 << csirs_pdu_rel15->symb_l0) | (2 << csirs_pdu_rel15->symb_l0));
                  }
                  else {
                    csirs_pdu_rel15->row = 9;
                    for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                      vrb_map[rb+csirs_pdu_rel15->bwp_start] |= (1 << csirs_pdu_rel15->symb_l0);
                  }
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p16:
                  if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm4_FD2_TD2)
                    csirs_pdu_rel15->row = 12;
                  else
                    csirs_pdu_rel15->row = 11;
                  for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                    vrb_map[rb+csirs_pdu_rel15->bwp_start] |= ((1 << csirs_pdu_rel15->symb_l0) | (2 << csirs_pdu_rel15->symb_l0));
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p24:
                  if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm4_FD2_TD2) {
                    csirs_pdu_rel15->row = 14;
                    for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                      vrb_map[rb+csirs_pdu_rel15->bwp_start] |= ((3 << csirs_pdu_rel15->symb_l0) | (3 << csirs_pdu_rel15->symb_l1));
                  }
                  else{
                    if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm8_FD2_TD4) {
                      csirs_pdu_rel15->row = 15;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb+csirs_pdu_rel15->bwp_start] |= (7 << csirs_pdu_rel15->symb_l0);
                    }
                    else {
                      csirs_pdu_rel15->row = 13;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb+csirs_pdu_rel15->bwp_start] |= ((3 << csirs_pdu_rel15->symb_l0) | (3 << csirs_pdu_rel15->symb_l1));
                    }
                  }
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p32:
                  if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm4_FD2_TD2) {
                    csirs_pdu_rel15->row = 17;
                    for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                      vrb_map[rb+csirs_pdu_rel15->bwp_start] |= ((3 << csirs_pdu_rel15->symb_l0) | (3 << csirs_pdu_rel15->symb_l1));
                  }
                  else{
                    if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm8_FD2_TD4) {
                      csirs_pdu_rel15->row = 18;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb+csirs_pdu_rel15->bwp_start] |= (7 << csirs_pdu_rel15->symb_l0);
                    }
                    else {
                      csirs_pdu_rel15->row = 16;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb+csirs_pdu_rel15->bwp_start] |= ((3 << csirs_pdu_rel15->symb_l0) | (3 << csirs_pdu_rel15->symb_l1));
                    }
                  }
                  break;
              default:
                AssertFatal(1==0,"Invalid number of ports in CSI-RS resource\n");
              }
              break;
          default:
            AssertFatal(1==0,"Invalid freqency domain allocation in CSI-RS resource\n");
          }
          dl_req->nPDUs++;
        }
      }
    }
  }
}

bool find_free_CCE(module_id_t module_id,
                   sub_frame_t slot,
                   int UE_id){
  NR_UE_sched_ctrl_t *sched_ctrl = &RC.nrmac[module_id]->UE_info.UE_sched_ctrl[UE_id];
  uint8_t nr_of_candidates;
  for (int i=0; i<5; i++) {
    // for now taking the lowest value among the available aggregation levels
    find_aggregation_candidates(&sched_ctrl->aggregation_level,
                                &nr_of_candidates,
                                sched_ctrl->search_space,
                                1<<i);
    if(nr_of_candidates>0) break;
  }
  AssertFatal(nr_of_candidates>0,"nr_of_candidates is 0\n");
  const int cid = sched_ctrl->coreset->controlResourceSetId;
  const uint16_t Y = RC.nrmac[module_id]->UE_info.Y[UE_id][cid][slot];
  const int m = RC.nrmac[module_id]->UE_info.num_pdcch_cand[UE_id][cid];
  if (UE_id >= 0) LOG_D(NR_MAC,"calling allocate_nr_CCEs with L %d, nr_of_candidates %d, Y %x\n",sched_ctrl->aggregation_level,nr_of_candidates,Y);
  sched_ctrl->cce_index = allocate_nr_CCEs(RC.nrmac[module_id],
                                           sched_ctrl->active_bwp,
                                           sched_ctrl->coreset,
                                           sched_ctrl->aggregation_level,
                                           Y,
                                           m,
                                           nr_of_candidates);
  if (sched_ctrl->cce_index < 0)
    return false;

  RC.nrmac[module_id]->UE_info.num_pdcch_cand[UE_id][cid]++;
  return true;
}


/*void fill_nfapi_coresets_and_searchspaces(NR_CellGroupConfig_t *cg,
					  nfapi_nr_coreset_t *coreset,
					  nfapi_nr_search_space_t *search_space) {

  nfapi_nr_coreset_t *cs;
  nfapi_nr_search_space_t *ss;
  NR_ServingCellConfigCommon_t *scc=cg->spCellConfig->reconfigurationWithSync->spCellConfigCommon;
  AssertFatal(cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
	      "downlinkBWP_ToAddModList has %d BWP!\n",
	      cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);

  NR_BWP_Downlink_t *bwp=cg->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[0];
  struct NR_PDCCH_Config__controlResourceSetToAddModList *coreset_list = bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList;
  AssertFatal(coreset_list->list.count>0,
	      "cs list has 0 elements\n");
  for (int i=0;i<coreset_list->list.count;i++) {
    NR_ControlResourceSet_t *coreset_i=coreset_list->list.array[i];
    cs = coreset + coreset_i->controlResourceSetId;
      
    cs->coreset_id = coreset_i->controlResourceSetId;
    AssertFatal(coreset_i->frequencyDomainResources.size <=8 && coreset_i->frequencyDomainResources.size>0,
		"coreset_i->frequencyDomainResources.size=%d\n",
		(int)coreset_i->frequencyDomainResources.size);
  
    for (int f=0;f<coreset_i->frequencyDomainResources.size;f++)
      ((uint8_t*)&cs->frequency_domain_resources)[coreset_i->frequencyDomainResources.size-1-f]=coreset_i->frequencyDomainResources.buf[f];
    
    cs->frequency_domain_resources>>=coreset_i->frequencyDomainResources.bits_unused;
    
    cs->duration = coreset_i->duration;
    // Need to add information about TCI_StateIDs

    if (coreset_i->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_nonInterleaved)
      cs->cce_reg_mapping_type = NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED;
    else {
      cs->cce_reg_mapping_type = NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED;

      if (coreset_i->cce_REG_MappingType.choice.interleaved->reg_BundleSize==NR_ControlResourceSet__cce_REG_MappingType__interleaved__reg_BundleSize_n6)
	cs->reg_bundle_size = 6;
      else cs->reg_bundle_size = 2+coreset_i->cce_REG_MappingType.choice.interleaved->reg_BundleSize;

      if (coreset_i->cce_REG_MappingType.choice.interleaved->interleaverSize==NR_ControlResourceSet__cce_REG_MappingType__interleaved__interleaverSize_n6)
	cs->interleaver_size = 6;
      else cs->interleaver_size = 2+coreset_i->cce_REG_MappingType.choice.interleaved->interleaverSize;

      if (coreset_i->cce_REG_MappingType.choice.interleaved->shiftIndex)
	cs->shift_index = *coreset_i->cce_REG_MappingType.choice.interleaved->shiftIndex;
      else cs->shift_index = 0;
    }
    
    if (coreset_i->precoderGranularity == NR_ControlResourceSet__precoderGranularity_sameAsREG_bundle)
      cs->precoder_granularity = NFAPI_NR_CSET_SAME_AS_REG_BUNDLE;
    else cs->precoder_granularity = NFAPI_NR_CSET_ALL_CONTIGUOUS_RBS;
    if (coreset_i->tci_PresentInDCI == NULL) cs->tci_present_in_dci = 0;
    else                                     cs->tci_present_in_dci = 1;

    if (coreset_i->tci_PresentInDCI == NULL) cs->dmrs_scrambling_id = 0;
    else                                     cs->dmrs_scrambling_id = *coreset_i->tci_PresentInDCI;
  }

  struct NR_PDCCH_ConfigCommon__commonSearchSpaceList *commonSearchSpaceList = bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList;
  AssertFatal(commonSearchSpaceList->list.count>0,
	      "common SearchSpace list has 0 elements\n");
  // Common searchspace list
  for (int i=0;i<commonSearchSpaceList->list.count;i++) {
    NR_SearchSpace_t *searchSpace_i=commonSearchSpaceList->list.array[i];  
    ss=search_space + searchSpace_i->searchSpaceId;
    if (searchSpace_i->controlResourceSetId) ss->coreset_id = *searchSpace_i->controlResourceSetId;
    switch(searchSpace_i->monitoringSlotPeriodicityAndOffset->present) {
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL1;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL2;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl2;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl4:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL4;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl4;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL5;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl5;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl8:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL8;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl8;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl10:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL10;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl10;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl16:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL16;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl16;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl20:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL20;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl20;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl40:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL40;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl40;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl80:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL80;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl80;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl160:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL160;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl160;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl320:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL320;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl320;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl640:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL640;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl640;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1280:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL1280;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl1280;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2560:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL2560;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl2560;
      break;
    default:
      AssertFatal(1==0,"Shouldn't get here\n");
      break;    
    }
    if (searchSpace_i->duration) ss->duration = *searchSpace_i->duration;
    else                         ss->duration = 1;


    AssertFatal(searchSpace_i->monitoringSymbolsWithinSlot->size == 2,
		"ss_i->monitoringSymbolsWithinSlot = %d != 2\n",
		(int)searchSpace_i->monitoringSymbolsWithinSlot->size);
    ((uint8_t*)&ss->monitoring_symbols_in_slot)[1] = searchSpace_i->monitoringSymbolsWithinSlot->buf[0];
    ((uint8_t*)&ss->monitoring_symbols_in_slot)[0] = searchSpace_i->monitoringSymbolsWithinSlot->buf[1];

    AssertFatal(searchSpace_i->nrofCandidates!=NULL,"searchSpace_%d->nrofCandidates is null\n",(int)searchSpace_i->searchSpaceId);
    if (searchSpace_i->nrofCandidates->aggregationLevel1 == NR_SearchSpace__nrofCandidates__aggregationLevel1_n8)
      ss->number_of_candidates[0] = 8;
    else ss->number_of_candidates[0] = searchSpace_i->nrofCandidates->aggregationLevel1;
    if (searchSpace_i->nrofCandidates->aggregationLevel2 == NR_SearchSpace__nrofCandidates__aggregationLevel2_n8)
      ss->number_of_candidates[1] = 8;
    else ss->number_of_candidates[1] = searchSpace_i->nrofCandidates->aggregationLevel2;
    if (searchSpace_i->nrofCandidates->aggregationLevel4 == NR_SearchSpace__nrofCandidates__aggregationLevel4_n8)
      ss->number_of_candidates[2] = 8;
    else ss->number_of_candidates[2] = searchSpace_i->nrofCandidates->aggregationLevel4;
    if (searchSpace_i->nrofCandidates->aggregationLevel8 == NR_SearchSpace__nrofCandidates__aggregationLevel8_n8)
      ss->number_of_candidates[3] = 8;
    else ss->number_of_candidates[3] = searchSpace_i->nrofCandidates->aggregationLevel8;
    if (searchSpace_i->nrofCandidates->aggregationLevel16 == NR_SearchSpace__nrofCandidates__aggregationLevel16_n8)
      ss->number_of_candidates[4] = 8;
    else ss->number_of_candidates[4] = searchSpace_i->nrofCandidates->aggregationLevel16;      

    AssertFatal(searchSpace_i->searchSpaceType->present==NR_SearchSpace__searchSpaceType_PR_common,
		"searchspace %d is not common\n",(int)searchSpace_i->searchSpaceId);
    AssertFatal(searchSpace_i->searchSpaceType->choice.common!=NULL,
		"searchspace %d common is null\n",(int)searchSpace_i->searchSpaceId);
    ss->search_space_type = NFAPI_NR_SEARCH_SPACE_TYPE_COMMON;
    if (searchSpace_i->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0)
      ss->css_formats_0_0_and_1_0 = 1;
    if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_0) {
      ss->css_format_2_0 = 1;
      // add aggregation info
    }
    if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_1)
      ss->css_format_2_1 = 1;
    if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_2)
      ss->css_format_2_2 = 1;
    if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_3)
      ss->css_format_2_3 = 1;
  }

  struct NR_PDCCH_Config__searchSpacesToAddModList *dedicatedSearchSpaceList = bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList;
  AssertFatal(dedicatedSearchSpaceList->list.count>0,
	      "Dedicated Search Space list has 0 elements\n");
  // Dedicated searchspace list
  for (int i=0;i<dedicatedSearchSpaceList->list.count;i++) {
    NR_SearchSpace_t *searchSpace_i=dedicatedSearchSpaceList->list.array[i];  
    ss=search_space + searchSpace_i->searchSpaceId;
    ss->search_space_id = searchSpace_i->searchSpaceId;
    if (searchSpace_i->controlResourceSetId) ss->coreset_id = *searchSpace_i->controlResourceSetId;
    switch(searchSpace_i->monitoringSlotPeriodicityAndOffset->present) {
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL1;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL2;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl2;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl4:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL4;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl4;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl5:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL5;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl5;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl8:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL8;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl8;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl10:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL10;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl10;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl16:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL16;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl16;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl20:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL20;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl20;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl40:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL40;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl40;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl80:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL80;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl80;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl160:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL160;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl160;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl320:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL320;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl320;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl640:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL640;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl640;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1280:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL1280;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl1280;
      break;
    case NR_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl2560:
      ss->slot_monitoring_periodicity = NFAPI_NR_SS_PERIODICITY_SL2560;
      ss->slot_monitoring_offset = searchSpace_i->monitoringSlotPeriodicityAndOffset->choice.sl2560;
      break;
    default:
      AssertFatal(1==0,"Shouldn't get here\n");
      break;    
    }
    if (searchSpace_i->duration) ss->duration = *searchSpace_i->duration;
    else                         ss->duration = 1;
    
    
    AssertFatal(searchSpace_i->monitoringSymbolsWithinSlot->size == 2,
		"ss_i->monitoringSymbolsWithinSlot = %d != 2\n",
		(int)searchSpace_i->monitoringSymbolsWithinSlot->size);
    ((uint8_t*)&ss->monitoring_symbols_in_slot)[1] = searchSpace_i->monitoringSymbolsWithinSlot->buf[0];
    ((uint8_t*)&ss->monitoring_symbols_in_slot)[0] = searchSpace_i->monitoringSymbolsWithinSlot->buf[1];
    
    AssertFatal(searchSpace_i->nrofCandidates!=NULL,"searchSpace_%d->nrofCandidates is null\n",(int)searchSpace_i->searchSpaceId);
    if (searchSpace_i->nrofCandidates->aggregationLevel1 == NR_SearchSpace__nrofCandidates__aggregationLevel1_n8)
      ss->number_of_candidates[0] = 8;
    else ss->number_of_candidates[0] = searchSpace_i->nrofCandidates->aggregationLevel1;
    if (searchSpace_i->nrofCandidates->aggregationLevel2 == NR_SearchSpace__nrofCandidates__aggregationLevel2_n8)
      ss->number_of_candidates[1] = 8;
    else ss->number_of_candidates[1] = searchSpace_i->nrofCandidates->aggregationLevel2;
    if (searchSpace_i->nrofCandidates->aggregationLevel4 == NR_SearchSpace__nrofCandidates__aggregationLevel4_n8)
      ss->number_of_candidates[2] = 8;
    else ss->number_of_candidates[2] = searchSpace_i->nrofCandidates->aggregationLevel4;
    if (searchSpace_i->nrofCandidates->aggregationLevel8 == NR_SearchSpace__nrofCandidates__aggregationLevel8_n8)
      ss->number_of_candidates[3] = 8;
    else ss->number_of_candidates[3] = searchSpace_i->nrofCandidates->aggregationLevel8;
    if (searchSpace_i->nrofCandidates->aggregationLevel16 == NR_SearchSpace__nrofCandidates__aggregationLevel16_n8)
      ss->number_of_candidates[4] = 8;
    else ss->number_of_candidates[4] = searchSpace_i->nrofCandidates->aggregationLevel16;      
    
    if (searchSpace_i->searchSpaceType->present==NR_SearchSpace__searchSpaceType_PR_ue_Specific && searchSpace_i->searchSpaceType->choice.ue_Specific!=NULL) {
      
      ss->search_space_type = NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC;
      
      ss->uss_dci_formats = searchSpace_i->searchSpaceType->choice.ue_Specific-> dci_Formats;
      
    } else if (searchSpace_i->searchSpaceType->present==NR_SearchSpace__searchSpaceType_PR_common && searchSpace_i->searchSpaceType->choice.common!=NULL) {
      ss->search_space_type = NFAPI_NR_SEARCH_SPACE_TYPE_COMMON;
      
      if (searchSpace_i->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0)
	ss->css_formats_0_0_and_1_0 = 1;
      if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_0) {
	ss->css_format_2_0 = 1;
	// add aggregation info
      }
      if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_1)
	ss->css_format_2_1 = 1;
      if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_2)
	ss->css_format_2_2 = 1;
      if (searchSpace_i->searchSpaceType->choice.common->dci_Format2_3)
	ss->css_format_2_3 = 1;
    }
  }
}
*/
