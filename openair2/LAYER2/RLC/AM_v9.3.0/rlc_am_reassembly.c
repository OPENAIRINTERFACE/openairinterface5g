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

#define RLC_AM_MODULE 1
#define RLC_AM_REASSEMBLY_C 1
#include "platform_types.h"
//-----------------------------------------------------------------------------

#include "assertions.h"
#include "rlc.h"
#include "rlc_am.h"
#include "list.h"
//#include "LAYER2/MAC/extern.h"
#include "common/utils/LOG/log.h"
#include "msc.h"

//-----------------------------------------------------------------------------
inline void
rlc_am_clear_rx_sdu (
  const protocol_ctxt_t *const ctxt_pP,
  rlc_am_entity_t *const      rlc_pP) {
  rlc_pP->output_sdu_size_to_write = 0;
}
//-----------------------------------------------------------------------------
void
rlc_am_reassembly (
  const protocol_ctxt_t *const ctxt_pP,
  rlc_am_entity_t *const rlc_pP,
  uint8_t *src_pP,
  const int32_t lengthP) {
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PAYLOAD] reassembly()  %d bytes\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        lengthP);

  if (rlc_pP->output_sdu_in_construction == NULL) {
    rlc_pP->output_sdu_in_construction = get_free_mem_block (RLC_SDU_MAX_SIZE, __func__);
    rlc_pP->output_sdu_size_to_write = 0;

    //assert(rlc_pP->output_sdu_in_construction != NULL);
    if(rlc_pP->output_sdu_in_construction == NULL) {
      LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PAYLOAD] output_sdu_in_construction is NULL\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
      return;
    }
  }

  if (rlc_pP->output_sdu_in_construction != NULL) {
    // check if no overflow in size
    if ((rlc_pP->output_sdu_size_to_write + lengthP) <= RLC_SDU_MAX_SIZE) {
      memcpy (&rlc_pP->output_sdu_in_construction->data[rlc_pP->output_sdu_size_to_write], src_pP, lengthP);
      rlc_pP->output_sdu_size_to_write += lengthP;
    } else {
      LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PAYLOAD] ERROR  SDU SIZE OVERFLOW SDU GARBAGED\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
#if STOP_ON_IP_TRAFFIC_OVERLOAD
      AssertFatal(0, PROTOCOL_RLC_AM_CTXT_FMT" RLC_AM_DATA_IND, SDU SIZE OVERFLOW SDU GARBAGED\n",
                  PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
#endif
      // erase  SDU
      rlc_pP->output_sdu_size_to_write = 0;
    }
  } else {
    LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PAYLOAD] ERROR  OUTPUT SDU IS NULL\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
