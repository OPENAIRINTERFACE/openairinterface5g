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

#include "oran-config.h"
#include "oran-params.h"
#include "common/utils/assertions.h"
#include "common_lib.h"

#include "xran_fh_o_du.h"
#include "xran_cp_api.h"
#include "rte_ether.h"

#include "stdio.h"
#include "string.h"

static void print_fh_eowd_cmn(unsigned index, const struct xran_ecpri_del_meas_cmn *eowd_cmn)
{
  printf("\
    eowd_cmn[%d]:\n\
      initiator_en %d\n\
      numberOfSamples %d\n\
      filterType %d\n\
      responseTo %ld\n\
      measVf %d\n\
      measState %d\n\
      measId %d\n\
      measMethod %d\n\
      owdm_enable %d\n\
      owdm_PlLength %d\n",
      index,
      eowd_cmn->initiator_en,
      eowd_cmn->numberOfSamples,
      eowd_cmn->filterType,
      eowd_cmn->responseTo,
      eowd_cmn->measVf,
      eowd_cmn->measState,
      eowd_cmn->measId,
      eowd_cmn->measMethod,
      eowd_cmn->owdm_enable,
      eowd_cmn->owdm_PlLength);
}

static void print_fh_eowd_port(unsigned index, unsigned vf, const struct xran_ecpri_del_meas_port *eowd_port)
{
  printf("\
    eowd_port[%d][%d]:\n\
      t1 %ld\n\
      t2 %ld\n\
      tr %ld\n\
      delta %ld\n\
      portid %d\n\
      runMeas %d\n\
      currentMeasID %d\n\
      msState %d\n\
      numMeas %d\n\
      txDone %d\n\
      rspTimerIdx %ld\n\
      delaySamples [%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld]\n\
      delayAvg %ld\n",
      index,
      vf,
      eowd_port->t1,
      eowd_port->t2,
      eowd_port->tr,
      eowd_port->delta,
      eowd_port->portid,
      eowd_port->runMeas,
      eowd_port->currentMeasID,
      eowd_port->msState,
      eowd_port->numMeas,
      eowd_port->txDone,
      eowd_port->rspTimerIdx,
      eowd_port->delaySamples[0],
      eowd_port->delaySamples[1],
      eowd_port->delaySamples[2],
      eowd_port->delaySamples[3],
      eowd_port->delaySamples[4],
      eowd_port->delaySamples[5],
      eowd_port->delaySamples[6],
      eowd_port->delaySamples[7],
      eowd_port->delaySamples[8],
      eowd_port->delaySamples[9],
      eowd_port->delaySamples[10],
      eowd_port->delaySamples[11],
      eowd_port->delaySamples[12],
      eowd_port->delaySamples[13],
      eowd_port->delaySamples[14],
      eowd_port->delaySamples[15],
      eowd_port->delayAvg);
}

static void print_fh_init_io_cfg(const struct xran_io_cfg *io_cfg)
{
  printf("\
  io_cfg:\n\
    id %d (%s)\n\
    num_vfs %d\n\
    num_rxq %d\n\
    dpdk_dev [%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s]\n\
    bbdev_dev %s\n\
    bbdev_mode %d\n\
    dpdkIoVaMode %d\n\
    dpdkMemorySize %d\n",
      io_cfg->id,
      io_cfg->id == 0 ? "O-DU" : "O-RU",
      io_cfg->num_vfs,
      io_cfg->num_rxq,
      io_cfg->dpdk_dev[XRAN_UP_VF],
      io_cfg->dpdk_dev[XRAN_CP_VF],
      io_cfg->dpdk_dev[XRAN_UP_VF1],
      io_cfg->dpdk_dev[XRAN_CP_VF1],
      io_cfg->dpdk_dev[XRAN_UP_VF2],
      io_cfg->dpdk_dev[XRAN_CP_VF2],
      io_cfg->dpdk_dev[XRAN_UP_VF3],
      io_cfg->dpdk_dev[XRAN_CP_VF3],
      io_cfg->dpdk_dev[XRAN_UP_VF4],
      io_cfg->dpdk_dev[XRAN_CP_VF4],
      io_cfg->dpdk_dev[XRAN_UP_VF5],
      io_cfg->dpdk_dev[XRAN_CP_VF5],
      io_cfg->dpdk_dev[XRAN_UP_VF6],
      io_cfg->dpdk_dev[XRAN_CP_VF6],
      io_cfg->dpdk_dev[XRAN_UP_VF7],
      io_cfg->dpdk_dev[XRAN_CP_VF7],
      io_cfg->bbdev_dev[0],
      io_cfg->bbdev_mode,
      io_cfg->dpdkIoVaMode,
      io_cfg->dpdkMemorySize);

  printf("\
    core %d\n\
    system_core %d\n\
    pkt_proc_core %016lx\n\
    pkt_proc_core_64_127 %016lx\n\
    pkt_aux_core %d\n\
    timing_core %d\n\
    port [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, ]\n\
    io_sleep %d\n\
    nEthLinePerPort %d\n\
    nEthLineSpeed %d\n\
    one_vf_cu_plane %d\n",
      io_cfg->core,
      io_cfg->system_core,
      io_cfg->pkt_proc_core,
      io_cfg->pkt_proc_core_64_127,
      io_cfg->pkt_aux_core,
      io_cfg->timing_core,
      io_cfg->port[XRAN_UP_VF],
      io_cfg->port[XRAN_CP_VF],
      io_cfg->port[XRAN_UP_VF1],
      io_cfg->port[XRAN_CP_VF1],
      io_cfg->port[XRAN_UP_VF2],
      io_cfg->port[XRAN_CP_VF2],
      io_cfg->port[XRAN_UP_VF3],
      io_cfg->port[XRAN_CP_VF3],
      io_cfg->port[XRAN_UP_VF4],
      io_cfg->port[XRAN_CP_VF4],
      io_cfg->port[XRAN_UP_VF5],
      io_cfg->port[XRAN_CP_VF5],
      io_cfg->port[XRAN_UP_VF6],
      io_cfg->port[XRAN_CP_VF6],
      io_cfg->port[XRAN_UP_VF7],
      io_cfg->port[XRAN_CP_VF7],
      io_cfg->io_sleep,
      io_cfg->nEthLinePerPort,
      io_cfg->nEthLineSpeed,
      io_cfg->one_vf_cu_plane);
  print_fh_eowd_cmn(0, &io_cfg->eowd_cmn[0]);
  print_fh_eowd_cmn(1, &io_cfg->eowd_cmn[1]);
  for (int i = 0; i < 2; ++i)
    for (int v = 0; v < io_cfg->num_vfs; ++v)
      print_fh_eowd_port(i, v, &io_cfg->eowd_port[i][v]);
}

