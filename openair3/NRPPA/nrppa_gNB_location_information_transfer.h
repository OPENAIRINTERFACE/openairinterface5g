/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef NRPPA_GNB_POSITIONING_PROCEDURES_H_
#define NRPPA_GNB_POSITIONING_PROCEDURES_H_

#include "nrppa_common.h"

int nrppa_gNB_handle_trp_information_request(nrppa_gnb_ue_info_t *nrppa_msg_info, const NRPPA_NRPPA_PDU_t *pdu);
void free_nrppa_trp_information_request(nrppa_trp_information_req_t *msg);
int nrppa_gNB_trp_information_response(instance_t instance, MessageDef *msg_p);
void free_nrppa_trp_information_request(nrppa_trp_information_req_t *msg);
NRPPA_TRPInformationTypeResponseItem_t encode_trp_info_type_response_item_nrppa(nrppa_trp_information_type_response_item_t *in);
void free_nrppa_trp_information_response(nrppa_trp_information_resp_t *msg);
int nrppa_gNB_handle_positioning_information_request(nrppa_gnb_ue_info_t *nrppa_msg_info, const NRPPA_NRPPA_PDU_t *pdu);
int nrppa_gNB_positioning_information_response(instance_t instance, MessageDef *msg_p);
NRPPA_SRSCarrier_List_t encode_srs_carrier_list_nrppa(const nrppa_srs_carrier_list_t *in_list);
void free_nrppa_srs_carrier_list(nrppa_srs_carrier_list_t *srs_carrier_list);
void free_nrppa_positioning_information_response(nrppa_positioning_information_resp_t *msg);
int nrppa_gNB_handle_positioning_activation_request(nrppa_gnb_ue_info_t *nrppa_msg_info, const NRPPA_NRPPA_PDU_t *pdu);
void decode_nrppa_srstype(NRPPA_SRSType_t *srs_type, nrppa_srs_type_t *out);
void free_nrppa_positioning_activation_request(nrppa_positioning_activation_req_t *msg);
int nrppa_gNB_positioning_activation_response(instance_t instance, MessageDef *msg_p);
int nrppa_gNB_trp_information_failure(instance_t instance, MessageDef *msg_p);

#endif /* NRPPA_GNB_POSITIONING_PROCEDURES_H_ */
