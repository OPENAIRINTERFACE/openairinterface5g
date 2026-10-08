/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "wls_common.h"
#include "wls_lib.h"
void fill_fapi_list_elem(p_fapi_api_queue_elem_t currElem, p_fapi_api_queue_elem_t nextElem, const uint8_t msgType, const uint8_t numMsgInBlock, const uint32_t alignOffset)
{
    currElem->msg_type             = msgType;
    currElem->num_message_in_block = numMsgInBlock;
    currElem->align_offset         = alignOffset;
    currElem->msg_len              = numMsgInBlock * alignOffset;
    currElem->p_next               = nextElem;
    currElem->p_tx_data_elm_list   = NULL;
    currElem->time_stamp           = 0;
}

uint64_t dequeueBlock(PWLS_MAC_CTX pWls)
{
  uint64_t pa = WLS_DequeueBlock(pWls->hWls);
  AssertFatal(pa, "WLS_DequeueBlock failed for pa_hdr\n");
  unsigned long* va = WLS_PA2VA(pWls->hWls, pa);
  AssertFatal(va, "WLS_PA2VA failed for headerElem\n");
  while (*va != -1) {
    pa = WLS_DequeueBlock(pWls->hWls);
    AssertFatal(pa, "WLS_DequeueBlock failed for pa_hdr\n");
    va = WLS_PA2VA(pWls->hWls, pa);
  }
  return pa;
}

uint8_t wls_msg_send(PWLS_MAC_CTX pWls, p_fapi_api_queue_elem_t currMsg)
{
  uint32_t msgLen = 0;

  AssertFatal(currMsg, "currMsg can't be NULL");
  AssertFatal(currMsg->p_next, "There needs to be at least 2 blocks to send!");
  msgLen = currMsg->msg_len + sizeof(fapi_api_queue_elem_t);
  // Send the first block
  if (WLS_Put(pWls, WLS_VA2PA(pWls, currMsg), msgLen, currMsg->msg_type, WLS_SG_FIRST) != 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failure in sending the header block\n");
    return false;
  }
  // Iterate over the linked list, until a NULL block or a block set to 0xFFFFFFFFFFFFFFFF ( invalid )
  while (((currMsg = currMsg->p_next)) && (uint64_t)(currMsg) != -1) {
    msgLen = currMsg->msg_len + sizeof(fapi_api_queue_elem_t);
    /* Send the block with appropriate flags, either WLS_SG_NEXT or WLS_SG_LAST */
    const unsigned short flags = currMsg->p_next != NULL ? WLS_SG_NEXT : WLS_SG_LAST;
    if (WLS_Put(pWls, WLS_VA2PA(pWls, currMsg), msgLen, currMsg->msg_type, flags) != 0) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failure in sending the message block\n");
      return false;
    }
  }
  return true;
}

void wls_return_msg_to_master(PWLS_MAC_CTX pWls,const uint64_t msg_PA)
{
  p_fapi_api_queue_elem_t fapiMsgElem = WLS_PA2VA(pWls->hWls, msg_PA);
  AssertFatal(fapiMsgElem, "WLS_PA2VA failed for fapiMsgElem\n");
  const uint8_t wls_header[] = {1, 0}; // num_messages ,  opaque_handle
  // No need to return the message contents, just the block
  fill_fapi_list_elem(fapiMsgElem, NULL, FAPI_VENDOR_MSG_HEADER_IND, 1, 2);
  memcpy((uint8_t *)(fapiMsgElem + 1), wls_header, 2);
  const uint32_t msgLen = fapiMsgElem->msg_len + sizeof(fapi_api_queue_elem_t);
  /* Send the block with appropriate only WLS_SG_FIRST flag, since we send only 1 block*/
  if (WLS_Put(pWls->hWls, msg_PA, msgLen, FAPI_VENDOR_MSG_HEADER_IND, WLS_SG_FIRST) != 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR,"Failure in sending block back to master\n");
  }
}

uint8_t wls_send_fapi_msg(PWLS_MAC_CTX pWls, const uint16_t message_id, const int packed_len, const uint8_t *message)
{
  int num_avail_blocks = WLS_NumBlocks(pWls->hWls);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG,"num_avail_blocks is %d\n", num_avail_blocks);
  int counter = 0;
  while (num_avail_blocks < 2) {
    num_avail_blocks = WLS_NumBlocks(pWls->hWls);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG,"num_avail_blocks is %d\n", num_avail_blocks);
    if (message_id == NFAPI_NR_PHY_MSG_TYPE_STOP_INDICATION) {
      // In the case of the STOP.indication, it can happen that this is called after the VNF stopped
      // For example, if it crashed for some reason, in that case, implement a timeout and return
      usleep(10 * 1000);
      counter++;
      if (counter == 10) {
        return true;
      }
    }
  }
  /* get PA blocks for header and msg */
  uint64_t pa_hdr = dequeueBlock(pWls);
  AssertFatal(pa_hdr, "WLS_DequeueBlock failed for pa_hdr\n");
  uint64_t pa_msg = dequeueBlock(pWls);
  AssertFatal(pa_msg, "WLS_DequeueBlock failed for pa_msg\n");
  p_fapi_api_queue_elem_t headerElem = WLS_PA2VA(pWls->hWls, pa_hdr);
  AssertFatal(headerElem, "WLS_PA2VA failed for headerElem\n");
  p_fapi_api_queue_elem_t fapiMsgElem = WLS_PA2VA(pWls->hWls, pa_msg);
  AssertFatal(fapiMsgElem, "WLS_PA2VA failed for fapiMsgElem\n");
  fill_fapi_list_elem(fapiMsgElem, NULL, message_id, 1, packed_len);
  memcpy((uint8_t *)(fapiMsgElem + 1), message, packed_len);
  uint8_t wls_header[] = {1, 0}; // num_messages ,  opaque_handle
  if (NFAPI_MODE == NFAPI_MODE_VNF) {
    // Use the opaque handle to signal to our PNF to not progress the FAPI PNF state machine
    wls_header[1] = 0xff;
  }
  fill_fapi_list_elem(headerElem, fapiMsgElem, FAPI_VENDOR_MSG_HEADER_IND, 1, 2);
  memcpy((uint8_t *)(headerElem + 1), wls_header, 2);
  uint8_t retval = wls_msg_send(pWls->hWls, headerElem);
  return retval;
}

