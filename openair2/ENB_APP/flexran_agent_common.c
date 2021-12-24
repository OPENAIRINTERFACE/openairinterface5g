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

/*! \file flexran_agent_common.c
 * \brief common primitives for all agents
 * \author Xenofon Foukas, Mohamed Kassem and Navid Nikaein
 * \date 2017
 * \version 0.1
 */

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>

#include "flexran_agent_common.h"
#include "flexran_agent_common_internal.h"
#include "flexran_agent_extern.h"
#include "flexran_agent_net_comm.h"
#include "flexran_agent_ran_api.h"
#include "flexran_agent_phy.h"
#include "flexran_agent_mac.h"
#include "flexran_agent_rrc.h"
#include "flexran_agent_s1ap.h"
#include "flexran_agent_app.h"
//#include "PHY/extern.h"
#include "common/utils/LOG/log.h"
#include "flexran_agent_mac_internal.h"
#include "flexran_agent_rrc_internal.h"

//#include "SCHED/defs.h"
#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "rrc_eNB_UE_context.h"

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

/*
 * message primitives
 */

int flexran_agent_serialize_message(Protocol__FlexranMessage *msg, void **buf, int *size) {
  *size = protocol__flexran_message__get_packed_size(msg);

  if (buf == NULL)
    goto error;

  *buf = malloc(*size);

  if (*buf == NULL)
    goto error;

  protocol__flexran_message__pack(msg, *buf);
  return 0;
error:
  LOG_E(FLEXRAN_AGENT, "an error occured\n"); // change the com
  return -1;
}



/* We assume that the buffer size is equal to the message size.
   Should be chekced durint Tx/Rx */
int flexran_agent_deserialize_message(void *data, int size, Protocol__FlexranMessage **msg) {
  *msg = protocol__flexran_message__unpack(NULL, size, data);

  if (*msg == NULL)
    goto error;

  return 0;
error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}



int flexran_create_header(xid_t xid, Protocol__FlexType type,  Protocol__FlexHeader **header) {
  *header = malloc(sizeof(Protocol__FlexHeader));

  if(*header == NULL)
    goto error;

  protocol__flex_header__init(*header);
  (*header)->version = FLEXRAN_VERSION;
  (*header)->has_version = 1;
  // check if the type is set
  (*header)->type = type;
  (*header)->has_type = 1;
  (*header)->xid = xid;
  (*header)->has_xid = 1;
  return 0;
error:
  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_hello(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header = NULL;
  /*TODO: Need to set random xid or xid from received hello message*/
  xid_t xid = 1;
  Protocol__FlexHello *hello_msg;
  hello_msg = malloc(sizeof(Protocol__FlexHello));

  if(hello_msg == NULL)
    goto error;

  protocol__flex_hello__init(hello_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_HELLO, &header) != 0)
    goto error;

  hello_msg->header = header;
  hello_msg->bs_id  = flexran_get_bs_id(mod_id);
  hello_msg->has_bs_id = 1;
  hello_msg->n_capabilities = flexran_get_capabilities(mod_id, &hello_msg->capabilities);
  hello_msg->n_splits = flexran_get_splits(mod_id, &hello_msg->splits);
  *msg = malloc(sizeof(Protocol__FlexranMessage));

  if(*msg == NULL)
    goto error;

  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_HELLO_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->has_msg_dir = 1;
  (*msg)->hello_msg = hello_msg;
  return 0;
error:

  if(header != NULL)
    free(header);

  if(hello_msg != NULL)
    free(hello_msg);

  if(*msg != NULL)
    free(*msg);

  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_destroy_hello(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_HELLO_MSG)
    goto error;

  free(msg->hello_msg->header);
  free(msg->hello_msg->capabilities);
  free(msg->hello_msg->splits);
  free(msg->hello_msg);
  free(msg);
  return 0;
error:
  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_echo_request(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header = NULL;
  /*TODO: Need to set a random xid*/
  xid_t xid = 1;
  Protocol__FlexEchoRequest *echo_request_msg = NULL;
  echo_request_msg = malloc(sizeof(Protocol__FlexEchoRequest));

  if(echo_request_msg == NULL)
    goto error;

  protocol__flex_echo_request__init(echo_request_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_ECHO_REQUEST, &header) != 0)
    goto error;

  echo_request_msg->header = header;
  *msg = malloc(sizeof(Protocol__FlexranMessage));

  if(*msg == NULL)
    goto error;

  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REQUEST_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->echo_request_msg = echo_request_msg;
  return 0;
error:

  if(header != NULL)
    free(header);

  if(echo_request_msg != NULL)
    free(echo_request_msg);

  if(*msg != NULL)
    free(*msg);

  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_destroy_echo_request(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REQUEST_MSG)
    goto error;

  free(msg->echo_request_msg->header);
  free(msg->echo_request_msg);
  free(msg);
  return 0;
error:
  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}



