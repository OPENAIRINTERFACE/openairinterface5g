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

/* TODO REMOVE_DU_RRC: the RRC in the DU is a hack and should be taken out in the future */
#include "RRC/LTE/rrc_extern.h"
#include "RRC/NR/nr_rrc_extern.h"
#include "RRC/NR/rrc_gNB_UE_context.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "RRC/NR/MESSAGES/asn1_msg.h"

#include "intertask_interface.h"

#include "T.h"
#include "NR_PDCCH-ConfigCommon.h"
#include "NR_ControlResourceSet.h"
#include "NR_SearchSpace.h"

#include "nfapi_nr_interface.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_gNB_SCHEDULER 1

#include "common/ran_context.h"

//#define DEBUG_DCI

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

// CQI TABLES (10 times the value in 214 to adequately compare with R)
// Table 1 (38.214 5.2.2.1-2)
uint16_t cqi_table1[16][2] = {{0,0},{2,780},{2,1200},{2,1930},{2,3080},{2,4490},{2,6020},{4,3780},
                              {4,4900},{4,6160},{6,4660},{6,5670},{6,6660},{6,7720},{6,8730},{6,9480}};

// Table 2 (38.214 5.2.2.1-3)
uint16_t cqi_table2[16][2] = {{0,0},{2,780},{2,1930},{2,4490},{4,3780},{4,4900},{4,6160},{6,4660},
                              {6,5670},{6,6660},{6,7720},{6,8730},{8,7110},{8,7970},{8,8850},{8,9480}};

// Table 2 (38.214 5.2.2.1-4)
uint16_t cqi_table3[16][2] = {{0,0},{2,300},{2,500},{2,780},{2,1200},{2,1930},{2,3080},{2,4490},
                              {2,6020},{4,3780},{4,4900},{4,6160},{6,4660},{6,5670},{6,6660},{6,7720}};


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

uint16_t set_pm_index(NR_UE_sched_ctrl_t *sched_ctrl,
                      int layers,
                      int N1, int N2,
                      int xp_pdsch_antenna_ports,
                      int codebook_mode) {

  int antenna_ports = (N1*N2)<<1;
  if (xp_pdsch_antenna_ports == 1 &&
      antenna_ports>1)
    return 0; //identity matrix (basic 5G configuration handled by PMI report is with XP antennas)

  int x1 = sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.pmi_x1;
  int x2 = sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.pmi_x2;
  LOG_D(NR_MAC,"PMI report: x1 %d x2 %d\n",x1,x2);

  sched_ctrl->set_pmi = false;

  if (antenna_ports == 2)
    return x2;
  else
    AssertFatal(1==0,"More than 2 antenna ports not yet supported\n");
}

