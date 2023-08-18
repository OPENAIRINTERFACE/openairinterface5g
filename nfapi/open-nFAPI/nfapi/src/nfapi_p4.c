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


#include <arpa/inet.h> // need for uintptr_t?

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <nfapi_interface.h>
#include <nfapi.h>
#include <debug.h>

static uint32_t get_packed_msg_len(uintptr_t msgHead, uintptr_t msgEnd) {
  if (msgEnd < msgHead) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "get_packed_msg_len: Error in pointers supplied %lu, %lu\n", msgHead, msgEnd);
    return 0;
  }

  return (msgEnd - msgHead);
}

static uint8_t pack_opaque_data_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_opaqaue_data_t *value = (nfapi_opaqaue_data_t *)tlv;
  return pusharray8(value->value, NFAPI_MAX_OPAQUE_DATA, value->length, ppWritePackedMsg, end);
}

static uint8_t unpack_opaque_data_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_opaqaue_data_t *value = (nfapi_opaqaue_data_t *)tlv;
  value->length = value->tl.length;

  if(value->length <= NFAPI_MAX_OPAQUE_DATA) {
    if(!pullarray8(ppReadPackedMsg, value->value, NFAPI_MAX_OPAQUE_DATA, value->length, end))
      return 0;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Opaque date to long %d \n", value->length);
    return 0;
  }

  return 1;
}

static uint8_t pack_lte_rssi_request_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lte_rssi_request_t *value = (nfapi_lte_rssi_request_t *)tlv;
  return (push8(value->frequency_band_indicator, ppWritePackedMsg, end) &&
          push16(value->measurement_period, ppWritePackedMsg, end) &&
          push8(value->bandwidth, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end) &&
          push8(value->number_of_earfcns, ppWritePackedMsg, end) &&
          pusharray16(value->earfcn, NFAPI_MAX_CARRIER_LIST, value->number_of_earfcns, ppWritePackedMsg, end));
}

static uint8_t pack_utran_rssi_request_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_utran_rssi_request_t *value = (nfapi_utran_rssi_request_t *)tlv;
  return (push8(value->frequency_band_indicator, ppWritePackedMsg, end) &&
          push16(value->measurement_period, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end) &&
          push8(value->number_of_uarfcns, ppWritePackedMsg, end) &&
          pusharray16(value->uarfcn, NFAPI_MAX_CARRIER_LIST, value->number_of_uarfcns, ppWritePackedMsg, end));
}

static uint8_t pack_geran_rssi_request_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_geran_rssi_request_t *value = (nfapi_geran_rssi_request_t *)tlv;
  uint16_t idx = 0;

  if(!(push8(value->frequency_band_indicator, ppWritePackedMsg, end) &&
       push16(value->measurement_period, ppWritePackedMsg, end) &&
       push32(value->timeout, ppWritePackedMsg, end) &&
       push8(value->number_of_arfcns, ppWritePackedMsg, end)))
    return 0;

  for(; idx < value->number_of_arfcns; ++idx) {
    if(!(push16(value->arfcn[idx].arfcn, ppWritePackedMsg, end) &&
         push8(value->arfcn[idx].direction, ppWritePackedMsg, end)))
      return 0;
  }

  return 1;
}

static uint8_t pack_nb_iot_rssi_request_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nb_iot_rssi_request_t *value = (nfapi_nb_iot_rssi_request_t *)tlv;
  uint16_t idx = 0;

  if(!(push8(value->frequency_band_indicator, ppWritePackedMsg, end) &&
       push16(value->measurement_period, ppWritePackedMsg, end) &&
       push32(value->timeout, ppWritePackedMsg, end) &&
       push8(value->number_of_earfcns, ppWritePackedMsg, end)))
    return 0;

  for(; idx < value->number_of_earfcns; ++idx) {
    if(!(push16(value->earfcn[idx].earfcn, ppWritePackedMsg, end) &&
         push8(value->earfcn[idx].number_of_ro_dl, ppWritePackedMsg, end)))
      return 0;

    uint8_t ro_dl_idx = 0;

    for(ro_dl_idx = 0; ro_dl_idx < value->earfcn[idx].number_of_ro_dl; ++ro_dl_idx) {
      if(!push8(value->earfcn[idx].ro_dl[ro_dl_idx], ppWritePackedMsg, end))
        return 0;
    }
  }

  return 1;
}



static uint8_t pack_rssi_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_rssi_request_t *pNfapiMsg = (nfapi_rssi_request_t *)msg;

  if(push8(pNfapiMsg->rat_type, ppWritePackedMsg, end) == 0)
    return 0;

  switch(pNfapiMsg->rat_type) {
    case NFAPI_RAT_TYPE_LTE:
      if(pack_tlv(NFAPI_LTE_RSSI_REQUEST_TAG, &pNfapiMsg->lte_rssi_request, ppWritePackedMsg, end, &pack_lte_rssi_request_value) == 0)
        return 0;

      break;

    case NFAPI_RAT_TYPE_UTRAN:
      if(pack_tlv(NFAPI_UTRAN_RSSI_REQUEST_TAG, &pNfapiMsg->utran_rssi_request, ppWritePackedMsg, end, &pack_utran_rssi_request_value) == 0)
        return 0;

      break;

    case NFAPI_RAT_TYPE_GERAN:
      if(pack_tlv(NFAPI_GERAN_RSSI_REQUEST_TAG, &pNfapiMsg->geran_rssi_request, ppWritePackedMsg, end, &pack_geran_rssi_request_value) == 0)
        return 0;

      break;

    case NFAPI_RAT_TYPE_NB_IOT:
      if(pack_tlv(NFAPI_NB_IOT_RSSI_REQUEST_TAG, &pNfapiMsg->nb_iot_rssi_request, ppWritePackedMsg, end, &pack_nb_iot_rssi_request_value) == 0)
        return 0;

      break;
  }

  return pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config);
}

static uint8_t pack_rssi_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_rssi_response_t *pNfapiMsg = (nfapi_rssi_response_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_rssi_indication_body_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_rssi_indication_body_t *value = (nfapi_rssi_indication_body_t *)tlv;
  return (push16(value->number_of_rssi, ppWritePackedMsg, end) &&
          pusharrays16(value->rssi, NFAPI_MAX_RSSI, value->number_of_rssi, ppWritePackedMsg, end));
}

static uint8_t pack_rssi_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_rssi_indication_t *pNfapiMsg = (nfapi_rssi_indication_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_tlv(NFAPI_RSSI_INDICATION_TAG, &pNfapiMsg->rssi_indication_body, ppWritePackedMsg, end, &pack_rssi_indication_body_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}


static uint8_t pack_lte_cell_search_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lte_cell_search_request_t *value = (nfapi_lte_cell_search_request_t *)msg;
  return (push16(value->earfcn, ppWritePackedMsg, end) &&
          push8(value->measurement_bandwidth,  ppWritePackedMsg, end) &&
          push8(value->exhaustive_search, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end) &&
          push8(value->number_of_pci, ppWritePackedMsg, end) &&
          pusharray16(value->pci, NFAPI_MAX_PCI_LIST, value->number_of_pci, ppWritePackedMsg, end));
}

static uint8_t pack_utran_cell_search_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_utran_cell_search_request_t *value = (nfapi_utran_cell_search_request_t *)msg;
  return (push16(value->uarfcn, ppWritePackedMsg, end) &&
          push8(value->exhaustive_search, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end) &&
          push8(value->number_of_psc, ppWritePackedMsg, end) &&
          pusharray16(value->psc, NFAPI_MAX_PSC_LIST, value->number_of_psc, ppWritePackedMsg, end));
}

static uint8_t pack_geran_cell_search_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_geran_cell_search_request_t *value = (nfapi_geran_cell_search_request_t *)msg;
  return (push32(value->timeout, ppWritePackedMsg, end) &&
          push8(value->number_of_arfcn, ppWritePackedMsg, end) &&
          pusharray16(value->arfcn, NFAPI_MAX_ARFCN_LIST, value->number_of_arfcn, ppWritePackedMsg, end));
}

static uint8_t pack_nb_iot_cell_search_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nb_iot_cell_search_request_t *value = (nfapi_nb_iot_cell_search_request_t *)msg;
  return (push16(value->earfcn, ppWritePackedMsg, end) &&
          push8(value->ro_dl, ppWritePackedMsg, end) &&
          push8(value->exhaustive_search, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end) &&
          push8(value->number_of_pci, ppWritePackedMsg, end) &&
          pusharray16(value->pci, NFAPI_MAX_PCI_LIST, value->number_of_pci, ppWritePackedMsg, end));
}