int flexran_agent_echo_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  xid_t xid;
  Protocol__FlexHeader *header = NULL;
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexEchoRequest *echo_req = input->echo_request_msg;
  xid = (echo_req->header)->xid;
  Protocol__FlexEchoReply *echo_reply_msg = NULL;
  echo_reply_msg = malloc(sizeof(Protocol__FlexEchoReply));

  if(echo_reply_msg == NULL)
    goto error;

  protocol__flex_echo_reply__init(echo_reply_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_ECHO_REPLY, &header) != 0)
    goto error;

  echo_reply_msg->header = header;
  *msg = malloc(sizeof(Protocol__FlexranMessage));

  if(*msg == NULL)
    goto error;

  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->has_msg_dir = 1;
  (*msg)->echo_reply_msg = echo_reply_msg;
  return 0;
error:

  if(header != NULL)
    free(header);

  if(echo_reply_msg != NULL)
    free(echo_reply_msg);

  if(*msg != NULL)
    free(*msg);

  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_destroy_echo_reply(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REPLY_MSG)
    goto error;

  free(msg->echo_reply_msg->header);
  free(msg->echo_reply_msg);
  free(msg);
  return 0;
error:
  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_destroy_enb_config_reply(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REPLY_MSG)
    goto error;

  free(msg->enb_config_reply_msg->header);
  Protocol__FlexEnbConfigReply *reply = msg->enb_config_reply_msg;

  for (int i = 0; i < reply->n_cell_config; i++) {
    if (reply->cell_config[i]->mbsfn_subframe_config_rfoffset)
      free(reply->cell_config[i]->mbsfn_subframe_config_rfoffset);

    if (reply->cell_config[i]->mbsfn_subframe_config_rfperiod)
      free(reply->cell_config[i]->mbsfn_subframe_config_rfperiod);

    if (reply->cell_config[i]->mbsfn_subframe_config_sfalloc)
      free(reply->cell_config[i]->mbsfn_subframe_config_sfalloc);

    /* si_config is shared between MAC and RRC, free here */
    if (reply->cell_config[i]->si_config) {
      for(int j = 0; j < reply->cell_config[i]->si_config->n_si_message; j++) {
        free(reply->cell_config[i]->si_config->si_message[j]);
      }

      free(reply->cell_config[i]->si_config->si_message);
      free(reply->cell_config[i]->si_config);
    }

    if (reply->cell_config[i]->slice_config) {
      flexran_agent_destroy_mac_slice_config(reply->cell_config[i]);
    }

    free(reply->cell_config[i]);
  }
  
  if (reply->s1ap)
    flexran_agent_free_s1ap_cell_config(&reply->s1ap);

  if (reply->loadedapps)
    free(reply->loadedapps);

  if (reply->loadedmacobjects)
    free(reply->loadedmacobjects);

  free(reply->cell_config);
  free(reply);
  free(msg);
  return 0;
