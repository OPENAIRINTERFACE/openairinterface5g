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

/* \file NR_IF_Module.c
 * \brief functions for NR UE FAPI-like interface
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "PHY/defs_nr_UE.h"
#include "NR_IF_Module.h"
#include "NR_MAC_UE/mac_proto.h"
#include "assertions.h"
#include "NR_MAC_UE/mac_extern.h"
#include "SCHED_NR_UE/fapi_nr_ue_l1.h"
#include "executables/softmodem-common.h"
#include "openair2/RRC/NR_UE/rrc_proto.h"
#include "openair2/GNB_APP/L1_nr_paramdef.h"
#include "openair2/GNB_APP/gnb_paramdef.h"
#include "radio/ETHERNET/if_defs.h"
#include <stdio.h>
#include "openair2/GNB_APP/MACRLC_nr_paramdef.h"

#define MAX_IF_MODULES 100

UL_IND_t *UL_INFO = NULL;

static eth_params_t         stub_eth_params;
static nr_ue_if_module_t *nr_ue_if_module_inst[MAX_IF_MODULES];
static int ue_tx_sock_descriptor = -1;
static int ue_rx_sock_descriptor = -1;
static int g_harq_pid;
sem_t sfn_slot_semaphore;

queue_t nr_sfn_slot_queue;
queue_t nr_chan_param_queue;
queue_t nr_dl_tti_req_queue;
queue_t nr_tx_req_queue;
queue_t nr_ul_dci_req_queue;
queue_t nr_ul_tti_req_queue;
pthread_mutex_t mac_IF_mutex;
static void save_pdsch_pdu_for_crnti(nfapi_nr_dl_tti_request_t *dl_tti_request);

void nrue_init_standalone_socket(int tx_port, int rx_port)
{
  {
    struct sockaddr_in server_address;
    int addr_len = sizeof(server_address);
    memset(&server_address, 0, addr_len);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(tx_port);

    int sd = socket(server_address.sin_family, SOCK_DGRAM, 0);
    if (sd < 0)
    {
      LOG_E(NR_MAC, "Socket creation error standalone PNF\n");
      return;
    }

    if (inet_pton(server_address.sin_family, stub_eth_params.remote_addr, &server_address.sin_addr) <= 0)
    {
      LOG_E(NR_MAC, "Invalid standalone PNF Address\n");
      close(sd);
      return;
    }

    // Using connect to use send() instead of sendto()
    if (connect(sd, (struct sockaddr *)&server_address, addr_len) < 0)
    {
      LOG_E(NR_MAC, "Connection to standalone PNF failed: %s\n", strerror(errno));
      close(sd);
      return;
    }
    assert(ue_tx_sock_descriptor == -1);
    ue_tx_sock_descriptor = sd;
    LOG_T(NR_RRC, "Successfully set up tx_socket in %s.\n", __FUNCTION__);
  }

  {
    struct sockaddr_in server_address;
    int addr_len = sizeof(server_address);
    memset(&server_address, 0, addr_len);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(rx_port);

    int sd = socket(server_address.sin_family, SOCK_DGRAM, 0);
    if (sd < 0)
    {
      LOG_E(MAC, "Socket creation error standalone PNF\n");
      return;
    }

    if (bind(sd, (struct sockaddr *)&server_address, addr_len) < 0)
    {
      LOG_E(NR_MAC, "Connection to standalone PNF failed: %s\n", strerror(errno));
      close(sd);
      return;
    }
    assert(ue_rx_sock_descriptor == -1);
    ue_rx_sock_descriptor = sd;
    LOG_T(NR_RRC, "Successfully set up rx_socket in %s.\n", __FUNCTION__);
  }
  LOG_D(NR_RRC, "NRUE standalone socket info: tx_port %d  rx_port %d on %s.\n",
        tx_port, rx_port, stub_eth_params.remote_addr);
}

void send_nsa_standalone_msg(NR_UL_IND_t *UL_INFO, uint16_t msg_id)
{
  switch(msg_id)
  {
    case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION:
    {
        char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
        LOG_T(NR_MAC, "RACH header id :%d\n", UL_INFO->rach_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->rach_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_D(NR_MAC, "NR_RACH_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
                UL_INFO->rach_ind.sfn, UL_INFO->rach_ind.slot, UL_INFO->rach_ind.number_of_pdus);
        if (send(ue_tx_sock_descriptor, buffer, encoded_size, 0) < 0)
        {
                LOG_E(NR_MAC, "Send Proxy NR_UE failed\n");
                return;
        }
        break;
    }
    case NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION:
    {
        char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
        LOG_T(NR_MAC, "RX header id :%d\n", UL_INFO->rx_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->rx_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_D(NR_MAC, "NR_RX_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
                UL_INFO->rx_ind.sfn, UL_INFO->rx_ind.slot, UL_INFO->rx_ind.number_of_pdus);
        if (send(ue_tx_sock_descriptor, buffer, encoded_size, 0) < 0)
        {
                LOG_E(NR_MAC, "Send Proxy NR_UE failed\n");
                return;
        }
        break;
    }
    case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION:
    {
        char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
        LOG_T(NR_MAC, "CRC header id :%d\n", UL_INFO->crc_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->crc_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_D(NR_MAC, "NR_CRC_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
                UL_INFO->crc_ind.sfn, UL_INFO->crc_ind.slot, UL_INFO->crc_ind.number_crcs);
        if (send(ue_tx_sock_descriptor, buffer, encoded_size, 0) < 0)
        {
                LOG_E(NR_MAC, "Send Proxy NR_UE failed\n");
                return;
        }
        break;
    }
    case NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION:
    {
        char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
        LOG_D(NR_MAC, "UCI header id :%d\n", UL_INFO->uci_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->uci_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_D(NR_MAC, "NR_UCI_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
                UL_INFO->uci_ind.sfn, UL_INFO->uci_ind.slot, UL_INFO->uci_ind.num_ucis);
        if (send(ue_tx_sock_descriptor, buffer, encoded_size, 0) < 0)
        {
                LOG_E(NR_MAC, "Send Proxy NR_UE failed\n");
                return;
        }
        break;
    }
    case NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION:
    break;
    default:
    break;
  }
}

bool sfn_slot_matcher(void *wanted, void *candidate)
{
  nfapi_p7_message_header_t *msg = candidate;
  int sfn_sf = *(int*)wanted;

  switch (msg->message_id)
  {
    case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION:
    {
      nfapi_nr_rach_indication_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->sfn && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION:
    {
      nfapi_nr_rx_data_indication_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->sfn && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION:
    {
      nfapi_nr_crc_indication_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->sfn && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION:
    {
      nfapi_nr_uci_indication_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->sfn && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
    {
      nfapi_nr_dl_tti_request_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->SFN && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->Slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
    {
      nfapi_nr_tx_data_request_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->SFN && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->Slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
    {
      nfapi_nr_ul_dci_request_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->SFN && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->Slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
    {
      nfapi_nr_ul_tti_request_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->SFN && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->Slot;
    }

    default:
      LOG_E(NR_MAC, "sfn_slot_match bad ID: %d\n", msg->message_id);

  }

  return false;
}

static void fill_dl_info_with_pdcch(fapi_nr_dci_indication_t *dci, nfapi_nr_dl_dci_pdu_t *rx_dci, int idx)
{
    int num_bytes = (rx_dci->PayloadSizeBits + 7) / 8;
    LOG_D(NR_PHY, "[%d, %d] PDCCH DCI (Payload) for rnti %x with PayloadSizeBits %d, num_bytes %d\n",
          dci->SFN, dci->slot, rx_dci->RNTI, rx_dci->PayloadSizeBits, num_bytes);
    for (int k = 0; k < num_bytes; k++)
    {
        LOG_D(NR_MAC, "PDCCH DCI PDU payload[%d] = %d\n", k, rx_dci->Payload[k]);
        dci->dci_list[idx].payloadBits[k] = rx_dci->Payload[k];
    }
    dci->dci_list[idx].payloadSize = rx_dci->PayloadSizeBits;
    dci->dci_list[idx].rnti = rx_dci->RNTI;
    dci->dci_list[idx].n_CCE = rx_dci->CceIndex;
    dci->dci_list[idx].N_CCE = rx_dci->AggregationLevel;
    dci->number_of_dcis = idx + 1;
}

static void fill_mib_in_rx_ind(nfapi_nr_dl_tti_request_pdu_t *pdu_list, fapi_nr_rx_indication_t *rx_ind, int pdu_idx, int pdu_type)
{
  AssertFatal(pdu_idx < sizeof(rx_ind->rx_indication_body) / sizeof(rx_ind->rx_indication_body[0]),
              "pdu_index (%d) is greater than rx_indication_body size!\n", pdu_idx);
  AssertFatal(pdu_idx == rx_ind->number_pdus,  "Invalid pdu_idx %d!\n", pdu_idx);

  LOG_T(NR_MAC, "Recevied an SSB and are filling rx_ind with the MIB!\n");

  nfapi_nr_dl_tti_ssb_pdu_rel15_t *ssb_pdu = &pdu_list->ssb_pdu.ssb_pdu_rel15;
  rx_ind->rx_indication_body[pdu_idx].ssb_pdu.cell_id = ssb_pdu->PhysCellId;
  rx_ind->rx_indication_body[pdu_idx].ssb_pdu.pdu[0] = (ssb_pdu->bchPayload) & 0xff;
  rx_ind->rx_indication_body[pdu_idx].ssb_pdu.pdu[1] = (ssb_pdu->bchPayload >> 8) & 0xff;
  rx_ind->rx_indication_body[pdu_idx].ssb_pdu.pdu[2] = (ssb_pdu->bchPayload >> 16) & 0xff;
  rx_ind->rx_indication_body[pdu_idx].ssb_pdu.rsrp_dBm = ssb_pdu->ssbRsrp;
  rx_ind->rx_indication_body[pdu_idx].ssb_pdu.ssb_index = ssb_pdu->SsbBlockIndex;
  rx_ind->rx_indication_body[pdu_idx].ssb_pdu.ssb_length = pdu_list->PDUSize;
  rx_ind->rx_indication_body[pdu_idx].ssb_pdu.ssb_start_subcarrier = ssb_pdu->SsbSubcarrierOffset;
  rx_ind->rx_indication_body[pdu_idx].ssb_pdu.decoded_pdu = true;
  rx_ind->rx_indication_body[pdu_idx].pdu_type = pdu_type;
  rx_ind->number_pdus = pdu_idx + 1;
}

static bool is_my_dci(NR_UE_MAC_INST_t *mac, nfapi_nr_dl_dci_pdu_t *received_pdu)
{
  /* For multiple UEs, we need to be able to filter the rx'd messages by
     the RNTI. The filtering is different between NSA mode and SA mode.
     NSA mode has a two step CFRA procedure and SA has a 4 step procedure.
     We only need to check if the rx'd RNTI doesnt match the CRNTI if the RAR
     has been processed already, in NSA mode.
     In SA, depending on the RA state, we can have a SIB (0xffff), RAR (0x10b),
     Msg3 (TC_RNTI) or an actual DCI message (CRNTI). When we get Msg3, the
     MAC instance of the UE still has a CRNTI = 0. We should only check if the
     CRNTI doesnt match the received RNTI in SA mode if Msg3 has been processed
     already. Only once the RA procedure succeeds is the CRNTI value updated
     to the TC_RNTI. */
  if (get_softmodem_params()->nsa) {
    if (received_pdu->RNTI != mac->crnti &&
        (received_pdu->RNTI != mac->ra.ra_rnti || mac->ra.RA_RAPID_found))
      return false;
  }
  if (get_softmodem_params()->sa) {
    if (mac->state == UE_NOT_SYNC)
      return false;
    if (received_pdu->RNTI == 0xFFFF && mac->phy_config_request_sent)
      return false;
    if (received_pdu->RNTI != mac->crnti && mac->ra.ra_state == RA_SUCCEEDED)
      return false;
    if (received_pdu->RNTI != mac->ra.t_crnti && mac->ra.ra_state == WAIT_CONTENTION_RESOLUTION)
      return false;
    if (received_pdu->RNTI != 0x10b && mac->ra.ra_state == WAIT_RAR)
      return false;
    if (received_pdu->RNTI != 0xFFFF && mac->ra.ra_state <= GENERATE_PREAMBLE)
      return false;
  }
  return true;
}

