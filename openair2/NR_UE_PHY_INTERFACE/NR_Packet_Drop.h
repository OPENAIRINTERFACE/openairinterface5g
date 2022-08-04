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

#include <platform_types.h>
#include <nfapi_nr_interface_scf.h>
#include <openair1/PHY/thread_NR_UE.h>
#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"

#define NR_NUM_MCS 29
#define NUM_SINR 100
#define NUM_BLER_COL 13
#define NUM_NFAPI_SLOT 20
#define NR_NUM_LAYER 1
#define MAX_CHAN_PARAMS 256

typedef struct nr_phy_channel_params_t
{
    uint16_t sfn_slot;
    uint16_t message_id;
    uint16_t nb_of_sinrs;
    float sinr[NR_NUM_LAYER];
} nr_phy_channel_params_t;

typedef struct
{
    uint8_t slot;
    uint16_t rnti[MAX_CHAN_PARAMS];
    uint8_t mcs[MAX_CHAN_PARAMS];
    uint8_t rvIndex[MAX_CHAN_PARAMS];
    float sinr;
    uint16_t num_pdus;
    bool drop_flag[MAX_CHAN_PARAMS];
    bool latest;
} slot_rnti_mcs_s;

typedef struct
{
    uint16_t length;
    float bler_table[NUM_SINR][NUM_BLER_COL];
} nr_bler_struct;

void read_channel_param(const nfapi_nr_dl_tti_pdsch_pdu_rel15_t * pdu, int sf, int index);
void save_pdsch_pdu_for_crnti(nfapi_nr_dl_tti_request_t *dl_tti_request);
float get_bler_val(uint8_t mcs, int sinr);
bool should_drop_transport_block(int slot, uint16_t rnti);
bool is_channel_modeling(void);

#endif
