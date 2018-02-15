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

/*! \file pdcp.c
 * \brief pdcp interface with RLC
 * \author Navid Nikaein and Lionel GAUTHIER
 * \date 2009-2012
 * \email navid.nikaein@eurecom.fr
 * \version 1.0
 */

#define PDCP_C
//#define DEBUG_PDCP_FIFO_FLUSH_SDU

#include "assertions.h"
#include "hashtable.h"
#include "pdcp.h"
#include "pdcp_util.h"
#include "pdcp_sequence_manager.h"
#include "LAYER2/RLC/rlc.h"
#include "LAYER2/MAC/extern.h"
#include "RRC/LITE/proto.h"
#include "pdcp_primitives.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "otg_rx.h"
#include "UTIL/LOG/log.h"
#include <inttypes.h>
#include "platform_constants.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "msc.h"

#if defined(ENABLE_SECURITY)
# include "UTIL/OSA/osa_defs.h"
#endif

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#if defined(LINK_ENB_PDCP_TO_GTPV1U)
#  include "gtpv1u_eNB_task.h"
#  include "gtpv1u.h"
#endif

extern int otg_enabled;

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

//-----------------------------------------------------------------------------
/*
 * If PDCP_UNIT_TEST is set here then data flow between PDCP and RLC is broken
 * and PDCP has no longer anything to do with RLC. In this case, after it's handed
 * an SDU it appends PDCP header and returns (by filling in incoming pointer parameters)
 * this mem_block_t to be dissected for testing purposes. For further details see test
 * code at targets/TEST/PDCP/test_pdcp.c:test_pdcp_data_req()
 */
boolean_t pdcp_data_req(
  protocol_ctxt_t*  ctxt_pP,
  const srb_flag_t     srb_flagP,
  const rb_id_t        rb_idP,
  const mui_t          muiP,
  const confirm_t      confirmP,
  const sdu_size_t     sdu_buffer_sizeP,
  unsigned char *const sdu_buffer_pP,
  const pdcp_transmission_mode_t modeP
)
//-----------------------------------------------------------------------------
{

  pdcp_t            *pdcp_p          = NULL;
  uint8_t            i               = 0;
  uint8_t            pdcp_header_len = 0;
  uint8_t            pdcp_tailer_len = 0;
  uint16_t           pdcp_pdu_size   = 0;
  uint16_t           current_sn      = 0;
  mem_block_t       *pdcp_pdu_p      = NULL;
  rlc_op_status_t    rlc_status;
  boolean_t          ret             = TRUE;

  hash_key_t         key             = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t     h_rc;
  uint8_t            rb_offset= (srb_flagP == 0) ? DTCH -1 : 0;
  uint16_t           pdcp_uid=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_REQ,VCD_FUNCTION_IN);
  CHECK_CTXT_ARGS(ctxt_pP);

#if T_TRACER
  if (ctxt_pP->enb_flag != ENB_FLAG_NO)
    T(T_ENB_PDCP_DL, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->rnti), T_INT(rb_idP), T_INT(sdu_buffer_sizeP));
#endif

  if (sdu_buffer_sizeP == 0) {
    LOG_W(PDCP, "Handed SDU is of size 0! Ignoring...\n");
    return FALSE;
  }

  /*
   * XXX MAX_IP_PACKET_SIZE is 4096, shouldn't this be MAX SDU size, which is 8188 bytes?
   */
  AssertFatal(sdu_buffer_sizeP<= MAX_IP_PACKET_SIZE,"Requested SDU size (%d) is bigger than that can be handled by PDCP (%u)!\n",
          sdu_buffer_sizeP, MAX_IP_PACKET_SIZE);
  
  if (modeP == PDCP_TRANSMISSION_MODE_TRANSPARENT) {
    AssertError (rb_idP < NB_RB_MBMS_MAX, return FALSE, "RB id is too high (%u/%d) %u %u!\n", rb_idP, NB_RB_MBMS_MAX, ctxt_pP->module_id, ctxt_pP->rnti);
  } else {
    if (srb_flagP) {
      AssertError (rb_idP < 3, return FALSE, "RB id is too high (%u/%d) %u %u!\n", rb_idP, 3, ctxt_pP->module_id, ctxt_pP->rnti);
    } else {
      AssertError (rb_idP < maxDRB, return FALSE, "RB id is too high (%u/%d) %u %u!\n", rb_idP, maxDRB, ctxt_pP->module_id, ctxt_pP->rnti);
    }
  }

  key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);

  if (h_rc != HASH_TABLE_OK) {
    if (modeP != PDCP_TRANSMISSION_MODE_TRANSPARENT) {
      LOG_W(PDCP, PROTOCOL_CTXT_FMT" Instance is not configured for rb_id %d Ignoring SDU...\n",
	    PROTOCOL_CTXT_ARGS(ctxt_pP),
	    rb_idP);
      ctxt_pP->configured=FALSE;
      return FALSE;
    }
  }else{
    // instance for a given RB is configured
    ctxt_pP->configured=TRUE;
  }
    
  if (ctxt_pP->enb_flag == ENB_FLAG_YES) {
    start_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_req);
  } else {
    start_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_req);
  }

  // PDCP transparent mode for MBMS traffic

  if (modeP == PDCP_TRANSMISSION_MODE_TRANSPARENT) {
    LOG_D(PDCP, " [TM] Asking for a new mem_block of size %d\n",sdu_buffer_sizeP);
    pdcp_pdu_p = get_free_mem_block(sdu_buffer_sizeP, __func__);

    if (pdcp_pdu_p != NULL) {
      memcpy(&pdcp_pdu_p->data[0], sdu_buffer_pP, sdu_buffer_sizeP);
#if defined(DEBUG_PDCP_PAYLOAD)
      rlc_util_print_hex_octets(PDCP,
                                (unsigned char*)&pdcp_pdu_p->data[0],
                                sdu_buffer_sizeP);
#endif
      rlc_status = rlc_data_req(ctxt_pP, srb_flagP, MBMS_FLAG_YES, rb_idP, muiP, confirmP, sdu_buffer_sizeP, pdcp_pdu_p);
    } else {
      rlc_status = RLC_OP_STATUS_OUT_OF_RESSOURCES;
      LOG_W(PDCP,PROTOCOL_CTXT_FMT" PDCP_DATA_REQ SDU DROPPED, OUT OF MEMORY \n",
            PROTOCOL_CTXT_ARGS(ctxt_pP));
#if defined(STOP_ON_IP_TRAFFIC_OVERLOAD)
      AssertFatal(0, PROTOCOL_CTXT_FMT"[RB %u] PDCP_DATA_REQ SDU DROPPED, OUT OF MEMORY \n",
                  PROTOCOL_CTXT_ARGS(ctxt_pP),
                  rb_idP);
#endif
    }
  } else {
    // calculate the pdcp header and trailer size
    if (srb_flagP) {
      pdcp_header_len = PDCP_CONTROL_PLANE_DATA_PDU_SN_SIZE;
      pdcp_tailer_len = PDCP_CONTROL_PLANE_DATA_PDU_MAC_I_SIZE;
    } else {
      pdcp_header_len = PDCP_USER_PLANE_DATA_PDU_LONG_SN_HEADER_SIZE;
      pdcp_tailer_len = 0;
    }

    pdcp_pdu_size = sdu_buffer_sizeP + pdcp_header_len + pdcp_tailer_len;

    LOG_D(PDCP, PROTOCOL_PDCP_CTXT_FMT"Data request notification  pdu size %d (header%d, trailer%d)\n",
          PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p),
          pdcp_pdu_size,
          pdcp_header_len,
          pdcp_tailer_len);

    /*
     * Allocate a new block for the new PDU (i.e. PDU header and SDU payload)
     */
    pdcp_pdu_p = get_free_mem_block(pdcp_pdu_size, __func__);

    if (pdcp_pdu_p != NULL) {
      /*
       * Create a Data PDU with header and append data
       *
       * Place User Plane PDCP Data PDU header first
       */

      if (srb_flagP) { // this Control plane PDCP Data PDU
        pdcp_control_plane_data_pdu_header pdu_header;
        pdu_header.sn = pdcp_get_next_tx_seq_number(pdcp_p);
        current_sn = pdu_header.sn;
        memset(&pdu_header.mac_i[0],0,PDCP_CONTROL_PLANE_DATA_PDU_MAC_I_SIZE);
        memset(&pdcp_pdu_p->data[sdu_buffer_sizeP + pdcp_header_len],0,PDCP_CONTROL_PLANE_DATA_PDU_MAC_I_SIZE);

        if (pdcp_serialize_control_plane_data_pdu_with_SRB_sn_buffer((unsigned char*)pdcp_pdu_p->data, &pdu_header) == FALSE) {
          LOG_E(PDCP, PROTOCOL_PDCP_CTXT_FMT" Cannot fill PDU buffer with relevant header fields!\n",
                PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p));

          if (ctxt_pP->enb_flag == ENB_FLAG_YES) {
            stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_req);
          } else {
            stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_req);
          }

          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_REQ,VCD_FUNCTION_OUT);
          return FALSE;
        }
      } else {
        pdcp_user_plane_data_pdu_header_with_long_sn pdu_header;
        pdu_header.dc = (modeP == PDCP_TRANSMISSION_MODE_DATA) ? PDCP_DATA_PDU_BIT_SET :  PDCP_CONTROL_PDU_BIT_SET;
        pdu_header.sn = pdcp_get_next_tx_seq_number(pdcp_p);
        current_sn = pdu_header.sn ;

        if (pdcp_serialize_user_plane_data_pdu_with_long_sn_buffer((unsigned char*)pdcp_pdu_p->data, &pdu_header) == FALSE) {
          LOG_E(PDCP, PROTOCOL_PDCP_CTXT_FMT" Cannot fill PDU buffer with relevant header fields!\n",
                PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p));
         
          if (ctxt_pP->enb_flag == ENB_FLAG_YES) {

            stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_req);
          } else {
            stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_req);
          }

          VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_REQ,VCD_FUNCTION_OUT);
          return FALSE;
        }
      }

      /*
       * Validate incoming sequence number, there might be a problem with PDCP initialization
       */
      if (current_sn > pdcp_calculate_max_seq_num_for_given_size(pdcp_p->seq_num_size)) {
        LOG_E(PDCP, PROTOCOL_PDCP_CTXT_FMT" Generated sequence number (%"PRIu16") is greater than a sequence number could ever be!\n"\
              "There must be a problem with PDCP initialization, ignoring this PDU...\n",
              PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p),
              current_sn);

        free_mem_block(pdcp_pdu_p, __func__);

        if (ctxt_pP->enb_flag == ENB_FLAG_YES) {
          stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_req);
        } else {
          stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_req);
        }

        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_REQ,VCD_FUNCTION_OUT);
        return FALSE;
      }

      LOG_D(PDCP, "Sequence number %d is assigned to current PDU\n", current_sn);

      /* Then append data... */
      memcpy(&pdcp_pdu_p->data[pdcp_header_len], sdu_buffer_pP, sdu_buffer_sizeP);

      //For control plane data that are not integrity protected,
      // the MAC-I field is still present and should be padded with padding bits set to 0.
      // NOTE: user-plane data are never integrity protected
      for (i=0; i<pdcp_tailer_len; i++) {
        pdcp_pdu_p->data[pdcp_header_len + sdu_buffer_sizeP + i] = 0x00;// pdu_header.mac_i[i];
      }

