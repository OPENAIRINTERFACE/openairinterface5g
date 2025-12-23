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

#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "BIT_STRING.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "GNB_APP/gnb_config.h"
#include "NR_MIB.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_BCCH-BCH-Message.h"
#include "NR_ServingCellConfigCommon.h"
#include "NR_MIB.h"
#include "SCHED_NR/phy_frame_config_nr.h"
#include "T.h"
#include "asn_internal.h"
#include "assertions.h"
#include "common/ran_context.h"
#include "common/utils/T/T.h"
#include "common/utils/nr/nr_common.h"
#include "executables/softmodem-common.h"
#include "f1ap_messages_types.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "nfapi_interface.h"
#include "nfapi_nr_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "utils.h"

c16_t convert_precoder_weight(double complex c_in)
{
  return (c16_t) {.r = round(SHRT_MAX*creal(c_in)), .i = round(SHRT_MAX*cimag(c_in))};
}

void get_K1_K2(int N1, int N2, int *K1, int *K2, int layers)
{
  // num of allowed k1 and k2 according to 5.2.2.2.1-3 and -4 in 38.214
  switch (layers) {
    case 1:
      *K1 = 1;
      *K2 = 1;
      break;
    case 2:
      *K2 = N2 == 1 ? 1 : 2;
      if(N2 == N1 || N1 == 2)
        *K1 = 2;
      else if (N2 == 1)
        *K1 = 4;
      else
        *K1 = 3;
      break;
    case 3:
    case 4:
      *K2 = N2 == 1 ? 1 : 2;
      if (N1 == 6)
        *K1 = 5;
      else
        *K1 = N1;
      break;
    default:
      AssertFatal(false, "Number of layers %d not supported\n", layers);
  }
}

int get_NTN_Koffset(const NR_ServingCellConfigCommon_t *scc)
{
  if (scc->ext2 && scc->ext2->ntn_Config_r17 && scc->ext2->ntn_Config_r17->cellSpecificKoffset_r17)
    return *scc->ext2->ntn_Config_r17->cellSpecificKoffset_r17 << *scc->ssbSubcarrierSpacing;
  return 0;
}

int precoding_weigths_generation(nfapi_nr_pm_list_t *mat,
                                 int pmiq,
                                 int L,
                                 int N1,
                                 int N2,
                                 int O1,
                                 int O2,
                                 int num_antenna_ports,
                                 double complex theta_n[4],
                                 double complex v_lm[N1 * O1 + 4 * O1][N2 * O2 + O2][N2 * N1])
{
  // Table 5.2.2.2.1-X:
  // Codebook for L-layer CSI reporting using antenna ports 3000 to 2999+PCSI-RS
  // pmi=1,...,pmi_size are computed as follows
  int K1 = 0, K2 = 0;
  get_K1_K2(N1, N2, &K1, &K2, L);
  int I2 = L == 1 ? 4 : 2;
  for (int k2 = 0; k2 < K2; k2++) {
    for (int k1 = 0; k1 < K1; k1++) {
      for (int mm = 0; mm < N2 * O2; mm++) { // i_1_2
        for (int ll = 0; ll < N1 * O1; ll++) { // i_1_1
          for (int nn = 0; nn < I2; nn++) { // i_2
            mat->pmi_pdu[pmiq].pm_idx = pmiq + 1; // index 0 is the identity matrix
            mat->pmi_pdu[pmiq].numLayers = L;
            mat->pmi_pdu[pmiq].num_ant_ports = num_antenna_ports;
            LOG_D(PHY, "layer %d Codebook pmiq = %d\n", L, pmiq);
            for (int j_col = 0; j_col < L; j_col++) {
              int llc = ll + (k1 * O1 * (j_col & 1));
              int mmc = mm + (k2 * O2 * (j_col & 1));
              double complex phase_sign = j_col <= ((L - 1) / 2) ? 1 : -1;
              double complex res_code;
              for (int i_rows = 0; i_rows < N1 * N2; i_rows++) {
                nfapi_nr_pm_weights_t *weights = &mat->pmi_pdu[pmiq].weights[j_col][i_rows];
                res_code = sqrt(1 / (double)L) * v_lm[llc][mmc][i_rows];
                c16_t precoder_weight = convert_precoder_weight(res_code);
                *weights = precoder_weight;
                LOG_D(PHY,
                      "%d Layer Precoding Matrix[pmi %d][antPort %d][layerIdx %d]= %f+j %f -> Fixed Point %d+j %d \n",
                      L,
                      pmiq,
                      i_rows,
                      j_col,
                      creal(res_code),
                      cimag(res_code),
                      weights->r,
                      weights->i);
              }
              for (int i_rows = N1 * N2; i_rows < 2 * N1 * N2; i_rows++) {
                nfapi_nr_pm_weights_t *weights = &mat->pmi_pdu[pmiq].weights[j_col][i_rows];
                res_code = sqrt(1 / (double)L) * (phase_sign)*theta_n[nn] * v_lm[llc][mmc][i_rows - N1 * N2];
                c16_t precoder_weight = convert_precoder_weight(res_code);
                *weights = precoder_weight;
                LOG_D(PHY,
                      "%d Layer Precoding Matrix[pmi %d][antPort %d][layerIdx %d]= %f+j %f -> Fixed Point %d+j %d \n",
                      L,
                      pmiq,
                      i_rows,
                      j_col,
                      creal(res_code),
                      cimag(res_code),
                      weights->r,
                      weights->i);
              }
            }
            pmiq++;
          }
        }
      }
    }
  }
  return pmiq;
}

nfapi_nr_pm_list_t init_DL_MIMO_codebook(gNB_MAC_INST *gNB, nr_pdsch_AntennaPorts_t antenna_ports)
{
  int num_antenna_ports = antenna_ports.N1 * antenna_ports.N2 * antenna_ports.XP;
  if (num_antenna_ports < 2)
    return (nfapi_nr_pm_list_t) {0};

  //NR Codebook Generation for codebook type1 SinglePanel
  int N1 = antenna_ports.N1;
  int N2 = antenna_ports.N2;
  //Uniform Planner Array: UPA
  //    X X X X ... X
  //    X X X X ... X
  // N2 . . . . ... .
  //    X X X X ... X
  //   |<-----N1---->|

  //Get the uniform planar array parameters
  // To be confirmed
  int O2 = N2 > 1 ? 4 : 1; //Vertical beam oversampling (1 or 4)
  int O1 = num_antenna_ports > 2 ? 4 : 1; //Horizontal beam oversampling (1 or 4)

  int max_mimo_layers = (num_antenna_ports < NR_MAX_NB_LAYERS) ? num_antenna_ports : NR_MAX_NB_LAYERS;
  AssertFatal(max_mimo_layers <= 4, "Max number of layers supported is 4\n");
  AssertFatal(num_antenna_ports < 16, "Max number of antenna ports supported is currently 16\n");

  int K1 = 0, K2 = 0;
  nfapi_nr_pm_list_t mat = {.num_pm_idx = 0};
  for (int i = 0; i < max_mimo_layers; i++) {
    get_K1_K2(N1, N2, &K1, &K2, i + 1);
    int i2_size = i == 0 ? 4 : 2;
    gNB->precoding_matrix_size[i] = i2_size * N1 * O1 * N2 * O2 * K1 * K2;
    mat.num_pm_idx += gNB->precoding_matrix_size[i];
  }

  mat.pmi_pdu = malloc16(mat.num_pm_idx * sizeof(*mat.pmi_pdu));
  AssertFatal(mat.pmi_pdu != NULL, "out of memory\n");

  // Generation of codebook Type1 with codebookMode 1 (num_antenna_ports < 16)

  // Generate DFT vertical beams
  // ll: index of a vertical beams vector (represented by i1_1 in TS 38.214)
  const int max_l = N1 * O1 + 4 * O1;  // max k1 is 4*O1
  double complex v[max_l][N1];
  for (int ll = 0; ll < max_l; ll++) { // i1_1
    for (int nn = 0; nn < N1; nn++) {
      v[ll][nn] = cexp(I * (2 * M_PI * nn * ll) / (N1 * O1));
      LOG_D(PHY, "v[%d][%d] = %f +j %f\n", ll, nn, creal(v[ll][nn]), cimag(v[ll][nn]));
    }
  }
  // Generate DFT Horizontal beams
  // mm: index of a Horizontal beams vector (represented by i1_2 in TS 38.214)
  const int max_m = N2 * O2 + O2; // max k2 is O2
  double complex u[max_m][N2];
  for (int mm = 0; mm < max_m; mm++) { // i1_2
    for (int nn = 0; nn < N2; nn++) {
      u[mm][nn] = cexp(I * (2 * M_PI * nn * mm) / (N2 * O2));
      LOG_D(PHY, "u[%d][%d] = %f +j %f\n", mm, nn, creal(u[mm][nn]), cimag(u[mm][nn]));
    }
  }
  // Generate co-phasing angles
  // i_2: index of a co-phasing vector
  // i1_1, i1_2, and i_2 are reported from UEs
  double complex theta_n[4];
  for (int nn = 0; nn < 4; nn++) {
    theta_n[nn] = cexp(I * M_PI * nn / 2);
    LOG_D(PHY, "theta_n[%d] = %f +j %f\n", nn, creal(theta_n[nn]), cimag(theta_n[nn]));
  }
  // Kronecker product v_lm
  double complex v_lm[max_l][max_m][N2 * N1];
  // v_ll_mm_codebook denotes the elements of a precoding matrix W_i1,1_i_1,2
  for (int ll = 0; ll < max_l; ll++) { // i_1_1
    for (int mm = 0; mm < max_m; mm++) { // i_1_2
      for (int nn1 = 0; nn1 < N1; nn1++) {
        for (int nn2 = 0; nn2 < N2; nn2++) {
          v_lm[ll][mm][nn1 * N2 + nn2] = v[ll][nn1] * u[mm][nn2];
          LOG_D(PHY,
                "v_lm[%d][%d][%d] = %f +j %f\n",
                ll,
                mm,
                nn1 * N2 + nn2,
                creal(v_lm[ll][mm][nn1 * N2 + nn2]),
                cimag(v_lm[ll][mm][nn1 * N2 + nn2]));
        }
      }
    }
  }

  int pmiq = 0;
  for (int layers = 1; layers <= max_mimo_layers; layers++)
    pmiq = precoding_weigths_generation(&mat, pmiq, layers, N1, N2, O1, O2, num_antenna_ports, theta_n, v_lm);

  return mat;
}

