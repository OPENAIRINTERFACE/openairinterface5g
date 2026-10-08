/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include "dci_payload_utils.h"
#include "nr_fapi_p7.h"
#include "nr_fapi_p7_utils.h"
#include "nr_fapi_common_util_test.h"

static void fill_ul_tti_request_beamforming(nfapi_nr_ul_beamforming_t *beamforming_pdu)
{
  beamforming_pdu->trp_scheme = rand8();
  beamforming_pdu->num_prgs = rand16_range(1, NFAPI_MAX_NUM_PRGS);
  beamforming_pdu->prg_size = rand16_range(1, 275);
  beamforming_pdu->dig_bf_interface = rand8_range(1,NFAPI_MAX_NUM_BG_IF);

  for (int prg = 0; prg < beamforming_pdu->num_prgs; ++prg) {
    for (int dbf_if = 0; dbf_if < beamforming_pdu->dig_bf_interface; ++dbf_if) {
      beamforming_pdu->prgs_list[prg].dig_bf_interface_list[dbf_if].beam_idx = rand16();
    }
  }
}

static void fill_ul_tti_request_prach_pdu(nfapi_nr_prach_pdu_t *pdu)
{
  pdu->phys_cell_id = rand16_range(0, 1007);
  pdu->num_prach_ocas = rand8_range(1, 7);
  pdu->prach_format = rand8_range(0, 13);
  pdu->num_ra = rand8_range(0, 7);
  pdu->prach_start_symbol = rand8_range(0, 13);
  pdu->num_cs = rand16_range(0, 419);
  fill_ul_tti_request_beamforming(&pdu->beamforming);
  pdu->param_v4.numSpatialStreamIndices = get_num_spatial_streams();
  fill_spatial_streams(pdu->param_v4.numSpatialStreamIndices, pdu->param_v4.spatialStreamIndices);
}

