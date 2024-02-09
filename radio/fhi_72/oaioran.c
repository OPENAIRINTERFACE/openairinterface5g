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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xran_fh_o_du.h"
#include "xran_compression.h"

// xran_cp_api.h uses SIMD, but does not include it
#include <immintrin.h>
#include "xran_cp_api.h"
#include "xran_sync_api.h"
#include "oran_isolate.h"
#include "oran-init.h"
#include "xran_common.h"
#include "oaioran.h"
#include <rte_ethdev.h>

#include "oran-config.h" // for g_kbar

#define USE_POLLING 1
// Declare variable useful for the send buffer function
volatile uint8_t first_call_set = 0;
volatile uint8_t first_rx_set = 0;
volatile int first_read_set = 0;

// Variable declaration useful for fill IQ samples from file
#define IQ_PLAYBACK_BUFFER_BYTES (XRAN_NUM_OF_SLOT_IN_TDD_LOOP * N_SYM_PER_SLOT * XRAN_MAX_PRBS * N_SC_PER_PRB * 4L)
/*
int rx_tti;
int rx_sym;
volatile uint32_t rx_cb_tti = 0;
volatile uint32_t rx_cb_frame = 0;
volatile uint32_t rx_cb_subframe = 0;
volatile uint32_t rx_cb_slot = 0;
*/

#define GetFrameNum(tti, SFNatSecStart, numSubFramePerSystemFrame, numSlotPerSubFrame) \
  ((((uint32_t)tti / ((uint32_t)numSubFramePerSystemFrame * (uint32_t)numSlotPerSubFrame)) + SFNatSecStart) & 0x3FF)
#define GetSlotNum(tti, numSlotPerSfn) ((uint32_t)tti % ((uint32_t)numSlotPerSfn))

int xran_is_prach_slot(uint8_t PortId, uint32_t subframe_id, uint32_t slot_id);
#include "common/utils/LOG/log.h"

#ifndef USE_POLLING
extern notifiedFIFO_t oran_sync_fifo;
#else
volatile oran_sync_info_t oran_sync_info;
#endif
void oai_xran_fh_rx_callback(void *pCallbackTag, xran_status_t status)
{
  struct xran_cb_tag *callback_tag = (struct xran_cb_tag *)pCallbackTag;
  uint64_t second;
  uint32_t tti;
  uint32_t frame;
  uint32_t subframe;
  uint32_t slot, slot2;
  uint32_t rx_sym;

  static int32_t last_slot = -1;
  static int32_t last_frame = -1;
  struct xran_device_ctx *xran_ctx = xran_dev_get_ctx();
  const struct xran_fh_init *fh_init = &xran_ctx->fh_init;
  int num_ports = fh_init->xran_ports;

  const struct xran_fh_config *fh_config = &xran_ctx->fh_cfg;
  const int slots_per_subframe = 1 << fh_config->frame_conf.nNumerology;

  static int rx_RU[XRAN_PORTS_NUM][160] = {0};
  uint32_t rx_tti = callback_tag->slotiId;

  tti = xran_get_slot_idx_from_tti(rx_tti, &frame, &subframe, &slot, &second);

  rx_sym = callback_tag->symbol;
  uint32_t ru_id = callback_tag->oXuId;
  if (rx_sym == 7) {
    if (first_call_set) {
      if (!first_rx_set) {
        LOG_I(NR_PHY, "first_rx is set (num_ports %d)\n", num_ports);
      }
      first_rx_set = 1;
      if (first_read_set == 1) {
        slot2 = slot + (subframe * slots_per_subframe);
        rx_RU[ru_id][slot2] = 1;
        if (last_frame > 0 && frame > 0
            && ((slot2 > 0 && last_frame != frame) || (slot2 == 0 && last_frame != ((1024 + frame - 1) & 1023))))
          LOG_E(PHY, "Jump in frame counter last_frame %d => %d, slot %d\n", last_frame, frame, slot2);
        for (int i = 0; i < num_ports; i++) {
          if (rx_RU[i][slot2] == 0)
            return;
        }
        for (int i = 0; i < num_ports; i++)
          rx_RU[i][slot2] = 0;

        if (last_slot == -1 || slot2 != last_slot) {
#ifndef USE_POLLING
          notifiedFIFO_elt_t *req = newNotifiedFIFO_elt(sizeof(oran_sync_info_t), 0, &oran_sync_fifo, NULL);
          oran_sync_info_t *info = (oran_sync_info_t *)NotifiedFifoData(req);
          info->sl = slot2;
          info->f = frame;
          LOG_D(PHY, "Push %d.%d.%d (slot %d, subframe %d,last_slot %d)\n", frame, info->sl, slot, ru_id, subframe, last_slot);
#else
          LOG_D(PHY, "Writing %d.%d.%d (slot %d, subframe %d,last_slot %d)\n", frame, slot2, ru_id, slot, subframe, last_slot);
          oran_sync_info.tti = tti;
          oran_sync_info.sl = slot2;
          oran_sync_info.f = frame;
#endif
#ifndef USE_POLLING
          pushNotifiedFIFO(&oran_sync_fifo, req);
#else
#endif
        } else
          LOG_E(PHY, "Cannot Push %d.%d.%d (slot %d, subframe %d,last_slot %d)\n", frame, slot2, ru_id, slot, subframe, last_slot);
        last_slot = slot2;
        last_frame = frame;
      } // first_read_set == 1
    } // first_call_set
  } // rx_sym == 7
}
void oai_xran_fh_srs_callback(void *pCallbackTag, xran_status_t status)
{
  rte_pause();
}
void oai_xran_fh_rx_prach_callback(void *pCallbackTag, xran_status_t status)
{
  rte_pause();
}