#if defined(ENABLE_SECURITY)

      if ((pdcp_p->security_activated != 0) &&
          (((pdcp_p->cipheringAlgorithm) != 0) ||
           ((pdcp_p->integrityProtAlgorithm) != 0))) {

        if (ctxt_pP->enb_flag == ENB_FLAG_YES) {
          start_meas(&eNB_pdcp_stats[ctxt_pP->module_id].apply_security);
        } else {
          start_meas(&UE_pdcp_stats[ctxt_pP->module_id].apply_security);
        }

        pdcp_apply_security(ctxt_pP,
                            pdcp_p,
                            srb_flagP,
                            rb_idP % maxDRB,
                            pdcp_header_len,
                            current_sn,
                            pdcp_pdu_p->data,
                            sdu_buffer_sizeP);

        if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
          stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].apply_security);
        } else {
          stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].apply_security);
        }
      }

#endif

      /* Print octets of outgoing data in hexadecimal form */
      LOG_D(PDCP, "Following content with size %d will be sent over RLC (PDCP PDU header is the first two bytes)\n",
            pdcp_pdu_size);
      //util_print_hex_octets(PDCP, (unsigned char*)pdcp_pdu_p->data, pdcp_pdu_size);
      //util_flush_hex_octets(PDCP, (unsigned char*)pdcp_pdu->data, pdcp_pdu_size);
    } else {
      LOG_E(PDCP, "Cannot create a mem_block for a PDU!\n");

      if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
        stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_req);
      } else {
        stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_req);
      }

#if defined(STOP_ON_IP_TRAFFIC_OVERLOAD)
      AssertFatal(0, "[FRAME %5u][%s][PDCP][MOD %u/%u][RB %u] PDCP_DATA_REQ SDU DROPPED, OUT OF MEMORY \n",
                  ctxt_pP->frame,
                  (ctxt_pP->enb_flag) ? "eNB" : "UE",
                  ctxt_pP->enb_module_id,
                  ctxt_pP->ue_module_id,
                  rb_idP);
#endif
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_REQ,VCD_FUNCTION_OUT);
      return FALSE;
    }

    /*
     * Ask sublayer to transmit data and check return value
     * to see if RLC succeeded
     */
#ifdef PDCP_MSG_PRINT
    int i=0;
    LOG_F(PDCP,"[MSG] PDCP DL %s PDU on rb_id %d\n", (srb_flagP)? "CONTROL" : "DATA", rb_idP);

    for (i = 0; i < pdcp_pdu_size; i++) {
      LOG_F(PDCP,"%02x ", ((uint8_t*)pdcp_pdu_p->data)[i]);
    }

    LOG_F(PDCP,"\n");
#endif
    rlc_status = rlc_data_req(ctxt_pP, srb_flagP, MBMS_FLAG_NO, rb_idP, muiP, confirmP, pdcp_pdu_size, pdcp_pdu_p);

  }

  switch (rlc_status) {
  case RLC_OP_STATUS_OK:
    LOG_D(PDCP, "Data sending request over RLC succeeded!\n");
    ret=TRUE;
    break;

  case RLC_OP_STATUS_BAD_PARAMETER:
    LOG_W(PDCP, "Data sending request over RLC failed with 'Bad Parameter' reason!\n");
    ret= FALSE;
    break;

  case RLC_OP_STATUS_INTERNAL_ERROR:
    LOG_W(PDCP, "Data sending request over RLC failed with 'Internal Error' reason!\n");
    ret= FALSE;
    break;

  case RLC_OP_STATUS_OUT_OF_RESSOURCES:
    LOG_W(PDCP, "Data sending request over RLC failed with 'Out of Resources' reason!\n");
    ret= FALSE;
    break;

  default:
    LOG_W(PDCP, "RLC returned an unknown status code after PDCP placed the order to send some data (Status Code:%d)\n", rlc_status);
    ret= FALSE;
    break;
  }

  if (ctxt_pP->enb_flag == ENB_FLAG_YES) {
    stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_req);
  } else {
    stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_req);
  }

  /*
   * Control arrives here only if rlc_data_req() returns RLC_OP_STATUS_OK
   * so we return TRUE afterwards
   */
  
  for (pdcp_uid=0; pdcp_uid< NUMBER_OF_UE_MAX;pdcp_uid++){
    if (pdcp_enb[ctxt_pP->module_id].rnti[pdcp_uid] == ctxt_pP->rnti ) 
      break;
  }

  //LOG_I(PDCP,"ueid %d lcid %d tx seq num %d\n", pdcp_uid, rb_idP+rb_offset, current_sn);
  Pdcp_stats_tx[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]++;
  Pdcp_stats_tx_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]++;
  Pdcp_stats_tx_bytes[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]+=sdu_buffer_sizeP;
  Pdcp_stats_tx_bytes_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]+=sdu_buffer_sizeP;
  Pdcp_stats_tx_sn[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]=current_sn;

  Pdcp_stats_tx_aiat[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]+= (pdcp_enb[ctxt_pP->module_id].sfn - Pdcp_stats_tx_iat[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]);
  Pdcp_stats_tx_aiat_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]+= (pdcp_enb[ctxt_pP->module_id].sfn - Pdcp_stats_tx_iat[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]); 
  Pdcp_stats_tx_iat[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]=pdcp_enb[ctxt_pP->module_id].sfn;
    
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_REQ,VCD_FUNCTION_OUT);
  return ret;

}


//-----------------------------------------------------------------------------
boolean_t
pdcp_data_ind(
  const protocol_ctxt_t* const ctxt_pP,
  const srb_flag_t   srb_flagP,
  const MBMS_flag_t  MBMS_flagP,
  const rb_id_t      rb_idP,
  const sdu_size_t   sdu_buffer_sizeP,
  mem_block_t* const sdu_buffer_pP
)
//-----------------------------------------------------------------------------
{
  pdcp_t      *pdcp_p          = NULL;
  list_t      *sdu_list_p      = NULL;
  mem_block_t *new_sdu_p       = NULL;
  uint8_t      pdcp_header_len = 0;
  uint8_t      pdcp_tailer_len = 0;
  pdcp_sn_t    sequence_number = 0;
  volatile sdu_size_t   payload_offset  = 0;
  rb_id_t      rb_id            = rb_idP;
  boolean_t    packet_forwarded = FALSE;
  hash_key_t      key             = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t  h_rc;
  uint8_t      rb_offset= (srb_flagP == 0) ? DTCH -1 :0;
  uint16_t     pdcp_uid=0;      
  uint8_t      oo_flag=0;
#if defined(LINK_ENB_PDCP_TO_GTPV1U)
  MessageDef  *message_p        = NULL;
  uint8_t     *gtpu_buffer_p    = NULL;
#endif


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_IND,VCD_FUNCTION_IN);

#ifdef PDCP_MSG_PRINT
  int i=0;
  LOG_F(PDCP,"[MSG] PDCP UL %s PDU on rb_id %d\n", (srb_flagP)? "CONTROL" : "DATA", rb_idP);

  for (i = 0; i < sdu_buffer_sizeP; i++) {
    LOG_F(PDCP,"%02x ", ((uint8_t*)sdu_buffer_pP->data)[i]);
  }

  LOG_F(PDCP,"\n");
#endif

#if T_TRACER
  if (ctxt_pP->enb_flag != ENB_FLAG_NO)
    T(T_ENB_PDCP_UL, T_INT(ctxt_pP->module_id), T_INT(ctxt_pP->rnti), T_INT(rb_idP), T_INT(sdu_buffer_sizeP));