/**
 * @brief Get the first UL slot index in period
 * @param fs frame structure
 * @param mixed indicates whether to include in the count also mixed slot with UL symbols or only full UL slot
 * @return slot index
 */
int get_first_ul_slot(const frame_structure_t *fs, bool mixed)
{
  DevAssert(fs);

  if (fs->frame_type == TDD) {
    for (int i = 0; i < fs->numb_slots_period; i++) {
      if ((mixed && is_ul_slot(i, fs)) || fs->period_cfg.tdd_slot_bitmap[i].slot_type == TDD_NR_UPLINK_SLOT) {
        return i;
      }
    }
  }

  return 0; // FDD
}

/**
 * @brief Get number of DL slots per period (full DL slots + mixed slots with DL symbols)
 */
int get_dl_slots_per_period(const frame_structure_t *fs)
{
  return fs->frame_type == TDD ? fs->period_cfg.num_dl_slots : fs->numb_slots_frame;
}

/**
 * @brief Get number of UL slots per period (full UL slots + mixed slots with UL symbols)
 */
int get_ul_slots_per_period(const frame_structure_t *fs)
{
  return fs->frame_type == TDD ? fs->period_cfg.num_ul_slots : fs->numb_slots_frame;
}

/**
 * @brief Get number of full UL slots per period
 * @param fs Pointer to the frame structure
 * @return Number of full UL slots
 */
int get_full_ul_slots_per_period(const frame_structure_t *fs)
{
  DevAssert(fs);

  if (fs->frame_type == FDD)
    return fs->numb_slots_frame;

  int count = 0;

  for (int i = 0; i < fs->numb_slots_period; i++)
    if (fs->period_cfg.tdd_slot_bitmap[i].slot_type == TDD_NR_UPLINK_SLOT)
      count++;

  LOG_D(NR_MAC, "Full UL slots in TDD period: %d\n", count);
  return count;
}

/**
 * @brief Get number of full DL slots per period
 * @param fs Pointer to the frame structure
 * @return Number of full DL slots
 */
int get_full_dl_slots_per_period(const frame_structure_t *fs)
{
  DevAssert(fs);

  if (fs->frame_type == FDD)
    return fs->numb_slots_frame;

  int count = 0;

  for (int i = 0; i < fs->numb_slots_period; i++)
    if (fs->period_cfg.tdd_slot_bitmap[i].slot_type == TDD_NR_DOWNLINK_SLOT)
      count++;

  LOG_D(NR_MAC, "Full DL slots in TDD period: %d\n", count);
  return count;
}

/**
 * @brief Get number of UL slots per frame
 */
int get_ul_slots_per_frame(const frame_structure_t *fs)
{
  return fs->frame_type == TDD ? fs->numb_period_frame * get_ul_slots_per_period(fs) : fs->numb_slots_frame;
}

/**
 * @brief Get the nth UL slot offset for UE index idx in a TDD period using the frame structure bitmap
 * @param fs frame structure
 * @param idx UE index
 * @param count_mixed indicates whether counting mixed slot with UL symbols (e.g. for SRS) or only full UL slots
 * @return slot index offset
 */
int get_ul_slot_offset(const frame_structure_t *fs, int idx, bool count_mixed)
{
  DevAssert(fs);

  // FDD
  if (fs->frame_type == FDD)
    return idx;

  // UL slots indexes in period
  int ul_slot_idxs[fs->numb_slots_period];
  int ul_slot_count = 0;

  /* Populate the indices of UL slots in the TDD period from the bitmap
  count also mixed slots with UL symbols if flag count_mixed is present */
  for (int i = 0; i < fs->numb_slots_period; i++) {
    if ((count_mixed && is_ul_slot(i, fs)) || fs->period_cfg.tdd_slot_bitmap[i].slot_type == TDD_NR_UPLINK_SLOT) {
      ul_slot_idxs[ul_slot_count++] = i;
    }
  }

  // Compute slot index offset
  int period_idx = idx / ul_slot_count; // wrap up the count of complete TDD periods spanned by the index
  int ul_slot_idx_in_period = idx % ul_slot_count; // wrap up the UL slot index within the current TDD period

  return ul_slot_idxs[ul_slot_idx_in_period] + period_idx * fs->numb_slots_period;
}

