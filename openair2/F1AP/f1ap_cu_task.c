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

/*! \file openair2/F1AP/f1ap_cu_task.c
* \brief data structures for F1 interface modules
* \author EURECOM/NTUST
* \date 2018
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr, bing-kai.hong@eurecom.fr
* \note
* \warning
*/

#include "f1ap_common.h"
#include "f1ap_cu_interface_management.h"
#include "f1ap_cu_rrc_message_transfer.h"
#include "f1ap_cu_ue_context_management.h"
#include "f1ap_cu_task.h"
#include "proto_agent.h"

void cu_task_handle_sctp_association_ind(instance_t instance, sctp_new_association_ind_t *sctp_new_association_ind) {
  createF1inst(true, instance, NULL);
  // save the assoc id
  f1ap_setup_req_t *f1ap_cu_data=f1ap_req(true, instance);
  f1ap_cu_data->assoc_id         = sctp_new_association_ind->assoc_id;
  f1ap_cu_data->sctp_in_streams  = sctp_new_association_ind->in_streams;
  f1ap_cu_data->sctp_out_streams = sctp_new_association_ind->out_streams;
  f1ap_cu_data->default_sctp_stream_id = 0;
  // Nothing
}

void cu_task_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp) {
  DevAssert(sctp_new_association_resp != NULL);

  if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
    LOG_W(F1AP, "Received unsuccessful result for SCTP association (%u), instance %ld, cnx_id %u\n",
          sctp_new_association_resp->sctp_state,
          instance,
          sctp_new_association_resp->ulp_cnx_id);
    //if (sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN)
    //proto_agent_stop(instance);
    //f1ap_handle_setup_message(instance, sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);
    return; // exit -1 for debugging
  }

  /* setup parameters for F1U and start the server */
  const cudu_params_t params = {
    .local_ipv4_address  = RC.nrrrc[instance]->eth_params_s.my_addr,
    .local_port          = RC.nrrrc[instance]->eth_params_s.my_portd,
    .remote_ipv4_address = RC.nrrrc[instance]->eth_params_s.remote_addr,
    .remote_port         = RC.nrrrc[instance]->eth_params_s.remote_portd
  };
  //AssertFatal(proto_agent_start(instance, &params) == 0,
  //          "could not start PROTO_AGENT for F1U on instance %ld!\n", instance);
}

void cu_task_handle_sctp_data_ind(instance_t instance, sctp_data_ind_t *sctp_data_ind) {
  int result;
  DevAssert(sctp_data_ind != NULL);
  f1ap_handle_message(instance, sctp_data_ind->assoc_id, sctp_data_ind->stream,
                      sctp_data_ind->buffer, sctp_data_ind->buffer_length);
  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
}

void cu_task_send_sctp_init_req(instance_t enb_id) {
  // 1. get the itti msg, and retrive the enb_id from the message
  // 2. use RC.rrc[enb_id] to fill the sctp_init_t with the ip, port
  // 3. creat an itti message to init
  LOG_I(F1AP, "F1AP_CU_SCTP_REQ(create socket)\n");
  MessageDef  *message_p = NULL;
  message_p = itti_alloc_new_message (TASK_CU_F1, 0, SCTP_INIT_MSG);
  message_p->ittiMsg.sctp_init.port = F1AP_PORT_NUMBER;
  message_p->ittiMsg.sctp_init.ppid = F1AP_SCTP_PPID;
  message_p->ittiMsg.sctp_init.ipv4 = 1;
  message_p->ittiMsg.sctp_init.ipv6 = 0;
  message_p->ittiMsg.sctp_init.nb_ipv4_addr = 1;

  if (RC.nrrrc[0]->node_type == ngran_gNB_CU) {
    message_p->ittiMsg.sctp_init.ipv4_address[0] = inet_addr(RC.nrrrc[enb_id]->eth_params_s.my_addr);
  } else {
    message_p->ittiMsg.sctp_init.ipv4_address[0] = inet_addr(RC.rrc[enb_id]->eth_params_s.my_addr);
  }

  /*
   * SR WARNING: ipv6 multi-homing fails sometimes for localhost.
   * * * * Disable it for now.
   */
  message_p->ittiMsg.sctp_init.nb_ipv6_addr = 0;
  message_p->ittiMsg.sctp_init.ipv6_address[0] = "0:0:0:0:0:0:0:1";
  itti_send_msg_to_task(TASK_SCTP, enb_id, message_p);
}

