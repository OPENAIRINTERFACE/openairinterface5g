/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "executables/softmodem-common.h"
#include "common/utils/ocp_itti/intertask_interface.h"
#include "common/ran_context.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "openair2/GNB_APP/gnb_config_common.h"
#include "openair2/GNB_APP/gnb_config_ng.h"
#include "openair2/GNB_APP/gnb_paramdef.h"
#include "openair3/SCTP/sctp_default_values.h"
#include "openair3/NRPPA/nrppa_gNB_config.h"
#include "openair3/NRPPA/nrppa_gNB.h"
#include "openair3/NAS/NR_UE/nr_nas_msg.h"
#include "executables/nr-uesoftmodem.h"
#include "openair3/NRPPA/nrppa_gNB_location_information_transfer.c"
#include "openair3/NRPPA/nrppa_gNB_measurement_information_transfer.h"
#include "openair2/SDAP/nr_sdap/nr_sdap_qos_rule.h"

RAN_CONTEXT_t RC;
THREAD_STRUCT thread_struct;
uint64_t downlink_frequency[MAX_NUM_CCs][4];
int64_t uplink_frequency_offset[MAX_NUM_CCs][4];
int oai_exit = 0;
int gnb_id;
uint32_t tac = 1;
uint16_t mcc = 1;
uint16_t mnc = 1;
uint8_t mnc_len = 2;
int NB_UE_INST = 1;
positioning_config_t *positioning_config = NULL;

void exit_function(const char *file, const char *function, const int line, const char *s, const int assert)
{
  if (assert) {
    abort();
  } else {
    sleep(1); // allow other threads to exit first
    exit(EXIT_SUCCESS);
  }
}

nfapi_mode_t nfapi_mod = -1;
void nfapi_setmode(nfapi_mode_t nfapi_mode)
{
  nfapi_mod = nfapi_mode;
}
nfapi_mode_t nfapi_getmode(void)
{
  return nfapi_mod;
}

ngran_node_t get_node_type()
{
  return ngran_gNB_CUUP;
}

nrUE_params_t *get_nrUE_params(void)
{
  static nrUE_params_t params = {0};
  return &params;
}

void nr_sdap_qos_rule_add(ue_id_t ue_id,
                         int pdusession_id,
                         uint8_t rule_id,
                         uint8_t qfi,
                         uint8_t precedence,
                         bool is_default,
                         const packet_filter_decoded_t *pf_list,
                         int num_pf)
{
}

void nr_sdap_qos_rule_remove(ue_id_t ue_id, int pdusession_id, uint8_t rule_id)
{
}

void nr_sdap_qos_rule_update(ue_id_t ue_id,
                            int pdusession_id,
                            uint8_t rule_id,
                            uint8_t qfi,
                            uint8_t precedence,
                            bool is_default,
                            const packet_filter_decoded_t *pf_list,
                            int num_pf,
                            bool replace)
{
}

void nr_sdap_qos_rule_delete_pf(ue_id_t ue_id, int pdusession_id, uint8_t rule_id, const uint8_t *pf_ids, int num_ids)
{
}

configmodule_interface_t *uniqCfg = NULL;

static nrppa_trp_information_resp_t fill_trp_resp(uint8_t transaction_id)
{
  dump_positioning_config(positioning_config);
  nrppa_trp_information_resp_t resp = {0};
  resp.transaction_id = transaction_id;

  nrppa_trp_information_list_t *list = &resp.trp_information_list;
  uint32_t trp_info_item_len = positioning_config->num_trp;
  list->trp_information_item_length = trp_info_item_len;
  list->trp_information_item = calloc_or_fail(trp_info_item_len, sizeof(*list->trp_information_item));

  uint8_t resp_item_len = 3;
  for (int i = 0; i < trp_info_item_len; i++) {
    nrppa_trp_information_t *tRPInformation = &list->trp_information_item[i];
    tRPInformation->trp_id = positioning_config->trps[i].id;
    nrppa_trp_information_type_response_list_t *resp_list = &tRPInformation->trp_information_type_response_list;
    resp_list->trp_information_type_response_item_length = resp_item_len;
    nrppa_trp_information_type_response_item_t *resp_item = calloc_or_fail(resp_item_len, sizeof(*resp_item));
    resp_list->trp_information_type_response_item = resp_item;

    // nrPCI
    resp_item[0].present = NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_PCI_NR;
    resp_item[0].choice.pci_nr = 123 + i;

    // nG_RAN_CGI
    resp_item[1].present = NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_NG_RAN_CGI;
    resp_item[1].choice.ng_ran_cgi.plmn.mcc = 001;
    resp_item[1].choice.ng_ran_cgi.plmn.mnc = 01;
    resp_item[1].choice.ng_ran_cgi.plmn.mnc_digit_length = 2;
    resp_item[1].choice.ng_ran_cgi.nr_cellid = gnb_id << 8; // HACK: Made to work with lmf

    // geographicalCoordinates
    resp_item[2].present = NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_GEOGRAPHICALCOORDINATES;
    nrppa_geographical_coordinates_t *geographicalCoordinates = &resp_item[2].choice.geographical_coordinates;
    nrppa_trp_position_definition_type_t *tRPPositionDefinitionType = &geographicalCoordinates->trp_position_definition_type;
    // referenced
    tRPPositionDefinitionType->present = NRPPA_TRP_POSITION_DEFINITION_TYPE_PR_REFERENCED;
    nrppa_trp_position_referenced_t *referenced = &tRPPositionDefinitionType->choice.referenced;
    // coordinate ID
    referenced->reference_point.present = NRPPA_REFERENCE_POINT_PR_COORDINATEID;
    referenced->reference_point.choice.coordinate_id = 2;
    nrppa_trp_reference_point_type_t *referencePointType = &referenced->reference_point_type;
    // relative cartesian
    referencePointType->present = NRPPA_TRP_REFERENCE_POINT_TYPE_PR_TRPPOSITION_RELATIVE_CARTESIAN;
    nrppa_relative_cartesian_location_t *trp_pos_cart = &referencePointType->choice.trp_position_relative_cartesian;
    // 0 = millimeter
    trp_pos_cart->xyz_unit = positioning_config->trps[i].unit;

    // random reference cartesian coordinates in millimeter
    trp_pos_cart->xvalue = positioning_config->trps[i].x_axis;
    trp_pos_cart->yvalue = positioning_config->trps[i].y_axis;
    trp_pos_cart->zvalue = positioning_config->trps[i].z_axis;
    // testing random values for uncertainity and confidence
    trp_pos_cart->location_uncertainty.horizontal_uncertainty = i + 1;
    trp_pos_cart->location_uncertainty.horizontal_confidence = i + 2;
    trp_pos_cart->location_uncertainty.vertical_uncertainty = i + 3;
    trp_pos_cart->location_uncertainty.vertical_confidence = i + 4;
  }
  return resp;
}

