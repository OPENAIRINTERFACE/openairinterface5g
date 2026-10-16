/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "wls_pnf.h"

#include <rte_eal.h>
#include <rte_string_fns.h>
#include <rte_common.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <pnf_p7.h>
#include <sys/time.h>

#include "wls_lib.h"
#include "pnf.h"
#include "nr_nfapi_p7.h"
#include "nfapi/open-nFAPI/fapi/inc/nr_fapi_p7_utils.h"

// A copy of the PNF P7 config is needed for the message processing thread, in other transport mechanisms where P7 is handled in a
// different thread, the config is passed to it upon the thread creation
nfapi_pnf_p7_config_t *wls_pnf_p7_config = NULL;

void wls_pnf_set_p7_config(void *p7_config)
{
  wls_pnf_p7_config = (nfapi_pnf_p7_config_t *)p7_config;
}

static WLS_MAC_CTX g_phy_wls;
uint8_t phy_dpdk_init(void);
uint8_t phy_wls_init(const char *dev_name, uint64_t nWlsMacMemSize, uint64_t nWlsPhyMemSize);
void phy_mac_recv();
void wls_mac_print_stats(void);

pnf_t *_this = NULL;

static PWLS_MAC_CTX wls_mac_get_ctx(void)
{
  return &g_phy_wls;
}

static nfapi_pnf_config_t *cfg;

void *wls_fapi_pnf_nr_start_thread(void *ptr)
{
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] IN WLS PNF NFAPI start thread %s\n", __FUNCTION__);
  cfg = (nfapi_pnf_config_t *)ptr;
  wls_fapi_nr_pnf_start();
  return (void *)0;
}

void wls_pnf_close(pthread_t p5_thread)
{
  _this->terminate = 1;
  WLS_WakeUp(wls_mac_get_ctx()->hWls);
  pthread_join(p5_thread, NULL);
  if (WLS_Close(wls_mac_get_ctx()->hWls) != 0) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] WLS_CLose call failed...\n");
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] WLS_CLose called successfully...\n");
  }
  rte_eal_cleanup();
}

void wls_fapi_nr_pnf_stop()
{
  AssertFatal(cfg->nr_stop_req, "STOP.request Handler should be set!");
  nfapi_nr_stop_request_scf_t req = {0};
  req.header.phy_id = 0;
  req.header.message_id = NFAPI_NR_PHY_MSG_TYPE_STOP_REQUEST;
  cfg->nr_stop_req(cfg, 0, &req);
}

int wls_fapi_nr_pnf_start()
{
  int64_t ret;
  // Verify that config is not null
  if (cfg == 0)
    return -1;

  NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);

  _this = (pnf_t *)(cfg);
  _this->terminate = 0;
  // Init PNF config
  nfapi_pnf_phy_config_t *phy = malloc_or_fail(sizeof(nfapi_pnf_phy_config_t));
  memset(phy, 0, sizeof(nfapi_pnf_phy_config_t));

  phy->state = NFAPI_PNF_PHY_IDLE;
  phy->phy_id = 0;
  phy->next = (_this->_public).phys;
  (_this->_public).phys = phy;
  _this->_public.state = NFAPI_PNF_RUNNING;

  // DPDK init
  ret = phy_dpdk_init();
  if (!ret) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "[PNF]  DPDK Init - Failed\n");
    return false;
  }
  NFAPI_TRACE(NFAPI_TRACE_INFO, "\n[PHY] DPDK Init - Done\n");

  // WLS init
  ret = phy_wls_init(WLS_DEV_NAME, WLS_MAC_MEMORY_SIZE, WLS_PHY_MEMORY_SIZE);
  if (!ret) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "[PNF]  WLS Init - Failed\n");
    return false;
  }
  NFAPI_TRACE(NFAPI_TRACE_INFO, "\n[PHY] WLS Init - Done\n");

  // Start Receiving loop from the VNF
  NFAPI_TRACE(NFAPI_TRACE_INFO, "Start Receiving loop from the VNF\n");
  phy_mac_recv();
  // Should never get here, to be removed, align with present implementation of PNF loop
  NFAPI_TRACE(NFAPI_TRACE_INFO, "\n[PHY] Exiting...\n");

  return true;
}