static uint8_t pack_cell_search_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_cell_search_request_t *pNfapiMsg = (nfapi_cell_search_request_t *)msg;

  if(push8(pNfapiMsg->rat_type, ppWritePackedMsg, end) == 0)
    return 0;

  switch(pNfapiMsg->rat_type) {
    case NFAPI_RAT_TYPE_LTE: {
      if(pack_tlv(NFAPI_LTE_CELL_SEARCH_REQUEST_TAG, &pNfapiMsg->lte_cell_search_request, ppWritePackedMsg, end, &pack_lte_cell_search_request_value) == 0)
        return 0;
    }
    break;

    case NFAPI_RAT_TYPE_UTRAN: {
      if(pack_tlv(NFAPI_UTRAN_CELL_SEARCH_REQUEST_TAG, &pNfapiMsg->utran_cell_search_request, ppWritePackedMsg, end, &pack_utran_cell_search_request_value) == 0)
        return 0;
    }
    break;

    case NFAPI_RAT_TYPE_GERAN: {
      if(pack_tlv(NFAPI_GERAN_CELL_SEARCH_REQUEST_TAG, &pNfapiMsg->geran_cell_search_request, ppWritePackedMsg, end, &pack_geran_cell_search_request_value) == 0)
        return 0;
    }
    break;

    case NFAPI_RAT_TYPE_NB_IOT: {
      if(pack_tlv(NFAPI_NB_IOT_CELL_SEARCH_REQUEST_TAG, &pNfapiMsg->nb_iot_cell_search_request, ppWritePackedMsg, end, &pack_nb_iot_cell_search_request_value) == 0)
        return 0;
    }
    break;
  };

  return (pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_cell_search_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_cell_search_response_t *pNfapiMsg = (nfapi_cell_search_response_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_lte_cell_search_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lte_cell_search_indication_t *value = (nfapi_lte_cell_search_indication_t *)msg;
  uint16_t idx = 0;

  if(push16(value->number_of_lte_cells_found, ppWritePackedMsg, end) == 0)
    return 0;

  for(idx = 0; idx < value->number_of_lte_cells_found; ++idx) {
    if(!(push16(value->lte_found_cells[idx].pci, ppWritePackedMsg, end) &&
         push8(value->lte_found_cells[idx].rsrp, ppWritePackedMsg, end) &&
         push8(value->lte_found_cells[idx].rsrq, ppWritePackedMsg, end) &&
         pushs16(value->lte_found_cells[idx].frequency_offset, ppWritePackedMsg, end)))
      return 0;
  }

  return 1;
}

static uint8_t pack_utran_cell_search_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_utran_cell_search_indication_t *value = (nfapi_utran_cell_search_indication_t *)msg;
  uint16_t idx = 0;

  if(push16(value->number_of_utran_cells_found, ppWritePackedMsg, end) == 0)
    return 0;

  for(idx = 0; idx < value->number_of_utran_cells_found; ++idx) {
    if(!(push16(value->utran_found_cells[idx].psc, ppWritePackedMsg, end) &&
         push8(value->utran_found_cells[idx].rscp, ppWritePackedMsg, end) &&
         push8(value->utran_found_cells[idx].ecno, ppWritePackedMsg, end) &&
         pushs16(value->utran_found_cells[idx].frequency_offset, ppWritePackedMsg, end)))
      return 0;
  }

  return 1;
}

static uint8_t pack_geran_cell_search_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_geran_cell_search_indication_t *value = (nfapi_geran_cell_search_indication_t *)msg;
  uint16_t idx = 0;

  if(push16(value->number_of_gsm_cells_found, ppWritePackedMsg, end) == 0)
    return 0;

  for(idx = 0; idx < value->number_of_gsm_cells_found; ++idx) {
    if(!(push16(value->gsm_found_cells[idx].arfcn, ppWritePackedMsg, end) &&
         push8(value->gsm_found_cells[idx].bsic, ppWritePackedMsg, end) &&
         push8(value->gsm_found_cells[idx].rxlev, ppWritePackedMsg, end) &&
         push8(value->gsm_found_cells[idx].rxqual, ppWritePackedMsg, end) &&
         pushs16(value->gsm_found_cells[idx].frequency_offset, ppWritePackedMsg, end) &&
         push32(value->gsm_found_cells[idx].sfn_offset, ppWritePackedMsg, end)))
      return 0;
  }

  return 1;
}

static uint8_t pack_nb_iot_cell_search_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nb_iot_cell_search_indication_t *value = (nfapi_nb_iot_cell_search_indication_t *)msg;
  uint16_t idx = 0;

  if(push16(value->number_of_nb_iot_cells_found, ppWritePackedMsg, end) == 0)
    return 0;

  for(idx = 0; idx < value->number_of_nb_iot_cells_found; ++idx) {
    if(!(push16(value->nb_iot_found_cells[idx].pci, ppWritePackedMsg, end) &&
         push8(value->nb_iot_found_cells[idx].rsrp, ppWritePackedMsg, end) &&
         push8(value->nb_iot_found_cells[idx].rsrq, ppWritePackedMsg, end) &&
         pushs16(value->nb_iot_found_cells[idx].frequency_offset, ppWritePackedMsg, end)))
      return 0;
  }

  return 1;
}
static uint8_t pack_cell_search_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_cell_search_indication_t *pNfapiMsg = (nfapi_cell_search_indication_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_tlv(NFAPI_LTE_CELL_SEARCH_INDICATION_TAG, &pNfapiMsg->lte_cell_search_indication, ppWritePackedMsg, end, &pack_lte_cell_search_indication_value) &&
          pack_tlv(NFAPI_UTRAN_CELL_SEARCH_INDICATION_TAG, &pNfapiMsg->utran_cell_search_indication, ppWritePackedMsg, end, &pack_utran_cell_search_indication_value) &&
          pack_tlv(NFAPI_GERAN_CELL_SEARCH_INDICATION_TAG, &pNfapiMsg->geran_cell_search_indication, ppWritePackedMsg, end, &pack_geran_cell_search_indication_value) &&
          pack_tlv(NFAPI_PNF_CELL_SEARCH_STATE_TAG, &pNfapiMsg->pnf_cell_search_state, ppWritePackedMsg, end, &pack_opaque_data_value) &&
          pack_tlv(NFAPI_NB_IOT_CELL_SEARCH_INDICATION_TAG, &pNfapiMsg->nb_iot_cell_search_indication, ppWritePackedMsg, end, &pack_nb_iot_cell_search_indication_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_lte_broadcast_detect_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lte_broadcast_detect_request_t *value = (nfapi_lte_broadcast_detect_request_t *)msg;
  return (push16(value->earfcn, ppWritePackedMsg, end) &&
          push16(value->pci, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end));
}

static uint8_t pack_utran_broadcast_detect_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_utran_broadcast_detect_request_t *value = (nfapi_utran_broadcast_detect_request_t *)msg;
  return (push16(value->uarfcn, ppWritePackedMsg, end) &&
          push16(value->psc, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end));
}

static uint8_t pack_nb_iot_broadcast_detect_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nb_iot_broadcast_detect_request_t *value = (nfapi_nb_iot_broadcast_detect_request_t *)msg;
  return (push16(value->earfcn, ppWritePackedMsg, end) &&
          push8(value->ro_dl, ppWritePackedMsg, end) &&
          push16(value->pci, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end));
}

static uint8_t pack_broadcast_detect_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_broadcast_detect_request_t *pNfapiMsg = (nfapi_broadcast_detect_request_t *)msg;

  if(push8(pNfapiMsg->rat_type, ppWritePackedMsg, end) == 0)
    return 0;

  switch(pNfapiMsg->rat_type) {
    case NFAPI_RAT_TYPE_LTE:
      if(pack_tlv(NFAPI_LTE_BROADCAST_DETECT_REQUEST_TAG, &pNfapiMsg->lte_broadcast_detect_request, ppWritePackedMsg, end,&pack_lte_broadcast_detect_request_value) == 0)
        return 0;

      break;

    case NFAPI_RAT_TYPE_UTRAN:
      if(pack_tlv(NFAPI_UTRAN_BROADCAST_DETECT_REQUEST_TAG, &pNfapiMsg->utran_broadcast_detect_request, ppWritePackedMsg, end, &pack_utran_broadcast_detect_request_value) == 0)
        return 0;

      break;

    case NFAPI_RAT_TYPE_NB_IOT:
      if(pack_tlv(NFAPI_NB_IOT_BROADCAST_DETECT_REQUEST_TAG, &pNfapiMsg->nb_iot_broadcast_detect_request, ppWritePackedMsg, end, &pack_nb_iot_broadcast_detect_request_value) == 0)
        return 0;

      break;
  }

  return (pack_tlv(NFAPI_PNF_CELL_SEARCH_STATE_TAG, &pNfapiMsg->pnf_cell_search_state, ppWritePackedMsg, end, &pack_opaque_data_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_broadcast_detect_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_broadcast_detect_response_t *pNfapiMsg = (nfapi_broadcast_detect_response_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_lte_broadcast_detect_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lte_broadcast_detect_indication_t *value = (nfapi_lte_broadcast_detect_indication_t *)msg;
  return (push8(value->number_of_tx_antenna, ppWritePackedMsg, end) &&
          push16(value->mib_length, ppWritePackedMsg, end) &&
          pusharray8(value->mib, NFAPI_MAX_MIB_LENGTH, value->mib_length, ppWritePackedMsg, end) &&
          push32(value->sfn_offset, ppWritePackedMsg, end));
}

static uint8_t pack_utran_broadcast_detect_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_utran_broadcast_detect_indication_t *value = (nfapi_utran_broadcast_detect_indication_t *)msg;
  return (push16(value->mib_length, ppWritePackedMsg, end) &&
          pusharray8(value->mib, NFAPI_MAX_MIB_LENGTH, value->mib_length, ppWritePackedMsg, end) &&
          push32(value->sfn_offset, ppWritePackedMsg, end));
}

static uint8_t pack_nb_iot_broadcast_detect_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nb_iot_broadcast_detect_indication_t *value = (nfapi_nb_iot_broadcast_detect_indication_t *)msg;
  return (push8(value->number_of_tx_antenna, ppWritePackedMsg, end) &&
          push16(value->mib_length, ppWritePackedMsg, end) &&
          pusharray8(value->mib, NFAPI_MAX_MIB_LENGTH, value->mib_length, ppWritePackedMsg, end) &&
          push32(value->sfn_offset, ppWritePackedMsg, end));
}

