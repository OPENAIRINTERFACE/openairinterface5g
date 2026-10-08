/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include "nfapi/tests/nr_fapi_test.h"
#include "nr_fapi_p5.h"
#include "nr_fapi_p5_utils.h"

void fill_param_response_tlv(nfapi_nr_param_response_scf_t *nfapi_resp)
{
  nfapi_resp->error_code = rand8_range(NFAPI_MSG_OK, NFAPI_MSG_INVALID_STATE);

  FILL_TLV(nfapi_resp->cell_param.release_capability, NFAPI_NR_PARAM_TLV_RELEASE_CAPABILITY_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->cell_param.phy_state, NFAPI_NR_PARAM_TLV_PHY_STATE_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->cell_param.skip_blank_dl_config, NFAPI_NR_PARAM_TLV_SKIP_BLANK_DL_CONFIG_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->cell_param.skip_blank_ul_config, NFAPI_NR_PARAM_TLV_SKIP_BLANK_UL_CONFIG_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->cell_param.num_config_tlvs_to_report,
           NFAPI_NR_PARAM_TLV_NUM_CONFIG_TLVS_TO_REPORT_TAG,
           rand16_range(0, NFAPI_NR_CONFIG_RSSI_MEASUREMENT_TAG - NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG));
  nfapi_resp->num_tlv++;

  nfapi_resp->cell_param.config_tlvs_to_report_list =
      calloc(nfapi_resp->cell_param.num_config_tlvs_to_report.value, sizeof(nfapi_uint8_tlv_t *));
  for (int i = 0; i < nfapi_resp->cell_param.num_config_tlvs_to_report.value; ++i) {
    FILL_TLV(nfapi_resp->cell_param.config_tlvs_to_report_list[i],
             rand16_range(NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG, NFAPI_NR_CONFIG_RSSI_MEASUREMENT_TAG),
             rand8_range(0, 5));
    nfapi_resp->cell_param.config_tlvs_to_report_list[i].tl.length = 1;
  }

  FILL_TLV(nfapi_resp->carrier_param.cyclic_prefix, NFAPI_NR_PARAM_TLV_CYCLIC_PREFIX_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->carrier_param.supported_subcarrier_spacings_dl,
           NFAPI_NR_PARAM_TLV_SUPPORTED_SUBCARRIER_SPACINGS_DL_TAG,
           rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->carrier_param.supported_bandwidth_dl, NFAPI_NR_PARAM_TLV_SUPPORTED_BANDWIDTH_DL_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->carrier_param.supported_subcarrier_spacings_ul,
           NFAPI_NR_PARAM_TLV_SUPPORTED_SUBCARRIER_SPACINGS_UL_TAG,
           rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->carrier_param.supported_bandwidth_ul, NFAPI_NR_PARAM_TLV_SUPPORTED_BANDWIDTH_UL_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdcch_param.cce_mapping_type, NFAPI_NR_PARAM_TLV_CCE_MAPPING_TYPE_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot,
           NFAPI_NR_PARAM_TLV_CORESET_OUTSIDE_FIRST_3_OFDM_SYMS_OF_SLOT_TAG,
           rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdcch_param.coreset_precoder_granularity_coreset,
           NFAPI_NR_PARAM_TLV_PRECODER_GRANULARITY_CORESET_TAG,
           rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdcch_param.pdcch_mu_mimo, NFAPI_NR_PARAM_TLV_PDCCH_MU_MIMO_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdcch_param.pdcch_precoder_cycling, NFAPI_NR_PARAM_TLV_PDCCH_PRECODER_CYCLING_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdcch_param.max_pdcch_per_slot, NFAPI_NR_PARAM_TLV_MAX_PDCCHS_PER_SLOT_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pucch_param.pucch_formats, NFAPI_NR_PARAM_TLV_PUCCH_FORMATS_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pucch_param.max_pucchs_per_slot, NFAPI_NR_PARAM_TLV_MAX_PUCCHS_PER_SLOT_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.pdsch_mapping_type, NFAPI_NR_PARAM_TLV_PDSCH_MAPPING_TYPE_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.pdsch_allocation_types, NFAPI_NR_PARAM_TLV_PDSCH_ALLOCATION_TYPES_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.pdsch_vrb_to_prb_mapping, NFAPI_NR_PARAM_TLV_PDSCH_VRB_TO_PRB_MAPPING_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.pdsch_cbg, NFAPI_NR_PARAM_TLV_PDSCH_CBG_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.pdsch_dmrs_config_types, NFAPI_NR_PARAM_TLV_PDSCH_DMRS_CONFIG_TYPES_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.pdsch_dmrs_max_length, NFAPI_NR_PARAM_TLV_PDSCH_DMRS_MAX_LENGTH_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.pdsch_dmrs_additional_pos, NFAPI_NR_PARAM_TLV_PDSCH_DMRS_ADDITIONAL_POS_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.max_pdsch_tbs_per_slot, NFAPI_NR_PARAM_TLV_MAX_PDSCH_S_TBS_PER_SLOT_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.max_number_mimo_layers_pdsch, NFAPI_NR_PARAM_TLV_MAX_NUMBER_MIMO_LAYERS_PDSCH_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.supported_max_modulation_order_dl,
           NFAPI_NR_PARAM_TLV_SUPPORTED_MAX_MODULATION_ORDER_DL_TAG,
           rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.max_mu_mimo_users_dl, NFAPI_NR_PARAM_TLV_MAX_MU_MIMO_USERS_DL_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.pdsch_data_in_dmrs_symbols, NFAPI_NR_PARAM_TLV_PDSCH_DATA_IN_DMRS_SYMBOLS_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.premption_support, NFAPI_NR_PARAM_TLV_PREMPTION_SUPPORT_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pdsch_param.pdsch_non_slot_support, NFAPI_NR_PARAM_TLV_PDSCH_NON_SLOT_SUPPORT_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.uci_mux_ulsch_in_pusch, NFAPI_NR_PARAM_TLV_UCI_MUX_ULSCH_IN_PUSCH_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.uci_only_pusch, NFAPI_NR_PARAM_TLV_UCI_ONLY_PUSCH_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.pusch_frequency_hopping, NFAPI_NR_PARAM_TLV_PUSCH_FREQUENCY_HOPPING_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.pusch_dmrs_config_types, NFAPI_NR_PARAM_TLV_PUSCH_DMRS_CONFIG_TYPES_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.pusch_dmrs_max_len, NFAPI_NR_PARAM_TLV_PUSCH_DMRS_MAX_LEN_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.pusch_dmrs_additional_pos, NFAPI_NR_PARAM_TLV_PUSCH_DMRS_ADDITIONAL_POS_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.pusch_cbg, NFAPI_NR_PARAM_TLV_PUSCH_CBG_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.pusch_mapping_type, NFAPI_NR_PARAM_TLV_PUSCH_MAPPING_TYPE_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.pusch_allocation_types, NFAPI_NR_PARAM_TLV_PUSCH_ALLOCATION_TYPES_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.pusch_vrb_to_prb_mapping, NFAPI_NR_PARAM_TLV_PUSCH_VRB_TO_PRB_MAPPING_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.pusch_max_ptrs_ports, NFAPI_NR_PARAM_TLV_PUSCH_MAX_PTRS_PORTS_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.max_pduschs_tbs_per_slot, NFAPI_NR_PARAM_TLV_MAX_PDUSCHS_TBS_PER_SLOT_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.max_number_mimo_layers_non_cb_pusch,
           NFAPI_NR_PARAM_TLV_MAX_NUMBER_MIMO_LAYERS_NON_CB_PUSCH_TAG,
           rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.supported_modulation_order_ul, NFAPI_NR_PARAM_TLV_SUPPORTED_MODULATION_ORDER_UL_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.max_mu_mimo_users_ul, NFAPI_NR_PARAM_TLV_MAX_MU_MIMO_USERS_UL_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.dfts_ofdm_support, NFAPI_NR_PARAM_TLV_DFTS_OFDM_SUPPORT_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->pusch_param.pusch_aggregation_factor, NFAPI_NR_PARAM_TLV_PUSCH_AGGREGATION_FACTOR_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->prach_param.prach_long_formats, NFAPI_NR_PARAM_TLV_PRACH_LONG_FORMATS_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->prach_param.prach_short_formats, NFAPI_NR_PARAM_TLV_PRACH_SHORT_FORMATS_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->prach_param.prach_restricted_sets, NFAPI_NR_PARAM_TLV_PRACH_RESTRICTED_SETS_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->prach_param.max_prach_fd_occasions_in_a_slot,
           NFAPI_NR_PARAM_TLV_MAX_PRACH_FD_OCCASIONS_IN_A_SLOT_TAG,
           rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->measurement_param.rssi_measurement_support, NFAPI_NR_PARAM_TLV_RSSI_MEASUREMENT_SUPPORT_TAG, rand8());
  nfapi_resp->num_tlv++;
}

