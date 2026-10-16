/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "intertask_interface.h"
#include "nrppa_common.h"
#include "nrppa_gNB_ue_context.h"
#include "nrppa_messages_types.h"
#include "nrppa_gNB_encoder.h"
#include "openair3/UTILS/conversions.h"
#include "nrppa_gNB_measurement_information_transfer.h"
#include "nrppa_gNB_location_information_transfer.h"

static void decode_srs_config(const NRPPA_SRSConfig_t *in_config, nrppa_srs_config_t *out_config)
{
  // optional: sRSResource_List
  if (in_config->sRSResource_List) {
    out_config->srs_resource_list = calloc_or_fail(1, sizeof(*out_config->srs_resource_list));
    nrppa_srs_resource_list_t *srs_resource_list = out_config->srs_resource_list;
    uint32_t srs_resource_list_length = in_config->sRSResource_List->list.count;
    srs_resource_list->srs_resource_list_length = srs_resource_list_length;
    srs_resource_list->srs_resource = calloc_or_fail(srs_resource_list_length, sizeof(*srs_resource_list->srs_resource));
    for (int i = 0; i < srs_resource_list_length; i++) {
      nrppa_srs_resource_t *srs_resource = &srs_resource_list->srs_resource[i];
      NRPPA_SRSResource_t *nrppa_srs_resource = in_config->sRSResource_List->list.array[i];
      srs_resource->srs_resource_id = nrppa_srs_resource->sRSResourceID;
      switch (nrppa_srs_resource->nrofSRS_Ports) {
        case NRPPA_SRSResource__nrofSRS_Ports_port1:
          srs_resource->nr_of_srs_ports = NRPPA_SRS_NUMBER_OF_PORTS_N1;
          break;
        case NRPPA_SRSResource__nrofSRS_Ports_ports2:
          srs_resource->nr_of_srs_ports = NRPPA_SRS_NUMBER_OF_PORTS_N2;
          break;
        case NRPPA_SRSResource__nrofSRS_Ports_ports4:
          srs_resource->nr_of_srs_ports = NRPPA_SRS_NUMBER_OF_PORTS_N4;
          break;
        default:
          AssertFatal(false, "illegal number of srs ports %ld\n", nrppa_srs_resource->nrofSRS_Ports);
          break;
      }

      nrppa_transmission_comb_t *srs_tx_comb = &srs_resource->transmission_comb;
      NRPPA_TransmissionComb_t *nrppa_srs_tx_comb = &nrppa_srs_resource->transmissionComb;
      switch (nrppa_srs_tx_comb->present) {
        case NRPPA_TransmissionComb_PR_NOTHING:
          srs_tx_comb->present = NRPPA_TRANSMISSION_COMB_PR_NOTHING;
          break;
        case NRPPA_TransmissionComb_PR_n2:
          srs_tx_comb->present = NRPPA_TRANSMISSION_COMB_PR_N2;
          srs_tx_comb->choice.n2.comb_offset_n2 = nrppa_srs_tx_comb->choice.n2->combOffset_n2;
          srs_tx_comb->choice.n2.cyclic_shift_n2 = nrppa_srs_tx_comb->choice.n2->cyclicShift_n2;
          break;
        case NRPPA_TransmissionComb_PR_n4:
          srs_tx_comb->present = NRPPA_TRANSMISSION_COMB_PR_N4;
          srs_tx_comb->choice.n4.comb_offset_n4 = nrppa_srs_tx_comb->choice.n4->combOffset_n4;
          srs_tx_comb->choice.n4.cyclic_shift_n4 = nrppa_srs_tx_comb->choice.n4->cyclicShift_n4;
          break;
        default:
          AssertFatal(false, "illegal transmissionComb %d\n", nrppa_srs_tx_comb->present);
          break;
      }

      srs_resource->start_position = nrppa_srs_resource->startPosition;

      switch (nrppa_srs_resource->nrofSymbols) {
        case NRPPA_SRSResource__nrofSymbols_n1:
          srs_resource->nr_of_symbols = NRPPA_SRS_NUMBER_OF_SYMBOLS_N1;
          break;
        case NRPPA_SRSResource__nrofSymbols_n2:
          srs_resource->nr_of_symbols = NRPPA_SRS_NUMBER_OF_SYMBOLS_N2;
          break;
        case NRPPA_SRSResource__nrofSymbols_n4:
          srs_resource->nr_of_symbols = NRPPA_SRS_NUMBER_OF_SYMBOLS_N4;
          break;
        default:
          AssertFatal(false, "illegal number of symbols %ld\n", nrppa_srs_resource->nrofSymbols);
          break;
      }

      switch (nrppa_srs_resource->repetitionFactor) {
        case NRPPA_SRSResource__repetitionFactor_n1:
          srs_resource->repetition_factor = NRPPA_SRS_REPETITION_FACTOR_RF1;
          break;
        case NRPPA_SRSResource__repetitionFactor_n2:
          srs_resource->repetition_factor = NRPPA_SRS_REPETITION_FACTOR_RF2;
          break;
        case NRPPA_SRSResource__repetitionFactor_n4:
          srs_resource->repetition_factor = NRPPA_SRS_REPETITION_FACTOR_RF4;
          break;
        default:
          AssertFatal(false, "illegal repetition factor %ld\n", nrppa_srs_resource->repetitionFactor);
          break;
      }

      srs_resource->freq_domain_position = nrppa_srs_resource->freqDomainPosition;
      srs_resource->freq_domain_shift = nrppa_srs_resource->freqDomainShift;
      srs_resource->c_srs = nrppa_srs_resource->c_SRS;
      srs_resource->b_srs = nrppa_srs_resource->b_SRS;
      srs_resource->b_hop = nrppa_srs_resource->b_hop;

      switch (nrppa_srs_resource->groupOrSequenceHopping) {
        case NRPPA_SRSResource__groupOrSequenceHopping_neither:
          srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_NOTHING;
          break;
        case NRPPA_SRSResource__groupOrSequenceHopping_groupHopping:
          srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_GROUPHOPPING;
          break;
        case NRPPA_SRSResource__groupOrSequenceHopping_sequenceHopping:
          srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_SEQUENCEHOPPING;
          break;
        default:
          AssertFatal(false, "illegal groupOrSequenceHopping %ld\n", nrppa_srs_resource->groupOrSequenceHopping);
          break;
      }

      NRPPA_ResourceType_t *nrppa_resourceType = &nrppa_srs_resource->resourceType;
      nrppa_resource_type_t *resource_type = &srs_resource->resource_type;
      if (nrppa_resourceType->present == NRPPA_ResourceType_PR_NOTHING) {
        resource_type->present = NRPPA_RESOURCE_TYPE_PR_NOTHING;
      } else if (nrppa_resourceType->present == NRPPA_ResourceType_PR_periodic) {
        resource_type->present = NRPPA_RESOURCE_TYPE_PR_PERIODIC;
        nrppa_resource_type_periodic_t *periodic = &resource_type->choice.periodic;
        switch (nrppa_resourceType->choice.periodic->periodicity) {
          case NRPPA_ResourceTypePeriodic__periodicity_slot1:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT1;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot2:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT2;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot4:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT4;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot5:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT5;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot8:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT8;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot10:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT10;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot16:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT16;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot20:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT20;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot32:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT32;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot40:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT40;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot64:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT64;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot80:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT80;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot160:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT160;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot320:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT320;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot640:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT640;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot1280:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT1280;
            break;
          case NRPPA_ResourceTypePeriodic__periodicity_slot2560:
            periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT2560;
            break;
          default:
            AssertFatal(false, "illegal periodicity %ld\n", nrppa_resourceType->choice.periodic->periodicity);
            break;
        }
        periodic->offset = nrppa_resourceType->choice.periodic->offset;
      } else if (nrppa_resourceType->present == NRPPA_ResourceType_PR_semi_persistent) {
        resource_type->present = NRPPA_RESOURCE_TYPE_PR_SEMI_PERSISTENT;
        nrppa_resource_type_semi_persistent_t *semi_persistent = &resource_type->choice.semi_persistent;
        switch (nrppa_resourceType->choice.semi_persistent->periodicity) {
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot1:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT1;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot2:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT2;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot4:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT4;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot5:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT5;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot8:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT8;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot10:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT10;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot16:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT16;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot20:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT20;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot32:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT32;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot40:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT40;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot64:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT64;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot80:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT80;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot160:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT160;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot320:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT320;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot640:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT640;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot1280:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT1280;
            break;
          case NRPPA_ResourceTypeSemi_persistent__periodicity_slot2560:
            semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT2560;
            break;
          default:
            AssertFatal(false, "illegal periodicity %ld\n", nrppa_resourceType->choice.semi_persistent->periodicity);
            break;
        }
        semi_persistent->offset = nrppa_resourceType->choice.semi_persistent->offset;
      } else if (nrppa_resourceType->present == NRPPA_ResourceType_PR_aperiodic) {
        resource_type->present = NRPPA_RESOURCE_TYPE_PR_APERIODIC;
        resource_type->choice.aperiodic = nrppa_resourceType->choice.aperiodic->aperiodicResourceType;
      } else {
        AssertFatal(false, "illegal resourceType %d\n", nrppa_resourceType->present);
      }

      srs_resource->sequence_id = nrppa_srs_resource->sequenceId;
    }
  }

  // optional: posSRSResource_List
  if (in_config->posSRSResource_List) {
    out_config->pos_srs_resource_list = calloc_or_fail(1, sizeof(*out_config->pos_srs_resource_list));
    nrppa_pos_srs_resource_list_t *pos_srs_resource_list = out_config->pos_srs_resource_list;
    uint32_t pos_srs_resource_list_length = in_config->posSRSResource_List->list.count;
    pos_srs_resource_list->pos_srs_resource_list_length = pos_srs_resource_list_length;
    pos_srs_resource_list->pos_srs_resource_item =
        calloc_or_fail(pos_srs_resource_list_length, sizeof(*pos_srs_resource_list->pos_srs_resource_item));
    for (int i = 0; i < pos_srs_resource_list_length; i++) {
      NRPPA_PosSRSResource_Item_t *nrppa_pos_srs_resource = in_config->posSRSResource_List->list.array[i];
      nrppa_pos_srs_resource_item_t *pos_srs_resource = &pos_srs_resource_list->pos_srs_resource_item[i];
      pos_srs_resource->srs_pos_resource_id = nrppa_pos_srs_resource->srs_PosResourceId;

      NRPPA_TransmissionCombPos_t *nrppa_tx_comb_pos = &nrppa_pos_srs_resource->transmissionCombPos;
      nrppa_transmission_comb_pos_t *tx_comb_pos = &pos_srs_resource->transmission_comb_pos;
      switch (nrppa_tx_comb_pos->present) {
        case NRPPA_TransmissionCombPos_PR_NOTHING:
          tx_comb_pos->present = NRPPA_TRANSMISSION_COMB_POS_PR_NOTHING;
          break;
        case NRPPA_TransmissionCombPos_PR_n2:
          tx_comb_pos->present = NRPPA_TRANSMISSION_COMB_POS_PR_N2;
          tx_comb_pos->choice.n2.comb_offset_n2 = nrppa_tx_comb_pos->choice.n2->combOffset_n2;
          tx_comb_pos->choice.n2.cyclic_shift_n2 = nrppa_tx_comb_pos->choice.n2->cyclicShift_n2;
          break;
        case NRPPA_TransmissionCombPos_PR_n4:
          tx_comb_pos->present = NRPPA_TRANSMISSION_COMB_POS_PR_N4;
          tx_comb_pos->choice.n4.comb_offset_n4 = nrppa_tx_comb_pos->choice.n4->combOffset_n4;
          tx_comb_pos->choice.n4.cyclic_shift_n4 = nrppa_tx_comb_pos->choice.n4->cyclicShift_n4;
          break;
        case NRPPA_TransmissionCombPos_PR_n8:
          tx_comb_pos->present = NRPPA_TRANSMISSION_COMB_POS_PR_N8;
          tx_comb_pos->choice.n8.comb_offset_n8 = nrppa_tx_comb_pos->choice.n8->combOffset_n8;
          tx_comb_pos->choice.n8.cyclic_shift_n8 = nrppa_tx_comb_pos->choice.n8->cyclicShift_n8;
          break;
        default:
          AssertFatal(false, "illegal transmissionComb %d\n", nrppa_tx_comb_pos->present);
          break;
      }

      pos_srs_resource->start_position = nrppa_pos_srs_resource->startPosition;

      switch (nrppa_pos_srs_resource->nrofSymbols) {
        case NRPPA_PosSRSResource_Item__nrofSymbols_n1:
          pos_srs_resource->nr_of_symbols = NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N1;
          break;
        case NRPPA_PosSRSResource_Item__nrofSymbols_n2:
          pos_srs_resource->nr_of_symbols = NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N2;
          break;
        case NRPPA_PosSRSResource_Item__nrofSymbols_n4:
          pos_srs_resource->nr_of_symbols = NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N4;
          break;
        case NRPPA_PosSRSResource_Item__nrofSymbols_n8:
          pos_srs_resource->nr_of_symbols = NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N8;
          break;
        case NRPPA_PosSRSResource_Item__nrofSymbols_n12:
          pos_srs_resource->nr_of_symbols = NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N12;
          break;
        default:
          AssertFatal(false, "illegal number of symbols %ld\n", nrppa_pos_srs_resource->nrofSymbols);
          break;
      }

      pos_srs_resource->freq_domain_shift = nrppa_pos_srs_resource->freqDomainShift;
      pos_srs_resource->c_srs = nrppa_pos_srs_resource->c_SRS;

      switch (nrppa_pos_srs_resource->groupOrSequenceHopping) {
        case NRPPA_PosSRSResource_Item__groupOrSequenceHopping_neither:
          pos_srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_NOTHING;
          break;
        case NRPPA_PosSRSResource_Item__groupOrSequenceHopping_groupHopping:
          pos_srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_GROUPHOPPING;
          break;
        case NRPPA_PosSRSResource_Item__groupOrSequenceHopping_sequenceHopping:
          pos_srs_resource->group_or_sequence_hopping = NRPPA_GROUPORSEQUENCEHOPPING_SEQUENCEHOPPING;
          break;
        default:
          AssertFatal(false, "illegal groupOrSequenceHopping %ld\n", nrppa_pos_srs_resource->groupOrSequenceHopping);
          break;
      }

      NRPPA_ResourceTypePos_t *nrppa_res_type_pos = &nrppa_pos_srs_resource->resourceTypePos;
      nrppa_resource_type_pos_t *res_type_pos = &pos_srs_resource->resource_type_pos;
      if (nrppa_res_type_pos->present == NRPPA_ResourceTypePos_PR_NOTHING) {
        res_type_pos->present = NRPPA_RESOURCE_TYPE_POS_PR_NOTHING;
      } else if (nrppa_res_type_pos->present == NRPPA_ResourceTypePos_PR_periodic) {
        res_type_pos->present = NRPPA_RESOURCE_TYPE_POS_PR_PERIODIC;
        nrppa_resource_type_periodic_pos_t *pos_periodic = &res_type_pos->choice.periodic;
        switch (nrppa_res_type_pos->choice.periodic->periodicity) {
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot1:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT1;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot2:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT2;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot4:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT4;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot5:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot8:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT8;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot10:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT10;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot16:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT16;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot20:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT20;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot32:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT32;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot40:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT40;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot64:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT64;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot80:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT80;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot160:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT160;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot320:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT320;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot640:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT640;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot1280:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT1280;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot2560:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT2560;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot5120:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5120;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot10240:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT10240;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot20480:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT20480;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot40960:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT40960;
            break;
          case NRPPA_ResourceTypePeriodicPos__periodicity_slot81920:
            pos_periodic->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT81920;
            break;
          default:
            AssertFatal(false, "illegal periodicity %ld\n", nrppa_res_type_pos->choice.periodic->periodicity);
            break;
        }
        pos_periodic->offset = nrppa_res_type_pos->choice.periodic->offset;
      } else if (nrppa_res_type_pos->present == NRPPA_ResourceTypePos_PR_semi_persistent) {
        res_type_pos->present = NRPPA_RESOURCE_TYPE_POS_PR_SEMI_PERSISTENT;
        nrppa_resource_type_semi_persistent_pos_t *pos_semi_persistent = &res_type_pos->choice.semi_persistent;
        switch (nrppa_res_type_pos->choice.semi_persistent->periodicity) {
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot1:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT1;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot2:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT2;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot4:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT4;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot5:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot8:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT8;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot10:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT10;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot16:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT16;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot20:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT20;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot32:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT32;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot40:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT40;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot64:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT64;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot80:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT80;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot160:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT160;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot320:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT320;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot640:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT640;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot1280:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT1280;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot2560:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT2560;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot5120:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5120;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot10240:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT10240;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot20480:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT20480;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot40960:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT40960;
            break;
          case NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot81920:
            pos_semi_persistent->periodicity = NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT81920;
            break;
          default:
            AssertFatal(false, "illegal periodicity %ld\n", nrppa_res_type_pos->choice.semi_persistent->periodicity);
            break;
        }
        pos_semi_persistent->offset = nrppa_res_type_pos->choice.semi_persistent->offset;
      } else if (nrppa_res_type_pos->present == NRPPA_ResourceTypePos_PR_aperiodic) {
        res_type_pos->present = NRPPA_RESOURCE_TYPE_POS_PR_APERIODIC;
        res_type_pos->choice.aperiodic.slot_offset = nrppa_res_type_pos->choice.aperiodic->slotOffset;
      } else {
        AssertFatal(false, "illegal resourceType %d\n", nrppa_res_type_pos->present);
      }

      pos_srs_resource->sequence_id = nrppa_pos_srs_resource->sequenceId;
    }
  }

  // optional: sRSResourceSet_List
  if (in_config->sRSResourceSet_List) {
    out_config->srs_resource_set_list = calloc_or_fail(1, sizeof(*out_config->srs_resource_set_list));
    nrppa_srs_resource_set_list_t *srs_resource_set_list = out_config->srs_resource_set_list;
    uint32_t srs_resource_set_list_length = in_config->sRSResourceSet_List->list.count;
    srs_resource_set_list->srs_resource_set_list_length = srs_resource_set_list_length;
    srs_resource_set_list->srs_resource_set =
        calloc_or_fail(srs_resource_set_list_length, sizeof(*srs_resource_set_list->srs_resource_set));
    for (int i = 0; i < srs_resource_set_list_length; i++) {
      NRPPA_SRSResourceSet_t *nrppa_srs_resource_set = in_config->sRSResourceSet_List->list.array[i];
      nrppa_srs_resource_set_t *srs_resource_set = &srs_resource_set_list->srs_resource_set[i];
      srs_resource_set->srs_resource_set_id = nrppa_srs_resource_set->sRSResourceSetID;
      uint8_t srs_resource_id_list_length = nrppa_srs_resource_set->sRSResourceID_List.list.count;
      srs_resource_set->srs_resource_id_list.srs_resource_id_list_length = srs_resource_id_list_length;
      srs_resource_set->srs_resource_id_list.srs_resource_id =
          calloc_or_fail(srs_resource_id_list_length, sizeof(*srs_resource_set->srs_resource_id_list.srs_resource_id));
      for (int j = 0; j < srs_resource_id_list_length; j++) {
        NRPPA_SRSResourceID_t *nrppa_srs_resource_id = nrppa_srs_resource_set->sRSResourceID_List.list.array[j];
        srs_resource_set->srs_resource_id_list.srs_resource_id[j] = *nrppa_srs_resource_id;
      }

      NRPPA_ResourceSetType_t *nrppa_res_set_type = &nrppa_srs_resource_set->resourceSetType;
      nrppa_resource_set_type_t *res_set_type = &srs_resource_set->resource_set_type;
      switch (nrppa_res_set_type->present) {
        case NRPPA_ResourceSetType_PR_NOTHING:
          res_set_type->present = NRPPA_RESOURCE_SET_TYPE_PR_NOTHING;
          break;
        case NRPPA_ResourceSetType_PR_periodic:
          res_set_type->present = NRPPA_RESOURCE_SET_TYPE_PR_PERIODIC;
          res_set_type->choice.periodic = nrppa_res_set_type->choice.periodic->periodicSet;
          break;
        case NRPPA_ResourceSetType_PR_semi_persistent:
          res_set_type->present = NRPPA_RESOURCE_SET_TYPE_PR_SEMI_PERSISTENT;
          res_set_type->choice.semi_persistent = nrppa_res_set_type->choice.semi_persistent->semi_persistentSet;
          break;
        case NRPPA_ResourceSetType_PR_aperiodic:
          res_set_type->present = NRPPA_RESOURCE_SET_TYPE_PR_APERIODIC;
          res_set_type->choice.aperiodic.srs_resource_trigger = nrppa_res_set_type->choice.aperiodic->sRSResourceTrigger;
          res_set_type->choice.aperiodic.slot_offset = nrppa_res_set_type->choice.aperiodic->slotoffset;
          break;
        default:
          AssertFatal(false, "illegal resource set type %d\n", nrppa_res_set_type->present);
          break;
      }
    }
  }

  // optional: posSRSResourceSet_List
  if (in_config->posSRSResourceSet_List) {
    out_config->pos_srs_resource_set_list = calloc_or_fail(1, sizeof(*out_config->pos_srs_resource_set_list));
    nrppa_pos_srs_resource_set_list_t *pos_srs_resource_set_list = out_config->pos_srs_resource_set_list;
    uint32_t pos_srs_resource_set_list_length = in_config->posSRSResourceSet_List->list.count;
    pos_srs_resource_set_list->pos_srs_resource_set_list_length = pos_srs_resource_set_list_length;
    pos_srs_resource_set_list->pos_srs_resource_set_item =
        calloc_or_fail(pos_srs_resource_set_list_length, sizeof(*pos_srs_resource_set_list->pos_srs_resource_set_item));
    for (int i = 0; i < pos_srs_resource_set_list_length; i++) {
      NRPPA_PosSRSResourceSet_Item_t *nrppa_pos_srs_resource_set = in_config->posSRSResourceSet_List->list.array[i];
      nrppa_pos_srs_resource_set_item_t *pos_srs_resource_set = &pos_srs_resource_set_list->pos_srs_resource_set_item[i];
      pos_srs_resource_set->pos_srs_resource_set_id = nrppa_pos_srs_resource_set->possrsResourceSetID;
      uint8_t pos_srs_resource_id_list_length = nrppa_pos_srs_resource_set->possRSResourceIDPerSet_List.list.count;
      pos_srs_resource_set->pos_srs_resource_id_list.pos_srs_resource_id_list_length = pos_srs_resource_id_list_length;
      pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id =
          calloc_or_fail(pos_srs_resource_id_list_length,
                         sizeof(*pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id));
      for (int j = 0; j < pos_srs_resource_id_list_length; j++) {
        NRPPA_SRSPosResourceID_t *nrppa_pos_srs_resource_id = nrppa_pos_srs_resource_set->possRSResourceIDPerSet_List.list.array[j];
        pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id[j] = *nrppa_pos_srs_resource_id;
      }

      NRPPA_PosResourceSetType_t *nrppa_pos_res_set_type = &nrppa_pos_srs_resource_set->posresourceSetType;
      nrppa_pos_resource_set_type_t *pos_res_set_type = &pos_srs_resource_set->pos_resource_set_type;
      switch (nrppa_pos_res_set_type->present) {
        case NRPPA_PosResourceSetType_PR_NOTHING:
          pos_res_set_type->present = NRPPA_POS_RESOURCE_SET_TYPE_PR_NOTHING;
          break;
        case NRPPA_PosResourceSetType_PR_periodic:
          pos_res_set_type->present = NRPPA_POS_RESOURCE_SET_TYPE_PR_PERIODIC;
          pos_res_set_type->choice.periodic = nrppa_pos_res_set_type->choice.periodic->posperiodicSet;
          break;
        case NRPPA_PosResourceSetType_PR_semi_persistent:
          pos_res_set_type->present = NRPPA_POS_RESOURCE_SET_TYPE_PR_SEMI_PERSISTENT;
          pos_res_set_type->choice.semi_persistent = nrppa_pos_res_set_type->choice.semi_persistent->possemi_persistentSet;
          break;
        case NRPPA_PosResourceSetType_PR_aperiodic:
          pos_res_set_type->present = NRPPA_POS_RESOURCE_SET_TYPE_PR_APERIODIC;
          pos_res_set_type->choice.srs_resource = nrppa_pos_res_set_type->choice.aperiodic->sRSResourceTrigger;
          break;
        default:
          AssertFatal(false, "illegal resource set type pos %d\n", nrppa_pos_res_set_type->present);
          break;
      }
    }
  }
}

