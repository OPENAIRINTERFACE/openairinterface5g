/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include "dci_payload_utils.h"
#include "nr_fapi_p7.h"
#include "nr_fapi_p7_utils.h"

static void fil_srs_indication_report_tlv(nfapi_srs_report_tlv_t *tlv)
{
  tlv->tag = rand16_range(0, 3);
  tlv->length = rand32_range(1, sizeof(tlv->value));
  const uint16_t last_idx = ((tlv->length + 3) / 4) - 1;
  for (int i = 0; i < last_idx; ++i) {
    tlv->value[i] = rand32();
  }
  const uint8_t num_bytes = 4 - get_tlv_padding(tlv->length);
  tlv->value[last_idx] = num_bytes == 4 ? rand32() : rand32_range(0, 1 << (8 * num_bytes));
}

static void fill_srs_indication_PDU(nfapi_nr_srs_indication_pdu_t *pdu)
{
  pdu->handle = rand32();
  pdu->rnti = rand16_range(1, 65535);
  pdu->timing_advance_offset = rand16_range(0, 63);
  pdu->timing_advance_offset_nsec = rands16_range(-16800, 16800);
  pdu->srs_usage = rand8_range(0,3);
  pdu->report_type = rand8_range(0,1);
  fil_srs_indication_report_tlv(&pdu->report_tlv);
}

static void fill_srs_indication(nfapi_nr_srs_indication_t *msg)
{
  msg->sfn = rand16_range(0, 1023);
  msg->slot = rand16_range(0, 159);
  msg->control_length = rand16(); // being ignored number_of_pdusat the moment, report is always sent inline
  msg->number_of_pdus = rand8_range(1, NFAPI_NR_SRS_IND_MAX_PDU); // Minimum 1 PDUs in order to test at least one
  msg->pdu_list = calloc_or_fail(msg->number_of_pdus, sizeof(*msg->pdu_list));
  for (int pdu_idx = 0; pdu_idx < msg->number_of_pdus; ++pdu_idx) {
    fill_srs_indication_PDU(&msg->pdu_list[pdu_idx]);
  }
}

static void test_pack_unpack(nfapi_nr_srs_indication_t *req)
{
  size_t message_size = get_srs_indication_size(req);
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
  nfapi_nr_srs_indication_t unpacked_req = {0};
  int unpack_result =
      fapi_nr_p7_message_unpack(msg_buf, header.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), 0);
  DevAssert(unpack_result >= 0);
  DevAssert(eq_srs_indication(&unpacked_req, req));
  free_srs_indication(&unpacked_req);
  free(msg_buf);
}

static void test_copy(const nfapi_nr_srs_indication_t *msg)
{
  // Test copy function
  nfapi_nr_srs_indication_t copy = {0};
  copy_srs_indication(msg, &copy);
  DevAssert(eq_srs_indication(msg, &copy));
  free_srs_indication(&copy);
}

int main()
{
  fapi_test_init();

  nfapi_nr_srs_indication_t *req = calloc_or_fail(1, sizeof(nfapi_nr_srs_indication_t));
  req->header.message_id = NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION;
  // Get the actual allocated size
  printf("Allocated size before filling: %zu bytes\n", get_srs_indication_size(req));
  // Fill TX_DATA request
  fill_srs_indication(req);
  printf("Allocated size after filling: %zu bytes\n", get_srs_indication_size(req));
  // Perform tests
  test_pack_unpack(req);
  test_copy(req);
  // All tests successful!
  free_srs_indication(req);
  free(req);
  return 0;
}
