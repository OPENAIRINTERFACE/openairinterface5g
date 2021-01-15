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

/*! \file rar_tools_nrUE.c
 * \brief RA tools for NR UE
 * \author Guido Casati
 * \date 2019
 * \version 1.0
 * @ingroup _mac

 */

/* Sim */
#include "SIMULATION/TOOLS/sim.h"

/* Utils */
#include "common/utils/LOG/log.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "UTIL/OPT/opt.h"

/* Common */
#include "common/ran_context.h"

/* PHY */
#include "openair1/SCHED_NR_UE/defs.h"

/* MAC */
#include "NR_MAC_UE/mac_proto.h"
#include "NR_MAC_UE/mac_extern.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include <common/utils/nr/nr_common.h>

// #define DEBUG_RAR
// #define DEBUG_MSG3

void ul_layers_config(NR_UE_MAC_INST_t * mac, nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu, dci_pdu_rel15_t *dci) {

  fapi_nr_pusch_config_dedicated_t *pusch_config_dedicated = &mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated;

  /* PRECOD_NBR_LAYERS */
  if ((pusch_config_dedicated->tx_config == tx_config_nonCodebook));
  // 0 bits if the higher layer parameter txConfig = nonCodeBook

  if ((pusch_config_dedicated->tx_config == tx_config_codebook)){

    uint8_t n_antenna_port = 0; //FIXME!!!

    if (n_antenna_port == 1); // 1 antenna port and the higher layer parameter txConfig = codebook 0 bits

    if (n_antenna_port == 4){ // 4 antenna port and the higher layer parameter txConfig = codebook

      // Table 7.3.1.1.2-2: transformPrecoder=disabled and maxRank = 2 or 3 or 4
      if ((pusch_config_dedicated->transform_precoder == transform_precoder_disabled)
        && ((pusch_config_dedicated->max_rank == 2) ||
        (pusch_config_dedicated->max_rank == 3) ||
        (pusch_config_dedicated->max_rank == 4))){

        if (pusch_config_dedicated->codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
          pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][0];
          pusch_config_pdu->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][1];
        }

        if (pusch_config_dedicated->codebook_subset == codebook_subset_partialAndNonCoherent){
          pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][2];
          pusch_config_pdu->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][3];
        }

        if (pusch_config_dedicated->codebook_subset == codebook_subset_nonCoherent){
          pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][4];
          pusch_config_pdu->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][5];
        }
      }

      // Table 7.3.1.1.2-3: transformPrecoder= enabled, or transformPrecoder=disabled and maxRank = 1
      if (((pusch_config_dedicated->transform_precoder == transform_precoder_enabled)
        || (pusch_config_dedicated->transform_precoder == transform_precoder_disabled))
        && (pusch_config_dedicated->max_rank == 1)){

        if (pusch_config_dedicated->codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
          pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][6];
          pusch_config_pdu->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][7];
        }

        if (pusch_config_dedicated->codebook_subset == codebook_subset_partialAndNonCoherent){
          pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][8];
          pusch_config_pdu->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][9];
        }

        if (pusch_config_dedicated->codebook_subset == codebook_subset_nonCoherent){
          pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][10];
          pusch_config_pdu->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][11];
        }
      }
    }

    if (n_antenna_port == 4){ // 2 antenna port and the higher layer parameter txConfig = codebook
      // Table 7.3.1.1.2-4: transformPrecoder=disabled and maxRank = 2
      if ((pusch_config_dedicated->transform_precoder == transform_precoder_disabled) && (pusch_config_dedicated->max_rank == 2)){

        if (pusch_config_dedicated->codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
          pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][12];
          pusch_config_pdu->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][13];
        }

        if (pusch_config_dedicated->codebook_subset == codebook_subset_nonCoherent){
          pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][14];
          pusch_config_pdu->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][15];
        }

      }

      // Table 7.3.1.1.2-5: transformPrecoder= enabled, or transformPrecoder= disabled and maxRank = 1
      if (((pusch_config_dedicated->transform_precoder == transform_precoder_enabled)
        || (pusch_config_dedicated->transform_precoder == transform_precoder_disabled))
        && (pusch_config_dedicated->max_rank == 1)){

        if (pusch_config_dedicated->codebook_subset == codebook_subset_fullyAndPartialAndNonCoherent) {
          pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][16];
          pusch_config_pdu->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][17];
        }

        if (pusch_config_dedicated->codebook_subset == codebook_subset_nonCoherent){
          pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][18];
          pusch_config_pdu->transform_precoding = table_7_3_1_1_2_2_3_4_5[dci->precoding_information.val][19];
        }

      }
    }
  }
}