static void config_common(gNB_MAC_INST *nrmac, const nr_mac_config_t *config, NR_ServingCellConfigCommon_t *scc)
{
  nfapi_nr_config_request_scf_t *cfg = &nrmac->config[0];
  nrmac->common_channels[0].ServingCellConfigCommon = scc;

  // Carrier configuration
  struct NR_FrequencyInfoDL *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
  frequency_range_t frequency_range = get_freq_range_from_band(*frequencyInfoDL->frequencyBandList.list.array[0]);
  int bw_index = get_supported_band_index(frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing,
                                          frequency_range,
                                          frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
  cfg->carrier_config.dl_bandwidth.value = get_supported_bw_mhz(frequency_range, bw_index);
  cfg->carrier_config.dl_bandwidth.tl.tag = NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG; // temporary
  cfg->num_tlv++;

  cfg->carrier_config.dl_frequency.value = from_nrarfcn(*frequencyInfoDL->frequencyBandList.list.array[0],
                                                        *scc->ssbSubcarrierSpacing,
                                                        frequencyInfoDL->absoluteFrequencyPointA)
                                            / 1000; // freq in kHz
  cfg->carrier_config.dl_frequency.tl.tag = NFAPI_NR_CONFIG_DL_FREQUENCY_TAG;
  cfg->num_tlv++;

  for (int i = 0; i < 5; i++) {
    if (i == frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing) {
      cfg->carrier_config.dl_grid_size[i].value = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.dl_k0[i].value = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
      cfg->carrier_config.dl_grid_size[i].tl.tag = NFAPI_NR_CONFIG_DL_GRID_SIZE_TAG;
      cfg->carrier_config.dl_k0[i].tl.tag = NFAPI_NR_CONFIG_DL_K0_TAG;
      cfg->num_tlv++;
      cfg->num_tlv++;
    } else {
      cfg->carrier_config.dl_grid_size[i].value = 0;
      cfg->carrier_config.dl_k0[i].value = 0;
    }
  }
  struct NR_FrequencyInfoUL *frequencyInfoUL = scc->uplinkConfigCommon->frequencyInfoUL;
  int scs = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
  frequency_range = get_freq_range_from_band(*frequencyInfoUL->frequencyBandList->list.array[0]);
  bw_index =
      get_supported_band_index(scs, frequency_range, frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth);
  cfg->carrier_config.uplink_bandwidth.value = get_supported_bw_mhz(frequency_range, bw_index);
  cfg->carrier_config.uplink_bandwidth.tl.tag = NFAPI_NR_CONFIG_UPLINK_BANDWIDTH_TAG; // temporary
  cfg->num_tlv++;

  int UL_pointA;
  if (frequencyInfoUL->absoluteFrequencyPointA == NULL)
    UL_pointA = frequencyInfoDL->absoluteFrequencyPointA;
  else
    UL_pointA = *frequencyInfoUL->absoluteFrequencyPointA;

  cfg->carrier_config.uplink_frequency.value = from_nrarfcn(*frequencyInfoUL->frequencyBandList->list.array[0],
                                                            *scc->ssbSubcarrierSpacing,
                                                            UL_pointA)
                                               / 1000; // freq in kHz
  cfg->carrier_config.uplink_frequency.tl.tag = NFAPI_NR_CONFIG_UPLINK_FREQUENCY_TAG;
  cfg->num_tlv++;

  for (int i = 0; i < 5; i++) {
    if (i == scs) {
      cfg->carrier_config.ul_grid_size[i].value = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
      cfg->carrier_config.ul_k0[i].value = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->offsetToCarrier;
      cfg->carrier_config.ul_grid_size[i].tl.tag = NFAPI_NR_CONFIG_UL_GRID_SIZE_TAG;
      cfg->carrier_config.ul_k0[i].tl.tag = NFAPI_NR_CONFIG_UL_K0_TAG;
      cfg->num_tlv++;
      cfg->num_tlv++;
    } else {
      cfg->carrier_config.ul_grid_size[i].value = 0;
      cfg->carrier_config.ul_k0[i].value = 0;
    }
  }

  NR_FreqBandIndicatorNR_t band = *frequencyInfoDL->frequencyBandList.list.array[0];
  frame_type_t frame_type = get_frame_type(band, *scc->ssbSubcarrierSpacing);
  nrmac->common_channels[0].frame_type = frame_type;

  // Cell configuration
  cfg->cell_config.phy_cell_id.value = *scc->physCellId;
  cfg->cell_config.phy_cell_id.tl.tag = NFAPI_NR_CONFIG_PHY_CELL_ID_TAG;
  cfg->num_tlv++;

  cfg->cell_config.frame_duplex_type.value = frame_type;
  cfg->cell_config.frame_duplex_type.tl.tag = NFAPI_NR_CONFIG_FRAME_DUPLEX_TYPE_TAG;
  cfg->num_tlv++;

  // SSB configuration
  cfg->ssb_config.ss_pbch_power.value = scc->ss_PBCH_BlockPower;
  cfg->ssb_config.ss_pbch_power.tl.tag = NFAPI_NR_CONFIG_SS_PBCH_POWER_TAG;
  cfg->num_tlv++;

  cfg->ssb_config.bch_payload.value = 1;
  cfg->ssb_config.bch_payload.tl.tag = NFAPI_NR_CONFIG_BCH_PAYLOAD_TAG;
  cfg->num_tlv++;

  cfg->ssb_config.scs_common.value = *scc->ssbSubcarrierSpacing;
  cfg->ssb_config.scs_common.tl.tag = NFAPI_NR_CONFIG_SCS_COMMON_TAG;
  cfg->num_tlv++;

  // PRACH configuration

  uint8_t nb_preambles = 64;
  NR_RACH_ConfigCommon_t *rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  if (rach_ConfigCommon->totalNumberOfRA_Preambles != NULL)
    nb_preambles = *rach_ConfigCommon->totalNumberOfRA_Preambles;

  cfg->prach_config.prach_sequence_length.value = rach_ConfigCommon->prach_RootSequenceIndex.present - 1;
  cfg->prach_config.prach_sequence_length.tl.tag = NFAPI_NR_CONFIG_PRACH_SEQUENCE_LENGTH_TAG;
  cfg->num_tlv++;

  cfg->prach_config.prach_ConfigurationIndex.value = rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;
  cfg->prach_config.prach_ConfigurationIndex.tl.tag = NFAPI_NR_CONFIG_PRACH_CONFIG_INDEX_TAG;
  cfg->num_tlv++;

  if (rach_ConfigCommon->msg1_SubcarrierSpacing)
    cfg->prach_config.prach_sub_c_spacing.value = *rach_ConfigCommon->msg1_SubcarrierSpacing;
  else {
    // If absent, use SCS as derived from the prach-ConfigurationIndex (for 839)
    int config_index = rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;
    int frame_type = get_frame_type(band, frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing);
    int format = get_nr_prach_format_from_index(config_index, UL_pointA, frame_type) & 0xff;
    cfg->prach_config.prach_sub_c_spacing.value = get_delta_f_RA_long(format);
  }

  cfg->prach_config.prach_sub_c_spacing.tl.tag = NFAPI_NR_CONFIG_PRACH_SUB_C_SPACING_TAG;
  cfg->num_tlv++;
  cfg->prach_config.restricted_set_config.value = rach_ConfigCommon->restrictedSetConfig;
  cfg->prach_config.restricted_set_config.tl.tag = NFAPI_NR_CONFIG_RESTRICTED_SET_CONFIG_TAG;
  cfg->num_tlv++;

  switch (rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM) {
    case 0:
      cfg->prach_config.num_prach_fd_occasions.value = 1;
      break;
    case 1:
      cfg->prach_config.num_prach_fd_occasions.value = 2;
      break;
    case 2:
      cfg->prach_config.num_prach_fd_occasions.value = 4;
      break;
    case 3:
      cfg->prach_config.num_prach_fd_occasions.value = 8;
      break;
    default:
      AssertFatal(1 == 0, "msg1 FDM identifier %ld undefined (0,1,2,3) \n", rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM);
  }
  cfg->prach_config.num_prach_fd_occasions.tl.tag = NFAPI_NR_CONFIG_NUM_PRACH_FD_OCCASIONS_TAG;
  cfg->num_tlv++;

  cfg->prach_config.num_prach_fd_occasions_list = (nfapi_nr_num_prach_fd_occasions_t *)malloc(
       cfg->prach_config.num_prach_fd_occasions.value * sizeof(nfapi_nr_num_prach_fd_occasions_t));
  for (int i = 0; i < cfg->prach_config.num_prach_fd_occasions.value; i++) {
    nfapi_nr_num_prach_fd_occasions_t *prach_fd_occasion = &cfg->prach_config.num_prach_fd_occasions_list[i];
    // prach_fd_occasion->num_prach_fd_occasions = i;
    if (cfg->prach_config.prach_sequence_length.value)
      prach_fd_occasion->prach_root_sequence_index.value = rach_ConfigCommon->prach_RootSequenceIndex.choice.l139;
    else
      prach_fd_occasion->prach_root_sequence_index.value = rach_ConfigCommon->prach_RootSequenceIndex.choice.l839;
    prach_fd_occasion->prach_root_sequence_index.tl.tag = NFAPI_NR_CONFIG_PRACH_ROOT_SEQUENCE_INDEX_TAG;
    cfg->num_tlv++;
    prach_fd_occasion->k1.value =
        NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE)
        + rach_ConfigCommon->rach_ConfigGeneric.msg1_FrequencyStart
        + (get_N_RA_RB(cfg->prach_config.prach_sub_c_spacing.value, scs) * i);
    if (IS_SA_MODE(get_softmodem_params())) {
      prach_fd_occasion->k1.value =
          NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE)
          + rach_ConfigCommon->rach_ConfigGeneric.msg1_FrequencyStart
          + (get_N_RA_RB(cfg->prach_config.prach_sub_c_spacing.value, scs) * i);
    } else {
      prach_fd_occasion->k1.value = rach_ConfigCommon->rach_ConfigGeneric.msg1_FrequencyStart
                                    + (get_N_RA_RB(cfg->prach_config.prach_sub_c_spacing.value, scs) * i);
    }
    prach_fd_occasion->k1.tl.tag = NFAPI_NR_CONFIG_K1_TAG;
    cfg->num_tlv++;
    prach_fd_occasion->prach_zero_corr_conf.value = rach_ConfigCommon->rach_ConfigGeneric.zeroCorrelationZoneConfig;
    prach_fd_occasion->prach_zero_corr_conf.tl.tag = NFAPI_NR_CONFIG_PRACH_ZERO_CORR_CONF_TAG;
    cfg->num_tlv++;
    prach_fd_occasion->num_root_sequences.value = compute_nr_root_seq(rach_ConfigCommon, nb_preambles, frame_type, frequency_range);
    prach_fd_occasion->num_root_sequences.tl.tag = NFAPI_NR_CONFIG_NUM_ROOT_SEQUENCES_TAG;
    cfg->num_tlv++;
    prach_fd_occasion->num_unused_root_sequences.tl.tag = NFAPI_NR_CONFIG_NUM_UNUSED_ROOT_SEQUENCES_TAG;
    prach_fd_occasion->num_unused_root_sequences.value = 0;
    cfg->num_tlv++;
  }

  cfg->prach_config.ssb_per_rach.value = rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present - 1;
  cfg->prach_config.ssb_per_rach.tl.tag = NFAPI_NR_CONFIG_SSB_PER_RACH_TAG;
  cfg->num_tlv++;

  // compute and store prach duration in slots from rach_ConfigCommon
  NR_RACH_ConfigGeneric_t *rachConfig =
      &scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric;
  NR_COMMON_channels_t *cc = nrmac->common_channels;
  const uint32_t pointA = scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA;
  const int prach_fmt = (get_nr_prach_format_from_index(rachConfig->prach_ConfigurationIndex, pointA, cc->frame_type) & 0xff);
  cc->prach_len = (prach_fmt < 4) ? get_long_prach_dur(prach_fmt, *scc->ssbSubcarrierSpacing) : 1;

  // SSB Table Configuration

  cfg->ssb_table.ssb_offset_point_a.value =
       get_ssb_offset_to_pointA(*scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB,
                                scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,
                                *scc->ssbSubcarrierSpacing,
                                frequency_range);
  cfg->ssb_table.ssb_offset_point_a.tl.tag = NFAPI_NR_CONFIG_SSB_OFFSET_POINT_A_TAG;
  cfg->num_tlv++;
  cfg->ssb_table.ssb_period.value = *scc->ssb_periodicityServingCell;
  cfg->ssb_table.ssb_period.tl.tag = NFAPI_NR_CONFIG_SSB_PERIOD_TAG;
  cfg->num_tlv++;
  cfg->ssb_table.ssb_subcarrier_offset.value =
       get_ssb_subcarrier_offset(*scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB,
                                 scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,
                                 *scc->ssbSubcarrierSpacing);
  cfg->ssb_table.ssb_subcarrier_offset.tl.tag = NFAPI_NR_CONFIG_SSB_SUBCARRIER_OFFSET_TAG;
  cfg->num_tlv++;

  uint8_t *mib_payload = nrmac->common_channels[0].MIB_pdu;
  uint32_t mib = (mib_payload[2] << 16) | (mib_payload[1] << 8) | mib_payload[0];
  cfg->ssb_table.MIB.tl.tag = NFAPI_NR_CONFIG_MIB_TAG;
  cfg->ssb_table.MIB.value = mib;
  cfg->num_tlv++;

  nrmac->ssb_SubcarrierOffset = cfg->ssb_table.ssb_subcarrier_offset.value;
  nrmac->ssb_OffsetPointA = cfg->ssb_table.ssb_offset_point_a.value;
  LOG_D(NR_MAC,
        "ssb_OffsetPointA %d, ssb_SubcarrierOffset %d\n",
        cfg->ssb_table.ssb_offset_point_a.value,
        cfg->ssb_table.ssb_subcarrier_offset.value);

  switch (scc->ssb_PositionsInBurst->present) {
    case 1:
      cfg->ssb_table.ssb_mask_list[0].ssb_mask.value = ((uint32_t)scc->ssb_PositionsInBurst->choice.shortBitmap.buf[0]) << 24;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask.value = 0;
      break;
    case 2:
      cfg->ssb_table.ssb_mask_list[0].ssb_mask.value = ((uint32_t)scc->ssb_PositionsInBurst->choice.mediumBitmap.buf[0]) << 24;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask.value = 0;
      break;
    case 3:
      cfg->ssb_table.ssb_mask_list[0].ssb_mask.value = 0;
      cfg->ssb_table.ssb_mask_list[1].ssb_mask.value = 0;
      for (int i = 0; i < 4; i++) {
        cfg->ssb_table.ssb_mask_list[0].ssb_mask.value += (uint32_t)scc->ssb_PositionsInBurst->choice.longBitmap.buf[3 - i]
                                                          << i * 8;
        cfg->ssb_table.ssb_mask_list[1].ssb_mask.value += (uint32_t)scc->ssb_PositionsInBurst->choice.longBitmap.buf[7 - i]
                                                          << i * 8;
      }
      break;
    default:
      AssertFatal(1 == 0, "SSB bitmap size value %d undefined (allowed values 1,2,3) \n", scc->ssb_PositionsInBurst->present);
  }

  cfg->ssb_table.ssb_mask_list[0].ssb_mask.tl.tag = NFAPI_NR_CONFIG_SSB_MASK_TAG;
  cfg->ssb_table.ssb_mask_list[1].ssb_mask.tl.tag = NFAPI_NR_CONFIG_SSB_MASK_TAG;
  cfg->num_tlv += 2;

  // logical antenna ports
  nr_pdsch_AntennaPorts_t pdsch_AntennaPorts = config->pdsch_AntennaPorts;
  int num_pdsch_antenna_ports = pdsch_AntennaPorts.N1 * pdsch_AntennaPorts.N2 * pdsch_AntennaPorts.XP;
  cfg->carrier_config.num_tx_ant.value = num_pdsch_antenna_ports;
  AssertFatal(num_pdsch_antenna_ports > 0 && num_pdsch_antenna_ports < 33, "pdsch_AntennaPorts in 1...32\n");
  cfg->carrier_config.num_tx_ant.tl.tag = NFAPI_NR_CONFIG_NUM_TX_ANT_TAG;

  int num_ssb = 0;
  for (int i = 0; i < 32; i++) {
    cfg->ssb_table.ssb_beam_id_list[i].beam_id.tl.tag = NFAPI_NR_CONFIG_BEAM_ID_TAG;
    if ((cfg->ssb_table.ssb_mask_list[0].ssb_mask.value >> (31 - i)) & 1) {
      cfg->ssb_table.ssb_beam_id_list[i].beam_id.value = num_ssb;
      num_ssb++;
    }
    cfg->num_tlv++;
  }
  for (int i = 0; i < 32; i++) {
    cfg->ssb_table.ssb_beam_id_list[32 + i].beam_id.tl.tag = NFAPI_NR_CONFIG_BEAM_ID_TAG;
    if ((cfg->ssb_table.ssb_mask_list[1].ssb_mask.value >> (31 - i)) & 1) {
      cfg->ssb_table.ssb_beam_id_list[32 + i].beam_id.value = num_ssb;
      num_ssb++;
    }
    cfg->num_tlv++;
  }

  int pusch_AntennaPorts = config->pusch_AntennaPorts;
  cfg->carrier_config.num_rx_ant.value = pusch_AntennaPorts;
  AssertFatal(pusch_AntennaPorts > 0 && pusch_AntennaPorts < 13, "pusch_AntennaPorts in 1...12\n");
  cfg->carrier_config.num_rx_ant.tl.tag = NFAPI_NR_CONFIG_NUM_RX_ANT_TAG;
  LOG_I(NR_MAC,
        "Set TX antenna number to %d, Set RX antenna number to %d (num ssb %d: %x,%x)\n",
        cfg->carrier_config.num_tx_ant.value,
        cfg->carrier_config.num_rx_ant.value,
        num_ssb,
        cfg->ssb_table.ssb_mask_list[0].ssb_mask.value,
        cfg->ssb_table.ssb_mask_list[1].ssb_mask.value);
  AssertFatal(cfg->carrier_config.num_tx_ant.value > 0,
              "carrier_config.num_tx_ant.value %d!\n",
              cfg->carrier_config.num_tx_ant.value);
  cfg->num_tlv++;
  cfg->num_tlv++;

  // Frame structure configuration
  uint8_t mu = frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
  if (cfg->cell_config.frame_duplex_type.value == TDD) {
    cfg->tdd_table.tdd_period.tl.tag = NFAPI_NR_CONFIG_TDD_PERIOD_TAG;
    cfg->num_tlv++;
    cfg->tdd_table.tdd_period.value = get_tdd_period_idx(scc->tdd_UL_DL_ConfigurationCommon);
    LOG_D(NR_MAC, "Setting TDD configuration period to %d\n", cfg->tdd_table.tdd_period.value);
  }
  frame_structure_t *fs = &nrmac->frame_structure;
  config_frame_structure(mu,
                         scc->tdd_UL_DL_ConfigurationCommon,
                         cfg->tdd_table.tdd_period.value,
                         cfg->cell_config.frame_duplex_type.value,
                         fs);
  if (cfg->cell_config.frame_duplex_type.value == TDD)
    set_tdd_config_nr(cfg, fs);

  // precoding matrix configuration (to be improved)
  cfg->pmi_list = init_DL_MIMO_codebook(nrmac, pdsch_AntennaPorts);

  int nb_beams = config->nb_bfw[1]; // number of beams
  if (nrmac->beam_info.beam_mode == PRECONFIGURED_BEAM_IDX) {
    LOG_I(NR_MAC, "Configuring analog beamforming in config_request message\n");
    cfg->analog_beamforming_ve.num_beams_period_vendor_ext.tl.tag = NFAPI_NR_FAPI_NUM_BEAMS_PERIOD_VENDOR_EXTENSION_TAG;
    cfg->analog_beamforming_ve.num_beams_period_vendor_ext.value = nrmac->beam_info.beams_per_period;
    cfg->num_tlv++;
    cfg->analog_beamforming_ve.analog_bf_vendor_ext.tl.tag = NFAPI_NR_FAPI_ANALOG_BF_VENDOR_EXTENSION_TAG;
    cfg->analog_beamforming_ve.analog_bf_vendor_ext.value = 1;  // analog BF enabled
    cfg->num_tlv++;
    cfg->analog_beamforming_ve.total_num_beams_vendor_ext.tl.tag = NFAPI_NR_FAPI_TOTAL_NUM_BEAMS_VENDOR_EXTENSION_TAG;
    cfg->analog_beamforming_ve.total_num_beams_vendor_ext.value = nb_beams;
    cfg->num_tlv++;
    cfg->analog_beamforming_ve.analog_beam_list = malloc16(nb_beams * sizeof(*cfg->analog_beamforming_ve.analog_beam_list));
    for (int i = 0; i < nb_beams; i++) {
      cfg->analog_beamforming_ve.analog_beam_list[i].tl.tag = NFAPI_NR_FAPI_ANALOG_BEAM_VENDOR_EXTENSION_TAG;
      cfg->analog_beamforming_ve.analog_beam_list[i].value = config->bw_list[i];
    }
  } else {
    cfg->analog_beamforming_ve.analog_bf_vendor_ext.value = 0;  // analog BF disabled
    if (NFAPI_MODE == NFAPI_MONOLITHIC) {
      cfg->analog_beamforming_ve.analog_bf_vendor_ext.tl.tag = NFAPI_NR_FAPI_ANALOG_BF_VENDOR_EXTENSION_TAG;
      cfg->num_tlv++;
    }
  }

  if (IS_SA_MODE(get_softmodem_params())) {
    int bw = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
    uint64_t bw_khz = (12 * bw) * (15 << mu);
    uint64_t carr_dl = (cfg->carrier_config.dl_frequency.value + bw_khz / 2) * 1000;
    uint64_t carr_ul = (cfg->carrier_config.uplink_frequency.value + bw_khz / 2) * 1000;
    int prb_offset = (frequency_range == FR1) ?
                     cfg->ssb_table.ssb_offset_point_a.value >> scs :
                     cfg->ssb_table.ssb_offset_point_a.value >> (scs - 2);
    int sc_offset = (frequency_range == FR1) ?
                    cfg->ssb_table.ssb_subcarrier_offset.value >> scs :
                    cfg->ssb_table.ssb_subcarrier_offset.value;
    if (cfg->carrier_config.dl_frequency.value != cfg->carrier_config.uplink_frequency.value) {
      LOG_I(NR_MAC,
            "Command line parameters for OAI UE: -C %lu --CO %ld -r %d --numerology %d --band %ld --ssb %d %s\n",
            carr_dl,
            carr_ul - carr_dl,
            bw,
            mu,
            band,
            12 * prb_offset + sc_offset,
            get_softmodem_params()->threequarter_fs ? "-E" : "");
    } else {
      LOG_I(NR_MAC,
            "Command line parameters for OAI UE: -C %lu -r %d --numerology %d --band %ld --ssb %d %s\n",
            carr_dl,
            bw,
            mu,
            band,
            12 * prb_offset + sc_offset,
            get_softmodem_params()->threequarter_fs ? "-E" : "");
    }
  }
}