static void copy_dl_tti_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_dl_tti_request_t *dl_tti_request)
{
    NR_UE_MAC_INST_t *mac = get_mac_inst(dl_info->module_id);
    mac->nr_ue_emul_l1.expected_sib = false;
    memset(mac->nr_ue_emul_l1.index_has_sib, 0, sizeof(mac->nr_ue_emul_l1.index_has_sib));
    mac->nr_ue_emul_l1.expected_rar = false;
    memset(mac->nr_ue_emul_l1.index_has_rar, 0, sizeof(mac->nr_ue_emul_l1.index_has_rar));
    mac->nr_ue_emul_l1.expected_dci = false;
    memset(mac->nr_ue_emul_l1.index_has_dci, 0, sizeof(mac->nr_ue_emul_l1.index_has_dci));
    int pdu_idx = 0;

    int num_pdus = dl_tti_request->dl_tti_request_body.nPDUs;
    AssertFatal(num_pdus >= 0, "Invalid dl_tti_request number of PDUS\n");

    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_dl_tti_request_pdu_t *pdu_list = &dl_tti_request->dl_tti_request_body.dl_tti_pdu_list[i];
        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE)
        {
            LOG_D(NR_PHY, "[%d, %d] PDSCH PDU for rnti %x\n",
                dl_tti_request->SFN, dl_tti_request->Slot, pdu_list->pdsch_pdu.pdsch_pdu_rel15.rnti);
        }

        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE)
        {
            LOG_D(NR_PHY, "[%d, %d] PDCCH DCI PDU (Format for incoming PDSCH PDU)\n",
                dl_tti_request->SFN, dl_tti_request->Slot);
            uint16_t num_dcis = pdu_list->pdcch_pdu.pdcch_pdu_rel15.numDlDci;
            if (num_dcis > 0)
            {
                if (!dl_info->dci_ind)
                {
                    dl_info->dci_ind = CALLOC(1, sizeof(fapi_nr_dci_indication_t));
                }
                dl_info->dci_ind->SFN = dl_tti_request->SFN;
                dl_info->dci_ind->slot = dl_tti_request->Slot;
                AssertFatal(num_dcis <= sizeof(dl_info->dci_ind->dci_list) / sizeof(dl_info->dci_ind->dci_list[0]),
                            "The number of DCIs is greater than dci_list");
                for (int j = 0; j < num_dcis; j++)
                {
                    nfapi_nr_dl_dci_pdu_t *dci_pdu_list = &pdu_list->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[j];
                    if (!is_my_dci(mac, dci_pdu_list))
                    {
                        continue;
                    }
                    fill_dl_info_with_pdcch(dl_info->dci_ind, dci_pdu_list, pdu_idx);
                    if (dci_pdu_list->RNTI == 0xffff)
                    {
                        mac->nr_ue_emul_l1.expected_sib = true;
                        mac->nr_ue_emul_l1.index_has_sib[j] = true;
                        LOG_T(NR_MAC, "Setting index_has_sib[%d] = true\n", j);
                    }
                    else if (dci_pdu_list->RNTI == mac->ra.ra_rnti)
                    {
                        mac->nr_ue_emul_l1.expected_rar = true;
                        mac->nr_ue_emul_l1.index_has_rar[j] = true;
                        LOG_T(NR_MAC, "Setting index_has_rar[%d] = true\n", j);
                    }
                    else
                    {
                        mac->nr_ue_emul_l1.expected_dci = true;
                        mac->nr_ue_emul_l1.index_has_dci[j] = true;
                        LOG_T(NR_MAC, "Setting index_has_dci[%d] = true\n", j);
                    }
                    pdu_idx++;
                }
            }
        }
        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_SSB_PDU_TYPE)
        {
            /* If we get a MIB, we want to handle it right away and then come back.
               The MIB and SIB come in the same dl_tti_req but the MIB should be
               processed first and then the DCI and payload of the SIB1 can be
               processed. The MIB should be handled first and then the rx_ind
               will be freed after handling. This is why the PDU index will
               always be zero for the RX_IND becasue we should not have more than
               one MIB. */
            if (!dl_info->rx_ind)
            {
                dl_info->rx_ind = CALLOC(1, sizeof(*dl_info->rx_ind));
            }
            fapi_nr_rx_indication_t *rx_ind = dl_info->rx_ind;
            rx_ind->sfn = dl_tti_request->SFN;
            rx_ind->slot = dl_tti_request->Slot;
            fill_mib_in_rx_ind(pdu_list, rx_ind, 0, FAPI_NR_RX_PDU_TYPE_SSB);
            nr_ue_dl_indication(&mac->dl_info);
        }
    }
    dl_info->slot = dl_tti_request->Slot;
    dl_info->frame = dl_tti_request->SFN;
}