static void print_fh_init_eaxcid_conf(const struct xran_eaxcid_config *eaxcid_conf)
{
  printf("\
  eAxCId_conf:\n\
    mask_cuPortId 0x%04x\n\
    mask_bandSectorId 0x%04x\n\
    mask_ccId 0x%04x\n\
    mask_ruPortId 0x%04x\n\
    bit_cuPortId %d\n\
    bit_bandSectorId %d\n\
    bit_ccId %d\n\
    bit_ruPortId %d\n",
      eaxcid_conf->mask_cuPortId,
      eaxcid_conf->mask_bandSectorId,
      eaxcid_conf->mask_ccId,
      eaxcid_conf->mask_ruPortId,
      eaxcid_conf->bit_cuPortId,
      eaxcid_conf->bit_bandSectorId,
      eaxcid_conf->bit_ccId,
      eaxcid_conf->bit_ruPortId);
}

static void print_ether_addr(const char *pre, int num_ether, const struct rte_ether_addr *addrs)
{
  printf("%s [", pre);
  for (int i = 0; i < num_ether; ++i) {
    char buf[18];
    rte_ether_format_addr(buf, 18, &addrs[i]);
    printf("%s", buf);
    if (i != num_ether - 1)
      printf(", ");
  }
  printf("]\n");
}

void print_fh_init(const struct xran_fh_init *fh_init)
{
  printf("xran_fh_init:\n");
  print_fh_init_io_cfg(&fh_init->io_cfg);
  print_fh_init_eaxcid_conf(&fh_init->eAxCId_conf);
  printf("\
  xran_ports %d\n\
  dpdkBasebandFecMode %d\n\
  dpdkBasebandDevice %s\n\
  filePrefix %s\n\
  mtu %d\n",
      fh_init->xran_ports,
      fh_init->dpdkBasebandFecMode,
      fh_init->dpdkBasebandDevice,
      fh_init->filePrefix,
      fh_init->mtu);
  print_ether_addr("  p_o_du_addr", fh_init->xran_ports * fh_init->io_cfg.num_vfs, (struct rte_ether_addr *)fh_init->p_o_du_addr);
  print_ether_addr("  p_o_ru_addr", fh_init->xran_ports * fh_init->io_cfg.num_vfs, (struct rte_ether_addr *)fh_init->p_o_ru_addr);
  printf("\
  totalBfWeights %d\n",
      fh_init->totalBfWeights);
}

static void print_prach_config(const struct xran_prach_config *prach_conf)
{
  printf("\
  prach_config:\n\
    nPrachConfIdx %d\n\
    nPrachSubcSpacing %d\n\
    nPrachZeroCorrConf %d\n\
    nPrachRestrictSet %d\n\
    nPrachRootSeqIdx %d\n\
    nPrachFreqStart %d\n\
    nPrachFreqOffset %d\n\
    nPrachFilterIdx %d\n\
    startSymId %d\n\
    lastSymId %d\n\
    startPrbc %d\n\
    numPrbc %d\n\
    timeOffset %d\n\
    freqOffset %d\n\
    eAxC_offset %d\n",
      prach_conf->nPrachConfIdx,
      prach_conf->nPrachSubcSpacing,
      prach_conf->nPrachZeroCorrConf,
      prach_conf->nPrachRestrictSet,
      prach_conf->nPrachRootSeqIdx,
      prach_conf->nPrachFreqStart,
      prach_conf->nPrachFreqOffset,
      prach_conf->nPrachFilterIdx,
      prach_conf->startSymId,
      prach_conf->lastSymId,
      prach_conf->startPrbc,
      prach_conf->numPrbc,
      prach_conf->timeOffset,
      prach_conf->freqOffset,
      prach_conf->eAxC_offset);
}

