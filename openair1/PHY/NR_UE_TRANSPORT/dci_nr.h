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

/*! \file PHY/LTE_TRANSPORT/dci_nr.h
* \brief typedefs for NR DCI structures from 38-212.
* \author R. Knopp, A. Mico Pereperez
* \date 2018
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#ifndef USER_MODE
#include "PHY/types.h"
#else
#include <stdint.h>
#endif



#define MAX_DCI_SIZE_BITS 45


#define NR_PDCCH_DCI_H
#ifdef NR_PDCCH_DCI_H
struct NR_DCI_INFO_EXTRACTED {
  uint8_t carrier_ind                     ; // 0  CARRIER_IND: 0 or 3 bits, as defined in Subclause x.x of [5, TS38.213]
  uint8_t sul_ind_0_1                     ; // 1  SUL_IND_0_1:
  uint8_t identifier_dci_formats          ; // 2  IDENTIFIER_DCI_FORMATS:
  uint8_t slot_format_ind                 ; // 3  SLOT_FORMAT_IND: size of DCI format 2_0 is configurable by higher layers up to 128 bits, according to Subclause 11.1.1 of [5, TS 38.213]
  uint8_t pre_emption_ind                 ; // 4  PRE_EMPTION_IND: size of DCI format 2_1 is configurable by higher layers up to 126 bits, according to Subclause 11.2 of [5, TS 38.213]. Each pre-emption indication is 14 bits
  uint8_t tpc_cmd_number                  ; // 5  TPC_CMD_NUMBER: The parameter xxx provided by higher layers determines the index to the TPC command number for an UL of a cell. Each TPC command number is 2 bits
  uint8_t block_number                    ; // 6  BLOCK_NUMBER: starting position of a block is determined by the parameter startingBitOfFormat2_3
  uint8_t bandwidth_part_ind              ; // 7  BANDWIDTH_PART_IND:
  uint16_t freq_dom_resource_assignment_UL; // 8  FREQ_DOM_RESOURCE_ASSIGNMENT_UL: PUSCH hopping with resource allocation type 1 not considered
                                            // (NOTE 1) If DCI format 0_0 is monitored in common search space
                                            // and if the number of information bits in the DCI format 0_0 prior to padding
                                            // is larger than the payload size of the DCI format 1_0 monitored in common search space
                                            // the bitwidth of the frequency domain resource allocation field in the DCI format 0_0
                                            // is reduced such that the size of DCI format 0_0 equals to the size of the DCI format 1_0
  uint16_t freq_dom_resource_assignment_DL; // 9  FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
  uint8_t time_dom_resource_assignment    ; // 10 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 6.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
                                            // where I the number of entries in the higher layer parameter pusch-AllocationList
  uint8_t vrb_to_prb_mapping              ; // 11 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
  uint8_t prb_bundling_size_ind           ; // 12 PRB_BUNDLING_SIZE_IND:0 bit if the higher layer parameter PRB_bundling is not configured or is set to 'static', or 1 bit if the higher layer parameter PRB_bundling is set to 'dynamic' according to Subclause 5.1.2.3 of [6, TS 38.214]
  uint8_t rate_matching_ind               ; // 13 RATE_MATCHING_IND: 0, 1, or 2 bits according to higher layer parameter rate-match-PDSCH-resource-set
  uint8_t zp_csi_rs_trigger               ; // 14 ZP_CSI_RS_TRIGGER:
  uint8_t freq_hopping_flag               ; // 15 FREQ_HOPPING_FLAG: 0 bit if only resource allocation type 0
  uint8_t tb1_mcs                         ; // 16 TB1_MCS:
  uint8_t tb1_ndi                         ; // 17 TB1_NDI:
  uint8_t tb1_rv                          ; // 18 TB1_RV:
  uint8_t tb2_mcs                         ; // 19 TB2_MCS:
  uint8_t tb2_ndi                         ; // 20 TB2_NDI:
  uint8_t tb2_rv                          ; // 21 TB2_RV:
  uint8_t mcs                             ; // 22 MCS:
  uint8_t ndi                             ; // 23 NDI:
  uint8_t rv                              ; // 24 RV:
  uint8_t harq_process_number             ; // 25 HARQ_PROCESS_NUMBER:
  uint8_t dai                             ; // 26 DAI: For format1_1: 4 if more than one serving cell are configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 MSB bits are the counter DAI and the 2 LSB bits are the total DAI
                                            // 2 if one serving cell is configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 bits are the counter DAI
                                            // 0 otherwise
  uint8_t first_dai                       ; // 27 FIRST_DAI: (1 or 2 bits) 1 bit for semi-static HARQ-ACK
  uint8_t second_dai                      ; // 28 SECOND_DAI: (0 or 2 bits) 2 bits for dynamic HARQ-ACK codebook with two HARQ-ACK sub-codebooks
  uint8_t tpc_pusch                       ; // 29 TPC_PUSCH:
  uint8_t tpc_pucch                       ; // 30 TPC_PUCCH:
  uint8_t pucch_resource_ind              ; // 31 PUCCH_RESOURCE_IND:
  uint8_t pdsch_to_harq_feedback_time_ind ; // 32 PDSCH_TO_HARQ_FEEDBACK_TIME_IND:
  uint8_t short_message_ind               ; // 33 SHORT_MESSAGE_IND: 1 bit if crc scrambled with P-RNTI
  uint8_t srs_resource_ind                ; // 34 SRS_RESOURCE_IND:
  uint8_t precod_nbr_layers               ; // 35 PRECOD_NBR_LAYERS:
  uint8_t antenna_ports                   ; // 36 ANTENNA_PORTS:
  uint8_t tci                             ; // 37 TCI: 0 bit if higher layer parameter tci-PresentInDCI is not enabled; otherwise 3 bits
  uint8_t srs_request                     ; // 38 SRS_REQUEST:
  uint8_t tpc_cmd_number_format2_3        ; // 39 TPC_CMD_NUMBER_FORMAT2_3:
  uint8_t csi_request                     ; // 40 CSI_REQUEST:
  uint8_t cbgti                           ; // 41 CBGTI: 0, 2, 4, 6, or 8 bits determined by higher layer parameter maxCodeBlockGroupsPerTransportBlock for the PDSCH
  uint8_t cbgfi                           ; // 42 CBGFI: 0 or 1 bit determined by higher layer parameter codeBlockGroupFlushIndicator
  uint8_t ptrs_dmrs                       ; // 43 PTRS_DMRS:
  uint8_t beta_offset_ind                 ; // 44 BETA_OFFSET_IND:
  uint8_t dmrs_seq_ini                    ; // 45 DMRS_SEQ_INI: 1 bit if the cell has two ULs and the number of bits for DCI format 1_0 before padding
                                            // is larger than the number of bits for DCI format 0_0 before padding; 0 bit otherwise
  uint8_t sul_ind_0_0                     ; // 46 SUL_IND_0_0:
  uint16_t padding                        ; // 47 PADDING: (Note 2) If DCI format 0_0 is monitored in common search space
                                            // and if the number of information bits in the DCI format 0_0 prior to padding
                                            // is less than the payload size of the DCI format 1_0 monitored in common search space
                                            // zeros shall be appended to the DCI format 0_0
                                            // until the payload size equals that of the DCI format 1_0

};
typedef struct NR_DCI_INFO_EXTRACTED NR_DCI_INFO_EXTRACTED_t;
#endif

