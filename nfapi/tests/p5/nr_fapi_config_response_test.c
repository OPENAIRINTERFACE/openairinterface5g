/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include "nfapi/tests/nr_fapi_test.h"
#include "nr_fapi_p5.h"
#include "nr_fapi_p5_utils.h"

void fill_config_response_tlv_list(nfapi_nr_generic_tlv_scf_t *list, uint8_t size)
{
  uint8_t const TLVtypes[] = {UINT_8, UINT_16, UINT_32, ARRAY_UINT_16};
  for (int i = 0; i < size; ++i) {
    nfapi_nr_generic_tlv_scf_t *element = &(list[i]);
    element->tl.tag = rand16_range(NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG, NFAPI_NR_CONFIG_RSSI_MEASUREMENT_TAG);
    element->tl.length = TLVtypes[rand8_range(0, sizeof(TLVtypes) - 1)];
    switch (element->tl.length) {
      case UINT_8:
        element->value.u8 = rand8();
        break;
      case UINT_16:
        element->value.u16 = rand16();
        break;
      case UINT_32:
        element->value.u32 = rand32();
        break;
      case ARRAY_UINT_16:
        for (int j = 0; j < 5; ++j) {
          element->value.array_u16[j] = rand16();
        }
        break;
      default:
        break;
    }
  }
}

void fill_config_response_tlv(nfapi_nr_config_response_scf_t *nfapi_resp)
{
  nfapi_resp->error_code = rand8();
  nfapi_resp->num_invalid_tlvs = rand8();
  nfapi_resp->num_invalid_tlvs_configured_in_idle = rand8();
  nfapi_resp->num_invalid_tlvs_configured_in_running = rand8();
  nfapi_resp->num_missing_tlvs = rand8();
  // Lists
  nfapi_resp->invalid_tlvs_list =
      (nfapi_nr_generic_tlv_scf_t *)malloc(nfapi_resp->num_invalid_tlvs * sizeof(nfapi_nr_generic_tlv_scf_t));
  nfapi_resp->invalid_tlvs_configured_in_idle_list =
      (nfapi_nr_generic_tlv_scf_t *)malloc(nfapi_resp->num_invalid_tlvs_configured_in_idle * sizeof(nfapi_nr_generic_tlv_scf_t));
  nfapi_resp->invalid_tlvs_configured_in_running_list =
      (nfapi_nr_generic_tlv_scf_t *)malloc(nfapi_resp->num_invalid_tlvs_configured_in_running * sizeof(nfapi_nr_generic_tlv_scf_t));
  nfapi_resp->missing_tlvs_list =
      (nfapi_nr_generic_tlv_scf_t *)malloc(nfapi_resp->num_missing_tlvs * sizeof(nfapi_nr_generic_tlv_scf_t));

  fill_config_response_tlv_list(nfapi_resp->invalid_tlvs_list, nfapi_resp->num_invalid_tlvs);
  fill_config_response_tlv_list(nfapi_resp->invalid_tlvs_configured_in_idle_list, nfapi_resp->num_invalid_tlvs_configured_in_idle);
  fill_config_response_tlv_list(nfapi_resp->invalid_tlvs_configured_in_running_list,
                                nfapi_resp->num_invalid_tlvs_configured_in_running);
  fill_config_response_tlv_list(nfapi_resp->missing_tlvs_list, nfapi_resp->num_missing_tlvs);
}

void test_pack_unpack(nfapi_nr_config_response_scf_t *req)
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
  nfapi_nr_config_response_scf_t unpacked_req = {0};
  int unpack_result =
      fapi_nr_p5_message_unpack(msg_buf, header.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), 0);
  DevAssert(unpack_result >= 0);
  DevAssert(eq_config_response(&unpacked_req, req));
  free_config_response(&unpacked_req);
}

void test_copy(const nfapi_nr_config_response_scf_t *msg)
{
  // Test copy function
  nfapi_nr_config_response_scf_t copy = {0};
  copy_config_response(msg, &copy);
  DevAssert(eq_config_response(msg, &copy));
  free_config_response(&copy);
}

int main()
{
  fapi_test_init();

  nfapi_nr_config_response_scf_t req = {.header.message_id = NFAPI_NR_PHY_MSG_TYPE_CONFIG_RESPONSE};
  // Fill Config response TVLs
  fill_config_response_tlv(&req);
  // Perform tests
  test_pack_unpack(&req);
  test_copy(&req);
  // All tests successful!
  free_config_response(&req);
  return 0;
}