static void fill_ul_tti_request_pusch_pdu(nfapi_nr_pusch_pdu_t *pdu)
{
  pdu->pdu_bit_map = rand16_range(0b0000, 0b1111);
  pdu->rnti = rand16_range(1, 65535);
  pdu->handle = rand32();
  pdu->bwp_size = rand16_range(1, 275);
  pdu->bwp_start = rand16_range(0, 274);
  pdu->subcarrier_spacing = rand8_range(0, 4);
  pdu->cyclic_prefix = rand8_range(0, 1);
  pdu->target_code_rate = rand16_range(300, 9480);
  uint8_t transform_precoding = rand8_range(0, 1);
  uint8_t qam_mod_order_values[] = {1, 2, 4, 6, 8};
  // Value: 2,4,6,8 if transform precoding is disabled
  // Value: 1,2,4,6,8 if transform precoding is enabled
  pdu->qam_mod_order = qam_mod_order_values[rand8_range(1 - pdu->transform_precoding, 4)];
  pdu->mcs_index = rand8_range(0, 31);
  pdu->mcs_table = rand8_range(0, 4);
  pdu->transform_precoding = transform_precoding;
  pdu->data_scrambling_id = rand16();
  pdu->nrOfLayers = rand8_range(1, 4);
  // Bitmap occupying the 14 LSBs
  pdu->ul_dmrs_symb_pos = rand16_range(0b00000000000000, 0b11111111111111);
  pdu->dmrs_config_type = rand8_range(0, 1);
  pdu->ul_dmrs_scrambling_id = rand16();
  pdu->pusch_identity = rand16_range(0, 1007);
  pdu->scid = rand8_range(0, 1);
  pdu->num_dmrs_cdm_grps_no_data = rand8_range(1, 3);
  // Bitmap occupying the 11 LSBs
  pdu->dmrs_ports = rand16_range(0b00000000000, 0b11111111111);
  pdu->resource_alloc = rand8_range(0, 1);
  for (int i = 0; i < 36; ++i) {
    pdu->rb_bitmap[i] = rand8();
  }
  pdu->rb_start = rand16_range(0, 274);
  pdu->rb_size = rand16_range(1, 275);
  pdu->vrb_to_prb_mapping = 0;
  pdu->frequency_hopping = rand8_range(0, 1);
  pdu->tx_direct_current_location = rand16_range(0, 4095);
  pdu->uplink_frequency_shift_7p5khz = rand8_range(0, 1);
  pdu->start_symbol_index = rand8_range(0, 13);
  pdu->nr_of_symbols = rand8_range(1, 14);

  // Check if PUSCH_PDU_BITMAP_PUSCH_DATA bit is set
  if (pdu->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_DATA) {
    nfapi_nr_pusch_data_t *pusch_data = &pdu->pusch_data;
    pusch_data->rv_index = rand8_range(0, 3);
    pusch_data->harq_process_id = rand8_range(0, 15);
    pusch_data->new_data_indicator = rand8_range(0, 1);
    // TBSize is 32 bits in width, but the value is from 0 to 65535 ( max for 16 bits )
    pusch_data->tb_size = rand16();
    // num_cb set always to 0, due to OAI not supporting CBG
    pusch_data->num_cb = 0;
    for (int i = 0; i < (pusch_data->num_cb + 7) / 8; ++i) {
      pusch_data->cb_present_and_position[i] = 0;
    }
  }

  // Check if PUSCH_PDU_BITMAP_PUSCH_UCI bit is set
  if (pdu->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_UCI) {
    nfapi_nr_pusch_uci_t *pusch_uci = &pdu->pusch_uci;
    // For testing purposes "decide" here if it uses small block length or polar
    pusch_uci->harq_ack_bit_length = rand16_range(0, 1706);
    if (pusch_uci->harq_ack_bit_length <= 11) {
      pusch_uci->csi_part1_bit_length = rand16_range(0, 11);
      pusch_uci->csi_part2_bit_length = rand16_range(0, 11);
    } else {
      pusch_uci->csi_part1_bit_length = rand16_range(12, 1706);
      pusch_uci->csi_part2_bit_length = rand16_range(12, 1706);
    }
    pusch_uci->alpha_scaling = rand8_range(0, 3);
    pusch_uci->beta_offset_harq_ack = rand8_range(0, 15);
    pusch_uci->beta_offset_csi1 = rand8_range(0, 18);
    pusch_uci->beta_offset_csi2 = rand8_range(0, 18);
  }

  // Check if PUSCH_PDU_BITMAP_PUSCH_PTRS bit is set
  if (pdu->pdu_bit_map & PUSCH_PDU_BITMAP_PUSCH_PTRS) {
    nfapi_nr_pusch_ptrs_t *pusch_ptrs = &pdu->pusch_ptrs;
    pusch_ptrs->num_ptrs_ports = rand8_range(1, 2);
    pusch_ptrs->ptrs_ports_list = calloc(pusch_ptrs->num_ptrs_ports, sizeof(nfapi_nr_ptrs_ports_t));
    for (int ptrs_port = 0; ptrs_port < pusch_ptrs->num_ptrs_ports; ++ptrs_port) {
      // Bitmap occupying the 12 LSBs
      pusch_ptrs->ptrs_ports_list[ptrs_port].ptrs_port_index = rand16_range(0b000000000000, 0b111111111111);
      pusch_ptrs->ptrs_ports_list[ptrs_port].ptrs_dmrs_port = rand8_range(0, 11);
      pusch_ptrs->ptrs_ports_list[ptrs_port].ptrs_re_offset = rand8_range(0, 11);
    }
    pusch_ptrs->ptrs_time_density = rand8_range(0, 2);
    pusch_ptrs->ptrs_freq_density = rand8_range(0, 1);
    pusch_ptrs->ul_ptrs_power = rand8_range(0, 3);
  }

  // Check if PUSCH_PDU_BITMAP_DFTS_OFDM bit is set
  if (pdu->pdu_bit_map & PUSCH_PDU_BITMAP_DFTS_OFDM) {
    nfapi_nr_dfts_ofdm_t *dfts_ofdm = &pdu->dfts_ofdm;
    dfts_ofdm->low_papr_group_number = rand8_range(0, 29);
    dfts_ofdm->low_papr_sequence_number = rand16();
    dfts_ofdm->ul_ptrs_sample_density = rand8_range(1, 8);
    dfts_ofdm->ul_ptrs_time_density_transform_precoding = rand8_range(1, 4);
  }

  fill_ul_tti_request_beamforming(&pdu->beamforming);
  pdu->maintenance_parms_v3.ldpcBaseGraph = rand8();
  pdu->maintenance_parms_v3.tbSizeLbrmBytes = rand32();
  pdu->param_v4.numSpatialStreamIndices = get_num_spatial_streams();
  fill_spatial_streams(pdu->param_v4.numSpatialStreamIndices, pdu->param_v4.spatialStreamIndices);
}

