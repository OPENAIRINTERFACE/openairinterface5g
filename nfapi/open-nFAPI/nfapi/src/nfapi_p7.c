/*
 * Copyright 2017 Cisco Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zlib.h>
#include <sched.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#include <nfapi_interface.h>
#include <nfapi.h>
#include <debug.h>
#include "nfapi_nr_interface_scf.h"

extern int nfapi_unpack_p7_vendor_extension(nfapi_p7_message_header_t *header, uint8_t **ppReadPackedMsg, void *user_data);
extern int nfapi_pack_p7_vendor_extension(nfapi_p7_message_header_t *header, uint8_t **ppWritePackedMsg, void *user_data);

uint32_t nfapi_calculate_checksum(uint8_t *buffer, uint16_t len) {
  uint32_t chksum = 0;
  // calcaulte upto the checksum
  chksum = crc32(chksum, buffer, 8);
  // skip the checksum
  uint8_t zeros[4] = {0, 0, 0, 0};
  chksum = crc32(chksum, zeros, 4);
  // continu with the rest of the mesage
  chksum = crc32(chksum, &buffer[NFAPI_P7_HEADER_LENGTH], len - NFAPI_P7_HEADER_LENGTH);
  // return the inverse
  return ~(chksum);
}

int nfapi_p7_update_checksum(uint8_t *buffer, uint32_t len) {
  uint32_t checksum = nfapi_calculate_checksum(buffer, len);
  uint8_t *p_write = &buffer[8];
  return (push32(checksum, &p_write, buffer + len) > 0 ? 0 : -1);
}

int nfapi_p7_update_transmit_timestamp(uint8_t *buffer, uint32_t timestamp) {
  uint8_t *p_write = &buffer[12];
  return (push32(timestamp, &p_write, buffer + 16) > 0 ? 0 : -1);
}

uint32_t nfapi_p7_calculate_checksum(uint8_t *buffer, uint32_t len) {
  return nfapi_calculate_checksum(buffer, len);
}

void *nfapi_p7_allocate(size_t size, nfapi_p7_codec_config_t *config) {
  if(size == 0)
    return 0;

  void *buffer_p = NULL;

  if(config && config->allocate) {
    buffer_p = (config->allocate)(size);

    if(buffer_p != NULL) {
      memset(buffer_p,0,size);
    }

    return buffer_p;
  } else {
    buffer_p = calloc(1, size);
    return buffer_p;
  }
}

void nfapi_p7_deallocate(void *ptr, nfapi_p7_codec_config_t *config) {
  if(ptr == NULL)
    return;

  if(config && config->deallocate) {
    return (config->deallocate)(ptr);
  } else {
    return free(ptr);
  }
}
// Pack routines


static uint8_t pack_dl_config_dci_dl_pdu_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel8_t *value = (nfapi_dl_config_dci_dl_pdu_rel8_t *)tlv;
  //NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() dci_format:%u\n", __FUNCTION__, value->dci_format);
  return ( push8(value->dci_format, ppWritePackedMsg, end) &&
           push8(value->cce_idx, ppWritePackedMsg, end) &&
           push8(value->aggregation_level, ppWritePackedMsg, end) &&
           push16(value->rnti, ppWritePackedMsg, end) &&
           push8(value->resource_allocation_type, ppWritePackedMsg, end) &&
           push8(value->virtual_resource_block_assignment_flag, ppWritePackedMsg, end) &&
           push32(value->resource_block_coding, ppWritePackedMsg, end) &&
           push8(value->mcs_1, ppWritePackedMsg, end) &&
           push8(value->redundancy_version_1, ppWritePackedMsg, end) &&
           push8(value->new_data_indicator_1, ppWritePackedMsg, end) &&
           push8(value->transport_block_to_codeword_swap_flag, ppWritePackedMsg, end) &&
           push8(value->mcs_2, ppWritePackedMsg, end) &&
           push8(value->redundancy_version_2, ppWritePackedMsg, end) &&
           push8(value->new_data_indicator_2, ppWritePackedMsg, end) &&
           push8(value->harq_process, ppWritePackedMsg, end) &&
           push8(value->tpmi, ppWritePackedMsg, end) &&
           push8(value->pmi, ppWritePackedMsg, end) &&
           push8(value->precoding_information, ppWritePackedMsg, end) &&
           push8(value->tpc, ppWritePackedMsg, end) &&
           push8(value->downlink_assignment_index, ppWritePackedMsg, end) &&
           push8(value->ngap, ppWritePackedMsg, end) &&
           push8(value->transport_block_size_index, ppWritePackedMsg, end) &&
           push8(value->downlink_power_offset, ppWritePackedMsg, end) &&
           push8(value->allocate_prach_flag, ppWritePackedMsg, end) &&
           push8(value->preamble_index, ppWritePackedMsg, end) &&
           push8(value->prach_mask_index, ppWritePackedMsg, end) &&
           push8(value->rnti_type, ppWritePackedMsg, end) &&
           push16(value->transmission_power, ppWritePackedMsg, end));
}

static uint8_t pack_dl_config_dci_dl_pdu_rel9_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel9_t *value = (nfapi_dl_config_dci_dl_pdu_rel9_t *)tlv;
  return( push8(value->mcch_flag, ppWritePackedMsg, end) &&
          push8(value->mcch_change_notification, ppWritePackedMsg, end) &&
          push8(value->scrambling_identity, ppWritePackedMsg, end));
}

static uint8_t pack_dl_config_dci_dl_pdu_rel10_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel10_t *value = (nfapi_dl_config_dci_dl_pdu_rel10_t *)tlv;
  return ( push8(value->cross_carrier_scheduling_flag, ppWritePackedMsg, end) &&
           push8(value->carrier_indicator, ppWritePackedMsg, end) &&
           push8(value->srs_flag, ppWritePackedMsg, end) &&
           push8(value->srs_request, ppWritePackedMsg, end) &&
           push8(value->antenna_ports_scrambling_and_layers, ppWritePackedMsg, end) &&
           push8(value->total_dci_length_including_padding, ppWritePackedMsg, end) &&
           push8(value->n_dl_rb, ppWritePackedMsg, end));
}

static uint8_t pack_dl_config_dci_dl_pdu_rel11_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel11_t *value = (nfapi_dl_config_dci_dl_pdu_rel11_t *)tlv;
  return ( push8(value->harq_ack_resource_offset, ppWritePackedMsg, end) &&
           push8(value->pdsch_re_mapping_quasi_co_location_indicator, ppWritePackedMsg, end));
}

static uint8_t pack_dl_config_dci_dl_pdu_rel12_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel12_t *value = (nfapi_dl_config_dci_dl_pdu_rel12_t *)tlv;
  return ( push8(value->primary_cell_type, ppWritePackedMsg, end) &&
           push8(value->ul_dl_configuration_flag, ppWritePackedMsg, end) &&
           push8(value->number_ul_dl_configurations, ppWritePackedMsg, end) &&
           pusharray8(value->ul_dl_configuration_indication, NFAPI_MAX_UL_DL_CONFIGURATIONS, value->number_ul_dl_configurations, ppWritePackedMsg, end));
}

static uint8_t pack_tpm_value(nfapi_dl_config_dci_dl_tpm_t *value, uint8_t **ppWritePackedMsg, uint8_t *end) {
  if (!( push8(value->num_prb_per_subband, ppWritePackedMsg, end) &&
         push8(value->number_of_subbands, ppWritePackedMsg, end) &&
         push8(value->num_antennas, ppWritePackedMsg, end)))
    return 0;

  uint8_t idx = 0;

  for(idx = 0; idx < value->number_of_subbands; ++idx) {
    nfapi_dl_config_dci_dl_tpm_subband_info_t *subband_info = &(value->subband_info[idx]);

    if(!(push8(subband_info->subband_index, ppWritePackedMsg, end) &&
         push8(subband_info->scheduled_ues, ppWritePackedMsg, end)))
      return 0;

    uint8_t antenna_idx = 0;
    uint8_t scheduled_ue_idx = 0;

    for(antenna_idx = 0; antenna_idx < value->num_antennas; ++antenna_idx) {
      for(scheduled_ue_idx = 0; scheduled_ue_idx < subband_info->scheduled_ues; ++scheduled_ue_idx) {
        if(!push16(subband_info->precoding_value[antenna_idx][scheduled_ue_idx], ppWritePackedMsg, end))
          return 0;
      }
    }
  }

  return 1;
}


static uint8_t pack_dl_tti_csi_rs_pdu_rel15_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nr_dl_tti_csi_rs_pdu_rel15_t *value = (nfapi_nr_dl_tti_csi_rs_pdu_rel15_t *)tlv;
  return(
          push16(value->bwp_size, ppWritePackedMsg, end) &&
          push16(value->bwp_start, ppWritePackedMsg, end) &&
          push8(value->subcarrier_spacing, ppWritePackedMsg, end) &&
          push8(value->cyclic_prefix, ppWritePackedMsg, end) &&
          push16(value->start_rb, ppWritePackedMsg, end) &&
          push16(value->nr_of_rbs, ppWritePackedMsg, end) &&
          push8(value->csi_type, ppWritePackedMsg, end) &&
          push8(value->row, ppWritePackedMsg, end) &&
          push16(value->freq_domain, ppWritePackedMsg, end) &&
          push8(value->symb_l0, ppWritePackedMsg, end) &&
          push8(value->symb_l1, ppWritePackedMsg, end) &&
          push8(value->cdm_type, ppWritePackedMsg, end) &&
          push8(value->freq_density, ppWritePackedMsg, end) &&
          push16(value->scramb_id, ppWritePackedMsg, end) &&
          push8(value->power_control_offset, ppWritePackedMsg, end) &&
          push8(value->power_control_offset_ss, ppWritePackedMsg, end)
        );
}


static uint8_t pack_dl_tti_pdcch_pdu_rel15_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *value = (nfapi_nr_dl_tti_pdcch_pdu_rel15_t *)tlv;

  for(uint8_t i = 0; i < MAX_DCI_CORESET; ++i) {
    if(!push16(value->dci_pdu[i].RNTI, ppWritePackedMsg, end) &&
        push16(value->dci_pdu[i].ScramblingId, ppWritePackedMsg, end) &&
        push16(value->dci_pdu[i].ScramblingRNTI, ppWritePackedMsg, end) &&
        push8(value->dci_pdu[i].CceIndex, ppWritePackedMsg, end) &&
        push8(value->dci_pdu[i].AggregationLevel, ppWritePackedMsg, end) &&
        push8(value->dci_pdu[i].beta_PDCCH_1_0, ppWritePackedMsg, end) &&
        push8(value->dci_pdu[i].powerControlOffsetSS, ppWritePackedMsg, end) &&
        push16(value->dci_pdu[i].PayloadSizeBits, ppWritePackedMsg, end) &&
        pusharray8(value->dci_pdu[i].Payload, DCI_PAYLOAD_BYTE_LEN, value->dci_pdu[i].PayloadSizeBits, ppWritePackedMsg, end));

    return 0;
  }

  // TODO: resolve the packaging of array (currently sending a single element)
  return(
          push16(value->BWPSize, ppWritePackedMsg, end) &&
          push16(value->BWPStart, ppWritePackedMsg, end) &&
          push8(value->SubcarrierSpacing, ppWritePackedMsg, end) &&
          push8(value->CyclicPrefix, ppWritePackedMsg, end) &&
          push8(value->StartSymbolIndex, ppWritePackedMsg, end) &&
          push8(value->DurationSymbols, ppWritePackedMsg, end) &&
          pusharray8(value->FreqDomainResource, 6, 6, ppWritePackedMsg, end) &&
          push8(value->CceRegMappingType, ppWritePackedMsg, end) &&
          push8(value->RegBundleSize, ppWritePackedMsg, end) &&
          push8(value->InterleaverSize, ppWritePackedMsg, end) &&
          push8(value->CoreSetType, ppWritePackedMsg, end) &&
          push16(value->ShiftIndex, ppWritePackedMsg, end) &&
          push8(value->precoderGranularity, ppWritePackedMsg, end) &&
          push16(value->numDlDci, ppWritePackedMsg, end));
}


static uint8_t pack_dl_tti_pdsch_pdu_rel15_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *value = (nfapi_nr_dl_tti_pdsch_pdu_rel15_t *)tlv;
  // TODO: resolve the packaging of array (currently sending a single element)
  return(
          push16(value->pduBitmap, ppWritePackedMsg, end) &&
          push16(value->rnti, ppWritePackedMsg, end) &&
          push16(value->pduIndex, ppWritePackedMsg, end) &&
          push16(value->BWPSize, ppWritePackedMsg, end) &&
          push16(value->BWPStart, ppWritePackedMsg, end) &&
          push8(value->SubcarrierSpacing, ppWritePackedMsg, end) &&
          push8(value->CyclicPrefix, ppWritePackedMsg, end) &&
          push8(value->NrOfCodewords, ppWritePackedMsg, end) &&
          pusharray16(value->targetCodeRate, 2, 1, ppWritePackedMsg, end) &&
          pusharray8(value->qamModOrder, 2, 1, ppWritePackedMsg, end) &&
          pusharray8(value->mcsIndex, 2, 1, ppWritePackedMsg, end) &&
          pusharray8(value->mcsTable, 2, 1, ppWritePackedMsg, end) &&
          pusharray8(value->rvIndex, 2, 1, ppWritePackedMsg, end) &&
          pusharray32(value->TBSize, 2, 1, ppWritePackedMsg, end) &&
          push16(value->dataScramblingId, ppWritePackedMsg, end) &&
          push8(value->nrOfLayers, ppWritePackedMsg, end) &&
          push8(value->transmissionScheme, ppWritePackedMsg, end) &&
          push8(value->refPoint, ppWritePackedMsg, end) &&
          push16(value->dlDmrsSymbPos, ppWritePackedMsg, end) &&
          push8(value->dmrsConfigType, ppWritePackedMsg, end) &&
          push16(value->dlDmrsScramblingId, ppWritePackedMsg, end) &&
          push8(value->SCID, ppWritePackedMsg, end) &&
          push8(value->numDmrsCdmGrpsNoData, ppWritePackedMsg, end) &&
          push16(value->dmrsPorts, ppWritePackedMsg, end) &&
          push8(value->resourceAlloc, ppWritePackedMsg, end) &&
          push16(value->rbStart, ppWritePackedMsg, end) &&
          push16(value->rbSize, ppWritePackedMsg, end) &&
          push8(value->VRBtoPRBMapping, ppWritePackedMsg, end) &&
          push8(value->StartSymbolIndex, ppWritePackedMsg, end) &&
          push8(value->NrOfSymbols, ppWritePackedMsg, end) &&
          push8(value->PTRSPortIndex, ppWritePackedMsg, end) &&
          push8(value->PTRSTimeDensity, ppWritePackedMsg, end) &&
          push8(value->PTRSFreqDensity, ppWritePackedMsg, end) &&
          push8(value->PTRSReOffset, ppWritePackedMsg, end)
        );
}


static uint8_t pack_dl_tti_ssb_pdu_rel15_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nr_dl_tti_ssb_pdu_rel15_t *value = (nfapi_nr_dl_tti_ssb_pdu_rel15_t *)tlv;
  return(
          push16(value->PhysCellId, ppWritePackedMsg, end) &&
          push8(value->BetaPss, ppWritePackedMsg, end) &&
          push8(value->SsbBlockIndex, ppWritePackedMsg, end) &&
          push8(value->SsbSubcarrierOffset, ppWritePackedMsg, end) &&
          push16(value->ssbOffsetPointA, ppWritePackedMsg, end) &&
          push8(value->bchPayloadFlag, ppWritePackedMsg, end) &&
          push32(value->bchPayload, ppWritePackedMsg, end)
          // TODO: pack precoding_and_beamforming too
        );
}


static uint8_t pack_dl_config_dci_dl_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel13_t *value = (nfapi_dl_config_dci_dl_pdu_rel13_t *)tlv;
  return( push8(value->laa_end_partial_sf_flag, ppWritePackedMsg, end) &&
          push8(value->laa_end_partial_sf_configuration, ppWritePackedMsg, end) &&
          push8(value->initial_lbt_sf, ppWritePackedMsg, end) &&
          push8(value->codebook_size_determination, ppWritePackedMsg, end) &&
          push8(value->drms_table_flag, ppWritePackedMsg, end) &&
          push8(value->tpm_struct_flag, ppWritePackedMsg, end) &&
          (value->tpm_struct_flag == 1 ? pack_tpm_value(&(value->tpm), ppWritePackedMsg, end) : 1));
}

static uint8_t pack_dl_config_bch_pdu_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_bch_pdu_rel8_t *value = (nfapi_dl_config_bch_pdu_rel8_t *)tlv;
  //NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s()\n", __FUNCTION__);
  return( push16(value->length, ppWritePackedMsg, end) &&
          push16(value->pdu_index, ppWritePackedMsg, end) &&
          push16(value->transmission_power, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_mch_pdu_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_mch_pdu_rel8_t *value = (nfapi_dl_config_mch_pdu_rel8_t *)tlv;
  return ( push16(value->length, ppWritePackedMsg, end) &&
           push16(value->pdu_index, ppWritePackedMsg, end) &&
           push16(value->rnti, ppWritePackedMsg, end) &&
           push8(value->resource_allocation_type, ppWritePackedMsg, end) &&
           push32(value->resource_block_coding, ppWritePackedMsg, end) &&
           push8(value->modulation, ppWritePackedMsg, end) &&
           push16(value->transmission_power, ppWritePackedMsg, end) &&
           push16(value->mbsfn_area_id, ppWritePackedMsg, end));
}

static uint8_t pack_bf_vector_info(void *elem, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_bf_vector_t *bf = (nfapi_bf_vector_t *)elem;
  return ( push8(bf->subband_index, ppWritePackedMsg, end) &&
           push8(bf->num_antennas, ppWritePackedMsg, end) &&
           pusharray16(bf->bf_value, NFAPI_MAX_NUM_ANTENNAS, bf->num_antennas, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_dlsch_pdu_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel8_t *value = (nfapi_dl_config_dlsch_pdu_rel8_t *)tlv;
  return ( push16(value->length, ppWritePackedMsg, end) &&
           push16(value->pdu_index, ppWritePackedMsg, end) &&
           push16(value->rnti, ppWritePackedMsg, end) &&
           push8(value->resource_allocation_type, ppWritePackedMsg, end) &&
           push8(value->virtual_resource_block_assignment_flag, ppWritePackedMsg, end) &&
           push32(value->resource_block_coding, ppWritePackedMsg, end) &&
           push8(value->modulation, ppWritePackedMsg, end) &&
           push8(value->redundancy_version, ppWritePackedMsg, end) &&
           push8(value->transport_blocks, ppWritePackedMsg, end) &&
           push8(value->transport_block_to_codeword_swap_flag, ppWritePackedMsg, end) &&
           push8(value->transmission_scheme, ppWritePackedMsg, end) &&
           push8(value->number_of_layers, ppWritePackedMsg, end) &&
           push8(value->number_of_subbands, ppWritePackedMsg, end) &&
           pusharray8(value->codebook_index, NFAPI_MAX_NUM_SUBBANDS, value->number_of_subbands, ppWritePackedMsg, end) &&
           push8(value->ue_category_capacity, ppWritePackedMsg, end) &&
           push8(value->pa, ppWritePackedMsg, end) &&
           push8(value->delta_power_offset_index, ppWritePackedMsg, end) &&
           push8(value->ngap, ppWritePackedMsg, end) &&
           push8(value->nprb, ppWritePackedMsg, end) &&
           push8(value->transmission_mode, ppWritePackedMsg, end) &&
           push8(value->num_bf_prb_per_subband, ppWritePackedMsg, end) &&
           push8(value->num_bf_vector, ppWritePackedMsg, end) &&
           packarray(value->bf_vector, sizeof(nfapi_bf_vector_t), NFAPI_MAX_BF_VECTORS, value->num_bf_vector, ppWritePackedMsg, end, &pack_bf_vector_info));
}
static uint8_t pack_dl_config_dlsch_pdu_rel9_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel9_t *value = (nfapi_dl_config_dlsch_pdu_rel9_t *)tlv;
  return ( push8(value->nscid, ppWritePackedMsg, end) );
}
static uint8_t pack_dl_config_dlsch_pdu_rel10_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel10_t *value = (nfapi_dl_config_dlsch_pdu_rel10_t *)tlv;
  return ( push8(value->csi_rs_flag, ppWritePackedMsg, end) &&
           push8(value->csi_rs_resource_config_r10, ppWritePackedMsg, end) &&
           push16(value->csi_rs_zero_tx_power_resource_config_bitmap_r10, ppWritePackedMsg, end) &&
           push8(value->csi_rs_number_nzp_configuration, ppWritePackedMsg, end) &&
           pusharray8(value->csi_rs_resource_config, NFAPI_MAX_CSI_RS_RESOURCE_CONFIG, value->csi_rs_number_nzp_configuration, ppWritePackedMsg, end) &&
           push8(value->pdsch_start, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_dlsch_pdu_rel11_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel11_t *value = (nfapi_dl_config_dlsch_pdu_rel11_t *)tlv;
  return( push8(value->drms_config_flag, ppWritePackedMsg, end) &&
          push16(value->drms_scrambling, ppWritePackedMsg, end) &&
          push8(value->csi_config_flag, ppWritePackedMsg, end) &&
          push16(value->csi_scrambling, ppWritePackedMsg, end) &&
          push8(value->pdsch_re_mapping_flag, ppWritePackedMsg, end) &&
          push8(value->pdsch_re_mapping_atenna_ports, ppWritePackedMsg, end) &&
          push8(value->pdsch_re_mapping_freq_shift, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_dlsch_pdu_rel12_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel12_t *value = (nfapi_dl_config_dlsch_pdu_rel12_t *)tlv;
  return( push8(value->altcqi_table_r12, ppWritePackedMsg, end) &&
          push8(value->maxlayers, ppWritePackedMsg, end) &&
          push8(value->n_dl_harq, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_dlsch_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel13_t *value = (nfapi_dl_config_dlsch_pdu_rel13_t *)tlv;
  return( push8(value->dwpts_symbols, ppWritePackedMsg, end) &&
          push8(value->initial_lbt_sf, ppWritePackedMsg, end) &&
          push8(value->ue_type, ppWritePackedMsg, end) &&
          push8(value->pdsch_payload_type, ppWritePackedMsg, end) &&
          push16(value->initial_transmission_sf_io, ppWritePackedMsg, end) &&
          push8(value->drms_table_flag, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_pch_pdu_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_pch_pdu_rel8_t *value = (nfapi_dl_config_pch_pdu_rel8_t *)tlv;
  return( push16(value->length, ppWritePackedMsg, end) &&
          push16(value->pdu_index, ppWritePackedMsg, end) &&
          push16(value->p_rnti, ppWritePackedMsg, end) &&
          push8(value->resource_allocation_type, ppWritePackedMsg, end) &&
          push8(value->virtual_resource_block_assignment_flag, ppWritePackedMsg, end) &&
          push32(value->resource_block_coding, ppWritePackedMsg, end) &&
          push8(value->mcs, ppWritePackedMsg, end) &&
          push8(value->redundancy_version, ppWritePackedMsg, end) &&
          push8(value->number_of_transport_blocks, ppWritePackedMsg, end) &&
          push8(value->transport_block_to_codeword_swap_flag, ppWritePackedMsg, end) &&
          push8(value->transmission_scheme, ppWritePackedMsg, end) &&
          push8(value->number_of_layers, ppWritePackedMsg, end) &&
          push8(value->codebook_index, ppWritePackedMsg, end) &&
          push8(value->ue_category_capacity, ppWritePackedMsg, end) &&
          push8(value->pa, ppWritePackedMsg, end) &&
          push16(value->transmission_power, ppWritePackedMsg, end) &&
          push8(value->nprb, ppWritePackedMsg, end) &&
          push8(value->ngap, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_pch_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_pch_pdu_rel13_t *value = (nfapi_dl_config_pch_pdu_rel13_t *)tlv;
  return ( push8(value->ue_mode, ppWritePackedMsg, end) &&
           push16(value->initial_transmission_sf_io, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_prs_pdu_rel9_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_prs_pdu_rel9_t *value = (nfapi_dl_config_prs_pdu_rel9_t *)tlv;
  return( push16(value->transmission_power, ppWritePackedMsg, end) &&
          push8(value->prs_bandwidth, ppWritePackedMsg, end) &&
          push8(value->prs_cyclic_prefix_type, ppWritePackedMsg, end) &&
          push8(value->prs_muting, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_csi_rs_pdu_rel10_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_csi_rs_pdu_rel10_t *value = (nfapi_dl_config_csi_rs_pdu_rel10_t *)tlv;
  return( push8(value->csi_rs_antenna_port_count_r10, ppWritePackedMsg, end) &&
          push8(value->csi_rs_resource_config_r10, ppWritePackedMsg, end) &&
          push16(value->transmission_power, ppWritePackedMsg, end) &&
          push16(value->csi_rs_zero_tx_power_resource_config_bitmap_r10, ppWritePackedMsg, end) &&
          push8(value->csi_rs_number_of_nzp_configuration, ppWritePackedMsg, end) &&
          pusharray8(value->csi_rs_resource_config, NFAPI_MAX_CSI_RS_RESOURCE_CONFIG, value->csi_rs_number_of_nzp_configuration, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_csi_rs_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_csi_rs_pdu_rel13_t *value = (nfapi_dl_config_csi_rs_pdu_rel13_t *)tlv;

  if(!(push8(value->csi_rs_class, ppWritePackedMsg, end) &&
       push8(value->cdm_type, ppWritePackedMsg, end) &&
       push8(value->num_bf_vector, ppWritePackedMsg, end))) {
    return 0;
  }

  uint16_t i;

  for(i = 0; i < value->num_bf_vector; ++i) {
    if(!(push8(value->bf_vector[i].csi_rs_resource_index, ppWritePackedMsg, end) &&
         pusharray16(value->bf_vector[i].bf_value, NFAPI_MAX_ANTENNA_PORT_COUNT, NFAPI_MAX_ANTENNA_PORT_COUNT, ppWritePackedMsg, end)))
      return 0;
  }

  return 1;
}
static uint8_t pack_bf_vector(nfapi_bf_vector_t *vector, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( push8(vector->subband_index, ppWritePackedMsg, end) &&
           push8(vector->num_antennas, ppWritePackedMsg, end) &&
           pusharray16(vector->bf_value, NFAPI_MAX_NUM_ANTENNAS, vector->num_antennas, ppWritePackedMsg, end));
}

static uint8_t pack_dl_config_epdcch_parameters_rel11_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_epdcch_parameters_rel11_t *value = (nfapi_dl_config_epdcch_parameters_rel11_t *)tlv;
  return ( push8(value->epdcch_resource_assignment_flag, ppWritePackedMsg, end) &&
           push16(value->epdcch_id, ppWritePackedMsg, end) &&
           push8(value->epdcch_start_symbol, ppWritePackedMsg, end) &&
           push8(value->epdcch_num_prb, ppWritePackedMsg, end) &&
           pusharray8(value->epdcch_prb_index, NFAPI_MAX_EPDCCH_PRB, value->epdcch_num_prb, ppWritePackedMsg, end) &&
           pack_bf_vector(&value->bf_vector, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_epdcch_parameters_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_epdcch_parameters_rel13_t *value = (nfapi_dl_config_epdcch_parameters_rel13_t *)tlv;
  return (push8(value->dwpts_symbols, ppWritePackedMsg, end) &&
          push8(value->initial_lbt_sf, ppWritePackedMsg, end));
}
static uint8_t pack_dl_config_mpdcch_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_mpdcch_pdu_rel13_t *value = (nfapi_dl_config_mpdcch_pdu_rel13_t *)tlv;
  return ( push8(value->mpdcch_narrow_band, ppWritePackedMsg, end) &&
           push8(value->number_of_prb_pairs, ppWritePackedMsg, end) &&
           push8(value->resource_block_assignment, ppWritePackedMsg, end) &&
           push8(value->mpdcch_tansmission_type, ppWritePackedMsg, end) &&
           push8(value->start_symbol, ppWritePackedMsg, end) &&
           push8(value->ecce_index, ppWritePackedMsg, end) &&
           push8(value->aggregation_level, ppWritePackedMsg, end) &&
           push8(value->rnti_type, ppWritePackedMsg, end) &&
           push16(value->rnti, ppWritePackedMsg, end) &&
           push8(value->ce_mode, ppWritePackedMsg, end) &&
           push16(value->drms_scrambling_init, ppWritePackedMsg, end) &&
           push16(value->initial_transmission_sf_io, ppWritePackedMsg, end) &&
           push16(value->transmission_power, ppWritePackedMsg, end) &&
           push8(value->dci_format, ppWritePackedMsg, end) &&
           push16(value->resource_block_coding, ppWritePackedMsg, end) &&
           push8(value->mcs, ppWritePackedMsg, end) &&
           push8(value->pdsch_reptition_levels, ppWritePackedMsg, end) &&
           push8(value->redundancy_version, ppWritePackedMsg, end) &&
           push8(value->new_data_indicator, ppWritePackedMsg, end) &&
           push8(value->harq_process, ppWritePackedMsg, end) &&
           push8(value->tpmi_length, ppWritePackedMsg, end) &&
           push8(value->tpmi, ppWritePackedMsg, end) &&
           push8(value->pmi_flag, ppWritePackedMsg, end) &&
           push8(value->pmi, ppWritePackedMsg, end) &&
           push8(value->harq_resource_offset, ppWritePackedMsg, end) &&
           push8(value->dci_subframe_repetition_number, ppWritePackedMsg, end) &&
           push8(value->tpc, ppWritePackedMsg, end) &&
           push8(value->downlink_assignment_index_length, ppWritePackedMsg, end) &&
           push8(value->downlink_assignment_index, ppWritePackedMsg, end) &&
           push8(value->allocate_prach_flag, ppWritePackedMsg, end) &&
           push8(value->preamble_index, ppWritePackedMsg, end) &&
           push8(value->prach_mask_index, ppWritePackedMsg, end) &&
           push8(value->starting_ce_level, ppWritePackedMsg, end) &&
           push8(value->srs_request, ppWritePackedMsg, end) &&
           push8(value->antenna_ports_and_scrambling_identity_flag, ppWritePackedMsg, end) &&
           push8(value->antenna_ports_and_scrambling_identity, ppWritePackedMsg, end) &&
           push8(value->frequency_hopping_enabled_flag, ppWritePackedMsg, end) &&
           push8(value->paging_direct_indication_differentiation_flag, ppWritePackedMsg, end) &&
           push8(value->direct_indication, ppWritePackedMsg, end) &&
           push8(value->total_dci_length_including_padding, ppWritePackedMsg, end) &&
           push8(value->number_of_tx_antenna_ports, ppWritePackedMsg, end) &&
           pusharray16(value->precoding_value, NFAPI_MAX_TX_PHYSICAL_ANTENNA_PORTS, value->number_of_tx_antenna_ports, ppWritePackedMsg, end));
}


static uint8_t pack_dl_config_nbch_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_nbch_pdu_rel13_t *value = (nfapi_dl_config_nbch_pdu_rel13_t *)tlv;
  return (push16(value->length, ppWritePackedMsg, end) &&
          push16(value->pdu_index, ppWritePackedMsg, end) &&
          push16(value->transmission_power, ppWritePackedMsg, end) &&
          push16(value->hyper_sfn_2_lsbs, ppWritePackedMsg, end));
}


static uint8_t pack_dl_config_npdcch_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_npdcch_pdu_rel13_t *value = (nfapi_dl_config_npdcch_pdu_rel13_t *)tlv;
  return (push16(value->length, ppWritePackedMsg, end) &&
          push16(value->pdu_index, ppWritePackedMsg, end) &&
          push8(value->ncce_index, ppWritePackedMsg, end) &&
          push8(value->aggregation_level, ppWritePackedMsg, end) &&
          push8(value->start_symbol, ppWritePackedMsg, end) &&
          push8(value->rnti_type, ppWritePackedMsg, end) &&
          push16(value->rnti, ppWritePackedMsg, end) &&
          push8(value->scrambling_reinitialization_batch_index, ppWritePackedMsg, end) &&
          push8(value->nrs_antenna_ports_assumed_by_the_ue, ppWritePackedMsg, end) &&
          push8(value->dci_format, ppWritePackedMsg, end) &&
          push8(value->scheduling_delay, ppWritePackedMsg, end) &&
          push8(value->resource_assignment, ppWritePackedMsg, end) &&
          push8(value->repetition_number, ppWritePackedMsg, end) &&
          push8(value->mcs, ppWritePackedMsg, end) &&
          push8(value->new_data_indicator, ppWritePackedMsg, end) &&
          push8(value->harq_ack_resource, ppWritePackedMsg, end) &&
          push8(value->npdcch_order_indication, ppWritePackedMsg, end) &&
          push8(value->starting_number_of_nprach_repetitions, ppWritePackedMsg, end) &&
          push8(value->subcarrier_indication_of_nprach, ppWritePackedMsg, end) &&
          push8(value->paging_direct_indication_differentation_flag, ppWritePackedMsg, end) &&
          push8(value->direct_indication, ppWritePackedMsg, end) &&
          push8(value->dci_subframe_repetition_number, ppWritePackedMsg, end) &&
          push8(value->total_dci_length_including_padding, ppWritePackedMsg, end));
}

static uint8_t pack_dl_config_ndlsch_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_ndlsch_pdu_rel13_t *value = (nfapi_dl_config_ndlsch_pdu_rel13_t *)tlv;
  return (push16(value->length, ppWritePackedMsg, end) &&
          push16(value->pdu_index, ppWritePackedMsg, end) &&
          push8(value->start_symbol, ppWritePackedMsg, end) &&
          push8(value->rnti_type, ppWritePackedMsg, end) &&
          push16(value->rnti, ppWritePackedMsg, end) &&
          push16(value->resource_assignment, ppWritePackedMsg, end) &&
          push16(value->repetition_number, ppWritePackedMsg, end) &&
          push8(value->modulation, ppWritePackedMsg, end) &&
          push8(value->number_of_subframes_for_resource_assignment, ppWritePackedMsg, end) &&
          push8(value->scrambling_sequence_initialization_cinit, ppWritePackedMsg, end) &&
          push16(value->sf_idx, ppWritePackedMsg, end) &&
          push8(value->nrs_antenna_ports_assumed_by_the_ue, ppWritePackedMsg, end));
}


static uint8_t pack_dl_tti_request_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nr_dl_tti_request_pdu_t *value = (nfapi_nr_dl_tti_request_pdu_t *)tlv;

  if(!(push16(value->PDUSize, ppWritePackedMsg, end) &&
       push16(value->PDUType, ppWritePackedMsg, end) ))
    return 0;

  // first match the pdu type, then call the respective function
  switch(value->PDUType) {
    case NFAPI_NR_DL_TTI_CSI_RS_PDU_TYPE: {
      if(!(pack_dl_tti_csi_rs_pdu_rel15_value(&value->csi_rs_pdu.csi_rs_pdu_rel15,ppWritePackedMsg,end)))
        return 0;
    }
    break;

    case NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE: {
      if(!(pack_dl_tti_pdcch_pdu_rel15_value(&value->pdcch_pdu.pdcch_pdu_rel15,ppWritePackedMsg,end)))
        return 0;
    }
    break;

    case NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE: {
      if(!(pack_dl_tti_pdsch_pdu_rel15_value(&value->pdsch_pdu.pdsch_pdu_rel15,ppWritePackedMsg,end)))
        return 0;
    }
    break;

    case NFAPI_NR_DL_TTI_SSB_PDU_TYPE: {
      if(!(pack_dl_tti_ssb_pdu_rel15_value(&value->ssb_pdu.ssb_pdu_rel15,ppWritePackedMsg,end)))
        return 0;
    }
    break;

    default: {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid DL_TTI pdu type %d \n", value->PDUType );
    }
    break;
  }

  return 1;
}

static uint8_t pack_dl_config_request_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_dl_config_request_body_t *value = (nfapi_dl_config_request_body_t *)tlv;

  //NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() dci:%u pdu:%u pdsch:%u rnti:%u pcfich:%u\n", __FUNCTION__, value->number_dci, value->number_pdu, value->number_pdsch_rnti, value->transmission_power_pcfich);

  if(!(push8(value->number_pdcch_ofdm_symbols, ppWritePackedMsg, end) &&
       push8(value->number_dci, ppWritePackedMsg, end) &&
       push16(value->number_pdu, ppWritePackedMsg, end) &&
       push8(value->number_pdsch_rnti, ppWritePackedMsg, end) &&
       push16(value->transmission_power_pcfich, ppWritePackedMsg, end))) {
    return 0;
  }

  uint16_t i = 0;
  uint16_t total_number_of_pdus = value->number_pdu;

  for(; i < total_number_of_pdus; ++i) {
    nfapi_dl_config_request_pdu_t *pdu = &(value->dl_config_pdu_list[i]);

    if(push8(pdu->pdu_type, ppWritePackedMsg, end) == 0)
      return 0;

    // Put a 0 size in and then determine the size after the pdu
    // has been writen and write the calculated size
    uint8_t *pWritePackedMsgPduSize = *ppWritePackedMsg;
    pdu->pdu_size = 0;

    if(push8(pdu->pdu_size, ppWritePackedMsg, end) == 0)
      return 0;

    switch(pdu->pdu_type) {
      case NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE: {
        //NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE\n", __FUNCTION__);
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel8, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel8_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL9_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel9, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel9_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL10_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel10, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel10_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL11_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel11, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel11_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL12_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel12, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel12_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL13_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel13, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel13_value))) {
          return 0;
        }
      }
      break;

      case NFAPI_DL_CONFIG_BCH_PDU_TYPE: {
        //NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() NFAPI_DL_CONFIG_BCH_PDU_TYPE\n", __FUNCTION__);
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_BCH_PDU_REL8_TAG, &pdu->bch_pdu.bch_pdu_rel8, ppWritePackedMsg, end, &pack_dl_config_bch_pdu_rel8_value)))
          return 0;
      }
      break;

      case NFAPI_DL_CONFIG_MCH_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_MCH_PDU_REL8_TAG, &pdu->mch_pdu.mch_pdu_rel8, ppWritePackedMsg, end, &pack_dl_config_mch_pdu_rel8_value)))
          return 0;
      }
      break;

      case NFAPI_DL_CONFIG_DLSCH_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL8_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel8, ppWritePackedMsg, end, &pack_dl_config_dlsch_pdu_rel8_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL9_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel9, ppWritePackedMsg, end, &pack_dl_config_dlsch_pdu_rel9_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL10_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel10, ppWritePackedMsg, end, &pack_dl_config_dlsch_pdu_rel10_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL11_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel11, ppWritePackedMsg, end, &pack_dl_config_dlsch_pdu_rel11_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL12_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel12, ppWritePackedMsg, end, &pack_dl_config_dlsch_pdu_rel12_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL13_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel13, ppWritePackedMsg, end, &pack_dl_config_dlsch_pdu_rel13_value)))
          return 0;
      }
      break;

      case NFAPI_DL_CONFIG_PCH_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_PCH_PDU_REL8_TAG, &pdu->pch_pdu.pch_pdu_rel8, ppWritePackedMsg, end, &pack_dl_config_pch_pdu_rel8_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_PCH_PDU_REL13_TAG, &pdu->pch_pdu.pch_pdu_rel13, ppWritePackedMsg, end, &pack_dl_config_pch_pdu_rel13_value)))
          return 0;
      }
      break;

      case NFAPI_DL_CONFIG_PRS_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_PRS_PDU_REL9_TAG, &pdu->prs_pdu.prs_pdu_rel9, ppWritePackedMsg, end, &pack_dl_config_prs_pdu_rel9_value)))
          return 0;
      }
      break;

      case NFAPI_DL_CONFIG_CSI_RS_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_CSI_RS_PDU_REL10_TAG, &pdu->csi_rs_pdu.csi_rs_pdu_rel10, ppWritePackedMsg, end,  &pack_dl_config_csi_rs_pdu_rel10_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_CSI_RS_PDU_REL13_TAG, &pdu->csi_rs_pdu.csi_rs_pdu_rel13, ppWritePackedMsg, end,  &pack_dl_config_csi_rs_pdu_rel13_value)))
          return 0;
      }
      break;

      case NFAPI_DL_CONFIG_EPDCCH_DL_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL8_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel8, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel8_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL9_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel9, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel9_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL10_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel10, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel10_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL11_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel11, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel11_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL12_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel12, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel12_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL13_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel13, ppWritePackedMsg, end, &pack_dl_config_dci_dl_pdu_rel13_value) &&
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_EPDCCH_PARAM_REL11_TAG, &pdu->epdcch_pdu.epdcch_params_rel11, ppWritePackedMsg, end, &pack_dl_config_epdcch_parameters_rel11_value) &
             pack_tlv(NFAPI_DL_CONFIG_REQUEST_EPDCCH_PARAM_REL13_TAG, &pdu->epdcch_pdu.epdcch_params_rel13, ppWritePackedMsg, end, &pack_dl_config_epdcch_parameters_rel13_value)))
          return 0;
      }
      break;

      case NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_MPDCCH_PDU_REL13_TAG, &pdu->mpdcch_pdu.mpdcch_pdu_rel13, ppWritePackedMsg, end, &pack_dl_config_mpdcch_pdu_rel13_value)))
          return 0;
      }
      break;

      case NFAPI_DL_CONFIG_NBCH_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_NBCH_PDU_REL13_TAG, &pdu->nbch_pdu.nbch_pdu_rel13, ppWritePackedMsg, end, &pack_dl_config_nbch_pdu_rel13_value)))
          return 0;
      }
      break;

      case NFAPI_DL_CONFIG_NPDCCH_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_NPDCCH_PDU_REL13_TAG, &pdu->npdcch_pdu.npdcch_pdu_rel13, ppWritePackedMsg, end, &pack_dl_config_npdcch_pdu_rel13_value)))
          return 0;
      }
      break;

      case NFAPI_DL_CONFIG_NDLSCH_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_DL_CONFIG_REQUEST_NDLSCH_PDU_REL13_TAG, &pdu->ndlsch_pdu.ndlsch_pdu_rel13, ppWritePackedMsg, end, &pack_dl_config_ndlsch_pdu_rel13_value)))
          return 0;
      }
      break;

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid pdu type %d \n", pdu->pdu_type );
      }
      break;
    };

    // add 1 for the pdu_type. The delta will include the pdu_size
    pdu->pdu_size = 1 + (*ppWritePackedMsg - pWritePackedMsgPduSize);

    push8(pdu->pdu_size, &pWritePackedMsgPduSize, end);
  }

  return 1;
}


static uint8_t pack_dl_tti_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nr_dl_tti_request_t *pNfapiMsg = (nfapi_nr_dl_tti_request_t *)msg;

  if (!(push16(pNfapiMsg->SFN, ppWritePackedMsg, end) &&
        push16(pNfapiMsg->Slot, ppWritePackedMsg, end) &&
        push8(pNfapiMsg->dl_tti_request_body.nGroup, ppWritePackedMsg, end) &&
        push8(pNfapiMsg->dl_tti_request_body.nPDUs, ppWritePackedMsg, end) &&
        pusharray8(pNfapiMsg->dl_tti_request_body.nUe,256,pNfapiMsg->dl_tti_request_body.nGroup, ppWritePackedMsg, end)
        //pusharray8(pNfapiMsg->PduIdx[0] ,256,256, ppWritePackedMsg, end)
       ))
    return 0;

  int arr[12];

  for(int i=0; i<pNfapiMsg->dl_tti_request_body.nGroup; i++) {
    for(int j=0; j<pNfapiMsg->dl_tti_request_body.nUe[i]; j++) {
      arr[j] = pNfapiMsg->dl_tti_request_body.PduIdx[i][j];
    }

    if(!(pusharrays32(arr,12,pNfapiMsg->dl_tti_request_body.nUe[i],ppWritePackedMsg, end)))
      return 0;
  }

  for(int i=0; i<pNfapiMsg->dl_tti_request_body.nPDUs; i++) {
    if(!pack_dl_tti_request_body_value(&pNfapiMsg->dl_tti_request_body.dl_tti_pdu_list[i],ppWritePackedMsg,end))
      return 0;
  }

  return 1;
}


static uint8_t pack_dl_config_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_dl_config_request_t *pNfapiMsg = (nfapi_dl_config_request_t *)msg;
  //return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
  //pack_tlv(NFAPI_DL_CONFIG_REQUEST_BODY_TAG, &pNfapiMsg->dl_config_request_body, ppWritePackedMsg, end, &pack_dl_config_request_body_value) &&
  //pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
  {
    uint8_t x = push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end);
    uint8_t y = pack_tlv(NFAPI_DL_CONFIG_REQUEST_BODY_TAG, &pNfapiMsg->dl_config_request_body, ppWritePackedMsg, end, &pack_dl_config_request_body_value);
    uint8_t z = pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config);

    if (!x || !y || !z) {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() NFAPI_DL_CONFIG_REQUEST x:%u y:%u z:%u \n", __FUNCTION__,x,y,z);
    }

    return x && y && z;
  }
}




static uint8_t pack_ul_config_request_ulsch_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_pdu_rel8_t *ulsch_pdu_rel8 = (nfapi_ul_config_ulsch_pdu_rel8_t *)tlv;
  return( push32(ulsch_pdu_rel8->handle, ppWritePackedMsg, end) &&
          push16(ulsch_pdu_rel8->size, ppWritePackedMsg, end) &&
          push16(ulsch_pdu_rel8->rnti, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->resource_block_start, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->number_of_resource_blocks, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->modulation_type, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->cyclic_shift_2_for_drms, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->frequency_hopping_enabled_flag, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->frequency_hopping_bits, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->new_data_indication, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->redundancy_version, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->harq_process_number, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->ul_tx_mode, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->current_tx_nb, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel8->n_srs, ppWritePackedMsg, end));
}
static uint8_t pack_ul_config_request_ulsch_rel10_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_pdu_rel10_t *ulsch_pdu_rel10 = (nfapi_ul_config_ulsch_pdu_rel10_t *)tlv;
  return (push8(ulsch_pdu_rel10->resource_allocation_type, ppWritePackedMsg, end) &&
          push32(ulsch_pdu_rel10->resource_block_coding, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel10->transport_blocks, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel10->transmission_scheme, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel10->number_of_layers, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel10->codebook_index, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel10->disable_sequence_hopping_flag, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_ulsch_rel11_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_pdu_rel11_t *ulsch_pdu_rel11 = (nfapi_ul_config_ulsch_pdu_rel11_t *)tlv;
  return (push8(ulsch_pdu_rel11->virtual_cell_id_enabled_flag, ppWritePackedMsg, end) &&
          push16(ulsch_pdu_rel11->npusch_identity, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel11->dmrs_config_flag, ppWritePackedMsg, end) &&
          push16(ulsch_pdu_rel11->ndmrs_csh_identity, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_ulsch_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_pdu_rel13_t *ulsch_pdu_rel13 = (nfapi_ul_config_ulsch_pdu_rel13_t *)tlv;
  return (push8(ulsch_pdu_rel13->ue_type, ppWritePackedMsg, end) &&
          push16(ulsch_pdu_rel13->total_number_of_repetitions, ppWritePackedMsg, end) &&
          push16(ulsch_pdu_rel13->repetition_number, ppWritePackedMsg, end) &&
          push16(ulsch_pdu_rel13->initial_transmission_sf_io, ppWritePackedMsg, end) &&
          push8(ulsch_pdu_rel13->empty_symbols_due_to_re_tunning, ppWritePackedMsg, end));
}

//Pack fns for ul_tti PDUS


static uint8_t pack_ul_tti_request_prach_pdu(nfapi_nr_prach_pdu_t *prach_pdu, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return(
          push16(prach_pdu->phys_cell_id, ppWritePackedMsg, end) &&
          push8(prach_pdu->num_prach_ocas, ppWritePackedMsg, end) &&
          push8(prach_pdu->prach_format, ppWritePackedMsg, end) &&
          push8(prach_pdu->num_ra, ppWritePackedMsg, end) &&
          push8(prach_pdu->prach_start_symbol, ppWritePackedMsg, end) &&
          push16(prach_pdu->num_cs, ppWritePackedMsg, end)
          // TODO: ignoring beamforming tlv for now
        );
}

static uint8_t pack_ul_tti_request_pucch_pdu(nfapi_nr_pucch_pdu_t *pucch_pdu, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return(
          push16(pucch_pdu->rnti, ppWritePackedMsg, end) &&
          push32(pucch_pdu->handle, ppWritePackedMsg, end) &&
          push16(pucch_pdu->bwp_size, ppWritePackedMsg, end) &&
          push16(pucch_pdu->bwp_start, ppWritePackedMsg, end) &&
          push8(pucch_pdu->subcarrier_spacing, ppWritePackedMsg, end) &&
          push8(pucch_pdu->cyclic_prefix, ppWritePackedMsg, end) &&
          push8(pucch_pdu->format_type, ppWritePackedMsg, end) &&
          push8(pucch_pdu->multi_slot_tx_indicator, ppWritePackedMsg, end) &&
          push16(pucch_pdu->prb_start, ppWritePackedMsg, end) &&
          push16(pucch_pdu->prb_size, ppWritePackedMsg, end) &&
          push8(pucch_pdu->start_symbol_index, ppWritePackedMsg, end) &&
          push8(pucch_pdu->nr_of_symbols, ppWritePackedMsg, end) &&
          push8(pucch_pdu->freq_hop_flag, ppWritePackedMsg, end) &&
          push16(pucch_pdu->second_hop_prb, ppWritePackedMsg, end) &&
          push8(pucch_pdu->group_hop_flag, ppWritePackedMsg, end) &&
          push8(pucch_pdu->sequence_hop_flag, ppWritePackedMsg, end) &&
          push16(pucch_pdu->hopping_id, ppWritePackedMsg, end) &&
          push16(pucch_pdu->initial_cyclic_shift, ppWritePackedMsg, end) &&
          push16(pucch_pdu->data_scrambling_id, ppWritePackedMsg, end) &&
          push8(pucch_pdu->time_domain_occ_idx, ppWritePackedMsg, end) &&
          push8(pucch_pdu->pre_dft_occ_idx, ppWritePackedMsg, end) &&
          push8(pucch_pdu->pre_dft_occ_len, ppWritePackedMsg, end) &&
          push8(pucch_pdu->add_dmrs_flag, ppWritePackedMsg, end) &&
          push16(pucch_pdu->dmrs_scrambling_id, ppWritePackedMsg, end) &&
          push8(pucch_pdu->dmrs_cyclic_shift, ppWritePackedMsg, end) &&
          push8(pucch_pdu->sr_flag, ppWritePackedMsg, end) &&
          push8(pucch_pdu->bit_len_harq, ppWritePackedMsg, end) &&
          push16(pucch_pdu->bit_len_csi_part1, ppWritePackedMsg, end) &&
          push16(pucch_pdu->bit_len_csi_part2, ppWritePackedMsg, end)
          // TODO: ignoring beamforming tlv for now
        );
}


static uint8_t pack_ul_tti_request_pusch_pdu(nfapi_nr_pusch_pdu_t *pusch_pdu, uint8_t **ppWritePackedMsg, uint8_t *end) {
  if (!(
        push16(pusch_pdu->pdu_bit_map, ppWritePackedMsg, end) &&
        push16(pusch_pdu->rnti, ppWritePackedMsg, end) &&
        push32(pusch_pdu->handle, ppWritePackedMsg, end) &&
        push16(pusch_pdu->bwp_size, ppWritePackedMsg, end) &&
        push16(pusch_pdu->bwp_start, ppWritePackedMsg, end) &&
        push8(pusch_pdu->subcarrier_spacing, ppWritePackedMsg, end) &&
        push8(pusch_pdu->cyclic_prefix, ppWritePackedMsg, end) &&
        push16(pusch_pdu->target_code_rate, ppWritePackedMsg, end) &&
        push8(pusch_pdu->qam_mod_order, ppWritePackedMsg, end) &&
        push8(pusch_pdu->mcs_index, ppWritePackedMsg, end) &&
        push8(pusch_pdu->mcs_table, ppWritePackedMsg, end) &&
        push8(pusch_pdu->transform_precoding, ppWritePackedMsg, end) &&
        push16(pusch_pdu->data_scrambling_id, ppWritePackedMsg, end) &&
        push8(pusch_pdu->nrOfLayers, ppWritePackedMsg, end) &&
        push16(pusch_pdu->ul_dmrs_symb_pos, ppWritePackedMsg, end) &&
        push8(pusch_pdu->dmrs_config_type, ppWritePackedMsg, end) &&
        push16(pusch_pdu->ul_dmrs_scrambling_id, ppWritePackedMsg, end) &&
        push8(pusch_pdu->scid, ppWritePackedMsg, end) &&
        push8(pusch_pdu->num_dmrs_cdm_grps_no_data, ppWritePackedMsg, end) &&
        push16(pusch_pdu->dmrs_ports, ppWritePackedMsg, end) &&
        push8(pusch_pdu->resource_alloc, ppWritePackedMsg, end) &&
        push8(pusch_pdu->resource_alloc,ppWritePackedMsg, end) &&
        push16(pusch_pdu->dmrs_ports, ppWritePackedMsg, end) &&
        push16(pusch_pdu->rb_start, ppWritePackedMsg, end) &&
        push16(pusch_pdu->rb_size, ppWritePackedMsg, end) &&
        push8(pusch_pdu->vrb_to_prb_mapping, ppWritePackedMsg, end) &&
        push8(pusch_pdu->frequency_hopping, ppWritePackedMsg, end) &&
        push16(pusch_pdu->tx_direct_current_location, ppWritePackedMsg, end) &&
        push8(pusch_pdu->uplink_frequency_shift_7p5khz, ppWritePackedMsg, end) &&
        push8(pusch_pdu->start_symbol_index, ppWritePackedMsg, end) &&
        push8(pusch_pdu->nr_of_symbols, ppWritePackedMsg, end)
        // TODO: ignoring beamforming tlv for now
      ))
    return 0;

  //Pack Optional Data only included if indicated in pduBitmap
  switch(pusch_pdu->pdu_bit_map) {
    case PUSCH_PDU_BITMAP_PUSCH_DATA: {
      // pack optional TLVs
      return(
              push8(pusch_pdu->pusch_data.rv_index, ppWritePackedMsg, end) &&
              push8(pusch_pdu->pusch_data.harq_process_id, ppWritePackedMsg, end) &&
              push32(pusch_pdu->pusch_data.tb_size, ppWritePackedMsg, end) &&
              push16(pusch_pdu->pusch_data.num_cb, ppWritePackedMsg, end) &&
              pusharray8(pusch_pdu->pusch_data.cb_present_and_position,1,1,ppWritePackedMsg, end)
            );
    }
    break;

    case PUSCH_PDU_BITMAP_PUSCH_UCI: {
      return(
              push16(pusch_pdu->pusch_uci.harq_ack_bit_length, ppWritePackedMsg, end) &&
              push16(pusch_pdu->pusch_uci.csi_part1_bit_length, ppWritePackedMsg, end) &&
              push16(pusch_pdu->pusch_uci.csi_part2_bit_length, ppWritePackedMsg, end) &&
              push8(pusch_pdu->pusch_uci.alpha_scaling, ppWritePackedMsg, end) &&
              push8(pusch_pdu->pusch_uci.beta_offset_harq_ack, ppWritePackedMsg, end) &&
              push8(pusch_pdu->pusch_uci.beta_offset_csi1, ppWritePackedMsg, end) &&
              push8(pusch_pdu->pusch_uci.beta_offset_csi2, ppWritePackedMsg, end)
            );
    }
    break;

    case PUSCH_PDU_BITMAP_PUSCH_PTRS: {
      return(
              push8(pusch_pdu->pusch_ptrs.num_ptrs_ports, ppWritePackedMsg, end) &&
              push8(pusch_pdu->pusch_ptrs.ptrs_ports_list->ptrs_dmrs_port, ppWritePackedMsg, end) &&
              push16(pusch_pdu->pusch_ptrs.ptrs_ports_list->ptrs_port_index, ppWritePackedMsg, end) &&
              push8(pusch_pdu->pusch_ptrs.ptrs_ports_list->ptrs_re_offset, ppWritePackedMsg, end) &&
              push8(pusch_pdu->pusch_ptrs.ptrs_time_density, ppWritePackedMsg, end) &&
              push8(pusch_pdu->pusch_ptrs.ptrs_freq_density, ppWritePackedMsg, end) &&
              push8(pusch_pdu->pusch_ptrs.ul_ptrs_power, ppWritePackedMsg, end)
            );
    }
    break;

    case PUSCH_PDU_BITMAP_DFTS_OFDM: {
      return(
              push8(pusch_pdu->dfts_ofdm.low_papr_group_number, ppWritePackedMsg, end) &&
              push16(pusch_pdu->dfts_ofdm.low_papr_sequence_number, ppWritePackedMsg, end) &&
              push8(pusch_pdu->dfts_ofdm.ul_ptrs_sample_density, ppWritePackedMsg, end) &&
              push8(pusch_pdu->dfts_ofdm.ul_ptrs_time_density_transform_precoding, ppWritePackedMsg, end)
            );
    }
    break;

    default: {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "Invalid pdu bitmap %d \n", pusch_pdu->pdu_bit_map );
    }
  }

  return 1;
}

static uint8_t pack_ul_tti_request_srs_pdu(nfapi_nr_srs_pdu_t *srs_pdu, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return(
          push16(srs_pdu->rnti, ppWritePackedMsg, end) &&
          push32(srs_pdu->handle, ppWritePackedMsg, end) &&
          push16(srs_pdu->bwp_size, ppWritePackedMsg, end) &&
          push16(srs_pdu->bwp_start, ppWritePackedMsg, end) &&
          push8(srs_pdu->subcarrier_spacing, ppWritePackedMsg, end) &&
          push8(srs_pdu->cyclic_prefix, ppWritePackedMsg, end) &&
          push8(srs_pdu->num_ant_ports, ppWritePackedMsg, end) &&
          push8(srs_pdu->num_symbols, ppWritePackedMsg, end) &&
          push8(srs_pdu->num_repetitions, ppWritePackedMsg, end) &&
          push8(srs_pdu->time_start_position, ppWritePackedMsg, end) &&
          push8(srs_pdu->config_index, ppWritePackedMsg, end) &&
          push16(srs_pdu->sequence_id, ppWritePackedMsg, end) &&
          push8(srs_pdu->bandwidth_index, ppWritePackedMsg, end) &&
          push8(srs_pdu->comb_size, ppWritePackedMsg, end) &&
          push8(srs_pdu->comb_offset, ppWritePackedMsg, end) &&
          push8(srs_pdu->cyclic_shift, ppWritePackedMsg, end) &&
          push8(srs_pdu->frequency_position, ppWritePackedMsg, end) &&
          push8(srs_pdu->frequency_shift, ppWritePackedMsg, end) &&
          push8(srs_pdu->frequency_hopping, ppWritePackedMsg, end) &&
          push8(srs_pdu->group_or_sequence_hopping, ppWritePackedMsg, end) &&
          push8(srs_pdu->resource_type, ppWritePackedMsg, end) &&
          push16(srs_pdu->t_srs, ppWritePackedMsg, end) &&
          push16(srs_pdu->t_offset, ppWritePackedMsg, end)
          // TODO: ignoring beamforming tlv for now
        );
}

static uint8_t pack_ul_config_request_ulsch_pdu(nfapi_ul_config_ulsch_pdu *ulsch_pdu, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( pack_tlv(NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL8_TAG, &ulsch_pdu->ulsch_pdu_rel8, ppWritePackedMsg, end, &pack_ul_config_request_ulsch_rel8_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL10_TAG, &ulsch_pdu->ulsch_pdu_rel10, ppWritePackedMsg, end, &pack_ul_config_request_ulsch_rel10_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL11_TAG, &ulsch_pdu->ulsch_pdu_rel11, ppWritePackedMsg, end, &pack_ul_config_request_ulsch_rel11_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL13_TAG, &ulsch_pdu->ulsch_pdu_rel13, ppWritePackedMsg, end, &pack_ul_config_request_ulsch_rel13_value));
}

static uint8_t pack_ul_config_request_cqi_ri_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_ri_information_rel8_t *cqi_ri_info_rel8 = (nfapi_ul_config_cqi_ri_information_rel8_t *)tlv;
  return ( push8(cqi_ri_info_rel8->dl_cqi_pmi_size_rank_1, ppWritePackedMsg, end) &&
           push8(cqi_ri_info_rel8->dl_cqi_pmi_size_rank_greater_1, ppWritePackedMsg, end) &&
           push8(cqi_ri_info_rel8->ri_size, ppWritePackedMsg, end) &&
           push8(cqi_ri_info_rel8->delta_offset_cqi, ppWritePackedMsg, end) &&
           push8(cqi_ri_info_rel8->delta_offset_ri, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_cqi_ri_rel9_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_ri_information_rel9_t *cqi_ri_info_rel9 = (nfapi_ul_config_cqi_ri_information_rel9_t *)tlv;

  if(!(push8(cqi_ri_info_rel9->report_type, ppWritePackedMsg, end) &&
       push8(cqi_ri_info_rel9->delta_offset_cqi, ppWritePackedMsg, end) &&
       push8(cqi_ri_info_rel9->delta_offset_ri, ppWritePackedMsg, end))) {
    return 0;
  }

  switch(cqi_ri_info_rel9->report_type) {
    case NFAPI_CSI_REPORT_TYPE_PERIODIC: {
      if(!(push8(cqi_ri_info_rel9->periodic_cqi_pmi_ri_report.dl_cqi_pmi_ri_size, ppWritePackedMsg, end) &&
           push8(cqi_ri_info_rel9->periodic_cqi_pmi_ri_report.control_type, ppWritePackedMsg, end))) {
        return 0;
      }
    }
    break;

    case NFAPI_CSI_REPORT_TYPE_APERIODIC: {
      if(push8(cqi_ri_info_rel9->aperiodic_cqi_pmi_ri_report.number_of_cc, ppWritePackedMsg, end) == 0)
        return 0;

      uint8_t i;

      for(i = 0; i < cqi_ri_info_rel9->aperiodic_cqi_pmi_ri_report.number_of_cc; ++i) {
        if(push8(cqi_ri_info_rel9->aperiodic_cqi_pmi_ri_report.cc[i].ri_size, ppWritePackedMsg, end) == 0)
          return 0;

        uint8_t j;

        for(j = 0; j < 8; ++j) {
          if(push8(cqi_ri_info_rel9->aperiodic_cqi_pmi_ri_report.cc[i].dl_cqi_pmi_size[j], ppWritePackedMsg, end) == 0)
            return 0;
        }
      }
    }
    break;

    default: {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid report type %d \n", cqi_ri_info_rel9->report_type );
    }
    break;
  };

  return 1;
}

static uint8_t pack_ul_config_request_cqi_ri_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_ri_information_rel13_t *cqi_ri_info_rel13 = (nfapi_ul_config_cqi_ri_information_rel13_t *)tlv;

  switch(cqi_ri_info_rel13->report_type) {
    case NFAPI_CSI_REPORT_TYPE_PERIODIC: {
      if(push16(cqi_ri_info_rel13->periodic_cqi_pmi_ri_report.dl_cqi_pmi_ri_size_2, ppWritePackedMsg, end) == 0)
        return 0;
    }
    break;

    case NFAPI_CSI_REPORT_TYPE_APERIODIC: {
      // No parameters
    }
    break;

    default: {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid report type %d \n", cqi_ri_info_rel13->report_type );
    }
    break;
  };

  return 1;
}

static uint8_t pack_ul_config_request_cqi_ri_information(nfapi_ul_config_cqi_ri_information *cqi_ri_info, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return (pack_tlv(NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL8_TAG, &cqi_ri_info->cqi_ri_information_rel8, ppWritePackedMsg, end, &pack_ul_config_request_cqi_ri_rel8_value) &&
          pack_tlv(NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL9_TAG, &cqi_ri_info->cqi_ri_information_rel9, ppWritePackedMsg, end, &pack_ul_config_request_cqi_ri_rel9_value) &&
          pack_tlv(NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL13_TAG, &cqi_ri_info->cqi_ri_information_rel13, ppWritePackedMsg, end, &pack_ul_config_request_cqi_ri_rel13_value));
}

static uint8_t pack_ul_config_request_init_tx_params_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_initial_transmission_parameters_rel8_t *init_tx_params_rel8 = (nfapi_ul_config_initial_transmission_parameters_rel8_t *)tlv;
  return (push8(init_tx_params_rel8->n_srs_initial, ppWritePackedMsg, end) &&
          push8(init_tx_params_rel8->initial_number_of_resource_blocks, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_initial_transmission_parameters(nfapi_ul_config_initial_transmission_parameters *init_tx_params, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return pack_tlv(NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG, &init_tx_params->initial_transmission_parameters_rel8, ppWritePackedMsg, end,
                  &pack_ul_config_request_init_tx_params_rel8_value);
}

static uint8_t pack_ul_config_request_ulsch_harq_info_rel10_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_harq_information_rel10_t *harq_info_rel10 = (nfapi_ul_config_ulsch_harq_information_rel10_t *)tlv;
  return (push8(harq_info_rel10->harq_size, ppWritePackedMsg, end) &&
          push8(harq_info_rel10->delta_offset_harq, ppWritePackedMsg, end) &&
          push8(harq_info_rel10->ack_nack_mode, ppWritePackedMsg, end));
}
static uint8_t pack_ul_config_request_ulsch_harq_info_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_harq_information_rel13_t *harq_info_rel13 = (nfapi_ul_config_ulsch_harq_information_rel13_t *)tlv;
  return (push16(harq_info_rel13->harq_size_2, ppWritePackedMsg, end) &&
          push8(harq_info_rel13->delta_offset_harq_2, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_ulsch_harq_information(nfapi_ul_config_ulsch_harq_information *harq_info, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( pack_tlv(NFAPI_UL_CONFIG_REQUEST_ULSCH_HARQ_INFORMATION_REL10_TAG, &harq_info->harq_information_rel10, ppWritePackedMsg, end, &pack_ul_config_request_ulsch_harq_info_rel10_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_ULSCH_HARQ_INFORMATION_REL13_TAG, &harq_info->harq_information_rel13, ppWritePackedMsg, end, &pack_ul_config_request_ulsch_harq_info_rel13_value));
}

static uint8_t pack_ul_config_request_ue_info_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_ue_information_rel8_t *ue_info_rel8 = (nfapi_ul_config_ue_information_rel8_t *)tlv;
  return ( push32(ue_info_rel8->handle, ppWritePackedMsg, end) &&
           push16(ue_info_rel8->rnti, ppWritePackedMsg, end));
}
static uint8_t pack_ul_config_request_ue_info_rel11_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_ue_information_rel11_t *ue_info_rel11 = (nfapi_ul_config_ue_information_rel11_t *)tlv;
  return ( push8(ue_info_rel11->virtual_cell_id_enabled_flag, ppWritePackedMsg, end) &&
           push16(ue_info_rel11->npusch_identity, ppWritePackedMsg, end));
}
static uint8_t pack_ul_config_request_ue_info_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_ue_information_rel13_t *ue_info_rel13 = (nfapi_ul_config_ue_information_rel13_t *)tlv;
  return ( push8(ue_info_rel13->ue_type, ppWritePackedMsg, end) &&
           push8(ue_info_rel13->empty_symbols, ppWritePackedMsg, end) &&
           push16(ue_info_rel13->total_number_of_repetitions, ppWritePackedMsg, end) &&
           push16(ue_info_rel13->repetition_number, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_ue_information(nfapi_ul_config_ue_information *ue_info, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( pack_tlv(NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG, &ue_info->ue_information_rel8, ppWritePackedMsg, end, &pack_ul_config_request_ue_info_rel8_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL11_TAG, &ue_info->ue_information_rel11, ppWritePackedMsg, end, &pack_ul_config_request_ue_info_rel11_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL13_TAG, &ue_info->ue_information_rel13, ppWritePackedMsg, end, &pack_ul_config_request_ue_info_rel13_value));
}

static uint8_t pack_ul_config_request_harq_info_rel10_tdd_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_harq_information_rel10_tdd_t *harq_info_rel10_tdd = (nfapi_ul_config_harq_information_rel10_tdd_t *)tlv;
  return ( push8(harq_info_rel10_tdd->harq_size, ppWritePackedMsg, end) &&
           push8(harq_info_rel10_tdd->ack_nack_mode, ppWritePackedMsg, end) &&
           push8(harq_info_rel10_tdd->number_of_pucch_resources, ppWritePackedMsg, end) &&
           push16(harq_info_rel10_tdd->n_pucch_1_0, ppWritePackedMsg, end) &&
           push16(harq_info_rel10_tdd->n_pucch_1_1, ppWritePackedMsg, end) &&
           push16(harq_info_rel10_tdd->n_pucch_1_2, ppWritePackedMsg, end) &&
           push16(harq_info_rel10_tdd->n_pucch_1_3, ppWritePackedMsg, end));
}
static uint8_t pack_ul_config_request_harq_info_rel8_fdd_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_harq_information_rel8_fdd_t *harq_info_rel8_fdd = (nfapi_ul_config_harq_information_rel8_fdd_t *)tlv;
  return ( push16(harq_info_rel8_fdd->n_pucch_1_0, ppWritePackedMsg, end) &&
           push8(harq_info_rel8_fdd->harq_size, ppWritePackedMsg, end));
}
static uint8_t pack_ul_config_request_harq_info_rel9_fdd_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_harq_information_rel9_fdd_t *harq_info_rel9_fdd = (nfapi_ul_config_harq_information_rel9_fdd_t *)tlv;
  return ( push8(harq_info_rel9_fdd->harq_size, ppWritePackedMsg, end) &&
           push8(harq_info_rel9_fdd->ack_nack_mode, ppWritePackedMsg, end) &&
           push8(harq_info_rel9_fdd->number_of_pucch_resources, ppWritePackedMsg, end) &&
           push16(harq_info_rel9_fdd->n_pucch_1_0, ppWritePackedMsg, end) &&
           push16(harq_info_rel9_fdd->n_pucch_1_1, ppWritePackedMsg, end) &&
           push16(harq_info_rel9_fdd->n_pucch_1_2, ppWritePackedMsg, end) &&
           push16(harq_info_rel9_fdd->n_pucch_1_3, ppWritePackedMsg, end));
}
static uint8_t pack_ul_config_request_harq_info_rel11_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_harq_information_rel11_t *harq_info_rel11 = (nfapi_ul_config_harq_information_rel11_t *)tlv;
  return ( push8(harq_info_rel11->num_ant_ports, ppWritePackedMsg, end) &&
           push16(harq_info_rel11->n_pucch_2_0, ppWritePackedMsg, end) &&
           push16(harq_info_rel11->n_pucch_2_1, ppWritePackedMsg, end) &&
           push16(harq_info_rel11->n_pucch_2_2, ppWritePackedMsg, end) &&
           push16(harq_info_rel11->n_pucch_2_3, ppWritePackedMsg, end));
}
static uint8_t pack_ul_config_request_harq_info_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_harq_information_rel13_t *harq_info_rel13 = (nfapi_ul_config_harq_information_rel13_t *)tlv;
  return ( push16(harq_info_rel13->harq_size_2, ppWritePackedMsg, end) &&
           push8(harq_info_rel13->starting_prb, ppWritePackedMsg, end) &&
           push8(harq_info_rel13->n_prb, ppWritePackedMsg, end) &&
           push8(harq_info_rel13->cdm_index, ppWritePackedMsg, end) &&
           push8(harq_info_rel13->n_srs, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_harq_information(nfapi_ul_config_harq_information *harq_info, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( pack_tlv(NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL10_TDD_TAG, &harq_info->harq_information_rel10_tdd, ppWritePackedMsg, end, &pack_ul_config_request_harq_info_rel10_tdd_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL8_FDD_TAG, &harq_info->harq_information_rel8_fdd, ppWritePackedMsg, end, &pack_ul_config_request_harq_info_rel8_fdd_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL9_FDD_TAG, &harq_info->harq_information_rel9_fdd, ppWritePackedMsg, end, &pack_ul_config_request_harq_info_rel9_fdd_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL11_TAG, &harq_info->harq_information_rel11, ppWritePackedMsg, end, &pack_ul_config_request_harq_info_rel11_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL13_TAG, &harq_info->harq_information_rel13, ppWritePackedMsg, end, &pack_ul_config_request_harq_info_rel13_value));
}

static uint8_t pack_ul_config_request_cqi_info_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_information_rel8_t *cqi_info_rel8 = (nfapi_ul_config_cqi_information_rel8_t *)tlv;
  return ( push16(cqi_info_rel8->pucch_index, ppWritePackedMsg, end) &&
           push8(cqi_info_rel8->dl_cqi_pmi_size, ppWritePackedMsg, end));
}
static uint8_t pack_ul_config_request_cqi_info_rel10_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_information_rel10_t *cqi_info_rel10 = (nfapi_ul_config_cqi_information_rel10_t *)tlv;
  return ( push8(cqi_info_rel10->number_of_pucch_resource, ppWritePackedMsg, end) &&
           push16(cqi_info_rel10->pucch_index_p1, ppWritePackedMsg, end));
}
static uint8_t pack_ul_config_request_cqi_info_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_information_rel13_t *cqi_info_rel13 = (nfapi_ul_config_cqi_information_rel13_t *)tlv;
  return ( push8(cqi_info_rel13->csi_mode, ppWritePackedMsg, end) &&
           push16(cqi_info_rel13->dl_cqi_pmi_size_2, ppWritePackedMsg, end) &&
           push8(cqi_info_rel13->starting_prb, ppWritePackedMsg, end) &&
           push8(cqi_info_rel13->n_prb, ppWritePackedMsg, end) &&
           push8(cqi_info_rel13->cdm_index, ppWritePackedMsg, end) &&
           push8(cqi_info_rel13->n_srs, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_cqi_information(nfapi_ul_config_cqi_information *cqi_info, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( pack_tlv(NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL8_TAG, &cqi_info->cqi_information_rel8, ppWritePackedMsg, end, &pack_ul_config_request_cqi_info_rel8_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL10_TAG, &cqi_info->cqi_information_rel10, ppWritePackedMsg, end, &pack_ul_config_request_cqi_info_rel10_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL13_TAG, &cqi_info->cqi_information_rel13, ppWritePackedMsg, end, &pack_ul_config_request_cqi_info_rel13_value));
}

static uint8_t pack_ul_config_request_sr_info_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_sr_information_rel8_t *sr_info_rel8 = (nfapi_ul_config_sr_information_rel8_t *)tlv;
  return push16(sr_info_rel8->pucch_index, ppWritePackedMsg, end);
}
static uint8_t pack_ul_config_request_sr_info_rel10_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_sr_information_rel10_t *sr_info_rel10 = (nfapi_ul_config_sr_information_rel10_t *)tlv;
  return ( push8(sr_info_rel10->number_of_pucch_resources, ppWritePackedMsg, end) &&
           push16(sr_info_rel10->pucch_index_p1, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_sr_information(nfapi_ul_config_sr_information *sr_info, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( pack_tlv(NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL8_TAG, &sr_info->sr_information_rel8, ppWritePackedMsg, end, &pack_ul_config_request_sr_info_rel8_value) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL10_TAG, &sr_info->sr_information_rel10, ppWritePackedMsg, end, &pack_ul_config_request_sr_info_rel10_value));
}

static uint8_t pack_ul_config_request_srs_pdu_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_srs_pdu_rel8_t *srs_pdu_rel8 = (nfapi_ul_config_srs_pdu_rel8_t *)tlv;
  return (push32(srs_pdu_rel8->handle, ppWritePackedMsg, end) &&
          push16(srs_pdu_rel8->size, ppWritePackedMsg, end) &&
          push16(srs_pdu_rel8->rnti, ppWritePackedMsg, end) &&
          push8(srs_pdu_rel8->srs_bandwidth, ppWritePackedMsg, end) &&
          push8(srs_pdu_rel8->frequency_domain_position, ppWritePackedMsg, end) &&
          push8(srs_pdu_rel8->srs_hopping_bandwidth, ppWritePackedMsg, end) &&
          push8(srs_pdu_rel8->transmission_comb, ppWritePackedMsg, end) &&
          push16(srs_pdu_rel8->i_srs, ppWritePackedMsg, end) &&
          push8(srs_pdu_rel8->sounding_reference_cyclic_shift, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_srs_pdu_rel10_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_srs_pdu_rel10_t *srs_pdu_rel10 = (nfapi_ul_config_srs_pdu_rel10_t *)tlv;
  return push8(srs_pdu_rel10->antenna_port, ppWritePackedMsg, end);
}

static uint8_t pack_ul_config_request_srs_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_srs_pdu_rel13_t *srs_pdu_rel13 = (nfapi_ul_config_srs_pdu_rel13_t *)tlv;
  return ( push8(srs_pdu_rel13->number_of_combs, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_nb_harq_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_nb_harq_information_rel13_fdd_t *nb_harq_pdu_rel13 = (nfapi_ul_config_nb_harq_information_rel13_fdd_t *)tlv;
  return ( push8(nb_harq_pdu_rel13->harq_ack_resource, ppWritePackedMsg, end));
}

static uint8_t pack_ul_config_request_nulsch_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_nulsch_pdu_rel13_t *nulsch_pdu_rel13 = (nfapi_ul_config_nulsch_pdu_rel13_t *)tlv;
  return (push8(nulsch_pdu_rel13->nulsch_format, ppWritePackedMsg, end) &&
          push32(nulsch_pdu_rel13->handle, ppWritePackedMsg, end) &&
          push16(nulsch_pdu_rel13->size, ppWritePackedMsg, end) &&
          push16(nulsch_pdu_rel13->rnti, ppWritePackedMsg, end) &&
          push8(nulsch_pdu_rel13->subcarrier_indication, ppWritePackedMsg, end) &&
          push8(nulsch_pdu_rel13->resource_assignment, ppWritePackedMsg, end) &&
          push8(nulsch_pdu_rel13->mcs, ppWritePackedMsg, end) &&
          push8(nulsch_pdu_rel13->redudancy_version, ppWritePackedMsg, end) &&
          push8(nulsch_pdu_rel13->repetition_number, ppWritePackedMsg, end) &&
          push8(nulsch_pdu_rel13->new_data_indication, ppWritePackedMsg, end) &&
          push8(nulsch_pdu_rel13->n_srs, ppWritePackedMsg, end) &&
          push16(nulsch_pdu_rel13->scrambling_sequence_initialization_cinit, ppWritePackedMsg, end) &&
          push16(nulsch_pdu_rel13->sf_idx, ppWritePackedMsg, end) &&
          pack_ul_config_request_ue_information(&(nulsch_pdu_rel13->ue_information), ppWritePackedMsg, end) &&
          pack_tlv(NFAPI_UL_CONFIG_REQUEST_NB_HARQ_INFORMATION_REL13_FDD_TAG, &nulsch_pdu_rel13->nb_harq_information.nb_harq_information_rel13_fdd, ppWritePackedMsg, end,
                   &pack_ul_config_request_nb_harq_rel13_value));
}
static uint8_t pack_ul_config_request_nrach_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_nrach_pdu_rel13_t *nrach_pdu_rel13 = (nfapi_ul_config_nrach_pdu_rel13_t *)tlv;
  return ( push8(nrach_pdu_rel13->nprach_config_0, ppWritePackedMsg, end) &&
           push8(nrach_pdu_rel13->nprach_config_1, ppWritePackedMsg, end) &&
           push8(nrach_pdu_rel13->nprach_config_2, ppWritePackedMsg, end));
}



static uint8_t pack_ul_tti_pdu_list_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nr_ul_tti_request_number_of_pdus_t *value = (nfapi_nr_ul_tti_request_number_of_pdus_t *)tlv;

  if(!(push16(value->pdu_size, ppWritePackedMsg, end) &&
       push16(value->pdu_type, ppWritePackedMsg, end) ))
    return 0;

  // first match the pdu type, then call the respective function
  switch(value->pdu_type) {
    case NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE: {
      if(!pack_ul_tti_request_prach_pdu(&value->prach_pdu, ppWritePackedMsg, end))
        return 0;
    }
    break;

    case NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE: {
      if(!pack_ul_tti_request_pucch_pdu(&value->pucch_pdu, ppWritePackedMsg, end))
        return 0;
    }
    break;

    case NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE: {
      if(!pack_ul_tti_request_pusch_pdu(&value->pusch_pdu, ppWritePackedMsg, end))
        return 0;
    }
    break;

    case NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE: {
      if(!pack_ul_tti_request_srs_pdu(&value->srs_pdu, ppWritePackedMsg, end))
        return 0;
    }
    break;

    default: {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid UL_TTI pdu type %d \n", value->pdu_type );
    }
    break;
  }

  return 1;
}

static uint8_t pack_ul_tti_groups_list_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nr_ul_tti_request_number_of_groups_t *value = (nfapi_nr_ul_tti_request_number_of_groups_t *)tlv;

  if(!push8(value->n_ue, ppWritePackedMsg, end))
    return 0;

  for(int i=0; i<value->n_ue; i++) {
    if(!push8(value->ue_list[i].pdu_idx, ppWritePackedMsg, end))
      return 0;
  }

  return 1;
}

static uint8_t pack_ul_config_request_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_config_request_body_t *value = (nfapi_ul_config_request_body_t *)tlv;

  if(!(push8(value->number_of_pdus, ppWritePackedMsg, end) &&
       push8(value->rach_prach_frequency_resources, ppWritePackedMsg, end) &&
       push8(value->srs_present, ppWritePackedMsg, end)))
    return 0;

  uint16_t i = 0;

  for(i = 0; i < value->number_of_pdus; ++i) {
    nfapi_ul_config_request_pdu_t *pdu = &(value->ul_config_pdu_list[i]);

    if(push8(pdu->pdu_type, ppWritePackedMsg, end) == 0)
      return 0;

    // Put a 0 size in and then determine the size after the pdu
    // has been writen and write the calculated size
    uint8_t *pWritePackedMsgPduSize = *ppWritePackedMsg;
    pdu->pdu_size = 0;

    if(push8(pdu->pdu_size, ppWritePackedMsg, end) == 0)
      return 0;

    switch(pdu->pdu_type) {
      case NFAPI_UL_CONFIG_ULSCH_PDU_TYPE: {
        if(!pack_ul_config_request_ulsch_pdu(&(pdu->ulsch_pdu), ppWritePackedMsg, end))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE: {
        if(!(pack_ul_config_request_ulsch_pdu(&(pdu->ulsch_cqi_ri_pdu.ulsch_pdu), ppWritePackedMsg, end) &&
             pack_ul_config_request_cqi_ri_information(&(pdu->ulsch_cqi_ri_pdu.cqi_ri_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_initial_transmission_parameters(&(pdu->ulsch_cqi_ri_pdu.initial_transmission_parameters), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE: {
        if(!(pack_ul_config_request_ulsch_pdu(&(pdu->ulsch_harq_pdu.ulsch_pdu), ppWritePackedMsg, end) &&
             pack_ul_config_request_ulsch_harq_information(&(pdu->ulsch_harq_pdu.harq_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_initial_transmission_parameters(&(pdu->ulsch_harq_pdu.initial_transmission_parameters), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE: {
        if(!(pack_ul_config_request_ulsch_pdu(&(pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu), ppWritePackedMsg, end) &&
             pack_ul_config_request_cqi_ri_information(&(pdu->ulsch_cqi_harq_ri_pdu.cqi_ri_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_ulsch_harq_information(&(pdu->ulsch_cqi_harq_ri_pdu.harq_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_initial_transmission_parameters(&(pdu->ulsch_cqi_harq_ri_pdu.initial_transmission_parameters), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE: {
        if(!(pack_ul_config_request_ue_information(&(pdu->uci_cqi_pdu.ue_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_cqi_information(&(pdu->uci_cqi_pdu.cqi_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE: {
        if(!(pack_ul_config_request_ue_information(&(pdu->uci_sr_pdu.ue_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_sr_information(&(pdu->uci_sr_pdu.sr_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE: {
        if(!(pack_ul_config_request_ue_information(&(pdu->uci_harq_pdu.ue_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_harq_information(&(pdu->uci_harq_pdu.harq_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE: {
        if(!(pack_ul_config_request_ue_information(&(pdu->uci_sr_harq_pdu.ue_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_sr_information(&(pdu->uci_sr_harq_pdu.sr_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_harq_information(&(pdu->uci_sr_harq_pdu.harq_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE: {
        if(!(pack_ul_config_request_ue_information(&(pdu->uci_cqi_harq_pdu.ue_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_cqi_information(&(pdu->uci_cqi_harq_pdu.cqi_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_harq_information(&(pdu->uci_cqi_harq_pdu.harq_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE: {
        if(!(pack_ul_config_request_ue_information(&(pdu->uci_cqi_sr_pdu.ue_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_cqi_information(&(pdu->uci_cqi_sr_pdu.cqi_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_sr_information(&(pdu->uci_cqi_sr_pdu.sr_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_UCI_CQI_SR_HARQ_PDU_TYPE: {
        if(!(pack_ul_config_request_ue_information(&(pdu->uci_cqi_sr_harq_pdu.ue_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_cqi_information(&(pdu->uci_cqi_sr_harq_pdu.cqi_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_sr_information(&(pdu->uci_cqi_sr_harq_pdu.sr_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_harq_information(&(pdu->uci_cqi_sr_harq_pdu.harq_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_SRS_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL8_TAG, &pdu->srs_pdu.srs_pdu_rel8, ppWritePackedMsg, end, &pack_ul_config_request_srs_pdu_rel8_value) &&
             pack_tlv(NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL10_TAG, &pdu->srs_pdu.srs_pdu_rel10, ppWritePackedMsg, end, &pack_ul_config_request_srs_pdu_rel10_value) &&
             pack_tlv(NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL13_TAG, &pdu->srs_pdu.srs_pdu_rel13, ppWritePackedMsg, end, &pack_ul_config_request_srs_pdu_rel13_value)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_HARQ_BUFFER_PDU_TYPE: {
        if(!(pack_ul_config_request_ue_information(&(pdu->harq_buffer_pdu.ue_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_UCI_CSI_PDU_TYPE: {
        if(!(pack_ul_config_request_ulsch_pdu(&(pdu->ulsch_uci_csi_pdu.ulsch_pdu), ppWritePackedMsg, end) &&
             pack_ul_config_request_cqi_information(&(pdu->ulsch_uci_csi_pdu.csi_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_UCI_HARQ_PDU_TYPE: {
        if(!(pack_ul_config_request_ulsch_pdu(&(pdu->ulsch_uci_harq_pdu.ulsch_pdu), ppWritePackedMsg, end) &&
             pack_ul_config_request_harq_information(&(pdu->ulsch_uci_harq_pdu.harq_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_CSI_UCI_HARQ_PDU_TYPE: {
        if(!(pack_ul_config_request_ulsch_pdu(&(pdu->ulsch_csi_uci_harq_pdu.ulsch_pdu), ppWritePackedMsg, end) &&
             pack_ul_config_request_cqi_information(&(pdu->ulsch_csi_uci_harq_pdu.csi_information), ppWritePackedMsg, end) &&
             pack_ul_config_request_harq_information(&(pdu->ulsch_csi_uci_harq_pdu.harq_information), ppWritePackedMsg, end)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_NULSCH_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_UL_CONFIG_REQUEST_NULSCH_PDU_REL13_TAG, &pdu->nulsch_pdu.nulsch_pdu_rel13, ppWritePackedMsg, end, &pack_ul_config_request_nulsch_pdu_rel13_value)))
          return 0;
      }
      break;

      case NFAPI_UL_CONFIG_NRACH_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_UL_CONFIG_REQUEST_NRACH_PDU_REL13_TAG, &pdu->nrach_pdu.nrach_pdu_rel13, ppWritePackedMsg, end, &pack_ul_config_request_nrach_pdu_rel13_value)))
          return 0;
      }
      break;

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid pdu type %d \n", pdu->pdu_type );
      }
      break;
    };

    // add 1 for the pdu_type. The delta will include the pdu_size
    pdu->pdu_size = 1 + (*ppWritePackedMsg - pWritePackedMsgPduSize);

    push8(pdu->pdu_size, &pWritePackedMsgPduSize, end);
  }

  return 1;
}


static uint8_t pack_ul_tti_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nr_ul_tti_request_t *pNfapiMsg = (nfapi_nr_ul_tti_request_t *)msg;

  if (!(push16(pNfapiMsg->SFN, ppWritePackedMsg, end) &&
        push16(pNfapiMsg->Slot, ppWritePackedMsg, end) &&
        push8(pNfapiMsg->n_pdus, ppWritePackedMsg, end) &&
        push8(pNfapiMsg->rach_present, ppWritePackedMsg, end) &&
        push8(pNfapiMsg->n_ulsch, ppWritePackedMsg, end) &&
        push8(pNfapiMsg->n_ulcch, ppWritePackedMsg, end) &&
        push8(pNfapiMsg->n_group, ppWritePackedMsg, end) ))
    return 0;

  for(int i=0; i<pNfapiMsg->n_pdus; i++) {
    if(!pack_ul_tti_pdu_list_value(&pNfapiMsg->pdus_list[i], ppWritePackedMsg, end))
      return 0;
  }

  for(int i=0; i<pNfapiMsg->n_group; i++) {
    if(!pack_ul_tti_groups_list_value(&pNfapiMsg->groups_list[i], ppWritePackedMsg, end))
      return 0;
  }

  return 1;
}


static uint8_t pack_ul_config_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_ul_config_request_t *pNfapiMsg = (nfapi_ul_config_request_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_UL_CONFIG_REQUEST_BODY_TAG, &pNfapiMsg->ul_config_request_body, ppWritePackedMsg, end, &pack_ul_config_request_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config)) ;
}

static uint8_t pack_hi_dci0_hi_rel8_pdu_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_hi_dci0_hi_pdu_rel8_t *hi_pdu_rel8 = (nfapi_hi_dci0_hi_pdu_rel8_t *)tlv;
  return ( push8(hi_pdu_rel8->resource_block_start, ppWritePackedMsg, end) &&
           push8(hi_pdu_rel8->cyclic_shift_2_for_drms, ppWritePackedMsg, end) &&
           push8(hi_pdu_rel8->hi_value, ppWritePackedMsg, end) &&
           push8(hi_pdu_rel8->i_phich, ppWritePackedMsg, end) &&
           push16(hi_pdu_rel8->transmission_power, ppWritePackedMsg, end));
}

static uint8_t pack_hi_dci0_hi_rel10_pdu_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_hi_dci0_hi_pdu_rel10_t *hi_pdu_rel10 = (nfapi_hi_dci0_hi_pdu_rel10_t *)tlv;
  return ( push8(hi_pdu_rel10->flag_tb2, ppWritePackedMsg, end) &&
           push8(hi_pdu_rel10->hi_value_2, ppWritePackedMsg, end));
}

static uint8_t pack_hi_dci0_dci_rel8_pdu_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_hi_dci0_dci_pdu_rel8_t *dci_pdu_rel8 = (nfapi_hi_dci0_dci_pdu_rel8_t *)tlv;
  return ( push8(dci_pdu_rel8->dci_format, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->cce_index, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->aggregation_level, ppWritePackedMsg, end) &&
           push16(dci_pdu_rel8->rnti, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->resource_block_start, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->number_of_resource_block, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->mcs_1, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->cyclic_shift_2_for_drms, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->frequency_hopping_enabled_flag, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->frequency_hopping_bits, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->new_data_indication_1, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->ue_tx_antenna_seleciton, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->tpc, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->cqi_csi_request, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->ul_index, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel8->dl_assignment_index, ppWritePackedMsg, end) &&
           push32(dci_pdu_rel8->tpc_bitmap, ppWritePackedMsg, end) &&
           push16(dci_pdu_rel8->transmission_power, ppWritePackedMsg, end));
}

static uint8_t pack_hi_dci0_dci_rel10_pdu_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_hi_dci0_dci_pdu_rel10_t *dci_pdu_rel10 = (nfapi_hi_dci0_dci_pdu_rel10_t *)tlv;
  return ( push8(dci_pdu_rel10->cross_carrier_scheduling_flag, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->carrier_indicator, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->size_of_cqi_csi_feild, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->srs_flag, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->srs_request, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->resource_allocation_flag, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->resource_allocation_type, ppWritePackedMsg, end) &&
           push32(dci_pdu_rel10->resource_block_coding, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->mcs_2, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->new_data_indication_2, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->number_of_antenna_ports, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->tpmi, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->total_dci_length_including_padding, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel10->n_ul_rb, ppWritePackedMsg, end));
}

static uint8_t pack_hi_dci0_dci_rel12_pdu_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_hi_dci0_dci_pdu_rel12_t *dci_pdu_rel12 = (nfapi_hi_dci0_dci_pdu_rel12_t *)tlv;
  return ( push8(dci_pdu_rel12->pscch_resource, ppWritePackedMsg, end) &&
           push8(dci_pdu_rel12->time_resource_pattern, ppWritePackedMsg, end));
}

static uint8_t pack_hi_dci0_mpdcch_dci_rel13_pdu_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_hi_dci0_mpdcch_dci_pdu_rel13_t *mpdcch_dci_pdu_rel13 = (nfapi_hi_dci0_mpdcch_dci_pdu_rel13_t *)tlv;
  return ( push8(mpdcch_dci_pdu_rel13->mpdcch_narrowband, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->number_of_prb_pairs, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->resource_block_assignment, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->mpdcch_transmission_type, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->start_symbol, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->ecce_index, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->aggreagation_level, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->rnti_type, ppWritePackedMsg, end) &&
           push16(mpdcch_dci_pdu_rel13->rnti, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->ce_mode, ppWritePackedMsg, end) &&
           push16(mpdcch_dci_pdu_rel13->drms_scrambling_init, ppWritePackedMsg, end) &&
           push16(mpdcch_dci_pdu_rel13->initial_transmission_sf_io, ppWritePackedMsg, end) &&
           push16(mpdcch_dci_pdu_rel13->transmission_power, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->dci_format, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->resource_block_start, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->number_of_resource_blocks, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->mcs, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->pusch_repetition_levels, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->frequency_hopping_flag, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->new_data_indication, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->harq_process, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->redudency_version, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->tpc, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->csi_request, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->ul_inex, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->dai_presence_flag, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->dl_assignment_index, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->srs_request, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->dci_subframe_repetition_number, ppWritePackedMsg, end) &&
           push32(mpdcch_dci_pdu_rel13->tcp_bitmap, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->total_dci_length_include_padding, ppWritePackedMsg, end) &&
           push8(mpdcch_dci_pdu_rel13->number_of_tx_antenna_ports, ppWritePackedMsg, end) &&
           pusharray16(mpdcch_dci_pdu_rel13->precoding_value, NFAPI_MAX_TX_PHYSICAL_ANTENNA_PORTS, mpdcch_dci_pdu_rel13->number_of_tx_antenna_ports, ppWritePackedMsg, end));
}

static uint8_t pack_hi_dci0_npdcch_dci_rel13_pdu_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_hi_dci0_npdcch_dci_pdu_rel13_t *npdcch_dci_pdu_rel13 = (nfapi_hi_dci0_npdcch_dci_pdu_rel13_t *)tlv;
  return ( push8(npdcch_dci_pdu_rel13->ncce_index, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->aggregation_level, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->start_symbol, ppWritePackedMsg, end) &&
           push16(npdcch_dci_pdu_rel13->rnti, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->scrambling_reinitialization_batch_index, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->nrs_antenna_ports_assumed_by_the_ue, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->subcarrier_indication, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->resource_assignment, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->scheduling_delay, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->mcs, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->redudancy_version, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->repetition_number, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->new_data_indicator, ppWritePackedMsg, end) &&
           push8(npdcch_dci_pdu_rel13->dci_subframe_repetition_number, ppWritePackedMsg, end));
}


static uint8_t pack_hi_dci0_request_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_hi_dci0_request_body_t *value = (nfapi_hi_dci0_request_body_t *)tlv;

  if(!(push16(value->sfnsf, ppWritePackedMsg, end) &&
       push8(value->number_of_dci, ppWritePackedMsg, end) &&
       push8(value->number_of_hi, ppWritePackedMsg, end)))
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_pdus = value->number_of_dci + value->number_of_hi;

  for(i = 0; i < total_number_of_pdus; ++i) {
    nfapi_hi_dci0_request_pdu_t *pdu = &(value->hi_dci0_pdu_list[i]);

    if(push8(pdu->pdu_type, ppWritePackedMsg, end) == 0)
      return 0;

    // Put a 0 size in and then determine the size after the pdu
    // has been writen and write the calculated size
    uint8_t *pWritePackedMsgPduSize = *ppWritePackedMsg;
    pdu->pdu_size = 0;

    if(push8(pdu->pdu_size, ppWritePackedMsg, end) == 0)
      return 0;

    switch(pdu->pdu_type) {
      case NFAPI_HI_DCI0_HI_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_HI_DCI0_REQUEST_HI_PDU_REL8_TAG, &pdu->hi_pdu.hi_pdu_rel8, ppWritePackedMsg, end, pack_hi_dci0_hi_rel8_pdu_value) &&
             pack_tlv(NFAPI_HI_DCI0_REQUEST_HI_PDU_REL10_TAG, &pdu->hi_pdu.hi_pdu_rel10, ppWritePackedMsg, end, pack_hi_dci0_hi_rel10_pdu_value)))
          return 0;
      }
      break;

      case NFAPI_HI_DCI0_DCI_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG, &pdu->dci_pdu.dci_pdu_rel8, ppWritePackedMsg, end, pack_hi_dci0_dci_rel8_pdu_value) &&
             pack_tlv(NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL10_TAG, &pdu->dci_pdu.dci_pdu_rel10, ppWritePackedMsg, end, pack_hi_dci0_dci_rel10_pdu_value) &&
             pack_tlv(NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL12_TAG, &pdu->dci_pdu.dci_pdu_rel12, ppWritePackedMsg, end, pack_hi_dci0_dci_rel12_pdu_value)))
          return 0;
      }
      break;

      case NFAPI_HI_DCI0_EPDCCH_DCI_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_HI_DCI0_REQUEST_EPDCCH_DCI_PDU_REL8_TAG, &pdu->epdcch_dci_pdu.epdcch_dci_pdu_rel8, ppWritePackedMsg, end, pack_hi_dci0_dci_rel8_pdu_value) &&
             pack_tlv(NFAPI_HI_DCI0_REQUEST_EPDCCH_DCI_PDU_REL10_TAG, &pdu->epdcch_dci_pdu.epdcch_dci_pdu_rel10, ppWritePackedMsg, end, pack_hi_dci0_dci_rel10_pdu_value) &&
             pack_tlv(NFAPI_HI_DCI0_REQUEST_EPDCCH_PARAMETERS_REL11_TAG, &pdu->epdcch_dci_pdu.epdcch_parameters_rel11, ppWritePackedMsg, end, pack_dl_config_epdcch_parameters_rel11_value)))
          return 0;
      }
      break;

      case NFAPI_HI_DCI0_MPDCCH_DCI_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_HI_DCI0_REQUEST_MPDCCH_DCI_PDU_REL13_TAG, &pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13, ppWritePackedMsg, end, pack_hi_dci0_mpdcch_dci_rel13_pdu_value)))
          return 0;
      }
      break;

      case NFAPI_HI_DCI0_NPDCCH_DCI_PDU_TYPE: {
        if(!(pack_tlv(NFAPI_HI_DCI0_REQUEST_NPDCCH_DCI_PDU_REL13_TAG, &pdu->npdcch_dci_pdu.npdcch_dci_pdu_rel13, ppWritePackedMsg, end, pack_hi_dci0_npdcch_dci_rel13_pdu_value)))
          return 0;
      }
      break;

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid pdu type %d \n", pdu->pdu_type );
      }
      break;
    };

    // add 1 for the pdu_type. The delta will include the pdu_size
    pdu->pdu_size = 1 + (*ppWritePackedMsg - pWritePackedMsgPduSize);

    push8(pdu->pdu_size, &pWritePackedMsgPduSize, end);
  }

  return 1;
}

static uint8_t pack_ul_dci_pdu_list_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nr_ul_dci_request_pdus_t *value = (nfapi_nr_ul_dci_request_pdus_t *)tlv;

  for(uint8_t i = 0; i < MAX_DCI_CORESET; ++i) {
    if(!push16(value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].RNTI, ppWritePackedMsg, end) &&
        push16(value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].ScramblingId, ppWritePackedMsg, end) &&
        push16(value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].ScramblingRNTI, ppWritePackedMsg, end) &&
        push8(value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].CceIndex, ppWritePackedMsg, end) &&
        push8(value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].AggregationLevel, ppWritePackedMsg, end) &&
        push8(value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].beta_PDCCH_1_0, ppWritePackedMsg, end) &&
        push8(value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].powerControlOffsetSS, ppWritePackedMsg, end) &&
        push16(value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].PayloadSizeBits, ppWritePackedMsg, end) &&
        pusharray8(value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].Payload, DCI_PAYLOAD_BYTE_LEN, value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].PayloadSizeBits, ppWritePackedMsg, end));

    return 0;
  }

  return (push16(value->PDUType, ppWritePackedMsg, end) &&
          push16(value->PDUSize, ppWritePackedMsg, end) &&
          push16(value->pdcch_pdu.pdcch_pdu_rel15.BWPSize, ppWritePackedMsg, end) &&
          push16(value->pdcch_pdu.pdcch_pdu_rel15.BWPStart, ppWritePackedMsg, end) &&
          push8(value->pdcch_pdu.pdcch_pdu_rel15.SubcarrierSpacing, ppWritePackedMsg, end) &&
          push8(value->pdcch_pdu.pdcch_pdu_rel15.CyclicPrefix, ppWritePackedMsg, end) &&
          push8(value->pdcch_pdu.pdcch_pdu_rel15.StartSymbolIndex, ppWritePackedMsg, end) &&
          push8(value->pdcch_pdu.pdcch_pdu_rel15.DurationSymbols, ppWritePackedMsg, end) &&
          pusharray8(value->pdcch_pdu.pdcch_pdu_rel15.FreqDomainResource, 6, 6, ppWritePackedMsg, end) &&
          push8(value->pdcch_pdu.pdcch_pdu_rel15.CceRegMappingType, ppWritePackedMsg, end) &&
          push8(value->pdcch_pdu.pdcch_pdu_rel15.RegBundleSize, ppWritePackedMsg, end) &&
          push8(value->pdcch_pdu.pdcch_pdu_rel15.InterleaverSize, ppWritePackedMsg, end) &&
          push8(value->pdcch_pdu.pdcch_pdu_rel15.CoreSetType, ppWritePackedMsg, end) &&
          push16(value->pdcch_pdu.pdcch_pdu_rel15.ShiftIndex, ppWritePackedMsg, end));
}

static uint8_t pack_ul_dci_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nr_ul_dci_request_t *pNfapiMsg = (nfapi_nr_ul_dci_request_t *)msg;

  if (!(push16(pNfapiMsg->SFN, ppWritePackedMsg, end) &&
        push16(pNfapiMsg->Slot, ppWritePackedMsg, end) &&
        push8(pNfapiMsg->numPdus, ppWritePackedMsg, end)
       ))
    return 0;

  for(int i=0; i<pNfapiMsg->numPdus; i++) {
    if(!pack_ul_dci_pdu_list_value(&pNfapiMsg->ul_dci_pdu_list[i], ppWritePackedMsg, end))
      return 0;
  }

  return 1;
}



static uint8_t pack_hi_dci0_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_hi_dci0_request_t *pNfapiMsg = (nfapi_hi_dci0_request_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_HI_DCI0_REQUEST_BODY_TAG, &pNfapiMsg->hi_dci0_request_body, ppWritePackedMsg, end, &pack_hi_dci0_request_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

//pack_tx_data_pdu_list_value
static uint8_t pack_tx_data_pdu_list_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nr_pdu_t *value = (nfapi_nr_pdu_t *)tlv;

  if(!(push32(value->num_TLV, ppWritePackedMsg, end) &&
       push16(value->PDU_index, ppWritePackedMsg, end) &&
       push16(value->PDU_length, ppWritePackedMsg, end)
      ))
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_tlvs = value->num_TLV;

  for(; i < total_number_of_tlvs; ++i) {
    if (!(push16(value->TLVs[i].length, ppWritePackedMsg, end) &&
          push16(value->TLVs[i].tag, ppWritePackedMsg, end)))
      return 0;

    switch(value->TLVs[i].tag) {
      case 0: {
        if(!pusharray32(value->TLVs[i].value.direct, 16384, value->TLVs[i].length, ppWritePackedMsg, end))
          return 0;

        break;
      }

      case 1: {
        if(!pusharray32(value->TLVs[i].value.ptr, value->TLVs[i].length, value->TLVs[i].length, ppWritePackedMsg, end))
          return 0;

        break;
      }

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid tag value %d \n", value->TLVs[i].tag );
        break;
      }
    }
  }

  return 1;
}

static uint8_t pack_tx_request_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_tx_request_body_t *value = (nfapi_tx_request_body_t *)tlv;

  if(push16(value->number_of_pdus, ppWritePackedMsg, end) == 0)
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_pdus = value->number_of_pdus;

  for(; i < total_number_of_pdus; ++i) {
    nfapi_tx_request_pdu_t *pdu = &(value->tx_pdu_list[i]);

    if(!(push16(pdu->pdu_length, ppWritePackedMsg, end) &&
         push16(pdu->pdu_index, ppWritePackedMsg, end)))
      return 0;

    uint8_t j;

    for(j = 0; j < pdu->num_segments; ++j) {
      // Use -1 as it is unbounded
      // DJP - does not handle -1
      // DJP - if(pusharray8(pdu->segments[j].segment_data, (uint32_t)(-1), pdu->segments[j].segment_length, ppWritePackedMsg, end) == 0)
      int push_ret = pusharray8(pdu->segments[j].segment_data, 65535, pdu->segments[j].segment_length, ppWritePackedMsg, end);

      if (pdu->segments[j].segment_length == 3) {
        NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() BCH? segment_data:%x %x %x\n", __FUNCTION__,
                    pdu->segments[j].segment_data[0],
                    pdu->segments[j].segment_data[1],
                    pdu->segments[j].segment_data[2]
                   );
      }

      //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() segment_data:%p segment_length:%u pusharray8()=%d\n", __FUNCTION__, pdu->segments[j].segment_data, pdu->segments[j].segment_length, push_ret);

      if (push_ret == 0) {
        return 0;
      }
    }
  }

  return 1;
}

static uint8_t pack_tx_data_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nr_tx_data_request_t *pNfapiMsg = (nfapi_nr_tx_data_request_t *)msg;

  if (!(
        push16(pNfapiMsg->SFN, ppWritePackedMsg, end) &&
        push16(pNfapiMsg->Slot, ppWritePackedMsg, end) &&
        push16(pNfapiMsg->Number_of_PDUs, ppWritePackedMsg, end)
      ))
    return 0;

  for(int i=0; i<pNfapiMsg->Number_of_PDUs; i++) {
    if(!pack_tx_data_pdu_list_value(&pNfapiMsg->pdu_list[i], ppWritePackedMsg, end))
      return 0;
  }

  return 1;
}

static uint8_t pack_tx_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_tx_request_t *pNfapiMsg = (nfapi_tx_request_t *)msg;
  int x = push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end);
  int y = pack_tlv(NFAPI_TX_REQUEST_BODY_TAG, &pNfapiMsg->tx_request_body, ppWritePackedMsg, end, &pack_tx_request_body_value);
  int z = pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config);
  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() x:%d y:%d z:%d\n", __FUNCTION__, x, y, z);
  return x && y && z;
}

static uint8_t pack_release_request_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ue_release_request_body_t *value = (nfapi_ue_release_request_body_t *)tlv;

  if(push16(value->number_of_TLVs, ppWritePackedMsg, end) == 0) {
    return 0;
  }

  uint8_t j;
  uint16_t num = value->number_of_TLVs;

  for(j = 0; j < num; ++j) {
    if(push16(value->ue_release_request_TLVs_list[j].rnti, ppWritePackedMsg, end) == 0) {
      return 0;
    }
  }

  return 1;
}

static uint8_t pack_ue_release_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_ue_release_request_t *pNfapiMsg = (nfapi_ue_release_request_t *)msg;
  int x = push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end);
  int y = pack_tlv(NFAPI_UE_RELEASE_BODY_TAG, &pNfapiMsg->ue_release_request_body, ppWritePackedMsg, end, &pack_release_request_body_value);
  int z = pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config);
  return x && y && z;
}

static uint8_t pack_ue_release_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_ue_release_response_t *pNfapiMsg = (nfapi_ue_release_response_t *)msg;
  int x = push32(pNfapiMsg->error_code, ppWritePackedMsg, end);
  int z = pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config);
  return x && z;
}

static uint8_t pack_rx_ue_information_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_rx_ue_information *value = (nfapi_rx_ue_information *)tlv;
  return ( push32(value->handle, ppWritePackedMsg, end) &&
           push16(value->rnti, ppWritePackedMsg, end) );
}

static uint8_t unpack_rx_ue_information_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_rx_ue_information *value = (nfapi_rx_ue_information *)tlv;
  return ( pull32(ppReadPackedMsg, &value->handle, end) &&
           pull16(ppReadPackedMsg, &value->rnti, end));
}

static uint8_t pack_harq_indication_tdd_harq_data_bundling(nfapi_harq_indication_tdd_harq_data_bundling_t *data, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( push8(data->value_0, ppWritePackedMsg, end) &&
           push8(data->value_1, ppWritePackedMsg, end));
}

static uint8_t pack_harq_indication_tdd_harq_data_multiplexing(nfapi_harq_indication_tdd_harq_data_multiplexing_t *data, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( push8(data->value_0, ppWritePackedMsg, end) &&
           push8(data->value_1, ppWritePackedMsg, end) &&
           push8(data->value_2, ppWritePackedMsg, end) &&
           push8(data->value_3, ppWritePackedMsg, end));
}

static uint8_t pack_harq_indication_tdd_harq_data_special_bundling(nfapi_harq_indication_tdd_harq_data_special_bundling_t *data, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( push8(data->value_0, ppWritePackedMsg, end) );
}

static uint8_t pack_harq_indication_tdd_harq_data(nfapi_harq_indication_tdd_harq_data_t *data, uint8_t **ppWritePackedMsg, uint8_t *end) {
  return ( push8(data->value_0, ppWritePackedMsg, end) );
}

static uint8_t pack_harq_indication_tdd_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_harq_indication_tdd_rel8_t *harq_indication_tdd_rel8 = (nfapi_harq_indication_tdd_rel8_t *)tlv;

  if(!(push8(harq_indication_tdd_rel8->mode, ppWritePackedMsg, end) &&
       push8(harq_indication_tdd_rel8->number_of_ack_nack, ppWritePackedMsg, end)))
    return 0;

  uint8_t result = 0;

  switch(harq_indication_tdd_rel8->mode) {
    case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_BUNDLING:
      result = pack_harq_indication_tdd_harq_data_bundling(&harq_indication_tdd_rel8->harq_data.bundling, ppWritePackedMsg, end);
      break;

    case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_MULIPLEXING:
      result = pack_harq_indication_tdd_harq_data_multiplexing(&harq_indication_tdd_rel8->harq_data.multiplex, ppWritePackedMsg, end);
      break;

    case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_SPECIAL_BUNDLING:
      result = pack_harq_indication_tdd_harq_data_special_bundling(&harq_indication_tdd_rel8->harq_data.special_bundling, ppWritePackedMsg, end);
      break;

    case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_CHANNEL_SELECTION:
    case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_3:
      result = 1;
      break;

    default:
      // err....
      break;
  }

  return result;
}

static uint8_t pack_harq_indication_tdd_rel9_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_harq_indication_tdd_rel9_t *harq_indication_tdd_rel9 = (nfapi_harq_indication_tdd_rel9_t *)tlv;

  if(!(push8(harq_indication_tdd_rel9->mode, ppWritePackedMsg, end) &&
       push8(harq_indication_tdd_rel9->number_of_ack_nack, ppWritePackedMsg, end)))
    return 0;

  uint8_t idx;

  for(idx = 0; idx < harq_indication_tdd_rel9->number_of_ack_nack; ++idx) {
    uint8_t result = 0;

    switch(harq_indication_tdd_rel9->mode) {
      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_BUNDLING:
        result = pack_harq_indication_tdd_harq_data(&(harq_indication_tdd_rel9->harq_data[idx].bundling), ppWritePackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_MULIPLEXING:
        result = pack_harq_indication_tdd_harq_data(&harq_indication_tdd_rel9->harq_data[idx].multiplex, ppWritePackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_SPECIAL_BUNDLING:
        result = pack_harq_indication_tdd_harq_data_special_bundling(&harq_indication_tdd_rel9->harq_data[idx].special_bundling, ppWritePackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_CHANNEL_SELECTION:
        result = pack_harq_indication_tdd_harq_data(&harq_indication_tdd_rel9->harq_data[idx].channel_selection, ppWritePackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_3:
        result = pack_harq_indication_tdd_harq_data(&harq_indication_tdd_rel9->harq_data[idx].format_3, ppWritePackedMsg, end);
        break;

      default:
        // err....
        break;
    }

    if(result == 0)
      return 0;
  }

  return 1;
}

static uint8_t pack_harq_indication_tdd_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_harq_indication_tdd_rel13_t *harq_indication_tdd_rel13 = (nfapi_harq_indication_tdd_rel13_t *)tlv;

  if(!(push8(harq_indication_tdd_rel13->mode, ppWritePackedMsg, end) &&
       push16(harq_indication_tdd_rel13->number_of_ack_nack, ppWritePackedMsg, end)))
    return 0;

  uint8_t idx;

  for(idx = 0; idx < harq_indication_tdd_rel13->number_of_ack_nack; ++idx) {
    uint8_t result = 0;

    switch(harq_indication_tdd_rel13->mode) {
      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_BUNDLING:
        result = pack_harq_indication_tdd_harq_data(&harq_indication_tdd_rel13->harq_data[idx].bundling, ppWritePackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_MULIPLEXING:
        result = pack_harq_indication_tdd_harq_data(&harq_indication_tdd_rel13->harq_data[idx].multiplex, ppWritePackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_SPECIAL_BUNDLING:
        result = pack_harq_indication_tdd_harq_data_special_bundling(&harq_indication_tdd_rel13->harq_data[idx].special_bundling, ppWritePackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_CHANNEL_SELECTION:
        result = pack_harq_indication_tdd_harq_data(&harq_indication_tdd_rel13->harq_data[idx].channel_selection, ppWritePackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_3:
        result = pack_harq_indication_tdd_harq_data(&harq_indication_tdd_rel13->harq_data[idx].format_3, ppWritePackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_4:
        result = pack_harq_indication_tdd_harq_data(&harq_indication_tdd_rel13->harq_data[idx].format_4, ppWritePackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_5:
        result = pack_harq_indication_tdd_harq_data(&harq_indication_tdd_rel13->harq_data[idx].format_5, ppWritePackedMsg, end);
        break;

      default:
        // err....
        break;
    }

    if(result == 0)
      return 0;
  }

  return 1;
}

static uint8_t pack_harq_indication_fdd_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_harq_indication_fdd_rel8_t *harq_indication_fdd_rel8 = (nfapi_harq_indication_fdd_rel8_t *)tlv;
  return ( push8(harq_indication_fdd_rel8->harq_tb1, ppWritePackedMsg, end) &&
           push8(harq_indication_fdd_rel8->harq_tb2, ppWritePackedMsg, end));
}

static uint8_t pack_harq_indication_fdd_rel9_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_harq_indication_fdd_rel9_t *harq_indication_fdd_rel9 = (nfapi_harq_indication_fdd_rel9_t *)tlv;
  return ( push8(harq_indication_fdd_rel9->mode, ppWritePackedMsg, end) &&
           push8(harq_indication_fdd_rel9->number_of_ack_nack, ppWritePackedMsg, end) &&
           pusharray8(harq_indication_fdd_rel9->harq_tb_n, NFAPI_HARQ_ACK_NACK_REL9_MAX, harq_indication_fdd_rel9->number_of_ack_nack, ppWritePackedMsg, end));
}

static uint8_t pack_harq_indication_fdd_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_harq_indication_fdd_rel13_t *harq_indication_fdd_rel13 = (nfapi_harq_indication_fdd_rel13_t *)tlv;
  return ( push8(harq_indication_fdd_rel13->mode, ppWritePackedMsg, end) &&
           push16(harq_indication_fdd_rel13->number_of_ack_nack, ppWritePackedMsg, end) &&
           pusharray8(harq_indication_fdd_rel13->harq_tb_n, NFAPI_HARQ_ACK_NACK_REL13_MAX, harq_indication_fdd_rel13->number_of_ack_nack, ppWritePackedMsg, end));
}

static uint8_t pack_ul_cqi_information_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_ul_cqi_information_t *value = (nfapi_ul_cqi_information_t *)tlv;
  return ( push8(value->ul_cqi, ppWritePackedMsg, end) &&
           push8(value->channel, ppWritePackedMsg, end));
}

static uint8_t pack_harq_indication_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_harq_indication_body_t *value = (nfapi_harq_indication_body_t *)tlv;

  if(push16(value->number_of_harqs, ppWritePackedMsg, end) == 0)
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_pdus = value->number_of_harqs;

  for(; i < total_number_of_pdus; ++i) {
    nfapi_harq_indication_pdu_t *pdu = &(value->harq_pdu_list[i]);
    uint8_t *instance_length_p = *ppWritePackedMsg;

    if(!push16(pdu->instance_length, ppWritePackedMsg, end))
      return 0;

    if(!(pack_tlv(NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, ppWritePackedMsg, end, pack_rx_ue_information_value) &&
         pack_tlv(NFAPI_HARQ_INDICATION_TDD_REL8_TAG, &pdu->harq_indication_tdd_rel8, ppWritePackedMsg, end, pack_harq_indication_tdd_rel8_value) &&
         pack_tlv(NFAPI_HARQ_INDICATION_TDD_REL9_TAG, &pdu->harq_indication_tdd_rel9, ppWritePackedMsg, end, pack_harq_indication_tdd_rel9_value) &&
         pack_tlv(NFAPI_HARQ_INDICATION_TDD_REL13_TAG, &pdu->harq_indication_tdd_rel13, ppWritePackedMsg, end, pack_harq_indication_tdd_rel13_value) &&
         pack_tlv(NFAPI_HARQ_INDICATION_FDD_REL8_TAG, &pdu->harq_indication_fdd_rel8, ppWritePackedMsg, end, pack_harq_indication_fdd_rel8_value) &&
         pack_tlv(NFAPI_HARQ_INDICATION_FDD_REL9_TAG, &pdu->harq_indication_fdd_rel9, ppWritePackedMsg, end, pack_harq_indication_fdd_rel9_value) &&
         pack_tlv(NFAPI_HARQ_INDICATION_FDD_REL13_TAG, &pdu->harq_indication_fdd_rel13, ppWritePackedMsg, end, pack_harq_indication_fdd_rel13_value) &&
         pack_tlv(NFAPI_UL_CQI_INFORMATION_TAG, &pdu->ul_cqi_information, ppWritePackedMsg, end, pack_ul_cqi_information_value)))
      return 0;

    // calculate the instance length subtracting the size of the instance
    // length feild
    uint16_t instance_length = *ppWritePackedMsg - instance_length_p - 2;
    push16(instance_length, &instance_length_p, end);
  }

  return 1;
}

static uint8_t pack_harq_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_harq_indication_t *pNfapiMsg = (nfapi_harq_indication_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_HARQ_INDICATION_BODY_TAG, &pNfapiMsg->harq_indication_body, ppWritePackedMsg, end, pack_harq_indication_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_crc_indication_rel8_body(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_crc_indication_rel8_t *crc_indication_rel8 = (nfapi_crc_indication_rel8_t *)tlv;
  return ( push8(crc_indication_rel8->crc_flag, ppWritePackedMsg, end) );
}

static uint8_t pack_crc_indication_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_crc_indication_body_t *value = (nfapi_crc_indication_body_t *)tlv;

  if(push16(value->number_of_crcs, ppWritePackedMsg, end) == 0)
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_pdus = value->number_of_crcs;

  for(; i < total_number_of_pdus; ++i) {
    nfapi_crc_indication_pdu_t *pdu = &(value->crc_pdu_list[i]);
    uint8_t *instance_length_p = *ppWritePackedMsg;

    if(!push16(pdu->instance_length, ppWritePackedMsg, end))
      return 0;

    if(!(pack_tlv(NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, ppWritePackedMsg, end, pack_rx_ue_information_value) &&
         pack_tlv(NFAPI_CRC_INDICATION_REL8_TAG, &pdu->crc_indication_rel8, ppWritePackedMsg, end, pack_crc_indication_rel8_body)))
      return 0;

    // calculate the instance length subtracting the size of the instance
    // length feild
    uint16_t instance_length = *ppWritePackedMsg - instance_length_p - 2;
    push16(instance_length, &instance_length_p, end);
  }

  return 1;
}

static uint8_t pack_crc_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_crc_indication_t *pNfapiMsg = (nfapi_crc_indication_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_CRC_INDICATION_BODY_TAG, &pNfapiMsg->crc_indication_body, ppWritePackedMsg, end, &pack_crc_indication_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}
static uint8_t pack_rx_indication_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_rx_indication_rel8_t *value = (nfapi_rx_indication_rel8_t *)tlv;
  return ( push16(value->length, ppWritePackedMsg, end) &&
           push16(value->offset, ppWritePackedMsg, end) &&
           push8(value->ul_cqi, ppWritePackedMsg, end) &&
           push16(value->timing_advance, ppWritePackedMsg, end));
}
static uint8_t pack_rx_indication_rel9_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_rx_indication_rel9_t *value = (nfapi_rx_indication_rel9_t *)tlv;
  return ( push16(value->timing_advance_r9, ppWritePackedMsg, end));
}

static uint8_t pack_rx_ulsch_indication_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_rx_indication_body_t *value = (nfapi_rx_indication_body_t *)tlv;

  //printf("RX ULSCH BODY\n");

  if( push16(value->number_of_pdus, ppWritePackedMsg, end) == 0)
    return 0;

  // need to calculate the data offset's.
  uint16_t i = 0;
  uint16_t offset = 2; // taking into account the number_of_pdus
  uint16_t total_number_of_pdus = value->number_of_pdus;
  //printf("ULSCH:pdus:%d\n", total_number_of_pdus);

  for(i = 0; i < total_number_of_pdus; ++i) {
    nfapi_rx_indication_pdu_t *pdu = &(value->rx_pdu_list[i]);

    if(pdu->rx_ue_information.tl.tag == NFAPI_RX_UE_INFORMATION_TAG) {
      //printf("NFAPI_RX_UE_INFORMATION_TAG\n");
      offset += 4 + 6;
    }

    if(pdu->rx_indication_rel8.tl.tag == NFAPI_RX_INDICATION_REL8_TAG) {
      //printf("NFAPI_RX_INDICATION_REL8_TAG\n");
      offset += 4 + 7;
    }

    if(pdu->rx_indication_rel9.tl.tag == NFAPI_RX_INDICATION_REL9_TAG) {
      //printf("NFAPI_RX_INDICATION_REL9_TAG\n");
      offset += 4 + 2;
    }
  }

  // Now update the structure to include the offset
  for(i =0; i < total_number_of_pdus; ++i) {
    nfapi_rx_indication_pdu_t *pdu = &(value->rx_pdu_list[i]);

    if(pdu->rx_indication_rel8.tl.tag == NFAPI_RX_INDICATION_REL8_TAG) {
      if(pdu->rx_indication_rel8.offset == 1) {
        pdu->rx_indication_rel8.offset = offset;
        offset += pdu->rx_indication_rel8.length;
      }
    }
  }

  // Write out the pdu
  for(i = 0; i < total_number_of_pdus; ++i) {
    nfapi_rx_indication_pdu_t *pdu = &(value->rx_pdu_list[i]);

    if(!(pack_tlv(NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, ppWritePackedMsg, end, pack_rx_ue_information_value) &&
         pack_tlv(NFAPI_RX_INDICATION_REL8_TAG, &pdu->rx_indication_rel8, ppWritePackedMsg, end, pack_rx_indication_rel8_value) &&
         pack_tlv(NFAPI_RX_INDICATION_REL9_TAG, &pdu->rx_indication_rel9, ppWritePackedMsg, end, pack_rx_indication_rel9_value)))
      return 0;
  }

  // Write out the pdu data
  for(i = 0; i < total_number_of_pdus; ++i) {
    uint16_t length = 0;
    nfapi_rx_indication_pdu_t *pdu = &(value->rx_pdu_list[i]);

    if(pdu->rx_indication_rel8.tl.tag == NFAPI_RX_INDICATION_REL8_TAG) {
      length = pdu->rx_indication_rel8.length;
    }

    if( pusharray8(value->rx_pdu_list[i].data, length, length, ppWritePackedMsg, end) == 0)
      return 0;
  }

  return 1;
}


static uint8_t pack_rx_ulsch_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_rx_indication_t *pNfapiMsg = (nfapi_rx_indication_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_RX_INDICATION_BODY_TAG, &pNfapiMsg->rx_indication_body, ppWritePackedMsg, end, pack_rx_ulsch_indication_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_preamble_pdu_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_preamble_pdu_rel8_t *preamble_rel8 = (nfapi_preamble_pdu_rel8_t *)tlv;
  return ( push16(preamble_rel8->rnti, ppWritePackedMsg, end) &&
           push8(preamble_rel8->preamble, ppWritePackedMsg, end) &&
           push16(preamble_rel8->timing_advance, ppWritePackedMsg, end));
}
static uint8_t pack_preamble_pdu_rel9_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_preamble_pdu_rel9_t *preamble_rel9 = (nfapi_preamble_pdu_rel9_t *)tlv;
  return ( push16(preamble_rel9->timing_advance_r9, ppWritePackedMsg, end) );
}
static uint8_t pack_preamble_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_preamble_pdu_rel13_t *preamble_rel13 = (nfapi_preamble_pdu_rel13_t *)tlv;
  return ( push8(preamble_rel13->rach_resource_type, ppWritePackedMsg, end) );
}

static uint8_t pack_rach_indication_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_rach_indication_body_t *value = (nfapi_rach_indication_body_t *)tlv;

  if( push16(value->number_of_preambles, ppWritePackedMsg, end) == 0)
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_pdus = value->number_of_preambles;

  for(; i < total_number_of_pdus; ++i) {
    nfapi_preamble_pdu_t *pdu = &(value->preamble_list[i]);
    uint8_t *instance_length_p = *ppWritePackedMsg;

    if(!push16(pdu->instance_length, ppWritePackedMsg, end))
      return 0;

    if(!(pack_tlv(NFAPI_PREAMBLE_REL8_TAG, &pdu->preamble_rel8, ppWritePackedMsg, end, pack_preamble_pdu_rel8_value) &&
         pack_tlv(NFAPI_PREAMBLE_REL9_TAG, &pdu->preamble_rel9, ppWritePackedMsg, end, pack_preamble_pdu_rel9_value) &&
         pack_tlv(NFAPI_PREAMBLE_REL13_TAG, &pdu->preamble_rel13, ppWritePackedMsg, end, pack_preamble_pdu_rel13_value)))
      return 0;

    // calculate the instance length subtracting the size of the instance
    // length feild
    uint16_t instance_length = *ppWritePackedMsg - instance_length_p - 2;
    push16(instance_length, &instance_length_p, end);
  }

  return 1;
}

static uint8_t pack_rach_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_rach_indication_t *pNfapiMsg = (nfapi_rach_indication_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_RACH_INDICATION_BODY_TAG, &pNfapiMsg->rach_indication_body, ppWritePackedMsg, end, pack_rach_indication_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_srs_indication_fdd_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_srs_indication_fdd_rel8_t *srs_pdu_rel8 = (nfapi_srs_indication_fdd_rel8_t *)tlv;
  return ( push16(srs_pdu_rel8->doppler_estimation, ppWritePackedMsg, end) &&
           push16(srs_pdu_rel8->timing_advance, ppWritePackedMsg, end) &&
           push8(srs_pdu_rel8->number_of_resource_blocks, ppWritePackedMsg, end) &&
           push8(srs_pdu_rel8->rb_start, ppWritePackedMsg, end) &&
           pusharray8(srs_pdu_rel8->snr, NFAPI_NUM_RB_MAX, srs_pdu_rel8->number_of_resource_blocks, ppWritePackedMsg, end));
}

static uint8_t pack_srs_indication_fdd_rel9_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_srs_indication_fdd_rel9_t *srs_pdu_rel9 = (nfapi_srs_indication_fdd_rel9_t *)tlv;
  return ( push16(srs_pdu_rel9->timing_advance_r9, ppWritePackedMsg, end) );
}

static uint8_t pack_srs_indication_tdd_rel10_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_srs_indication_ttd_rel10_t *srs_pdu_rel10 = (nfapi_srs_indication_ttd_rel10_t *)tlv;
  return ( push8(srs_pdu_rel10->uppts_symbol, ppWritePackedMsg, end) );
}

static uint8_t pack_srs_indication_fdd_rel11_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_srs_indication_fdd_rel11_t *srs_pdu_rel11 = (nfapi_srs_indication_fdd_rel11_t *)tlv;
  return ( push16(srs_pdu_rel11->ul_rtoa, ppWritePackedMsg, end) ) ;
}

static uint8_t pack_tdd_channel_measurement_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_tdd_channel_measurement_t *value = (nfapi_tdd_channel_measurement_t *)tlv;

  if(!(push8(value->num_prb_per_subband, ppWritePackedMsg, end) &&
       push8(value->number_of_subbands, ppWritePackedMsg, end) &&
       push8(value->num_atennas, ppWritePackedMsg, end)))
    return 0;

  uint8_t idx = 0;

  for(idx = 0; idx < value->number_of_subbands; ++idx) {
    if(!(push8(value->subands[idx].subband_index, ppWritePackedMsg, end) &&
         pusharray16(value->subands[idx].channel, NFAPI_MAX_NUM_PHYSICAL_ANTENNAS, value->num_atennas, ppWritePackedMsg, end)))
      return 0;
  }

  return 1;
}

static uint8_t pack_srs_indication_body_value(void *tlv, uint8_t **ppWritePackedMsg,  uint8_t *end) {
  nfapi_srs_indication_body_t *value = (nfapi_srs_indication_body_t *)tlv;

  if( push8(value->number_of_ues, ppWritePackedMsg, end) == 0)
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_pdus = value->number_of_ues;

  for(; i < total_number_of_pdus; ++i) {
    nfapi_srs_indication_pdu_t *pdu = &(value->srs_pdu_list[i]);
    uint8_t *instance_length_p = *ppWritePackedMsg;

    if(!push16(pdu->instance_length, ppWritePackedMsg, end))
      return 0;

    if(!(pack_tlv(NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, ppWritePackedMsg, end, &pack_rx_ue_information_value) &&
         pack_tlv(NFAPI_SRS_INDICATION_FDD_REL8_TAG, &pdu->srs_indication_fdd_rel8, ppWritePackedMsg, end, &pack_srs_indication_fdd_rel8_value) &&
         pack_tlv(NFAPI_SRS_INDICATION_FDD_REL9_TAG, &pdu->srs_indication_fdd_rel9, ppWritePackedMsg, end, &pack_srs_indication_fdd_rel9_value) &&
         pack_tlv(NFAPI_SRS_INDICATION_TDD_REL10_TAG, &pdu->srs_indication_tdd_rel10, ppWritePackedMsg, end, &pack_srs_indication_tdd_rel10_value) &&
         pack_tlv(NFAPI_SRS_INDICATION_FDD_REL11_TAG, &pdu->srs_indication_fdd_rel11, ppWritePackedMsg, end, &pack_srs_indication_fdd_rel11_value) &&
         pack_tlv(NFAPI_TDD_CHANNEL_MEASUREMENT_TAG, &pdu->tdd_channel_measurement, ppWritePackedMsg, end, &pack_tdd_channel_measurement_value)))
      return 0;

    // calculate the instance length subtracting the size of the instance
    // length feild
    uint16_t instance_length = *ppWritePackedMsg - instance_length_p - 2;
    push16(instance_length, &instance_length_p, end);
  }

  return 1;
}

static uint8_t pack_srs_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_srs_indication_t *pNfapiMsg = (nfapi_srs_indication_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_SRS_INDICATION_BODY_TAG, &pNfapiMsg->srs_indication_body, ppWritePackedMsg, end, &pack_srs_indication_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_sr_indication_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_sr_indication_body_t *value = (nfapi_sr_indication_body_t *)tlv;

  if(push16(value->number_of_srs, ppWritePackedMsg, end) == 0)
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_pdus = value->number_of_srs;

  for(; i < total_number_of_pdus; ++i) {
    nfapi_sr_indication_pdu_t *pdu = &(value->sr_pdu_list[i]);
    uint8_t *instance_length_p = *ppWritePackedMsg;

    if(!push16(pdu->instance_length, ppWritePackedMsg, end))
      return 0;

    if(!(pack_tlv(NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, ppWritePackedMsg, end, pack_rx_ue_information_value) &&
         pack_tlv(NFAPI_UL_CQI_INFORMATION_TAG, &pdu->ul_cqi_information, ppWritePackedMsg, end, pack_ul_cqi_information_value)))
      return 0;

    // calculate the instance length subtracting the size of the instance
    // length feild
    uint16_t instance_length = *ppWritePackedMsg - instance_length_p - 2;
    push16(instance_length, &instance_length_p, end);
  }

  return 1;
}

static uint8_t pack_sr_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_sr_indication_t *pNfapiMsg = (nfapi_sr_indication_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_SR_INDICATION_BODY_TAG, &pNfapiMsg->sr_indication_body, ppWritePackedMsg, end, &pack_sr_indication_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_cqi_indication_rel8_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_cqi_indication_rel8_t *cqi_pdu_rel8 = (nfapi_cqi_indication_rel8_t *)tlv;
  return ( push16(cqi_pdu_rel8->length, ppWritePackedMsg, end) &&
           push16(cqi_pdu_rel8->data_offset, ppWritePackedMsg, end) &&
           push8(cqi_pdu_rel8->ul_cqi, ppWritePackedMsg, end) &&
           push8(cqi_pdu_rel8->ri, ppWritePackedMsg, end) &&
           push16(cqi_pdu_rel8->timing_advance, ppWritePackedMsg, end));
}

static uint8_t pack_cqi_indication_rel9_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_cqi_indication_rel9_t *cqi_pdu_rel9 = (nfapi_cqi_indication_rel9_t *)tlv;
  return  ( push16(cqi_pdu_rel9->length, ppWritePackedMsg, end) &&
            push16(cqi_pdu_rel9->data_offset, ppWritePackedMsg, end) &&
            push8(cqi_pdu_rel9->ul_cqi, ppWritePackedMsg, end) &&
            push8(cqi_pdu_rel9->number_of_cc_reported, ppWritePackedMsg, end) &&
            pusharray8(cqi_pdu_rel9->ri, NFAPI_CC_MAX, cqi_pdu_rel9->number_of_cc_reported, ppWritePackedMsg, end) &&
            push16(cqi_pdu_rel9->timing_advance, ppWritePackedMsg, end) &&
            push16(cqi_pdu_rel9->timing_advance_r9, ppWritePackedMsg, end));
}

static uint8_t pack_cqi_indication_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_cqi_indication_body_t *value = (nfapi_cqi_indication_body_t *)tlv;

  if( push16(value->number_of_cqis, ppWritePackedMsg, end) == 0)
    return 0;

  // need to calculate the data offset's. This very bittle due the hardcoding
  // of the sizes. can not use the sizeof as we have an array for the Rel9
  // info
  uint16_t i = 0;
  uint16_t offset = 2; // taking into account the number_of_cqis
  uint16_t total_number_of_pdus = value->number_of_cqis;

  for(i = 0; i < total_number_of_pdus; ++i) {
    nfapi_cqi_indication_pdu_t *pdu = &(value->cqi_pdu_list[i]);
    offset += 2; // for the instance length

    if(pdu->rx_ue_information.tl.tag == NFAPI_RX_UE_INFORMATION_TAG) {
      offset += 4 + 6; // sizeof(nfapi_rx_ue_information) - sizeof(nfapi_tl_t)
    }

    if(pdu->cqi_indication_rel8.tl.tag == NFAPI_CQI_INDICATION_REL8_TAG) {
      offset += 4 + 8;
    }

    if(pdu->cqi_indication_rel9.tl.tag == NFAPI_CQI_INDICATION_REL9_TAG) {
      offset += 4 + 10 + pdu->cqi_indication_rel9.number_of_cc_reported;
    }

    if(pdu->ul_cqi_information.tl.tag == NFAPI_UL_CQI_INFORMATION_TAG) {
      offset += 4 + 2;
    }
  }

  // Now update the structure to include the offset
  for(i =0; i < total_number_of_pdus; ++i) {
    nfapi_cqi_indication_pdu_t *pdu = &(value->cqi_pdu_list[i]);

    if(pdu->cqi_indication_rel8.tl.tag == NFAPI_CQI_INDICATION_REL8_TAG) {
      if(pdu->cqi_indication_rel8.data_offset == 1) {
        pdu->cqi_indication_rel8.data_offset = offset;
        offset += pdu->cqi_indication_rel8.length;
      }
    }

    if(pdu->cqi_indication_rel9.tl.tag == NFAPI_CQI_INDICATION_REL9_TAG) {
      if(pdu->cqi_indication_rel9.data_offset == 1) {
        pdu->cqi_indication_rel9.data_offset = offset;
        offset += pdu->cqi_indication_rel9.length;
      }
    }
  }

  // Write out the cqi information
  for(i = 0; i < total_number_of_pdus; ++i) {
    nfapi_cqi_indication_pdu_t *pdu = &(value->cqi_pdu_list[i]);
    uint8_t *instance_length_p = *ppWritePackedMsg;

    if(!push16(pdu->instance_length, ppWritePackedMsg, end))
      return 0;

    if(!(pack_tlv(NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, ppWritePackedMsg, end,pack_rx_ue_information_value) &&
         pack_tlv(NFAPI_CQI_INDICATION_REL8_TAG, &pdu->cqi_indication_rel8, ppWritePackedMsg, end, pack_cqi_indication_rel8_value) &&
         pack_tlv(NFAPI_CQI_INDICATION_REL9_TAG, &pdu->cqi_indication_rel9, ppWritePackedMsg, end, pack_cqi_indication_rel9_value) &&
         pack_tlv(NFAPI_UL_CQI_INFORMATION_TAG, &pdu->ul_cqi_information, ppWritePackedMsg, end, pack_ul_cqi_information_value)))
      return 0;

    // calculate the instance length subtracting the size of the instance
    // length feild
    uint16_t instance_length = *ppWritePackedMsg - instance_length_p - 2;
    push16(instance_length, &instance_length_p, end);
  }

  // Write out the cqi raw data
  for(i = 0; i < total_number_of_pdus; ++i) {
    uint16_t length = 0;
    nfapi_cqi_indication_pdu_t *pdu = &(value->cqi_pdu_list[i]);

    if(pdu->cqi_indication_rel8.tl.tag == NFAPI_CQI_INDICATION_REL8_TAG) {
      length = pdu->cqi_indication_rel8.length;
    }

    if(pdu->cqi_indication_rel9.tl.tag == NFAPI_CQI_INDICATION_REL9_TAG) {
      length = pdu->cqi_indication_rel9.length;
    }

    if( pusharray8(value->cqi_raw_pdu_list[i].pdu, NFAPI_CQI_RAW_MAX_LEN, length, ppWritePackedMsg, end) == 0)
      return 0;
  }

  return 1;
}

static uint8_t pack_cqi_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_cqi_indication_t *pNfapiMsg = (nfapi_cqi_indication_t *)msg;
  //Fixme: allocate some mem to fix pure bug, need to find out proper size
  pNfapiMsg->vendor_extension=(nfapi_vendor_extension_tlv_t)malloc( sizeof(* pNfapiMsg->vendor_extension));
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_CQI_INDICATION_BODY_TAG, &pNfapiMsg->cqi_indication_body, ppWritePackedMsg, end, pack_cqi_indication_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_lbt_pdsch_req_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lbt_pdsch_req_pdu_rel13_t *value = (nfapi_lbt_pdsch_req_pdu_rel13_t *)tlv;
  return ( push32(value->handle, ppWritePackedMsg, end) &&
           push32(value->mp_cca, ppWritePackedMsg, end) &&
           push32(value->n_cca, ppWritePackedMsg, end) &&
           push32(value->offset, ppWritePackedMsg, end) &&
           push32(value->lte_txop_sf, ppWritePackedMsg, end) &&
           push16(value->txop_sfn_sf_end, ppWritePackedMsg, end) &&
           push32(value->lbt_mode, ppWritePackedMsg, end));
}

static uint8_t pack_lbt_drs_req_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lbt_drs_req_pdu_rel13_t *value = (nfapi_lbt_drs_req_pdu_rel13_t *)tlv;
  return ( push32(value->handle, ppWritePackedMsg, end) &&
           push32(value->offset, ppWritePackedMsg, end) &&
           push16(value->sfn_sf_end, ppWritePackedMsg, end) &&
           push32(value->lbt_mode, ppWritePackedMsg, end));
}

static uint8_t pack_lbt_dl_config_request_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lbt_dl_config_request_body_t *value = (nfapi_lbt_dl_config_request_body_t *)tlv;

  if( push16(value->number_of_pdus, ppWritePackedMsg, end) == 0)
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_pdus = value->number_of_pdus;

  for(; i < total_number_of_pdus; ++i) {
    nfapi_lbt_dl_config_request_pdu_t *pdu = &(value->lbt_dl_config_req_pdu_list[i]);

    if( push8(pdu->pdu_type, ppWritePackedMsg, end) == 0)
      return 0;

    // Put a 0 size in and then determine the size after the pdu
    // has been writen and write the calculated size
    uint8_t *pWritePackedMsgPduSize = *ppWritePackedMsg;
    pdu->pdu_size = 0;

    if( push8(pdu->pdu_size, ppWritePackedMsg, end) == 0)
      return 0;

    switch(pdu->pdu_type) {
      case NFAPI_LBT_DL_CONFIG_REQUEST_PDSCH_PDU_TYPE: {
        if( pack_tlv(NFAPI_LBT_PDSCH_REQ_PDU_REL13_TAG, &pdu->lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13, ppWritePackedMsg, end, pack_lbt_pdsch_req_pdu_rel13_value) == 0)
          return 0;
      }
      break;

      case NFAPI_LBT_DL_CONFIG_REQUEST_DRS_PDU_TYPE: {
        if(pack_tlv(NFAPI_LBT_DRS_REQ_PDU_REL13_TAG, &pdu->lbt_drs_req_pdu.lbt_drs_req_pdu_rel13, ppWritePackedMsg, end, pack_lbt_drs_req_pdu_rel13_value) == 0)
          return 0;
      }
      break;

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "LBT_DL_CONFIG.request invalid pdu type %d \n", pdu->pdu_type );
      }
      break;
    };

    // add 1 for the pdu_type. The delta will include the pdu_size
    pdu->pdu_size = 1 + (*ppWritePackedMsg - pWritePackedMsgPduSize);

    push8(pdu->pdu_size, &pWritePackedMsgPduSize, end);
  }

  return 1;
}

static uint8_t pack_lbt_pdsch_rsp_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lbt_pdsch_rsp_pdu_rel13_t *value = (nfapi_lbt_pdsch_rsp_pdu_rel13_t *)tlv;
  return ( push32(value->handle, ppWritePackedMsg, end) &&
           push32(value->result, ppWritePackedMsg, end) &&
           push32(value->lte_txop_symbols, ppWritePackedMsg, end) &&
           push32(value->initial_partial_sf, ppWritePackedMsg, end));
}

static uint8_t pack_lbt_drs_rsp_pdu_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lbt_drs_rsp_pdu_rel13_t *value = (nfapi_lbt_drs_rsp_pdu_rel13_t *)tlv;
  return ( push32(value->handle, ppWritePackedMsg, end) &&
           push32(value->result, ppWritePackedMsg, end));
}

static uint8_t pack_lbt_dl_config_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_lbt_dl_config_request_t *pNfapiMsg = (nfapi_lbt_dl_config_request_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_LBT_DL_CONFIG_REQUEST_BODY_TAG, &pNfapiMsg->lbt_dl_config_request_body, ppWritePackedMsg, end, &pack_lbt_dl_config_request_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_lbt_dl_config_indication_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lbt_dl_indication_body_t *value = (nfapi_lbt_dl_indication_body_t *)tlv;

  if( push16(value->number_of_pdus, ppWritePackedMsg, end) == 0)
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_pdus = value->number_of_pdus;

  for(; i < total_number_of_pdus; ++i) {
    nfapi_lbt_dl_indication_pdu_t *pdu = &(value->lbt_indication_pdu_list[i]);

    if( push8(pdu->pdu_type, ppWritePackedMsg, end) == 0)
      return 0;

    // Put a 0 size in and then determine the size after the pdu
    // has been writen and write the calculated size
    uint8_t *pWritePackedMsgPduSize = *ppWritePackedMsg;
    pdu->pdu_size = 0;

    if(push8(pdu->pdu_size, ppWritePackedMsg, end) == 0)
      return 0;

    switch(pdu->pdu_type) {
      case NFAPI_LBT_DL_RSP_PDSCH_PDU_TYPE: {
        if( pack_tlv(NFAPI_LBT_PDSCH_RSP_PDU_REL13_TAG, &pdu->lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13, ppWritePackedMsg, end, pack_lbt_pdsch_rsp_pdu_rel13_value) == 0)
          return 0;
      }
      break;

      case NFAPI_LBT_DL_RSP_DRS_PDU_TYPE: {
        if( pack_tlv(NFAPI_LBT_DRS_RSP_PDU_REL13_TAG, &pdu->lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13, ppWritePackedMsg, end, pack_lbt_drs_rsp_pdu_rel13_value) == 0)
          return 0;
      }
      break;

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "LBT_DL.indication body invalid pdu type %d \n", pdu->pdu_type );
      }
      break;
    };

    // add 1 for the pdu_type. The delta will include the pdu_size
    pdu->pdu_size = 1 + (*ppWritePackedMsg - pWritePackedMsgPduSize);

    push8(pdu->pdu_size, &pWritePackedMsgPduSize, end);
  }

  return 1;
}

static uint8_t pack_lbt_dl_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_lbt_dl_indication_t *pNfapiMsg = (nfapi_lbt_dl_indication_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_LBT_DL_INDICATION_BODY_TAG, &pNfapiMsg->lbt_dl_indication_body, ppWritePackedMsg, end, &pack_lbt_dl_config_indication_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_nb_harq_indication_fdd_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nb_harq_indication_fdd_rel13_t *nb_harq_indication_fdd_rel13 = (nfapi_nb_harq_indication_fdd_rel13_t *)tlv;
  return ( push8(nb_harq_indication_fdd_rel13->harq_tb1, ppWritePackedMsg, end) );
}

static uint8_t pack_nb_harq_indication_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nb_harq_indication_body_t *value = (nfapi_nb_harq_indication_body_t *)tlv;

  if( push16(value->number_of_harqs, ppWritePackedMsg, end) == 0)
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_harqs = value->number_of_harqs;

  for(; i < total_number_of_harqs; ++i) {
    nfapi_nb_harq_indication_pdu_t *pdu = &(value->nb_harq_pdu_list[i]);
    uint8_t *instance_length_p = *ppWritePackedMsg;

    if(!push16(pdu->instance_length, ppWritePackedMsg, end))
      return 0;

    if(!(pack_tlv(NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, ppWritePackedMsg, end, pack_rx_ue_information_value) &&
         pack_tlv(NFAPI_NB_HARQ_INDICATION_FDD_REL13_TAG, &pdu->nb_harq_indication_fdd_rel13, ppWritePackedMsg, end, pack_nb_harq_indication_fdd_rel13_value) &&
         pack_tlv(NFAPI_UL_CQI_INFORMATION_TAG, &pdu->ul_cqi_information, ppWritePackedMsg, end, pack_ul_cqi_information_value)))
      return 0;

    // calculate the instance length subtracting the size of the instance
    // length feild
    uint16_t instance_length = *ppWritePackedMsg - instance_length_p - 2;
    push16(instance_length, &instance_length_p, end);
  }

  return 1;
}


static uint8_t pack_nb_harq_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nb_harq_indication_t *pNfapiMsg = (nfapi_nb_harq_indication_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_NB_HARQ_INDICATION_BODY_TAG, &pNfapiMsg->nb_harq_indication_body, ppWritePackedMsg, end, &pack_nb_harq_indication_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_nrach_indication_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nrach_indication_pdu_rel13_t *nrach_indication_fdd_rel13 = (nfapi_nrach_indication_pdu_rel13_t *)tlv;
  return ( push16(nrach_indication_fdd_rel13->rnti, ppWritePackedMsg, end) &&
           push8(nrach_indication_fdd_rel13->initial_sc, ppWritePackedMsg, end) &&
           push16(nrach_indication_fdd_rel13->timing_advance, ppWritePackedMsg, end) &&
           push8(nrach_indication_fdd_rel13->nrach_ce_level, ppWritePackedMsg, end));
}


static uint8_t pack_nrach_indication_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nrach_indication_body_t *value = (nfapi_nrach_indication_body_t *)tlv;

  if( push8(value->number_of_initial_scs_detected, ppWritePackedMsg, end) == 0)
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_initial_scs_detected = value->number_of_initial_scs_detected;

  for(; i < total_number_of_initial_scs_detected; ++i) {
    nfapi_nrach_indication_pdu_t *pdu = &(value->nrach_pdu_list[i]);

    //uint8_t* instance_length_p = *ppWritePackedMsg;
    //if(!push16(pdu->instance_length, ppWritePackedMsg, end))
    //  return 0;

    if(!(pack_tlv(NFAPI_NRACH_INDICATION_REL13_TAG, &pdu->nrach_indication_rel13, ppWritePackedMsg, end, pack_nrach_indication_rel13_value)))
      return 0;

    // calculate the instance length subtracting the size of the instance
    // length feild
    //uint16_t instance_length = *ppWritePackedMsg - instance_length_p - 2;
    //push16(instance_length, &instance_length_p, end);
  }

  return 1;
}

static uint8_t pack_nrach_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nrach_indication_t *pNfapiMsg = (nfapi_nrach_indication_t *)msg;
  return ( push16(pNfapiMsg->sfn_sf, ppWritePackedMsg, end) &&
           pack_tlv(NFAPI_NRACH_INDICATION_BODY_TAG, &pNfapiMsg->nrach_indication_body, ppWritePackedMsg, end, &pack_nrach_indication_body_value) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_nr_dl_node_sync(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nr_dl_node_sync_t *pNfapiMsg = (nfapi_nr_dl_node_sync_t *)msg;
  return ( push32(pNfapiMsg->t1, ppWritePackedMsg, end) &&
           pushs32(pNfapiMsg->delta_sfn_slot, ppWritePackedMsg, end) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_dl_node_sync(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_dl_node_sync_t *pNfapiMsg = (nfapi_dl_node_sync_t *)msg;
  return ( push32(pNfapiMsg->t1, ppWritePackedMsg, end) &&
           pushs32(pNfapiMsg->delta_sfn_sf, ppWritePackedMsg, end) &&
           pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_nr_ul_node_sync(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nr_ul_node_sync_t *pNfapiMsg = (nfapi_nr_ul_node_sync_t *)msg;
  return (push32(pNfapiMsg->t1, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->t2, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->t3, ppWritePackedMsg, end) &&
          pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_ul_node_sync(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_ul_node_sync_t *pNfapiMsg = (nfapi_ul_node_sync_t *)msg;
  return (push32(pNfapiMsg->t1, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->t2, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->t3, ppWritePackedMsg, end) &&
          pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_timing_info(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_timing_info_t *pNfapiMsg = (nfapi_timing_info_t *)msg;
  return (push32(pNfapiMsg->last_sfn_sf, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->time_since_last_timing_info, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->dl_config_jitter, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->tx_request_jitter, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->ul_config_jitter, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->hi_dci0_jitter, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->dl_config_latest_delay, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->tx_request_latest_delay, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->ul_config_latest_delay, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->hi_dci0_latest_delay, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->dl_config_earliest_arrival, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->tx_request_earliest_arrival, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->ul_config_earliest_arrival, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->hi_dci0_earliest_arrival, ppWritePackedMsg, end) &&
          pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_nr_timing_info(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nr_timing_info_t *pNfapiMsg = (nfapi_nr_timing_info_t *)msg;
  return (push32(pNfapiMsg->last_sfn, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->last_slot, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->time_since_last_timing_info, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->dl_tti_jitter, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->tx_data_request_jitter, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->ul_tti_jitter, ppWritePackedMsg, end) &&
          push32(pNfapiMsg->ul_dci_jitter, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->dl_tti_latest_delay, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->tx_data_request_latest_delay, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->ul_tti_latest_delay, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->ul_dci_latest_delay, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->dl_tti_earliest_arrival, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->tx_data_request_earliest_arrival, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->ul_tti_earliest_arrival, ppWritePackedMsg, end) &&
          pushs32(pNfapiMsg->ul_dci_earliest_arrival, ppWritePackedMsg, end) &&
          pack_p7_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}



// Main pack function - public

int nfapi_nr_p7_message_pack(void *pMessageBuf, void *pPackedBuf, uint32_t packedBufLen, nfapi_p7_codec_config_t *config) {
  nfapi_p7_message_header_t *pMessageHeader = pMessageBuf;
  uint8_t *pWritePackedMessage = pPackedBuf;
  uint8_t *pPackedLengthField = &pWritePackedMessage[4];
  uint8_t *end = pPackedBuf + packedBufLen;

  if (pMessageBuf == NULL || pPackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack supplied pointers are null\n");
    return -1;
  }

  /*
  printf("\n P7 MESSAGE SENT: \n");
  for(int i=0; i< packedBufLen; i++){
    printf("%d", *(uint8_t *)(pMessageBuf + i));
  }
  printf("\n");
  */
  // process the header
  if(!(push16(pMessageHeader->phy_id, &pWritePackedMessage, end) &&
       push16(pMessageHeader->message_id, &pWritePackedMessage, end) &&
       push16(0/*pMessageHeader->message_length*/, &pWritePackedMessage, end) &&
       push16(pMessageHeader->m_segment_sequence, &pWritePackedMessage, end) &&
       push32(0/*pMessageHeader->checksum*/, &pWritePackedMessage, end) &&
       push32(pMessageHeader->transmit_timestamp, &pWritePackedMessage, end))) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack header failed\n");
    return -1;
  }

  if (pMessageHeader->message_id != NFAPI_TIMING_INFO) {
    //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() message_id:0x%04x phy_id:%u m_segment_sequence:%u timestamp:%u\n", __FUNCTION__, pMessageHeader->message_id, pMessageHeader->phy_id, pMessageHeader->m_segment_sequence, pMessageHeader->transmit_timestamp);
  }

  // look for the specific message
  uint8_t result = 0;

  switch (pMessageHeader->message_id) {
    case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
      result = pack_dl_tti_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
      result = pack_ul_tti_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
      result = pack_tx_data_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
      result = pack_ul_dci_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_UE_RELEASE_REQUEST:
      result =pack_ue_release_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_UE_RELEASE_RESPONSE:
      result =pack_ue_release_response(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_HARQ_INDICATION:
      result = pack_harq_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_CRC_INDICATION:
      result = pack_crc_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_RX_ULSCH_INDICATION:
      //printf("RX ULSCH\n");
      result = pack_rx_ulsch_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_RACH_INDICATION:
      result = pack_rach_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_SRS_INDICATION:
      result = pack_srs_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_RX_SR_INDICATION:
      result = pack_sr_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_RX_CQI_INDICATION:
      result = pack_cqi_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_LBT_DL_CONFIG_REQUEST:
      result = pack_lbt_dl_config_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_LBT_DL_INDICATION:
      result = pack_lbt_dl_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NB_HARQ_INDICATION:
      result = pack_nb_harq_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NRACH_INDICATION:
      result = pack_nrach_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_DL_NODE_SYNC:
      result = pack_nr_dl_node_sync(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_NODE_SYNC:
      result = pack_nr_ul_node_sync(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_TIMING_INFO:
      result = pack_nr_timing_info(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    default: {
      if(pMessageHeader->message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
          pMessageHeader->message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        if(config && config->pack_p7_vendor_extension) {
          result = (config->pack_p7_vendor_extension)(pMessageHeader, &pWritePackedMessage, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s VE NFAPI message ID %d. No ve ecoder provided\n", __FUNCTION__, pMessageHeader->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown message ID %d\n", __FUNCTION__, pMessageHeader->message_id);
      }
    }
    break;
  }

  if(result == 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack failed to pack message\n");
    return -1;
  }

  // check for a valid message length
  uintptr_t msgHead = (uintptr_t)pPackedBuf;
  uintptr_t msgEnd = (uintptr_t)pWritePackedMessage;
  uint32_t packedMsgLen = msgEnd - msgHead;
  uint16_t packedMsgLen16;

  if (packedMsgLen > 0xFFFF || packedMsgLen > packedBufLen) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Packed message length error %d, buffer supplied %d\n", packedMsgLen, packedBufLen);
    return -1;
  } else {
    packedMsgLen16 = (uint16_t)packedMsgLen;
  }

  // Update the message length in the header
  pMessageHeader->message_length = packedMsgLen16;

  if(!push16(packedMsgLen16, &pPackedLengthField, end))
    return -1;

  if(1) {
    //quick test
    if(pMessageHeader->message_length != packedMsgLen) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi packedMsgLen(%d) != message_length(%d) id %d\n", packedMsgLen, pMessageHeader->message_length, pMessageHeader->message_id);
    }
  }

  return (packedMsgLen);
}

int nfapi_p7_message_pack(void *pMessageBuf, void *pPackedBuf, uint32_t packedBufLen, nfapi_p7_codec_config_t *config) {
  nfapi_p7_message_header_t *pMessageHeader = pMessageBuf;
  uint8_t *pWritePackedMessage = pPackedBuf;
  uint8_t *pPackedLengthField = &pWritePackedMessage[4];
  uint8_t *end = pPackedBuf + packedBufLen;

  if (pMessageBuf == NULL || pPackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack supplied pointers are null\n");
    return -1;
  }

  /*
  printf("\n P7 MESSAGE SENT: \n");
  for(int i=0; i< packedBufLen; i++){
    printf("%d", *(uint8_t *)(pMessageBuf + i));
  }
  printf("\n");
  */
  // process the header
  if(!(push16(pMessageHeader->phy_id, &pWritePackedMessage, end) &&
       push16(pMessageHeader->message_id, &pWritePackedMessage, end) &&
       push16(0/*pMessageHeader->message_length*/, &pWritePackedMessage, end) &&
       push16(pMessageHeader->m_segment_sequence, &pWritePackedMessage, end) &&
       push32(0/*pMessageHeader->checksum*/, &pWritePackedMessage, end) &&
       push32(pMessageHeader->transmit_timestamp, &pWritePackedMessage, end))) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack header failed\n");
    return -1;
  }

  if (pMessageHeader->message_id != NFAPI_TIMING_INFO) {
    //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() message_id:0x%04x phy_id:%u m_segment_sequence:%u timestamp:%u\n", __FUNCTION__, pMessageHeader->message_id, pMessageHeader->phy_id, pMessageHeader->m_segment_sequence, pMessageHeader->transmit_timestamp);
  }

  // look for the specific message
  uint8_t result = 0;

  switch (pMessageHeader->message_id) {
    case NFAPI_DL_CONFIG_REQUEST:
      //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() NFAPI_DL_CONFIG_REQUEST\n", __FUNCTION__);
      result = pack_dl_config_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_UL_CONFIG_REQUEST:
      result = pack_ul_config_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_TX_REQUEST:
      //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() NFAPI_TX_REQUEST\n", __FUNCTION__);
      result = pack_tx_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_HI_DCI0_REQUEST:
      result = pack_hi_dci0_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_UE_RELEASE_REQUEST:
      result =pack_ue_release_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_UE_RELEASE_RESPONSE:
      result =pack_ue_release_response(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_HARQ_INDICATION:
      result = pack_harq_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_CRC_INDICATION:
      result = pack_crc_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_RX_ULSCH_INDICATION:
      //printf("RX ULSCH\n");
      result = pack_rx_ulsch_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_RACH_INDICATION:
      result = pack_rach_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_SRS_INDICATION:
      result = pack_srs_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_RX_SR_INDICATION:
      result = pack_sr_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_RX_CQI_INDICATION:
      result = pack_cqi_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_LBT_DL_CONFIG_REQUEST:
      result = pack_lbt_dl_config_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_LBT_DL_INDICATION:
      result = pack_lbt_dl_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NB_HARQ_INDICATION:
      result = pack_nb_harq_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NRACH_INDICATION:
      result = pack_nrach_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_DL_NODE_SYNC:
      result = pack_dl_node_sync(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_UL_NODE_SYNC:
      result = pack_ul_node_sync(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_TIMING_INFO:
      result = pack_timing_info(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    default: {
      if(pMessageHeader->message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
          pMessageHeader->message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        if(config && config->pack_p7_vendor_extension) {
          result = (config->pack_p7_vendor_extension)(pMessageHeader, &pWritePackedMessage, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s VE NFAPI message ID %d. No ve ecoder provided\n", __FUNCTION__, pMessageHeader->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown message ID %d\n", __FUNCTION__, pMessageHeader->message_id);
      }
    }
    break;
  }

  if(result == 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack failed to pack message\n");
    return -1;
  }

  // check for a valid message length
  uintptr_t msgHead = (uintptr_t)pPackedBuf;
  uintptr_t msgEnd = (uintptr_t)pWritePackedMessage;
  uint32_t packedMsgLen = msgEnd - msgHead;
  uint16_t packedMsgLen16;

  if (packedMsgLen > 0xFFFF || packedMsgLen > packedBufLen) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Packed message length error %d, buffer supplied %d\n", packedMsgLen, packedBufLen);
    return -1;
  } else {
    packedMsgLen16 = (uint16_t)packedMsgLen;
  }

  // Update the message length in the header
  pMessageHeader->message_length = packedMsgLen16;

  if(!push16(packedMsgLen16, &pPackedLengthField, end))
    return -1;

  if(1) {
    //quick test
    if(pMessageHeader->message_length != packedMsgLen) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi packedMsgLen(%d) != message_length(%d) id %d\n", packedMsgLen, pMessageHeader->message_length, pMessageHeader->message_id);
    }
  }

  return (packedMsgLen);
}

// Unpack routines
// NR:
static uint8_t unpack_dl_tti_csi_rs_pdu_rel15_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nr_dl_tti_csi_rs_pdu_rel15_t *value = (nfapi_nr_dl_tti_csi_rs_pdu_rel15_t *)tlv;
  return(
          pull16(ppReadPackedMsg, &value->bwp_size, end) &&
          pull16(ppReadPackedMsg, &value->bwp_start, end) &&
          pull8(ppReadPackedMsg, &value->subcarrier_spacing, end) &&
          pull8(ppReadPackedMsg, &value->cyclic_prefix, end) &&
          pull16(ppReadPackedMsg, &value->start_rb, end) &&
          pull16(ppReadPackedMsg, &value->nr_of_rbs, end) &&
          pull8(ppReadPackedMsg, &value->csi_type, end) &&
          pull8(ppReadPackedMsg, &value->row, end) &&
          pull16(ppReadPackedMsg, &value->freq_domain, end) &&
          pull8(ppReadPackedMsg, &value->symb_l0, end) &&
          pull8(ppReadPackedMsg, &value->symb_l1, end) &&
          pull8(ppReadPackedMsg, &value->cdm_type, end) &&
          pull8(ppReadPackedMsg, &value->freq_density, end) &&
          pull16(ppReadPackedMsg, &value->scramb_id, end) &&
          pull8(ppReadPackedMsg, &value->power_control_offset, end) &&
          pull8(ppReadPackedMsg, &value->power_control_offset_ss, end)
        );
}


static uint8_t unpack_dl_tti_pdcch_pdu_rel15_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *value = (nfapi_nr_dl_tti_pdcch_pdu_rel15_t *)tlv;

  for(uint8_t i = 0; i < MAX_DCI_CORESET; ++i) {
    if(!pull16(ppReadPackedMsg, &value->dci_pdu[i].RNTI, end) &&
        pull16(ppReadPackedMsg, &value->dci_pdu[i].ScramblingId, end) &&
        pull16(ppReadPackedMsg, &value->dci_pdu[i].ScramblingRNTI, end) &&
        pull8(ppReadPackedMsg, &value->dci_pdu[i].CceIndex, end) &&
        pull8(ppReadPackedMsg, &value->dci_pdu[i].AggregationLevel, end) &&
        pull8(ppReadPackedMsg, &value->dci_pdu[i].beta_PDCCH_1_0, end) &&
        pull8(ppReadPackedMsg, &value->dci_pdu[i].powerControlOffsetSS, end) &&
        pull16(ppReadPackedMsg, &value->dci_pdu[i].PayloadSizeBits, end) &&
        pullarray8(ppReadPackedMsg, value->dci_pdu[i].Payload, DCI_PAYLOAD_BYTE_LEN, value->dci_pdu[i].PayloadSizeBits, end));

    return 0;
  }

  // TODO: resolve the packaging of array (currently sending a single element)
  return(
          pull16(ppReadPackedMsg, &value->BWPSize, end) &&
          pull16(ppReadPackedMsg, &value->BWPStart, end) &&
          pull8(ppReadPackedMsg, &value->SubcarrierSpacing, end) &&
          pull8(ppReadPackedMsg, &value->CyclicPrefix, end) &&
          pull8(ppReadPackedMsg, &value->StartSymbolIndex, end) &&
          pull8(ppReadPackedMsg, &value->DurationSymbols, end) &&
          pullarray8(ppReadPackedMsg, value->FreqDomainResource, 6, 6, end) &&
          pull8(ppReadPackedMsg, &value->CceRegMappingType, end) &&
          pull8(ppReadPackedMsg, &value->RegBundleSize, end) &&
          pull8(ppReadPackedMsg, &value->InterleaverSize, end) &&
          pull8(ppReadPackedMsg, &value->CoreSetType, end) &&
          pull16(ppReadPackedMsg, &value->ShiftIndex, end) &&
          pull8(ppReadPackedMsg, &value->precoderGranularity, end) &&
          pull16(ppReadPackedMsg, &value->numDlDci, end));
}


static uint8_t unpack_dl_tti_pdsch_pdu_rel15_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *value = (nfapi_nr_dl_tti_pdsch_pdu_rel15_t *)tlv;
  // TODO: resolve the packaging of array (currently sending a single element)
  return(
          pull16(ppReadPackedMsg, &value->pduBitmap, end) &&
          pull16(ppReadPackedMsg, &value->rnti, end) &&
          pull16(ppReadPackedMsg, &value->pduIndex, end) &&
          pull16(ppReadPackedMsg, &value->BWPSize, end) &&
          pull16(ppReadPackedMsg, &value->BWPStart, end) &&
          pull8(ppReadPackedMsg, &value->SubcarrierSpacing, end) &&
          pull8(ppReadPackedMsg, &value->CyclicPrefix, end) &&
          pull8(ppReadPackedMsg, &value->NrOfCodewords, end) &&
          pullarray16(ppReadPackedMsg, value->targetCodeRate, 2, 1, end) &&
          pullarray8(ppReadPackedMsg, value->qamModOrder, 2, 1, end) &&
          pullarray8(ppReadPackedMsg, value->mcsIndex, 2, 1, end) &&
          pullarray8(ppReadPackedMsg, value->mcsTable, 2, 1, end) &&
          pullarray8(ppReadPackedMsg, value->rvIndex, 2, 1, end) &&
          pullarray32(ppReadPackedMsg, value->TBSize, 2, 1, end) &&
          pull16(ppReadPackedMsg, &value->dataScramblingId, end) &&
          pull8(ppReadPackedMsg, &value->nrOfLayers, end) &&
          pull8(ppReadPackedMsg, &value->transmissionScheme, end) &&
          pull8(ppReadPackedMsg, &value->refPoint, end) &&
          pull16(ppReadPackedMsg, &value->dlDmrsSymbPos, end) &&
          pull8(ppReadPackedMsg, &value->dmrsConfigType, end) &&
          pull16(ppReadPackedMsg, &value->dlDmrsScramblingId, end) &&
          pull8(ppReadPackedMsg, &value->SCID, end) &&
          pull8(ppReadPackedMsg, &value->numDmrsCdmGrpsNoData, end) &&
          pull16(ppReadPackedMsg, &value->dmrsPorts, end) &&
          pull8(ppReadPackedMsg, &value->resourceAlloc, end) &&
          pull16(ppReadPackedMsg, &value->rbStart, end) &&
          pull16(ppReadPackedMsg, &value->rbSize, end) &&
          pull8(ppReadPackedMsg, &value->VRBtoPRBMapping, end) &&
          pull8(ppReadPackedMsg, &value->StartSymbolIndex, end) &&
          pull8(ppReadPackedMsg, &value->NrOfSymbols, end) &&
          pull8(ppReadPackedMsg, &value->PTRSPortIndex, end) &&
          pull8(ppReadPackedMsg, &value->PTRSTimeDensity, end) &&
          pull8(ppReadPackedMsg, &value->PTRSFreqDensity, end) &&
          pull8(ppReadPackedMsg, &value->PTRSReOffset, end)
        );
}


static uint8_t unpack_dl_tti_ssb_pdu_rel15_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nr_dl_tti_ssb_pdu_rel15_t *value = (nfapi_nr_dl_tti_ssb_pdu_rel15_t *)tlv;
  return(
          pull16(ppReadPackedMsg, &value->PhysCellId, end) &&
          pull8(ppReadPackedMsg, &value->BetaPss, end) &&
          pull8(ppReadPackedMsg, &value->SsbBlockIndex, end) &&
          pull8(ppReadPackedMsg, &value->SsbSubcarrierOffset, end) &&
          pull16(ppReadPackedMsg, &value->ssbOffsetPointA, end) &&
          pull8(ppReadPackedMsg, &value->bchPayloadFlag, end) &&
          pull32(ppReadPackedMsg, &value->bchPayload, end)
          // TODO: pack precoding_and_beamforming too
        );
}


// LTE:
static uint8_t unpack_dl_config_dci_dl_pdu_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel8_t *dci_dl_pdu_rel8 = (nfapi_dl_config_dci_dl_pdu_rel8_t *)tlv;
  return (pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->dci_format, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->cce_idx, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->aggregation_level, end) &&
          pull16(ppReadPackedMsg, &dci_dl_pdu_rel8->rnti, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->resource_allocation_type, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->virtual_resource_block_assignment_flag, end) &&
          pull32(ppReadPackedMsg, &dci_dl_pdu_rel8->resource_block_coding, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->mcs_1, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->redundancy_version_1, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->new_data_indicator_1, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->transport_block_to_codeword_swap_flag, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->mcs_2, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->redundancy_version_2, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->new_data_indicator_2, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->harq_process, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->tpmi, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->pmi, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->precoding_information, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->tpc, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->downlink_assignment_index, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->ngap, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->transport_block_size_index, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->downlink_power_offset, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->allocate_prach_flag, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->preamble_index, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->prach_mask_index, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel8->rnti_type, end) &&
          pull16(ppReadPackedMsg, &dci_dl_pdu_rel8->transmission_power, end));
}

static uint8_t unpack_dl_config_dci_dl_pdu_rel9_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel9_t *dci_dl_pdu_rel9 = (nfapi_dl_config_dci_dl_pdu_rel9_t *)tlv;
  return ( pull8(ppReadPackedMsg, &dci_dl_pdu_rel9->mcch_flag, end) &&
           pull8(ppReadPackedMsg, &dci_dl_pdu_rel9->mcch_change_notification, end) &&
           pull8(ppReadPackedMsg, &dci_dl_pdu_rel9->scrambling_identity, end));
}

static uint8_t unpack_dl_config_dci_dl_pdu_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel10_t *dci_dl_pdu_rel10 = (nfapi_dl_config_dci_dl_pdu_rel10_t *)tlv;
  return (pull8(ppReadPackedMsg, &dci_dl_pdu_rel10->cross_carrier_scheduling_flag, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel10->carrier_indicator, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel10->srs_flag, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel10->srs_request, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel10->antenna_ports_scrambling_and_layers, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel10->total_dci_length_including_padding, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel10->n_dl_rb, end));
}

static uint8_t unpack_dl_config_dci_dl_pdu_rel11_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel11_t *dci_dl_pdu_rel11 = (nfapi_dl_config_dci_dl_pdu_rel11_t *)tlv;
  return (pull8(ppReadPackedMsg, &dci_dl_pdu_rel11->harq_ack_resource_offset, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel11->pdsch_re_mapping_quasi_co_location_indicator, end));
}

static uint8_t unpack_dl_config_dci_dl_pdu_rel12_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel12_t *dci_dl_pdu_rel12 = (nfapi_dl_config_dci_dl_pdu_rel12_t *)tlv;
  return (pull8(ppReadPackedMsg, &dci_dl_pdu_rel12->primary_cell_type, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel12->ul_dl_configuration_flag, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel12->number_ul_dl_configurations, end) &&
          pullarray8(ppReadPackedMsg, dci_dl_pdu_rel12->ul_dl_configuration_indication, NFAPI_MAX_UL_DL_CONFIGURATIONS, dci_dl_pdu_rel12->number_ul_dl_configurations, end));
}

static uint8_t unpack_tpm_value(uint8_t **ppReadPackedMsg, nfapi_dl_config_dci_dl_tpm_t *value, uint8_t *end) {
  if(!(pull8(ppReadPackedMsg, &value->num_prb_per_subband, end) &&
       pull8(ppReadPackedMsg, &value->number_of_subbands, end) &&
       pull8(ppReadPackedMsg, &value->num_antennas, end)))
    return 0;

  uint8_t idx = 0;

  for(idx = 0; idx < value->number_of_subbands; ++idx) {
    nfapi_dl_config_dci_dl_tpm_subband_info_t *subband_info = &(value->subband_info[idx]);

    if(!(pull8(ppReadPackedMsg, &subband_info->subband_index, end) &&
         pull8(ppReadPackedMsg, &subband_info->scheduled_ues, end)))
      return 0;

    uint8_t antenna_idx = 0;
    uint8_t scheduled_ue_idx = 0;

    for(antenna_idx = 0; antenna_idx < value->num_antennas; ++antenna_idx) {
      for(scheduled_ue_idx = 0; scheduled_ue_idx < subband_info->scheduled_ues; ++scheduled_ue_idx) {
        if(!pull16(ppReadPackedMsg, &(subband_info->precoding_value[antenna_idx][scheduled_ue_idx]), end))
          return 0;
      }
    }
  }

  return 1;
}


static uint8_t unpack_dl_config_dci_dl_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dci_dl_pdu_rel13_t *dci_dl_pdu_rel13 = (nfapi_dl_config_dci_dl_pdu_rel13_t *)tlv;
  // If the length is greater than 5 then the TPM struct flag and possiably the TPM structure have been
  // added
  uint8_t tpm_struct_flag_present = dci_dl_pdu_rel13->tl.length > 5;
  dci_dl_pdu_rel13->tpm_struct_flag = 0;
  return (pull8(ppReadPackedMsg, &dci_dl_pdu_rel13->laa_end_partial_sf_flag, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel13->laa_end_partial_sf_configuration, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel13->initial_lbt_sf, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel13->codebook_size_determination, end) &&
          pull8(ppReadPackedMsg, &dci_dl_pdu_rel13->drms_table_flag, end) &&
          ( (tpm_struct_flag_present == 1) ? pull8(ppReadPackedMsg, &dci_dl_pdu_rel13->tpm_struct_flag, end) : 1) &&
          ( (tpm_struct_flag_present == 1 &&  dci_dl_pdu_rel13->tpm_struct_flag == 1) ? unpack_tpm_value(ppReadPackedMsg, &dci_dl_pdu_rel13->tpm, end) : 1));
}

static uint8_t unpack_dl_config_bch_pdu_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_bch_pdu_rel8_t *bch_pdu_rel8 = (nfapi_dl_config_bch_pdu_rel8_t *)tlv;
  return ( pull16(ppReadPackedMsg, &bch_pdu_rel8->length, end) &&
           pull16(ppReadPackedMsg, (uint16_t *)&bch_pdu_rel8->pdu_index, end) &&
           pull16(ppReadPackedMsg, &bch_pdu_rel8->transmission_power, end));
}

static uint8_t unpack_dl_config_mch_pdu_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_mch_pdu_rel8_t *mch_pdu_rel8 = (nfapi_dl_config_mch_pdu_rel8_t *)tlv;
  return (pull16(ppReadPackedMsg, &mch_pdu_rel8->length, end) &&
          pull16(ppReadPackedMsg, (uint16_t *)&mch_pdu_rel8->pdu_index, end) &&
          pull16(ppReadPackedMsg, &mch_pdu_rel8->rnti, end) &&
          pull8(ppReadPackedMsg, &mch_pdu_rel8->resource_allocation_type, end) &&
          pull32(ppReadPackedMsg, &mch_pdu_rel8->resource_block_coding, end) &&
          pull8(ppReadPackedMsg, &mch_pdu_rel8->modulation, end) &&
          pull16(ppReadPackedMsg, &mch_pdu_rel8->transmission_power, end) &&
          pull16(ppReadPackedMsg, &mch_pdu_rel8->mbsfn_area_id, end));
}

static uint8_t unpack_dl_config_dlsch_pdu_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel8_t *dlsch_pdu_rel8 = (nfapi_dl_config_dlsch_pdu_rel8_t *)tlv;

  if (!(pull16(ppReadPackedMsg, &dlsch_pdu_rel8->length, end) &&
        pull16(ppReadPackedMsg, (uint16_t *)&dlsch_pdu_rel8->pdu_index, end) &&
        pull16(ppReadPackedMsg, &dlsch_pdu_rel8->rnti, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->resource_allocation_type, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->virtual_resource_block_assignment_flag, end) &&
        pull32(ppReadPackedMsg, &dlsch_pdu_rel8->resource_block_coding, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->modulation, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->redundancy_version, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->transport_blocks, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->transport_block_to_codeword_swap_flag, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->transmission_scheme, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->number_of_layers, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->number_of_subbands, end) &&
        pullarray8(ppReadPackedMsg, dlsch_pdu_rel8->codebook_index, NFAPI_MAX_NUM_SUBBANDS, dlsch_pdu_rel8->number_of_subbands, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->ue_category_capacity, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->pa, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->delta_power_offset_index, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->ngap, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->nprb, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->transmission_mode, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->num_bf_prb_per_subband, end) &&
        pull8(ppReadPackedMsg, &dlsch_pdu_rel8->num_bf_vector, end)))
    return 0;

  uint16_t j = 0;

  for(j = 0; j < dlsch_pdu_rel8->num_bf_vector; ++j) {
    if(!(pull8(ppReadPackedMsg, &dlsch_pdu_rel8->bf_vector[j].subband_index, end) &&
         pull8(ppReadPackedMsg, &dlsch_pdu_rel8->bf_vector[j].num_antennas, end) &&
         pullarray16(ppReadPackedMsg, dlsch_pdu_rel8->bf_vector[j].bf_value, NFAPI_MAX_NUM_ANTENNAS, dlsch_pdu_rel8->bf_vector[j].num_antennas, end)))
      return 0;
  }

  return 1;
}
static uint8_t unpack_dl_config_dlsch_pdu_rel9_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel9_t *dlsch_pdu_rel9 = (nfapi_dl_config_dlsch_pdu_rel9_t *)tlv;
  return ( pull8(ppReadPackedMsg, &dlsch_pdu_rel9->nscid, end) );
}
static uint8_t unpack_dl_config_dlsch_pdu_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel10_t *dlsch_pdu_rel10 = (nfapi_dl_config_dlsch_pdu_rel10_t *)tlv;
  return ( pull8(ppReadPackedMsg, &dlsch_pdu_rel10->csi_rs_flag, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel10->csi_rs_resource_config_r10, end) &&
           pull16(ppReadPackedMsg, &dlsch_pdu_rel10->csi_rs_zero_tx_power_resource_config_bitmap_r10, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel10->csi_rs_number_nzp_configuration, end) &&
           pullarray8(ppReadPackedMsg, dlsch_pdu_rel10->csi_rs_resource_config, NFAPI_MAX_CSI_RS_RESOURCE_CONFIG, dlsch_pdu_rel10->csi_rs_number_nzp_configuration, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel10->pdsch_start, end)) ;
}
static uint8_t unpack_dl_config_dlsch_pdu_rel11_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel11_t *dlsch_pdu_rel11 = (nfapi_dl_config_dlsch_pdu_rel11_t *)tlv;
  return ( pull8(ppReadPackedMsg, &dlsch_pdu_rel11->drms_config_flag, end) &&
           pull16(ppReadPackedMsg, &dlsch_pdu_rel11->drms_scrambling, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel11->csi_config_flag, end) &&
           pull16(ppReadPackedMsg, &dlsch_pdu_rel11->csi_scrambling, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel11->pdsch_re_mapping_flag, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel11->pdsch_re_mapping_atenna_ports, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel11->pdsch_re_mapping_freq_shift, end));
}
static uint8_t unpack_dl_config_dlsch_pdu_rel12_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel12_t *dlsch_pdu_rel12 = (nfapi_dl_config_dlsch_pdu_rel12_t *)tlv;
  return ( pull8(ppReadPackedMsg, &dlsch_pdu_rel12->altcqi_table_r12, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel12->maxlayers, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel12->n_dl_harq, end));
}
static uint8_t unpack_dl_config_dlsch_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_dlsch_pdu_rel13_t *dlsch_pdu_rel13 = (nfapi_dl_config_dlsch_pdu_rel13_t *)tlv;
  return ( pull8(ppReadPackedMsg, &dlsch_pdu_rel13->dwpts_symbols, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel13->initial_lbt_sf, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel13->ue_type, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel13->pdsch_payload_type, end) &&
           pull16(ppReadPackedMsg, &dlsch_pdu_rel13->initial_transmission_sf_io, end) &&
           pull8(ppReadPackedMsg, &dlsch_pdu_rel13->drms_table_flag, end));
}

static uint8_t unpack_dl_config_pch_pdu_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_pch_pdu_rel8_t *pch_pdu_rel8 = (nfapi_dl_config_pch_pdu_rel8_t *)tlv;
  return ( pull16(ppReadPackedMsg, &pch_pdu_rel8->length, end) &&
           pull16(ppReadPackedMsg, (uint16_t *)&pch_pdu_rel8->pdu_index, end) &&
           pull16(ppReadPackedMsg, &pch_pdu_rel8->p_rnti, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->resource_allocation_type, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->virtual_resource_block_assignment_flag, end) &&
           pull32(ppReadPackedMsg, &pch_pdu_rel8->resource_block_coding, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->mcs, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->redundancy_version, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->number_of_transport_blocks, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->transport_block_to_codeword_swap_flag, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->transmission_scheme, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->number_of_layers, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->codebook_index, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->ue_category_capacity, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->pa, end) &&
           pull16(ppReadPackedMsg, &pch_pdu_rel8->transmission_power, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->nprb, end) &&
           pull8(ppReadPackedMsg, &pch_pdu_rel8->ngap, end));
}
static uint8_t unpack_dl_config_pch_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_pch_pdu_rel13_t *pch_pdu_rel13 = (nfapi_dl_config_pch_pdu_rel13_t *)tlv;
  return ( pull8(ppReadPackedMsg, &pch_pdu_rel13->ue_mode, end) &&
           pull16(ppReadPackedMsg, &pch_pdu_rel13->initial_transmission_sf_io, end));
}

static uint8_t unpack_dl_config_prs_pdu_rel9_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_prs_pdu_rel9_t *prs_pdu_rel9 = (nfapi_dl_config_prs_pdu_rel9_t *)tlv;
  return ( pull16(ppReadPackedMsg, &prs_pdu_rel9->transmission_power, end) &&
           pull8(ppReadPackedMsg, &prs_pdu_rel9->prs_bandwidth, end) &&
           pull8(ppReadPackedMsg, &prs_pdu_rel9->prs_cyclic_prefix_type, end) &&
           pull8(ppReadPackedMsg, &prs_pdu_rel9->prs_muting, end));
}

static uint8_t unpack_dl_config_csi_rs_pdu_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_csi_rs_pdu_rel10_t *csi_rs_pdu_rel10 = (nfapi_dl_config_csi_rs_pdu_rel10_t *)tlv;
  return ( pull8(ppReadPackedMsg, &csi_rs_pdu_rel10->csi_rs_antenna_port_count_r10, end) &&
           pull8(ppReadPackedMsg, &csi_rs_pdu_rel10->csi_rs_resource_config_r10, end) &&
           pull16(ppReadPackedMsg, &csi_rs_pdu_rel10->transmission_power, end) &&
           pull16(ppReadPackedMsg, &csi_rs_pdu_rel10->csi_rs_zero_tx_power_resource_config_bitmap_r10, end) &&
           pull8(ppReadPackedMsg, &csi_rs_pdu_rel10->csi_rs_number_of_nzp_configuration, end) &&
           pullarray8(ppReadPackedMsg, csi_rs_pdu_rel10->csi_rs_resource_config, NFAPI_MAX_CSI_RS_RESOURCE_CONFIG, csi_rs_pdu_rel10->csi_rs_number_of_nzp_configuration, end));
}

static uint8_t unpack_dl_config_csi_rs_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_csi_rs_pdu_rel13_t *csi_rs_pdu_rel13 = (nfapi_dl_config_csi_rs_pdu_rel13_t *)tlv;

  if (!(pull8(ppReadPackedMsg, &csi_rs_pdu_rel13->csi_rs_class, end) &&
        pull8(ppReadPackedMsg, &csi_rs_pdu_rel13->cdm_type, end) &&
        pull8(ppReadPackedMsg, &csi_rs_pdu_rel13->num_bf_vector, end)))
    return 0;

  uint16_t idx =0;

  for(idx = 0; idx < csi_rs_pdu_rel13->num_bf_vector; ++idx) {
    if(!(pull8(ppReadPackedMsg, &csi_rs_pdu_rel13->bf_vector[idx].csi_rs_resource_index, end)))
      return 0;

    NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : HOW TO DECODE BF VALUE \n");
    //pullarray16(ppReadPackedMsg, &csi_rs_pdu_rel13->bf_vector[idx].bf_vector, ??);
  }

  return 1;
}

static uint8_t unpack_dl_config_epdcch_params_rel11_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_epdcch_parameters_rel11_t *epdcch_params_rel11 = (nfapi_dl_config_epdcch_parameters_rel11_t *)tlv;
  return (pull8(ppReadPackedMsg, &epdcch_params_rel11->epdcch_resource_assignment_flag, end) &&
          pull16(ppReadPackedMsg, &epdcch_params_rel11->epdcch_id, end) &&
          pull8(ppReadPackedMsg, &epdcch_params_rel11->epdcch_start_symbol, end) &&
          pull8(ppReadPackedMsg, &epdcch_params_rel11->epdcch_num_prb, end) &&
          pullarray8(ppReadPackedMsg, epdcch_params_rel11->epdcch_prb_index, NFAPI_MAX_EPDCCH_PRB, epdcch_params_rel11->epdcch_num_prb, end) &&
          pull8(ppReadPackedMsg, &epdcch_params_rel11->bf_vector.subband_index, end) &&
          pull8(ppReadPackedMsg, &epdcch_params_rel11->bf_vector.num_antennas, end) &&
          pullarray16(ppReadPackedMsg, epdcch_params_rel11->bf_vector.bf_value, NFAPI_MAX_NUM_ANTENNAS, epdcch_params_rel11->bf_vector.num_antennas, end));
}

static uint8_t unpack_dl_config_epdcch_params_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_epdcch_parameters_rel13_t *epdcch_params_rel13 = (nfapi_dl_config_epdcch_parameters_rel13_t *)tlv;
  return ( pull8(ppReadPackedMsg, &epdcch_params_rel13->dwpts_symbols, end) &&
           pull8(ppReadPackedMsg, &epdcch_params_rel13->initial_lbt_sf, end));
}

static uint8_t unpack_dl_config_mpdcch_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_mpdcch_pdu_rel13_t *mpdcch_params_rel13 = (nfapi_dl_config_mpdcch_pdu_rel13_t *)tlv;
  return ( pull8(ppReadPackedMsg, &mpdcch_params_rel13->mpdcch_narrow_band, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->number_of_prb_pairs, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->resource_block_assignment, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->mpdcch_tansmission_type, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->start_symbol, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->ecce_index, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->aggregation_level, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->rnti_type, end) &&
           pull16(ppReadPackedMsg, &mpdcch_params_rel13->rnti, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->ce_mode, end) &&
           pull16(ppReadPackedMsg, &mpdcch_params_rel13->drms_scrambling_init, end) &&
           pull16(ppReadPackedMsg, &mpdcch_params_rel13->initial_transmission_sf_io, end) &&
           pull16(ppReadPackedMsg, &mpdcch_params_rel13->transmission_power, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->dci_format, end) &&
           pull16(ppReadPackedMsg, &mpdcch_params_rel13->resource_block_coding, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->mcs, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->pdsch_reptition_levels, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->redundancy_version, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->new_data_indicator, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->harq_process, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->tpmi_length, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->tpmi, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->pmi_flag, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->pmi, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->harq_resource_offset, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->dci_subframe_repetition_number, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->tpc, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->downlink_assignment_index_length, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->downlink_assignment_index, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->allocate_prach_flag, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->preamble_index, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->prach_mask_index, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->starting_ce_level, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->srs_request, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->antenna_ports_and_scrambling_identity_flag, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->antenna_ports_and_scrambling_identity, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->frequency_hopping_enabled_flag, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->paging_direct_indication_differentiation_flag, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->direct_indication, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->total_dci_length_including_padding, end) &&
           pull8(ppReadPackedMsg, &mpdcch_params_rel13->number_of_tx_antenna_ports, end) &&
           pullarray16(ppReadPackedMsg, mpdcch_params_rel13->precoding_value, NFAPI_MAX_TX_PHYSICAL_ANTENNA_PORTS, mpdcch_params_rel13->number_of_tx_antenna_ports, end));
}


static uint8_t unpack_dl_config_nbch_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_nbch_pdu_rel13_t *nbch_params_rel13 = (nfapi_dl_config_nbch_pdu_rel13_t *)tlv;
  return ( pull16(ppReadPackedMsg, &nbch_params_rel13->length, end) &&
           pull16(ppReadPackedMsg, (uint16_t *)&nbch_params_rel13->pdu_index, end) &&
           pull16(ppReadPackedMsg, &nbch_params_rel13->transmission_power, end) &&
           pull16(ppReadPackedMsg, &nbch_params_rel13->hyper_sfn_2_lsbs, end));
}

static uint8_t unpack_dl_config_npdcch_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_npdcch_pdu_rel13_t *npdcch_params_rel13 = (nfapi_dl_config_npdcch_pdu_rel13_t *)tlv;
  return ( pull16(ppReadPackedMsg, &npdcch_params_rel13->length, end) &&
           pull16(ppReadPackedMsg, (uint16_t *)&npdcch_params_rel13->pdu_index, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->ncce_index, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->aggregation_level, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->start_symbol, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->rnti_type, end) &&
           pull16(ppReadPackedMsg, &npdcch_params_rel13->rnti, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->scrambling_reinitialization_batch_index, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->nrs_antenna_ports_assumed_by_the_ue, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->dci_format, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->scheduling_delay, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->resource_assignment, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->repetition_number, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->mcs, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->new_data_indicator, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->harq_ack_resource, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->npdcch_order_indication, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->starting_number_of_nprach_repetitions, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->subcarrier_indication_of_nprach, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->paging_direct_indication_differentation_flag, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->direct_indication, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->dci_subframe_repetition_number, end) &&
           pull8(ppReadPackedMsg, &npdcch_params_rel13->total_dci_length_including_padding, end));
}

static uint8_t unpack_dl_config_ndlsch_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_dl_config_ndlsch_pdu_rel13_t *ndlsch_params_rel13 = (nfapi_dl_config_ndlsch_pdu_rel13_t *)tlv;
  return ( pull16(ppReadPackedMsg, &ndlsch_params_rel13->length, end) &&
           pull16(ppReadPackedMsg, (uint16_t *)&ndlsch_params_rel13->pdu_index, end) &&
           pull8(ppReadPackedMsg, &ndlsch_params_rel13->start_symbol, end) &&
           pull8(ppReadPackedMsg, &ndlsch_params_rel13->rnti_type, end) &&
           pull16(ppReadPackedMsg, &ndlsch_params_rel13->rnti, end) &&
           pull16(ppReadPackedMsg, &ndlsch_params_rel13->resource_assignment, end) &&
           pull16(ppReadPackedMsg, &ndlsch_params_rel13->repetition_number, end) &&
           pull8(ppReadPackedMsg, &ndlsch_params_rel13->modulation, end) &&
           pull8(ppReadPackedMsg, &ndlsch_params_rel13->number_of_subframes_for_resource_assignment, end) &&
           pull8(ppReadPackedMsg, &ndlsch_params_rel13->scrambling_sequence_initialization_cinit, end) &&
           pull16(ppReadPackedMsg, &ndlsch_params_rel13->sf_idx, end) &&
           pull8(ppReadPackedMsg, &ndlsch_params_rel13->nrs_antenna_ports_assumed_by_the_ue, end));
}


static uint8_t unpack_dl_tti_request_body_value(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg) {
  nfapi_nr_dl_tti_request_pdu_t *value = (nfapi_nr_dl_tti_request_pdu_t *)msg;

  if(!(pull32(ppReadPackedMsg, &value->PDUSize, end) &&
       pull16(ppReadPackedMsg, &value->PDUType, end) ))
    return 0;

  // first match the pdu type, then call the respective function
  switch(value->PDUType) {
    case NFAPI_NR_DL_TTI_CSI_RS_PDU_TYPE: {
      if(!(unpack_dl_tti_csi_rs_pdu_rel15_value(&value->csi_rs_pdu.csi_rs_pdu_rel15,ppReadPackedMsg,end)))
        return 0;
    }
    break;

    case NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE: {
      if(!(unpack_dl_tti_pdcch_pdu_rel15_value(&value->pdcch_pdu.pdcch_pdu_rel15,ppReadPackedMsg,end)))
        return 0;
    }
    break;

    case NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE: {
      if(!(unpack_dl_tti_pdsch_pdu_rel15_value(&value->pdsch_pdu.pdsch_pdu_rel15,ppReadPackedMsg,end)))
        return 0;
    }
    break;

    case NFAPI_NR_DL_TTI_SSB_PDU_TYPE: {
      if(!(unpack_dl_tti_ssb_pdu_rel15_value(&value->ssb_pdu.ssb_pdu_rel15,ppReadPackedMsg,end)))
        return 0;
    }
    break;

    default: {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid DL_TTI pdu type %d \n", value->PDUType );
    }
    break;
  }

  return 1;
}




static uint8_t unpack_dl_config_request_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_dl_config_request_body_t *value = (nfapi_dl_config_request_body_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->number_pdcch_ofdm_symbols, end) &&
       pull8(ppReadPackedMsg, &value->number_dci, end) &&
       pull16(ppReadPackedMsg, &value->number_pdu, end) &&
       pull8(ppReadPackedMsg, &value->number_pdsch_rnti, end) &&
       pull16(ppReadPackedMsg, &value->transmission_power_pcfich, end)))
    return 0;

  if(value->number_pdu > NFAPI_DL_CONFIG_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of dl config pdu's exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_pdu, NFAPI_DL_CONFIG_MAX_PDU);
    return 0;
  }

  if(value->number_pdu) {
    value->dl_config_pdu_list = (nfapi_dl_config_request_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_dl_config_request_pdu_t) * value->number_pdu, config);

    if(value->dl_config_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate dl config pdu list (count:%d)\n", __FUNCTION__, value->number_pdu);
      return 0;
    }
  } else {
    value->dl_config_pdu_list = 0;
  }

  uint16_t i;
  uint16_t total_number_of_pdus = value->number_pdu;

  for(i = 0; i < total_number_of_pdus; ++i) {
    nfapi_dl_config_request_pdu_t *pdu = &(value->dl_config_pdu_list[i]);

    if(!(pull8(ppReadPackedMsg, &pdu->pdu_type, end) &&
         pull8(ppReadPackedMsg, &pdu->pdu_size, end)))
      return 0;

    uint8_t *packedPduEnd = (*ppReadPackedMsg) + pdu->pdu_size - 2;

    if(packedPduEnd > end) {
      // pdu end of beyond buffer end
      return 0;
    }

    switch(pdu->pdu_type) {
      case NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel8, &unpack_dl_config_dci_dl_pdu_rel8_value},
          { NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL9_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel9, &unpack_dl_config_dci_dl_pdu_rel9_value},
          { NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL10_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel10, &unpack_dl_config_dci_dl_pdu_rel10_value},
          { NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL11_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel11, &unpack_dl_config_dci_dl_pdu_rel11_value},
          { NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL12_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel12, &unpack_dl_config_dci_dl_pdu_rel12_value},
          { NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL13_TAG, &pdu->dci_dl_pdu.dci_dl_pdu_rel13, &unpack_dl_config_dci_dl_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_BCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_BCH_PDU_REL8_TAG, &pdu->bch_pdu.bch_pdu_rel8, &unpack_dl_config_bch_pdu_rel8_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_MCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_MCH_PDU_REL8_TAG, &pdu->mch_pdu.mch_pdu_rel8, &unpack_dl_config_mch_pdu_rel8_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_DLSCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL8_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel8, &unpack_dl_config_dlsch_pdu_rel8_value},
          { NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL9_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel9, &unpack_dl_config_dlsch_pdu_rel9_value},
          { NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL10_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel10, &unpack_dl_config_dlsch_pdu_rel10_value},
          { NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL11_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel11, &unpack_dl_config_dlsch_pdu_rel11_value},
          { NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL12_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel12, &unpack_dl_config_dlsch_pdu_rel12_value},
          { NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL13_TAG, &pdu->dlsch_pdu.dlsch_pdu_rel13, &unpack_dl_config_dlsch_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_PCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_PCH_PDU_REL8_TAG, &pdu->pch_pdu.pch_pdu_rel8, &unpack_dl_config_pch_pdu_rel8_value},
          { NFAPI_DL_CONFIG_REQUEST_PCH_PDU_REL13_TAG, &pdu->pch_pdu.pch_pdu_rel13, &unpack_dl_config_pch_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_PRS_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_PRS_PDU_REL9_TAG, &pdu->prs_pdu.prs_pdu_rel9, &unpack_dl_config_prs_pdu_rel9_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_CSI_RS_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_CSI_RS_PDU_REL10_TAG, &pdu->csi_rs_pdu.csi_rs_pdu_rel10, &unpack_dl_config_csi_rs_pdu_rel10_value},
          { NFAPI_DL_CONFIG_REQUEST_CSI_RS_PDU_REL13_TAG, &pdu->csi_rs_pdu.csi_rs_pdu_rel13, &unpack_dl_config_csi_rs_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_EPDCCH_DL_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL8_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel8, &unpack_dl_config_dci_dl_pdu_rel8_value},
          { NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL9_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel9, &unpack_dl_config_dci_dl_pdu_rel9_value},
          { NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL10_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel10, &unpack_dl_config_dci_dl_pdu_rel10_value},
          { NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL11_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel11, &unpack_dl_config_dci_dl_pdu_rel11_value},
          { NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL12_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel12, &unpack_dl_config_dci_dl_pdu_rel12_value},
          { NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL13_TAG, &pdu->epdcch_pdu.epdcch_pdu_rel13, &unpack_dl_config_dci_dl_pdu_rel13_value},
          { NFAPI_DL_CONFIG_REQUEST_EPDCCH_PARAM_REL11_TAG, &pdu->epdcch_pdu.epdcch_params_rel11, &unpack_dl_config_epdcch_params_rel11_value},
          { NFAPI_DL_CONFIG_REQUEST_EPDCCH_PARAM_REL13_TAG, &pdu->epdcch_pdu.epdcch_params_rel13, &unpack_dl_config_epdcch_params_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_MPDCCH_PDU_REL13_TAG, &pdu->mpdcch_pdu.mpdcch_pdu_rel13, &unpack_dl_config_mpdcch_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_NBCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_NBCH_PDU_REL13_TAG, &pdu->nbch_pdu.nbch_pdu_rel13, &unpack_dl_config_nbch_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_NPDCCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_NPDCCH_PDU_REL13_TAG, &pdu->npdcch_pdu.npdcch_pdu_rel13, &unpack_dl_config_npdcch_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_DL_CONFIG_NDLSCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_DL_CONFIG_REQUEST_NDLSCH_PDU_REL13_TAG, &pdu->ndlsch_pdu.ndlsch_pdu_rel13, &unpack_dl_config_ndlsch_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      default:
        // Need to log an error
        break;
    }
  }

  return 1;
}


static uint8_t unpack_dl_tti_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_nr_dl_tti_request_t *pNfapiMsg = (nfapi_nr_dl_tti_request_t *)msg;

  if (!(pull16(ppReadPackedMsg,&pNfapiMsg->SFN, end) &&
        pull16(ppReadPackedMsg, &pNfapiMsg->Slot, end) &&
        pull8(ppReadPackedMsg, &pNfapiMsg->dl_tti_request_body.nGroup, end) &&
        pull8(ppReadPackedMsg, &pNfapiMsg->dl_tti_request_body.nPDUs, end) &&
        pullarray8(ppReadPackedMsg,pNfapiMsg->dl_tti_request_body.nUe,256,pNfapiMsg->dl_tti_request_body.nGroup, end)
        //pusharray8(pNfapiMsg->PduIdx[0] ,256,256, ppWritePackedMsg, end)
       ))
    return 0;

  int arr[12];

  for(int i=0; i<pNfapiMsg->dl_tti_request_body.nGroup; i++) {
    for(int j=0; j<pNfapiMsg->dl_tti_request_body.nUe[i]; j++) {
      arr[j] = pNfapiMsg->dl_tti_request_body.PduIdx[i][j];
    }

    if(!(pullarrays32(ppReadPackedMsg,arr,12,pNfapiMsg->dl_tti_request_body.nUe[i], end)))
      return 0;
  }

  for(int i=0; i<pNfapiMsg->dl_tti_request_body.nPDUs; i++) {
    if(!unpack_dl_tti_request_body_value(ppReadPackedMsg, end, &pNfapiMsg->dl_tti_request_body.dl_tti_pdu_list[i]))
      return 0;
  }

  return 1;
}


static uint8_t unpack_ul_tti_request_prach_pdu(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nr_prach_pdu_t *prach_pdu = (nfapi_nr_prach_pdu_t *)tlv;
  return(
          pull16(ppReadPackedMsg, &prach_pdu->phys_cell_id, end) &&
          pull8(ppReadPackedMsg, &prach_pdu->num_prach_ocas, end) &&
          pull8(ppReadPackedMsg, &prach_pdu->prach_format, end) &&
          pull8(ppReadPackedMsg, &prach_pdu->num_ra, end) &&
          pull8(ppReadPackedMsg, &prach_pdu->prach_start_symbol, end) &&
          pull16(ppReadPackedMsg, &prach_pdu->num_cs, end)
          // TODO: ignoring beamforming tlv for now
        );
}


static uint8_t unpack_ul_tti_request_pucch_pdu(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nr_pucch_pdu_t *pucch_pdu = (nfapi_nr_pucch_pdu_t *)tlv;
  return(
          pull16(ppReadPackedMsg, &pucch_pdu->rnti, end) &&
          pull32(ppReadPackedMsg, &pucch_pdu->handle, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->bwp_size, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->bwp_start, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->subcarrier_spacing, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->cyclic_prefix, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->format_type, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->multi_slot_tx_indicator, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->prb_start, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->prb_size, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->start_symbol_index, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->nr_of_symbols, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->freq_hop_flag, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->second_hop_prb, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->group_hop_flag, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->sequence_hop_flag, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->hopping_id, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->initial_cyclic_shift, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->data_scrambling_id, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->time_domain_occ_idx, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->pre_dft_occ_idx, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->pre_dft_occ_len, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->add_dmrs_flag, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->dmrs_scrambling_id, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->dmrs_cyclic_shift, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->sr_flag, end) &&
          pull8(ppReadPackedMsg, &pucch_pdu->bit_len_harq, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->bit_len_csi_part1, end) &&
          pull16(ppReadPackedMsg, &pucch_pdu->bit_len_csi_part2, end)
          // TODO: ignoring beamforming tlv for now
        );
}


static uint8_t unpack_ul_tti_request_pusch_pdu(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nr_pusch_pdu_t *pusch_pdu = (nfapi_nr_pusch_pdu_t *)tlv;

  if (!(
        pull16(ppReadPackedMsg, &pusch_pdu->pdu_bit_map, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->rnti, end) &&
        pull32(ppReadPackedMsg, &pusch_pdu->handle, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->bwp_size, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->bwp_start, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->subcarrier_spacing, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->cyclic_prefix, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->target_code_rate,  end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->qam_mod_order,  end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->mcs_index,  end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->mcs_table, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->transform_precoding, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->data_scrambling_id, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->nrOfLayers, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->ul_dmrs_symb_pos, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->dmrs_config_type, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->ul_dmrs_scrambling_id, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->scid, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->num_dmrs_cdm_grps_no_data, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->dmrs_ports, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->resource_alloc, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->resource_alloc,end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->dmrs_ports, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->rb_start, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->rb_size, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->vrb_to_prb_mapping, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->frequency_hopping, end) &&
        pull16(ppReadPackedMsg, &pusch_pdu->tx_direct_current_location, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->uplink_frequency_shift_7p5khz, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->start_symbol_index, end) &&
        pull8(ppReadPackedMsg, &pusch_pdu->nr_of_symbols, end)
        // TODO: ignoring beamforming tlv for now
      ))
    return 0;

  //Pack Optional Data only included if indicated in pduBitmap
  switch(pusch_pdu->pdu_bit_map) {
    case PUSCH_PDU_BITMAP_PUSCH_DATA: {
      // pack optional TLVs
      return(
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_data.rv_index, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_data.harq_process_id, end) &&
              pull32(ppReadPackedMsg, &pusch_pdu->pusch_data.tb_size, end) &&
              pull16(ppReadPackedMsg, &pusch_pdu->pusch_data.num_cb, end) &&
              pullarray8(ppReadPackedMsg, pusch_pdu->pusch_data.cb_present_and_position,1,1,end)
            );
    }
    break;

    case PUSCH_PDU_BITMAP_PUSCH_UCI: {
      return(
              pull16(ppReadPackedMsg, &pusch_pdu->pusch_uci.harq_ack_bit_length, end) &&
              pull16(ppReadPackedMsg, &pusch_pdu->pusch_uci.csi_part1_bit_length, end) &&
              pull16(ppReadPackedMsg, &pusch_pdu->pusch_uci.csi_part2_bit_length, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_uci.alpha_scaling, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_uci.beta_offset_harq_ack, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_uci.beta_offset_csi1, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_uci.beta_offset_csi2, end)
            );
    }
    break;

    case PUSCH_PDU_BITMAP_PUSCH_PTRS: {
      return(
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_ptrs.num_ptrs_ports, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_ptrs.ptrs_ports_list->ptrs_dmrs_port, end) &&
              +               pull16(ppReadPackedMsg, &pusch_pdu->pusch_ptrs.ptrs_ports_list->ptrs_port_index, end) &&
              +               pull8(ppReadPackedMsg, &pusch_pdu->pusch_ptrs.ptrs_ports_list->ptrs_re_offset, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_ptrs.ptrs_time_density, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_ptrs.ptrs_freq_density, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->pusch_ptrs.ul_ptrs_power, end)
            );
    }
    break;

    case PUSCH_PDU_BITMAP_DFTS_OFDM: {
      return(
              pull8(ppReadPackedMsg, &pusch_pdu->dfts_ofdm.low_papr_group_number, end) &&
              pull16(ppReadPackedMsg, &pusch_pdu->dfts_ofdm.low_papr_sequence_number, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->dfts_ofdm.ul_ptrs_sample_density, end) &&
              pull8(ppReadPackedMsg, &pusch_pdu->dfts_ofdm.ul_ptrs_time_density_transform_precoding, end)
            );
    }
    break;

    default: {
      NFAPI_TRACE(NFAPI_TRACE_INFO, "Invalid pdu bitmap %d \n", pusch_pdu->pdu_bit_map );
    }
  }

  return 1;
}


static uint8_t unpack_ul_tti_request_srs_pdu(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nr_srs_pdu_t *srs_pdu = (nfapi_nr_srs_pdu_t *)tlv;
  return(
          pull16(ppReadPackedMsg, &srs_pdu->rnti, end) &&
          pull32(ppReadPackedMsg, &srs_pdu->handle, end) &&
          pull16(ppReadPackedMsg, &srs_pdu->bwp_size, end) &&
          pull16(ppReadPackedMsg, &srs_pdu->bwp_start, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->subcarrier_spacing, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->cyclic_prefix, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->num_ant_ports, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->num_symbols,  end) &&
          pull8(ppReadPackedMsg, &srs_pdu->num_repetitions, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->time_start_position, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->config_index, end) &&
          pull16(ppReadPackedMsg, &srs_pdu->sequence_id, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->bandwidth_index, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->comb_size, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->comb_offset, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->cyclic_shift, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->frequency_position, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->frequency_shift, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->frequency_hopping, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->group_or_sequence_hopping, end) &&
          pull8(ppReadPackedMsg, &srs_pdu->resource_type, end) &&
          pull16(ppReadPackedMsg, &srs_pdu->t_srs, end) &&
          pull16(ppReadPackedMsg, &srs_pdu->t_offset, end)
          // TODO: ignoring beamforming tlv for now
        );
}


static uint8_t unpack_ul_tti_pdu_list_value(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg) {
  nfapi_nr_ul_tti_request_number_of_pdus_t *pNfapiMsg = (nfapi_nr_ul_tti_request_number_of_pdus_t *)msg;

  if(!(pull16(ppReadPackedMsg, &pNfapiMsg->pdu_size, end) &&
       pull16(ppReadPackedMsg, &pNfapiMsg->pdu_type, end) ))
    return 0;

  // first natch the pdu type, then call the respective function
  switch(pNfapiMsg->pdu_type) {
    case NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE: {
      if(!unpack_ul_tti_request_prach_pdu(&pNfapiMsg->prach_pdu, ppReadPackedMsg, end))
        return 0;
    }
    break;

    case NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE: {
      if(!unpack_ul_tti_request_pucch_pdu(&pNfapiMsg->pucch_pdu, ppReadPackedMsg, end))
        return 0;
    }
    break;

    case NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE: {
      if(!unpack_ul_tti_request_pusch_pdu(&pNfapiMsg->pusch_pdu, ppReadPackedMsg, end))
        return 0;
    }
    break;

    case NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE: {
      if(!unpack_ul_tti_request_srs_pdu(&pNfapiMsg->srs_pdu, ppReadPackedMsg, end))
        return 0;
    }
    break;

    default: {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid UL_TTI pdu type %d \n", pNfapiMsg->pdu_type );
    }
    break;
  }

  return 1;
}


static uint8_t unpack_ul_tti_groups_list_value(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg) {
  nfapi_nr_ul_tti_request_number_of_groups_t *pNfapiMsg = (nfapi_nr_ul_tti_request_number_of_groups_t *)msg;

  if(!pull8(ppReadPackedMsg, &pNfapiMsg->n_ue, end))
    return 0;

  for (int i = 0; i < pNfapiMsg->n_ue; i++) {
    if(!pull8(ppReadPackedMsg, &pNfapiMsg->ue_list[i].pdu_idx,end) )
      return 0;
  }

  return 1;
}


static uint8_t unpack_ul_tti_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_nr_ul_tti_request_t *pNfapiMsg = (nfapi_nr_ul_tti_request_t *)msg;

  if (!(
        pull16(ppReadPackedMsg, &pNfapiMsg->SFN, end) &&
        pull16(ppReadPackedMsg, &pNfapiMsg->Slot, end) &&
        pull8(ppReadPackedMsg, &pNfapiMsg->n_pdus, end) &&
        pull8(ppReadPackedMsg, &pNfapiMsg->n_group, end) &&
        pull8(ppReadPackedMsg, &pNfapiMsg->rach_present, end) &&
        pull8(ppReadPackedMsg, &pNfapiMsg->n_ulcch, end) &&
        pull8(ppReadPackedMsg, &pNfapiMsg->n_ulsch, end) ))
    return 0;

  for(int i=0; i< pNfapiMsg->n_pdus; i++) {
    if (!unpack_ul_tti_pdu_list_value(ppReadPackedMsg, end, &pNfapiMsg->pdus_list[i]))
      return 0;
  }

  for(int i=0; i< pNfapiMsg->n_group; i++) {
    if (!unpack_ul_tti_groups_list_value(ppReadPackedMsg, end, &pNfapiMsg->groups_list[i]))
      return 0;
  }

  return 1;
}



static uint8_t unpack_dl_config_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_dl_config_request_t *pNfapiMsg = (nfapi_dl_config_request_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_DL_CONFIG_REQUEST_BODY_TAG, &pNfapiMsg->dl_config_request_body, &unpack_dl_config_request_body_value},
  };
  return ( pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
           unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_ul_config_ulsch_pdu_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_pdu_rel8_t *ulsch_pdu_rel8 = (nfapi_ul_config_ulsch_pdu_rel8_t *)tlv;
  return (pull32(ppReadPackedMsg, &ulsch_pdu_rel8->handle, end) &&
          pull16(ppReadPackedMsg, &ulsch_pdu_rel8->size, end) &&
          pull16(ppReadPackedMsg, &ulsch_pdu_rel8->rnti, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->resource_block_start, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->number_of_resource_blocks, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->modulation_type, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->cyclic_shift_2_for_drms, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->frequency_hopping_enabled_flag, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->frequency_hopping_bits, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->new_data_indication, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->redundancy_version, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->harq_process_number, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->ul_tx_mode, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->current_tx_nb, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel8->n_srs, end ));
}

static uint8_t unpack_ul_config_ulsch_pdu_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_pdu_rel10_t *ulsch_pdu_rel10 = (nfapi_ul_config_ulsch_pdu_rel10_t *)tlv;
  return (pull8(ppReadPackedMsg, &ulsch_pdu_rel10->resource_allocation_type, end) &&
          pull32(ppReadPackedMsg, &ulsch_pdu_rel10->resource_block_coding, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel10->transport_blocks, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel10->transmission_scheme, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel10->number_of_layers, end) &
          pull8(ppReadPackedMsg, &ulsch_pdu_rel10->codebook_index, end) &&
          pull8(ppReadPackedMsg, &ulsch_pdu_rel10->disable_sequence_hopping_flag, end));
}
static uint8_t unpack_ul_config_ulsch_pdu_rel11_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_pdu_rel11_t *ulsch_pdu_rel11 = (nfapi_ul_config_ulsch_pdu_rel11_t *)tlv;
  return ( pull8(ppReadPackedMsg, &ulsch_pdu_rel11->virtual_cell_id_enabled_flag, end) &&
           pull16(ppReadPackedMsg, &ulsch_pdu_rel11->npusch_identity, end) &&
           pull8(ppReadPackedMsg, &ulsch_pdu_rel11->dmrs_config_flag, end) &&
           pull16(ppReadPackedMsg, &ulsch_pdu_rel11->ndmrs_csh_identity, end));
}
static uint8_t unpack_ul_config_ulsch_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_pdu_rel13_t *ulsch_pdu_rel13 = (nfapi_ul_config_ulsch_pdu_rel13_t *)tlv;
  return (pull8(ppReadPackedMsg,  &ulsch_pdu_rel13->ue_type, end) &&
          pull16(ppReadPackedMsg, &ulsch_pdu_rel13->total_number_of_repetitions, end) &&
          pull16(ppReadPackedMsg, &ulsch_pdu_rel13->repetition_number, end) &&
          pull16(ppReadPackedMsg, &ulsch_pdu_rel13->initial_transmission_sf_io, end) &&
          pull8(ppReadPackedMsg,  &ulsch_pdu_rel13->empty_symbols_due_to_re_tunning, end));
}
static uint8_t unpack_ul_config_cqi_ri_info_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_ri_information_rel8_t *cqi_ri_info_rel8 = (nfapi_ul_config_cqi_ri_information_rel8_t *)tlv;
  return (pull8(ppReadPackedMsg, &cqi_ri_info_rel8->dl_cqi_pmi_size_rank_1, end) &&
          pull8(ppReadPackedMsg, &cqi_ri_info_rel8->dl_cqi_pmi_size_rank_greater_1, end) &&
          pull8(ppReadPackedMsg, &cqi_ri_info_rel8->ri_size, end) &&
          pull8(ppReadPackedMsg, &cqi_ri_info_rel8->delta_offset_cqi, end) &&
          pull8(ppReadPackedMsg, &cqi_ri_info_rel8->delta_offset_ri, end));
}

static uint8_t unpack_ul_config_cqi_ri_info_rel9_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_ri_information_rel9_t *cqi_ri_info_rel9 = (nfapi_ul_config_cqi_ri_information_rel9_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &cqi_ri_info_rel9->report_type, end) &&
       pull8(ppReadPackedMsg, &cqi_ri_info_rel9->delta_offset_cqi, end) &&
       pull8(ppReadPackedMsg, &cqi_ri_info_rel9->delta_offset_ri, end)))
    return 0;

  switch(cqi_ri_info_rel9->report_type) {
    case NFAPI_CSI_REPORT_TYPE_PERIODIC: {
      if(!(pull8(ppReadPackedMsg, &cqi_ri_info_rel9->periodic_cqi_pmi_ri_report.dl_cqi_pmi_ri_size, end) &&
           pull8(ppReadPackedMsg, &cqi_ri_info_rel9->periodic_cqi_pmi_ri_report.control_type, end)))
        return 0;
    }
    break;

    case NFAPI_CSI_REPORT_TYPE_APERIODIC: {
      if(pull8(ppReadPackedMsg, &cqi_ri_info_rel9->aperiodic_cqi_pmi_ri_report.number_of_cc, end) ==0)
        return 0;

      uint8_t i;

      for(i = 0; i < cqi_ri_info_rel9->aperiodic_cqi_pmi_ri_report.number_of_cc; ++i) {
        if(pull8(ppReadPackedMsg, &cqi_ri_info_rel9->aperiodic_cqi_pmi_ri_report.cc[i].ri_size, end) == 0)
          return 0;

        uint8_t j;

        for(j = 0; j < 8; ++j) {
          if(pull8(ppReadPackedMsg, &cqi_ri_info_rel9->aperiodic_cqi_pmi_ri_report.cc[i].dl_cqi_pmi_size[j], end) == 0)
            return 0;
        }
      }
    }
    break;

    default: {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid report type %d \n", cqi_ri_info_rel9->report_type );
      return 0;
    }
    break;
  };

  return 1;
}

// NOTE : This function is a little unconventional as we uese the side to
// determine the report type
static uint8_t unpack_ul_config_cqi_ri_info_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_ri_information_rel13_t *cqi_ri_info_rel13 = (nfapi_ul_config_cqi_ri_information_rel13_t *)tlv;

  if(cqi_ri_info_rel13->tl.length == 0) {
    cqi_ri_info_rel13->report_type = NFAPI_CSI_REPORT_TYPE_APERIODIC;
  } else {
    cqi_ri_info_rel13->report_type = NFAPI_CSI_REPORT_TYPE_PERIODIC;

    if(pull16(ppReadPackedMsg, &cqi_ri_info_rel13->periodic_cqi_pmi_ri_report.dl_cqi_pmi_ri_size_2, end) == 0)
      return 0;
  }

  return 1;
}
static uint8_t unpack_ul_config_cqi_init_tx_params_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_initial_transmission_parameters_rel8_t *init_tx_params_rel8 = (nfapi_ul_config_initial_transmission_parameters_rel8_t *)tlv;
  return (pull8(ppReadPackedMsg, &init_tx_params_rel8->n_srs_initial, end) &&
          pull8(ppReadPackedMsg, &init_tx_params_rel8->initial_number_of_resource_blocks, end));
}
static uint8_t unpack_ul_config_ulsch_harq_info_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_harq_information_rel10_t *harq_info_rel10 = (nfapi_ul_config_ulsch_harq_information_rel10_t *)tlv;
  return (pull8(ppReadPackedMsg, &harq_info_rel10->harq_size, end) &&
          pull8(ppReadPackedMsg, &harq_info_rel10->delta_offset_harq, end) &&
          pull8(ppReadPackedMsg, &harq_info_rel10->ack_nack_mode, end));
}

static uint8_t unpack_ul_config_ulsch_harq_info_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_ulsch_harq_information_rel13_t *harq_info_rel13 = (nfapi_ul_config_ulsch_harq_information_rel13_t *)tlv;
  return (pull16(ppReadPackedMsg, &harq_info_rel13->harq_size_2, end) &&
          pull8(ppReadPackedMsg, &harq_info_rel13->delta_offset_harq_2, end));
}

static uint8_t unpack_ul_config_ue_info_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_ue_information_rel8_t *ue_info_rel8 = (nfapi_ul_config_ue_information_rel8_t *)tlv;
  return (pull32(ppReadPackedMsg, &ue_info_rel8->handle, end) &&
          pull16(ppReadPackedMsg, (uint16_t *)&ue_info_rel8->rnti, end));
}
static uint8_t unpack_ul_config_ue_info_rel11_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_ue_information_rel11_t *ue_info_rel11 = (nfapi_ul_config_ue_information_rel11_t *)tlv;
  return (pull8(ppReadPackedMsg, &ue_info_rel11->virtual_cell_id_enabled_flag, end) &&
          pull16(ppReadPackedMsg, &ue_info_rel11->npusch_identity, end));
}
static uint8_t unpack_ul_config_ue_info_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_ue_information_rel13_t *ue_info_rel13 = (nfapi_ul_config_ue_information_rel13_t *)tlv;
  return (pull8(ppReadPackedMsg, &ue_info_rel13->ue_type, end) &&
          pull8(ppReadPackedMsg, &ue_info_rel13->empty_symbols, end) &&
          pull16(ppReadPackedMsg, &ue_info_rel13->total_number_of_repetitions, end) &&
          pull16(ppReadPackedMsg, &ue_info_rel13->repetition_number, end));
}

static uint8_t unpack_ul_config_cqi_info_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_information_rel8_t *cqi_info_rel8 = (nfapi_ul_config_cqi_information_rel8_t *)tlv;
  return ( pull16(ppReadPackedMsg, &cqi_info_rel8->pucch_index, end) &&
           pull8(ppReadPackedMsg, &cqi_info_rel8->dl_cqi_pmi_size, end));
}
static uint8_t unpack_ul_config_cqi_info_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_information_rel10_t *cqi_info_rel10 = (nfapi_ul_config_cqi_information_rel10_t *)tlv;
  return (pull8(ppReadPackedMsg, &cqi_info_rel10->number_of_pucch_resource, end) &&
          pull16(ppReadPackedMsg, &cqi_info_rel10->pucch_index_p1, end));
}
static uint8_t unpack_ul_config_cqi_info_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_cqi_information_rel13_t *cqi_info_rel13 = (nfapi_ul_config_cqi_information_rel13_t *)tlv;
  return (pull8(ppReadPackedMsg, &cqi_info_rel13->csi_mode, end) &&
          pull16(ppReadPackedMsg, &cqi_info_rel13->dl_cqi_pmi_size_2, end) &&
          pull8(ppReadPackedMsg, &cqi_info_rel13->starting_prb, end) &&
          pull8(ppReadPackedMsg, &cqi_info_rel13->n_prb, end) &&
          pull8(ppReadPackedMsg, &cqi_info_rel13->cdm_index, end) &&
          pull8(ppReadPackedMsg, &cqi_info_rel13->n_srs, end));
}

static uint8_t unpack_ul_config_sr_info_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_sr_information_rel8_t *sr_info_rel8 = (nfapi_ul_config_sr_information_rel8_t *)tlv;
  return ( pull16(ppReadPackedMsg, &sr_info_rel8->pucch_index, end));
}

static uint8_t unpack_ul_config_sr_info_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_sr_information_rel10_t *sr_info_rel10 = (nfapi_ul_config_sr_information_rel10_t *)tlv;
  return (pull8(ppReadPackedMsg, &sr_info_rel10->number_of_pucch_resources, end) &&
          pull16(ppReadPackedMsg, &sr_info_rel10->pucch_index_p1, end));
}

static uint8_t unpack_ul_config_harq_info_rel10_tdd_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_harq_information_rel10_tdd_t *harq_info_tdd_rel10 = (nfapi_ul_config_harq_information_rel10_tdd_t *)tlv;
  return (pull8(ppReadPackedMsg, &harq_info_tdd_rel10->harq_size, end) &&
          pull8(ppReadPackedMsg, &harq_info_tdd_rel10->ack_nack_mode, end) &&
          pull8(ppReadPackedMsg, &harq_info_tdd_rel10->number_of_pucch_resources, end) &&
          pull16(ppReadPackedMsg, &harq_info_tdd_rel10->n_pucch_1_0, end) &&
          pull16(ppReadPackedMsg, &harq_info_tdd_rel10->n_pucch_1_1, end) &&
          pull16(ppReadPackedMsg, &harq_info_tdd_rel10->n_pucch_1_2, end) &&
          pull16(ppReadPackedMsg, &harq_info_tdd_rel10->n_pucch_1_3, end));
}

static uint8_t unpack_ul_config_harq_info_rel8_fdd_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_harq_information_rel8_fdd_t *harq_info_fdd_rel8 = (nfapi_ul_config_harq_information_rel8_fdd_t *)tlv;
  return (pull16(ppReadPackedMsg, &harq_info_fdd_rel8->n_pucch_1_0, end) &&
          pull8(ppReadPackedMsg, &harq_info_fdd_rel8->harq_size, end));
}

static uint8_t unpack_ul_config_harq_info_rel9_fdd_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_harq_information_rel9_fdd_t *harq_info_fdd_rel9 = (nfapi_ul_config_harq_information_rel9_fdd_t *)tlv;
  return (pull8(ppReadPackedMsg, &harq_info_fdd_rel9->harq_size, end) &&
          pull8(ppReadPackedMsg, &harq_info_fdd_rel9->ack_nack_mode, end) &&
          pull8(ppReadPackedMsg, &harq_info_fdd_rel9->number_of_pucch_resources, end) &&
          pull16(ppReadPackedMsg, &harq_info_fdd_rel9->n_pucch_1_0, end) &&
          pull16(ppReadPackedMsg, &harq_info_fdd_rel9->n_pucch_1_1, end) &&
          pull16(ppReadPackedMsg, &harq_info_fdd_rel9->n_pucch_1_2, end) &&
          pull16(ppReadPackedMsg, &harq_info_fdd_rel9->n_pucch_1_3, end));
}

static uint8_t unpack_ul_config_harq_info_rel11_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_harq_information_rel11_t *harq_info_rel11 = (nfapi_ul_config_harq_information_rel11_t *)tlv;
  return (pull8(ppReadPackedMsg, &harq_info_rel11->num_ant_ports, end) &&
          pull16(ppReadPackedMsg, &harq_info_rel11->n_pucch_2_0, end) &&
          pull16(ppReadPackedMsg, &harq_info_rel11->n_pucch_2_1, end) &&
          pull16(ppReadPackedMsg, &harq_info_rel11->n_pucch_2_2, end) &&
          pull16(ppReadPackedMsg, &harq_info_rel11->n_pucch_2_3, end));
}

static uint8_t unpack_ul_config_harq_info_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_harq_information_rel13_t *harq_info_rel13 = (nfapi_ul_config_harq_information_rel13_t *)tlv;
  return (pull16(ppReadPackedMsg, &harq_info_rel13->harq_size_2, end) &&
          pull8(ppReadPackedMsg, &harq_info_rel13->starting_prb, end) &&
          pull8(ppReadPackedMsg, &harq_info_rel13->n_prb, end) &&
          pull8(ppReadPackedMsg, &harq_info_rel13->cdm_index, end) &&
          pull8(ppReadPackedMsg, &harq_info_rel13->n_srs, end));
}


static uint8_t unpack_ul_config_srs_pdu_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_srs_pdu_rel8_t *srs_pdu_rel8 = (nfapi_ul_config_srs_pdu_rel8_t *)tlv;
  return (pull32(ppReadPackedMsg, &srs_pdu_rel8->handle, end) &&
          pull16(ppReadPackedMsg, &srs_pdu_rel8->size, end) &&
          pull16(ppReadPackedMsg, &srs_pdu_rel8->rnti, end) &&
          pull8(ppReadPackedMsg, &srs_pdu_rel8->srs_bandwidth, end) &&
          pull8(ppReadPackedMsg, &srs_pdu_rel8->frequency_domain_position, end) &&
          pull8(ppReadPackedMsg, &srs_pdu_rel8->srs_hopping_bandwidth, end) &&
          pull8(ppReadPackedMsg, &srs_pdu_rel8->transmission_comb, end) &&
          pull16(ppReadPackedMsg, &srs_pdu_rel8->i_srs, end) &&
          pull8(ppReadPackedMsg, &srs_pdu_rel8->sounding_reference_cyclic_shift, end));
}

static uint8_t unpack_ul_config_srs_pdu_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_srs_pdu_rel10_t *srs_pdu_rel10 = (nfapi_ul_config_srs_pdu_rel10_t *)tlv;
  return pull8(ppReadPackedMsg, &srs_pdu_rel10->antenna_port, end);
}

static uint8_t unpack_ul_config_srs_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_srs_pdu_rel13_t *srs_pdu_rel13 = (nfapi_ul_config_srs_pdu_rel13_t *)tlv;
  return (pull8(ppReadPackedMsg, &srs_pdu_rel13->number_of_combs, end));
}

static uint8_t unpack_ul_nb_harq_info_rel13_fdd_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_nb_harq_information_rel13_fdd_t *nb_harq_info_fdd_rel13 = (nfapi_ul_config_nb_harq_information_rel13_fdd_t *)tlv;
  return (pull8(ppReadPackedMsg, &nb_harq_info_fdd_rel13->harq_ack_resource, end));
}

static uint8_t unpack_ul_config_nulsch_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_nulsch_pdu_rel13_t *nulsch_pdu_rel13 = (nfapi_ul_config_nulsch_pdu_rel13_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &nulsch_pdu_rel13->nulsch_format, end) &&
       pull32(ppReadPackedMsg, &nulsch_pdu_rel13->handle, end) &&
       pull16(ppReadPackedMsg, &nulsch_pdu_rel13->size, end) &&
       pull16(ppReadPackedMsg, &nulsch_pdu_rel13->rnti, end) &&
       pull8(ppReadPackedMsg, &nulsch_pdu_rel13->subcarrier_indication, end) &&
       pull8(ppReadPackedMsg, &nulsch_pdu_rel13->resource_assignment, end) &&
       pull8(ppReadPackedMsg, &nulsch_pdu_rel13->mcs, end) &&
       pull8(ppReadPackedMsg, &nulsch_pdu_rel13->redudancy_version, end) &&
       pull8(ppReadPackedMsg, &nulsch_pdu_rel13->repetition_number, end) &&
       pull8(ppReadPackedMsg, &nulsch_pdu_rel13->new_data_indication, end) &&
       pull8(ppReadPackedMsg, &nulsch_pdu_rel13->n_srs, end) &&
       pull16(ppReadPackedMsg, &nulsch_pdu_rel13->scrambling_sequence_initialization_cinit, end) &&
       pull16(ppReadPackedMsg, &nulsch_pdu_rel13->sf_idx, end)))
    return 0;

  unpack_tlv_t unpack_fns[] = {
    { NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG, &nulsch_pdu_rel13->ue_information.ue_information_rel8, &unpack_ul_config_ue_info_rel8_value},
    { NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL11_TAG, &nulsch_pdu_rel13->ue_information.ue_information_rel11, &unpack_ul_config_ue_info_rel11_value},
    { NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL13_TAG, &nulsch_pdu_rel13->ue_information.ue_information_rel13, &unpack_ul_config_ue_info_rel13_value},
    { NFAPI_UL_CONFIG_REQUEST_NB_HARQ_INFORMATION_REL13_FDD_TAG, &nulsch_pdu_rel13->nb_harq_information.nb_harq_information_rel13_fdd, &unpack_ul_nb_harq_info_rel13_fdd_value},
  };
  return unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, 0, 0);
}

static uint8_t unpack_ul_config_nrach_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_config_nrach_pdu_rel13_t *nrach_pdu_rel13 = (nfapi_ul_config_nrach_pdu_rel13_t *)tlv;
  return (pull8(ppReadPackedMsg, &nrach_pdu_rel13->nprach_config_0, end) &&
          pull8(ppReadPackedMsg, &nrach_pdu_rel13->nprach_config_1, end) &&
          pull8(ppReadPackedMsg, &nrach_pdu_rel13->nprach_config_2, end));
}


static uint8_t unpack_ul_config_request_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
#define UL_CONFIG_ULSCH_PDU_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL8_TAG, &_pdu.ulsch_pdu_rel8, &unpack_ul_config_ulsch_pdu_rel8_value}, \
  { NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL10_TAG, &_pdu.ulsch_pdu_rel10, &unpack_ul_config_ulsch_pdu_rel10_value}, \
  { NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL11_TAG, &_pdu.ulsch_pdu_rel11, &unpack_ul_config_ulsch_pdu_rel11_value}, \
  { NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL13_TAG, &_pdu.ulsch_pdu_rel13, &unpack_ul_config_ulsch_pdu_rel13_value},
#define UL_CONFIG_CQI_RI_INFO_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL8_TAG, &_pdu.cqi_ri_information_rel8, &unpack_ul_config_cqi_ri_info_rel8_value}, \
  { NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL9_TAG, &_pdu.cqi_ri_information_rel9, &unpack_ul_config_cqi_ri_info_rel9_value}, \
  { NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL13_TAG, &_pdu.cqi_ri_information_rel13, &unpack_ul_config_cqi_ri_info_rel13_value},
#define UL_CONFIG_ULSCH_HARQ_INFO_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_ULSCH_HARQ_INFORMATION_REL10_TAG, &_pdu.harq_information_rel10, &unpack_ul_config_ulsch_harq_info_rel10_value},\
  { NFAPI_UL_CONFIG_REQUEST_ULSCH_HARQ_INFORMATION_REL13_TAG, &_pdu.harq_information_rel13, &unpack_ul_config_ulsch_harq_info_rel13_value},
#define UL_CONFIG_INIT_TX_PARAMS_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG, &_pdu.initial_transmission_parameters_rel8, &unpack_ul_config_cqi_init_tx_params_rel8_value},
#define UL_CONFIG_UCI_UE_INFO_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG, &_pdu.ue_information_rel8, &unpack_ul_config_ue_info_rel8_value}, \
  { NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL11_TAG, &_pdu.ue_information_rel11, &unpack_ul_config_ue_info_rel11_value}, \
  { NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL13_TAG, &_pdu.ue_information_rel13, &unpack_ul_config_ue_info_rel13_value},
#define UL_CONFIG_UCI_CQI_INFO_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL8_TAG, &_pdu.cqi_information_rel8, &unpack_ul_config_cqi_info_rel8_value}, \
  { NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL10_TAG, &_pdu.cqi_information_rel10, &unpack_ul_config_cqi_info_rel10_value}, \
  { NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL13_TAG, &_pdu.cqi_information_rel13, &unpack_ul_config_cqi_info_rel13_value},
#define UL_CONFIG_UCI_SR_INFO_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL8_TAG, &_pdu.sr_information_rel8, &unpack_ul_config_sr_info_rel8_value}, \
  { NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL10_TAG, &_pdu.sr_information_rel10, &unpack_ul_config_sr_info_rel10_value},
#define UL_CONFIG_UCI_HARQ_INFO_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL10_TDD_TAG, &_pdu.harq_information_rel10_tdd, &unpack_ul_config_harq_info_rel10_tdd_value}, \
  { NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL8_FDD_TAG, &_pdu.harq_information_rel8_fdd, &unpack_ul_config_harq_info_rel8_fdd_value}, \
  { NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL9_FDD_TAG, &_pdu.harq_information_rel9_fdd, &unpack_ul_config_harq_info_rel9_fdd_value}, \
  { NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL11_TAG, &_pdu.harq_information_rel11, &unpack_ul_config_harq_info_rel11_value}, \
  { NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL13_TAG, &_pdu.harq_information_rel13, &unpack_ul_config_harq_info_rel13_value},
#define UL_CONFIG_SRS_PDU_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL8_TAG, &_pdu.srs_pdu_rel8, &unpack_ul_config_srs_pdu_rel8_value}, \
  { NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL10_TAG, &_pdu.srs_pdu_rel10, &unpack_ul_config_srs_pdu_rel10_value}, \
  { NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL13_TAG, &_pdu.srs_pdu_rel13, &unpack_ul_config_srs_pdu_rel13_value},
#define UL_CONFIG_NULSCH_PDU_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_NULSCH_PDU_REL13_TAG, &_pdu.nulsch_pdu_rel13, &unpack_ul_config_nulsch_pdu_rel13_value},
#define UL_CONFIG_NRACH_PDU_UNPACK_FNS(_pdu) \
  { NFAPI_UL_CONFIG_REQUEST_NRACH_PDU_REL13_TAG, &_pdu.nrach_pdu_rel13, &unpack_ul_config_nrach_pdu_rel13_value},
  nfapi_ul_config_request_body_t *value = (nfapi_ul_config_request_body_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->number_of_pdus, end) &&
       pull8(ppReadPackedMsg, &value->rach_prach_frequency_resources, end) &&
       pull8(ppReadPackedMsg, &value->srs_present, end)))
    return 0;

  if(value->number_of_pdus > NFAPI_UL_CONFIG_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of ul config pdus exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_pdus, NFAPI_UL_CONFIG_MAX_PDU);
    return 0;
  }

  if(value->number_of_pdus > 0) {
    value->ul_config_pdu_list = (nfapi_ul_config_request_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_ul_config_request_pdu_t) * value->number_of_pdus, config);

    if(value->ul_config_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate ul config pdu list (count:%d)\n", __FUNCTION__, value->number_of_pdus);
      return 0;
    }
  } else {
    value->ul_config_pdu_list = 0;
  }

  uint16_t i;
  uint16_t total_number_of_pdus = value->number_of_pdus;

  for(i = 0; i < total_number_of_pdus; ++i) {
    nfapi_ul_config_request_pdu_t *pdu = &(value->ul_config_pdu_list[i]);

    if(!(pull8(ppReadPackedMsg, &pdu->pdu_type, end) &&
         pull8(ppReadPackedMsg, &pdu->pdu_size, end)))
      return 0;

    uint8_t *packedPduEnd = (*ppReadPackedMsg) + pdu->pdu_size - 2;

    if(packedPduEnd > end) {
      // pdu end is past buffer end
      return 0;
    }

    switch(pdu->pdu_type) {
      case NFAPI_UL_CONFIG_ULSCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_ULSCH_PDU_UNPACK_FNS(pdu->ulsch_pdu)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_ULSCH_PDU_UNPACK_FNS(pdu->ulsch_cqi_ri_pdu.ulsch_pdu)
          UL_CONFIG_CQI_RI_INFO_UNPACK_FNS(pdu->ulsch_cqi_ri_pdu.cqi_ri_information)
          UL_CONFIG_INIT_TX_PARAMS_UNPACK_FNS(pdu->ulsch_cqi_ri_pdu.initial_transmission_parameters)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_ULSCH_PDU_UNPACK_FNS(pdu->ulsch_harq_pdu.ulsch_pdu)
          UL_CONFIG_ULSCH_HARQ_INFO_UNPACK_FNS(pdu->ulsch_harq_pdu.harq_information)
          UL_CONFIG_INIT_TX_PARAMS_UNPACK_FNS(pdu->ulsch_harq_pdu.initial_transmission_parameters)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_ULSCH_PDU_UNPACK_FNS(pdu->ulsch_cqi_harq_ri_pdu.ulsch_pdu)
          UL_CONFIG_CQI_RI_INFO_UNPACK_FNS(pdu->ulsch_cqi_harq_ri_pdu.cqi_ri_information)
          UL_CONFIG_ULSCH_HARQ_INFO_UNPACK_FNS(pdu->ulsch_cqi_harq_ri_pdu.harq_information)
          UL_CONFIG_INIT_TX_PARAMS_UNPACK_FNS(pdu->ulsch_cqi_harq_ri_pdu.initial_transmission_parameters)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_UCI_UE_INFO_UNPACK_FNS(pdu->uci_cqi_pdu.ue_information)
          UL_CONFIG_UCI_CQI_INFO_UNPACK_FNS(pdu->uci_cqi_pdu.cqi_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_UCI_UE_INFO_UNPACK_FNS(pdu->uci_sr_pdu.ue_information)
          UL_CONFIG_UCI_SR_INFO_UNPACK_FNS(pdu->uci_sr_pdu.sr_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_UCI_UE_INFO_UNPACK_FNS(pdu->uci_harq_pdu.ue_information)
          UL_CONFIG_UCI_HARQ_INFO_UNPACK_FNS(pdu->uci_harq_pdu.harq_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_UCI_UE_INFO_UNPACK_FNS(pdu->uci_sr_harq_pdu.ue_information)
          UL_CONFIG_UCI_SR_INFO_UNPACK_FNS(pdu->uci_sr_harq_pdu.sr_information)
          UL_CONFIG_UCI_HARQ_INFO_UNPACK_FNS(pdu->uci_sr_harq_pdu.harq_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_UCI_UE_INFO_UNPACK_FNS(pdu->uci_cqi_harq_pdu.ue_information)
          UL_CONFIG_UCI_CQI_INFO_UNPACK_FNS(pdu->uci_cqi_harq_pdu.cqi_information)
          UL_CONFIG_UCI_HARQ_INFO_UNPACK_FNS(pdu->uci_cqi_harq_pdu.harq_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_UCI_UE_INFO_UNPACK_FNS(pdu->uci_cqi_sr_pdu.ue_information)
          UL_CONFIG_UCI_CQI_INFO_UNPACK_FNS(pdu->uci_cqi_sr_pdu.cqi_information)
          UL_CONFIG_UCI_SR_INFO_UNPACK_FNS(pdu->uci_cqi_sr_pdu.sr_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_UCI_CQI_SR_HARQ_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_UCI_UE_INFO_UNPACK_FNS(pdu->uci_cqi_sr_harq_pdu.ue_information)
          UL_CONFIG_UCI_CQI_INFO_UNPACK_FNS(pdu->uci_cqi_sr_harq_pdu.cqi_information)
          UL_CONFIG_UCI_SR_INFO_UNPACK_FNS(pdu->uci_cqi_sr_harq_pdu.sr_information)
          UL_CONFIG_UCI_HARQ_INFO_UNPACK_FNS(pdu->uci_cqi_sr_harq_pdu.harq_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_SRS_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_SRS_PDU_UNPACK_FNS(pdu->srs_pdu)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_HARQ_BUFFER_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_UCI_UE_INFO_UNPACK_FNS(pdu->harq_buffer_pdu.ue_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_UCI_CSI_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_ULSCH_PDU_UNPACK_FNS(pdu->ulsch_uci_csi_pdu.ulsch_pdu)
          UL_CONFIG_UCI_CQI_INFO_UNPACK_FNS(pdu->ulsch_uci_csi_pdu.csi_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_UCI_HARQ_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_ULSCH_PDU_UNPACK_FNS(pdu->ulsch_uci_harq_pdu.ulsch_pdu)
          UL_CONFIG_UCI_HARQ_INFO_UNPACK_FNS(pdu->ulsch_uci_harq_pdu.harq_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_ULSCH_CSI_UCI_HARQ_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_ULSCH_PDU_UNPACK_FNS(pdu->ulsch_csi_uci_harq_pdu.ulsch_pdu)
          UL_CONFIG_UCI_CQI_INFO_UNPACK_FNS(pdu->ulsch_csi_uci_harq_pdu.csi_information)
          UL_CONFIG_UCI_HARQ_INFO_UNPACK_FNS(pdu->ulsch_csi_uci_harq_pdu.harq_information)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_NULSCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_NULSCH_PDU_UNPACK_FNS(pdu->nulsch_pdu)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_UL_CONFIG_NRACH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          UL_CONFIG_NRACH_PDU_UNPACK_FNS(pdu->nrach_pdu)
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;
    }
  }

  return 1;
}


static uint8_t unpack_ul_config_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_ul_config_request_t *pNfapiMsg = (nfapi_ul_config_request_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_UL_CONFIG_REQUEST_BODY_TAG, &pNfapiMsg->ul_config_request_body, &unpack_ul_config_request_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_hi_dci0_hi_pdu_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_hi_dci0_hi_pdu_rel8_t *hi_pdu_rel8 = (nfapi_hi_dci0_hi_pdu_rel8_t *)tlv;
  return( pull8(ppReadPackedMsg, &hi_pdu_rel8->resource_block_start, end) &&
          pull8(ppReadPackedMsg, &hi_pdu_rel8->cyclic_shift_2_for_drms, end) &&
          pull8(ppReadPackedMsg, &hi_pdu_rel8->hi_value, end) &&
          pull8(ppReadPackedMsg, &hi_pdu_rel8->i_phich, end) &&
          pull16(ppReadPackedMsg, &hi_pdu_rel8->transmission_power, end));
}

static uint8_t unpack_hi_dci0_hi_pdu_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_hi_dci0_hi_pdu_rel10_t *hi_pdu_rel10 = (nfapi_hi_dci0_hi_pdu_rel10_t *)tlv;
  return (pull8(ppReadPackedMsg, &hi_pdu_rel10->flag_tb2, end) &&
          pull8(ppReadPackedMsg, &hi_pdu_rel10->hi_value_2, end));
}

static uint8_t unpack_hi_dci0_dci_pdu_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_hi_dci0_dci_pdu_rel8_t *dci_pdu_rel8 = (nfapi_hi_dci0_dci_pdu_rel8_t *)tlv;
  return (pull8(ppReadPackedMsg, &dci_pdu_rel8->dci_format, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->cce_index, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->aggregation_level, end) &&
          pull16(ppReadPackedMsg, &dci_pdu_rel8->rnti, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->resource_block_start, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->number_of_resource_block, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->mcs_1, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->cyclic_shift_2_for_drms, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->frequency_hopping_enabled_flag, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->frequency_hopping_bits, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->new_data_indication_1, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->ue_tx_antenna_seleciton, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->tpc, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->cqi_csi_request, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->ul_index, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel8->dl_assignment_index, end) &&
          pull32(ppReadPackedMsg, &dci_pdu_rel8->tpc_bitmap, end) &&
          pull16(ppReadPackedMsg, &dci_pdu_rel8->transmission_power, end));
}

static uint8_t unpack_hi_dci0_dci_pdu_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_hi_dci0_dci_pdu_rel10_t *dci_pdu_rel10 = (nfapi_hi_dci0_dci_pdu_rel10_t *)tlv;
  return (pull8(ppReadPackedMsg, &dci_pdu_rel10->cross_carrier_scheduling_flag, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->carrier_indicator, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->size_of_cqi_csi_feild, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->srs_flag, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->srs_request, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->resource_allocation_flag, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->resource_allocation_type, end) &&
          pull32(ppReadPackedMsg, &dci_pdu_rel10->resource_block_coding, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->mcs_2, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->new_data_indication_2, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->number_of_antenna_ports, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->tpmi, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->total_dci_length_including_padding, end) &&
          pull8(ppReadPackedMsg, &dci_pdu_rel10->n_ul_rb, end));
}

static uint8_t unpack_hi_dci0_dci_pdu_rel12_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_hi_dci0_dci_pdu_rel12_t *dci_pdu_rel12 = (nfapi_hi_dci0_dci_pdu_rel12_t *)tlv;
  return ( pull8(ppReadPackedMsg, &dci_pdu_rel12->pscch_resource, end) &&
           pull8(ppReadPackedMsg, &dci_pdu_rel12->time_resource_pattern, end));
}

static uint8_t unpack_hi_dci0_mpdcch_dci_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_hi_dci0_mpdcch_dci_pdu_rel13_t *value = (nfapi_hi_dci0_mpdcch_dci_pdu_rel13_t *)tlv;
  return (pull8(ppReadPackedMsg, &value->mpdcch_narrowband, end) &&
          pull8(ppReadPackedMsg, &value->number_of_prb_pairs, end) &&
          pull8(ppReadPackedMsg, &value->resource_block_assignment, end) &&
          pull8(ppReadPackedMsg, &value->mpdcch_transmission_type, end) &&
          pull8(ppReadPackedMsg, &value->start_symbol, end) &&
          pull8(ppReadPackedMsg, &value->ecce_index, end) &&
          pull8(ppReadPackedMsg, &value->aggreagation_level, end) &&
          pull8(ppReadPackedMsg, &value->rnti_type, end) &&
          pull16(ppReadPackedMsg, &value->rnti, end) &&
          pull8(ppReadPackedMsg, &value->ce_mode, end) &&
          pull16(ppReadPackedMsg, &value->drms_scrambling_init, end) &&
          pull16(ppReadPackedMsg, &value->initial_transmission_sf_io, end) &&
          pull16(ppReadPackedMsg, &value->transmission_power, end) &&
          pull8(ppReadPackedMsg, &value->dci_format, end) &&
          pull8(ppReadPackedMsg, &value->resource_block_start, end) &&
          pull8(ppReadPackedMsg, &value->number_of_resource_blocks, end) &&
          pull8(ppReadPackedMsg, &value->mcs, end) &&
          pull8(ppReadPackedMsg, &value->pusch_repetition_levels, end) &&
          pull8(ppReadPackedMsg, &value->frequency_hopping_flag, end) &&
          pull8(ppReadPackedMsg, &value->new_data_indication, end) &&
          pull8(ppReadPackedMsg, &value->harq_process, end) &&
          pull8(ppReadPackedMsg, &value->redudency_version, end) &&
          pull8(ppReadPackedMsg, &value->tpc, end) &&
          pull8(ppReadPackedMsg, &value->csi_request, end) &&
          pull8(ppReadPackedMsg, &value->ul_inex, end) &&
          pull8(ppReadPackedMsg, &value->dai_presence_flag, end) &&
          pull8(ppReadPackedMsg, &value->dl_assignment_index, end) &&
          pull8(ppReadPackedMsg, &value->srs_request, end) &&
          pull8(ppReadPackedMsg, &value->dci_subframe_repetition_number, end) &&
          pull32(ppReadPackedMsg, &value->tcp_bitmap, end) &&
          pull8(ppReadPackedMsg, &value->total_dci_length_include_padding, end) &&
          pull8(ppReadPackedMsg, &value->number_of_tx_antenna_ports, end) &&
          pullarray16(ppReadPackedMsg, value->precoding_value, NFAPI_MAX_TX_PHYSICAL_ANTENNA_PORTS, value->number_of_tx_antenna_ports, end));
}

static uint8_t unpack_hi_dci0_npdcch_dci_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_hi_dci0_npdcch_dci_pdu_rel13_t *value = (nfapi_hi_dci0_npdcch_dci_pdu_rel13_t *)tlv;
  return (pull8(ppReadPackedMsg, &value->ncce_index, end) &&
          pull8(ppReadPackedMsg, &value->aggregation_level, end) &&
          pull8(ppReadPackedMsg, &value->start_symbol, end) &&
          pull16(ppReadPackedMsg, &value->rnti, end) &&
          pull8(ppReadPackedMsg, &value->scrambling_reinitialization_batch_index, end) &&
          pull8(ppReadPackedMsg, &value->nrs_antenna_ports_assumed_by_the_ue, end) &&
          pull8(ppReadPackedMsg, &value->subcarrier_indication, end) &&
          pull8(ppReadPackedMsg, &value->resource_assignment, end) &&
          pull8(ppReadPackedMsg, &value->scheduling_delay, end) &&
          pull8(ppReadPackedMsg, &value->mcs, end) &&
          pull8(ppReadPackedMsg, &value->redudancy_version, end) &&
          pull8(ppReadPackedMsg, &value->repetition_number, end) &&
          pull8(ppReadPackedMsg, &value->new_data_indicator, end) &&
          pull8(ppReadPackedMsg, &value->dci_subframe_repetition_number, end));
}

static uint8_t unpack_hi_dci0_request_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_hi_dci0_request_body_t *value = (nfapi_hi_dci0_request_body_t *)tlv;

  if(!(pull16(ppReadPackedMsg, &value->sfnsf, end) &&
       pull8(ppReadPackedMsg, &value->number_of_dci, end) &&
       pull8(ppReadPackedMsg, &value->number_of_hi, end)))
    return 0;

  uint8_t totalNumPdus = value->number_of_hi + value->number_of_dci;

  if(totalNumPdus > NFAPI_HI_DCI0_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of dci0 pdu's exceed maxium (count:%d max:%d)\n", __FUNCTION__, totalNumPdus, NFAPI_HI_DCI0_MAX_PDU);
    return 0;
  }

  if(totalNumPdus > 0) {
    value->hi_dci0_pdu_list = (nfapi_hi_dci0_request_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_hi_dci0_request_pdu_t) * totalNumPdus, config);

    if(value->hi_dci0_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate hi dci0 pdu list (count:%d)\n", __FUNCTION__, totalNumPdus);
      return 0;
    }
  } else {
    value->hi_dci0_pdu_list = 0;
  }

  uint8_t i;

  for(i = 0; i < totalNumPdus; ++i) {
    nfapi_hi_dci0_request_pdu_t *pdu = &(value->hi_dci0_pdu_list[i]);

    if(!(pull8(ppReadPackedMsg, &pdu->pdu_type, end) &&
         pull8(ppReadPackedMsg, &pdu->pdu_size, end)))
      return 0;

    uint8_t *packedPduEnd = (*ppReadPackedMsg) + pdu->pdu_size - 2;

    if(packedPduEnd > end) {
      // pdu end if past buffer end
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s pdu size to big %d %d\n", __FUNCTION__, packedPduEnd, end);
      return 0;
    }

    switch(pdu->pdu_type) {
      case NFAPI_HI_DCI0_HI_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_HI_DCI0_REQUEST_HI_PDU_REL8_TAG, &pdu->hi_pdu.hi_pdu_rel8, &unpack_hi_dci0_hi_pdu_rel8_value},
          { NFAPI_HI_DCI0_REQUEST_HI_PDU_REL10_TAG, &pdu->hi_pdu.hi_pdu_rel10, &unpack_hi_dci0_hi_pdu_rel10_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_HI_DCI0_DCI_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG, &pdu->dci_pdu.dci_pdu_rel8, &unpack_hi_dci0_dci_pdu_rel8_value},
          { NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL10_TAG, &pdu->dci_pdu.dci_pdu_rel10, &unpack_hi_dci0_dci_pdu_rel10_value},
          { NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL12_TAG, &pdu->dci_pdu.dci_pdu_rel12, &unpack_hi_dci0_dci_pdu_rel12_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_HI_DCI0_EPDCCH_DCI_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_HI_DCI0_REQUEST_EPDCCH_DCI_PDU_REL8_TAG, &pdu->epdcch_dci_pdu.epdcch_dci_pdu_rel8, &unpack_hi_dci0_dci_pdu_rel8_value},
          { NFAPI_HI_DCI0_REQUEST_EPDCCH_DCI_PDU_REL10_TAG, &pdu->epdcch_dci_pdu.epdcch_dci_pdu_rel10, &unpack_hi_dci0_dci_pdu_rel10_value},
          { NFAPI_HI_DCI0_REQUEST_EPDCCH_PARAMETERS_REL11_TAG, &pdu->epdcch_dci_pdu.epdcch_parameters_rel11, &unpack_dl_config_epdcch_params_rel11_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_HI_DCI0_MPDCCH_DCI_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_HI_DCI0_REQUEST_MPDCCH_DCI_PDU_REL13_TAG, &pdu->mpdcch_dci_pdu.mpdcch_dci_pdu_rel13, &unpack_hi_dci0_mpdcch_dci_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_HI_DCI0_NPDCCH_DCI_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_HI_DCI0_REQUEST_NPDCCH_DCI_PDU_REL13_TAG, &pdu->npdcch_dci_pdu.npdcch_dci_pdu_rel13, &unpack_hi_dci0_npdcch_dci_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid pdu type %d \n", pdu->pdu_type );
      }
      break;
    };
  }

  return 1;
}
//unpack_ul_dci_pdu_list_value

static uint8_t unpack_ul_dci_pdu_list_value(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg) {
  nfapi_nr_ul_dci_request_pdus_t *value = (nfapi_nr_ul_dci_request_pdus_t *)msg;

  for(uint8_t i = 0; i < MAX_DCI_CORESET; ++i) {
    if(!pull16(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].RNTI,  end) &&
        pull16(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].ScramblingId, end) &&
        pull16(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].ScramblingRNTI, end) &&
        pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].CceIndex, end) &&
        pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].AggregationLevel, end) &&
        pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].beta_PDCCH_1_0, end) &&
        pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].powerControlOffsetSS, end) &&
        pull16(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].PayloadSizeBits, end) &&
        pullarray8(ppReadPackedMsg, value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].Payload, DCI_PAYLOAD_BYTE_LEN, value->pdcch_pdu.pdcch_pdu_rel15.dci_pdu[i].PayloadSizeBits, end));

    return 0;
  }

  return (pull16(ppReadPackedMsg, &value->PDUType, end) &&
          pull16(ppReadPackedMsg, &value->PDUSize, end) &&
          pull16(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.BWPSize, end) &&
          pull16(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.BWPStart, end) &&
          pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.SubcarrierSpacing, end) &&
          pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.CyclicPrefix, end) &&
          pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.StartSymbolIndex, end) &&
          pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.DurationSymbols, end) &&
          pullarray8(ppReadPackedMsg, value->pdcch_pdu.pdcch_pdu_rel15.FreqDomainResource, 6, 6, end) &&
          pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.CceRegMappingType, end) &&
          pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.RegBundleSize, end) &&
          pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.InterleaverSize, end) &&
          pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.CoreSetType, end) &&
          pull16(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.ShiftIndex, end) &&
          pull8(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.precoderGranularity, end) &&
          pull16(ppReadPackedMsg, &value->pdcch_pdu.pdcch_pdu_rel15.numDlDci, end));
}

static uint8_t unpack_ul_dci_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_nr_ul_dci_request_t *pNfapiMsg = (nfapi_nr_ul_dci_request_t *)msg;

  if (!(pull16(ppReadPackedMsg, &pNfapiMsg->SFN, end) &&
        pull16(ppReadPackedMsg, &pNfapiMsg->Slot, end) &&
        pull8(ppReadPackedMsg, &pNfapiMsg->numPdus, end)
       ))
    return 0;

  for(int i=0; i< pNfapiMsg->numPdus; i++) {
    if (!unpack_ul_dci_pdu_list_value(ppReadPackedMsg, end, &pNfapiMsg->ul_dci_pdu_list[i]))
      return 0;
  }

  return 1;
}

static uint8_t unpack_hi_dci0_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_hi_dci0_request_t *pNfapiMsg = (nfapi_hi_dci0_request_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_HI_DCI0_REQUEST_BODY_TAG, &pNfapiMsg->hi_dci0_request_body, &unpack_hi_dci0_request_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}
static uint8_t unpack_tx_data_pdu_list_value(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg) {
  nfapi_nr_pdu_t *pNfapiMsg = (nfapi_nr_pdu_t *)msg;

  if(!(pull32(ppReadPackedMsg, &pNfapiMsg->num_TLV, end) &&
       pull16(ppReadPackedMsg, &pNfapiMsg->PDU_index, end) &&
       pull16(ppReadPackedMsg, &pNfapiMsg->PDU_length, end)
      ))
    return 0;

  uint16_t i = 0;
  uint16_t total_number_of_tlvs = pNfapiMsg->num_TLV;

  for(; i < total_number_of_tlvs; ++i) {
    if (!(pull16(ppReadPackedMsg, &pNfapiMsg->TLVs[i].length, end) &&
          pull16(ppReadPackedMsg, &pNfapiMsg->TLVs[i].tag, end)))
      return 0;

    switch(pNfapiMsg->TLVs[i].tag) {
      case 0: {
        if(!pullarray32(ppReadPackedMsg, pNfapiMsg->TLVs[i].value.direct, 16384, pNfapiMsg->TLVs[i].length, end))
          return 0;

        break;
      }

      case 1: {
        if(!pullarray32(ppReadPackedMsg, pNfapiMsg->TLVs[i].value.ptr, pNfapiMsg->TLVs[i].length, pNfapiMsg->TLVs[i].length, end))
          return 0;

        break;
      }

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : Invalid tag value %d \n", pNfapiMsg->TLVs[i].tag );
        break;
      }
    }
  }

  return 1;
}

static uint8_t unpack_tx_data_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_nr_tx_data_request_t *pNfapiMsg = (nfapi_nr_tx_data_request_t *)msg;

  if(!(pull16(ppReadPackedMsg, &pNfapiMsg->SFN, end) &&
       pull16(ppReadPackedMsg, &pNfapiMsg->Slot, end) &&
       pull16(ppReadPackedMsg, &pNfapiMsg->Number_of_PDUs, end)))
    return 0;

  for(int i=0; i< pNfapiMsg->Number_of_PDUs; i++) {
    if (!unpack_tx_data_pdu_list_value(ppReadPackedMsg, end, &pNfapiMsg->pdu_list[i]))
      return 0;
  }

  return 1;
}

static uint8_t unpack_tx_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  uint8_t proceed = 1;
  nfapi_tx_request_t *pNfapiMsg = (nfapi_tx_request_t *)msg;

  if(pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) == 0)
    return 0;

  while (((uint8_t *)(*ppReadPackedMsg) < end) && proceed) {
    nfapi_tl_t generic_tl;

    if(unpack_tl(ppReadPackedMsg, &generic_tl, end) == 0)
      return 0;

    switch(generic_tl.tag) {
      case NFAPI_TX_REQUEST_BODY_TAG: {
        pNfapiMsg->tx_request_body.tl = generic_tl;

        if( pull16(ppReadPackedMsg, &pNfapiMsg->tx_request_body.number_of_pdus, end) == 0)
          return 0;

        if(pNfapiMsg->tx_request_body.number_of_pdus > NFAPI_TX_MAX_PDU) {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of tx pdu's exceed maxium (count:%d max:%d)\n", __FUNCTION__, pNfapiMsg->tx_request_body.number_of_pdus, NFAPI_TX_MAX_PDU);
          return 0;
        }

        if(pNfapiMsg->tx_request_body.number_of_pdus > 0) {
          pNfapiMsg->tx_request_body.tx_pdu_list = (nfapi_tx_request_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_tx_request_pdu_t) * pNfapiMsg->tx_request_body.number_of_pdus, config);

          if(pNfapiMsg->tx_request_body.tx_pdu_list == NULL) {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate tx  pdu list (count:%d)\n", __FUNCTION__, pNfapiMsg->tx_request_body.number_of_pdus);
            return 0;
          }
        } else {
          pNfapiMsg->tx_request_body.tx_pdu_list = 0;
        }

        uint16_t i;
        uint16_t totalNumPdus = pNfapiMsg->tx_request_body.number_of_pdus;

        for(i = 0; i < totalNumPdus; ++i) {
          nfapi_tx_request_pdu_t *pdu = &(pNfapiMsg->tx_request_body.tx_pdu_list[i]);

          if (pdu) {
            uint16_t length = 0;
            uint16_t index = 0;

            if(!(pull16(ppReadPackedMsg, &length, end) &&
                 pull16(ppReadPackedMsg, &index, end)))
              return 0;

            pdu->pdu_length = length;
            pdu->pdu_index = index;
            // TODO : May need to rethink this bit
            pdu->num_segments = 1;
            pdu->segments[0].segment_length = pdu->pdu_length;
            pdu->segments[0].segment_data = nfapi_p7_allocate(pdu->pdu_length, config);

            if(pdu->segments[0].segment_data) {
              if(!pullarray8(ppReadPackedMsg, pdu->segments[0].segment_data, pdu->segments[0].segment_length, pdu->segments[0].segment_length, end))
                return 0;

              if (pdu->segments[0].segment_length == 3) {
                NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() BCH? segment_data:%x %x %x\n", __FUNCTION__,
                            pdu->segments[0].segment_data[0],
                            pdu->segments[0].segment_data[1],
                            pdu->segments[0].segment_data[2]
                           );
              }
            } else {
              NFAPI_TRACE(NFAPI_TRACE_ERROR, "unpack_tx_request: Failed to allocate pdu (len:%d) %d/%d %d\n", pdu->pdu_length, totalNumPdus, i, pdu->pdu_index);
            }
          } else {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "NULL pdu\n");
          }
        }
      }
      break;

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "unpack_tx_request FIXME : Invalid pdu type %d \n", generic_tl.tag );
      }
      break;
    };
  }

  return 1;
}

static uint8_t unpack_ue_release_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  uint8_t proceed = 1;
  nfapi_ue_release_request_t *pNfapiMsg = (nfapi_ue_release_request_t *)msg;

  if(pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) == 0)
    return 0;

  while (((uint8_t *)(*ppReadPackedMsg) < end) && proceed) {
    nfapi_tl_t generic_tl;

    if(unpack_tl(ppReadPackedMsg, &generic_tl, end) == 0)
      return 0;

    switch(generic_tl.tag) {
      case NFAPI_UE_RELEASE_BODY_TAG: {
        pNfapiMsg->ue_release_request_body.tl = generic_tl;

        if( pull16(ppReadPackedMsg, &pNfapiMsg->ue_release_request_body.number_of_TLVs, end) == 0)
          return 0;

        if(pNfapiMsg->ue_release_request_body.number_of_TLVs > NFAPI_RELEASE_MAX_RNTI) {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of relese rnti's exceed maxium (count:%d max:%d)\n", __FUNCTION__, pNfapiMsg->ue_release_request_body.number_of_TLVs, NFAPI_RELEASE_MAX_RNTI);
          return 0;
        } else {
          uint8_t j;
          uint16_t num = pNfapiMsg->ue_release_request_body.number_of_TLVs;

          for(j = 0; j < num; ++j) {
            if(pull16(ppReadPackedMsg, &pNfapiMsg->ue_release_request_body.ue_release_request_TLVs_list[j].rnti, end) == 0) {
              return 0;
            }
          }
        }
      }
      break;

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "unpack_ue_release_request FIXME : Invalid type %d \n", generic_tl.tag );
      }
      break;
    };
  }

  return 1;
}

static uint8_t unpack_harq_indication_tdd_harq_data_bundling(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_harq_indication_tdd_harq_data_bundling_t *value = (nfapi_harq_indication_tdd_harq_data_bundling_t *)tlv;
  return (pull8(ppReadPackedMsg, &value->value_0, end) &&
          pull8(ppReadPackedMsg, &value->value_1, end));
}

static uint8_t unpack_harq_indication_tdd_harq_data_multiplexing(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_harq_indication_tdd_harq_data_multiplexing_t *value = (nfapi_harq_indication_tdd_harq_data_multiplexing_t *)tlv;
  return (pull8(ppReadPackedMsg, &value->value_0, end) &&
          pull8(ppReadPackedMsg, &value->value_1, end) &&
          pull8(ppReadPackedMsg, &value->value_2, end) &&
          pull8(ppReadPackedMsg, &value->value_3, end));
}
static uint8_t unpack_harq_indication_tdd_harq_data_special_bundling(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_harq_indication_tdd_harq_data_special_bundling_t *value = (nfapi_harq_indication_tdd_harq_data_special_bundling_t *)tlv;
  return ( pull8(ppReadPackedMsg, &value->value_0, end));
}
static uint8_t unpack_harq_indication_tdd_harq_data(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_harq_indication_tdd_harq_data_t *value = (nfapi_harq_indication_tdd_harq_data_t *)tlv;
  return  (pull8(ppReadPackedMsg, &value->value_0, end));
}

static uint8_t unpack_harq_indication_tdd_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_harq_indication_tdd_rel8_t *value = (nfapi_harq_indication_tdd_rel8_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->mode, end) &&
       pull8(ppReadPackedMsg, &value->number_of_ack_nack, end)))
    return 0;

  uint8_t result = 0;

  switch(value->mode) {
    case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_BUNDLING:
      result = unpack_harq_indication_tdd_harq_data_bundling(&value->harq_data.bundling, ppReadPackedMsg, end);
      break;

    case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_MULIPLEXING:
      result = unpack_harq_indication_tdd_harq_data_multiplexing(&value->harq_data.multiplex, ppReadPackedMsg, end);
      break;

    case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_SPECIAL_BUNDLING:
      result = unpack_harq_indication_tdd_harq_data_special_bundling(&value->harq_data.special_bundling, ppReadPackedMsg, end);
      break;

    case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_3:
    case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_CHANNEL_SELECTION:
      result = 1;
      break;

    default:
      // TODO add error message
      return 0;
      break;
  }

  return result;
}

static uint8_t unpack_harq_indication_tdd_rel9_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_harq_indication_tdd_rel9_t *value = (nfapi_harq_indication_tdd_rel9_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->mode, end) &&
       pull8(ppReadPackedMsg, &value->number_of_ack_nack, end)))
    return 0;

  if(value->number_of_ack_nack > NFAPI_MAX_NUMBER_ACK_NACK_TDD) {
    // TODO : add error message
    return 0;
  }

  uint16_t idx = 0;

  for(idx = 0; idx < value->number_of_ack_nack; ++idx) {
    uint8_t result = 0;

    switch(value->mode) {
      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_BUNDLING:
        result = unpack_harq_indication_tdd_harq_data(&value->harq_data[idx].bundling, ppReadPackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_MULIPLEXING:
        result = unpack_harq_indication_tdd_harq_data(&value->harq_data[idx].multiplex, ppReadPackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_SPECIAL_BUNDLING:
        result = unpack_harq_indication_tdd_harq_data_special_bundling(&value->harq_data[idx].special_bundling, ppReadPackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_CHANNEL_SELECTION:
        result = unpack_harq_indication_tdd_harq_data(&value->harq_data[idx].channel_selection, ppReadPackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_3:
        result = unpack_harq_indication_tdd_harq_data(&value->harq_data[idx].format_3, ppReadPackedMsg, end);
        break;

      default:
        // TODO add error message
        return 0;
        break;
    }

    if(result == 0)
      return 0;
  }

  return 1;
}

static uint8_t unpack_harq_indication_tdd_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_harq_indication_tdd_rel13_t *value = (nfapi_harq_indication_tdd_rel13_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->mode, end) &&
       pull16(ppReadPackedMsg, &value->number_of_ack_nack, end)))
    return 0;

  if(value->number_of_ack_nack > NFAPI_MAX_NUMBER_ACK_NACK_TDD) {
    // TODO : add error message
    return 0;
  }

  uint16_t idx = 0;

  for(idx = 0; idx < value->number_of_ack_nack; ++idx) {
    uint8_t result = 0;

    switch(value->mode) {
      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_BUNDLING:
        result = unpack_harq_indication_tdd_harq_data(&value->harq_data[idx].bundling, ppReadPackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_MULIPLEXING:
        result = unpack_harq_indication_tdd_harq_data(&value->harq_data[idx].multiplex, ppReadPackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_SPECIAL_BUNDLING:
        result = unpack_harq_indication_tdd_harq_data_special_bundling(&value->harq_data[idx].special_bundling, ppReadPackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_CHANNEL_SELECTION:
        result = unpack_harq_indication_tdd_harq_data(&value->harq_data[idx].channel_selection, ppReadPackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_3:
        result = unpack_harq_indication_tdd_harq_data(&value->harq_data[idx].format_3, ppReadPackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_4:
        result = unpack_harq_indication_tdd_harq_data(&value->harq_data[idx].format_4, ppReadPackedMsg, end);
        break;

      case NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_5:
        result = unpack_harq_indication_tdd_harq_data(&value->harq_data[idx].format_5, ppReadPackedMsg, end);
        break;

      default:
        // TODO add error message
        return 0;
        break;
    }

    if(result == 0)
      return 0;
  }

  return 1;
}

static uint8_t unpack_harq_indication_fdd_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_harq_indication_fdd_rel8_t *value = (nfapi_harq_indication_fdd_rel8_t *)tlv;
  return (pull8(ppReadPackedMsg, &value->harq_tb1, end) &&
          pull8(ppReadPackedMsg, &value->harq_tb2, end));
}

static uint8_t unpack_harq_indication_fdd_rel9_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_harq_indication_fdd_rel9_t *value = (nfapi_harq_indication_fdd_rel9_t *)tlv;
  return (pull8(ppReadPackedMsg, &value->mode, end) &&
          pull8(ppReadPackedMsg, &value->number_of_ack_nack, end) &&
          pullarray8(ppReadPackedMsg, value->harq_tb_n, NFAPI_HARQ_ACK_NACK_REL9_MAX, value->number_of_ack_nack, end));
}

static uint8_t unpack_harq_indication_fdd_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_harq_indication_fdd_rel13_t *value = (nfapi_harq_indication_fdd_rel13_t *)tlv;
  return (pull8(ppReadPackedMsg, &value->mode, end) &&
          pull16(ppReadPackedMsg, &value->number_of_ack_nack, end) &&
          pullarray8(ppReadPackedMsg, value->harq_tb_n, NFAPI_HARQ_ACK_NACK_REL13_MAX, value->number_of_ack_nack, end));
}

static uint8_t unpack_ul_cqi_information_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_ul_cqi_information_t *value = (nfapi_ul_cqi_information_t *)tlv;
  return (pull8(ppReadPackedMsg, &value->ul_cqi, end) &&
          pull8(ppReadPackedMsg, &value->channel, end));
}



static uint8_t unpack_harq_indication_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_harq_indication_body_t *value = (nfapi_harq_indication_body_t *)tlv;
  uint8_t *harqBodyEnd = *ppReadPackedMsg + value->tl.length;

  if(harqBodyEnd > end)
    return 0;

  if(pull16(ppReadPackedMsg, &value->number_of_harqs, end) == 0)
    return 0;

  if(value->number_of_harqs > NFAPI_HARQ_IND_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of harq ind pdus exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_harqs, NFAPI_HARQ_IND_MAX_PDU);
    return 0;
  }

  value->harq_pdu_list = (nfapi_harq_indication_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_harq_indication_pdu_t) * value->number_of_harqs, config);

  if(value->harq_pdu_list == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate harq ind pdu list (count:%d)\n", __FUNCTION__, value->number_of_harqs);
    return 0;
  }

  uint8_t i = 0;

  for(i = 0; i < value->number_of_harqs; ++i) {
    nfapi_harq_indication_pdu_t *pdu = &(value->harq_pdu_list[i]);

    if(pull16(ppReadPackedMsg, &pdu->instance_length, end) == 0)
      return 0;

    uint8_t *harqPduInstanceEnd = *ppReadPackedMsg + pdu->instance_length;
    unpack_tlv_t unpack_fns[] = {
      { NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, unpack_rx_ue_information_value },
      { NFAPI_HARQ_INDICATION_TDD_REL8_TAG, &pdu->harq_indication_tdd_rel8, &unpack_harq_indication_tdd_rel8_value},
      { NFAPI_HARQ_INDICATION_TDD_REL9_TAG, &pdu->harq_indication_tdd_rel9, &unpack_harq_indication_tdd_rel9_value},
      { NFAPI_HARQ_INDICATION_TDD_REL13_TAG, &pdu->harq_indication_tdd_rel13, &unpack_harq_indication_tdd_rel13_value},
      { NFAPI_HARQ_INDICATION_FDD_REL8_TAG, &pdu->harq_indication_fdd_rel8, &unpack_harq_indication_fdd_rel8_value},
      { NFAPI_HARQ_INDICATION_FDD_REL9_TAG, &pdu->harq_indication_fdd_rel9, &unpack_harq_indication_fdd_rel9_value},
      { NFAPI_HARQ_INDICATION_FDD_REL13_TAG, &pdu->harq_indication_fdd_rel13, &unpack_harq_indication_fdd_rel13_value},
      { NFAPI_UL_CQI_INFORMATION_TAG, &pdu->ul_cqi_information, &unpack_ul_cqi_information_value}
    };

    if(unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, harqPduInstanceEnd, 0, 0) == 0)
      return 0;
  }

  return 1;
}

static uint8_t unpack_harq_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_harq_indication_t *pNfapiMsg = (nfapi_harq_indication_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_HARQ_INDICATION_BODY_TAG, &pNfapiMsg->harq_indication_body, &unpack_harq_indication_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_crc_indication_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_crc_indication_rel8_t *crc_pdu_rel8 = (nfapi_crc_indication_rel8_t *)tlv;
  return ( pull8(ppReadPackedMsg, &crc_pdu_rel8->crc_flag, end) );
}

static uint8_t unpack_crc_indication_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end,  nfapi_p7_codec_config_t *config) {
  nfapi_crc_indication_body_t *value = (nfapi_crc_indication_body_t *)tlv;
  uint8_t *crcBodyEnd = *ppReadPackedMsg + value->tl.length;

  if(crcBodyEnd > end)
    return 0;

  if(pull16(ppReadPackedMsg, &value->number_of_crcs, end) == 0)
    return 0;

  if(value->number_of_crcs > NFAPI_CRC_IND_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of crc ind pdu's exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_crcs, NFAPI_CRC_IND_MAX_PDU);
    return 0;
  }

  if(value->number_of_crcs > 0) {
    value->crc_pdu_list = (nfapi_crc_indication_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_crc_indication_pdu_t) * value->number_of_crcs, config);

    if(value->crc_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate crc ind pdu list (count:%d)\n", __FUNCTION__, value->number_of_crcs);
      return 0;
    }
  } else {
    value->crc_pdu_list = 0;
  }

  uint8_t i = 0;

  for(i = 0; i < value->number_of_crcs; ++i) {
    nfapi_crc_indication_pdu_t *pdu = &(value->crc_pdu_list[i]);

    if(pull16(ppReadPackedMsg, &pdu->instance_length, end) == 0)
      return 0;

    uint8_t *crcPduInstanceEnd = *ppReadPackedMsg + pdu->instance_length;
    unpack_tlv_t unpack_fns[] = {
      { NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, unpack_rx_ue_information_value },
      { NFAPI_CRC_INDICATION_REL8_TAG, &pdu->crc_indication_rel8, unpack_crc_indication_rel8_value },
    };

    if(unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, crcPduInstanceEnd, 0, 0) == 0)
      return 0;
  }

  return 1;
}

static uint8_t unpack_crc_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_crc_indication_t *pNfapiMsg = (nfapi_crc_indication_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_CRC_INDICATION_BODY_TAG, &pNfapiMsg->crc_indication_body, &unpack_crc_indication_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_rx_indication_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_rx_indication_rel8_t *value = (nfapi_rx_indication_rel8_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->length, end) &&
          pull16(ppReadPackedMsg, &value->offset, end) &&
          pull8(ppReadPackedMsg, &value->ul_cqi, end) &&
          pull16(ppReadPackedMsg, &value->timing_advance, end));
}
static uint8_t unpack_rx_indication_rel9_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_rx_indication_rel9_t *value = (nfapi_rx_indication_rel9_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->timing_advance_r9, end));
}

static uint8_t unpack_rx_indication_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_rx_indication_body_t *value = (nfapi_rx_indication_body_t *)tlv;
  // the rxBodyEnd points to the end of the cqi PDU's
  uint8_t *rxBodyEnd = *ppReadPackedMsg + value->tl.length;
  uint8_t *rxPduEnd = rxBodyEnd;
  uint8_t *numberOfPdusAddress = *ppReadPackedMsg;

  if(rxBodyEnd > end) {
    // pdu end is past buffer end
    return 0;
  }

  if(pull16(ppReadPackedMsg, &value->number_of_pdus, end) == 0)
    return 0;

  if(value->number_of_pdus > NFAPI_RX_IND_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of rx ind pdu's exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_pdus, NFAPI_RX_IND_MAX_PDU);
    return 0;
  }

  if(value->number_of_pdus > 0) {
    value->rx_pdu_list = (nfapi_rx_indication_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_rx_indication_pdu_t) * value->number_of_pdus, config);

    if(value->rx_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate rx ind pdu list (count:%d)\n", __FUNCTION__, value->number_of_pdus);
      return 0;
    }
  } else {
    value->rx_pdu_list = 0;
  }

  uint8_t i = 0;
  nfapi_rx_indication_pdu_t *pdu = 0;

  while((uint8_t *)(*ppReadPackedMsg) < rxBodyEnd && (uint8_t *)(*ppReadPackedMsg) < rxPduEnd) {
    nfapi_tl_t generic_tl;

    if( unpack_tl(ppReadPackedMsg, &generic_tl, end) == 0)
      return 0;

    switch(generic_tl.tag) {
      case NFAPI_RX_UE_INFORMATION_TAG: {
        pdu = &(value->rx_pdu_list[i++]);
        pdu->rx_ue_information.tl = generic_tl;

        if(unpack_rx_ue_information_value(&pdu->rx_ue_information, ppReadPackedMsg, end) == 0)
          return 0;
      }
      break;

      case NFAPI_RX_INDICATION_REL8_TAG: {
        if(pdu != 0) {
          pdu->rx_indication_rel8.tl = generic_tl;

          if(unpack_rx_indication_rel8_value(&pdu->rx_indication_rel8, ppReadPackedMsg, end) == 0)
            return 0;

          if(pdu->rx_indication_rel8.offset > 0) {
            // Need to check that the data is within the tlv
            if(numberOfPdusAddress + pdu->rx_indication_rel8.offset + pdu->rx_indication_rel8.length <= rxBodyEnd) {
              // If this the first pdu set the rxPduEnd
              if(numberOfPdusAddress + pdu->rx_indication_rel8.offset < rxPduEnd) {
                rxPduEnd = numberOfPdusAddress + pdu->rx_indication_rel8.offset;

                if(rxPduEnd > end) {
                  // pdu end is past buffer end
                  return 0;
                }
              }
            } else {
              NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME: the rx data is outside of the tlv\n");
            }
          }
        }
      }
      break;

      case NFAPI_RX_INDICATION_REL9_TAG: {
        if(pdu != 0) {
          pdu->rx_indication_rel9.tl = generic_tl;

          if(unpack_rx_indication_rel9_value(&pdu->rx_indication_rel9, ppReadPackedMsg, end) == 0)
            return 0;
        }
      }
      break;

      default: {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "RX_ULSCH.indication Invalid pdu type %d \n", generic_tl.tag );
      }
      break;
    }
  }

  uint8_t idx = 0;

  for(idx = 0; idx < value->number_of_pdus; ++idx) {
    if(value->rx_pdu_list[idx].rx_indication_rel8.tl.tag == NFAPI_RX_INDICATION_REL8_TAG) {
      uint32_t length = value->rx_pdu_list[idx].rx_indication_rel8.length;
      value->rx_pdu_list[idx].data = nfapi_p7_allocate(length, config);

      if(pullarray8(ppReadPackedMsg, value->rx_pdu_list[idx].data, length, length, end) == 0) {
        return 0;
      }
    }
  }

  return 1;
}

static uint8_t unpack_rx_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_rx_indication_t *pNfapiMsg = (nfapi_rx_indication_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_RX_INDICATION_BODY_TAG, &pNfapiMsg->rx_indication_body, &unpack_rx_indication_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_preamble_pdu_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_preamble_pdu_rel8_t *preamble_pdu_rel8 = (nfapi_preamble_pdu_rel8_t *)tlv;
  return (pull16(ppReadPackedMsg, &preamble_pdu_rel8->rnti, end) &&
          pull8(ppReadPackedMsg, &preamble_pdu_rel8->preamble, end) &&
          pull16(ppReadPackedMsg, &preamble_pdu_rel8->timing_advance, end));
}

static uint8_t unpack_preamble_pdu_rel9_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_preamble_pdu_rel9_t *preamble_pdu_rel9 = (nfapi_preamble_pdu_rel9_t *)tlv;
  return pull16(ppReadPackedMsg, &preamble_pdu_rel9->timing_advance_r9, end);
}

static uint8_t unpack_preamble_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_preamble_pdu_rel13_t *preamble_pdu_rel13 = (nfapi_preamble_pdu_rel13_t *)tlv;
  return pull8(ppReadPackedMsg, &preamble_pdu_rel13->rach_resource_type, end);
}

static uint8_t unpack_rach_indication_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_rach_indication_body_t *value = (nfapi_rach_indication_body_t *)tlv;
  uint8_t *rachBodyEnd = *ppReadPackedMsg + value->tl.length;

  if(rachBodyEnd > end)
    return 0;

  if(pull16(ppReadPackedMsg, &value->number_of_preambles, end) == 0)
    return 0;

  if(value->number_of_preambles > NFAPI_PREAMBLE_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of preamble du's exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_preambles, NFAPI_PREAMBLE_MAX_PDU);
    return 0;
  }

  if(value->number_of_preambles > 0) {
    value->preamble_list = (nfapi_preamble_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_preamble_pdu_t) * value->number_of_preambles, config);

    if(value->preamble_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate preamble pdu list (count:%d)\n", __FUNCTION__, value->number_of_preambles);
      return 0;
    }
  } else {
    value->preamble_list = 0;
  }

  uint8_t i = 0;

  for(i = 0; i < value->number_of_preambles; ++i) {
    nfapi_preamble_pdu_t *pdu = &(value->preamble_list[i]);

    if(pull16(ppReadPackedMsg, &pdu->instance_length, end) == 0)
      return 0;

    uint8_t *preamblePduInstanceEnd = *ppReadPackedMsg + pdu->instance_length;
    unpack_tlv_t unpack_fns[] = {
      { NFAPI_PREAMBLE_REL8_TAG, &pdu->preamble_rel8, unpack_preamble_pdu_rel8_value },
      { NFAPI_PREAMBLE_REL9_TAG, &pdu->preamble_rel9, unpack_preamble_pdu_rel9_value },
      { NFAPI_PREAMBLE_REL13_TAG, &pdu->preamble_rel13, unpack_preamble_pdu_rel13_value },
    };

    if(unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, preamblePduInstanceEnd, 0, 0) == 0)
      return 0;
  }

  return 1;
}

static uint8_t unpack_rach_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_rach_indication_t *pNfapiMsg = (nfapi_rach_indication_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_RACH_INDICATION_BODY_TAG, &pNfapiMsg->rach_indication_body, &unpack_rach_indication_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_srs_indication_fdd_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_srs_indication_fdd_rel8_t *srs_pdu_fdd_rel8 = (nfapi_srs_indication_fdd_rel8_t *)tlv;

  if(!(pull16(ppReadPackedMsg, &srs_pdu_fdd_rel8->doppler_estimation, end) &&
       pull16(ppReadPackedMsg, &srs_pdu_fdd_rel8->timing_advance, end) &&
       pull8(ppReadPackedMsg, &srs_pdu_fdd_rel8->number_of_resource_blocks, end) &&
       pull8(ppReadPackedMsg, &srs_pdu_fdd_rel8->rb_start, end) &&
       pullarray8(ppReadPackedMsg, srs_pdu_fdd_rel8->snr, NFAPI_NUM_RB_MAX, srs_pdu_fdd_rel8->number_of_resource_blocks, end)))
    return 0;

  return 1;
}

static uint8_t unpack_srs_indication_fdd_rel9_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_srs_indication_fdd_rel9_t *srs_pdu_fdd_rel9 = (nfapi_srs_indication_fdd_rel9_t *)tlv;
  return (pull16(ppReadPackedMsg, &srs_pdu_fdd_rel9->timing_advance_r9, end));
}

static uint8_t unpack_srs_indication_tdd_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_srs_indication_ttd_rel10_t *srs_pdu_tdd_rel10 = (nfapi_srs_indication_ttd_rel10_t *)tlv;
  return (pull8(ppReadPackedMsg, &srs_pdu_tdd_rel10->uppts_symbol, end));
}

static uint8_t unpack_srs_indication_fdd_rel11_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_srs_indication_fdd_rel11_t *srs_pdu_fdd_rel11 = (nfapi_srs_indication_fdd_rel11_t *)tlv;
  return ( pull16(ppReadPackedMsg, &srs_pdu_fdd_rel11->ul_rtoa, end));
}

static uint8_t unpack_tdd_channel_measurement_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_tdd_channel_measurement_t *value = (nfapi_tdd_channel_measurement_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->num_prb_per_subband, end) &&
       pull8(ppReadPackedMsg, &value->number_of_subbands, end) &&
       pull8(ppReadPackedMsg, &value->num_atennas, end)))
    return 0;

  if(value->number_of_subbands > NFAPI_MAX_NUM_SUBBANDS) {
    // todo : add error
    return 0;
  }

  if(value->num_atennas > NFAPI_MAX_NUM_PHYSICAL_ANTENNAS) {
    // todo : add error
    return 0;
  }

  uint8_t idx = 0;

  for(idx = 0; idx < value->number_of_subbands; ++idx) {
    if(!(pull8(ppReadPackedMsg, &value->subands[idx].subband_index, end) &&
         pullarray16(ppReadPackedMsg, value->subands[idx].channel, NFAPI_MAX_NUM_PHYSICAL_ANTENNAS, value->num_atennas, end)))
      return 0;
  }

  return 1;
}


static uint8_t unpack_srs_indication_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_srs_indication_body_t *value = (nfapi_srs_indication_body_t *)tlv;
  uint8_t *srsBodyEnd = *ppReadPackedMsg + value->tl.length;

  if(srsBodyEnd > end)
    return 0;

  if(pull8(ppReadPackedMsg, &value->number_of_ues, end) == 0)
    return 0;

  if(value->number_of_ues > NFAPI_SRS_IND_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of srs ind pdu's exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_ues, NFAPI_SRS_IND_MAX_PDU);
    return 0;
  }

  if(value->number_of_ues > 0) {
    value->srs_pdu_list = (nfapi_srs_indication_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_srs_indication_pdu_t) * value->number_of_ues, config);

    if(value->srs_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate srs ind pdu list (count:%d)\n", __FUNCTION__, value->number_of_ues);
      return 0;
    }
  } else {
    value->srs_pdu_list = 0;
  }

  uint8_t i = 0;

  for(i = 0; i < value->number_of_ues; ++i) {
    nfapi_srs_indication_pdu_t *pdu = &(value->srs_pdu_list[i]);

    if(pull16(ppReadPackedMsg, &pdu->instance_length, end) == 0)
      return 0;

    uint8_t *srsPduInstanceEnd = *ppReadPackedMsg + pdu->instance_length;
    unpack_tlv_t unpack_fns[] = {
      { NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, unpack_rx_ue_information_value },
      { NFAPI_SRS_INDICATION_FDD_REL8_TAG, &pdu->srs_indication_fdd_rel8, unpack_srs_indication_fdd_rel8_value},
      { NFAPI_SRS_INDICATION_FDD_REL9_TAG, &pdu->srs_indication_fdd_rel9, unpack_srs_indication_fdd_rel9_value},
      { NFAPI_SRS_INDICATION_TDD_REL10_TAG, &pdu->srs_indication_tdd_rel10, unpack_srs_indication_tdd_rel10_value},
      { NFAPI_SRS_INDICATION_FDD_REL11_TAG, &pdu->srs_indication_fdd_rel11, unpack_srs_indication_fdd_rel11_value},
      { NFAPI_TDD_CHANNEL_MEASUREMENT_TAG, &pdu->tdd_channel_measurement, unpack_tdd_channel_measurement_value},
    };

    if(unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, srsPduInstanceEnd, 0, 0) == 0)
      return 0;
  }

  return 1;
}

static uint8_t unpack_srs_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_srs_indication_t *pNfapiMsg = (nfapi_srs_indication_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_SRS_INDICATION_BODY_TAG, &pNfapiMsg->srs_indication_body, &unpack_srs_indication_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}


static uint8_t unpack_sr_indication_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_sr_indication_body_t *value = (nfapi_sr_indication_body_t *)tlv;
  uint8_t *srBodyEnd = *ppReadPackedMsg + value->tl.length;

  if(srBodyEnd > end)
    return 0;

  if(pull16(ppReadPackedMsg, &value->number_of_srs, end) == 0)
    return 0;

  if(value->number_of_srs > NFAPI_SR_IND_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of sr ind pdu's exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_srs, NFAPI_SR_IND_MAX_PDU);
    return 0;
  }

  if(value->number_of_srs > 0) {
    value->sr_pdu_list = (nfapi_sr_indication_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_sr_indication_pdu_t) * value->number_of_srs, config);

    if(value->sr_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate sr ind pdu list (count:%d)\n", __FUNCTION__, value->number_of_srs);
      return 0;
    }
  } else {
    value->sr_pdu_list = 0;
  }

  uint8_t i = 0;

  for(i = 0; i < value->number_of_srs; ++i) {
    nfapi_sr_indication_pdu_t *pdu = &(value->sr_pdu_list[i]);

    if(pull16(ppReadPackedMsg, &pdu->instance_length, end) == 0)
      return 0;

    uint8_t *srPduInstanceEnd = *ppReadPackedMsg + pdu->instance_length;
    unpack_tlv_t unpack_fns[] = {
      { NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, unpack_rx_ue_information_value },
      { NFAPI_UL_CQI_INFORMATION_TAG, &pdu->ul_cqi_information, unpack_ul_cqi_information_value },
    };

    if(unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, srPduInstanceEnd, 0, 0) == 0)
      return 0;
  }

  return 1;
}

static int unpack_sr_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_sr_indication_t *pNfapiMsg = (nfapi_sr_indication_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_SR_INDICATION_BODY_TAG, &pNfapiMsg->sr_indication_body, &unpack_sr_indication_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}
static uint8_t unpack_cqi_indication_rel8_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_cqi_indication_rel8_t *cqi_pdu_rel8 = (nfapi_cqi_indication_rel8_t *)tlv;
  return (pull16(ppReadPackedMsg, &cqi_pdu_rel8->length, end) &&
          pull16(ppReadPackedMsg, &cqi_pdu_rel8->data_offset, end) &&
          pull8(ppReadPackedMsg, &cqi_pdu_rel8->ul_cqi, end) &&
          pull8(ppReadPackedMsg, &cqi_pdu_rel8->ri, end) &&
          pull16(ppReadPackedMsg, &cqi_pdu_rel8->timing_advance, end));
}

static uint8_t unpack_cqi_indication_rel9_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_cqi_indication_rel9_t *cqi_pdu_rel9 = (nfapi_cqi_indication_rel9_t *)tlv;

  if(!(pull16(ppReadPackedMsg, &cqi_pdu_rel9->length, end) &&
       pull16(ppReadPackedMsg, &cqi_pdu_rel9->data_offset, end) &&
       pull8(ppReadPackedMsg, &cqi_pdu_rel9->ul_cqi, end) &&
       pull8(ppReadPackedMsg, &cqi_pdu_rel9->number_of_cc_reported, end)))
    return 0;

  if(cqi_pdu_rel9->number_of_cc_reported > NFAPI_CC_MAX) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "FIXME : out of bound array\n");
    return 0;
  }

  if(!(pullarray8(ppReadPackedMsg, cqi_pdu_rel9->ri, NFAPI_CC_MAX, cqi_pdu_rel9->number_of_cc_reported, end) &&
       pull16(ppReadPackedMsg, &cqi_pdu_rel9->timing_advance, end) &&
       pull16(ppReadPackedMsg, &cqi_pdu_rel9->timing_advance_r9, end)))
    return 0;

  return 1;
}

static uint8_t  unpack_cqi_indication_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end,  nfapi_p7_codec_config_t *config) {
  nfapi_cqi_indication_body_t *value = (nfapi_cqi_indication_body_t *)tlv;
  // the cqiBodyEnd points to the end of the cqi PDU's
  uint8_t *cqiBodyEnd = *ppReadPackedMsg + value->tl.length;

  //uint8_t* cqiPduEnd = cqiBodyEnd;
  //uint8_t* numberOfPdusAddress = *ppReadPackedMsg;

  if(cqiBodyEnd > end)
    return 0;

  if(pull16(ppReadPackedMsg, &value->number_of_cqis, end) == 0)
    return 0;

  if(value->number_of_cqis > NFAPI_CQI_IND_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of cqi ind pdu's exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_cqis, NFAPI_CQI_IND_MAX_PDU);
    return -1;
  }

  if(value->number_of_cqis > 0) {
    value->cqi_pdu_list = (nfapi_cqi_indication_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_cqi_indication_pdu_t) * value->number_of_cqis, config);

    if(value->cqi_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate cqi ind pdu list (count:%d)\n", __FUNCTION__, value->number_of_cqis);
      return 0;
    }
  } else {
    value->cqi_pdu_list = 0;
  }

  if(value->number_of_cqis > 0) {
    value->cqi_raw_pdu_list = (nfapi_cqi_indication_raw_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_cqi_indication_raw_pdu_t) * value->number_of_cqis, config);

    if(value->cqi_raw_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate raw cqi ind pdu list (count:%d)\n", __FUNCTION__, value->number_of_cqis);
      return 0;
    }
  } else {
    value->cqi_raw_pdu_list = 0;
  }

  uint8_t i = 0;

  for(i = 0; i < value->number_of_cqis; ++i) {
    nfapi_cqi_indication_pdu_t *pdu = &(value->cqi_pdu_list[i]);
    memset(pdu, 0, sizeof(nfapi_cqi_indication_pdu_t));

    if(pull16(ppReadPackedMsg, &pdu->instance_length, end) == 0)
      return 0;

    uint8_t *cqiPduInstanceEnd = *ppReadPackedMsg + pdu->instance_length;

    while((uint8_t *)(*ppReadPackedMsg) < cqiPduInstanceEnd) {
      nfapi_tl_t generic_tl;

      if(unpack_tl(ppReadPackedMsg, &generic_tl, end) == 0)
        return 0;

      switch(generic_tl.tag) {
        case NFAPI_RX_UE_INFORMATION_TAG:
          pdu->rx_ue_information.tl = generic_tl;

          if(unpack_rx_ue_information_value(&pdu->rx_ue_information, ppReadPackedMsg, end) == 0)
            return 0;

          break;

        case NFAPI_CQI_INDICATION_REL8_TAG:
          pdu->cqi_indication_rel8.tl = generic_tl;

          if(unpack_cqi_indication_rel8_value(&pdu->cqi_indication_rel8, ppReadPackedMsg, end) == 0)
            return 0;

          break;

        case NFAPI_CQI_INDICATION_REL9_TAG:
          pdu->cqi_indication_rel9.tl = generic_tl;

          if(unpack_cqi_indication_rel9_value(&pdu->cqi_indication_rel9, ppReadPackedMsg, end) == 0)
            return 0;

          break;

        case NFAPI_UL_CQI_INFORMATION_TAG:
          pdu->ul_cqi_information.tl = generic_tl;

          if(unpack_ul_cqi_information_value(&pdu->ul_cqi_information, ppReadPackedMsg, end) == 0)
            return 0;

          break;

        default: {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "RX_CQI.indication Invalid pdu type %d \n", generic_tl.tag );
        }
        break;
      };
    }
  }

  uint8_t idx = 0;

  for(idx = 0; idx < value->number_of_cqis; ++idx) {
    if(value->cqi_pdu_list[idx].cqi_indication_rel8.tl.tag == NFAPI_CQI_INDICATION_REL8_TAG) {
      if(pullarray8(ppReadPackedMsg, &(value->cqi_raw_pdu_list[idx].pdu[0]), NFAPI_CQI_RAW_MAX_LEN, value->cqi_pdu_list[idx].cqi_indication_rel8.length, end) == 0)
        return 0;
    } else if(value->cqi_pdu_list[idx].cqi_indication_rel9.tl.tag == NFAPI_CQI_INDICATION_REL9_TAG) {
      if(pullarray8(ppReadPackedMsg, &(value->cqi_raw_pdu_list[idx].pdu[0]), NFAPI_CQI_RAW_MAX_LEN, value->cqi_pdu_list[idx].cqi_indication_rel9.length, end) == 0)
        return 0;
    }
  }

  return 1;
}

static uint8_t unpack_cqi_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_cqi_indication_t *pNfapiMsg = (nfapi_cqi_indication_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_CQI_INDICATION_BODY_TAG, &pNfapiMsg->cqi_indication_body, &unpack_cqi_indication_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}
static uint8_t unpack_lbt_pdsch_req_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lbt_pdsch_req_pdu_rel13_t *value = (nfapi_lbt_pdsch_req_pdu_rel13_t *)tlv;
  return (pull32(ppReadPackedMsg, &value->handle, end) &&
          pull32(ppReadPackedMsg, &value->mp_cca, end) &&
          pull32(ppReadPackedMsg, &value->n_cca, end) &&
          pull32(ppReadPackedMsg, &value->offset, end) &&
          pull32(ppReadPackedMsg, &value->lte_txop_sf, end) &&
          pull16(ppReadPackedMsg, &value->txop_sfn_sf_end, end) &&
          pull32(ppReadPackedMsg, &value->lbt_mode, end));
}

static uint8_t unpack_lbt_drs_req_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lbt_drs_req_pdu_rel13_t *value = (nfapi_lbt_drs_req_pdu_rel13_t *)tlv;
  return (pull32(ppReadPackedMsg, &value->handle, end) &&
          pull32(ppReadPackedMsg, &value->offset, end) &&
          pull16(ppReadPackedMsg, &value->sfn_sf_end, end) &&
          pull32(ppReadPackedMsg, &value->lbt_mode, end));
}


static uint8_t unpack_lbt_config_request_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_lbt_dl_config_request_body_t *value = (nfapi_lbt_dl_config_request_body_t *)tlv;

  if(pull16(ppReadPackedMsg, &value->number_of_pdus, end) == 0)
    return 0;

  if(value->number_of_pdus > NFAPI_LBT_DL_CONFIG_REQ_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of lbt dl config pdu's exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_pdus, NFAPI_LBT_DL_CONFIG_REQ_MAX_PDU);
    return 0;
  }

  if(value->number_of_pdus) {
    value->lbt_dl_config_req_pdu_list = (nfapi_lbt_dl_config_request_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_lbt_dl_config_request_pdu_t) * value->number_of_pdus, config);

    if(value->lbt_dl_config_req_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate lbt dl config pdu list (count:%d)\n", __FUNCTION__, value->number_of_pdus);
      return 0;
    }
  } else {
    value->lbt_dl_config_req_pdu_list = 0;
  }

  uint16_t i;
  uint16_t total_number_of_pdus = value->number_of_pdus;

  for(i = 0; i < total_number_of_pdus; ++i) {
    nfapi_lbt_dl_config_request_pdu_t *pdu = &(value->lbt_dl_config_req_pdu_list[i]);

    if(!(pull8(ppReadPackedMsg, &pdu->pdu_type, end) &&
         pull8(ppReadPackedMsg, &pdu->pdu_size, end)))
      return 0;

    uint8_t *packedPduEnd = (*ppReadPackedMsg) + pdu->pdu_size - 2;

    if(packedPduEnd > end)
      return 0;

    switch(pdu->pdu_type) {
      case NFAPI_LBT_DL_CONFIG_REQUEST_PDSCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_LBT_PDSCH_REQ_PDU_REL13_TAG, &pdu->lbt_pdsch_req_pdu.lbt_pdsch_req_pdu_rel13, &unpack_lbt_pdsch_req_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_LBT_DL_CONFIG_REQUEST_DRS_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_LBT_DRS_REQ_PDU_REL13_TAG, &pdu->lbt_drs_req_pdu.lbt_drs_req_pdu_rel13, &unpack_lbt_drs_req_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      default:
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "LBT_DL_CONFIG.request body invalid pdu type %d\n", pdu->pdu_type);
        return 0;
    }
  }

  return 1;
}
static uint8_t unpack_lbt_dl_config_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_lbt_dl_config_request_t *pNfapiMsg = (nfapi_lbt_dl_config_request_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_LBT_DL_CONFIG_REQUEST_BODY_TAG, &pNfapiMsg->lbt_dl_config_request_body, &unpack_lbt_config_request_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_lbt_pdsch_rsp_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lbt_pdsch_rsp_pdu_rel13_t *value = (nfapi_lbt_pdsch_rsp_pdu_rel13_t *)tlv;
  return (pull32(ppReadPackedMsg, &value->handle, end) &&
          pull32(ppReadPackedMsg, &value->result, end) &&
          pull32(ppReadPackedMsg, &value->lte_txop_symbols, end) &&
          pull32(ppReadPackedMsg, &value->initial_partial_sf, end));
}
static uint8_t unpack_lbt_drs_rsp_pdu_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lbt_drs_rsp_pdu_rel13_t *value = (nfapi_lbt_drs_rsp_pdu_rel13_t *)tlv;
  return (pull32(ppReadPackedMsg, &value->handle, end) &&
          pull32(ppReadPackedMsg, &value->result, end));
}

static uint8_t unpack_lbt_indication_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_lbt_dl_indication_body_t *value = (nfapi_lbt_dl_indication_body_t *)tlv;

  if(pull16(ppReadPackedMsg, &value->number_of_pdus, end) == 0)
    return 0;

  if(value->number_of_pdus > NFAPI_LBT_IND_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of lbt dl ind pdu's exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_pdus, NFAPI_LBT_IND_MAX_PDU);
    return 0;
  }

  if(value->number_of_pdus > 0) {
    value->lbt_indication_pdu_list = (nfapi_lbt_dl_indication_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_lbt_dl_indication_pdu_t) * value->number_of_pdus, config);

    if(value->lbt_indication_pdu_list == NULL) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate lbt dl ind config pdu list (count:%d)\n", __FUNCTION__, value->number_of_pdus);
      return 0;
    }
  } else {
    value->lbt_indication_pdu_list = 0;
  }

  uint16_t i;
  uint16_t total_number_of_pdus = value->number_of_pdus;

  for(i = 0; i < total_number_of_pdus; ++i) {
    nfapi_lbt_dl_indication_pdu_t *pdu = &(value->lbt_indication_pdu_list[i]);

    if(!(pull8(ppReadPackedMsg, &pdu->pdu_type, end) &&
         pull8(ppReadPackedMsg, &pdu->pdu_size, end)))
      return 0;

    uint8_t *packedPduEnd = (*ppReadPackedMsg) + pdu->pdu_size - 2;

    if(packedPduEnd > end)
      return 0;

    switch(pdu->pdu_type) {
      case NFAPI_LBT_DL_RSP_PDSCH_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_LBT_PDSCH_RSP_PDU_REL13_TAG, &pdu->lbt_pdsch_rsp_pdu.lbt_pdsch_rsp_pdu_rel13, &unpack_lbt_pdsch_rsp_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      case NFAPI_LBT_DL_RSP_DRS_PDU_TYPE: {
        unpack_tlv_t unpack_fns[] = {
          { NFAPI_LBT_DRS_RSP_PDU_REL13_TAG, &pdu->lbt_drs_rsp_pdu.lbt_drs_rsp_pdu_rel13, &unpack_lbt_drs_rsp_pdu_rel13_value},
        };
        unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, packedPduEnd, 0, 0);
      }
      break;

      default:
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "LBT_DL.indication body invalid pdu type %d\n", pdu->pdu_type);
        return 0;
    }
  }

  return 1;
}
static uint8_t unpack_lbt_dl_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_lbt_dl_indication_t *pNfapiMsg = (nfapi_lbt_dl_indication_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_LBT_DL_INDICATION_BODY_TAG, &pNfapiMsg->lbt_dl_indication_body, &unpack_lbt_indication_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_nb_harq_indication_fdd_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nb_harq_indication_fdd_rel13_t *value = (nfapi_nb_harq_indication_fdd_rel13_t *)tlv;
  return (pull8(ppReadPackedMsg, &value->harq_tb1, end));
}


static uint8_t unpack_nb_harq_indication_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nb_harq_indication_body_t *value = (nfapi_nb_harq_indication_body_t *)tlv;
  uint8_t *nbharqBodyEnd = *ppReadPackedMsg + value->tl.length;

  if(nbharqBodyEnd > end)
    return 0;

  if(pull16(ppReadPackedMsg, &value->number_of_harqs, end) == 0)
    return 0;

  if(value->number_of_harqs > NFAPI_HARQ_IND_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of harq ind pdus exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_harqs, NFAPI_HARQ_IND_MAX_PDU);
    return 0;
  }

  value->nb_harq_pdu_list = (nfapi_nb_harq_indication_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_nb_harq_indication_pdu_t) * value->number_of_harqs, config);

  if(value->nb_harq_pdu_list == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate harq ind pdu list (count:%d)\n", __FUNCTION__, value->number_of_harqs);
    return 0;
  }

  uint8_t i = 0;

  for(i = 0; i < value->number_of_harqs; ++i) {
    nfapi_nb_harq_indication_pdu_t *pdu = &(value->nb_harq_pdu_list[i]);

    if(pull16(ppReadPackedMsg, &pdu->instance_length, end) == 0)
      return 0;

    uint8_t *harqPduInstanceEnd = *ppReadPackedMsg + pdu->instance_length;
    unpack_tlv_t unpack_fns[] = {
      { NFAPI_RX_UE_INFORMATION_TAG, &pdu->rx_ue_information, unpack_rx_ue_information_value },
      { NFAPI_NB_HARQ_INDICATION_FDD_REL13_TAG, &pdu->nb_harq_indication_fdd_rel13, &unpack_nb_harq_indication_fdd_rel13_value},
      { NFAPI_UL_CQI_INFORMATION_TAG, &pdu->ul_cqi_information, &unpack_ul_cqi_information_value}
    };

    if(unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, harqPduInstanceEnd, 0, 0) == 0)
      return 0;
  }

  return 1;
}

static uint8_t unpack_nb_harq_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_nb_harq_indication_t *pNfapiMsg = (nfapi_nb_harq_indication_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_NB_HARQ_INDICATION_BODY_TAG, &pNfapiMsg->nb_harq_indication_body, &unpack_nb_harq_indication_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_nrach_indication_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nrach_indication_pdu_rel13_t *value = (nfapi_nrach_indication_pdu_rel13_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->rnti, end) &&
          pull8(ppReadPackedMsg, &value->initial_sc, end) &&
          pull16(ppReadPackedMsg, &value->timing_advance, end) &&
          pull8(ppReadPackedMsg, &value->nrach_ce_level, end));
}

static uint8_t unpack_ue_release_resp(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_ue_release_response_t *pNfapiMsg = (nfapi_ue_release_response_t *)msg;

  if(pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) == 0) {
    return 0;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "ue_release_response:error_code = %d\n", pNfapiMsg->error_code);
  }

  return 1;
}

static uint8_t unpack_nrach_indication_body_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  nfapi_nrach_indication_body_t *value = (nfapi_nrach_indication_body_t *)tlv;
  uint8_t *nrachBodyEnd = *ppReadPackedMsg + value->tl.length;

  if(nrachBodyEnd > end)
    return 0;

  if(pull8(ppReadPackedMsg, &value->number_of_initial_scs_detected, end) == 0)
    return 0;

  if(value->number_of_initial_scs_detected > NFAPI_PREAMBLE_MAX_PDU) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s number of detected scs ind pdus exceed maxium (count:%d max:%d)\n", __FUNCTION__, value->number_of_initial_scs_detected, NFAPI_PREAMBLE_MAX_PDU);
    return 0;
  }

  value->nrach_pdu_list = (nfapi_nrach_indication_pdu_t *)nfapi_p7_allocate(sizeof(nfapi_nrach_indication_pdu_t) * value->number_of_initial_scs_detected, config);

  if(value->nrach_pdu_list == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s failed to allocate nrach ind pdu list (count:%d)\n", __FUNCTION__, value->number_of_initial_scs_detected);
    return 0;
  }

  uint8_t i = 0;

  for(i = 0; i < value->number_of_initial_scs_detected; ++i) {
    nfapi_nrach_indication_pdu_t *pdu = &(value->nrach_pdu_list[i]);
    uint8_t *nrachPduInstanceEnd = *ppReadPackedMsg + 4 + 6;
    unpack_tlv_t unpack_fns[] = {
      { NFAPI_NRACH_INDICATION_REL13_TAG, &pdu->nrach_indication_rel13, &unpack_nrach_indication_rel13_value},
    };

    if(unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, nrachPduInstanceEnd, 0, 0) == 0)
      return 0;
  }

  return 1;
}

static uint8_t unpack_nrach_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_nrach_indication_t *pNfapiMsg = (nfapi_nrach_indication_t *)msg;
  unpack_p7_tlv_t unpack_fns[] = {
    { NFAPI_NRACH_INDICATION_BODY_TAG, &pNfapiMsg->nrach_indication_body, &unpack_nrach_indication_body_value},
  };
  return (pull16(ppReadPackedMsg, &pNfapiMsg->sfn_sf, end) &&
          unpack_p7_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_nr_dl_node_sync(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_nr_dl_node_sync_t *pNfapiMsg = (nfapi_nr_dl_node_sync_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->t1, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->delta_sfn_slot, end) &&
          unpack_p7_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_dl_node_sync(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_dl_node_sync_t *pNfapiMsg = (nfapi_dl_node_sync_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->t1, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->delta_sfn_sf, end) &&
          unpack_p7_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}


static uint8_t unpack_nr_ul_node_sync(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_nr_ul_node_sync_t *pNfapiMsg = (nfapi_nr_ul_node_sync_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->t1, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->t2, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->t3, end) &&
          unpack_p7_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}


static uint8_t unpack_ul_node_sync(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_ul_node_sync_t *pNfapiMsg = (nfapi_ul_node_sync_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->t1, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->t2, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->t3, end) &&
          unpack_p7_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_timing_info(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_timing_info_t *pNfapiMsg = (nfapi_timing_info_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->last_sfn_sf, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->time_since_last_timing_info, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->dl_config_jitter, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->tx_request_jitter, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->ul_config_jitter, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->hi_dci0_jitter, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->dl_config_latest_delay, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->tx_request_latest_delay, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->ul_config_latest_delay, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->hi_dci0_latest_delay, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->dl_config_earliest_arrival, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->tx_request_earliest_arrival, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->ul_config_earliest_arrival, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->hi_dci0_earliest_arrival, end) &&
          unpack_p7_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}


static uint8_t unpack_nr_timing_info(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config) {
  nfapi_nr_timing_info_t *pNfapiMsg = (nfapi_nr_timing_info_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->last_sfn, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->last_slot, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->time_since_last_timing_info, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->dl_tti_jitter, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->tx_data_request_jitter, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->ul_tti_jitter, end) &&
          pull32(ppReadPackedMsg, &pNfapiMsg->ul_dci_jitter, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->dl_tti_latest_delay, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->tx_data_request_latest_delay, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->ul_tti_latest_delay, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->ul_dci_latest_delay, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->dl_tti_earliest_arrival, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->tx_data_request_earliest_arrival, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->ul_tti_earliest_arrival, end) &&
          pulls32(ppReadPackedMsg, &pNfapiMsg->ul_dci_earliest_arrival, end) &&
          unpack_p7_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}



// unpack length check

static int check_unpack_length(nfapi_message_id_e msgId, uint32_t unpackedBufLen) {
  int retLen = 0;

  switch (msgId) {
    case NFAPI_DL_CONFIG_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_dl_config_request_t))
        retLen = sizeof(nfapi_dl_config_request_t);

      break;

    case NFAPI_UL_CONFIG_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_ul_config_request_t))
        retLen = sizeof(nfapi_ul_config_request_t);

      break;

    case NFAPI_SUBFRAME_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_subframe_indication_t))
        retLen = sizeof(nfapi_subframe_indication_t);

      break;

    case NFAPI_HI_DCI0_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_hi_dci0_request_t))
        retLen = sizeof(nfapi_hi_dci0_request_t);

      break;

    case NFAPI_TX_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_tx_request_t))
        retLen = sizeof(nfapi_tx_request_t);

      break;

    case NFAPI_HARQ_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_harq_indication_t))
        retLen = sizeof(nfapi_harq_indication_t);

      break;

    case NFAPI_CRC_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_crc_indication_t))
        retLen = sizeof(nfapi_crc_indication_t);

      break;

    case NFAPI_RX_ULSCH_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_rx_indication_t))
        retLen = sizeof(nfapi_rx_indication_t);

      break;

    case NFAPI_RACH_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_rach_indication_t))
        retLen = sizeof(nfapi_rach_indication_t);

      break;

    case NFAPI_SRS_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_srs_indication_t))
        retLen = sizeof(nfapi_srs_indication_t);

      break;

    case NFAPI_RX_SR_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_sr_indication_t))
        retLen = sizeof(nfapi_sr_indication_t);

      break;

    case NFAPI_RX_CQI_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_cqi_indication_t))
        retLen = sizeof(nfapi_cqi_indication_t);

      break;

    case NFAPI_LBT_DL_CONFIG_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_lbt_dl_config_request_t))
        retLen = sizeof(nfapi_lbt_dl_config_request_t);

      break;

    case NFAPI_LBT_DL_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_lbt_dl_indication_t))
        retLen = sizeof(nfapi_lbt_dl_indication_t);

      break;

    case NFAPI_NB_HARQ_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_nb_harq_indication_t))
        retLen = sizeof(nfapi_nb_harq_indication_t);

      break;

    case NFAPI_NRACH_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_nrach_indication_t))
        retLen = sizeof(nfapi_nrach_indication_t);

      break;

    case NFAPI_DL_NODE_SYNC:
      if (unpackedBufLen >= sizeof(nfapi_dl_node_sync_t))
        retLen = sizeof(nfapi_dl_node_sync_t);

      break;

    case NFAPI_UL_NODE_SYNC:
      if (unpackedBufLen >= sizeof(nfapi_ul_node_sync_t))
        retLen = sizeof(nfapi_ul_node_sync_t);

      break;

    case NFAPI_TIMING_INFO:
      if (unpackedBufLen >= sizeof(nfapi_timing_info_t))
        retLen = sizeof(nfapi_timing_info_t);

      break;

    case NFAPI_UE_RELEASE_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_ue_release_request_t))
        retLen = sizeof(nfapi_ue_release_request_t);

      break;

    case NFAPI_UE_RELEASE_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_ue_release_response_t))
        retLen = sizeof(nfapi_ue_release_response_t);

      break;

    default:
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unknown message ID %d\n", msgId);
      break;
  }

  return retLen;
}

static int check_nr_unpack_length(nfapi_message_id_e msgId, uint32_t unpackedBufLen) {
  int retLen = 0;

  switch (msgId) {
    case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_nr_dl_tti_request_t))
        retLen = sizeof(nfapi_nr_dl_tti_request_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_nr_ul_tti_request_t))
        retLen = sizeof(nfapi_nr_ul_tti_request_t);

      break;

    case NFAPI_SUBFRAME_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_subframe_indication_t))
        retLen = sizeof(nfapi_subframe_indication_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_nr_ul_dci_request_t))
        retLen = sizeof(nfapi_nr_ul_dci_request_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_nr_tx_data_request_t))
        retLen = sizeof(nfapi_nr_tx_data_request_t);

      break;

    case NFAPI_HARQ_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_harq_indication_t))
        retLen = sizeof(nfapi_harq_indication_t);

      break;

    case NFAPI_CRC_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_crc_indication_t))
        retLen = sizeof(nfapi_crc_indication_t);

      break;

    case NFAPI_RX_ULSCH_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_rx_indication_t))
        retLen = sizeof(nfapi_rx_indication_t);

      break;

    case NFAPI_RACH_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_rach_indication_t))
        retLen = sizeof(nfapi_rach_indication_t);

      break;

    case NFAPI_SRS_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_srs_indication_t))
        retLen = sizeof(nfapi_srs_indication_t);

      break;

    case NFAPI_RX_SR_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_sr_indication_t))
        retLen = sizeof(nfapi_sr_indication_t);

      break;

    case NFAPI_RX_CQI_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_cqi_indication_t))
        retLen = sizeof(nfapi_cqi_indication_t);

      break;

    case NFAPI_LBT_DL_CONFIG_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_lbt_dl_config_request_t))
        retLen = sizeof(nfapi_lbt_dl_config_request_t);

      break;

    case NFAPI_LBT_DL_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_lbt_dl_indication_t))
        retLen = sizeof(nfapi_lbt_dl_indication_t);

      break;

    case NFAPI_NB_HARQ_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_nb_harq_indication_t))
        retLen = sizeof(nfapi_nb_harq_indication_t);

      break;

    case NFAPI_NRACH_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_nrach_indication_t))
        retLen = sizeof(nfapi_nrach_indication_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_DL_NODE_SYNC:
      if (unpackedBufLen >= sizeof(nfapi_nr_dl_node_sync_t))
        retLen = sizeof(nfapi_nr_dl_node_sync_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_NODE_SYNC:
      if (unpackedBufLen >= sizeof(nfapi_nr_ul_node_sync_t))
        retLen = sizeof(nfapi_nr_ul_node_sync_t);

      break;

    case NFAPI_TIMING_INFO:
      if (unpackedBufLen >= sizeof(nfapi_timing_info_t))
        retLen = sizeof(nfapi_timing_info_t);

      break;

    case NFAPI_UE_RELEASE_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_ue_release_request_t))
        retLen = sizeof(nfapi_ue_release_request_t);

      break;

    case NFAPI_UE_RELEASE_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_ue_release_response_t))
        retLen = sizeof(nfapi_ue_release_response_t);

      break;

    default:
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unknown message ID %d\n", msgId);
      break;
  }

  return retLen;
}



// Main unpack functions - public

int nfapi_p7_message_header_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p7_codec_config_t *config) {
  nfapi_p7_message_header_t *pMessageHeader = pUnpackedBuf;
  uint8_t *pReadPackedMessage = pMessageBuf;
  uint8_t *end = pMessageBuf + messageBufLen;

  if (pMessageBuf == NULL || pUnpackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 header unpack supplied pointers are null\n");
    return -1;
  }

  if (messageBufLen < NFAPI_P7_HEADER_LENGTH || unpackedBufLen < sizeof(nfapi_p7_message_header_t)) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 header unpack supplied message buffer is too small %d, %d\n", messageBufLen, unpackedBufLen);
    return -1;
  }

  // process the header
  if(!(pull16(&pReadPackedMessage, &pMessageHeader->phy_id, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->message_id, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->message_length, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->m_segment_sequence, end) &&
       pull32(&pReadPackedMessage, &pMessageHeader->checksum, end) &&
       pull32(&pReadPackedMessage, &pMessageHeader->transmit_timestamp, end)))
    return -1;

  return 0;
}

int nfapi_p7_message_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p7_codec_config_t *config) {
  int result = 0;
  nfapi_p7_message_header_t *pMessageHeader = (nfapi_p7_message_header_t *)pUnpackedBuf;
  uint8_t *pReadPackedMessage = pMessageBuf;
  uint8_t *end = pMessageBuf + messageBufLen;

  if (pMessageBuf == NULL || pUnpackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 unpack supplied pointers are null\n");
    return -1;
  }

  if (messageBufLen < NFAPI_P7_HEADER_LENGTH || unpackedBufLen < sizeof(nfapi_p7_message_header_t)) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 unpack supplied message buffer is too small %d, %d\n", messageBufLen, unpackedBufLen);
    return -1;
  }

  /*
    uint8_t *ptr = pMessageBuf;
  printf("\n Read P7 message unpack: ");
  while(ptr < end){
    printf(" %d ", *ptr);
    ptr++;
  }
  printf("\n");
  */
  // clean the supplied buffer for - tag value blanking
  (void)memset(pUnpackedBuf, 0, unpackedBufLen);

  // process the header
  if(!(pull16(&pReadPackedMessage, &pMessageHeader->phy_id, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->message_id, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->message_length, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->m_segment_sequence, end) &&
       pull32(&pReadPackedMessage, &pMessageHeader->checksum, end) &&
       pull32(&pReadPackedMessage, &pMessageHeader->transmit_timestamp, end))) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 unpack header failed\n");
    return -1;
  }

  if((uint8_t *)(pMessageBuf + pMessageHeader->message_length) > end) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 unpack message length is greater than the message buffer \n");
    return -1;
  }

  /*
  if(check_unpack_length(pMessageHeader->message_id, unpackedBufLen) == 0)
  {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 unpack unpack buffer is not large enough \n");
    return -1;
  }
  */

  // look for the specific message
  switch (pMessageHeader->message_id) {
    case NFAPI_DL_CONFIG_REQUEST:
      if (check_unpack_length(NFAPI_DL_CONFIG_REQUEST, unpackedBufLen))
        result = unpack_dl_config_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_UL_CONFIG_REQUEST:
      if (check_unpack_length(NFAPI_UL_CONFIG_REQUEST, unpackedBufLen))
        result = unpack_ul_config_request(&pReadPackedMessage, end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_TX_REQUEST:
      if (check_unpack_length(NFAPI_TX_REQUEST, unpackedBufLen))
        result = unpack_tx_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_HI_DCI0_REQUEST:
      if (check_unpack_length(NFAPI_HI_DCI0_REQUEST, unpackedBufLen))
        result = unpack_hi_dci0_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_UE_RELEASE_REQUEST:
      if (check_unpack_length(NFAPI_UE_RELEASE_REQUEST, unpackedBufLen))
        result = unpack_ue_release_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_HARQ_INDICATION:
      if (check_unpack_length(NFAPI_HARQ_INDICATION, unpackedBufLen))
        result = unpack_harq_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_CRC_INDICATION:
      if (check_unpack_length(NFAPI_CRC_INDICATION, unpackedBufLen))
        result = unpack_crc_indication(&pReadPackedMessage,end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_RX_ULSCH_INDICATION:
      if (check_unpack_length(NFAPI_RX_ULSCH_INDICATION, unpackedBufLen))
        result = unpack_rx_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_RACH_INDICATION:
      if (check_unpack_length(NFAPI_RACH_INDICATION, unpackedBufLen))
        result = unpack_rach_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_SRS_INDICATION:
      if (check_unpack_length(NFAPI_SRS_INDICATION, unpackedBufLen))
        result = unpack_srs_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_RX_SR_INDICATION:
      if (check_unpack_length(NFAPI_RX_SR_INDICATION, unpackedBufLen))
        result = unpack_sr_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_RX_CQI_INDICATION:
      if (check_unpack_length(NFAPI_RX_CQI_INDICATION, unpackedBufLen))
        result = unpack_cqi_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_LBT_DL_CONFIG_REQUEST:
      if (check_unpack_length(NFAPI_LBT_DL_CONFIG_REQUEST, unpackedBufLen))
        result = unpack_lbt_dl_config_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_LBT_DL_INDICATION:
      if (check_unpack_length(NFAPI_LBT_DL_INDICATION, unpackedBufLen))
        result = unpack_lbt_dl_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_NB_HARQ_INDICATION:
      if (check_unpack_length(NFAPI_NB_HARQ_INDICATION, unpackedBufLen))
        result = unpack_nb_harq_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_NRACH_INDICATION:
      if (check_unpack_length(NFAPI_NRACH_INDICATION, unpackedBufLen))
        result = unpack_nrach_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_DL_NODE_SYNC:
      if (check_unpack_length(NFAPI_DL_NODE_SYNC, unpackedBufLen))
        result = unpack_dl_node_sync(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_UL_NODE_SYNC:
      if (check_unpack_length(NFAPI_UL_NODE_SYNC, unpackedBufLen))
        result = unpack_ul_node_sync(&pReadPackedMessage, end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_TIMING_INFO:
      if (check_unpack_length(NFAPI_TIMING_INFO, unpackedBufLen))
        result = unpack_timing_info(&pReadPackedMessage, end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_UE_RELEASE_RESPONSE:
      if (check_unpack_length(NFAPI_UE_RELEASE_RESPONSE, unpackedBufLen))
        result = unpack_ue_release_resp(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    default:
      if(pMessageHeader->message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
          pMessageHeader->message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        if(config && config->unpack_p7_vendor_extension) {
          result = (config->unpack_p7_vendor_extension)(pMessageHeader, &pReadPackedMessage, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s VE NFAPI message ID %d. No ve decoder provided\n", __FUNCTION__, pMessageHeader->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown message ID %d\n", __FUNCTION__, pMessageHeader->message_id);
      }

      break;
  }

  if(result == 0)
    return -1;
  else
    return 0;
}

int nfapi_nr_p7_message_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p7_codec_config_t *config) {
  int result = 0;
  nfapi_p7_message_header_t *pMessageHeader = (nfapi_p7_message_header_t *)pUnpackedBuf;
  uint8_t *pReadPackedMessage = pMessageBuf;
  uint8_t *end = pMessageBuf + messageBufLen;

  if (pMessageBuf == NULL || pUnpackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 unpack supplied pointers are null\n");
    return -1;
  }

  if (messageBufLen < NFAPI_P7_HEADER_LENGTH || unpackedBufLen < sizeof(nfapi_p7_message_header_t)) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 unpack supplied message buffer is too small %d, %d\n", messageBufLen, unpackedBufLen);
    return -1;
  }

  /*
    uint8_t *ptr = pMessageBuf;
  printf("\n Read P7 message unpack: ");
  while(ptr < end){
    printf(" %d ", *ptr);
    ptr++;
  }
  printf("\n");
  */
  // clean the supplied buffer for - tag value blanking
  (void)memset(pUnpackedBuf, 0, unpackedBufLen);

  // process the header
  if(!(pull16(&pReadPackedMessage, &pMessageHeader->phy_id, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->message_id, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->message_length, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->m_segment_sequence, end) &&
       pull32(&pReadPackedMessage, &pMessageHeader->checksum, end) &&
       pull32(&pReadPackedMessage, &pMessageHeader->transmit_timestamp, end))) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 unpack header failed\n");
    return -1;
  }

  if((uint8_t *)(pMessageBuf + pMessageHeader->message_length) > end) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 unpack message length is greater than the message buffer \n");
    return -1;
  }

  /*
  if(check_unpack_length(pMessageHeader->message_id, unpackedBufLen) == 0)
  {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 unpack unpack buffer is not large enough \n");
    return -1;
  }
  */

  // look for the specific message
  switch (pMessageHeader->message_id) {
    case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
      if (check_nr_unpack_length(NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST, unpackedBufLen))
        result = unpack_dl_tti_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
      if (check_nr_unpack_length(NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST, unpackedBufLen))
        result = unpack_ul_tti_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
      if (check_nr_unpack_length(NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST, unpackedBufLen))
        result = unpack_tx_data_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
      if (check_nr_unpack_length(NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST, unpackedBufLen))
        result = unpack_ul_dci_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_UE_RELEASE_REQUEST:
      if (check_nr_unpack_length(NFAPI_UE_RELEASE_REQUEST, unpackedBufLen))
        result = unpack_ue_release_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_HARQ_INDICATION:
      if (check_nr_unpack_length(NFAPI_HARQ_INDICATION, unpackedBufLen))
        result = unpack_harq_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_CRC_INDICATION:
      if (check_nr_unpack_length(NFAPI_CRC_INDICATION, unpackedBufLen))
        result = unpack_crc_indication(&pReadPackedMessage,end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_RX_ULSCH_INDICATION:
      if (check_nr_unpack_length(NFAPI_RX_ULSCH_INDICATION, unpackedBufLen))
        result = unpack_rx_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_RACH_INDICATION:
      if (check_nr_unpack_length(NFAPI_RACH_INDICATION, unpackedBufLen))
        result = unpack_rach_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_SRS_INDICATION:
      if (check_nr_unpack_length(NFAPI_SRS_INDICATION, unpackedBufLen))
        result = unpack_srs_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_RX_SR_INDICATION:
      if (check_nr_unpack_length(NFAPI_RX_SR_INDICATION, unpackedBufLen))
        result = unpack_sr_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_RX_CQI_INDICATION:
      if (check_nr_unpack_length(NFAPI_RX_CQI_INDICATION, unpackedBufLen))
        result = unpack_cqi_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_LBT_DL_CONFIG_REQUEST:
      if (check_nr_unpack_length(NFAPI_LBT_DL_CONFIG_REQUEST, unpackedBufLen))
        result = unpack_lbt_dl_config_request(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_LBT_DL_INDICATION:
      if (check_nr_unpack_length(NFAPI_LBT_DL_INDICATION, unpackedBufLen))
        result = unpack_lbt_dl_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_NB_HARQ_INDICATION:
      if (check_nr_unpack_length(NFAPI_NB_HARQ_INDICATION, unpackedBufLen))
        result = unpack_nb_harq_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_NRACH_INDICATION:
      if (check_nr_unpack_length(NFAPI_NRACH_INDICATION, unpackedBufLen))
        result = unpack_nrach_indication(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_NR_PHY_MSG_TYPE_DL_NODE_SYNC:
      if (check_nr_unpack_length(NFAPI_NR_PHY_MSG_TYPE_DL_NODE_SYNC, unpackedBufLen))
        result = unpack_nr_dl_node_sync(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_NODE_SYNC:
      if (check_nr_unpack_length(NFAPI_NR_PHY_MSG_TYPE_UL_NODE_SYNC, unpackedBufLen))
        result = unpack_nr_ul_node_sync(&pReadPackedMessage, end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_TIMING_INFO:
      if (check_nr_unpack_length(NFAPI_TIMING_INFO, unpackedBufLen))
        result = unpack_nr_timing_info(&pReadPackedMessage, end, pMessageHeader, config);
      else
        return -1;

      break;

    case NFAPI_UE_RELEASE_RESPONSE:
      if (check_nr_unpack_length(NFAPI_UE_RELEASE_RESPONSE, unpackedBufLen))
        result = unpack_ue_release_resp(&pReadPackedMessage,  end, pMessageHeader, config);
      else
        return -1;

      break;

    default:
      if(pMessageHeader->message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
          pMessageHeader->message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        if(config && config->unpack_p7_vendor_extension) {
          result = (config->unpack_p7_vendor_extension)(pMessageHeader, &pReadPackedMessage, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s VE NFAPI message ID %d. No ve decoder provided\n", __FUNCTION__, pMessageHeader->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown message ID %d\n", __FUNCTION__, pMessageHeader->message_id);
      }

      break;
  }

  if(result == 0)
    return -1;
  else
    return 0;
}