#endif

  if (MBMS_flagP) {
    AssertError (rb_idP < NB_RB_MBMS_MAX, return FALSE,
                 "RB id is too high (%u/%d) %u rnti %x!\n",
                 rb_idP,
                 NB_RB_MBMS_MAX,
                 ctxt_pP->module_id,
                 ctxt_pP->rnti);

    if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
      LOG_D(PDCP, "e-MBMS Data indication notification for PDCP entity from eNB %u to UE %x "
            "and radio bearer ID %d rlc sdu size %d ctxt_pP->enb_flag %d\n",
            ctxt_pP->module_id,
            ctxt_pP->rnti,
            rb_idP,
            sdu_buffer_sizeP,
            ctxt_pP->enb_flag);

    } else {
      LOG_D(PDCP, "Data indication notification for PDCP entity from UE %x to eNB %u "
            "and radio bearer ID %d rlc sdu size %d ctxt_pP->enb_flag %d\n",
            ctxt_pP->rnti,
            ctxt_pP->module_id ,
            rb_idP,
            sdu_buffer_sizeP,
            ctxt_pP->enb_flag);
    }

  } else {
    rb_id = rb_idP % maxDRB;
    AssertError (rb_id < maxDRB, return FALSE, "RB id is too high (%u/%d) %u UE %x!\n",
                 rb_id,
                 maxDRB,
                 ctxt_pP->module_id,
                 ctxt_pP->rnti);
    AssertError (rb_id > 0, return FALSE, "RB id is too low (%u/%d) %u UE %x!\n",
                 rb_id,
                 maxDRB,
                 ctxt_pP->module_id,
                 ctxt_pP->rnti);
    key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_id, srb_flagP);
    h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);

    if (h_rc != HASH_TABLE_OK) {
      LOG_W(PDCP,
            PROTOCOL_CTXT_FMT"Could not get PDCP instance key 0x%"PRIx64"\n",
            PROTOCOL_CTXT_ARGS(ctxt_pP),
            key);
      free_mem_block(sdu_buffer_pP, __func__);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_IND,VCD_FUNCTION_OUT);
      return FALSE;
    }
  }

  sdu_list_p = &pdcp_sdu_list;

  if (sdu_buffer_sizeP == 0) {
    LOG_W(PDCP, "SDU buffer size is zero! Ignoring this chunk!\n");
    return FALSE;
  }

  if (ctxt_pP->enb_flag) {
    start_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_ind);
  } else {
    start_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_ind);
  }

  /*
   * Parse the PDU placed at the beginning of SDU to check
   * if incoming SN is in line with RX window
   */

  if (MBMS_flagP == 0 ) {
    if (srb_flagP) { //SRB1/2
      pdcp_header_len = PDCP_CONTROL_PLANE_DATA_PDU_SN_SIZE;
      pdcp_tailer_len = PDCP_CONTROL_PLANE_DATA_PDU_MAC_I_SIZE;
      sequence_number =   pdcp_get_sequence_number_of_pdu_with_SRB_sn((unsigned char*)sdu_buffer_pP->data);
    } else { // DRB
      pdcp_tailer_len = 0;

      if (pdcp_p->seq_num_size == PDCP_SN_7BIT) {
        pdcp_header_len = PDCP_USER_PLANE_DATA_PDU_SHORT_SN_HEADER_SIZE;
        sequence_number =     pdcp_get_sequence_number_of_pdu_with_short_sn((unsigned char*)sdu_buffer_pP->data);
      } else if (pdcp_p->seq_num_size == PDCP_SN_12BIT) {
        pdcp_header_len = PDCP_USER_PLANE_DATA_PDU_LONG_SN_HEADER_SIZE;
        sequence_number =     pdcp_get_sequence_number_of_pdu_with_long_sn((unsigned char*)sdu_buffer_pP->data);
      } else {
        //sequence_number = 4095;
        LOG_E(PDCP,
              PROTOCOL_PDCP_CTXT_FMT"wrong sequence number  (%d) for this pdcp entity \n",
              PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
              pdcp_p->seq_num_size);
      }

      //uint8_t dc = pdcp_get_dc_filed((unsigned char*)sdu_buffer_pP->data);
    }

    /*
     * Check if incoming SDU is long enough to carry a PDU header
     */
    if (sdu_buffer_sizeP < pdcp_header_len + pdcp_tailer_len ) {
      LOG_W(PDCP,
            PROTOCOL_PDCP_CTXT_FMT"Incoming (from RLC) SDU is short of size (size:%d)! Ignoring...\n",
            PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
            sdu_buffer_sizeP);
      free_mem_block(sdu_buffer_pP, __func__);

      if (ctxt_pP->enb_flag) {
        stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_ind);
      } else {
        stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_ind);
      }

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_IND,VCD_FUNCTION_OUT);
      return FALSE;
    }

    if (pdcp_is_rx_seq_number_valid(sequence_number, pdcp_p, srb_flagP) == TRUE) {
#if 0
      LOG_T(PDCP, "Incoming PDU has a sequence number (%d) in accordance with RX window\n", sequence_number);
#endif
      /* if (dc == PDCP_DATA_PDU )
      LOG_D(PDCP, "Passing piggybacked SDU to NAS driver...\n");
      else
      LOG_D(PDCP, "Passing piggybacked SDU to RRC ...\n");*/
    } else {
      oo_flag=1;
      LOG_W(PDCP,
            PROTOCOL_PDCP_CTXT_FMT"Incoming PDU has an unexpected sequence number (%d), RX window synchronisation have probably been lost!\n",
            PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
            sequence_number);
      /*
       * XXX Till we implement in-sequence delivery and duplicate discarding
       * mechanism all out-of-order packets will be delivered to RRC/IP
       */
#if 0
      LOG_D(PDCP, "Ignoring PDU...\n");
      free_mem_block(sdu_buffer, __func__);
      return FALSE;
#else
      //LOG_W(PDCP, "Delivering out-of-order SDU to upper layer...\n");
#endif
    }

    // SRB1/2: control-plane data
    if (srb_flagP) {
#if defined(ENABLE_SECURITY)

      if (pdcp_p->security_activated == 1) {
        if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
          start_meas(&eNB_pdcp_stats[ctxt_pP->module_id].validate_security);
        } else {
          start_meas(&UE_pdcp_stats[ctxt_pP->module_id].validate_security);
        }

        pdcp_validate_security(ctxt_pP,
                               pdcp_p,
                               srb_flagP,
                               rb_idP,
                               pdcp_header_len,
                               sequence_number,
                               sdu_buffer_pP->data,
                               sdu_buffer_sizeP - pdcp_tailer_len);

        if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
          stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].validate_security);
        } else {
          stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].validate_security);
        }
      }

#endif
      //rrc_lite_data_ind(module_id, //Modified MW - L2 Interface
  	MSC_LOG_TX_MESSAGE(
  	    (ctxt_pP->enb_flag == ENB_FLAG_NO)? MSC_PDCP_UE:MSC_PDCP_ENB,
        (ctxt_pP->enb_flag == ENB_FLAG_NO)? MSC_RRC_UE:MSC_RRC_ENB,
        NULL,0,
        PROTOCOL_PDCP_CTXT_FMT" DATA-IND len %u",
        PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
        sdu_buffer_sizeP - pdcp_header_len - pdcp_tailer_len);
        rrc_data_ind(ctxt_pP,
		   rb_id,
		   sdu_buffer_sizeP - pdcp_header_len - pdcp_tailer_len,
		   (uint8_t*)&sdu_buffer_pP->data[pdcp_header_len]);
        free_mem_block(sdu_buffer_pP, __func__);


      // free_mem_block(new_sdu, __func__);
      if (ctxt_pP->enb_flag) {
        stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_ind);
      } else {
        stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_ind);
      }

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_IND,VCD_FUNCTION_OUT);
      return TRUE;
    }

    /*
     * DRBs
     */
    payload_offset=pdcp_header_len;// PDCP_USER_PLANE_DATA_PDU_LONG_SN_HEADER_SIZE;
#if defined(ENABLE_SECURITY)

    if (pdcp_p->security_activated == 1) {
      if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
        start_meas(&eNB_pdcp_stats[ctxt_pP->module_id].validate_security);
      } else {
        start_meas(&UE_pdcp_stats[ctxt_pP->module_id].validate_security);
      }

      pdcp_validate_security(
        ctxt_pP,
        pdcp_p,
        srb_flagP,
        rb_idP,
        pdcp_header_len,
        sequence_number,
        sdu_buffer_pP->data,
        sdu_buffer_sizeP - pdcp_tailer_len);

      if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
        stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].validate_security);
      } else {
        stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].validate_security);
      }

    }

#endif
  } else {
    payload_offset=0;
  }

  if (otg_enabled==1) {
    LOG_D(OTG,"Discarding received packed\n");
    free_mem_block(sdu_buffer_pP, __func__);

    if (ctxt_pP->enb_flag) {
      stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_ind);
    } else {
      stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_ind);
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_IND,VCD_FUNCTION_OUT);
    return TRUE;
  }

  // XXX Decompression would be done at this point

  /*
   * After checking incoming sequence number PDCP header
   * has to be stripped off so here we copy SDU buffer starting
   * from its second byte (skipping 0th and 1st octets, i.e.
   * PDCP header)
   */
#if defined(LINK_ENB_PDCP_TO_GTPV1U)

  if ((TRUE == ctxt_pP->enb_flag) && (FALSE == srb_flagP)) {
    MSC_LOG_TX_MESSAGE(
    		MSC_PDCP_ENB,
    		MSC_GTPU_ENB,
    		NULL,0,
    		"0 GTPV1U_ENB_TUNNEL_DATA_REQ  ue %x rab %u len %u",
    		ctxt_pP->rnti,
    		rb_id + 4,
    		sdu_buffer_sizeP - payload_offset);
    //LOG_T(PDCP,"Sending to GTPV1U %d bytes\n", sdu_buffer_sizeP - payload_offset);
    gtpu_buffer_p = itti_malloc(TASK_PDCP_ENB, TASK_GTPV1_U,
                                sdu_buffer_sizeP - payload_offset + GTPU_HEADER_OVERHEAD_MAX);
    AssertFatal(gtpu_buffer_p != NULL, "OUT OF MEMORY");
    memcpy(&gtpu_buffer_p[GTPU_HEADER_OVERHEAD_MAX], &sdu_buffer_pP->data[payload_offset], sdu_buffer_sizeP - payload_offset);
    message_p = itti_alloc_new_message(TASK_PDCP_ENB, GTPV1U_ENB_TUNNEL_DATA_REQ);
    AssertFatal(message_p != NULL, "OUT OF MEMORY");
    GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).buffer       = gtpu_buffer_p;
    GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).length       = sdu_buffer_sizeP - payload_offset;
    GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).offset       = GTPU_HEADER_OVERHEAD_MAX;
    GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).rnti         = ctxt_pP->rnti;
    GTPV1U_ENB_TUNNEL_DATA_REQ(message_p).rab_id       = rb_id + 4;
    itti_send_msg_to_task(TASK_GTPV1_U, INSTANCE_DEFAULT, message_p);
    packet_forwarded = TRUE;    
  }
  
#else
  packet_forwarded = FALSE;
#endif

  if (FALSE == packet_forwarded) {
    new_sdu_p = get_free_mem_block(sdu_buffer_sizeP - payload_offset + sizeof (pdcp_data_ind_header_t), __func__);

    if (new_sdu_p) {
      if (pdcp_p->rlc_mode == RLC_MODE_AM ) {
        pdcp_p->last_submitted_pdcp_rx_sn = sequence_number;
      }

      /*
       * Prepend PDCP indication header which is going to be removed at pdcp_fifo_flush_sdus()
       */
      memset(new_sdu_p->data, 0, sizeof (pdcp_data_ind_header_t));
      ((pdcp_data_ind_header_t *) new_sdu_p->data)->data_size = sdu_buffer_sizeP - payload_offset;
      AssertFatal((sdu_buffer_sizeP - payload_offset >= 0), "invalid PDCP SDU size!");

      // Here there is no virtualization possible
      // set ((pdcp_data_ind_header_t *) new_sdu_p->data)->inst for IP layer here
      if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
        ((pdcp_data_ind_header_t *) new_sdu_p->data)->rb_id = rb_id;
#if defined(ENABLE_USE_MME)
        /* for the UE compiled in S1 mode, we need 1 here
         * for the UE compiled in noS1 mode, we need 0
         * TODO: be sure of this
         */
        ((pdcp_data_ind_header_t*) new_sdu_p->data)->inst  = 1;
#endif
      } else {
        ((pdcp_data_ind_header_t*) new_sdu_p->data)->rb_id = rb_id + (ctxt_pP->module_id * maxDRB);
      }
#ifdef DEBUG_PDCP_FIFO_FLUSH_SDU
      static uint32_t pdcp_inst = 0;
      ((pdcp_data_ind_header_t*) new_sdu_p->data)->inst = pdcp_inst++;
      LOG_D(PDCP, "inst=%d size=%d\n", ((pdcp_data_ind_header_t*) new_sdu_p->data)->inst, ((pdcp_data_ind_header_t *) new_sdu_p->data)->data_size);
#endif

      memcpy(&new_sdu_p->data[sizeof (pdcp_data_ind_header_t)], \
             &sdu_buffer_pP->data[payload_offset], \
             sdu_buffer_sizeP - payload_offset);
      list_add_tail_eurecom (new_sdu_p, sdu_list_p);

      
      
    }
  }

  /* Print octets of incoming data in hexadecimal form */
  LOG_D(PDCP, "Following content has been received from RLC (%d,%d)(PDCP header has already been removed):\n",
	sdu_buffer_sizeP  - payload_offset + (int)sizeof(pdcp_data_ind_header_t),
	sdu_buffer_sizeP  - payload_offset);
  //util_print_hex_octets(PDCP, &new_sdu_p->data[sizeof (pdcp_data_ind_header_t)], sdu_buffer_sizeP - payload_offset);
  //util_flush_hex_octets(PDCP, &new_sdu_p->data[sizeof (pdcp_data_ind_header_t)], sdu_buffer_sizeP - payload_offset);
  
  /*
     * Update PDCP statistics
   * XXX Following two actions are identical, is there a merge error?
   */
  
  for (pdcp_uid=0; pdcp_uid< NUMBER_OF_UE_MAX;pdcp_uid++){
    if (pdcp_enb[ctxt_pP->module_id].rnti[pdcp_uid] == ctxt_pP->rnti ){
      break;
    }
  }	
  
  Pdcp_stats_rx[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]++;
  Pdcp_stats_rx_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]++;
  Pdcp_stats_rx_bytes[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]+=(sdu_buffer_sizeP  - payload_offset);
  Pdcp_stats_rx_bytes_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]+=(sdu_buffer_sizeP  - payload_offset);
  
  Pdcp_stats_rx_sn[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]=sequence_number;
  
  if (oo_flag == 1 )
    Pdcp_stats_rx_outoforder[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]++;
  
  Pdcp_stats_rx_aiat[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]+= (pdcp_enb[ctxt_pP->module_id].sfn - Pdcp_stats_rx_iat[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]);
  Pdcp_stats_rx_aiat_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]+=(pdcp_enb[ctxt_pP->module_id].sfn - Pdcp_stats_rx_iat[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]);
  Pdcp_stats_rx_iat[ctxt_pP->module_id][pdcp_uid][rb_idP+rb_offset]=pdcp_enb[ctxt_pP->module_id].sfn;

  