uint8_t get_mcs_from_cqi(int mcs_table, int cqi_table, int cqi_idx)
{
  if (cqi_idx <= 0) {
    LOG_E(NR_MAC, "invalid cqi_idx %d, default to MCS 9\n", cqi_idx);
    return 9;
  }

  if (mcs_table != cqi_table) {
    LOG_E(NR_MAC, "indices of CQI (%d) and MCS (%d) tables don't correspond yet\n", cqi_table, mcs_table);
    return 9;
  }

  uint16_t target_coderate, target_qm;
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
  const int max_mcs = mcs_table == 1 ? 27 : 28;
  for (int i = 0; i <= max_mcs; i++) {
    const int R = nr_get_code_rate_dl(i, mcs_table);
    const int Qm = nr_get_Qm_dl(i, mcs_table);
    if (Qm == target_qm && target_coderate <= R)
      return i;
  }

  LOG_E(NR_MAC, "could not find maximum MCS from cqi_idx %d, default to 9\n", cqi_idx);
  return 9;
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

NR_ControlResourceSet_t *get_coreset(gNB_MAC_INST *nrmac,
                                     NR_ServingCellConfigCommon_t *scc,
                                     void *bwp,
                                     NR_SearchSpace_t *ss,
                                     NR_SearchSpace__searchSpaceType_PR ss_type) {

  NR_ControlResourceSetId_t coreset_id = *ss->controlResourceSetId;

  if (ss_type == NR_SearchSpace__searchSpaceType_PR_common) { // common search space
    NR_ControlResourceSet_t *coreset;
    if(coreset_id == 0) {
      coreset =  nrmac->sched_ctrlCommon->coreset; // this is coreset 0
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

  int n = 0;
  if(bwp_Dedicated)
    n = bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count;
  else
    n = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList->list.count;

  for (int i=0;i<n;i++) {
    NR_SearchSpace_t *ss = NULL;
    if(bwp_Dedicated)
      ss = bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.array[i];
    else
      ss = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList->list.array[i];
    AssertFatal(ss->controlResourceSetId != NULL, "ss->controlResourceSetId is null\n");
    AssertFatal(ss->searchSpaceType != NULL, "ss->searchSpaceType is null\n");
    if (ss->searchSpaceType->present == target_ss) {
      return ss;
    }
  }
  AssertFatal(0, "Couldn't find an adequate searchspace bwp_Dedicated %p\n",bwp_Dedicated);
}

NR_sched_pdcch_t set_pdcch_structure(gNB_MAC_INST *gNB_mac,
                                     NR_SearchSpace_t *ss,
                                     NR_ControlResourceSet_t *coreset,
                                     NR_ServingCellConfigCommon_t *scc,
                                     NR_BWP_t *bwp,
                                     NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config) {

  int sps;
  NR_sched_pdcch_t pdcch;

  AssertFatal(*ss->controlResourceSetId == coreset->controlResourceSetId,
              "coreset id in SS %ld does not correspond to the one in coreset %ld",
              *ss->controlResourceSetId, coreset->controlResourceSetId);

  if (bwp) { // This is not for SIB1
    if(coreset->controlResourceSetId == 0){
      pdcch.BWPSize  = gNB_mac->cset0_bwp_size;
      pdcch.BWPStart = gNB_mac->cset0_bwp_start;
    }
    else {
      pdcch.BWPSize  = NRRIV2BW(bwp->locationAndBandwidth, MAX_BWP_SIZE);
      pdcch.BWPStart = NRRIV2PRBOFFSET(bwp->locationAndBandwidth, MAX_BWP_SIZE);
    }
    pdcch.SubcarrierSpacing = bwp->subcarrierSpacing;
    pdcch.CyclicPrefix = (bwp->cyclicPrefix==NULL) ? 0 : *bwp->cyclicPrefix;

    //AssertFatal(pdcch_scs==kHz15, "PDCCH SCS above 15kHz not allowed if a symbol above 2 is monitored");
    sps = bwp->cyclicPrefix == NULL ? 14 : 12;
  }
  else {
    AssertFatal(type0_PDCCH_CSS_config!=NULL,"type0_PDCCH_CSS_config is null,bwp %p\n",bwp);
    pdcch.BWPSize = type0_PDCCH_CSS_config->num_rbs;
    pdcch.BWPStart = type0_PDCCH_CSS_config->cset_start_rb;
    pdcch.SubcarrierSpacing = type0_PDCCH_CSS_config->scs_pdcch;
    pdcch.CyclicPrefix = 0;
    sps = 14;
  }

  AssertFatal(ss->monitoringSymbolsWithinSlot!=NULL,"ss->monitoringSymbolsWithinSlot is null\n");
  AssertFatal(ss->monitoringSymbolsWithinSlot->buf!=NULL,"ss->monitoringSymbolsWithinSlot->buf is null\n");

  // for SPS=14 8 MSBs in positions 13 downto 6
  uint16_t monitoringSymbolsWithinSlot = (ss->monitoringSymbolsWithinSlot->buf[0]<<(sps-8)) |
                                         (ss->monitoringSymbolsWithinSlot->buf[1]>>(16-sps));

  for (int i=0; i<sps; i++) {
    if ((monitoringSymbolsWithinSlot>>(sps-1-i))&1) {
      pdcch.StartSymbolIndex=i;
      break;
    }
  }

  pdcch.DurationSymbols = coreset->duration;

  //cce-REG-MappingType
  pdcch.CceRegMappingType = coreset->cce_REG_MappingType.present == NR_ControlResourceSet__cce_REG_MappingType_PR_interleaved?
    NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED : NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED;

  if (pdcch.CceRegMappingType == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
    pdcch.RegBundleSize = (coreset->cce_REG_MappingType.choice.interleaved->reg_BundleSize ==
                                NR_ControlResourceSet__cce_REG_MappingType__interleaved__reg_BundleSize_n6) ? 6 : (2+coreset->cce_REG_MappingType.choice.interleaved->reg_BundleSize);
    pdcch.InterleaverSize = (coreset->cce_REG_MappingType.choice.interleaved->interleaverSize ==
                                  NR_ControlResourceSet__cce_REG_MappingType__interleaved__interleaverSize_n6) ? 6 : (2+coreset->cce_REG_MappingType.choice.interleaved->interleaverSize);
    AssertFatal(scc->physCellId != NULL,"scc->physCellId is null\n");
    pdcch.ShiftIndex = coreset->cce_REG_MappingType.choice.interleaved->shiftIndex != NULL ? *coreset->cce_REG_MappingType.choice.interleaved->shiftIndex : *scc->physCellId;
  }
  else {
    pdcch.RegBundleSize = 6;
    pdcch.InterleaverSize = 0;
    pdcch.ShiftIndex = 0;
  }

  int N_rb = 0; // nb of rbs of coreset per symbol
  for (int i=0;i<6;i++) {
    for (int t=0;t<8;t++) {
      N_rb+=((coreset->frequencyDomainResources.buf[i]>>t)&1);
    }
  }
  pdcch.n_rb = N_rb*=6; // each bit of frequencyDomainResources represents 6 PRBs

  return pdcch;
}


int find_pdcch_candidate(gNB_MAC_INST *mac,
                         int cc_id,
                         int aggregation,
                         int nr_of_candidates,
                         NR_sched_pdcch_t *pdcch,
                         NR_ControlResourceSet_t *coreset,
                         uint32_t Y){

  uint16_t *vrb_map = mac->common_channels[cc_id].vrb_map;
  const int N_ci = 0;

  const int N_rb = pdcch->n_rb;  // nb of rbs of coreset per symbol
  const int N_symb = coreset->duration; // nb of coreset symbols
  const int N_regs = N_rb*N_symb; // nb of REGs per coreset
  const int N_cces = N_regs / NR_NB_REG_PER_CCE; // nb of cces in coreset
  const int R = pdcch->InterleaverSize;
  const int L = pdcch->RegBundleSize;
  const int C = R>0 ? N_regs/(L*R) : 0;
  const int B_rb = L/N_symb; // nb of RBs occupied by each REG bundle

  // loop over all the available candidates
  // this implements TS 38.211 Sec. 7.3.2.2
  for(int m=0; m<nr_of_candidates; m++) { // loop over candidates
    bool taken = false; // flag if the resource for a given candidate are taken
    int first_cce = aggregation * (( Y + CEILIDIV((m*N_cces),(aggregation*nr_of_candidates)) + N_ci ) % CEILIDIV(N_cces,aggregation));
    LOG_D(NR_MAC,"Candidate %d of %d first_cce %d (L %d N_cces %d Y %d)\n", m, nr_of_candidates, first_cce, aggregation, N_cces, Y);
    for (int j=first_cce; (j<first_cce+aggregation) && !taken; j++) { // loop over CCEs
      for (int k=6*j/L; (k<(6*j/L+6/L)) && !taken; k++) { // loop over REG bundles
        int f = cce_to_reg_interleaving(R, k, pdcch->ShiftIndex, C, L, N_regs);
        for(int rb=0; rb<B_rb; rb++) { // loop over the RBs of the bundle
          if(vrb_map[pdcch->BWPStart + f*B_rb + rb]&SL_to_bitmap(pdcch->StartSymbolIndex,N_symb)) {
            taken = true;
            break;
          }
        }
      }
    }
    if(!taken)
      return first_cce;
  }
  return -1;
}

void fill_pdcch_vrb_map(gNB_MAC_INST *mac,
                        int CC_id,
                        NR_sched_pdcch_t *pdcch,
                        int first_cce,
                        int aggregation){

  uint16_t *vrb_map = mac->common_channels[CC_id].vrb_map;

  int N_rb = pdcch->n_rb; // nb of rbs of coreset per symbol
  int L = pdcch->RegBundleSize;
  int R = pdcch->InterleaverSize;
  int n_shift = pdcch->ShiftIndex;
  int N_symb = pdcch->DurationSymbols;
  int N_regs = N_rb*N_symb; // nb of REGs per coreset
  int B_rb = L/N_symb; // nb of RBs occupied by each REG bundle
  int C = R>0 ? N_regs/(L*R) : 0;

  for (int j=first_cce; j<first_cce+aggregation; j++) { // loop over CCEs
    for (int k=6*j/L; k<(6*j/L+6/L); k++) { // loop over REG bundles
      int f = cce_to_reg_interleaving(R, k, n_shift, C, L, N_regs);
      for(int rb=0; rb<B_rb; rb++) // loop over the RBs of the bundle
        vrb_map[pdcch->BWPStart + f*B_rb + rb] |= SL_to_bitmap(pdcch->StartSymbolIndex, N_symb);
    }
  }
}

bool nr_find_nb_rb(uint16_t Qm,
                   uint16_t R,
                   uint8_t nrOfLayers,
                   uint16_t nb_symb_sch,
                   uint16_t nb_dmrs_prb,
                   uint32_t bytes,
                   uint16_t nb_rb_min,
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
  *nb_rb = nb_rb_min;
  *tbs = nr_compute_tbs(Qm, R, *nb_rb, nb_symb_sch, nb_dmrs_prb, 0, 0, nrOfLayers) >> 3;
  if (bytes <= *tbs)
    return true;

  /* perform binary search to allocate all bytes within a TBS up to nb_rb_max
   * RBs */
  int hi = nb_rb_max;
  int lo = nb_rb_min;
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

void nr_set_pdsch_semi_static(const NR_UE_DL_BWP_t *dl_bwp,
                              const NR_ServingCellConfigCommon_t *scc,
                              int tda,
                              uint8_t layers,
                              NR_UE_sched_ctrl_t *sched_ctrl,
                              NR_pdsch_semi_static_t *ps)
{
  bool reset_dmrs = false;

  NR_PDSCH_Config_t *pdsch_Config = dl_bwp->pdsch_Config;
  LOG_D(NR_MAC,"tda %d, ps->time_domain_allocation %d,layers %d, ps->nrOfLayers %d, pdsch_config %p\n",tda,ps->time_domain_allocation,layers,ps->nrOfLayers,pdsch_Config);
  reset_dmrs = true;
  ps->time_domain_allocation = tda;
  NR_PDSCH_TimeDomainResourceAllocationList_t *tdaList = dl_bwp->tdaList;
  AssertFatal(tda < tdaList->list.count, "time_domain_allocation %d>=%d\n", tda, tdaList->list.count);
  ps->mapping_type = tdaList->list.array[tda]->mappingType;
  if (pdsch_Config) {
    if (ps->mapping_type == NR_PDSCH_TimeDomainResourceAllocation__mappingType_typeB)
      ps->dmrsConfigType = pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeB->choice.setup->dmrs_Type != NULL;
    else
      ps->dmrsConfigType = pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type != NULL;
  }
  else
    ps->dmrsConfigType = NFAPI_NR_DMRS_TYPE1;
  const int startSymbolAndLength = tdaList->list.array[tda]->startSymbolAndLength;
  SLIV2SL(startSymbolAndLength, &ps->startSymbolIndex, &ps->nrOfSymbols);

  if (dl_bwp->dci_format == NR_DL_DCI_FORMAT_1_0) {
    if (ps->nrOfSymbols == 2)
      ps->numDmrsCdmGrpsNoData = 1;
    else
      ps->numDmrsCdmGrpsNoData = 2;
    ps->dmrs_ports_id = 0;
    ps->frontloaded_symb = 1;
    ps->nrOfLayers = 1;
  }
  else {
    LOG_D(NR_MAC,"checking layers\n");
    if (ps->nrOfLayers != layers || ps->numDmrsCdmGrpsNoData == 0) {
      reset_dmrs = true;
      ps->nrOfLayers = layers;
      set_dl_dmrs_ports(ps);
    }
  }

  ps->N_PRB_DMRS = ps->numDmrsCdmGrpsNoData * (ps->dmrsConfigType == NFAPI_NR_DMRS_TYPE1 ? 6 : 4);

  if (reset_dmrs) {
    ps->dl_dmrs_symb_pos = fill_dmrs_mask(pdsch_Config, scc ? scc->dmrs_TypeA_Position : 0, ps->nrOfSymbols, ps->startSymbolIndex, ps->mapping_type, ps->frontloaded_symb);
    ps->N_DMRS_SLOT = get_num_dmrs(ps->dl_dmrs_symb_pos);
  }
  LOG_D(NR_MAC,"Filling dmrs info, ps->N_PRB_DMRS %d, ps->dl_dmrs_symb_pos %x, ps->N_DMRS_SLOT %d\n",ps->N_PRB_DMRS,ps->dl_dmrs_symb_pos,ps->N_DMRS_SLOT);
}

void nr_set_pusch_semi_static(const NR_UE_UL_BWP_t *ul_bwp,
                              const NR_ServingCellConfigCommon_t *scc,
                              int tda,
                              uint8_t nrOfLayers,
                              NR_pusch_semi_static_t *ps) {

  ps->time_domain_allocation = tda;

  const int startSymbolAndLength = ul_bwp->tdaList->list.array[tda]->startSymbolAndLength;
  SLIV2SL(startSymbolAndLength,
          &ps->startSymbolIndex,
          &ps->nrOfSymbols);

  ps->nrOfLayers = nrOfLayers;
  // TODO setting of cdm groups with no data to be redone for MIMO
  if (ul_bwp->transform_precoding || nrOfLayers<3)
    ps->num_dmrs_cdm_grps_no_data = (ul_bwp->dci_format == NR_UL_DCI_FORMAT_0_1) ? 1 : (ps->nrOfSymbols == 2 ? 1 : 2);
  else
    ps->num_dmrs_cdm_grps_no_data = 2;

  /* DMRS calculations */
  ps->mapping_type = ul_bwp->tdaList->list.array[tda]->mappingType;
  ps->NR_DMRS_UplinkConfig = ul_bwp->pusch_Config ?
    (ps->mapping_type == NR_PUSCH_TimeDomainResourceAllocation__mappingType_typeA ?
     ul_bwp->pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup :
     ul_bwp->pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup) : NULL;
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
      ? ps->num_dmrs_cdm_grps_no_data * 6
      : ps->num_dmrs_cdm_grps_no_data * 4;
}

#define BLER_UPDATE_FRAME 10
#define BLER_FILTER 0.9f
int get_mcs_from_bler(const NR_bler_options_t *bler_options,
                      const NR_mac_dir_stats_t *stats,
                      NR_bler_stats_t *bler_stats,
                      int max_mcs,
                      frame_t frame)
{
  /* first call: everything is zero. Initialize to sensible default */
  if (bler_stats->last_frame == 0 && bler_stats->mcs == 0) {
    bler_stats->last_frame = frame;
    bler_stats->mcs = 9;
    bler_stats->bler = (bler_options->lower + bler_options->upper) / 2.0f;
  }
  int diff = frame - bler_stats->last_frame;
  if (diff < 0) // wrap around
    diff += 1024;

  max_mcs = min(max_mcs, bler_options->max_mcs);
  const uint8_t old_mcs = min(bler_stats->mcs, max_mcs);
  if (diff < BLER_UPDATE_FRAME)
    return old_mcs; // no update

  // last update is longer than x frames ago
  const int dtx = (int)(stats->rounds[0] - bler_stats->rounds[0]);
  const int dretx = (int)(stats->rounds[1] - bler_stats->rounds[1]);
  const float bler_window = dtx > 0 ? (float) dretx / dtx : bler_stats->bler;
  bler_stats->bler = BLER_FILTER * bler_stats->bler + (1 - BLER_FILTER) * bler_window;

  int new_mcs = old_mcs;
  if (bler_stats->bler < bler_options->lower && old_mcs < max_mcs && dtx > 9)
    new_mcs += 1;
  else if ((bler_stats->bler > bler_options->upper && old_mcs > 6) // above threshold
      || (dtx <= 3 && old_mcs > 9))                                // no activity
    new_mcs -= 1;
  // else we are within threshold boundaries

  bler_stats->last_frame = frame;
  bler_stats->mcs = new_mcs;
  memcpy(bler_stats->rounds, stats->rounds, sizeof(stats->rounds));
  LOG_D(MAC, "frame %4d MCS %d -> %d (dtx %d, dretx %d, BLER wnd %.3f avg %.6f)\n",
        frame, old_mcs, new_mcs, dtx, dretx, bler_window, bler_stats->bler);
  return new_mcs;
}

void nr_configure_css_dci_initial(nfapi_nr_dl_tti_pdcch_pdu_rel15_t* pdcch_pdu,
				  nr_scs_e scs_common,
				  nr_scs_e pdcch_scs,
				  frequency_range_t freq_range,
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

void config_uldci(const NR_SIB1_t *sib1,
                  const NR_ServingCellConfigCommon_t *scc,
                  const nfapi_nr_pusch_pdu_t *pusch_pdu,
                  dci_pdu_rel15_t *dci_pdu_rel15,
                  int time_domain_assignment,
                  uint8_t tpc,
                  NR_UE_UL_BWP_t *ul_bwp) {

  int bwp_id = ul_bwp->bwp_id;
  nr_dci_format_t dci_format = ul_bwp->dci_format;

  dci_pdu_rel15->frequency_domain_assignment.val =
      PRBalloc_to_locationandbandwidth0(pusch_pdu->rb_size, pusch_pdu->rb_start, ul_bwp->BWPSize);
  dci_pdu_rel15->time_domain_assignment.val = time_domain_assignment;
  dci_pdu_rel15->frequency_hopping_flag.val = pusch_pdu->frequency_hopping;
  dci_pdu_rel15->mcs = pusch_pdu->mcs_index;
  dci_pdu_rel15->ndi = pusch_pdu->pusch_data.new_data_indicator;
  dci_pdu_rel15->rv = pusch_pdu->pusch_data.rv_index;
  dci_pdu_rel15->harq_pid = pusch_pdu->pusch_data.harq_process_id;
  dci_pdu_rel15->tpc = tpc;
  NR_PUSCH_Config_t *pusch_Config = ul_bwp->pusch_Config;

  if (pusch_Config) AssertFatal(pusch_Config->resourceAllocation == NR_PUSCH_Config__resourceAllocation_resourceAllocationType1,
			"Only frequency resource allocation type 1 is currently supported\n");
  switch (dci_format) {
    case NR_UL_DCI_FORMAT_0_0:
      dci_pdu_rel15->format_indicator = 0;
      break;
    case NR_UL_DCI_FORMAT_0_1:
      LOG_D(NR_MAC,"Configuring DCI Format 0_1\n");
      dci_pdu_rel15->dai[0].val = 0; //TODO
      // bwp indicator as per table 7.3.1.1.2-1 in 38.212
      dci_pdu_rel15->bwp_indicator.val = ul_bwp->n_ul_bwp < 4 ? bwp_id : bwp_id - 1;
      // SRS resource indicator
      if (pusch_Config &&
          pusch_Config->txConfig != NULL) {
        AssertFatal(*pusch_Config->txConfig == NR_PUSCH_Config__txConfig_codebook,
                    "Non Codebook configuration non supported\n");
        dci_pdu_rel15->srs_resource_indicator.val = 0; // taking resource 0 for SRS
      }
      dci_pdu_rel15->precoding_information.val= 0;
      if (pusch_pdu->nrOfLayers == 2)
        dci_pdu_rel15->precoding_information.val = 4;
      else if (pusch_pdu->nrOfLayers == 4)
        dci_pdu_rel15->precoding_information.val = 11;

      // antenna_ports.val = 0 for transform precoder is disabled, dmrs-Type=1, maxLength=1, Rank=1/2/3/4
      // Antenna Ports
      dci_pdu_rel15->antenna_ports.val = 0;

      // DMRS sequence initialization
      dci_pdu_rel15->dmrs_sequence_initialization.val = pusch_pdu->scid;
      break;
    default :
      AssertFatal(0, "Valid UL formats are 0_0 and 0_1\n");
  }

  LOG_D(NR_MAC,
        "%s() ULDCI type 0 payload: dci_format %d, freq_alloc %d, time_alloc %d, freq_hop_flag %d, precoding_information.val %d antenna_ports.val %d mcs %d tpc %d ndi %d rv %d\n",
        __func__,
        dci_format,
        dci_pdu_rel15->frequency_domain_assignment.val,
        dci_pdu_rel15->time_domain_assignment.val,
        dci_pdu_rel15->frequency_hopping_flag.val,
        dci_pdu_rel15->precoding_information.val,
        dci_pdu_rel15->antenna_ports.val,
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

void nr_configure_pdcch(nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu,
                        NR_ControlResourceSet_t *coreset,
                        bool is_sib1,
                        NR_sched_pdcch_t *pdcch) {


  pdcch_pdu->BWPSize = pdcch->BWPSize;
  pdcch_pdu->BWPStart = pdcch->BWPStart;
  pdcch_pdu->SubcarrierSpacing = pdcch->SubcarrierSpacing;
  pdcch_pdu->CyclicPrefix = pdcch->CyclicPrefix;
  pdcch_pdu->StartSymbolIndex = pdcch->StartSymbolIndex;

  pdcch_pdu->DurationSymbols  = coreset->duration;

  for (int i=0;i<6;i++)
    pdcch_pdu->FreqDomainResource[i] = coreset->frequencyDomainResources.buf[i];

  LOG_D(MAC,"Coreset : BWPstart %d, BWPsize %d, SCS %d, freq %x, , duration %d\n",
        pdcch_pdu->BWPStart,pdcch_pdu->BWPSize,(int)pdcch_pdu->SubcarrierSpacing,(int)coreset->frequencyDomainResources.buf[0],(int)coreset->duration);

  pdcch_pdu->CceRegMappingType = pdcch->CceRegMappingType;
  pdcch_pdu->RegBundleSize = pdcch->RegBundleSize;
  pdcch_pdu->InterleaverSize = pdcch->InterleaverSize;
  pdcch_pdu->ShiftIndex = pdcch->ShiftIndex;

  if(coreset->controlResourceSetId == 0) {
    if(is_sib1)
      pdcch_pdu->CoreSetType = NFAPI_NR_CSET_CONFIG_MIB_SIB1;
    else
      pdcch_pdu->CoreSetType = NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG_CSET_0;
  } else{
    pdcch_pdu->CoreSetType = NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG;
  }

  //precoderGranularity
  pdcch_pdu->precoderGranularity = coreset->precoderGranularity;
}

int nr_get_pucch_resource(NR_ControlResourceSet_t *coreset,
                          NR_PUCCH_Config_t *pucch_Config,
                          int CCEIndex) {
  int r_pucch = -1;
  if(pucch_Config == NULL) {
    int n_rb,rb_offset;
    get_coreset_rballoc(coreset->frequencyDomainResources.buf,&n_rb,&rb_offset);
    const uint16_t N_cce = n_rb * coreset->duration / NR_NB_REG_PER_CCE;
    const int delta_PRI=0;
    r_pucch = ((CCEIndex<<1)/N_cce)+(delta_PRI<<1);
  }
  return r_pucch;
}

// This function configures pucch pdu fapi structure
void nr_configure_pucch(nfapi_nr_pucch_pdu_t* pucch_pdu,
                        NR_ServingCellConfigCommon_t *scc,
                        NR_UE_info_t* UE,
                        uint8_t pucch_resource,
                        uint16_t O_csi,
                        uint16_t O_ack,
                        uint8_t O_sr,
                        int r_pucch) {

  NR_PUCCH_Resource_t *pucchres;
  NR_PUCCH_ResourceSet_t *pucchresset;
  NR_PUCCH_FormatConfig_t *pucchfmt;
  NR_PUCCH_ResourceId_t *resource_id = NULL;
  NR_UE_UL_BWP_t *current_BWP = &UE->current_UL_BWP;

  long *id0 = NULL;
  int n_list, n_set;
  uint16_t N2,N3;
  int res_found = 0;

  pucch_pdu->bit_len_harq = O_ack;
  pucch_pdu->bit_len_csi_part1 = O_csi;

  uint16_t O_uci = O_csi + O_ack;

  NR_PUSCH_Config_t *pusch_Config = current_BWP->pusch_Config;

  long *pusch_id = pusch_Config ? pusch_Config->dataScramblingIdentityPUSCH : NULL;

  if (pusch_Config && pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA != NULL)
    id0 = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup->transformPrecodingDisabled->scramblingID0;
  else if (pusch_Config && pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB != NULL)
    id0 = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->transformPrecodingDisabled->scramblingID0;
  else id0 = scc->physCellId;

  NR_PUCCH_ConfigCommon_t *pucch_ConfigCommon = current_BWP->pucch_ConfigCommon;

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

  pucch_pdu->bwp_size  = current_BWP->BWPSize;
  pucch_pdu->bwp_start = current_BWP->BWPStart;
  pucch_pdu->subcarrier_spacing = current_BWP->scs;
  pucch_pdu->cyclic_prefix = (current_BWP->cyclicprefix==NULL) ? 0 : *current_BWP->cyclicprefix;

  NR_PUCCH_Config_t *pucch_Config = current_BWP->pucch_Config;
  if (r_pucch<0 || pucch_Config){
      LOG_D(NR_MAC,"pucch_acknak: Filling dedicated configuration for PUCCH\n");

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
        pucch_pdu->rnti = UE->rnti;
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
    LOG_D(NR_MAC,"Configure pucch: pucch_pdu->format_type %d pucch_pdu->bit_len_harq %d, pucch->pdu->bit_len_csi %d\n",pucch_pdu->format_type,pucch_pdu->bit_len_harq,pucch_pdu->bit_len_csi_part1);
  }
  else { // this is the default PUCCH configuration, PUCCH format 0 or 1
    LOG_D(NR_MAC,"pucch_acknak: Filling default PUCCH configuration from Tables (r_pucch %d, pucch_Config %p)\n",r_pucch,pucch_Config);
    int rsetindex = *pucch_ConfigCommon->pucch_ResourceCommon;
    int prb_start, second_hop_prb, nr_of_symb, start_symb;
    set_r_pucch_parms(rsetindex,
                      r_pucch,
                      pucch_pdu->bwp_size,
                      &prb_start,
                      &second_hop_prb,
                      &nr_of_symb,
                      &start_symb);

    pucch_pdu->prb_start = prb_start;
    pucch_pdu->rnti = UE->rnti;
    pucch_pdu->freq_hop_flag = 1;
    pucch_pdu->second_hop_prb = second_hop_prb;
    pucch_pdu->format_type = default_pucch_fmt[rsetindex];
    pucch_pdu->initial_cyclic_shift = r_pucch%default_pucch_csset[rsetindex];
    if (rsetindex==3||rsetindex==7||rsetindex==11) pucch_pdu->initial_cyclic_shift*=6;
    else if (rsetindex==1||rsetindex==2) pucch_pdu->initial_cyclic_shift*=4;
    else pucch_pdu->initial_cyclic_shift*=3;
    pucch_pdu->nr_of_symbols = nr_of_symb;
    pucch_pdu->start_symbol_index = start_symb;
    if (pucch_pdu->format_type == 1) pucch_pdu->time_domain_occ_idx = 0; // check this!!
    pucch_pdu->sr_flag = O_sr;
    pucch_pdu->prb_size=1;
  }
}


void set_r_pucch_parms(int rsetindex,
                       int r_pucch,
                       int bwp_size,
                       int *prb_start,
                       int *second_hop_prb,
                       int *nr_of_symbols,
                       int *start_symbol_index) {

  // procedure described in 38.213 section 9.2.1

  int prboffset = r_pucch/default_pucch_csset[rsetindex];
  int prboffsetm8 = (r_pucch-8)/default_pucch_csset[rsetindex];

  *prb_start = (r_pucch>>3)==0 ?
              default_pucch_prboffset[rsetindex] + prboffset:
              bwp_size-1-default_pucch_prboffset[rsetindex]-prboffsetm8;

  *second_hop_prb = (r_pucch>>3)==0?
                   bwp_size-1-default_pucch_prboffset[rsetindex]-prboffset:
                   default_pucch_prboffset[rsetindex] + prboffsetm8;

  *nr_of_symbols = default_pucch_numbsymb[rsetindex];
  *start_symbol_index = default_pucch_firstsymb[rsetindex];
}

void prepare_dci(const NR_CellGroupConfig_t *CellGroup,
                 const NR_UE_DL_BWP_t *current_BWP,
                 const NR_ControlResourceSet_t *coreset,
                 dci_pdu_rel15_t *dci_pdu_rel15,
                 nr_dci_format_t format) {

  AssertFatal(CellGroup!=NULL,"CellGroup shouldn't be null here\n");

  const NR_PDSCH_Config_t *pdsch_Config = current_BWP ? current_BWP->pdsch_Config : NULL;

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
      if (coreset->tci_PresentInDCI != NULL)
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
    if (current_BWP->pdsch_servingcellconfig &&
        current_BWP->pdsch_servingcellconfig->codeBlockGroupTransmission != NULL)
      AssertFatal(1==0,"CBG transmission currently not supported\n");
    break;
  default :
    AssertFatal(1==0,"Prepare dci currently only implemented for 1_1 and 0_1 \n");
  }
}


void fill_dci_pdu_rel15(const NR_ServingCellConfigCommon_t *scc,
                        const NR_CellGroupConfig_t *CellGroup,
                        const NR_UE_DL_BWP_t *current_BWP,
                        nfapi_nr_dl_dci_pdu_t *pdcch_dci_pdu,
                        dci_pdu_rel15_t *dci_pdu_rel15,
                        int dci_format,
                        int rnti_type,
                        int N_RB,
                        int bwp_id,
                        NR_ControlResourceSet_t *coreset,
                        uint16_t cset0_bwp_size) {
  uint8_t fsize = 0, pos = 0;

  uint64_t *dci_pdu = (uint64_t *)pdcch_dci_pdu->Payload;
  *dci_pdu=0;
  NR_ControlResourceSetId_t coreset_id = coreset->controlResourceSetId;
  int dci_size = nr_dci_size(scc->downlinkConfigCommon->initialDownlinkBWP,
                             scc->uplinkConfigCommon->initialUplinkBWP,
                             CellGroup, dci_pdu_rel15, dci_format,
                             rnti_type, N_RB, bwp_id, coreset_id, cset0_bwp_size);
  pdcch_dci_pdu->PayloadSizeBits = dci_size;
  AssertFatal(dci_size <= 64, "DCI sizes above 64 bits not yet supported");
  if (dci_format == NR_DL_DCI_FORMAT_1_1 || dci_format == NR_UL_DCI_FORMAT_0_1)
    prepare_dci(CellGroup, current_BWP, coreset, dci_pdu_rel15, dci_format);

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
      for (int a = pos; a < dci_size; a++)
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

#ifdef DEBUG_DCI
        LOG_I(NR_MAC,"============= NR_UL_DCI_FORMAT_0_1 =============\n");
        LOG_I(NR_MAC,"dci_size = %i\n", dci_size);
        LOG_I(NR_MAC,"dci_pdu_rel15->format_indicator = %i\n", dci_pdu_rel15->format_indicator);
        LOG_I(NR_MAC,"dci_pdu_rel15->carrier_indicator.val = %i\n", dci_pdu_rel15->carrier_indicator.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->ul_sul_indicator.val = %i\n", dci_pdu_rel15->ul_sul_indicator.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->bwp_indicator.val = %i\n", dci_pdu_rel15->bwp_indicator.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->frequency_domain_assignment.val = %i\n", dci_pdu_rel15->frequency_domain_assignment.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->time_domain_assignment.val = %i\n", dci_pdu_rel15->time_domain_assignment.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->frequency_hopping_flag.val = %i\n", dci_pdu_rel15->frequency_hopping_flag.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->mcs = %i\n", dci_pdu_rel15->mcs);
        LOG_I(NR_MAC,"dci_pdu_rel15->ndi = %i\n", dci_pdu_rel15->ndi);
        LOG_I(NR_MAC,"dci_pdu_rel15->rv= %i\n", dci_pdu_rel15->rv);
        LOG_I(NR_MAC,"dci_pdu_rel15->harq_pid = %i\n", dci_pdu_rel15->harq_pid);
        LOG_I(NR_MAC,"dci_pdu_rel15->dai[0].val = %i\n", dci_pdu_rel15->dai[0].val);
        LOG_I(NR_MAC,"dci_pdu_rel15->dai[1].val = %i\n", dci_pdu_rel15->dai[1].val);
        LOG_I(NR_MAC,"dci_pdu_rel15->tpc = %i\n", dci_pdu_rel15->tpc);
        LOG_I(NR_MAC,"dci_pdu_rel15->srs_resource_indicator.val = %i\n", dci_pdu_rel15->srs_resource_indicator.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->precoding_information.val = %i\n", dci_pdu_rel15->precoding_information.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->antenna_ports.val = %i\n", dci_pdu_rel15->antenna_ports.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->srs_request.val = %i\n", dci_pdu_rel15->srs_request.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->csi_request.val = %i\n", dci_pdu_rel15->csi_request.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->cbgti.val = %i\n", dci_pdu_rel15->cbgti.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->ptrs_dmrs_association.val = %i\n", dci_pdu_rel15->ptrs_dmrs_association.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->beta_offset_indicator.val = %i\n", dci_pdu_rel15->beta_offset_indicator.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->dmrs_sequence_initialization.val = %i\n", dci_pdu_rel15->dmrs_sequence_initialization.val);
        LOG_I(NR_MAC,"dci_pdu_rel15->ulsch_indicator = %i\n", dci_pdu_rel15->ulsch_indicator);
#endif

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
void dump_nr_list(NR_UE_info_t **list)
{
  UE_iterator(list, UE) {
    LOG_T(NR_MAC, "NR list UEs rntis %04x\n", (*list)->rnti);
  }
}

/*
 * Create a new NR_list
 */
void create_nr_list(NR_list_t *list, int len)
{
  list->head = -1;
  list->next = malloc(len * sizeof(*list->next));
  AssertFatal(list->next, "cannot malloc() memory for NR_list_t->next\n");
  for (int i = 0; i < len; ++i)
    list->next[i] = -1;
  list->tail = -1;
  list->len = len;
}

/*
 * Resize an NR_list
 */
void resize_nr_list(NR_list_t *list, int new_len)
{
  if (new_len == list->len)
    return;
  if (new_len > list->len) {
    /* list->head remains */
    const int old_len = list->len;
    int* n = realloc(list->next, new_len * sizeof(*list->next));
    AssertFatal(n, "cannot realloc() memory for NR_list_t->next\n");
    list->next = n;
    for (int i = old_len; i < new_len; ++i)
      list->next[i] = -1;
    /* list->tail remains */
    list->len = new_len;
  } else { /* new_len < len */
    AssertFatal(list->head < new_len, "shortened list head out of index %d (new len %d)\n", list->head, new_len);
    AssertFatal(list->tail < new_len, "shortened list tail out of index %d (new len %d)\n", list->head, new_len);
    for (int i = 0; i < list->len; ++i)
      AssertFatal(list->next[i] < new_len, "shortened list entry out of index %d (new len %d)\n", list->next[i], new_len);
    /* list->head remains */
    int *n = realloc(list->next, new_len * sizeof(*list->next));
    AssertFatal(n, "cannot realloc() memory for NR_list_t->next\n");
    list->next = n;
    /* list->tail remains */
    list->len = new_len;
  }
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

NR_UE_info_t *find_nr_UE(NR_UEs_t *UEs, rnti_t rntiP)
{

  UE_iterator(UEs->list, UE) {
    if (UE->rnti == rntiP) {
      LOG_D(NR_MAC,"Search and found rnti: %04x\n", rntiP);
      return UE;
    }
  }
  LOG_W(NR_MAC,"Search for not existing rnti (ignore for RA): %04x\n", rntiP);
  return NULL;
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

/* hack data to remove UE in the phy */
int rnti_to_remove[10];
volatile int rnti_to_remove_count;
pthread_mutex_t rnti_to_remove_mutex = PTHREAD_MUTEX_INITIALIZER;

void delete_nr_ue_data(NR_UE_info_t *UE, NR_COMMON_channels_t *ccPtr, uid_allocator_t *uia)
{
  NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
  destroy_nr_list(&sched_ctrl->available_dl_harq);
  destroy_nr_list(&sched_ctrl->feedback_dl_harq);
  destroy_nr_list(&sched_ctrl->retrans_dl_harq);
  destroy_nr_list(&sched_ctrl->available_ul_harq);
  destroy_nr_list(&sched_ctrl->feedback_ul_harq);
  destroy_nr_list(&sched_ctrl->retrans_ul_harq);
  uid_linear_allocator_free(uia, UE->uid);
  LOG_I(NR_MAC, "Remove NR rnti 0x%04x\n", UE->rnti);
  const rnti_t rnti = UE->rnti;
  free(UE);
  /* hack to remove UE in the phy */
  if (pthread_mutex_lock(&rnti_to_remove_mutex))
    exit(1);
  if (rnti_to_remove_count == 10)
    exit(1);
  rnti_to_remove[rnti_to_remove_count] = rnti;
  LOG_W(NR_MAC, "to remove in mac rnti_to_remove[%d] = 0x%04x\n", rnti_to_remove_count, rnti);
  rnti_to_remove_count++;
  if (pthread_mutex_unlock(&rnti_to_remove_mutex))
    exit(1);

  /* clear RA process(es?) associated to the UE */
  for (int cc_id = 0; cc_id < NFAPI_CC_MAX; cc_id++) {
    for (int i = 0; i < NR_NB_RA_PROC_MAX; i++) {
      NR_COMMON_channels_t *cc = &ccPtr[cc_id];
      if (cc->ra[i].rnti == rnti) {
        LOG_D(NR_MAC, "free RA process %d for rnti %04x\n", i, rnti);
        /* is it enough? */
        cc->ra[i].cfra  = false;
        cc->ra[i].rnti  = 0;
        cc->ra[i].crnti = 0;
      }
    }
  }
}

// main function to configure parameters of current BWP
void configure_UE_BWP(gNB_MAC_INST *nr_mac,
                      NR_ServingCellConfigCommon_t *scc,
                      NR_UE_sched_ctrl_t *sched_ctrl,
                      NR_RA_t *ra,
                      NR_UE_info_t *UE) {

  AssertFatal((ra != NULL && UE == NULL) || (ra == NULL && UE != NULL), "RA and UE structures are mutually exlusive in BWP configuration\n");

  NR_CellGroupConfig_t *CellGroup;
  NR_UE_DL_BWP_t *DL_BWP;
  NR_UE_UL_BWP_t *UL_BWP;

  if (ra) {
    DL_BWP = &ra->DL_BWP;
    UL_BWP = &ra->UL_BWP;
    CellGroup = ra->CellGroup;
  }
  else {
    DL_BWP = &UE->current_DL_BWP;
    UL_BWP = &UE->current_UL_BWP;
    CellGroup = UE->CellGroup;
  }
  NR_BWP_Downlink_t *dl_bwp = NULL;
  NR_BWP_Uplink_t *ul_bwp = NULL;
  NR_BWP_DownlinkDedicated_t *bwpd = NULL;
  NR_BWP_UplinkDedicated_t *ubwpd = NULL;
  DL_BWP->n_dl_bwp = 1;
  UL_BWP->n_ul_bwp = 1;
  int old_dl_bwp_id = DL_BWP->bwp_id;
  int old_ul_bwp_id = UL_BWP->bwp_id;

  int target_ss;

  if (CellGroup &&
      CellGroup->spCellConfig &&
      CellGroup->spCellConfig->spCellConfigDedicated) {

    const NR_ServingCellConfig_t *servingCellConfig = CellGroup->spCellConfig->spCellConfigDedicated;
    DL_BWP->pdsch_servingcellconfig = servingCellConfig->pdsch_ServingCellConfig? servingCellConfig->pdsch_ServingCellConfig->choice.setup : NULL;
    target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;

    if(UE && UE->Msg3_dcch_dtch) {
      // switching to initial BWP
      DL_BWP->bwp_id = 0;
      UL_BWP->bwp_id = 0;
      UE->Msg3_dcch_dtch = false;
    }
    else {
      // (re)configuring BWP
      // TODO BWP switching not via RRC reconfiguration
      // via RRC if firstActiveXlinkBWP_Id is NULL, MAC stays on the same BWP as before
      if (servingCellConfig->firstActiveDownlinkBWP_Id)
        DL_BWP->bwp_id = *servingCellConfig->firstActiveDownlinkBWP_Id;
      if (servingCellConfig->uplinkConfig->firstActiveUplinkBWP_Id)
        UL_BWP->bwp_id = *servingCellConfig->uplinkConfig->firstActiveUplinkBWP_Id;
    }

    const struct NR_ServingCellConfig__downlinkBWP_ToAddModList *bwpList = servingCellConfig->downlinkBWP_ToAddModList;
    if(bwpList)
      DL_BWP->n_dl_bwp = bwpList->list.count;
    if (DL_BWP->bwp_id>0) {
      for (int i=0; i<bwpList->list.count; i++) {
        dl_bwp = bwpList->list.array[i];
        if(dl_bwp->bwp_Id == DL_BWP->bwp_id)
          break;
      }
      AssertFatal(dl_bwp!=NULL,"Couldn't find DLBWP corresponding to BWP ID %ld\n",DL_BWP->bwp_id);
    }

    const struct NR_UplinkConfig__uplinkBWP_ToAddModList *ubwpList = servingCellConfig->uplinkConfig->uplinkBWP_ToAddModList;
    if(ubwpList)
      UL_BWP->n_ul_bwp = ubwpList->list.count;
    if (UL_BWP->bwp_id>0) {
      for (int i=0; i<ubwpList->list.count; i++) {
        ul_bwp = ubwpList->list.array[i];
        if(ul_bwp->bwp_Id == UL_BWP->bwp_id)
          break;
      }
      AssertFatal(ul_bwp!=NULL,"Couldn't find ULBWP corresponding to BWP ID %ld\n",UL_BWP->bwp_id);
    }

    // selection of dedicated BWPs
    if(dl_bwp)
      bwpd = dl_bwp->bwp_Dedicated;
    else
      bwpd = servingCellConfig->initialDownlinkBWP;
    if(ul_bwp)
      ubwpd = ul_bwp->bwp_Dedicated;
    else
      ubwpd = servingCellConfig->uplinkConfig->initialUplinkBWP;

    DL_BWP->pdsch_Config = bwpd->pdsch_Config->choice.setup;
    UL_BWP->pusch_Config = ubwpd->pusch_Config->choice.setup;
    UL_BWP->pucch_Config = ubwpd->pucch_Config->choice.setup;
    UL_BWP->srs_Config = ubwpd->srs_Config->choice.setup;
    UL_BWP->csi_MeasConfig = servingCellConfig->csi_MeasConfig ? servingCellConfig->csi_MeasConfig->choice.setup : NULL;
  }
  else {
    DL_BWP->bwp_id = 0;
    DL_BWP->bwp_id = 0;
    target_ss = NR_SearchSpace__searchSpaceType_PR_common;
    DL_BWP->pdsch_Config = NULL;
    UL_BWP->pusch_Config = NULL;
    UL_BWP->pucch_Config = NULL;
    UL_BWP->csi_MeasConfig = NULL;
  }

  if (old_dl_bwp_id != DL_BWP->bwp_id)
    LOG_I(NR_MAC, "Switching to DL-BWP %li\n", DL_BWP->bwp_id);
  if (old_ul_bwp_id != UL_BWP->bwp_id)
    LOG_I(NR_MAC, "Switching to UL-BWP %li\n", UL_BWP->bwp_id);

  // TDA lists
  if (DL_BWP->bwp_id>0)
    DL_BWP->tdaList = dl_bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
  else
    DL_BWP->tdaList = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;

  if(UL_BWP->bwp_id>0)
    UL_BWP->tdaList = ul_bwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
  else
    UL_BWP->tdaList = scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;

  // setting generic parameters
  NR_BWP_t dl_genericParameters = (DL_BWP->bwp_id>0 && dl_bwp) ?
    dl_bwp->bwp_Common->genericParameters:
    scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters;

  DL_BWP->scs = dl_genericParameters.subcarrierSpacing;
  DL_BWP->cyclicprefix = dl_genericParameters.cyclicPrefix;
  DL_BWP->BWPSize = NRRIV2BW(dl_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  DL_BWP->BWPStart = NRRIV2PRBOFFSET(dl_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  NR_BWP_t ul_genericParameters = (UL_BWP->bwp_id>0 && ul_bwp) ?
    ul_bwp->bwp_Common->genericParameters:
    scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;

  UL_BWP->scs = ul_genericParameters.subcarrierSpacing;
  UL_BWP->cyclicprefix = ul_genericParameters.cyclicPrefix;
  UL_BWP->BWPSize = NRRIV2BW(ul_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  UL_BWP->BWPStart = NRRIV2PRBOFFSET(ul_genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  // Set downlink MCS table
  if (DL_BWP->pdsch_Config &&
      DL_BWP->pdsch_Config->mcs_Table) {
    if (*DL_BWP->pdsch_Config->mcs_Table == 0)
      DL_BWP->mcsTableIdx = 1;
    else
      DL_BWP->mcsTableIdx = 2;
  } else
    DL_BWP->mcsTableIdx = 0;
  LOG_D(NR_MAC,"DL MCS Table Index: %d\n",DL_BWP->mcsTableIdx);

  if (UL_BWP->pusch_Config == NULL || !UL_BWP->pusch_Config->transformPrecoder)
    UL_BWP->transform_precoding = !scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder;
  else
    UL_BWP->transform_precoding = *UL_BWP->pusch_Config->transformPrecoder;

  if(UL_BWP->bwp_id>0)
    UL_BWP->pucch_ConfigCommon = ul_bwp->bwp_Common->pucch_ConfigCommon->choice.setup;
  else
    UL_BWP->pucch_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->pucch_ConfigCommon->choice.setup;


  if(UE) {
    // setting PDCCH related structures for sched_ctrl
    sched_ctrl->search_space = get_searchspace(scc,
                                               bwpd,
                                               target_ss);
    sched_ctrl->coreset = get_coreset(nr_mac,
                                      scc,
                                      bwpd,
                                      sched_ctrl->search_space,
                                      target_ss);

    sched_ctrl->sched_pdcch = set_pdcch_structure(nr_mac,
                                                  sched_ctrl->search_space,
                                                  sched_ctrl->coreset,
                                                  scc,
                                                  &dl_genericParameters,
                                                  nr_mac->type0_PDCCH_CSS_config);

    // set DL DCI format
    DL_BWP->dci_format = (sched_ctrl->search_space->searchSpaceType &&
                         sched_ctrl->search_space->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific) ?
                         (sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats == NR_SearchSpace__searchSpaceType__ue_Specific__dci_Formats_formats0_1_And_1_1 ?
                         NR_DL_DCI_FORMAT_1_1 : NR_DL_DCI_FORMAT_1_0) :
                         NR_DL_DCI_FORMAT_1_0;
    // set UL DCI format
    UL_BWP->dci_format = (sched_ctrl->search_space->searchSpaceType &&
                         sched_ctrl->search_space->searchSpaceType->present == NR_SearchSpace__searchSpaceType_PR_ue_Specific) ?
                         (sched_ctrl->search_space->searchSpaceType->choice.ue_Specific->dci_Formats == NR_SearchSpace__searchSpaceType__ue_Specific__dci_Formats_formats0_1_And_1_1 ?
                         NR_UL_DCI_FORMAT_0_1 : NR_UL_DCI_FORMAT_0_0) :
                         NR_UL_DCI_FORMAT_0_0;

    if (UL_BWP->csi_MeasConfig)
      compute_csi_bitlen (UL_BWP->csi_MeasConfig, UE->csi_report_template);

  }

  if(ra) {
    // setting PDCCH related structures for RA
    struct NR_PDCCH_ConfigCommon__commonSearchSpaceList *commonSearchSpaceList = NULL;
    NR_SearchSpaceId_t ra_SearchSpace = 0;
    if(dl_bwp) {
      commonSearchSpaceList = dl_bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList;
      ra_SearchSpace = *dl_bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->ra_SearchSpace;
    } else {
      commonSearchSpaceList = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList;
      ra_SearchSpace = *scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->ra_SearchSpace;
    }
    AssertFatal(commonSearchSpaceList->list.count > 0, "common SearchSpace list has 0 elements\n");
    for (int i = 0; i < commonSearchSpaceList->list.count; i++) {
      NR_SearchSpace_t * ss = commonSearchSpaceList->list.array[i];
      if (ss->searchSpaceId == ra_SearchSpace)
        ra->ra_ss = ss;
    }
    AssertFatal(ra->ra_ss!=NULL,"SearchSpace cannot be null for RA\n");

    ra->coreset = get_coreset(nr_mac, scc, dl_bwp, ra->ra_ss, NR_SearchSpace__searchSpaceType_PR_common);
    ra->sched_pdcch = set_pdcch_structure(nr_mac,
                                          ra->ra_ss,
                                          ra->coreset,
                                          scc,
                                          &dl_genericParameters,
                                          &nr_mac->type0_PDCCH_CSS_config[ra->beam_id]);

    UL_BWP->dci_format = NR_UL_DCI_FORMAT_0_0;
    DL_BWP->dci_format = NR_DL_DCI_FORMAT_1_0;
  }

  // Set uplink MCS table
  long *mcs_Table = NULL;
  if (UL_BWP->pusch_Config)
    mcs_Table = UL_BWP->transform_precoding ?
                UL_BWP->pusch_Config->mcs_Table :
                UL_BWP->pusch_Config->mcs_TableTransformPrecoder;

  UL_BWP->mcs_table = get_pusch_mcs_table(mcs_Table,
                                          UL_BWP->transform_precoding ? 0 : 1,
                                          UL_BWP->dci_format,
                                          NR_RNTI_C,
                                          target_ss,
                                          false);
}

//------------------------------------------------------------------------------
NR_UE_info_t *add_new_nr_ue(gNB_MAC_INST *nr_mac, rnti_t rntiP, NR_CellGroupConfig_t *CellGroup)
{
  NR_ServingCellConfigCommon_t *scc = nr_mac->common_channels[0].ServingCellConfigCommon;
  NR_UEs_t *UE_info = &nr_mac->UE_info;
  LOG_I(NR_MAC, "Adding UE with rnti 0x%04x\n",
        rntiP);
  dump_nr_list(UE_info->list);

  // We will attach at the end, to mitigate race conditions
  // This is not good, but we will fix it progressively
  NR_UE_info_t *UE=calloc(1,sizeof(NR_UE_info_t));
  if(!UE) {
    LOG_E(NR_MAC,"want to add UE %04x but the fixed allocated size is full\n",rntiP);
    return NULL;
  }

  UE->rnti = rntiP;
  UE->uid = uid_linear_allocator_new(&UE_info->uid_allocator);
  UE->CellGroup = CellGroup;

  if (CellGroup)
    UE->Msg4_ACKed = true;
  else
    UE->Msg4_ACKed = false;

  NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
  memset(sched_ctrl, 0, sizeof(*sched_ctrl));
  sched_ctrl->dl_max_mcs = 28; /* do not limit MCS for individual UEs */
  sched_ctrl->ta_update = 31;
  sched_ctrl->sched_srs.frame = -1;
  sched_ctrl->sched_srs.slot = -1;

  // initialize UE BWP information
  NR_UE_DL_BWP_t *dl_bwp = &UE->current_DL_BWP;
  memset(dl_bwp, 0, sizeof(*dl_bwp));
  NR_UE_UL_BWP_t *ul_bwp = &UE->current_UL_BWP;
  memset(ul_bwp, 0, sizeof(*ul_bwp));
  configure_UE_BWP(nr_mac, scc, sched_ctrl, NULL, UE);

  /* set illegal time domain allocation to force recomputation of all fields */
  sched_ctrl->pdsch_semi_static.time_domain_allocation = -1;
  sched_ctrl->pusch_semi_static.time_domain_allocation = -1;

  /* Set default BWPs */
  sched_ctrl->next_dl_bwp_id = -1;
  sched_ctrl->next_ul_bwp_id = -1;
  AssertFatal(ul_bwp->n_ul_bwp <= NR_MAX_NUM_BWP,
              "uplinkBWP_ToAddModList has %d BWP!\n",
              ul_bwp->n_ul_bwp);

  /* get Number of HARQ processes for this UE */
  // pdsch_servingcellconfig == NULL in SA -> will create default (8) number of HARQ processes
  create_dl_harq_list(sched_ctrl, dl_bwp->pdsch_servingcellconfig);
  // add all available UL HARQ processes for this UE
  // nb of ul harq processes not configurable
  create_nr_list(&sched_ctrl->available_ul_harq, 16);
  for (int harq = 0; harq < 16; harq++)
    add_tail_nr_list(&sched_ctrl->available_ul_harq, harq);
  create_nr_list(&sched_ctrl->feedback_ul_harq, 16);
  create_nr_list(&sched_ctrl->retrans_ul_harq, 16);

  pthread_mutex_lock(&UE_info->mutex);
  int i;
  for(i=0; i<MAX_MOBILES_PER_GNB; i++) {
    if (UE_info->list[i] == NULL) {
      UE_info->list[i] = UE;
      break;
    }
  }
  if (i == MAX_MOBILES_PER_GNB) {
    LOG_E(NR_MAC,"Try to add UE %04x but the list is full\n", rntiP);
    delete_nr_ue_data(UE, nr_mac->common_channels, &UE_info->uid_allocator);
    pthread_mutex_unlock(&UE_info->mutex);
    return NULL;
  }
  pthread_mutex_unlock(&UE_info->mutex);

  LOG_D(NR_MAC, "Add NR rnti %x\n", rntiP);
  dump_nr_list(UE_info->list);
  return (UE);
}

void create_dl_harq_list(NR_UE_sched_ctrl_t *sched_ctrl,
                         const NR_PDSCH_ServingCellConfig_t *pdsch) {
  const int nrofHARQ = pdsch && pdsch->nrofHARQ_ProcessesForPDSCH ?
                       get_nrofHARQ_ProcessesForPDSCH(*pdsch->nrofHARQ_ProcessesForPDSCH) : 8;
  // add all available DL HARQ processes for this UE
  AssertFatal(sched_ctrl->available_dl_harq.len == sched_ctrl->feedback_dl_harq.len
              && sched_ctrl->available_dl_harq.len == sched_ctrl->retrans_dl_harq.len,
              "HARQ lists have different lengths (%d/%d/%d)\n",
              sched_ctrl->available_dl_harq.len,
              sched_ctrl->feedback_dl_harq.len,
              sched_ctrl->retrans_dl_harq.len);
  if (sched_ctrl->available_dl_harq.len == 0) {
    create_nr_list(&sched_ctrl->available_dl_harq, nrofHARQ);
    for (int harq = 0; harq < nrofHARQ; harq++)
      add_tail_nr_list(&sched_ctrl->available_dl_harq, harq);
    create_nr_list(&sched_ctrl->feedback_dl_harq, nrofHARQ);
    create_nr_list(&sched_ctrl->retrans_dl_harq, nrofHARQ);
  } else if (sched_ctrl->available_dl_harq.len == nrofHARQ) {
    LOG_D(NR_MAC, "nrofHARQ %d already configured\n", nrofHARQ);
  } else {
    const int old_nrofHARQ = sched_ctrl->available_dl_harq.len;
    AssertFatal(nrofHARQ > old_nrofHARQ,
                "cannot resize HARQ list to be smaller (nrofHARQ %d, old_nrofHARQ %d)\n",
                nrofHARQ, old_nrofHARQ);
    resize_nr_list(&sched_ctrl->available_dl_harq, nrofHARQ);
    for (int harq = old_nrofHARQ; harq < nrofHARQ; harq++)
      add_tail_nr_list(&sched_ctrl->available_dl_harq, harq);
    resize_nr_list(&sched_ctrl->feedback_dl_harq, nrofHARQ);
    resize_nr_list(&sched_ctrl->retrans_dl_harq, nrofHARQ);
  }
}

void reset_dl_harq_list(NR_UE_sched_ctrl_t *sched_ctrl) {
  int harq;
  while ((harq = sched_ctrl->feedback_dl_harq.head) >= 0) {
    remove_front_nr_list(&sched_ctrl->feedback_dl_harq);
    add_tail_nr_list(&sched_ctrl->available_dl_harq, harq);
  }

  while ((harq = sched_ctrl->retrans_dl_harq.head) >= 0) {
    remove_front_nr_list(&sched_ctrl->retrans_dl_harq);
    add_tail_nr_list(&sched_ctrl->available_dl_harq, harq);
  }

  for (int i = 0; i < NR_MAX_NB_HARQ_PROCESSES; i++) {
    sched_ctrl->harq_processes[i].feedback_slot = -1;
    sched_ctrl->harq_processes[i].round = 0;
    sched_ctrl->harq_processes[i].is_waiting = false;
  }
}

void reset_ul_harq_list(NR_UE_sched_ctrl_t *sched_ctrl) {
  int harq;
  while ((harq = sched_ctrl->feedback_ul_harq.head) >= 0) {
    remove_front_nr_list(&sched_ctrl->feedback_ul_harq);
    add_tail_nr_list(&sched_ctrl->available_ul_harq, harq);
  }

  while ((harq = sched_ctrl->retrans_ul_harq.head) >= 0) {
    remove_front_nr_list(&sched_ctrl->retrans_ul_harq);
    add_tail_nr_list(&sched_ctrl->available_ul_harq, harq);
  }

  for (int i = 0; i < NR_MAX_NB_HARQ_PROCESSES; i++) {
    sched_ctrl->ul_harq_processes[i].feedback_slot = -1;
    sched_ctrl->ul_harq_processes[i].round = 0;
    sched_ctrl->ul_harq_processes[i].is_waiting = false;
  }
}

void mac_remove_nr_ue(gNB_MAC_INST *nr_mac, rnti_t rnti)
{
 NR_UEs_t *UE_info = &nr_mac->UE_info;
 pthread_mutex_lock(&UE_info->mutex);
 UE_iterator(UE_info->list, UE) {
   if (UE->rnti==rnti)
     break;
 }

 if (!UE) {
   LOG_W(NR_MAC,"Call to del rnti %04x, but not existing\n", rnti);
   pthread_mutex_unlock(&UE_info->mutex);
   return;
 }

 NR_UE_info_t * newUEs[MAX_MOBILES_PER_GNB+1]={0};
 int newListIdx=0;
 for (int i=0; i<MAX_MOBILES_PER_GNB; i++)
   if(UE_info->list[i] && UE_info->list[i]->rnti != rnti)
     newUEs[newListIdx++]=UE_info->list[i];
 memcpy(UE_info->list, newUEs, sizeof(UE_info->list));
 pthread_mutex_unlock(&UE_info->mutex);

 delete_nr_ue_data(UE, nr_mac->common_channels, &UE_info->uid_allocator);
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


void get_pdsch_to_harq_feedback(NR_PUCCH_Config_t *pucch_Config,
                                nr_dci_format_t dci_format,
                                int *max_fb_time,
                                uint8_t *pdsch_to_harq_feedback) {

  if (dci_format == NR_DL_DCI_FORMAT_1_0) {
    for (int i=0; i<8; i++) {
      pdsch_to_harq_feedback[i] = i+1;
      if(pdsch_to_harq_feedback[i]>*max_fb_time)
        *max_fb_time = pdsch_to_harq_feedback[i];
    }
  }
  else {
    AssertFatal(pucch_Config!=NULL,"pucch_Config shouldn't be null here\n");
    if(pucch_Config->dl_DataToUL_ACK != NULL) {
      for (int i=0; i<8; i++) {
        pdsch_to_harq_feedback[i] = *pucch_Config->dl_DataToUL_ACK->list.array[i];
        if(pdsch_to_harq_feedback[i]>*max_fb_time)
          *max_fb_time = pdsch_to_harq_feedback[i];
      }
    }
    else
      AssertFatal(0==1,"There is no allocated dl_DataToUL_ACK for pdsch to harq feedback\n");
  }
}

void nr_csirs_scheduling(int Mod_idP,
                         frame_t frame,
                         sub_frame_t slot,
                         int n_slots_frame){

  int CC_id = 0;
  NR_UEs_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  gNB_MAC_INST *gNB_mac = RC.nrmac[Mod_idP];
  uint16_t *vrb_map = gNB_mac->common_channels[CC_id].vrb_map;

  UE_info->sched_csirs = false;

  UE_iterator(UE_info->list, UE) {

    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
    if (sched_ctrl->rrc_processing_timer > 0) {
      continue;
    }

    NR_UE_DL_BWP_t *dl_bwp = &UE->current_DL_BWP;
    NR_UE_UL_BWP_t *ul_bwp = &UE->current_UL_BWP;

    if (!ul_bwp->csi_MeasConfig) continue;

    NR_CSI_MeasConfig_t *csi_measconfig = ul_bwp->csi_MeasConfig;

    // looking for the correct CSI-RS resource in current BWP
    NR_NZP_CSI_RS_ResourceSetId_t *nzp = NULL;
    for (int csi_list=0; csi_list<csi_measconfig->csi_ResourceConfigToAddModList->list.count; csi_list++) {
      NR_CSI_ResourceConfig_t *csires = csi_measconfig->csi_ResourceConfigToAddModList->list.array[csi_list];
      if(csires->bwp_Id == dl_bwp->bwp_id &&
         csires->csi_RS_ResourceSetList.present == NR_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_nzp_CSI_RS_SSB &&
         csires->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList) {
        nzp = csires->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList->list.array[0];
      }
    }

    if (csi_measconfig->nzp_CSI_RS_ResourceToAddModList != NULL &&
        nzp != NULL) {

      NR_NZP_CSI_RS_Resource_t *nzpcsi;
      int period, offset;

      nfapi_nr_dl_tti_request_body_t *dl_req = &gNB_mac->DL_req[CC_id].dl_tti_request_body;

      for (int id = 0; id < csi_measconfig->nzp_CSI_RS_ResourceToAddModList->list.count; id++){
        nzpcsi = csi_measconfig->nzp_CSI_RS_ResourceToAddModList->list.array[id];
        // transmitting CSI-RS only for current BWP
        if (nzpcsi->nzp_CSI_RS_ResourceId != *nzp)
          continue;

        NR_CSI_RS_ResourceMapping_t  resourceMapping = nzpcsi->resourceMapping;
        csi_period_offset(NULL,nzpcsi->periodicityAndOffset,&period,&offset);

        if((frame*n_slots_frame+slot-offset)%period == 0) {

          LOG_D(NR_MAC,"Scheduling CSI-RS in frame %d slot %d Resource ID %ld\n",frame,slot,nzpcsi->nzp_CSI_RS_ResourceId);
          UE_info->sched_csirs = true;

          nfapi_nr_dl_tti_request_pdu_t *dl_tti_csirs_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
          memset((void*)dl_tti_csirs_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
          dl_tti_csirs_pdu->PDUType = NFAPI_NR_DL_TTI_CSI_RS_PDU_TYPE;
          dl_tti_csirs_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_csi_rs_pdu));

          nfapi_nr_dl_tti_csi_rs_pdu_rel15_t *csirs_pdu_rel15 = &dl_tti_csirs_pdu->csi_rs_pdu.csi_rs_pdu_rel15;

          csirs_pdu_rel15->subcarrier_spacing = dl_bwp->scs;
          if (dl_bwp->cyclicprefix)
            csirs_pdu_rel15->cyclic_prefix = *dl_bwp->cyclicprefix;
          else
            csirs_pdu_rel15->cyclic_prefix = 0;

          // According to last paragraph of TS 38.214 5.2.2.3.1
          if (resourceMapping.freqBand.startingRB < dl_bwp->BWPStart) {
            csirs_pdu_rel15->start_rb = dl_bwp->BWPStart;
          } else {
            csirs_pdu_rel15->start_rb = resourceMapping.freqBand.startingRB;
          }
          if (resourceMapping.freqBand.nrofRBs > (dl_bwp->BWPStart + dl_bwp->BWPSize - csirs_pdu_rel15->start_rb)) {
            csirs_pdu_rel15->nr_of_rbs = dl_bwp->BWPStart + dl_bwp->BWPSize - csirs_pdu_rel15->start_rb;
          } else {
            csirs_pdu_rel15->nr_of_rbs = resourceMapping.freqBand.nrofRBs;
          }
          AssertFatal(csirs_pdu_rel15->nr_of_rbs >= 24, "CSI-RS has %d RBs, but the minimum is 24\n", csirs_pdu_rel15->nr_of_rbs);

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
                vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 1);
              break;
            case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row2:
              csirs_pdu_rel15->row = 2;
              csirs_pdu_rel15->freq_domain = (((resourceMapping.frequencyDomainAllocation.choice.row2.buf[1]>>4)&0x0f) |
                                             ((resourceMapping.frequencyDomainAllocation.choice.row2.buf[0]<<4)&0xff0));
              for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 1);
              break;
            case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row4:
              csirs_pdu_rel15->row = 4;
              csirs_pdu_rel15->freq_domain = ((resourceMapping.frequencyDomainAllocation.choice.row4.buf[0])>>5)&0x07;
              for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 1);
              break;
            case NR_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_other:
              csirs_pdu_rel15->freq_domain = ((resourceMapping.frequencyDomainAllocation.choice.other.buf[0])>>2)&0x3f;
              // determining the row of table 7.4.1.5.3-1 in 38.211
              switch(resourceMapping.nrofPorts){
                case NR_CSI_RS_ResourceMapping__nrofPorts_p1:
                  AssertFatal(1==0,"Resource with 1 CSI port shouldn't be within other rows\n");
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p2:
                  csirs_pdu_rel15->row = 3;
                  for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                    vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 1);
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p4:
                  csirs_pdu_rel15->row = 5;
                  for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                    vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 2);
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p8:
                  if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm4_FD2_TD2) {
                    csirs_pdu_rel15->row = 8;
                    for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                      vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 2);
                  }
                  else{
                    int num_k = 0;
                    for (int k=0; k<6; k++)
                      num_k+=(((csirs_pdu_rel15->freq_domain)>>k)&0x01);
                    if(num_k==4) {
                      csirs_pdu_rel15->row = 6;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 1);
                    }
                    else {
                      csirs_pdu_rel15->row = 7;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 2);
                    }
                  }
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p12:
                  if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm4_FD2_TD2) {
                    csirs_pdu_rel15->row = 10;
                    for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                      vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 2);
                  }
                  else {
                    csirs_pdu_rel15->row = 9;
                    for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                      vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 1);
                  }
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p16:
                  if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm4_FD2_TD2)
                    csirs_pdu_rel15->row = 12;
                  else
                    csirs_pdu_rel15->row = 11;
                  for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                    vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 2);
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p24:
                  if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm4_FD2_TD2) {
                    csirs_pdu_rel15->row = 14;
                    for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                      vrb_map[rb] |= (SL_to_bitmap(csirs_pdu_rel15->symb_l0, 2) | SL_to_bitmap(csirs_pdu_rel15->symb_l1, 2));
                  }
                  else{
                    if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm8_FD2_TD4) {
                      csirs_pdu_rel15->row = 15;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 3);
                    }
                    else {
                      csirs_pdu_rel15->row = 13;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb] |= (SL_to_bitmap(csirs_pdu_rel15->symb_l0, 2) | SL_to_bitmap(csirs_pdu_rel15->symb_l1, 2));
                    }
                  }
                  break;
                case NR_CSI_RS_ResourceMapping__nrofPorts_p32:
                  if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm4_FD2_TD2) {
                    csirs_pdu_rel15->row = 17;
                    for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                      vrb_map[rb] |= (SL_to_bitmap(csirs_pdu_rel15->symb_l0, 2) | SL_to_bitmap(csirs_pdu_rel15->symb_l1, 2));
                  }
                  else{
                    if (resourceMapping.cdm_Type == NR_CSI_RS_ResourceMapping__cdm_Type_cdm8_FD2_TD4) {
                      csirs_pdu_rel15->row = 18;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb] |= SL_to_bitmap(csirs_pdu_rel15->symb_l0, 3);
                    }
                    else {
                      csirs_pdu_rel15->row = 16;
                      for (int rb = csirs_pdu_rel15->start_rb; rb < (csirs_pdu_rel15->start_rb + csirs_pdu_rel15->nr_of_rbs); rb++)
                        vrb_map[rb] |= (SL_to_bitmap(csirs_pdu_rel15->symb_l0, 2) | SL_to_bitmap(csirs_pdu_rel15->symb_l1, 2));
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

void nr_mac_update_timers(module_id_t module_id,
                          frame_t frame,
                          sub_frame_t slot) {

  NR_UEs_t *UE_info = &RC.nrmac[module_id]->UE_info;
  UE_iterator(UE_info->list, UE) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
    if (sched_ctrl->rrc_processing_timer > 0) {
      sched_ctrl->rrc_processing_timer--;
      if (sched_ctrl->rrc_processing_timer == 0) {
        LOG_I(NR_MAC, "(%d.%d) De-activating RRC processing timer for UE %04x\n", frame, slot, UE->rnti);

        NR_CellGroupConfig_t *cg = NULL;
        uper_decode(NULL,
                    &asn_DEF_NR_CellGroupConfig,   //might be added prefix later
                    (void **)&cg,
                    (uint8_t *)UE->cg_buf,
                    (UE->enc_rval.encoded+7)/8, 0, 0);
        UE->CellGroup = cg;

        if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
          xer_fprint(stdout, &asn_DEF_NR_CellGroupConfig, (const void *) UE->CellGroup);
        }

        NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon;

        LOG_I(NR_MAC,"Modified rnti %04x with CellGroup\n", UE->rnti);
        process_CellGroup(cg,&UE->UE_sched_ctrl);
        NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;

        configure_UE_BWP(RC.nrmac[module_id], scc, sched_ctrl, NULL, UE);

        if (get_softmodem_params()->sa) {
          // add all available DL HARQ processes for this UE in SA
          create_dl_harq_list(sched_ctrl, UE->current_DL_BWP.pdsch_servingcellconfig);
        }

        NR_pdsch_semi_static_t *ps = &sched_ctrl->pdsch_semi_static;
        const uint8_t layers = set_dl_nrOfLayers(sched_ctrl);
        const int tda = get_dl_tda(RC.nrmac[module_id], scc, slot);

        nr_set_pdsch_semi_static(&UE->current_DL_BWP,
                                 scc,
                                 tda,
                                 layers,
                                 sched_ctrl,
                                 ps);


        NR_pusch_semi_static_t *ups = &sched_ctrl->pusch_semi_static;
        const uint8_t nrOfLayers = 1;
        const int utda = get_ul_tda(RC.nrmac[module_id], scc, slot);

        nr_set_pusch_semi_static(&UE->current_UL_BWP,
                                 scc,
                                 utda,
                                 nrOfLayers,
                                 ups);
      }
    }
  }
}