static void decode_srs_carrier_list_item(const NRPPA_SRSCarrier_List_Item_t *in_item, nrppa_srs_carrier_list_item_t *out_item)
{
  // pointA
  out_item->pointA = in_item->pointA;

  // Uplink Channel BW-PerSCS-List
  nrppa_uplink_channel_bw_per_scs_list_t *uplink_channel_bw_per_scs_list = &out_item->uplink_channel_bw_per_scs_list;
  const NRPPA_UplinkChannelBW_PerSCS_List_t *nrppa_uplink_channel_bw_per_scs_list = &in_item->uplinkChannelBW_PerSCS_List;

  uint32_t scs_specific_carrier_list_length = nrppa_uplink_channel_bw_per_scs_list->list.count;
  AssertFatal(scs_specific_carrier_list_length > 0, "Atleast 1 uplink channel bw per scs list should be present\n");
  uplink_channel_bw_per_scs_list->scs_specific_carrier_list_length = scs_specific_carrier_list_length;
  uplink_channel_bw_per_scs_list->scs_specific_carrier =
      calloc_or_fail(scs_specific_carrier_list_length, sizeof(*uplink_channel_bw_per_scs_list->scs_specific_carrier));
  for (int i = 0; i < scs_specific_carrier_list_length; i++) {
    NRPPA_SCS_SpecificCarrier_t *nrppa_scs_specific_carrier = nrppa_uplink_channel_bw_per_scs_list->list.array[i];
    nrppa_scs_specific_carrier_t *scs_specific_carrier = &uplink_channel_bw_per_scs_list->scs_specific_carrier[i];
    // offset to carrier
    scs_specific_carrier->offset_to_carrier = nrppa_scs_specific_carrier->offsetToCarrier;
    // subcarrier spacing
    switch (nrppa_scs_specific_carrier->subcarrierSpacing) {
      case NRPPA_SCS_SpecificCarrier__subcarrierSpacing_kHz15:
        scs_specific_carrier->subcarrier_spacing = NRPPA_SUBCARRIER_SPACING_15KHZ;
        break;
      case NRPPA_SCS_SpecificCarrier__subcarrierSpacing_kHz30:
        scs_specific_carrier->subcarrier_spacing = NRPPA_SUBCARRIER_SPACING_30KHZ;
        break;
      case NRPPA_SCS_SpecificCarrier__subcarrierSpacing_kHz60:
        scs_specific_carrier->subcarrier_spacing = NRPPA_SUBCARRIER_SPACING_60KHZ;
        break;
      case NRPPA_SCS_SpecificCarrier__subcarrierSpacing_kHz120:
        scs_specific_carrier->subcarrier_spacing = NRPPA_SUBCARRIER_SPACING_120KHZ;
        break;
      default:
        AssertFatal(false, "illegal subcarrier spacing %ld\n", nrppa_scs_specific_carrier->subcarrierSpacing);
        break;
    }
    // carrier bandwidth
    scs_specific_carrier->carrier_bandwidth = nrppa_scs_specific_carrier->carrierBandwidth;
  }

  // Active UL BWP
  const NRPPA_ActiveULBWP_t *nrppa_active_ul_bwp = &in_item->activeULBWP;
  nrppa_active_ul_bwp_t *active_ul_bwp = &out_item->active_ul_bwp;

  // location and bandwidth
  active_ul_bwp->location_and_bandwidth = nrppa_active_ul_bwp->locationAndBandwidth;
  // subcarrier spacing
  switch (nrppa_active_ul_bwp->subcarrierSpacing) {
    case NRPPA_ActiveULBWP__subcarrierSpacing_kHz15:
      active_ul_bwp->subcarrier_spacing = NRPPA_SUBCARRIER_SPACING_15KHZ;
      break;
    case NRPPA_ActiveULBWP__subcarrierSpacing_kHz30:
      active_ul_bwp->subcarrier_spacing = NRPPA_SUBCARRIER_SPACING_30KHZ;
      break;
    case NRPPA_ActiveULBWP__subcarrierSpacing_kHz60:
      active_ul_bwp->subcarrier_spacing = NRPPA_SUBCARRIER_SPACING_60KHZ;
      break;
    case NRPPA_ActiveULBWP__subcarrierSpacing_kHz120:
      active_ul_bwp->subcarrier_spacing = NRPPA_SUBCARRIER_SPACING_120KHZ;
      break;
    default:
      AssertFatal(false, "illegal subcarrier spacing %ld\n", nrppa_active_ul_bwp->subcarrierSpacing);
      break;
  }

  // cyclic prefix
  if (nrppa_active_ul_bwp->cyclicPrefix == NRPPA_ActiveULBWP__cyclicPrefix_normal)
    active_ul_bwp->cyclic_prefix = NRPPA_CP_TYPE_NORMAL;
  else
    active_ul_bwp->cyclic_prefix = NRPPA_CP_TYPE_EXTENDED;

  // Tx Direct Current Location
  active_ul_bwp->tx_direct_current_location = nrppa_active_ul_bwp->txDirectCurrentLocation;

  // SRS Config
  const NRPPA_SRSConfig_t *nrppa_sRSConfig = &nrppa_active_ul_bwp->sRSConfig;
  nrppa_srs_config_t *sRSConfig = &active_ul_bwp->srs_config;
  decode_srs_config(nrppa_sRSConfig, sRSConfig);
}