static nrppa_positioning_information_resp_t fill_position_information_resp(uint16_t transaction_id)
{
  nrppa_positioning_information_resp_t resp = {0};
  resp.transaction_id = transaction_id;
  resp.srs_configuration = calloc_or_fail(1, sizeof(*resp.srs_configuration));
  nrppa_srs_carrier_list_t *srs_carrier_list = &resp.srs_configuration->srs_carrier_list;
  uint32_t srs_carrier_list_len = 2;
  srs_carrier_list->srs_carrier_list_length = srs_carrier_list_len;
  srs_carrier_list->srs_carrier_list_item = calloc_or_fail(srs_carrier_list_len, sizeof(*srs_carrier_list->srs_carrier_list_item));
  for (int i = 0; i < srs_carrier_list_len; i++) {
    nrppa_srs_carrier_list_item_t *srs_carrier_list_item = &srs_carrier_list->srs_carrier_list_item[i];
    // pointA
    srs_carrier_list_item->pointA = 87641 + i;

    // Uplink Channel BW-PerSCS-List
    nrppa_uplink_channel_bw_per_scs_list_t *uplink_channel_bw_per_scs_list = &srs_carrier_list_item->uplink_channel_bw_per_scs_list;

    uint32_t scs_specific_carrier_list_length = 3;
    uplink_channel_bw_per_scs_list->scs_specific_carrier_list_length = scs_specific_carrier_list_length;
    uplink_channel_bw_per_scs_list->scs_specific_carrier =
        calloc_or_fail(scs_specific_carrier_list_length, sizeof(*uplink_channel_bw_per_scs_list->scs_specific_carrier));
    for (int j = 0; j < scs_specific_carrier_list_length; j++) {
      nrppa_scs_specific_carrier_t *scs_specific_carrier = &uplink_channel_bw_per_scs_list->scs_specific_carrier[j];
      // offset to carrier
      scs_specific_carrier->offset_to_carrier = 100 + i + j;
      // subcarrier spacing
      scs_specific_carrier->subcarrier_spacing = NRPPA_SUBCARRIER_SPACING_30KHZ;
      // carrier bandwidth
      scs_specific_carrier->carrier_bandwidth = 106 + i + j;
    }

    // Active UL BWP
    nrppa_active_ul_bwp_t *active_ul_bwp = &srs_carrier_list_item->active_ul_bwp;

    // location and bandwidth
    active_ul_bwp->location_and_bandwidth = 2775 + i;
    // subcarrier spacing
    active_ul_bwp->subcarrier_spacing = NRPPA_SUBCARRIER_SPACING_30KHZ;
    // cyclic prefix
    active_ul_bwp->cyclic_prefix = NRPPA_CP_TYPE_NORMAL;
    // Tx Direct Current Location
    active_ul_bwp->tx_direct_current_location = 10 + i;

    // SRS Config
    nrppa_srs_config_t *sRSConfig = &active_ul_bwp->srs_config;

    // srs_resource_list
    sRSConfig->srs_resource_list = calloc_or_fail(1, sizeof(*sRSConfig->srs_resource_list));
    nrppa_srs_resource_list_t *srs_resource_list = sRSConfig->srs_resource_list;
    uint32_t srs_resource_list_length = 3;
    srs_resource_list->srs_resource_list_length = srs_resource_list_length;
    srs_resource_list->srs_resource = calloc_or_fail(srs_resource_list_length, sizeof(*srs_resource_list->srs_resource));

    // SRS resource config 1 : periodic
    nrppa_srs_resource_t *srs_resource = &srs_resource_list->srs_resource[0];
    srs_resource->srs_resource_id = 22;
    srs_resource->nr_of_srs_ports = NRPPA_SRS_NUMBER_OF_PORTS_N2;
    srs_resource->transmission_comb.present = NRPPA_TRANSMISSION_COMB_PR_N2;
    srs_resource->transmission_comb.choice.n2.comb_offset_n2 = 1;
    srs_resource->transmission_comb.choice.n2.cyclic_shift_n2 = 3;
    srs_resource->start_position = 8;
    srs_resource->nr_of_symbols = NRPPA_SRS_NUMBER_OF_SYMBOLS_N4;
    srs_resource->repetition_factor = NRPPA_SRS_REPETITION_FACTOR_RF2;
    srs_resource->freq_domain_position = 32;
    srs_resource->freq_domain_shift = 108;
    srs_resource->c_srs = 31;
    srs_resource->b_srs = 1;
    srs_resource->b_hop = 3;
    srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_GROUPHOPPING;
    srs_resource->resource_type.present = NRPPA_RESOURCE_TYPE_PR_PERIODIC;

    nrppa_resource_type_periodic_t *periodic = &srs_resource->resource_type.choice.periodic;
    periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT16;
    periodic->offset = 1000;
    srs_resource->sequence_id = 18;

    // SRS resource config 2 : semi-persistent
    srs_resource = &srs_resource_list->srs_resource[1];
    srs_resource->srs_resource_id = 24;
    srs_resource->nr_of_srs_ports = NRPPA_SRS_NUMBER_OF_PORTS_N4;
    srs_resource->transmission_comb.present = NRPPA_TRANSMISSION_COMB_PR_N4;
    srs_resource->transmission_comb.choice.n2.comb_offset_n2 = 3;
    srs_resource->transmission_comb.choice.n2.cyclic_shift_n2 = 5;
    srs_resource->start_position = 8;
    srs_resource->nr_of_symbols = NRPPA_SRS_NUMBER_OF_SYMBOLS_N2;
    srs_resource->repetition_factor = NRPPA_SRS_REPETITION_FACTOR_RF2;
    srs_resource->freq_domain_position = 32;
    srs_resource->freq_domain_shift = 108;
    srs_resource->c_srs = 31;
    srs_resource->b_srs = 1;
    srs_resource->b_hop = 3;
    srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_GROUPHOPPING;
    srs_resource->resource_type.present = NRPPA_RESOURCE_TYPE_PR_SEMI_PERSISTENT;

    srs_resource->resource_type.present = NRPPA_RESOURCE_TYPE_PR_SEMI_PERSISTENT;
    nrppa_resource_type_semi_persistent_t *semi_persistent = &srs_resource->resource_type.choice.semi_persistent;
    semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT40;
    semi_persistent->offset = 1000;
    srs_resource->sequence_id = 12;

    // SRS resource config 2 : aperiodic
    srs_resource = &srs_resource_list->srs_resource[2];
    srs_resource->srs_resource_id = 26;
    srs_resource->nr_of_srs_ports = NRPPA_SRS_NUMBER_OF_PORTS_N2;
    srs_resource->transmission_comb.present = NRPPA_TRANSMISSION_COMB_PR_N2;
    srs_resource->transmission_comb.choice.n2.comb_offset_n2 = 1;
    srs_resource->transmission_comb.choice.n2.cyclic_shift_n2 = 3;
    srs_resource->start_position = 8;
    srs_resource->nr_of_symbols = NRPPA_SRS_NUMBER_OF_SYMBOLS_N4;
    srs_resource->repetition_factor = NRPPA_SRS_REPETITION_FACTOR_RF2;
    srs_resource->freq_domain_position = 32;
    srs_resource->freq_domain_shift = 108;
    srs_resource->c_srs = 31;
    srs_resource->b_srs = 1;
    srs_resource->b_hop = 3;
    srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_SEQUENCEHOPPING;
    srs_resource->resource_type.present = NRPPA_RESOURCE_TYPE_PR_APERIODIC;
    // set aperiodic = 0 to enable
    srs_resource->resource_type.choice.aperiodic = 0;
    srs_resource->sequence_id = 10;

    // optional: pos_srs_resource_list
    sRSConfig->pos_srs_resource_list = calloc_or_fail(1, sizeof(*sRSConfig->pos_srs_resource_list));
    nrppa_pos_srs_resource_list_t *pos_srs_resource_list = sRSConfig->pos_srs_resource_list;
    uint32_t pos_srs_resource_list_length = 3;
    pos_srs_resource_list->pos_srs_resource_list_length = pos_srs_resource_list_length;
    pos_srs_resource_list->pos_srs_resource_item =
        calloc_or_fail(pos_srs_resource_list_length, sizeof(*pos_srs_resource_list->pos_srs_resource_item));

    // periodic
    nrppa_pos_srs_resource_item_t *pos_srs_resource = &pos_srs_resource_list->pos_srs_resource_item[0];
    pos_srs_resource->srs_pos_resource_id = 12;
    pos_srs_resource->transmission_comb_pos.present = NRPPA_TRANSMISSION_COMB_POS_PR_N2;
    pos_srs_resource->transmission_comb_pos.choice.n2.comb_offset_n2 = 1;
    pos_srs_resource->transmission_comb_pos.choice.n2.cyclic_shift_n2 = 4;
    pos_srs_resource->start_position = 9;
    pos_srs_resource->nr_of_symbols = NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N8;
    pos_srs_resource->freq_domain_shift = 100;
    pos_srs_resource->c_srs = 20;
    pos_srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_GROUPHOPPING;
    pos_srs_resource->resource_type_pos.present = NRPPA_RESOURCE_TYPE_POS_PR_PERIODIC;
    pos_srs_resource->resource_type_pos.choice.periodic.periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5;
    pos_srs_resource->resource_type_pos.choice.periodic.offset = 10;
    pos_srs_resource->sequence_id = 1;

    // semi-persistent
    pos_srs_resource = &pos_srs_resource_list->pos_srs_resource_item[1];
    pos_srs_resource->srs_pos_resource_id = 13;
    pos_srs_resource->transmission_comb_pos.present = NRPPA_TRANSMISSION_COMB_POS_PR_N4;
    pos_srs_resource->transmission_comb_pos.choice.n4.comb_offset_n4 = 3;
    pos_srs_resource->transmission_comb_pos.choice.n4.cyclic_shift_n4 = 5;
    pos_srs_resource->start_position = 12;
    pos_srs_resource->nr_of_symbols = NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N12;
    pos_srs_resource->freq_domain_shift = 100;
    pos_srs_resource->c_srs = 20;
    pos_srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_GROUPHOPPING;
    pos_srs_resource->resource_type_pos.present = NRPPA_RESOURCE_TYPE_POS_PR_SEMI_PERSISTENT;
    pos_srs_resource->resource_type_pos.choice.semi_persistent.periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT32;
    pos_srs_resource->resource_type_pos.choice.semi_persistent.offset = 5;
    pos_srs_resource->sequence_id = 2;

    // aperiodic
    pos_srs_resource = &pos_srs_resource_list->pos_srs_resource_item[2];
    pos_srs_resource->srs_pos_resource_id = 14;
    pos_srs_resource->transmission_comb_pos.present = NRPPA_TRANSMISSION_COMB_POS_PR_N8;
    pos_srs_resource->transmission_comb_pos.choice.n8.comb_offset_n8 = 5;
    pos_srs_resource->transmission_comb_pos.choice.n8.cyclic_shift_n8 = 2;
    pos_srs_resource->start_position = 9;
    pos_srs_resource->nr_of_symbols = NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N4;
    pos_srs_resource->freq_domain_shift = 100;
    pos_srs_resource->c_srs = 20;
    pos_srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_SEQUENCEHOPPING;
    pos_srs_resource->resource_type_pos.present = NRPPA_RESOURCE_TYPE_POS_PR_APERIODIC;
    pos_srs_resource->resource_type_pos.choice.periodic.periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5;
    pos_srs_resource->resource_type_pos.choice.aperiodic.slot_offset = 31;
    pos_srs_resource->sequence_id = 3;

    // optional: srs_resource_set_list
    sRSConfig->srs_resource_set_list = calloc_or_fail(1, sizeof(*sRSConfig->srs_resource_set_list));
    nrppa_srs_resource_set_list_t *srs_resource_set_list = sRSConfig->srs_resource_set_list;
    uint32_t srs_resource_set_list_length = 3;
    srs_resource_set_list->srs_resource_set_list_length = srs_resource_set_list_length;
    srs_resource_set_list->srs_resource_set =
        calloc_or_fail(srs_resource_set_list_length, sizeof(*srs_resource_set_list->srs_resource_set));

    // periodic
    nrppa_srs_resource_set_t *srs_resource_set = &srs_resource_set_list->srs_resource_set[0];
    srs_resource_set->srs_resource_set_id = 0;
    uint8_t srs_resource_id_list_length = 5;
    srs_resource_set->srs_resource_id_list.srs_resource_id_list_length = srs_resource_id_list_length;
    srs_resource_set->srs_resource_id_list.srs_resource_id =
        calloc_or_fail(srs_resource_id_list_length, sizeof(*srs_resource_set->srs_resource_id_list.srs_resource_id));
    for (int k = 0; k < srs_resource_id_list_length; k++) {
      srs_resource_set->srs_resource_id_list.srs_resource_id[k] = k;
    }
    srs_resource_set->resource_set_type.present = NRPPA_RESOURCE_SET_TYPE_PR_PERIODIC;
    // set periodic = 0 to enable
    srs_resource_set->resource_set_type.choice.periodic = 0;

    // semi-persistent
    srs_resource_set = &srs_resource_set_list->srs_resource_set[1];
    srs_resource_set->srs_resource_set_id = 1;
    srs_resource_id_list_length = 5;
    srs_resource_set->srs_resource_id_list.srs_resource_id_list_length = srs_resource_id_list_length;
    srs_resource_set->srs_resource_id_list.srs_resource_id =
        calloc_or_fail(srs_resource_id_list_length, sizeof(srs_resource_set->srs_resource_id_list.srs_resource_id));
    for (int k = 0; k < srs_resource_id_list_length; k++) {
      srs_resource_set->srs_resource_id_list.srs_resource_id[k] = k;
    }
    srs_resource_set->resource_set_type.present = NRPPA_RESOURCE_SET_TYPE_PR_SEMI_PERSISTENT;
    // set semi_persistent = 0 to enable
    srs_resource_set->resource_set_type.choice.semi_persistent = 0;

    // aperiodic
    srs_resource_set = &srs_resource_set_list->srs_resource_set[2];
    srs_resource_set->srs_resource_set_id = 2;
    srs_resource_id_list_length = 5;
    srs_resource_set->srs_resource_id_list.srs_resource_id_list_length = srs_resource_id_list_length;
    srs_resource_set->srs_resource_id_list.srs_resource_id =
        calloc_or_fail(srs_resource_id_list_length, sizeof(*srs_resource_set->srs_resource_id_list.srs_resource_id));
    for (int k = 0; k < srs_resource_id_list_length; k++) {
      srs_resource_set->srs_resource_id_list.srs_resource_id[k] = k;
    }

    srs_resource_set->resource_set_type.present = NRPPA_RESOURCE_SET_TYPE_PR_APERIODIC;
    srs_resource_set->resource_set_type.choice.aperiodic.srs_resource_trigger = 2;
    srs_resource_set->resource_set_type.choice.aperiodic.slot_offset = 20;

    // optional: pos_srs_resource_set_list
    sRSConfig->pos_srs_resource_set_list = calloc_or_fail(1, sizeof(*sRSConfig->pos_srs_resource_set_list));
    nrppa_pos_srs_resource_set_list_t *pos_srs_resource_set_list = sRSConfig->pos_srs_resource_set_list;
    uint32_t pos_srs_resource_set_list_length = 3;
    pos_srs_resource_set_list->pos_srs_resource_set_list_length = pos_srs_resource_set_list_length;
    pos_srs_resource_set_list->pos_srs_resource_set_item =
        calloc_or_fail(pos_srs_resource_set_list_length, sizeof(*pos_srs_resource_set_list->pos_srs_resource_set_item));

    // periodic
    nrppa_pos_srs_resource_set_item_t *pos_srs_resource_set = &pos_srs_resource_set_list->pos_srs_resource_set_item[0];
    pos_srs_resource_set->pos_srs_resource_set_id = 0;
    uint8_t pos_srs_resource_id_list_length = 5;
    pos_srs_resource_set->pos_srs_resource_id_list.pos_srs_resource_id_list_length = pos_srs_resource_id_list_length;
    pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id =
        calloc_or_fail(pos_srs_resource_id_list_length,
                       sizeof(*pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id));
    for (int k = 0; k < pos_srs_resource_id_list_length; k++) {
      pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id[k] = k;
    }
    pos_srs_resource_set->pos_resource_set_type.present = NRPPA_POS_RESOURCE_SET_TYPE_PR_PERIODIC;
    // set periodic = 0 to enable
    pos_srs_resource_set->pos_resource_set_type.choice.periodic = 0;

    // semi-persistent
    pos_srs_resource_set = &pos_srs_resource_set_list->pos_srs_resource_set_item[1];
    pos_srs_resource_set->pos_srs_resource_set_id = 1;
    pos_srs_resource_id_list_length = 5;
    pos_srs_resource_set->pos_srs_resource_id_list.pos_srs_resource_id_list_length = pos_srs_resource_id_list_length;
    pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id =
        calloc_or_fail(pos_srs_resource_id_list_length,
                       sizeof(*pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id));
    for (int k = 0; k < pos_srs_resource_id_list_length; k++) {
      pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id[k] = k;
    }
    pos_srs_resource_set->pos_resource_set_type.present = NRPPA_POS_RESOURCE_SET_TYPE_PR_SEMI_PERSISTENT;
    // set semi_persistent = 0 to enable
    pos_srs_resource_set->pos_resource_set_type.choice.semi_persistent = 0;

    // aperiodic
    pos_srs_resource_set = &pos_srs_resource_set_list->pos_srs_resource_set_item[2];
    pos_srs_resource_set->pos_srs_resource_set_id = 2;
    pos_srs_resource_id_list_length = 5;
    pos_srs_resource_set->pos_srs_resource_id_list.pos_srs_resource_id_list_length = pos_srs_resource_id_list_length;
    pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id =
        calloc_or_fail(pos_srs_resource_id_list_length,
                       sizeof(*pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id));
    for (int k = 0; k < pos_srs_resource_id_list_length; k++) {
      pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id[k] = k;
    }
    pos_srs_resource_set->pos_resource_set_type.present = NRPPA_POS_RESOURCE_SET_TYPE_PR_APERIODIC;
    pos_srs_resource_set->pos_resource_set_type.choice.srs_resource = 3;
  }
  return resp;
}

