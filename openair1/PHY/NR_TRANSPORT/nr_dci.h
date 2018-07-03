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
#ifndef __PHY_NR_TRANSPORT_DCI__H
#define __PHY_NR_TRANSPORT_DCI__H

#include "defs_gNB.h"


typedef enum {
  nr_dci_format_0_0=0,
  nr_dci_format_0_1,
  nr_dci_format_2_0,
  nr_dci_format_2_1,
  nr_dci_format_2_2,
  nr_dci_format_2_3,
  nr_dci_format_1_0,
  nr_dci_format_1_1,
} nr_dci_format_e;

typedef enum {
  nr_rnti_type_SI_RNTI=0,
  nr_rnti_type_RA_RNTI,
  nr_rnti_type_C_RNTI,
  nr_rnti_type_TC_RNTI,
  nr_rnti_type_CS_RNTI,
  nr_rnti_type_P_RNTI
} nr_rnti_type_e;


typedef struct {
  /// Length of DCI in bits
  uint8_t size;
  /// Aggregation level
  uint8_t L;
  /// Position of first CCE of the dci
  int firstCCE;
  /// flag to indicate that this is a RA response
  boolean_t ra_flag;
  /// rnti
  nr_rnti_type_e rnti;
  /// Format
  DCI_format_t format;
  /// DCI pdu
  uint8_t dci_pdu[8];
} NR_DCI_ALLOC_t;


uint8_t nr_get_dci_size(nr_dci_format_e format,
                        nr_rnti_type_e rnti,
                        NR_BWP_PARMS bwp,
                        nfapi_nr_config_request_t* config);

uint8_t nr_generate_dci_top(NR_DCI_ALLOC_t dci_alloc,
                            int32_t** txdataF,
                            int16_t amp,
                            NR_DL_FRAME_PARMS* frame_parms,
                            nfapi_config_request_t* config)

#endif //__PHY_NR_TRANSPORT_DCI__H
