/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "intertask_interface.h"
#include "nrppa_common.h"
#include "nrppa_gNB_location_information_transfer.h"
#include "nrppa_gNB_ue_context.h"
#include "nrppa_messages_types.h"
#include "nrppa_gNB_encoder.h"
#include "openair3/UTILS/conversions.h"

void free_nrppa_trp_information_request(nrppa_trp_information_req_t *msg)
{
  if (msg->trp_information_type_list.trp_information_type_item) {
    free(msg->trp_information_type_list.trp_information_type_item);
  }
}

static NRPPA_TRPPositionDirect_t encode_trp_position_direct(nrppa_trp_position_direct_t *in)
{
  NRPPA_TRPPositionDirect_t out = {0};

  switch (in->accuracy.present) {
    case NRPPA_TRP_POSITION_DIRECT_ACCURACY_PR_NOTHING:
      out.accuracy.present = NRPPA_TRPPositionDirectAccuracy_PR_NOTHING;
      break;
    case NRPPA_TRP_POSITION_DIRECT_ACCURACY_PR_TRPPOSITION:
      out.accuracy.present = NRPPA_TRPPositionDirectAccuracy_PR_tRPPosition;
      asn1cCalloc(out.accuracy.choice.tRPPosition, out_pos);
      nrppa_access_point_position_t *in_pos = &in->accuracy.choice.trp_position;

      out_pos->latitudeSign = in_pos->latitude_sign;
      out_pos->latitude = in_pos->latitude;
      out_pos->longitude = in_pos->longitude;
      out_pos->directionOfAltitude = in_pos->direction_of_altitude;
      out_pos->altitude = in_pos->altitude;
      out_pos->uncertaintySemi_major = in_pos->uncertainty_semi_major;
      out_pos->uncertaintySemi_minor = in_pos->uncertainty_semi_minor;
      out_pos->orientationOfMajorAxis = in_pos->orientation_of_major_axis;
      out_pos->uncertaintyAltitude = in_pos->uncertainty_altitude;
      out_pos->confidence = in_pos->confidence;
      break;
    case NRPPA_TRP_POSITION_DIRECT_ACCURACY_PR_TRPHAPOSITION:
      out.accuracy.present = NRPPA_TRPPositionDirectAccuracy_PR_tRPHAposition;
      asn1cCalloc(out.accuracy.choice.tRPHAposition, out_ha_pos);
      nrppa_ngran_high_accuracy_access_point_position_t *in_ha_pos = &in->accuracy.choice.trp_HAposition;
      out_ha_pos->latitude = in_ha_pos->latitude;
      out_ha_pos->longitude = in_ha_pos->longitude;
      out_ha_pos->altitude = in_ha_pos->altitude;
      out_ha_pos->uncertaintySemi_major = in_ha_pos->uncertainty_semi_major;
      out_ha_pos->uncertaintySemi_minor = in_ha_pos->uncertainty_semi_minor;
      out_ha_pos->orientationOfMajorAxis = in_ha_pos->orientation_of_major_axis;
      out_ha_pos->horizontalConfidence = in_ha_pos->horizontal_confidence;
      out_ha_pos->uncertaintyAltitude = in_ha_pos->uncertainty_altitude;
      out_ha_pos->verticalConfidence = in_ha_pos->vertical_confidence;
      break;
    default:
      AssertFatal(false, "illegal direct accuracy entry\n");
      break;
  }
  return out;
}

static NRPPA_TRPPositionReferenced_t encode_trp_position_referenced(nrppa_trp_position_referenced_t *in)
{
  NRPPA_TRPPositionReferenced_t out = {0};
  NRPPA_ReferencePoint_t *out_ref_point = &out.referencePoint;
  nrppa_reference_point_t *in_ref_point = &in->reference_point;
  switch (in_ref_point->present) {
    case NRPPA_REFERENCE_POINT_PR_NOTHING:
      out_ref_point->present = NRPPA_ReferencePoint_PR_NOTHING;
      break;
    case NRPPA_REFERENCE_POINT_PR_COORDINATEID:
      out_ref_point->present = NRPPA_ReferencePoint_PR_relativeCoordinateID;
      out_ref_point->choice.relativeCoordinateID = in_ref_point->choice.coordinate_id;
      break;
    case NRPPA_REFERENCE_POINT_PR_REFERENCEPOINTCOORDINATE:
      out_ref_point->present = NRPPA_ReferencePoint_PR_referencePointCoordinate;
      asn1cCalloc(out_ref_point->choice.referencePointCoordinate, out_ref_coord);
      nrppa_access_point_position_t *in_ref_coord = &in_ref_point->choice.reference_point_coordinate;
      out_ref_coord->latitudeSign = in_ref_coord->latitude_sign;
      out_ref_coord->latitude = in_ref_coord->latitude;
      out_ref_coord->longitude = in_ref_coord->longitude;
      out_ref_coord->directionOfAltitude = in_ref_coord->direction_of_altitude;
      out_ref_coord->altitude = in_ref_coord->altitude;
      out_ref_coord->uncertaintySemi_major = in_ref_coord->uncertainty_semi_major;
      out_ref_coord->uncertaintySemi_minor = in_ref_coord->uncertainty_semi_minor;
      out_ref_coord->orientationOfMajorAxis = in_ref_coord->orientation_of_major_axis;
      out_ref_coord->uncertaintyAltitude = in_ref_coord->uncertainty_altitude;
      out_ref_coord->confidence = in_ref_coord->confidence;
      break;
    case NRPPA_REFERENCE_POINT_PR_REFERENCEPOINTCOORDINATEHA:
      out_ref_point->present = NRPPA_ReferencePoint_PR_referencePointCoordinateHA;
      asn1cCalloc(out_ref_point->choice.referencePointCoordinateHA, out_ref_coord_ha);
      nrppa_ngran_high_accuracy_access_point_position_t *in_ref_coord_ha = &in_ref_point->choice.reference_point_coordinateHA;
      out_ref_coord_ha->latitude = in_ref_coord_ha->latitude;
      out_ref_coord_ha->longitude = in_ref_coord_ha->longitude;
      out_ref_coord_ha->altitude = in_ref_coord_ha->altitude;
      out_ref_coord_ha->uncertaintySemi_major = in_ref_coord_ha->uncertainty_semi_major;
      out_ref_coord_ha->uncertaintySemi_minor = in_ref_coord_ha->uncertainty_semi_minor;
      out_ref_coord_ha->orientationOfMajorAxis = in_ref_coord_ha->orientation_of_major_axis;
      out_ref_coord_ha->horizontalConfidence = in_ref_coord_ha->horizontal_confidence;
      out_ref_coord_ha->uncertaintyAltitude = in_ref_coord_ha->uncertainty_altitude;
      out_ref_coord_ha->verticalConfidence = in_ref_coord_ha->vertical_confidence;
      break;
    default:
      AssertFatal(false, "illegal reference point entry\n");
      break;
  }

  NRPPA_TRPReferencePointType_t *out_ref_point_type = &out.referencePointType;
  nrppa_trp_reference_point_type_t *in_ref_point_type = &in->reference_point_type;
  switch (in_ref_point_type->present) {
    case NRPPA_TRP_REFERENCE_POINT_TYPE_PR_NOTHING:
      out_ref_point_type->present = NRPPA_TRPReferencePointType_PR_NOTHING;
      break;
    case NRPPA_TRP_REFERENCE_POINT_TYPE_PR_TRPPOSITION_RELATIVE_GEODETIC:
      out_ref_point_type->present = NRPPA_TRPReferencePointType_PR_tRPPositionRelativeGeodetic;
      asn1cCalloc(out_ref_point_type->choice.tRPPositionRelativeGeodetic, out_geodetic);
      nrppa_relative_geodetic_location_t *in_geodetic = &in_ref_point_type->choice.trp_position_relative_geodetic;
      out_geodetic->milli_Arc_SecondUnits = in_geodetic->milli_arc_second_units;
      out_geodetic->heightUnits = in_geodetic->height_units;
      out_geodetic->deltaLatitude = in_geodetic->delta_latitude;
      out_geodetic->deltaLongitude = in_geodetic->delta_longitude;
      out_geodetic->deltaHeight = in_geodetic->delta_height;
      NRPPA_LocationUncertainty_t *out_loc_uncertainty_g = &out_geodetic->locationUncertainty;
      nrppa_location_uncertainty_t *in_loc_uncertainty_g = &in_geodetic->location_uncertainty;
      out_loc_uncertainty_g->horizontalUncertainty = in_loc_uncertainty_g->horizontal_uncertainty;
      out_loc_uncertainty_g->horizontalConfidence = in_loc_uncertainty_g->horizontal_confidence;
      out_loc_uncertainty_g->verticalUncertainty = in_loc_uncertainty_g->vertical_uncertainty;
      out_loc_uncertainty_g->verticalConfidence = in_loc_uncertainty_g->vertical_confidence;
      break;
    case NRPPA_TRP_REFERENCE_POINT_TYPE_PR_TRPPOSITION_RELATIVE_CARTESIAN:
      out_ref_point_type->present = NRPPA_TRPReferencePointType_PR_tRPPositionRelativeCartesian;
      asn1cCalloc(out_ref_point_type->choice.tRPPositionRelativeCartesian, out_cartesian);
      nrppa_relative_cartesian_location_t *in_cartesian = &in_ref_point_type->choice.trp_position_relative_cartesian;

      out_cartesian->xYZunit = in_cartesian->xyz_unit;
      out_cartesian->xvalue = in_cartesian->xvalue;
      out_cartesian->yvalue = in_cartesian->yvalue;
      out_cartesian->zvalue = in_cartesian->zvalue;
      NRPPA_LocationUncertainty_t *out_loc_uncertainty_c = &out_cartesian->locationUncertainty;
      nrppa_location_uncertainty_t *in_loc_uncertainty_c = &in_cartesian->location_uncertainty;
      out_loc_uncertainty_c->horizontalUncertainty = in_loc_uncertainty_c->horizontal_uncertainty;
      out_loc_uncertainty_c->horizontalConfidence = in_loc_uncertainty_c->horizontal_confidence;
      out_loc_uncertainty_c->verticalUncertainty = in_loc_uncertainty_c->vertical_uncertainty;
      out_loc_uncertainty_c->verticalConfidence = in_loc_uncertainty_c->vertical_confidence;
      break;
    default:
      AssertFatal(false, "illegal trp reference point type entry\n");
      break;
  }
  return out;
}