static void initialize_beam_information(NR_beam_info_t *beam_info, int mu, int slots_per_frame)
{
  if (beam_info->beam_mode == NO_BEAM_MODE)
    return;

  int size = mu == 0 ? slots_per_frame << 1 : slots_per_frame;
  // slots in beam duration gives the number of consecutive slots tied the the same beam
  AssertFatal(size % beam_info->beam_duration == 0,
              "Beam duration %d should be divider of number of slots per frame %d\n",
              beam_info->beam_duration,
              slots_per_frame);
  beam_info->beam_allocation_size = size / beam_info->beam_duration;
  for (int i = 0; i < beam_info->beams_per_period; i++) {
    beam_info->beam_allocation[i] = malloc16(beam_info->beam_allocation_size * sizeof(int));
    for (int j = 0; j < beam_info->beam_allocation_size; j++)
      beam_info->beam_allocation[i][j] = -1;
  }
}

static void config_sched_ctrlCommon(gNB_MAC_INST *nr_mac)
{
  const NR_MIB_t *mib = nr_mac->common_channels[0].mib->message.choice.mib;
  NR_ServingCellConfigCommon_t *scc = nr_mac->common_channels[0].ServingCellConfigCommon;

  NR_UE_sched_ctrl_t *sched_ctrlCommon = calloc_or_fail(1, sizeof(*sched_ctrlCommon));
  nr_mac->sched_ctrlCommon = sched_ctrlCommon;
  sched_ctrlCommon->search_space = calloc_or_fail(1, sizeof(*sched_ctrlCommon->search_space));
  sched_ctrlCommon->coreset = calloc_or_fail(1, sizeof(*sched_ctrlCommon->coreset));

  NR_SubcarrierSpacing_t scs = *scc->ssbSubcarrierSpacing;
  const long band = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
  uint16_t ssb_start_symbol = get_ssb_start_symbol(band, scs, 0);

  int8_t ssb_period = *scc->ssb_periodicityServingCell;
  uint8_t ssb_frame_periodicity = 1;
  if (ssb_period > 1)
    ssb_frame_periodicity = 1 << (ssb_period - 1);

  const int8_t numb_slots_frame = nr_mac->frame_structure.numb_slots_frame;
  frequency_range_t frequency_range = scc->ssb_PositionsInBurst->present == 3 ? FR2 : FR1;
  const int prb_offset = frequency_range == FR1 ? nr_mac->ssb_OffsetPointA >> scs : nr_mac->ssb_OffsetPointA >> (scs - 2);

  NR_Type0_PDCCH_CSS_config_t type0_PDCCH_CSS_config = {0};
  get_type0_PDCCH_CSS_config_parameters(&type0_PDCCH_CSS_config,
                                        0,
                                        mib,
                                        numb_slots_frame,
                                        nr_mac->ssb_SubcarrierOffset,
                                        ssb_start_symbol,
                                        scs,
                                        frequency_range,
                                        band,
                                        0,
                                        ssb_frame_periodicity,
                                        prb_offset);

  fill_searchSpaceZero(sched_ctrlCommon->search_space, numb_slots_frame, &type0_PDCCH_CSS_config);

  fill_coresetZero(sched_ctrlCommon->coreset, &type0_PDCCH_CSS_config);
  nr_mac->cset0_bwp_start = type0_PDCCH_CSS_config.cset_start_rb;
  nr_mac->cset0_bwp_size = type0_PDCCH_CSS_config.num_rbs;
}