static uint8_t pack_broadcast_detect_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_broadcast_detect_indication_t *pNfapiMsg = (nfapi_broadcast_detect_indication_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_tlv(NFAPI_LTE_BROADCAST_DETECT_INDICATION_TAG, &pNfapiMsg->lte_broadcast_detect_indication, ppWritePackedMsg, end, &pack_lte_broadcast_detect_indication_value) &&
          pack_tlv(NFAPI_UTRAN_BROADCAST_DETECT_INDICATION_TAG, &pNfapiMsg->utran_broadcast_detect_indication, ppWritePackedMsg, end, &pack_utran_broadcast_detect_indication_value) &&
          pack_tlv(NFAPI_PNF_CELL_BROADCAST_STATE_TAG, &pNfapiMsg->pnf_cell_broadcast_state, ppWritePackedMsg, end, &pack_opaque_data_value) &&
          pack_tlv(NFAPI_NB_IOT_BROADCAST_DETECT_INDICATION_TAG, &pNfapiMsg->nb_iot_broadcast_detect_indication, ppWritePackedMsg, end, &pack_nb_iot_broadcast_detect_indication_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_lte_system_information_schedule_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lte_system_information_schedule_request_t *value = (nfapi_lte_system_information_schedule_request_t *)msg;
  return (push16(value->earfcn, ppWritePackedMsg, end) &&
          push16(value->pci, ppWritePackedMsg, end) &&
          push16(value->downlink_channel_bandwidth, ppWritePackedMsg, end) &&
          push8(value->phich_configuration, ppWritePackedMsg, end) &&
          push8(value->number_of_tx_antenna, ppWritePackedMsg, end) &&
          push8(value->retry_count, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end));
}

static uint8_t pack_nb_iot_system_information_schedule_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nb_iot_system_information_schedule_request_t *value = (nfapi_nb_iot_system_information_schedule_request_t *)msg;
  return (push16(value->earfcn, ppWritePackedMsg, end) &&
          push8(value->ro_dl, ppWritePackedMsg, end) &&
          push16(value->pci, ppWritePackedMsg, end) &&
          push8(value->scheduling_info_sib1_nb, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end));
}


static uint8_t pack_system_information_schedule_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_schedule_request_t *pNfapiMsg = (nfapi_system_information_schedule_request_t *)msg;

  if(push8(pNfapiMsg->rat_type, ppWritePackedMsg, end) == 0)
    return 0;

  switch(pNfapiMsg->rat_type) {
    case NFAPI_RAT_TYPE_LTE:
      if(pack_tlv(NFAPI_LTE_SYSTEM_INFORMATION_SCHEDULE_REQUEST_TAG, &pNfapiMsg->lte_system_information_schedule_request, ppWritePackedMsg, end, &pack_lte_system_information_schedule_request_value) == 0)
        return 0;

      break;

    case NFAPI_RAT_TYPE_NB_IOT:
      if(pack_tlv(NFAPI_NB_IOT_SYSTEM_INFORMATION_SCHEDULE_REQUEST_TAG, &pNfapiMsg->nb_iot_system_information_schedule_request, ppWritePackedMsg, end,
                  &pack_nb_iot_system_information_schedule_request_value) == 0)
        return 0;

      break;
  }

  return (pack_tlv(NFAPI_PNF_CELL_BROADCAST_STATE_TAG, &pNfapiMsg->pnf_cell_broadcast_state, ppWritePackedMsg, end, &pack_opaque_data_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_system_information_schedule_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_schedule_response_t *pNfapiMsg = (nfapi_system_information_schedule_response_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_lte_system_information_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lte_system_information_indication_t *value = (nfapi_lte_system_information_indication_t *)msg;
  return (push8(value->sib_type, ppWritePackedMsg, end) &&
          push16(value->sib_length, ppWritePackedMsg, end) &&
          pusharray8(value->sib, NFAPI_MAX_SIB_LENGTH, value->sib_length, ppWritePackedMsg, end));
}

static uint8_t pack_nb_iot_system_information_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nb_iot_system_information_indication_t *value = (nfapi_nb_iot_system_information_indication_t *)msg;
  return (push8(value->sib_type, ppWritePackedMsg, end) &&
          push16(value->sib_length, ppWritePackedMsg, end) &&
          pusharray8(value->sib, NFAPI_MAX_SIB_LENGTH, value->sib_length, ppWritePackedMsg, end));
}

static uint8_t pack_system_information_schedule_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_schedule_indication_t *pNfapiMsg = (nfapi_system_information_schedule_indication_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_tlv(NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->lte_system_information_indication, ppWritePackedMsg, end, &pack_lte_system_information_indication_value) &&
          pack_tlv(NFAPI_NB_IOT_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->nb_iot_system_information_indication, ppWritePackedMsg, end, &pack_nb_iot_system_information_indication_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_lte_system_information_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_lte_system_information_request_t *value = (nfapi_lte_system_information_request_t *)msg;
  uint16_t idx = 0;

  if(!(push16(value->earfcn, ppWritePackedMsg, end) &&
       push16(value->pci, ppWritePackedMsg, end) &&
       push16(value->downlink_channel_bandwidth, ppWritePackedMsg, end) &&
       push8(value->phich_configuration, ppWritePackedMsg, end) &&
       push8(value->number_of_tx_antenna, ppWritePackedMsg, end) &&
       push8(value->number_of_si_periodicity, ppWritePackedMsg, end)))
    return 0;

  for(idx = 0; idx < value->number_of_si_periodicity; ++idx) {
    if(!(push8(value->si_periodicity[idx].si_periodicity, ppWritePackedMsg, end) &&
         push8(value->si_periodicity[idx].si_index, ppWritePackedMsg, end)))
      return 0;
  }

  return (push8(value->si_window_length, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end));
}
static uint8_t pack_utran_system_information_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_utran_system_information_request_t *value = (nfapi_utran_system_information_request_t *)msg;
  return (push16(value->uarfcn, ppWritePackedMsg, end) &&
          push16(value->psc, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end));
}
static uint8_t pack_geran_system_information_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_geran_system_information_request_t *value = (nfapi_geran_system_information_request_t *)msg;
  return (push16(value->arfcn, ppWritePackedMsg, end) &&
          push8(value->bsic, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end));
}
static uint8_t pack_nb_iot_system_information_request_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nb_iot_system_information_request_t *value = (nfapi_nb_iot_system_information_request_t *)msg;
  uint16_t idx = 0;

  if(!(push16(value->earfcn, ppWritePackedMsg, end) &&
       push8(value->ro_dl, ppWritePackedMsg, end) &&
       push16(value->pci, ppWritePackedMsg, end) &&
       push8(value->number_of_si_periodicity, ppWritePackedMsg, end)))
    return 0;

  for(idx = 0; idx < value->number_of_si_periodicity; ++idx) {
    if(!(push8(value->si_periodicity[idx].si_periodicity, ppWritePackedMsg, end) &&
         push8(value->si_periodicity[idx].si_repetition_pattern, ppWritePackedMsg, end) &&
         push8(value->si_periodicity[idx].si_tb_size, ppWritePackedMsg, end) &&
         push8(value->si_periodicity[idx].number_of_si_index, ppWritePackedMsg, end)))
      return 0;

    uint8_t si_idx;

    for(si_idx = 0; si_idx < value->si_periodicity[idx].number_of_si_index; ++si_idx) {
      if(!(push8(value->si_periodicity[idx].si_index[si_idx], ppWritePackedMsg, end)))
        return 0;
    }
  }

  return (push8(value->si_window_length, ppWritePackedMsg, end) &&
          push32(value->timeout, ppWritePackedMsg, end));
}