static void print_srs_config(const struct xran_srs_config *srs_conf)
{
  printf("\
  srs_config:\n\
    symbMask %04x\n\
    eAxC_offset %d\n",
      srs_conf->symbMask,
      srs_conf->eAxC_offset);
}

static void print_frame_config(const struct xran_frame_config *frame_conf)
{
  printf("\
  frame_conf:\n\
    nFrameDuplexType %s\n\
    nNumerology %d\n\
    nTddPeriod %d\n",
      frame_conf->nFrameDuplexType == XRAN_TDD ? "TDD" : "FDD",
      frame_conf->nNumerology,
      frame_conf->nTddPeriod);
  for (int i = 0; i < frame_conf->nTddPeriod; ++i) {
    printf("    sSlotConfig[%d]: ", i);
    for (int s = 0; s < XRAN_NUM_OF_SYMBOL_PER_SLOT; ++s) {
      uint8_t nSymbolType = frame_conf->sSlotConfig[i].nSymbolType[s];
      printf("%c", nSymbolType == 0 ? 'D' : (nSymbolType == 1 ? 'U' : 'G'));
    }
    printf("\n");
  }
}

static void print_ru_config(const struct xran_ru_config *ru_conf)
{
  printf("\
  ru_config:\n\
    xranTech %s\n\
    xranCat %s\n\
    xranCompHdrType %s\n\
    iqWidth %d\n\
    compMeth %d\n\
    iqWidth_PRACH %d\n\
    compMeth_PRACH %d\n\
    fftSize %d\n\
    byteOrder %s\n\
    iqOrder %s\n\
    xran_max_frame %d\n",
      ru_conf->xranTech == XRAN_RAN_5GNR ? "NR" : "LTE",
      ru_conf->xranCat == XRAN_CATEGORY_A ? "A" : "B",
      ru_conf->xranCompHdrType == XRAN_COMP_HDR_TYPE_DYNAMIC ? "dynamic" : "static",
      ru_conf->iqWidth,
      ru_conf->compMeth,
      ru_conf->iqWidth_PRACH,
      ru_conf->compMeth_PRACH,
      ru_conf->fftSize,
      ru_conf->byteOrder == XRAN_NE_BE_BYTE_ORDER ? "network/BE" : "CPU/LE",
      ru_conf->iqOrder == XRAN_I_Q_ORDER ? "I_Q" : "Q_I",
      ru_conf->xran_max_frame);
}

void print_fh_config(const struct xran_fh_config *fh_config)
{
  printf("xran_fh_config:\n");
  printf("\
  dpdk_port %d\n\
  sector_id %d\n\
  nCC %d\n\
  neAxc %d\n\
  neAxcUl %d\n\
  nAntElmTRx %d\n\
  nDLFftSize %d\n\
  nULFftSize %d\n\
  nDLRBs %d\n\
  nULRBs %d\n\
  nDLAbsFrePointA %d\n\
  nULAbsFrePointA %d\n\
  nDLCenterFreqARFCN %d\n\
  nULCenterFreqARFCN %d\n\
  ttiCb %p\n\
  ttiCbParam %p\n",
      fh_config->dpdk_port,
      fh_config->sector_id,
      fh_config->nCC,
      fh_config->neAxc,
      fh_config->neAxcUl,
      fh_config->nAntElmTRx,
      fh_config->nDLFftSize,
      fh_config->nULFftSize,
      fh_config->nDLRBs,
      fh_config->nULRBs,
      fh_config->nDLAbsFrePointA,
      fh_config->nULAbsFrePointA,
      fh_config->nDLCenterFreqARFCN,
      fh_config->nULCenterFreqARFCN,
      fh_config->ttiCb,
      fh_config->ttiCbParam);

  printf("\
  Tadv_cp_dl %d\n\
  T2a_min_cp_dl %d\n\
  T2a_max_cp_dl %d\n\
  T2a_min_cp_ul %d\n\
  T2a_max_cp_ul %d\n\
  T2a_min_up %d\n\
  T2a_max_up %d\n\
  Ta3_min %d\n\
  Ta3_max %d\n\
  T1a_min_cp_dl %d\n\
  T1a_max_cp_dl %d\n\
  T1a_min_cp_ul %d\n\
  T1a_max_cp_ul %d\n\
  T1a_min_up %d\n\
  T1a_max_up %d\n\
  Ta4_min %d\n\
  Ta4_max %d\n",
      fh_config->Tadv_cp_dl,
      fh_config->T2a_min_cp_dl,
      fh_config->T2a_max_cp_dl,
      fh_config->T2a_min_cp_ul,
      fh_config->T2a_max_cp_ul,
      fh_config->T2a_min_up,
      fh_config->T2a_max_up,
      fh_config->Ta3_min,
      fh_config->Ta3_max,
      fh_config->T1a_min_cp_dl,
      fh_config->T1a_max_cp_dl,
      fh_config->T1a_min_cp_ul,
      fh_config->T1a_max_cp_ul,
      fh_config->T1a_min_up,
      fh_config->T1a_max_up,
      fh_config->Ta4_min,
      fh_config->Ta4_max);

  printf("\
  enableCP %d\n\
  prachEnable %d\n\
  srsEnable %d\n\
  puschMaskEnable %d\n\
  puschMaskSlot %d\n\
  cp_vlan_tag %d\n\
  up_vlan_tag %d\n\
  debugStop %d\n\
  debugStopCount %d\n\
  DynamicSectionEna %d\n\
  GPS_Alpha %d\n\
  GPS_Beta %d\n",
      fh_config->enableCP,
      fh_config->prachEnable,
      fh_config->srsEnable,
      fh_config->puschMaskEnable,
      fh_config->puschMaskSlot,
      fh_config->cp_vlan_tag,
      fh_config->up_vlan_tag,
      fh_config->debugStop,
      fh_config->debugStopCount,
      fh_config->DynamicSectionEna,
      fh_config->GPS_Alpha,
      fh_config->GPS_Beta);

  print_prach_config(&fh_config->prach_conf);
  print_srs_config(&fh_config->srs_conf);
  print_frame_config(&fh_config->frame_conf);
  print_ru_config(&fh_config->ru_conf);

  printf("\
  bbdev_enc %p\n\
  bbdev_dec %p\n\
  tx_cp_eAxC2Vf [not implemented by fhi_lib]\n\
  tx_up_eAxC2Vf [not implemented by fhi_lib]\n\
  rx_cp_eAxC2Vf [not implemented by fhi_lib]\n\
  rx_up_eAxC2Vf [not implemented by fhi_lib]\n\
  log_level %d\n\
  max_sections_per_slot %d\n\
  max_sections_per_symbol %d\n",
      fh_config->bbdev_enc,
      fh_config->bbdev_dec,
      fh_config->log_level,
      fh_config->max_sections_per_slot,
      fh_config->max_sections_per_symbol);
}

