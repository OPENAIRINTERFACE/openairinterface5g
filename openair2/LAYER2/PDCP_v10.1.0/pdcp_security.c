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

/*! \file pdcp_security.c
 * \brief PDCP Security Methods
 * \author ROUX Sebastie and Navid Nikaein
 * \email openair_tech@eurecom.fr, navid.nikaein@eurecom.fr
 * \date 2014
 */
#include <stdint.h>

#include "assertions.h"

#include "UTIL/LOG/log.h"
#include "UTIL/OSA/osa_defs.h"

#include "UTIL/LOG/vcd_signal_dumper.h"

#include "LAYER2/MAC/extern.h"

#include "pdcp.h"
#include "msc.h"
#include "pdcp_primitives.h"

#if defined(ENABLE_SECURITY)

static
uint32_t pdcp_get_next_count_tx(pdcp_t *const pdcp_pP, const srb_flag_t srb_flagP, const uint16_t pdcp_sn);
static
uint32_t pdcp_get_next_count_rx(pdcp_t *const pdcp_pP, const srb_flag_t srb_flagP, const uint16_t pdcp_sn);

//-----------------------------------------------------------------------------
static
uint32_t pdcp_get_next_count_tx(
  pdcp_t * const pdcp_pP,
  const srb_flag_t srb_flagP,
  const uint16_t pdcp_sn
)
{
  uint32_t count;

  /* For TX COUNT = TX_HFN << length of SN | pdcp SN */
  if (srb_flagP) {
    /* 5 bits length SN */
    count = ((pdcp_pP->tx_hfn << 5)  | (pdcp_sn & 0x001F));
  } else {
    if (pdcp_pP->seq_num_size == PDCP_Config__rlc_UM__pdcp_SN_Size_len7bits) {
      count = ((pdcp_pP->tx_hfn << 7) | (pdcp_sn & 0x07F));
    } else { /*Default is the 12 bits length SN */
      count = ((pdcp_pP->tx_hfn << 12) | (pdcp_sn & 0x0FFF));
    }
  }

  LOG_D(PDCP, "[OSA] TX COUNT = 0x%08x\n", count);

  return count;
}

//-----------------------------------------------------------------------------
static
uint32_t pdcp_get_next_count_rx(
  pdcp_t * const pdcp_pP,
  const srb_flag_t srb_flagP,
  const uint16_t pdcp_sn)
{
  uint32_t count;

  /* For RX COUNT = RX_HFN << length of SN | pdcp SN of received PDU */
  if (srb_flagP) {
    /* 5 bits length SN */
    count = (((pdcp_pP->rx_hfn + pdcp_pP->rx_hfn_offset) << 5)  | (pdcp_sn & 0x001F));
  } else {
    if (pdcp_pP->seq_num_size == PDCP_Config__rlc_UM__pdcp_SN_Size_len7bits) {
      /* 7 bits length SN */
      count = (((pdcp_pP->rx_hfn + pdcp_pP->rx_hfn_offset) << 7) | (pdcp_sn & 0x007F));
    } else { // default
      /* 12 bits length SN */
      count = (((pdcp_pP->rx_hfn + pdcp_pP->rx_hfn_offset) << 12) | (pdcp_sn & 0x0FFF));
    }
  }

  // reset the hfn offset
  pdcp_pP->rx_hfn_offset =0;
  LOG_D(PDCP, "[OSA] RX COUNT = 0x%08x\n", count);

  return count;
}


//-----------------------------------------------------------------------------
int
pdcp_apply_security(
  const protocol_ctxt_t* const ctxt_pP,
  pdcp_t        *const pdcp_pP,
  const srb_flag_t     srb_flagP,
  const rb_id_t        rb_id,
  const uint8_t        pdcp_header_len,
  const uint16_t       current_sn,
  uint8_t       * const pdcp_pdu_buffer,
  const uint16_t      sdu_buffer_size
)
{
  uint8_t *buffer_encrypted = NULL;
  stream_cipher_t encrypt_params;

  DevAssert(pdcp_pP != NULL);
  DevAssert(pdcp_pdu_buffer != NULL);
  DevCheck(rb_id < NB_RB_MAX && rb_id >= 0, rb_id, NB_RB_MAX, 0);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_APPLY_SECURITY, VCD_FUNCTION_IN);

  encrypt_params.direction  = (pdcp_pP->is_ue == 1) ? SECU_DIRECTION_UPLINK : SECU_DIRECTION_DOWNLINK;
  encrypt_params.bearer     = rb_id - 1;
  encrypt_params.count      = pdcp_get_next_count_tx(pdcp_pP, srb_flagP, current_sn);
  encrypt_params.key_length = 16;

  if (srb_flagP) {
    /* SRBs */
    uint8_t *mac_i;

    LOG_D(PDCP, "[OSA][RB %d] %s Applying control-plane security %d \n",
          rb_id, (pdcp_pP->is_ue != 0) ? "UE -> eNB" : "eNB -> UE", pdcp_pP->integrityProtAlgorithm);

    encrypt_params.message    = pdcp_pdu_buffer;
    encrypt_params.blength    = (pdcp_header_len + sdu_buffer_size) << 3;
    encrypt_params.key        = pdcp_pP->kRRCint + 16; // + 128;

    mac_i = &pdcp_pdu_buffer[pdcp_header_len + sdu_buffer_size];

    /* Both header and data parts are integrity protected for
     * control-plane PDUs */
    stream_compute_integrity(pdcp_pP->integrityProtAlgorithm,
                             &encrypt_params,
                             mac_i);

    encrypt_params.key = pdcp_pP->kRRCenc;  // + 128  // bit key
  } else {
    LOG_D(PDCP, "[OSA][RB %d] %s Applying user-plane security\n",
          rb_id, (pdcp_pP->is_ue != 0) ? "UE -> eNB" : "eNB -> UE");

    encrypt_params.key = pdcp_pP->kUPenc;//  + 128;
  }

  encrypt_params.message    = &pdcp_pdu_buffer[pdcp_header_len];
  encrypt_params.blength    = sdu_buffer_size << 3;

  buffer_encrypted = &pdcp_pdu_buffer[pdcp_header_len];

  /* Apply ciphering if any requested */
  stream_encrypt(pdcp_pP->cipheringAlgorithm,
                 &encrypt_params,
                 &buffer_encrypted);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_APPLY_SECURITY, VCD_FUNCTION_OUT);

  return 0;
}

