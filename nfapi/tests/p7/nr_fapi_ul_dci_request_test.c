/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include "dci_payload_utils.h"
#include "nr_fapi_p7.h"
#include "nr_fapi_p7_utils.h"

static void fill_beamforming(nfapi_nr_tx_precoding_and_beamforming_t *precodingAndBeamforming)
{
  precodingAndBeamforming->num_prgs = rand16_range(1, NFAPI_MAX_NUM_PRGS);
  precodingAndBeamforming->prg_size = rand16_range(1, 275);
  precodingAndBeamforming->dig_bf_interfaces = rand8_range(0,NFAPI_MAX_NUM_BG_IF);
  for (int prg = 0; prg < precodingAndBeamforming->num_prgs; ++prg) {
    precodingAndBeamforming->prgs_list[prg].pm_idx = rand16();
    for (int dbf_if = 0; dbf_if < precodingAndBeamforming->dig_bf_interfaces; ++dbf_if) {
      precodingAndBeamforming->prgs_list[prg].dig_bf_interface_list[dbf_if].beam_idx = rand16();
    }
  }
}

static void fill_ul_dci_request_pdcch_pdu(nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdu)
{
  pdu->BWPSize = rand16();
  pdu->BWPStart = rand16();
  pdu->SubcarrierSpacing = rand8();
  pdu->CyclicPrefix = rand8();
  pdu->StartSymbolIndex = rand8();
  pdu->DurationSymbols = rand8();
  for (int fdr_idx = 0; fdr_idx < 6; ++fdr_idx) {
    pdu->FreqDomainResource[fdr_idx] = rand8();
  }
  pdu->CceRegMappingType = rand8();
  pdu->RegBundleSize = rand8();
  pdu->InterleaverSize = rand8();
  pdu->CoreSetType = rand8();
  pdu->ShiftIndex = rand8();
  pdu->precoderGranularity = rand8();
  pdu->numDlDci = rand8_range(1, MAX_DCI_CORESET);
  for (int dl_dci = 0; dl_dci < pdu->numDlDci; ++dl_dci) {
    pdu->dci_pdu[dl_dci].RNTI = rand16();
    pdu->dci_pdu[dl_dci].ScramblingId = rand16();
    pdu->dci_pdu[dl_dci].ScramblingRNTI = rand16();
    pdu->dci_pdu[dl_dci].CceIndex = rand8();
    pdu->dci_pdu[dl_dci].AggregationLevel = rand8();
    fill_beamforming(&pdu->dci_pdu[dl_dci].precodingAndBeamforming);
    pdu->dci_pdu[dl_dci].beta_PDCCH_1_0 = rand8();
    pdu->dci_pdu[dl_dci].powerControlOffsetSS = rand8();
    pdu->dci_pdu[dl_dci].PayloadSizeBits = rand16_range(0, DCI_PAYLOAD_BYTE_LEN * 8);
    generate_payload(pdu->dci_pdu[dl_dci].PayloadSizeBits, pdu->dci_pdu[dl_dci].Payload);
  }
}

static void fill_ul_dci_request_pdu(nfapi_nr_ul_dci_request_pdus_t *pdu)
{
  pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
  pdu->PDUSize = rand16();
  fill_ul_dci_request_pdcch_pdu(&pdu->pdcch_pdu.pdcch_pdu_rel15);
}

static void fill_ul_dci_request(nfapi_nr_ul_dci_request_t *msg)
{
  msg->SFN = rand16_range(0, 1023);
  msg->Slot = rand16_range(0, 159);
  msg->numPdus = rand8_range(1, 10); // Minimum 1 PDUs in order to test at least one
  printf(" NUM PDUS  %d\n", msg->numPdus);
  for (int pdu_idx = 0; pdu_idx < msg->numPdus; ++pdu_idx) {
    fill_ul_dci_request_pdu(&msg->ul_dci_pdu_list[pdu_idx]);
  }
}

static void test_pack_unpack(nfapi_nr_ul_dci_request_t *req)
{
  uint8_t msg_buf[1024 * 1024 * 2];
  // first test the packing procedure
  int pack_result = fapi_nr_p7_message_pack(req, msg_buf, sizeof(msg_buf), NULL);

  DevAssert(pack_result >= 0 + NFAPI_HEADER_LENGTH);
  // test the unpacking of the header
  // copy first NFAPI_HEADER_LENGTH bytes into a new buffer, to simulate SCTP PEEK
  fapi_message_header_t header;
  uint32_t header_buffer_size = NFAPI_HEADER_LENGTH;
  uint8_t header_buffer[header_buffer_size];
  for (int idx = 0; idx < header_buffer_size; idx++) {
    header_buffer[idx] = msg_buf[idx];
  }
  uint8_t *pReadPackedMessage = header_buffer;
  int unpack_header_result = fapi_nr_p7_message_header_unpack(pReadPackedMessage, NFAPI_HEADER_LENGTH, &header, sizeof(header), 0);
  DevAssert(unpack_header_result >= 0);
  DevAssert(header.message_id == req->header.message_id);
  DevAssert(header.message_length == req->header.message_length);
  // test the unpacking and compare with initial message
  nfapi_nr_ul_dci_request_t unpacked_req = {0};
  int unpack_result =
      fapi_nr_p7_message_unpack(msg_buf, header.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), 0);
  DevAssert(unpack_result >= 0);
  DevAssert(eq_ul_dci_request(&unpacked_req, req));
  free_ul_dci_request(&unpacked_req);
}

static void test_copy(const nfapi_nr_ul_dci_request_t *msg)
{
  // Test copy function
  nfapi_nr_ul_dci_request_t copy = {0};
  copy_ul_dci_request(msg, &copy);
  DevAssert(eq_ul_dci_request(msg, &copy));
  free_ul_dci_request(&copy);
}

int main()
{
  fapi_test_init();

  nfapi_nr_ul_dci_request_t req = {.header.message_id = NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST};

  // Fill UL_DCI request
  fill_ul_dci_request(&req);
  // Perform tests
  test_pack_unpack(&req);
  test_copy(&req);
  // All tests successful!
  free_ul_dci_request(&req);
  return 0;
}