static const paramdef_t *gpd(const paramdef_t *pd, int num, const char *name)
{
  /* the config module does not know const-correctness... */
  int idx = config_paramidx_fromname((paramdef_t *)pd, num, (char *)name);
  DevAssert(idx >= 0);
  return &pd[idx];
}

static uint64_t get_u64_mask(const paramdef_t *pd)
{
  DevAssert(pd != NULL);
  AssertFatal(pd->numelt > 0, "no entries for creation of mask\n");
  uint64_t mask = 0;
  for (int i = 0; i < pd->numelt; ++i) {
    int num = pd->iptr[i];
    AssertFatal(num >= 0 && num < 64, "cannot put element of %d in 64-bit mask\n", num);
    mask |= 1 << num;
  }
  return mask;
}

static bool set_fh_io_cfg(struct xran_io_cfg *io_cfg, const paramdef_t *fhip, int nump)
{
  DevAssert(fhip != NULL);
  int num_dev = gpd(fhip, nump, ORAN_CONFIG_DPDK_DEVICES)->numelt;
  AssertFatal(num_dev > 0, "need to provide DPDK devices for O-RAN 7.2 Fronthaul\n");
  AssertFatal(num_dev < 17, "too many DPDK devices for O-RAN 7.2 Fronthaul\n");

  io_cfg->id = 0; // 0 = O-DU
  io_cfg->num_vfs = num_dev;
  io_cfg->num_rxq = 2; // Assume two HW RX queues per RU
  for (int i = 0; i < num_dev; ++i)
    io_cfg->dpdk_dev[i] = strdup(gpd(fhip, nump, ORAN_CONFIG_DPDK_DEVICES)->strlistptr[i]);
  //io_cfg->bbdev_dev = NULL;
  io_cfg->bbdev_mode = XRAN_BBDEV_NOT_USED; // none
  io_cfg->dpdkIoVaMode = 0; /* IOVA mode */
  io_cfg->dpdkMemorySize = 0; /* DPDK memory size */
  io_cfg->core = *gpd(fhip, nump, ORAN_CONFIG_IO_CORE)->iptr;
  io_cfg->system_core = *gpd(fhip, nump, ORAN_CONFIG_SYSTEM_CORE)->iptr;
  io_cfg->pkt_proc_core = get_u64_mask(gpd(fhip, nump, ORAN_CONFIG_WORKER_CORES));
  io_cfg->pkt_proc_core_64_127 = 0x0; // bitmap 0 -> no core
  io_cfg->pkt_aux_core = 0; /* sapmle app says 0 = "do not start" */
  io_cfg->timing_core = *gpd(fhip, nump, ORAN_CONFIG_IO_CORE)->iptr; /* sample app: equal to io_core */
  //io_cfg->port = {0}; // all 0
  io_cfg->io_sleep = 0; // no sleep
  io_cfg->nEthLinePerPort = *gpd(fhip, nump, ORAN_CONFIG_NETHPERPORT)->uptr;
  io_cfg->nEthLineSpeed = *gpd(fhip, nump, ORAN_CONFIG_NETHSPEED)->uptr;
  io_cfg->one_vf_cu_plane = 0; // false: C/U-plane don't share VF
  // io_cfg->eowd_cmn[0] // all 0
  // io_cfg->eowd_cmn[1] // all 0
  // io_cfg->eowd_port[0]... // all 0

  return true;
}

