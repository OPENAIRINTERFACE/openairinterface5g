/*
 * Copyright 2017 Cisco Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mac.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vendor_ext.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

#include <mutex>
#include <queue>
#include <list>

uint32_t rand_range(uint32_t min, uint32_t max) {
  return ((rand() % (max + 1 - min)) + min);
}

struct mac_pdu {
  mac_pdu() : buffer_len(1500), buffer(0), len(0) {
    buffer = (char *) malloc(buffer_len);
  }

  virtual ~mac_pdu() {
    free(buffer);
  }

  unsigned buffer_len;
  char *buffer;
  unsigned len;
};

class mac_private {
  std::mutex mutex;
  std::queue<mac_pdu *> rx_buffer;

  std::queue<mac_pdu *> free_store;
 public:

  mac_private(bool _wireshark_test_mode)
    : byte_count(0), tick(0), wireshark_test_mode(_wireshark_test_mode) {
  }

  mac_pdu *allocate_mac_pdu() {
    mac_pdu *pdu = 0;
    mutex.lock();

    if(free_store.empty()) {
      pdu = new mac_pdu();
    } else {
      pdu = free_store.front();
      free_store.pop();
    }

    mutex.unlock();
    return pdu;
  }

  void release_mac_pdu(mac_pdu *pdu) {
    mutex.lock();
    free_store.push(pdu);
    mutex.unlock();
  }



  void push_rx_buffer(mac_pdu *buff) {
    mutex.lock();
    rx_buffer.push(buff);
    mutex.unlock();
  }

  mac_pdu *pop_rx_buffer() {
    mac_pdu *buff = 0;
    mutex.lock();

    if(!rx_buffer.empty()) {
      buff = rx_buffer.front();
      rx_buffer.pop();
    }

    mutex.unlock();
    return buff;
  }

  uint32_t byte_count;
  uint32_t tick;

  bool wireshark_test_mode;

};

extern "C"
{
  typedef struct {
    mac_t _public;

    int rx_sock;
    int tx_sock;
    struct sockaddr_in tx_addr;

    uint32_t tx_byte_count;

    mac_private *mac;

  } mac_internal_t;

  mac_t *mac_create(uint8_t wireshark_test_mode) {
    mac_internal_t *instance = (mac_internal_t *)malloc(sizeof(mac_internal_t));
    instance->mac = new mac_private((wireshark_test_mode >= 1));
    return (mac_t *)instance;
  }

  void mac_destroy(mac_t *mac) {
    mac_internal_t *instance = (mac_internal_t *)mac;
    delete instance->mac;
    free(instance);
  }

  void *mac_rx_thread_start(void *ptr) {
    mac_internal_t *instance = (mac_internal_t *)ptr;

    while(1) {
      mac_pdu *pdu = instance->mac->allocate_mac_pdu();
      int len = recvfrom(instance->rx_sock, pdu->buffer, pdu->buffer_len, 0, 0, 0);

      if(len > 0) {
        pdu->len = len;
        instance->mac->push_rx_buffer(pdu);
      } else {
        instance->mac->release_mac_pdu(pdu);
      }
    }

    return 0;
  }

  void mac_start_data(mac_t *mac, unsigned rx_port, const char *tx_address, unsigned tx_port) {
    mac_internal_t *instance = (mac_internal_t *)mac;
    printf("[MAC] Rx Data from %u\n", rx_port);
    printf("[MAC] Tx Data to %s.%u\n", tx_address, tx_port);
    instance->rx_sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(instance->rx_sock < 0) {
      printf("[MAC] Failed to create socket\n");
      return;
    }

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(sockaddr_in));

    addr.sin_family = AF_INET;

    addr.sin_port = htons(rx_port);

    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(instance->rx_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
      printf("[MAC] Failed to bind to %u\n", rx_port);
      close(instance->rx_sock);
      return;
    }

    pthread_t mac_rx_thread;
    pthread_create(&mac_rx_thread, NULL, &mac_rx_thread_start, instance);
    instance->tx_sock = socket(AF_INET, SOCK_DGRAM, 0);
    instance->tx_addr.sin_family = AF_INET;
    instance->tx_addr.sin_port = htons(tx_port);
    instance->tx_addr.sin_addr.s_addr = inet_addr(tx_address);
  }



  void generate_test_subframe(mac_t *mac, uint16_t phy_id, uint16_t sfn_sf) {
    //mac_internal_t* instance = (mac_internal_t*)mac;
    uint8_t max_num_dl_pdus = 50;
    nfapi_dl_config_request_pdu_t dl_config_pdus[max_num_dl_pdus];
    memset(&dl_config_pdus, 0, sizeof(dl_config_pdus));
    nfapi_dl_config_request_t dl_config_req;
    memset(&dl_config_req, 0, sizeof(dl_config_req));
    dl_config_req.header.message_id = NFAPI_DL_CONFIG_REQUEST;
    dl_config_req.header.phy_id = phy_id;
    dl_config_req.sfn_sf = sfn_sf;
    dl_config_req.dl_config_request_body.tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
    dl_config_req.dl_config_request_body.number_pdu = rand_range(4, max_num_dl_pdus);
    uint16_t i = 0;

    for(i = 0; i < dl_config_req.dl_config_request_body.number_pdu; ++i) {
      dl_config_pdus[i].pdu_type = rand_range(0, 11);

      switch(dl_config_pdus[i].pdu_type) {
        case NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE: {
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.dci_format = rand_range(0, 9);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.cce_idx = rand_range(0, 255);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level  = rand_range(0, 32);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti = rand_range(0, (uint16_t)-1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.resource_allocation_type = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.virtual_resource_block_assignment_flag = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = rand_range(0, 320000);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.mcs_1 = rand_range(0, 31);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1 = rand_range(0, 3);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1 = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.transport_block_to_codeword_swap_flag = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.mcs_2 = rand_range(0, 31);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_2 = rand_range(0, 31);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_2 = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.harq_process = rand_range(0, 31);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.tpmi = rand_range(0, 15);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.pmi = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.precoding_information = rand_range(0, 63);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.tpc = rand_range(0, 3);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.downlink_assignment_index = rand_range(0, 15);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.ngap = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.transport_block_size_index = rand_range(0, 31);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.downlink_power_offset = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.allocate_prach_flag = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.preamble_index = rand_range(0, 63);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.prach_mask_index = rand_range(0, 15);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.rnti_type = rand_range(0, 3);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel8.transmission_power = rand_range(0, 10000);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel9.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL9_TAG;
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel9.mcch_flag = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel9.mcch_change_notification = rand_range(0, 255);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel9.scrambling_identity = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel10.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL10_TAG;
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel10.cross_carrier_scheduling_flag = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel10.carrier_indicator = rand_range(0, 7);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel10.srs_flag = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel10.srs_request = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel10.antenna_ports_scrambling_and_layers = rand_range(0, 15);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel10.total_dci_length_including_padding = rand_range(0, 255);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel10.n_dl_rb = rand_range(0, 100);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel11.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL11_TAG;
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel11.harq_ack_resource_offset = rand_range(0, 3);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel11.pdsch_re_mapping_quasi_co_location_indicator = rand_range(0, 3);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel12.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL12_TAG;
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel12.primary_cell_type = rand_range(0, 2);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel12.ul_dl_configuration_flag = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel12.number_ul_dl_configurations = 2;
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel12.ul_dl_configuration_indication[0] = rand_range(1, 5);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel12.ul_dl_configuration_indication[1] = rand_range(1, 5);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL13_TAG;
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.laa_end_partial_sf_flag = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.laa_end_partial_sf_configuration = rand_range(0, 255);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.initial_lbt_sf = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.codebook_size_determination = rand_range(0, 1);
          dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.drms_table_flag = rand_range(0, 1);

          // if the tpm extention is present of not.
          if(rand_range(0, 1)) {
            dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm_struct_flag = rand_range(0, 1);
            dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm.num_prb_per_subband = rand_range(0, 8);
            dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm.number_of_subbands = rand_range(0, 13);
            dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm.num_antennas = 1; // rand_range(0, 8);

            for(int j = 0; j < dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm.number_of_subbands; ++j) {
              dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[j].subband_index = j;
              dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[j].scheduled_ues = 1; //rand_range(1, 4);

              for(int k = 0; k < dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm.num_antennas; ++k)
                for(int l = 0; l < dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[j].scheduled_ues; ++l)
                  dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm.subband_info[j].precoding_value[k][l] = rand_range(0, 65535);
            }
          } else {
            dl_config_pdus[i].dci_dl_pdu.dci_dl_pdu_rel13.tpm_struct_flag = 0;
          }
        }
        break;

        case NFAPI_DL_CONFIG_BCH_PDU_TYPE: {
          dl_config_pdus[i].bch_pdu.bch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_BCH_PDU_REL8_TAG;
          dl_config_pdus[i].bch_pdu.bch_pdu_rel8.length = rand_range(0, 42);
          dl_config_pdus[i].bch_pdu.bch_pdu_rel8.pdu_index = rand_range(0, 65535);
          dl_config_pdus[i].bch_pdu.bch_pdu_rel8.transmission_power = rand_range(0, 10000);
        }
        break;

        case NFAPI_DL_CONFIG_MCH_PDU_TYPE: {
          dl_config_pdus[i].mch_pdu.mch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_MCH_PDU_REL8_TAG;
          dl_config_pdus[i].mch_pdu.mch_pdu_rel8.length = rand_range(0, 42);
          dl_config_pdus[i].mch_pdu.mch_pdu_rel8.pdu_index = rand_range(0, 65535);
          dl_config_pdus[i].mch_pdu.mch_pdu_rel8.rnti = 0xFFFD;
          dl_config_pdus[i].mch_pdu.mch_pdu_rel8.resource_allocation_type = 0;
          dl_config_pdus[i].mch_pdu.mch_pdu_rel8.resource_block_coding = 0;
          dl_config_pdus[i].mch_pdu.mch_pdu_rel8.modulation = rand_range(0, 8);
          dl_config_pdus[i].mch_pdu.mch_pdu_rel8.transmission_power = rand_range(0, 10000);
          dl_config_pdus[i].mch_pdu.mch_pdu_rel8.mbsfn_area_id = rand_range(0, 255);
        }
        break;

        case NFAPI_DL_CONFIG_DLSCH_PDU_TYPE: {
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL8_TAG;
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.length = rand_range(0, 42);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.pdu_index = rand_range(0, 65535);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.rnti = rand_range(1, 65535);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type = rand_range(0, 5);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.resource_block_coding = rand_range(0, 32000);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.modulation = rand_range(2, 8);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.redundancy_version = rand_range(0, 3);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.transport_blocks = rand_range(1, 2);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.transmission_scheme = rand_range(0, 13);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.number_of_layers = rand_range(1, 8);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.number_of_subbands = 2; //rand_range(0, 13);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.codebook_index[0] = rand_range(0, 15);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.codebook_index[1] = rand_range(0, 15);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity = rand_range(0, 14);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.pa = rand_range(0, 7);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.ngap = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.nprb = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.transmission_mode = rand_range(1, 10);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband = 2; //rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.num_bf_vector = 2; //rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.bf_vector[0].subband_index = rand_range(0, 4);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.bf_vector[0].num_antennas = 1;
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.bf_vector[0].bf_value[0] = rand_range(0, 128);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.bf_vector[1].subband_index = rand_range(0, 4);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.bf_vector[1].num_antennas = 1;
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel8.bf_vector[1].bf_value[0] = rand_range(0, 128);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel9.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL9_TAG;
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel9.nscid = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel10.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL10_TAG;
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel10.csi_rs_flag = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel10.csi_rs_resource_config_r10 = 0;
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel10.csi_rs_zero_tx_power_resource_config_bitmap_r10 = rand_range(0, 65535);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel10.csi_rs_number_nzp_configuration = 1;
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel10.csi_rs_resource_config[0] = rand_range(0, 31);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel10.pdsch_start = rand_range(0, 4);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel11.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL11_TAG;
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel11.drms_config_flag = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel11.drms_scrambling = rand_range(0, 503);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel11.csi_config_flag = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel11.csi_scrambling = rand_range(0, 503);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel11.pdsch_re_mapping_flag = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel11.pdsch_re_mapping_atenna_ports = rand_range(1,4);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel11.pdsch_re_mapping_freq_shift = rand_range(0, 5);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel12.tl.tag = NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL12_TAG;
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel12.altcqi_table_r12 = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel12.maxlayers = rand_range(1, 8);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel12.n_dl_harq = rand_range(0, 255);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel13.dwpts_symbols = rand_range(3, 14);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel13.initial_lbt_sf = rand_range(0, 1);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel13.ue_type = rand_range(0, 2);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type = rand_range(0, 2);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io = rand_range(0, 10239);
          dl_config_pdus[i].dlsch_pdu.dlsch_pdu_rel13.drms_table_flag = rand_range(0, 1);
        }
        break;

        case NFAPI_DL_CONFIG_PCH_PDU_TYPE: {
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_PCH_PDU_REL8_TAG;
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.length = rand_range(0, 42);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.pdu_index = rand_range(0, 65535);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.p_rnti = 0xFFFE;
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.resource_allocation_type = rand_range(2, 6);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.virtual_resource_block_assignment_flag = rand_range(0, 1);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.resource_block_coding = rand_range(0, 34000);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.mcs = 0;
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.redundancy_version = 0;
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.number_of_transport_blocks = 1;
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.transport_block_to_codeword_swap_flag = 0;
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.transmission_scheme = rand_range(1, 6);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.number_of_layers = rand_range(1, 4);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.codebook_index = 0;
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.ue_category_capacity = rand_range(0, 14);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.pa = rand_range(0, 7);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.transmission_power = rand_range(0, 10000);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.nprb = rand_range(0, 1);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel8.ngap = rand_range(0, 1);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_PCH_PDU_REL13_TAG;
          dl_config_pdus[i].pch_pdu.pch_pdu_rel13.ue_mode = rand_range(0, 1);
          dl_config_pdus[i].pch_pdu.pch_pdu_rel13.initial_transmission_sf_io = rand_range(0, 10239);
        }
        break;

        case NFAPI_DL_CONFIG_PRS_PDU_TYPE: {
          dl_config_pdus[i].prs_pdu.prs_pdu_rel9.tl.tag = NFAPI_DL_CONFIG_REQUEST_PRS_PDU_REL9_TAG;
          dl_config_pdus[i].prs_pdu.prs_pdu_rel9.transmission_power = rand_range(0, 10000);
          dl_config_pdus[i].prs_pdu.prs_pdu_rel9.prs_bandwidth = rand_range(6, 100);
          dl_config_pdus[i].prs_pdu.prs_pdu_rel9.prs_cyclic_prefix_type = rand_range(0, 1);
          dl_config_pdus[i].prs_pdu.prs_pdu_rel9.prs_muting = rand_range(0, 1);
        }
        break;

        case NFAPI_DL_CONFIG_CSI_RS_PDU_TYPE: {
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel10.tl.tag = NFAPI_DL_CONFIG_REQUEST_CSI_RS_PDU_REL10_TAG;
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel10.csi_rs_antenna_port_count_r10 = rand_range(1, 16);
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel10.csi_rs_resource_config_r10 = 0;
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel10.transmission_power = rand_range(0, 10000);
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel10.csi_rs_zero_tx_power_resource_config_bitmap_r10 = rand_range(0, 8);
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel10.csi_rs_number_of_nzp_configuration = 2;
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel10.csi_rs_resource_config[0] = rand_range(0, 31);
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel10.csi_rs_resource_config[1] = rand_range(0, 31);
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_CSI_RS_PDU_REL13_TAG;
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel13.csi_rs_class = rand_range(0, 2);
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel13.cdm_type = rand_range(0, 1);
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel13.num_bf_vector = 0; // set to zero as not clear how to handle bf value array
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel13.bf_vector[0].csi_rs_resource_index = rand_range(0, 7);
          dl_config_pdus[i].csi_rs_pdu.csi_rs_pdu_rel13.bf_vector[0].bf_value[0] = 42;
        }
        break;

        case NFAPI_DL_CONFIG_EPDCCH_DL_PDU_TYPE: {
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL8_TAG;
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.dci_format = rand_range(0, 9);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.cce_idx = rand_range(0, 255);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.aggregation_level  = rand_range(0, 32);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.rnti = rand_range(0, (uint16_t)-1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.resource_allocation_type = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.virtual_resource_block_assignment_flag = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.resource_block_coding = rand_range(0, 320000);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.mcs_1 = rand_range(0, 31);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.redundancy_version_1 = rand_range(0, 3);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.new_data_indicator_1 = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.transport_block_to_codeword_swap_flag = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.mcs_2 = rand_range(0, 31);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.redundancy_version_2 = rand_range(0, 31);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.new_data_indicator_2 = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.harq_process = rand_range(0, 31);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.tpmi = rand_range(0, 15);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.pmi = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.precoding_information = rand_range(0, 63);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.tpc = rand_range(0, 3);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.downlink_assignment_index = rand_range(0, 15);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.ngap = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.transport_block_size_index = rand_range(0, 31);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.downlink_power_offset = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.allocate_prach_flag = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.preamble_index = rand_range(0, 63);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.prach_mask_index = rand_range(0, 15);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.rnti_type = rand_range(0, 3);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel8.transmission_power = rand_range(0, 10000);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel9.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL9_TAG;
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel9.mcch_flag = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel9.mcch_change_notification = rand_range(0, 255);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel9.scrambling_identity = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel10.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL10_TAG;
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel10.cross_carrier_scheduling_flag = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel10.carrier_indicator = rand_range(0, 7);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel10.srs_flag = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel10.srs_request = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel10.antenna_ports_scrambling_and_layers = rand_range(0, 15);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel10.total_dci_length_including_padding = rand_range(0, 255);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel10.n_dl_rb = rand_range(0, 100);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel11.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL11_TAG;
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel11.harq_ack_resource_offset = rand_range(0, 3);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel11.pdsch_re_mapping_quasi_co_location_indicator = rand_range(0, 3);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel12.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL12_TAG;
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel12.primary_cell_type = rand_range(0, 2);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel12.ul_dl_configuration_flag = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel12.number_ul_dl_configurations = 2;
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel12.ul_dl_configuration_indication[0] = rand_range(1, 5);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel12.ul_dl_configuration_indication[1] = rand_range(1, 5);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL13_TAG;
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel13.laa_end_partial_sf_flag = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel13.laa_end_partial_sf_configuration = rand_range(0, 255);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel13.initial_lbt_sf = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel13.codebook_size_determination = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_pdu_rel13.drms_table_flag = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PARAM_REL11_TAG;
          dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.epdcch_resource_assignment_flag = rand_range(0, 1);
          dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.epdcch_id = rand_range(0, 503);
          dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.epdcch_start_symbol = rand_range(1, 4);
          dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.epdcch_num_prb = rand_range(2, 8);

          for(int j = 0; j < dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.epdcch_num_prb; ++j)
            dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.epdcch_prb_index[j] = rand_range(0, 99);

          dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.bf_vector.subband_index = rand_range(0, 25);
          dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.bf_vector.num_antennas= rand_range(1, 4);

          for(int j = 0; j < dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.epdcch_num_prb; ++j)
            dl_config_pdus[i].epdcch_pdu.epdcch_params_rel11.bf_vector.bf_value[j] = rand_range(0, 65535);

          dl_config_pdus[i].epdcch_pdu.epdcch_params_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_EPDCCH_PARAM_REL13_TAG;
          dl_config_pdus[i].epdcch_pdu.epdcch_params_rel13.dwpts_symbols = rand_range(3, 14);
          dl_config_pdus[i].epdcch_pdu.epdcch_params_rel13.initial_lbt_sf = rand_range(0, 1);
        }
        break;

        case NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE: {
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_MPDCCH_PDU_REL13_TAG;
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_narrow_band = rand_range(0, 15);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.number_of_prb_pairs = rand_range(2, 6);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.resource_block_assignment = rand_range(0, 14);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_tansmission_type = rand_range(0, 1);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.start_symbol = rand_range(1, 4);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.ecce_index = rand_range(0, 22);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.aggregation_level = rand_range(2, 24);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.rnti_type = rand_range(0, 4);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.rnti = rand_range(1, 65535);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.ce_mode = rand_range(1, 2);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.drms_scrambling_init = rand_range(0, 503);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.initial_transmission_sf_io = rand_range(0, 10239);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.transmission_power = rand_range(0, 10000);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.dci_format = rand_range(10, 12);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.resource_block_coding = rand_range(0, 0xFFFF);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.mcs = rand_range(0, 15);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.pdsch_reptition_levels = rand_range(1, 8);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.redundancy_version = rand_range(0, 3);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.new_data_indicator = rand_range(0, 1);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.harq_process = rand_range(0, 15);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.tpmi_length = rand_range(0, 4);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.tpmi = rand_range(0, 15);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.pmi_flag = rand_range(0, 1);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.pmi = rand_range(0, 1);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.harq_resource_offset = rand_range(0, 3);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.dci_subframe_repetition_number = rand_range(1, 4);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.tpc = rand_range(0, 3);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index_length = rand_range(0, 4);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index = rand_range(0, 15);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.allocate_prach_flag = rand_range(0, 63);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.preamble_index = rand_range(0, 15);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.prach_mask_index = rand_range(0, 3);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.starting_ce_level = rand_range(0, 1);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.srs_request = rand_range(0, 1);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity_flag = rand_range(0, 1);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity = rand_range(0, 3);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.frequency_hopping_enabled_flag = rand_range(0, 1);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.paging_direct_indication_differentiation_flag = rand_range(0, 1);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.direct_indication = rand_range(0, 255);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.total_dci_length_including_padding = rand_range(0, 1);
          dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.number_of_tx_antenna_ports = rand_range(0, 8);

          for(int j = 0 ; j < dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.number_of_tx_antenna_ports; ++j)
            dl_config_pdus[i].mpdcch_pdu.mpdcch_pdu_rel13.precoding_value[j] = rand_range(0, 65535);
        }
        break;

        case NFAPI_DL_CONFIG_NBCH_PDU_TYPE: {
          dl_config_pdus[i].nbch_pdu.nbch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_NBCH_PDU_REL13_TAG;
          dl_config_pdus[i].nbch_pdu.nbch_pdu_rel13.length = rand_range(0, 5555);
          dl_config_pdus[i].nbch_pdu.nbch_pdu_rel13.pdu_index = rand_range(0, 65535);
          dl_config_pdus[i].nbch_pdu.nbch_pdu_rel13.transmission_power = rand_range(0, 10000);
          dl_config_pdus[i].nbch_pdu.nbch_pdu_rel13.hyper_sfn_2_lsbs = rand_range(0, 3);
        }
        break;

        case NFAPI_DL_CONFIG_NPDCCH_PDU_TYPE: {
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_NPDCCH_PDU_REL13_TAG;
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.length = rand_range(0, 5555);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.pdu_index = rand_range(0, 65535);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.ncce_index = rand_range(0, 1);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.aggregation_level = rand_range(1, 2);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.start_symbol = rand_range(0, 4);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.rnti_type = rand_range(0, 3);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.rnti = rand_range(1, 65535);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.scrambling_reinitialization_batch_index = rand_range(1, 4);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.nrs_antenna_ports_assumed_by_the_ue = rand_range(1, 2);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.dci_format = rand_range(0, 1);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.scheduling_delay = rand_range(0, 7);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.resource_assignment = rand_range(0, 7);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.repetition_number = rand_range(0, 15);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.mcs = rand_range(0, 13);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.new_data_indicator = rand_range(0, 1);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.harq_ack_resource = rand_range(0, 15);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.npdcch_order_indication = rand_range(0, 1);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.starting_number_of_nprach_repetitions = rand_range(0, 3);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.subcarrier_indication_of_nprach = rand_range(0, 63);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.paging_direct_indication_differentation_flag = rand_range(0, 1);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.direct_indication = rand_range(0, 255);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.dci_subframe_repetition_number  = rand_range(0, 7);
          dl_config_pdus[i].npdcch_pdu.npdcch_pdu_rel13.total_dci_length_including_padding = rand_range(0, 255);
        }
        break;

        case NFAPI_DL_CONFIG_NDLSCH_PDU_TYPE: {
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.tl.tag = NFAPI_DL_CONFIG_REQUEST_NDLSCH_PDU_REL13_TAG;
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.length = rand_range(0, 5555);
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.pdu_index = rand_range(0, 65535);
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.start_symbol = rand_range(0, 4);
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.rnti_type = rand_range(0, 1);
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.rnti = rand_range(1, 65535);
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.resource_assignment = rand_range(0, 7);
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.repetition_number = rand_range(0, 15);
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.modulation = 2;
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.number_of_subframes_for_resource_assignment = rand_range(1, 10);
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.scrambling_sequence_initialization_cinit = rand_range(0, 65535);
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.sf_idx = rand_range(1, 10240);
          dl_config_pdus[i].ndlsch_pdu.ndlsch_pdu_rel13.nrs_antenna_ports_assumed_by_the_ue = rand_range(1, 2);
        }
        break;
      };
    }

    dl_config_req.dl_config_request_body.dl_config_pdu_list = dl_config_pdus;
    mac->dl_config_req(mac, &dl_config_req);
    uint8_t num_ul_pdus = 18;
    nfapi_ul_config_request_pdu_t ul_config_pdus[num_ul_pdus];
    memset(&ul_config_pdus, 0, sizeof(ul_config_pdus));
    nfapi_ul_config_request_t ul_config_req;
    memset(&ul_config_req, 0, sizeof(ul_config_req));
    ul_config_req.header.message_id = NFAPI_UL_CONFIG_REQUEST;
    ul_config_req.header.phy_id = phy_id;
    ul_config_req.sfn_sf = sfn_sf;
    ul_config_req.ul_config_request_body.tl.tag = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;
    ul_config_req.ul_config_request_body.number_of_pdus = num_ul_pdus;
    ul_config_req.ul_config_request_body.rach_prach_frequency_resources = rand_range(0, 255);
    ul_config_req.ul_config_request_body.srs_present = rand_range(0, 1);
    auto ul_config_ulsch_pdu_test_gen = [](nfapi_ul_config_ulsch_pdu& ulsch_pdu) {
      ulsch_pdu.ulsch_pdu_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL8_TAG;
      ulsch_pdu.ulsch_pdu_rel8.handle = rand_range(0, 50000);
      ulsch_pdu.ulsch_pdu_rel8.size = rand_range(0, 32000);
      ulsch_pdu.ulsch_pdu_rel8.rnti = rand_range(1, 65535);
      ulsch_pdu.ulsch_pdu_rel8.resource_block_start = rand_range(0, 99);
      ulsch_pdu.ulsch_pdu_rel8.number_of_resource_blocks = rand_range(1, 100);
      ulsch_pdu.ulsch_pdu_rel8.modulation_type = rand_range(2, 6);
      ulsch_pdu.ulsch_pdu_rel8.cyclic_shift_2_for_drms = rand_range(0, 7);
      ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_enabled_flag = rand_range(0, 1);
      ulsch_pdu.ulsch_pdu_rel8.frequency_hopping_bits = rand_range(0, 3);
      ulsch_pdu.ulsch_pdu_rel8.new_data_indication = rand_range(0, 1);
      ulsch_pdu.ulsch_pdu_rel8.redundancy_version = rand_range(0, 3);
      ulsch_pdu.ulsch_pdu_rel8.harq_process_number = rand_range(0, 15);
      ulsch_pdu.ulsch_pdu_rel8.ul_tx_mode = rand_range(0, 1);
      ulsch_pdu.ulsch_pdu_rel8.current_tx_nb = rand_range(0, 5);
      ulsch_pdu.ulsch_pdu_rel8.n_srs = rand_range(0, 1);
      ulsch_pdu.ulsch_pdu_rel10.tl.tag = NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL10_TAG;
      ulsch_pdu.ulsch_pdu_rel10.resource_allocation_type = rand_range(0, 1);
      ulsch_pdu.ulsch_pdu_rel10.resource_block_coding = rand_range(0, 35000);
      ulsch_pdu.ulsch_pdu_rel10.transport_blocks = rand_range(1, 2);
      ulsch_pdu.ulsch_pdu_rel10.transmission_scheme = rand_range(0, 1);
      ulsch_pdu.ulsch_pdu_rel10.number_of_layers = rand_range(1, 4);
      ulsch_pdu.ulsch_pdu_rel10.codebook_index = rand_range(0, 23);
      ulsch_pdu.ulsch_pdu_rel10.disable_sequence_hopping_flag = rand_range(0, 1);
      ulsch_pdu.ulsch_pdu_rel11.tl.tag = NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL11_TAG;
      ulsch_pdu.ulsch_pdu_rel11.virtual_cell_id_enabled_flag = rand_range(0, 1);
      ulsch_pdu.ulsch_pdu_rel11.npusch_identity = rand_range(0, 509);
      ulsch_pdu.ulsch_pdu_rel11.dmrs_config_flag = rand_range(0, 1);
      ulsch_pdu.ulsch_pdu_rel11.ndmrs_csh_identity = rand_range(0, 509);
      ulsch_pdu.ulsch_pdu_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL13_TAG;
      ulsch_pdu.ulsch_pdu_rel13.ue_type = rand_range(0, 2);
      ulsch_pdu.ulsch_pdu_rel13.total_number_of_repetitions = rand_range(1, 2048);
      ulsch_pdu.ulsch_pdu_rel13.repetition_number = rand_range(1, 2048);
      ulsch_pdu.ulsch_pdu_rel13.initial_transmission_sf_io = rand_range(0, 10239);
      ulsch_pdu.ulsch_pdu_rel13.empty_symbols_due_to_re_tunning = rand_range(0, 8);
    };
    auto ul_config_cqi_ri_info_test_gen = [](nfapi_ul_config_cqi_ri_information& cqi_ri_information) {
      cqi_ri_information.cqi_ri_information_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL8_TAG;
      cqi_ri_information.cqi_ri_information_rel8.dl_cqi_pmi_size_rank_1 = rand_range(0, 255);
      cqi_ri_information.cqi_ri_information_rel8.dl_cqi_pmi_size_rank_greater_1 = rand_range(0, 255);
      cqi_ri_information.cqi_ri_information_rel8.ri_size = rand_range(0, 3);
      cqi_ri_information.cqi_ri_information_rel8.delta_offset_cqi = rand_range(0, 15);
      cqi_ri_information.cqi_ri_information_rel8.delta_offset_ri = rand_range(0, 15);
      cqi_ri_information.cqi_ri_information_rel9.tl.tag = NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL9_TAG;
      cqi_ri_information.cqi_ri_information_rel9.report_type = 1; // rand_range(0, 1);
      cqi_ri_information.cqi_ri_information_rel9.delta_offset_cqi = rand_range(0, 15);
      cqi_ri_information.cqi_ri_information_rel9.delta_offset_ri = rand_range(0, 15);

      if(cqi_ri_information.cqi_ri_information_rel9.report_type == 0) {
        cqi_ri_information.cqi_ri_information_rel9.periodic_cqi_pmi_ri_report.dl_cqi_pmi_ri_size = rand_range(0, 255);
        cqi_ri_information.cqi_ri_information_rel9.periodic_cqi_pmi_ri_report.control_type = rand_range(0, 1);
      } else {
        cqi_ri_information.cqi_ri_information_rel9.aperiodic_cqi_pmi_ri_report.number_of_cc = 1;
        cqi_ri_information.cqi_ri_information_rel9.aperiodic_cqi_pmi_ri_report.cc[0].ri_size = rand_range(0, 3);
        cqi_ri_information.cqi_ri_information_rel9.aperiodic_cqi_pmi_ri_report.cc[0].dl_cqi_pmi_size[0] = rand_range(0, 255);
      }

      cqi_ri_information.cqi_ri_information_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL13_TAG;
      cqi_ri_information.cqi_ri_information_rel13.report_type = rand_range(0, 1);

      if(cqi_ri_information.cqi_ri_information_rel13.report_type == 0) {
        cqi_ri_information.cqi_ri_information_rel13.periodic_cqi_pmi_ri_report.dl_cqi_pmi_ri_size_2 = rand_range(255, 10000);
      }
    };
    auto ul_config_init_tx_params_test_gen = [](nfapi_ul_config_initial_transmission_parameters& initial_transmission_parameters) {
      initial_transmission_parameters.initial_transmission_parameters_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG;
      initial_transmission_parameters.initial_transmission_parameters_rel8.n_srs_initial = rand_range(0, 1);
      initial_transmission_parameters.initial_transmission_parameters_rel8.initial_number_of_resource_blocks = rand_range(1, 100);
    };
    auto ul_config_harqinfo_test_gen = [](nfapi_ul_config_ulsch_harq_information& harq_information) {
      harq_information.harq_information_rel10.tl.tag = NFAPI_UL_CONFIG_REQUEST_ULSCH_HARQ_INFORMATION_REL10_TAG;
      harq_information.harq_information_rel10.harq_size = rand_range(0, 21);
      harq_information.harq_information_rel10.delta_offset_harq = rand_range(0, 15);
      harq_information.harq_information_rel10.ack_nack_mode = rand_range(0, 5);
      harq_information.harq_information_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_ULSCH_HARQ_INFORMATION_REL13_TAG;
      harq_information.harq_information_rel13.harq_size_2 = rand_range(0, 21);
      harq_information.harq_information_rel13.delta_offset_harq_2 = rand_range(0, 15);
    };
    auto ul_config_sr_info_test_gen = [](nfapi_ul_config_sr_information& sr_info) {
      sr_info.sr_information_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL8_TAG;
      sr_info.sr_information_rel8.pucch_index = rand_range(0, 2047);
      sr_info.sr_information_rel10.tl.tag = NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL10_TAG;
      sr_info.sr_information_rel10.number_of_pucch_resources = rand_range(1, 2);
      sr_info.sr_information_rel10.pucch_index_p1 = rand_range(0, 2047);
    };
    auto ul_config_ue_info_test_gen = [](nfapi_ul_config_ue_information& ue_information) {
      ue_information.ue_information_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG;
      ue_information.ue_information_rel8.handle = rand_range(0, 99999);
      ue_information.ue_information_rel8.rnti = rand_range(1, 65535);
      ue_information.ue_information_rel11.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL11_TAG;
      ue_information.ue_information_rel11.virtual_cell_id_enabled_flag = rand_range(0, 1);
      ue_information.ue_information_rel11.npusch_identity = rand_range(0, 503);
      ue_information.ue_information_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL13_TAG;
      ue_information.ue_information_rel13.ue_type = rand_range(0, 2);
      ue_information.ue_information_rel13.empty_symbols = rand_range(0, 2);
      ue_information.ue_information_rel13.total_number_of_repetitions = rand_range(1, 32);
      ue_information.ue_information_rel13.repetition_number = rand_range(1, 32);
    };
    auto ul_config_cqi_info_test_gen = [](nfapi_ul_config_cqi_information& cqi_information) {
      cqi_information.cqi_information_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL8_TAG;
      cqi_information.cqi_information_rel8.pucch_index = rand_range(0, 1184);
      cqi_information.cqi_information_rel8.dl_cqi_pmi_size = rand_range(0, 255);
      cqi_information.cqi_information_rel10.tl.tag = NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL10_TAG;
      cqi_information.cqi_information_rel10.number_of_pucch_resource = rand_range(1, 2);
      cqi_information.cqi_information_rel10.pucch_index_p1 = rand_range(0, 1184);
      cqi_information.cqi_information_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL13_TAG;
      cqi_information.cqi_information_rel13.csi_mode = rand_range(0, 2);
      cqi_information.cqi_information_rel13.dl_cqi_pmi_size_2 = rand_range(0, 999);
      cqi_information.cqi_information_rel13.starting_prb = rand_range(0, 109);
      cqi_information.cqi_information_rel13.n_prb = rand_range(0, 7);
      cqi_information.cqi_information_rel13.cdm_index = rand_range(0, 1);
      cqi_information.cqi_information_rel13.n_srs = rand_range(0, 1);
    };
    auto ul_config_harq_info_test_gen = [](nfapi_ul_config_harq_information& harq_information) {
      harq_information.harq_information_rel10_tdd.tl.tag = NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL10_TDD_TAG;
      harq_information.harq_information_rel10_tdd.harq_size = rand_range(0, 21);
      harq_information.harq_information_rel10_tdd.ack_nack_mode = rand_range(0, 5);
      harq_information.harq_information_rel10_tdd.number_of_pucch_resources = rand_range(0, 4);
      harq_information.harq_information_rel10_tdd.n_pucch_1_0 = rand_range(0, 2047);
      harq_information.harq_information_rel10_tdd.n_pucch_1_1 = rand_range(0, 2047);
      harq_information.harq_information_rel10_tdd.n_pucch_1_2 = rand_range(0, 2047);
      harq_information.harq_information_rel10_tdd.n_pucch_1_3 = rand_range(0, 2047);
      harq_information.harq_information_rel8_fdd.tl.tag = NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL8_FDD_TAG;
      harq_information.harq_information_rel8_fdd.n_pucch_1_0 = rand_range(0, 2047);
      harq_information.harq_information_rel8_fdd.harq_size = rand_range(1, 2);
      harq_information.harq_information_rel9_fdd.tl.tag = NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL9_FDD_TAG;
      harq_information.harq_information_rel9_fdd.harq_size = rand_range(1, 10);
      harq_information.harq_information_rel9_fdd.ack_nack_mode = rand_range(0, 4);
      harq_information.harq_information_rel9_fdd.number_of_pucch_resources = rand_range(0, 4);
      harq_information.harq_information_rel9_fdd.n_pucch_1_0 = rand_range(0, 2047);
      harq_information.harq_information_rel9_fdd.n_pucch_1_1 = rand_range(0, 2047);
      harq_information.harq_information_rel9_fdd.n_pucch_1_2 = rand_range(0, 2047);
      harq_information.harq_information_rel9_fdd.n_pucch_1_3 = rand_range(0, 2047);
      harq_information.harq_information_rel11.tl.tag = NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL11_TAG;
      harq_information.harq_information_rel11.num_ant_ports = rand_range(1, 2);
      harq_information.harq_information_rel11.n_pucch_2_0 = rand_range(0, 2047);
      harq_information.harq_information_rel11.n_pucch_2_1 = rand_range(0, 2047);
      harq_information.harq_information_rel11.n_pucch_2_2 = rand_range(0, 2047);
      harq_information.harq_information_rel11.n_pucch_2_3 = rand_range(0, 2047);
      harq_information.harq_information_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL13_TAG;
      harq_information.harq_information_rel13.harq_size_2 = rand_range(0, 999);
      harq_information.harq_information_rel13.starting_prb = rand_range(0, 109);
      harq_information.harq_information_rel13.n_prb = rand_range(0, 7);
      harq_information.harq_information_rel13.cdm_index = rand_range(0, 1);
      harq_information.harq_information_rel13.n_srs = rand_range(0, 1);
    };
    ul_config_pdus[0].pdu_type = NFAPI_UL_CONFIG_ULSCH_PDU_TYPE;
    ul_config_ulsch_pdu_test_gen(ul_config_pdus[0].ulsch_pdu);
    ul_config_pdus[1].pdu_type = NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE;
    ul_config_ulsch_pdu_test_gen(ul_config_pdus[1].ulsch_cqi_ri_pdu.ulsch_pdu);
    ul_config_cqi_ri_info_test_gen(ul_config_pdus[1].ulsch_cqi_ri_pdu.cqi_ri_information);
    ul_config_init_tx_params_test_gen(ul_config_pdus[1].ulsch_cqi_ri_pdu.initial_transmission_parameters);
    ul_config_pdus[2].pdu_type = NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE;
    ul_config_ulsch_pdu_test_gen(ul_config_pdus[2].ulsch_harq_pdu.ulsch_pdu);
    ul_config_harqinfo_test_gen(ul_config_pdus[2].ulsch_harq_pdu.harq_information);
    ul_config_init_tx_params_test_gen(ul_config_pdus[2].ulsch_harq_pdu.initial_transmission_parameters);
    ul_config_pdus[3].pdu_type = NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE;
    ul_config_ulsch_pdu_test_gen(ul_config_pdus[3].ulsch_cqi_harq_ri_pdu.ulsch_pdu);
    ul_config_cqi_ri_info_test_gen(ul_config_pdus[3].ulsch_cqi_harq_ri_pdu.cqi_ri_information);
    ul_config_harqinfo_test_gen(ul_config_pdus[3].ulsch_cqi_harq_ri_pdu.harq_information);
    ul_config_init_tx_params_test_gen(ul_config_pdus[3].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters);
    ul_config_pdus[4].pdu_type = NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE;
    ul_config_ue_info_test_gen(ul_config_pdus[4].uci_cqi_pdu.ue_information);
    ul_config_cqi_info_test_gen(ul_config_pdus[4].uci_cqi_pdu.cqi_information);
    ul_config_pdus[5].pdu_type = NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE;
    ul_config_ue_info_test_gen(ul_config_pdus[5].uci_sr_pdu.ue_information);
    ul_config_sr_info_test_gen(ul_config_pdus[5].uci_sr_pdu.sr_information);
    ul_config_pdus[6].pdu_type = NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE;
    ul_config_ue_info_test_gen(ul_config_pdus[6].uci_harq_pdu.ue_information);
    ul_config_harq_info_test_gen(ul_config_pdus[6].uci_harq_pdu.harq_information);
    ul_config_pdus[7].pdu_type = NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE;
    ul_config_ue_info_test_gen(ul_config_pdus[7].uci_sr_harq_pdu.ue_information);
    ul_config_sr_info_test_gen(ul_config_pdus[7].uci_sr_harq_pdu.sr_information);
    ul_config_harq_info_test_gen(ul_config_pdus[7].uci_sr_harq_pdu.harq_information);
    ul_config_pdus[8].pdu_type = NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE;
    ul_config_ue_info_test_gen(ul_config_pdus[8].uci_cqi_harq_pdu.ue_information);
    ul_config_cqi_info_test_gen(ul_config_pdus[8].uci_cqi_harq_pdu.cqi_information);
    ul_config_harq_info_test_gen(ul_config_pdus[8].uci_cqi_harq_pdu.harq_information);
    ul_config_pdus[9].pdu_type = NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE;
    ul_config_ue_info_test_gen(ul_config_pdus[9].uci_cqi_sr_pdu.ue_information);
    ul_config_cqi_info_test_gen(ul_config_pdus[9].uci_cqi_sr_pdu.cqi_information);
    ul_config_sr_info_test_gen(ul_config_pdus[9].uci_cqi_sr_pdu.sr_information);
    ul_config_pdus[10].pdu_type = NFAPI_UL_CONFIG_UCI_CQI_SR_HARQ_PDU_TYPE;
    ul_config_ue_info_test_gen(ul_config_pdus[10].uci_cqi_sr_harq_pdu.ue_information);
    ul_config_cqi_info_test_gen(ul_config_pdus[10].uci_cqi_sr_harq_pdu.cqi_information);
    ul_config_sr_info_test_gen(ul_config_pdus[10].uci_cqi_sr_harq_pdu.sr_information);
    ul_config_harq_info_test_gen(ul_config_pdus[10].uci_cqi_sr_harq_pdu.harq_information);
    ul_config_pdus[11].pdu_type = NFAPI_UL_CONFIG_SRS_PDU_TYPE;
    ul_config_pdus[11].srs_pdu.srs_pdu_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL8_TAG;
    ul_config_pdus[11].srs_pdu.srs_pdu_rel8.handle = rand_range(0, 9999);
    ul_config_pdus[11].srs_pdu.srs_pdu_rel8.size = rand_range(1, 999);
    ul_config_pdus[11].srs_pdu.srs_pdu_rel8.rnti = rand_range(1, 65535);
    ul_config_pdus[11].srs_pdu.srs_pdu_rel8.srs_bandwidth = rand_range(0, 3);
    ul_config_pdus[11].srs_pdu.srs_pdu_rel8.frequency_domain_position = rand_range(0, 23);
    ul_config_pdus[11].srs_pdu.srs_pdu_rel8.srs_hopping_bandwidth = rand_range(0, 3);
    ul_config_pdus[11].srs_pdu.srs_pdu_rel8.transmission_comb = rand_range(0, 3);
    ul_config_pdus[11].srs_pdu.srs_pdu_rel8.i_srs = rand_range(0, 1023);
    ul_config_pdus[11].srs_pdu.srs_pdu_rel8.sounding_reference_cyclic_shift = rand_range(0, 11);
    ul_config_pdus[11].srs_pdu.srs_pdu_rel10.tl.tag = NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL10_TAG;
    ul_config_pdus[11].srs_pdu.srs_pdu_rel10.antenna_port = rand_range(0, 2);
    ul_config_pdus[11].srs_pdu.srs_pdu_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL13_TAG;
    ul_config_pdus[11].srs_pdu.srs_pdu_rel13.number_of_combs = rand_range(0, 1);
    ul_config_pdus[12].pdu_type = NFAPI_UL_CONFIG_HARQ_BUFFER_PDU_TYPE;
    ul_config_ue_info_test_gen(ul_config_pdus[12].harq_buffer_pdu.ue_information);
    ul_config_pdus[13].pdu_type = NFAPI_UL_CONFIG_ULSCH_UCI_CSI_PDU_TYPE;
    ul_config_ulsch_pdu_test_gen(ul_config_pdus[13].ulsch_uci_csi_pdu.ulsch_pdu);
    ul_config_cqi_info_test_gen(ul_config_pdus[13].ulsch_uci_csi_pdu.csi_information);
    ul_config_pdus[14].pdu_type = NFAPI_UL_CONFIG_ULSCH_UCI_HARQ_PDU_TYPE;
    ul_config_ulsch_pdu_test_gen(ul_config_pdus[14].ulsch_uci_harq_pdu.ulsch_pdu);
    ul_config_harq_info_test_gen(ul_config_pdus[14].ulsch_uci_harq_pdu.harq_information);
    ul_config_pdus[15].pdu_type = NFAPI_UL_CONFIG_ULSCH_CSI_UCI_HARQ_PDU_TYPE;
    ul_config_ulsch_pdu_test_gen(ul_config_pdus[15].ulsch_csi_uci_harq_pdu.ulsch_pdu);
    ul_config_cqi_info_test_gen(ul_config_pdus[15].ulsch_csi_uci_harq_pdu.csi_information);
    ul_config_harq_info_test_gen(ul_config_pdus[15].ulsch_csi_uci_harq_pdu.harq_information);
    ul_config_pdus[16].pdu_type = NFAPI_UL_CONFIG_NULSCH_PDU_TYPE;
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_NULSCH_PDU_REL13_TAG;
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.nulsch_format = rand_range(0, 1);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.handle = rand_range(0, 0xFFFF);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.size = rand_range(0, 65535);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.rnti = rand_range(1, 65535);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.subcarrier_indication = rand_range(0, 47);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.resource_assignment = rand_range(0, 7);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.mcs = rand_range(0, 12);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.redudancy_version = rand_range(0, 1);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.repetition_number = rand_range(0, 7);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.new_data_indication = rand_range(0, 1);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.n_srs = rand_range(0, 1);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.scrambling_sequence_initialization_cinit = rand_range(0, 65535);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.sf_idx = rand_range(0, 40960);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG;
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel8.handle = rand_range(0, 0xFF);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel8.rnti = rand_range(1, 65535);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel11.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL11_TAG;
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel11.virtual_cell_id_enabled_flag = rand_range(0, 1);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel11.npusch_identity = rand_range(0, 503);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL13_TAG;
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.ue_type = rand_range(0, 2);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.empty_symbols = rand_range(0, 0x3);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.total_number_of_repetitions = rand_range(1, 32);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.ue_information.ue_information_rel13.repetition_number = rand_range(1, 32);
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.nb_harq_information.nb_harq_information_rel13_fdd.tl.tag = NFAPI_UL_CONFIG_REQUEST_NB_HARQ_INFORMATION_REL13_FDD_TAG;
    ul_config_pdus[16].nulsch_pdu.nulsch_pdu_rel13.nb_harq_information.nb_harq_information_rel13_fdd.harq_ack_resource = rand_range(0, 15);
    ul_config_pdus[17].pdu_type = NFAPI_UL_CONFIG_NRACH_PDU_TYPE;
    ul_config_pdus[17].nrach_pdu.nrach_pdu_rel13.tl.tag = NFAPI_UL_CONFIG_REQUEST_NRACH_PDU_REL13_TAG;
    ul_config_pdus[17].nrach_pdu.nrach_pdu_rel13.nprach_config_0 = rand_range(0, 1);
    ul_config_pdus[17].nrach_pdu.nrach_pdu_rel13.nprach_config_1 = rand_range(0, 1);
    ul_config_pdus[17].nrach_pdu.nrach_pdu_rel13.nprach_config_2 = rand_range(0, 1);
    ul_config_req.ul_config_request_body.ul_config_pdu_list = ul_config_pdus;
    mac->ul_config_req(mac, &ul_config_req);
    uint8_t num_dci_pdus = 4;
    uint8_t num_hi_pdus = 1;
    nfapi_hi_dci0_request_pdu_t hi_dci0_pdus[num_dci_pdus + num_hi_pdus];
    memset(&hi_dci0_pdus, 0, sizeof(hi_dci0_pdus));
    nfapi_hi_dci0_request_t hi_dci0_req;
    memset(&hi_dci0_req, 0, sizeof(hi_dci0_req));
    hi_dci0_req.header.message_id = NFAPI_HI_DCI0_REQUEST;
    hi_dci0_req.header.phy_id = phy_id;
    hi_dci0_req.sfn_sf = sfn_sf;
    hi_dci0_req.hi_dci0_request_body.tl.tag = NFAPI_HI_DCI0_REQUEST_BODY_TAG;
    hi_dci0_req.hi_dci0_request_body.sfnsf = sfn_sf;
    hi_dci0_req.hi_dci0_request_body.number_of_dci = num_dci_pdus;
    hi_dci0_req.hi_dci0_request_body.number_of_hi = num_hi_pdus;
    hi_dci0_pdus[0].pdu_type = NFAPI_HI_DCI0_HI_PDU_TYPE;
    hi_dci0_pdus[0].hi_pdu.hi_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_HI_PDU_REL8_TAG;
    hi_dci0_pdus[0].hi_pdu.hi_pdu_rel8.resource_block_start = rand_range(0, 100);
    hi_dci0_pdus[0].hi_pdu.hi_pdu_rel8.cyclic_shift_2_for_drms = rand_range(0, 7);
    hi_dci0_pdus[0].hi_pdu.hi_pdu_rel8.hi_value = rand_range(0, 1);
    hi_dci0_pdus[0].hi_pdu.hi_pdu_rel8.i_phich = rand_range(0, 1);
    hi_dci0_pdus[0].hi_pdu.hi_pdu_rel8.transmission_power = rand_range(0, 10000);
    hi_dci0_pdus[0].hi_pdu.hi_pdu_rel10.tl.tag = NFAPI_HI_DCI0_REQUEST_HI_PDU_REL10_TAG;
    hi_dci0_pdus[0].hi_pdu.hi_pdu_rel10.flag_tb2 = rand_range(0, 1);
    hi_dci0_pdus[0].hi_pdu.hi_pdu_rel10.hi_value_2 = rand_range(0, 1);
    hi_dci0_pdus[1].pdu_type = NFAPI_HI_DCI0_DCI_PDU_TYPE;
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG;
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.dci_format = rand_range(0, 4);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.cce_index = rand_range(0, 88);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.aggregation_level = rand_range(1, 8);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.rnti = rand_range(1, 65535);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.resource_block_start = rand_range(0, 100);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.number_of_resource_block = rand_range(0, 100);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.mcs_1 = rand_range(0, 31);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.cyclic_shift_2_for_drms = rand_range(0, 7);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.frequency_hopping_enabled_flag = rand_range(0, 1);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.frequency_hopping_bits = rand_range(0, 3);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.new_data_indication_1 = rand_range(0, 1);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.ue_tx_antenna_seleciton = rand_range(0, 2);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.tpc = rand_range(0, 3);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.cqi_csi_request = rand_range(0, 7);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.ul_index = rand_range(0, 3);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.dl_assignment_index = rand_range(1, 4);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.tpc_bitmap = rand_range(0, 9999);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel8.transmission_power = rand_range(0, 10000);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.tl.tag = NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL10_TAG;
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.cross_carrier_scheduling_flag = rand_range(0, 1);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.carrier_indicator = rand_range(0, 7);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.size_of_cqi_csi_feild = rand_range(0, 2);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.srs_flag = rand_range(0, 1);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.srs_request = rand_range(0, 1);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.resource_allocation_flag = rand_range(0, 1);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.resource_allocation_type = rand_range(0, 1);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.resource_block_coding = rand_range(0, 9999);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.mcs_2 = rand_range(0, 31);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.new_data_indication_2 = rand_range(0, 1);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.number_of_antenna_ports = rand_range(0, 2);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.tpmi = rand_range(0, 63);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.total_dci_length_including_padding = rand_range(0, 255);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel10.n_ul_rb = rand_range(6, 100);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel12.tl.tag = NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL12_TAG;
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel12.pscch_resource = rand_range(0, 16);
    hi_dci0_pdus[1].dci_pdu.dci_pdu_rel12.time_resource_pattern = rand_range(0, 32);
    hi_dci0_pdus[2].pdu_type = NFAPI_HI_DCI0_EPDCCH_DCI_PDU_TYPE;
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.tl.tag = NFAPI_HI_DCI0_REQUEST_EPDCCH_DCI_PDU_REL8_TAG;
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.dci_format = rand_range(0, 4);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.cce_index = rand_range(0, 88);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.aggregation_level = rand_range(1, 8);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.rnti = rand_range(1, 65535);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.resource_block_start = rand_range(0, 100);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.number_of_resource_block = rand_range(0, 100);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.mcs_1 = rand_range(0, 31);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.cyclic_shift_2_for_drms = rand_range(0, 7);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.frequency_hopping_enabled_flag = rand_range(0, 1);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.frequency_hopping_bits = rand_range(0, 3);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.new_data_indication_1 = rand_range(0, 1);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.ue_tx_antenna_seleciton = rand_range(0, 2);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.tpc = rand_range(0, 3);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.cqi_csi_request = rand_range(0, 7);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.ul_index = rand_range(0, 3);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.dl_assignment_index = rand_range(1, 4);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.tpc_bitmap = rand_range(0, 9999);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel8.transmission_power = rand_range(0, 10000);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.tl.tag = NFAPI_HI_DCI0_REQUEST_EPDCCH_DCI_PDU_REL10_TAG;
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.cross_carrier_scheduling_flag = rand_range(0, 1);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.carrier_indicator = rand_range(0, 7);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.size_of_cqi_csi_feild = rand_range(0, 2);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.srs_flag = rand_range(0, 1);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.srs_request = rand_range(0, 1);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.resource_allocation_flag = rand_range(0, 1);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.resource_allocation_type = rand_range(0, 1);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.resource_block_coding = rand_range(0, 9999);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.mcs_2 = rand_range(0, 31);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.new_data_indication_2 = rand_range(0, 1);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.number_of_antenna_ports = rand_range(0, 2);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.tpmi = rand_range(0, 63);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.total_dci_length_including_padding = rand_range(0, 255);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_dci_pdu_rel10.n_ul_rb = rand_range(6, 100);
    hi_dci0_pdus[2].epdcch_dci_pdu.epdcch_parameters_rel11.tl.tag = NFAPI_HI_DCI0_REQUEST_EPDCCH_PARAMETERS_REL11_TAG;
    hi_dci0_pdus[3].pdu_type = NFAPI_HI_DCI0_MPDCCH_DCI_PDU_TYPE;
    hi_dci0_pdus[3].mpdcch_dci_pdu.mpdcch_dci_pdu_rel13.tl.tag = NFAPI_HI_DCI0_REQUEST_MPDCCH_DCI_PDU_REL13_TAG;
    hi_dci0_pdus[4].pdu_type = NFAPI_HI_DCI0_NPDCCH_DCI_PDU_TYPE;
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.tl.tag = NFAPI_HI_DCI0_REQUEST_NPDCCH_DCI_PDU_REL13_TAG;
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.ncce_index = rand_range(0, 1);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.aggregation_level = rand_range(1, 2);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.start_symbol = rand_range(0, 4);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.rnti = rand_range(1, 65535);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.scrambling_reinitialization_batch_index = rand_range(1, 4);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.nrs_antenna_ports_assumed_by_the_ue = rand_range(1, 2);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.subcarrier_indication = rand_range(0, 63);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.resource_assignment = rand_range(0, 7);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.scheduling_delay = rand_range(0, 3);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.mcs = rand_range(0, 12);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.redudancy_version = rand_range(0, 1);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.repetition_number = rand_range(0, 7);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.new_data_indicator = rand_range(0, 1);
    hi_dci0_pdus[4].npdcch_dci_pdu.npdcch_dci_pdu_rel13.dci_subframe_repetition_number = rand_range(0, 3);
    hi_dci0_req.hi_dci0_request_body.hi_dci0_pdu_list = hi_dci0_pdus;
    mac->hi_dci0_req(mac, &hi_dci0_req);
    uint8_t num_tx_pdus = 2;
    nfapi_tx_request_pdu_t tx_pdus[num_tx_pdus];
    memset(&tx_pdus, 0, sizeof(tx_pdus));
    nfapi_tx_request_t tx_req;
    memset(&tx_req, 0, sizeof(tx_req));
    tx_req.header.message_id = NFAPI_TX_REQUEST;
    tx_req.header.phy_id = phy_id;
    tx_req.sfn_sf = sfn_sf;
    tx_req.tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
    tx_req.tx_request_body.number_of_pdus = num_tx_pdus;
    uint32_t data[2];
    data[0] = 0x11223344;
    data[1] = 0x55667788;
    tx_pdus[0].pdu_length = 8;
    tx_pdus[0].pdu_index = 1;
    tx_pdus[0].num_segments = 2;
    tx_pdus[0].segments[0].segment_length = 4;
    tx_pdus[0].segments[0].segment_data = (uint8_t *)&data[0];
    tx_pdus[0].segments[1].segment_length = 4;
    tx_pdus[0].segments[1].segment_data = (uint8_t *)&data[1];
    tx_pdus[1].pdu_length = 8;
    tx_pdus[1].pdu_index = 2;
    tx_pdus[1].num_segments = 2;
    tx_pdus[1].segments[0].segment_length = 4;
    tx_pdus[1].segments[0].segment_data = (uint8_t *)&data[0];
    tx_pdus[1].segments[1].segment_length = 4;
    tx_pdus[1].segments[1].segment_data = (uint8_t *)&data[1];
    tx_req.tx_request_body.tx_pdu_list = tx_pdus;
    mac->tx_req(mac, &tx_req);
  }

  void generate_subframe(mac_t *mac, uint16_t phy_id, uint16_t sfn_sf) {
    mac_internal_t *instance = (mac_internal_t *)mac;
    nfapi_dl_config_request_t dl_config_req;
    memset(&dl_config_req, 0, sizeof(dl_config_req));
    dl_config_req.header.message_id = NFAPI_DL_CONFIG_REQUEST;
    dl_config_req.header.phy_id = phy_id;
    dl_config_req.sfn_sf = sfn_sf;
    dl_config_req.dl_config_request_body.tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
    dl_config_req.dl_config_request_body.number_pdu = 8;
    nfapi_dl_config_request_pdu_t pdus[8];
    memset(&pdus, 0, sizeof(pdus));

    for(int i = 0; i < 8; i++) {
      pdus[i].pdu_type = NFAPI_DL_CONFIG_BCH_PDU_TYPE;
      pdus[i].bch_pdu.bch_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_BCH_PDU_REL8_TAG;
      pdus[i].bch_pdu.bch_pdu_rel8.length = 42;
      pdus[i].bch_pdu.bch_pdu_rel8.pdu_index = i;
      pdus[i].bch_pdu.bch_pdu_rel8.transmission_power = 56;
    }

    dl_config_req.dl_config_request_body.dl_config_pdu_list = pdus;
    /*
    vendor_ext_tlv_1 ve;
    ve.tl.tag = VENDOR_EXT_TLV_1_TAG;
    ve.dummy = 999;
    dl_config_req.vendor_extension = &ve.tl;
    */
    mac->dl_config_req(mac, &dl_config_req);
    nfapi_ul_config_request_t ul_config_req;
    memset(&ul_config_req, 0, sizeof(ul_config_req));
    ul_config_req.header.message_id = NFAPI_UL_CONFIG_REQUEST;
    ul_config_req.header.phy_id = phy_id;
    ul_config_req.sfn_sf = sfn_sf;
    mac->ul_config_req(mac, &ul_config_req);
    nfapi_hi_dci0_request_t hi_dci0_req;
    memset(&hi_dci0_req, 0, sizeof(hi_dci0_req));
    hi_dci0_req.header.message_id = NFAPI_HI_DCI0_REQUEST;
    hi_dci0_req.header.phy_id = phy_id;
    hi_dci0_req.sfn_sf = sfn_sf;
    mac->hi_dci0_req(mac, &hi_dci0_req);
    nfapi_tx_request_t tx_req;
    memset(&tx_req, 0, sizeof(tx_req));
    tx_req.header.message_id = NFAPI_TX_REQUEST;
    tx_req.header.phy_id = phy_id;
    tx_req.sfn_sf = sfn_sf;
    nfapi_tx_request_pdu_t tx_pdus[8];
    tx_req.tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
    tx_req.tx_request_body.number_of_pdus = 0;
    tx_req.tx_request_body.tx_pdu_list = &tx_pdus[0];
    mac_pdu *buff = 0;
    int i = 0;
    std::list<mac_pdu *> free_list;

    do {
      buff = instance->mac->pop_rx_buffer();

      if(buff != 0) {
        if(buff->len == 0) {
          printf("[MAC] Buffer length = 0\n");
        }

        tx_req.tx_request_body.tx_pdu_list[i].pdu_length = buff->len;
        tx_req.tx_request_body.tx_pdu_list[i].pdu_index = i;
        tx_req.tx_request_body.tx_pdu_list[i].num_segments = 1;
        tx_req.tx_request_body.tx_pdu_list[i].segments[0].segment_length = buff->len;
        tx_req.tx_request_body.tx_pdu_list[i].segments[0].segment_data = (uint8_t *)buff->buffer;
        tx_req.tx_request_body.number_of_pdus++;
        i++;
        instance->mac->byte_count += buff->len;
        free_list.push_back(buff);
      }
    } while(buff != 0 && i < 8);

    mac->tx_req(mac, &tx_req);

    //for(int j = 0; j < tx_req.tx_request_body.number_of_pdus; ++j)
    for(mac_pdu *pdu : free_list) {
      instance->mac->release_mac_pdu(pdu);
      //free(tx_req.tx_request_body.tx_pdu_list[j].segments[0].segment_data);
    }
  }

  void mac_subframe_ind(mac_t *mac, uint16_t phy_id, uint16_t sfn_sf) {
    //printf("[MAC] subframe indication phyid:%d sfnsf:%d\n", phy_id, sfn_sf);
    mac_internal_t *instance = (mac_internal_t *)mac;

    if(instance->mac->tick == 1000) {
      if(instance->mac->byte_count > 0) {
        printf("[MAC] Rx rate %d bytes/sec\n", instance->mac->byte_count);
        instance->mac->byte_count = 0;
      }

      instance->mac->tick = 0;
    }

    instance->mac->tick++;

    if(instance->mac->wireshark_test_mode) {
      generate_test_subframe(mac, phy_id, sfn_sf);
    } else {
      generate_subframe(mac, phy_id, sfn_sf);
    }
  }

  void mac_harq_ind(mac_t *mac, nfapi_harq_indication_t *ind) {
  }
  void mac_crc_ind(mac_t *mac, nfapi_crc_indication_t *ind) {
  }
  void mac_rx_ind(mac_t *mac, nfapi_rx_indication_t *ind) {
    mac_internal_t *instance = (mac_internal_t *)mac;

    for(int i = 0; i < ind->rx_indication_body.number_of_pdus; ++i) {
      uint16_t len = ind->rx_indication_body.rx_pdu_list[i].rx_indication_rel8.length;
      uint8_t *data = ind->rx_indication_body.rx_pdu_list[i].data;
      //printf("[MAC] sfnsf:%d len:%d\n", ind->sfn_sf,len);
      //
      instance->tx_byte_count += len;
      int sendto_result = sendto(instance->tx_sock, data, len, 0, (struct sockaddr *)&(instance->tx_addr), sizeof(instance->tx_addr));

      if(sendto_result < 0) {
        // error
      }
    }
  }
  void mac_rach_ind(mac_t *mac, nfapi_rach_indication_t *ind) {
  }
  void mac_srs_ind(mac_t *mac, nfapi_srs_indication_t *ind) {
  }
  void mac_sr_ind(mac_t *mac, nfapi_sr_indication_t *ind) {
  }
  void mac_cqi_ind(mac_t *mac, nfapi_cqi_indication_t *ind) {
  }
  void mac_lbt_dl_ind(mac_t *mac, nfapi_lbt_dl_indication_t *ind) {
  }
  void mac_nb_harq_ind(mac_t *mac, nfapi_nb_harq_indication_t *ind) {
  }
  void mac_nrach_ind(mac_t *mac, nfapi_nrach_indication_t *ind) {
  }
}

