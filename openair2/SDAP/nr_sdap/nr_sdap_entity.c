/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "nr_sdap_entity.h"
#include <openair2/LAYER2/nr_pdcp/nr_pdcp_oai_api.h>
#include <openair3/ocp-gtpu/gtp_itf.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "T.h"
#include "assertions.h"
#include "common/utils/T/T.h"
#include "gtpv1_u_messages_types.h"
#include "intertask_interface.h"
#include "openair2/LAYER2/RLC/rlc.h"
#include "nr_sdap.h"
#include "tuntap_if.h"
#include "utils.h"

#define NO_SDAP_HEADER 0

typedef struct {
  nr_sdap_entity_t *sdap_entity_llist;
} nr_sdap_entity_info;

static nr_sdap_entity_info sdap_info;

instance_t *N3GTPUInst = NULL;

/** @brief Tear down gNB TUN dataplane for one SDAP entity
 * Skip UE entities: NAS owns the TUN fd so it survives SDAP delete in 5GMM-IDLE
 * (needed for MO Service Request UL and re-bind after UP restore, TS 24.501 clause 5.6.1) */
static void remove_ip_if(nr_sdap_entity_t *entity)
{
  DevAssert(entity != NULL);
  sdap_tun_endpoint_t *tun = &entity->tun;

  if (!tun->is_gnb) {
    nr_sdap_qos_rules_free(entity);
    return; /* UE: NAS owns the TUN fd, do not close/destroy */
  }
  if (tun->sock < 0)
    return;

  /* Stop/join the reader before close: on Linux, close() may not wake a blocked read() */
  nr_sdap_tun_stop_reader(&entity->pdusession_thread);
  close(tun->sock);
  tun->sock = -1;

  char ifname[IFNAMSIZ];
  nr_sdap_generate_gnb_tun_ifname(ifname, tun->ue_id);
  tuntap_destroy(ifname);
  LOG_I(SDAP, "Destroyed TUN dataplane for UE %ld PDU session %d (%s)\n", tun->ue_id, tun->pdusession_id, ifname);
}

/** @brief Returns a bitmap indicating the SDAP entity role,
 *        i.e. for UL transmission, header for UL data is present in RX/TX
 *             for DL transmission, header for DL data is present in RX/TX */
static int get_sdap_role(bool is_gnb, const NR_SDAP_Config_t *sdap_config)
{
  sdap_role_t role_ul = is_gnb ? SDAP_UL_RX : SDAP_UL_TX;
  sdap_role_t role_dl = is_gnb ? SDAP_DL_TX : SDAP_DL_RX;
  const bool ul_hdr = sdap_config->sdap_HeaderUL == NR_SDAP_Config__sdap_HeaderUL_present;
  const bool dl_hdr = sdap_config->sdap_HeaderDL == NR_SDAP_Config__sdap_HeaderDL_present;
  int role = NO_SDAP_HEADER;
  if (ul_hdr)
    role |= role_ul;
  if (dl_hdr)
    role |= role_dl;
  LOG_D(SDAP,
        "PDU session %ld %s: sdap-HeaderUL %s, sdap-HeaderDL %s (role 0x%x)\n",
        sdap_config->pdu_Session,
        is_gnb ? "gNB" : "UE",
        ul_hdr ? "present" : "absent",
        dl_hdr ? "present" : "absent",
        role);
  return role;
}

/** Return the first table row for drb_id (all QFIs on a DRB share entity_role) */
static const qfi2drb_t *nr_sdap_drb_lookup(const nr_sdap_entity_t *entity, int drb_id)
{
  for (int qfi = 0; qfi < SDAP_MAX_QFI; qfi++) {
    if (entity->qfi2drb_table[qfi].drb_id == drb_id)
      return &entity->qfi2drb_table[qfi];
  }
  if (entity->default_drb.drb_id == drb_id)
    return &entity->default_drb;
  return NULL;
}

void nr_pdcp_submit_sdap_ctrl_pdu(ue_id_t ue_id, int sdap_ctrl_pdu_drb, nr_sdap_ul_hdr_t ctrl_pdu)
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
  LOG_D(SDAP, "Control PDU - Submitting Control PDU to DRB ID: %d\n", sdap_ctrl_pdu_drb);
  LOG_D(SDAP, "QFI: %u\n R: %u\n D/C: %u\n", ctrl_pdu.QFI, ctrl_pdu.R, ctrl_pdu.DC);
  return;
}