error:
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_destroy_ue_config_reply(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_CONFIG_REPLY_MSG)
    goto error;

  free(msg->ue_config_reply_msg->header);
  int i;
  Protocol__FlexUeConfigReply *reply = msg->ue_config_reply_msg;

  for(i = 0; i < reply->n_ue_config; i++) {
    free(reply->ue_config[i]->capabilities);
    free(reply->ue_config[i]);
  }

  free(reply->ue_config);
  free(reply);
  free(msg);
  return 0;
error:
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_destroy_lc_config_reply(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_LC_CONFIG_REPLY_MSG)
    goto error;

  int i, j;
  free(msg->lc_config_reply_msg->header);

  for (i = 0; i < msg->lc_config_reply_msg->n_lc_ue_config; i++) {
    for (j = 0; j < msg->lc_config_reply_msg->lc_ue_config[i]->n_lc_config; j++) {
      free(msg->lc_config_reply_msg->lc_ue_config[i]->lc_config[j]);
    }

    free(msg->lc_config_reply_msg->lc_ue_config[i]->lc_config);
    free(msg->lc_config_reply_msg->lc_ue_config[i]);
  }

  free(msg->lc_config_reply_msg->lc_ue_config);
  free(msg->lc_config_reply_msg);
  free(msg);
  return 0;
error:
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_destroy_enb_config_request(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REQUEST_MSG)
    goto error;

  free(msg->enb_config_request_msg->header);
  free(msg->enb_config_request_msg);
  free(msg);
  return 0;
error:
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_destroy_ue_config_request(Protocol__FlexranMessage *msg) {
  /* TODO: Deallocate memory for a dynamically allocated UE config message */
  return 0;
}

int flexran_agent_destroy_lc_config_request(Protocol__FlexranMessage *msg) {
  /* TODO: Deallocate memory for a dynamically allocated LC config message */
  return 0;
}

// call this function to start a nanosecond-resolution timer
struct timespec timer_start(void) {
  struct timespec start_time;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
  return start_time;
}

// call this function to end a timer, returning nanoseconds elapsed as a long
long timer_end(struct timespec start_time) {
  struct timespec end_time;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
  long diffInNanos = end_time.tv_nsec - start_time.tv_nsec;
  return diffInNanos;
}

int flexran_agent_control_delegation(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexControlDelegation *control_delegation_msg = input->control_delegation_msg;
  *msg = NULL;

  char target[512];
  int len = snprintf(target, sizeof(target), "%s/libflex.%s.so",
                     RC.flexran[mod_id]->cache_name,
                     control_delegation_msg->name);
  if (len >= sizeof(target)) {
    LOG_E(FLEXRAN_AGENT, "target has been truncated, cannot write file name\n");
    return 0;
  }

  if (control_delegation_msg->has_payload) {
    /* use low-level API: check whether exists while creating so we can abort if
     * it exists to not overwrite anything */
    int fd = open(target, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd >= 0) {
      ssize_t l = write(fd,
                        control_delegation_msg->payload.data,
                        control_delegation_msg->payload.len);
      close(fd);
      if (l < control_delegation_msg->payload.len) {
        LOG_E(FLEXRAN_AGENT,
              "could not write complete control delegation to %s: only %ld out of "
              "%ld bytes\n",
              target,
              l,
              control_delegation_msg->payload.len);
        return 0;
      } else if (l < 0) {
        LOG_E(FLEXRAN_AGENT, "can not write control delegation data to %s: %s\n",
              target, strerror(errno));
        return 0;
      }
      LOG_I(FLEXRAN_AGENT, "wrote shared object %s\n", target);
    } else {
      if (errno == EEXIST) {
        LOG_W(FLEXRAN_AGENT, "file %s already exists, remove it first\n", target);
      } else {
        LOG_E(FLEXRAN_AGENT, "can not write control delegation data to %s: %s\n",
              target, strerror(errno));
      }
      return 0;
    }
  } else {
    LOG_W(FLEXRAN_AGENT, "remove file %s\n", target);
    int rc = remove(target);
    if (rc < 0)
      LOG_E(FLEXRAN_AGENT, "cannot remove file %s: %s\n", target, strerror(errno));
  }

  if (control_delegation_msg->has_delegation_type
      && control_delegation_msg->delegation_type == PROTOCOL__FLEX_CONTROL_DELEGATION_TYPE__FLCDT_MAC_DL_UE_SCHEDULER
      && control_delegation_msg->header
      && control_delegation_msg->header->has_xid) {
    /* Inform the MAC subsystem that a control delegation for it has arrived */
    /* TODO this should be triggered by an agent reconfiguration? */
    flexran_agent_mac_inform_delegation(mod_id, control_delegation_msg);
  }

  return 0;
}