void schedule_nr_bwp_switch(module_id_t module_id,
                            frame_t frame,
                            sub_frame_t slot) {

  NR_UEs_t *UE_info = &RC.nrmac[module_id]->UE_info;

  // TODO: Implementation of a algorithm to perform:
  //  - DL BWP selection:     sched_ctrl->next_dl_bwp_id = dl_bwp_id
  //  - UL BWP selection:     sched_ctrl->next_ul_bwp_id = ul_bwp_id

  UE_iterator(UE_info->list, UE) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
    NR_UE_DL_BWP_t *dl_bwp = &UE->current_DL_BWP;
    NR_UE_UL_BWP_t *ul_bwp = &UE->current_UL_BWP;
    if (sched_ctrl->rrc_processing_timer == 0 && UE->Msg4_ACKed && sched_ctrl->next_dl_bwp_id >= 0) {

      int schedule_bwp_switching = false;
      if (dl_bwp->bwp_id == 0) {
        // Switching from Initial BWP to Dedicated BWP
        if (sched_ctrl->next_dl_bwp_id > 0 && sched_ctrl->next_ul_bwp_id > 0) {
          schedule_bwp_switching = true;
          LOG_W(NR_MAC,"%4d.%2d UE %04x Schedule BWP switch from Initial DL BWP to %ld and from Initial UL BWP to %ld\n",
                frame, slot, UE->rnti, sched_ctrl->next_dl_bwp_id, sched_ctrl->next_ul_bwp_id);
        }
      } else if (dl_bwp->bwp_id != sched_ctrl->next_dl_bwp_id && ul_bwp->bwp_id != sched_ctrl->next_ul_bwp_id) {
        // Switching between Dedicated BWPs
        schedule_bwp_switching = true;
        LOG_W(NR_MAC,"%4d.%2d UE %04x Schedule BWP switch from dl_bwp_id %ld to %ld and from ul_bwp_id %ld to %ld\n",
              frame, slot, UE->rnti, dl_bwp->bwp_id, sched_ctrl->next_dl_bwp_id, ul_bwp->bwp_id, sched_ctrl->next_ul_bwp_id);
      }

      if (schedule_bwp_switching) {
        AssertFatal(sched_ctrl->next_dl_bwp_id > 0 && sched_ctrl->next_ul_bwp_id > 0, "BWP switching from a Dedicated BWP to the Initial BWP not handled yet!");
        nr_mac_rrc_bwp_switch_req(module_id, frame, slot, UE->rnti, sched_ctrl->next_dl_bwp_id, sched_ctrl->next_ul_bwp_id);
      }
    }
  }
}

