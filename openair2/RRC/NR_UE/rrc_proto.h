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

/* \file proto.h
 * \brief RRC functions prototypes for eNB and UE
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef _RRC_PROTO_H_
#define _RRC_PROTO_H_


#include "rrc_defs.h"
#include "NR_RRCReconfiguration.h"
#include "NR_MeasConfig.h"
#include "NR_CellGroupConfig.h"
#include "NR_RadioBearerConfig.h"
#include "openair2/PHY_INTERFACE/queue.h"

extern queue_t nr_rach_ind_queue;
extern queue_t nr_rx_ind_queue;
extern queue_t nr_crc_ind_queue;
extern queue_t nr_uci_ind_queue;
extern queue_t nr_sfn_slot_queue;
extern queue_t nr_dl_tti_req_queue;
extern queue_t nr_tx_req_queue;
extern queue_t nr_ul_dci_req_queue;
extern queue_t nr_ul_tti_req_queue;
extern queue_t nr_wait_ul_tti_req_queue;
//
//  main_rrc.c
//
/**\brief Layer 3 initialization*/
NR_UE_RRC_INST_t* nr_l3_init_ue(char*);

//
//  UE_rrc.c
//

/**\brief Initial the top level RRC structure instance*/
NR_UE_RRC_INST_t* openair_rrc_top_init_ue_nr(char*);



/**\brief Decode RRC Connection Reconfiguration, sent from E-UTRA RRC Connection Reconfiguration v1510 carring EN-DC config
   \param buffer  encoded NR-RRC-Connection-Reconfiguration/Secondary-Cell-Group-Config message.
   \param size    length of buffer*/
//TODO check to use which one
//int8_t nr_rrc_ue_decode_rrcReconfiguration(const uint8_t *buffer, const uint32_t size);
int8_t nr_rrc_ue_decode_secondary_cellgroup_config(const module_id_t module_id, const uint8_t *buffer, const uint32_t size);
   

/**\brief Process NR RRC connection reconfiguration via SRB3
   \param rrcReconfiguration  decoded rrc connection reconfiguration*/
int8_t nr_rrc_ue_process_rrcReconfiguration(const module_id_t module_id, NR_RRCReconfiguration_t *rrcReconfiguration);

/**\prief Process measurement config from NR RRC connection reconfiguration message
   \param meas_config   measurement configuration*/
int8_t nr_rrc_ue_process_meas_config(NR_MeasConfig_t *meas_config);

/**\prief Process secondary cell group config from NR RRC connection reconfiguration message or EN-DC primitives
   \param cell_group_config   secondary cell group configuration*/
//TODO check EN-DC function call flow.
int8_t nr_rrc_ue_process_scg_config(const module_id_t module_id, NR_CellGroupConfig_t *cell_group_config);

/**\prief Process radio bearer config from NR RRC connection reconfiguration message
   \param radio_bearer_config    radio bearer configuration*/
int8_t nr_rrc_ue_process_radio_bearer_config(NR_RadioBearerConfig_t *radio_bearer_config);

/**\brief decode NR BCCH-BCH (MIB) message
   \param module_idP    module id
   \param gNB_index     gNB index
   \param sduP          pointer to buffer of ASN message BCCH-BCH
   \param sdu_len       length of buffer*/
int8_t nr_rrc_ue_decode_NR_BCCH_BCH_Message(const module_id_t module_id, const uint8_t gNB_index, uint8_t *const bufferP, const uint8_t buffer_len);

/**\brief decode NR BCCH-DLSCH (SI) messages
   \param module_idP    module id
   \param gNB_index     gNB index
   \param sduP          pointer to buffer of ASN message BCCH-DLSCH
   \param sdu_len       length of buffer
   \param rsrq          RSRQ
   \param rsrp          RSRP*/
int8_t nr_rrc_ue_decode_NR_BCCH_DL_SCH_Message(const module_id_t module_id, const uint8_t gNB_index, uint8_t *const bufferP, const uint8_t buffer_len, const uint8_t rsrq, const uint8_t rsrp);

/**\brief Decode NR DCCH from gNB, sent from lower layer through SRB3
   \param module_id  module id
   \param gNB_index  gNB index
   \param buffer     encoded DCCH bytes stream message
   \param size       length of buffer*/
int8_t nr_rrc_ue_decode_NR_DL_DCCH_Message(const module_id_t module_id, const uint8_t gNB_index, const uint8_t *buffer, const uint32_t size);

/**\brief interface between MAC and RRC thru SRB0 (RLC TM/no PDCP)
   \param module_id  module id
   \param CC_id      component carrier id
   \param gNB_index  gNB index
   \param channel    indicator for channel of the pdu
   \param pduP       pointer to pdu
   \param pdu_len    data length of pdu*/
int8_t nr_mac_rrc_data_ind_ue(const module_id_t module_id,
                              const int CC_id,
                              const uint8_t gNB_index,
                              const frame_t frame,
                              const sub_frame_t sub_frame,
                              const rnti_t rnti,
                              const channel_t channel,
                              const uint8_t* pduP,
                              const sdu_size_t pdu_len);

/**\brief
   \param module_id  module id
   \param CC_id      component carrier id
   \param gNB_index  gNB index
   \param frame_t    frameP
   \param rb_id_t    SRB id
   \param buffer_pP  pointer to buffer*/
int8_t nr_mac_rrc_data_req_ue(const module_id_t Mod_idP,
                              const int         CC_id,
                              const uint8_t     gNB_id,
                              const frame_t     frameP,
                              const rb_id_t     Srb_id,
                              uint8_t           *buffer_pP);

uint8_t
rrc_data_req_nr_ue(
  const protocol_ctxt_t   *const ctxt_pP,
  const rb_id_t                  rb_idP,
  const mui_t                    muiP,
  const confirm_t                confirmP,
  const sdu_size_t               sdu_sizeP,
  uint8_t                 *const buffer_pP,
  const pdcp_transmission_mode_t modeP
);

/**\brief RRC UE task.
   \param void *args_p Pointer on arguments to start the task. */
void *rrc_nrue_task(void *args_p);

/**\brief RRC NSA UE task.
   \param void *args_p Pointer on arguments to start the task. */
void *recv_msgs_from_lte_ue(void *args_p);

void init_connections_with_lte_ue(void);

void nsa_sendmsg_to_lte_ue(const void *message, size_t msg_len, MessagesIds msg_type);

/**\brief RRC UE generate RRCSetupRequest message.
   \param module_id  module id
   \param gNB_index  gNB index  */
void nr_rrc_ue_generate_RRCSetupRequest(module_id_t module_id, const uint8_t gNB_index);

void process_lte_nsa_msg(nsa_msg_t *msg, int msg_len);

int get_from_lte_ue_fd();

/** @}*/
#endif