int flexran_agent_destroy_control_delegation(Protocol__FlexranMessage *msg) {
  /*TODO: Dealocate memory for a dynamically allocated control delegation message*/
  return 0;
}

int flexran_agent_map_name_to_delegated_object(mid_t mod_id, const char *name,
    char *path, int maxlen) {
  int len = snprintf(path, maxlen, "%s/libflex.%s.so",
                     RC.flexran[mod_id]->cache_name, name);
  if (len >= maxlen) {
    LOG_E(FLEXRAN_AGENT, "path has been truncated, cannot read object\n");
    return -1;
  }

  struct stat buf;
  int status = stat(path, &buf);
  if (status < 0) {
    LOG_E(FLEXRAN_AGENT, "Could not stat object %s: %s\n", path, strerror(errno));
    return -1;
  }
  return 0;
}

int flexran_agent_reconfiguration(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexAgentReconfiguration *agent_reconfiguration_msg = input->agent_reconfiguration_msg;
  if (agent_reconfiguration_msg->policy) {
    /* for compatibility: call old YAML configuration code, although we don't
     * use it anymore */
    apply_reconfiguration_policy(mod_id,
                                 agent_reconfiguration_msg->policy,
                                 strlen(agent_reconfiguration_msg->policy));
  }
  for (int i = 0; i < agent_reconfiguration_msg->n_systems; ++i) {
    const Protocol__FlexAgentReconfigurationSystem *sys = agent_reconfiguration_msg->systems[i];
    if (strcmp(sys->system, "app") == 0) {
      flexran_agent_handle_apps(mod_id, sys->subsystems, sys->n_subsystems);
    } else {
      LOG_E(FLEXRAN_AGENT,
            "unknown system name %s in flex_agent_reconfiguration message\n",
            sys->system);
    }
  }
  *msg = NULL;
  return 0;
}

int flexran_agent_destroy_agent_reconfiguration(Protocol__FlexranMessage *msg) {
  /*TODO: Dealocate memory for a dynamically allocated agent reconfiguration message*/
  return 0;
}

int flexran_agent_destroy_control_delegation_request(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_CONTROL_DEL_REQ_MSG)
    return -1;

  free(msg->control_del_req_msg->header);
  free(msg->control_del_req_msg->name);
  free(msg);
  return 0;
}