static NRPPA_GeographicalCoordinates_t encode_geographical_coordinates_nrppa(nrppa_geographical_coordinates_t *in)
{
  NRPPA_GeographicalCoordinates_t out = {0};
  NRPPA_TRPPositionDefinitionType_t *out_type = &out.tRPPositionDefinitionType;
  nrppa_trp_position_definition_type_t *in_type = &in->trp_position_definition_type;
  switch (in_type->present) {
    case NRPPA_TRP_POSITION_DEFINITION_TYPE_PR_NOTHING:
      out_type->present = NRPPA_TRPPositionDefinitionType_PR_NOTHING;
      break;
    case NRPPA_TRP_POSITION_DEFINITION_TYPE_PR_DIRECT:
      out_type->present = NRPPA_TRPPositionDefinitionType_PR_direct;
      asn1cCalloc(out_type->choice.direct, direct);
      *direct = encode_trp_position_direct(&in_type->choice.direct);
      break;
    case NRPPA_TRP_POSITION_DEFINITION_TYPE_PR_REFERENCED:
      out_type->present = NRPPA_TRPPositionDefinitionType_PR_referenced;
      asn1cCalloc(out_type->choice.referenced, referenced);
      *referenced = encode_trp_position_referenced(&in_type->choice.referenced);
      break;
    default:
      AssertFatal(false, "illegal Geographical Coordinates entry\n");
      break;
  }
  return out;
}

NRPPA_TRPInformationTypeResponseItem_t encode_trp_info_type_response_item_nrppa(nrppa_trp_information_type_response_item_t *in)
{
  NRPPA_TRPInformationTypeResponseItem_t out = {0};
  switch (in->present) {
    case NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_NOTHING:
      out.present = NRPPA_TRPInformationTypeResponseItem_PR_NOTHING;
      break;
    case NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_PCI_NR:
      out.present = NRPPA_TRPInformationTypeResponseItem_PR_pCI_NR;
      out.choice.pCI_NR = in->choice.pci_nr;
      break;
    case NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_NG_RAN_CGI:
      out.present = NRPPA_TRPInformationTypeResponseItem_PR_cGI_NR;
      asn1cCalloc(out.choice.cGI_NR, cgi_nr);
      MCC_MNC_TO_PLMNID(in->choice.ng_ran_cgi.plmn.mcc,
                        in->choice.ng_ran_cgi.plmn.mnc,
                        in->choice.ng_ran_cgi.plmn.mnc_digit_length,
                        &(cgi_nr->pLMN_Identity));
      NR_CELL_ID_TO_BIT_STRING(in->choice.ng_ran_cgi.nr_cellid, &(cgi_nr->nRcellIdentifier));
      break;
    case NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_NRARFCN:
      out.present = NRPPA_TRPInformationTypeResponseItem_PR_aRFCN;
      out.choice.aRFCN = in->choice.nr_arfcn;
      break;
    case NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_PRSCONFIGURATION:
      out.present = NRPPA_TRPInformationTypeResponseItem_PR_pRSConfiguration;
      AssertFatal(false, "TRP information type response item PRS configuration unsupported\n");
      break;
    case NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_SSBINFORMATION:
      out.present = NRPPA_TRPInformationTypeResponseItem_PR_sSBinformation;
      AssertFatal(false, "TRP information type response item SSB Information unsupported\n");
      break;
    case NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_SFNINITIALISATIONTIME:
      out.present = NRPPA_TRPInformationTypeResponseItem_PR_sFNInitialisationTime;
      AssertFatal(false, "TRP information type response item SFN Initialization Time unsupported\n");
      break;
    case NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_SPATIALDIRECTIONINFORMATION:
      out.present = NRPPA_TRPInformationTypeResponseItem_PR_spatialDirectionInformation;
      AssertFatal(false, "TRP information type response item Spatial Direction Information unsupported\n");
      break;
    case NRPPA_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_GEOGRAPHICALCOORDINATES:
      out.present = NRPPA_TRPInformationTypeResponseItem_PR_geographicalCoordinates;
      asn1cCalloc(out.choice.geographicalCoordinates, nrppa_geo_coord);
      nrppa_geographical_coordinates_t *geo_coord = &in->choice.geographical_coordinates;
      *nrppa_geo_coord = encode_geographical_coordinates_nrppa(geo_coord);
      break;
    default:
      AssertFatal(false, "received illegal trp information type response item %d\n", in->present);
      break;
  }
  return out;
}

void free_nrppa_trp_information_response(nrppa_trp_information_resp_t *msg)
{
  nrppa_trp_information_list_t *trp_information_list = &msg->trp_information_list;
  uint32_t trp_info_item_length = trp_information_list->trp_information_item_length;
  for (int i = 0; i < trp_info_item_length; i++) {
    nrppa_trp_information_t *trp_information_item = &trp_information_list->trp_information_item[i];
    nrppa_trp_information_type_response_list_t *trp_info_type_resp_list = &trp_information_item->trp_information_type_response_list;
    free(trp_info_type_resp_list->trp_information_type_response_item);
  }
  free(msg->trp_information_list.trp_information_item);
}