uint8_t phy_dpdk_init(void)
{
  unsigned long i;
  char *argv[] = {"OAI_PNF", "--proc-type=primary", "--file-prefix", WLS_DEV_NAME, "--iova-mode=pa", "--no-pci"};
  int argc = RTE_DIM(argv);
  /* initialize EAL first */
  NFAPI_TRACE(NFAPI_TRACE_INFO, "\n[PHY] Calling rte_eal_init: ");

  for (i = 0; i < RTE_DIM(argv); i++) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s ", argv[i]);
  }

  NFAPI_TRACE(NFAPI_TRACE_INFO, "\n");

  if (rte_eal_init(argc, argv) < 0)
    rte_panic("Cannot init EAL\n");

  return true;
}

uint8_t phy_wls_init(const char *dev_name, uint64_t nWlsMacMemSize, uint64_t nWlsPhyMemSize)
// uint8_t phy_wls_init(const char *dev_name,  uint64_t nBlockSize)
{
  PWLS_MAC_CTX pWls = wls_mac_get_ctx();
  pWls->nTotalAllocCnt = 0;
  pWls->nTotalFreeCnt = 0;
  pWls->nTotalUlBufAllocCnt = 0;
  pWls->nTotalUlBufFreeCnt = 0;
  pWls->nTotalDlBufAllocCnt = 0;
  pWls->nTotalDlBufFreeCnt = 0;

  pWls->hWls = WLS_Open(dev_name, WLS_SLAVE_CLIENT, &nWlsMacMemSize, &nWlsPhyMemSize, WLS_UL_ENQUEUE_SIZE);
  if (pWls->hWls) {
    /* allocate chuck of memory */
    if (WLS_Alloc(pWls->hWls, nWlsMacMemSize + nWlsPhyMemSize) != NULL) {
      NFAPI_TRACE(NFAPI_TRACE_DEBUG, "WLS Memory allocated successfully\n");
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unable to alloc WLS Memory\n");
      return false;
    }
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "can't open WLS instance");
    return false;
  }
  return true;
}

bool wls_pnf_nr_send_p5_message(pnf_t *pnf, nfapi_nr_p4_p5_message_header_t *msg, uint32_t msg_len)
{
  int packed_len = cfg->pack_func(msg, msg_len, pnf->tx_message_buffer, sizeof(pnf->tx_message_buffer), &pnf->_public.codec_config);

  if (packed_len < 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p5_message_pack failed (%d)\n", packed_len);
    return false;
  }

  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "fapi_nr_p5_message_pack succeeded having packed %d bytes\n", packed_len);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "in msg, msg_len is %d\n", msg->message_length);
  PWLS_MAC_CTX pWls = wls_mac_get_ctx();


  return wls_send_fapi_msg(pWls, msg->message_id, packed_len, pnf->tx_message_buffer);
}

bool wls_pnf_nr_send_p7_message(pnf_p7_t *pnf_p7, nfapi_nr_p7_message_header_t *msg, uint32_t msg_len)
{
  UNUSED(msg_len);
  if (pnf_p7->terminate) {
    // don't send any further P7 messages after terminating
    return false;
  }
  if (!isFAPIMessageIDValid(msg->message_id)) {
    return false;
  }

  // if it's an nFAPI message, ignore
  if ((msg->message_id == NFAPI_NR_PHY_MSG_TYPE_UL_NODE_SYNC || msg->message_id == NFAPI_NR_PHY_MSG_TYPE_DL_NODE_SYNC
       || msg->message_id == NFAPI_NR_PHY_MSG_TYPE_TIMING_INFO)) {
    return false;
  }

  PWLS_MAC_CTX pWls = wls_mac_get_ctx();
  int num_avail_blocks = WLS_NumBlocks(pWls->hWls);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "num_avail_blocks is %d\n", num_avail_blocks);
  while (num_avail_blocks < 2) {
    num_avail_blocks = WLS_NumBlocks(pWls->hWls);
    if (_this->terminate) {
      return false;
    }
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "num_avail_blocks is %d\n", num_avail_blocks);
  }
  /* get PA blocks for header and msg */
  const uint64_t pa_hdr = dequeueBlock(pWls);
  AssertFatal(pa_hdr, "WLS_DequeueBlock failed for pa_hdr\n");
  const uint64_t pa_msg = dequeueBlock(pWls);
  AssertFatal(pa_msg, "WLS_DequeueBlock failed for pa_msg\n");
  p_fapi_api_queue_elem_t headerElem = WLS_PA2VA(pWls->hWls, pa_hdr);
  AssertFatal(headerElem, "WLS_PA2VA failed for headerElem\n");
  p_fapi_api_queue_elem_t fapiMsgElem = WLS_PA2VA(pWls->hWls, pa_msg);
  AssertFatal(fapiMsgElem, "WLS_PA2VA failed for fapiMsgElem\n");
  const uint8_t wls_header[] = {1, 0}; // num_messages ,  opaque_handle
  fill_fapi_list_elem(headerElem, fapiMsgElem, FAPI_VENDOR_MSG_HEADER_IND, 1, 2);
  memcpy((uint8_t *)(headerElem + 1), wls_header, 2);

  const int packed_len = ((nfapi_pnf_p7_config_t *)pnf_p7)->pack_func(msg, (fapiMsgElem + 1), NFAPI_MAX_PACKED_MESSAGE_SIZE, NULL);

  if (packed_len < 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p7_message_pack failed (%d)\n", packed_len);
    return false;
  }
  fill_fapi_list_elem(fapiMsgElem, NULL, msg->message_id, 1, packed_len);
  const uint8_t retval = wls_msg_send(pWls->hWls, headerElem);
  return retval;
}

