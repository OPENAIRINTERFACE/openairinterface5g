/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include "nfapi/tests/nr_fapi_test.h"
#include "nr_fapi_p5.h"
#include "nr_fapi_p5_utils.h"

static void fill_config_request_tlv_tdd_rand(nfapi_nr_config_request_scf_t *nfapi_resp)
{
  FILL_TLV(nfapi_resp->carrier_config.dl_bandwidth, NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->carrier_config.dl_frequency, NFAPI_NR_CONFIG_DL_FREQUENCY_TAG, rand32());
  nfapi_resp->num_tlv++;

  for (int i = 0; i < 5; ++i) {
    FILL_TLV(nfapi_resp->carrier_config.dl_k0[i], NFAPI_NR_CONFIG_DL_K0_TAG, rand16());
  }
  // these 5 are 1 tlv
  nfapi_resp->num_tlv++;
  for (int i = 0; i < 5; ++i) {
    FILL_TLV(nfapi_resp->carrier_config.dl_grid_size[i], NFAPI_NR_CONFIG_DL_GRID_SIZE_TAG, rand16());
  }
  // these 5 are 1 tlv
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->carrier_config.num_tx_ant, NFAPI_NR_CONFIG_NUM_TX_ANT_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->carrier_config.uplink_bandwidth, NFAPI_NR_CONFIG_UPLINK_BANDWIDTH_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->carrier_config.uplink_frequency, NFAPI_NR_CONFIG_UPLINK_FREQUENCY_TAG, rand16());
  nfapi_resp->num_tlv++;

  for (int i = 0; i < 5; ++i) {
    FILL_TLV(nfapi_resp->carrier_config.ul_k0[i], NFAPI_NR_CONFIG_UL_K0_TAG, rand16());
  }
  // these 5 are 1 tlv
  nfapi_resp->num_tlv++;
  for (int i = 0; i < 5; ++i) {
    FILL_TLV(nfapi_resp->carrier_config.ul_grid_size[i], NFAPI_NR_CONFIG_UL_GRID_SIZE_TAG, rand16());
  }
  // these 5 are 1 tlv
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->carrier_config.num_rx_ant, NFAPI_NR_CONFIG_NUM_RX_ANT_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->carrier_config.frequency_shift_7p5khz, NFAPI_NR_CONFIG_FREQUENCY_SHIFT_7P5KHZ_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->cell_config.phy_cell_id, NFAPI_NR_CONFIG_PHY_CELL_ID_TAG, rand16());
  nfapi_resp->num_tlv++;

  // TDD because below we pack the TDD table
  FILL_TLV(nfapi_resp->cell_config.frame_duplex_type, NFAPI_NR_CONFIG_FRAME_DUPLEX_TYPE_TAG, 1 /* TDD */);
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->ssb_config.ss_pbch_power, NFAPI_NR_CONFIG_SS_PBCH_POWER_TAG, (int32_t)rand32());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->ssb_config.bch_payload, NFAPI_NR_CONFIG_BCH_PAYLOAD_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->ssb_config.scs_common, NFAPI_NR_CONFIG_SCS_COMMON_TAG, 1 /* 30 kHz */);
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->prach_config.prach_sequence_length, NFAPI_NR_CONFIG_PRACH_SEQUENCE_LENGTH_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->prach_config.prach_sub_c_spacing, NFAPI_NR_CONFIG_PRACH_SUB_C_SPACING_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->prach_config.restricted_set_config, NFAPI_NR_CONFIG_RESTRICTED_SET_CONFIG_TAG, rand8());
  nfapi_resp->num_tlv++;
  /*Number of RACH frequency domain
    occasions. Corresponds to the
    parameter 𝑀𝑀 in [38.211, sec 6.3.3.2]
    which equals the higher layer
    parameter msg1FDM
    Value: 1,2,4,8*/
  const uint8_t num_prach_fd_ocasions[] = {1, 2, 4, 8};
  FILL_TLV(nfapi_resp->prach_config.num_prach_fd_occasions,
           NFAPI_NR_CONFIG_NUM_PRACH_FD_OCCASIONS_TAG,
           num_prach_fd_ocasions[rand8_range(0, sizeof(num_prach_fd_ocasions) - 1)]);
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->prach_config.prach_ConfigurationIndex, NFAPI_NR_CONFIG_PRACH_CONFIG_INDEX_TAG, rand8());
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_config.num_prach_fd_occasions_list = (nfapi_nr_num_prach_fd_occasions_t *)malloc(
      nfapi_resp->prach_config.num_prach_fd_occasions.value * sizeof(nfapi_nr_num_prach_fd_occasions_t));
  for (int i = 0; i < nfapi_resp->prach_config.num_prach_fd_occasions.value; i++) {
    nfapi_nr_num_prach_fd_occasions_t *prach_fd_occasion = &(nfapi_resp->prach_config.num_prach_fd_occasions_list[i]);

    FILL_TLV(prach_fd_occasion->prach_root_sequence_index, NFAPI_NR_CONFIG_PRACH_ROOT_SEQUENCE_INDEX_TAG, rand16());
    nfapi_resp->num_tlv++;

    FILL_TLV(prach_fd_occasion->num_root_sequences, NFAPI_NR_CONFIG_NUM_ROOT_SEQUENCES_TAG, rand8());
    nfapi_resp->num_tlv++;

    FILL_TLV(prach_fd_occasion->k1, NFAPI_NR_CONFIG_K1_TAG, rand8());
    nfapi_resp->num_tlv++;

    FILL_TLV(prach_fd_occasion->prach_zero_corr_conf, NFAPI_NR_CONFIG_PRACH_ZERO_CORR_CONF_TAG, rand8());
    nfapi_resp->num_tlv++;

    // This doesn't make sense to be more than the total num_root_sequences
    /* SCF 222.10.02 : Number of unused sequences available
       for noise estimation per FD occasion. At
       least one unused root sequence is
       required per FD occasion.  */
    if (prach_fd_occasion->num_root_sequences.value != 0) {
      FILL_TLV(prach_fd_occasion->num_unused_root_sequences,
               NFAPI_NR_CONFIG_NUM_UNUSED_ROOT_SEQUENCES_TAG,
               rand8_range(1, prach_fd_occasion->num_root_sequences.value));
    } else {
      FILL_TLV(prach_fd_occasion->num_unused_root_sequences, NFAPI_NR_CONFIG_NUM_UNUSED_ROOT_SEQUENCES_TAG, 1);
    }
    nfapi_resp->num_tlv++;

    prach_fd_occasion->unused_root_sequences_list =
        (nfapi_uint8_tlv_t *)malloc(prach_fd_occasion->num_unused_root_sequences.value * sizeof(nfapi_uint8_tlv_t));
    for (int k = 0; k < prach_fd_occasion->num_unused_root_sequences.value; k++) {
      FILL_TLV(prach_fd_occasion->unused_root_sequences_list[k], NFAPI_NR_CONFIG_UNUSED_ROOT_SEQUENCES_TAG, rand16());
      nfapi_resp->num_tlv++;
    }
  }

  FILL_TLV(nfapi_resp->prach_config.ssb_per_rach, NFAPI_NR_CONFIG_SSB_PER_RACH_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->prach_config.prach_multiple_carriers_in_a_band,
           NFAPI_NR_CONFIG_PRACH_MULTIPLE_CARRIERS_IN_A_BAND_TAG,
           rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->ssb_table.ssb_offset_point_a, NFAPI_NR_CONFIG_SSB_OFFSET_POINT_A_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->ssb_table.beta_pss, NFAPI_NR_CONFIG_BETA_PSS_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->ssb_table.ssb_period, NFAPI_NR_CONFIG_SSB_PERIOD_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->ssb_table.ssb_subcarrier_offset, NFAPI_NR_CONFIG_SSB_SUBCARRIER_OFFSET_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->ssb_table.MIB, NFAPI_NR_CONFIG_MIB_TAG, rand32());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->ssb_table.ssb_mask_list[0].ssb_mask, NFAPI_NR_CONFIG_SSB_MASK_TAG, rand32());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->ssb_table.ssb_mask_list[1].ssb_mask, NFAPI_NR_CONFIG_SSB_MASK_TAG, rand32());
  nfapi_resp->num_tlv++;

  for (int i = 0; i < 64; i++) {
    FILL_TLV(nfapi_resp->ssb_table.ssb_beam_id_list[i].beam_id, NFAPI_NR_CONFIG_BEAM_ID_TAG, rand8());
    nfapi_resp->num_tlv++;
  }

  FILL_TLV(nfapi_resp->ssb_table.case_v3, NFAPI_NR_FAPI_SSB_CASE_VENDOR_EXTENSION_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->tdd_table.tdd_period, NFAPI_NR_CONFIG_TDD_PERIOD_TAG, 6 /* ms5 */);
  nfapi_resp->num_tlv++;

