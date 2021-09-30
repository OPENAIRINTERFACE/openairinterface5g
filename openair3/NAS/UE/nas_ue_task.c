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

#include "utils.h"
# include "assertions.h"
# include "intertask_interface.h"
# include "nas_ue_task.h"
# include "common/utils/LOG/log.h"
# include "vendor_ext.h"
# include "user_defs.h"
# include "user_api.h"
# include "nas_parser.h"
# include "nas_proc.h"
# include "msc.h"
# include "memory.h"

#include "nas_user.h"
#include <common/utils/msc/msc.h>

// FIXME make command line option for NAS_UE_AUTOSTART
# define NAS_UE_AUTOSTART 1

// FIXME review these externs
extern unsigned char NB_eNB_INST;
extern uint16_t NB_UE_INST;

char *make_port_str_from_ueid(const char *base_port_str, int ueid);

static int nas_ue_process_events(nas_user_container_t *users, struct epoll_event *events, int nb_events)
{
  int event;
  int exit_loop = FALSE;

  LOG_I(NAS, "[UE] Received %d events\n", nb_events);

  for (event = 0; event < nb_events; event++) {
    if (events[event].events != 0) {
      /* If the event has not been yet been processed (not an itti message) */
      nas_user_t *user = find_user_from_fd(users, events[event].data.fd);
      if ( user != NULL ) {
        exit_loop = nas_user_receive_and_process(user, NULL);
      } else {
        LOG_E(NAS, "[UE] Received an event from an unknown fd %d!\n", events[event].data.fd);
      }
    }
  }

  return (exit_loop);
}

// Initialize user api id and port number
void nas_user_api_id_initialize(nas_user_t *user) {
  user_api_id_t *user_api_id = calloc_or_fail(sizeof(user_api_id_t));
  user->user_api_id = user_api_id;
  char *port = make_port_str_from_ueid(NAS_PARSER_DEFAULT_USER_PORT_NUMBER, user->ueid);
  if ( port == NULL ) {
      LOG_E(NAS, "[UE %d] can't get port from ueid!", user->ueid);
      exit (EXIT_FAILURE);
  }
  if (user_api_initialize (user_api_id, NAS_PARSER_DEFAULT_USER_HOSTNAME, port, NULL,
              NULL) != RETURNok) {
      LOG_E(NAS, "[UE %d] user interface initialization failed!", user->ueid);
      exit (EXIT_FAILURE);
  }
  free(port);
  itti_subscribe_event_fd (TASK_NAS_UE, user_api_get_fd(user_api_id));
}