static bool set_fh_eaxcid_conf(struct xran_eaxcid_config *eaxcid_conf, enum xran_category cat)
{
  // values taken from sample app
  switch (cat) {
    case XRAN_CATEGORY_A:
      eaxcid_conf->mask_cuPortId = 0xf000;
      eaxcid_conf->mask_bandSectorId = 0x0f00;
      eaxcid_conf->mask_ccId = 0x00f0;
      eaxcid_conf->mask_ruPortId = 0x000f;
      eaxcid_conf->bit_cuPortId = 12;
      eaxcid_conf->bit_bandSectorId = 8;
      eaxcid_conf->bit_ccId = 4;
      eaxcid_conf->bit_ruPortId = 0;
      break;
    case XRAN_CATEGORY_B:
      eaxcid_conf->mask_cuPortId = 0xf000;
      eaxcid_conf->mask_bandSectorId = 0x0c00;
      eaxcid_conf->mask_ccId = 0x0300;
      eaxcid_conf->mask_ruPortId = 0x000f;
      eaxcid_conf->bit_cuPortId = 12;
      eaxcid_conf->bit_bandSectorId = 10;
      eaxcid_conf->bit_ccId = 8;
      eaxcid_conf->bit_ruPortId = 0;
      break;
    default:
      return false;
  }

  return true;
}

uint8_t *get_ether_addr(const char *addr, struct rte_ether_addr *ether_addr)
{
#pragma GCC diagnostic push
  // the following line disables the deprecated warning
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  int ret = rte_ether_unformat_addr(addr, ether_addr);
#pragma GCC diagnostic pop
  if (ret == 0)
    return (uint8_t *)ether_addr;
  return NULL;
}

bool set_fh_init(struct xran_fh_init *fh_init)
{
  memset(fh_init, 0, sizeof(*fh_init));

  // verify oran section is present: we don't have a list but the below returns
  // numelt > 0 if the block is there
  paramlist_def_t pl = {0};
  strncpy(pl.listname, CONFIG_STRING_ORAN, sizeof(pl.listname) - 1);
  config_getlist(config_get_if(), &pl, NULL, 0, /* prefix */ NULL);
  if (pl.numelt == 0) {
    printf("Configuration section \"%s\" not present: cannot initialize fhi_lib!\n", CONFIG_STRING_ORAN);
    return false;
  }

  paramdef_t fhip[] = ORAN_GLOBALPARAMS_DESC;
  int nump = sizeofArray(fhip);
  int ret = config_get(config_get_if(), fhip, nump, CONFIG_STRING_ORAN);
  if (ret <= 0) {
    printf("problem reading section \"%s\"\n", CONFIG_STRING_ORAN);
    return false;
  }

  paramdef_t FHconfigs[] = ORAN_FH_DESC;
  paramlist_def_t FH_ConfigList = {CONFIG_STRING_ORAN_FH};
  char aprefix[MAX_OPTNAME_SIZE] = {0};
  sprintf(aprefix, "%s", CONFIG_STRING_ORAN);
  const int nfh = sizeofArray(FHconfigs);
  config_getlist(config_get_if(), &FH_ConfigList, FHconfigs, nfh, aprefix);
  int num_rus = FH_ConfigList.numelt;
  int num_ru_addr = gpd(fhip, nump, ORAN_CONFIG_RU_ADDR)->numelt;
  int num_du_addr = gpd(fhip, nump, ORAN_CONFIG_DU_ADDR)->numelt;
  int num_vfs = gpd(fhip, nump, ORAN_CONFIG_DPDK_DEVICES)->numelt;
  if (num_ru_addr != num_du_addr) {
    printf("need to have same number of DUs and RUs!\n");
    return false;
  }
  if (num_ru_addr != num_vfs) {
    printf("need to have as many RU/DU entries as DPDK devices (one VF for CP and UP each)\n");
    return false;
  }

  if (!set_fh_io_cfg(&fh_init->io_cfg, fhip, nump))
    return false;
  if (!set_fh_eaxcid_conf(&fh_init->eAxCId_conf, XRAN_CATEGORY_A))
    return false;

  fh_init->xran_ports = num_rus;
  fh_init->dpdkBasebandFecMode = 0;
  fh_init->dpdkBasebandDevice = NULL;
  fh_init->filePrefix = strdup(*gpd(fhip, nump, ORAN_CONFIG_FILE_PREFIX)->strptr); // see DPDK --file-prefix
  fh_init->mtu = *gpd(fhip, nump, ORAN_CONFIG_MTU)->uptr;

  // if multiple RUs: xran_ethdi_init_dpdk_io() iterates over
  // &p_o_ru_addr[i]
  char **du_addrs = gpd(fhip, nump, ORAN_CONFIG_DU_ADDR)->strlistptr;
  fh_init->p_o_du_addr = calloc(num_du_addr, sizeof(struct rte_ether_addr));
  AssertFatal(fh_init->p_o_du_addr != NULL, "out of memory\n");
  for (int i = 0; i < num_du_addr; ++i) {
    struct rte_ether_addr *ea = (struct rte_ether_addr *)fh_init->p_o_du_addr;
    if (get_ether_addr(du_addrs[i], &ea[i]) == NULL) {
      printf("could not read ethernet address '%s' for DU!\n", du_addrs[i]);
      return false;
    }
  }
  fh_init->p_o_ru_addr = calloc(num_ru_addr, sizeof(struct rte_ether_addr));
  char **ru_addrs = gpd(fhip, nump, ORAN_CONFIG_RU_ADDR)->strlistptr;
  AssertFatal(fh_init->p_o_ru_addr != NULL, "out of memory\n");
  for (int i = 0; i < num_ru_addr; ++i) {
    struct rte_ether_addr *ea = (struct rte_ether_addr *)fh_init->p_o_ru_addr;
    if (get_ether_addr(ru_addrs[i], &ea[i]) == NULL) {
      printf("could not read ethernet address '%s' for RU!\n", ru_addrs[i]);
      return false;
    }
  }
  fh_init->totalBfWeights = 32;

  return true;
}