static uint8_t pack_system_information_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_request_t *pNfapiMsg = (nfapi_system_information_request_t *)msg;

  if(push8(pNfapiMsg->rat_type, ppWritePackedMsg, end) == 0)
    return 0;

  switch(pNfapiMsg->rat_type) {
    case NFAPI_RAT_TYPE_LTE:
      if(pack_tlv(NFAPI_LTE_SYSTEM_INFORMATION_REQUEST_TAG, &pNfapiMsg->lte_system_information_request, ppWritePackedMsg, end, &pack_lte_system_information_request_value) == 0)
        return 0;

      break;

    case NFAPI_RAT_TYPE_UTRAN:
      if(pack_tlv(NFAPI_UTRAN_SYSTEM_INFORMATION_REQUEST_TAG, &pNfapiMsg->utran_system_information_request, ppWritePackedMsg, end, &pack_utran_system_information_request_value) == 0)
        return 0;

      break;

    case NFAPI_RAT_TYPE_GERAN:
      if(pack_tlv(NFAPI_GERAN_SYSTEM_INFORMATION_REQUEST_TAG, &pNfapiMsg->geran_system_information_request, ppWritePackedMsg, end, &pack_geran_system_information_request_value) == 0)
        return 0;

      break;

    case NFAPI_RAT_TYPE_NB_IOT:
      if(pack_tlv(NFAPI_NB_IOT_SYSTEM_INFORMATION_REQUEST_TAG, &pNfapiMsg->nb_iot_system_information_request, ppWritePackedMsg, end, &pack_nb_iot_system_information_request_value) == 0)
        return 0;

      break;
  }

  return (pack_tlv(NFAPI_PNF_CELL_BROADCAST_STATE_TAG, &pNfapiMsg->pnf_cell_broadcast_state, ppWritePackedMsg, end, &pack_opaque_data_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_system_information_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end,  nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_response_t *pNfapiMsg = (nfapi_system_information_response_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_utran_system_information_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_utran_system_information_indication_t *value = (nfapi_utran_system_information_indication_t *)msg;
  return (push16(value->sib_length, ppWritePackedMsg, end) &&
          pusharray8(value->sib, NFAPI_MAX_SIB_LENGTH, value->sib_length, ppWritePackedMsg, end));
}

static uint8_t pack_geran_system_information_indication_value(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_geran_system_information_indication_t *value = (nfapi_geran_system_information_indication_t *)msg;
  return (push16(value->si_length, ppWritePackedMsg, end) &&
          pusharray8(value->si, NFAPI_MAX_SIB_LENGTH, value->si_length, ppWritePackedMsg, end));
}

static uint8_t pack_system_information_indication(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_indication_t *pNfapiMsg = (nfapi_system_information_indication_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_tlv(NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->lte_system_information_indication, ppWritePackedMsg, end, &pack_lte_system_information_indication_value) &&
          pack_tlv(NFAPI_UTRAN_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->utran_system_information_indication, ppWritePackedMsg, end, &pack_utran_system_information_indication_value) &&
          pack_tlv(NFAPI_GERAN_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->geran_system_information_indication, ppWritePackedMsg, end, &pack_geran_system_information_indication_value) &&
          pack_tlv(NFAPI_NB_IOT_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->nb_iot_system_information_indication, ppWritePackedMsg, end, &pack_nb_iot_system_information_indication_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_nmm_stop_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_nmm_stop_request_t *pNfapiMsg = (nfapi_nmm_stop_request_t *)msg;
  return (pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_nmm_stop_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_nmm_stop_response_t *pNfapiMsg = (nfapi_nmm_stop_response_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t unpack_lte_rssi_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  //int result = 0;
  nfapi_lte_rssi_request_t *value = (nfapi_lte_rssi_request_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->frequency_band_indicator, end) &&
       pull16(ppReadPackedMsg, &value->measurement_period, end) &&
       pull8(ppReadPackedMsg, &value->bandwidth, end) &&
       pull32(ppReadPackedMsg, &value->timeout, end) &&
       pull8(ppReadPackedMsg, &value->number_of_earfcns, end)))
    return 0;

  if(value->number_of_earfcns <= NFAPI_MAX_CARRIER_LIST) {
    if(pullarray16(ppReadPackedMsg, value->earfcn, NFAPI_MAX_CARRIER_LIST, value->number_of_earfcns, end) == 0)
      return 0;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More EARFCN's than we can decode %d \n", value->number_of_earfcns);
    return 0;
  }

  return 1;
}

static uint8_t unpack_utran_rssi_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_utran_rssi_request_t *value = (nfapi_utran_rssi_request_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->frequency_band_indicator, end) &&
       pull16(ppReadPackedMsg, &value->measurement_period, end) &&
       pull32(ppReadPackedMsg, &value->timeout, end) &&
       pull8(ppReadPackedMsg, &value->number_of_uarfcns, end)))
    return 0;

  if(value->number_of_uarfcns <= NFAPI_MAX_CARRIER_LIST) {
    if(pullarray16(ppReadPackedMsg, value->uarfcn, NFAPI_MAX_CARRIER_LIST, value->number_of_uarfcns, end) == 0)
      return 0;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More UARFCN's than we can decode %d \n", value->number_of_uarfcns);
    return 0;
  }

  return 1;
}

static uint8_t unpack_geran_rssi_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_geran_rssi_request_t *value = (nfapi_geran_rssi_request_t *)tlv;
  uint16_t idx = 0;

  if(!(pull8(ppReadPackedMsg, &value->frequency_band_indicator, end) &&
       pull16(ppReadPackedMsg, &value->measurement_period, end) &&
       pull32(ppReadPackedMsg, &value->timeout, end) &&
       pull8(ppReadPackedMsg, &value->number_of_arfcns, end)))
    return 0;

  if(value->number_of_arfcns <= NFAPI_MAX_CARRIER_LIST) {
    for(idx = 0; idx < value->number_of_arfcns; ++idx) {
      if(!(pull16(ppReadPackedMsg, &value->arfcn[idx].arfcn, end) &&
           pull8(ppReadPackedMsg, &value->arfcn[idx].direction, end)))
        return 0;
    }
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More ARFCN's than we can decode %d \n", value->number_of_arfcns);
    return 0;
  }

  return 1;
}

static uint8_t unpack_nb_iot_rssi_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nb_iot_rssi_request_t *value = (nfapi_nb_iot_rssi_request_t *)tlv;
  uint16_t idx = 0;

  if(!(pull8(ppReadPackedMsg, &value->frequency_band_indicator, end) &&
       pull16(ppReadPackedMsg, &value->measurement_period, end) &&
       pull32(ppReadPackedMsg, &value->timeout, end) &&
       pull8(ppReadPackedMsg, &value->number_of_earfcns, end)))
    return 0;

  if(value->number_of_earfcns <= NFAPI_MAX_CARRIER_LIST) {
    for(idx = 0; idx < value->number_of_earfcns; ++idx) {
      if(!(pull16(ppReadPackedMsg, &value->earfcn[idx].earfcn, end) &&
           pull8(ppReadPackedMsg, &value->earfcn[idx].number_of_ro_dl, end)))
        return 0;

      if(value->earfcn[idx].number_of_ro_dl <= NFAPI_MAX_RO_DL) {
        uint8_t ro_dl_idx = 0;

        for(ro_dl_idx = 0; ro_dl_idx < value->earfcn[idx].number_of_ro_dl; ++ro_dl_idx) {
          if(!pull8(ppReadPackedMsg, &value->earfcn[idx].ro_dl[ro_dl_idx], end))
            return 0;
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "More ROdl's than we can decode %d \n", value->earfcn[idx].number_of_ro_dl);
        return 0;
      }
    }
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More EARFCN's than we can decode %d \n", value->number_of_earfcns);
    return 0;
  }

  return 1;
}

static uint8_t unpack_rssi_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_rssi_request_t *pNfapiMsg = (nfapi_rssi_request_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_LTE_RSSI_REQUEST_TAG, &pNfapiMsg->lte_rssi_request, &unpack_lte_rssi_request_value},
    { NFAPI_UTRAN_RSSI_REQUEST_TAG, &pNfapiMsg->utran_rssi_request, &unpack_utran_rssi_request_value},
    { NFAPI_GERAN_RSSI_REQUEST_TAG, &pNfapiMsg->geran_rssi_request, &unpack_geran_rssi_request_value},
    { NFAPI_NB_IOT_RSSI_REQUEST_TAG, &pNfapiMsg->nb_iot_rssi_request, &unpack_nb_iot_rssi_request_value},
  };
  int result = 0;
  result = (pull8(ppReadPackedMsg, &pNfapiMsg->rat_type, end) &&
            unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension)));

  // Verify that the rat type and the tlv match
  if(result == 1 &&
      !((pNfapiMsg->rat_type == NFAPI_RAT_TYPE_LTE && pNfapiMsg->lte_rssi_request.tl.tag == NFAPI_LTE_RSSI_REQUEST_TAG) ||
        (pNfapiMsg->rat_type == NFAPI_RAT_TYPE_UTRAN && pNfapiMsg->utran_rssi_request.tl.tag == NFAPI_UTRAN_RSSI_REQUEST_TAG) ||
        (pNfapiMsg->rat_type == NFAPI_RAT_TYPE_GERAN && pNfapiMsg->geran_rssi_request.tl.tag == NFAPI_GERAN_RSSI_REQUEST_TAG) ||
        (pNfapiMsg->rat_type == NFAPI_RAT_TYPE_NB_IOT && pNfapiMsg->nb_iot_rssi_request.tl.tag == NFAPI_NB_IOT_RSSI_REQUEST_TAG))) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Mismatch RAT Type: %d and TAG value: 0x%04x \n", pNfapiMsg->rat_type, pNfapiMsg->lte_rssi_request.tl.tag);
    result = 0;
  }

  return result;
}

static uint8_t unpack_rssi_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_rssi_response_t *pNfapiMsg = (nfapi_rssi_response_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension)));
}

static uint8_t unpack_rssi_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_rssi_indication_body_t *value = (nfapi_rssi_indication_body_t *)tlv;

  if(pull16(ppReadPackedMsg, &value->number_of_rssi, end) == 0)
    return 0;

  if(value->number_of_rssi <= NFAPI_MAX_RSSI) {
    if(pullarrays16(ppReadPackedMsg, value->rssi, NFAPI_MAX_RSSI, value->number_of_rssi, end) == 0)
      return 0;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More RSSI's than we can decode %d \n", value->number_of_rssi);
    return 0;
  }

  return 1;
}

static uint8_t unpack_rssi_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_rssi_indication_t *pNfapiMsg = (nfapi_rssi_indication_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_RSSI_INDICATION_TAG, &pNfapiMsg->rssi_indication_body, &unpack_rssi_indication_value},
  };
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_lte_cell_search_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lte_cell_search_request_t *value = (nfapi_lte_cell_search_request_t *)tlv;

  if(!(pull16(ppReadPackedMsg, &value->earfcn, end) &&
       pull8(ppReadPackedMsg, &value->measurement_bandwidth, end) &&
       pull8(ppReadPackedMsg, &value->exhaustive_search, end) &&
       pull32(ppReadPackedMsg, &value->timeout, end) &&
       pull8(ppReadPackedMsg, &value->number_of_pci, end)))
    return 0;

  if(value->number_of_pci <= NFAPI_MAX_PCI_LIST) {
    if(pullarray16(ppReadPackedMsg, value->pci, NFAPI_MAX_PCI_LIST, value->number_of_pci, end) == 0)
      return 0;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More PCI's than we can decode %d \n", value->number_of_pci);
    return 0;
  }

  return 1;
}

static uint8_t unpack_utran_cell_search_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_utran_cell_search_request_t *value = (nfapi_utran_cell_search_request_t *)tlv;

  if(!(pull16(ppReadPackedMsg, &value->uarfcn, end) &&
       pull8(ppReadPackedMsg, &value->exhaustive_search, end) &&
       pull32(ppReadPackedMsg, &value->timeout, end) &&
       pull8(ppReadPackedMsg, &value->number_of_psc, end)))
    return 0;

  if(value->number_of_psc <= NFAPI_MAX_PSC_LIST) {
    if(pullarray16(ppReadPackedMsg, value->psc, NFAPI_MAX_PSC_LIST, value->number_of_psc, end) == 0)
      return 0;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More PSC's than we can decode %d \n", value->number_of_psc);
    return 0;
  }

  return 1;
}

