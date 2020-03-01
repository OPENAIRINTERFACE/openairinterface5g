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
# include "create_nr_tasks.h"
# include "common/utils/LOG/log.h"

# ifdef OPENAIR2
#include "sctp_eNB_task.h"
#include "s1ap_eNB.h"
#include "nas_ue_task.h"
#include "udp_eNB_task.h"
#include "gtpv1u_eNB_task.h"
#   if ENABLE_RAL
#     include "lteRALue.h"
#     include "lteRALenb.h"
#   endif
#include "RRC/NR/nr_rrc_defs.h"
# endif
# include "gnb_app.h"

extern int emulate_rf;

int create_gNB_tasks(uint32_t gnb_nb)
{
  LOG_D(GNB_APP, "%s(gnb_nb:%d\n", __FUNCTION__, gnb_nb);
  
  itti_wait_ready(1);
  if (itti_create_task (TASK_L2L1, l2l1_task, NULL) < 0) {
    LOG_E(PDCP, "Create task for L2L1 failed\n");
    return -1;
  }

  if (gnb_nb > 0) {
    /* Last task to create, others task must be ready before its start */
    if (itti_create_task (TASK_GNB_APP, gNB_app_task, NULL) < 0) {
      LOG_E(GNB_APP, "Create task for gNB APP failed\n");
      return -1;
    }
  }

/*
  if (EPC_MODE_ENABLED) {
      if (gnb_nb > 0) {
        if (itti_create_task (TASK_SCTP, sctp_eNB_task, NULL) < 0) {
          LOG_E(SCTP, "Create task for SCTP failed\n");
          return -1;
        }

        if (itti_create_task (TASK_S1AP, s1ap_eNB_task, NULL) < 0) {
          LOG_E(S1AP, "Create task for S1AP failed\n");
          return -1;
        }
        if(!emulate_rf){
          if (itti_create_task (TASK_UDP, udp_eNB_task, NULL) < 0) {
            LOG_E(UDP_, "Create task for UDP failed\n");
            return -1;
          }
        }

        if (itti_create_task (TASK_GTPV1_U, &gtpv1u_eNB_task, NULL) < 0) {
          LOG_E(GTPU, "Create task for GTPV1U failed\n");
          return -1;
        }
      }

   }
*/

    if (gnb_nb > 0) {
      LOG_I(NR_RRC,"Creating NR RRC gNB Task\n");

      if (itti_create_task (TASK_RRC_GNB, rrc_gnb_task, NULL) < 0) {
        LOG_E(NR_RRC, "Create task for NR RRC gNB failed\n");
        return -1;
      }
    }


  itti_wait_ready(0);

  return 0;
}
#endif
