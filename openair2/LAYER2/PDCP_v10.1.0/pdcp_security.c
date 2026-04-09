/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief PDCP Security Methods
 */
#include <stdint.h>

#include "assertions.h"

#include "common/utils/LOG/log.h"

#include "common/utils/LOG/vcd_signal_dumper.h"
#include "executables/softmodem-common.h"
#include "LAYER2/MAC/mac_extern.h"

#include "pdcp.h"
#include "pdcp_primitives.h"

#include "openair3/SECU/secu_defs.h"

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
    if (pdcp_pP->seq_num_size == LTE_PDCP_Config__rlc_UM__pdcp_SN_Size_len7bits) {
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
  const uint32_t hfn,
  const int sn)
{
  uint32_t count;

  /* For RX COUNT = RX_HFN << length of SN | pdcp SN of received PDU */
  if (srb_flagP) {
    /* 5 bits length SN */
    count = (hfn << 5)  | (sn & 0x001F);
  } else {
    if (pdcp_pP->seq_num_size == 7) {
      /* 7 bits length SN */
      count = (hfn << 7) | (sn & 0x007F);
    } else { // default
      /* 12 bits length SN */
      count = (hfn << 12) | (sn & 0x0FFF);
    }
  }

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
  nas_stream_cipher_t encrypt_params = {0};
  stream_security_container_t *container;

  DevAssert(pdcp_pP != NULL);
  DevAssert(pdcp_pdu_buffer != NULL);
  DevCheck(rb_id < NB_RB_MAX && rb_id >= 0, rb_id, NB_RB_MAX, 0);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_APPLY_SECURITY, VCD_FUNCTION_IN);

  encrypt_params.direction  = (pdcp_pP->is_ue == 1) ? SECU_DIRECTION_UPLINK : SECU_DIRECTION_DOWNLINK;
  encrypt_params.bearer     = rb_id - 1;
  encrypt_params.count      = pdcp_get_next_count_tx(pdcp_pP, srb_flagP, current_sn);

  if (srb_flagP)
    container = pdcp_pP->security_container_rrc;
  else
    container = pdcp_pP->security_container_up;

  if (srb_flagP) {
    /* SRBs */
    uint8_t *mac_i;

    LOG_D(PDCP, "[OSA][RB %ld] %s Applying control-plane security %d \n",
          rb_id, (pdcp_pP->is_ue != 0) ? "UE -> eNB" : "eNB -> UE", pdcp_pP->integrityProtAlgorithm);

    encrypt_params.message    = pdcp_pdu_buffer;
    encrypt_params.blength    = (pdcp_header_len + sdu_buffer_size) << 3;
    encrypt_params.context    = container->integrity_context;

    mac_i = &pdcp_pdu_buffer[pdcp_header_len + sdu_buffer_size];

    /* Both header and data parts are integrity protected for
     * control-plane PDUs */
    stream_compute_integrity(pdcp_pP->integrityProtAlgorithm,
                             &encrypt_params,
                             mac_i);
  } else {
    LOG_D(PDCP, "[OSA][RB %ld] %s Applying user-plane security\n",
          rb_id, (pdcp_pP->is_ue != 0) ? "UE -> eNB" : "eNB -> UE");
  }

  encrypt_params.context = container->ciphering_context;

  encrypt_params.message    = &pdcp_pdu_buffer[pdcp_header_len];
  encrypt_params.blength    = sdu_buffer_size << 3;

  buffer_encrypted = &pdcp_pdu_buffer[pdcp_header_len];

  /* Apply ciphering if any requested */
  stream_compute_encrypt(pdcp_pP->cipheringAlgorithm, &encrypt_params, buffer_encrypted);

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
  const uint32_t       hfn,
  const int            sn,
  uint8_t       *const pdcp_pdu_buffer,
  const uint16_t       sdu_buffer_size
)
{
// Skip security checking for simulation
return 0;
  uint8_t *buffer_decrypted = NULL;
  nas_stream_cipher_t decrypt_params = {0};
  stream_security_container_t *container;

  DevAssert(pdcp_pP != NULL);

  DevAssert(pdcp_pdu_buffer != NULL);
  DevCheck(rb_id < NB_RB_MAX && rb_id >= 0, rb_id, NB_RB_MAX, 0);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_VALIDATE_SECURITY, VCD_FUNCTION_IN);

  buffer_decrypted = (uint8_t*)&pdcp_pdu_buffer[pdcp_header_len];

  decrypt_params.direction  = (pdcp_pP->is_ue == 1) ? SECU_DIRECTION_DOWNLINK : SECU_DIRECTION_UPLINK ;
  decrypt_params.bearer     = rb_id - 1;
  decrypt_params.count      = pdcp_get_next_count_rx(pdcp_pP, srb_flagP, hfn, sn);
  decrypt_params.message    = &pdcp_pdu_buffer[pdcp_header_len];
  decrypt_params.blength    = (sdu_buffer_size - pdcp_header_len) << 3;

  if (srb_flagP) {
    LOG_D(PDCP, "[OSA][RB %ld] %s Validating control-plane security\n", rb_id, (pdcp_pP->is_ue != 0) ? "eNB -> UE" : "UE -> eNB");
    container = pdcp_pP->security_container_rrc;
  } else {
    LOG_D(PDCP, "[OSA][RB %ld] %s Validating user-plane security\n", rb_id, (pdcp_pP->is_ue != 0) ? "eNB -> UE" : "UE -> eNB");
    container = pdcp_pP->security_container_up;
  }

  /* Uncipher the block */
  decrypt_params.context = container->ciphering_context;
  stream_compute_encrypt(pdcp_pP->cipheringAlgorithm, &decrypt_params, buffer_decrypted);

  if (!IS_SOFTMODEM_IQPLAYER) {
    if (srb_flagP) {
      /* Now check the integrity of the complete PDU */
      decrypt_params.message    = pdcp_pdu_buffer;
      decrypt_params.blength    = sdu_buffer_size << 3;
      decrypt_params.context    = container->integrity_context;

      if (pdcp_pP->integrityProtAlgorithm != EIA0_ALG_ID) {
        uint8_t result[4] = {0};
        stream_compute_integrity(pdcp_pP->integrityProtAlgorithm, &decrypt_params, result);

        if (memcmp(result, &pdcp_pdu_buffer[sdu_buffer_size], 4) != 0) {
          LOG_E(PDCP,
                "[OSA][RB %ld] %s failed to validate MAC-I of incoming PDU\n",
                rb_id,
                (pdcp_pP->is_ue != 0) ? "UE" : "eNB");
          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_VALIDATE_SECURITY, VCD_FUNCTION_OUT);
          return -1;
        }
      }
    }
  } /* !IQPLAYER */
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_VALIDATE_SECURITY, VCD_FUNCTION_OUT);

  return 0;
}
