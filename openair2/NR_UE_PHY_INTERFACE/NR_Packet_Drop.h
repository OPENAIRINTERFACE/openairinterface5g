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
#ifndef __NR_CHAN_MODEL_H__
#define __NR_CHAN_MODEL_H__

#include "common/platform_types.h"
#include <nfapi_nr_interface_scf.h>
#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"

#define NR_NUM_MCS 29
#define NR_NUM_SINR 372
#define NUM_BLER_COL 13
#define NUM_NFAPI_SLOT 20
#define NR_NUM_LAYER 1
#define MAX_NR_CHAN_PARAMS 256

typedef struct NR_UL_TIME_ALIGNMENT NR_UL_TIME_ALIGNMENT_t;

typedef struct {
  float sinr;
  float rsrp;
  float rsrq;
  uint8_t source;
  uint8_t pmi;
  uint8_t ri;
  uint8_t cqi;
  uint8_t area_code;
} nr_channel_status;

typedef struct nr_phy_channel_params_t {
  uint16_t sfn_slot;
  uint16_t message_id;
  uint16_t nb_of_csi;
  nr_channel_status csi[NR_NUM_LAYER];
} nr_phy_channel_params_t;

typedef struct {
  uint8_t slot;
  uint16_t rnti[MAX_NR_CHAN_PARAMS];
  uint8_t mcs[MAX_NR_CHAN_PARAMS];
  uint8_t rvIndex[MAX_NR_CHAN_PARAMS];
  float sinr;
  uint16_t num_pdus;
  bool drop_flag[MAX_NR_CHAN_PARAMS];
  bool latest;
  uint8_t area_code;
} slot_rnti_mcs_s;

typedef struct {
  uint16_t length;
  float bler_table[NR_NUM_SINR][NUM_BLER_COL];
} nr_bler_struct;

extern nr_bler_struct nr_bler_data[NR_NUM_MCS];
extern nr_bler_struct nr_mimo_bler_data[NR_NUM_MCS];

void read_channel_param(const nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdu, int sf, int index);
float get_bler_val(uint8_t mcs, int sinr);
bool should_drop_transport_block(int slot, uint16_t rnti);
bool is_channel_modeling(void);
int get_mcs_from_sinr(nr_bler_struct *nr_bler_data, float sinr);

#endif