static NRPPA_SRSConfig_t encode_srs_config(const nrppa_srs_config_t *in_config)
{
  NRPPA_SRSConfig_t out_config = {0};
  // optional: srs_resource_list
  if (in_config->srs_resource_list) {
    nrppa_srs_resource_list_t *srs_resource_list = in_config->srs_resource_list;
    uint32_t srs_resource_list_length = srs_resource_list->srs_resource_list_length;
    asn1cCalloc(out_config.sRSResource_List, nrppa_sRSResource_List);
    for (int i = 0; i < srs_resource_list_length; i++) {
      asn1cSequenceAdd(nrppa_sRSResource_List->list, NRPPA_SRSResource_t, nrppa_srs_resource);
      nrppa_srs_resource_t *srs_resource = &srs_resource_list->srs_resource[i];
      nrppa_srs_resource->sRSResourceID = srs_resource->srs_resource_id;

      switch (srs_resource->nr_of_srs_ports) {
        case NRPPA_SRS_NUMBER_OF_PORTS_N1:
          nrppa_srs_resource->nrofSRS_Ports = NRPPA_SRSResource__nrofSRS_Ports_port1;
          break;
        case NRPPA_SRS_NUMBER_OF_PORTS_N2:
          nrppa_srs_resource->nrofSRS_Ports = NRPPA_SRSResource__nrofSRS_Ports_ports2;
          break;
        case NRPPA_SRS_NUMBER_OF_PORTS_N4:
          nrppa_srs_resource->nrofSRS_Ports = NRPPA_SRSResource__nrofSRS_Ports_ports4;
          break;
        default:
          AssertFatal(false, "illegal number of srs ports %d\n", srs_resource->nr_of_srs_ports);
          break;
      }

      nrppa_transmission_comb_t *srs_res_tx_comb = &srs_resource->transmission_comb;
      switch (srs_res_tx_comb->present) {
        case NRPPA_TRANSMISSION_COMB_PR_NOTHING:
          nrppa_srs_resource->transmissionComb.present = NRPPA_TransmissionComb_PR_NOTHING;
          break;
        case NRPPA_TRANSMISSION_COMB_PR_N2:
          nrppa_srs_resource->transmissionComb.present = NRPPA_TransmissionComb_PR_n2;
          asn1cCalloc(nrppa_srs_resource->transmissionComb.choice.n2, nrppa_n2);
          nrppa_n2->combOffset_n2 = srs_res_tx_comb->choice.n2.comb_offset_n2;
          nrppa_n2->cyclicShift_n2 = srs_res_tx_comb->choice.n2.cyclic_shift_n2;
          break;
        case NRPPA_TRANSMISSION_COMB_PR_N4:
          nrppa_srs_resource->transmissionComb.present = NRPPA_TransmissionComb_PR_n4;
          asn1cCalloc(nrppa_srs_resource->transmissionComb.choice.n4, nrppa_n4);
          nrppa_n4->combOffset_n4 = srs_res_tx_comb->choice.n4.comb_offset_n4;
          nrppa_n4->cyclicShift_n4 = srs_res_tx_comb->choice.n4.cyclic_shift_n4;
          break;
        default:
          AssertFatal(false, "illegal transmissionComb %d\n", srs_res_tx_comb->present);
          break;
      }

      nrppa_srs_resource->startPosition = srs_resource->start_position;

      switch (srs_resource->nr_of_symbols) {
        case NRPPA_SRS_NUMBER_OF_SYMBOLS_N1:
          nrppa_srs_resource->nrofSymbols = NRPPA_SRSResource__nrofSymbols_n1;
          break;
        case NRPPA_SRS_NUMBER_OF_SYMBOLS_N2:
          nrppa_srs_resource->nrofSymbols = NRPPA_SRSResource__nrofSymbols_n2;
          break;
        case NRPPA_SRS_NUMBER_OF_SYMBOLS_N4:
          nrppa_srs_resource->nrofSymbols = NRPPA_SRSResource__nrofSymbols_n4;
          break;
        default:
          AssertFatal(false, "illegal number of symbols %d\n", srs_resource->nr_of_symbols);
          break;
      }

      switch (srs_resource->repetition_factor) {
        case NRPPA_SRS_REPETITION_FACTOR_RF1:
          nrppa_srs_resource->repetitionFactor = NRPPA_SRSResource__repetitionFactor_n1;
          break;
        case NRPPA_SRS_REPETITION_FACTOR_RF2:
          nrppa_srs_resource->repetitionFactor = NRPPA_SRSResource__repetitionFactor_n2;
          break;
        case NRPPA_SRS_REPETITION_FACTOR_RF4:
          nrppa_srs_resource->repetitionFactor = NRPPA_SRSResource__repetitionFactor_n4;
          break;
        default:
          AssertFatal(false, "illegal repetition factor %d\n", srs_resource->repetition_factor);
          break;
      }

      nrppa_srs_resource->freqDomainPosition = srs_resource->freq_domain_position;
      nrppa_srs_resource->freqDomainShift = srs_resource->freq_domain_shift;
      nrppa_srs_resource->c_SRS = srs_resource->c_srs;
      nrppa_srs_resource->b_SRS = srs_resource->b_srs;
      nrppa_srs_resource->b_hop = srs_resource->b_hop;

      switch (srs_resource->group_or_sequence_hopping) {
        case NRPPA_GROUPORSEQUENCEHOPPING_NOTHING:
          nrppa_srs_resource->groupOrSequenceHopping = NRPPA_SRSResource__groupOrSequenceHopping_neither;
          break;
        case NRPPA_GROUPORSEQUENCEHOPPING_GROUPHOPPING:
          nrppa_srs_resource->groupOrSequenceHopping = NRPPA_SRSResource__groupOrSequenceHopping_groupHopping;
          break;
        case NRPPA_GROUPORSEQUENCEHOPPING_SEQUENCEHOPPING:
          nrppa_srs_resource->groupOrSequenceHopping = NRPPA_SRSResource__groupOrSequenceHopping_sequenceHopping;
          break;
        default:
          AssertFatal(false, "illegal groupOrSequenceHopping %d\n", srs_resource->group_or_sequence_hopping);
          break;
      }

      NRPPA_ResourceType_t *nrppa_resourceType = &nrppa_srs_resource->resourceType;
      nrppa_resource_type_t *resource_type = &srs_resource->resource_type;
      if (resource_type->present == NRPPA_RESOURCE_TYPE_PR_NOTHING) {
        nrppa_resourceType->present = NRPPA_ResourceType_PR_NOTHING;
      } else if (resource_type->present == NRPPA_RESOURCE_TYPE_PR_PERIODIC) {
        nrppa_resourceType->present = NRPPA_ResourceType_PR_periodic;
        asn1cCalloc(nrppa_resourceType->choice.periodic, nrppa_periodic);
        switch (resource_type->choice.periodic.periodicity) {
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT1:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot1;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT2:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot2;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT4:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot4;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT5:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot5;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT8:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot8;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT10:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot10;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT16:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot16;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT20:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot20;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT32:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot32;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT40:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot40;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT64:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot64;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT80:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot80;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT160:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot160;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT320:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot320;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT640:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot640;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT1280:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot1280;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT2560:
            nrppa_periodic->periodicity = NRPPA_ResourceTypePeriodic__periodicity_slot2560;
            break;
          default:
            AssertFatal(false, "illegal periodicity %d\n", resource_type->choice.periodic.periodicity);
            break;
        }
        nrppa_periodic->offset = resource_type->choice.periodic.offset;
      } else if (resource_type->present == NRPPA_RESOURCE_TYPE_PR_SEMI_PERSISTENT) {
        nrppa_resourceType->present = NRPPA_ResourceType_PR_semi_persistent;
        asn1cCalloc(nrppa_resourceType->choice.semi_persistent, nrppa_semi_persistent);
        switch (resource_type->choice.semi_persistent.periodicity) {
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT1:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot1;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT2:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot2;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT4:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot4;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT5:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot5;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT8:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot8;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT10:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot10;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT16:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot16;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT20:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot20;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT32:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot32;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT40:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot40;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT64:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot64;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT80:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot80;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT160:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot160;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT320:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot320;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT640:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot640;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT1280:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot1280;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_PERIODICITY_SLOT2560:
            nrppa_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistent__periodicity_slot2560;
            break;
          default:
            AssertFatal(false, "illegal periodicity %d\n", resource_type->choice.semi_persistent.periodicity);
            break;
        }
        nrppa_semi_persistent->offset = resource_type->choice.semi_persistent.offset;
      } else if (resource_type->present == NRPPA_RESOURCE_TYPE_PR_APERIODIC) {
        nrppa_resourceType->present = NRPPA_ResourceType_PR_aperiodic;
        asn1cCalloc(nrppa_resourceType->choice.aperiodic, nrppa_aperiodic);
        nrppa_aperiodic->aperiodicResourceType = resource_type->choice.aperiodic;
      } else {
        AssertFatal(false, "illegal resourceType %d\n", resource_type->present);
      }

      nrppa_srs_resource->sequenceId = srs_resource->sequence_id;
    }
  }

  // optional: pos_srs_resource_list
  if (in_config->pos_srs_resource_list) {
    nrppa_pos_srs_resource_list_t *pos_srs_resource_list = in_config->pos_srs_resource_list;
    uint32_t pos_srs_resource_list_length = pos_srs_resource_list->pos_srs_resource_list_length;
    asn1cCalloc(out_config.posSRSResource_List, nrppa_posSRSResource_List);
    for (int i = 0; i < pos_srs_resource_list_length; i++) {
      asn1cSequenceAdd(nrppa_posSRSResource_List->list, NRPPA_PosSRSResource_Item_t, nrppa_pos_srs_resource);
      nrppa_pos_srs_resource_item_t *pos_srs_resource = &pos_srs_resource_list->pos_srs_resource_item[i];
      nrppa_pos_srs_resource->srs_PosResourceId = pos_srs_resource->srs_pos_resource_id;

      nrppa_transmission_comb_pos_t *pos_srs_res_tx_comb = &pos_srs_resource->transmission_comb_pos;
      switch (pos_srs_res_tx_comb->present) {
        case NRPPA_TRANSMISSION_COMB_POS_PR_NOTHING:
          nrppa_pos_srs_resource->transmissionCombPos.present = NRPPA_TransmissionCombPos_PR_NOTHING;
          break;
        case NRPPA_TRANSMISSION_COMB_POS_PR_N2:
          nrppa_pos_srs_resource->transmissionCombPos.present = NRPPA_TransmissionCombPos_PR_n2;
          asn1cCalloc(nrppa_pos_srs_resource->transmissionCombPos.choice.n2, nrppa_pos_srs_resource_n2);
          nrppa_transmission_comb_pos_n2_t *pos_srs_resource_n2 = &pos_srs_res_tx_comb->choice.n2;
          nrppa_pos_srs_resource_n2->combOffset_n2 = pos_srs_resource_n2->comb_offset_n2;
          nrppa_pos_srs_resource_n2->cyclicShift_n2 = pos_srs_resource_n2->cyclic_shift_n2;
          break;
        case NRPPA_TRANSMISSION_COMB_POS_PR_N4:
          nrppa_pos_srs_resource->transmissionCombPos.present = NRPPA_TransmissionCombPos_PR_n4;
          asn1cCalloc(nrppa_pos_srs_resource->transmissionCombPos.choice.n4, nrppa_pos_srs_resource_n4);
          nrppa_transmission_comb_pos_n4_t *pos_srs_resource_n4 = &pos_srs_res_tx_comb->choice.n4;
          nrppa_pos_srs_resource_n4->combOffset_n4 = pos_srs_resource_n4->comb_offset_n4;
          nrppa_pos_srs_resource_n4->cyclicShift_n4 = pos_srs_resource_n4->cyclic_shift_n4;
          break;
        case NRPPA_TRANSMISSION_COMB_POS_PR_N8:
          nrppa_pos_srs_resource->transmissionCombPos.present = NRPPA_TransmissionCombPos_PR_n8;
          asn1cCalloc(nrppa_pos_srs_resource->transmissionCombPos.choice.n8, nrppa_pos_srs_resource_n8);
          nrppa_transmission_comb_pos_n8_t *pos_srs_resource_n8 = &pos_srs_res_tx_comb->choice.n8;
          nrppa_pos_srs_resource_n8->combOffset_n8 = pos_srs_resource_n8->comb_offset_n8;
          nrppa_pos_srs_resource_n8->cyclicShift_n8 = pos_srs_resource_n8->cyclic_shift_n8;
          break;
        default:
          AssertFatal(false, "illegal transmissionComb %d\n", pos_srs_res_tx_comb->present);
          break;
      }

      nrppa_pos_srs_resource->startPosition = pos_srs_resource->start_position;

      switch (pos_srs_resource->nr_of_symbols) {
        case NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N1:
          nrppa_pos_srs_resource->nrofSymbols = NRPPA_PosSRSResource_Item__nrofSymbols_n1;
          break;
        case NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N2:
          nrppa_pos_srs_resource->nrofSymbols = NRPPA_PosSRSResource_Item__nrofSymbols_n2;
          break;
        case NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N4:
          nrppa_pos_srs_resource->nrofSymbols = NRPPA_PosSRSResource_Item__nrofSymbols_n4;
          break;
        case NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N8:
          nrppa_pos_srs_resource->nrofSymbols = NRPPA_PosSRSResource_Item__nrofSymbols_n8;
          break;
        case NRPPA_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N12:
          nrppa_pos_srs_resource->nrofSymbols = NRPPA_PosSRSResource_Item__nrofSymbols_n12;
          break;
        default:
          AssertFatal(false, "illegal number of symbols %d\n", pos_srs_resource->nr_of_symbols);
          break;
      }

      nrppa_pos_srs_resource->freqDomainShift = pos_srs_resource->freq_domain_shift;
      nrppa_pos_srs_resource->c_SRS = pos_srs_resource->c_srs;

      switch (pos_srs_resource->group_or_sequence_hopping) {
        case NRPPA_GROUPORSEQUENCEHOPPING_NOTHING:
          nrppa_pos_srs_resource->groupOrSequenceHopping = NRPPA_PosSRSResource_Item__groupOrSequenceHopping_neither;
          break;
        case NRPPA_GROUPORSEQUENCEHOPPING_GROUPHOPPING:
          nrppa_pos_srs_resource->groupOrSequenceHopping = NRPPA_PosSRSResource_Item__groupOrSequenceHopping_groupHopping;
          break;
        case NRPPA_GROUPORSEQUENCEHOPPING_SEQUENCEHOPPING:
          nrppa_pos_srs_resource->groupOrSequenceHopping = NRPPA_PosSRSResource_Item__groupOrSequenceHopping_sequenceHopping;
          break;
        default:
          AssertFatal(false, "illegal groupOrSequenceHopping %d\n", pos_srs_resource->group_or_sequence_hopping);
          break;
      }

      nrppa_resource_type_pos_t *resource_type_pos = &pos_srs_resource->resource_type_pos;
      NRPPA_ResourceTypePos_t *nrppa_resourceTypePos = &nrppa_pos_srs_resource->resourceTypePos;
      if (resource_type_pos->present == NRPPA_RESOURCE_TYPE_POS_PR_NOTHING) {
        nrppa_resourceTypePos->present = NRPPA_ResourceTypePos_PR_NOTHING;
      } else if (resource_type_pos->present == NRPPA_RESOURCE_TYPE_POS_PR_PERIODIC) {
        nrppa_resourceTypePos->present = NRPPA_ResourceTypePos_PR_periodic;
        asn1cCalloc(nrppa_resourceTypePos->choice.periodic, nrppa_pos_periodic);
        switch (resource_type_pos->choice.periodic.periodicity) {
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT1:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot1;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT2:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot2;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT4:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot4;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot5;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT8:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot8;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT10:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot10;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT16:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot16;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT20:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot20;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT32:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot32;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT40:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot40;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT64:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot64;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT80:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot80;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT160:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot160;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT320:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot320;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT640:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot640;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT1280:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot1280;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT2560:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot2560;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5120:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot5120;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT10240:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot10240;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT20480:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot20480;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT40960:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot40960;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT81920:
            nrppa_pos_periodic->periodicity = NRPPA_ResourceTypePeriodicPos__periodicity_slot81920;
            break;
          default:
            AssertFatal(false, "illegal periodicity %d\n", resource_type_pos->choice.periodic.periodicity);
            break;
        }

        nrppa_pos_periodic->offset = resource_type_pos->choice.periodic.offset;
      } else if (resource_type_pos->present == NRPPA_RESOURCE_TYPE_POS_PR_SEMI_PERSISTENT) {
        nrppa_resourceTypePos->present = NRPPA_ResourceTypePos_PR_semi_persistent;
        asn1cCalloc(nrppa_resourceTypePos->choice.semi_persistent, nrppa_pos_semi_persistent);
        switch (resource_type_pos->choice.semi_persistent.periodicity) {
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT1:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot1;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT2:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot2;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT4:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot4;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot5;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT8:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot8;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT10:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot10;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT16:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot16;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT20:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot20;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT32:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot32;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT40:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot40;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT64:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot64;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT80:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot80;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT160:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot160;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT320:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot320;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT640:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot640;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT1280:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot1280;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT2560:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot2560;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5120:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot5120;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT10240:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot10240;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT20480:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot20480;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT40960:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot40960;
            break;
          case NRPPA_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT81920:
            nrppa_pos_semi_persistent->periodicity = NRPPA_ResourceTypeSemi_persistentPos__periodicity_slot81920;
            break;
          default:
            AssertFatal(false, "illegal periodicity %d\n", resource_type_pos->choice.semi_persistent.periodicity);
            break;
        }
        nrppa_pos_semi_persistent->offset = resource_type_pos->choice.semi_persistent.offset;
      } else if (resource_type_pos->present == NRPPA_RESOURCE_TYPE_POS_PR_APERIODIC) {
        nrppa_resourceTypePos->present = NRPPA_ResourceTypePos_PR_aperiodic;
        asn1cCalloc(nrppa_resourceTypePos->choice.aperiodic, nrppa_pos_aperiodic);
        nrppa_pos_aperiodic->slotOffset = resource_type_pos->choice.aperiodic.slot_offset;
      } else {
        AssertFatal(false, "illegal resourceType %d\n", resource_type_pos->present);
      }

      nrppa_pos_srs_resource->sequenceId = pos_srs_resource->sequence_id;
    }
  }

  // optional: srs_resource_set_list
  if (in_config->srs_resource_set_list) {
    nrppa_srs_resource_set_list_t *srs_resource_set_list = in_config->srs_resource_set_list;
    uint32_t srs_resource_set_list_length = srs_resource_set_list->srs_resource_set_list_length;
    asn1cCalloc(out_config.sRSResourceSet_List, nrppa_sRSResourceSet_List);
    for (int i = 0; i < srs_resource_set_list_length; i++) {
      asn1cSequenceAdd(nrppa_sRSResourceSet_List->list, NRPPA_SRSResourceSet_t, nrppa_srs_resource_set);
      nrppa_srs_resource_set_t *srs_resource_set = &srs_resource_set_list->srs_resource_set[i];
      nrppa_srs_resource_set->sRSResourceSetID = srs_resource_set->srs_resource_set_id;
      uint8_t srs_resource_id_list_length = srs_resource_set->srs_resource_id_list.srs_resource_id_list_length;
      for (int j = 0; j < srs_resource_id_list_length; j++) {
        asn1cSequenceAdd(nrppa_srs_resource_set->sRSResourceID_List.list, NRPPA_SRSResourceID_t, nrppa_srs_resource_id);
        *nrppa_srs_resource_id = srs_resource_set->srs_resource_id_list.srs_resource_id[j];
      }

      NRPPA_ResourceSetType_t *nrppa_resourceSetType = &nrppa_srs_resource_set->resourceSetType;
      nrppa_resource_set_type_t *resource_set_type = &srs_resource_set->resource_set_type;
      switch (resource_set_type->present) {
        case NRPPA_RESOURCE_SET_TYPE_PR_NOTHING:
          nrppa_resourceSetType->present = NRPPA_ResourceSetType_PR_NOTHING;
          break;
        case NRPPA_RESOURCE_SET_TYPE_PR_PERIODIC:
          nrppa_resourceSetType->present = NRPPA_ResourceSetType_PR_periodic;
          asn1cCalloc(nrppa_resourceSetType->choice.periodic, nrppa_periodic_set_type);
          nrppa_periodic_set_type->periodicSet = resource_set_type->choice.periodic;
          break;
        case NRPPA_RESOURCE_SET_TYPE_PR_SEMI_PERSISTENT:
          nrppa_resourceSetType->present = NRPPA_ResourceSetType_PR_semi_persistent;
          asn1cCalloc(nrppa_resourceSetType->choice.semi_persistent, nrppa_semi_persistent_set_type);
          nrppa_semi_persistent_set_type->semi_persistentSet = resource_set_type->choice.semi_persistent;
          break;
        case NRPPA_RESOURCE_SET_TYPE_PR_APERIODIC:
          nrppa_resourceSetType->present = NRPPA_ResourceSetType_PR_aperiodic;
          asn1cCalloc(nrppa_resourceSetType->choice.aperiodic, nrppa_aperiodic_set_type);
          nrppa_aperiodic_set_type->sRSResourceTrigger = resource_set_type->choice.aperiodic.srs_resource_trigger;
          nrppa_aperiodic_set_type->slotoffset = resource_set_type->choice.aperiodic.slot_offset;
          break;
        default:
          AssertFatal(false, "illegal resource set type %d\n", resource_set_type->present);
          break;
      }
    }
  }

  // optional: pos_srs_resource_set_list
  if (in_config->pos_srs_resource_set_list) {
    nrppa_pos_srs_resource_set_list_t *pos_srs_resource_set_list = in_config->pos_srs_resource_set_list;
    uint32_t pos_srs_resource_set_list_length = pos_srs_resource_set_list->pos_srs_resource_set_list_length;
    asn1cCalloc(out_config.posSRSResourceSet_List, nrppa_posSRSResourceSet_List);
    for (int i = 0; i < pos_srs_resource_set_list_length; i++) {
      asn1cSequenceAdd(nrppa_posSRSResourceSet_List->list, NRPPA_PosSRSResourceSet_Item_t, nrppa_pos_srs_resource_set);
      nrppa_pos_srs_resource_set_item_t *pos_srs_resource_set = &pos_srs_resource_set_list->pos_srs_resource_set_item[i];
      nrppa_pos_srs_resource_set->possrsResourceSetID = pos_srs_resource_set->pos_srs_resource_set_id;
      uint8_t pos_srs_resource_id_list_length = pos_srs_resource_set->pos_srs_resource_id_list.pos_srs_resource_id_list_length;
      for (int j = 0; j < pos_srs_resource_id_list_length; j++) {
        asn1cSequenceAdd(nrppa_pos_srs_resource_set->possRSResourceIDPerSet_List.list,
                         NRPPA_SRSPosResourceID_t,
                         nrppa_pos_srs_resource_id);
        *nrppa_pos_srs_resource_id = pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id[j];
      }

      NRPPA_PosResourceSetType_t *nrppa_posresourceSetType = &nrppa_pos_srs_resource_set->posresourceSetType;
      nrppa_pos_resource_set_type_t *pos_resource_set_type = &pos_srs_resource_set->pos_resource_set_type;
      switch (pos_resource_set_type->present) {
        case NRPPA_POS_RESOURCE_SET_TYPE_PR_NOTHING:
          nrppa_posresourceSetType->present = NRPPA_PosResourceSetType_PR_NOTHING;
          break;
        case NRPPA_POS_RESOURCE_SET_TYPE_PR_PERIODIC:
          nrppa_posresourceSetType->present = NRPPA_PosResourceSetType_PR_periodic;
          asn1cCalloc(nrppa_posresourceSetType->choice.periodic, nrppa_pos_periodic_set_type);
          nrppa_pos_periodic_set_type->posperiodicSet = pos_resource_set_type->choice.periodic;
          break;
        case NRPPA_POS_RESOURCE_SET_TYPE_PR_SEMI_PERSISTENT:
          nrppa_posresourceSetType->present = NRPPA_PosResourceSetType_PR_semi_persistent;
          asn1cCalloc(nrppa_posresourceSetType->choice.semi_persistent, nrppa_pos_semi_persistent_set_type);
          nrppa_pos_semi_persistent_set_type->possemi_persistentSet = pos_resource_set_type->choice.semi_persistent;
          break;
        case NRPPA_POS_RESOURCE_SET_TYPE_PR_APERIODIC:
          nrppa_posresourceSetType->present = NRPPA_PosResourceSetType_PR_aperiodic;
          asn1cCalloc(nrppa_posresourceSetType->choice.aperiodic, nrppa_pos_aperiodic_set_type);
          nrppa_pos_aperiodic_set_type->sRSResourceTrigger = pos_resource_set_type->choice.srs_resource;
          break;
        default:
          AssertFatal(false, "illegal resource set type pos %d\n", pos_resource_set_type->present);
          break;
      }
    }
  }
  return out_config;
}