int flexran_agent_lc_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  xid_t xid;
  Protocol__FlexHeader *header = NULL;
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexLcConfigRequest *lc_config_request_msg = input->lc_config_request_msg;
  xid = (lc_config_request_msg->header)->xid;
  Protocol__FlexLcConfigReply *lc_config_reply_msg;
  lc_config_reply_msg = malloc(sizeof(Protocol__FlexLcConfigReply));

  if(lc_config_reply_msg == NULL)
    goto error;

  protocol__flex_lc_config_reply__init(lc_config_reply_msg);

  if(flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_GET_LC_CONFIG_REPLY, &header) != 0)
    goto error;

  lc_config_reply_msg->header = header;
  /* the lc_config_reply entirely depends on MAC except for the
   * mac_eNB_get_rrc_status() function (which in the current OAI implementation
   * is reachable if F1 is present). Therefore we check here wether MAC CM is
   * present and the message gets properly filled if it is or remains empty if
   * not */
  lc_config_reply_msg->n_lc_ue_config =
    flexran_agent_get_mac_xface(mod_id) ? flexran_get_mac_num_ues(mod_id) : 0;
  Protocol__FlexLcUeConfig **lc_ue_config = NULL;

  if (lc_config_reply_msg->n_lc_ue_config > 0) {
    lc_ue_config = malloc(sizeof(Protocol__FlexLcUeConfig *) * lc_config_reply_msg->n_lc_ue_config);

    if (lc_ue_config == NULL) {
      goto error;
    }

    // Fill the config for each UE
    for (int i = 0; i < lc_config_reply_msg->n_lc_ue_config; i++) {
      lc_ue_config[i] = malloc(sizeof(Protocol__FlexLcUeConfig));
      if (!lc_ue_config[i]){
        for (int j = 0; j < i; j++){
          free(lc_ue_config[j]);
        }
        free(lc_ue_config);
        goto error;
      }

      protocol__flex_lc_ue_config__init(lc_ue_config[i]);
      const int UE_id = flexran_get_mac_ue_id(mod_id, i);
      flexran_agent_fill_mac_lc_ue_config(mod_id, UE_id, lc_ue_config[i]);
    } // end for UE

    lc_config_reply_msg->lc_ue_config = lc_ue_config;
  } // lc_config_reply_msg->n_lc_ue_config > 0

  *msg = malloc(sizeof(Protocol__FlexranMessage));

  if (*msg == NULL){
    for (int k = 0; k < lc_config_reply_msg->n_lc_ue_config; k++){
      free(lc_ue_config[k]);
    }
    free(lc_ue_config);
    goto error;
  }

  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_LC_CONFIG_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->lc_config_reply_msg = lc_config_reply_msg;
  return 0;
error:

  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);

  if (lc_config_reply_msg != NULL)
    free(lc_config_reply_msg);

  if(*msg != NULL)
    free(*msg);

  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

/*
 * ************************************
 * UE Configuration Reply
 * ************************************
 */

int flexran_agent_ue_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  xid_t xid;
  Protocol__FlexHeader *header = NULL;
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexUeConfigRequest *ue_config_request_msg = input->ue_config_request_msg;
  xid = (ue_config_request_msg->header)->xid;
  Protocol__FlexUeConfigReply *ue_config_reply_msg;
  ue_config_reply_msg = malloc(sizeof(Protocol__FlexUeConfigReply));

  if(ue_config_reply_msg == NULL)
    goto error;

  protocol__flex_ue_config_reply__init(ue_config_reply_msg);

  if(flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_GET_UE_CONFIG_REPLY, &header) != 0)
    goto error;

  ue_config_reply_msg->header = header;
  ue_config_reply_msg->n_ue_config = flexran_agent_get_num_ues(mod_id);

  Protocol__FlexUeConfig **ue_config;

  if (ue_config_reply_msg->n_ue_config > 0) {
    ue_config = malloc(sizeof(Protocol__FlexUeConfig *) * ue_config_reply_msg->n_ue_config);

    if (ue_config == NULL) {
      goto error;
    }

    rnti_t rntis[ue_config_reply_msg->n_ue_config];
    flexran_get_rrc_rnti_list(mod_id, rntis, ue_config_reply_msg->n_ue_config);

    for (int i = 0; i < ue_config_reply_msg->n_ue_config; i++) {
      const rnti_t rnti = rntis[i];
      ue_config[i] = malloc(sizeof(Protocol__FlexUeConfig));
      protocol__flex_ue_config__init(ue_config[i]);

      if (flexran_agent_get_rrc_xface(mod_id))
        flexran_agent_fill_rrc_ue_config(mod_id, rnti, ue_config[i]);

      if (flexran_agent_get_mac_xface(mod_id)) {
        const int UE_id = flexran_get_mac_ue_id_rnti(mod_id, rnti);
        flexran_agent_fill_mac_ue_config(mod_id, UE_id, ue_config[i]);
      }
    }

    ue_config_reply_msg->ue_config = ue_config;
  }

  *msg = malloc(sizeof(Protocol__FlexranMessage));

  if (*msg == NULL)
    goto error;

  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_CONFIG_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->ue_config_reply_msg = ue_config_reply_msg;
  return 0;