/**
 * \brief Initializes the UL TDA information at the scheduler, as used for
 * every UE in UL. This just copies the information from the SCC into a smaller
 * list. Note that this list should not be modified, as get_num_ul_tda() and
 * nr_rrc_config_ul_tda() are designed to work in sync (get_num_ul_tda()
 * assumes an ordering, output by nr_rrc_config_ul_tda()).
 */
static void init_ul_tda_info(const NR_PUSCH_TimeDomainResourceAllocationList_t *l, seq_arr_t *sa_tda)
{
  DevAssert(l->list.count > 0 && l->list.count <= 16);
  DevAssert(seq_arr_size(sa_tda) == 0);

  for (int i = 0; i < l->list.count; ++i) {
    NR_PUSCH_TimeDomainResourceAllocation_t *t = l->list.array[i];
    DevAssert(t->k2);
    NR_tda_info_t tda = { .mapping_type = typeB, .k2 = *t->k2, .valid_tda = true };
    SLIV2SL(t->startSymbolAndLength, &tda.startSymbolIndex, &tda.nrOfSymbols);
    LOG_I(NR_MAC, "TDA index %d: start %d length %d k2 %ld\n", i, tda.startSymbolIndex, tda.nrOfSymbols, tda.k2);
    seq_arr_push_back(sa_tda, &tda, sizeof(tda));
  }
}