int oai_physide_dl_tti_call_back(void *param)
{
  if (!first_call_set)
    printf("first_call set from phy cb first_call_set=%p\n", &first_call_set);
  first_call_set = 1;
  return 0;
}

int oai_physide_ul_half_slot_call_back(void *param)
{
  rte_pause();
  return 0;
}

int oai_physide_ul_full_slot_call_back(void *param)
{
  rte_pause();
  return 0;
}

int read_prach_data(ru_info_t *ru, int frame, int slot)
{
  /* calculate tti and subframe_id from frame, slot num */
  int sym_idx = 0;

  struct xran_device_ctx *xran_ctx = xran_dev_get_ctx();
  struct xran_prach_cp_config *pPrachCPConfig = &(xran_ctx->PrachCPConfig);
  struct xran_ru_config *ru_conf = &(xran_ctx->fh_cfg.ru_conf);
  int slots_per_frame = 10 << xran_ctx->fh_cfg.frame_conf.nNumerology;
  int slots_per_subframe = 1 << xran_ctx->fh_cfg.frame_conf.nNumerology;

  int tti = slots_per_frame * (frame) + (slot);
  uint32_t subframe = slot / slots_per_subframe;
  uint32_t is_prach_slot = xran_is_prach_slot(0, subframe, (slot % slots_per_subframe));

  int nb_rx_per_ru = ru->nb_rx / xran_ctx->fh_init.xran_ports;
  /* If it is PRACH slot, copy prach IQ from XRAN PRACH buffer to OAI PRACH buffer */
  if (is_prach_slot) {
    for (sym_idx = 0; sym_idx < pPrachCPConfig->numSymbol; sym_idx++) {
      for (int aa = 0; aa < ru->nb_rx; aa++) {
        int16_t *dst, *src;
        int idx = 0;
        xran_ctx = xran_dev_get_ctx_by_id(aa / nb_rx_per_ru);
        dst = ru->prach_buf[aa]; // + (sym_idx*576));
        src = (int16_t *)((uint8_t *)xran_ctx->sFHPrachRxBbuIoBufCtrlDecomp[tti % XRAN_N_FE_BUF_LEN][0][aa % nb_rx_per_ru]
                              .sBufferList.pBuffers[sym_idx]
                              .pData);

        /* convert Network order to host order */
        if (ru_conf->compMeth_PRACH == XRAN_COMPMETHOD_NONE) {
          if (sym_idx == 0) {
            for (idx = 0; idx < 139 * 2; idx++) {
              dst[idx] = ((int16_t)ntohs(src[idx + g_kbar]));
            }
          } else {
            for (idx = 0; idx < 139 * 2; idx++) {
              dst[idx] += ((int16_t)ntohs(src[idx + g_kbar]));
            }
          }
        } else if (ru_conf->compMeth_PRACH == XRAN_COMPMETHOD_BLKFLOAT) {
          struct xranlib_decompress_request bfp_decom_req;
          struct xranlib_decompress_response bfp_decom_rsp;

          int16_t local_dst[12 * 2 * N_SC_PER_PRB] __attribute__((aligned(64)));

          int payload_len = (3 * ru_conf->iqWidth_PRACH + 1) * 12; // 12 = closest number of PRBs to 139 REs

          memset(&bfp_decom_req, 0, sizeof(struct xranlib_decompress_request));
          memset(&bfp_decom_rsp, 0, sizeof(struct xranlib_decompress_response));

          bfp_decom_req.data_in = (int8_t *)src;
          bfp_decom_req.numRBs = 12; // closest number of PRBs to 139 REs
          bfp_decom_req.len = payload_len;
          bfp_decom_req.compMethod = XRAN_COMPMETHOD_BLKFLOAT;
          bfp_decom_req.iqWidth = ru_conf->iqWidth_PRACH;

          bfp_decom_rsp.data_out = (int16_t *)local_dst;
          bfp_decom_rsp.len = 0;
          xranlib_decompress_avx512(&bfp_decom_req, &bfp_decom_rsp);
          // note: this is hardwired for 139 point PRACH sequence, kbar=2
          if (sym_idx == 0) //
            for (idx = 0; idx < (139 * 2); idx++)
              dst[idx] = local_dst[idx + g_kbar];
          else
            for (idx = 0; idx < (139 * 2); idx++)
              dst[idx] += (local_dst[idx + g_kbar]);
        } // COMPMETHOD_BLKFLOAT
      } // aa
    } // symb_indx
  } // is_prach_slot
  return (0);
}