//-----------------------------------------------------------------------------
int
pdcp_validate_security(
  const protocol_ctxt_t* const ctxt_pP,
  pdcp_t         * const pdcp_pP,
  const srb_flag_t     srb_flagP,
  const rb_id_t        rb_id,
  const uint8_t        pdcp_header_len,
  const uint16_t       current_sn,
  uint8_t       *const pdcp_pdu_buffer,
  const uint16_t       sdu_buffer_size
)
{
  uint8_t *buffer_decrypted = NULL;
  stream_cipher_t decrypt_params;

  DevAssert(pdcp_pP != NULL);

  DevAssert(pdcp_pdu_buffer != NULL);
  DevCheck(rb_id < NB_RB_MAX && rb_id >= 0, rb_id, NB_RB_MAX, 0);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_VALIDATE_SECURITY, VCD_FUNCTION_IN);

  buffer_decrypted = (uint8_t*)&pdcp_pdu_buffer[pdcp_header_len];

  decrypt_params.direction  = (pdcp_pP->is_ue == 1) ? SECU_DIRECTION_DOWNLINK : SECU_DIRECTION_UPLINK ;
  decrypt_params.bearer     = rb_id - 1;
  decrypt_params.count      = pdcp_get_next_count_rx(pdcp_pP, srb_flagP, current_sn);
  decrypt_params.message    = &pdcp_pdu_buffer[pdcp_header_len];
  decrypt_params.blength    = (sdu_buffer_size - pdcp_header_len) << 3;
  decrypt_params.key_length = 16;

  if (srb_flagP) {
    LOG_D(PDCP, "[OSA][RB %d] %s Validating control-plane security\n",
          rb_id, (pdcp_pP->is_ue != 0) ? "eNB -> UE" : "UE -> eNB");
    decrypt_params.key = pdcp_pP->kRRCenc;// + 128;
  } else {
    LOG_D(PDCP, "[OSA][RB %d] %s Validating user-plane security\n",
          rb_id, (pdcp_pP->is_ue != 0) ? "eNB -> UE" : "UE -> eNB");
    decrypt_params.key = pdcp_pP->kUPenc;// + 128;
  }

  /* Uncipher the block */
  stream_decrypt(pdcp_pP->cipheringAlgorithm,
                 &decrypt_params,
                 &buffer_decrypted);
#if !defined(USRP_REC_PLAY)
  if (srb_flagP) {
    /* Now check the integrity of the complete PDU */
    decrypt_params.message    = pdcp_pdu_buffer;
    decrypt_params.blength    = sdu_buffer_size << 3;
    decrypt_params.key        = pdcp_pP->kRRCint + 16;// 128;

    if (stream_check_integrity(pdcp_pP->integrityProtAlgorithm,
                               &decrypt_params,
                               &pdcp_pdu_buffer[sdu_buffer_size]) != 0) {
      MSC_LOG_EVENT(
    	  (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
    	  " Security: failed MAC-I Algo %X UE %"PRIx16" ",
    	  pdcp_pP->integrityProtAlgorithm,
    	  ctxt_pP->rnti);
      LOG_E(PDCP, "[OSA][RB %d] %s failed to validate MAC-I of incoming PDU\n",
            rb_id, (pdcp_pP->is_ue != 0) ? "UE" : "eNB");
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_VALIDATE_SECURITY, VCD_FUNCTION_OUT);
      return -1;
    }
  }
#endif
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_VALIDATE_SECURITY, VCD_FUNCTION_OUT);

  return 0;
}

#endif /* ENABLE_SECURITY */