// todo: this function shall be reviewed completely because of the many comments left by the author
void ul_ports_config(NR_UE_MAC_INST_t * mac, nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu, dci_pdu_rel15_t *dci) {

  /* ANTENNA_PORTS */
  uint8_t rank = 0; // We need to initialize rank FIXME!!!
  fapi_nr_pusch_config_dedicated_t *pusch_config_dedicated = &mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated;

  if ((pusch_config_dedicated->transform_precoder == transform_precoder_enabled) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.max_length == 1)) { // tables 7.3.1.1.2-6
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = 2; //TBC
      pusch_config_pdu->dmrs_ports = dci->antenna_ports.val; //TBC
  }

  if ((pusch_config_dedicated->transform_precoder == transform_precoder_enabled) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.max_length == 2)) { // tables 7.3.1.1.2-7

    pusch_config_pdu->num_dmrs_cdm_grps_no_data = 2; //TBC
    pusch_config_pdu->dmrs_ports = (dci->antenna_ports.val > 3)?(dci->antenna_ports.val-4):(dci->antenna_ports.val); //TBC
    //pusch_config_pdu->n_front_load_symb = (dci->antenna_ports > 3)?2:1; //FIXME
  }

  if ((pusch_config_dedicated->transform_precoder == transform_precoder_disabled) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.max_length == 1)) { // tables 7.3.1.1.2-8/9/10/11

    if (rank == 1) {
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 1)?2:1; //TBC
      pusch_config_pdu->dmrs_ports = (dci->antenna_ports.val > 1)?(dci->antenna_ports.val-2):(dci->antenna_ports.val); //TBC
    }

    if (rank == 2){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 0)?2:1; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = (dci->antenna_ports > 1)?(dci->antenna_ports > 2 ?0:2):0;
      //pusch_config_pdu->dmrs_ports[1] = (dci->antenna_ports > 1)?(dci->antenna_ports > 2 ?2:3):1;
    }

    if (rank == 3){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = 2; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = 0;
      //pusch_config_pdu->dmrs_ports[1] = 1;
      //pusch_config_pdu->dmrs_ports[2] = 2;
    }

    if (rank == 4){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = 2; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = 0;
      //pusch_config_pdu->dmrs_ports[1] = 1;
      //pusch_config_pdu->dmrs_ports[2] = 2;
      //pusch_config_pdu->dmrs_ports[3] = 3;
    }
  }

  if ((pusch_config_dedicated->transform_precoder == transform_precoder_disabled) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 1) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.max_length == 2)) { // tables 7.3.1.1.2-12/13/14/15

    if (rank == 1){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 1)?2:1; //TBC
      pusch_config_pdu->dmrs_ports = (dci->antenna_ports.val > 1)?(dci->antenna_ports.val > 5 ?(dci->antenna_ports.val-6):(dci->antenna_ports.val-2)):dci->antenna_ports.val; //TBC
      //pusch_config_pdu->n_front_load_symb = (dci->antenna_ports.val > 6)?2:1; //FIXME
    }

    if (rank == 2){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 0)?2:1; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = table_7_3_1_1_2_13[dci->antenna_ports.val][1];
      //pusch_config_pdu->dmrs_ports[1] = table_7_3_1_1_2_13[dci->antenna_ports.val][2];
      //pusch_config_pdu->n_front_load_symb = (dci->antenna_ports.val > 3)?2:1; // FIXME
    }

    if (rank == 3){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = 2; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = table_7_3_1_1_2_14[dci->antenna_ports.val][1];
      //pusch_config_pdu->dmrs_ports[1] = table_7_3_1_1_2_14[dci->antenna_ports.val][2];
      //pusch_config_pdu->dmrs_ports[2] = table_7_3_1_1_2_14[dci->antenna_ports.val][3];
      //pusch_config_pdu->n_front_load_symb = (dci->antenna_ports.val > 1)?2:1; //FIXME
    }

    if (rank == 4){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = 2; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = table_7_3_1_1_2_15[dci->antenna_ports.val][1];
      //pusch_config_pdu->dmrs_ports[1] = table_7_3_1_1_2_15[dci->antenna_ports.val][2];
      //pusch_config_pdu->dmrs_ports[2] = table_7_3_1_1_2_15[dci->antenna_ports.val][3];
      //pusch_config_pdu->dmrs_ports[3] = table_7_3_1_1_2_15[dci->antenna_ports.val][4];
      //pusch_config_pdu->n_front_load_symb = (dci->antenna_ports.val > 1)?2:1; //FIXME
    }
  }

  if ((pusch_config_dedicated->transform_precoder == transform_precoder_disabled) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 2) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.max_length == 1)) { // tables 7.3.1.1.2-16/17/18/19

    if (rank == 1){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 1)?((dci->antenna_ports.val > 5)?3:2):1; //TBC
      pusch_config_pdu->dmrs_ports = (dci->antenna_ports.val > 1)?(dci->antenna_ports.val > 5 ?(dci->antenna_ports.val-6):(dci->antenna_ports.val-2)):dci->antenna_ports.val; //TBC
    }

    if (rank == 2){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 0)?((dci->antenna_ports.val > 2)?3:2):1; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = table_7_3_1_1_2_17[dci->antenna_ports.val][1];
      //pusch_config_pdu->dmrs_ports[1] = table_7_3_1_1_2_17[dci->antenna_ports.val][2];
    }

    if (rank == 3){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = (dci->antenna_ports.val > 0)?3:2; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = table_7_3_1_1_2_18[dci->antenna_ports.val][1];
      //pusch_config_pdu->dmrs_ports[1] = table_7_3_1_1_2_18[dci->antenna_ports.val][2];
      //pusch_config_pdu->dmrs_ports[2] = table_7_3_1_1_2_18[dci->antenna_ports.val][3];
    }

    if (rank == 4){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = dci->antenna_ports.val + 2; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = 0;
      //pusch_config_pdu->dmrs_ports[1] = 1;
      //pusch_config_pdu->dmrs_ports[2] = 2;
      //pusch_config_pdu->dmrs_ports[3] = 3;
    }
  }

  if ((pusch_config_dedicated->transform_precoder == transform_precoder_disabled) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.dmrs_type == 2) &&
    (pusch_config_dedicated->dmrs_ul_for_pusch_mapping_type_a.max_length == 2)) { // tables 7.3.1.1.2-20/21/22/23

    if (rank == 1){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = table_7_3_1_1_2_20[dci->antenna_ports.val][0]; //TBC
      pusch_config_pdu->dmrs_ports = table_7_3_1_1_2_20[dci->antenna_ports.val][1]; //TBC
      //pusch_config_pdu->n_front_load_symb = table_7_3_1_1_2_20[dci->antenna_ports.val][2]; //FIXME
    }

    if (rank == 2){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = table_7_3_1_1_2_21[dci->antenna_ports.val][0]; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = table_7_3_1_1_2_21[dci->antenna_ports.val][1];
      //pusch_config_pdu->dmrs_ports[1] = table_7_3_1_1_2_21[dci->antenna_ports.val][2];
      //pusch_config_pdu->n_front_load_symb = table_7_3_1_1_2_21[dci->antenna_ports.val][3]; //FIXME
      }

    if (rank == 3){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = table_7_3_1_1_2_22[dci->antenna_ports.val][0]; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = table_7_3_1_1_2_22[dci->antenna_ports.val][1];
      //pusch_config_pdu->dmrs_ports[1] = table_7_3_1_1_2_22[dci->antenna_ports.val][2];
      //pusch_config_pdu->dmrs_ports[2] = table_7_3_1_1_2_22[dci->antenna_ports.val][3];
      //pusch_config_pdu->n_front_load_symb = table_7_3_1_1_2_22[dci->antenna_ports.val][4]; //FIXME
    }

    if (rank == 4){
      pusch_config_pdu->num_dmrs_cdm_grps_no_data = table_7_3_1_1_2_23[dci->antenna_ports.val][0]; //TBC
      pusch_config_pdu->dmrs_ports = 0; //FIXME
      //pusch_config_pdu->dmrs_ports[0] = table_7_3_1_1_2_23[dci->antenna_ports.val][1];
      //pusch_config_pdu->dmrs_ports[1] = table_7_3_1_1_2_23[dci->antenna_ports.val][2];
      //pusch_config_pdu->dmrs_ports[2] = table_7_3_1_1_2_23[dci->antenna_ports.val][3];
      //pusch_config_pdu->dmrs_ports[3] = table_7_3_1_1_2_23[dci->antenna_ports.val][4];
      //pusch_config_pdu->n_front_load_symb = table_7_3_1_1_2_23[dci->antenna_ports.val][5]; //FIXME
    }
  }
}