#if defined(STOP_ON_IP_TRAFFIC_OVERLOAD)
  else {
    AssertFatal(0, PROTOCOL_PDCP_CTXT_FMT" PDCP_DATA_IND SDU DROPPED, OUT OF MEMORY \n",
                PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p));
  }

#endif

  free_mem_block(sdu_buffer_pP, __func__);

  if (ctxt_pP->enb_flag) {
    stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].data_ind);
  } else {
    stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].data_ind);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_DATA_IND,VCD_FUNCTION_OUT);
  return TRUE;
}

void pdcp_update_stats(const protocol_ctxt_t* const  ctxt_pP){

  uint8_t            pdcp_uid = 0;
  uint8_t            rb_id     = 0;
  
 // these stats are measured for both eNB and UE on per seond basis 
  for (rb_id =0; rb_id < NB_RB_MAX; rb_id ++){
    for (pdcp_uid=0; pdcp_uid< NUMBER_OF_UE_MAX;pdcp_uid++){
      //printf("frame %d and subframe %d \n", pdcp_enb[ctxt_pP->module_id].frame, pdcp_enb[ctxt_pP->module_id].subframe);
      // tx stats
      if (pdcp_enb[ctxt_pP->module_id].sfn % Pdcp_stats_tx_window_ms[ctxt_pP->module_id][pdcp_uid] == 0){
	// unit: bit/s
	Pdcp_stats_tx_throughput_w[ctxt_pP->module_id][pdcp_uid][rb_id]=Pdcp_stats_tx_bytes_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]*8;
	Pdcp_stats_tx_w[ctxt_pP->module_id][pdcp_uid][rb_id]= Pdcp_stats_tx_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id];
	Pdcp_stats_tx_bytes_w[ctxt_pP->module_id][pdcp_uid][rb_id]= Pdcp_stats_tx_bytes_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id];
	if (Pdcp_stats_tx_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id] > 0){
	  Pdcp_stats_tx_aiat_w[ctxt_pP->module_id][pdcp_uid][rb_id]=(Pdcp_stats_tx_aiat_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]/Pdcp_stats_tx_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]);
	}else {
	  Pdcp_stats_tx_aiat_w[ctxt_pP->module_id][pdcp_uid][rb_id]=0;
	}
	// reset the tmp vars 
	Pdcp_stats_tx_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]=0;
	Pdcp_stats_tx_bytes_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]=0;
	Pdcp_stats_tx_aiat_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]=0;
	
      }
      if (pdcp_enb[ctxt_pP->module_id].sfn % Pdcp_stats_rx_window_ms[ctxt_pP->module_id][pdcp_uid] == 0){
	// rx stats
	Pdcp_stats_rx_goodput_w[ctxt_pP->module_id][pdcp_uid][rb_id]=Pdcp_stats_rx_bytes_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]*8;
	Pdcp_stats_rx_w[ctxt_pP->module_id][pdcp_uid][rb_id]= 	Pdcp_stats_rx_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id];
	Pdcp_stats_rx_bytes_w[ctxt_pP->module_id][pdcp_uid][rb_id]= Pdcp_stats_rx_bytes_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id];
	
	if(Pdcp_stats_rx_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id] > 0){
	  Pdcp_stats_rx_aiat_w[ctxt_pP->module_id][pdcp_uid][rb_id]= (Pdcp_stats_rx_aiat_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]/Pdcp_stats_rx_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]);
	} else {
	  Pdcp_stats_rx_aiat_w[ctxt_pP->module_id][pdcp_uid][rb_id]=0;
	}
	
	// reset the tmp vars 
	Pdcp_stats_rx_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]=0;
	Pdcp_stats_rx_bytes_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]=0;
	Pdcp_stats_rx_aiat_tmp_w[ctxt_pP->module_id][pdcp_uid][rb_id]=0;
      } 
    }
    
  }
}
//-----------------------------------------------------------------------------
void
pdcp_run (
  const protocol_ctxt_t* const  ctxt_pP
)
//-----------------------------------------------------------------------------
{
  
#if defined(ENABLE_ITTI)
  MessageDef   *msg_p;
  const char   *msg_name;
  instance_t    instance;
  int           result;
  protocol_ctxt_t  ctxt;
#endif

  
  if (ctxt_pP->enb_flag) {
    start_meas(&eNB_pdcp_stats[ctxt_pP->module_id].pdcp_run);
  } else {
    start_meas(&UE_pdcp_stats[ctxt_pP->module_id].pdcp_run);
  }

  pdcp_enb[ctxt_pP->module_id].sfn++; // range: 0 to 18,446,744,073,709,551,615
  pdcp_enb[ctxt_pP->module_id].frame=ctxt_pP->frame; // 1023 
  pdcp_enb[ctxt_pP->module_id].subframe= ctxt_pP->subframe;
  pdcp_update_stats(ctxt_pP);
   
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_RUN, VCD_FUNCTION_IN);

#if defined(ENABLE_ITTI)

  do {
    // Checks if a message has been sent to PDCP sub-task
    itti_poll_msg (ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE, &msg_p);

    if (msg_p != NULL) {
      msg_name = ITTI_MSG_NAME (msg_p);
      instance = ITTI_MSG_INSTANCE (msg_p);

      switch (ITTI_MSG_ID(msg_p)) {
      case RRC_DCCH_DATA_REQ:
	PROTOCOL_CTXT_SET_BY_MODULE_ID(
          &ctxt,
          RRC_DCCH_DATA_REQ (msg_p).module_id,
          RRC_DCCH_DATA_REQ (msg_p).enb_flag,
          RRC_DCCH_DATA_REQ (msg_p).rnti,
          RRC_DCCH_DATA_REQ (msg_p).frame, 
	  0,
	  RRC_DCCH_DATA_REQ (msg_p).eNB_index);
        LOG_I(PDCP, PROTOCOL_CTXT_FMT"Received %s from %s: instance %d, rb_id %d, muiP %d, confirmP %d, mode %d\n",
              PROTOCOL_CTXT_ARGS(&ctxt),
              msg_name,
              ITTI_MSG_ORIGIN_NAME(msg_p),
              instance,
              RRC_DCCH_DATA_REQ (msg_p).rb_id,
              RRC_DCCH_DATA_REQ (msg_p).muip,
              RRC_DCCH_DATA_REQ (msg_p).confirmp,
              RRC_DCCH_DATA_REQ (msg_p).mode);

        result = pdcp_data_req (&ctxt,
                                SRB_FLAG_YES,
                                RRC_DCCH_DATA_REQ (msg_p).rb_id,
                                RRC_DCCH_DATA_REQ (msg_p).muip,
                                RRC_DCCH_DATA_REQ (msg_p).confirmp,
                                RRC_DCCH_DATA_REQ (msg_p).sdu_size,
                                RRC_DCCH_DATA_REQ (msg_p).sdu_p,
                                RRC_DCCH_DATA_REQ (msg_p).mode);
        if (result != TRUE)
          LOG_E(PDCP, "PDCP data request failed!\n");

        // Message buffer has been processed, free it now.
        result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), RRC_DCCH_DATA_REQ (msg_p).sdu_p);
        AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
        break;

      case RRC_PCCH_DATA_REQ:
      {
        sdu_size_t     sdu_buffer_sizeP;
        sdu_buffer_sizeP = RRC_PCCH_DATA_REQ(msg_p).sdu_size;
        uint8_t CC_id = RRC_PCCH_DATA_REQ(msg_p).CC_id;
        uint8_t ue_index = RRC_PCCH_DATA_REQ(msg_p).ue_index;
        RC.rrc[ctxt_pP->module_id]->carrier[CC_id].sizeof_paging[ue_index] = sdu_buffer_sizeP;
        if (sdu_buffer_sizeP > 0) {
        	memcpy(RC.rrc[ctxt_pP->module_id]->carrier[CC_id].paging[ue_index], RRC_PCCH_DATA_REQ(msg_p).sdu_p, sdu_buffer_sizeP);
        }
        //paging pdcp log
        LOG_D(PDCP, "PDCP Received RRC_PCCH_DATA_REQ CC_id %d length %d \n", CC_id, sdu_buffer_sizeP);
      }
      break;

      default:
        LOG_E(PDCP, "Received unexpected message %s\n", msg_name);
        break;
      }

      result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
      AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    }
  } while(msg_p != NULL);

# if 0
  {
    MessageDef *msg_resp_p;

    msg_resp_p = itti_alloc_new_message(TASK_PDCP_ENB, MESSAGE_TEST);

    itti_send_msg_to_task(TASK_RRC_ENB, 1, msg_resp_p);
  }
  {
    MessageDef *msg_resp_p;

    msg_resp_p = itti_alloc_new_message(TASK_PDCP_ENB, MESSAGE_TEST);

    itti_send_msg_to_task(TASK_ENB_APP, 2, msg_resp_p);
  }
  {
    MessageDef *msg_resp_p;

    msg_resp_p = itti_alloc_new_message(TASK_PDCP_ENB, MESSAGE_TEST);

    itti_send_msg_to_task(TASK_MAC_ENB, 3, msg_resp_p);
  }