static void fill_rx_ind(nfapi_nr_pdu_t *pdu_list, fapi_nr_rx_indication_t *rx_ind, int pdu_idx, int pdu_type)
{
    NR_UE_MAC_INST_t *mac = get_mac_inst(0);
    AssertFatal(pdu_list->num_TLV < sizeof(pdu_list->TLVs) / sizeof(pdu_list->TLVs[0]), "Num TLVs exceeds TLV array size");
    int length = 0;
    for (int j = 0; j < pdu_list->num_TLV; j++)
    {
        length += pdu_list->TLVs[j].length;
    }
    LOG_D(NR_PHY, "%s: num_tlv %d and length %d, pdu index %d\n",
        __FUNCTION__, pdu_list->num_TLV, length, pdu_idx);
    uint8_t *pdu = malloc(length);
    AssertFatal(pdu != NULL, "%s: Out of memory in malloc", __FUNCTION__);
    rx_ind->rx_indication_body[pdu_idx].pdsch_pdu.pdu = pdu;
    for (int j = 0; j < pdu_list->num_TLV; j++)
    {
        const uint32_t *ptr;
        if (pdu_list->TLVs[j].tag)
            ptr = pdu_list->TLVs[j].value.ptr;
        else
            ptr = pdu_list->TLVs[j].value.direct;
        memcpy(pdu, ptr, pdu_list->TLVs[j].length);
        pdu += pdu_list->TLVs[j].length;
    }
    bool ack_nack = true;
    if (mac->ra.ra_state >= RA_SUCCEEDED && should_drop_transport_block(rx_ind->slot, mac->crnti))
    {
      ack_nack = false;
    }
    rx_ind->rx_indication_body[pdu_idx].pdsch_pdu.ack_nack = ack_nack;
    rx_ind->rx_indication_body[pdu_idx].pdsch_pdu.pdu_length = length;
    rx_ind->rx_indication_body[pdu_idx].pdu_type = pdu_type;

}


static void copy_tx_data_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_tx_data_request_t *tx_data_request)
{
    NR_UE_MAC_INST_t *mac = get_mac_inst(dl_info->module_id);
    int num_pdus = tx_data_request->Number_of_PDUs;
    AssertFatal(num_pdus >= 0, "Invalid tx_data_request number of PDUS\n");

    if (!dl_info->rx_ind)
    {
        dl_info->rx_ind = CALLOC(num_pdus, sizeof(fapi_nr_rx_indication_t));
    }
    AssertFatal(dl_info->rx_ind != NULL, "%s: Out of memory in calloc", __FUNCTION__);
    fapi_nr_rx_indication_t *rx_ind = dl_info->rx_ind;
    rx_ind->sfn = tx_data_request->SFN;
    rx_ind->slot = tx_data_request->Slot;

    int pdu_idx = 0;

    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_pdu_t *pdu_list = &tx_data_request->pdu_list[i];
        if (mac->nr_ue_emul_l1.index_has_sib[i])
        {
            AssertFatal(!get_softmodem_params()->nsa,
                        "Should not be processing SIB in NSA mode, something bad happened\n");
            fill_rx_ind(pdu_list, rx_ind, pdu_idx, FAPI_NR_RX_PDU_TYPE_SIB);
            pdu_idx++;
        }
        else if (mac->nr_ue_emul_l1.index_has_rar[i])
        {
            fill_rx_ind(pdu_list, rx_ind, pdu_idx, FAPI_NR_RX_PDU_TYPE_RAR);
            pdu_idx++;
        }
        else if (mac->nr_ue_emul_l1.index_has_dci[i])
        {
            fill_rx_ind(pdu_list, rx_ind, pdu_idx, FAPI_NR_RX_PDU_TYPE_DLSCH);
            pdu_idx++;
        }
        else
        {
            LOG_T(NR_MAC, "mac->nr_ue_emul_l1.index_has_dci[%d] = 0, so this index contained a DCI for a different UE\n", i);
        }

    }
    dl_info->slot = tx_data_request->Slot;
    dl_info->frame = tx_data_request->SFN;
    dl_info->rx_ind->number_pdus = pdu_idx;
}