static bool nr_sdap_tx_entity(nr_sdap_entity_t *entity,
                              protocol_ctxt_t *ctxt_p,
                              const srb_flag_t srb_flag,
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
  bool sdap_ul_tx = false;
  bool sdap_dl_tx = false;

  if(sdu_buffer == NULL) {
    LOG_E(SDAP, "%s:%d:%s: NULL sdu_buffer \n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  uint8_t sdap_buf[SDAP_MAX_PDU];
  const qfi2drb_t *map = entity->qfi2drb_map(entity, qfi);
  if (!map) {
    LOG_W(SDAP, "Dropping TX SDAP SDU: no DRB mapping for QFI %u (pdu_session=%d)\n", qfi, entity->tun.pdusession_id);
    return false;
  }
  const int drb_id = map->drb_id;
  const int drb_role = map->entity_role;

  sdap_ul_tx = drb_role & SDAP_UL_TX;
  sdap_dl_tx = drb_role & SDAP_DL_TX;
  LOG_D(SDAP, "TX - QFI: %u is mapped to DRB ID: %d\n", qfi, drb_id);

  if (!sdap_ul_tx && !sdap_dl_tx) {
    LOG_D(SDAP, "TX - DRB ID: %d does not have SDAP header\n", drb_id);
    ret = nr_pdcp_data_req_drb(ctxt_p,
                               srb_flag,
                               drb_id,
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

  // Reuse a QFI that the UE has already mapped to the default DRB to avoid
  // encoding an invalid QFI in the SDAP header. This follows TS 37.324 5.2.1,
  // which maps SDAP SDUs from unmapped QoS flows (e.g., a UFP-generated Router Advertisement) to the default DRB.
  uint8_t hdr_qfi = qfi;
  if (qfi >= SDAP_MAX_QFI) { // qfi = -1 (uint_8 255)
    for (int i = 0; i < SDAP_MAX_QFI; i++) {
      if (entity->qfi2drb_table[i].drb_id == drb_id) {
        hdr_qfi = i;
        break;
      }
    }
    DevAssert (hdr_qfi < SDAP_MAX_QFI);
    LOG_W(SDAP, "TX SDAP SDU with no QFI marking: reusing QFI %u already mapped to DRB %d\n", hdr_qfi, drb_id);
  }

  if (sdap_dl_tx) { // create DL Data PDU with SDAP header
    offset = SDAP_HDR_LENGTH;
    /*
     * TS 37.324 4.4 Functions
     * marking QoS flow ID in DL packets.
     *
     * Construct the DL SDAP data PDU.
     */
    nr_sdap_dl_hdr_t sdap_hdr;
    sdap_hdr.QFI = hdr_qfi;
    sdap_hdr.RQI = rqi;
    sdap_hdr.RDI = 0; // SDAP Hardcoded Value
    /* Add the SDAP DL Header to the buffer */
    memcpy(&sdap_buf[0], &sdap_hdr, SDAP_HDR_LENGTH);
    memcpy(&sdap_buf[SDAP_HDR_LENGTH], sdu_buffer, sdu_buffer_size);
    LOG_D(SDAP, "TX Entity QFI: %u \n", sdap_hdr.QFI);
    LOG_D(SDAP, "TX Entity RQI: %u \n", sdap_hdr.RQI);
    LOG_D(SDAP, "TX Entity RDI: %u \n", sdap_hdr.RDI);
  } else if (sdap_ul_tx) { // create UL Data PDU with SDAP header
    offset = SDAP_HDR_LENGTH;
    /*
     * TS 37.324 4.4 Functions
     * marking QoS flow ID in UL packets.
     *
     * 5.2.1 Uplink
     * construct the UL SDAP data PDU as specified in the subclause 6.2.2.3.
     */
    nr_sdap_ul_hdr_t sdap_hdr;
    sdap_hdr.QFI = hdr_qfi;
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
                             drb_id,
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

static void
nr_sdap_rx_entity(nr_sdap_entity_t *entity, int drb_id, int is_gnb, int pdusession_id, ue_id_t ue_id, char *buf, int size)
{
  int qfi = -1;
  const qfi2drb_t *map = nr_sdap_drb_lookup(entity, drb_id);
  if (!map) {
    LOG_W(SDAP, "Dropping RX payload: DRB %d has no qfi2drb_table entry (ue=%ld, pdu_session=%d)\n", drb_id, ue_id, pdusession_id);
    return;
  }
  const int drb_role = map->entity_role;
  bool sdap_header_rx = is_gnb ? (drb_role & SDAP_UL_RX) : (drb_role & SDAP_DL_RX);

  if (is_gnb) {
    if (sdap_header_rx) {
     /** Extract QFI from SDAP header for gNB UL RX
      * Per TS 37.324 §6.2.2, QFI is carried in SDAP header when SDAP header is present
      * This QFI will be used in GTP-U extension header (PDU Session Container)
      * for N3-U tunnel (the first 6 bits in the first octet) */
      const nr_sdap_ul_hdr_t *sdap_hdr = (const nr_sdap_ul_hdr_t *)buf;
      qfi = sdap_hdr->QFI;
      if (qfi >= SDAP_MAX_QFI) {
        LOG_E(SDAP, "Invalid QFI %d received in SDAP header\n", qfi);
        return;
      }
      /* TS 37.324 §5.2.1: with no stored rule, map to the default DRB when configured
       * TS 38.331: default DRB always has a UL SDAP header
       * Drop only when neither a stored rule nor default DRB applies to this QFI */
      const qfi2drb_t *qfi_map = entity->qfi2drb_map(entity, qfi);
      if (!qfi_map || qfi_map->drb_id != drb_id) {
        LOG_W(SDAP,
              "Dropping UL PDU: QFI=%d %s (ue=%ld, pdu_session=%d, receiving_drb=%d)\n",
              qfi,
              !qfi_map ? "no rule/default DRB" : "wrong DRB",
              ue_id,
              pdusession_id,
              drb_id);
        return;
      }

      int offset = SDAP_HDR_LENGTH;
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
      // Pushing SDAP SDU to GTP-U Layer
      size_t gtp_len = size - offset;
      uint8_t *gtp_buf = (uint8_t *)buf + offset;
      LOG_D(SDAP,
            "UL SDAP->GTP: ue=%lu pdu_session=%d qfi=%u payload_len=%zu\n",
            ue_id,
            pdusession_id,
            qfi,
            gtp_len);
      /** Send to GTP-U with QFI marking for N3-U tunnel
       * Per TS 29.281 §5.2, QFI is carried in PDU Session Container extension header
       * For N3-U tunnels (qfi>=0), QFI is added to extension header for UL routing at UPF */
      gtpv1uSendDirectWithQFI(*N3GTPUInst, ue_id, pdusession_id, qfi, gtp_buf, gtp_len);
    } else {
      /* N3-U tunnel with no SDAP header: no QFI marking */
      DevAssert(qfi == -1);
      LOG_D(SDAP, "UL SDAP->GTP: ue=%lu pdu_session=%d payload_len=%d\n", ue_id, pdusession_id, size);
      gtpv1uSendDirect(*N3GTPUInst, ue_id, pdusession_id, (uint8_t *)buf, size, false, false);
    }
  } else { //nrUE
    /*
     * TS 37.324 5.2 Data transfer
     * 5.2.2 Downlink
     * if the DRB from which this SDAP data PDU is received is configured by RRC with the presence of SDAP header.
     */
    int offset = 0;
    if (sdap_header_rx) { // DL Data/Control PDU with SDAP header
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
        if (!entity->qfi2drb_table[sdap_hdr->QFI].drb_id && entity->default_drb.drb_id) {
          nr_sdap_ul_hdr_t sdap_ctrl_pdu = entity->sdap_construct_ctrl_pdu(sdap_hdr->QFI);
          int sdap_ctrl_pdu_drb = entity->sdap_map_ctrl_pdu(entity, SDAP_CTRL_PDU_MAP_DEF_DRB, sdap_hdr->QFI);
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
        if (drb_id != entity->qfi2drb_table[sdap_hdr->QFI].drb_id) {
          nr_sdap_ul_hdr_t sdap_ctrl_pdu = entity->sdap_construct_ctrl_pdu(sdap_hdr->QFI);
          int sdap_ctrl_pdu_drb = entity->sdap_map_ctrl_pdu(entity, SDAP_CTRL_PDU_MAP_RULE_DRB, sdap_hdr->QFI);
          entity->sdap_submit_ctrl_pdu(ue_id, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
        }

        /*
         * TS 37.324 §5.3.2 Reflective mapping: store the DL QoS flow to DRB mapping
         * as the UL mapping rule. sdap-HeaderUL/DL are per DRB: copy the
         * receiving DRB role onto the QFI row together with drb_id.
         */
        entity->qfi2drb_table[sdap_hdr->QFI].drb_id = drb_id;
        entity->qfi2drb_table[sdap_hdr->QFI].entity_role = drb_role;
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
    if (entity->tun.sock < 0) {
      LOG_D(SDAP, "[UE %ld] PDU session %d: TUN not attached, drop DL SDU (%d B)\n", ue_id, pdusession_id, size - offset);
      return;
    }
    int len = write(entity->tun.sock, &buf[offset], size - offset);
    LOG_D(SDAP, "RX Entity len : %d\n", len);
    LOG_D(SDAP, "RX Entity size : %d\n", size);
    LOG_D(SDAP, "RX Entity offset : %d\n", offset);

    if (len != size-offset)
      LOG_E(SDAP, "write failed to fd %d! errno = %s\n", entity->tun.sock, strerror(errno));
  }
}

static void nr_sdap_qfi2drb_map_add(nr_sdap_entity_t *entity,
                                    const uint8_t qfi,
                                    const uint8_t drb_id,
                                    const uint8_t role)
{
  qfi2drb_t *qfi2drb = &entity->qfi2drb_table[qfi];
  LOG_D(SDAP, "%s mapping: QFI %u -> DRB %d \n", qfi2drb->drb_id == SDAP_NO_MAPPING_RULE ? "Add" : "Update", qfi, drb_id);
  qfi2drb->drb_id = drb_id;
  qfi2drb->entity_role = role;
}

static void nr_sdap_qfi2drb_map_del(nr_sdap_entity_t *entity, const uint8_t qfi)
{
  qfi2drb_t *qfi2drb = &entity->qfi2drb_table[qfi];
  qfi2drb->drb_id = SDAP_NO_MAPPING_RULE;
  qfi2drb->entity_role = NO_SDAP_HEADER;
  LOG_D(SDAP, "Deleted mapping for QFI=%d, DRB=%d\n", qfi, qfi2drb->drb_id);
}

/**
 * @brief   get the DRB ID mapped to the QFI, for both DL and UL
 * @return  qfi2drb_table entry for the QFI {drb_id, entity_role},
 *          or the default DRB row if no mapping rule exists,
 *          or NULL if no mapping and no default DRB exists for that QFI
 * @ref     TS 37.324, 5.2.1 Uplink
 *          If there is no stored QoS flow to DRB mapping rule for the
 *          QoS flow as specified in the subclause 5.3, and a default DRB is configured,
 *          map the SDAP SDU to the default DRB else, map the SDAP SDU to the DRB according
 *          to the stored QoS flow to DRB mapping rule. */
static const qfi2drb_t *nr_sdap_qfi2drb(const nr_sdap_entity_t *entity, uint8_t qfi)
{
  /* Fetch DRB ID mapped to QFI */
  // Handle the out-of-bound case when a DL PDU arrives with no QFI marking (QFI = -1 (255 in uint8_t)).
  const qfi2drb_t *row = qfi < SDAP_MAX_QFI ? &entity->qfi2drb_table[qfi] : NULL;
  if (row && row->drb_id) {
    /* QoS flow to DRB mapping rule exists, return corresponding DRB ID */
    LOG_D(SDAP, "Existing QoS flow to DRB mapping rule: QFI %u to DRB %d\n", qfi, row->drb_id);
    return row;
  } else if (entity->default_drb.drb_id) {
    /* QoS flow to DRB mapping rule does not exist: map SDAP SDU to default DRB */
    LOG_D(SDAP, "QoS flow to DRB mapping rule does not exists! mapping SDU to Default DRB: %d\n", entity->default_drb.drb_id);
    return &entity->default_drb;
  } else {
    /* Note: UE undefined behaviour when neither a default DRB
       nor a stored QoS flow to DRB mapping rule exists */
    LOG_E(SDAP, "Mapping rule and default DRB do not exist for QFI:%u\n", qfi);
    return NULL;
  }
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

int nr_sdap_map_ctrl_pdu(nr_sdap_entity_t *entity, int map_type, uint8_t dl_qfi)
{
  int drb_of_endmarker = 0;
  if(map_type == SDAP_CTRL_PDU_MAP_DEF_DRB){
    drb_of_endmarker = entity->default_drb.drb_id;
    LOG_D(SDAP, "Mapping Control PDU QFI: %u to Default DRB: %d\n", dl_qfi, drb_of_endmarker);
  }
  if(map_type == SDAP_CTRL_PDU_MAP_RULE_DRB){
    const qfi2drb_t *map = entity->qfi2drb_map(entity, dl_qfi);
    if (!map) {
      LOG_E(SDAP, "No DRB mapping for control PDU QFI %u\n", dl_qfi);
      return 0;
    }
    drb_of_endmarker = map->drb_id;
    LOG_D(SDAP, "Mapping Control PDU QFI: %u to DRB: %d\n", dl_qfi, drb_of_endmarker);
  }
  return drb_of_endmarker;
}

/**
 * @brief Submit the end-marker control PDU to PDCP according to TS 37.324, clause 5.3
 */
void nr_sdap_submit_ctrl_pdu(ue_id_t ue_id, int sdap_ctrl_pdu_drb, nr_sdap_ul_hdr_t ctrl_pdu)
{
  if(sdap_ctrl_pdu_drb){
    nr_pdcp_submit_sdap_ctrl_pdu(ue_id, sdap_ctrl_pdu_drb, ctrl_pdu);
    LOG_D(SDAP, "Sent Control PDU to PDCP Layer.\n");
  }
}

/** @brief UL QoS flow to DRB mapping configuration for a SDAP entity has already been established
 *         according to TS 37.324, 5.3 QoS flow to DRB Mapping, 5.3.1 Configuration Procedures. */
static void nr_sdap_ue_control_pdu_config(nr_sdap_entity_t *entity, const ue_id_t ue_id, const sdap_config_t *sdap)
{
  // handle QFIs to DRB mapping rule to add
  for (int i = 0; i < sdap->mappedQFIs2AddCount; i++) {
    uint8_t qfi = sdap->mappedQFIs2Add[i];
    /* a default DRB exists and there is no stored QFI to DRB mapping rule for the QFI */
    if (entity->default_drb.drb_id && entity->qfi2drb_table[qfi].drb_id == SDAP_NO_MAPPING_RULE) {
      // construct an end-marker control PDU (6.2.3 TS 37.324)
      nr_sdap_ul_hdr_t sdap_ctrl_pdu = entity->sdap_construct_ctrl_pdu(qfi);
      // map the end-marker control PDU to the default DRB
      int sdap_ctrl_pdu_drb = entity->sdap_map_ctrl_pdu(entity, SDAP_CTRL_PDU_MAP_DEF_DRB, qfi);
      // submit the end-marker control PDU to the lower layers
      entity->sdap_submit_ctrl_pdu(ue_id, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
    }
    /* the stored UL QFI to DRB mapping rule is different from the configured one and has UL SDAP header */
    bool ul_sdap_header = (entity->qfi2drb_table[qfi].entity_role & SDAP_UL_TX) != 0;
    if (entity->qfi2drb_table[qfi].drb_id != sdap->drb_id && ul_sdap_header) {
      // construct an end-marker control PDU (6.2.3 TS 37.324)
      nr_sdap_ul_hdr_t sdap_ctrl_pdu = entity->sdap_construct_ctrl_pdu(qfi);
      // map the end-marker control PDU to the DRB according to the stored QoS flow to DRB mapping rule
      int sdap_ctrl_pdu_drb = entity->sdap_map_ctrl_pdu(entity, SDAP_CTRL_PDU_MAP_RULE_DRB, qfi);
      // submit the end-marker control PDU to the lower layers
      entity->sdap_submit_ctrl_pdu(ue_id, sdap_ctrl_pdu_drb, sdap_ctrl_pdu);
    }
  }
}

/** @brief Add QoS flows to a DRB */
static void nr_sdap_add_qos_flows_to_drb(nr_sdap_entity_t *entity, const sdap_config_t *sdap)
{
  for (int i = 0; i < sdap->mappedQFIs2AddCount; i++) {
    uint8_t qfi = sdap->mappedQFIs2Add[i];
    LOG_D(SDAP, "Adding QFI to DRB mapping rules: %d mapped QFIs for DRB %d\n", sdap->mappedQFIs2AddCount, sdap->drb_id);
    if (qfi < SDAP_MAX_QFI && sdap->drb_id > SDAP_NO_MAPPING_RULE && sdap->drb_id <= MAX_DRBS_PER_UE) {
      entity->qfi2drb_map_add(entity, qfi, sdap->drb_id, sdap->role);
    } else {
      LOG_E(SDAP, "Failed to add qfi2drb mapping: QFI=%d, DRB=%d\n", qfi, sdap->drb_id);
    }
  }
}

/** @brief Remove QoS flows from a DRB */
static void nr_sdap_rm_qos_flows_from_drb(nr_sdap_entity_t *entity, const sdap_config_t *sdap)
{
  for (int i = 0; i < sdap->mappedQFIs2ReleaseCount; i++) {
    uint8_t qfi = sdap->mappedQFIs2Release[i];
    LOG_D(SDAP, "Removing QFI from DRB mapping rules: QFI=%d for DRB=%d\n", qfi, sdap->drb_id);
    entity->qfi2drb_map_delete(entity, qfi);
  }
}

/** @brief Update QFI to DRB mapping rules
 * @param qfi the QoS Flow index, used as unique index of the qfi2drb mapping table
 * @param drb the DRB ID to be mapped */
static void nr_sdap_qfi2drb_map_update(nr_sdap_entity_t *entity, const sdap_config_t *sdap)
{
  nr_sdap_add_qos_flows_to_drb(entity, sdap);
  nr_sdap_rm_qos_flows_from_drb(entity, sdap);

  /* TS 38.331: all QFIs on a DRB share the same sdap-HeaderUL/DL role */
  for (int i = 0; i < SDAP_MAX_QFI; i++) {
    if (entity->qfi2drb_table[i].drb_id == sdap->drb_id)
      entity->qfi2drb_table[i].entity_role = sdap->role;
  }

  if (sdap->role == NO_SDAP_HEADER) {
    int qfis_on_this_drb = 0;
    for (int qfi = 0; qfi < SDAP_MAX_QFI; qfi++) {
      if (entity->qfi2drb_table[qfi].drb_id == sdap->drb_id) {
        qfis_on_this_drb++;
      }
    }
    AssertFatal(qfis_on_this_drb <= 1,
                "PDU session %d DRB %d: both SDAP headers absent: at most one QoS flow per DRB\n",
        entity->tun.pdusession_id,
                sdap->drb_id);
  }
}

/** @brief UE: end-marker control PDUs before QFI to DRB map update (TS 37.324 clause 5.3.1) */
static void nr_sdap_ue_qfi2drb_map_update(nr_sdap_entity_t *entity, const sdap_config_t *sdap)
{
  nr_sdap_ue_control_pdu_config(entity, entity->tun.ue_id, sdap);
  nr_sdap_qfi2drb_map_update(entity, sdap);
}

/**
 * @brief   add a new SDAP entity according to 5.1.1. of 3GPP TS 37.324
 * @note    there is one SDAP entity per PDU session
 *
 * @param   is_gnb, indicates whether it is for gNB or UE
 * @param   ue_id, UE ID
 * @param   sdap, SDAP configuration */
static void nr_sdap_add_entity(const int is_gnb, const ue_id_t ue_id, const sdap_config_t *sdap)
{
  nr_sdap_entity_t *sdap_entity = calloc_or_fail(1, sizeof(*sdap_entity));
  LOG_I(SDAP, "Creating SDAP entity ue_id=%ld pdu_session_id=%d\n", ue_id, sdap->pdusession_id);

  // SDAP entity ids
  sdap_entity->tun.ue_id = ue_id;
  sdap_entity->tun.pdusession_id = sdap->pdusession_id;
  sdap_entity->tun.is_gnb = is_gnb;

  // rx/tx entities
  sdap_entity->tx_entity = nr_sdap_tx_entity;
  sdap_entity->rx_entity = nr_sdap_rx_entity;

  // control pdu function pointers
  sdap_entity->sdap_construct_ctrl_pdu = nr_sdap_construct_ctrl_pdu;
  sdap_entity->sdap_map_ctrl_pdu = nr_sdap_map_ctrl_pdu;
  sdap_entity->sdap_submit_ctrl_pdu = nr_sdap_submit_ctrl_pdu;

  // QFI to DRB mapping function pointers
  sdap_entity->qfi2drb_map_update = is_gnb ? nr_sdap_qfi2drb_map_update : nr_sdap_ue_qfi2drb_map_update;
  sdap_entity->qfi2drb_map_add = nr_sdap_qfi2drb_map_add;
  sdap_entity->qfi2drb_map_delete = nr_sdap_qfi2drb_map_del;
  sdap_entity->qfi2drb_map = nr_sdap_qfi2drb;
  sdap_entity->tun.sock = -1;

  // Initialize QoS rules for UL packet filter matching at UE
  if (!is_gnb)
    nr_sdap_qos_rules_init(sdap_entity);

  // set default DRB
  if (sdap->defaultDRB) {
    sdap_entity->default_drb.drb_id = sdap->drb_id;
    sdap_entity->default_drb.entity_role = sdap->role;
    LOG_I(SDAP, "Default DRB for the created SDAP entity: DRB %d \n", sdap_entity->default_drb.drb_id);
  }

  if (sdap->role == NO_SDAP_HEADER) {
    AssertFatal(sdap->mappedQFIs2AddCount <= 1,
                "PDU session %d: both SDAP headers absent: at most one QoS flow at entity creation\n",
                sdap->pdusession_id);
  }

  // Add QoS flows to the DRB (initial configuration)
  nr_sdap_add_qos_flows_to_drb(sdap_entity, sdap);

  // update SDAP entity list pointers
  sdap_entity->next_entity = sdap_info.sdap_entity_llist;
  sdap_info.sdap_entity_llist = sdap_entity;

  if (IS_SOFTMODEM_NOS1 && is_gnb) {
    // In NOS1 mode, terminate SDAP for the first UE on the gNB. This allows injecting/receiving
    // PDCP SDUs to/from the TUN interface.
    start_sdap_tun_gnb_first_ue_default_pdu_session(ue_id, sdap_entity->tun.pdusession_id);
  }
}

/** @brief Add or modify an SDAP entity if it already exists */
void nr_sdap_addmod_entity(const int is_gnb, const ue_id_t ue_id, const sdap_config_t *sdap)
{
  nr_sdap_entity_t *sdap_entity = nr_sdap_get_entity(ue_id, sdap->pdusession_id);
  if (sdap_entity) {
    sdap_entity->qfi2drb_map_update(sdap_entity, sdap);
  } else {
    nr_sdap_add_entity(is_gnb, ue_id, sdap);
  }
}

/**
 * @brief   Fetches the SDAP entity for the give PDU session ID.
 * @note    There is one SDAP entity per PDU session.
 * @return  The pointer to the SDAP entity if existing, NULL otherwise
 */
nr_sdap_entity_t *nr_sdap_get_entity(ue_id_t ue_id, int pdusession_id)
{
  nr_sdap_entity_t *sdap_entity;
  sdap_entity = sdap_info.sdap_entity_llist;

  if (sdap_entity == NULL) {
    LOG_D(SDAP, "Could not find SDAP entity: entity list empty (ue_id=%ld pdu_session_id=%d)\n", ue_id, pdusession_id);
    return NULL;
  }

  while ((sdap_entity->tun.ue_id != ue_id || sdap_entity->tun.pdusession_id != pdusession_id) && sdap_entity->next_entity != NULL) {
    sdap_entity = sdap_entity->next_entity;
  }

  if (sdap_entity->tun.ue_id == ue_id && sdap_entity->tun.pdusession_id == pdusession_id)
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
    LOG_E(SDAP, "Couldn't find a SDAP entity associated with PDU session ID %d\n", pdusession_id);
}

/** @brief Set QoS-flow-to-DRB mapping for a given DRB from the E1 QoS Flow list.
 * On E1 Bearer Context Modification (CP to UP), Flow Mapping Information in DRB To Modify
 * carries the QoS Flow QoS Parameters List for that DRB. Per TS 38.463, when present the
 * CU-UP replaces the previous mapping for that DRB.
 * @param sdap mapped QoS flows to add */
void nr_sdap_entity_update_qos_flows(ue_id_t ue_id, sdap_config_t *sdap)
{
  DevAssert(sdap);
  nr_sdap_entity_t *entity = nr_sdap_get_entity(ue_id, sdap->pdusession_id);
  if (!entity) {
    LOG_W(SDAP, "No entity when updating QoS flows for DRB %d (pdu_session=%d)\n", sdap->drb_id, sdap->pdusession_id);
    return;
  }

  const qfi2drb_t *drb_map = nr_sdap_drb_lookup(entity, sdap->drb_id);
  if (!drb_map) {
    LOG_E(SDAP, "DRB %d: no qfi2drb_table entry, skip QoS-flow mapping update (pdu_session=%d)\n", sdap->drb_id, sdap->pdusession_id);
    return;
  }
  /* TS 38.331: sdap-HeaderUL/DL cannot change after DRB establishment */
  sdap->role = drb_map->entity_role;
  sdap->defaultDRB = (entity->default_drb.drb_id == sdap->drb_id);

  /* TS 37.324 clause 5.3.1 mappedQoS-FlowsToRelease: QFIs on this DRB not in the new list */
  for (int q = 0; q < SDAP_MAX_QFI; q++) {
    if (entity->qfi2drb_table[q].drb_id != sdap->drb_id)
      continue;
    bool keep = false;
    for (int i = 0; i < sdap->mappedQFIs2AddCount; i++) {
      if (sdap->mappedQFIs2Add[i] == q) {
        keep = true;
        break;
      }
    }
    if (!keep) {
      DevAssert(sdap->mappedQFIs2ReleaseCount < SDAP_MAX_QFI);
      sdap->mappedQFIs2Release[sdap->mappedQFIs2ReleaseCount++] = q;
      LOG_I(SDAP, "Update: release QFI %d from DRB %d (pdu_session=%d)\n", q, sdap->drb_id, sdap->pdusession_id);
    }
  }

  LOG_I(SDAP, "Update: add %d QFIs to DRB %d (pdu_session=%d)\n", sdap->mappedQFIs2AddCount, sdap->drb_id, sdap->pdusession_id);
  /* TS 37.324 clause 5.3.1: mappedQoS-FlowsToAdd and apply mapping on the DRB */
  entity->qfi2drb_map_update(entity, sdap);
}

bool nr_sdap_delete_entity(ue_id_t ue_id, int pdusession_id)
{
  if (pdusession_id < 0 || pdusession_id > NR_MAX_NB_PDU_SESSIONS) {
    LOG_E(SDAP, "Invalid range of pdusession_id [0, %d].\n", NR_MAX_NB_PDU_SESSIONS);
    return false;
  }

  for (nr_sdap_entity_t **pp = &sdap_info.sdap_entity_llist; *pp != NULL; pp = &(*pp)->next_entity) {
    nr_sdap_entity_t *entity = *pp;
    if (entity->tun.ue_id != ue_id || entity->tun.pdusession_id != pdusession_id)
      continue;
    *pp = entity->next_entity;
    remove_ip_if(entity);
    free(entity);
    LOG_D(SDAP, "Successfully deleted SDAP entity for UE %lx and PDU Session id %d\n", ue_id, pdusession_id);
    return true;
  }

  LOG_E(SDAP, "Entity does not exist or it was not found.\n");
  return false;
}

bool nr_sdap_delete_ue_entities(ue_id_t ue_id)
{
  bool ret = false;

  if (ue_id > SDAP_MAX_UE_ID) {
    LOG_W(SDAP, "Invalid range of ue_id [0, %d]\n", SDAP_MAX_UE_ID);
    return false;
  }

  for (nr_sdap_entity_t **pp = &sdap_info.sdap_entity_llist; *pp != NULL;) {
    nr_sdap_entity_t *entity = *pp;
    if (entity->tun.ue_id != ue_id) {
      pp = &(*pp)->next_entity;
      continue;
    }
    *pp = entity->next_entity;
    remove_ip_if(entity);
    free(entity);
    LOG_I(SDAP, "Successfully deleted SDAP entity for UE %ld\n", ue_id);
    ret = true;
  }
  return ret;
}

/** @brief This function gets the relevant SDAP config from the received SDAP-Config */
sdap_config_t nr_sdap_get_config(const int is_gnb, const NR_SDAP_Config_t *sdap_Config, const int drb_id)
{
  DevAssert(sdap_Config);
  sdap_config_t sdapConfig = {0};
  sdapConfig.drb_id = drb_id;
  sdapConfig.role = get_sdap_role(is_gnb, sdap_Config);
  sdapConfig.defaultDRB = sdap_Config->defaultDRB;
  // 3GPP TS 38.331 The network sets sdap-HeaderUL to present if the field defaultDRB is set to true
  if (sdapConfig.defaultDRB && (sdap_Config->sdap_HeaderUL != NR_SDAP_Config__sdap_HeaderUL_present))
    LOG_D(SDAP, "Received SDAP-Config with defaultDRB but sdap-HeaderUL is not present\n");
  if (sdap_Config->mappedQoS_FlowsToAdd) {
    sdapConfig.mappedQFIs2AddCount = sdap_Config->mappedQoS_FlowsToAdd->list.count;
    DevAssert(sdapConfig.mappedQFIs2AddCount <= SDAP_MAX_QFI);
    LOG_D(SDAP, "DRB %d: mapped QFIs = %d  \n", sdapConfig.drb_id, sdapConfig.mappedQFIs2AddCount);
    for (int i = 0; i < sdapConfig.mappedQFIs2AddCount; i++){
      DevAssert(i < SDAP_MAX_QFI);
      sdapConfig.mappedQFIs2Add[i] = *sdap_Config->mappedQoS_FlowsToAdd->list.array[i];
      LOG_D(SDAP, "Captured mappedQoS_FlowsToAdd[%d] from RRC: %ld\n", i, sdapConfig.mappedQFIs2Add[i]);
    }
  }
  sdapConfig.pdusession_id = sdap_Config->pdu_Session;
  if (sdap_Config->mappedQoS_FlowsToRelease) {
    sdapConfig.mappedQFIs2ReleaseCount = sdap_Config->mappedQoS_FlowsToRelease->list.count;
    DevAssert(sdapConfig.mappedQFIs2ReleaseCount <= SDAP_MAX_QFI);
    for (int i = 0; i < sdapConfig.mappedQFIs2ReleaseCount; i++) {
      DevAssert(i < SDAP_MAX_QFI);
      sdapConfig.mappedQFIs2Release[i] = *sdap_Config->mappedQoS_FlowsToRelease->list.array[i];
    }
  }
  return sdapConfig;
}

/**
 * @brief SDAP Entity reconfiguration at UE according to TS 37.324
 *        and triggered by RRC reconfiguration events according to clause 5.3.5.6.5 of TS 38.331.
 *        This function performs:
 *        - QoS flow to DRB mapping according to clause 5.3.1 of TS 37.324
 */
void nr_reconfigure_sdap_entity(NR_SDAP_Config_t *sdap_config, ue_id_t ue_id, int pdusession_id, int drb_id)
{
  bool is_gnb = false;
  /* fetch SDAP entity */
  nr_sdap_entity_t *sdap_entity = nr_sdap_get_entity(ue_id, pdusession_id);
  AssertError(sdap_entity != NULL,
              return,
              "Could not find SDAP Entity for RNTI/UE ID: %lu and PDU SESSION ID: %d\n",
              ue_id,
              pdusession_id);
  /* QFI to DRB mapping */
  sdap_config_t sdap = nr_sdap_get_config(is_gnb, sdap_config, drb_id);
  sdap_entity->qfi2drb_map_update(sdap_entity, &sdap);
}