static nrppa_measurement_resp_t fill_measurement_response(uint16_t transaction_id, uint32_t lmf_measurement_id)
{
  nrppa_measurement_resp_t resp = {0};
  resp.transaction_id = transaction_id;
  resp.lmf_measurement_id = lmf_measurement_id;
  resp.ran_measurement_id = 1;
  resp.measurement_response_list = calloc_or_fail(1, sizeof(*resp.measurement_response_list));
  uint32_t meas_response_list_len = positioning_config->num_trp;
  resp.measurement_response_list->measurement_response_list_length = meas_response_list_len;
  resp.measurement_response_list->measurement_response_item =
      calloc_or_fail(meas_response_list_len, sizeof(*resp.measurement_response_list->measurement_response_item));
  for (int i = 0; i < meas_response_list_len; i++) {
    nrppa_measurement_response_item_t *meas_response_item = &resp.measurement_response_list->measurement_response_item[i];
    meas_response_item->trp_id = positioning_config->trps[i].id;

    nrppa_measurement_result_t *MeasurementResult = &meas_response_item->measurement_result;
    uint32_t meas_result_length = 4;
    MeasurementResult->measurement_result_item_length = meas_result_length;
    MeasurementResult->measurement_result_item =
        calloc_or_fail(meas_result_length, sizeof(*MeasurementResult->measurement_result_item));

    // Angle of arrival
    nrppa_measurement_result_item_t *measurement_result_item = &MeasurementResult->measurement_result_item[0];
    nrppa_measured_results_value_t *measuredResultsValue = &measurement_result_item->measured_results_value;
    measuredResultsValue->present = NRPPA_MEASURED_RESULTS_VALUE_PR_UL_ANGLEOFARRIVAL;
    nrppa_ul_aoa_t *uL_AngleOfArrival = &measuredResultsValue->choice.ul_angle_of_arrival;
    uL_AngleOfArrival->azimuth_aoa = 700 + i;
    uL_AngleOfArrival->zenith_aoa = calloc_or_fail(1, sizeof(*uL_AngleOfArrival->zenith_aoa));
    *uL_AngleOfArrival->zenith_aoa = 450 + i;

    nrppa_time_stamp_t *timeStamp = &measurement_result_item->time_stamp;
    timeStamp->system_frame_number = 100;
    timeStamp->slot_index.present = NRPPA_TIME_STAMP_SLOT_INDEX_PR_SCS_30;
    timeStamp->slot_index.choice.scs_30 = 15;

    // UL SRS RSRP
    measurement_result_item = &MeasurementResult->measurement_result_item[1];
    measuredResultsValue = &measurement_result_item->measured_results_value;
    measuredResultsValue->present = NRPPA_MEASURED_RESULTS_VALUE_PR_UL_SRS_RSRP;
    measuredResultsValue->choice.ul_srs_rsrp = 100 + i;

    timeStamp = &measurement_result_item->time_stamp;
    timeStamp->system_frame_number = 101;
    timeStamp->slot_index.present = NRPPA_TIME_STAMP_SLOT_INDEX_PR_SCS_15;
    timeStamp->slot_index.choice.scs_15 = 8;

    // UL RToA
    measurement_result_item = &MeasurementResult->measurement_result_item[2];
    measuredResultsValue = &measurement_result_item->measured_results_value;
    measuredResultsValue->present = NRPPA_MEASURED_RESULTS_VALUE_PR_UL_RTOA;
    nrppa_ul_rtoa_measurement_t *uL_RTOA = &measuredResultsValue->choice.ul_rtoa;
    uL_RTOA->present = NRPPA_ULRTOAMEAS_PR_K1;
    uL_RTOA->choice.k1 = 98500 + i;

    timeStamp = &measurement_result_item->time_stamp;
    timeStamp->system_frame_number = 102;
    timeStamp->slot_index.present = NRPPA_TIME_STAMP_SLOT_INDEX_PR_SCS_60;
    timeStamp->slot_index.choice.scs_60 = 31;

    // gNB RX-TX Time Diff
    measurement_result_item = &MeasurementResult->measurement_result_item[3];
    measuredResultsValue = &measurement_result_item->measured_results_value;
    measuredResultsValue->present = NRPPA_MEASURED_RESULTS_VALUE_PR_GNB_RXTXTIMEDIFF;
    nrppa_gnb_rx_tx_time_diff_t *gNB_RxTxTimeDiff = &measuredResultsValue->choice.gnb_rx_tx_time_diff;
    gNB_RxTxTimeDiff->present = NRPPA_GNBRXTXTIMEDIFFMEAS_PR_K3;
    gNB_RxTxTimeDiff->choice.k3 = 98412 + i;

    timeStamp = &measurement_result_item->time_stamp;
    timeStamp->system_frame_number = 103;
    timeStamp->slot_index.present = NRPPA_TIME_STAMP_SLOT_INDEX_PR_SCS_120;
    timeStamp->slot_index.choice.scs_120 = 70;
  }
  return resp;
}

