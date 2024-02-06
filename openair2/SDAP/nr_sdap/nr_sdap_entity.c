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

#include "nr_sdap_entity.h"
#include "common/utils/LOG/log.h"
#include <openair2/LAYER2/nr_pdcp/nr_pdcp_oai_api.h>
#include <openair3/ocp-gtpu/gtp_itf.h>
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_ue_manager.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct {
  nr_sdap_entity_t *sdap_entity_llist;
} nr_sdap_entity_info;

static nr_sdap_entity_info sdap_info;

instance_t *N3GTPUInst = NULL;

void nr_pdcp_submit_sdap_ctrl_pdu(ue_id_t ue_id, rb_id_t sdap_ctrl_pdu_drb, nr_sdap_ul_hdr_t ctrl_pdu)
{

  protocol_ctxt_t ctxt = { .rntiMaybeUEid = ue_id };
  nr_pdcp_data_req_drb(&ctxt,
                       SRB_FLAG_NO,
                       sdap_ctrl_pdu_drb,
                       RLC_MUI_UNDEFINED,
                       SDU_CONFIRM_NO,
                       SDAP_HDR_LENGTH,
                       (unsigned char *)&ctrl_pdu,
                       PDCP_TRANSMISSION_MODE_UNKNOWN,
                       NULL,
                       NULL);
  LOG_D(SDAP, "Control PDU - Submitting Control PDU to DRB ID:  %ld\n", sdap_ctrl_pdu_drb);
  LOG_D(SDAP, "QFI: %u\n R: %u\n D/C: %u\n", ctrl_pdu.QFI, ctrl_pdu.R, ctrl_pdu.DC);
  return;
}