static void copy_ul_dci_data_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_ul_dci_request_t *ul_dci_req)
{
    NR_UE_MAC_INST_t *mac = get_mac_inst(dl_info->module_id);
    int pdu_idx = 0;

    int num_pdus = ul_dci_req->numPdus;
    AssertFatal(num_pdus >= 0, "Invalid ul_dci_request number of PDUS\n");

    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_ul_dci_request_pdus_t *pdu_list = &ul_dci_req->ul_dci_pdu_list[i];
        AssertFatal(pdu_list->PDUType == 0, "ul_dci_req pdu type != PUCCH");
        LOG_D(NR_PHY, "[%d %d] PUCCH PDU in ul_dci for rnti %x and numDLDCI = %d\n",
             ul_dci_req->SFN, ul_dci_req->Slot, pdu_list->pdcch_pdu.pdcch_pdu_rel15.dci_pdu->RNTI,
             pdu_list->pdcch_pdu.pdcch_pdu_rel15.numDlDci);
        uint16_t num_dci = pdu_list->pdcch_pdu.pdcch_pdu_rel15.numDlDci;
        if (num_dci > 0)
        {
            if (!dl_info->dci_ind)
            {
                dl_info->dci_ind = CALLOC(1, sizeof(fapi_nr_dci_indication_t));
            }
            AssertFatal(dl_info->dci_ind != NULL, "%s: Out of memory in calloc", __FUNCTION__);
            dl_info->dci_ind->SFN = ul_dci_req->SFN;
            dl_info->dci_ind->slot = ul_dci_req->Slot;
            AssertFatal(num_dci < sizeof(dl_info->dci_ind->dci_list) / sizeof(dl_info->dci_ind->dci_list[0]), "The number of DCIs is greater than dci_list");
            for (int j = 0; j < num_dci; j++)
            {
                nfapi_nr_dl_dci_pdu_t *dci_pdu_list = &pdu_list->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[j];
                if (dci_pdu_list->RNTI != mac->crnti)
                {
                  LOG_T(NR_MAC, "dci_pdu_list->RNTI (%x) != mac->crnti (%x)\n", dci_pdu_list->RNTI, mac->crnti);
                  continue;
                }
                fill_dl_info_with_pdcch(dl_info->dci_ind, dci_pdu_list, pdu_idx);
                pdu_idx++;
            }
        }
    }
    dl_info->frame = ul_dci_req->SFN;
    dl_info->slot = ul_dci_req->Slot;
}

static bool send_crc_ind_and_rx_ind(int sfn_slot)
{
  bool sent_crc_rx = true;

  nfapi_nr_rx_data_indication_t *rx_ind = unqueue_matching(&nr_rx_ind_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &sfn_slot);
  nfapi_nr_crc_indication_t *crc_ind = unqueue_matching(&nr_crc_ind_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &sfn_slot);

  if (crc_ind && crc_ind->number_crcs > 0)
  {
    NR_UE_MAC_INST_t *mac = get_mac_inst(0);
    for (int i = 0; i < crc_ind->number_crcs; i++) {
        int harq_pid = crc_ind->crc_list[i].harq_id;
        LOG_T(NR_MAC, "Resetting harq_pid %d active_ul_harq_sfn_slot\n", harq_pid);
        mac->nr_ue_emul_l1.harq[harq_pid].active_ul_harq_sfn_slot = -1;
    }
    NR_UL_IND_t UL_INFO = {
      .crc_ind = *crc_ind,
    };
    send_nsa_standalone_msg(&UL_INFO, crc_ind->header.message_id);
    free(crc_ind->crc_list);
    free(crc_ind);
  }
  if (rx_ind && rx_ind->number_of_pdus > 0)
  {
    NR_UL_IND_t UL_INFO = {
      .rx_ind = *rx_ind,
    };
    send_nsa_standalone_msg(&UL_INFO, rx_ind->header.message_id);
    free(rx_ind->pdu_list);
    free(rx_ind);
  }
  else
  {
    sent_crc_rx = false;
  }
  return sent_crc_rx;
}

static void copy_ul_tti_data_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_ul_tti_request_t *ul_tti_req)
{
    int num_pdus = ul_tti_req->n_pdus;
    int sfn_slot = NFAPI_SFNSLOT2HEX(ul_tti_req->SFN, ul_tti_req->Slot);
    AssertFatal(num_pdus >= 0, "Invalid ul_tti_request number of PDUS\n");
    AssertFatal(num_pdus <= sizeof(ul_tti_req->pdus_list) / sizeof(ul_tti_req->pdus_list[0]),
                "Too many pdus %d in ul_tti_req\n", num_pdus);

    if (!send_crc_ind_and_rx_ind(sfn_slot))
    {
        LOG_T(NR_MAC, "CRC_RX ind not sent\n");
        if (!put_queue(&nr_ul_tti_req_queue, ul_tti_req))
        {
            LOG_E(NR_PHY, "put_queue failed for ul_tti_req.\n");
        }
        return;
    }
}

static void fill_dci_from_dl_config(nr_downlink_indication_t*dl_ind, fapi_nr_dl_config_request_t *dl_config)
{

  if (!dl_ind->dci_ind)
    return;

  AssertFatal(dl_config->number_pdus < sizeof(dl_config->dl_config_list) / sizeof(dl_config->dl_config_list[0]),
              "Too many dl_config pdus %d", dl_config->number_pdus);
  for (int i = 0; i < dl_config->number_pdus; i++) {
    LOG_D(PHY, "Filling DCI with a total of %d total DL PDUs (dl_config %p) \n",
          dl_config->number_pdus, dl_config);
    fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15_dci = &dl_config->dl_config_list[i].dci_config_pdu.dci_config_rel15;
    int num_dci_options = rel15_dci->num_dci_options;
    if (num_dci_options <= 0)
      LOG_D(NR_MAC, "num_dci_opts = %d for pdu[%d] in dl_config_list\n", rel15_dci->num_dci_options, i);
    AssertFatal(num_dci_options <= sizeof(rel15_dci->dci_length_options) / sizeof(rel15_dci->dci_length_options[0]),
                "num_dci_options %d > dci_length_options array\n", num_dci_options);
    AssertFatal(num_dci_options <= sizeof(rel15_dci->dci_format_options) / sizeof(rel15_dci->dci_format_options[0]),
                "num_dci_options %d > dci_format_options array\n", num_dci_options);

    for (int j = 0; j < num_dci_options; j++) {
      int num_dcis = dl_ind->dci_ind->number_of_dcis;
      AssertFatal(num_dcis <= sizeof(dl_ind->dci_ind->dci_list) / sizeof(dl_ind->dci_ind->dci_list[0]),
                  "dl_config->number_pdus %d > dci_ind->dci_list array\n", num_dcis);
      for (int k = 0; k < num_dcis; k++) {
        LOG_T(NR_PHY, "Received len %d, length options[%d] %d, format assigned %d, format options[%d] %d\n",
              dl_ind->dci_ind->dci_list[k].payloadSize, j, rel15_dci->dci_length_options[j],
              dl_ind->dci_ind->dci_list[k].dci_format, j, rel15_dci->dci_format_options[j]);
        if (rel15_dci->dci_length_options[j] == dl_ind->dci_ind->dci_list[k].payloadSize) {
          dl_ind->dci_ind->dci_list[k].dci_format = rel15_dci->dci_format_options[j];
          dl_ind->dci_ind->dci_list[k].ss_type = rel15_dci->ss_type_options[j];
          dl_ind->dci_ind->dci_list[k].coreset_type = rel15_dci->coreset.CoreSetType;
          LOG_D(NR_PHY, "format assigned dl_ind->dci_ind->dci_list[k].dci_format %d\n",
                dl_ind->dci_ind->dci_list[k].dci_format);
        }
      }
    }
  }
}