static void fill_ul_tti_request_pucch_pdu(nfapi_nr_pucch_pdu_t *pdu)
{
  pdu->rnti = rand16_range(1, 65535);
  pdu->handle = rand32();
  pdu->bwp_size = rand16_range(1, 275);
  pdu->bwp_start = rand16_range(0, 274);
  pdu->subcarrier_spacing = rand8_range(0, 4);
  pdu->cyclic_prefix = rand8_range(0, 1);
  pdu->format_type = rand8_range(0, 4);
  pdu->multi_slot_tx_indicator = rand8_range(0, 3);
  pdu->pi_2bpsk = rand8_range(0, 1);
  pdu->prb_start = rand16_range(0, 274);
  pdu->prb_size = rand16_range(1, 16);
  pdu->start_symbol_index = rand8_range(0, 13);
  if (pdu->format_type == 0 || pdu->format_type == 2) {
    pdu->nr_of_symbols = rand8_range(1, 2);
  } else {
    pdu->nr_of_symbols = rand8_range(4, 14);
  }
  pdu->freq_hop_flag = rand8_range(0, 1);
  pdu->second_hop_prb = rand16_range(0, 274);
  // Following 4 parameters are Valid for formats 0, 1, 3 and 4, assuming that otherwise, set to 0
  if (pdu->format_type == 0 || pdu->format_type == 1 || pdu->format_type == 3 || pdu->format_type == 4) {
    pdu->group_hop_flag = rand8_range(0, 1);
    pdu->sequence_hop_flag = rand8_range(0, 1);
    pdu->hopping_id = rand16_range(0, 1023);
    pdu->initial_cyclic_shift = rand16_range(0, 11);
  } else {
    pdu->group_hop_flag = 0;
    pdu->sequence_hop_flag = 0;
    pdu->hopping_id = 0;
    pdu->initial_cyclic_shift = 0;
  }
  // Valid for format 2, 3 and 4.
  if (pdu->format_type == 2 || pdu->format_type == 3 || pdu->format_type == 4) {
    pdu->data_scrambling_id = rand16_range(0, 1023);
  } else {
    pdu->data_scrambling_id = 0;
  }

  // Valid for format 1.
  if (pdu->format_type == 1) {
    pdu->time_domain_occ_idx = rand8_range(0, 6);
  } else {
    pdu->time_domain_occ_idx = 0;
  }

  // the following 2 parameters are Valid for format 4.
  if (pdu->format_type == 4) {
    pdu->pre_dft_occ_idx = rand8_range(0, 3);
    // Value: 2 or 4
    pdu->pre_dft_occ_len = 2 + (2 * rand8_range(0, 1));
  } else {
    pdu->pre_dft_occ_idx = 0;
    pdu->pre_dft_occ_len = 0;
  }

  // Valid for formats 3 and 4.
  if (pdu->format_type == 3 || pdu->format_type == 4) {
    pdu->add_dmrs_flag = rand8_range(0, 1);
  } else {
    pdu->add_dmrs_flag = 0;
  }

  // Valid for format 2.
  if (pdu->format_type == 2) {
    pdu->dmrs_scrambling_id = rand16();
  } else {
    pdu->dmrs_scrambling_id = 0;
  }

  // Valid for format 4.
  if (pdu->format_type == 4) {
    pdu->dmrs_cyclic_shift = rand8_range(0, 9);
  } else {
    pdu->dmrs_cyclic_shift = 0;
  }

  // Valid for format 0 and 1
  if (pdu->format_type == 0 || pdu->format_type == 1) {
    pdu->sr_flag = rand8_range(0, 1);
  } else {
    pdu->sr_flag = 0;
  }
  //	Value:
  //	0 = no HARQ bits
  //	1->2 = Valid for Formats 0 and 1
  //	2 -> 1706 = Valid for Formats 2, 3 and 4
  if (pdu->format_type == 0 || pdu->format_type == 1) {
    //{0,1,2}
    pdu->bit_len_harq = rand16_range(0, 2);
  } else {
    // {0}∪{2,…,1706}
    uint16_t bit_len_harq = rand16_range(0, 1705);
    if (bit_len_harq == 0) {
      pdu->bit_len_harq = 0;
    } else {
      // Shift range {1,…,1705} -> {2,…,1706}
      pdu->bit_len_harq = bit_len_harq + 1;
    }
  }

  // Following 2 parameters are Valid for format 2, 3 and 4.
  if (pdu->format_type == 2 || pdu->format_type == 3 || pdu->format_type == 4) {
    pdu->bit_len_csi_part1 = rand16_range(0, 1706);
    pdu->bit_len_csi_part2 = rand16_range(0, 1706);
  } else {
    pdu->bit_len_csi_part1 = 0;
    pdu->bit_len_csi_part2 = 0;
  }

  fill_ul_tti_request_beamforming(&pdu->beamforming);
  pdu->param_v4.numSpatialStreamIndices = get_num_spatial_streams();
  fill_spatial_streams(pdu->param_v4.numSpatialStreamIndices, pdu->param_v4.spatialStreamIndices);
}

