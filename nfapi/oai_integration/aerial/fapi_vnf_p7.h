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

/*! \file fapi/oai-integration/fapi_vnf_p7.h
* \brief Header file for fapi_vnf_p7.c
* \author Ruben S. Silva
* \date 2023
* \version 0.1
* \company OpenAirInterface Software Alliance
* \email: contact@openairinterface.org, rsilva@allbesmart.pt
* \note
* \warning
 */

#ifdef ENABLE_AERIAL
#ifndef OPENAIRINTERFACE_FAPI_VNF_P7_H
#define OPENAIRINTERFACE_FAPI_VNF_P7_H

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nfapi_nr_interface_scf.h"
#include "nfapi_vnf_interface.h"
#include "nfapi_vnf.h"
#include "nfapi.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "fapi_nvIPC.h"

#include "PHY/defs_eNB.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "lte-softmodem.h"

#include "common/ran_context.h"
#include "openair2/PHY_INTERFACE/queue_t.h"
#include "gnb_ind_vars.h"
#include "nfapi/open-nFAPI/vnf/inc/vnf.h"
#include "nfapi/open-nFAPI/vnf/inc/vnf_p7.h"
typedef struct {
  uint8_t enabled;
  uint32_t rx_port;
  uint32_t tx_port;
  char tx_addr[80];
} udp_data;

typedef struct {
  uint16_t index;
  uint16_t id;
  uint8_t rfs[2];
  uint8_t excluded_rfs[2];

  udp_data udp;

  char local_addr[80];
  int local_port;

  char *remote_addr;
  int remote_port;

  uint8_t duplex_mode;
  uint16_t dl_channel_bw_support;
  uint16_t ul_channel_bw_support;
  uint8_t num_dl_layers_supported;
  uint8_t num_ul_layers_supported;
  uint16_t release_supported;
  uint8_t nmm_modes_supported;

  uint8_t dl_ues_per_subframe;
  uint8_t ul_ues_per_subframe;

  uint8_t first_subframe_ind;

  // timing information recevied from the vnf
  uint8_t timing_window;
  uint8_t timing_info_mode;
  uint8_t timing_info_period;

} phy_info;

typedef struct {
  uint16_t index;
  uint16_t band;
  int16_t max_transmit_power;
  int16_t min_transmit_power;
  uint8_t num_antennas_supported;
  uint32_t min_downlink_frequency;
  uint32_t max_downlink_frequency;
  uint32_t max_uplink_frequency;
  uint32_t min_uplink_frequency;
} rf_info;

typedef struct {
  int release;
  phy_info phys[2];
  rf_info rfs[2];

  uint8_t sync_mode;
  uint8_t location_mode;
  uint8_t location_coordinates[6];
  uint32_t dl_config_timing;
  uint32_t ul_config_timing;
  uint32_t tx_timing;
  uint32_t hi_dci0_timing;

  uint16_t max_phys;
  uint16_t max_total_bw;
  uint16_t max_total_dl_layers;
  uint16_t max_total_ul_layers;
  uint8_t shared_bands;
  uint8_t shared_pa;
  int16_t max_total_power;
  uint8_t oui;

  uint8_t wireshark_test_mode;

} pnf_info;

typedef struct mac mac_t;
typedef struct mac {
  void *user_data;

  void (*dl_config_req)(mac_t *mac, nfapi_dl_config_request_t *req);
  void (*ul_config_req)(mac_t *mac, nfapi_ul_config_request_t *req);
  void (*hi_dci0_req)(mac_t *mac, nfapi_hi_dci0_request_t *req);
  void (*tx_req)(mac_t *mac, nfapi_tx_request_t *req);
} mac_t;

typedef struct {
  int local_port;
  char local_addr[80];

  unsigned timing_window;
  unsigned periodic_timing_enabled;
  unsigned aperiodic_timing_enabled;
  unsigned periodic_timing_period;

  // This is not really the right place if we have multiple PHY,
  // should be part of the phy struct
  udp_data udp;

  uint8_t thread_started;

  nfapi_vnf_p7_config_t *config;

  mac_t *mac;

} vnf_p7_info;

typedef struct {
  uint8_t wireshark_test_mode;
  pnf_info pnfs[2];
  vnf_p7_info p7_vnfs[2];

} vnf_info;