# endif
#endif

  // IP/NAS -> PDCP traffic : TX, read the pkt from the upper layer buffer
#if defined(LINK_ENB_PDCP_TO_GTPV1U)

  if (ctxt_pP->enb_flag == ENB_FLAG_NO)
#endif
  {
    pdcp_fifo_read_input_sdus(ctxt_pP);
  }

  // PDCP -> NAS/IP traffic: RX
  if (ctxt_pP->enb_flag) {
    start_meas(&eNB_pdcp_stats[ctxt_pP->module_id].pdcp_ip);
  }

  else {
    start_meas(&UE_pdcp_stats[ctxt_pP->module_id].pdcp_ip);
  }

  pdcp_fifo_flush_sdus(ctxt_pP);

  if (ctxt_pP->enb_flag) {
    stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].pdcp_ip);
  } else {
    stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].pdcp_ip);
  }

  if (ctxt_pP->enb_flag) {
    stop_meas(&eNB_pdcp_stats[ctxt_pP->module_id].pdcp_run);
  } else {
    stop_meas(&UE_pdcp_stats[ctxt_pP->module_id].pdcp_run);
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDCP_RUN, VCD_FUNCTION_OUT);
}

void pdcp_add_UE(const protocol_ctxt_t* const  ctxt_pP){
  int i, ue_flag=1; //, ret=-1; to be decied later
  for (i=0; i < NUMBER_OF_UE_MAX; i++){
    if (pdcp_enb[ctxt_pP->module_id].rnti[i] == ctxt_pP->rnti) {
      ue_flag=-1;
      break;
    }
  }
  if (ue_flag == 1 ){
    for (i=0; i < NUMBER_OF_UE_MAX ; i++){
      if (pdcp_enb[ctxt_pP->module_id].rnti[i] == 0 ){
	pdcp_enb[ctxt_pP->module_id].rnti[i]=ctxt_pP->rnti;
	pdcp_enb[ctxt_pP->module_id].uid[i]=i;
	pdcp_enb[ctxt_pP->module_id].num_ues++;
	printf("add new uid is %d %x\n\n", i, ctxt_pP->rnti);
	// ret=1;
	break;
      }
    }
  }
  //return ret;
}

//-----------------------------------------------------------------------------
boolean_t
pdcp_remove_UE(
  const protocol_ctxt_t* const  ctxt_pP
)
//-----------------------------------------------------------------------------
{
  DRB_Identity_t  srb_id         = 0;
  DRB_Identity_t  drb_id         = 0;
  hash_key_t      key            = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t  h_rc;
  int i; 
   // check and remove SRBs first

  for(int i = 0;i<NUMBER_OF_UE_MAX;i++){
    if(pdcp_eNB_UE_instance_to_rnti[i] == ctxt_pP->rnti){
      pdcp_eNB_UE_instance_to_rnti[i] = NOT_A_RNTI;
      break;
    }
  }

  for (srb_id=1; srb_id<3; srb_id++) {
    key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, srb_id, SRB_FLAG_YES);
    h_rc = hashtable_remove(pdcp_coll_p, key);
  }

  for (drb_id=0; drb_id<maxDRB; drb_id++) {
    key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, drb_id, SRB_FLAG_NO);
    h_rc = hashtable_remove(pdcp_coll_p, key);

  }

  (void)h_rc; /* remove gcc warning "set but not used" */

  // remove ue for pdcp enb inst
   for (i=0; i < NUMBER_OF_UE_MAX; i++) {
    if (pdcp_enb[ctxt_pP->module_id].rnti[i] == ctxt_pP->rnti ) {
      LOG_I(PDCP, "remove uid is %d/%d %x\n", i,
	    pdcp_enb[ctxt_pP->module_id].uid[i],
	    pdcp_enb[ctxt_pP->module_id].rnti[i]);
      pdcp_enb[ctxt_pP->module_id].uid[i]=0;
      pdcp_enb[ctxt_pP->module_id].rnti[i]=0;
      pdcp_enb[ctxt_pP->module_id].num_ues--;
      break;
    }
  }
   
  return 1;
}


//-----------------------------------------------------------------------------
boolean_t
rrc_pdcp_config_asn1_req (
  const protocol_ctxt_t* const  ctxt_pP,
  SRB_ToAddModList_t  *const srb2add_list_pP,
  DRB_ToAddModList_t  *const drb2add_list_pP,
  DRB_ToReleaseList_t *const drb2release_list_pP,
  const uint8_t                   security_modeP,
  uint8_t                  *const kRRCenc_pP,
  uint8_t                  *const kRRCint_pP,
  uint8_t                  *const kUPenc_pP
#if defined(Rel10) || defined(Rel14)
  ,PMCH_InfoList_r9_t*  const pmch_InfoList_r9_pP
#endif
  ,rb_id_t                 *const defaultDRB 
)
//-----------------------------------------------------------------------------
{
  long int        lc_id          = 0;
  DRB_Identity_t  srb_id         = 0;
  long int        mch_id         = 0;
  rlc_mode_t      rlc_type       = RLC_MODE_NONE;
  DRB_Identity_t  drb_id         = 0;
  DRB_Identity_t *pdrb_id_p      = NULL;
  uint8_t         drb_sn         = 12;
  uint8_t         srb_sn         = 5; // fixed sn for SRBs
  uint8_t         drb_report     = 0;
  long int        cnt            = 0;
  uint16_t        header_compression_profile = 0;
  config_action_t action                     = CONFIG_ACTION_ADD;
  SRB_ToAddMod_t *srb_toaddmod_p = NULL;
  DRB_ToAddMod_t *drb_toaddmod_p = NULL;
  pdcp_t         *pdcp_p         = NULL;

  hash_key_t      key            = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t  h_rc;
  hash_key_t      key_defaultDRB = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t  h_defaultDRB_rc;
#if defined(Rel10) || defined(Rel14)
  int i,j;
  MBMS_SessionInfoList_r9_t *mbms_SessionInfoList_r9_p = NULL;
  MBMS_SessionInfo_r9_t     *MBMS_SessionInfo_p        = NULL;
#endif

  LOG_T(PDCP, PROTOCOL_CTXT_FMT" %s() SRB2ADD %p DRB2ADD %p DRB2RELEASE %p\n",
        PROTOCOL_CTXT_ARGS(ctxt_pP),
        __FUNCTION__,
        srb2add_list_pP,
        drb2add_list_pP,
        drb2release_list_pP);

  // srb2add_list does not define pdcp config, we use rlc info to setup the pdcp dcch0 and dcch1 channels

  if (srb2add_list_pP != NULL) {
    for (cnt=0; cnt<srb2add_list_pP->list.count; cnt++) {
      srb_id = srb2add_list_pP->list.array[cnt]->srb_Identity;
      srb_toaddmod_p = srb2add_list_pP->list.array[cnt];
      rlc_type = RLC_MODE_AM;
      lc_id = srb_id;
      key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, srb_id, SRB_FLAG_YES);
      h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);

      if (h_rc == HASH_TABLE_OK) {
        action = CONFIG_ACTION_MODIFY;
        LOG_D(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_MODIFY key 0x%"PRIx64"\n",
              PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
              key);
      } else {
        action = CONFIG_ACTION_ADD;
        pdcp_p = calloc(1, sizeof(pdcp_t));
        h_rc = hashtable_insert(pdcp_coll_p, key, pdcp_p);

        if (h_rc != HASH_TABLE_OK) {
          LOG_E(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_ADD key 0x%"PRIx64" FAILED\n",
                PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
                key);
          free(pdcp_p);
          return TRUE;

      } else {
	  LOG_D(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_ADD key 0x%"PRIx64"\n",
                PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
                key);
        }
      }

      if (srb_toaddmod_p->rlc_Config) {
        switch (srb_toaddmod_p->rlc_Config->present) {
        case SRB_ToAddMod__rlc_Config_PR_NOTHING:
          break;

        case SRB_ToAddMod__rlc_Config_PR_explicitValue:
          switch (srb_toaddmod_p->rlc_Config->choice.explicitValue.present) {
          case RLC_Config_PR_NOTHING:
            break;

          default:
            pdcp_config_req_asn1 (
              ctxt_pP,
              pdcp_p,
              SRB_FLAG_YES,
              rlc_type,
              action,
              lc_id,
              mch_id,
              srb_id,
              srb_sn,
              0, // drb_report
              0, // header compression
              security_modeP,
              kRRCenc_pP,
              kRRCint_pP,
              kUPenc_pP);
            break;
          }

          break;

        case SRB_ToAddMod__rlc_Config_PR_defaultValue:
        	pdcp_config_req_asn1 (
        	              ctxt_pP,
        	              pdcp_p,
        	              SRB_FLAG_YES,
        	              rlc_type,
        	              action,
        	              lc_id,
        	              mch_id,
        	              srb_id,
        	              srb_sn,
        	              0, // drb_report
        	              0, // header compression
        	              security_modeP,
        	              kRRCenc_pP,
        	              kRRCint_pP,
        	              kUPenc_pP);
          // already the default values
          break;

        default:
          DevParam(srb_toaddmod_p->rlc_Config->present, ctxt_pP->module_id, ctxt_pP->rnti);
          break;
        }
      }
    }
  }

  // reset the action

  if (drb2add_list_pP != NULL) {
    for (cnt=0; cnt<drb2add_list_pP->list.count; cnt++) {

      drb_toaddmod_p = drb2add_list_pP->list.array[cnt];

      drb_id = drb_toaddmod_p->drb_Identity;// + drb_id_offset;
      if (drb_toaddmod_p->logicalChannelIdentity) {
        lc_id = *(drb_toaddmod_p->logicalChannelIdentity);
      } else {
        LOG_E(PDCP, PROTOCOL_PDCP_CTXT_FMT" logicalChannelIdentity is missing in DRB-ToAddMod information element!\n",
              PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p));
        continue;
      }

      if (lc_id == 1 || lc_id == 2) {
        LOG_E(RLC, PROTOCOL_CTXT_FMT" logicalChannelIdentity = %ld is invalid in RRC message when adding DRB!\n", PROTOCOL_CTXT_ARGS(ctxt_pP), lc_id);
        continue;
      }

      DevCheck4(drb_id < maxDRB, drb_id, maxDRB, ctxt_pP->module_id, ctxt_pP->rnti);
      key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, drb_id, SRB_FLAG_NO);
      h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);

      if (h_rc == HASH_TABLE_OK) {
        action = CONFIG_ACTION_MODIFY;
        LOG_D(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_MODIFY key 0x%"PRIx64"\n",
              PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
              key);

      } else {
        action = CONFIG_ACTION_ADD;
        pdcp_p = calloc(1, sizeof(pdcp_t));
        h_rc = hashtable_insert(pdcp_coll_p, key, pdcp_p);

        // save the first configured DRB-ID as the default DRB-ID
        if ((defaultDRB != NULL) && (*defaultDRB == drb_id)) {
          key_defaultDRB = PDCP_COLL_KEY_DEFAULT_DRB_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag);
          h_defaultDRB_rc = hashtable_insert(pdcp_coll_p, key_defaultDRB, pdcp_p);
        } else {
          h_defaultDRB_rc = HASH_TABLE_OK; // do not trigger any error handling if this is not a default DRB
        }

        if (h_defaultDRB_rc != HASH_TABLE_OK) {
          LOG_E(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_ADD ADD default DRB key 0x%"PRIx64" FAILED\n",
                PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
                key_defaultDRB);
          free(pdcp_p);
          return TRUE;
        } else if (h_rc != HASH_TABLE_OK) {
          LOG_E(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_ADD ADD key 0x%"PRIx64" FAILED\n",
                PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
                key);
          free(pdcp_p);
          return TRUE;
        } else {
          LOG_D(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_ADD ADD key 0x%"PRIx64"\n",
                PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p),
                key);
         }
      }

      if (drb_toaddmod_p->pdcp_Config) {
        if (drb_toaddmod_p->pdcp_Config->discardTimer) {
          // set the value of the timer
        }

        if (drb_toaddmod_p->pdcp_Config->rlc_AM) {
          drb_report = drb_toaddmod_p->pdcp_Config->rlc_AM->statusReportRequired;
          drb_sn = PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits; // default SN size
          rlc_type = RLC_MODE_AM;
        }

        if (drb_toaddmod_p->pdcp_Config->rlc_UM) {
          drb_sn = drb_toaddmod_p->pdcp_Config->rlc_UM->pdcp_SN_Size;
          rlc_type =RLC_MODE_UM;
        }

        switch (drb_toaddmod_p->pdcp_Config->headerCompression.present) {
        case PDCP_Config__headerCompression_PR_NOTHING:
        case PDCP_Config__headerCompression_PR_notUsed:
          header_compression_profile=0x0;
          break;

        case PDCP_Config__headerCompression_PR_rohc:

          // parse the struc and get the rohc profile
          if(drb_toaddmod_p->pdcp_Config->headerCompression.choice.rohc.profiles.profile0x0001) {
            header_compression_profile=0x0001;
          } else if(drb_toaddmod_p->pdcp_Config->headerCompression.choice.rohc.profiles.profile0x0002) {
            header_compression_profile=0x0002;
          } else if(drb_toaddmod_p->pdcp_Config->headerCompression.choice.rohc.profiles.profile0x0003) {
            header_compression_profile=0x0003;
          } else if(drb_toaddmod_p->pdcp_Config->headerCompression.choice.rohc.profiles.profile0x0004) {
            header_compression_profile=0x0004;
          } else if(drb_toaddmod_p->pdcp_Config->headerCompression.choice.rohc.profiles.profile0x0006) {
            header_compression_profile=0x0006;
          } else if(drb_toaddmod_p->pdcp_Config->headerCompression.choice.rohc.profiles.profile0x0101) {
            header_compression_profile=0x0101;
          } else if(drb_toaddmod_p->pdcp_Config->headerCompression.choice.rohc.profiles.profile0x0102) {
            header_compression_profile=0x0102;
          } else if(drb_toaddmod_p->pdcp_Config->headerCompression.choice.rohc.profiles.profile0x0103) {
            header_compression_profile=0x0103;
          } else if(drb_toaddmod_p->pdcp_Config->headerCompression.choice.rohc.profiles.profile0x0104) {
            header_compression_profile=0x0104;
          } else {
            header_compression_profile=0x0;
            LOG_W(PDCP,"unknown header compresion profile\n");
          }

          // set the applicable profile
          break;

        default:
          LOG_W(PDCP,PROTOCOL_PDCP_CTXT_FMT"[RB %ld] unknown drb_toaddmod->PDCP_Config->headerCompression->present \n",
                PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p), drb_id);
          break;
        }

        pdcp_config_req_asn1 (
          ctxt_pP,
          pdcp_p,
          SRB_FLAG_NO,
          rlc_type,
          action,
          lc_id,
          mch_id,
          drb_id,
          drb_sn,
          drb_report,
          header_compression_profile,
          security_modeP,
          kRRCenc_pP,
          kRRCint_pP,
          kUPenc_pP);
      }
    }
  }

  if (drb2release_list_pP != NULL) {
    for (cnt=0; cnt<drb2release_list_pP->list.count; cnt++) {
      pdrb_id_p = drb2release_list_pP->list.array[cnt];
      drb_id =  *pdrb_id_p;
      key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, drb_id, SRB_FLAG_NO);
      h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);

      if (h_rc != HASH_TABLE_OK) {
        LOG_E(PDCP, PROTOCOL_CTXT_FMT" PDCP REMOVE FAILED drb_id %ld\n",
              PROTOCOL_CTXT_ARGS(ctxt_pP),
              drb_id);
        continue;
      }
      lc_id = pdcp_p->lcid;

      action = CONFIG_ACTION_REMOVE;
      pdcp_config_req_asn1 (
        ctxt_pP,
        pdcp_p,
        SRB_FLAG_NO,
        rlc_type,
        action,
        lc_id,
        mch_id,
        drb_id,
        0,
        0,
        0,
        security_modeP,
        kRRCenc_pP,
        kRRCint_pP,
        kUPenc_pP);
      h_rc = hashtable_remove(pdcp_coll_p, key);

      if ((defaultDRB != NULL) && (*defaultDRB == drb_id)) {
        // default DRB being removed. nevertheless this shouldn't happen as removing default DRB is not allowed in standard
        key_defaultDRB = PDCP_COLL_KEY_DEFAULT_DRB_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag);
        h_defaultDRB_rc = hashtable_get(pdcp_coll_p, key_defaultDRB, (void**)&pdcp_p);

        if (h_defaultDRB_rc == HASH_TABLE_OK) {
          h_defaultDRB_rc = hashtable_remove(pdcp_coll_p, key_defaultDRB);
        } else {
          LOG_E(PDCP, PROTOCOL_CTXT_FMT" PDCP REMOVE FAILED default DRB\n", PROTOCOL_CTXT_ARGS(ctxt_pP));
        }
      } else {
        key_defaultDRB = HASH_TABLE_OK; // do not trigger any error handling if this is not a default DRB
      }
    }
  }