int xran_fh_rx_read_slot(ru_info_t *ru, int *frame, int *slot)
{
  void *ptr = NULL;
  int32_t *pos = NULL;
  int idx = 0;
  static int last_slot = -1;
  first_read_set = 1;

  static int64_t old_rx_counter[XRAN_PORTS_NUM] = {0};
  static int64_t old_tx_counter[XRAN_PORTS_NUM] = {0};
  struct xran_common_counters x_counters[XRAN_PORTS_NUM];
  static int outcnt = 0;
#ifndef USE_POLLING
  // pull next even from oran_sync_fifo
  notifiedFIFO_elt_t *res = pollNotifiedFIFO(&oran_sync_fifo);
  while (res == NULL) {
    res = pollNotifiedFIFO(&oran_sync_fifo);
  }
  oran_sync_info_t *info = (oran_sync_info_t *)NotifiedFifoData(res);

  *slot = info->sl;
  *frame = info->f;
  delNotifiedFIFO_elt(res);
#else
  LOG_D(PHY, "In  xran_fh_rx_read_slot, first_rx_set %d\n", first_rx_set);
  while (first_rx_set == 0) {
  }

  *slot = oran_sync_info.sl;
  *frame = oran_sync_info.f;
  uint32_t tti_in = oran_sync_info.tti;

  LOG_D(PHY, "oran slot %d, last_slot %d\n", *slot, last_slot);
  int cnt = 0;
  // while (*slot == last_slot)  {
  while (tti_in == oran_sync_info.tti) {
    //*slot = oran_sync_info.sl;
    cnt++;
  }
  LOG_D(PHY, "cnt %d, Reading %d.%d\n", cnt, *frame, *slot);
  last_slot = *slot;
#endif
  // return(0);

  struct xran_device_ctx *xran_ctx = xran_dev_get_ctx();
  int slots_per_frame = 10 << xran_ctx->fh_cfg.frame_conf.nNumerology;

  int tti = slots_per_frame * (*frame) + (*slot);

  read_prach_data(ru, *frame, *slot);

  const struct xran_fh_init *fh_init = &xran_ctx->fh_init;
  int nPRBs = xran_ctx->fh_cfg.nULRBs;
  int fftsize = 1 << xran_ctx->fh_cfg.ru_conf.fftSize;

  int slot_offset_rxdata = 3 & (*slot);
  uint32_t slot_size = 4 * 14 * fftsize;
  uint8_t *rx_data = (uint8_t *)ru->rxdataF[0];
  uint8_t *start_ptr = NULL;
  int nb_rx_per_ru = ru->nb_rx / fh_init->xran_ports;
  for (uint16_t cc_id = 0; cc_id < 1 /*nSectorNum*/; cc_id++) { // OAI does not support multiple CC yet.
    for (uint8_t ant_id = 0; ant_id < ru->nb_rx; ant_id++) {
      rx_data = (uint8_t *)ru->rxdataF[ant_id];
      start_ptr = rx_data + (slot_size * slot_offset_rxdata);
      xran_ctx = xran_dev_get_ctx_by_id(ant_id / nb_rx_per_ru);
      const struct xran_fh_config *fh_config = &xran_ctx->fh_cfg;
      int tdd_period = fh_config->frame_conf.nTddPeriod;
      int slot_in_period = *slot % tdd_period;
      if (fh_config->frame_conf.sSlotConfig[slot_in_period].nSymbolType[XRAN_NUM_OF_SYMBOL_PER_SLOT - 1] == 0)
        continue;
      // skip processing this slot if the last symbol in the slot is TX
      // (no RX in this slot)
      // This loop would better be more inner to avoid confusion and maybe also errors.
      for (int32_t sym_idx = 0; sym_idx < XRAN_NUM_OF_SYMBOL_PER_SLOT; sym_idx++) {
        /* the callback is for mixed and UL slots. In mixed, we have to
         * skip DL and guard symbols. */
        if (fh_config->frame_conf.sSlotConfig[slot_in_period].nSymbolType[sym_idx] != 1 /* UL */)
          continue;

        uint8_t *pData;
        uint8_t *pPrbMapData = xran_ctx->sFrontHaulRxPrbMapBbuIoBufCtrl[tti % XRAN_N_FE_BUF_LEN][cc_id][ant_id % nb_rx_per_ru]
                                   .sBufferList.pBuffers->pData;
        struct xran_prb_map *pPrbMap = (struct xran_prb_map *)pPrbMapData;

        struct xran_prb_elm *pRbElm = &pPrbMap->prbMap[0];
        struct xran_section_desc *p_sec_desc = pRbElm->p_sec_desc[sym_idx][0];
        uint32_t one_rb_size =
            (((pRbElm->iqWidth == 0) || (pRbElm->iqWidth == 16)) ? (N_SC_PER_PRB * 2 * 2) : (3 * pRbElm->iqWidth + 1));
        if (fh_init->mtu < pRbElm->nRBSize * one_rb_size)
          pData = xran_ctx->sFrontHaulRxBbuIoBufCtrl[tti % XRAN_N_FE_BUF_LEN][cc_id][ant_id % nb_rx_per_ru]
                      .sBufferList.pBuffers[sym_idx % XRAN_NUM_OF_SYMBOL_PER_SLOT]
                      .pData;
        else
          pData = p_sec_desc->pData;
        ptr = pData;
        pos = (int32_t *)(start_ptr + (4 * sym_idx * fftsize));
        if (ptr == NULL || pos == NULL)
          continue;

        struct xran_prb_map *pRbMap = pPrbMap;

        uint32_t idxElm = 0;
        uint8_t *src = (uint8_t *)ptr;
        int16_t payload_len = 0;


        LOG_D(PHY, "pRbMap->nPrbElm %d\n", pRbMap->nPrbElm);
        for (idxElm = 0; idxElm < pRbMap->nPrbElm; idxElm++) {
          LOG_D(PHY,
                "prbMap[%d] : PRBstart %d nPRBs %d\n",
                idxElm,
                pRbMap->prbMap[idxElm].nRBStart,
                pRbMap->prbMap[idxElm].nRBSize);
          pRbElm = &pRbMap->prbMap[idxElm];
          int pos_len = 0;
          int neg_len = 0;

          if (pRbElm->nRBStart < (nPRBs >> 1)) // there are PRBs left of DC
            neg_len = min((nPRBs * 6) - (pRbElm->nRBStart * 12), pRbElm->nRBSize * N_SC_PER_PRB);
          pos_len = (pRbElm->nRBSize * N_SC_PER_PRB) - neg_len;

          src = pData;
          // Calculation of the pointer for the section in the buffer.
          // positive half
          uint8_t *dst1 = (uint8_t *)(pos + (neg_len == 0 ? ((pRbElm->nRBStart * N_SC_PER_PRB) - (nPRBs * 6)) : 0));
          // negative half
          uint8_t *dst2 = (uint8_t *)(pos + (pRbElm->nRBStart * N_SC_PER_PRB) + fftsize - (nPRBs * 6));
          int32_t local_dst[pRbElm->nRBSize * N_SC_PER_PRB] __attribute__((aligned(64)));
          if (pRbElm->compMethod == XRAN_COMPMETHOD_NONE) {
            // NOTE: gcc 11 knows how to generate AVX2 for this!
            for (idx = 0; idx < pRbElm->nRBSize * N_SC_PER_PRB * 2; idx++)
              ((int16_t *)local_dst)[idx] = ((int16_t)ntohs(((uint16_t *)src)[idx])) >> 2;
            memcpy((void *)dst2, (void *)local_dst, neg_len * 4);
            memcpy((void *)dst1, (void *)&local_dst[neg_len], pos_len * 4);
          } else if (pRbElm->compMethod == XRAN_COMPMETHOD_BLKFLOAT) {
            struct xranlib_decompress_request bfp_decom_req;
            struct xranlib_decompress_response bfp_decom_rsp;

            payload_len = (3 * pRbElm->iqWidth + 1) * pRbElm->nRBSize;

            memset(&bfp_decom_req, 0, sizeof(struct xranlib_decompress_request));
            memset(&bfp_decom_rsp, 0, sizeof(struct xranlib_decompress_response));

            bfp_decom_req.data_in = (int8_t *)src;
            bfp_decom_req.numRBs = pRbElm->nRBSize;
            bfp_decom_req.len = payload_len;
            bfp_decom_req.compMethod = pRbElm->compMethod;
            bfp_decom_req.iqWidth = pRbElm->iqWidth;

            bfp_decom_rsp.data_out = (int16_t *)local_dst;
            bfp_decom_rsp.len = 0;

            xranlib_decompress_avx512(&bfp_decom_req, &bfp_decom_rsp);
            memcpy((void *)dst2, (void *)local_dst, neg_len * 4);
            memcpy((void *)dst1, (void *)&local_dst[neg_len], pos_len * 4);
            outcnt++;
          } else {
            printf("pRbElm->compMethod == %d is not supported\n", pRbElm->compMethod);
            exit(-1);
          }
        }
      } // sym_ind
    } // ant_ind
  } // vv_inf
  if ((*frame & 0x7f) == 0 && *slot == 0 && xran_get_common_counters(gxran_handle, &x_counters[0]) == XRAN_STATUS_SUCCESS) {
    for (int o_xu_id = 0; o_xu_id < fh_init->xran_ports; o_xu_id++) {
      LOG_I(NR_PHY,
            "[%s%d][rx %7ld pps %7ld kbps %7ld][tx %7ld pps %7ld kbps %7ld][Total Msgs_Rcvd %ld]\n",
            "o-du ",
            o_xu_id,
            x_counters[o_xu_id].rx_counter,
            x_counters[o_xu_id].rx_counter - old_rx_counter[o_xu_id],
            x_counters[o_xu_id].rx_bytes_per_sec * 8 / 1000L,
            x_counters[o_xu_id].tx_counter,
            x_counters[o_xu_id].tx_counter - old_tx_counter[o_xu_id],
            x_counters[o_xu_id].tx_bytes_per_sec * 8 / 1000L,
            x_counters[o_xu_id].Total_msgs_rcvd);
      for (int rxant = 0; rxant < ru->nb_rx / fh_init->xran_ports; rxant++)
        LOG_I(NR_PHY,
              "[%s%d][pusch%d %7ld prach%d %7ld]\n",
              "o_du",
              o_xu_id,
              rxant,
              x_counters[o_xu_id].rx_pusch_packets[rxant],
              rxant,
              x_counters[o_xu_id].rx_prach_packets[rxant]);
      if (x_counters[o_xu_id].rx_counter > old_rx_counter[o_xu_id])
        old_rx_counter[o_xu_id] = x_counters[o_xu_id].rx_counter;
      if (x_counters[o_xu_id].tx_counter > old_tx_counter[o_xu_id])
        old_tx_counter[o_xu_id] = x_counters[o_xu_id].tx_counter;
    }
  }
  return (0);
}