// This piece of code is not used in "normal" ue, but in "fapi mode"
void check_and_process_dci(nfapi_nr_dl_tti_request_t *dl_tti_request,
                           nfapi_nr_tx_data_request_t *tx_data_request,
                           nfapi_nr_ul_dci_request_t *ul_dci_request,
                           nfapi_nr_ul_tti_request_t *ul_tti_request)
{
    frame_t frame = 0;
    int slot = 0;
    NR_UE_MAC_INST_t *mac = get_mac_inst(0);

    if (pthread_mutex_lock(&mac->mutex_dl_info)) abort();

    if (dl_tti_request) {
        frame = dl_tti_request->SFN;
        slot = dl_tti_request->Slot;
        LOG_D(NR_PHY, "[%d, %d] dl_tti_request\n", frame, slot);
        copy_dl_tti_req_to_dl_info(&mac->dl_info, dl_tti_request);
    }
    /* This checks if the previously recevied DCI matches our current RNTI
       value. The assumption is that if the DCI matches our RNTI, then the
       incoming tx_data_request is also destined for the current UE. If the
       RAR hasn't been processed yet, we do not want to be filtering the
       tx_data_requests. */
    if (tx_data_request) {
        if (mac->nr_ue_emul_l1.expected_sib ||
            mac->nr_ue_emul_l1.expected_rar ||
            mac->nr_ue_emul_l1.expected_dci) {
            frame = tx_data_request->SFN;
            slot = tx_data_request->Slot;
            LOG_D(NR_PHY, "[%d, %d] PDSCH in tx_request\n", frame, slot);
            copy_tx_data_req_to_dl_info(&mac->dl_info, tx_data_request);
        }
        else {
            LOG_D(NR_MAC, "Unexpected tx_data_req\n");
        }
    }
    else if (ul_dci_request) {
        frame = ul_dci_request->SFN;
        slot = ul_dci_request->Slot;
        LOG_D(NR_PHY, "[%d, %d] ul_dci_request\n", frame, slot);
        copy_ul_dci_data_req_to_dl_info(&mac->dl_info, ul_dci_request);
    }
    else if (ul_tti_request) {
        frame = ul_tti_request->SFN;
        slot = ul_tti_request->Slot;
        LOG_T(NR_PHY, "[%d, %d] ul_tti_request\n", frame, slot);
        copy_ul_tti_data_req_to_dl_info(&mac->dl_info, ul_tti_request);
    }
    else {
        if (pthread_mutex_unlock(&mac->mutex_dl_info)) abort();
        LOG_T(NR_MAC, "All indications were NULL in %s\n", __FUNCTION__);
        return;
    }

    if (dl_tti_request || tx_data_request || ul_dci_request) {
      fapi_nr_dl_config_request_t *dl_config = get_dl_config_request(mac, slot);
      fill_dci_from_dl_config(&mac->dl_info, dl_config);
    }
    nr_ue_dl_scheduler(mac, &mac->dl_info);
    nr_ue_dl_indication(&mac->dl_info);

    if (pthread_mutex_unlock(&mac->mutex_dl_info))
      abort();
    int slots_per_frame = 20; //30 kHZ subcarrier spacing
    int slot_ahead = 2; // TODO: Make this dynamic

    if (is_nr_UL_slot(mac->tdd_UL_DL_ConfigurationCommon,
                      (slot + slot_ahead) % slots_per_frame,
                      mac->frame_type)
        && mac->ra.ra_state != RA_SUCCEEDED) {
      // If we filled dl_info AFTER we got the slot indication, we want to check if we should fill tx_req:
      nr_uplink_indication_t ul_info = {.slot = (slot + slot_ahead) % slots_per_frame,
                                        .frame = slot + slot_ahead >= slots_per_frame ? (frame + 1) % 1024 : frame};
      nr_ue_ul_scheduler(mac, &ul_info);
    }
}

void save_nr_measurement_info(nfapi_nr_dl_tti_request_t *dl_tti_request)
{
    int num_pdus = dl_tti_request->dl_tti_request_body.nPDUs;
    char buffer[MAX_MESSAGE_SIZE];
    if (num_pdus <= 0)
    {
        LOG_E(NR_PHY, "%s: dl_tti_request number of PDUS <= 0\n", __FUNCTION__);
        abort();
    }
    LOG_T(NR_PHY, "%s: dl_tti_request number of PDUS: %d\n", __FUNCTION__, num_pdus);
    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_dl_tti_request_pdu_t *pdu_list = &dl_tti_request->dl_tti_request_body.dl_tti_pdu_list[i];
        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_SSB_PDU_TYPE)
        {
            LOG_T(NR_PHY, "Cell_id: %d, the ssb_block_idx %d, sc_offset: %d and payload %d\n",
                pdu_list->ssb_pdu.ssb_pdu_rel15.PhysCellId,
                pdu_list->ssb_pdu.ssb_pdu_rel15.SsbBlockIndex,
                pdu_list->ssb_pdu.ssb_pdu_rel15.SsbSubcarrierOffset,
                pdu_list->ssb_pdu.ssb_pdu_rel15.bchPayload);
            pdu_list->ssb_pdu.ssb_pdu_rel15.ssbRsrp = 60;
            LOG_T(NR_RRC, "Setting pdulist[%d].ssbRsrp to %d\n", i, pdu_list->ssb_pdu.ssb_pdu_rel15.ssbRsrp);
        }
    }

    size_t pack_len = nfapi_nr_p7_message_pack((void *)dl_tti_request,
                                    buffer,
                                    sizeof(buffer),
                                    NULL);
    if (pack_len < 0)
    {
        LOG_E(NR_PHY, "%s: Error packing nr p7 message.\n", __FUNCTION__);
    }
    nsa_sendmsg_to_lte_ue(buffer, pack_len, NR_UE_RRC_MEASUREMENT);
    LOG_A(NR_RRC, "Populated NR_UE_RRC_MEASUREMENT information and sent to LTE UE\n");
}