#if STOP_ON_IP_TRAFFIC_OVERLOAD
    AssertFatal(0, PROTOCOL_RLC_AM_CTXT_FMT" RLC_AM_DATA_IND, SDU DROPPED, OUT OF MEMORY\n",
                PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
#endif
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_send_sdu (
  const protocol_ctxt_t *const ctxt_pP,
  rlc_am_entity_t *const      rlc_pP) {
  if ((rlc_pP->output_sdu_in_construction)) {
    LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND_SDU] %d bytes sdu %p\n",
          PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
          rlc_pP->output_sdu_size_to_write,
          rlc_pP->output_sdu_in_construction);

    if (rlc_pP->output_sdu_size_to_write > 0) {
      rlc_pP->stat_rx_pdcp_sdu   += 1;
      rlc_pP->stat_rx_pdcp_bytes += rlc_pP->output_sdu_size_to_write;
#if TEST_RLC_AM
      rlc_am_v9_3_0_test_data_ind (rlc_pP->module_id,
                                   rlc_pP->rb_id,
                                   rlc_pP->output_sdu_size_to_write,
                                   rlc_pP->output_sdu_in_construction);
#else

      if ( LOG_DEBUGFLAG(DEBUG_RLC)) {
        char                 message_string[7000];
        size_t               message_string_size = 0;
        int                  octet_index, index;
        message_string_size += sprintf(&message_string[message_string_size], "Bearer	  : %u\n", rlc_pP->rb_id);
        message_string_size += sprintf(&message_string[message_string_size], "SDU size    : %u\n", rlc_pP->output_sdu_size_to_write);
        message_string_size += sprintf(&message_string[message_string_size], "\nPayload  : \n");
        message_string_size += sprintf(&message_string[message_string_size], "------+-------------------------------------------------|\n");
        message_string_size += sprintf(&message_string[message_string_size], "      |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n");
        message_string_size += sprintf(&message_string[message_string_size], "------+-------------------------------------------------|\n");

        for (octet_index = 0; octet_index < rlc_pP->output_sdu_size_to_write; octet_index++) {
          if ((octet_index % 16) == 0) {
            if (octet_index != 0) {
              message_string_size += sprintf(&message_string[message_string_size], " |\n");
            }

            message_string_size += sprintf(&message_string[message_string_size], " %04d |", octet_index);
          }

          /*
           * Print every single octet in hexadecimal form
           */
          message_string_size += sprintf(&message_string[message_string_size],
                                         " %02x",
                                         rlc_pP->output_sdu_in_construction->data[octet_index]);
          /*
           * Align newline and pipes according to the octets in groups of 2
           */
        }

        /*
         * Append enough spaces and put final pipe
         */
        for (index = octet_index; index < 16; ++index) {
          message_string_size += sprintf(&message_string[message_string_size], "   ");
        }

        message_string_size += sprintf(&message_string[message_string_size], " |\n");
        LOG_T(RLC, "%s", message_string);
      }

#if !ENABLE_ITTI
      RLC_AM_MUTEX_UNLOCK(&rlc_pP->lock_input_sdus);
#endif
      MSC_LOG_TX_MESSAGE(
        (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_RLC_ENB:MSC_RLC_UE,
        (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
        (const char *)(rlc_pP->output_sdu_in_construction->data),
        rlc_pP->output_sdu_size_to_write,
        MSC_AS_TIME_FMT" "PROTOCOL_RLC_AM_MSC_FMT" DATA-IND size %u",
        MSC_AS_TIME_ARGS(ctxt_pP),
        PROTOCOL_RLC_AM_MSC_ARGS(ctxt_pP,rlc_pP),
        rlc_pP->output_sdu_size_to_write
      );
      rlc_data_ind (ctxt_pP,
                    BOOL_NOT(rlc_pP->is_data_plane),
                    MBMS_FLAG_NO,
                    rlc_pP->rb_id,
                    rlc_pP->output_sdu_size_to_write,
                    rlc_pP->output_sdu_in_construction);
#if !ENABLE_ITTI
      RLC_AM_MUTEX_LOCK(&rlc_pP->lock_input_sdus, ctxt_pP, rlc_pP);
#endif
#endif
      rlc_pP->output_sdu_in_construction = NULL;
    } else {
      LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[SEND_SDU] ERROR SIZE <= 0 ... DO NOTHING, SET SDU SIZE TO 0\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
      //msg("[RLC_AM][MOD %d] Freeing mem_block ...\n", rlc_pP->module_id);
      //free_mem_block (rlc_pP->output_sdu_in_construction, __func__);
      //Assertion(eNB)_PRAN_DesignDocument_annex No.764
      LOG_E(RLC, PROTOCOL_RLC_AM_CTXT_FMT" SEND SDU REQUESTED %d bytes\n",
            PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
            rlc_pP->output_sdu_size_to_write);
      /*
            AssertFatal(3==4,
                        PROTOCOL_RLC_AM_CTXT_FMT" SEND SDU REQUESTED %d bytes",
                        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
                        rlc_pP->output_sdu_size_to_write);
      */
    }

    rlc_pP->output_sdu_size_to_write = 0;
  }
}
//-----------------------------------------------------------------------------
void
rlc_am_reassemble_pdu(
  const protocol_ctxt_t *const ctxt_pP,
  rlc_am_entity_t *const      rlc_pP,
  mem_block_t *const          tb_pP,
  boolean_t free_rlc_pdu) {
  int i,j;
  rlc_am_pdu_info_t *pdu_info        = &((rlc_am_rx_pdu_management_t *)(tb_pP->data))->pdu_info;
  LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PDU] TRY REASSEMBLY PDU SN=%03d\n",
        PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP),
        pdu_info->sn);

  if ( LOG_DEBUGFLAG(DEBUG_RLC)) {
    rlc_am_display_data_pdu_infos(ctxt_pP, rlc_pP, pdu_info);
  }

  if (pdu_info->e == RLC_E_FIXED_PART_DATA_FIELD_FOLLOW) {
    switch (pdu_info->fi) {
      case RLC_FI_1ST_BYTE_DATA_IS_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_LAST_BYTE_SDU:
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PDU] TRY REASSEMBLY PDU NO E_LI FI=11 (00)\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
        // one complete SDU
        rlc_am_send_sdu(ctxt_pP, rlc_pP); // may be not necessary
        rlc_am_reassembly (ctxt_pP, rlc_pP, pdu_info->payload, pdu_info->payload_size);
        rlc_am_send_sdu(ctxt_pP, rlc_pP); // may be not necessary
        //rlc_pP->reassembly_missing_sn_detected = 0;
        break;

      case RLC_FI_1ST_BYTE_DATA_IS_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_NOT_LAST_BYTE_SDU:
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PDU] TRY REASSEMBLY PDU NO E_LI FI=10 (01)\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
        // one beginning segment of SDU in PDU
        rlc_am_send_sdu(ctxt_pP, rlc_pP); // may be not necessary
        rlc_am_reassembly (ctxt_pP, rlc_pP,pdu_info->payload, pdu_info->payload_size);
        //rlc_pP->reassembly_missing_sn_detected = 0;
        break;

      case RLC_FI_1ST_BYTE_DATA_IS_NOT_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_LAST_BYTE_SDU:
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PDU] TRY REASSEMBLY PDU NO E_LI FI=01 (10)\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
        // one last segment of SDU
        //if (rlc_pP->reassembly_missing_sn_detected == 0) {
        rlc_am_reassembly (ctxt_pP, rlc_pP, pdu_info->payload, pdu_info->payload_size);
        rlc_am_send_sdu(ctxt_pP, rlc_pP);
        //} // else { clear sdu already done
        //rlc_pP->reassembly_missing_sn_detected = 0;
        break;

      case RLC_FI_1ST_BYTE_DATA_IS_NOT_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_NOT_LAST_BYTE_SDU:
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PDU] TRY REASSEMBLY PDU NO E_LI FI=00 (11)\n",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));
        //if (rlc_pP->reassembly_missing_sn_detected == 0) {
        // one whole segment of SDU in PDU
        rlc_am_reassembly (ctxt_pP, rlc_pP, pdu_info->payload, pdu_info->payload_size);
        //} else {
        //    rlc_pP->reassembly_missing_sn_detected = 1; // not necessary but for readability of the code
        //}
        break;

      default:
        //Assertion(eNB)_PRAN_DesignDocument_annex No.1428
        LOG_E(RLC, "RLC_E_FIXED_PART_DATA_FIELD_FOLLOW error pdu_info->fi[%d]\n", pdu_info->fi);
        //      assert(0 != 0);
    }
  } else {
    switch (pdu_info->fi) {
      case RLC_FI_1ST_BYTE_DATA_IS_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_LAST_BYTE_SDU:
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PDU] TRY REASSEMBLY PDU FI=11 (00) Li=",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));

        for (i=0; i < pdu_info->num_li; i++) {
          LOG_D(RLC, "%d ",pdu_info->li_list[i]);
        }

        LOG_D(RLC, "\n");
        //msg(" remaining size %d\n",size);
        // N complete SDUs
        rlc_am_send_sdu(ctxt_pP, rlc_pP);
        j = 0;

        for (i = 0; i < pdu_info->num_li; i++) {
          rlc_am_reassembly (ctxt_pP, rlc_pP, &pdu_info->payload[j], pdu_info->li_list[i]);
          rlc_am_send_sdu(ctxt_pP, rlc_pP);
          j = j + pdu_info->li_list[i];
        }

        if (pdu_info->hidden_size > 0) { // normally should always be > 0 but just for help debug
          // data is already ok, done by last loop above
          rlc_am_reassembly (ctxt_pP, rlc_pP, &pdu_info->payload[j], pdu_info->hidden_size);
          rlc_am_send_sdu(ctxt_pP, rlc_pP);
        }

        //rlc_pP->reassembly_missing_sn_detected = 0;
        break;

      case RLC_FI_1ST_BYTE_DATA_IS_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_NOT_LAST_BYTE_SDU:
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PDU] TRY REASSEMBLY PDU FI=10 (01) Li=",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));

        for (i=0; i < pdu_info->num_li; i++) {
          LOG_D(RLC, "%d ",pdu_info->li_list[i]);
        }

        LOG_D(RLC, "\n");
        //msg(" remaining size %d\n",size);
        // N complete SDUs + one segment of SDU in PDU
        rlc_am_send_sdu(ctxt_pP, rlc_pP);
        j = 0;

        for (i = 0; i < pdu_info->num_li; i++) {
          rlc_am_reassembly (ctxt_pP, rlc_pP, &pdu_info->payload[j], pdu_info->li_list[i]);
          rlc_am_send_sdu(ctxt_pP, rlc_pP);
          j = j + pdu_info->li_list[i];
        }

        if (pdu_info->hidden_size > 0) { // normally should always be > 0 but just for help debug
          // data is already ok, done by last loop above
          rlc_am_reassembly (ctxt_pP, rlc_pP, &pdu_info->payload[j], pdu_info->hidden_size);
        }

        //rlc_pP->reassembly_missing_sn_detected = 0;
        break;

      case RLC_FI_1ST_BYTE_DATA_IS_NOT_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_LAST_BYTE_SDU:
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PDU] TRY REASSEMBLY PDU FI=01 (10) Li=",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));

        for (i=0; i < pdu_info->num_li; i++) {
          LOG_D(RLC, "%d ",pdu_info->li_list[i]);
        }

        LOG_D(RLC, "\n");
        //msg(" remaining size %d\n",size);
        // one last segment of SDU + N complete SDUs in PDU
        j = 0;

        for (i = 0; i < pdu_info->num_li; i++) {
          rlc_am_reassembly (ctxt_pP, rlc_pP, &pdu_info->payload[j], pdu_info->li_list[i]);
          rlc_am_send_sdu(ctxt_pP, rlc_pP);
          j = j + pdu_info->li_list[i];
        }

        if (pdu_info->hidden_size > 0) { // normally should always be > 0 but just for help debug
          // data is already ok, done by last loop above
          rlc_am_reassembly (ctxt_pP, rlc_pP, &pdu_info->payload[j], pdu_info->hidden_size);
          rlc_am_send_sdu(ctxt_pP, rlc_pP);
        }

        //rlc_pP->reassembly_missing_sn_detected = 0;
        break;

      case RLC_FI_1ST_BYTE_DATA_IS_NOT_1ST_BYTE_SDU_LAST_BYTE_DATA_IS_NOT_LAST_BYTE_SDU:
        LOG_D(RLC, PROTOCOL_RLC_AM_CTXT_FMT"[REASSEMBLY PDU] TRY REASSEMBLY PDU FI=00 (11) Li=",
              PROTOCOL_RLC_AM_CTXT_ARGS(ctxt_pP,rlc_pP));

        for (i=0; i < pdu_info->num_li; i++) {
          LOG_D(RLC, "%d ",pdu_info->li_list[i]);
        }

        LOG_D(RLC, "\n");
        //msg(" remaining size %d\n",size);
        j = 0;

        for (i = 0; i < pdu_info->num_li; i++) {
          rlc_am_reassembly (ctxt_pP, rlc_pP, &pdu_info->payload[j], pdu_info->li_list[i]);
          rlc_am_send_sdu(ctxt_pP, rlc_pP);
          j = j + pdu_info->li_list[i];
        }

        if (pdu_info->hidden_size > 0) { // normally should always be > 0 but just for help debug
          // data is already ok, done by last loop above
          rlc_am_reassembly (ctxt_pP, rlc_pP, &pdu_info->payload[j], pdu_info->hidden_size);
        }

        //rlc_pP->reassembly_missing_sn_detected = 0;
        break;

      default:
        //Assertion(eNB)_PRAN_DesignDocument_annex No.1429
        LOG_E(RLC, "not RLC_E_FIXED_PART_DATA_FIELD_FOLLOW error pdu_info->fi[%d]\n", pdu_info->fi);
        //      assert(1 != 1);
    }
  }

  if (free_rlc_pdu) {
    free_mem_block(tb_pP, __func__);
  }
}
