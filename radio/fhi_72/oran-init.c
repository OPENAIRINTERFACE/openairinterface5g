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

#include "xran_fh_o_du.h"
#include "xran_pkt.h"
#include "xran_pkt_up.h"
#include "rte_ether.h"

#include "oran-config.h"
#include "oran-init.h"
#include "oaioran.h"

#include "common/utils/assertions.h"
#include "common_lib.h"

/* PRACH data samples are 32 bits wide (16bits for I/Q). Each packet contains
 * 840 samples for long sequence or 144 for short sequence. The payload length
 * is 840*16*2/8 octets.*/
#ifdef FCN_1_2_6_EARLIER
#define PRACH_PLAYBACK_BUFFER_BYTES (144 * 4L)
#else
#define PRACH_PLAYBACK_BUFFER_BYTES (840 * 4L)
#endif

// structure holding allocated memory for ports (multiple DUs) and sectors
// (multiple CCs)
static oran_port_instance_t gPortInst[XRAN_PORTS_NUM][XRAN_MAX_SECTOR_NR];
void *gxran_handle;

static uint32_t get_nSW_ToFpga_FTH_TxBufferLen(int mu, int sections)
{
  uint32_t xran_max_sections_per_slot = RTE_MAX(sections, XRAN_MIN_SECTIONS_PER_SLOT);
  uint32_t overhead = xran_max_sections_per_slot
                      * (RTE_PKTMBUF_HEADROOM + sizeof(struct rte_ether_hdr) + sizeof(struct xran_ecpri_hdr)
                         + sizeof(struct radio_app_common_hdr) + sizeof(struct data_section_hdr));
  if (mu <= 1) {
    return 13168 + overhead; /* 273*12*4 + 64* + ETH AND ORAN HDRs */
  } else if (mu == 3) {
    return 3328 + overhead;
  } else {
    assert(false && "numerology not supported\n");
  }
}

static uint32_t get_nFpgaToSW_FTH_RxBufferLen(int mu)
{
  /* note: previous code checked MTU:
   * mu <= 1: return mtu > XRAN_MTU_DEFAULT ? 13168 : XRAN_MTU_DEFAULT;
   * mu == 3: return mtu > XRAN_MTU_DEFAULT ? 3328 : XRAN_MTU_DEFAULT;
   * but I don't understand the interest: if the buffer is a big bigger, there
   * is no problem, or we could just set the MTU size as buffer size?!
   * Go with Max for the moment
   */
  if (mu <= 1) {
    return 13168; /* 273*12*4 + 64*/
  } else if (mu == 3) {
    return 3328;
  } else {
    assert(false && "numerology not supported\n");
  }
}

static struct xran_prb_map get_xran_prb_map_dl(const struct xran_fh_config *f)
{
  struct xran_prb_map prbmap = {
      .dir = XRAN_DIR_DL,
      .xran_port = 0,
      .band_id = 0,
      .cc_id = 0,
      .ru_port_id = 0,
      .tti_id = 0,
      .nPrbElm = 1,
  };
  struct xran_prb_elm *e = &prbmap.prbMap[0];
  e->nStartSymb = 0;
  e->numSymb = 14;
  e->nRBStart = 0;
  e->nRBSize = f->nDLRBs;
  e->nBeamIndex = 0;
  e->compMethod = f->ru_conf.compMeth;
  e->iqWidth = f->ru_conf.iqWidth;
  return prbmap;
}

static struct xran_prb_map get_xran_prb_map_ul(const struct xran_fh_config *f)
{
  struct xran_prb_map prbmap = {
      .dir = XRAN_DIR_UL,
      .xran_port = 0,
      .band_id = 0,
      .cc_id = 0,
      .ru_port_id = 0,
      .tti_id = 0,
      .start_sym_id = 0,
      .nPrbElm = 1,
  };
  struct xran_prb_elm *e = &prbmap.prbMap[0];
  e->nStartSymb = 0;
  e->numSymb = 14;
  e->nRBStart = 0;
  e->nRBSize = f->nULRBs;
  e->nBeamIndex = 0;
  e->compMethod = f->ru_conf.compMeth;
  e->iqWidth = f->ru_conf.iqWidth;
  return prbmap;
}