void decode_srs_carrier_list(nrppa_srs_carrier_list_t *out_list, const NRPPA_SRSCarrier_List_t *in_list)
{
  uint32_t list_len = in_list->list.count;
  AssertFatal(list_len > 0, "atleast 1 SRS carrier list must be present");
  out_list->srs_carrier_list_length = list_len;
  out_list->srs_carrier_list_item = calloc_or_fail(list_len, sizeof(*out_list->srs_carrier_list_item));
  for (int i = 0; i < list_len; i++) {
    NRPPA_SRSCarrier_List_Item_t *in_item = in_list->list.array[i];
    nrppa_srs_carrier_list_item_t *out_item = &out_list->srs_carrier_list_item[i];
    decode_srs_carrier_list_item(in_item, out_item);
  }
}

void free_nrppa_measurement_request(nrppa_measurement_req_t *msg)
{
  free(msg->trp_measurement_request_list.trp_measurement_request_item);
  if (msg->measurement_quantities.measurement_quantities_item) {
    free(msg->measurement_quantities.measurement_quantities_item);
  }

  /* SRS Configuration (O) */
  if (msg->srs_configuration) {
    nrppa_srs_carrier_list_t *srs_carrier_list = &msg->srs_configuration->srs_carrier_list;
    free_nrppa_srs_carrier_list(srs_carrier_list);
    free(msg->srs_configuration);
  }
}