#if defined(Rel10) || defined(Rel14)

  if (pmch_InfoList_r9_pP != NULL) {
    for (i=0; i<pmch_InfoList_r9_pP->list.count; i++) {
      mbms_SessionInfoList_r9_p = &(pmch_InfoList_r9_pP->list.array[i]->mbms_SessionInfoList_r9);

      for (j=0; j<mbms_SessionInfoList_r9_p->list.count; j++) {
        MBMS_SessionInfo_p = mbms_SessionInfoList_r9_p->list.array[j];
        lc_id = MBMS_SessionInfo_p->sessionId_r9->buf[0];
        mch_id = MBMS_SessionInfo_p->tmgi_r9.serviceId_r9.buf[2]; //serviceId is 3-octet string

        // can set the mch_id = i
        if (ctxt_pP->enb_flag) {
          drb_id =  (mch_id * maxSessionPerPMCH ) + lc_id ;//+ (maxDRB + 3)*MAX_MOBILES_PER_ENB; // 1

          if (pdcp_mbms_array_eNB[ctxt_pP->module_id][mch_id][lc_id].instanciated_instance == TRUE) {
            action = CONFIG_ACTION_MBMS_MODIFY;
          } else {
            action = CONFIG_ACTION_MBMS_ADD;
          }
        } else {
          drb_id =  (mch_id * maxSessionPerPMCH ) + lc_id; // + (maxDRB + 3); // 15

          if (pdcp_mbms_array_ue[ctxt_pP->module_id][mch_id][lc_id].instanciated_instance == TRUE) {
            action = CONFIG_ACTION_MBMS_MODIFY;
          } else {
            action = CONFIG_ACTION_MBMS_ADD;
          }
        }

        pdcp_config_req_asn1 (
          ctxt_pP,
          NULL,  // unused for MBMS
          SRB_FLAG_NO,
          RLC_MODE_NONE,
          action,
          lc_id,
          mch_id,
          drb_id,
          0,   // unused for MBMS
          0,   // unused for MBMS
          0,   // unused for MBMS
          0,   // unused for MBMS
          NULL,  // unused for MBMS
          NULL,  // unused for MBMS
          NULL); // unused for MBMS
      }
    }
  }

#endif
  return 0;
}

//-----------------------------------------------------------------------------
boolean_t
pdcp_config_req_asn1 (
  const protocol_ctxt_t* const  ctxt_pP,
  pdcp_t         * const        pdcp_pP,
  const srb_flag_t              srb_flagP,
  const rlc_mode_t              rlc_modeP,
  const config_action_t         actionP,
  const uint16_t                lc_idP,
  const uint16_t                mch_idP,
  const rb_id_t                 rb_idP,
  const uint8_t                 rb_snP,
  const uint8_t                 rb_reportP,
  const uint16_t                header_compression_profileP,
  const uint8_t                 security_modeP,
  uint8_t         *const        kRRCenc_pP,
  uint8_t         *const        kRRCint_pP,
  uint8_t         *const        kUPenc_pP)