// Configuration of Msg3 PDU according to clauses:
// - 8.3 of 3GPP TS 38.213 version 16.3.0 Release 16
// - 6.1.2.2 of TS 38.214
// - 6.1.3 of TS 38.214
// - 6.2.2 of TS 38.214
// - 6.1.4.2 of TS 38.214
// - 6.4.1.1.1 of TS 38.211
// - 6.3.1.7 of 38.211
int nr_config_pusch_pdu(NR_UE_MAC_INST_t *mac,
                        nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu,
                        dci_pdu_rel15_t *dci,
                        RAR_grant_t *rar_grant,
                        uint16_t rnti,
                        uint8_t *dci_format){

  int f_alloc;
  int mask;
  int StartSymbolIndex;
  int NrOfSymbols;
  uint8_t nb_dmrs_re_per_rb;

  uint16_t        l_prime_mask = 1;
  uint16_t number_dmrs_symbols = 0;
  int                N_PRB_oh  = 0;

  NR_ServingCellConfigCommon_t *scc = mac->scc;
  int rnti_type = get_rnti_type(mac, rnti);

  // Common configuration
  pusch_config_pdu->dmrs_config_type = pusch_dmrs_type1;
  pusch_config_pdu->pdu_bit_map      = PUSCH_PDU_BITMAP_PUSCH_DATA;
  pusch_config_pdu->nrOfLayers       = 1;
  pusch_config_pdu->rnti             = rnti;

  if (rar_grant) {

    // Note: for Msg3 or MsgA PUSCH transmission the N_PRB_oh is always set to 0

    NR_BWP_Uplink_t *ubwp = mac->ULbwp[0];
    NR_BWP_UplinkDedicated_t *ibwp = mac->scg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP;
    NR_PUSCH_Config_t *pusch_Config = ibwp->pusch_Config->choice.setup;
    int startSymbolAndLength = ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[rar_grant->Msg3_t_alloc]->startSymbolAndLength;

    // active BWP start
    int abwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    int abwp_size = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

    // initial BWP start
    int ibwp_start = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    int ibwp_size = NRRIV2BW(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

    // BWP start selection according to 8.3 of TS 38.213
    pusch_config_pdu->bwp_size = ibwp_size;
    if ((ibwp_start < abwp_start) || (ibwp_size > abwp_size))
      pusch_config_pdu->bwp_start = abwp_start;
    else
      pusch_config_pdu->bwp_start = ibwp_start;

    //// Resource assignment from RAR
    // Frequency domain allocation according to 8.3 of TS 38.213
    if (ibwp_size < 180)
      mask = (1 << ((int) ceil(log2((ibwp_size*(ibwp_size+1))>>1)))) - 1;
    else
      mask = (1 << (28 - (int)(ceil(log2((ibwp_size*(ibwp_size+1))>>1))))) - 1;
    f_alloc = rar_grant->Msg3_f_alloc & mask;
    nr_ue_process_dci_freq_dom_resource_assignment(pusch_config_pdu, NULL, ibwp_size, 0, f_alloc);

    // virtual resource block to physical resource mapping for Msg3 PUSCH (6.3.1.7 in 38.211)
    pusch_config_pdu->rb_start += ibwp_start - abwp_start;

    // Time domain allocation
    SLIV2SL(startSymbolAndLength, &StartSymbolIndex, &NrOfSymbols);
    pusch_config_pdu->start_symbol_index = StartSymbolIndex;
    pusch_config_pdu->nr_of_symbols = NrOfSymbols;

    #ifdef DEBUG_MSG3
    LOG_D(MAC, "In %s BWP assignment (BWP (start %d, size %d) \n", __FUNCTION__, pusch_config_pdu->bwp_start, pusch_config_pdu->bwp_size);
    #endif

    // MCS
    pusch_config_pdu->mcs_index = rar_grant->mcs;
    // Frequency hopping
    pusch_config_pdu->frequency_hopping = rar_grant->freq_hopping;

    // DM-RS configuration according to 6.2.2 UE DM-RS transmission procedure in 38.214
    pusch_config_pdu->num_dmrs_cdm_grps_no_data = 2;
    pusch_config_pdu->dmrs_ports = 1;

    // DMRS sequence initialization [TS 38.211, sec 6.4.1.1.1].
    // Should match what is sent in DCI 0_1, otherwise set to 0.
    pusch_config_pdu->scid = 0;

    // Transform precoding according to 6.1.3 UE procedure for applying transform precoding on PUSCH in 38.214
    pusch_config_pdu->transform_precoding = get_transformPrecoding(scc, pusch_Config, NULL, NULL, NR_RNTI_RA, 0); // TBR fix rnti and take out

    // Resource allocation in frequency domain according to 6.1.2.2 in TS 38.214
    pusch_config_pdu->resource_alloc = pusch_Config->resourceAllocation;

    //// Completing PUSCH PDU
    pusch_config_pdu->mcs_table = 0;
    pusch_config_pdu->cyclic_prefix = 0;
    pusch_config_pdu->data_scrambling_id = *scc->physCellId;
    pusch_config_pdu->ul_dmrs_scrambling_id = *scc->physCellId;
    pusch_config_pdu->subcarrier_spacing = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
    pusch_config_pdu->vrb_to_prb_mapping = 0;
    pusch_config_pdu->uplink_frequency_shift_7p5khz = 0;
    //Optional Data only included if indicated in pduBitmap
    pusch_config_pdu->pusch_data.rv_index = 0;  // 8.3 in 38.213
    pusch_config_pdu->pusch_data.harq_process_id = 0;
    pusch_config_pdu->pusch_data.new_data_indicator = 1; // new data
    pusch_config_pdu->pusch_data.num_cb = 0;

  } else if (dci) {

    int target_ss;
    uint8_t  ptrs_time_density;
    uint8_t  ptrs_freq_density;
    nfapi_nr_ue_ptrs_ports_t ptrs_ports_list;
    uint16_t n_RB_ULBWP = NRRIV2BW(mac->ULbwp[0]->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    fapi_nr_pusch_config_dedicated_t *pusch_config_dedicated = &mac->phy_config.config_req.ul_bwp_dedicated.pusch_config_dedicated;
    NR_PUSCH_Config_t *pusch_Config = mac->ULbwp[0]->bwp_Dedicated->pusch_Config->choice.setup;

    // These should come from RRC config!!!
    uint8_t  ptrs_mcs1          = 2;
    uint8_t  ptrs_mcs2          = 4;
    uint8_t  ptrs_mcs3          = 10;
    uint16_t n_rb0              = 25;
    uint16_t n_rb1              = 75;

    /* Transform precoding */
    if (rnti_type != NR_RNTI_CS || (rnti_type == NR_RNTI_CS && dci->ndi == 1)) {
      pusch_config_pdu->transform_precoding = get_transformPrecoding(scc, pusch_Config, NULL, dci_format, rnti_type, 0);
    }

    /*DCI format-related configuration*/
    if (*dci_format == NR_UL_DCI_FORMAT_0_0) {

      target_ss = NR_SearchSpace__searchSpaceType_PR_common;

    } else if (*dci_format == NR_UL_DCI_FORMAT_0_1) {

      /* BANDWIDTH_PART_IND */
      config_bwp_ue(mac, &dci->bwp_indicator.val, dci_format);
      target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;
      ul_layers_config(mac, pusch_config_pdu, dci);
      ul_ports_config(mac, pusch_config_pdu, dci);

    } else {

      LOG_E(MAC, "In %s: UL grant from DCI format %d is not handled...\n", __FUNCTION__, *dci_format);
      return -1;

    }

    /* IDENTIFIER_DCI_FORMATS */
    /* FREQ_DOM_RESOURCE_ASSIGNMENT_UL */
    if (nr_ue_process_dci_freq_dom_resource_assignment(pusch_config_pdu, NULL, n_RB_ULBWP, 0, dci->frequency_domain_assignment.val) < 0){
      return -1;
    }
    /* TIME_DOM_RESOURCE_ASSIGNMENT */
    if (nr_ue_process_dci_time_dom_resource_assignment(mac, pusch_config_pdu, NULL, dci->time_domain_assignment.val) < 0) {
      return -1;
    }

    /* FREQ_HOPPING_FLAG */
    if ((pusch_config_dedicated->resource_allocation != 0) && (pusch_config_dedicated->frequency_hopping !=0)){
      pusch_config_pdu->frequency_hopping = dci->frequency_hopping_flag.val;
    }

    /* MCS */
    pusch_config_pdu->mcs_index = dci->mcs;

    /* MCS TABLE */
    if (pusch_config_pdu->transform_precoding == transform_precoder_disabled) {
      pusch_config_pdu->mcs_table = get_pusch_mcs_table(pusch_Config->mcs_Table, 0, *dci_format, rnti_type, target_ss, false);
    } else {
      pusch_config_pdu->mcs_table = get_pusch_mcs_table(pusch_Config->mcs_TableTransformPrecoder, 1, *dci_format, rnti_type, target_ss, false);
    }

    /* NDI */
    pusch_config_pdu->pusch_data.new_data_indicator = dci->ndi;
    /* RV */
    pusch_config_pdu->pusch_data.rv_index = dci->rv;
    /* HARQ_PROCESS_NUMBER */
    pusch_config_pdu->pusch_data.harq_process_id = dci->harq_pid;
    /* TPC_PUSCH */
    // according to TS 38.213 Table Table 7.1.1-1
    if (dci->tpc == 0) {
      pusch_config_pdu->absolute_delta_PUSCH = -4;
    }
    if (dci->tpc == 1) {
      pusch_config_pdu->absolute_delta_PUSCH = -1;
    }
    if (dci->tpc == 2) {
      pusch_config_pdu->absolute_delta_PUSCH = 1;
    }
    if (dci->tpc == 3) {
      pusch_config_pdu->absolute_delta_PUSCH = 4;
    }

    ptrs_time_density  = get_L_ptrs(ptrs_mcs1, ptrs_mcs2, ptrs_mcs3, pusch_config_pdu->mcs_index, pusch_config_pdu->mcs_table);
    ptrs_freq_density  = get_K_ptrs(n_rb0, n_rb1, pusch_config_pdu->rb_size);
    // PTRS ports configuration
    // TbD: ptrs_dmrs_port and ptrs_port_index are not initialised!
    ptrs_ports_list.ptrs_re_offset = 0;

    /* DMRS */
    l_prime_mask = get_l_prime(pusch_config_pdu->nr_of_symbols, typeB, pusch_dmrs_pos0, pusch_len1);
    pusch_config_pdu->num_dmrs_cdm_grps_no_data = 1;

    // Num PRB Overhead from PUSCH-ServingCellConfig
    if (mac->scg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->xOverhead == NULL) {
      N_PRB_oh = 0;
    } else {
      N_PRB_oh = *mac->scg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->xOverhead;
    }

    /* PTRS */
    pusch_config_pdu->pusch_ptrs.ptrs_time_density = ptrs_time_density;
    pusch_config_pdu->pusch_ptrs.ptrs_freq_density = ptrs_freq_density;
    pusch_config_pdu->pusch_ptrs.ptrs_ports_list   = &ptrs_ports_list;

    if (1 << pusch_config_pdu->pusch_ptrs.ptrs_time_density >= pusch_config_pdu->nr_of_symbols) {
      pusch_config_pdu->pdu_bit_map &= ~PUSCH_PDU_BITMAP_PUSCH_PTRS; // disable PUSCH PTRS
    }

  }

  LOG_D(MAC, "In %s: received UL grant (rb_start %d, rb_size %d, start_symbol_index %d, nr_of_symbols %d) for RNTI type %s \n",
    __FUNCTION__,
    pusch_config_pdu->rb_start,
    pusch_config_pdu->rb_size,
    pusch_config_pdu->start_symbol_index,
    pusch_config_pdu->nr_of_symbols,
    rnti_types[rnti_type]);

  pusch_config_pdu->ul_dmrs_symb_pos = l_prime_mask << pusch_config_pdu->start_symbol_index;;
  pusch_config_pdu->target_code_rate = nr_get_code_rate_ul(pusch_config_pdu->mcs_index, pusch_config_pdu->mcs_table);
  pusch_config_pdu->qam_mod_order = nr_get_Qm_ul(pusch_config_pdu->mcs_index, pusch_config_pdu->mcs_table);

  if (pusch_config_pdu->target_code_rate == 0 || pusch_config_pdu->qam_mod_order == 0) {
    LOG_W(MAC, "In %s: Invalid code rate or Mod order, likely due to unexpected UL DCI. Ignoring DCI! \n", __FUNCTION__);
    return -1;
  }

  get_num_re_dmrs(pusch_config_pdu, &nb_dmrs_re_per_rb, &number_dmrs_symbols);

  // Compute TBS
  pusch_config_pdu->pusch_data.tb_size = nr_compute_tbs(pusch_config_pdu->qam_mod_order,
                                                        pusch_config_pdu->target_code_rate,
                                                        pusch_config_pdu->rb_size,
                                                        pusch_config_pdu->nr_of_symbols,
                                                        nb_dmrs_re_per_rb*number_dmrs_symbols,
                                                        N_PRB_oh,
                                                        0, // TBR to verify tb scaling
                                                        pusch_config_pdu->nrOfLayers)/8;

  return 0;

}

/////////////////////////////////////
//    Random Access Response PDU   //
//         TS 38.213 ch 8.2        //
//        TS 38.321 ch 6.2.3       //
/////////////////////////////////////
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |// bit-wise
//| E | T |       R A P I D       |//
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |//
//| R |           T A             |//
//|       T A         |  UL grant |//
//|            UL grant           |//
//|            UL grant           |//
//|            UL grant           |//
//|         T C - R N T I         |//
//|         T C - R N T I         |//
/////////////////////////////////////
//       UL grant  (27 bits)       //
/////////////////////////////////////
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |// bit-wise
//|-------------------|FHF|F_alloc|//
//|        Freq allocation        |//
//|    F_alloc    |Time allocation|//
//|      MCS      |     TPC   |CSI|//
/////////////////////////////////////
// TbD WIP Msg3 development ongoing
// - apply UL grant freq alloc & time alloc as per 8.2 TS 38.213
// - apply tpc command
// WIP fix:
// - time domain indication hardcoded to 0 for k2 offset
// - extend TS 38.213 ch 8.3 Msg3 PUSCH
// - b buffer
// - ulsch power offset
// - optimize: mu_pusch, j and table_6_1_2_1_1_2_time_dom_res_alloc_A are already defined in nr_ue_procedures
int nr_ue_process_rar(nr_downlink_indication_t *dl_info, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment, int pdu_id){

  module_id_t mod_id       = dl_info->module_id;
  frame_t frame            = dl_info->frame;
  int slot                 = dl_info->slot;
  int cc_id                = dl_info->cc_id;
  uint8_t gNB_id           = dl_info->gNB_index;
  NR_UE_MAC_INST_t *mac    = get_mac_inst(mod_id);
  RA_config_t *ra          = &mac->ra;
  uint8_t n_subPDUs        = 0;  // number of RAR payloads
  uint8_t n_subheaders     = 0;  // number of MAC RAR subheaders
  uint8_t *dlsch_buffer    = dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.pdu;
  uint8_t is_Msg3          = 1;
  frame_t frame_tx         = 0;
  int slot_tx              = 0;
  uint16_t rnti            = 0;
  int ret                  = 0;
  NR_RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *) dlsch_buffer; // RAR subheader pointer
  NR_MAC_RAR *rar          = (NR_MAC_RAR *) (dlsch_buffer + 1);   // RAR subPDU pointer
  uint8_t preamble_index   = get_ra_PreambleIndex(mod_id, cc_id, gNB_id); //prach_resources->ra_PreambleIndex;

  LOG_D(MAC, "In %s:[%d.%d]: [UE %d][RAPROC] invoking MAC for received RAR (current preamble %d)\n", __FUNCTION__, frame, slot, mod_id, preamble_index);

  while (1) {
    n_subheaders++;
    if (rarh->T == 1) {
      n_subPDUs++;
      LOG_D(MAC, "[UE %d][RAPROC] Got RAPID RAR subPDU\n", mod_id);
    } else {
      n_subPDUs++;
      ra->RA_backoff_indicator = table_7_2_1[((NR_RA_HEADER_BI *)rarh)->BI];
      ra->RA_BI_found = 1;
      LOG_D(MAC, "[UE %d][RAPROC] Got BI RAR subPDU %d\n", mod_id, ra->RA_backoff_indicator);
    }
    if (rarh->RAPID == preamble_index) {
      LOG_I(MAC, "[UE %d][RAPROC][%d.%d] Found RAR with the intended RAPID %d\n", mod_id, frame, slot, rarh->RAPID);
      rar = (NR_MAC_RAR *) (dlsch_buffer + n_subheaders + (n_subPDUs - 1) * sizeof(NR_MAC_RAR));
      ra->RA_RAPID_found = 1;
      break;
    }
    if (rarh->E == 0) {
      LOG_W(PHY,"[UE %d][RAPROC] Received RAR preamble (%d) doesn't match the intended RAPID...\n", mod_id, preamble_index);
      break;
    } else {
      rarh += sizeof(NR_MAC_RAR) + 1;
    }
  }

  #ifdef DEBUG_RAR
  LOG_D(MAC, "[DEBUG_RAR] (%d,%d) number of RAR subheader %d; number of RAR pyloads %d\n", frame, slot, n_subheaders, n_subPDUs);
  LOG_D(MAC, "[DEBUG_RAR] Received RAR (%02x|%02x.%02x.%02x.%02x.%02x.%02x) for preamble %d/%d\n", *(uint8_t *) rarh, rar[0], rar[1], rar[2], rar[3], rar[4], rar[5], rarh->RAPID, preamble_index);
  #endif

  if (ra->RA_RAPID_found) {

    RAR_grant_t rar_grant;

    unsigned char tpc_command;
#ifdef DEBUG_RAR
    unsigned char csi_req;
#endif

  // TC-RNTI
  ra->t_crnti = rar->TCRNTI_2 + (rar->TCRNTI_1 << 8);

  // TA command
  ul_time_alignment->apply_ta = 1;
  ul_time_alignment->ta_command = rar->TA2 + (rar->TA1 << 5);

#ifdef DEBUG_RAR
  // CSI
  csi_req = (unsigned char) (rar->UL_GRANT_4 & 0x01);
#endif

  // TPC
  tpc_command = (unsigned char) ((rar->UL_GRANT_4 >> 1) & 0x07);
  switch (tpc_command){
    case 0:
      ra->Msg3_TPC = -6;
      break;
    case 1:
      ra->Msg3_TPC = -4;
      break;
    case 2:
      ra->Msg3_TPC = -2;
      break;
    case 3:
      ra->Msg3_TPC = 0;
      break;
    case 4:
      ra->Msg3_TPC = 2;
      break;
    case 5:
      ra->Msg3_TPC = 4;
      break;
    case 6:
      ra->Msg3_TPC = 6;
      break;
    case 7:
      ra->Msg3_TPC = 8;
      break;
  }
    // MCS
    rar_grant.mcs = (unsigned char) (rar->UL_GRANT_4 >> 4);
    // time alloc
    rar_grant.Msg3_t_alloc = (unsigned char) (rar->UL_GRANT_3 & 0x07);
    // frequency alloc
    rar_grant.Msg3_f_alloc = (uint16_t) ((rar->UL_GRANT_3 >> 4) | (rar->UL_GRANT_2 << 4) | ((rar->UL_GRANT_1 & 0x03) << 12));
    // frequency hopping
    rar_grant.freq_hopping = (unsigned char) (rar->UL_GRANT_1 >> 2);
    // TC-RNTI
    if (ra->t_crnti) {
      rnti = ra->t_crnti;
    } else {
      rnti = mac->crnti;
    }

  #ifdef DEBUG_RAR
  LOG_D(MAC, "In %s:[%d.%d]: [UE %d] Received RAR with t_alloc %d f_alloc %d ta_command %d mcs %d freq_hopping %d tpc_command %d csi_req %d t_crnti %x \n",
    __FUNCTION__,
    frame,
    slot,
    mod_id,
    rar_grant.Msg3_t_alloc,
    rar_grant.Msg3_f_alloc,
    ul_time_alignment->ta_command,
    rar_grant.mcs,
    rar_grant.freq_hopping,
    tpc_command,
    csi_req,
    ra->t_crnti);
  #endif

    // Schedule Msg3
    ret = nr_ue_pusch_scheduler(mac, is_Msg3, frame, slot, &frame_tx, &slot_tx, rar_grant.Msg3_t_alloc);

    if (ret != -1){

      fapi_nr_ul_config_request_t *ul_config = get_ul_config_request(mac, slot_tx);

      if (!ul_config) {
        LOG_W(MAC, "In %s: ul_config request is NULL. Probably due to unexpected UL DCI in frame.slot %d.%d. Ignoring DCI!\n", __FUNCTION__, frame, slot);
        return -1;
      }

      nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu = &ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu;

      fill_ul_config(ul_config, frame_tx, slot_tx, FAPI_NR_UL_CONFIG_TYPE_PUSCH);

      // Config Msg3 PDU
      nr_config_pusch_pdu(mac, pusch_config_pdu, NULL, &rar_grant, rnti, NULL);

    }

  } else {

    ra->t_crnti = 0;
    ul_time_alignment->ta_command = (0xffff);

  }

  return ret;

}