static void procPhyMessages(uint32_t msg_size, void *msg_buf, uint16_t msg_id)
{

  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PHY] Received Msg ID 0x%02x Length %d\n", msg_id, msg_size);
  if (msg_buf == NULL || _this == NULL) {
    AssertFatal(msg_buf != NULL && _this != NULL, "%s: NULL parameters\n", __FUNCTION__);
  }
  // Add the size of the header to the message_size, because the size in the header does not include the header length
  msg_size += NFAPI_NR_P5_HEADER_LENGTH;
  switch (msg_id) {
    case NFAPI_NR_PHY_MSG_TYPE_PARAM_REQUEST ... NFAPI_NR_PHY_MSG_TYPE_STOP_REQUEST:
      pnf_nr_handle_p5_message(_this, msg_buf, msg_size);
      break;
    case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST ... NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION: {
      pnf_nr_handle_p7_message(msg_buf, msg_size, (pnf_p7_t *)wls_pnf_p7_config, 0);
      break;
    }
    default:
      printf("\n Unknown Msg ID 0x%02x\n", msg_id);
      break;
  }
}

void phy_mac_recv()
{
  /* Number of Memory blocks to get */
  uint32_t msgSize;
  uint16_t msgType;
  uint16_t flags;
  PWLS_MAC_CTX pWls = wls_mac_get_ctx();
  // This flag is used to determine whether to return the received blocks to the PNF
  bool to_return = false;
  while (_this->terminate == 0) {
    int numMsgToGet = WLS_Wait(pWls->hWls);
    if (numMsgToGet == 0 || _this->terminate) {
      continue;
    }
    uint64_t PAs[numMsgToGet];
    uint8_t pa_idx = 0;
    while (numMsgToGet--) {
      PAs[pa_idx] = WLS_Get(pWls->hWls, &msgSize, &msgType, &flags);
      pa_idx++;
    }
    for (int i = 0; i < pa_idx; i++) {
      p_fapi_api_queue_elem_t msg = WLS_PA2VA(pWls->hWls, PAs[i]);
      if (msg->msg_type != FAPI_VENDOR_MSG_HEADER_IND) {
        uint8_t *msgt = (uint8_t *)(msg + 1);
        uint32_t len = msgt[7] | (msgt[6] << 8) | (msgt[5] << 16) | (msgt[4] << 24);
        procPhyMessages(len, (void *)(msg + 1), msg->msg_type);
      } else {
        // We check the header block to determine if it's the OAI VNF, if so, we're required to return the pages for enqueueing
        if (!to_return) {
          uint8_t *msgt = (uint8_t *)(msg + 1);
          if (msgt[1] == 0xff) {
            to_return = true;
          }
        }
      }
    }
    // A STOP.indication might have been processed, which set's terminate to 1
    // don't return pages in that case
    if (_this->terminate) {
      break;
    }
    if (to_return) {
      // We only return the pages to be enqueued in case we're using the OAI VNF,
      // The Radisys O-DU handles enqueueing the blocks differently, and also doesn't handle receiving a single block
      for (int i = 0; i < pa_idx; i++) {
        wls_return_msg_to_master(pWls, PAs[i]);
      }
    }
  }
}