int xran_fh_tx_send_slot(ru_info_t *ru, int frame, int slot, uint64_t timestamp)
{
  int tti = /*frame*SUBFRAMES_PER_SYSTEMFRAME*SLOTNUM_PER_SUBFRAME+*/ 20 * frame
            + slot; // commented out temporarily to check that compilation of oran 5g is working.

  void *ptr = NULL;
  int32_t *pos = NULL;
  int idx = 0;

  struct xran_device_ctx *xran_ctx = xran_dev_get_ctx();
  const struct xran_fh_init *fh_init = &xran_ctx->fh_init;
  int nPRBs = xran_ctx->fh_cfg.nDLRBs;
  int fftsize = 1 << xran_ctx->fh_cfg.ru_conf.fftSize;
  int nb_tx_per_ru = ru->nb_tx / fh_init->xran_ports;

  for (uint16_t cc_id = 0; cc_id < 1 /*nSectorNum*/; cc_id++) { // OAI does not support multiple CC yet.
    for (uint8_t ant_id = 0; ant_id < ru->nb_tx; ant_id++) {
      xran_ctx = xran_dev_get_ctx_by_id(ant_id / nb_tx_per_ru);
      // This loop would better be more inner to avoid confusion and maybe also errors.
      for (int32_t sym_idx = 0; sym_idx < XRAN_NUM_OF_SYMBOL_PER_SLOT; sym_idx++) {
        uint8_t *pData = xran_ctx->sFrontHaulTxBbuIoBufCtrl[tti % XRAN_N_FE_BUF_LEN][cc_id][ant_id % nb_tx_per_ru]
                             .sBufferList.pBuffers[sym_idx % XRAN_NUM_OF_SYMBOL_PER_SLOT]
                             .pData;
        uint8_t *pPrbMapData = xran_ctx->sFrontHaulTxPrbMapBbuIoBufCtrl[tti % XRAN_N_FE_BUF_LEN][cc_id][ant_id % nb_tx_per_ru]
                                   .sBufferList.pBuffers->pData;
        struct xran_prb_map *pPrbMap = (struct xran_prb_map *)pPrbMapData;
        ptr = pData;
        pos = &ru->txdataF_BF[ant_id][sym_idx * fftsize];

        uint8_t *u8dptr;
        struct xran_prb_map *pRbMap = pPrbMap;
        int32_t sym_id = sym_idx % XRAN_NUM_OF_SYMBOL_PER_SLOT;
        if (ptr && pos) {
          uint32_t idxElm = 0;
          u8dptr = (uint8_t *)ptr;
          int16_t payload_len = 0;

          uint8_t *dst = (uint8_t *)u8dptr;

          struct xran_prb_elm *p_prbMapElm = &pRbMap->prbMap[idxElm];

          for (idxElm = 0; idxElm < pRbMap->nPrbElm; idxElm++) {
            struct xran_section_desc *p_sec_desc = NULL;
            p_prbMapElm = &pRbMap->prbMap[idxElm];
            p_sec_desc =
                // assumes one fragment per symbol
                p_prbMapElm->p_sec_desc[sym_id][0];

            dst = xran_add_hdr_offset(dst, p_prbMapElm->compMethod);

            if (p_sec_desc == NULL) {
              printf("p_sec_desc == NULL\n");
              exit(-1);
            }
            uint16_t *dst16 = (uint16_t *)dst;

            int pos_len = 0;
            int neg_len = 0;

            if (p_prbMapElm->nRBStart < (nPRBs >> 1)) // there are PRBs left of DC
              neg_len = min((nPRBs * 6) - (p_prbMapElm->nRBStart * 12), p_prbMapElm->nRBSize * N_SC_PER_PRB);
            pos_len = (p_prbMapElm->nRBSize * N_SC_PER_PRB) - neg_len;
            // Calculation of the pointer for the section in the buffer.
            // start of positive frequency component
            uint16_t *src1 = (uint16_t *)&pos[(neg_len == 0) ? ((p_prbMapElm->nRBStart * N_SC_PER_PRB) - (nPRBs * 6)) : 0];
            // start of negative frequency component
            uint16_t *src2 = (uint16_t *)&pos[(p_prbMapElm->nRBStart * N_SC_PER_PRB) + fftsize - (nPRBs * 6)];

            uint32_t local_src[p_prbMapElm->nRBSize * N_SC_PER_PRB] __attribute__((aligned(64)));
            memcpy((void *)local_src, (void *)src2, neg_len * 4);
            memcpy((void *)&local_src[neg_len], (void *)src1, pos_len * 4);
            if (p_prbMapElm->compMethod == XRAN_COMPMETHOD_NONE) {
              payload_len = p_prbMapElm->nRBSize * N_SC_PER_PRB * 4L;
              /* convert to Network order */
              // NOTE: ggc 11 knows how to generate AVX2 for this!
              for (idx = 0; idx < (pos_len + neg_len) * 2; idx++)
                ((uint16_t *)dst16)[idx] = htons(((uint16_t *)local_src)[idx]);
            } else if (p_prbMapElm->compMethod == XRAN_COMPMETHOD_BLKFLOAT) {
              struct xranlib_compress_request bfp_com_req;
              struct xranlib_compress_response bfp_com_rsp;
              payload_len = (3 * p_prbMapElm->iqWidth + 1) * p_prbMapElm->nRBSize;
              memset(&bfp_com_req, 0, sizeof(struct xranlib_compress_request));
              memset(&bfp_com_rsp, 0, sizeof(struct xranlib_compress_response));

              bfp_com_req.data_in = (int16_t *)local_src;
              bfp_com_req.numRBs = p_prbMapElm->nRBSize;
              bfp_com_req.len = payload_len;
              bfp_com_req.compMethod = p_prbMapElm->compMethod;
              bfp_com_req.iqWidth = p_prbMapElm->iqWidth;

              bfp_com_rsp.data_out = (int8_t *)dst;
              bfp_com_rsp.len = 0;

              xranlib_compress_avx512(&bfp_com_req, &bfp_com_rsp);

            } else {
              printf("p_prbMapElm->compMethod == %d is not supported\n", p_prbMapElm->compMethod);
              exit(-1);
            }

            p_sec_desc->iq_buffer_offset = RTE_PTR_DIFF(dst, u8dptr);
            p_sec_desc->iq_buffer_len = payload_len;

            dst += payload_len;
            dst = xran_add_hdr_offset(dst, p_prbMapElm->compMethod);
          }

          // The tti should be updated as it increased.
          pRbMap->tti_id = tti;

        } else {
          printf("ptr ==NULL\n");
          exit(-1); // fails here??
        }
      }
    }
  }
  return (0);
}
