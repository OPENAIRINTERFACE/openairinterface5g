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

/* \file main_ue_nr.c
 * \brief top init of Layer 2
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

//#include "defs.h"
#include "mac_proto.h"
#include "radio/COMMON/common_lib.h"
//#undef MALLOC
#include "assertions.h"
#include "executables/softmodem-common.h"
#include "nr_rlc/nr_rlc_oai_api.h"
#include "RRC/NR_UE/rrc_proto.h"
#include <pthread.h>

static NR_UE_MAC_INST_t *nr_ue_mac_inst; 

void send_srb0_rrc(int rnti, const uint8_t *sdu, sdu_size_t sdu_len, void *data)
{
  AssertFatal(sdu_len > 0 && sdu_len < CCCH_SDU_SIZE, "invalid CCCH SDU size %d\n", sdu_len);

  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_CCCH_DATA_IND);
  memset(NR_RRC_MAC_CCCH_DATA_IND(message_p).sdu, 0, sdu_len);
  memcpy(NR_RRC_MAC_CCCH_DATA_IND(message_p).sdu, sdu, sdu_len);
  NR_RRC_MAC_CCCH_DATA_IND(message_p).sdu_size = sdu_len;
  NR_RRC_MAC_CCCH_DATA_IND(message_p).rnti = rnti;
  itti_send_msg_to_task(TASK_RRC_NRUE, 0, message_p);
}

void send_msg3_rrc_request(module_id_t mod_id, int rnti)
{
  nr_rlc_activate_srb0(rnti, NULL, send_srb0_rrc);
  nr_mac_rrc_msg3_ind(mod_id, rnti);
}

NR_UE_MAC_INST_t * nr_l2_init_ue()
{
    LOG_I(NR_MAC, "MAIN: init UE MAC functions \n");
    //init mac here
    nr_ue_mac_inst = (NR_UE_MAC_INST_t *)calloc(NB_NR_UE_MAC_INST, sizeof(NR_UE_MAC_INST_t));

    for (int j = 0; j < NB_NR_UE_MAC_INST; j++)
      nr_ue_init_mac(j);

    if (get_softmodem_params()->sa)
      ue_init_config_request(nr_ue_mac_inst, get_softmodem_params()->numerology);

    int rc = rlc_module_init(0);
    AssertFatal(rc == 0, "%s: Could not initialize RLC layer\n", __FUNCTION__);

    return (nr_ue_mac_inst);
}

NR_UE_MAC_INST_t *get_mac_inst(module_id_t module_id)
{
    return &nr_ue_mac_inst[(int)module_id];
}

void reset_mac_inst(NR_UE_MAC_INST_t *nr_mac)
{
  // MAC reset according to 38.321 Section 5.12

  nr_ue_mac_default_configs(nr_mac);
  // initialize Bj for each logical channel to zero
  // Done in default config but to -1 (is that correct?)

  // stop all running timers
  // TODO

  // consider all timeAlignmentTimers as expired and perform the corresponding actions in clause 5.2
  // TODO

  // set the NDIs for all uplink HARQ processes to the value 0
  for (int k = 0; k < NR_MAX_HARQ_PROCESSES; k++)
    nr_mac->UL_ndi[k] = -1; // initialize to invalid value

  // stop any ongoing RACH procedure
  if (nr_mac->ra.ra_state < RA_SUCCEEDED)
    nr_mac->ra.ra_state = RA_UE_IDLE;

  // discard explicitly signalled contention-free Random Access Resources
  // TODO not sure what needs to be done here

  // flush Msg3 buffer
  // TODO we don't have a Msg3 buffer

  // cancel any triggered Scheduling Request procedure
  // Done in default config

  // cancel any triggered Buffer Status Reporting procedure
  // Done in default config

  // cancel any triggered Power Headroom Reporting procedure
  // TODO PHR not implemented yet

  // flush the soft buffers for all DL HARQ processes
  for (int k = 0; k < NR_MAX_HARQ_PROCESSES; k++)
    memset(&nr_mac->dl_harq_info[k], 0, sizeof(NR_UE_HARQ_STATUS_t));

  // for each DL HARQ process, consider the next received transmission for a TB as the very first transmission
  // TODO there is nothing in the MAC indicating first transmission

  // release, if any, Temporary C-RNTI
  nr_mac->ra.t_crnti = 0;

  // reset BFI_COUNTER
  // TODO beam failure procedure not implemented
}
