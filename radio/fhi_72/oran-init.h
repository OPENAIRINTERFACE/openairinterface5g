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

#ifndef ORAN_INIT_H
#define ORAN_INIT_H

typedef struct oran_bufs {
  struct xran_flat_buffer tx[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN][XRAN_NUM_OF_SYMBOL_PER_SLOT];
  struct xran_flat_buffer tx_prbmap[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  struct xran_flat_buffer rx[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN][XRAN_NUM_OF_SYMBOL_PER_SLOT];
  struct xran_flat_buffer rx_prbmap[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];

  struct xran_flat_buffer prach[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN][XRAN_NUM_OF_SYMBOL_PER_SLOT];
  struct xran_flat_buffer prachdecomp[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN][XRAN_NUM_OF_SYMBOL_PER_SLOT];
} oran_bufs_t;

typedef struct oran_buf_list {
  // xran API requires buffer lists as structs of arrays
  struct xran_buffer_list src[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  struct xran_buffer_list srccp[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  struct xran_buffer_list dst[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  struct xran_buffer_list dstcp[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];

  struct xran_buffer_list prachdst[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];
  struct xran_buffer_list prachdstdecomp[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN];

  oran_bufs_t bufs;
} oran_buf_list_t;

typedef struct oran_port_instance_t {
  oran_buf_list_t *buf_list;
  void *instanceHandle;
  //uint32_t dpdkPoolIndex[MAX_SW_XRAN_INTERFACE_NUM];

  struct xran_cb_tag RxCbTag[XRAN_PORTS_NUM][XRAN_MAX_SECTOR_NR];
  struct xran_cb_tag PrachCbTag[XRAN_PORTS_NUM][XRAN_MAX_SECTOR_NR];
} oran_port_instance_t;

extern struct xran_fh_config gxran_fh_config[XRAN_PORTS_NUM];
extern void *gxran_handle;

struct openair0_config;
int *oai_oran_initialize(const struct openair0_config *openair0_cfg);

#endif /* ORAN_INIT_H */