void *
F1AP_CU_task(void *arg) {
  MessageDef *received_msg = NULL;
  int         result;
  LOG_I(F1AP, "Starting F1AP at CU\n");
  // no RLC in CU, initialize mem pool for PDCP
  pool_buffer_init();
  itti_mark_task_ready(TASK_CU_F1);
  cu_task_send_sctp_init_req(0);

  while (1) {
    itti_receive_msg(TASK_CU_F1, &received_msg);

    switch (ITTI_MSG_ID(received_msg)) {
      case SCTP_NEW_ASSOCIATION_IND:
        LOG_I(F1AP, "CU Task Received SCTP_NEW_ASSOCIATION_IND for instance %ld\n",
              ITTI_MSG_DESTINATION_INSTANCE(received_msg));
        cu_task_handle_sctp_association_ind(ITTI_MSG_ORIGIN_INSTANCE(received_msg),
                                            &received_msg->ittiMsg.sctp_new_association_ind);
        break;

      case SCTP_NEW_ASSOCIATION_RESP:
        LOG_I(F1AP, "CU Task Received SCTP_NEW_ASSOCIATION_RESP for instance %ld\n",
              ITTI_MSG_DESTINATION_INSTANCE(received_msg));
        cu_task_handle_sctp_association_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                             &received_msg->ittiMsg.sctp_new_association_resp);
        break;

      case SCTP_DATA_IND:
        LOG_I(F1AP, "CU Task Received SCTP_DATA_IND for Instance %ld\n",
              ITTI_MSG_DESTINATION_INSTANCE(received_msg));
        cu_task_handle_sctp_data_ind(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                     &received_msg->ittiMsg.sctp_data_ind);
        break;

      case F1AP_SETUP_RESP: // from rrc
        LOG_I(F1AP, "CU Task Received F1AP_SETUP_RESP\n");
        CU_send_F1_SETUP_RESPONSE(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                  &F1AP_SETUP_RESP(received_msg));
        break;

      case F1AP_GNB_CU_CONFIGURATION_UPDATE: // from rrc
        LOG_I(F1AP, "CU Task Received F1AP_GNB_CU_CONFIGURAITON_UPDATE\n");
        // CU_send_f1setup_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
        //                                       &F1AP_SETUP_RESP(received_msg));
        CU_send_gNB_CU_CONFIGURATION_UPDATE(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                            &F1AP_GNB_CU_CONFIGURATION_UPDATE(received_msg));
        break;

      case F1AP_DL_RRC_MESSAGE: // from rrc
        LOG_I(F1AP, "CU Task Received F1AP_DL_RRC_MESSAGE\n");
        CU_send_DL_RRC_MESSAGE_TRANSFER(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                        &F1AP_DL_RRC_MESSAGE(received_msg));
        break;

      case F1AP_UE_CONTEXT_SETUP_REQ: // from rrc
        LOG_I(F1AP, "CU Task Received F1AP_UE_CONTEXT_SETUP_REQ\n");
        CU_send_UE_CONTEXT_SETUP_REQUEST(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                         &F1AP_UE_CONTEXT_SETUP_REQ(received_msg));
        break;

      case F1AP_UE_CONTEXT_RELEASE_CMD: // from rrc
        LOG_I(F1AP, "CU Task Received F1AP_UE_CONTEXT_RELEASE_CMD\n");
        CU_send_UE_CONTEXT_RELEASE_COMMAND(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                           &F1AP_UE_CONTEXT_RELEASE_CMD(received_msg));
        break;

      //    case F1AP_SETUP_RESPONSE: // This is from RRC
      //    CU_send_F1_SETUP_RESPONSE(instance, *f1ap_setup_ind, &(F1AP_SETUP_RESP) f1ap_setup_resp)
      //        break;

      //    case F1AP_SETUP_FAILURE: // This is from RRC
      //    CU_send_F1_SETUP_FAILURE(instance, *f1ap_setup_ind, &(F1AP_SETUP_FAILURE) f1ap_setup_failure)
      //       break;

      case TERMINATE_MESSAGE:
        LOG_W(F1AP, " *** Exiting F1AP thread\n");
        itti_exit_task();
        break;

      default:
        LOG_E(F1AP, "CU Received unhandled message: %d:%s\n",
              ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
        break;
    } // switch

    result = itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    received_msg = NULL;
  } // while

  return NULL;
}
