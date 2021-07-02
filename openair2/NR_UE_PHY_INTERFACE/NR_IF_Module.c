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

#include "PHY/defs_nr_UE.h"
#include "NR_IF_Module.h"
#include "NR_MAC_UE/mac_proto.h"
#include "assertions.h"
#include "NR_MAC_UE/mac_extern.h"
#include "SCHED_NR_UE/fapi_nr_ue_l1.h"
#include "executables/softmodem-common.h"
#include "openair2/RRC/NR_UE/rrc_proto.h"

#include <stdio.h>

#define MAX_IF_MODULES 100

const char *dl_indication_type[] = {"MIB", "SIB", "DLSCH", "DCI", "RAR"};

UL_IND_t *UL_INFO = NULL;


static nr_ue_if_module_t *nr_ue_if_module_inst[MAX_IF_MODULES];
static int ue_tx_sock_descriptor = -1;
static int ue_rx_sock_descriptor = -1;
int current_sfn_slot;
sem_t sfn_slot_semaphore;

void nrue_init_standalone_socket(const char *addr, int tx_port, int rx_port)
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
      LOG_E(MAC, "Socket creation error standalone PNF\n");
      return;
    }

    if (inet_pton(server_address.sin_family, addr, &server_address.sin_addr) <= 0)
    {
      LOG_E(MAC, "Invalid standalone PNF Address\n");
      close(sd);
      return;
    }

    // Using connect to use send() instead of sendto()
    if (connect(sd, (struct sockaddr *)&server_address, addr_len) < 0)
    {
      LOG_E(MAC, "Connection to standalone PNF failed: %s\n", strerror(errno));
      close(sd);
      return;
    }
    assert(ue_tx_sock_descriptor == -1);
    ue_tx_sock_descriptor = sd;
    LOG_D(NR_RRC, "Sucessfully set up tx_socket in %s.\n", __FUNCTION__);
  }

  {
    struct sockaddr_in server_address;
    int addr_len = sizeof(server_address);
    memset(&server_address, 0, addr_len);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(rx_port);

    int sd = socket(server_address.sin_family, SOCK_DGRAM, 0);
    if (sd < 0)
    {
      LOG_E(MAC, "Socket creation error standalone PNF\n");
      return;
    }

    if (inet_pton(server_address.sin_family, addr, &server_address.sin_addr) <= 0)
    {
      LOG_E(MAC, "Invalid standalone PNF Address\n");
      close(sd);
      return;
    }

    if (bind(sd, (struct sockaddr *)&server_address, addr_len) < 0)
    {
      LOG_E(MAC, "Connection to standalone PNF failed: %s\n", strerror(errno));
      close(sd);
      return;
    }
    assert(ue_rx_sock_descriptor == -1);
    ue_rx_sock_descriptor = sd;
    LOG_D(NR_RRC, "Sucessfully set up rx_socket in %s.\n", __FUNCTION__);
  }
  LOG_I(NR_RRC, "NRUE standalone socket info: tx_port %d  rx_port %d on %s.\n",
        tx_port, rx_port, addr);
}