static uint8_t unpack_geran_cell_search_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_geran_cell_search_request_t *value = (nfapi_geran_cell_search_request_t *)tlv;

  if(!(pull32(ppReadPackedMsg, &value->timeout, end) &&
       pull8(ppReadPackedMsg, &value->number_of_arfcn, end)))
    return 0;

  if(value->number_of_arfcn <= NFAPI_MAX_ARFCN_LIST) {
    if(pullarray16(ppReadPackedMsg, value->arfcn, NFAPI_MAX_ARFCN_LIST, value->number_of_arfcn, end) == 0)
      return 0;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More ARFCN's than we can decode %d \n", value->number_of_arfcn);
    return 0;
  }

  return 1;
}

static uint8_t unpack_nb_iot_cell_search_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nb_iot_cell_search_request_t *value = (nfapi_nb_iot_cell_search_request_t *)tlv;

  if(!(pull16(ppReadPackedMsg, &value->earfcn, end) &&
       pull8(ppReadPackedMsg, &value->ro_dl, end) &&
       pull8(ppReadPackedMsg, &value->exhaustive_search, end) &&
       pull32(ppReadPackedMsg, &value->timeout, end) &&
       pull8(ppReadPackedMsg, &value->number_of_pci, end)))
    return 0;

  if(value->number_of_pci <= NFAPI_MAX_PCI_LIST) {
    if(pullarray16(ppReadPackedMsg, value->pci, NFAPI_MAX_PCI_LIST, value->number_of_pci, end) == 0)
      return 0;
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More PCI's than we can decode %d \n", value->number_of_pci);
    return 0;
  }

  return 1;
}

static uint8_t unpack_cell_search_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_cell_search_request_t *pNfapiMsg = (nfapi_cell_search_request_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_LTE_CELL_SEARCH_REQUEST_TAG, &pNfapiMsg->lte_cell_search_request, &unpack_lte_cell_search_request_value},
    { NFAPI_UTRAN_CELL_SEARCH_REQUEST_TAG, &pNfapiMsg->utran_cell_search_request, &unpack_utran_cell_search_request_value},
    { NFAPI_GERAN_CELL_SEARCH_REQUEST_TAG, &pNfapiMsg->geran_cell_search_request, &unpack_geran_cell_search_request_value},
    { NFAPI_NB_IOT_CELL_SEARCH_REQUEST_TAG, &pNfapiMsg->nb_iot_cell_search_request, &unpack_nb_iot_cell_search_request_value},
  };
  int result = (pull8(ppReadPackedMsg, &pNfapiMsg->rat_type, end) &&
                unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));

  // Verify that the rat type and the tlv match
  if(result == 1 &&
      !((pNfapiMsg->rat_type == NFAPI_RAT_TYPE_LTE && pNfapiMsg->lte_cell_search_request.tl.tag == NFAPI_LTE_CELL_SEARCH_REQUEST_TAG) ||
        (pNfapiMsg->rat_type == NFAPI_RAT_TYPE_UTRAN && pNfapiMsg->utran_cell_search_request.tl.tag == NFAPI_UTRAN_CELL_SEARCH_REQUEST_TAG) ||
        (pNfapiMsg->rat_type == NFAPI_RAT_TYPE_GERAN && pNfapiMsg->geran_cell_search_request.tl.tag == NFAPI_GERAN_CELL_SEARCH_REQUEST_TAG) ||
        (pNfapiMsg->rat_type == NFAPI_RAT_TYPE_NB_IOT && pNfapiMsg->nb_iot_cell_search_request.tl.tag == NFAPI_NB_IOT_CELL_SEARCH_REQUEST_TAG))) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Mismatch RAT Type: %d and TAG value: 0x%04x \n", pNfapiMsg->rat_type, pNfapiMsg->lte_cell_search_request.tl.tag);
    result = 0;
  }

  return result;
}

static uint8_t unpack_cell_search_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_cell_search_response_t *pNfapiMsg = (nfapi_cell_search_response_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_lte_cell_search_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lte_cell_search_indication_t *value = (nfapi_lte_cell_search_indication_t *)tlv;
  uint16_t idx = 0;

  if(pull16(ppReadPackedMsg, &value->number_of_lte_cells_found, end) == 0)
    return 0;

  if(value->number_of_lte_cells_found <= NFAPI_MAX_LTE_CELLS_FOUND) {
    for(idx = 0; idx < value->number_of_lte_cells_found; ++idx) {
      if(!(pull16(ppReadPackedMsg, &value->lte_found_cells[idx].pci, end) &&
           pull8(ppReadPackedMsg, &value->lte_found_cells[idx].rsrp, end) &&
           pull8(ppReadPackedMsg, &value->lte_found_cells[idx].rsrq, end) &&
           pulls16(ppReadPackedMsg, &value->lte_found_cells[idx].frequency_offset, end)))
        return 0;
    }
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More found LTE cells than we can decode %d \n", value->number_of_lte_cells_found);
    return 0;
  }

  return 1;
}

static uint8_t unpack_utran_cell_search_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_utran_cell_search_indication_t *value = (nfapi_utran_cell_search_indication_t *)tlv;
  uint16_t idx = 0;

  if(pull16(ppReadPackedMsg, &value->number_of_utran_cells_found, end) == 0)
    return 0;

  if(value->number_of_utran_cells_found <= NFAPI_MAX_UTRAN_CELLS_FOUND) {
    for(idx = 0; idx < value->number_of_utran_cells_found; ++idx) {
      if(!(pull16(ppReadPackedMsg, &value->utran_found_cells[idx].psc, end) &&
           pull8(ppReadPackedMsg, &value->utran_found_cells[idx].rscp, end) &&
           pull8(ppReadPackedMsg, &value->utran_found_cells[idx].ecno, end) &&
           pulls16(ppReadPackedMsg, &value->utran_found_cells[idx].frequency_offset, end)))
        return 0;
    }
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More found UTRAN cells than we can decode %d \n", value->number_of_utran_cells_found);
    return 0;
  }

  return 1;
}

static uint8_t unpack_geran_cell_search_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_geran_cell_search_indication_t *value = (nfapi_geran_cell_search_indication_t *)tlv;
  uint16_t idx = 0;

  if(pull16(ppReadPackedMsg, &value->number_of_gsm_cells_found, end) == 0)
    return 0;

  if(value->number_of_gsm_cells_found <= NFAPI_MAX_GSM_CELLS_FOUND) {
    for(idx = 0; idx < value->number_of_gsm_cells_found; ++idx) {
      if(!(pull16(ppReadPackedMsg, &value->gsm_found_cells[idx].arfcn, end) &&
           pull8(ppReadPackedMsg, &value->gsm_found_cells[idx].bsic, end) &&
           pull8(ppReadPackedMsg, &value->gsm_found_cells[idx].rxlev, end) &&
           pull8(ppReadPackedMsg, &value->gsm_found_cells[idx].rxqual, end) &&
           pulls16(ppReadPackedMsg, &value->gsm_found_cells[idx].frequency_offset, end) &&
           pull32(ppReadPackedMsg, &value->gsm_found_cells[idx].sfn_offset, end)))
        return 0;
    }
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More found GSM cells than we can decode %d \n", value->number_of_gsm_cells_found);
    return 0;
  }

  return 1;
}

static uint8_t unpack_nb_iot_cell_search_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nb_iot_cell_search_indication_t *value = (nfapi_nb_iot_cell_search_indication_t *)tlv;
  uint16_t idx = 0;

  if(pull16(ppReadPackedMsg, &value->number_of_nb_iot_cells_found, end) == 0)
    return 0;

  if(value->number_of_nb_iot_cells_found <= NFAPI_MAX_NB_IOT_CELLS_FOUND) {
    for(idx = 0; idx < value->number_of_nb_iot_cells_found; ++idx) {
      if(!(pull16(ppReadPackedMsg, &value->nb_iot_found_cells[idx].pci, end) &&
           pull8(ppReadPackedMsg, &value->nb_iot_found_cells[idx].rsrp, end) &&
           pull8(ppReadPackedMsg, &value->nb_iot_found_cells[idx].rsrq, end) &&
           pulls16(ppReadPackedMsg, &value->nb_iot_found_cells[idx].frequency_offset, end)))
        return 0;
    }
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More found NB_IOT cells than we can decode %d \n", value->number_of_nb_iot_cells_found);
    return 0;
  }

  return 1;
}

static uint8_t unpack_cell_search_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_cell_search_indication_t *pNfapiMsg = (nfapi_cell_search_indication_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_LTE_CELL_SEARCH_INDICATION_TAG, &pNfapiMsg->lte_cell_search_indication, &unpack_lte_cell_search_indication_value},
    { NFAPI_UTRAN_CELL_SEARCH_INDICATION_TAG, &pNfapiMsg->utran_cell_search_indication, &unpack_utran_cell_search_indication_value},
    { NFAPI_GERAN_CELL_SEARCH_INDICATION_TAG, &pNfapiMsg->geran_cell_search_indication, &unpack_geran_cell_search_indication_value},
    { NFAPI_PNF_CELL_SEARCH_STATE_TAG, &pNfapiMsg->pnf_cell_search_state, &unpack_opaque_data_value},
    { NFAPI_NB_IOT_CELL_SEARCH_INDICATION_TAG, &pNfapiMsg->nb_iot_cell_search_indication, &unpack_nb_iot_cell_search_indication_value},
  };
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_lte_broadcast_detect_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lte_broadcast_detect_request_t *value = (nfapi_lte_broadcast_detect_request_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->earfcn, end) &&
          pull16(ppReadPackedMsg, &value->pci, end) &&
          pull32(ppReadPackedMsg, &value->timeout, end));
}