static void fill_ul_tti_request_srs_parameters(nfapi_v4_srs_parameters_t *params, const uint8_t num_symbols){
  params->srs_bandwidth_size = rand16_range(4, 272);
  for (int idx = 0; idx < num_symbols; ++idx) {
    nfapi_v4_srs_parameters_symbols_t *symbol = &params->symbol_list[idx];
    symbol->srs_bandwidth_start = rand16_range(0,268);
    symbol->sequence_group = rand8_range(0,29);
    symbol->sequence_number = rand8_range(0,1);
  }

#ifdef ENABLE_AERIAL
  // For Aerial, we always process the 4 reported symbols, not only the ones indicated by num_symbols
  for (int symbol_idx = num_symbols; symbol_idx < 4; ++symbol_idx) {
    nfapi_v4_srs_parameters_symbols_t *symbol = &params->symbol_list[idx];
    symbol->srs_bandwidth_start = rand16_range(0,268);
    symbol->sequence_group = rand8_range(0,29);
    symbol->sequence_number = rand8_range(0,1);
  }
#endif // ENABLE_AERIAL

  params->usage = rand32_range(0, 0b1111);
  const uint8_t nUsage = __builtin_popcount(params->usage);
  for (int idx = 0; idx < nUsage; ++idx) {
    params->report_type[idx] = rand8_range(0,1);
  }
  params->singular_Value_representation = rand8_range(0,1);
  params->iq_representation = rand8_range(0,1);
  params->prg_size = rand16_range(1,272);
  params->num_total_ue_antennas = rand8_range(1, 16);
  params->ue_antennas_in_this_srs_resource_set = rand32();
  params->sampled_ue_antennas = rand32();
  params->report_scope = rand8_range(0,1);
  params->num_ul_spatial_streams_ports = rand8_range(0, 255);
  for (int idx = 0; idx < params->num_ul_spatial_streams_ports; ++idx) {
    params->Ul_spatial_stream_ports[idx] = rand8();
  }
}

static void fill_ul_tti_request_srs_pdu(nfapi_nr_srs_pdu_t *pdu)
{
  pdu->rnti = rand16_range(1, 65535);
  pdu->handle = rand32();
  pdu->bwp_size = rand16_range(1, 275);
  pdu->bwp_start = rand16_range(0, 274);
  pdu->subcarrier_spacing = rand8_range(0, 4);
  pdu->cyclic_prefix = rand8_range(0, 1);
  pdu->num_ant_ports = rand8_range(0, 2);
  pdu->num_symbols = rand8_range(0, 2);
  pdu->num_repetitions = rand8_range(0, 2);
  pdu->time_start_position = rand8_range(0, 13);
  pdu->config_index = rand8_range(0, 63);
  pdu->sequence_id = rand16_range(0, 1023);
  pdu->bandwidth_index = rand8_range(0, 3);
  pdu->comb_size = rand8_range(0, 1);
  // Following 2 parameters value range differs according to comb_size
  if (pdu->comb_size == 0) {
    pdu->comb_offset = rand8_range(0, 1);
    pdu->cyclic_shift = rand8_range(0, 7);
  } else {
    pdu->comb_offset = rand8_range(0, 3);
    pdu->cyclic_shift = rand8_range(0, 11);
  }
  pdu->frequency_position = rand8_range(0, 67);
  pdu->frequency_shift = rand16_range(0, 268);
  pdu->frequency_hopping = rand8_range(0, 3);
  pdu->group_or_sequence_hopping = rand8_range(0, 2);
  pdu->resource_type = rand8_range(0, 2);
  // Value: 1,2,3,4,5,8,10,16,20,32,40,64,80,160,320,640,1280,2560
  const uint16_t t_srs_values[] = {1, 2, 3, 4, 5, 8, 10, 16, 20, 32, 40, 64, 80, 160, 320, 640, 1280, 2560};
  pdu->t_srs = t_srs_values[rand16_range(0, 17)];
  pdu->t_offset = rand16_range(0, 2559);
  fill_ul_tti_request_beamforming(&pdu->beamforming);
  fill_ul_tti_request_srs_parameters(&pdu->srs_parameters_v4, 1 << pdu->num_symbols);
}

