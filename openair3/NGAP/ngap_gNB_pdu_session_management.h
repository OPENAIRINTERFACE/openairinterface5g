/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef NGAP_GNB_PDU_SESSION_MANAGEMENT_H_
#define NGAP_GNB_PDU_SESSION_MANAGEMENT_H_

#include <stdint.h>
#include "assertions.h"
#include "ngap_messages_types.h"
#include "ngap_msg_includes.h"

int ngap_gNB_pdusession_setup_resp(instance_t instance, ngap_pdusession_setup_resp_t *pdusession_setup_resp_p);

int ngap_gNB_pdusession_modify_resp(instance_t instance, ngap_pdusession_modify_resp_t *pdusession_modify_resp_p);

int ngap_gNB_pdusession_release_resp(instance_t instance, ngap_pdusession_release_resp_t *pdusession_release_resp_p);

int ngap_gNB_pdusession_resource_notify(instance_t instance, ngap_pdusession_resource_notify_t *msg);

#endif /* NGAP_GNB_PDU_SESSION_MANAGEMENT_H_ */