static NRPPA_SRSCarrier_List_Item_t encode_srs_carrier_list_item(const nrppa_srs_carrier_list_item_t *in_item)
{
  NRPPA_SRSCarrier_List_Item_t out_item = {0};
  // pointA
  out_item.pointA = in_item->pointA;

  // Uplink Channel BW-PerSCS-List
  const nrppa_uplink_channel_bw_per_scs_list_t *uplink_channel_bw_per_scs_list = &in_item->uplink_channel_bw_per_scs_list;
  NRPPA_UplinkChannelBW_PerSCS_List_t *nrppa_uplink_channel_bw_per_scs_list = &out_item.uplinkChannelBW_PerSCS_List;

  uint32_t scs_specific_carrier_list_length = uplink_channel_bw_per_scs_list->scs_specific_carrier_list_length;
  for (int i = 0; i < scs_specific_carrier_list_length; i++) {
    asn1cSequenceAdd(nrppa_uplink_channel_bw_per_scs_list->list, NRPPA_SCS_SpecificCarrier_t, nrppa_scs_specific_carrier);
    nrppa_scs_specific_carrier_t *scs_specific_carrier = &uplink_channel_bw_per_scs_list->scs_specific_carrier[i];
    // offset to carrier
    nrppa_scs_specific_carrier->offsetToCarrier = scs_specific_carrier->offset_to_carrier;
    // subcarrier spacing
    switch (scs_specific_carrier->subcarrier_spacing) {
      case NRPPA_SUBCARRIER_SPACING_15KHZ:
        nrppa_scs_specific_carrier->subcarrierSpacing = NRPPA_SCS_SpecificCarrier__subcarrierSpacing_kHz15;
        break;
      case NRPPA_SUBCARRIER_SPACING_30KHZ:
        nrppa_scs_specific_carrier->subcarrierSpacing = NRPPA_SCS_SpecificCarrier__subcarrierSpacing_kHz30;
        break;
      case NRPPA_SUBCARRIER_SPACING_60KHZ:
        nrppa_scs_specific_carrier->subcarrierSpacing = NRPPA_SCS_SpecificCarrier__subcarrierSpacing_kHz60;
        break;
      case NRPPA_SUBCARRIER_SPACING_120KHZ:
        nrppa_scs_specific_carrier->subcarrierSpacing = NRPPA_SCS_SpecificCarrier__subcarrierSpacing_kHz120;
        break;
      default:
        AssertFatal(false, "illegal subcarrier spacing %d\n", scs_specific_carrier->subcarrier_spacing);
        break;
    }
    // carrier bandwidth
    nrppa_scs_specific_carrier->carrierBandwidth = scs_specific_carrier->carrier_bandwidth;
  }

  // Active UL BWP
  NRPPA_ActiveULBWP_t *nrppa_active_ul_bwp = &out_item.activeULBWP;
  const nrppa_active_ul_bwp_t *active_ul_bwp = &in_item->active_ul_bwp;

  // location and bandwidth
  nrppa_active_ul_bwp->locationAndBandwidth = active_ul_bwp->location_and_bandwidth;
  // subcarrier spacing
  switch (active_ul_bwp->subcarrier_spacing) {
    case NRPPA_SUBCARRIER_SPACING_15KHZ:
      nrppa_active_ul_bwp->subcarrierSpacing = NRPPA_ActiveULBWP__subcarrierSpacing_kHz15;
      break;
    case NRPPA_SUBCARRIER_SPACING_30KHZ:
      nrppa_active_ul_bwp->subcarrierSpacing = NRPPA_ActiveULBWP__subcarrierSpacing_kHz30;
      break;
    case NRPPA_SUBCARRIER_SPACING_60KHZ:
      nrppa_active_ul_bwp->subcarrierSpacing = NRPPA_ActiveULBWP__subcarrierSpacing_kHz60;
      break;
    case NRPPA_SUBCARRIER_SPACING_120KHZ:
      nrppa_active_ul_bwp->subcarrierSpacing = NRPPA_ActiveULBWP__subcarrierSpacing_kHz120;
      break;
    default:
      AssertFatal(false, "illegal subcarrier spacing %d\n", active_ul_bwp->subcarrier_spacing);
      break;
  }

  // cyclic prefix
  if (active_ul_bwp->cyclic_prefix == NRPPA_CP_TYPE_NORMAL)
    nrppa_active_ul_bwp->cyclicPrefix = NRPPA_ActiveULBWP__cyclicPrefix_normal;
  else
    nrppa_active_ul_bwp->cyclicPrefix = NRPPA_ActiveULBWP__cyclicPrefix_extended;

  // Tx Direct Current Location
  nrppa_active_ul_bwp->txDirectCurrentLocation = active_ul_bwp->tx_direct_current_location;

  // SRS Config
  const nrppa_srs_config_t *sRSConfig = &active_ul_bwp->srs_config;
  nrppa_active_ul_bwp->sRSConfig = encode_srs_config(sRSConfig);
  return out_item;
}