static NRPPA_TrpMeasurementResult_t encode_measurement_results(nrppa_measurement_result_t *in_result)
{
  NRPPA_TrpMeasurementResult_t out_result = {0};
  for (int i = 0; i < in_result->measurement_result_item_length; i++) {
    asn1cSequenceAdd(out_result.list, NRPPA_TrpMeasurementResultItem_t, out_item);
    nrppa_measurement_result_item_t *in_item = &in_result->measurement_result_item[i];
    NRPPA_TrpMeasuredResultsValue_t *out_meas_res_val = &out_item->measuredResultsValue;
    nrppa_measured_results_value_t *in_meas_res_val = &in_item->measured_results_value;
    switch (in_meas_res_val->present) {
      case NRPPA_MEASURED_RESULTS_VALUE_PR_NOTHING:
        out_meas_res_val->present = NRPPA_TrpMeasuredResultsValue_PR_NOTHING;
        break;
      case NRPPA_MEASURED_RESULTS_VALUE_PR_UL_ANGLEOFARRIVAL:
        out_meas_res_val->present = NRPPA_TrpMeasuredResultsValue_PR_uL_AngleOfArrival;
        asn1cCalloc(out_meas_res_val->choice.uL_AngleOfArrival, out_ul_aoa);
        nrppa_ul_aoa_t *in_ul_aoa = &in_meas_res_val->choice.ul_angle_of_arrival;
        out_ul_aoa->azimuthAoA = in_ul_aoa->azimuth_aoa;
        if (in_ul_aoa->zenith_aoa) {
          asn1cCalloc(out_ul_aoa->zenithAoA, out_z_aoa);
          *out_z_aoa = *in_ul_aoa->zenith_aoa;
        }
        break;
      case NRPPA_MEASURED_RESULTS_VALUE_PR_UL_SRS_RSRP:
        out_meas_res_val->present = NRPPA_TrpMeasuredResultsValue_PR_uL_SRS_RSRP;
        out_meas_res_val->choice.uL_SRS_RSRP = in_meas_res_val->choice.ul_srs_rsrp;
        break;
      case NRPPA_MEASURED_RESULTS_VALUE_PR_UL_RTOA:
        out_meas_res_val->present = NRPPA_TrpMeasuredResultsValue_PR_uL_RTOA;
        asn1cCalloc(out_meas_res_val->choice.uL_RTOA, out_ul_rtoa);
        nrppa_ul_rtoa_measurement_t *in_ul_rtoa = &in_meas_res_val->choice.ul_rtoa;
        switch (in_ul_rtoa->present) {
          case NRPPA_ULRTOAMEAS_PR_NOTHING:
            out_ul_rtoa->uLRTOAmeas.present = NRPPA_ULRTOAMeas_PR_NOTHING;
            break;
          case NRPPA_ULRTOAMEAS_PR_K0:
            out_ul_rtoa->uLRTOAmeas.present = NRPPA_ULRTOAMeas_PR_k0;
            out_ul_rtoa->uLRTOAmeas.choice.k0 = in_ul_rtoa->choice.k0;
            break;
          case NRPPA_ULRTOAMEAS_PR_K1:
            out_ul_rtoa->uLRTOAmeas.present = NRPPA_ULRTOAMeas_PR_k1;
            out_ul_rtoa->uLRTOAmeas.choice.k1 = in_ul_rtoa->choice.k1;
            break;
          case NRPPA_ULRTOAMEAS_PR_K2:
            out_ul_rtoa->uLRTOAmeas.present = NRPPA_ULRTOAMeas_PR_k2;
            out_ul_rtoa->uLRTOAmeas.choice.k2 = in_ul_rtoa->choice.k2;
            break;
          case NRPPA_ULRTOAMEAS_PR_K3:
            out_ul_rtoa->uLRTOAmeas.present = NRPPA_ULRTOAMeas_PR_k3;
            out_ul_rtoa->uLRTOAmeas.choice.k3 = in_ul_rtoa->choice.k3;
            break;
          case NRPPA_ULRTOAMEAS_PR_K4:
            out_ul_rtoa->uLRTOAmeas.present = NRPPA_ULRTOAMeas_PR_k4;
            out_ul_rtoa->uLRTOAmeas.choice.k4 = in_ul_rtoa->choice.k4;
            break;
          case NRPPA_ULRTOAMEAS_PR_K5:
            out_ul_rtoa->uLRTOAmeas.present = NRPPA_ULRTOAMeas_PR_k5;
            out_ul_rtoa->uLRTOAmeas.choice.k5 = in_ul_rtoa->choice.k5;
            break;
          default:
            AssertFatal(false, "illegal ul rtoa measure type\n");
            break;
        }
        break;
      case NRPPA_MEASURED_RESULTS_VALUE_PR_GNB_RXTXTIMEDIFF:
        out_meas_res_val->present = NRPPA_TrpMeasuredResultsValue_PR_gNB_RxTxTimeDiff;
        asn1cCalloc(out_meas_res_val->choice.gNB_RxTxTimeDiff, out_rxtx_time_diff);
        nrppa_gnb_rx_tx_time_diff_t *in_rxtx_time_diff = &in_meas_res_val->choice.gnb_rx_tx_time_diff;
        switch (in_rxtx_time_diff->present) {
          case NRPPA_GNBRXTXTIMEDIFFMEAS_PR_NOTHING:
            out_rxtx_time_diff->rxTxTimeDiff.present = NRPPA_GNBRxTxTimeDiffMeas_PR_NOTHING;
            break;
          case NRPPA_GNBRXTXTIMEDIFFMEAS_PR_K0:
            out_rxtx_time_diff->rxTxTimeDiff.present = NRPPA_GNBRxTxTimeDiffMeas_PR_k0;
            out_rxtx_time_diff->rxTxTimeDiff.choice.k0 = in_rxtx_time_diff->choice.k0;
            break;
          case NRPPA_GNBRXTXTIMEDIFFMEAS_PR_K1:
            out_rxtx_time_diff->rxTxTimeDiff.present = NRPPA_GNBRxTxTimeDiffMeas_PR_k1;
            out_rxtx_time_diff->rxTxTimeDiff.choice.k1 = in_rxtx_time_diff->choice.k1;
            break;
          case NRPPA_GNBRXTXTIMEDIFFMEAS_PR_K2:
            out_rxtx_time_diff->rxTxTimeDiff.present = NRPPA_GNBRxTxTimeDiffMeas_PR_k2;
            out_rxtx_time_diff->rxTxTimeDiff.choice.k2 = in_rxtx_time_diff->choice.k2;
            break;
          case NRPPA_GNBRXTXTIMEDIFFMEAS_PR_K3:
            out_rxtx_time_diff->rxTxTimeDiff.present = NRPPA_GNBRxTxTimeDiffMeas_PR_k3;
            out_rxtx_time_diff->rxTxTimeDiff.choice.k3 = in_rxtx_time_diff->choice.k3;
            break;
          case NRPPA_GNBRXTXTIMEDIFFMEAS_PR_K4:
            out_rxtx_time_diff->rxTxTimeDiff.present = NRPPA_GNBRxTxTimeDiffMeas_PR_k4;
            out_rxtx_time_diff->rxTxTimeDiff.choice.k4 = in_rxtx_time_diff->choice.k4;
            break;
          case NRPPA_GNBRXTXTIMEDIFFMEAS_PR_K5:
            out_rxtx_time_diff->rxTxTimeDiff.present = NRPPA_GNBRxTxTimeDiffMeas_PR_k5;
            out_rxtx_time_diff->rxTxTimeDiff.choice.k5 = in_rxtx_time_diff->choice.k5;
            break;
          default:
            AssertFatal(false, "illegal ul rtoa measure type\n");
            break;
        }
        break;
      default:
        AssertFatal(false, "illegal measured results type\n");
        break;
    }

    NRPPA_TimeStamp_t *out_time_stamp = &out_item->timeStamp;
    nrppa_time_stamp_t *in_time_stamp = &in_item->time_stamp;
    out_time_stamp->systemFrameNumber = in_time_stamp->system_frame_number;
    switch (in_time_stamp->slot_index.present) {
      case NRPPA_TIME_STAMP_SLOT_INDEX_PR_NOTHING:
        out_time_stamp->slotIndex.present = NRPPA_TimeStampSlotIndex_PR_NOTHING;
        break;
      case NRPPA_TIME_STAMP_SLOT_INDEX_PR_SCS_15:
        out_time_stamp->slotIndex.present = NRPPA_TimeStampSlotIndex_PR_sCS_15;
        out_time_stamp->slotIndex.choice.sCS_15 = in_time_stamp->slot_index.choice.scs_15;
        break;
      case NRPPA_TIME_STAMP_SLOT_INDEX_PR_SCS_30:
        out_time_stamp->slotIndex.present = NRPPA_TimeStampSlotIndex_PR_sCS_30;
        out_time_stamp->slotIndex.choice.sCS_30 = in_time_stamp->slot_index.choice.scs_30;
        break;
      case NRPPA_TIME_STAMP_SLOT_INDEX_PR_SCS_60:
        out_time_stamp->slotIndex.present = NRPPA_TimeStampSlotIndex_PR_sCS_60;
        out_time_stamp->slotIndex.choice.sCS_60 = in_time_stamp->slot_index.choice.scs_60;
        break;
      case NRPPA_TIME_STAMP_SLOT_INDEX_PR_SCS_120:
        out_time_stamp->slotIndex.present = NRPPA_TimeStampSlotIndex_PR_sCS_120;
        out_time_stamp->slotIndex.choice.sCS_120 = in_time_stamp->slot_index.choice.scs_120;
        break;
      default:
        AssertFatal(false, "illegal slot index\n");
        break;
    }
  }
  return out_result;
}