#define SET_SYMBOL_TYPE(TDD_PERIOD_LIST, START, END, TYPE) \
  for (int SyM = (START); SyM < (END); SyM++) { \
    FILL_TLV((TDD_PERIOD_LIST)[SyM].slot_config, NFAPI_NR_CONFIG_SLOT_CONFIG_TAG, (TYPE)); \
  }

  int slots = 20;
  int sym = 14;
  nfapi_resp->tdd_table.max_tdd_periodicity_list = calloc(slots, sizeof(*nfapi_resp->tdd_table.max_tdd_periodicity_list));
  for (int i = 0; i < slots; i++) {
    nfapi_nr_max_tdd_periodicity_t* list = &nfapi_resp->tdd_table.max_tdd_periodicity_list[i];
    list->max_num_of_symbol_per_slot_list = calloc(sym, sizeof(*list->max_num_of_symbol_per_slot_list));
    // DDDDDDMUUU
    if ((i >= 0 && i <= 5) || (i >= 10 && i <= 15)) {
      SET_SYMBOL_TYPE(list->max_num_of_symbol_per_slot_list, 0, sym, 0 /* DL */);
    } else if (i == 6 || i == 16) {
      SET_SYMBOL_TYPE(list->max_num_of_symbol_per_slot_list, 0, 4, 0 /* DL */);
      SET_SYMBOL_TYPE(list->max_num_of_symbol_per_slot_list, 4, 10, 2 /* guard */);
      SET_SYMBOL_TYPE(list->max_num_of_symbol_per_slot_list, 10, sym, 1 /* UL */);
    } else {
      SET_SYMBOL_TYPE(list->max_num_of_symbol_per_slot_list, 0, sym, 1 /* UL */);
    }
    nfapi_resp->num_tlv += sym;
  }

  FILL_TLV(nfapi_resp->measurement_config.rssi_measurement, NFAPI_NR_CONFIG_RSSI_MEASUREMENT_TAG, rand8());
  nfapi_resp->num_tlv++;

  uint16_t nb_beams = rand16_range(1, 15);
  uint16_t nb_tx = rand16_range(1, 15);
  nfapi_resp->dbt_config.num_dig_beams = nb_beams;
  if (nfapi_resp->dbt_config.num_dig_beams > 0) {
    nfapi_resp->dbt_config.num_txrus = nb_tx;
    nfapi_resp->dbt_config.dig_beam_list = calloc(nb_beams , sizeof(*nfapi_resp->dbt_config.dig_beam_list));
    for (int i = 0; i < nb_beams; i++) {
      nfapi_nr_dig_beam_t *beam = &nfapi_resp->dbt_config.dig_beam_list[i];
      beam->beam_idx = i;
      beam->txru_list = calloc(nb_tx , sizeof(*beam->txru_list));
      for (int j = 0; j < nb_tx; j++) {
        nfapi_nr_txru_t *txru = &beam->txru_list[j];
        txru->dig_beam_weight_Re = rand16_range(1, 0xffff);
        txru->dig_beam_weight_Im = rand16_range(1, 0xffff);
      }
    }
    nfapi_resp->num_tlv++;
  }

  nfapi_nr_pm_list_t *pm_list = &nfapi_resp->pmi_list;
  const uint16_t num_layers = rand16_range(1, 4);
  const uint16_t num_ant_ports = rand16_range(1, 4);
  pm_list->num_pm_idx = rand16_range(1, 4);
  if (nfapi_resp->pmi_list.num_pm_idx > 0) {
    pm_list->pmi_pdu = calloc(pm_list->num_pm_idx, sizeof(*pm_list->pmi_pdu));
    for (int i = 0; i < pm_list->num_pm_idx; i++) {
      nfapi_nr_pm_pdu_t *pmi_pdu = &pm_list->pmi_pdu[i];
      pmi_pdu->pm_idx = i+1;
      pmi_pdu->numLayers = num_layers;
      pmi_pdu->num_ant_ports = num_ant_ports;
      for (int j = 0; j < num_layers; j++) {
        for (int k = 0; k < num_ant_ports; k++) {
          nfapi_nr_pm_weights_t* pm_weight = &pmi_pdu->weights[j][k];
          pm_weight->r = (int16_t)rand16_range(1, 0xffff);
          pm_weight->i = (int16_t)rand16_range(1, 0xffff);
        }
      }
    }
    nfapi_resp->num_tlv++;
  }

  nfapi_resp->nfapi_config.p7_vnf_address_ipv4.tl.tag = NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
  for (int i = 0; i < NFAPI_IPV4_ADDRESS_LENGTH; ++i) {
    nfapi_resp->nfapi_config.p7_vnf_address_ipv4.address[i] = rand8();
  }
  nfapi_resp->num_tlv++;

  nfapi_resp->nfapi_config.p7_vnf_address_ipv6.tl.tag = NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV6_TAG;
  for (int i = 0; i < NFAPI_IPV6_ADDRESS_LENGTH; ++i) {
    nfapi_resp->nfapi_config.p7_vnf_address_ipv6.address[i] = rand8();
  }
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->nfapi_config.p7_vnf_port, NFAPI_NR_NFAPI_P7_VNF_PORT_TAG, rand16());
  nfapi_resp->num_tlv++;

  nfapi_resp->nfapi_config.p7_pnf_address_ipv4.tl.tag = NFAPI_NR_NFAPI_P7_PNF_ADDRESS_IPV4_TAG;
  for (int i = 0; i < NFAPI_IPV4_ADDRESS_LENGTH; ++i) {
    nfapi_resp->nfapi_config.p7_pnf_address_ipv4.address[i] = rand8();
  }
  nfapi_resp->num_tlv++;

  nfapi_resp->nfapi_config.p7_pnf_address_ipv6.tl.tag = NFAPI_NR_NFAPI_P7_PNF_ADDRESS_IPV6_TAG;
  for (int i = 0; i < NFAPI_IPV6_ADDRESS_LENGTH; ++i) {
    nfapi_resp->nfapi_config.p7_pnf_address_ipv6.address[i] = rand8();
  }
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->nfapi_config.p7_pnf_port, NFAPI_NR_NFAPI_P7_PNF_PORT_TAG, rand16());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->nfapi_config.timing_window, NFAPI_NR_NFAPI_TIMING_WINDOW_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->nfapi_config.timing_info_mode, NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG, rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->nfapi_config.timing_info_period, NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG, rand8());
  nfapi_resp->num_tlv++;
  /*
  // TODO: Uncomment this block when ready to enable the pack of the following VE TLVs in nr_fapi_p5.c
  // NFAPI_NR_FAPI_NUM_BEAMS_PERIOD_VENDOR_EXTENSION_TAG
  // NFAPI_NR_FAPI_ANALOG_BF_VENDOR_EXTENSION_TAG
  FILL_TLV(nfapi_resp->analog_beamforming_ve.num_beams_period_vendor_ext,
           NFAPI_NR_FAPI_NUM_BEAMS_PERIOD_VENDOR_EXTENSION_TAG,
           rand8());
  nfapi_resp->num_tlv++;

  FILL_TLV(nfapi_resp->analog_beamforming_ve.analog_bf_vendor_ext, NFAPI_NR_FAPI_ANALOG_BF_VENDOR_EXTENSION_TAG, rand8());
  nfapi_resp->num_tlv++;
  */
}