static uint8_t unpack_utran_broadcast_detect_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_utran_broadcast_detect_request_t *value = (nfapi_utran_broadcast_detect_request_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->uarfcn, end) &&
          pull16(ppReadPackedMsg, &value->psc, end) &&
          pull32(ppReadPackedMsg, &value->timeout, end));
}

static uint8_t unpack_nb_iot_broadcast_detect_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nb_iot_broadcast_detect_request_t *value = (nfapi_nb_iot_broadcast_detect_request_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->earfcn, end) &&
          pull8(ppReadPackedMsg, &value->ro_dl, end) &&
          pull16(ppReadPackedMsg, &value->pci, end) &&
          pull32(ppReadPackedMsg, &value->timeout, end));
}

static uint8_t unpack_broadcast_detect_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_broadcast_detect_request_t *pNfapiMsg = (nfapi_broadcast_detect_request_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_LTE_BROADCAST_DETECT_REQUEST_TAG, &pNfapiMsg->lte_broadcast_detect_request, &unpack_lte_broadcast_detect_request_value},
    { NFAPI_UTRAN_BROADCAST_DETECT_REQUEST_TAG, &pNfapiMsg->utran_broadcast_detect_request, &unpack_utran_broadcast_detect_request_value},
    { NFAPI_PNF_CELL_SEARCH_STATE_TAG, &pNfapiMsg->pnf_cell_search_state, &unpack_opaque_data_value},
    { NFAPI_NB_IOT_BROADCAST_DETECT_REQUEST_TAG, &pNfapiMsg->nb_iot_broadcast_detect_request, &unpack_nb_iot_broadcast_detect_request_value}
  };
  return (pull8(ppReadPackedMsg, &pNfapiMsg->rat_type, end) &&
          unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_broadcast_detect_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_broadcast_detect_response_t *pNfapiMsg = (nfapi_broadcast_detect_response_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_lte_broadcast_detect_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lte_broadcast_detect_indication_t *value = (nfapi_lte_broadcast_detect_indication_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->number_of_tx_antenna, end) &&
       pull16(ppReadPackedMsg, &value->mib_length, end)))
    return 0;

  if(value->mib_length > NFAPI_MAX_MIB_LENGTH) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "MIB too long %d \n", value->mib_length);
    return 0;
  }

  return (pullarray8(ppReadPackedMsg, value->mib, NFAPI_MAX_MIB_LENGTH, value->mib_length, end) &&
          pull32(ppReadPackedMsg, &value->sfn_offset, end));
}

static uint8_t unpack_utran_broadcast_detect_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_utran_broadcast_detect_indication_t *value = (nfapi_utran_broadcast_detect_indication_t *)tlv;

  if(pull16(ppReadPackedMsg, &value->mib_length, end) == 0)
    return 0;

  if(value->mib_length > NFAPI_MAX_MIB_LENGTH) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "MIB too long %d \n", value->mib_length);
    return 0;
  }

  return (pullarray8(ppReadPackedMsg, value->mib, NFAPI_MAX_MIB_LENGTH, value->mib_length, end) &&
          pull32(ppReadPackedMsg, &value->sfn_offset, end));
}

static uint8_t unpack_nb_iot_broadcast_detect_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nb_iot_broadcast_detect_indication_t *value = (nfapi_nb_iot_broadcast_detect_indication_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->number_of_tx_antenna, end) &&
       pull16(ppReadPackedMsg, &value->mib_length, end)))
    return 0;

  if(value->mib_length > NFAPI_MAX_MIB_LENGTH) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "MIB too long %d \n", value->mib_length);
    return 0;
  }

  return (pullarray8(ppReadPackedMsg, value->mib, NFAPI_MAX_MIB_LENGTH, value->mib_length, end) &&
          pull32(ppReadPackedMsg, &value->sfn_offset, end));
}

static uint8_t unpack_broadcast_detect_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_broadcast_detect_indication_t *pNfapiMsg = (nfapi_broadcast_detect_indication_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_LTE_BROADCAST_DETECT_INDICATION_TAG, &pNfapiMsg->lte_broadcast_detect_indication, &unpack_lte_broadcast_detect_indication_value},
    { NFAPI_UTRAN_BROADCAST_DETECT_INDICATION_TAG, &pNfapiMsg->utran_broadcast_detect_indication, &unpack_utran_broadcast_detect_indication_value},
    { NFAPI_NB_IOT_BROADCAST_DETECT_INDICATION_TAG, &pNfapiMsg->nb_iot_broadcast_detect_indication, &unpack_nb_iot_broadcast_detect_indication_value},
    { NFAPI_PNF_CELL_BROADCAST_STATE_TAG, &pNfapiMsg->pnf_cell_broadcast_state, &unpack_opaque_data_value}
  };
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_lte_system_information_schedule_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lte_system_information_schedule_request_t *value = (nfapi_lte_system_information_schedule_request_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->earfcn, end) &&
          pull16(ppReadPackedMsg, &value->pci, end) &&
          pull16(ppReadPackedMsg, &value->downlink_channel_bandwidth, end) &&
          pull8(ppReadPackedMsg, &value->phich_configuration, end) &&
          pull8(ppReadPackedMsg, &value->number_of_tx_antenna, end) &&
          pull8(ppReadPackedMsg, &value->retry_count, end) &&
          pull32(ppReadPackedMsg, &value->timeout, end));
}

static uint8_t unpack_nb_iot_system_information_schedule_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nb_iot_system_information_schedule_request_t *value = (nfapi_nb_iot_system_information_schedule_request_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->earfcn, end) &&
          pull8(ppReadPackedMsg, &value->ro_dl, end) &&
          pull16(ppReadPackedMsg, &value->pci, end) &&
          pull8(ppReadPackedMsg, &value->scheduling_info_sib1_nb, end) &&
          pull32(ppReadPackedMsg, &value->timeout, end));
}

static uint8_t unpack_system_information_schedule_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_schedule_request_t *pNfapiMsg = (nfapi_system_information_schedule_request_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_LTE_SYSTEM_INFORMATION_SCHEDULE_REQUEST_TAG, &pNfapiMsg->lte_system_information_schedule_request, &unpack_lte_system_information_schedule_request_value},
    { NFAPI_NB_IOT_SYSTEM_INFORMATION_SCHEDULE_REQUEST_TAG, &pNfapiMsg->nb_iot_system_information_schedule_request, &unpack_nb_iot_system_information_schedule_request_value},
    { NFAPI_PNF_CELL_BROADCAST_STATE_TAG, &pNfapiMsg->pnf_cell_broadcast_state, &unpack_opaque_data_value}
  };
  return (pull8(ppReadPackedMsg, &pNfapiMsg->rat_type, end) &&
          unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_system_information_schedule_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_schedule_response_t *pNfapiMsg = (nfapi_system_information_schedule_response_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_lte_system_information_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lte_system_information_indication_t *value = (nfapi_lte_system_information_indication_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->sib_type, end) &&
       pull16(ppReadPackedMsg, &value->sib_length, end)))
    return 0;

  if(value->sib_length > NFAPI_MAX_SIB_LENGTH) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "SIB too long %d \n", value->sib_length);
    return 0;
  }

  if(pullarray8(ppReadPackedMsg, value->sib, NFAPI_MAX_SIB_LENGTH,  value->sib_length, end) == 0)
    return 0;

  return 1;
}

static uint8_t unpack_nb_iot_system_information_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nb_iot_system_information_indication_t *value = (nfapi_nb_iot_system_information_indication_t *)tlv;

  if(!(pull8(ppReadPackedMsg, &value->sib_type, end) &&
       pull16(ppReadPackedMsg, &value->sib_length, end)))
    return 0;

  if(value->sib_length > NFAPI_MAX_SIB_LENGTH) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "SIB too long %d \n", value->sib_length);
    return 0;
  }

  if(pullarray8(ppReadPackedMsg, value->sib, NFAPI_MAX_SIB_LENGTH,  value->sib_length, end) == 0)
    return 0;

  return 1;
}

static uint8_t unpack_system_information_schedule_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_schedule_indication_t *pNfapiMsg = (nfapi_system_information_schedule_indication_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->lte_system_information_indication, &unpack_lte_system_information_indication_value},
    { NFAPI_NB_IOT_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->nb_iot_system_information_indication, &unpack_nb_iot_system_information_indication_value},
  };
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_lte_system_information_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_lte_system_information_request_t *value = (nfapi_lte_system_information_request_t *)tlv;
  uint16_t idx = 0;

  if(!(pull16(ppReadPackedMsg, &value->earfcn, end) &&
       pull16(ppReadPackedMsg, &value->pci, end) &&
       pull16(ppReadPackedMsg, &value->downlink_channel_bandwidth, end) &&
       pull8(ppReadPackedMsg, &value->phich_configuration, end) &&
       pull8(ppReadPackedMsg, &value->number_of_tx_antenna, end) &&
       pull8(ppReadPackedMsg, &value->number_of_si_periodicity, end)))
    return 0;

  if(value->number_of_si_periodicity > NFAPI_MAX_SI_PERIODICITY) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More found SI periodicity than we can decode %d \n", value->number_of_si_periodicity);
    return 0;
  }

  for(idx = 0; idx < value->number_of_si_periodicity; ++idx) {
    if(!(pull8(ppReadPackedMsg, &value->si_periodicity[idx].si_periodicity, end) &&
         pull8(ppReadPackedMsg, &value->si_periodicity[idx].si_index, end)))
      return 0;
  }

  if(!(pull8(ppReadPackedMsg, &value->si_window_length, end) &&
       pull32(ppReadPackedMsg, &value->timeout, end)))
    return 0;

  return 1;
}