static enum xran_cp_filterindex get_prach_filterindex_fr1(duplex_mode_t mode, int prach_index)
{
  if (mode == duplex_mode_TDD) {
    // 38.211 table 6.3.3.2-3 "unpaired spectrum" -> TDD
    switch (prach_index) {
      case 0 ... 39:
      case 256 ... 262:
        return XRAN_FILTERINDEX_PRACH_012;
      case 40 ... 66:
        return XRAN_FILTERINDEX_PRACH_3;
      case 67 ... 255:
        return XRAN_FILTERINDEX_PRACH_ABC;
    }
  } else if (mode == duplex_mode_FDD) {
    // 38.211 table 6.3.3.2-2 "paired spectrum" -> FDD
    switch (prach_index) {
      case 0 ... 59:
        return XRAN_FILTERINDEX_PRACH_012;
      case 60 ... 86:
        return XRAN_FILTERINDEX_PRACH_3;
      case 87 ... 255:
        return XRAN_FILTERINDEX_PRACH_ABC;
      default:
        AssertFatal(false, "unknown PRACH index %d\n", prach_index);
    }
  } else {
    AssertFatal(false, "unsupported duplex mode %d\n", mode);
  }
  return XRAN_FILTERINDEX_STANDARD;
}

// PRACH guard interval. Raymond: "[it] is not in the configuration, (i.e. it
// is deterministic depending on others). LiteON must hard-code this in the
// O-RU itself, benetel doesn't (as O-RAN specifies). So we will need to tell
// the driver what the case is and provide"
// this is a hack
int g_kbar;

static bool set_fh_prach_config(const openair0_config_t *oai0,
                                const paramdef_t *prachp,
                                int nprach,
                                struct xran_prach_config *prach_config)
{
  const split7_config_t *s7cfg = &oai0->split7;

  prach_config->nPrachConfIdx = s7cfg->prach_index;
  prach_config->nPrachSubcSpacing = oai0->nr_scs_for_raster;
  prach_config->nPrachZeroCorrConf = 0;
  prach_config->nPrachRestrictSet = 0;
  prach_config->nPrachRootSeqIdx = 0;
  prach_config->nPrachFreqStart = s7cfg->prach_freq_start;
  prach_config->nPrachFreqOffset = (s7cfg->prach_freq_start * 12 - oai0->num_rb_dl * 6) * 2;
  if (oai0->nr_band < 100)
    prach_config->nPrachFilterIdx = get_prach_filterindex_fr1(oai0->duplex_mode, s7cfg->prach_index);
  else
    prach_config->nPrachFilterIdx = XRAN_FILTERINDEX_PRACH_ABC;
  prach_config->startSymId = 0;
  prach_config->lastSymId = 0;
  prach_config->startPrbc = 0;
  prach_config->numPrbc = 0;
  prach_config->timeOffset = 0;
  prach_config->freqOffset = 0;
  prach_config->eAxC_offset = *gpd(prachp, nprach, ORAN_PRACH_CONFIG_EAXC_OFFSET)->u8ptr;

  g_kbar = *gpd(prachp, nprach, ORAN_PRACH_CONFIG_KBAR)->uptr;

  return true;
}

static bool set_fh_srs_config(struct xran_srs_config *srs_config)
{
  srs_config->symbMask = 0;
  srs_config->eAxC_offset = 8;
  return true;
}

static bool set_fh_frame_config(const openair0_config_t *oai0, struct xran_frame_config *frame_config)
{
  const split7_config_t *s7cfg = &oai0->split7;
  frame_config->nFrameDuplexType = oai0->duplex_mode == duplex_mode_TDD ? XRAN_TDD : XRAN_FDD;
  frame_config->nNumerology = oai0->nr_scs_for_raster;
  frame_config->nTddPeriod = s7cfg->n_tdd_period;

  struct xran_slot_config *sc = &frame_config->sSlotConfig[0];
  for (int slot = 0; slot < frame_config->nTddPeriod; ++slot)
    for (int sym = 0; sym < 14; ++sym)
      sc[slot].nSymbolType[sym] = s7cfg->slot_dirs[slot].sym_dir[sym];
  return true;
}