NRPPA_TRP_MeasurementResponseList_t encode_trp_measurement_reponse_list(nrppa_measurement_response_list_t *in_list)
{
  NRPPA_TRP_MeasurementResponseList_t out_list = {0};
  for (int i = 0; i < in_list->measurement_response_list_length; i++) {
    asn1cSequenceAdd(out_list.list, NRPPA_TRP_MeasurementResponseItem_t, out_item);
    nrppa_measurement_response_item_t *in_item = &in_list->measurement_response_item[i];
    out_item->tRP_ID = in_item->trp_id;
    out_item->measurementResult = encode_measurement_results(&in_item->measurement_result);
  }

  return out_list;
}

void free_nrppa_measurement_resp(nrppa_measurement_resp_t *msg)
{
  if (msg->measurement_response_list) {
    nrppa_measurement_response_list_t *list = msg->measurement_response_list;
    uint32_t meas_resp_list_len = list->measurement_response_list_length;
    for (int i = 0; i < meas_resp_list_len; i++) {
      nrppa_measurement_response_item_t *meas_response_item = &list->measurement_response_item[i];
      nrppa_measurement_result_t *MeasurementResult = &meas_response_item->measurement_result;
      uint32_t meas_result_length = MeasurementResult->measurement_result_item_length;
      for (int j = 0; j < meas_result_length; j++) {
        nrppa_measurement_result_item_t *item = &MeasurementResult->measurement_result_item[j];
        nrppa_measured_results_value_t *measuredResultsValue = &item->measured_results_value;
        if (measuredResultsValue->present == NRPPA_MEASURED_RESULTS_VALUE_PR_UL_ANGLEOFARRIVAL) {
          if (measuredResultsValue->choice.ul_angle_of_arrival.zenith_aoa) {
            free(measuredResultsValue->choice.ul_angle_of_arrival.zenith_aoa);
          }
        }
      }
      free(MeasurementResult->measurement_result_item);
    }
    free(msg->measurement_response_list->measurement_response_item);
    free(msg->measurement_response_list);
  }
}

