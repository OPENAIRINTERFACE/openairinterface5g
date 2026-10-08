/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include "dci_payload_utils.h"
#include "nr_fapi_p7.h"
#include "nr_fapi_p7_utils.h"

static void fill_uci_indication_sr_pdu_0_1(nfapi_nr_sr_pdu_0_1_t *pdu)
{
  pdu->sr_indication = rand8_range(0, 1);
  pdu->sr_confidence_level = rand8_range(0, 1);
}

static void fill_uci_indication_sr_pdu_2_3_4(nfapi_nr_sr_pdu_2_3_4_t *pdu)
{
  pdu->sr_bit_len = rand16_range(1, 8);
  pdu->sr_payload = calloc(nr_bits_to_bytes(pdu->sr_bit_len), sizeof(*pdu->sr_payload));
  for (int i = 0; i < nr_bits_to_bytes(pdu->sr_bit_len); ++i) {
    pdu->sr_payload[i] = rand8();
  }
}

static void fill_uci_indication_harq_pdu_0_1(nfapi_nr_harq_pdu_0_1_t *pdu)
{
  pdu->num_harq = rand8_range(1, 2);
  pdu->harq_confidence_level = rand8_range(0, 1);
  for (int i = 0; i < pdu->num_harq; ++i) {
    pdu->harq_list[i].harq_value = rand8_range(0, 2);
  }
}

static void fill_uci_indication_harq_pdu_2_3_4(nfapi_nr_harq_pdu_2_3_4_t *pdu)
{
  pdu->harq_crc = rand8_range(0, 2);
  pdu->harq_bit_len = rand16_range(1, 1706);
  pdu->harq_payload = calloc(nr_bits_to_bytes(pdu->harq_bit_len), sizeof(*pdu->harq_payload));
  for (int i = 0; i < nr_bits_to_bytes(pdu->harq_bit_len); ++i) {
    pdu->harq_payload[i] = rand8();
  }
}

static void fill_uci_indication_csi_part1(nfapi_nr_csi_part1_pdu_t *pdu)
{
  pdu->csi_part1_crc = rand8_range(0, 2);
  pdu->csi_part1_bit_len = rand16_range(1, 1706);
  pdu->csi_part1_payload = calloc(nr_bits_to_bytes(pdu->csi_part1_bit_len), sizeof(*pdu->csi_part1_payload));
  for (int i = 0; i < nr_bits_to_bytes(pdu->csi_part1_bit_len); ++i) {
    pdu->csi_part1_payload[i] = rand8();
  }
}

static void fill_uci_indication_csi_part2(nfapi_nr_csi_part2_pdu_t *pdu)
{
  pdu->csi_part2_crc = rand8_range(0, 2);
  pdu->csi_part2_bit_len = rand16_range(1, 1706);
  pdu->csi_part2_payload = calloc(nr_bits_to_bytes(pdu->csi_part2_bit_len), sizeof(*pdu->csi_part2_payload));
  for (int i = 0; i < nr_bits_to_bytes(pdu->csi_part2_bit_len); ++i) {
    pdu->csi_part2_payload[i] = rand8();
  }
}

static void fill_uci_indication_PUSCH(nfapi_nr_uci_pusch_pdu_t *pdu)
{
  pdu->pduBitmap = rand8_range(0, 0b1110) & 0b1110; // Bit 0 is always unused
  pdu->handle = rand32();
  pdu->rnti = rand16_range(1, 65535);
  pdu->ul_cqi = rand8();
  pdu->timing_advance = rand16_range(0, 63);
  pdu->rssi = rand16_range(0, 1280);

  // Bit 0 not used in PUSCH PDU
  // HARQ
  if ((pdu->pduBitmap >> 1) & 0x01) {
    fill_uci_indication_harq_pdu_2_3_4(&pdu->harq);
  }
  // CSI Part 1
  if ((pdu->pduBitmap >> 2) & 0x01) {
    fill_uci_indication_csi_part1(&pdu->csi_part1);
  }
  // CSI Part 2
  if ((pdu->pduBitmap >> 3) & 0x01) {
    fill_uci_indication_csi_part2(&pdu->csi_part2);
  }
}

static void fill_uci_indication_PUCCH_0_1(nfapi_nr_uci_pucch_pdu_format_0_1_t *pdu)
{
  pdu->pduBitmap = rand8_range(0, 0b11);
  pdu->handle = rand32();
  pdu->rnti = rand16_range(1, 65535);
  pdu->pucch_format = rand8_range(0, 1);
  pdu->ul_cqi = rand8();
  pdu->timing_advance = rand16_range(0, 63);
  pdu->rssi = rand16_range(0, 1280);

  // SR
  if (pdu->pduBitmap & 0x01) {
    fill_uci_indication_sr_pdu_0_1(&pdu->sr);
  }
  // HARQ
  if ((pdu->pduBitmap >> 1) & 0x01) {
    fill_uci_indication_harq_pdu_0_1(&pdu->harq);
  }
}