static bool set_fh_ru_config(const paramdef_t *rup, int nru, struct xran_ru_config *ru_config)
{
  ru_config->xranTech = XRAN_RAN_5GNR;
  ru_config->xranCat = XRAN_CATEGORY_A;
  ru_config->xranCompHdrType = XRAN_COMP_HDR_TYPE_STATIC;
  ru_config->iqWidth = *gpd(rup, nru, ORAN_RU_CONFIG_IQWIDTH)->uptr;
  AssertFatal(ru_config->iqWidth <= 16, "IQ Width cannot be > 16!\n");
  ru_config->compMeth = ru_config->iqWidth < 16 ? XRAN_COMPMETHOD_BLKFLOAT : XRAN_COMPMETHOD_NONE;
  ru_config->iqWidth_PRACH = *gpd(rup, nru, ORAN_RU_CONFIG_IQWIDTH_PRACH)->uptr;
  AssertFatal(ru_config->iqWidth_PRACH <= 16, "IQ Width for PRACH cannot be > 16!\n");
  ru_config->compMeth_PRACH = ru_config->iqWidth_PRACH < 16 ? XRAN_COMPMETHOD_BLKFLOAT : XRAN_COMPMETHOD_NONE;
  ru_config->fftSize = *gpd(rup, nru, ORAN_RU_CONFIG_FFT_SIZE)->uptr;
  ru_config->byteOrder = XRAN_NE_BE_BYTE_ORDER;
  ru_config->iqOrder = XRAN_I_Q_ORDER;
  ru_config->xran_max_frame = 0;
  return true;
}

static bool set_maxmin_pd(const paramdef_t *pd, int num, const char *name, uint16_t *min, uint16_t *max)
{
  const paramdef_t *p = gpd(pd, num, name);
  if (p->numelt != 2) {
    printf("parameter list \"%s\" should have exactly two parameters (max&min), but has %d\n", name, num);
    return false;
  }
  *min = p->uptr[0];
  *max = p->uptr[1];
  if (*min > *max) {
    printf("min parameter of \"%s\" is larger than max!\n", name);
    return false;
  }
  return true;
}