static uint8_t unpack_utran_system_information_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_utran_system_information_request_t *value = (nfapi_utran_system_information_request_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->uarfcn, end) &&
          pull16(ppReadPackedMsg, &value->psc, end) &&
          pull32(ppReadPackedMsg, &value->timeout, end));
}

static uint8_t unpack_geran_system_information_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_geran_system_information_request_t *value = (nfapi_geran_system_information_request_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->arfcn, end) &&
          pull8(ppReadPackedMsg, &value->bsic, end) &&
          pull32(ppReadPackedMsg, &value->timeout, end));
}

static uint8_t unpack_nb_iot_system_information_request_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nb_iot_system_information_request_t *value = (nfapi_nb_iot_system_information_request_t *)tlv;
  uint16_t idx = 0;

  if(!(pull16(ppReadPackedMsg, &value->earfcn, end) &&
       pull8(ppReadPackedMsg, &value->ro_dl, end) &&
       pull16(ppReadPackedMsg, &value->pci, end) &&
       pull8(ppReadPackedMsg, &value->number_of_si_periodicity, end)))
    return 0;

  if(value->number_of_si_periodicity > NFAPI_MAX_SI_PERIODICITY) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "More found SI periodicity than we can decode %d \n", value->number_of_si_periodicity);
    return 0;
  }

  for(idx = 0; idx < value->number_of_si_periodicity; ++idx) {
    if(!(pull8(ppReadPackedMsg, &value->si_periodicity[idx].si_periodicity, end) &&
         pull8(ppReadPackedMsg, &value->si_periodicity[idx].si_repetition_pattern, end) &&
         pull8(ppReadPackedMsg, &value->si_periodicity[idx].si_tb_size, end) &&
         pull8(ppReadPackedMsg, &value->si_periodicity[idx].number_of_si_index, end)))
      return 0;

    uint8_t si_idx;

    for(si_idx = 0; si_idx < value->si_periodicity[idx].number_of_si_index; ++si_idx) {
      if(!(pull8(ppReadPackedMsg, &value->si_periodicity[idx].si_index[si_idx], end)))
        return 0;
    }
  }

  if(!(pull8(ppReadPackedMsg, &value->si_window_length, end) &&
       pull32(ppReadPackedMsg, &value->timeout, end)))
    return 0;

  return 1;
}

static uint8_t unpack_system_information_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_request_t *pNfapiMsg = (nfapi_system_information_request_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_LTE_SYSTEM_INFORMATION_REQUEST_TAG, &pNfapiMsg->lte_system_information_request, &unpack_lte_system_information_request_value},
    { NFAPI_UTRAN_SYSTEM_INFORMATION_REQUEST_TAG, &pNfapiMsg->utran_system_information_request, &unpack_utran_system_information_request_value},
    { NFAPI_GERAN_SYSTEM_INFORMATION_REQUEST_TAG, &pNfapiMsg->geran_system_information_request, &unpack_geran_system_information_request_value},
    { NFAPI_NB_IOT_SYSTEM_INFORMATION_REQUEST_TAG, &pNfapiMsg->nb_iot_system_information_request, &unpack_nb_iot_system_information_request_value},
    { NFAPI_PNF_CELL_BROADCAST_STATE_TAG, &pNfapiMsg->pnf_cell_broadcast_state, &unpack_opaque_data_value}
  };
  int result = (pull8(ppReadPackedMsg, &pNfapiMsg->rat_type, end) &&
                unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));

  // Verify that the rat type and the tlv match
  if(result == 1 &&
      !((pNfapiMsg->rat_type == NFAPI_RAT_TYPE_LTE && pNfapiMsg->lte_system_information_request.tl.tag == NFAPI_LTE_SYSTEM_INFORMATION_REQUEST_TAG) ||
        (pNfapiMsg->rat_type == NFAPI_RAT_TYPE_UTRAN && pNfapiMsg->utran_system_information_request.tl.tag == NFAPI_UTRAN_SYSTEM_INFORMATION_REQUEST_TAG) ||
        (pNfapiMsg->rat_type == NFAPI_RAT_TYPE_GERAN && pNfapiMsg->geran_system_information_request.tl.tag == NFAPI_GERAN_SYSTEM_INFORMATION_REQUEST_TAG))) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Mismatch RAT Type: %d and TAG value: 0x%04x \n", pNfapiMsg->rat_type, pNfapiMsg->lte_system_information_request.tl.tag);
    result = 0;
  }

  return result;
}

static uint8_t unpack_system_information_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_response_t *pNfapiMsg = (nfapi_system_information_response_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_utran_system_information_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_utran_system_information_indication_t *value = (nfapi_utran_system_information_indication_t *)tlv;

  if(pull16(ppReadPackedMsg, &value->sib_length, end) == 0)
    return 0;

  if(value->sib_length > NFAPI_MAX_SIB_LENGTH) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "SIB too long %d \n", value->sib_length);
    return 0;
  }

  if(pullarray8(ppReadPackedMsg, value->sib, NFAPI_MAX_SIB_LENGTH, value->sib_length, end) == 0)
    return 0;

  return 1;
}

static uint8_t unpack_geran_system_information_indication_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_geran_system_information_indication_t *value = (nfapi_geran_system_information_indication_t *)tlv;

  if(pull16(ppReadPackedMsg, &value->si_length, end) == 0)
    return 0;

  if(value->si_length > NFAPI_MAX_SI_LENGTH) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "SIB too long %d \n", value->si_length);
    return 0;
  }

  if(pullarray8(ppReadPackedMsg, value->si, NFAPI_MAX_SI_LENGTH, value->si_length, end) == 0)
    return 0;

  return 1;
}

static uint8_t unpack_system_information_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_system_information_indication_t *pNfapiMsg = (nfapi_system_information_indication_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->lte_system_information_indication, &unpack_lte_system_information_indication_value},
    { NFAPI_UTRAN_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->utran_system_information_indication, &unpack_utran_system_information_indication_value},
    { NFAPI_GERAN_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->geran_system_information_indication, &unpack_geran_system_information_indication_value},
    { NFAPI_NB_IOT_SYSTEM_INFORMATION_INDICATION_TAG, &pNfapiMsg->nb_iot_system_information_indication, &unpack_nb_iot_system_information_indication_value},
  };
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_nmm_stop_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_nmm_stop_request_t *pNfapiMsg = (nfapi_nmm_stop_request_t *)msg;
  return unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension);
}

static uint8_t unpack_nmm_stop_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_nmm_stop_response_t *pNfapiMsg = (nfapi_nmm_stop_response_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
          unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}


static int check_unpack_length(nfapi_message_id_e msgId, uint32_t unpackedBufLen) {
  int retLen = 0;

  switch (msgId) {
    case NFAPI_RSSI_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_rssi_request_t))
        retLen = sizeof(nfapi_rssi_request_t);

      break;

    case NFAPI_RSSI_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_rssi_response_t))
        retLen = sizeof(nfapi_rssi_response_t);

      break;

    case NFAPI_RSSI_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_rssi_indication_t))
        retLen = sizeof(nfapi_rssi_indication_t);

      break;

    case NFAPI_CELL_SEARCH_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_cell_search_request_t))
        retLen = sizeof(nfapi_cell_search_request_t);

      break;

    case NFAPI_CELL_SEARCH_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_cell_search_response_t))
        retLen = sizeof(nfapi_cell_search_response_t);

      break;

    case NFAPI_CELL_SEARCH_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_cell_search_indication_t))
        retLen = sizeof(nfapi_cell_search_indication_t);

      break;

    case NFAPI_BROADCAST_DETECT_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_broadcast_detect_request_t))
        retLen = sizeof(nfapi_broadcast_detect_request_t);

      break;

    case NFAPI_BROADCAST_DETECT_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_broadcast_detect_response_t))
        retLen = sizeof(nfapi_broadcast_detect_response_t);

      break;

    case NFAPI_BROADCAST_DETECT_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_broadcast_detect_indication_t))
        retLen = sizeof(nfapi_broadcast_detect_indication_t);

      break;

    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_system_information_schedule_request_t))
        retLen = sizeof(nfapi_system_information_schedule_request_t);

      break;

    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_system_information_schedule_response_t))
        retLen = sizeof(nfapi_system_information_schedule_response_t);

      break;

    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_system_information_schedule_indication_t))
        retLen = sizeof(nfapi_system_information_schedule_indication_t);

      break;

    case NFAPI_SYSTEM_INFORMATION_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_system_information_request_t))
        retLen = sizeof(nfapi_system_information_request_t);

      break;

    case NFAPI_SYSTEM_INFORMATION_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_system_information_response_t))
        retLen = sizeof(nfapi_system_information_response_t);

      break;

    case NFAPI_SYSTEM_INFORMATION_INDICATION:
      if (unpackedBufLen >= sizeof(nfapi_system_information_indication_t))
        retLen = sizeof(nfapi_system_information_indication_t);

      break;

    case NFAPI_NMM_STOP_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_nmm_stop_request_t))
        retLen = sizeof(nfapi_nmm_stop_request_t);

      break;

    case NFAPI_NMM_STOP_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_nmm_stop_response_t))
        retLen = sizeof(nfapi_nmm_stop_response_t);

      break;

    default:
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "Unknown message ID %d\n", msgId);
      break;
  }

  return retLen;
}