NRPPA_SRSCarrier_List_t encode_srs_carrier_list_nrppa(const nrppa_srs_carrier_list_t *in_list)
{
  NRPPA_SRSCarrier_List_t out_list = {0};
  uint32_t list_len = in_list->srs_carrier_list_length;
  for (int i = 0; i < list_len; i++) {
    asn1cSequenceAdd(out_list.list, NRPPA_SRSCarrier_List_Item_t, out_item);
    nrppa_srs_carrier_list_item_t *in_item = &in_list->srs_carrier_list_item[i];
    *out_item = encode_srs_carrier_list_item(in_item);
  }
  return out_list;
}

void free_nrppa_srs_carrier_list(nrppa_srs_carrier_list_t *srs_carrier_list)
{
  uint32_t srs_carrier_list_len = srs_carrier_list->srs_carrier_list_length;
  for (int i = 0; i < srs_carrier_list_len; i++) {
    nrppa_srs_carrier_list_item_t *srs_carrier_list_item = &srs_carrier_list->srs_carrier_list_item[i];
    free(srs_carrier_list_item->uplink_channel_bw_per_scs_list.scs_specific_carrier);

    nrppa_active_ul_bwp_t *active_ul_bwp = &srs_carrier_list_item->active_ul_bwp;
    nrppa_srs_config_t *sRSConfig = &active_ul_bwp->srs_config;
    if (sRSConfig->srs_resource_list) {
      nrppa_srs_resource_list_t *srs_resource_list = sRSConfig->srs_resource_list;
      free(srs_resource_list->srs_resource);
      free(sRSConfig->srs_resource_list);
    }
    if (sRSConfig->pos_srs_resource_list) {
      nrppa_pos_srs_resource_list_t *pos_srs_resource_list = sRSConfig->pos_srs_resource_list;
      free(pos_srs_resource_list->pos_srs_resource_item);
      free(sRSConfig->pos_srs_resource_list);
    }
    if (sRSConfig->srs_resource_set_list) {
      nrppa_srs_resource_set_list_t *srs_resource_set_list = sRSConfig->srs_resource_set_list;
      uint32_t srs_resource_set_list_length = srs_resource_set_list->srs_resource_set_list_length;
      for (int j = 0; j < srs_resource_set_list_length; j++) {
        nrppa_srs_resource_set_t *srs_resource_set = &srs_resource_set_list->srs_resource_set[j];
        free(srs_resource_set->srs_resource_id_list.srs_resource_id);
      }
      free(srs_resource_set_list->srs_resource_set);
      free(sRSConfig->srs_resource_set_list);
    }
    if (sRSConfig->pos_srs_resource_set_list) {
      nrppa_pos_srs_resource_set_list_t *pos_srs_resource_set_list = sRSConfig->pos_srs_resource_set_list;
      uint32_t pos_srs_resource_set_list_length = pos_srs_resource_set_list->pos_srs_resource_set_list_length;
      for (int j = 0; j < pos_srs_resource_set_list_length; j++) {
        nrppa_pos_srs_resource_set_item_t *pos_srs_resource_set = &pos_srs_resource_set_list->pos_srs_resource_set_item[j];
        free(pos_srs_resource_set->pos_srs_resource_id_list.srs_pos_resource_id);
      }
      free(pos_srs_resource_set_list->pos_srs_resource_set_item);
      free(sRSConfig->pos_srs_resource_set_list);
    }
  }
  free(srs_carrier_list->srs_carrier_list_item);
}