static bool nr_sdap_tx_entity(nr_sdap_entity_t *entity,
                              protocol_ctxt_t *ctxt_p,
                              const srb_flag_t srb_flag,
                              const rb_id_t rb_id,
                              const mui_t mui,
                              const confirm_t confirm,
                              const sdu_size_t sdu_buffer_size,
                              unsigned char *const sdu_buffer,
                              const pdcp_transmission_mode_t pt_mode,
                              const uint32_t *sourceL2Id,
                              const uint32_t *destinationL2Id,
                              const uint8_t qfi,
                              const bool rqi) {
  /* The offset of the SDAP header, it might be 0 if has_sdap_tx is not true in the pdcp entity. */
  int offset=0;
  bool ret = false;
  /*Hardcode DRB ID given from upper layer (ue/enb_tun_read_thread rb_id), it will change if we have SDAP*/
  rb_id_t sdap_drb_id = rb_id;
  int pdcp_ent_has_sdap = 0;

  if(sdu_buffer == NULL) {
    LOG_E(SDAP, "%s:%d:%s: NULL sdu_buffer \n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  uint8_t sdap_buf[SDAP_MAX_PDU];
  rb_id_t pdcp_entity = entity->qfi2drb_map(entity, qfi, rb_id);

  if(pdcp_entity){
    sdap_drb_id = pdcp_entity;
    pdcp_ent_has_sdap = entity->qfi2drb_table[qfi].has_sdap_tx;
    LOG_D(SDAP, "TX - QFI: %u is mapped to DRB ID: %ld\n", qfi, entity->qfi2drb_table[qfi].drb_id);
  }

  if(!pdcp_ent_has_sdap){
    LOG_D(SDAP, "TX - DRB ID: %ld does not have SDAP\n", entity->qfi2drb_table[qfi].drb_id);
    ret = nr_pdcp_data_req_drb(ctxt_p,
                               srb_flag,
                               sdap_drb_id,
                               mui,
                               confirm,
                               sdu_buffer_size,
                               sdu_buffer,
                               pt_mode,
                               sourceL2Id,
                               destinationL2Id);

    if(!ret)
      LOG_E(SDAP, "%s:%d:%s: PDCP refused PDU\n", __FILE__, __LINE__, __FUNCTION__);

    return ret;
  }

  if(sdu_buffer_size == 0 || sdu_buffer_size > 8999) {
    LOG_E(SDAP, "%s:%d:%s: NULL or 0 or exceeded sdu_buffer_size (over max PDCP SDU)\n", __FILE__, __LINE__, __FUNCTION__);
    return 0;
  }

  if(ctxt_p->enb_flag) { // gNB
    offset = SDAP_HDR_LENGTH;
    /*
     * TS 37.324 4.4 Functions
     * marking QoS flow ID in DL packets.
     *
     * Construct the DL SDAP data PDU.
     */
    nr_sdap_dl_hdr_t sdap_hdr;
    sdap_hdr.QFI = qfi;
    sdap_hdr.RQI = rqi;
    sdap_hdr.RDI = 0; // SDAP Hardcoded Value
    /* Add the SDAP DL Header to the buffer */
    memcpy(&sdap_buf[0], &sdap_hdr, SDAP_HDR_LENGTH);
    memcpy(&sdap_buf[SDAP_HDR_LENGTH], sdu_buffer, sdu_buffer_size);
    LOG_D(SDAP, "TX Entity QFI: %u \n", sdap_hdr.QFI);
    LOG_D(SDAP, "TX Entity RQI: %u \n", sdap_hdr.RQI);
    LOG_D(SDAP, "TX Entity RDI: %u \n", sdap_hdr.RDI);
  } else { // nrUE
    offset = SDAP_HDR_LENGTH;
    /*
     * TS 37.324 4.4 Functions
     * marking QoS flow ID in UL packets.
     *
     * 5.2.1 Uplink
     * construct the UL SDAP data PDU as specified in the subclause 6.2.2.3.
     */
    nr_sdap_ul_hdr_t sdap_hdr;
    sdap_hdr.QFI = qfi;
    sdap_hdr.R = 0;
    sdap_hdr.DC = rqi;
    /* Add the SDAP UL Header to the buffer */
    memcpy(&sdap_buf[0], &sdap_hdr, SDAP_HDR_LENGTH);
    memcpy(&sdap_buf[SDAP_HDR_LENGTH], sdu_buffer, sdu_buffer_size);
    LOG_D(SDAP, "TX Entity QFI: %u \n", sdap_hdr.QFI);
    LOG_D(SDAP, "TX Entity R:   %u \n", sdap_hdr.R);
    LOG_D(SDAP, "TX Entity DC:  %u \n", sdap_hdr.DC);
  }

  /*
   * TS 37.324 5.2 Data transfer
   * 5.2.1 Uplink UE side
   * submit the constructed UL SDAP data PDU to the lower layers
   *
   * Downlink gNB side
   */
  ret = nr_pdcp_data_req_drb(ctxt_p,
                             srb_flag,
                             sdap_drb_id,
                             mui,
                             confirm,
                             sdu_buffer_size + offset,
                             sdap_buf,
                             pt_mode,
                             sourceL2Id,
                             destinationL2Id);

  if(!ret)
    LOG_E(SDAP, "%s:%d:%s: PDCP refused PDU\n", __FILE__, __LINE__, __FUNCTION__);

  return ret;
}

static void nr_sdap_rx_entity(nr_sdap_entity_t *entity,
                              rb_id_t pdcp_entity,
                              int is_gnb,
                              bool has_sdap_rx,
                              int pdusession_id,
                              ue_id_t ue_id,
                              char *buf,
                              int size) {
  /* The offset of the SDAP header, it might be 0 if has_sdap_rx is not true in the pdcp entity. */
  int offset=0;

  if (is_gnb) { // gNB
    if (has_sdap_rx) { // Handling the SDAP Header
      offset = SDAP_HDR_LENGTH;
      nr_sdap_ul_hdr_t *sdap_hdr = (nr_sdap_ul_hdr_t *)buf;
      LOG_D(SDAP, "RX Entity Received QFI:    %u\n", sdap_hdr->QFI);
      LOG_D(SDAP, "RX Entity Received R bit:  %u\n", sdap_hdr->R);
      LOG_D(SDAP, "RX Entity Received DC bit: %u\n", sdap_hdr->DC);

      switch (sdap_hdr->DC) {
        case SDAP_HDR_UL_DATA_PDU:
          LOG_D(SDAP, "RX Entity Received SDAP Data PDU\n");
          break;

        case SDAP_HDR_UL_CTRL_PDU:
          LOG_D(SDAP, "RX Entity Received SDAP Control PDU\n");
          break;
      }
    }

    // Pushing SDAP SDU to GTP-U Layer
    MessageDef *message_p = itti_alloc_new_message_sized(TASK_PDCP_ENB,
                                                         0,
                                                         GTPV1U_TUNNEL_DATA_REQ,
                                                         sizeof(gtpv1u_tunnel_data_req_t)
                                                           + size + GTPU_HEADER_OVERHEAD_MAX - offset);
    AssertFatal(message_p != NULL, "OUT OF MEMORY");
    gtpv1u_tunnel_data_req_t *req = &GTPV1U_TUNNEL_DATA_REQ(message_p);
    uint8_t *gtpu_buffer_p = (uint8_t *) (req + 1);
    memcpy(gtpu_buffer_p + GTPU_HEADER_OVERHEAD_MAX, buf + offset, size - offset);
    req->buffer        = gtpu_buffer_p;
    req->length        = size - offset;
    req->offset        = GTPU_HEADER_OVERHEAD_MAX;
    req->ue_id         = ue_id;
    req->bearer_id     = pdusession_id;
    LOG_D(SDAP, "%s()  sending message to gtp size %d\n", __func__,  size-offset);
    // very very dirty hack gloabl var N3GTPUInst
    itti_send_msg_to_task(TASK_GTPV1_U, *N3GTPUInst, message_p);
  } else { //nrUE
    /*
     * TS 37.324 5.2 Data transfer
     * 5.2.2 Downlink
     * if the DRB from which this SDAP data PDU is received is configured by RRC with the presence of SDAP header.
     */
    if (has_sdap_rx) { // Handling the SDAP Header
      offset = SDAP_HDR_LENGTH;
      /*
       * TS 37.324 5.2 Data transfer
       * 5.2.2 Downlink
       * retrieve the SDAP SDU from the DL SDAP data PDU as specified in the subclause 6.2.2.2.
       */
      nr_sdap_dl_hdr_t *sdap_hdr = (nr_sdap_dl_hdr_t *)buf;
      LOG_D(SDAP, "RX Entity Received QFI : %u\n", sdap_hdr->QFI);
      LOG_D(SDAP, "RX Entity Received RQI : %u\n", sdap_hdr->RQI);
      LOG_D(SDAP, "RX Entity Received RDI : %u\n", sdap_hdr->RDI);

      /*
       * TS 37.324 5.2 Data transfer
       * 5.2.2 Downlink
       * Perform reflective QoS flow to DRB mapping as specified in the subclause 5.3.2.
       */
      if(sdap_hdr->RDI == SDAP_REFLECTIVE_MAPPING) {
        LOG_D(SDAP, "RX - Performing Reflective Mapping\n");
        /*
         * TS 37.324 5.3 QoS flow to DRB Mapping 
         * 5.3.2 Reflective mapping
         * If there is no stored QoS flow to DRB mapping rule for the QoS flow and a default DRB is configured.
         */
        if(!entity->qfi2drb_table[sdap_hdr->QFI].drb_id && entity->default_drb){
          nr_sdap_ul_hdr_t sdap_ctrl_pdu = entity->sdap_construct_ctrl_pdu(sdap_hdr->QFI);
          rb_id_t sdap_ctrl_pdu_drb = entity->sdap_map_ctrl_pdu(entity, pdcp_entity, SDAP_CTRL_PDU_MAP_DEF_DRB, sdap_hdr->QFI);
          entity->sdap_submit_ctrl_pdu(ue_id, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
        }

        /*
         * TS 37.324 5.3 QoS flow to DRB mapping 
         * 5.3.2 Reflective mapping
         * if the stored QoS flow to DRB mapping rule for the QoS flow 
         * is different from the QoS flow to DRB mapping of the DL SDAP data PDU
         * and
         * the DRB according to the stored QoS flow to DRB mapping rule is configured by RRC
         * with the presence of UL SDAP header
         */
        if (pdcp_entity != entity->qfi2drb_table[sdap_hdr->QFI].drb_id) {
          nr_sdap_ul_hdr_t sdap_ctrl_pdu = entity->sdap_construct_ctrl_pdu(sdap_hdr->QFI);
          rb_id_t sdap_ctrl_pdu_drb = entity->sdap_map_ctrl_pdu(entity, pdcp_entity, SDAP_CTRL_PDU_MAP_RULE_DRB, sdap_hdr->QFI);
          entity->sdap_submit_ctrl_pdu(ue_id, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
        }

        /*
         * TS 37.324 5.3 QoS flow to DRB Mapping 
         * 5.3.2 Reflective mapping
         * store the QoS flow to DRB mapping of the DL SDAP data PDU as the QoS flow to DRB mapping rule for the UL. 
         */ 
        entity->qfi2drb_table[sdap_hdr->QFI].drb_id = pdcp_entity;
      }

      /*
       * TS 37.324 5.2 Data transfer
       * 5.2.2 Downlink
       * perform RQI handling as specified in the subclause 5.4
       */
      if(sdap_hdr->RQI == SDAP_RQI_HANDLING) {
        LOG_W(SDAP, "UE - TODD 5.4\n");
      }
    } /*  else - retrieve the SDAP SDU from the DL SDAP data PDU as specified in the subclause 6.2.2.1 */

    /*
     * TS 37.324 5.2 Data transfer
     * 5.2.2 Downlink
     * deliver the retrieved SDAP SDU to the upper layer.
     */
    extern int nas_sock_fd[];
    int len = write(nas_sock_fd[0], &buf[offset], size-offset);
    LOG_D(SDAP, "RX Entity len : %d\n", len);
    LOG_D(SDAP, "RX Entity size : %d\n", size);
    LOG_D(SDAP, "RX Entity offset : %d\n", offset);

    if (len != size-offset)
      LOG_E(SDAP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
  }
}

void nr_sdap_qfi2drb_map_update(nr_sdap_entity_t *entity, uint8_t qfi, rb_id_t drb, bool has_sdap_rx, bool has_sdap_tx)
{
  if(qfi < SDAP_MAX_QFI &&
     qfi > SDAP_MAP_RULE_EMPTY &&
     drb > 0 &&
     drb <= AVLBL_DRB){
    entity->qfi2drb_table[qfi].drb_id = drb;
    entity->qfi2drb_table[qfi].has_sdap_rx = has_sdap_rx;
    entity->qfi2drb_table[qfi].has_sdap_tx = has_sdap_tx;
    LOG_D(SDAP, "Updated mapping: QFI %u -> DRB %ld \n", qfi, entity->qfi2drb_table[qfi].drb_id);
  } else {
    LOG_D(SDAP, "Map updated failed, QFI: %u, DRB: %ld\n", qfi, drb);
  }
}

void nr_sdap_qfi2drb_map_del(nr_sdap_entity_t *entity, uint8_t qfi){
  entity->qfi2drb_table[qfi].drb_id = SDAP_NO_MAPPING_RULE;
  LOG_D(SDAP, "Deleted mapping for QFI: %u \n", qfi);
}

rb_id_t nr_sdap_qfi2drb_map(nr_sdap_entity_t *entity, uint8_t qfi, rb_id_t upper_layer_rb_id){
  rb_id_t pdcp_entity;

  pdcp_entity = entity->qfi2drb_table[qfi].drb_id;

  if(pdcp_entity){
    LOG_D(SDAP, "Mapping rule exists for QFI: %u\n", qfi);
    return pdcp_entity;
  } else if(entity->default_drb) {
    LOG_D(SDAP, "Mapping QFI: %u to Default DRB: %ld\n", qfi, entity->default_drb);
    entity->qfi2drb_map_update(entity, qfi, entity->default_drb, entity->qfi2drb_table[qfi].has_sdap_rx, entity->qfi2drb_table[qfi].has_sdap_tx);
    return entity->default_drb;
  } else {
    LOG_D(SDAP, "Mapping rule and default DRB do not exist for QFI:%u\n", qfi);
    return SDAP_MAP_RULE_EMPTY;
  }

  return pdcp_entity;
}

nr_sdap_ul_hdr_t nr_sdap_construct_ctrl_pdu(uint8_t qfi){
  nr_sdap_ul_hdr_t sdap_end_marker_hdr;
  sdap_end_marker_hdr.QFI = qfi;
  sdap_end_marker_hdr.R = 0;
  sdap_end_marker_hdr.DC = SDAP_HDR_UL_CTRL_PDU;
  LOG_D(SDAP, "Constructed Control PDU with QFI:%u R:%u DC:%u \n", sdap_end_marker_hdr.QFI,
                                                                   sdap_end_marker_hdr.R,
                                                                   sdap_end_marker_hdr.DC);
  return sdap_end_marker_hdr;
}

rb_id_t nr_sdap_map_ctrl_pdu(nr_sdap_entity_t *entity, rb_id_t pdcp_entity, int map_type, uint8_t dl_qfi){
  rb_id_t drb_of_endmarker = 0;
  if(map_type == SDAP_CTRL_PDU_MAP_DEF_DRB){
    drb_of_endmarker = entity->default_drb;
    LOG_D(SDAP, "Mapping Control PDU QFI: %u to Default DRB: %ld\n", dl_qfi, drb_of_endmarker);
  }
  if(map_type == SDAP_CTRL_PDU_MAP_RULE_DRB){
    drb_of_endmarker = entity->qfi2drb_map(entity, dl_qfi, pdcp_entity);
    LOG_D(SDAP, "Mapping Control PDU QFI: %u to DRB: %ld\n", dl_qfi, drb_of_endmarker);
  }
  return drb_of_endmarker;
}

void nr_sdap_submit_ctrl_pdu(ue_id_t ue_id, rb_id_t sdap_ctrl_pdu_drb, nr_sdap_ul_hdr_t ctrl_pdu)
{
  if(sdap_ctrl_pdu_drb){
    nr_pdcp_submit_sdap_ctrl_pdu(ue_id, sdap_ctrl_pdu_drb, ctrl_pdu);
    LOG_D(SDAP, "Sent Control PDU to PDCP Layer.\n");
  }
}

void nr_sdap_ue_qfi2drb_config(nr_sdap_entity_t *existing_sdap_entity, rb_id_t pdcp_entity, ue_id_t ue_id, NR_QFI_t *mapped_qfi_2_add, uint8_t mappedQFIs2AddCount, uint8_t drb_identity, bool has_sdap_rx, bool has_sdap_tx)
{
  LOG_D(SDAP, "RRC Configuring SDAP Entity\n");
  uint8_t qfi = 0;

  for(int i = 0; i < mappedQFIs2AddCount; i++){
    qfi = mapped_qfi_2_add[i];
    if (existing_sdap_entity->default_drb && existing_sdap_entity->qfi2drb_table[qfi].drb_id == SDAP_NO_MAPPING_RULE) {
      nr_sdap_ul_hdr_t sdap_ctrl_pdu = existing_sdap_entity->sdap_construct_ctrl_pdu(qfi);
      rb_id_t sdap_ctrl_pdu_drb = existing_sdap_entity->sdap_map_ctrl_pdu(existing_sdap_entity, pdcp_entity, SDAP_CTRL_PDU_MAP_DEF_DRB, qfi);
      existing_sdap_entity->sdap_submit_ctrl_pdu(ue_id, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
    }
    if (existing_sdap_entity->qfi2drb_table[qfi].drb_id != drb_identity && existing_sdap_entity->qfi2drb_table[qfi].has_sdap_tx) {
      nr_sdap_ul_hdr_t sdap_ctrl_pdu = existing_sdap_entity->sdap_construct_ctrl_pdu(qfi);
      rb_id_t sdap_ctrl_pdu_drb = existing_sdap_entity->sdap_map_ctrl_pdu(existing_sdap_entity, pdcp_entity, SDAP_CTRL_PDU_MAP_RULE_DRB, qfi);
      existing_sdap_entity->sdap_submit_ctrl_pdu(ue_id, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
    }
    LOG_D(SDAP, "Storing the configured QoS flow to DRB mapping rule\n");
    existing_sdap_entity->qfi2drb_map_update(existing_sdap_entity, qfi, drb_identity, has_sdap_rx, has_sdap_tx);
  }
}

nr_sdap_entity_t *new_nr_sdap_entity(int is_gnb,
                                     bool has_sdap_rx,
                                     bool has_sdap_tx,
                                     ue_id_t ue_id,
                                     int pdusession_id,
                                     bool is_defaultDRB,
                                     uint8_t drb_identity,
                                     NR_QFI_t *mapped_qfi_2_add,
                                     uint8_t mappedQFIs2AddCount)
{
  if (nr_sdap_get_entity(ue_id, pdusession_id)) {
    LOG_E(SDAP, "SDAP Entity for UE already exists with RNTI/UE ID: %lu and PDU SESSION ID: %d\n", ue_id, pdusession_id);
    nr_sdap_entity_t *existing_sdap_entity = nr_sdap_get_entity(ue_id, pdusession_id);
    rb_id_t pdcp_entity = existing_sdap_entity->default_drb;
    if(!is_gnb)
      nr_sdap_ue_qfi2drb_config(existing_sdap_entity, pdcp_entity, ue_id, mapped_qfi_2_add, mappedQFIs2AddCount, drb_identity, has_sdap_rx, has_sdap_tx);
    return existing_sdap_entity;
  }

  nr_sdap_entity_t *sdap_entity;
  sdap_entity = calloc(1, sizeof(nr_sdap_entity_t));

  if(sdap_entity == NULL) {
    LOG_E(SDAP, "SDAP Entity creation failed, out of memory\n");
    exit(1);
  }

  sdap_entity->ue_id = ue_id;
  sdap_entity->pdusession_id = pdusession_id;

  sdap_entity->tx_entity = nr_sdap_tx_entity;
  sdap_entity->rx_entity = nr_sdap_rx_entity;

  sdap_entity->sdap_construct_ctrl_pdu = nr_sdap_construct_ctrl_pdu;
  sdap_entity->sdap_map_ctrl_pdu = nr_sdap_map_ctrl_pdu;
  sdap_entity->sdap_submit_ctrl_pdu = nr_sdap_submit_ctrl_pdu;

  sdap_entity->qfi2drb_map_update = nr_sdap_qfi2drb_map_update;
  sdap_entity->qfi2drb_map_delete = nr_sdap_qfi2drb_map_del;
  sdap_entity->qfi2drb_map = nr_sdap_qfi2drb_map;

  if(is_defaultDRB) {
    sdap_entity->default_drb = drb_identity;
    LOG_I(SDAP, "Default DRB for the created SDAP entity: %ld \n", sdap_entity->default_drb);
    LOG_D(SDAP, "RRC updating mapping rules: %d\n", mappedQFIs2AddCount);
    for (int i = 0; i < mappedQFIs2AddCount; i++)
      sdap_entity->qfi2drb_map_update(sdap_entity, mapped_qfi_2_add[i], sdap_entity->default_drb, has_sdap_rx, has_sdap_tx);
  }

  sdap_entity->next_entity = sdap_info.sdap_entity_llist;
  sdap_info.sdap_entity_llist = sdap_entity;
  return sdap_entity;
}

nr_sdap_entity_t *nr_sdap_get_entity(ue_id_t ue_id, int pdusession_id)
{
  nr_sdap_entity_t *sdap_entity;
  sdap_entity = sdap_info.sdap_entity_llist;

  if(sdap_entity == NULL)
    return NULL;

  while ((sdap_entity->ue_id != ue_id || sdap_entity->pdusession_id != pdusession_id) && sdap_entity->next_entity != NULL) {
    sdap_entity = sdap_entity->next_entity;
  }

  if (sdap_entity->ue_id == ue_id && sdap_entity->pdusession_id == pdusession_id)
    return sdap_entity;

  return NULL;
}

void nr_sdap_release_drb(ue_id_t ue_id, int drb_id, int pdusession_id)
{
  // remove all QoS flow to DRB mappings associated with the released DRB
  nr_sdap_entity_t *sdap = nr_sdap_get_entity(ue_id, pdusession_id);
  if (sdap) {
    for (int i = 0; i < SDAP_MAX_QFI; i++) {
      if (sdap->qfi2drb_table[i].drb_id == drb_id)
        sdap->qfi2drb_table[i].drb_id = SDAP_NO_MAPPING_RULE;
    }
  }
  else
    LOG_E(SDAP, "Couldn't find a SDAP entity associated with PDU session ID %d\n",
          pdusession_id);
}

bool nr_sdap_delete_entity(ue_id_t ue_id, int pdusession_id)
{
  nr_sdap_entity_t *entityPtr = sdap_info.sdap_entity_llist;
  nr_sdap_entity_t *entityPrev = NULL;
  bool ret = false;
  int upperBound = 0;

  if (entityPtr == NULL && (pdusession_id) * (pdusession_id - NGAP_MAX_PDU_SESSION) > 0) {
    LOG_E(SDAP, "SDAP entities not established or Invalid range of pdusession_id [0, 256].\n");
    return ret;
  }
  LOG_D(SDAP, "Deleting SDAP entity for UE %lx and PDU Session id %d\n", ue_id, entityPtr->pdusession_id);

  if (entityPtr->ue_id == ue_id && entityPtr->pdusession_id == pdusession_id) {
    sdap_info.sdap_entity_llist = sdap_info.sdap_entity_llist->next_entity;
    free(entityPtr);
    LOG_D(SDAP, "Successfully deleted Entity.\n");
    ret = true;
  } else {
    while ((entityPtr->ue_id != ue_id || entityPtr->pdusession_id != pdusession_id) && entityPtr->next_entity != NULL
           && upperBound < SDAP_MAX_NUM_OF_ENTITIES) {
      entityPrev = entityPtr;
      entityPtr = entityPtr->next_entity;
      upperBound++;
    }

    if (entityPtr->ue_id == ue_id && entityPtr->pdusession_id == pdusession_id) {
      entityPrev->next_entity = entityPtr->next_entity;
      free(entityPtr);
      LOG_D(SDAP, "Successfully deleted Entity.\n");
      ret = true;
    }
  }
  LOG_E(SDAP, "Entity does not exist or it was not found.\n");
  return ret;
}

bool nr_sdap_delete_ue_entities(ue_id_t ue_id)
{
  nr_sdap_entity_t *entityPtr = sdap_info.sdap_entity_llist;
  nr_sdap_entity_t *entityPrev = NULL;
  int upperBound = 0;
  bool ret = false;

  if (entityPtr == NULL && (ue_id) * (ue_id - SDAP_MAX_UE_ID) > 0) {
    LOG_W(SDAP, "SDAP entities not established or Invalid range of ue_id [0, 65536]\n");
    return ret;
  }

  /* Handle scenario where ue_id matches the head of the list */
  while (entityPtr != NULL && entityPtr->ue_id == ue_id && upperBound < MAX_DRBS_PER_UE) {
    sdap_info.sdap_entity_llist = entityPtr->next_entity;
    free(entityPtr);
    entityPtr = sdap_info.sdap_entity_llist;
    ret = true;
  }

  while (entityPtr != NULL && upperBound < SDAP_MAX_NUM_OF_ENTITIES) {
    if (entityPtr->ue_id != ue_id) {
      entityPrev = entityPtr;
      entityPtr = entityPtr->next_entity;
    } else {
      entityPrev->next_entity = entityPtr->next_entity;
      free(entityPtr);
      entityPtr = entityPrev->next_entity;
      LOG_I(SDAP, "Successfully deleted SDAP entity for UE %ld\n", ue_id);
      ret = true;
    }
  }
  return ret;
}