static void fill_ul_tti_request(nfapi_nr_ul_tti_request_t *msg)
{
  msg->SFN = rand16_range(0, 1023);
  msg->Slot = rand16_range(0, 159);
  msg->n_pdus = rand8_range(4, 16); // Minimum 4 PDUs in order to test at least one of each

  printf(" NUM PDUS  %d\n", msg->n_pdus);
  msg->rach_present = 1; // rand8_range(0, 1); Always set to 1, since there can only be 1 PRACH PDU
  printf(" NUM RACH PDUS  %d\n", msg->rach_present);
  uint8_t available_PDUs = msg->n_pdus - msg->rach_present;
  msg->n_ulsch = rand8_range(1, available_PDUs - 2);
  printf(" NUM PUSCH PDUS  %d\n", msg->n_ulsch);
  available_PDUs -= msg->n_ulsch;
  msg->n_ulcch = rand8_range(1, available_PDUs - 1);
  printf(" NUM PUCCH PDUS  %d\n", msg->n_ulcch);
  // The variable available_PDUs now contains the number of SRS PDUs needed to add to reach n_pdus
  available_PDUs -= msg->n_ulcch;
  printf(" NUM SRS PDUS  %d\n", available_PDUs);
  msg->n_group = rand8_range(0, NFAPI_MAX_NUM_GROUPS);
  printf(" NUM Groups  %d\n", msg->n_group);

  // Add PRACH PDU, if applicable

  if (msg->rach_present == 1) {
    nfapi_nr_ul_tti_request_number_of_pdus_t *prach_pdu = &msg->pdus_list[0];
    prach_pdu->pdu_type = NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE;
    prach_pdu->pdu_size = rand16();
    fill_ul_tti_request_prach_pdu(&prach_pdu->prach_pdu);
  }

  // start either at 0 or 1, same value as rach_present
  int pdu_idx = msg->rach_present;
  // Assign all PUSCH PDUs
  for (int i = 0; i < msg->n_ulsch; ++i, ++pdu_idx) {
    nfapi_nr_ul_tti_request_number_of_pdus_t *pdu = &msg->pdus_list[pdu_idx];
    pdu->pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
    pdu->pdu_size = rand16();
    fill_ul_tti_request_pusch_pdu(&pdu->pusch_pdu);
  }
  // Assign all PUCCH PDUs
  for (int i = 0; i < msg->n_ulcch; ++i, ++pdu_idx) {
    nfapi_nr_ul_tti_request_number_of_pdus_t *pdu = &msg->pdus_list[pdu_idx];
    pdu->pdu_type = NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE;
    pdu->pdu_size = rand16();
    fill_ul_tti_request_pucch_pdu(&pdu->pucch_pdu);
  }

  // Assing all SRS PDUs
  for (int i = 0; i < available_PDUs; ++i, ++pdu_idx) {
    nfapi_nr_ul_tti_request_number_of_pdus_t *pdu = &msg->pdus_list[pdu_idx];
    pdu->pdu_type = NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE;
    pdu->pdu_size = rand16();
    fill_ul_tti_request_srs_pdu(&pdu->srs_pdu);
  }

  for (int group_idx = 0; group_idx < msg->n_group; ++group_idx) {
    nfapi_nr_ul_tti_request_number_of_groups_t *group = &msg->groups_list[group_idx];
    group->n_ue = rand8_range(1, 6);
    for (int ue = 0; ue < group->n_ue; ++ue) {
      group->ue_list[ue].pdu_idx = rand8();
    }
  }
}

static void test_pack_unpack(nfapi_nr_ul_tti_request_t *req)
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
  nfapi_nr_ul_tti_request_t unpacked_req = {0};
  int unpack_result =
      fapi_nr_p7_message_unpack(msg_buf, header.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), 0);
  DevAssert(unpack_result >= 0);
  DevAssert(eq_ul_tti_request(&unpacked_req, req));
  free_ul_tti_request(&unpacked_req);
}

static void test_copy(const nfapi_nr_ul_tti_request_t *msg)
{
  // Test copy function
  nfapi_nr_ul_tti_request_t copy = {0};
  copy_ul_tti_request(msg, &copy);
  DevAssert(eq_ul_tti_request(msg, &copy));
  free_ul_tti_request(&copy);
}

int main()
{
  fapi_test_init();

  nfapi_nr_ul_tti_request_t req = {.header.message_id = NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST};

  // Fill DL_TTI request
  fill_ul_tti_request(&req);
  // Perform tests
  test_pack_unpack(&req);
  test_copy(&req);
  // All tests successful!
  free_ul_tti_request(&req);
  return 0;
}