void free_nrppa_positioning_information_response(nrppa_positioning_information_resp_t *msg)
{
  /* SRS Configuration (O) */
  if (msg->srs_configuration) {
    nrppa_srs_carrier_list_t *srs_carrier_list = &msg->srs_configuration->srs_carrier_list;
    free_nrppa_srs_carrier_list(srs_carrier_list);
    free(msg->srs_configuration);
  }
}

void decode_nrppa_srstype(NRPPA_SRSType_t *srs_type, nrppa_srs_type_t *out)
{
  switch (srs_type->present) {
    case NRPPA_SRSType_PR_NOTHING:
      out->present = NRPPA_SRS_TYPE_PR_NOTHING;
      break;
    case NRPPA_SRSType_PR_semipersistentSRS:
      out->present = NRPPA_SRS_TYPE_PR_SEMIPERSISTENTSRS;
      out->choice.srs_resource_set_id = calloc_or_fail(1, sizeof(*out->choice.srs_resource_set_id));
      *out->choice.srs_resource_set_id = srs_type->choice.semipersistentSRS->sRSResourceSetID;
      break;
    case NRPPA_SRSType_PR_aperiodicSRS:
      out->present = NRPPA_SRS_TYPE_PR_APERIODICSRS;
      out->choice.aperiodic = calloc_or_fail(1, sizeof(*out->choice.aperiodic));
      *out->choice.aperiodic = srs_type->choice.aperiodicSRS->aperiodic;
      break;
    default:
      AssertFatal(false, "received illegal SRS type\n");
      break;
  }
}

void free_nrppa_positioning_activation_request(nrppa_positioning_activation_req_t *msg)
{
  if (msg->srs_type.present == NRPPA_SRS_TYPE_PR_SEMIPERSISTENTSRS) {
    free(msg->srs_type.choice.srs_resource_set_id);
  } else if (msg->srs_type.present == NRPPA_SRS_TYPE_PR_APERIODICSRS) {
    free(msg->srs_type.choice.aperiodic);
  }
}

int nrppa_gNB_handle_trp_information_request(nrppa_gnb_ue_info_t *nrppa_msg_info, const NRPPA_NRPPA_PDU_t *pdu)
{
  LOG_I(NRPPA, "Processing Received TRP Information Request \n");
  DevAssert(pdu != NULL);
  DevAssert(nrppa_msg_info != NULL);

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, pdu);
  }

  // Forward request to RRC
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, NRPPA_TRP_INFORMATION_REQ);
  nrppa_trp_information_req_t *req = &NRPPA_TRP_INFORMATION_REQ(msg);

  // Processing Received TRPInformationRequest
  NRPPA_TRPInformationRequest_t *container = NULL;
  NRPPA_TRPInformationRequest_IEs_t *ie = NULL;

  // IE 9.2.3 Message type : mandatory
  container = &pdu->choice.initiatingMessage->value.choice.TRPInformationRequest;

  // IE 9.2.4 nrppatransactionID : mandatory
  req->transaction_id = pdu->choice.initiatingMessage->nrppatransactionID;

  // IE TRP List : optional
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_TRPInformationRequest_IEs_t, ie, container, NRPPA_ProtocolIE_ID_id_TRPList, false);

  if (ie == NULL) {
    req->has_trp_list = false;
  } else {
    LOG_W(NRPPA, "TRPInformationRequest IE TRP List : not handled\n");
  }

  // IE TRP Information Type List: mandatory
  // not implemented in oai-lmf
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_TRPInformationRequest_IEs_t,
                              ie,
                              container,
                              NRPPA_ProtocolIE_ID_id_TRPInformationTypeListTRPReq,
                              false);

  if (ie == NULL) {
    LOG_W(NRPPA, "TRPInformationRequest IE TRP Information Type List is mandatory but not handled\n");
  } else {
    uint8_t trp_info_list_len = ie->value.choice.TRPInformationTypeListTRPReq.list.count;
    AssertError(trp_info_list_len > 0, return false, "at least 1 TRP Information Type must be present");
    nrppa_trp_information_type_list_t *trp_info_list = &req->trp_information_type_list;
    trp_info_list->trp_information_type_list_length = trp_info_list_len;
    trp_info_list->trp_information_type_item = calloc_or_fail(trp_info_list_len, sizeof(*trp_info_list->trp_information_type_item));
    for (int i = 0; i < trp_info_list_len; i++) {
      NRPPA_TRPInformationTypeItemTRPReq_t *item =
          (NRPPA_TRPInformationTypeItemTRPReq_t *)ie->value.choice.TRPInformationTypeListTRPReq.list.array[i];
      trp_info_list->trp_information_type_item[i] = item->value.choice.TRPInformationTypeItem;
    }
  }

  nrppa_store_ue_context(nrppa_msg_info, req->transaction_id);

  LOG_I(NRPPA, "Forwarding to RRC TRPInformationRequest transaction_id=%d\n", req->transaction_id);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
  return 0;
}