int nrppa_gNB_handle_measurement_request(nrppa_gnb_ue_info_t *nrppa_msg_info, const NRPPA_NRPPA_PDU_t *pdu)
{
  DevAssert(pdu != NULL);
  DevAssert(nrppa_msg_info != NULL);

  LOG_I(NRPPA, "Processing Received Measurement Request \n");
  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, pdu);
  }

  // Forward request to RRC
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, NRPPA_MEASUREMENT_REQ);
  nrppa_measurement_req_t *req = &NRPPA_MEASUREMENT_REQ(msg);

  // Processing Received TRPInformationRequest
  NRPPA_MeasurementRequest_t *container = NULL;
  NRPPA_MeasurementRequest_IEs_t *ie = NULL;

  // IE 9.2.3 Message type : mandatory
  container = &pdu->choice.initiatingMessage->value.choice.MeasurementRequest;

  // IE 9.2.4 nrppatransactionID : mandatory
  req->transaction_id = pdu->choice.initiatingMessage->nrppatransactionID;

  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_MeasurementRequest_IEs_t, ie, container, NRPPA_ProtocolIE_ID_id_LMF_Measurement_ID, true);

  // LMF Measurement  ID : mandatory
  req->lmf_measurement_id = ie->value.choice.Measurement_ID;

  // IE TRP Measurement Request List : mandatory
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_MeasurementRequest_IEs_t,
                              ie,
                              container,
                              NRPPA_ProtocolIE_ID_id_TRP_MeasurementRequestList,
                              true);
  NRPPA_TRP_MeasurementRequestList_t *nrppa_trp_meas_list = &ie->value.choice.TRP_MeasurementRequestList;
  uint8_t meas_req_item_len = nrppa_trp_meas_list->list.count;
  AssertError(meas_req_item_len > 0, return false, "at least 1 TRP Measurement Request Item must be present");
  nrppa_trp_measurement_request_list_t *trp_meas_list = &req->trp_measurement_request_list;
  trp_meas_list->trp_measurement_request_list_length = meas_req_item_len;
  trp_meas_list->trp_measurement_request_item =
      calloc_or_fail(meas_req_item_len, sizeof(*trp_meas_list->trp_measurement_request_item));
  for (int i = 0; i < meas_req_item_len; i++) {
    trp_meas_list->trp_measurement_request_item[i].trp_id = nrppa_trp_meas_list->list.array[i]->tRP_ID;
  }

  // IE Report Characteristics : mandatory
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_MeasurementRequest_IEs_t, ie, container, NRPPA_ProtocolIE_ID_id_ReportCharacteristics, true);
  switch (ie->value.choice.ReportCharacteristics) {
    case NRPPA_ReportCharacteristics_onDemand:
      req->report_characteristics = NRPPA_POSREPORTCHARACTERISTICS_ONDEMAND;
      break;
    case NRPPA_ReportCharacteristics_periodic:
      req->report_characteristics = NRPPA_POSREPORTCHARACTERISTICS_PERIODIC;
      break;
    default:
      AssertError(false, return false, "illegal report characteristics\n");
      break;
  }

  // IE Measurement Periodicity : C-if Report Charateristics periodic
  if (req->report_characteristics == NRPPA_POSREPORTCHARACTERISTICS_PERIODIC) {
    NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_MeasurementRequest_IEs_t, ie, container, NRPPA_ProtocolIE_ID_id_MeasurementPeriodicity, true);
    switch (ie->value.choice.MeasurementPeriodicity) {
      case NRPPA_MeasurementPeriodicity_ms120:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MS120;
        break;
      case NRPPA_MeasurementPeriodicity_ms240:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MS240;
        break;
      case NRPPA_MeasurementPeriodicity_ms480:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MS480;
        break;
      case NRPPA_MeasurementPeriodicity_ms640:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MS640;
        break;
      case NRPPA_MeasurementPeriodicity_ms1024:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MS1024;
        break;
      case NRPPA_MeasurementPeriodicity_ms2048:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MS2048;
        break;
      case NRPPA_MeasurementPeriodicity_ms5120:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MS5120;
        break;
      case NRPPA_MeasurementPeriodicity_ms10240:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MS10240;
        break;
      case NRPPA_MeasurementPeriodicity_min1:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MIN1;
        break;
      case NRPPA_MeasurementPeriodicity_min6:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MIN6;
        break;
      case NRPPA_MeasurementPeriodicity_min12:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MIN12;
        break;
      case NRPPA_MeasurementPeriodicity_min30:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MIN30;
        break;
      case NRPPA_MeasurementPeriodicity_min60:
        req->measurement_periodicity = NRPPA_POSMEASUREMENTPERIODICITY_MIN60;
        break;
      default:
        AssertError(false, return false, "illegal measurement periodicity\n");
        break;
    }
  }

  // IE Measurement Quantity : mandatory (but not handled in OAI-LMF)
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_MeasurementRequest_IEs_t, ie, container, NRPPA_ProtocolIE_ID_id_MeasurementQuantities, false);
  if (ie == NULL) {
    LOG_W(NRPPA, "MeasurementQuantities is madatory but not handled in OAI LMF\n");
  } else {
    NRPPA_TRPMeasurementQuantities_t *nrppa_meas_q = &ie->value.choice.TRPMeasurementQuantities;
    uint32_t q_len = nrppa_meas_q->list.count;
    AssertError(q_len > 0, return false, "at least 1 TRP Measurement Quantity must be present");
    nrppa_measurement_quantities_t *meas_q = &req->measurement_quantities;
    meas_q->measurement_quantities_length = q_len;
    meas_q->measurement_quantities_item = calloc_or_fail(q_len, sizeof(*meas_q->measurement_quantities_item));
    nrppa_measurement_quantities_item_t *q_item = meas_q->measurement_quantities_item;
    for (int i = 0; i < meas_req_item_len; i++) {
      switch (nrppa_meas_q->list.array[i]->tRPMeasurementQuantities_Item) {
        case NRPPA_TRPMeasurementQuantities_Item_gNB_RxTxTimeDiff:
          q_item[i].measurement_type = NRPPA_POSMEASUREMENTTYPE_GNB_RX_TX;
          break;
        case NRPPA_TRPMeasurementQuantities_Item_uL_SRS_RSRP:
          q_item[i].measurement_type = NRPPA_POSMEASUREMENTTYPE_UL_SRS_RSRP;
          break;
        case NRPPA_TRPMeasurementQuantities_Item_uL_AoA:
          q_item[i].measurement_type = NRPPA_POSMEASUREMENTTYPE_UL_AOA;
          break;
        case NRPPA_TRPMeasurementQuantities_Item_uL_RTOA:
          q_item[i].measurement_type = NRPPA_POSMEASUREMENTTYPE_UL_RTOA;
          break;
        default:
          AssertError(false, return false, "illegal measurement quantity\n");
          break;
      }
    }
  }

  // IE SRS Configuration : optional
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_MeasurementRequest_IEs_t, ie, container, NRPPA_ProtocolIE_ID_id_SRSConfiguration, false);
  if (ie != NULL) {
    NRPPA_SRSCarrier_List_t *nrppa_srs_carrier_list = &ie->value.choice.SRSConfiguration.sRSCarrier_List;
    req->srs_configuration = calloc_or_fail(1, sizeof(*req->srs_configuration));
    nrppa_srs_carrier_list_t *srs_carrier_list = &req->srs_configuration->srs_carrier_list;
    decode_srs_carrier_list(srs_carrier_list, nrppa_srs_carrier_list);
  }

  nrppa_store_ue_context(nrppa_msg_info, req->transaction_id);

  LOG_I(NRPPA, "Forwarding to RRC Measurement Request transaction_id %d\n", req->transaction_id);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
  return 0;
}