error:

  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);

  if (ue_config_reply_msg != NULL)
    free(ue_config_reply_msg);

  if(*msg != NULL)
    free(*msg);

  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


/*
 * ************************************
 * eNB Configuration Request and Reply
 * ************************************
 */

int flexran_agent_enb_config_request(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header = NULL;
  xid_t xid = 1;
  Protocol__FlexEnbConfigRequest *enb_config_request_msg;
  enb_config_request_msg = malloc(sizeof(Protocol__FlexEnbConfigRequest));

  if(enb_config_request_msg == NULL)
    goto error;

  protocol__flex_enb_config_request__init(enb_config_request_msg);

  if(flexran_create_header(xid,PROTOCOL__FLEX_TYPE__FLPT_GET_ENB_CONFIG_REQUEST, &header) != 0)
    goto error;

  enb_config_request_msg->header = header;
  *msg = malloc(sizeof(Protocol__FlexranMessage));

  if(*msg == NULL)
    goto error;

  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REQUEST_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->enb_config_request_msg = enb_config_request_msg;
  return 0;
error:

  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);

  if (enb_config_request_msg != NULL)
    free(enb_config_request_msg);

  if(*msg != NULL)
    free(*msg);

  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_enb_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  xid_t xid;
  Protocol__FlexHeader *header = NULL;
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexEnbConfigRequest *enb_config_req_msg = input->enb_config_request_msg;
  xid = (enb_config_req_msg->header)->xid;
  Protocol__FlexEnbConfigReply *enb_config_reply_msg;
  enb_config_reply_msg = malloc(sizeof(Protocol__FlexEnbConfigReply));

  if(enb_config_reply_msg == NULL)
    goto error;

  protocol__flex_enb_config_reply__init(enb_config_reply_msg);

  if(flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_GET_ENB_CONFIG_REPLY, &header) != 0)
    goto error;

  enb_config_reply_msg->header = header;
  enb_config_reply_msg->n_cell_config = MAX_NUM_CCs;
  Protocol__FlexCellConfig **cell_conf;

  if(enb_config_reply_msg->n_cell_config > 0) {
    cell_conf = malloc(sizeof(Protocol__FlexCellConfig *) * enb_config_reply_msg->n_cell_config);

    if(cell_conf == NULL)
      goto error;

    for(int i = 0; i < enb_config_reply_msg->n_cell_config; i++) {
      cell_conf[i] = malloc(sizeof(Protocol__FlexCellConfig));

      if (!cell_conf[i]) {
        for (int j = 0; j < i; j++) {
          free(cell_conf[j]);
        }
        free(cell_conf);
        goto error;
      }

      protocol__flex_cell_config__init(cell_conf[i]);

      if (flexran_agent_get_phy_xface(mod_id))
        flexran_agent_fill_phy_cell_config(mod_id, i, cell_conf[i]);

      if (flexran_agent_get_rrc_xface(mod_id))
        flexran_agent_fill_rrc_cell_config(mod_id, i, cell_conf[i]);

      if (flexran_agent_get_mac_xface(mod_id))
        flexran_agent_fill_mac_cell_config(mod_id, i, cell_conf[i]);

      cell_conf[i]->carrier_index = i;
      cell_conf[i]->has_carrier_index = 1;
    }
    
    enb_config_reply_msg->cell_config=cell_conf;
  }
  
  if (flexran_agent_get_s1ap_xface(mod_id))
    flexran_agent_fill_s1ap_cell_config(mod_id, &enb_config_reply_msg->s1ap);

  flexran_agent_fill_loaded_apps(mod_id, enb_config_reply_msg);

  flexran_agent_mac_fill_loaded_mac_objects(mod_id, enb_config_reply_msg);

  *msg = malloc(sizeof(Protocol__FlexranMessage));

  if(*msg == NULL) {
    for (int k = 0; k < enb_config_reply_msg->n_cell_config; k++) {
      free(cell_conf[k]);
    }
    free(cell_conf);
    goto error;
  }

  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->enb_config_reply_msg = enb_config_reply_msg;
  return 0;