int aerial_wake_gNB_rxtx(PHY_VARS_gNB *gNB, uint16_t sfn, uint16_t slot);
int aerial_wake_eNB_rxtx(PHY_VARS_eNB *eNB, uint16_t sfn, uint16_t sf);
int aerial_phy_sync_indication(struct nfapi_vnf_p7_config *config, uint8_t sync);
int aerial_phy_slot_indication(struct nfapi_vnf_p7_config *config, uint16_t phy_id, uint16_t sfn, uint16_t slot);
int aerial_phy_harq_indication(struct nfapi_vnf_p7_config *config, nfapi_harq_indication_t *ind);
int aerial_phy_nr_crc_indication(nfapi_nr_crc_indication_t *ind);
int aerial_phy_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind);
int aerial_phy_nr_rach_indication(nfapi_nr_rach_indication_t *ind);
int aerial_phy_nr_uci_indication(nfapi_nr_uci_indication_t *ind);
int aerial_phy_srs_indication(struct nfapi_vnf_p7_config *config, nfapi_srs_indication_t *ind);
int aerial_phy_sr_indication(struct nfapi_vnf_p7_config *config, nfapi_sr_indication_t *ind);
int aerial_phy_cqi_indication(struct nfapi_vnf_p7_config *config, nfapi_cqi_indication_t *ind);
int aerial_phy_lbt_dl_indication(struct nfapi_vnf_p7_config *config, nfapi_lbt_dl_indication_t *ind);
int aerial_phy_nb_harq_indication(struct nfapi_vnf_p7_config *config, nfapi_nb_harq_indication_t *ind);
int aerial_phy_nrach_indication(struct nfapi_vnf_p7_config *config, nfapi_nrach_indication_t *ind);
int aerial_phy_nr_slot_indication(nfapi_nr_slot_indication_scf_t *ind);
int aerial_phy_nr_srs_indication(nfapi_nr_srs_indication_t *ind);
void *aerial_vnf_allocate(size_t size);
void aerial_vnf_deallocate(void *ptr);
int aerial_phy_vendor_ext(struct nfapi_vnf_p7_config *config, nfapi_p7_message_header_t *msg);
int aerial_phy_unpack_p7_vendor_extension(nfapi_p7_message_header_t *header,
                                          uint8_t **ppReadPackedMessage,
                                          uint8_t *end,
                                          nfapi_p7_codec_config_t *config);
int aerial_phy_pack_p7_vendor_extension(nfapi_p7_message_header_t *header,
                                        uint8_t **ppWritePackedMsg,
                                        uint8_t *end,
                                        nfapi_p7_codec_config_t *config);
int aerial_phy_unpack_vendor_extension_tlv(nfapi_tl_t *tl,
                                           uint8_t **ppReadPackedMessage,
                                           uint8_t *end,
                                           void **ve,
                                           nfapi_p7_codec_config_t *codec);
int aerial_phy_pack_vendor_extension_tlv(void *ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *codec);
nfapi_p7_message_header_t *aerial_phy_allocate_p7_vendor_ext(uint16_t message_id, uint16_t *msg_size);
void aerial_phy_deallocate_p7_vendor_ext(nfapi_p7_message_header_t *header);

uint8_t aerial_unpack_nr_slot_indication(uint8_t **ppReadPackedMsg,
                                         uint8_t *end,
                                         nfapi_nr_slot_indication_scf_t *msg,
                                         nfapi_p7_codec_config_t *config);
uint8_t aerial_unpack_nr_rx_data_indication(uint8_t **ppReadPackedMsg,
                                            uint8_t *end,
                                            uint8_t **pDataMsg,
                                            uint8_t *data_end,
                                            nfapi_nr_rx_data_indication_t *msg,
                                            nfapi_p7_codec_config_t *config);
uint8_t aerial_unpack_nr_crc_indication(uint8_t **ppReadPackedMsg,
                                        uint8_t *end,
                                        nfapi_nr_crc_indication_t *msg,
                                        nfapi_p7_codec_config_t *config);
uint8_t aerial_unpack_nr_uci_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config);
uint8_t aerial_unpack_nr_srs_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config);
uint8_t aerial_unpack_nr_rach_indication(uint8_t **ppReadPackedMsg,
                                         uint8_t *end,
                                         nfapi_nr_rach_indication_t *msg,
                                         nfapi_p7_codec_config_t *config);

// int fapi_nr_p7_message_pack(void *pMessageBuf, void *pPackedBuf, uint32_t packedBufLen, nfapi_p7_codec_config_t* config);
int fapi_nr_pack_and_send_p7_message(vnf_p7_t *vnf_p7, nfapi_p7_message_header_t *header);
#endif // OPENAIRINTERFACE_FAPI_VNF_P7_H
#endif
