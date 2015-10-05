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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "intertask_interface.h"
#include "mme_config.h"
#include "nas_defs.h"
#include "msc.h"

#if !defined(DISABLE_USE_NAS)
# include "nas_network.h"
# include "nas_proc.h"
# include "emm_main.h"
# include "nas_log.h"
# include "nas_timer.h"
#endif

#define NAS_ERROR(x, args...) do { fprintf(stderr, "[NAS] [E]"x, ##args); } while(0)
#define NAS_DEBUG(x, args...) do { fprintf(stdout, "[NAS] [D]"x, ##args); } while(0)
#define NAS_WARN(x, args...)  do { fprintf(stdout, "[NAS] [W]"x, ##args); } while(0)

static void *nas_intertask_interface(void *args_p)
{
  itti_mark_task_ready(TASK_NAS_MME);
  MSC_START_USE();

  while(1) {
    MessageDef *received_message_p;

next_message:
    itti_receive_msg(TASK_NAS_MME, &received_message_p);

    switch (ITTI_MSG_ID(received_message_p)) {
    case NAS_CONNECTION_ESTABLISHMENT_IND: {
#if defined(DISABLE_USE_NAS)
      MessageDef       *message_p;
      nas_attach_req_t *nas_req_p;
      s1ap_initial_ue_message_t *transparent;

      NAS_DEBUG("NAS abstraction: Generating NAS ATTACH REQ\n");

      message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_ATTACH_REQ);

      nas_req_p = &message_p->ittiMsg.nas_attach_req;
      transparent = &message_p->ittiMsg.nas_attach_req.transparent;

      nas_req_p->initial = INITIAL_REQUEST;
      sprintf(nas_req_p->imsi, "%14llu", 20834123456789ULL);

      memcpy(transparent, &received_message_p->ittiMsg.nas_conn_est_ind.transparent,
             sizeof(s1ap_initial_ue_message_t));

      itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
#else
      nas_establish_ind_t *nas_est_ind_p;

      nas_est_ind_p = &received_message_p->ittiMsg.nas_conn_est_ind.nas;

      nas_proc_establish_ind(nas_est_ind_p->UEid,
                             nas_est_ind_p->tac,
                             nas_est_ind_p->initialNasMsg.data,
                             nas_est_ind_p->initialNasMsg.length);
#endif
    }
    break;

#if defined(DISABLE_USE_NAS)

    case NAS_ATTACH_ACCEPT: {
      itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, received_message_p);
      goto next_message;
    }
    break;

    case NAS_AUTHENTICATION_REQ: {
      MessageDef      *message_p;
      nas_auth_resp_t *nas_resp_p;

      NAS_DEBUG("NAS abstraction: Generating NAS AUTHENTICATION RESPONSE\n");

      message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_AUTHENTICATION_RESP);

      nas_resp_p = &message_p->ittiMsg.nas_auth_resp;

      memcpy(nas_resp_p->imsi, received_message_p->ittiMsg.nas_auth_req.imsi, 16);

      itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
    }
    break;
#else

    case NAS_UPLINK_DATA_IND: {
      nas_proc_ul_transfer_ind(NAS_UL_DATA_IND(received_message_p).UEid,
                               NAS_UL_DATA_IND(received_message_p).nasMsg.data,
                               NAS_UL_DATA_IND(received_message_p).nasMsg.length);
    }
    break;

    case NAS_DOWNLINK_DATA_CNF: {
      nas_proc_dl_transfer_cnf(NAS_DL_DATA_CNF(received_message_p).UEid);
    } break;

    case NAS_DOWNLINK_DATA_REJ: {
      nas_proc_dl_transfer_rej(NAS_DL_DATA_REJ(received_message_p).UEid);
    } break;

    case NAS_AUTHENTICATION_PARAM_RSP: {
      nas_proc_auth_param_res(&NAS_AUTHENTICATION_PARAM_RSP(received_message_p));
    }
    break;

    case NAS_AUTHENTICATION_PARAM_FAIL: {
      nas_proc_auth_param_fail(&NAS_AUTHENTICATION_PARAM_FAIL(received_message_p));
    }
    break;
#endif

    case NAS_PDN_CONNECTIVITY_RSP: {
      nas_proc_pdn_connectivity_res(&NAS_PDN_CONNECTIVITY_RSP(received_message_p));
    }
    break;

    case NAS_PDN_CONNECTIVITY_FAIL: {
      nas_proc_pdn_connectivity_fail(&NAS_PDN_CONNECTIVITY_FAIL(received_message_p));
    }
    break;


    case TIMER_HAS_EXPIRED: {
#if !defined(DISABLE_USE_NAS)
      /* Call the NAS timer api */
      nas_timer_handle_signal_expiry(TIMER_HAS_EXPIRED(received_message_p).timer_id,
                                     TIMER_HAS_EXPIRED(received_message_p).arg);
#endif
    }
    break;

    case S1AP_ENB_DEREGISTERED_IND: {
#if !defined(DISABLE_USE_NAS)
      int i;

      for (i = 0; i < S1AP_ENB_DEREGISTERED_IND(received_message_p).nb_ue_to_deregister; i ++) {
        nas_proc_deregister_ue(S1AP_ENB_DEREGISTERED_IND(received_message_p).mme_ue_s1ap_id[i]);
      }

#endif
    }
    break;

    case S1AP_DEREGISTER_UE_REQ: {
#if !defined(DISABLE_USE_NAS)
      nas_proc_deregister_ue(S1AP_DEREGISTER_UE_REQ(received_message_p).mme_ue_s1ap_id);
#endif
    }
    break;

    case TERMINATE_MESSAGE: {
      itti_exit_task();
    }
    break;

    default: {
      NAS_DEBUG("Unkwnon message ID %d:%s from %s\n",
                ITTI_MSG_ID(received_message_p),
                ITTI_MSG_NAME(received_message_p),
                ITTI_MSG_ORIGIN_NAME(received_message_p));
    }
    break;
    }

    itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
    received_message_p = NULL;
  }

  return NULL;
}

int nas_init(mme_config_t *mme_config_p)
{
  NAS_DEBUG("Initializing NAS task interface\n");

#if !defined(DISABLE_USE_NAS)
  nas_log_init(0x3F);
  nas_network_initialize(mme_config_p);
#endif

  if (itti_create_task(TASK_NAS_MME, &nas_intertask_interface,
                       NULL) < 0) {
    NAS_ERROR("Create task failed");
    NAS_DEBUG("Initializing NAS task interface: FAILED\n");
    return -1;
  }

  NAS_DEBUG("Initializing NAS task interface: DONE\n");
  return 0;
}