// Emulate registration request to register UE in the AMF (required for positioning)
void send_initial_ue_message(instance_t instance)
{
  MessageDef *msg_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_NAS_FIRST_REQ);

  NGAP_NAS_FIRST_REQ(msg_p).gNB_ue_ngap_id = 1; // Simulated UE ID

  NGAP_NAS_FIRST_REQ(msg_p).plmn.mcc = mcc;
  NGAP_NAS_FIRST_REQ(msg_p).plmn.mnc = mnc;
  NGAP_NAS_FIRST_REQ(msg_p).plmn.mnc_digit_length = mnc_len;

  NGAP_NAS_FIRST_REQ(msg_p).nr_cell_id = gnb_id;

  NGAP_NAS_FIRST_REQ(msg_p).establishment_cause = NGAP_RRC_CAUSE_MO_SIGNALLING;

  as_nas_info_t initialNasMsg = {0};
  nr_ue_nas_t *nr_ue_nas = get_ue_nas_info(0);
  // serving network name (snn)
  // Ideally snn should be filled from SIB1, but we operate at NAS/RRC level and we dont decode SIB1
  nr_ue_nas->sn_id = calloc(1, sizeof(plmn_id_t));
  nr_ue_nas->sn_id->mcc = mcc;
  nr_ue_nas->sn_id->mnc = mnc;
  nr_ue_nas->sn_id->mnc_digit_length = mnc_len;
  generateRegistrationRequest(&initialNasMsg, nr_ue_nas, false);

  NGAP_NAS_FIRST_REQ(msg_p).nas_pdu.buf = initialNasMsg.nas_data;
  NGAP_NAS_FIRST_REQ(msg_p).nas_pdu.len = initialNasMsg.length;

  NGAP_NAS_FIRST_REQ(msg_p).ue_identity.presenceMask = 0;

  itti_send_msg_to_task(TASK_NGAP, instance, msg_p);
}