int nrppa_gNB_measurement_response(instance_t instance, MessageDef *msg_p)
{
  DevAssert(msg_p);
  nrppa_measurement_resp_t *resp = &NRPPA_MEASUREMENT_RESP(msg_p);
  nrppa_gNB_ue_context_t *ue_info = nrppa_detach_ue_context(resp->transaction_id);

  if (ue_info->gNB_ue_ngap_id != 0 && ue_info->amf_ue_ngap_id != 0) {
    LOG_E(NRPPA, "Illegal gNB_ue_ngap_id %d and amf_ue_ngap_id %ld\n", ue_info->gNB_ue_ngap_id, ue_info->amf_ue_ngap_id);
    free_nrppa_measurement_resp(resp);
    nrppa_free_ue_context(ue_info);
    return -1;
  }

  LOG_I(NRPPA,
        "Received measurement response info from RRC with transaction_id %u and gNB_ue_ngap_id %u\n",
        ue_info->transaction_id,
        ue_info->gNB_ue_ngap_id);

  // Prepare NRPPA TRP Information transfer Response
  NRPPA_NRPPA_PDU_t pdu = {0};

  // IE: 9.2.3 Message Type : mandatory
  pdu.present = NRPPA_NRPPA_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, head);
  head->procedureCode = NRPPA_ProcedureCode_id_Measurement;
  head->criticality = NRPPA_Criticality_reject;
  head->value.present = NRPPA_SuccessfulOutcome__value_PR_MeasurementResponse;

  // IE 9.2.4 nrppatransactionID : mandatory
  head->nrppatransactionID = resp->transaction_id;
  NRPPA_MeasurementResponse_t *out = &head->value.choice.MeasurementResponse;

  // IE LMF Measurement ID : mandatory
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_MeasurementResponse_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_LMF_Measurement_ID;
    ie->criticality = NRPPA_Criticality_reject;
    ie->value.present = NRPPA_MeasurementResponse_IEs__value_PR_Measurement_ID;
    ie->value.choice.Measurement_ID = resp->lmf_measurement_id;
  }

  // IE RAN Measurement ID : mandatory
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_MeasurementResponse_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_RAN_Measurement_ID;
    ie->criticality = NRPPA_Criticality_reject;
    ie->value.present = NRPPA_MeasurementResponse_IEs__value_PR_Measurement_ID_1;
    ie->value.choice.Measurement_ID_1 = resp->ran_measurement_id;
  }

  // IE TRP Measurement Response List : mandatory
  {
    if (resp->measurement_response_list) {
      asn1cSequenceAdd(out->protocolIEs.list, NRPPA_MeasurementResponse_IEs_t, ie);
      ie->id = NRPPA_ProtocolIE_ID_id_TRP_MeasurementResponseList;
      ie->criticality = NRPPA_Criticality_reject;
      ie->value.present = NRPPA_MeasurementResponse_IEs__value_PR_TRP_MeasurementResponseList;
      ie->value.choice.TRP_MeasurementResponseList = encode_trp_measurement_reponse_list(resp->measurement_response_list);
    }
  }

  free_nrppa_measurement_resp(resp);

  LOG_I(NRPPA, "Calling encoder for Measurement Response \n");

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &pdu);
  }

  // Encode NRPPA message
  uint8_t *buffer = NULL;
  uint32_t length = 0;
  if (nrppa_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    LOG_E(NRPPA, "Failed to encode Uplink NRPPa Measurement Response\n");
    nrppa_free_ue_context(ue_info);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
    return -1;
  }

  MessageDef *msg = itti_alloc_new_message(TASK_NRPPA, 0, NGAP_UPLINKNONUEASSOCIATEDNRPPA);
  ngap_uplink_non_ue_associated_nrppa_t *ULNRPPA = &NGAP_UPLINKNONUEASSOCIATEDNRPPA(msg);

  // Routing ID
  ULNRPPA->routing_id = create_byte_array(ue_info->routing_id.len, ue_info->routing_id.buf);

  // NRPPa PDU
  ULNRPPA->nrppa_pdu = create_byte_array(length, buffer);

  // Forward the NRPPA PDU to NGAP
  itti_send_msg_to_task(TASK_NGAP, instance, msg);
  nrppa_free_ue_context(ue_info);
  free(buffer);
  return length;
}

