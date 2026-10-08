/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include "nfapi/tests/nr_fapi_test.h"
#include "nr_fapi_p5.h"
#include "nr_fapi_p5_utils.h"

void test_pack_unpack(nfapi_nr_error_indication_scf_t *req)
{
  uint8_t msg_buf[65535];
  uint16_t msg_len = sizeof(*req);
  // first test the packing procedure
  int pack_result = fapi_nr_p5_message_pack(req, msg_len, msg_buf, sizeof(msg_buf), NULL);
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
  int unpack_header_result = fapi_nr_message_header_unpack(pReadPackedMessage, NFAPI_HEADER_LENGTH, &header, sizeof(header), 0);
  DevAssert(unpack_header_result >= 0);
  DevAssert(header.message_id == req->header.message_id);
  DevAssert(header.message_length == req->header.message_length);
  // test the unpacking and compare with initial message
  nfapi_nr_error_indication_scf_t unpacked_req = {0};
  int unpack_result =
      fapi_nr_p5_message_unpack(msg_buf, header.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), NULL);
  DevAssert(unpack_result >= 0);
  DevAssert(eq_error_indication(&unpacked_req, req));
  free_error_indication(&unpacked_req);
}

void test_copy(const nfapi_nr_error_indication_scf_t *msg)
{
  // Test copy function
  nfapi_nr_error_indication_scf_t copy = {0};
  copy_error_indication(msg, &copy);
  DevAssert(eq_error_indication(msg, &copy));
  free_error_indication(&copy);
}

int main()
{
  fapi_test_init();

  nfapi_nr_error_indication_scf_t req = {.header.message_id = NFAPI_NR_PHY_MSG_TYPE_ERROR_INDICATION};
  // Fill the message with randomized data
  req.sfn = rand16_range(0, 1023);
  req.slot = rand16_range(0, 159);
  req.message_id = rand8_range(0x00, 0xFF);
  req.error_code = rand8_range(0x00, 0x08);
  // Perform tests
  test_pack_unpack(&req);
  test_copy(&req);
  // All tests successful!
  free_error_indication(&req);
  return 0;
}