//-----------------------------------------------------------------------------
{
  
  switch (actionP) {
  case CONFIG_ACTION_ADD:
    DevAssert(pdcp_pP != NULL);
    if (ctxt_pP->enb_flag == ENB_FLAG_YES) {
      pdcp_pP->is_ue = FALSE;
      pdcp_add_UE(ctxt_pP);
      
      //pdcp_eNB_UE_instance_to_rnti[ctxtP->module_id] = ctxt_pP->rnti;
//      pdcp_eNB_UE_instance_to_rnti[pdcp_eNB_UE_instance_to_rnti_index] = ctxt_pP->rnti;
      if( srb_flagP == SRB_FLAG_NO ) {
          for(int i = 0;i<NUMBER_OF_UE_MAX;i++){
              if(pdcp_eNB_UE_instance_to_rnti[pdcp_eNB_UE_instance_to_rnti_index] == NOT_A_RNTI){
                  break;
              }
              pdcp_eNB_UE_instance_to_rnti_index = (pdcp_eNB_UE_instance_to_rnti_index + 1) % NUMBER_OF_UE_MAX;
          }
          pdcp_eNB_UE_instance_to_rnti[pdcp_eNB_UE_instance_to_rnti_index] = ctxt_pP->rnti;
          pdcp_eNB_UE_instance_to_rnti_index = (pdcp_eNB_UE_instance_to_rnti_index + 1) % NUMBER_OF_UE_MAX;
      }
      //pdcp_eNB_UE_instance_to_rnti_index = (pdcp_eNB_UE_instance_to_rnti_index + 1) % NUMBER_OF_UE_MAX;
    } else {
      pdcp_pP->is_ue = TRUE;
      pdcp_UE_UE_module_id_to_rnti[ctxt_pP->module_id] = ctxt_pP->rnti;
    }
    pdcp_pP->is_srb                     = (srb_flagP == SRB_FLAG_YES) ? TRUE : FALSE;
    pdcp_pP->lcid                       = lc_idP;
    pdcp_pP->rb_id                      = rb_idP;
    pdcp_pP->header_compression_profile = header_compression_profileP;
    pdcp_pP->status_report              = rb_reportP;

    if (rb_snP == PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits) {
      pdcp_pP->seq_num_size = PDCP_SN_12BIT;
    } else if (rb_snP == PDCP_Config__rlc_UM__pdcp_SN_Size_len7bits) {
      pdcp_pP->seq_num_size = PDCP_SN_7BIT;
    } else {
      pdcp_pP->seq_num_size = PDCP_SN_5BIT;
    }


    pdcp_pP->rlc_mode                         = rlc_modeP;
    pdcp_pP->next_pdcp_tx_sn                  = 0;
    pdcp_pP->next_pdcp_rx_sn                  = 0;
    pdcp_pP->next_pdcp_rx_sn_before_integrity = 0;
    pdcp_pP->tx_hfn                           = 0;
    pdcp_pP->rx_hfn                           = 0;
    pdcp_pP->last_submitted_pdcp_rx_sn        = 4095;
    pdcp_pP->first_missing_pdu                = -1;
    pdcp_pP->rx_hfn_offset                    = 0;

    LOG_N(PDCP, PROTOCOL_PDCP_CTXT_FMT" Action ADD  LCID %d (%s id %d) "
            "configured with SN size %d bits and RLC %s\n",
          PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_pP),
          lc_idP,
	  (srb_flagP == SRB_FLAG_YES) ? "SRB" : "DRB",
          rb_idP,
          pdcp_pP->seq_num_size,
	  (rlc_modeP == RLC_MODE_AM ) ? "AM" : (rlc_modeP == RLC_MODE_TM) ? "TM" : "UM");
    /* Setup security */
    if (security_modeP != 0xff) {
      pdcp_config_set_security(
        ctxt_pP,
        pdcp_pP,
        rb_idP,
        lc_idP,
        security_modeP,
        kRRCenc_pP,
        kRRCint_pP,
        kUPenc_pP);
    }
    break;

  case CONFIG_ACTION_MODIFY:
    DevAssert(pdcp_pP != NULL);
    pdcp_pP->header_compression_profile=header_compression_profileP;
    pdcp_pP->status_report = rb_reportP;
    pdcp_pP->rlc_mode = rlc_modeP;

    /* Setup security */
    if (security_modeP != 0xff) {
      pdcp_config_set_security(
        ctxt_pP,
        pdcp_pP,
        rb_idP,
        lc_idP,
        security_modeP,
        kRRCenc_pP,
        kRRCint_pP,
        kUPenc_pP);
    }

    if (rb_snP == PDCP_Config__rlc_UM__pdcp_SN_Size_len7bits) {
      pdcp_pP->seq_num_size = 7;
    } else if (rb_snP == PDCP_Config__rlc_UM__pdcp_SN_Size_len12bits) {
      pdcp_pP->seq_num_size = 12;
    } else {
      pdcp_pP->seq_num_size=5;
    }

    LOG_N(PDCP,PROTOCOL_PDCP_CTXT_FMT" Action MODIFY LCID %d "
            "RB id %d reconfigured with SN size %d and RLC %s \n",
          PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_pP),
          lc_idP,
          rb_idP,
          rb_snP,
            (rlc_modeP == RLC_MODE_AM) ? "AM" : (rlc_modeP == RLC_MODE_TM) ? "TM" : "UM");
    break;

  case CONFIG_ACTION_REMOVE:
    DevAssert(pdcp_pP != NULL);
//#warning "TODO pdcp_module_id_to_rnti"
    //pdcp_module_id_to_rnti[ctxt_pP.module_id ][dst_id] = NOT_A_RNTI;
    LOG_D(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_REMOVE LCID %d RBID %d configured\n",
          PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_pP),
          lc_idP,
          rb_idP);

   if (ctxt_pP->enb_flag == ENB_FLAG_YES) {
     // pdcp_remove_UE(ctxt_pP);
   }

    /* Security keys */
    if (pdcp_pP->kUPenc != NULL) {
      free(pdcp_pP->kUPenc);
    }

    if (pdcp_pP->kRRCint != NULL) {
      free(pdcp_pP->kRRCint);
    }

    if (pdcp_pP->kRRCenc != NULL) {
      free(pdcp_pP->kRRCenc);
    }

    memset(pdcp_pP, 0, sizeof(pdcp_t));
    break;
#if defined(Rel10) || defined(Rel14)

  case CONFIG_ACTION_MBMS_ADD:
  case CONFIG_ACTION_MBMS_MODIFY:
    LOG_D(PDCP," %s service_id/mch index %d, session_id/lcid %d, rbid %d configured\n",
          //PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_pP),
          actionP == CONFIG_ACTION_MBMS_ADD ? "CONFIG_ACTION_MBMS_ADD" : "CONFIG_ACTION_MBMS_MODIFY",
          mch_idP,
          lc_idP,
          rb_idP);

    if (ctxt_pP->enb_flag == ENB_FLAG_YES) {
      pdcp_mbms_array_eNB[ctxt_pP->module_id][mch_idP][lc_idP].instanciated_instance = TRUE ;
      pdcp_mbms_array_eNB[ctxt_pP->module_id][mch_idP][lc_idP].rb_id = rb_idP;
    } else {
      pdcp_mbms_array_ue[ctxt_pP->module_id][mch_idP][lc_idP].instanciated_instance = TRUE ;
      pdcp_mbms_array_ue[ctxt_pP->module_id][mch_idP][lc_idP].rb_id = rb_idP;
    }

    break;
#endif

  case CONFIG_ACTION_SET_SECURITY_MODE:
    pdcp_config_set_security(
      ctxt_pP,
      pdcp_pP,
      rb_idP,
      lc_idP,
      security_modeP,
      kRRCenc_pP,
      kRRCint_pP,
      kUPenc_pP);
    break;

  default:
    DevParam(actionP, ctxt_pP->module_id, ctxt_pP->rnti);
    break;
  }

  return 0;
}

//-----------------------------------------------------------------------------
void
pdcp_config_set_security(
  const protocol_ctxt_t* const  ctxt_pP,
  pdcp_t         * const pdcp_pP,
  const rb_id_t         rb_idP,
  const uint16_t        lc_idP,
  const uint8_t         security_modeP,
  uint8_t        * const kRRCenc,
  uint8_t        * const kRRCint,
  uint8_t        * const  kUPenc)
//-----------------------------------------------------------------------------
{
  DevAssert(pdcp_pP != NULL);

  if ((security_modeP >= 0) && (security_modeP <= 0x77)) {
    pdcp_pP->cipheringAlgorithm     = security_modeP & 0x0f;
    pdcp_pP->integrityProtAlgorithm = (security_modeP>>4) & 0xf;

    LOG_D(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_SET_SECURITY_MODE: cipheringAlgorithm %d integrityProtAlgorithm %d\n",
          PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_pP),
            pdcp_pP->cipheringAlgorithm,
            pdcp_pP->integrityProtAlgorithm);

    pdcp_pP->kRRCenc = kRRCenc;
    pdcp_pP->kRRCint = kRRCint;
    pdcp_pP->kUPenc  = kUPenc;

    /* Activate security */
    pdcp_pP->security_activated = 1;
	MSC_LOG_EVENT(
	  (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
	  "0 Set security ciph %X integ %x UE %"PRIx16" ",
	  pdcp_pP->cipheringAlgorithm,
	  pdcp_pP->integrityProtAlgorithm,
	  ctxt_pP->rnti);
  } else {
	  MSC_LOG_EVENT(
	    (ctxt_pP->enb_flag == ENB_FLAG_YES) ? MSC_PDCP_ENB:MSC_PDCP_UE,
	    "0 Set security failed UE %"PRIx16" ",
	    ctxt_pP->rnti);
	  LOG_E(PDCP,PROTOCOL_PDCP_CTXT_FMT"  bad security mode %d",
          PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_pP),
          security_modeP);
  }
}

//-----------------------------------------------------------------------------
void
rrc_pdcp_config_req (
  const protocol_ctxt_t* const  ctxt_pP,
  const srb_flag_t srb_flagP,
  const uint32_t actionP,
  const rb_id_t rb_idP,
  const uint8_t security_modeP)