static void test_pack_unpack(nfapi_nr_config_request_scf_t *req)
{
  uint8_t msg_buf[65535];
  uint16_t msg_len = sizeof(*req);

  // first test the packing procedure
  int pack_result = fapi_nr_p5_message_pack(req, msg_len, msg_buf, sizeof(msg_buf), NULL);
  // PARAM.request message body length is 0
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
  nfapi_nr_config_request_scf_t unpacked_req = {0};
  int unpack_result =
      fapi_nr_p5_message_unpack(msg_buf, header.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), NULL);
  DevAssert(unpack_result >= 0);
  DevAssert(eq_config_request(&unpacked_req, req));
  free_config_request(&unpacked_req);
}

static void test_copy(const nfapi_nr_config_request_scf_t *msg)
{
  // Test copy function
  nfapi_nr_config_request_scf_t copy = {0};
  copy_config_request(msg, &copy);
  DevAssert(eq_config_request(msg, &copy));
  free_config_request(&copy);
}

static void test_config_req_rand(void)
{
  nfapi_nr_config_request_scf_t req = {.header.message_id = NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST};
  // Fill CONFIG.request TVLs
  fill_config_request_tlv_tdd_rand(&req);
  // Perform tests
  test_pack_unpack(&req);
  test_copy(&req);
  // All tests successful!
  free_config_request(&req);
}