static uint32_t oran_allocate_uplane_buffers(
    void *instHandle,
    struct xran_buffer_list list[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN],
    struct xran_flat_buffer buf[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN][XRAN_NUM_OF_SYMBOL_PER_SLOT],
    uint32_t ant,
    uint32_t bufSize)
{
  xran_status_t status;
  uint32_t pool;
  uint32_t numBufs = XRAN_N_FE_BUF_LEN * ant * XRAN_NUM_OF_SYMBOL_PER_SLOT;
  status = xran_bm_init(instHandle, &pool, numBufs, bufSize);
  AssertFatal(XRAN_STATUS_SUCCESS == status, "Failed at xran_bm_init(), status %d\n", status);
  printf("xran_bm_init() hInstance %p poolIdx %d elements %d size %d\n", instHandle, pool, numBufs, bufSize);
  int count = 0;
  for (uint32_t a = 0; a < ant; ++a) {
    for (uint32_t j = 0; j < XRAN_N_FE_BUF_LEN; ++j) {
      list[a][j].pBuffers = &buf[a][j][0];
      for (uint32_t k = 0; k < XRAN_NUM_OF_SYMBOL_PER_SLOT; ++k) {
        struct xran_flat_buffer *fb = &list[a][j].pBuffers[k];
        fb->nElementLenInBytes = bufSize;
        fb->nNumberOfElements = 1;
        fb->nOffsetInBytes = 0;
        void *ptr;
        void *mb;
        status = xran_bm_allocate_buffer(instHandle, pool, &ptr, &mb);
        AssertFatal(XRAN_STATUS_SUCCESS == status && ptr != NULL && mb != NULL,
                    "Failed at xran_bm_allocate_buffer(), status %d\n",
                    status);
        count++;
        fb->pData = ptr;
        fb->pCtrl = mb;
        memset(ptr, 0, bufSize);
      }
    }
  }
  printf("xran_bm_allocate_buffer() hInstance %p poolIdx %d count %d\n", instHandle, pool, count);
  return pool;
}

typedef struct oran_mixed_slot {
  uint32_t idx;
  uint32_t num_dlsym;
  uint32_t num_ulsym;
  uint32_t start_ulsym;
} oran_mixed_slot_t;
static oran_mixed_slot_t get_mixed_slot_info(const struct xran_frame_config *fconfig)
{
  oran_mixed_slot_t info = {0};
  for (size_t sl = 0; sl < fconfig->nTddPeriod; ++sl) {
    info.num_dlsym = info.num_ulsym = 0;
    for (size_t sym = 0; sym < XRAN_NUM_OF_SYMBOL_PER_SLOT; ++sym) {
      uint8_t t = fconfig->sSlotConfig[sl].nSymbolType[sym];
      if (t == 0 /* DL */) {
        info.num_dlsym++;
      } else if (t == 1 /* UL */) {
        if (info.num_ulsym == 0)
          info.start_ulsym = sym;
        info.num_ulsym++;
      } else if (t == 2 /* Mixed */) {
        info.idx = sl;
      } else {
        AssertFatal(false, "unknown symbol type %d\n", t);
      }
    }
    if (info.idx > 0)
      return info;
  }
  AssertFatal(false, "could not find mixed slot!\n");
  return info;
}

typedef struct oran_cplane_prb_config {
  uint8_t nTddPeriod;
  uint32_t mixed_slot_index;
  struct xran_prb_map slotMap;
  struct xran_prb_map mixedSlotMap;
} oran_cplane_prb_config;

