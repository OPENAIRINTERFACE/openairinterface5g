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

/*! \file f1ap_du_interface_management.h
 * \brief f1ap interface management for DU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */
#include "f1ap_common.h"
#include "f1ap_du_paging.h"
#include "conversions.h"
#include "asn1_conversions.h"
#include "openair2/RRC/LTE/rrc_proto.h"

int DU_handle_Paging(instance_t       instance,
                     uint32_t         assoc_id,
                     uint32_t         stream,
                     F1AP_F1AP_PDU_t *pdu) {
  F1AP_Paging_t *paging;
  F1AP_PagingIEs_t *ie;
  long tmsi;
  uint8_t pagingdrx;
  uint8_t  *fiveg_tmsi_buf = NULL;

  DevAssert(pdu);

  if ( LOG_DEBUGFLAG(DEBUG_ASN1) ) {
    xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, pdu);
  }

  paging = &pdu->choice.initiatingMessage->value.choice.Paging;
  // get UEIdentityIndexValue
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_PagingIEs_t, ie, paging,
                             F1AP_ProtocolIE_ID_id_UEIdentityIndexValue, true);

  LOG_D(F1AP, "indexLength10 %d\n", BIT_STRING_to_uint32(&ie->value.choice.UEIdentityIndexValue.choice.indexLength10));

  // get PagingIdentity
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_PagingIEs_t, ie, paging,
                             F1AP_ProtocolIE_ID_id_PagingIdentity, true);

  if(ie != NULL){
    fiveg_tmsi_buf = ie->value.choice.PagingIdentity.choice.cNUEPagingIdentity->choice.fiveG_S_TMSI.buf;

    tmsi = ((long)fiveg_tmsi_buf[0] << 40) +
           ((long)fiveg_tmsi_buf[1] << 32) +
           (fiveg_tmsi_buf[2] << 24) +
           (fiveg_tmsi_buf[3] << 16) +
           (fiveg_tmsi_buf[4] << 8) +
           fiveg_tmsi_buf[5];

    LOG_D(F1AP, "tmsi %ld\n", tmsi);
  }

  // get PagingDRX
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_PagingIEs_t, ie, paging,
                             F1AP_ProtocolIE_ID_id_PagingDRX, true);

  if(ie != NULL) {
    pagingdrx = (uint8_t)ie->value.choice.PagingDRX;
  }

  LOG_D(F1AP, "pagingdrx %u\n", pagingdrx);

  // get PagingCell_List
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_PagingIEs_t, ie, paging,
                             F1AP_ProtocolIE_ID_id_PagingCell_List, true);

  for (uint8_t CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    rrc_gNB_generate_pcch_msg((uint32_t)tmsi, pagingdrx, instance, CC_id);
  }

  return 0;
}