int nrppa_gNB_trp_information_response(instance_t instance, MessageDef *msg_p)
{
  DevAssert(msg_p);
  nrppa_trp_information_resp_t *resp = &NRPPA_TRP_INFORMATION_RESP(msg_p);
  nrppa_gNB_ue_context_t *ue_info = nrppa_detach_ue_context(resp->transaction_id);

  if (ue_info->gNB_ue_ngap_id != 0 && ue_info->amf_ue_ngap_id != 0) {
    LOG_E(NRPPA, "Illegal gNB_ue_ngap_id %d and amf_ue_ngap_id %ld\n", ue_info->gNB_ue_ngap_id, ue_info->amf_ue_ngap_id);
    free_nrppa_trp_information_response(resp);
    nrppa_free_ue_context(ue_info);
    return -1;
  }

  LOG_I(NRPPA, "Received TRP Information Response info from RRC with transaction_id = %u\n", ue_info->transaction_id);

  // Prepare NRPPA TRP Information transfer Response
  NRPPA_NRPPA_PDU_t pdu = {0};

  // IE: 9.2.3 Message Type : mandatory
  pdu.present = NRPPA_NRPPA_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, head);
  head->procedureCode = NRPPA_ProcedureCode_id_tRPInformationExchange;
  head->criticality = NRPPA_Criticality_reject;
  head->value.present = NRPPA_SuccessfulOutcome__value_PR_TRPInformationResponse;

  // IE 9.2.4 nrppatransactionID : mandatory
  head->nrppatransactionID = resp->transaction_id;
  NRPPA_TRPInformationResponse_t *out = &head->value.choice.TRPInformationResponse;

  // IE TRP Information List : mandatory
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_TRPInformationResponse_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_TRPInformationListTRPResp;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_TRPInformationResponse_IEs__value_PR_TRPInformationListTRPResp;

    uint32_t trp_info_item_len = resp->trp_information_list.trp_information_item_length;
    for (int i = 0; i < trp_info_item_len; i++) {
      asn1cSequenceAdd(ie->value.choice.TRPInformationListTRPResp.list, TRPInformationListTRPResp__Member, trpinfolist_member);
      nrppa_trp_information_t *nrppa_tRPInformation = &resp->trp_information_list.trp_information_item[i];

      NRPPA_TRPInformation_t *trp_info = &trpinfolist_member->tRPInformation;
      trp_info->tRP_ID = nrppa_tRPInformation->trp_id;

      nrppa_trp_information_type_response_list_t *nrppa_tRPInformationTypeResponseList =
          &nrppa_tRPInformation->trp_information_type_response_list;
      uint8_t trp_info_type_resp_item_len = nrppa_tRPInformationTypeResponseList->trp_information_type_response_item_length;

      for (int j = 0; j < trp_info_type_resp_item_len; j++) {
        asn1cSequenceAdd(trp_info->tRPInformationTypeResponseList.list, NRPPA_TRPInformationTypeResponseItem_t, trp_info_item);
        nrppa_trp_information_type_response_item_t *resp_item =
            &nrppa_tRPInformationTypeResponseList->trp_information_type_response_item[j];
        *trp_info_item = encode_trp_info_type_response_item_nrppa(resp_item);
      }
    }
  }

  free_nrppa_trp_information_response(resp);

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &pdu);
  }

  // Encode NRPPA message
  uint8_t *buffer = NULL;
  uint32_t length = 0;
  if (nrppa_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    LOG_E(NRPPA, "Failed to encode Uplink NRPPa TRP Information Response\n");
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

int nrppa_gNB_handle_positioning_information_request(nrppa_gnb_ue_info_t *nrppa_msg_info, const NRPPA_NRPPA_PDU_t *pdu)
{
  DevAssert(pdu != NULL);
  DevAssert(nrppa_msg_info != NULL);

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, pdu);
  }

  LOG_I(NRPPA, "Processing Received Positioning Information Request\n");

  // Preparing positioning information request for RRC
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, NRPPA_POSITIONING_INFORMATION_REQ);
  nrppa_positioning_information_req_t *req = &NRPPA_POSITIONING_INFORMATION_REQ(msg);

  // IE 9.2.4 nrppatransactionID : mandatory
  req->transaction_id = pdu->choice.initiatingMessage->nrppatransactionID;

  nrppa_store_ue_context(nrppa_msg_info, req->transaction_id);

  LOG_I(NRPPA, "Forwarding to RRC Positioning Information Request transaction id=%d\n", req->transaction_id);

  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
  return 0;
}

int nrppa_gNB_positioning_information_response(instance_t instance, MessageDef *msg_p)
{
  DevAssert(msg_p);
  nrppa_positioning_information_resp_t *resp = &NRPPA_POSITIONING_INFORMATION_RESP(msg_p);
  nrppa_gNB_ue_context_t *ue_info = nrppa_detach_ue_context(resp->transaction_id);

  if (ue_info->gNB_ue_ngap_id <= 0 && ue_info->amf_ue_ngap_id <= 0) {
    LOG_E(NRPPA, "Illegal gNB_ue_ngap_id %d and amf_ue_ngap_id %ld\n", ue_info->gNB_ue_ngap_id, ue_info->amf_ue_ngap_id);
    free_nrppa_positioning_information_response(resp);
    nrppa_free_ue_context(ue_info);
    return -1;
  }

  LOG_I(NRPPA,
        "Received Positioning Information Response info from RRC with transaction_id = %u and gNB_ue_ngap_id %u\n",
        ue_info->transaction_id,
        ue_info->gNB_ue_ngap_id);

  // Prepare NRPPA TRP Information transfer Response
  NRPPA_NRPPA_PDU_t pdu = {0};

  // IE: 9.2.3 Message Type : mandatory
  pdu.present = NRPPA_NRPPA_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, head);
  head->procedureCode = NRPPA_ProcedureCode_id_positioningInformationExchange;
  head->criticality = NRPPA_Criticality_reject;
  head->value.present = NRPPA_SuccessfulOutcome__value_PR_PositioningInformationResponse;

  // IE 9.2.4 nrppatransactionID : mandatory
  head->nrppatransactionID = resp->transaction_id;
  NRPPA_PositioningInformationResponse_t *out = &head->value.choice.PositioningInformationResponse;

  // IE SRS Configuration : optional
  {
    if (resp->srs_configuration) {
      asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningInformationResponse_IEs_t, ie);
      ie->id = NRPPA_ProtocolIE_ID_id_SRSConfiguration;
      ie->criticality = NRPPA_Criticality_ignore;
      ie->value.present = NRPPA_PositioningInformationResponse_IEs__value_PR_SRSConfiguration;
      nrppa_srs_carrier_list_t *srs_carrier_list = &resp->srs_configuration->srs_carrier_list;
      ie->value.choice.SRSConfiguration.sRSCarrier_List = encode_srs_carrier_list_nrppa(srs_carrier_list);
    }
  }

  free_nrppa_positioning_information_response(resp);

  LOG_I(NRPPA, "Calling encoder for Positioning Information Response \n");

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &pdu);
  }

  // Encode NRPPA message
  uint8_t *buffer = NULL;
  uint32_t length = 0;
  if (nrppa_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    LOG_E(NRPPA, "Failed to encode Uplink NRPPa PositioningInformationResponse\n");
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
    return -1;
  }

  MessageDef *msg = itti_alloc_new_message(TASK_NRPPA, 0, NGAP_UPLINKUEASSOCIATEDNRPPA);
  ngap_uplink_ue_associated_nrppa_t *ULNRPPA = &NGAP_UPLINKUEASSOCIATEDNRPPA(msg);

  ULNRPPA->gNB_ue_ngap_id = ue_info->gNB_ue_ngap_id;
  ULNRPPA->amf_ue_ngap_id = ue_info->amf_ue_ngap_id;

  // Routing ID
  ULNRPPA->routing_id = create_byte_array(ue_info->routing_id.len, ue_info->routing_id.buf);

  // NRPPA PDU
  ULNRPPA->nrppa_pdu = create_byte_array(length, buffer);

  // Forward the NRPPA PDU to NGAP
  itti_send_msg_to_task(TASK_NGAP, instance, msg);
  nrppa_free_ue_context(ue_info);
  free(buffer);
  return length;
}

