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

/*
                                gnb_app.c
                             -------------------
  AUTHOR  : Laurent Winckel, Sebastien ROUX, Lionel GAUTHIER, Navid Nikaein, WEI-TAI CHEN
  COMPANY : EURECOM, NTUST
  EMAIL   : Lionel.Gauthier@eurecom.fr and Navid Nikaein, kroempa@gmail.com
*/

#include <string.h>
#include <stdio.h>
#include <nr_pdcp/nr_pdcp.h>
#include <softmodem-common.h>
#include <split_headers.h>
#include <proto_agent.h>

#include "gnb_app.h"
#include "gnb_config.h"
#include "assertions.h"
#include "common/ran_context.h"

#include "common/utils/LOG/log.h"

#include "x2ap_eNB.h"
#include "intertask_interface.h"
#include "ngap_gNB.h"
#include "sctp_eNB_task.h"
#include "gtpv1u_eNB_task.h"
#include "PHY/INIT/phy_init.h" 
#include "f1ap_cu_task.h"
#include "f1ap_du_task.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include <openair2/LAYER2/nr_pdcp/nr_pdcp.h>
#include "openair2/LAYER2/PDCP_v10.1.0/pdcp.h"
extern unsigned char NB_gNB_INST;

extern RAN_CONTEXT_t RC;

#define GNB_REGISTER_RETRY_DELAY 10

/*------------------------------------------------------------------------------*/
static void configure_nr_rrc(uint32_t gnb_id)
{
  MessageDef *msg_p = NULL;
  //  int CC_id;

  msg_p = itti_alloc_new_message (TASK_GNB_APP, 0, NRRRC_CONFIGURATION_REQ);

  if (RC.nrrrc[gnb_id]) {
    RCconfig_NRRRC(msg_p,gnb_id, RC.nrrrc[gnb_id]);
    
    LOG_I(GNB_APP, "RRC starting with node type %d\n", RC.nrrrc[gnb_id]->node_type);
    LOG_I(GNB_APP,"Sending configuration message to NR_RRC task\n");
    itti_send_msg_to_task (TASK_RRC_GNB, GNB_MODULE_ID_TO_INSTANCE(gnb_id), msg_p);

  }
  else AssertFatal(0,"NRRRC context for gNB %d not allocated\n",gnb_id);
}

/*------------------------------------------------------------------------------*/


static uint32_t gNB_app_register(uint32_t gnb_id_start, uint32_t gnb_id_end)//, const Enb_properties_array_t *enb_properties)
{
  uint32_t         gnb_id;
  MessageDef      *msg_p;
  uint32_t         register_gnb_pending = 0;

  for (gnb_id = gnb_id_start; (gnb_id < gnb_id_end) ; gnb_id++) {
    {
      if(NGAP_CONF_MODE){
        ngap_register_gnb_req_t *ngap_register_gNB; //Type Temporarily reuse
          
        // note:  there is an implicit relationship between the data structure and the message name
        msg_p = itti_alloc_new_message (TASK_GNB_APP, 0, NGAP_REGISTER_GNB_REQ); //Message Temporarily reuse

        RCconfig_NR_NG(msg_p, gnb_id);

        ngap_register_gNB = &NGAP_REGISTER_GNB_REQ(msg_p); //Message Temporarily reuse

        LOG_I(GNB_APP,"default drx %d\n",ngap_register_gNB->default_drx);

        itti_send_msg_to_task (TASK_NGAP, GNB_MODULE_ID_TO_INSTANCE(gnb_id), msg_p);
      }
      if (gnb_id == 0) RCconfig_nr_gtpu();
    }

    LOG_I(GNB_APP,"[gNB %d] gNB_app_register for instance %d\n", gnb_id, GNB_MODULE_ID_TO_INSTANCE(gnb_id));

    register_gnb_pending++;
    }

  return register_gnb_pending;
}