void nr_mac_config_scc(gNB_MAC_INST *nrmac, NR_ServingCellConfigCommon_t *scc, const nr_mac_config_t *config)
{
  DevAssert(nrmac != NULL);
  DevAssert(scc != NULL);
  DevAssert(config != NULL);

  AssertFatal(scc->ssb_PositionsInBurst->present > 0 && scc->ssb_PositionsInBurst->present < 4,
              "SSB Bitmap type %d is not valid\n",
              scc->ssb_PositionsInBurst->present);

  const int NTN_gNB_Koffset = get_NTN_Koffset(scc);
  const int n = get_slots_per_frame_from_scs(*scc->ssbSubcarrierSpacing);
  const int size = n << (int)ceil(log2((NTN_gNB_Koffset + 13) / n + 1)); // 13 is upper limit for max_fb_time
  nrmac->vrb_map_UL_size = size;

  int num_beams = 1;
  if(nrmac->beam_info.beam_mode != NO_BEAM_MODE)
    num_beams = nrmac->beam_info.beams_per_period;
  for (int i = 0; i < num_beams; i++) {
    nrmac->common_channels[0].vrb_map_UL[i] = calloc(size * MAX_BWP_SIZE, sizeof(uint16_t));
    AssertFatal(nrmac->common_channels[0].vrb_map_UL[i],
                "could not allocate memory for RC.nrmac[]->common_channels[0].vrb_map_UL[%d]\n", i);
  }

  nrmac->UL_tti_req_ahead_size = size;
  nrmac->UL_tti_req_ahead[0] = calloc(size, sizeof(nfapi_nr_ul_tti_request_t));
  AssertFatal(nrmac->UL_tti_req_ahead[0], "could not allocate memory for nrmac->UL_tti_req_ahead[0]\n");

  initialize_beam_information(&nrmac->beam_info, *scc->ssbSubcarrierSpacing, n);

  LOG_D(NR_MAC, "Configuring common parameters from NR ServingCellConfig\n");

  config_common(nrmac, config, scc);
  fapi_beam_index_allocation(scc, config, nrmac);

  if (NFAPI_MODE == NFAPI_MONOLITHIC) {
    // nothing to be sent in the other cases
    NR_PHY_Config_t phycfg = {.Mod_id = 0, .CC_id = 0, .cfg = &nrmac->config[0]};
    DevAssert(nrmac->if_inst->NR_PHY_config_req);
    nrmac->if_inst->NR_PHY_config_req(&phycfg);
  }

  find_SSB_and_RO_available(nrmac);

  if (IS_SA_MODE(get_softmodem_params()))
    config_sched_ctrlCommon(nrmac);

  seq_arr_init(&nrmac->ul_tda, sizeof(NR_tda_info_t));
  init_ul_tda_info(scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList, &nrmac->ul_tda);
}

bool nr_mac_configure_other_sib(gNB_MAC_INST *nrmac, int num_cu_sib, const f1ap_sib_msg_t cu_sib[num_cu_sib])
{
  NR_COMMON_channels_t *cc = &nrmac->common_channels[0];
  seq_arr_t *du_SIBs = nrmac->common_channels[0].du_SIBs;
  int num_du_sib = 0;
  if (du_SIBs)
    num_du_sib = du_SIBs->size;
  if (num_cu_sib + num_du_sib == 0)
    return false; /* no updates */

  int config_sibs[num_cu_sib + num_du_sib];
  NR_SystemInformation_IEs_t *sysInfo = calloc(1, sizeof(*sysInfo)); //for SIB up to 14 included
  NR_SystemInformation_IEs_t *sysInfov17 = calloc(1, sizeof(*sysInfov17)); // for othersibs
  for (int i = 0; i < num_cu_sib; i++) {
    config_sibs[i] = cu_sib[i].SI_type;
    switch (config_sibs[i]) {
      case 2: {
        struct NR_SystemInformation_IEs__sib_TypeAndInfo__Member *type = calloc(1, sizeof(*type));
        type->present = NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib2;
        // SIB2 coming from CU need to decode it
        NR_SIB2_t *sib2 = NULL;
        asn_dec_rval_t dec_rval = uper_decode(NULL,
                                              &asn_DEF_NR_SIB2,
                                              (void **)&sib2,
                                              cu_sib[i].SI_container,
                                              cu_sib[i].SI_container_length,
                                              0,
                                              0);
        if (dec_rval.code != RC_OK) {
          LOG_E(NR_MAC, "cannot decode SIB%d from CU\n", config_sibs[i]);
          ASN_STRUCT_FREE(asn_DEF_NR_SIB2, cu_sib[i].SI_container);
        }
        type->choice.sib2 = sib2;
        add_sib_to_systeminformation(sysInfo, type);
        break;
      }
      default :
        AssertFatal(false, "Invalid or not supported SIB%d\n", config_sibs[i]);
    }
  }

  for (int i = 0; i < num_du_sib; i++) {
    nr_SIBs_t *si = (nr_SIBs_t *)seq_arr_at(du_SIBs, i);
    int sib_idx = i + num_cu_sib;
    config_sibs[sib_idx] = si->SIB_type;
    switch (config_sibs[sib_idx]) {
      case 19: {
        struct NR_SystemInformation_IEs__sib_TypeAndInfo__Member *type_du = calloc(1, sizeof(*type_du));
        type_du->present = NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib19_v1700;
        NR_SIB19_r17_t *sib19 = get_SIB19_NR(cc->ServingCellConfigCommon);
        type_du->choice.sib19_v1700 = sib19;
        add_sib_to_systeminformation(sysInfov17, type_du);
        break;
      }
      default :
        AssertFatal(false, "Invalid or not supported SIB%d\n", sib_idx);
    }
  }

  update_SIB1_NR_SI(cc->sib1, num_cu_sib + num_du_sib, config_sibs);
  cc->sib1_bcch_length = encode_SIB_NR(cc->sib1, cc->sib1_bcch_pdu, sizeof(cc->sib1_bcch_pdu));
  AssertFatal(cc->sib1_bcch_length > 0, "could not encode SIB1\n");

  // generate otherSIB payloads
  cc->other_sib_bcch_length[0] = encode_sysinfo_ie(sysInfo, cc->other_sib_bcch_pdu[0], sizeof(cc->other_sib_bcch_pdu[0]));
  cc->other_sib_bcch_length[1] = encode_sysinfo_ie(sysInfov17, cc->other_sib_bcch_pdu[1], sizeof(cc->other_sib_bcch_pdu[1]));

  ASN_STRUCT_FREE(asn_DEF_NR_SystemInformation_IEs, sysInfo);
  ASN_STRUCT_FREE(asn_DEF_NR_SystemInformation_IEs, sysInfov17);
  return true;
}

