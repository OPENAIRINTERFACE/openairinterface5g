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
#include <openair2/LAYER2/PDCP_v10.1.0/pdcp.h>
#include <openair3/ocp-gtpu/gtp_itf.h>
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_ue_manager.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct {
  nr_sdap_entity_t *sdap_entity_llist;
} nr_sdap_entity_info;

static nr_sdap_entity_info sdap_info;

nr_pdcp_ue_manager_t *nr_pdcp_sdap_get_ue_manager(void);

void nr_pdcp_submit_sdap_ctrl_pdu(int rnti, rb_id_t sdap_ctrl_pdu_drb, nr_sdap_ul_hdr_t ctrl_pdu){
  nr_pdcp_ue_t *ue;
  nr_pdcp_ue_manager_t *nr_pdcp_ue_manager;
  nr_pdcp_ue_manager = nr_pdcp_sdap_get_ue_manager();
  ue = nr_pdcp_manager_get_ue(nr_pdcp_ue_manager, rnti);
  ue->drb[sdap_ctrl_pdu_drb-1]->recv_sdu(ue->drb[sdap_ctrl_pdu_drb-1], (char*)&ctrl_pdu, SDAP_HDR_LENGTH, RLC_MUI_UNDEFINED);
  return;
}

static boolean_t nr_sdap_tx_entity(nr_sdap_entity_t *entity,
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
                                   const boolean_t rqi
                                  ) {
  /* The offset of the SDAP header, it might be 0 if the has_sdap is not true in the pdcp entity. */
  int offset=0;
  boolean_t ret=false;
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
    pdcp_ent_has_sdap = entity->qfi2drb_table[qfi].hasSdap;
  }

  if(!pdcp_ent_has_sdap){
    ret = pdcp_data_req(ctxt_p,
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
    LOG_D(SDAP, "TX Entity R: %u \n", sdap_hdr.R);
    LOG_D(SDAP, "TX Entity DC: %u \n", sdap_hdr.DC);
  }

  /*
   * TS 37.324 5.2 Data transfer
   * 5.2.1 Uplink UE side
   * submit the constructed UL SDAP data PDU to the lower layers
   *
   * Downlink gNB side
   */
  ret = pdcp_data_req(ctxt_p,
                      srb_flag,
                      sdap_drb_id,
                      mui,
                      confirm,
                      sdu_buffer_size+offset,
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
                              int has_sdap,
                              int has_sdapHeader,
                              int pdusession_id,
                              int rnti,
                              char *buf,
                              int size) {
  /* The offset of the SDAP header, it might be 0 if the has_sdap is not true in the pdcp entity. */
  int offset=0;

  if(is_gnb) { // gNB
    if(has_sdap && has_sdapHeader ) { // Handling the SDAP Header
      offset = SDAP_HDR_LENGTH;
      nr_sdap_ul_hdr_t *sdap_hdr = (nr_sdap_ul_hdr_t *)buf;
      LOG_D(SDAP, "RX Entity Received QFI : %u\n", sdap_hdr->QFI);
      LOG_D(SDAP, "RX Entity Received Reserved bit : %u\n", sdap_hdr->R);
      LOG_D(SDAP, "RX Entity Received DC bit : %u\n", sdap_hdr->DC);

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
    MessageDef *message_p;
    uint8_t *gtpu_buffer_p;
    gtpu_buffer_p = itti_malloc(TASK_PDCP_ENB, TASK_GTPV1_U, size + GTPU_HEADER_OVERHEAD_MAX - offset);
    AssertFatal(gtpu_buffer_p != NULL, "OUT OF MEMORY");
    memcpy(&gtpu_buffer_p[GTPU_HEADER_OVERHEAD_MAX], buf+offset, size-offset);
    message_p = itti_alloc_new_message(TASK_PDCP_ENB, 0 , GTPV1U_GNB_TUNNEL_DATA_REQ);
    AssertFatal(message_p != NULL, "OUT OF MEMORY");
    GTPV1U_GNB_TUNNEL_DATA_REQ(message_p).buffer = gtpu_buffer_p;
    GTPV1U_GNB_TUNNEL_DATA_REQ(message_p).length              = size-offset;
    GTPV1U_GNB_TUNNEL_DATA_REQ(message_p).offset              = GTPU_HEADER_OVERHEAD_MAX;
    GTPV1U_GNB_TUNNEL_DATA_REQ(message_p).rnti                = rnti;
    GTPV1U_GNB_TUNNEL_DATA_REQ(message_p).pdusession_id       = pdusession_id;
    LOG_D(SDAP, "%s()  sending message to gtp size %d\n", __func__,  size-offset);
    itti_send_msg_to_task(TASK_GTPV1_U, INSTANCE_DEFAULT, message_p);
  } else { //nrUE
    /*
     * TS 37.324 5.2 Data transfer
     * 5.2.2 Downlink
     * if the DRB from which this SDAP data PDU is received is configured by RRC with the presence of SDAP header.
     */
    if(has_sdap && has_sdapHeader) { // Handling the SDAP Header
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
        /*
         * TS 37.324 5.3 QoS flow to DRB Mapping 
         * 5.3.2 Reflective mapping
         * If there is no stored QoS flow to DRB mapping rule for the QoS flow and a default DRB is configured.
         */
        if(!entity->qfi2drb_table[sdap_hdr->QFI].drb_id && entity->default_drb){
          nr_sdap_ul_hdr_t sdap_ctrl_pdu = entity->sdap_construct_ctrl_pdu(sdap_hdr->QFI);
          rb_id_t sdap_ctrl_pdu_drb = entity->sdap_map_ctrl_pdu(entity, pdcp_entity, SDAP_CTRL_PDU_MAP_DEF_DRB, sdap_hdr->QFI);
          entity->sdap_submit_ctrl_pdu(rnti, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
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
        if( (pdcp_entity != entity->qfi2drb_table[sdap_hdr->QFI].drb_id) && 
             has_sdapHeader ){
          nr_sdap_ul_hdr_t sdap_ctrl_pdu = entity->sdap_construct_ctrl_pdu(sdap_hdr->QFI);
          rb_id_t sdap_ctrl_pdu_drb = entity->sdap_map_ctrl_pdu(entity, pdcp_entity, SDAP_CTRL_PDU_MAP_RULE_DRB, sdap_hdr->QFI);
          entity->sdap_submit_ctrl_pdu(rnti, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
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

void nr_sdap_qfi2drb_map_update(nr_sdap_entity_t *entity, uint8_t qfi, rb_id_t drb, boolean_t hasSdap){
  if(qfi < SDAP_MAX_QFI &&
     qfi > SDAP_MAP_RULE_EMPTY &&
     drb > 0 &&
     drb <= AVLBL_DRB)
  {
    entity->qfi2drb_table[qfi].drb_id = drb;
    entity->qfi2drb_table[qfi].hasSdap = hasSdap;
    LOG_D(SDAP, "Updated QFI to DRB Map: QFI %u -> DRB %ld \n", qfi, entity->qfi2drb_table[qfi].drb_id);
    LOG_D(SDAP, "DRB %ld %s\n", entity->qfi2drb_table[qfi].drb_id, hasSdap ? "has SDAP" : "does not have SDAP");
  }
}

void nr_sdap_qfi2drb_map_del(nr_sdap_entity_t *entity, uint8_t qfi){
  entity->qfi2drb_table[qfi].drb_id = SDAP_NO_MAPPING_RULE;
  LOG_D(SDAP, "Deleted QFI to DRB Map for QFI %u \n", qfi);
}

rb_id_t nr_sdap_qfi2drb_map(nr_sdap_entity_t *entity, uint8_t qfi, rb_id_t upper_layer_rb_id){
  rb_id_t pdcp_entity;

  pdcp_entity = entity->qfi2drb_table[qfi].drb_id;

  if(pdcp_entity){
    return pdcp_entity;
  } else if(entity->default_drb) {
    LOG_D(SDAP, "Mapped QFI %u to Default DRB\n", qfi);
    return entity->default_drb;
  } else {
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
    LOG_D(SDAP, "Mapped Control PDU to default drb\n");
  }
  if(map_type == SDAP_CTRL_PDU_MAP_RULE_DRB){
    drb_of_endmarker = entity->qfi2drb_map(entity, dl_qfi, pdcp_entity);
    LOG_D(SDAP, "Mapped Control PDU according to the mapping rule, qfi %u \n", dl_qfi);
  }
  return drb_of_endmarker;
}

void nr_sdap_submit_ctrl_pdu(int rnti, rb_id_t sdap_ctrl_pdu_drb, nr_sdap_ul_hdr_t ctrl_pdu){
  if(sdap_ctrl_pdu_drb){
    nr_pdcp_submit_sdap_ctrl_pdu(rnti, sdap_ctrl_pdu_drb, ctrl_pdu);
    LOG_D(SDAP, "Sent Control PDU to PDCP Layer.\n");
  }
}

void nr_sdap_ue_qfi2drb_config(nr_sdap_entity_t *existing_sdap_entity,
                               rb_id_t pdcp_entity,
                               uint16_t rnti,
                               NR_QFI_t *mapped_qfi_2_add,
                               uint8_t mappedQFIs2AddCount,
                               uint8_t drb_identity)
{
  uint8_t qfi = 0;

  for(int i = 0; i < mappedQFIs2AddCount; i++){
    qfi = mapped_qfi_2_add[i];
    if(existing_sdap_entity->default_drb && existing_sdap_entity->qfi2drb_table[qfi].drb_id == SDAP_NO_MAPPING_RULE){
      nr_sdap_ul_hdr_t sdap_ctrl_pdu = existing_sdap_entity->sdap_construct_ctrl_pdu(qfi);
      rb_id_t sdap_ctrl_pdu_drb = existing_sdap_entity->sdap_map_ctrl_pdu(existing_sdap_entity, pdcp_entity, SDAP_CTRL_PDU_MAP_DEF_DRB, qfi);
      existing_sdap_entity->sdap_submit_ctrl_pdu(rnti, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
    }
    if(existing_sdap_entity->qfi2drb_table[qfi].drb_id != drb_identity && existing_sdap_entity->qfi2drb_table[qfi].hasSdap){
      nr_sdap_ul_hdr_t sdap_ctrl_pdu = existing_sdap_entity->sdap_construct_ctrl_pdu(qfi);
      rb_id_t sdap_ctrl_pdu_drb = existing_sdap_entity->sdap_map_ctrl_pdu(existing_sdap_entity, pdcp_entity, SDAP_CTRL_PDU_MAP_RULE_DRB, qfi);
      existing_sdap_entity->sdap_submit_ctrl_pdu(rnti, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
    }
  }
}

nr_sdap_entity_t *new_nr_sdap_entity(int has_sdap,
                                     uint16_t rnti,
                                     int pdusession_id,
                                     boolean_t is_defaultDRB,
                                     uint8_t drb_identity,
                                     NR_QFI_t *mapped_qfi_2_add,
                                     uint8_t mappedQFIs2AddCount)
{
  if(nr_sdap_get_entity(rnti, pdusession_id)) {
    LOG_E(SDAP, "SDAP Entity for UE already exists.\n");
    nr_sdap_entity_t *existing_sdap_entity = nr_sdap_get_entity(rnti, pdusession_id);
    rb_id_t pdcp_entity = existing_sdap_entity->default_drb;
    nr_sdap_ue_qfi2drb_config(existing_sdap_entity, pdcp_entity, rnti, mapped_qfi_2_add, mappedQFIs2AddCount, drb_identity);
    return existing_sdap_entity;
  }

  nr_sdap_entity_t *sdap_entity;
  sdap_entity = calloc(1, sizeof(nr_sdap_entity_t));

  if(sdap_entity == NULL) {
    LOG_E(SDAP, "SDAP Entity creation failed, out of memory\n");
    exit(1);
  }

  sdap_entity->rnti = rnti;
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

    if(mappedQFIs2AddCount) {
      for (int i = 0; i < mappedQFIs2AddCount; i++)
      {
        LOG_D(SDAP, "Mapped QFI to Add : %ld \n", mapped_qfi_2_add[i]);
        sdap_entity->qfi2drb_map_update(sdap_entity, mapped_qfi_2_add[i], sdap_entity->default_drb, has_sdap);
      }
    }
  }

  sdap_entity->next_entity = sdap_info.sdap_entity_llist;
  sdap_info.sdap_entity_llist = sdap_entity;
  return sdap_entity;
}

nr_sdap_entity_t *nr_sdap_get_entity(uint16_t rnti, int pdusession_id) {
  nr_sdap_entity_t *sdap_entity;
  sdap_entity = sdap_info.sdap_entity_llist;

  if(sdap_entity == NULL)
    return NULL;

  while(( sdap_entity->rnti != rnti || sdap_entity->pdusession_id != pdusession_id ) && sdap_entity->next_entity != NULL) {
    sdap_entity = sdap_entity->next_entity;
  }

  if (sdap_entity->rnti == rnti && sdap_entity->pdusession_id == pdusession_id)
    return sdap_entity;

  return NULL;
}


void delete_nr_sdap_entity(uint16_t rnti) {
  nr_sdap_entity_t *entityPtr, *entityPrev = NULL;
  entityPtr = sdap_info.sdap_entity_llist;

  if(entityPtr->rnti == rnti) {
    sdap_info.sdap_entity_llist = sdap_info.sdap_entity_llist->next_entity;
    free(entityPtr);
  } else {
    while(entityPtr->rnti != rnti && entityPtr->next_entity != NULL) {
      entityPrev = entityPtr;
      entityPtr = entityPtr->next_entity;
    }

    if(entityPtr->rnti != rnti) {
      entityPrev->next_entity = entityPtr->next_entity;
      free(entityPtr);
    }
  }
}