int nfapi_p4_message_pack(void *pMessageBuf, uint32_t messageBufLen, void *pPackedBuf, uint32_t packedBufLen, nfapi_p4_p5_codec_config_t *config) {
  nfapi_p4_p5_message_header_t *pMessageHeader = pMessageBuf;
  uint32_t packedMsgLen;
  uint16_t packedMsgLen16;

  if (pMessageBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P4 Pack supplied message buffer pointer is null\n");
    return -1;
  }

  if (packedBufLen == 0 || pPackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P4 Pack supplied packed buffer pointer is null or packed buffer length is 0\n");
    return -1;
  }

  uint8_t *end = pPackedBuf + packedBufLen;
  uint8_t *pWritePackedMessage = pPackedBuf;
  uint8_t *pPackedLengthField = &pWritePackedMessage[4];

  // process the header
  if(!(push16(pMessageHeader->phy_id, &pWritePackedMessage, end) &&
       push16(pMessageHeader->message_id, &pWritePackedMessage, end) &&
       push16(0/*pMessageHeader->message_length*/, &pWritePackedMessage, end) &&
       push16(pMessageHeader->spare, &pWritePackedMessage, end))) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to pack p4 message header\n");
    return -1;
  }

  // look for the specific message
  uint8_t result = 0;

  switch (pMessageHeader->message_id) {
    case NFAPI_RSSI_REQUEST:
      result = pack_rssi_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_RSSI_RESPONSE:
      result = pack_rssi_response(pMessageHeader, &pWritePackedMessage,  end,config);
      break;

    case NFAPI_RSSI_INDICATION:
      result = pack_rssi_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_CELL_SEARCH_REQUEST:
      result = pack_cell_search_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_CELL_SEARCH_RESPONSE:
      result = pack_cell_search_response(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_CELL_SEARCH_INDICATION:
      result = pack_cell_search_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_BROADCAST_DETECT_REQUEST:
      result = pack_broadcast_detect_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_BROADCAST_DETECT_RESPONSE:
      result = pack_broadcast_detect_response(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_BROADCAST_DETECT_INDICATION:
      result = pack_broadcast_detect_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST:
      result = pack_system_information_schedule_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE:
      result = pack_system_information_schedule_response(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION:
      result = pack_system_information_schedule_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_SYSTEM_INFORMATION_REQUEST:
      result = pack_system_information_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_SYSTEM_INFORMATION_RESPONSE:
      result = pack_system_information_response(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_SYSTEM_INFORMATION_INDICATION:
      result = pack_system_information_indication(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NMM_STOP_REQUEST:
      result = pack_nmm_stop_request(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    case NFAPI_NMM_STOP_RESPONSE:
      result = pack_nmm_stop_response(pMessageHeader, &pWritePackedMessage, end, config);
      break;

    default: {
      if(pMessageHeader->message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
          pMessageHeader->message_id <= NFAPI_VENDOR_EXT_MSG_MIN) {
        if(config && config->pack_p4_p5_vendor_extension) {
          result =(config->pack_p4_p5_vendor_extension)(pMessageHeader, &pWritePackedMessage, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s VE NFAPI message ID %d. No ve encoder provided\n", __FUNCTION__, pMessageHeader->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown message ID %d\n", __FUNCTION__, pMessageHeader->message_id);
      }
    }
    break;
  }

  // return the packed length
  if(result == 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Result is 0\n");
    return -1;
  }

  // check for a valid message length
  packedMsgLen = get_packed_msg_len((uintptr_t)pPackedBuf, (uintptr_t)pWritePackedMessage);

  if (packedMsgLen > 0xFFFF || packedMsgLen > packedBufLen) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Packed message length error %d, buffer supplied %d\n", packedMsgLen, packedBufLen);
    return -1;
  } else {
    packedMsgLen16 = (uint16_t)packedMsgLen;
  }

  // Update the message length in the header
  if(push16(packedMsgLen16, &pPackedLengthField, end) == 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to pack p4 message header lengt\n");
    return -1;
  }

  return (packedMsgLen);
}

// Main unpack functions - public

int nfapi_p4_message_header_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p4_p5_codec_config_t *config) {
  nfapi_p4_p5_message_header_t *pMessageHeader = pUnpackedBuf;
  uint8_t *pReadPackedMessage = pMessageBuf;

  if (pMessageBuf == NULL || pUnpackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P4 header unpack supplied pointers are null\n");
    return -1;
  }

  uint8_t *end = pMessageBuf + messageBufLen;

  if (messageBufLen < NFAPI_HEADER_LENGTH || unpackedBufLen < sizeof(nfapi_p4_p5_message_header_t)) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 header unpack supplied message buffer is too small %d, %d\n", messageBufLen, unpackedBufLen);
    return -1;
  }

  // process the headei
  if (pull16(&pReadPackedMessage, &pMessageHeader->phy_id, end) &&
      pull16(&pReadPackedMessage, &pMessageHeader->message_id, end) &&
      pull16(&pReadPackedMessage, &pMessageHeader->message_length, end) &&
      pull16(&pReadPackedMessage, &pMessageHeader->spare, end))
    return -1;

  return 0;
}

int nfapi_p4_message_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p4_p5_codec_config_t *config) {
  int result = 0;
  nfapi_p4_p5_message_header_t *pMessageHeader = pUnpackedBuf;
  uint8_t *pReadPackedMessage = pMessageBuf;

  if (pMessageBuf == NULL || pUnpackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P4 unpack supplied pointers are null\n");
    return -1;
  }

  uint8_t *end = pMessageBuf + messageBufLen;

  if (messageBufLen < NFAPI_HEADER_LENGTH || unpackedBufLen < sizeof(nfapi_p4_p5_message_header_t)) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P4 unpack supplied message buffer is too small %d, %d\n", messageBufLen, unpackedBufLen);
    return -1;
  }

  // clean the supplied buffer for - tag value blanking
  (void)memset(pUnpackedBuf, 0, unpackedBufLen);

  // process the header
  if(!(pull16(&pReadPackedMessage, &pMessageHeader->phy_id, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->message_id, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->message_length, end) &&
       pull16(&pReadPackedMessage, &pMessageHeader->spare, end)))
    return -1;

  // look for the specific message
  switch (pMessageHeader->message_id) {
    case NFAPI_RSSI_REQUEST:
      if (check_unpack_length(NFAPI_RSSI_REQUEST, unpackedBufLen))
        result = unpack_rssi_request(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_RSSI_RESPONSE:
      if (check_unpack_length(NFAPI_RSSI_RESPONSE, unpackedBufLen))
        result = unpack_rssi_response(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_RSSI_INDICATION:
      if (check_unpack_length(NFAPI_RSSI_INDICATION, unpackedBufLen))
        result = unpack_rssi_indication(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_CELL_SEARCH_REQUEST:
      if (check_unpack_length(NFAPI_CELL_SEARCH_REQUEST, unpackedBufLen))
        result = unpack_cell_search_request(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_CELL_SEARCH_RESPONSE:
      if (check_unpack_length(NFAPI_CELL_SEARCH_RESPONSE, unpackedBufLen))
        result = unpack_cell_search_response(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_CELL_SEARCH_INDICATION:
      if (check_unpack_length(NFAPI_CELL_SEARCH_INDICATION, unpackedBufLen))
        result = unpack_cell_search_indication(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_BROADCAST_DETECT_REQUEST:
      if (check_unpack_length(NFAPI_BROADCAST_DETECT_REQUEST, unpackedBufLen))
        result = unpack_broadcast_detect_request(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_BROADCAST_DETECT_RESPONSE:
      if (check_unpack_length(NFAPI_BROADCAST_DETECT_RESPONSE, unpackedBufLen))
        result = unpack_broadcast_detect_response(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_BROADCAST_DETECT_INDICATION:
      if (check_unpack_length(NFAPI_BROADCAST_DETECT_INDICATION, unpackedBufLen))
        result = unpack_broadcast_detect_indication(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST:
      if (check_unpack_length(NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST, unpackedBufLen))
        result = unpack_system_information_schedule_request(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE:
      if (check_unpack_length(NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE, unpackedBufLen))
        result = unpack_system_information_schedule_response(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION:
      if (check_unpack_length(NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION, unpackedBufLen))
        result = unpack_system_information_schedule_indication(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_SYSTEM_INFORMATION_REQUEST:
      if (check_unpack_length(NFAPI_SYSTEM_INFORMATION_REQUEST, unpackedBufLen))
        result = unpack_system_information_request(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_SYSTEM_INFORMATION_RESPONSE:
      if (check_unpack_length(NFAPI_SYSTEM_INFORMATION_RESPONSE, unpackedBufLen))
        result = unpack_system_information_response(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result =  -1;

      break;

    case NFAPI_SYSTEM_INFORMATION_INDICATION:
      if (check_unpack_length(NFAPI_SYSTEM_INFORMATION_INDICATION, unpackedBufLen))
        result = unpack_system_information_indication(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result =  -1;

      break;

    case NFAPI_NMM_STOP_REQUEST:
      if (check_unpack_length(NFAPI_NMM_STOP_REQUEST, unpackedBufLen))
        result = unpack_nmm_stop_request(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    case NFAPI_NMM_STOP_RESPONSE:
      if (check_unpack_length(NFAPI_NMM_STOP_RESPONSE, unpackedBufLen))
        result = unpack_nmm_stop_response(&pReadPackedMessage, end, pMessageHeader, config);
      else
        result = -1;

      break;

    default:
      if(pMessageHeader->message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
          pMessageHeader->message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        if(config && config->unpack_p4_p5_vendor_extension) {
          result = (config->unpack_p4_p5_vendor_extension)(pMessageHeader, &pReadPackedMessage, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s VE NFAPI message ID %d. No ve decoder provided\n", __FUNCTION__, pMessageHeader->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown P4 message ID %d\n", __FUNCTION__, pMessageHeader->message_id);
      }

      break;
  }

  if(result == 0)
    return -1;

  return result;
}