//-----------------------------------------------------------------------------
{
  pdcp_t *pdcp_p = NULL;
  hash_key_t       key           = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, rb_idP, srb_flagP);
  hashtable_rc_t   h_rc;
  h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);

  if (h_rc == HASH_TABLE_OK) {

  /*
   * Initialize sequence number state variables of relevant PDCP entity
   */
  switch (actionP) {
  case CONFIG_ACTION_ADD:
    pdcp_p->is_srb = srb_flagP;
    pdcp_p->rb_id  = rb_idP;

    if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
      pdcp_p->is_ue = TRUE;
    } else {
      pdcp_p->is_ue = FALSE;
    }

    pdcp_p->next_pdcp_tx_sn = 0;
    pdcp_p->next_pdcp_rx_sn = 0;
    pdcp_p->tx_hfn = 0;
    pdcp_p->rx_hfn = 0;
    /* SN of the last PDCP SDU delivered to upper layers */
    pdcp_p->last_submitted_pdcp_rx_sn = 4095;

    if (rb_idP < DTCH) { // SRB
      pdcp_p->seq_num_size = 5;
    } else { // DRB
      pdcp_p->seq_num_size = 12;
    }

    pdcp_p->first_missing_pdu = -1;
      LOG_D(PDCP,PROTOCOL_PDCP_CTXT_FMT" Config request : Action ADD:  radio bearer id %d (already added) configured\n",
            PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p),
            rb_idP);
    break;

  case CONFIG_ACTION_MODIFY:
    break;

  case CONFIG_ACTION_REMOVE:
      LOG_D(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_REMOVE: radio bearer id %d configured\n",
            PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p),
            rb_idP);
    pdcp_p->next_pdcp_tx_sn = 0;
    pdcp_p->next_pdcp_rx_sn = 0;
    pdcp_p->tx_hfn = 0;
    pdcp_p->rx_hfn = 0;
    pdcp_p->last_submitted_pdcp_rx_sn = 4095;
    pdcp_p->seq_num_size = 0;
    pdcp_p->first_missing_pdu = -1;
    pdcp_p->security_activated = 0;
      h_rc = hashtable_remove(pdcp_coll_p, key);

    break;

  case CONFIG_ACTION_SET_SECURITY_MODE:
    if ((security_modeP >= 0) && (security_modeP <= 0x77)) {
      pdcp_p->cipheringAlgorithm= security_modeP & 0x0f;
      pdcp_p->integrityProtAlgorithm = (security_modeP>>4) & 0xf;
        LOG_D(PDCP, PROTOCOL_PDCP_CTXT_FMT" CONFIG_ACTION_SET_SECURITY_MODE: cipheringAlgorithm %d integrityProtAlgorithm %d\n",
              PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p),
            pdcp_p->cipheringAlgorithm,
            pdcp_p->integrityProtAlgorithm );
    } else {
        LOG_W(PDCP,PROTOCOL_PDCP_CTXT_FMT" bad security mode %d", PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p), security_modeP);
    }

    break;

  default:
      DevParam(actionP, ctxt_pP->module_id, ctxt_pP->rnti);
    break;
  }
  } else {
    switch (actionP) {
    case CONFIG_ACTION_ADD:
      pdcp_p = calloc(1, sizeof(pdcp_t));
      h_rc = hashtable_insert(pdcp_coll_p, key, pdcp_p);

      if (h_rc != HASH_TABLE_OK) {
        LOG_E(PDCP, PROTOCOL_PDCP_CTXT_FMT" PDCP ADD FAILED\n",
              PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP, pdcp_p));
        free(pdcp_p);

      } else {
        pdcp_p->is_srb = srb_flagP;
        pdcp_p->rb_id  = rb_idP;

        if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
          pdcp_p->is_ue = TRUE;

        } else {
          pdcp_p->is_ue = FALSE;
}

        pdcp_p->next_pdcp_tx_sn = 0;
        pdcp_p->next_pdcp_rx_sn = 0;
        pdcp_p->tx_hfn = 0;
        pdcp_p->rx_hfn = 0;
        /* SN of the last PDCP SDU delivered to upper layers */
        pdcp_p->last_submitted_pdcp_rx_sn = 4095;

        if (rb_idP < DTCH) { // SRB
          pdcp_p->seq_num_size = 5;

        } else { // DRB
          pdcp_p->seq_num_size = 12;
        }

        pdcp_p->first_missing_pdu = -1;
        LOG_D(PDCP,PROTOCOL_PDCP_CTXT_FMT" Inserting PDCP instance in collection key 0x%"PRIx64"\n",
              PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p), key);
        LOG_D(PDCP,PROTOCOL_PDCP_CTXT_FMT" Config request : Action ADD:  radio bearer id %d configured\n",
              PROTOCOL_PDCP_CTXT_ARGS(ctxt_pP,pdcp_p),
              rb_idP);
      }

      break;

    case CONFIG_ACTION_REMOVE:
      LOG_D(PDCP, PROTOCOL_CTXT_FMT" CONFIG_REQ PDCP CONFIG_ACTION_REMOVE PDCP instance not found\n",
            PROTOCOL_CTXT_ARGS(ctxt_pP));
      break;

    default:
      LOG_E(PDCP, PROTOCOL_CTXT_FMT" CONFIG_REQ PDCP NOT FOUND\n",
            PROTOCOL_CTXT_ARGS(ctxt_pP));
    }
  }
}


//-----------------------------------------------------------------------------
 
int
pdcp_module_init (
  void
)
//-----------------------------------------------------------------------------
{
 
#ifdef PDCP_USE_RT_FIFO
  int ret;

  ret=rtf_create(PDCP2NW_DRIVER_FIFO,32768);

  if (ret < 0) {
    LOG_E(PDCP, "Cannot create PDCP2NW_DRIVER_FIFO fifo %d (ERROR %d)\n", PDCP2NW_DRIVER_FIFO, ret);
    return -1;
  } else {
    LOG_D(PDCP, "Created PDCP2NAS fifo %d\n", PDCP2NW_DRIVER_FIFO);
    rtf_reset(PDCP2NW_DRIVER_FIFO);
  }

  ret=rtf_create(NW_DRIVER2PDCP_FIFO,32768);

  if (ret < 0) {
    LOG_E(PDCP, "Cannot create NW_DRIVER2PDCP_FIFO fifo %d (ERROR %d)\n", NW_DRIVER2PDCP_FIFO, ret);

    return -1;
  } else {
    LOG_D(PDCP, "Created NW_DRIVER2PDCP_FIFO fifo %d\n", NW_DRIVER2PDCP_FIFO);
    rtf_reset(NW_DRIVER2PDCP_FIFO);
  }

  pdcp_2_nas_irq = 0;
  pdcp_input_sdu_remaining_size_to_read=0;
  pdcp_input_sdu_size_read=0;
#endif

  return 0;
}

//-----------------------------------------------------------------------------
void
pdcp_free (
  void* pdcp_pP
)
//-----------------------------------------------------------------------------
{
  pdcp_t* pdcp_p = (pdcp_t*)pdcp_pP;

  if (pdcp_p != NULL) {
    if (pdcp_p->kUPenc != NULL) {
      free(pdcp_p->kUPenc);
    }

    if (pdcp_p->kRRCint != NULL) {
      free(pdcp_p->kRRCint);
    }

    if (pdcp_p->kRRCenc != NULL) {
      free(pdcp_p->kRRCenc);
    }

    memset(pdcp_pP, 0, sizeof(pdcp_t));
    free(pdcp_pP);
  }
}

//-----------------------------------------------------------------------------
void pdcp_module_cleanup (void)
//-----------------------------------------------------------------------------
{
#ifdef PDCP_USE_RT_FIFO
  rtf_destroy(NW_DRIVER2PDCP_FIFO);
  rtf_destroy(PDCP2NW_DRIVER_FIFO);
#endif
}

//-----------------------------------------------------------------------------
void pdcp_layer_init(void)
//-----------------------------------------------------------------------------
{

  module_id_t       instance;
  int i,j;
#if defined(Rel10) || defined(Rel14)
  mbms_session_id_t session_id;
  mbms_service_id_t service_id;
#endif
  /*
   * Initialize SDU list
   */
  list_init(&pdcp_sdu_list, NULL);
  pdcp_coll_p = hashtable_create ((maxDRB + 2) * 16, NULL, pdcp_free);
  AssertFatal(pdcp_coll_p != NULL, "UNRECOVERABLE error, PDCP hashtable_create failed");

  for (instance = 0; instance < NUMBER_OF_UE_MAX; instance++) {
#if defined(Rel10) || defined(Rel14)

    for (service_id = 0; service_id < maxServiceCount; service_id++) {
      for (session_id = 0; session_id < maxSessionPerPMCH; session_id++) {
        memset(&pdcp_mbms_array_ue[instance][service_id][session_id], 0, sizeof(pdcp_mbms_t));
      }
    }
#endif
    pdcp_eNB_UE_instance_to_rnti[instance] = NOT_A_RNTI;
  }
  pdcp_eNB_UE_instance_to_rnti_index = 0; 

    
  for (instance = 0; instance < NUMBER_OF_eNB_MAX; instance++) {
#if defined(Rel10) || defined(Rel14)

    for (service_id = 0; service_id < maxServiceCount; service_id++) {
      for (session_id = 0; session_id < maxSessionPerPMCH; session_id++) {
        memset(&pdcp_mbms_array_eNB[instance][service_id][session_id], 0, sizeof(pdcp_mbms_t));
      }
    }

#endif
  }

  LOG_I(PDCP, "PDCP layer has been initialized\n");

  pdcp_output_sdu_bytes_to_write=0;
  pdcp_output_header_bytes_to_write=0;
  pdcp_input_sdu_remaining_size_to_read=0;

  memset(pdcp_enb, 0, sizeof(pdcp_enb_t));

  
  memset(Pdcp_stats_tx_window_ms, 0, sizeof(Pdcp_stats_tx_window_ms));
  memset(Pdcp_stats_rx_window_ms, 0, sizeof(Pdcp_stats_rx_window_ms));
  for (i =0; i< MAX_NUM_CCs ; i ++){
    for (j=0; j< NUMBER_OF_UE_MAX;j++){
      Pdcp_stats_tx_window_ms[i][j]=100;
      Pdcp_stats_rx_window_ms[i][j]=100;
    }
  }
  
  memset(Pdcp_stats_tx, 0, sizeof(Pdcp_stats_tx));
  memset(Pdcp_stats_tx_w, 0, sizeof(Pdcp_stats_tx_w));
  memset(Pdcp_stats_tx_tmp_w, 0, sizeof(Pdcp_stats_tx_tmp_w));
  memset(Pdcp_stats_tx_bytes, 0, sizeof(Pdcp_stats_tx_bytes));
  memset(Pdcp_stats_tx_bytes_w, 0, sizeof(Pdcp_stats_tx_bytes_w));
  memset(Pdcp_stats_tx_bytes_tmp_w, 0, sizeof(Pdcp_stats_tx_bytes_tmp_w));
  memset(Pdcp_stats_tx_sn, 0, sizeof(Pdcp_stats_tx_sn));
  memset(Pdcp_stats_tx_throughput_w, 0, sizeof(Pdcp_stats_tx_throughput_w));
  memset(Pdcp_stats_tx_aiat, 0, sizeof(Pdcp_stats_tx_aiat));
  memset(Pdcp_stats_tx_iat, 0, sizeof(Pdcp_stats_tx_iat));
  

  memset(Pdcp_stats_rx, 0, sizeof(Pdcp_stats_rx));
  memset(Pdcp_stats_rx_w, 0, sizeof(Pdcp_stats_rx_w));
  memset(Pdcp_stats_rx_tmp_w, 0, sizeof(Pdcp_stats_rx_tmp_w));
  memset(Pdcp_stats_rx_bytes, 0, sizeof(Pdcp_stats_rx_bytes));
  memset(Pdcp_stats_rx_bytes_w, 0, sizeof(Pdcp_stats_rx_bytes_w));
  memset(Pdcp_stats_rx_bytes_tmp_w, 0, sizeof(Pdcp_stats_rx_bytes_tmp_w));
  memset(Pdcp_stats_rx_sn, 0, sizeof(Pdcp_stats_rx_sn));
  memset(Pdcp_stats_rx_goodput_w, 0, sizeof(Pdcp_stats_rx_goodput_w));
  memset(Pdcp_stats_rx_aiat, 0, sizeof(Pdcp_stats_rx_aiat));
  memset(Pdcp_stats_rx_iat, 0, sizeof(Pdcp_stats_rx_iat));
  memset(Pdcp_stats_rx_outoforder, 0, sizeof(Pdcp_stats_rx_outoforder));
    
}

//-----------------------------------------------------------------------------
void pdcp_layer_cleanup (void)
//-----------------------------------------------------------------------------
{
  list_free (&pdcp_sdu_list);
  hashtable_destroy(pdcp_coll_p);
}

#ifdef PDCP_USE_RT_FIFO
EXPORT_SYMBOL(pdcp_2_nas_irq);
#endif //PDCP_USE_RT_FIFO