int nrppa_gNB_measurement_failure(instance_t instance, MessageDef *msg_p)
{
  DevAssert(msg_p);
  nrppa_measurement_failure_t *fail = &NRPPA_MEASUREMENT_FAILURE(msg_p);
  nrppa_gNB_ue_context_t *ue_info = nrppa_detach_ue_context(fail->transaction_id);

  if (ue_info->gNB_ue_ngap_id != 0 && ue_info->amf_ue_ngap_id != 0) {
    LOG_E(NRPPA, "Illegal gNB_ue_ngap_id %d and amf_ue_ngap_id %ld\n", ue_info->gNB_ue_ngap_id, ue_info->amf_ue_ngap_id);
    nrppa_free_ue_context(ue_info);
    return -1;
  }

  LOG_I(NRPPA,
        "Received measurement failure info from RRC with transaction_id %u and gNB_ue_ngap_id %u\n",
        ue_info->transaction_id,
        ue_info->gNB_ue_ngap_id);

  // Prepare NRPPA TRP Information transfer Response
  NRPPA_NRPPA_PDU_t pdu = {0};

  // IE: 9.2.3 Message Type : mandatory
  pdu.present = NRPPA_NRPPA_PDU_PR_unsuccessfulOutcome;
  asn1cCalloc(pdu.choice.unsuccessfulOutcome, head);
  head->procedureCode = NRPPA_ProcedureCode_id_Measurement;
  head->criticality = NRPPA_Criticality_reject;
  head->value.present = NRPPA_UnsuccessfulOutcome__value_PR_MeasurementFailure;

  // IE 9.2.4 nrppatransactionID : mandatory
  head->nrppatransactionID = fail->transaction_id;
  NRPPA_MeasurementFailure_t *out = &head->value.choice.MeasurementFailure;

  // IE LMF Measurement ID : mandatory
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_MeasurementFailure_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_LMF_Measurement_ID;
    ie->criticality = NRPPA_Criticality_reject;
    ie->value.present = NRPPA_MeasurementFailure_IEs__value_PR_Measurement_ID;
    ie->value.choice.Measurement_ID = fail->lmf_measurement_id;
  }

  // IE Cause : mandatory
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_MeasurementFailure_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_Cause;
    ie->criticality = NRPPA_Criticality_reject;
    ie->value.present = NRPPA_MeasurementFailure_IEs__value_PR_Cause;
    ie->value.choice.Cause = encode_nrppa_cause(fail->cause);
  }

  LOG_I(NRPPA, "Calling encoder for Measurement Failure \n");

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &pdu);
  }

  // Encode NRPPA message
  uint8_t *buffer = NULL;
  uint32_t length = 0;
  if (nrppa_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    LOG_E(NRPPA, "Failed to encode Uplink NRPPa Measurement Failure\n");
    nrppa_free_ue_context(ue_info);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
    return -1;
  }

  MessageDef *msg = itti_alloc_new_message(TASK_NRPPA, 0, NGAP_UPLINKNONUEASSOCIATEDNRPPA);
  ngap_uplink_non_ue_associated_nrppa_t *ULNRPPA = &NGAP_UPLINKNONUEASSOCIATEDNRPPA(msg);

  // Routing ID
  ULNRPPA->routing_id = create_byte_array(ue_info->routing_id.len, ue_info->routing_id.buf);

  // NRPPa PDU
  ULNRPPA->nrppa_pdu = create_byte_array(length, buffer);

  // Forward the NRPPA PDU to NGAP
  itti_send_msg_to_task(TASK_NGAP, instance, msg);
  nrppa_free_ue_context(ue_info);
  free(buffer);
  return length;
}
