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

#ifndef NR_PDCP_OAI_API_H
#define NR_PDCP_OAI_API_H

#include "pdcp.h"
#include "nr_pdcp_ue_manager.h"

void nr_pdcp_layer_init(bool uses_e1);
uint64_t nr_pdcp_module_init(uint64_t _pdcp_optmask, int id);

void du_rlc_data_req(const protocol_ctxt_t *const ctxt_pP,
                     const srb_flag_t srb_flagP,
                     const MBMS_flag_t MBMS_flagP,
                     const rb_id_t rb_idP,
                     const mui_t muiP,
                     confirm_t confirmP,
                     sdu_size_t sdu_sizeP,
                     uint8_t *sdu_pP);

bool pdcp_data_ind(const protocol_ctxt_t *const ctxt_pP,
                   const srb_flag_t srb_flagP,
                   const MBMS_flag_t MBMS_flagP,
                   const rb_id_t rb_id,
                   const sdu_size_t sdu_buffer_size,
                   uint8_t *const sdu_buffer,
                   const uint32_t *const srcID,
                   const uint32_t *const dstID);

void nr_pdcp_add_drbs(eNB_flag_t enb_flag,
                      ue_id_t UEid,
                      NR_DRB_ToAddModList_t *const drb2add_list,
                      const uint8_t security_modeP,
                      uint8_t *const kUPenc,
                      uint8_t *const kUPint);

void add_drb(int is_gnb,
             ue_id_t UEid,
             struct NR_DRB_ToAddMod *s,
             int ciphering_algorithm,
             int integrity_algorithm,
             unsigned char *ciphering_key,
             unsigned char *integrity_key);

void nr_pdcp_remove_UE(ue_id_t ue_id);
void nr_pdcp_reestablishment(ue_id_t ue_id, int rb_id, bool srb_flag);

void nr_pdcp_suspend_srb(ue_id_t ue_id, int srb_id);
void nr_pdcp_suspend_drb(ue_id_t ue_id, int drb_id);
void nr_pdcp_reconfigure_srb(ue_id_t ue_id, int srb_id, long t_Reordering);
void nr_pdcp_reconfigure_drb(ue_id_t ue_id, int drb_id, NR_PDCP_Config_t *pdcp_config, NR_SDAP_Config_t *sdap_config);
void nr_pdcp_release_srb(ue_id_t ue_id, int srb_id);
void nr_pdcp_release_drb(ue_id_t ue_id, int drb_id);


void add_srb(int is_gnb,
             ue_id_t UEid,
             struct NR_SRB_ToAddMod *s,
             int ciphering_algorithm,
             int integrity_algorithm,
             unsigned char *ciphering_key,
             unsigned char *integrity_key);

void nr_pdcp_config_set_security(ue_id_t ue_id,
                                 const rb_id_t rb_id,
                                 const uint8_t security_modeP,
                                 uint8_t *const kRRCenc_pP,
                                 uint8_t *const kRRCint_pP,
                                 uint8_t *const kUPenc_pP);

bool cu_f1u_data_req(protocol_ctxt_t  *ctxt_pP,
                     const srb_flag_t srb_flagP,
                     const rb_id_t rb_id,
                     const mui_t muiP,
                     const confirm_t confirmP,
                     const sdu_size_t sdu_buffer_size,
                     unsigned char *const sdu_buffer,
                     const pdcp_transmission_mode_t mode,
                     const uint32_t *const sourceL2Id,
                     const uint32_t *const destinationL2Id);

typedef void (*deliver_pdu)(void *data, ue_id_t ue_id, int srb_id,
                            char *buf, int size, int sdu_id);
/* default implementation of deliver_pdu */
void deliver_pdu_srb_rlc(void *data, ue_id_t ue_id, int srb_id, char *buf, int size, int sdu_id);
bool nr_pdcp_data_req_srb(ue_id_t ue_id,
                          const rb_id_t rb_id,
                          const mui_t muiP,
                          const sdu_size_t sdu_buffer_size,
                          unsigned char *const sdu_buffer,
                          deliver_pdu deliver_cb,
                          void *data);
bool nr_pdcp_data_req_drb(protocol_ctxt_t *ctxt_pP,
                          const srb_flag_t srb_flagP,
                          const rb_id_t rb_id,
                          const mui_t muiP,
                          const confirm_t confirmP,
                          const sdu_size_t sdu_buffer_size,
                          unsigned char *const sdu_buffer,
                          const pdcp_transmission_mode_t mode,
                          const uint32_t *const sourceL2Id,
                          const uint32_t *const destinationL2Id);

void nr_pdcp_tick(int frame, int subframe);

nr_pdcp_ue_manager_t *nr_pdcp_sdap_get_ue_manager();

int nr_pdcp_get_num_ues(ue_id_t *ue_list, int len);

bool nr_pdcp_get_statistics(ue_id_t ue_id, int srb_flag, int rb_id, nr_pdcp_statistics_t *out);

#endif /* NR_PDCP_OAI_API_H */