/*------------------------------------------------------------------------------*/
static uint32_t gNB_app_register_x2(uint32_t gnb_id_start, uint32_t gnb_id_end) {
  uint32_t         gnb_id;
  MessageDef      *msg_p;
  uint32_t         register_gnb_x2_pending = 0;

  for (gnb_id = gnb_id_start; (gnb_id < gnb_id_end) ; gnb_id++) {
    {
      msg_p = itti_alloc_new_message (TASK_GNB_APP, 0, X2AP_REGISTER_ENB_REQ);
      LOG_I(X2AP, "GNB_ID: %d \n", gnb_id);
      RCconfig_NR_X2(msg_p, gnb_id);
      itti_send_msg_to_task (TASK_X2AP, ENB_MODULE_ID_TO_INSTANCE(gnb_id), msg_p);
      register_gnb_x2_pending++;
    }
  }

  return register_gnb_x2_pending;
}

/*------------------------------------------------------------------------------*/

static void init_pdcp(void) {
  if (!NODE_IS_DU(RC.nrrrc[0]->node_type)) {
    pdcp_layer_init();
    uint32_t pdcp_initmask = (IS_SOFTMODEM_NOS1) ?
                             (PDCP_USE_NETLINK_BIT | LINK_ENB_PDCP_TO_IP_DRIVER_BIT) : LINK_ENB_PDCP_TO_GTPV1U_BIT;
    if (IS_SOFTMODEM_NOS1) {
      LOG_I(PDCP, "IS_SOFTMODEM_NOS1 option enabled\n");
      pdcp_initmask = pdcp_initmask | ENB_NAS_USE_TUN_BIT | SOFTMODEM_NOKRNMOD_BIT;
    }

    nr_pdcp_module_init(pdcp_initmask, 0);

    if (NODE_IS_CU(RC.nrrrc[0]->node_type)) {
      LOG_I(PDCP, "node is CU, pdcp send rlc_data_req by proto_agent \n");
      pdcp_set_rlc_data_req_func((send_rlc_data_req_func_t)proto_agent_send_rlc_data_req);
    } else {
      LOG_I(PDCP, "node is gNB \n");
      pdcp_set_rlc_data_req_func((send_rlc_data_req_func_t) rlc_data_req);
      pdcp_set_pdcp_data_ind_func((pdcp_data_ind_func_t) pdcp_data_ind);
    }
  } else {
    LOG_I(PDCP, "node is DU, rlc send pdcp_data_ind by proto_agent \n");
    pdcp_set_pdcp_data_ind_func((pdcp_data_ind_func_t) proto_agent_send_pdcp_data_ind);
  }
}

/*------------------------------------------------------------------------------*/