static void fill_config_request_tlv_fdd(nfapi_nr_config_request_scf_t *req)
{
  /* carrier config */
  FILL_TLV(req->carrier_config.dl_bandwidth, NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG, 5);
  req->num_tlv++;
  FILL_TLV(req->carrier_config.dl_frequency, NFAPI_NR_CONFIG_DL_FREQUENCY_TAG, 2150430);
  req->num_tlv++;
  for (int i = 0; i < 5; ++i)
    FILL_TLV(req->carrier_config.dl_k0[i], NFAPI_NR_CONFIG_DL_K0_TAG, 0);
  // these 5 are 1 tlv
  req->num_tlv++;
  FILL_TLV(req->carrier_config.dl_grid_size[0], NFAPI_NR_CONFIG_DL_GRID_SIZE_TAG, 25);
  for (int i = 1; i < 5; ++i)
    FILL_TLV(req->carrier_config.dl_grid_size[i], NFAPI_NR_CONFIG_DL_GRID_SIZE_TAG, 0);
  // these 5 are 1 tlv
  req->num_tlv++;
  FILL_TLV(req->carrier_config.num_tx_ant, NFAPI_NR_CONFIG_NUM_TX_ANT_TAG, 1);
  req->num_tlv++;
  FILL_TLV(req->carrier_config.uplink_bandwidth, NFAPI_NR_CONFIG_UPLINK_BANDWIDTH_TAG, 5);
  req->num_tlv++;
  FILL_TLV(req->carrier_config.uplink_frequency, NFAPI_NR_CONFIG_UPLINK_FREQUENCY_TAG, 1750430);
  req->num_tlv++;
  for (int i = 0; i < 5; ++i)
    FILL_TLV(req->carrier_config.ul_k0[i], NFAPI_NR_CONFIG_UL_K0_TAG, 0);
  // these 5 are 1 tlv
  req->num_tlv++;
  FILL_TLV(req->carrier_config.ul_grid_size[0], NFAPI_NR_CONFIG_UL_GRID_SIZE_TAG, 25);
  for (int i = 0; i < 5; ++i)
    FILL_TLV(req->carrier_config.ul_grid_size[i], NFAPI_NR_CONFIG_UL_GRID_SIZE_TAG, 0);
  // these 5 are 1 tlv
  req->num_tlv++;
  FILL_TLV(req->carrier_config.num_rx_ant, NFAPI_NR_CONFIG_NUM_RX_ANT_TAG, 1);
  req->num_tlv++;
  FILL_TLV(req->carrier_config.frequency_shift_7p5khz, NFAPI_NR_CONFIG_FREQUENCY_SHIFT_7P5KHZ_TAG, 0);
  req->num_tlv++;

  /* cell config */
  FILL_TLV(req->cell_config.phy_cell_id, NFAPI_NR_CONFIG_PHY_CELL_ID_TAG, 0);
  req->num_tlv++;
  FILL_TLV(req->cell_config.frame_duplex_type, NFAPI_NR_CONFIG_FRAME_DUPLEX_TYPE_TAG, 0 /* FDD */);
  req->num_tlv++;

  /* SSB config */
  FILL_TLV(req->ssb_config.ss_pbch_power, NFAPI_NR_CONFIG_SS_PBCH_POWER_TAG, -25);
  req->num_tlv++;
  FILL_TLV(req->ssb_config.bch_payload, NFAPI_NR_CONFIG_BCH_PAYLOAD_TAG, 1 /* PHY generates timing bits */);
  req->num_tlv++;
  FILL_TLV(req->ssb_config.scs_common, NFAPI_NR_CONFIG_SCS_COMMON_TAG, 0 /* 15 kHz */);
  req->num_tlv++;

  /* PRACH config */
  FILL_TLV(req->prach_config.prach_sequence_length, NFAPI_NR_CONFIG_PRACH_SEQUENCE_LENGTH_TAG, 1);
  req->num_tlv++;
  FILL_TLV(req->prach_config.prach_sub_c_spacing, NFAPI_NR_CONFIG_PRACH_SUB_C_SPACING_TAG, 0);
  req->num_tlv++;
  FILL_TLV(req->prach_config.restricted_set_config, NFAPI_NR_CONFIG_RESTRICTED_SET_CONFIG_TAG, 0);
  req->num_tlv++;
  FILL_TLV(req->prach_config.prach_ConfigurationIndex, NFAPI_NR_CONFIG_PRACH_CONFIG_INDEX_TAG, 98);
  req->num_tlv++;
  int num_prach = 1;
  FILL_TLV(req->prach_config.num_prach_fd_occasions, NFAPI_NR_CONFIG_NUM_PRACH_FD_OCCASIONS_TAG, num_prach);
  req->num_tlv++;
  nfapi_nr_num_prach_fd_occasions_t *l = calloc(num_prach, sizeof(*l));
  req->prach_config.num_prach_fd_occasions_list = l;
  FILL_TLV(l->prach_root_sequence_index, NFAPI_NR_CONFIG_PRACH_ROOT_SEQUENCE_INDEX_TAG, 1);
  req->num_tlv++;
  FILL_TLV(l->num_root_sequences, NFAPI_NR_CONFIG_NUM_ROOT_SEQUENCES_TAG, 16);
  req->num_tlv++;
  FILL_TLV(l->k1, NFAPI_NR_CONFIG_K1_TAG, 0);
  req->num_tlv++;
  FILL_TLV(l->prach_zero_corr_conf, NFAPI_NR_CONFIG_PRACH_ZERO_CORR_CONF_TAG, 13);
  req->num_tlv++;
  /* should be at least according to spec */
  FILL_TLV(l->num_unused_root_sequences, NFAPI_NR_CONFIG_NUM_UNUSED_ROOT_SEQUENCES_TAG, 0);
  req->num_tlv++;
  l->unused_root_sequences_list = NULL;
  FILL_TLV(req->prach_config.ssb_per_rach, NFAPI_NR_CONFIG_SSB_PER_RACH_TAG, 3);
  req->num_tlv++;
  FILL_TLV(req->prach_config.prach_multiple_carriers_in_a_band, NFAPI_NR_CONFIG_PRACH_MULTIPLE_CARRIERS_IN_A_BAND_TAG, 0);
  req->num_tlv++;

  /* SSB table */
  FILL_TLV(req->ssb_table.ssb_offset_point_a, NFAPI_NR_CONFIG_SSB_OFFSET_POINT_A_TAG, 4);
  req->num_tlv++;
  FILL_TLV(req->ssb_table.beta_pss, NFAPI_NR_CONFIG_BETA_PSS_TAG, 0);
  req->num_tlv++;
  FILL_TLV(req->ssb_table.ssb_period, NFAPI_NR_CONFIG_SSB_PERIOD_TAG, 2);
  req->num_tlv++;
  FILL_TLV(req->ssb_table.ssb_subcarrier_offset, NFAPI_NR_CONFIG_SSB_SUBCARRIER_OFFSET_TAG, 0);
  req->num_tlv++;
  FILL_TLV(req->ssb_table.MIB, NFAPI_NR_CONFIG_MIB_TAG, 0);
  req->num_tlv++;

  FILL_TLV(req->ssb_table.ssb_mask_list[0].ssb_mask, NFAPI_NR_CONFIG_SSB_MASK_TAG, 2147483648);
  req->num_tlv++;
  FILL_TLV(req->ssb_table.ssb_mask_list[1].ssb_mask, NFAPI_NR_CONFIG_SSB_MASK_TAG, 0);
  req->num_tlv++;
  for (int i = 0; i < 64; i++) {
    FILL_TLV(req->ssb_table.ssb_beam_id_list[i].beam_id, NFAPI_NR_CONFIG_BEAM_ID_TAG, 0);
    req->num_tlv++;
  }

  /* NOTE: no TDD table! */

  /* Measurement config rssi_measurement NULL -> not present */

  /* nFAPI config */
  /* IPv4 address is 127.0.0.1 */
  req->nfapi_config.p7_vnf_address_ipv4.tl.tag = NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
  req->nfapi_config.p7_vnf_address_ipv4.address[0] = 127;
  req->nfapi_config.p7_vnf_address_ipv4.address[1] = 0;
  req->nfapi_config.p7_vnf_address_ipv4.address[2] = 0;
  req->nfapi_config.p7_vnf_address_ipv4.address[3] = 1;
  req->num_tlv++;
  /* no IPv6 address */
  FILL_TLV(req->nfapi_config.p7_vnf_port, NFAPI_NR_NFAPI_P7_VNF_PORT_TAG, 50011);
  req->num_tlv++;
  FILL_TLV(req->nfapi_config.timing_window, NFAPI_NR_NFAPI_TIMING_WINDOW_TAG, 30);
  req->num_tlv++;
  FILL_TLV(req->nfapi_config.timing_info_mode, NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG, 3);
  req->num_tlv++;
  FILL_TLV(req->nfapi_config.timing_info_period, NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG, 10);
  req->num_tlv++;
  FILL_TLV(req->nfapi_config.dl_tti_timing_offset, NFAPI_NR_NFAPI_DL_TTI_TIMING_OFFSET, 0);
  req->num_tlv++;
  FILL_TLV(req->nfapi_config.dl_tti_timing_offset, NFAPI_NR_NFAPI_UL_TTI_TIMING_OFFSET, 0);
  req->num_tlv++;
  FILL_TLV(req->nfapi_config.dl_tti_timing_offset, NFAPI_NR_NFAPI_UL_DCI_TIMING_OFFSET, 0);
  req->num_tlv++;
  FILL_TLV(req->nfapi_config.dl_tti_timing_offset, NFAPI_NR_NFAPI_TX_DATA_TIMING_OFFSET, 0);
  req->num_tlv++;

  /* PMI list NULL */

  /* Digital beamforming NULL */

  /* Analog beamforming NULL */

  //DevAssert(req->num_tlv == 103);
}

static void test_config_req_fdd(void)
{
  nfapi_nr_config_request_scf_t req = {.header.phy_id = 1, .header.message_id = NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST};
  // Fill CONFIG.request TVLs
  fill_config_request_tlv_fdd(&req);
  // Perform tests
  test_pack_unpack(&req);
  test_copy(&req);
  // All tests successful!
  free_config_request(&req);
}

int main()
{
  fapi_test_init();
  test_config_req_rand();
  test_config_req_fdd();
  return 0;
}