static void enqueue_nr_nfapi_msg(void *buffer, ssize_t len, nfapi_p7_message_header_t header)
{
    NR_UE_MAC_INST_t *mac = get_mac_inst(0);
    switch (header.message_id)
    {
        case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
        {
            nfapi_nr_dl_tti_request_t *dl_tti_request = MALLOC(sizeof(*dl_tti_request));
            if (nfapi_nr_p7_message_unpack(buffer, len, dl_tti_request,
                                            sizeof(*dl_tti_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message dl_tti_request failed to unpack\n");
                break;
            }
            LOG_D(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST message in sfn/slot %d %d. \n",
                    dl_tti_request->SFN, dl_tti_request->Slot);

            if (is_channel_modeling())
                save_pdsch_pdu_for_crnti(dl_tti_request);

            if (!put_queue(&nr_dl_tti_req_queue, dl_tti_request))
            {
                LOG_E(NR_PHY, "put_queue failed for dl_tti_request.\n");
                free(dl_tti_request);
                dl_tti_request = NULL;
            }
            break;
        }

        case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
        {
            nfapi_nr_tx_data_request_t *tx_data_request = MALLOC(sizeof(*tx_data_request));
            if (nfapi_nr_p7_message_unpack(buffer, len, tx_data_request,
                                        sizeof(*tx_data_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message tx_data_request failed to unpack\n");
                break;
            }
            LOG_D(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST message in SFN/slot %d %d. \n",
                    tx_data_request->SFN, tx_data_request->Slot);
            if (!put_queue(&nr_tx_req_queue, tx_data_request))
            {
                LOG_E(NR_PHY, "put_queue failed for tx_request.\n");
                free(tx_data_request);
                tx_data_request = NULL;
            }
            break;
        }

        case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
        {
            nfapi_nr_ul_dci_request_t *ul_dci_request = MALLOC(sizeof(*ul_dci_request));
            if (nfapi_nr_p7_message_unpack(buffer, len, ul_dci_request,
                                            sizeof(*ul_dci_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message ul_dci_request failed to unpack\n");
                break;
            }
            LOG_D(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST message in SFN/slot %d %d. \n",
                    ul_dci_request->SFN, ul_dci_request->Slot);
            if (!put_queue(&nr_ul_dci_req_queue, ul_dci_request))
            {
                LOG_E(NR_PHY, "put_queue failed for ul_dci_request.\n");
                free(ul_dci_request);
                ul_dci_request = NULL;
            }
            break;
        }

        case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
        {
            nfapi_nr_ul_tti_request_t *ul_tti_request = MALLOC(sizeof(*ul_tti_request));
            if (nfapi_nr_p7_message_unpack(buffer, len, ul_tti_request,
                                           sizeof(*ul_tti_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message ul_tti_request failed to unpack\n");
                break;
            }
            /* We are filtering UL_TTI_REQs below. We only care about UL_TTI_REQs that
               will trigger sending a ul_harq (CRC/RX pair). This UL_TTI_REQ will have
               NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE. If we have not yet completed the CBRA/
               CFRA procedure, we need to queue all UL_TTI_REQs. */
            for (int i = 0; i < ul_tti_request->n_pdus; i++) {
                if (ul_tti_request->pdus_list[i].pdu_type == NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE &&
                    mac->ra.ra_state >= RA_SUCCEEDED) {
                    if (!put_queue(&nr_ul_tti_req_queue, ul_tti_request))
                    {
                        LOG_D(NR_PHY, "put_queue failed for ul_tti_request, calling put_queue_replace.\n");
                        nfapi_nr_ul_tti_request_t *evicted_ul_tti_req = put_queue_replace(&nr_ul_tti_req_queue, ul_tti_request);
                        free(evicted_ul_tti_req);
                    }
                    break;
                }
                else if (mac->ra.ra_state < RA_SUCCEEDED) {
                    if (!put_queue(&nr_ul_tti_req_queue, ul_tti_request))
                    {
                        LOG_D(NR_PHY, "put_queue failed for ul_tti_request, calling put_queue_replace.\n");
                        nfapi_nr_ul_tti_request_t *evicted_ul_tti_req = put_queue_replace(&nr_ul_tti_req_queue, ul_tti_request);
                        free(evicted_ul_tti_req);
                    }
                    break;
                }
            }
            break;
        }

        default:
            LOG_E(NR_PHY, "Invalid nFAPI message. Header ID %d\n",
                  header.message_id);
            break;
    }
    return;
}

static void save_pdsch_pdu_for_crnti(nfapi_nr_dl_tti_request_t *dl_tti_request)
{
  int count_sent = 0;
  NR_UE_MAC_INST_t *mac = get_mac_inst(0);
  int number_of_pdu = dl_tti_request->dl_tti_request_body.nPDUs;
  if (number_of_pdu <= 0)
  {
    LOG_E(NR_MAC, "%s: dl_tti_request pdu size <= 0\n", __FUNCTION__);
    abort();
  }

  for (int i = 0; i < number_of_pdu; i++)
  {
    const nfapi_nr_dl_tti_request_pdu_t *pdu_list = &dl_tti_request->dl_tti_request_body.dl_tti_pdu_list[i];

    if (pdu_list->PDUType == NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE)
    {
      const nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu = &pdu_list->pdsch_pdu.pdsch_pdu_rel15;
      if (pdsch_pdu->rnti == mac->crnti && mac->ra.ra_state == RA_SUCCEEDED)
      {
        read_channel_param(pdsch_pdu, dl_tti_request->Slot, count_sent);
        count_sent++;
        LOG_T(NR_MAC, "pdsch_pdu->rnti %x  mac->crnti %x mac->ra.ra_state %d count_sent %d\n",
                      pdsch_pdu->rnti, mac->crnti, mac->ra.ra_state, count_sent);
      }
    }
  }
}

static uint16_t get_message_id(const uint8_t *buffer, ssize_t len)
{
  if (len < 4)
    return 0;

  // Unpack 2 bytes message_id from buffer.
  uint16_t v;
  memcpy(&v, buffer + 2, sizeof(v));
  return ntohs(v);
}

void *nrue_standalone_pnf_task(void *context)
{
  struct sockaddr_in server_address;
  socklen_t addr_len = sizeof(server_address);
  int sd = ue_rx_sock_descriptor;
  assert(sd > 0);

  char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];

  LOG_I(NR_RRC, "Successfully started %s.\n", __FUNCTION__);

  while (true)
  {
    ssize_t len = recvfrom(sd, buffer, sizeof(buffer), MSG_TRUNC, (struct sockaddr *)&server_address, &addr_len);
    if (len == -1)
    {
      LOG_E(NR_PHY, "reading from standalone pnf sctp socket failed \n");
      continue;
    }
    if (len > sizeof(buffer))
    {
      LOG_E(NR_PHY, "%s(%d). Message truncated. %zd\n", __FUNCTION__, __LINE__, len);
      continue;
    }
    if (len == sizeof(uint16_t))
    {
      uint16_t *sfn_slot = CALLOC(1, sizeof(*sfn_slot));
      memcpy(sfn_slot, buffer, sizeof(*sfn_slot));

      LOG_D(NR_PHY, "Received from proxy sfn %d slot %d\n",
            NFAPI_SFNSLOT2SFN(*sfn_slot), NFAPI_SFNSLOT2SLOT(*sfn_slot));

      if (!put_queue(&nr_sfn_slot_queue, sfn_slot))
      {
        LOG_E(NR_PHY, "put_queue failed for sfn slot.\n");
      }

      if (sem_post(&sfn_slot_semaphore) != 0)
      {
        LOG_E(NR_PHY, "sem_post() error\n");
        abort();
      }
    }
    else if (get_message_id((const uint8_t *)buffer, len) == 0x0FFF) // 0x0FFF : channel info identifier.
    {
      nr_phy_channel_params_t *ch_info = CALLOC(1, sizeof(*ch_info));
      memcpy(ch_info, buffer, sizeof(*ch_info));

      if (ch_info->nb_of_csi > 1)
        LOG_W(NR_PHY, "Expecting only one CSI report.\n");

      // TODO: Update sinr field of slot_rnti_mcs to be array.
      for (int i = 0; i < ch_info->nb_of_csi; ++i)
      {
        slot_rnti_mcs[NFAPI_SFNSLOT2SLOT(ch_info->sfn_slot)].sinr = ch_info->csi[i].sinr;
        slot_rnti_mcs[NFAPI_SFNSLOT2SLOT(ch_info->sfn_slot)].area_code = ch_info->csi[i].area_code;

        LOG_D(NR_PHY, "Received_SINR[%d] = %f, sfn:slot %d:%d\n",
              i, ch_info->csi[i].sinr, NFAPI_SFNSLOT2SFN(ch_info->sfn_slot), NFAPI_SFNSLOT2SLOT(ch_info->sfn_slot));
      }

      if (!put_queue(&nr_chan_param_queue, ch_info))
      {
        LOG_E(NR_PHY, "put_queue failed for sfn slot.\n");
      }

      if (sem_post(&sfn_slot_semaphore) != 0)
      {
        LOG_E(NR_MAC, "sem_post() error\n");
        abort();
      }
    }
    else
    {
      nfapi_p7_message_header_t header;
      if (nfapi_p7_message_header_unpack(buffer, len, &header, sizeof(header), NULL) < 0)
      {
        LOG_E(NR_PHY, "Header unpack failed for nrue_standalone pnf\n");
        continue;
      }
      enqueue_nr_nfapi_msg(buffer, len, header);
    }
  } //while(true)
}

//  L2 Abstraction Layer
static int handle_bcch_bch(NR_UE_MAC_INST_t *mac,
                           int cc_id,
                           unsigned int gNB_index,
                           void *phy_data,
                           uint8_t *pduP,
                           unsigned int additional_bits,
                           uint32_t ssb_index,
                           uint32_t ssb_length,
                           uint16_t ssb_start_subcarrier,
                           uint16_t cell_id)
{
  mac->mib_ssb = ssb_index;
  mac->physCellId = cell_id;
  mac->mib_additional_bits = additional_bits;
  if(ssb_length == 64)
    mac->frequency_range = FR2;
  else
    mac->frequency_range = FR1;
  nr_mac_rrc_data_ind_ue(mac->ue_id, cc_id, gNB_index, 0, 0, 0, NR_BCCH_BCH, (uint8_t *) pduP, 3);    //  fixed 3 bytes MIB PDU
  return 0;
}

//  L2 Abstraction Layer
static int handle_bcch_dlsch(NR_UE_MAC_INST_t *mac,
                             int cc_id,
                             unsigned int gNB_index,
                             uint8_t ack_nack,
                             uint8_t *pduP,
                             uint32_t pdu_len)
{
  return nr_ue_decode_BCCH_DL_SCH(mac, cc_id, gNB_index, ack_nack, pduP, pdu_len);
}

//  L2 Abstraction Layer
static int handle_dci(NR_UE_MAC_INST_t *mac,
                      int cc_id,
                      unsigned int gNB_index,
                      frame_t frame,
                      int slot,
                      fapi_nr_dci_indication_pdu_t *dci)
{
  return nr_ue_process_dci_indication_pdu(mac, cc_id, gNB_index, frame, slot, dci);
}

static void handle_ssb_meas(NR_UE_MAC_INST_t *mac, uint8_t ssb_index, int16_t rsrp_dbm)
{
  mac->ssb_measurements.ssb_index = ssb_index;
  mac->ssb_measurements.ssb_rsrp_dBm = rsrp_dbm;
}

// L2 Abstraction Layer
// Note: sdu should always be processed because data and timing advance updates are transmitted by the UE
static int8_t handle_dlsch(NR_UE_MAC_INST_t *mac, nr_downlink_indication_t *dl_info, int pdu_id)
{
  /* L1 assigns harq_pid, but in emulated L1 mode we need to assign
     the harq_pid based on the saved global g_harq_pid. Because we are
     emulating L1, no antenna measurements are conducted to calculate
     a harq_pid, therefore we must set it here. */
  if (get_softmodem_params()->emulate_l1)
    dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.harq_pid = g_harq_pid;

  update_harq_status(mac,
                     dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.harq_pid,
                     dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.ack_nack);
  if(dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.ack_nack)
    nr_ue_send_sdu(mac, dl_info, pdu_id);

  return 0;
}

static void handle_rlm(rlm_t rlm_result, int frame, NR_UE_MAC_INST_t *mac)
{
  if (rlm_result == RLM_no_monitoring)
    return;
  bool is_sync = rlm_result == RLM_in_sync ? true : false;
  nr_mac_rrc_sync_ind(mac->ue_id, frame, is_sync);
}

static int8_t handle_csirs_measurements(NR_UE_MAC_INST_t *mac,
                                        frame_t frame,
                                        int slot,
                                        fapi_nr_csirs_measurements_t *csirs_measurements)
{
  handle_rlm(csirs_measurements->radiolink_monitoring, frame, mac);
  return nr_ue_process_csirs_measurements(mac, frame, slot, csirs_measurements);
}

void update_harq_status(NR_UE_MAC_INST_t *mac, uint8_t harq_pid, uint8_t ack_nack)
{
  NR_UE_HARQ_STATUS_t *current_harq = &mac->dl_harq_info[harq_pid];

  if (current_harq->active) {
    LOG_D(PHY,"Updating harq_status for harq_id %d, ack/nak %d\n", harq_pid, current_harq->ack);
    // we can prepare feedback for MSG4 in advance
    if (mac->ra.ra_state == WAIT_CONTENTION_RESOLUTION)
      prepare_msg4_feedback(mac, harq_pid, ack_nack);
    else {
      current_harq->ack = ack_nack;
      current_harq->ack_received = true;
    }
  }
  else {
    //shouldn't get here
    LOG_E(NR_MAC, "Trying to process acknack for an inactive harq process (%d)\n", harq_pid);
  }
}

int nr_ue_ul_indication(nr_uplink_indication_t *ul_info)
{
  int ret = pthread_mutex_lock(&mac_IF_mutex);
  AssertFatal(!ret, "mutex failed %d\n", ret);
  LOG_D(PHY, "Locked in ul, slot %d\n", ul_info->slot);

  module_id_t module_id = ul_info->module_id;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

  LOG_T(NR_MAC, "Not calling scheduler mac->ra.ra_state = %d\n", mac->ra.ra_state);

  if (mac->phy_config_request_sent && is_nr_UL_slot(mac->tdd_UL_DL_ConfigurationCommon, ul_info->slot, mac->frame_type))
    nr_ue_ul_scheduler(mac, ul_info);
  pthread_mutex_unlock(&mac_IF_mutex);

  return 0;
}

static uint32_t nr_ue_dl_processing(nr_downlink_indication_t *dl_info)
{
  uint32_t ret_mask = 0x0;
  NR_UE_MAC_INST_t *mac = get_mac_inst(dl_info->module_id);

  // DL indication after reception of DCI or DL PDU
  if (dl_info && dl_info->dci_ind && dl_info->dci_ind->number_of_dcis) {
    LOG_T(MAC, "[L2][IF MODULE][DL INDICATION][DCI_IND]\n");
    for (int i = 0; i < dl_info->dci_ind->number_of_dcis; i++) {
      LOG_T(MAC, ">>>NR_IF_Module i=%d, dl_info->dci_ind->number_of_dcis=%d\n", i, dl_info->dci_ind->number_of_dcis);
      int8_t ret = handle_dci(mac,
                              dl_info->cc_id,
                              dl_info->gNB_index,
                              dl_info->frame,
                              dl_info->slot,
                              dl_info->dci_ind->dci_list + i);
      if (ret < 0)
        continue;
      fapi_nr_dci_indication_pdu_t *dci_index = dl_info->dci_ind->dci_list + i;

      /* The check below filters out UL_DCIs which are being processed as DL_DCIs. */
      if (dci_index->dci_format != NR_DL_DCI_FORMAT_1_0 && dci_index->dci_format != NR_DL_DCI_FORMAT_1_1) {
        LOG_D(NR_MAC, "We are filtering a UL_DCI to prevent it from being treated like a DL_DCI\n");
        continue;
      }
      dci_pdu_rel15_t *def_dci_pdu_rel15 = &mac->def_dci_pdu_rel15[dl_info->slot][dci_index->dci_format];
      g_harq_pid = def_dci_pdu_rel15->harq_pid;
      LOG_T(NR_MAC, "Setting harq_pid = %d and dci_index = %d (based on format)\n", g_harq_pid, dci_index->dci_format);

      ret_mask |= (ret << FAPI_NR_DCI_IND);
      AssertFatal(nr_ue_if_module_inst[dl_info->module_id] != NULL, "IF module is NULL!\n");
      fapi_nr_dl_config_request_t *dl_config = get_dl_config_request(mac, dl_info->slot);
      nr_scheduled_response_t scheduled_response = {.dl_config = dl_config,
                                                    .mac = mac,
                                                    .module_id = dl_info->module_id,
                                                    .CC_id = dl_info->cc_id,
                                                    .phy_data = dl_info->phy_data};
      nr_ue_if_module_inst[dl_info->module_id]->scheduled_response(&scheduled_response);
      memset(def_dci_pdu_rel15, 0, sizeof(*def_dci_pdu_rel15));
    }
    dl_info->dci_ind = NULL;
    }

    if (dl_info->rx_ind != NULL) {

      for (int i = 0; i < dl_info->rx_ind->number_pdus; ++i) {

        fapi_nr_rx_indication_body_t rx_indication_body = dl_info->rx_ind->rx_indication_body[i];
        LOG_D(NR_MAC,
              "slot %d Sending DL indication to MAC. 1 PDU type %d of %d total number of PDUs \n",
              dl_info->slot,
              rx_indication_body.pdu_type,
              dl_info->rx_ind->number_pdus);

        switch(rx_indication_body.pdu_type){
          case FAPI_NR_RX_PDU_TYPE_SSB:
            handle_rlm(rx_indication_body.ssb_pdu.radiolink_monitoring,
                       dl_info->frame,
                       mac);
            if(rx_indication_body.ssb_pdu.decoded_pdu) {
              handle_ssb_meas(mac,
                              rx_indication_body.ssb_pdu.ssb_index,
                              rx_indication_body.ssb_pdu.rsrp_dBm);
              ret_mask |= (handle_bcch_bch(mac,
                                           dl_info->cc_id,
                                           dl_info->gNB_index,
                                           dl_info->phy_data,
                                           rx_indication_body.ssb_pdu.pdu,
                                           rx_indication_body.ssb_pdu.additional_bits,
                                           rx_indication_body.ssb_pdu.ssb_index,
                                           rx_indication_body.ssb_pdu.ssb_length,
                                           rx_indication_body.ssb_pdu.ssb_start_subcarrier,
                                           rx_indication_body.ssb_pdu.cell_id)) << FAPI_NR_RX_PDU_TYPE_SSB;
            }
            break;
          case FAPI_NR_RX_PDU_TYPE_SIB:
            ret_mask |= (handle_bcch_dlsch(mac,
                                           dl_info->cc_id, dl_info->gNB_index,
                                           rx_indication_body.pdsch_pdu.ack_nack,
                                           rx_indication_body.pdsch_pdu.pdu,
                                           rx_indication_body.pdsch_pdu.pdu_length)) << FAPI_NR_RX_PDU_TYPE_SIB;
            break;
          case FAPI_NR_RX_PDU_TYPE_DLSCH:
            ret_mask |= (handle_dlsch(mac, dl_info, i)) << FAPI_NR_RX_PDU_TYPE_DLSCH;
            break;
          case FAPI_NR_RX_PDU_TYPE_RAR:
            ret_mask |= (handle_dlsch(mac, dl_info, i)) << FAPI_NR_RX_PDU_TYPE_RAR;
            if (!dl_info->rx_ind->rx_indication_body[i].pdsch_pdu.ack_nack)
              LOG_W(PHY, "Received a RAR-Msg2 but LDPC decode failed\n");
            else
              LOG_I(PHY, "RAR-Msg2 decoded\n");
            break;
          case FAPI_NR_CSIRS_IND:
            ret_mask |= (handle_csirs_measurements(mac,
                                                   dl_info->frame,
                                                   dl_info->slot,
                                                   &rx_indication_body.csirs_measurements)) << FAPI_NR_CSIRS_IND;
            break;
          default:
            break;
        }
      }
      dl_info->rx_ind = NULL;
    }
    return ret_mask;
}

int nr_ue_dl_indication(nr_downlink_indication_t *dl_info)
{
  int ret = pthread_mutex_lock(&mac_IF_mutex);
  AssertFatal(!ret, "mutex failed %d\n", ret);
  uint32_t ret2 = 0;
  NR_UE_MAC_INST_t *mac = get_mac_inst(dl_info->module_id);
  if (!dl_info->dci_ind && !dl_info->rx_ind)
    // DL indication to process DCI reception
    nr_ue_dl_scheduler(mac, dl_info);
  else
    // DL indication to process data channels
    ret2 = nr_ue_dl_processing(dl_info);
  pthread_mutex_unlock(&mac_IF_mutex);
  return ret2;
}

nr_ue_if_module_t *nr_ue_if_module_init(uint32_t module_id)
{
  if (nr_ue_if_module_inst[module_id] == NULL) {
    nr_ue_if_module_inst[module_id] = (nr_ue_if_module_t *)malloc(sizeof(nr_ue_if_module_t));
    memset((void*)nr_ue_if_module_inst[module_id],0,sizeof(nr_ue_if_module_t));

    nr_ue_if_module_inst[module_id]->cc_mask=0;
    nr_ue_if_module_inst[module_id]->current_frame = 0;
    nr_ue_if_module_inst[module_id]->current_slot = 0;
    nr_ue_if_module_inst[module_id]->phy_config_request = nr_ue_phy_config_request;
    nr_ue_if_module_inst[module_id]->synch_request = nr_ue_synch_request;
    if (get_softmodem_params()->emulate_l1)
      nr_ue_if_module_inst[module_id]->scheduled_response = nr_ue_scheduled_response_stub;
    else
      nr_ue_if_module_inst[module_id]->scheduled_response = nr_ue_scheduled_response;
    nr_ue_if_module_inst[module_id]->dl_indication = nr_ue_dl_indication;
    nr_ue_if_module_inst[module_id]->ul_indication = nr_ue_ul_indication;
  }
  pthread_mutex_init(&mac_IF_mutex, NULL);

  return nr_ue_if_module_inst[module_id];
}

void RCconfig_nr_ue_macrlc(void) {
  int j;
  paramdef_t MACRLC_Params[] = MACRLCPARAMS_DESC;
  paramlist_def_t MACRLC_ParamList = {CONFIG_STRING_MACRLC_LIST, NULL, 0};

  config_getlist(config_get_if(), &MACRLC_ParamList, MACRLC_Params, sizeofArray(MACRLC_Params), NULL);
  if (MACRLC_ParamList.numelt > 0) {
    for (j = 0; j < MACRLC_ParamList.numelt; j++) {
      if (strcmp(*(MACRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "nfapi") == 0) {
        stub_eth_params.local_if_name = strdup(
            *(MACRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_IF_NAME_IDX].strptr));
        stub_eth_params.my_addr = strdup(
            *(MACRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_ADDRESS_IDX].strptr));
        stub_eth_params.remote_addr = strdup(
            *(MACRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_ADDRESS_IDX].strptr));
        stub_eth_params.my_portc =
            *(MACRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTC_IDX].iptr);
        stub_eth_params.remote_portc =
            *(MACRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTC_IDX].iptr);
        stub_eth_params.my_portd =
            *(MACRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTD_IDX].iptr);
        stub_eth_params.remote_portd =
            *(MACRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTD_IDX].iptr);
        stub_eth_params.transp_preference = ETH_UDP_MODE;
      }
    }
  }
}