bool nr_update_sib19(const gnb_sat_position_update_t *sat_position)
{
  gNB_MAC_INST *nrmac = RC.nrmac[0];
  NR_COMMON_channels_t *cc = &nrmac->common_channels[0];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;

  if (!scc || !scc->ext2 || !scc->ext2->ntn_Config_r17)
    return false;

  const vector_t *pos = &sat_position->position;
  const vector_t *vel = &sat_position->velocity;

  LOG_D(NR_MAC, "SFN = %d, SubFrame = %d\n", sat_position->sfn, sat_position->subframe);
  LOG_D(NR_MAC, "TA_Common: value %d, %f msec\n", sat_position->delay, sat_position->delay * 4.072e-6);
  LOG_D(NR_MAC, "TA_CommonDrift: value %d, = %f µsec/sec\n", sat_position->drift, sat_position->drift * 0.2e-3);
  LOG_D(NR_MAC, "TA_CommonDriftVariant: value %d, %f µsec/sec^2\n", sat_position->accel, sat_position->accel * 0.2e-4);
  LOG_D(NR_MAC, "SAT Position: values %d/%d/%d, %.3f/%3f/%3f metres in X/Y/Z\n", pos->X, pos->Y, pos->Z, pos->X * 1.3, pos->Y * 1.3, pos->Z * 1.3);
  LOG_D(NR_MAC, "SAT Velocity: values %d/%d/%d, %.3f/%3f/%3f m/s in X/Y/Z\n", vel->X, vel->Y, vel->Z, vel->X * 0.06, vel->Y * 0.06, vel->Z * 0.06);

  NR_SCHED_LOCK(&nrmac->sched_lock);

  if (!scc->ext2->ntn_Config_r17->epochTime_r17)
    scc->ext2->ntn_Config_r17->epochTime_r17 = calloc (1, sizeof(*scc->ext2->ntn_Config_r17->epochTime_r17));

  NR_EpochTime_r17_t *epoch_time_r17 = scc->ext2->ntn_Config_r17->epochTime_r17;
  epoch_time_r17->sfn_r17 = sat_position->sfn;
  epoch_time_r17->subFrameNR_r17 = sat_position->subframe;

  if (!scc->ext2->ntn_Config_r17->ta_Info_r17)
    scc->ext2->ntn_Config_r17->ta_Info_r17 = calloc(1, sizeof(*scc->ext2->ntn_Config_r17->ta_Info_r17));

  NR_TA_Info_r17_t *sib19_ta_info = scc->ext2->ntn_Config_r17->ta_Info_r17;

  // SIB19 provides Round trip delay on feeder link (between gNB and SAT).
  sib19_ta_info->ta_Common_r17 = sat_position->delay;

  if (sat_position->drift) {
    if (!sib19_ta_info->ta_CommonDrift_r17)
      sib19_ta_info->ta_CommonDrift_r17 = calloc(1, sizeof(*sib19_ta_info->ta_CommonDrift_r17));
    *sib19_ta_info->ta_CommonDrift_r17 = sat_position->drift;
  } else
    free_and_zero(sib19_ta_info->ta_CommonDrift_r17);

  if (sat_position->accel) {
    if (!sib19_ta_info->ta_CommonDriftVariant_r17)
      sib19_ta_info->ta_CommonDriftVariant_r17 = calloc(1, sizeof(*sib19_ta_info->ta_CommonDriftVariant_r17));
    *sib19_ta_info->ta_CommonDriftVariant_r17 = sat_position->accel;
  } else
    free_and_zero(sib19_ta_info->ta_CommonDriftVariant_r17);

  // Currently PositionVelocity is supported and not yet the OrbitalParams
  if (!scc->ext2->ntn_Config_r17->ephemerisInfo_r17) {
    scc->ext2->ntn_Config_r17->ephemerisInfo_r17 = calloc(1, sizeof(*scc->ext2->ntn_Config_r17->ephemerisInfo_r17));
    scc->ext2->ntn_Config_r17->ephemerisInfo_r17->present = NR_EphemerisInfo_r17_PR_NOTHING;
  }
  if (scc->ext2->ntn_Config_r17->ephemerisInfo_r17->present == NR_EphemerisInfo_r17_PR_orbital_r17) {
    ASN_STRUCT_FREE(asn_DEF_NR_Orbital_r17, scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.orbital_r17);
    scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.orbital_r17 = NULL;
    scc->ext2->ntn_Config_r17->ephemerisInfo_r17->present = NR_EphemerisInfo_r17_PR_NOTHING;
  }
  if (!scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.positionVelocity_r17)
    scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.positionVelocity_r17 =
        calloc(1, sizeof(*scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.positionVelocity_r17));

  scc->ext2->ntn_Config_r17->ephemerisInfo_r17->present = NR_EphemerisInfo_r17_PR_positionVelocity_r17;
  NR_PositionVelocity_r17_t *sib19_PosVel = scc->ext2->ntn_Config_r17->ephemerisInfo_r17->choice.positionVelocity_r17;

  sib19_PosVel->positionX_r17 = pos->X;
  sib19_PosVel->positionY_r17 = pos->Y;
  sib19_PosVel->positionZ_r17 = pos->Z;
  sib19_PosVel->velocityVX_r17 = vel->X;
  sib19_PosVel->velocityVY_r17 = vel->Y;
  sib19_PosVel->velocityVZ_r17 = vel->Z;

  NR_SystemInformation_IEs_t *sysInfov17 = calloc(1, sizeof(*sysInfov17)); // for othersibs
  struct NR_SystemInformation_IEs__sib_TypeAndInfo__Member *type_du = calloc(1, sizeof(*type_du));
  type_du->present = NR_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib19_v1700;
  NR_SIB19_r17_t *sib19 = get_SIB19_NR(cc->ServingCellConfigCommon);
  type_du->choice.sib19_v1700 = sib19;
  add_sib_to_systeminformation(sysInfov17, type_du);

  cc->other_sib_bcch_length[1] = encode_sysinfo_ie(sysInfov17, cc->other_sib_bcch_pdu[1], sizeof(cc->other_sib_bcch_pdu[1]));
  AssertFatal(cc->other_sib_bcch_length[1] > 0, "could not encode SIB19\n");
  ASN_STRUCT_FREE(asn_DEF_NR_SystemInformation_IEs, sysInfov17);

  NR_SCHED_UNLOCK(&nrmac->sched_lock);

  return true;
}

bool nr_trigger_bwp_switch(uint16_t rnti, int bwp_id)
{
  gNB_MAC_INST *nrmac = RC.nrmac[0];
  NR_SCHED_LOCK(&nrmac->sched_lock);
  NR_UE_info_t *UE = find_nr_UE(&nrmac->UE_info, rnti);
  bool success = false;
  if (!UE) {
    LOG_W(NR_MAC, "could not find UE for RNTI %04x\n", rnti);
  } else if (UE->current_DL_BWP.bwp_id == bwp_id) {
    LOG_W(NR_MAC, "UE %04x is already on BWP ID %d, not triggering reconfiguration\n", rnti, bwp_id);
  } else { // UE != NULL && current_DL_BWP.bwp_id != bwp_id
    nr_mac_trigger_reconfiguration(nrmac, UE, bwp_id);
    success = true;
  }
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
  return success;
}