error:

  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);

  if (enb_config_reply_msg != NULL)
    free(enb_config_reply_msg);

  if(*msg != NULL)
    free(*msg);

  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_rrc_reconfiguration(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexRrcTriggering *triggering = input->rrc_triggering;
  // Set the proper values using FlexRAN API (protected with mutex ?)
  if (!flexran_agent_get_rrc_xface(mod_id)) {
    LOG_E(FLEXRAN_AGENT, "%s(): no RRC present, aborting\n", __func__);
    return -1;
  }

  int num_ue = flexran_get_rrc_num_ues(mod_id);
  if (num_ue == 0)
    return 0;

  rnti_t rntis[num_ue];
  flexran_get_rrc_rnti_list(mod_id, rntis, num_ue);
  for (int i = 0; i < num_ue; i++) {
    const rnti_t rnti = rntis[i];
    const int error = update_rrc_reconfig(mod_id, rnti, triggering);
    if (error < 0) {
      LOG_E(FLEXRAN_AGENT, "Error in updating user %d\n", i);
      continue;
    }
    // Call the proper wrapper in FlexRAN API
    if (flexran_call_rrc_reconfiguration (mod_id, rnti) < 0) {
      LOG_E(FLEXRAN_AGENT, "Error in reconfiguring user %d\n", i);
    }
  }

  *msg = NULL;
  return 0;
}

int flexran_agent_rrc_trigger_handover(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexHoCommand *ho_command = input->ho_command_msg;

  int rnti_found = 0;

  // Set the proper values using FlexRAN API (protected with mutex ?)
  if (!flexran_agent_get_rrc_xface(mod_id)) {
    LOG_E(FLEXRAN_AGENT, "%s(): no RRC present, aborting\n", __func__);
    return -1;
  }

  int num_ue = flexran_get_rrc_num_ues(mod_id);
  if (num_ue == 0)
    return 0;

  if (!ho_command->has_rnti) {
    LOG_E(FLEXRAN_AGENT, "%s(): no UE rnti is present, aborting\n", __func__);
    return -1;
  }

  if (!ho_command->has_target_phy_cell_id) {
    LOG_E(FLEXRAN_AGENT, "%s(): no target physical cell id is  present, aborting\n", __func__);
    return -1;
  }

  rnti_t rntis[num_ue];
  flexran_get_rrc_rnti_list(mod_id, rntis, num_ue);
  for (int i = 0; i < num_ue; i++) {
    const rnti_t rnti = rntis[i];
    if (ho_command->rnti == rnti) {
      rnti_found = 1;
      // Call the proper wrapper in FlexRAN API
      if (flexran_call_rrc_trigger_handover(mod_id, ho_command->rnti, ho_command->target_phy_cell_id) < 0) {
        LOG_E(FLEXRAN_AGENT, "Error in handovering user %d/RNTI %x\n", i, rnti);
      }
      break;
    }
  }

  if (!rnti_found)
    return -1;

  *msg = NULL;
  return 0;
}

int flexran_agent_destroy_rrc_reconfiguration(Protocol__FlexranMessage *msg) {
  // TODO
  return 0;
}