void *gNB_app_task(void *args_p)
{
  MessageDef *msg_p = NULL;
  const char *msg_name = NULL;
  instance_t instance;
  int result;
  /* for no gcc warnings */
  (void)instance;

  itti_mark_task_ready(TASK_GNB_APP);
  do {
    // Wait for a message
    itti_receive_msg(TASK_GNB_APP, &msg_p);

    msg_name = ITTI_MSG_NAME(msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE(msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
      case TERMINATE_MESSAGE:
        LOG_W(GNB_APP, " *** Exiting GNB_APP thread\n");
        itti_exit_task();
        break;

      case NGAP_REGISTER_GNB_CNF:
        LOG_I(GNB_APP, "[gNB %ld] Received %s: associated AMF %d\n", instance, msg_name, NGAP_REGISTER_GNB_CNF(msg_p).nb_amf);
        send_initial_ue_message(instance);
        break;

      default:
        LOG_E(GNB_APP, "Received unexpected message %s\n", msg_name);
        break;
    }
    result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);

  return NULL;
}

void *rrc_gnb_task(void *args_p)
{
  MessageDef *msg_p;
  MessageDef *msg_p_resp;
  instance_t instance;
  int result;

  itti_mark_task_ready(TASK_RRC_GNB);
  LOG_I(NR_RRC, "Entering main loop of NR_RRC message task\n");

  while (1) {
    // Wait for a message
    itti_receive_msg(TASK_RRC_GNB, &msg_p);
    const char *msg_name_p = ITTI_MSG_NAME(msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE(msg_p);
    LOG_D(NR_RRC,
          "RRC GNB Task Received %s for instance %ld from task %s\n",
          ITTI_MSG_NAME(msg_p),
          ITTI_MSG_DESTINATION_INSTANCE(msg_p),
          ITTI_MSG_ORIGIN_NAME(msg_p));
    switch (ITTI_MSG_ID(msg_p)) {
      case TERMINATE_MESSAGE:
        LOG_W(NR_RRC, " *** Exiting NR_RRC thread\n");
        itti_exit_task();
        break;
      case NGAP_DOWNLINK_NAS:
        LOG_I(NR_RRC, "Ignore NGAP_DOWNLINK_NAS: dont have to handle\n");
        ngap_downlink_nas_t *nas = &NGAP_DOWNLINK_NAS(msg_p);
        if (nas->nas_pdu.buf) {
          free(nas->nas_pdu.buf);
        }
        break;
      case NRPPA_TRP_INFORMATION_REQ:
        nrppa_trp_information_req_t *trp_req = &NRPPA_TRP_INFORMATION_REQ(msg_p);
        LOG_I(NR_RRC, "Received NRPPA_TRP_INFORMATION_REQ transaction_id %d\n", trp_req->transaction_id);
        msg_p_resp = itti_alloc_new_message(TASK_RRC_GNB, 0, NRPPA_TRP_INFORMATION_RESP);
        NRPPA_TRP_INFORMATION_RESP(msg_p_resp) = fill_trp_resp(trp_req->transaction_id);
        LOG_I(NR_RRC, "Sending NRPPA_TRP_INFORMATION_RESP to TASK_NRPPA\n");
        itti_send_msg_to_task(TASK_NRPPA, 0, msg_p_resp);
        break;
      case NRPPA_POSITIONING_INFORMATION_REQ:
        nrppa_positioning_information_req_t *pos_req = &NRPPA_POSITIONING_INFORMATION_REQ(msg_p);
        LOG_I(NR_RRC, "Received NRPPA_POSITIONING_INFORMATION_REQ transaction_id %d\n", pos_req->transaction_id);
        msg_p_resp = itti_alloc_new_message(TASK_RRC_GNB, 0, NRPPA_POSITIONING_INFORMATION_RESP);
        NRPPA_POSITIONING_INFORMATION_RESP(msg_p_resp) = fill_position_information_resp(pos_req->transaction_id);
        LOG_I(NR_RRC, "Sending NRPPA_POSITIONING_INFORMATION_RESP to TASK_NRPPA\n");
        itti_send_msg_to_task(TASK_NRPPA, 0, msg_p_resp);
        break;
      case NRPPA_POSITIONING_ACTIVATION_REQ:
        nrppa_positioning_activation_req_t *req = &NRPPA_POSITIONING_ACTIVATION_REQ(msg_p);
        LOG_I(NR_RRC, "Received NRPPA_POSITIONING_ACTIVATION_REQ transaction_id %d\n", req->transaction_id);
        msg_p_resp = itti_alloc_new_message(TASK_RRC_GNB, 0, NRPPA_POSITIONING_ACTIVATION_RESP);
        NRPPA_POSITIONING_ACTIVATION_RESP(msg_p_resp).transaction_id = req->transaction_id;
        LOG_I(NR_RRC, "Sending NRPPA_POSITIONING_ACTIVATION_RESP to TASK_NRPPA\n");
        itti_send_msg_to_task(TASK_NRPPA, 0, msg_p_resp);
        free_nrppa_positioning_activation_request(req);
        break;
      case NRPPA_MEASUREMENT_REQ:
        nrppa_measurement_req_t *m_req = &NRPPA_MEASUREMENT_REQ(msg_p);
        LOG_I(NR_RRC, "Received NRPPA_MEASUREMENT_REQ transaction_id %d\n", m_req->transaction_id);
        msg_p_resp = itti_alloc_new_message(TASK_RRC_GNB, 0, NRPPA_MEASUREMENT_RESP);
        NRPPA_MEASUREMENT_RESP(msg_p_resp) = fill_measurement_response(m_req->transaction_id, m_req->lmf_measurement_id);
        LOG_I(NR_RRC, "Sending NRPPA_MEASUREMENT_RESP to TASK_NRPPA\n");
        itti_send_msg_to_task(TASK_NRPPA, 0, msg_p_resp);
        free_nrppa_measurement_request(m_req);
        break;
      default:
        LOG_E(NR_RRC, "[gNB %ld] Received unexpected message %s\n", instance, msg_name_p);
        break;
    }
    result = itti_free(ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    msg_p = NULL;
  }
}

int main(int argc, char **argv)
{
  // load the config file
  if ((uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY)) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }
  logInit();

  positioning_config = RCconfig_nr_positioning();
  if (positioning_config == NULL) {
    LOG_E(NRPPA, "No TRPs configured for positioning in the configuration file.\n");
    exit(EXIT_FAILURE);
  }

  nas_init_nrue(NB_UE_INST);

  set_softmodem_sighandler();

  // Initialize ITTI tasks
  itti_init(TASK_MAX, tasks_info);

  int rc;
  rc = itti_create_task(TASK_SCTP, sctp_eNB_task, NULL);
  AssertFatal(rc >= 0, "Create task for SCTP failed\n");

  rc = itti_create_task(TASK_NGAP, ngap_gNB_task, NULL);
  AssertFatal(rc >= 0, "Create task for NGAP failed\n");

  rc = itti_create_task(TASK_NRPPA, nrppa_gNB_task, NULL);
  AssertFatal(rc >= 0, "Create task for NRPPA failed\n");

  rc = itti_create_task(TASK_RRC_GNB, rrc_gnb_task, NULL);
  AssertFatal(rc >= 0, "Create task for RRC failed\n");

  rc = itti_create_task(TASK_GNB_APP, gNB_app_task, NULL);
  AssertFatal(rc >= 0, "Create task for RRC failed\n");

  MessageDef *msg_p;
  msg_p = itti_alloc_new_message(TASK_GNB_APP, 0, NGAP_REGISTER_GNB_REQ);
  RCconfig_NR_NG(msg_p, 0);
  gnb_id = NGAP_REGISTER_GNB_REQ(msg_p).gNB_id;
  tac = NGAP_REGISTER_GNB_REQ(msg_p).tac;
  mcc = NGAP_REGISTER_GNB_REQ(msg_p).plmn[0].plmn.mcc;
  mnc = NGAP_REGISTER_GNB_REQ(msg_p).plmn[0].plmn.mnc;
  mnc_len = NGAP_REGISTER_GNB_REQ(msg_p).plmn[0].plmn.mnc_digit_length;

  LOG_I(GNB_APP, "Sending NGAP_REGISTER_GNB_REQ to TASK_NGAP\n");
  itti_send_msg_to_task(TASK_NGAP, 0, msg_p);

  printf("TYPE <CTRL-C> TO TERMINATE\n");
  itti_wait_tasks_end(NULL);

  logClean();
  free(positioning_config);
  printf("Bye.\n");
  return 0;
}