static void oran_allocate_cplane_buffers(void *instHandle,
                                         struct xran_buffer_list list[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN],
                                         struct xran_flat_buffer buf[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN],
                                         uint32_t ant,
                                         uint32_t sect,
                                         uint32_t size_of_prb_map,
                                         const oran_cplane_prb_config *prb_conf)
{
  xran_status_t status;
  uint32_t poolSec;
  uint32_t numBufsSec = XRAN_N_FE_BUF_LEN * ant * XRAN_NUM_OF_SYMBOL_PER_SLOT * sect * XRAN_MAX_FRAGMENT;
  uint32_t bufSizeSec = sizeof(struct xran_section_desc);
  status = xran_bm_init(instHandle, &poolSec, numBufsSec, bufSizeSec);
  AssertFatal(XRAN_STATUS_SUCCESS == status, "Failed at xran_bm_init(), status %d\n", status);
  printf("xran_bm_init() hInstance %p poolIdx %d elements %d size %d\n", instHandle, poolSec, numBufsSec, bufSizeSec);

  uint32_t poolPrb;
  uint32_t numBufsPrb = XRAN_N_FE_BUF_LEN * ant * XRAN_NUM_OF_SYMBOL_PER_SLOT;
  uint32_t bufSizePrb = size_of_prb_map;
  status = xran_bm_init(instHandle, &poolPrb, numBufsPrb, bufSizePrb);
  AssertFatal(XRAN_STATUS_SUCCESS == status, "Failed at xran_bm_init(), status %d\n", status);
  printf("xran_bm_init() hInstance %p poolIdx %d elements %d size %d\n", instHandle, poolPrb, numBufsPrb, bufSizePrb);

  uint32_t count1 = 0;
  uint32_t count2 = 0;
  for (uint32_t a = 0; a < ant; a++) {
    for (uint32_t j = 0; j < XRAN_N_FE_BUF_LEN; ++j) {
      list[a][j].pBuffers = &buf[a][j];
      struct xran_flat_buffer *fb = list[a][j].pBuffers;
      fb->nElementLenInBytes = bufSizePrb;
      fb->nNumberOfElements = 1;
      fb->nOffsetInBytes = 0;
      void *ptr;
      void *mb;
      status = xran_bm_allocate_buffer(instHandle, poolPrb, &ptr, &mb);
      AssertFatal(XRAN_STATUS_SUCCESS == status && ptr != NULL && mb != NULL,
                  "Failed at xran_bm_allocate_buffer(), status %d\n",
                  status);
      count1++;
      fb->pData = ptr;
      fb->pCtrl = mb;

      // the original sample app code copies up to size_of_prb_map, but I think
      // this is wrong because the way it is computed leads to a number larger
      // than sizeof(map)
      struct xran_prb_map *p_rb_map = (struct xran_prb_map *)ptr;
      const struct xran_prb_map *src = &prb_conf->slotMap;
      if ((j % prb_conf->nTddPeriod) == prb_conf->mixed_slot_index)
        src = &prb_conf->mixedSlotMap;
      memcpy(p_rb_map, src, sizeof(*src));

      for (uint32_t elm_id = 0; elm_id < p_rb_map->nPrbElm; ++elm_id) {
        struct xran_prb_elm *pPrbElem = &p_rb_map->prbMap[elm_id];
        for (uint32_t k = 0; k < XRAN_NUM_OF_SYMBOL_PER_SLOT; ++k) {
          for (uint32_t m = 0; m < XRAN_MAX_FRAGMENT; ++m) {
            void *sd_ptr;
            void *sd_mb;
            status = xran_bm_allocate_buffer(instHandle, poolSec, &sd_ptr, &sd_mb);
            AssertFatal(XRAN_STATUS_SUCCESS == status,
                        "Failed at xran_bm_allocate_buffer(), status %d m %d k %d elm_id %d\n",
                        status,
                        m,
                        k,
                        elm_id);
            count2++;
            pPrbElem->p_sec_desc[k][m] = sd_ptr;
            memset(sd_ptr, 0, sizeof(struct xran_section_desc));
          }
        }
      }
    }
  }
  printf("xran_bm_allocate_buffer() hInstance %p poolIdx %d count %d\n", instHandle, poolPrb, count1);
  printf("xran_bm_allocate_buffer() hInstance %p poolIdx %d count %d\n", instHandle, poolSec, count2);
}

/* callback not actively used */
static void oai_xran_fh_rx_prach_callback(void *pCallbackTag, xran_status_t status)
{
  rte_pause();
}