void prepare_du_configuration_update(gNB_MAC_INST *mac,
                                     f1ap_served_cell_info_t *info,
                                     NR_BCCH_BCH_Message_t *mib,
                                     const NR_BCCH_DL_SCH_Message_t *sib1)
{
  /* send gNB-DU configuration update to RRC */
  f1ap_gnb_du_configuration_update_t update = {
    .transaction_id = 1,
    .num_status = 1,
    .status[0].plmn = info->plmn,
    .status[0].nr_cellid = info->nr_cellid,
    .status[0].service_state = F1AP_STATE_IN_SERVICE,
  };
  if (mib && sib1) {
    update.num_cells_to_modify = 1,
    update.cell_to_modify[0].old_nr_cellid = info->nr_cellid;
    update.cell_to_modify[0].info = *info;
    update.cell_to_modify[0].sys_info = get_sys_info(mib, sib1, NULL);
  }
  mac->mac_rrc.gnb_du_configuration_update(&update);
}

void nr_mac_configure_sib1(gNB_MAC_INST *nrmac, const plmn_id_t *plmn, uint64_t cellID, int tac, int num_plmn, const plmn_id_t *plmn_list)
{
  AssertFatal(IS_SA_MODE(get_softmodem_params()), "error: SIB1 only applicable for SA\n");

  // LOG_I(NR_MAC, "[cyhtest] nr_mac_configure_sib1: num_plmn=%d\n", num_plmn);
  // for (int i = 0; i < num_plmn; i++) {
  //   LOG_I(NR_MAC, "[cyhtest] nr_mac_configure_sib1: plmn_list[%d] = %d.%d (mnc_digit_length=%d)\n", 
  //         i, plmn_list[i].mcc, plmn_list[i].mnc, plmn_list[i].mnc_digit_length);
  // }

  NR_COMMON_channels_t *cc = &nrmac->common_channels[0];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  NR_BCCH_DL_SCH_Message_t *sib1 = get_SIB1_NR(scc, plmn, cellID, tac, &nrmac->radio_config, num_plmn, plmn_list);
  
  // Access SIB1 content through the message structure
  if (sib1 && sib1->message.choice.c1 && 
      sib1->message.choice.c1->present == NR_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1) {
    NR_SIB1_t *sib1_content = sib1->message.choice.c1->choice.systemInformationBlockType1;
    LOG_I(NR_MAC, "[cyhtest] after get_SIB1_NR: plmn_IdentityInfoList.list.count=%d\n", 
      sib1_content->cellAccessRelatedInfo.plmn_IdentityInfoList.list.count);
    for (int i = 0; i < sib1_content->cellAccessRelatedInfo.plmn_IdentityInfoList.list.count; i++) {
      NR_PLMN_IdentityInfo_t *plmn_info = sib1_content->cellAccessRelatedInfo.plmn_IdentityInfoList.list.array[i];
      LOG_I(NR_MAC, "[cyhtest] after get_SIB1_NR: plmn_IdentityInfoList.list.array[%d]->plmn_IdentityList.list.count=%d\n",
        i, plmn_info->plmn_IdentityList.list.count);
      for (int j = 0; j < plmn_info->plmn_IdentityList.list.count; j++) {
        NR_PLMN_Identity_t *plmn_id = plmn_info->plmn_IdentityList.list.array[j];
        LOG_I(NR_MAC, "[cyhtest] after get_SIB1_NR: plmn_IdentityList.list.array[%d][%d] mcc=%02lx%02lx%02lx mnc=%02lx%02lx%02lx\n",
          i, j,
          plmn_id->mcc ? *plmn_id->mcc->list.array[0] : 0,
          plmn_id->mcc ? *plmn_id->mcc->list.array[1] : 0,
          plmn_id->mcc ? *plmn_id->mcc->list.array[2] : 0,
          *plmn_id->mnc.list.array[0],
          *plmn_id->mnc.list.array[1],
          plmn_id->mnc.list.count > 2 ? *plmn_id->mnc.list.array[2] : 0);
      }
    }
  }
  
  cc->sib1 = sib1;
  cc->sib1_bcch_length = encode_SIB_NR(sib1, cc->sib1_bcch_pdu, sizeof(cc->sib1_bcch_pdu));
  AssertFatal(cc->sib1_bcch_length > 0, "could not encode SIB1\n");
}

static bool process_addmod_bearers_cellGroupConfig(NR_UE_sched_ctrl_t *sched_ctrl, const NR_RLC_BearerConfig_t *conf)
{
  int priority = conf->mac_LogicalChannelConfig->ul_SpecificParameters->priority;
  nr_lc_config_t c = {.lcid = conf->logicalChannelIdentity, .priority = priority};
  return nr_mac_add_lcid(sched_ctrl, &c);
}

bool nr_mac_add_test_ue(gNB_MAC_INST *nrmac, uint32_t rnti, NR_CellGroupConfig_t *CellGroup)
{
  /* ideally, instead of this function, "users" of this function should call
   * the ue context setup request function in mac_rrc_dl_handler.c */
  DevAssert(nrmac != NULL);
  DevAssert(CellGroup != NULL);
  DevAssert(get_softmodem_params()->phy_test);
  NR_SCHED_LOCK(&nrmac->sched_lock);

  NR_UE_info_t *UE = get_new_nr_ue_inst(&nrmac->UE_info.uid_allocator, rnti, CellGroup);
  DevAssert(UE->uid < MAX_MOBILES_PER_GNB); // physical simulators: we assume we can always create a UE
  free_and_zero(UE->ra); // physical simulators: UE will not do RA
  UE->local_bwp_id = 1;  // for physical simulators
  bool res = add_connected_nr_ue(nrmac, UE);
  if (!res) {
    LOG_E(NR_MAC, "Error adding UE %04x\n", rnti);
    delete_nr_ue_data(UE, NULL, &nrmac->UE_info.uid_allocator);
    NR_SCHED_UNLOCK(&nrmac->sched_lock);
    return false;
  }
  int ss_type = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
  configure_UE_BWP(nrmac, nrmac->common_channels[0].ServingCellConfigCommon, UE, false, ss_type, -1, -1);
  const struct NR_CellGroupConfig__rlc_BearerToAddModList *l = CellGroup->rlc_BearerToAddModList;
  for (int i = 0; l != NULL && i < l->list.count; ++i)
    process_addmod_bearers_cellGroupConfig(&UE->UE_sched_ctrl, CellGroup->rlc_BearerToAddModList->list.array[i]);
  AssertFatal(CellGroup->rlc_BearerToReleaseList == NULL, "cannot release bearers while adding new UEs\n");
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
  LOG_I(NR_MAC, "Added new UE %x with initial CellGroup\n", rnti);
  return true;
}

void nr_mac_prepare_ra_ue(gNB_MAC_INST *nrmac, NR_UE_info_t *UE)
{
  DevAssert(nrmac != NULL);
  NR_SCHED_ENSURE_LOCKED(&nrmac->sched_lock);
  NR_RA_t *ra = UE->ra;
  ra->cfra = true;
  NR_CellGroupConfig_t *CellGroup = UE->CellGroup;
  DevAssert(CellGroup != NULL);
  struct NR_CFRA *cfra = CellGroup->spCellConfig->reconfigurationWithSync->rach_ConfigDedicated->choice.uplink->cfra;
  uint8_t num_preamble = cfra->resources.choice.ssb->ssb_ResourceList.list.count;
  ra->preambles.num_preambles = num_preamble;
  NR_COMMON_channels_t *cc = &nrmac->common_channels[0];
  char buf[200];
  int idx = 0;
  for (int i = 0; i < cc->num_active_ssb; i++) {
    for (int j = 0; j < num_preamble; j++) {
      if (cc->ssb_index[i] == cfra->resources.choice.ssb->ssb_ResourceList.list.array[j]->ssb) {
        // one dedicated preamble for each beam
        ra->preambles.preamble_list[i] = cfra->resources.choice.ssb->ssb_ResourceList.list.array[j]->ra_PreambleIndex;
        if (idx < sizeof(buf) - 1)
          idx += snprintf(buf + idx, sizeof(buf) - idx, "  %d", ra->preambles.preamble_list[i]);
        break;
      }
    }
  }
  LOG_I(NR_MAC,
        "Added new %s process for UE RNTI %04x with initial CellGroup and %d preamble(s): %s\n",
        ra->cfra ? "CFRA" : "CBRA",
        UE->rnti,
        num_preamble,
        buf);
}