int flexran_agent_destroy_rrc_trigger_handover(Protocol__FlexranMessage *msg) {
  // TODO
  return 0;
}

int flexran_agent_handle_enb_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexEnbConfigReply *enb_config = input->enb_config_reply_msg;

  if (enb_config->n_cell_config > 1)
    LOG_W(FLEXRAN_AGENT, "ignoring slice configs for other cell except cell 0\n");

  if (enb_config->n_cell_config > 0) {
    if (flexran_agent_get_mac_xface(mod_id) && enb_config->cell_config[0]->slice_config) {
      prepare_update_slice_config(mod_id,
                                  &enb_config->cell_config[0]->slice_config,
                                  1 /* request objects if necessary */);
    }
    if (enb_config->cell_config[0]->has_eutra_band
        && enb_config->cell_config[0]->has_dl_freq
        && enb_config->cell_config[0]->has_ul_freq
        && enb_config->cell_config[0]->has_dl_bandwidth) {
      initiate_soft_restart(mod_id, enb_config->cell_config[0]);
    }
    if (flexran_agent_get_rrc_xface(mod_id)
        && enb_config->cell_config[0]->has_x2_ho_net_control) {
      LOG_I(FLEXRAN_AGENT,
            "setting X2 HO NetControl to %d\n",
            enb_config->cell_config[0]->x2_ho_net_control);
      const int rc = flexran_set_x2_ho_net_control(mod_id, enb_config->cell_config[0]->x2_ho_net_control);
      if (rc < 0)
        LOG_E(FLEXRAN_AGENT, "Error in configuring X2 handover controlled by network");
    }
    if (flexran_agent_get_rrc_xface(mod_id) && enb_config->cell_config[0]->n_plmn_id > 0) {
      flexran_agent_handle_plmn_update(mod_id,
                                       0,
                                       enb_config->cell_config[0]->n_plmn_id,
                                       enb_config->cell_config[0]->plmn_id);
    }
  }

  if (flexran_agent_get_s1ap_xface(mod_id) && enb_config->s1ap) {
    flexran_agent_handle_mme_update(mod_id,
                                    enb_config->s1ap->n_mme,
                                    enb_config->s1ap->mme);
  }

  *msg = NULL;
  return 0;
}

int flexran_agent_handle_ue_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  int i;
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexUeConfigReply *ue_config_reply = input->ue_config_reply_msg;

  for (i = 0; flexran_agent_get_mac_xface(mod_id) && i < ue_config_reply->n_ue_config; i++)
    prepare_ue_slice_assoc_update(mod_id, &ue_config_reply->ue_config[i]);
  /* prepare_ue_slice_assoc_update takes ownership of the individual
   * FlexUeConfig messages. Therefore, mark zero messages to not accidentally
   * free them twice */
  ue_config_reply->n_ue_config = 0;

  *msg = NULL;
  return 0;
}

int flexran_agent_get_num_ues(mid_t mod_id) {
  const int has_rrc = flexran_agent_get_rrc_xface(mod_id) != NULL;
  const int has_mac = flexran_agent_get_mac_xface(mod_id) != NULL;
  DevAssert(has_rrc || has_mac);
  if (has_rrc && !has_mac)
    return flexran_get_rrc_num_ues(mod_id);
  if (!has_rrc && has_mac)
    return flexran_get_mac_num_ues(mod_id);

  /* has both */
  const int nrrc = flexran_get_rrc_num_ues(mod_id);
  const int nmac = flexran_get_mac_num_ues(mod_id);
  if (nrrc != nmac) {
    const int n_ue = nrrc < nmac ? nrrc : nmac;
    LOG_E(FLEXRAN_AGENT, "%s(): different numbers of UEs in RRC (%d) and MAC (%d), reporting for %d UEs\n",
          __func__, nrrc, nmac, n_ue);
    return n_ue;
  }
  return nrrc;
}