static void fill_uci_indication_PUCCH_2_3_4(nfapi_nr_uci_pucch_pdu_format_2_3_4_t *pdu)
{
  pdu->pduBitmap = rand8_range(0, 0b1111);
  pdu->handle = rand32();
  pdu->rnti = rand16_range(1, 65535);
  pdu->pucch_format = rand8_range(0, 2);
  pdu->ul_cqi = rand8();
  pdu->timing_advance = rand16_range(0, 63);
  pdu->rssi = rand16_range(0, 1280);
  // SR
  if (pdu->pduBitmap & 0x01) {
    fill_uci_indication_sr_pdu_2_3_4(&pdu->sr);
  }
  // HARQ
  if ((pdu->pduBitmap >> 1) & 0x01) {
    fill_uci_indication_harq_pdu_2_3_4(&pdu->harq);
  }
  // CSI Part 1
  if ((pdu->pduBitmap >> 2) & 0x01) {
    fill_uci_indication_csi_part1(&pdu->csi_part1);
  }
  // CSI Part 2
  if ((pdu->pduBitmap >> 3) & 0x01) {
    fill_uci_indication_csi_part2(&pdu->csi_part2);
  }
}

static void fill_uci_indication_UCI(nfapi_nr_uci_t *uci)
{
  uci->pdu_type = rand16_range(NFAPI_NR_UCI_PUSCH_PDU_TYPE, NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE);
  uci->pdu_size = rand16();
  switch (uci->pdu_type) {
    case NFAPI_NR_UCI_PUSCH_PDU_TYPE:
      fill_uci_indication_PUSCH(&uci->pusch_pdu);
      break;
    case NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE:
      fill_uci_indication_PUCCH_0_1(&uci->pucch_pdu_format_0_1);
      break;
    case NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE:
      fill_uci_indication_PUCCH_2_3_4(&uci->pucch_pdu_format_2_3_4);
      break;
    default:
      AssertFatal(1 == 0, "Unknown UCI.indication PDU Type %d\n", uci->pdu_type);
      break;
  }
}

static void fill_uci_indication(nfapi_nr_uci_indication_t *msg)
{
  msg->sfn = rand16_range(0, 1023);
  msg->slot = rand16_range(0, 159);
  msg->num_ucis = rand8_range(1, NFAPI_NR_UCI_IND_MAX_PDU); // Minimum 1 PDUs in order to test at least one
  msg->uci_list = calloc_or_fail(msg->num_ucis, sizeof(*msg->uci_list));
  for (int crc_idx = 0; crc_idx < msg->num_ucis; ++crc_idx) {
    fill_uci_indication_UCI(&msg->uci_list[crc_idx]);
  }
}

static void test_pack_unpack(nfapi_nr_uci_indication_t *req)
{
  size_t message_size = get_uci_indication_size(req);
  uint8_t *msg_buf = calloc_or_fail(message_size, sizeof(uint8_t));
  /*uint8_t msg_buf[1024*1024*3];
  size_t message_size = sizeof(msg_buf);*/
  // first test the packing procedure
  int pack_result = fapi_nr_p7_message_pack(req, msg_buf, message_size, NULL);

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
  nfapi_nr_uci_indication_t unpacked_req = {0};
  int unpack_result =
      fapi_nr_p7_message_unpack(msg_buf, header.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), 0);
  DevAssert(unpack_result >= 0);
  DevAssert(eq_uci_indication(&unpacked_req, req));
  free_uci_indication(&unpacked_req);
  free(msg_buf);
}

static void test_copy(const nfapi_nr_uci_indication_t *msg)
{
  // Test copy function
  nfapi_nr_uci_indication_t copy = {0};
  copy_uci_indication(msg, &copy);
  DevAssert(eq_uci_indication(msg, &copy));
  free_uci_indication(&copy);
}

int main()
{
  fapi_test_init();

  nfapi_nr_uci_indication_t *req = calloc_or_fail(1, sizeof(nfapi_nr_uci_indication_t));
  req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION;
  // Get the actual allocated size
  printf("Allocated size before filling: %zu bytes\n", get_uci_indication_size(req));
  // Fill TX_DATA request
  fill_uci_indication(req);
  printf("Allocated size after filling: %zu bytes\n", get_uci_indication_size(req));
  // Perform tests
  test_pack_unpack(req);
  test_copy(req);
  // All tests successful!
  free_uci_indication(req);
  free(req);
  return 0;
}