static void oran_allocate_buffers(void *handle,
                                  int xran_inst,
                                  int num_sectors,
                                  oran_port_instance_t *portInstances,
                                  const struct xran_fh_config *fh_config)
{
  AssertFatal(num_sectors == 1, "only support one sector at the moment\n");
  oran_port_instance_t *pi = &portInstances[0];
  AssertFatal(handle != NULL, "no handle provided\n");
  uint32_t xran_max_antenna_nr = RTE_MAX(fh_config->neAxc, fh_config->neAxcUl);
  uint32_t xran_max_sections_per_slot = RTE_MAX(fh_config->max_sections_per_slot, XRAN_MIN_SECTIONS_PER_SLOT);
  uint32_t size_of_prb_map = sizeof(struct xran_prb_map) + sizeof(struct xran_prb_elm) * (xran_max_sections_per_slot - 1);

  pi->buf_list = _mm_malloc(sizeof(*pi->buf_list), 256);
  AssertFatal(pi->buf_list != NULL, "out of memory\n");
  oran_buf_list_t *bl = pi->buf_list;

  xran_status_t status;
  printf("xran_sector_get_instances() o_xu_id %d xran_handle %p\n", xran_inst, handle);
  status = xran_sector_get_instances(xran_inst, handle, num_sectors, &pi->instanceHandle);
  printf("-> hInstance %p\n", pi->instanceHandle);
  AssertFatal(status == XRAN_STATUS_SUCCESS, "get sector instance failed for XRAN nInstanceNum %d\n", xran_inst);

  const uint32_t txBufSize = get_nSW_ToFpga_FTH_TxBufferLen(fh_config->frame_conf.nNumerology, fh_config->max_sections_per_slot);
  oran_allocate_uplane_buffers(pi->instanceHandle, bl->src, bl->bufs.tx, xran_max_antenna_nr, txBufSize);

  oran_mixed_slot_t info = get_mixed_slot_info(&fh_config->frame_conf);
  struct xran_prb_map dlPm = get_xran_prb_map_dl(fh_config);
  struct xran_prb_map dlPmMixed = dlPm;
  dlPmMixed.prbMap[0].nStartSymb = 0;
  dlPmMixed.prbMap[0].numSymb = info.num_dlsym;
  oran_cplane_prb_config dlConf = {
      .nTddPeriod = fh_config->frame_conf.nTddPeriod,
      .mixed_slot_index = info.idx,
      .slotMap = dlPm,
      .mixedSlotMap = dlPmMixed,
  };
  oran_allocate_cplane_buffers(pi->instanceHandle,
                               bl->srccp,
                               bl->bufs.tx_prbmap,
                               xran_max_antenna_nr,
                               xran_max_sections_per_slot,
                               size_of_prb_map,
                               &dlConf);

  const uint32_t rxBufSize = get_nFpgaToSW_FTH_RxBufferLen(fh_config->frame_conf.nNumerology);
  oran_allocate_uplane_buffers(pi->instanceHandle, bl->dst, bl->bufs.rx, xran_max_antenna_nr, rxBufSize);

  struct xran_prb_map ulPm = get_xran_prb_map_ul(fh_config);
  struct xran_prb_map ulPmMixed = ulPm;
  ulPmMixed.prbMap[0].nStartSymb = info.start_ulsym;
  ulPmMixed.prbMap[0].numSymb = info.num_ulsym;
  oran_cplane_prb_config ulConf = {
      .nTddPeriod = fh_config->frame_conf.nTddPeriod,
      .mixed_slot_index = info.idx,
      .slotMap = ulPm,
      .mixedSlotMap = ulPmMixed,
  };
  oran_allocate_cplane_buffers(pi->instanceHandle,
                               bl->dstcp,
                               bl->bufs.rx_prbmap,
                               xran_max_antenna_nr,
                               xran_max_sections_per_slot,
                               size_of_prb_map,
                               &ulConf);

  // PRACH
  const uint32_t prachBufSize = PRACH_PLAYBACK_BUFFER_BYTES;
  oran_allocate_uplane_buffers(pi->instanceHandle, bl->prachdst, bl->bufs.prach, xran_max_antenna_nr, prachBufSize);
  // PRACH decomp buffer does not have separate DPDK-allocated memory pool
  // bufs, it points to the same pool as the prach buffer. Unclear to me why
  for (uint32_t a = 0; a < xran_max_antenna_nr; ++a) {
    for (uint32_t j = 0; j < XRAN_N_FE_BUF_LEN; ++j) {
      bl->prachdstdecomp[a][j].pBuffers = &bl->bufs.prachdecomp[a][j][0];
      for (uint32_t k = 0; k < XRAN_NUM_OF_SYMBOL_PER_SLOT; ++k) {
        struct xran_flat_buffer *fb = &bl->prachdstdecomp[a][j].pBuffers[k];
        fb->pData = bl->prachdst[a][j].pBuffers[k].pData;
      }
    }
  }

  struct xran_buffer_list *src[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  struct xran_buffer_list *srccp[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  struct xran_buffer_list *dst[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  struct xran_buffer_list *dstcp[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  struct xran_buffer_list *prach[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  struct xran_buffer_list *prachdecomp[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  for (uint32_t a = 0; a < XRAN_MAX_ANTENNA_NR; ++a) {
    for (uint32_t j = 0; j < XRAN_N_FE_BUF_LEN; ++j) {
      src[a][j] = &bl->src[a][j];
      srccp[a][j] = &bl->srccp[a][j];
      dst[a][j] = &bl->dst[a][j];
      dstcp[a][j] = &bl->dstcp[a][j];
      prach[a][j] = &bl->prachdst[a][j];
      prachdecomp[a][j] = &bl->prachdstdecomp[a][j];
    }
  }

  xran_5g_fronthault_config(pi->instanceHandle, src, srccp, dst, dstcp, oai_xran_fh_rx_callback, &portInstances->RxCbTag[0][0]);
  xran_5g_prach_req(pi->instanceHandle, prach, prachdecomp, oai_xran_fh_rx_prach_callback, &portInstances->PrachCbTag[0][0]);
}

int *oai_oran_initialize(const openair0_config_t *openair0_cfg)
{
  int32_t xret = 0;

  struct xran_fh_init init = {0};
  if (!set_fh_init(&init)) {
    printf("could not read FHI 7.2/ORAN config\n");
    return NULL;
  }
  print_fh_init(&init);

  /* read all configuration before starting anything */
  struct xran_fh_config xran_fh_config[XRAN_PORTS_NUM] = {0};
  for (int32_t o_xu_id = 0; o_xu_id < init.xran_ports; o_xu_id++) {
    if (!set_fh_config(o_xu_id, init.xran_ports, openair0_cfg, &xran_fh_config[o_xu_id])) {
      printf("could not read FHI 7.2/RU-specific config\n");
      return NULL;
    }
    print_fh_config(&xran_fh_config[o_xu_id]);
  }

  xret = xran_init(0, NULL, &init, NULL, &gxran_handle);
  if (xret != XRAN_STATUS_SUCCESS) {
    printf("xran_init failed %d\n", xret);
    exit(-1);
  }

  /** process all the O-RU|O-DU for use case */
  for (int32_t o_xu_id = 0; o_xu_id < init.xran_ports; o_xu_id++) {
    xret = xran_open(gxran_handle, &xran_fh_config[o_xu_id]);
    if (xret != XRAN_STATUS_SUCCESS) {
      printf("xran_open failed %d\n", xret);
      exit(-1);
    }

    int sector = 0;
    oran_port_instance_t *pi = &gPortInst[o_xu_id][sector];
    oran_allocate_buffers(gxran_handle, o_xu_id, 1, pi, &xran_fh_config[o_xu_id]);

    if ((xret = xran_reg_physide_cb(gxran_handle, oai_physide_dl_tti_call_back, NULL, 10, XRAN_CB_TTI)) != XRAN_STATUS_SUCCESS) {
      printf("xran_reg_physide_cb failed %d\n", xret);
      exit(-1);
    }
  }

  return (void *)gxran_handle;
}