void *gNB_app_task(void *args_p)
{

  uint32_t                        gnb_nb = RC.nb_nr_inst; 
  uint32_t                        gnb_id_start = 0;
  uint32_t                        gnb_id_end = gnb_id_start + gnb_nb;
  uint32_t                        gnb_id;
  MessageDef                      *msg_p           = NULL;
  const char                      *msg_name        = NULL;
  instance_t                      instance;
  int                             result;
  /* for no gcc warnings */
  (void)instance;

  int cell_to_activate = 0;
  itti_mark_task_ready (TASK_GNB_APP);

  LOG_I(PHY, "%s() Task ready initialize structures\n", __FUNCTION__);

  RCconfig_NR_L1();

  if (RC.nb_nr_macrlc_inst>0) RCconfig_nr_macrlc();

  LOG_I(PHY, "%s() RC.nb_nr_L1_inst:%d\n", __FUNCTION__, RC.nb_nr_L1_inst);

  if (RC.nb_nr_L1_inst>0) AssertFatal(l1_north_init_gNB()==0,"could not initialize L1 north interface\n");

  AssertFatal (gnb_nb <= RC.nb_nr_inst,
               "Number of gNB is greater than gNB defined in configuration file (%d/%d)!",
               gnb_nb, RC.nb_nr_inst);

  LOG_I(GNB_APP,"Allocating gNB_RRC_INST for %d instances\n",RC.nb_nr_inst);

  RC.nrrrc = (gNB_RRC_INST **)malloc(RC.nb_nr_inst*sizeof(gNB_RRC_INST *));
  LOG_I(PHY, "%s() RC.nb_nr_inst:%d RC.nrrrc:%p\n", __FUNCTION__, RC.nb_nr_inst, RC.nrrrc);

  for (gnb_id = gnb_id_start; (gnb_id < gnb_id_end) ; gnb_id++) {
    RC.nrrrc[gnb_id] = (gNB_RRC_INST*)malloc(sizeof(gNB_RRC_INST));
    LOG_I(PHY, "%s() Creating RRC instance RC.nrrrc[%d]:%p (%d of %d)\n", __FUNCTION__, gnb_id, RC.nrrrc[gnb_id], gnb_id+1, gnb_id_end);
    memset((void *)RC.nrrrc[gnb_id],0,sizeof(gNB_RRC_INST));
    configure_nr_rrc(gnb_id);
  }

  if (RC.nb_nr_inst > 0)  {
    init_pdcp();
  }

  if (is_x2ap_enabled() ) { //&& !NODE_IS_DU(RC.rrc[0]->node_type)
	  LOG_I(X2AP, "X2AP enabled \n");
	  __attribute__((unused)) uint32_t x2_register_gnb_pending = gNB_app_register_x2 (gnb_id_start, gnb_id_end);
  }

  /* For the CU case the gNB registration with the AMF might have to take place after the F1 setup, as the PLMN info
     * can originate from the DU. Add check on whether x2ap is enabled to account for ENDC NSA scenario.*/
  if ((AMF_MODE_ENABLED || is_x2ap_enabled()) && !NODE_IS_DU(RC.nrrrc[0]->node_type) && !NODE_IS_CU(RC.nrrrc[0]->node_type)) {
    /* Try to register each gNB */
    //registered_gnb = 0;
    __attribute__((unused)) uint32_t register_gnb_pending = gNB_app_register (gnb_id_start, gnb_id_end);
  }
  
  if (RC.nb_nr_inst > 0) {
    if (NODE_IS_CU(RC.nrrrc[0]->node_type)) {

      if (itti_create_task(TASK_CU_F1, F1AP_CU_task, NULL) < 0) {
          LOG_E(F1AP, "Create task for F1AP CU failed\n");
          AssertFatal(1==0,"exiting");
      }
    }

    if (NODE_IS_DU(RC.nrrrc[0]->node_type)) {

      if (itti_create_task(TASK_DU_F1, F1AP_DU_task, NULL) < 0) {
        LOG_E(F1AP, "Create task for F1AP DU failed\n");
        AssertFatal(1==0,"exiting");
      }
      // configure F1AP here for F1C
      LOG_I(GNB_APP,"ngran_gNB_DU: Allocating ITTI message for F1AP_SETUP_REQ\n");
      msg_p = itti_alloc_new_message (TASK_GNB_APP, 0, F1AP_SETUP_REQ);
      RCconfig_NR_DU_F1(msg_p, 0);
      
      itti_send_msg_to_task (TASK_DU_F1, GNB_MODULE_ID_TO_INSTANCE(0), msg_p);
    }
  }
  do {
    // Wait for a message
    itti_receive_msg (TASK_GNB_APP, &msg_p);

    msg_name = ITTI_MSG_NAME (msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      LOG_W(GNB_APP, " *** Exiting GNB_APP thread\n");
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(GNB_APP, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;



    case NGAP_REGISTER_GNB_CNF:
      LOG_I(GNB_APP, "[gNB %ld] Received %s: associated AMF %d\n", instance, msg_name,
            NGAP_REGISTER_GNB_CNF(msg_p).nb_amf);
/*
      DevAssert(register_gnb_pending > 0);
      register_gnb_pending--;

      // Check if at least gNB is registered with one AMF 
      if (NGAP_REGISTER_GNB_CNF(msg_p).nb_amf > 0) {
        registered_gnb++;
      }

      // Check if all register gNB requests have been processed 
      if (register_gnb_pending == 0) {
        if (registered_gnb == gnb_nb) {
          // If all gNB are registered, start L2L1 task 
          MessageDef *msg_init_p;

          msg_init_p = itti_alloc_new_message (TASK_GNB_APP, 0, INITIALIZE_MESSAGE);
          itti_send_msg_to_task (TASK_L2L1, INSTANCE_DEFAULT, msg_init_p);

        } else {
          uint32_t not_associated = gnb_nb - registered_gnb;

          LOG_W(GNB_APP, " %d gNB %s not associated with a AMF, retrying registration in %d seconds ...\n",
                not_associated, not_associated > 1 ? "are" : "is", GNB_REGISTER_RETRY_DELAY);

          // Restart the gNB registration process in GNB_REGISTER_RETRY_DELAY seconds 
          if (timer_setup (GNB_REGISTER_RETRY_DELAY, 0, TASK_GNB_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
                           NULL, &gnb_register_retry_timer_id) < 0) {
            LOG_E(GNB_APP, " Can not start gNB register retry timer, use \"sleep\" instead!\n");

            sleep(GNB_REGISTER_RETRY_DELAY);
            // Restart the registration process 
            registered_gnb = 0;
            register_gnb_pending = gNB_app_register (gnb_id_start, gnb_id_end);//, gnb_properties_p);
          }
        }
      }
*/
      break;

    case F1AP_SETUP_RESP:
      AssertFatal(NODE_IS_DU(RC.nrrrc[0]->node_type), "Should not have received F1AP_SETUP_RESP in CU/gNB\n");

      LOG_I(GNB_APP, "Received %s: associated ngran_gNB_CU %s with %d cells to activate\n", ITTI_MSG_NAME (msg_p),
      F1AP_SETUP_RESP(msg_p).gNB_CU_name,F1AP_SETUP_RESP(msg_p).num_cells_to_activate);
      cell_to_activate = F1AP_SETUP_RESP(msg_p).num_cells_to_activate;
      
      gNB_app_handle_f1ap_setup_resp(&F1AP_SETUP_RESP(msg_p));

      break;
    case F1AP_GNB_CU_CONFIGURATION_UPDATE:
      AssertFatal(NODE_IS_DU(RC.nrrrc[0]->node_type), "Should not have received F1AP_GNB_CU_CONFIGURATION_UPDATE in CU/gNB\n");

      LOG_I(GNB_APP, "Received %s: associated ngran_gNB_CU %s with %d cells to activate\n", ITTI_MSG_NAME (msg_p),
      F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).gNB_CU_name,F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).num_cells_to_activate);
      
      cell_to_activate += F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).num_cells_to_activate;
      gNB_app_handle_f1ap_gnb_cu_configuration_update(&F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p));

      /* Check if at least gNB is registered with one AMF */
      AssertFatal(cell_to_activate == 1,"No cells to activate or cells > 1 %d\n",cell_to_activate);

      break;
    case NGAP_DEREGISTERED_GNB_IND:
      LOG_W(GNB_APP, "[gNB %ld] Received %s: associated AMF %d\n", instance, msg_name,
            NGAP_DEREGISTERED_GNB_IND(msg_p).nb_amf);

      /* TODO handle recovering of registration */
      break;

    case TIMER_HAS_EXPIRED:
      LOG_I(GNB_APP, " Received %s: timer_id %ld\n", msg_name, TIMER_HAS_EXPIRED(msg_p).timer_id);

      //if (TIMER_HAS_EXPIRED (msg_p).timer_id == gnb_register_retry_timer_id) {
        /* Restart the registration process */
      //  registered_gnb = 0;
      //  register_gnb_pending = gNB_app_register(gnb_id_start, gnb_id_end);//, gnb_properties_p);
      //}

      break;

    default:
      LOG_E(GNB_APP, "Received unexpected message %s\n", msg_name);
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);


  return NULL;
}
