/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

# include "intertask_interface.h"
# include "create_tasks.h"
# include "common/utils/LOG/log.h"
# include "executables/softmodem-common.h"

  #include "sctp_eNB_task.h"
  #include "s1ap_eNB.h"
  #include "openair3/NAS/UE/nas_ue_task.h"
  #include "udp_eNB_task.h"
  #include "gtpv1u_eNB_task.h"
  #if ENABLE_RAL
    #include "lteRALue.h"
    #include "lteRALenb.h"
  #endif
  #include "RRC/LTE/rrc_defs.h"
# include "enb_app.h"

int create_tasks_ue(uint32_t ue_nb) {
  LOG_D(ENB_APP, "%s(ue_nb:%d)\n", __FUNCTION__, ue_nb);
  itti_wait_ready(1);

  if (EPC_MODE_ENABLED) {
#      if defined(NAS_BUILT_IN_UE)

    if (ue_nb > 0) {
      nas_user_container_t *users = calloc(1, sizeof(*users));

      if (users == NULL) abort();

      users->count = ue_nb;

      if (itti_create_task (TASK_NAS_UE, nas_ue_task, users) < 0) {
        LOG_E(NAS, "Create task for NAS UE failed\n");
        return -1;
      }
    }

#      endif
  } /* EPC_MODE_ENABLED */

  if (ue_nb > 0) {
    if (itti_create_task (TASK_RRC_UE, rrc_ue_task, NULL) < 0) {
      LOG_E(RRC, "Create task for RRC UE failed\n");
      return -1;
    }

    if (get_softmodem_params()->nsa) {
      init_connections_with_nr_ue();
      LOG_I(RRC, "Started LTE-NR link in the LTE UE\n");
      if (itti_create_task (TASK_RRC_NSA_UE, recv_msgs_from_nr_ue, NULL) < 0) {
        LOG_E(RRC, "Create task for RRC NSA UE failed\n");
        return -1;
      }
    }
  }

  itti_wait_ready(0);
  return 0;
}

