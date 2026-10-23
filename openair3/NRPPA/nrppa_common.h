/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef NRPPA_COMMON_H_
#define NRPPA_COMMON_H_

#include "oai_asn1.h"
#include "common/utils/LOG/log.h"
#include "ds/byte_array.h"
#include "nrppa_includes.h"
#include "NRPPA_NRPPA-PDU.h"
#include "NRPPA_TRPInformationTypeResponseItem.h"
#include "NRPPA_SRSCarrier-List.h"
#include "NRPPA_SRSType.h"
#include "common/5g_platform_types.h"
#include "openair2/COMMON/nrppa_messages_types.h"

#define NRPPA_FIND_PROTOCOLIE_BY_ID(IE_TYPE, ie, container, IE_ID, mandatory)                                                  \
  do {                                                                                                                         \
    IE_TYPE **ptr;                                                                                                             \
    ie = NULL;                                                                                                                 \
    for (ptr = container->protocolIEs.list.array; ptr < &container->protocolIEs.list.array[container->protocolIEs.list.count]; \
         ptr++) {                                                                                                              \
      if ((*ptr)->id == IE_ID) {                                                                                               \
        ie = *ptr;                                                                                                             \
        break;                                                                                                                 \
      }                                                                                                                        \
    }                                                                                                                          \
    if (ie == NULL) {                                                                                                          \
      if (mandatory) {                                                                                                         \
        AssertFatal(false, "NRPPA_FIND_PROTOCOLIE_BY_ID ie is NULL (searching for ie: %ld)\n", IE_ID);                         \
      } else {                                                                                                                 \
        LOG_I(NRPPA, "NRPPA_FIND_PROTOCOLIE_BY_ID ie is NULL (searching for ie: %ld)\n", IE_ID);                               \
      }                                                                                                                        \
    }                                                                                                                          \
  } while (0);                                                                                                                 \
  if (mandatory && !ie)                                                                                                        \
  return -1

// gnb and ue related info in NRPPA message
typedef struct nrppa_gnb_ue_info_s {
  instance_t instance;
  int32_t gNB_ue_ngap_id;
  int64_t amf_ue_ngap_id;
  byte_array_t routing_id;
} nrppa_gnb_ue_info_t;

typedef int (*nrppa_message_decoded_callback)(nrppa_gnb_ue_info_t *nrppa_msg_info, const NRPPA_NRPPA_PDU_t *pdu);
NRPPA_Cause_t encode_nrppa_cause(nrppa_cause_t cause);

#endif /* NRPPA_COMMON_H_ */