void send_nsa_standalone_msg(NR_UL_IND_t *UL_INFO, uint16_t msg_id)
{
  switch(msg_id)
  {
    case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION:
    {
        char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
        LOG_D(NR_MAC, "RACH header id :%d", UL_INFO->rach_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->rach_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_I(NR_MAC, "NR_RACH_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
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
        LOG_D(NR_MAC, "RX header id :%d", UL_INFO->rx_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->rx_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_I(NR_MAC, "NR_RX_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
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
        LOG_D(NR_MAC, "CRC header id :%d", UL_INFO->crc_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->crc_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_I(NR_MAC, "NR_CRC_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
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
        LOG_I(NR_MAC, "UCI header id :%d", UL_INFO->uci_ind.header.message_id);
        int encoded_size = nfapi_nr_p7_message_pack(&UL_INFO->uci_ind, buffer, sizeof(buffer), NULL);
        if (encoded_size <= 0)
        {
                LOG_E(NR_MAC, "nfapi_nr_p7_message_pack has failed. Encoded size = %d\n", encoded_size);
                return;
        }

        LOG_I(NR_MAC, "NR_UCI_IND sent to Proxy, Size: %d Frame %d Slot %d Num PDUS %d\n", encoded_size,
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

static void copy_dl_tti_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_dl_tti_request_t *dl_tti_request)
{
    int num_pdus = dl_tti_request->dl_tti_request_body.nPDUs;
    if (num_pdus <= 0)
    {
        LOG_E(NR_PHY, "%s: dl_tti_request number of PDUS <= 0\n", __FUNCTION__);
        abort();
    }

    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_dl_tti_request_pdu_t *pdu_list = &dl_tti_request->dl_tti_request_body.dl_tti_pdu_list[i];
        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE)
        {
            LOG_I(NR_PHY, "[%d, %d] PDSCH PDU for rnti %x\n",
                dl_tti_request->SFN, dl_tti_request->Slot, pdu_list->pdsch_pdu.pdsch_pdu_rel15.rnti);
        }

        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE)
        {
            LOG_I(NR_PHY, "[%d, %d] PDCCH DCI PDU (Format for incoming PDSCH PDU)\n",
                dl_tti_request->SFN, dl_tti_request->Slot);
            uint16_t num_dcis = pdu_list->pdcch_pdu.pdcch_pdu_rel15.numDlDci;
            if (num_dcis > 0)
            {
                dl_info->dci_ind = CALLOC(1, sizeof(fapi_nr_dci_indication_t));
                dl_info->dci_ind->number_of_dcis = num_dcis;
                dl_info->dci_ind->SFN = dl_tti_request->SFN;
                dl_info->dci_ind->slot = dl_tti_request->Slot;
                AssertFatal(num_dcis < sizeof(dl_info->dci_ind->dci_list) / sizeof(dl_info->dci_ind->dci_list[0]), "The number of DCIs is greater than dci_list");
                for (int j = 0; j < num_dcis; j++)
                {
                    nfapi_nr_dl_dci_pdu_t *dci_pdu_list = &pdu_list->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[j];
                    int num_bytes = dci_pdu_list->PayloadSizeBits / 8;
                    if (dci_pdu_list->PayloadSizeBits % 8 > 0)
                    {
                        num_bytes++;
                    }
                    LOG_I(NR_PHY, "[%d, %d] PDCCH DCI (Payload) for rnti %x with PayloadSizeBits %d, num_bytes %d\n",
                        dl_tti_request->SFN, dl_tti_request->Slot, dci_pdu_list->RNTI, dci_pdu_list->PayloadSizeBits, num_bytes);
                    for (int k = 0; k < num_bytes; k++)
                    {
                        LOG_I(NR_MAC, "PDCCH DCI PDU payload[%d] = %d\n", k, dci_pdu_list->Payload[k]);
                        dl_info->dci_ind->dci_list[j].payloadBits[k] = dci_pdu_list->Payload[k];
                    }
                    dl_info->dci_ind->dci_list[j].payloadSize = dci_pdu_list->PayloadSizeBits;
                    dl_info->dci_ind->dci_list[j].rnti = dci_pdu_list->RNTI;
                }
            }
        }
    }
    dl_info->slot = dl_tti_request->Slot;
    dl_info->frame = dl_tti_request->SFN;
}

static void copy_tx_data_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_tx_data_request_t *tx_data_request)
{
    int num_pdus = tx_data_request->Number_of_PDUs;
    if (num_pdus <= 0)
    {
        LOG_E(NR_PHY, "%s: tx_data_request number of PDUS <= 0\n", __FUNCTION__);
        abort();
    }

    dl_info->rx_ind = CALLOC(1, sizeof(fapi_nr_rx_indication_t));
    dl_info->rx_ind->sfn = tx_data_request->SFN;
    dl_info->rx_ind->slot = tx_data_request->Slot;
    dl_info->rx_ind->number_pdus = num_pdus;

    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_pdu_t *pdu_list = &tx_data_request->pdu_list[i];
        AssertFatal(pdu_list->num_TLV < sizeof(pdu_list->TLVs) / sizeof(pdu_list->TLVs[0]), "Num TLVs exceeds TLV array size");
        for (int j = 0; j < pdu_list->num_TLV; j++)
        {
            if (pdu_list->TLVs[j].tag)
                dl_info->rx_ind->rx_indication_body[i].pdsch_pdu.pdu = (void*) pdu_list->TLVs[j].value.ptr; // Melissa, fix me!
            else if (!pdu_list->TLVs[j].tag)
                dl_info->rx_ind->rx_indication_body[i].pdsch_pdu.pdu = (void*) pdu_list->TLVs[j].value.direct; // Melissa, fix me!
            dl_info->rx_ind->rx_indication_body[i].pdsch_pdu.pdu_length = pdu_list->TLVs[j].length;
            if (tx_data_request->Slot == 7) { //Melissa this means we have an RAR, sorta hacky though
                dl_info->rx_ind->rx_indication_body[i].pdu_type = FAPI_NR_RX_PDU_TYPE_RAR;
            }
            else if (tx_data_request->Slot != 7 && get_softmodem_params()->nsa) {
                dl_info->rx_ind->rx_indication_body[i].pdu_type = FAPI_NR_RX_PDU_TYPE_DLSCH;
            }
        }
    }
    dl_info->slot = tx_data_request->Slot;
    dl_info->frame = tx_data_request->SFN;
}

static void copy_ul_dci_data_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_ul_dci_request_t *ul_dci_req)
{
    int num_pdus = ul_dci_req->numPdus;
    if (num_pdus <= 0)
    {
        LOG_E(NR_PHY, "%s: ul_dci_request number of PDUS <= 0\n", __FUNCTION__);
        abort();
    }

    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_ul_dci_request_pdus_t *pdu_list = &ul_dci_req->ul_dci_pdu_list[i];
        AssertFatal(pdu_list->PDUType == 0, "ul_dci_req pdu type != PUCCH");
        LOG_I(NR_PHY, "[%d %d] PUCCH PDU in ul_dci for rnti %x\n", ul_dci_req->SFN, ul_dci_req->Slot, pdu_list->pdcch_pdu.pdcch_pdu_rel15.dci_pdu->RNTI);
        uint16_t num_dci = pdu_list->pdcch_pdu.pdcch_pdu_rel15.numDlDci;
        if (num_dci > 0)
        {
            dl_info->dci_ind = CALLOC(1, sizeof(fapi_nr_dci_indication_t));
            dl_info->dci_ind->number_of_dcis = num_dci;
            dl_info->dci_ind->SFN = ul_dci_req->SFN;
            dl_info->dci_ind->slot = ul_dci_req->Slot;
            AssertFatal(num_dci < sizeof(dl_info->dci_ind->dci_list) / sizeof(dl_info->dci_ind->dci_list[0]), "The number of DCIs is greater than dci_list");
            for (int j = 0; j < num_dci; j++)
            {
                nfapi_nr_dl_dci_pdu_t *dci_pdu_list = &pdu_list->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[j];
                int num_bytes = dci_pdu_list->PayloadSizeBits / 8;
                if (dci_pdu_list->PayloadSizeBits % 8 > 0)
                {
                    num_bytes++;
                }
                LOG_I(NR_PHY, "[%d, %d] ul_dci_req PDCCH DCI for rnti %x with PayloadSizeBits %d and num_bytes %d\n",
                        ul_dci_req->SFN, ul_dci_req->Slot, dci_pdu_list->RNTI, dci_pdu_list->PayloadSizeBits, num_bytes);
                for (int k = 0; k < num_bytes; k++)
                {
                    LOG_I(NR_MAC, "PDCCH DCI PDU payload[%d] = %d\n", k, dci_pdu_list->Payload[k]);
                    dl_info->dci_ind->dci_list[j].payloadBits[k] = dci_pdu_list->Payload[k];
                }
                dl_info->dci_ind->dci_list[j].payloadSize = dci_pdu_list->PayloadSizeBits;
                dl_info->dci_ind->dci_list[j].rnti = dci_pdu_list->RNTI;
            }
        }
    }
    dl_info->frame = ul_dci_req->SFN;
    dl_info->slot = ul_dci_req->Slot;
}

static void copy_ul_tti_data_req_to_dl_info(nr_downlink_indication_t *dl_info, nfapi_nr_ul_tti_request_t *ul_tti_req)
{
    int num_pdus = ul_tti_req->n_pdus;
    if (num_pdus <= 0)
    {
        LOG_E(NR_PHY, "%s: ul_tti_request number of PDUS <= 0\n", __FUNCTION__);
        abort();
    }
    AssertFatal(num_pdus <= sizeof(ul_tti_req->pdus_list) / sizeof(ul_tti_req->pdus_list[0]),
                "Too many pdus %d in ul_tti_req\n", num_pdus);

    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_ul_tti_request_number_of_pdus_t *pdu_list = &ul_tti_req->pdus_list[i];
        LOG_D(NR_PHY, "This is the pdu type %d in ul_tti_req\n", pdu_list->pdu_type);
        if (pdu_list->pdu_type == NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE)
        {
            nfapi_nr_uci_indication_t *uci_ind = unqueue(&nr_uci_ind_queue);
            if (uci_ind && uci_ind->num_ucis > 0)
            {
                uci_ind->sfn = ul_tti_req->SFN;
                uci_ind->slot = ul_tti_req->Slot;
                LOG_I(NR_MAC, "We have unqueued the previously filled uci_ind and updated the snf/slot to %d/%d.\n",
                    uci_ind->sfn, uci_ind->slot);

                NR_UL_IND_t UL_INFO = {
                    .uci_ind = *uci_ind,
                };
                send_nsa_standalone_msg(&UL_INFO, uci_ind->header.message_id);
                free(uci_ind->uci_list);
            }
        }

    }
}

static void fill_dci_from_dl_config(nr_downlink_indication_t*dl_ind, fapi_nr_dl_config_request_t *dl_config)
{
  if (!dl_ind->dci_ind)
  {
    return;
  }

  AssertFatal(dl_config->number_pdus < sizeof(dl_config->dl_config_list) / sizeof(dl_config->dl_config_list[0]),
              "Too many dl_config pdus %d", dl_config->number_pdus);
  for (int i = 0; i < dl_config->number_pdus; i++)
  {
    LOG_I(PHY, "In %s: filling DCI with a total of %d total DL PDUs (dl_config %p) \n",
          __FUNCTION__, dl_config->number_pdus, dl_config);
    fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15_dci = &dl_config->dl_config_list[i].dci_config_pdu.dci_config_rel15;
    int num_dci_options = rel15_dci->num_dci_options;
    if (num_dci_options <= 0)
    {
      LOG_I(NR_MAC, "num_dci_opts = %d for %dth pdu in dl_config_list\n", rel15_dci->num_dci_options, i);
    }
    AssertFatal(num_dci_options <= sizeof(rel15_dci->dci_length_options) / sizeof(rel15_dci->dci_length_options[0]),
                "num_dci_options %d > dci_length_options array\n", num_dci_options);
    AssertFatal(num_dci_options <= sizeof(rel15_dci->dci_format_options) / sizeof(rel15_dci->dci_format_options[0]),
                "num_dci_options %d > dci_format_options array\n", num_dci_options);

    for (int j = 0; j < num_dci_options; j++)
    {
      int num_dcis = dl_ind->dci_ind->number_of_dcis;
      AssertFatal(num_dcis <= sizeof(dl_ind->dci_ind->dci_list) / sizeof(dl_ind->dci_ind->dci_list[0]),
                  "dl_config->number_pdus %d > dci_ind->dci_list array\n", num_dcis);
      for (int k = 0; k < num_dcis; k++)
      {
        LOG_I(NR_PHY, "Received len %d, length options[%d] %d, format assigned %d, format options[%d] %d\n",
                  dl_ind->dci_ind->dci_list[k].payloadSize, j, rel15_dci->dci_length_options[j],
                  dl_ind->dci_ind->dci_list[k].dci_format, j, rel15_dci->dci_format_options[j]);
        if (rel15_dci->dci_length_options[j] == dl_ind->dci_ind->dci_list[k].payloadSize)
        {
            dl_ind->dci_ind->dci_list[k].dci_format = rel15_dci->dci_format_options[j];
        }
        int CCEind = rel15_dci->CCE[j];
        int L = rel15_dci->L[j];
        dl_ind->dci_ind->dci_list[k].n_CCE = CCEind;
        dl_ind->dci_ind->dci_list[k].N_CCE = L;
      }
    }
  }
}

static void check_and_process_dci(nfapi_nr_dl_tti_request_t *dl_tti_request,
                                 nfapi_nr_tx_data_request_t *tx_data_request,
                                 nfapi_nr_ul_dci_request_t *ul_dci_request,
                                 nfapi_nr_ul_tti_request_t *ul_tti_request)
{
    frame_t frame = 0;
    int slot = 0;
    NR_UE_MAC_INST_t *mac = get_mac_inst(0);

    if (mac->scc == NULL)
    {
      return;
    }

    if (dl_tti_request)
    {
        frame = dl_tti_request->SFN;
        slot = dl_tti_request->Slot;
        LOG_I(NR_PHY, "[%d, %d] dl_tti_request\n", frame, slot);
        copy_dl_tti_req_to_dl_info(&mac->dl_info, dl_tti_request);
    }
    else if (tx_data_request)
    {
        frame = tx_data_request->SFN;
        slot = tx_data_request->Slot;
        LOG_I(NR_PHY, "[%d, %d] PDSCH in tx_request\n", frame, slot);
        copy_tx_data_req_to_dl_info(&mac->dl_info, tx_data_request);
    }
    else if (ul_dci_request)
    {
        frame = ul_dci_request->SFN;
        slot = ul_dci_request->Slot;
        LOG_I(NR_PHY, "[%d, %d] ul_dci_request\n", frame, slot);
        copy_ul_dci_data_req_to_dl_info(&mac->dl_info, ul_dci_request);
    }
    else if (ul_tti_request)
    {
        frame = ul_tti_request->SFN;
        slot = ul_tti_request->Slot;
        LOG_I(NR_PHY, "[%d, %d] ul_tti_request\n", frame, slot);
        copy_ul_tti_data_req_to_dl_info(&mac->dl_info, ul_tti_request);
    }
    else
    {
        return;
    }


    NR_UL_TIME_ALIGNMENT_t ul_time_alignment;
    memset(&ul_time_alignment, 0, sizeof(ul_time_alignment));
    fill_dci_from_dl_config(&mac->dl_info, &mac->dl_config_request);
    nr_ue_dl_indication(&mac->dl_info, &ul_time_alignment);

    // If we filled dl_info AFTER we got the slot indication, we want to check if we should fill tx_req:
    nr_uplink_indication_t ul_info;
    memset(&ul_info, 0, sizeof(ul_info));
    int slots_per_frame = 20; //30 kHZ subcarrier spacing
    int slot_ahead = 6; // Melissa lets make this dynamic
    ul_info.frame_rx = frame;
    ul_info.slot_rx = slot;
    ul_info.slot_tx = (slot + slot_ahead) % slots_per_frame;
    ul_info.frame_tx = (ul_info.slot_rx + slot_ahead >= slots_per_frame) ? ul_info.frame_rx + 1 : ul_info.frame_rx;
    if (mac->scc && is_nr_UL_slot(mac->scc, ul_info.slot_tx))
    {
        nr_ue_ul_indication(&ul_info);
    }


    #if 0 //Melissa may want to free this
    free(dl_info.dci_ind);
    dl_info.dci_ind = NULL;

    free(dl_info.rx_ind);
    dl_info.rx_ind = NULL;
    #endif

}

static void save_nr_measurement_info(nfapi_nr_dl_tti_request_t *dl_tti_request)
{
    int num_pdus = dl_tti_request->dl_tti_request_body.nPDUs;
    char buffer[MAX_MESSAGE_SIZE];
    if (num_pdus <= 0)
    {
        LOG_E(NR_PHY, "%s: dl_tti_request number of PDUS <= 0\n", __FUNCTION__);
        abort();
    }
    LOG_D(NR_PHY, "%s: dl_tti_request number of PDUS: %d\n", __FUNCTION__, num_pdus);
    for (int i = 0; i < num_pdus; i++)
    {
        nfapi_nr_dl_tti_request_pdu_t *pdu_list = &dl_tti_request->dl_tti_request_body.dl_tti_pdu_list[i];
        if (pdu_list->PDUType == NFAPI_NR_DL_TTI_SSB_PDU_TYPE)
        {
            LOG_D(NR_PHY, "Cell_id: %d, the ssb_block_idx %d, sc_offset: %d and payload %d\n",
                pdu_list->ssb_pdu.ssb_pdu_rel15.PhysCellId,
                pdu_list->ssb_pdu.ssb_pdu_rel15.SsbBlockIndex,
                pdu_list->ssb_pdu.ssb_pdu_rel15.SsbSubcarrierOffset,
                pdu_list->ssb_pdu.ssb_pdu_rel15.bchPayload);
            pdu_list->ssb_pdu.ssb_pdu_rel15.ssbRsrp = 60;
            LOG_D(NR_RRC, "Setting pdulist[%d].ssbRsrp to %d\n", i, pdu_list->ssb_pdu.ssb_pdu_rel15.ssbRsrp);
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

void *nrue_standalone_pnf_task(void *context)
{
  struct sockaddr_in server_address;
  socklen_t addr_len = sizeof(server_address);
  char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
  int sd = ue_rx_sock_descriptor;
  assert(sd > 0);
  LOG_I(NR_RRC, "Sucessfully started %s.\n", __FUNCTION__);

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
      uint16_t sfn_slot = 0;
      memcpy((void *)&sfn_slot, buffer, sizeof(sfn_slot));
      current_sfn_slot = sfn_slot;

      if (sem_post(&sfn_slot_semaphore) != 0)
      {
        LOG_E(NR_PHY, "sem_post() error\n");
        abort();
      }
      int sfn = NFAPI_SFNSLOT2SFN(sfn_slot);
      int slot = NFAPI_SFNSLOT2SLOT(sfn_slot);
      LOG_I(NR_PHY, "Received from proxy sfn %d slot %d\n", sfn, slot);
    }
    else if (len == sizeof(nr_phy_channel_params_t))
    {
      nr_phy_channel_params_t ch_info;
      memcpy(&ch_info, buffer, sizeof(nr_phy_channel_params_t));
      current_sfn_slot = ch_info.sfn_slot;
      if (sem_post(&sfn_slot_semaphore) != 0)
      {
        LOG_E(MAC, "sem_post() error\n");
        abort();
      }
      LOG_I(MAC, "Received_SINR = %f\n", ch_info.sinr);
    }
    else
    {
      nfapi_p7_message_header_t header;
      nfapi_nr_dl_tti_request_t dl_tti_request;
      nfapi_nr_ul_tti_request_t ul_tti_request;
      nfapi_nr_tx_data_request_t tx_data_request;
      nfapi_nr_ul_dci_request_t ul_dci_request;
      if (nfapi_p7_message_header_unpack((void *)buffer, len, &header, sizeof(header), NULL) < 0)
      {
        LOG_E(NR_PHY, "Header unpack failed for nrue_standalone pnf\n");
        continue;
      }
      else
      {
        switch (header.message_id)
        {
          case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
            if (nfapi_nr_p7_message_unpack((void *)buffer, len, &dl_tti_request,
                                           sizeof(nfapi_nr_dl_tti_request_t), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message dl_tti_request failed to unpack\n");
                break;
            }
            LOG_I(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST message in sfn/slot %d %d. \n",
                  dl_tti_request.SFN, dl_tti_request.Slot);
            save_nr_measurement_info(&dl_tti_request);
            check_and_process_dci(&dl_tti_request, NULL, NULL, NULL);
            break;
          case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
            if (nfapi_nr_p7_message_unpack((void *)buffer, len, &tx_data_request,
                                           sizeof(tx_data_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message tx_data_request failed to unpack\n");
                break;
            }
            LOG_I(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST message in SFN/slot %d %d. \n",
                  tx_data_request.SFN, tx_data_request.Slot);
            check_and_process_dci(NULL, &tx_data_request, NULL, NULL);
            break;
          case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
            if (nfapi_nr_p7_message_unpack((void *)buffer, len, &ul_dci_request,
                                           sizeof(ul_dci_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message ul_dci_request failed to unpack\n");
                break;
            }
            LOG_I(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST message in SFN/slot %d %d. \n",
                  ul_dci_request.SFN, ul_dci_request.Slot);
            check_and_process_dci(NULL, NULL, &ul_dci_request, NULL);
            break;
          case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
            if (nfapi_nr_p7_message_unpack((void *)buffer, len, &ul_tti_request,
                                           sizeof(ul_tti_request), NULL) < 0)
            {
                LOG_E(NR_PHY, "Message ul_tti_request failed to unpack\n");
                break;
            }
            LOG_I(NR_PHY, "Received an NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST message in SFN/slot %d %d. \n",
                  ul_tti_request.SFN, ul_tti_request.Slot);
            check_and_process_dci(NULL, NULL, NULL, &ul_tti_request);
            break;
          default:
            LOG_E(NR_PHY, "Case Statement has no corresponding nfapi message, this is the header ID %d\n", header.message_id);
            break;
        }
      }
    }
  } //while(true)
}

//  L2 Abstraction Layer
int handle_bcch_bch(module_id_t module_id, int cc_id, unsigned int gNB_index, uint8_t *pduP, unsigned int additional_bits, uint32_t ssb_index, uint32_t ssb_length, uint16_t cell_id){

  return nr_ue_decode_mib(module_id,
			  cc_id,
			  gNB_index,
			  additional_bits,
			  ssb_length,  //  Lssb = 64 is not support    
			  ssb_index,
			  pduP, 
			  cell_id);

}

//  L2 Abstraction Layer
int handle_bcch_dlsch(module_id_t module_id, int cc_id, unsigned int gNB_index, uint32_t sibs_mask, uint8_t *pduP, uint32_t pdu_len){
  return nr_ue_decode_sib1(module_id, cc_id, gNB_index, sibs_mask, pduP, pdu_len);
}

//  L2 Abstraction Layer
int handle_dci(module_id_t module_id, int cc_id, unsigned int gNB_index, frame_t frame, int slot, fapi_nr_dci_indication_pdu_t *dci){

  return nr_ue_process_dci_indication_pdu(module_id, cc_id, gNB_index, frame, slot, dci);

}

// L2 Abstraction Layer
// Note: sdu should always be processed because data and timing advance updates are transmitted by the UE
int8_t handle_dlsch (nr_downlink_indication_t *dl_info, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment, int pdu_id){

  nr_ue_send_sdu(dl_info, ul_time_alignment, pdu_id);

  return 0;

}

int nr_ue_ul_indication(nr_uplink_indication_t *ul_info){

  NR_UE_L2_STATE_t ret;
  module_id_t module_id = ul_info->module_id;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);

  ret = nr_ue_scheduler(NULL, ul_info);

  if (is_nr_UL_slot(mac->scc, ul_info->slot_tx))
    nr_ue_prach_scheduler(module_id, ul_info->frame_tx, ul_info->slot_tx, ul_info->thread_id);

  switch(ret){
  case UE_CONNECTION_OK:
    break;
  case UE_CONNECTION_LOST:
    break;
  case UE_PHY_RESYNCH:
    break;
  case UE_PHY_HO_PRACH:
    break;
  default:
    break;
  }

  return 0;
}

int nr_ue_dl_indication(nr_downlink_indication_t *dl_info, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment){

  int32_t i;
  uint32_t ret_mask = 0x0;
  module_id_t module_id = dl_info->module_id;
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  fapi_nr_dl_config_request_t *dl_config = &mac->dl_config_request;

  if ((!dl_info->dci_ind && !dl_info->rx_ind) || !def_dci_pdu_rel15) { //Melissa review this with Raymond
    // UL indication to schedule DCI reception
    nr_ue_scheduler(dl_info, NULL);
  } else {
    // UL indication after reception of DCI or DL PDU
    if(dl_info->dci_ind != NULL){
      LOG_D(MAC,"[L2][IF MODULE][DL INDICATION][DCI_IND]\n");
      for(i=0; i<dl_info->dci_ind->number_of_dcis; ++i){
        LOG_D(MAC,">>>NR_IF_Module i=%d, dl_info->dci_ind->number_of_dcis=%d\n",i,dl_info->dci_ind->number_of_dcis);
        nr_scheduled_response_t scheduled_response;
        int8_t ret = handle_dci(dl_info->module_id,
                                dl_info->cc_id,
                                dl_info->gNB_index,
                                dl_info->frame,
                                dl_info->slot,
                                dl_info->dci_ind->dci_list+i);

        ret_mask |= (ret << FAPI_NR_DCI_IND);
        if (ret >= 0) {
          AssertFatal( nr_ue_if_module_inst[module_id] != NULL, "IF module is NULL!\n" );
          AssertFatal( nr_ue_if_module_inst[module_id]->scheduled_response != NULL, "scheduled_response is NULL!\n" );
          fill_scheduled_response(&scheduled_response, dl_config, NULL, NULL, dl_info->module_id, dl_info->cc_id, dl_info->frame, dl_info->slot, dl_info->thread_id);
          nr_ue_if_module_inst[module_id]->scheduled_response(&scheduled_response);
        }
      }
    }

    if(dl_info->rx_ind != NULL){

      for(i=0; i<dl_info->rx_ind->number_pdus; ++i){

        LOG_D(MAC, "In %s sending DL indication to MAC. 1 PDU type %s of %d total number of PDUs \n",
          __FUNCTION__,
          dl_indication_type[dl_info->rx_ind->rx_indication_body[i].pdu_type - 1],
          dl_info->rx_ind->number_pdus);

        switch(dl_info->rx_ind->rx_indication_body[i].pdu_type){

        case FAPI_NR_RX_PDU_TYPE_MIB:

          ret_mask |= (handle_bcch_bch(dl_info->module_id, dl_info->cc_id, dl_info->gNB_index,
                      (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.pdu,
                      (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.additional_bits,
                      (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.ssb_index,
                      (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.ssb_length,
                      (dl_info->rx_ind->rx_indication_body+i)->mib_pdu.cell_id)) << FAPI_NR_RX_PDU_TYPE_MIB;

          break;

        case FAPI_NR_RX_PDU_TYPE_SIB:

          ret_mask |= (handle_bcch_dlsch(dl_info->module_id,
                       dl_info->cc_id, dl_info->gNB_index,
                      (dl_info->rx_ind->rx_indication_body+i)->sib_pdu.sibs_mask,
                      (dl_info->rx_ind->rx_indication_body+i)->sib_pdu.pdu,
                      (dl_info->rx_ind->rx_indication_body+i)->sib_pdu.pdu_length)) << FAPI_NR_RX_PDU_TYPE_SIB;

          break;

        case FAPI_NR_RX_PDU_TYPE_DLSCH:

          ret_mask |= (handle_dlsch(dl_info, ul_time_alignment, i)) << FAPI_NR_RX_PDU_TYPE_DLSCH;

          break;

        case FAPI_NR_RX_PDU_TYPE_RAR:

          ret_mask |= (handle_dlsch(dl_info, ul_time_alignment, i)) << FAPI_NR_RX_PDU_TYPE_RAR;

          break;

        default:
          break;
        }
      }
    }

    //clean up nr_downlink_indication_t *dl_info
    dl_info->rx_ind = NULL;
    dl_info->dci_ind = NULL;

  }
  return 0;
}

nr_ue_if_module_t *nr_ue_if_module_init(uint32_t module_id){

  if (nr_ue_if_module_inst[module_id] == NULL) {
    nr_ue_if_module_inst[module_id] = (nr_ue_if_module_t *)malloc(sizeof(nr_ue_if_module_t));
    memset((void*)nr_ue_if_module_inst[module_id],0,sizeof(nr_ue_if_module_t));

    nr_ue_if_module_inst[module_id]->cc_mask=0;
    nr_ue_if_module_inst[module_id]->current_frame = 0;
    nr_ue_if_module_inst[module_id]->current_slot = 0;
    nr_ue_if_module_inst[module_id]->phy_config_request = nr_ue_phy_config_request;
    if (get_softmodem_params()->nsa) //Melissa, this is also a hack. Get a better flag.
      nr_ue_if_module_inst[module_id]->scheduled_response = nr_ue_scheduled_response_stub;
    else
      nr_ue_if_module_inst[module_id]->scheduled_response = nr_ue_scheduled_response;
    nr_ue_if_module_inst[module_id]->dl_indication = nr_ue_dl_indication;
    nr_ue_if_module_inst[module_id]->ul_indication = nr_ue_ul_indication;
  }

  return nr_ue_if_module_inst[module_id];
}

int nr_ue_if_module_kill(uint32_t module_id) {

  if (nr_ue_if_module_inst[module_id] != NULL){
    free(nr_ue_if_module_inst[module_id]);
  }
  return 0;
}

int nr_ue_dcireq(nr_dcireq_t *dcireq) {

  fapi_nr_dl_config_request_t *dl_config = &dcireq->dl_config_req;
  NR_UE_MAC_INST_t *UE_mac = get_mac_inst(0);
  dl_config->sfn = UE_mac->dl_config_request.sfn;
  dl_config->slot = UE_mac->dl_config_request.slot;

  LOG_D(PHY, "Entering UE DCI configuration frame %d slot %d \n", dcireq->frame, dcireq->slot);

  ue_dci_configuration(UE_mac, dl_config, dcireq->frame, dcireq->slot);

  return 0;
}