void test_pack_unpack(nfapi_nr_param_response_scf_t *req)
{
  uint8_t msg_buf[65535];
  uint16_t msg_len = sizeof(*req);
  // first test the packing procedure
  int pack_result = fapi_nr_p5_message_pack(req, msg_len, msg_buf, sizeof(msg_buf), NULL);
  // PARAM.response message body length is AT LEAST 10 (NFAPI_HEADER_LENGTH + 1 byte error_code + 1 byte num_tlv)
  DevAssert(pack_result >= NFAPI_HEADER_LENGTH + 1 + 1);
  // test the unpacking of the header
  // copy first NFAPI_HEADER_LENGTH bytes into a new buffer, to simulate SCTP PEEK
  nfapi_nr_p4_p5_message_header_t header;
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
  // test the unpaking and compare with initial message
  nfapi_nr_param_response_scf_t unpacked_req = {0};
  int unpack_result =
      fapi_nr_p5_message_unpack(msg_buf, header.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), NULL);
  DevAssert(unpack_result >= 0);
  DevAssert(eq_param_response(&unpacked_req, req));
  free_param_response(&unpacked_req);
}

void test_copy(const nfapi_nr_param_response_scf_t *msg)
{
  // Test copy function
  nfapi_nr_param_response_scf_t copy = {0};
  copy_param_response(msg, &copy);
  DevAssert(eq_param_response(msg, &copy));
  free_param_response(&copy);
}

int main()
{
  fapi_test_init();
  nfapi_nr_param_response_scf_t req = {.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE};
  // Fill Param response TVLs
  fill_param_response_tlv(&req);
  // Perform tests
  test_pack_unpack(&req);
  test_copy(&req);
  // All tests successful!
  free_param_response(&req);
  return 0;
}