extern uint32_t to_nrarfcn(int nr_bandP, uint64_t dl_CarrierFreq, uint8_t scs_index, uint32_t bw);
bool set_fh_config(int ru_idx, int num_rus, const openair0_config_t *oai0, struct xran_fh_config *fh_config)
{
  AssertFatal(num_rus == 1 || num_rus == 2, "only support 1 or 2 RUs as of now\n");
  AssertFatal(ru_idx < num_rus, "illegal ru_idx %d: must be < %d\n", ru_idx, num_rus);
  DevAssert(oai0->tx_num_channels > 0 && oai0->rx_num_channels > 0);
  DevAssert(oai0->tx_bw > 0 && oai0->rx_bw > 0);
  //AssertFatal(oai0->tx_num_channels == oai0->rx_num_channels, "cannot handle unequal number of TX/RX channels\n");
  DevAssert(oai0->tx_freq[0] > 0);
  for (int i = 1; i < oai0->tx_num_channels; ++i)
    DevAssert(oai0->tx_freq[0] == oai0->tx_freq[i]);
  DevAssert(oai0->rx_freq[0] > 0);
  for (int i = 1; i < oai0->rx_num_channels; ++i)
    DevAssert(oai0->rx_freq[0] == oai0->rx_freq[i]);
  DevAssert(oai0->nr_band > 0);
  DevAssert(oai0->nr_scs_for_raster > 0);

  // we simply assume that the loading process provides function to_nrarfcn()
  // to calculate the ARFCN numbers from frequency. That is not clean, but the
  // best we can do without copy-pasting the function.
  uint32_t nDLCenterFreqARFCN = to_nrarfcn(oai0->nr_band, oai0->tx_freq[0], oai0->nr_scs_for_raster, oai0->tx_bw);
  uint32_t nULCenterFreqARFCN = to_nrarfcn(oai0->nr_band, oai0->rx_freq[0], oai0->nr_scs_for_raster, oai0->rx_bw);

  paramdef_t FHconfigs[] = ORAN_FH_DESC;
  paramlist_def_t FH_ConfigList = {CONFIG_STRING_ORAN_FH};
  char aprefix[MAX_OPTNAME_SIZE] = {0};
  sprintf(aprefix, "%s", CONFIG_STRING_ORAN);
  const int nfh = sizeofArray(FHconfigs);
  config_getlist(config_get_if(), &FH_ConfigList, FHconfigs, nfh, aprefix);
  if (FH_ConfigList.numelt == 0) {
    printf("No configuration section \"%s\" found inside \"%s\": cannot initialize fhi_lib!\n", CONFIG_STRING_ORAN_FH, aprefix);
    return false;
  }
  paramdef_t *fhp = FH_ConfigList.paramarray[ru_idx];

  paramdef_t rup[] = ORAN_RU_DESC;
  int nru = sizeofArray(rup);
  sprintf(aprefix, "%s.%s.[%d].%s", CONFIG_STRING_ORAN, CONFIG_STRING_ORAN_FH, ru_idx, CONFIG_STRING_ORAN_RU);
  int ret = config_get(config_get_if(), rup, nru, aprefix);
  if (ret < 0) {
    printf("No configuration section \"%s\": cannot initialize fhi_lib!\n", aprefix);
    return false;
  }
  paramdef_t prachp[] = ORAN_PRACH_DESC;
  int nprach = sizeofArray(prachp);
  sprintf(aprefix, "%s.%s.[%d].%s", CONFIG_STRING_ORAN, CONFIG_STRING_ORAN_FH, ru_idx, CONFIG_STRING_ORAN_PRACH);
  ret = config_get(config_get_if(), prachp, nprach, aprefix);
  if (ret < 0) {
    printf("No configuration section \"%s\": cannot initialize fhi_lib!\n", aprefix);
    return false;
  }

  memset(fh_config, 0, sizeof(*fh_config));

  fh_config->dpdk_port = ru_idx;
  fh_config->sector_id = 0;
  fh_config->nCC = 1;
  fh_config->neAxc = oai0->tx_num_channels / num_rus;
  fh_config->neAxcUl = oai0->rx_num_channels / num_rus;
  fh_config->nAntElmTRx = oai0->tx_num_channels / num_rus;
  fh_config->nDLFftSize = 0;
  fh_config->nULFftSize = 0;
  fh_config->nDLRBs = oai0->num_rb_dl;
  fh_config->nULRBs = oai0->num_rb_dl;
  fh_config->nDLAbsFrePointA = 0;
  fh_config->nULAbsFrePointA = 0;
  fh_config->nDLCenterFreqARFCN = nDLCenterFreqARFCN;
  fh_config->nULCenterFreqARFCN = nULCenterFreqARFCN;
  fh_config->ttiCb = NULL;
  fh_config->ttiCbParam = NULL;
  fh_config->Tadv_cp_dl = *gpd(fhp, nfh, ORAN_FH_CONFIG_TADV_CP_DL)->uptr;
  if (!set_maxmin_pd(fhp, nfh, ORAN_FH_CONFIG_T2A_CP_DL, &fh_config->T2a_min_cp_dl, &fh_config->T2a_max_cp_dl))
    return false;
  if (!set_maxmin_pd(fhp, nfh, ORAN_FH_CONFIG_T2A_CP_UL, &fh_config->T2a_min_cp_ul, &fh_config->T2a_max_cp_ul))
    return false;
  if (!set_maxmin_pd(fhp, nfh, ORAN_FH_CONFIG_T2A_UP, &fh_config->T2a_min_up, &fh_config->T2a_max_up))
    return false;
  if (!set_maxmin_pd(fhp, nfh, ORAN_FH_CONFIG_TA3, &fh_config->Ta3_min, &fh_config->Ta3_max))
    return false;
  if (!set_maxmin_pd(fhp, nfh, ORAN_FH_CONFIG_T1A_CP_DL, &fh_config->T1a_min_cp_dl, &fh_config->T1a_max_cp_dl))
    return false;
  if (!set_maxmin_pd(fhp, nfh, ORAN_FH_CONFIG_T1A_CP_UL, &fh_config->T1a_min_cp_ul, &fh_config->T1a_max_cp_ul))
    return false;
  if (!set_maxmin_pd(fhp, nfh, ORAN_FH_CONFIG_T1A_UP, &fh_config->T1a_min_up, &fh_config->T1a_max_up))
    return false;
  if (!set_maxmin_pd(fhp, nfh, ORAN_FH_CONFIG_TA4, &fh_config->Ta4_min, &fh_config->Ta4_max))
    return false;
  fh_config->enableCP = 1;
  fh_config->prachEnable = 1;
  fh_config->srsEnable = 0;
  fh_config->puschMaskEnable = 0;
  fh_config->puschMaskSlot = 0;
  fh_config->cp_vlan_tag = *gpd(fhp, nfh, ORAN_FH_CONFIG_CP_VLAN_TAG)->uptr;
  fh_config->up_vlan_tag = *gpd(fhp, nfh, ORAN_FH_CONFIG_UP_VLAN_TAG)->uptr;
  fh_config->debugStop = 0;
  fh_config->debugStopCount = 0;
  fh_config->DynamicSectionEna = 0;
  fh_config->GPS_Alpha = 0;
  fh_config->GPS_Beta = 0;

  if (!set_fh_prach_config(oai0, prachp, nprach, &fh_config->prach_conf))
    return false;
  if (!set_fh_srs_config(&fh_config->srs_conf))
    return false;
  if (!set_fh_frame_config(oai0, &fh_config->frame_conf))
    return false;
  if (!set_fh_ru_config(rup, nru, &fh_config->ru_conf))
    return false;

  fh_config->bbdev_enc = NULL;
  fh_config->bbdev_dec = NULL;
  // fh_config->tx_cp_eAxC2Vf [not implemented by fhi_lib]
  // fh_config->tx_up_eAxC2Vf [not implemented by fhi_lib]
  // fh_config->rx_cp_eAxC2Vf [not implemented by fhi_lib]
  // fh_config->rx_up_eAxC2Vf [not implemented by fhi_lib]
  fh_config->log_level = 1;
  fh_config->max_sections_per_slot = 8;
  fh_config->max_sections_per_symbol = 8;

  return true;
}