int nrppa_gNB_handle_positioning_activation_request(nrppa_gnb_ue_info_t *nrppa_msg_info, const NRPPA_NRPPA_PDU_t *pdu)
{
  LOG_I(NRPPA, "Processing Received Positioning Activation Request \n");
  DevAssert(pdu != NULL);
  DevAssert(nrppa_msg_info != NULL);

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, pdu);
  }

  // Preparing positioning information request for RRC
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, NRPPA_POSITIONING_ACTIVATION_REQ);
  nrppa_positioning_activation_req_t *req = &NRPPA_POSITIONING_ACTIVATION_REQ(msg);

  // Processing Received PositioningActivationRequest
  NRPPA_PositioningActivationRequest_t *container = NULL;
  NRPPA_PositioningActivationRequestIEs_t *ie = NULL;

  // IE 9.2.3 Message type : mandatory
  container = &pdu->choice.initiatingMessage->value.choice.PositioningActivationRequest;

  // IE 9.2.4 nrppatransactionID : mandatory
  req->transaction_id = pdu->choice.initiatingMessage->nrppatransactionID;

  // IE SRSType : mandatory
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_PositioningActivationRequestIEs_t, ie, container, NRPPA_ProtocolIE_ID_id_SRSType, true);
  decode_nrppa_srstype(&ie->value.choice.SRSType, &req->srs_type);

  nrppa_store_ue_context(nrppa_msg_info, req->transaction_id);

  LOG_I(NRPPA, "Forwarding to RRC Positioning Activation Request transaction id = %d\n", req->transaction_id);

  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
  return 0;
}

int nrppa_gNB_positioning_activation_response(instance_t instance, MessageDef *msg_p)
{
  DevAssert(msg_p);
  nrppa_positioning_activation_resp_t *resp = &NRPPA_POSITIONING_ACTIVATION_RESP(msg_p);
  nrppa_gNB_ue_context_t *ue_info = nrppa_detach_ue_context(resp->transaction_id);

  if (ue_info->gNB_ue_ngap_id <= 0 && ue_info->amf_ue_ngap_id <= 0) {
    LOG_E(NRPPA, "Illegal gNB_ue_ngap_id %d and amf_ue_ngap_id %ld\n", ue_info->gNB_ue_ngap_id, ue_info->amf_ue_ngap_id);
    nrppa_free_ue_context(ue_info);
    return -1;
  }

  LOG_I(NRPPA,
        "Received PositioningInformationResponse info from RRC with transaction_id=%u and gNB_ue_ngap_id %u\n",
        ue_info->transaction_id,
        ue_info->gNB_ue_ngap_id);

  // Prepare NRPPA TRP Information transfer Response
  NRPPA_NRPPA_PDU_t pdu = {0};

  // IE: 9.2.3 Message Type : mandatory
  pdu.present = NRPPA_NRPPA_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, head);
  head->procedureCode = NRPPA_ProcedureCode_id_positioningActivation;
  head->criticality = NRPPA_Criticality_reject;
  head->value.present = NRPPA_SuccessfulOutcome__value_PR_PositioningActivationResponse;

  // IE 9.2.4 nrppatransactionID : mandatory
  head->nrppatransactionID = resp->transaction_id;

  LOG_I(NRPPA, "Calling encoder for PositioningActivationResponse \n");

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &pdu);
  }

  // Encode NRPPA message
  uint8_t *buffer = NULL;
  uint32_t length = 0;
  if (nrppa_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    LOG_E(NRPPA, "Failed to encode Uplink NRPPa PositioningActivationResponse\n");
    nrppa_free_ue_context(ue_info);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
    return -1;
  }

  MessageDef *msg = itti_alloc_new_message(TASK_NRPPA, 0, NGAP_UPLINKUEASSOCIATEDNRPPA);
  ngap_uplink_ue_associated_nrppa_t *ULNRPPA = &NGAP_UPLINKUEASSOCIATEDNRPPA(msg);

  ULNRPPA->gNB_ue_ngap_id = ue_info->gNB_ue_ngap_id;
  ULNRPPA->amf_ue_ngap_id = ue_info->amf_ue_ngap_id;

  // Routing ID
  ULNRPPA->routing_id = create_byte_array(ue_info->routing_id.len, ue_info->routing_id.buf);

  // NRPPA PDU
  ULNRPPA->nrppa_pdu = create_byte_array(length, buffer);

  // Forward the NRPPA PDU to NGAP
  itti_send_msg_to_task(TASK_NGAP, instance, msg);
  nrppa_free_ue_context(ue_info);
  free(buffer);
  return length;
}

int nrppa_gNB_trp_information_failure(instance_t instance, MessageDef *msg_p)
{
  DevAssert(msg_p);
  nrppa_trp_information_failure_t *fail = &NRPPA_TRP_INFORMATION_FAILURE(msg_p);
  nrppa_gNB_ue_context_t *ue_info = nrppa_detach_ue_context(fail->transaction_id);

  if (ue_info->gNB_ue_ngap_id != 0 && ue_info->amf_ue_ngap_id != 0) {
    LOG_E(NRPPA, "Illegal gNB_ue_ngap_id %d and amf_ue_ngap_id %ld\n", ue_info->gNB_ue_ngap_id, ue_info->amf_ue_ngap_id);
    nrppa_free_ue_context(ue_info);
    return -1;
  }

  LOG_I(NRPPA,
        "Received TRP Information Failure from RRC with transaction_id=%u and gNB_ue_ngap_id %u\n",
        ue_info->transaction_id,
        ue_info->gNB_ue_ngap_id);

  NRPPA_NRPPA_PDU_t pdu = {0};

  // IE: 9.2.3 Message Type : mandatory
  pdu.present = NRPPA_NRPPA_PDU_PR_unsuccessfulOutcome;
  asn1cCalloc(pdu.choice.unsuccessfulOutcome, head);
  head->procedureCode = NRPPA_ProcedureCode_id_tRPInformationExchange;
  head->criticality = NRPPA_Criticality_reject;
  head->value.present = NRPPA_UnsuccessfulOutcome__value_PR_TRPInformationFailure;

  // IE 9.2.4 nrppatransactionID : mandatory
  head->nrppatransactionID = fail->transaction_id;
  NRPPA_TRPInformationFailure_t *out = &head->value.choice.TRPInformationFailure;

  // IE 9.2.1 Cause : mandatory
  asn1cSequenceAdd(out->protocolIEs.list, NRPPA_TRPInformationFailure_IEs_t, ie);
  ie->id = NRPPA_ProtocolIE_ID_id_Cause;
  ie->criticality = NRPPA_Criticality_ignore;
  ie->value.present = NRPPA_TRPInformationFailure_IEs__value_PR_Cause;
  ie->value.choice.Cause = encode_nrppa_cause(fail->cause);

  LOG_I(NRPPA, "Calling encoder for TRP Information Failure \n");

  if (LOG_DEBUGFLAG(DEBUG_ASN1)) {
    xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &pdu);
  }

  // Encode NRPPA message
  uint8_t *buffer = NULL;
  uint32_t length = 0;
  if (nrppa_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    LOG_E(NRPPA, "Failed to encode Uplink NRPPa TRP Information Failure\n");
    nrppa_free_ue_context(ue_info);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
    return -1;
  }

  MessageDef *msg = itti_alloc_new_message(TASK_NRPPA, 0, NGAP_UPLINKNONUEASSOCIATEDNRPPA);
  ngap_uplink_non_ue_associated_nrppa_t *ULNRPPA = &NGAP_UPLINKNONUEASSOCIATEDNRPPA(msg);

  // Routing ID
  ULNRPPA->routing_id = create_byte_array(ue_info->routing_id.len, ue_info->routing_id.buf);

  // NRPPA PDU
  ULNRPPA->nrppa_pdu = create_byte_array(length, buffer);

  // Forward the NRPPA PDU to NGAP
  itti_send_msg_to_task(TASK_NGAP, instance, msg);
  nrppa_free_ue_context(ue_info);
  free(buffer);
  return length;
  return 0;
}