void *nas_ue_task(void *args_p)
{
  int                   nb_events;
  struct epoll_event   *events;
  MessageDef           *msg_p;
  instance_t            instance;
  unsigned int          Mod_id;
  int                   result;
  nas_user_container_t *users=args_p;

  itti_mark_task_ready (TASK_NAS_UE);
  MSC_START_USE();
  LOG_I(NAS, "ue_idx_standalone val:: %u\n", ue_idx_standalone);
  /* Initialize UE NAS (EURECOM-NAS) */
  // Intead of for loop for standalone mode
  if (ue_idx_standalone == 0xFFFF)
  {
    for (int i = 0; i < users->count; i++)
    {
      nas_user_t *user = &users->item[i];
      user->ueid = i;

      /* Get USIM data application filename */
      user->usim_data_store = memory_get_path_from_ueid(USIM_API_NVRAM_DIRNAME, USIM_API_NVRAM_FILENAME, user->ueid);
      if (user->usim_data_store == NULL)
      {
        LOG_E(NAS, "[UE %d] - Failed to get USIM data application filename", user->ueid);
        exit(EXIT_FAILURE);
      }

      /* Get UE's data pathname */
      user->user_nvdata_store = memory_get_path_from_ueid(USER_NVRAM_DIRNAME, USER_NVRAM_FILENAME, user->ueid);
      if (user->user_nvdata_store == NULL)
      {
        LOG_E(NAS, "[UE %d] - Failed to get USIM nvdata filename", user->ueid);
        exit(EXIT_FAILURE);
      }

      /* Get EMM data pathname */
      user->emm_nvdata_store = memory_get_path_from_ueid(EMM_NVRAM_DIRNAME, EMM_NVRAM_FILENAME, user->ueid);
      if (user->emm_nvdata_store == NULL)
      {
        LOG_E(NAS, "[UE %d] - Failed to get EMM nvdata filename", user->ueid);
        exit(EXIT_FAILURE);
      }

      /* Initialize user interface (to exchange AT commands with user process) */
      nas_user_api_id_initialize(user);
      /* allocate needed structures */
      user->user_at_commands = calloc_or_fail(sizeof(user_at_commands_t));
      user->at_response = calloc_or_fail(sizeof(at_response_t));
      user->lowerlayer_data = calloc_or_fail(sizeof(lowerlayer_data_t));
      /* Initialize NAS user */
      nas_user_initialize(user, &user_api_emm_callback, &user_api_esm_callback, FIRMWARE_VERSION);
    }
  }
  else
  {
    // use new parameter passed into lte-uesoftmodem which instead of looping
    // calls functions on specific UE index.

      nas_user_t *user = &users->item[0];
      user->ueid = ue_idx_standalone;
      LOG_I(NAS, "[UE %d] Configuring\n", user->ueid);

      /* Get USIM data application filename */ //
      user->usim_data_store = memory_get_path_from_ueid(USIM_API_NVRAM_DIRNAME, USIM_API_NVRAM_FILENAME, user->ueid);
      if (user->usim_data_store == NULL)
      {
        LOG_E(NAS, "[UE %d] - Failed to get USIM data application filename", user->ueid);
        exit(EXIT_FAILURE);
      }
      /* Get UE's data pathname */
      user->user_nvdata_store = memory_get_path_from_ueid(USER_NVRAM_DIRNAME, USER_NVRAM_FILENAME, user->ueid);
      if (user->user_nvdata_store == NULL)
      {
        LOG_E(NAS, "[UE %d] - Failed to get USIM nvdata filename", user->ueid);
        exit(EXIT_FAILURE);
      }
      /* Get EMM data pathname */
      user->emm_nvdata_store = memory_get_path_from_ueid(EMM_NVRAM_DIRNAME, EMM_NVRAM_FILENAME, user->ueid);
      if (user->emm_nvdata_store == NULL)
      {
        LOG_E(NAS, "[UE %d] - Failed to get EMM nvdata filename", user->ueid);
        exit(EXIT_FAILURE);
      }
      /* Initialize user interface (to exchange AT commands with user process) */
      nas_user_api_id_initialize(user);
      /* allocate needed structures */
      user->user_at_commands = calloc_or_fail(sizeof(user_at_commands_t));
      user->at_response = calloc_or_fail(sizeof(at_response_t));
      user->lowerlayer_data = calloc_or_fail(sizeof(lowerlayer_data_t));
      /* Initialize NAS user */
      nas_user_initialize(user, &user_api_emm_callback, &user_api_esm_callback, FIRMWARE_VERSION);
      user->ueid = 0;
  }

  /* Set UE activation state */
  for (instance = NB_eNB_INST; instance < (NB_eNB_INST + NB_UE_INST); instance++) {
    MessageDef *message_p;

    message_p = itti_alloc_new_message(TASK_NAS_UE, 0, DEACTIVATE_MESSAGE);
    itti_send_msg_to_task(TASK_L2L1, instance, message_p);
  }

  while(1) {
    // Wait for a message or an event
    itti_receive_msg (TASK_NAS_UE, &msg_p);

    if (msg_p != NULL) {
      instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);
      Mod_id = instance - NB_eNB_INST;
      if (instance == INSTANCE_DEFAULT) {
        printf("%s:%d: FATAL: instance is INSTANCE_DEFAULT, should not happen.\n",
               __FILE__, __LINE__);
        exit_fun("exit... \n");
      }
      nas_user_t *user = &users->item[Mod_id];

      switch (ITTI_MSG_ID(msg_p)) {
      case INITIALIZE_MESSAGE:
        LOG_I(NAS, "[UE %d] Received %s\n", Mod_id,  ITTI_MSG_NAME (msg_p));
#if (NAS_UE_AUTOSTART != 0)
        {
          /* Send an activate modem command to NAS like UserProcess should do it */
          char *user_data = "at+cfun=1\r";

          nas_user_receive_and_process (user, user_data);
        }
#endif
        break;

      case TERMINATE_MESSAGE:
        itti_exit_task ();
        break;

      case MESSAGE_TEST:
        LOG_I(NAS, "[UE %d] Received %s\n", Mod_id,  ITTI_MSG_NAME (msg_p));
        break;

      case NAS_CELL_SELECTION_CNF:
        LOG_I(NAS, "[UE %d] Received %s: errCode %u, cellID %u, tac %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_CELL_SELECTION_CNF (msg_p).errCode, NAS_CELL_SELECTION_CNF (msg_p).cellID, NAS_CELL_SELECTION_CNF (msg_p).tac);

        {
          int cell_found = (NAS_CELL_SELECTION_CNF (msg_p).errCode == AS_SUCCESS);

          nas_proc_cell_info (user, cell_found, NAS_CELL_SELECTION_CNF (msg_p).tac,
                              NAS_CELL_SELECTION_CNF (msg_p).cellID, NAS_CELL_SELECTION_CNF (msg_p).rat,
                              NAS_CELL_SELECTION_CNF (msg_p).rsrq, NAS_CELL_SELECTION_CNF (msg_p).rsrp);
        }
        break;

      case NAS_CELL_SELECTION_IND:
        LOG_I(NAS, "[UE %d] Received %s: cellID %u, tac %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_CELL_SELECTION_IND (msg_p).cellID, NAS_CELL_SELECTION_IND (msg_p).tac);

        /* TODO not processed by NAS currently */
        break;

      case NAS_PAGING_IND:
        LOG_I(NAS, "[UE %d] Received %s: cause %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_PAGING_IND (msg_p).cause);

        /* TODO not processed by NAS currently */
        break;

      case NAS_CONN_ESTABLI_CNF:
        LOG_I(NAS, "[UE %d] Received %s: errCode %u, length %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_CONN_ESTABLI_CNF (msg_p).errCode, NAS_CONN_ESTABLI_CNF (msg_p).nasMsg.length);

        if ((NAS_CONN_ESTABLI_CNF (msg_p).errCode == AS_SUCCESS)
            || (NAS_CONN_ESTABLI_CNF (msg_p).errCode == AS_TERMINATED_NAS)) {
          nas_proc_establish_cnf(user, NAS_CONN_ESTABLI_CNF (msg_p).nasMsg.data, NAS_CONN_ESTABLI_CNF (msg_p).nasMsg.length);

          /* TODO checks if NAS will free the nas message, better to do it there anyway! */
          // result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), NAS_CONN_ESTABLI_CNF(msg_p).nasMsg.data);
          // AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
        }

        break;

      case NAS_CONN_RELEASE_IND:
        LOG_I(NAS, "[UE %d] Received %s: cause %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_CONN_RELEASE_IND (msg_p).cause);

        nas_proc_release_ind (user, NAS_CONN_RELEASE_IND (msg_p).cause);
        break;

      case NAS_UPLINK_DATA_CNF:
        LOG_I(NAS, "[UE %d] Received %s: UEid %u, errCode %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_UPLINK_DATA_CNF (msg_p).UEid, NAS_UPLINK_DATA_CNF (msg_p).errCode);

        if (NAS_UPLINK_DATA_CNF (msg_p).errCode == AS_SUCCESS) {
          nas_proc_ul_transfer_cnf (user);
        } else {
          nas_proc_ul_transfer_rej (user);
        }

        break;

      case NAS_DOWNLINK_DATA_IND:
        LOG_I(NAS, "[UE %d] Received %s: UEid %u, length %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_DOWNLINK_DATA_IND (msg_p).UEid, NAS_DOWNLINK_DATA_IND (msg_p).nasMsg.length);

        nas_proc_dl_transfer_ind (user, NAS_DOWNLINK_DATA_IND(msg_p).nasMsg.data, NAS_DOWNLINK_DATA_IND(msg_p).nasMsg.length);

        if (0) {
          /* TODO checks if NAS will free the nas message, better to do it there anyway! */
          result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), NAS_DOWNLINK_DATA_IND(msg_p).nasMsg.data);
          AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
        }

        break;

      default:
        LOG_E(NAS, "[UE %d] Received unexpected message %s\n", Mod_id,  ITTI_MSG_NAME (msg_p));
        break;
      }

      result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
      AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
      msg_p = NULL;
    }

    nb_events = itti_get_events(TASK_NAS_UE, &events);

    if ((nb_events > 0) && (events != NULL)) {
      if (nas_ue_process_events(users, events, nb_events) == TRUE) {
        LOG_E(NAS, "[UE] Received exit loop\n");
      }
    }
  }

  free(users);
  return NULL;
}

nas_user_t *find_user_from_fd(nas_user_container_t *users, int fd) {
  for (int i=0; i<users->count; i++) {
    nas_user_t *user = &users->item[i];
    if (fd == user_api_get_fd(user->user_api_id)) {
      return user;
    }
  }
  return NULL;
}

char *make_port_str_from_ueid(const char *base_port_str, int ueid) {
  int port;
  int base_port;
  char *endptr = NULL;

  base_port = strtol(base_port_str, &endptr, 10);
  if ( base_port_str == endptr ) {
    return NULL;
  }

  port = base_port + ueid;
  if ( port<1 || port > 65535 ) {
    return NULL;
  }

  return itoa(port);
}