void UL_tti_req_ahead_initialization(gNB_MAC_INST * gNB, NR_ServingCellConfigCommon_t *scc, int n, int CCid) {

  if(gNB->UL_tti_req_ahead[CCid]) return;

  gNB->UL_tti_req_ahead[CCid] = calloc(n, sizeof(nfapi_nr_ul_tti_request_t));
  AssertFatal(gNB->UL_tti_req_ahead[CCid],
              "could not allocate memory for RC.nrmac[]->UL_tti_req_ahead[]\n");
  /* fill in slot/frame numbers: slot is fixed, frame will be updated by scheduler
   * consider that scheduler runs sl_ahead: the first sl_ahead slots are
   * already "in the past" and thus we put frame 1 instead of 0! */
  for (int i = 0; i < n; ++i) {
    nfapi_nr_ul_tti_request_t *req = &gNB->UL_tti_req_ahead[CCid][i];
    req->SFN = i < (gNB->if_inst->sl_ahead-1);
    req->Slot = i;
  }
}

void send_initial_ul_rrc_message(module_id_t        module_id,
                                 int                CC_id,
                                 const NR_UE_info_t *UE,
                                 rb_id_t            srb_id,
                                 const uint8_t      *sdu,
                                 sdu_size_t         sdu_len) {
  const gNB_MAC_INST *mac = RC.nrmac[module_id];
  const rnti_t rnti = UE->rnti;
  LOG_W(MAC,
        "[RAPROC] Received SDU for CCCH on SRB %ld length %d for UE %04x\n",
        srb_id, sdu_len, rnti);

  /* TODO REMOVE_DU_RRC: the RRC in the DU is a hack and should be taken out in the future */
  if (NODE_IS_DU(RC.nrrrc[module_id]->node_type)) {
    struct rrc_gNB_ue_context_s *ue_context_p = rrc_gNB_allocate_new_UE_context(RC.nrrrc[module_id]);
    ue_context_p->ue_id_rnti                    = rnti;
    ue_context_p->ue_context.rnti               = rnti;
    ue_context_p->ue_context.random_ue_identity = rnti;
    ue_context_p->ue_context.Srb0.Active        = 1;
    RB_INSERT(rrc_nr_ue_tree_s, &RC.nrrrc[module_id]->rrc_ue_head, ue_context_p);
  }

  const NR_ServingCellConfigCommon_t *scc = RC.nrrrc[module_id]->carrier.servingcellconfigcommon;
  const NR_ServingCellConfig_t *sccd = RC.nrrrc[module_id]->configuration.scd;
  NR_CellGroupConfig_t cellGroupConfig = {0};
  fill_initial_cellGroupConfig(UE->uid, &cellGroupConfig, scc, sccd, &RC.nrrrc[module_id]->configuration);

  uint8_t du2cu_rrc_container[1024];
  asn_enc_rval_t enc_rval = uper_encode_to_buffer(&asn_DEF_NR_CellGroupConfig,
                                                  NULL,
                                                  &cellGroupConfig,
                                                  du2cu_rrc_container,
                                                  sizeof(du2cu_rrc_container));
  AssertFatal(enc_rval.encoded > 0,
              "Could not encode cellGroupConfig for UE %04x, failed element %s\n",
              rnti,
              enc_rval.failed_type->name);

  const f1ap_initial_ul_rrc_message_t ul_rrc_msg = {
    /* TODO: add mcc, mnc, cell_id, ..., is not available at MAC yet */
    .crnti = rnti,
    .rrc_container = (uint8_t *) sdu,
    .rrc_container_length = sdu_len,
    .du2cu_rrc_container = (uint8_t *) du2cu_rrc_container,
    .du2cu_rrc_container_length = (enc_rval.encoded + 7) / 8
  };
  mac->mac_rrc.initial_ul_rrc_message_transfer(module_id, &ul_rrc_msg);
}
