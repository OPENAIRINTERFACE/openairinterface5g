/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef PDU_SESSION_H_
#define PDU_SESSION_H_

#include "common/5g_platform_types.h"

typedef struct {
  int id;
  int type;
  nssai_t nssai;
  char *dnn; // [103]?
  char *ifname;
} pdu_session_config_t;

int get_pdu_session_configs(const char *uiccName, pdu_session_config_t *configs, int max_len);

#endif /* PDU_SESSION_H_ */
