/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include "nfapi/tests/nr_fapi_test.h"
#include "nr_fapi_p7.h"
#include "nr_fapi_p7_utils.h"

// The phy_id is carried on the wire in the opaque handle byte of the FAPI
// message header (SCF 222 §3.3.1.1), so only values 0..255 survive the round trip.
static void test_phy_id_roundtrip(uint16_t phy_id)
{
  nfapi_nr_slot_indication_scf_t req = {.header.message_id = NFAPI_NR_PHY_MSG_TYPE_SLOT_INDICATION, .header.phy_id = phy_id};
  req.sfn = rand16_range(0, 1023);
  req.slot = rand16_range(0, 159);
  uint8_t msg_buf[1024];
  int pack_result = fapi_nr_p7_message_pack(&req, msg_buf, sizeof(msg_buf), NULL);
  // Should always return 4 (2 bytes sfn + 2 bytes slot)
  DevAssert(pack_result == 4 + NFAPI_HEADER_LENGTH);
  // the opaque handle is the second byte of the packed header
  DevAssert(msg_buf[1] == phy_id);

  // test the header-peek unpack
  nfapi_nr_p7_message_header_t peek_hdr = {0};
  int unpack_header_result = fapi_nr_message_header_unpack(msg_buf, NFAPI_HEADER_LENGTH, &peek_hdr, sizeof(peek_hdr), 0);
  DevAssert(unpack_header_result >= 0);
  DevAssert(peek_hdr.phy_id == phy_id);
  DevAssert(peek_hdr.message_id == req.header.message_id);

  // test the full message unpack
  nfapi_nr_slot_indication_scf_t unpacked_req = {0};
  int unpack_result =
      fapi_nr_p7_message_unpack(msg_buf, peek_hdr.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), 0);
  DevAssert(unpack_result >= 0);
  DevAssert(unpacked_req.header.phy_id == phy_id);
  DevAssert(eq_slot_indication(&unpacked_req, &req));
  free_slot_indication(&unpacked_req);
  free_slot_indication(&req);
}

int main()
{
  fapi_test_init();

  for (uint16_t phy_id = 0; phy_id <= UINT8_MAX; phy_id++) {
    test_phy_id_roundtrip(phy_id);
  }
  // All tests successful!
  return 0;
}
