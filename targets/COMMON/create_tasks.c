/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

   Contact Information
   OpenAirInterface Admin: openair_admin@eurecom.fr
   OpenAirInterface Tech : openair_tech@eurecom.fr
   OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# include "create_tasks.h"
# include "log.h"

# ifdef OPENAIR2
#   if defined(ENABLE_USE_MME)
#     include "sctp_eNB_task.h"
#     include "s1ap_eNB.h"
#     include "nas_ue_task.h"
#     include "udp_eNB_task.h"
#     include "gtpv1u_eNB_task.h"
#   endif
#   if ENABLE_RAL
#     include "lteRALue.h"
#     include "lteRALenb.h"
#   endif
#   include "RRC/LITE/defs.h"
# endif
# include "enb_app.h"

int create_tasks(uint32_t enb_nb, uint32_t ue_nb)
{
  itti_wait_ready(1);
  if (itti_create_task (TASK_L2L1, l2l1_task, NULL) < 0) {
    LOG_E(PDCP, "Create task for L2L1 failed\n");
    return -1;
  }

  if (enb_nb > 0) {
    /* Last task to create, others task must be ready before its start */
    if (itti_create_task (TASK_ENB_APP, eNB_app_task, NULL) < 0) {
      LOG_E(ENB_APP, "Create task for eNB APP failed\n");
      return -1;
    }
  }


# ifdef OPENAIR2
  {
#   if defined(ENABLE_USE_MME)
    {
      if (enb_nb > 0) {
        if (itti_create_task (TASK_SCTP, sctp_eNB_task, NULL) < 0) {
          LOG_E(SCTP, "Create task for SCTP failed\n");
          return -1;
        }

        if (itti_create_task (TASK_S1AP, s1ap_eNB_task, NULL) < 0) {
          LOG_E(S1AP, "Create task for S1AP failed\n");
          return -1;
        }

        if (itti_create_task (TASK_UDP, udp_eNB_task, NULL) < 0) {
          LOG_E(UDP_, "Create task for UDP failed\n");
          return -1;
        }

        if (itti_create_task (TASK_GTPV1_U, &gtpv1u_eNB_task, NULL) < 0) {
          LOG_E(GTPU, "Create task for GTPV1U failed\n");
          return -1;
        }
      }

#      if defined(NAS_BUILT_IN_UE)
      if (ue_nb > 0) {
        if (itti_create_task (TASK_NAS_UE, nas_ue_task, NULL) < 0) {
          LOG_E(NAS, "Create task for NAS UE failed\n");
          return -1;
        }
      }
#      endif
    }
#   endif

    if (enb_nb > 0) {
      if (itti_create_task (TASK_RRC_ENB, rrc_enb_task, NULL) < 0) {
        LOG_E(RRC, "Create task for RRC eNB failed\n");
        return -1;
      }

#   if ENABLE_RAL

      if (itti_create_task (TASK_RAL_ENB, eRAL_task, NULL) < 0) {
        LOG_E(RAL_ENB, "Create task for RAL eNB failed\n");
        return -1;
      }

#   endif
    }

    if (ue_nb > 0) {
      if (itti_create_task (TASK_RRC_UE, rrc_ue_task, NULL) < 0) {
        LOG_E(RRC, "Create task for RRC UE failed\n");
        return -1;
      }

#   if ENABLE_RAL

      if (itti_create_task (TASK_RAL_UE, mRAL_task, NULL) < 0) {
        LOG_E(RAL_UE, "Create task for RAL UE failed\n");
        return -1;
      }

#   endif
    }
  }
# endif // openair2: NN: should be openair3


  itti_wait_ready(0);

  return 0;
}
#endif
