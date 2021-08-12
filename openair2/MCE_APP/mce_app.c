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
                                mce_app.c
                             -------------------
  AUTHOR  : Javier Morgade
  COMPANY : VICOMTECH Spain
  EMAIL   : javier.morgade@ieee.org
*/

#include <string.h>
#include <stdio.h>

#include "mce_app.h"
#include "mce_config.h"
#include "assertions.h"
#include "common/ran_context.h"
#include "targets/RT/USER/lte-softmodem.h"

#include "common/utils/LOG/log.h"

# include "intertask_interface.h"
#   include "s1ap_eNB.h"
#   include "sctp_eNB_task.h"
#   include "gtpv1u_eNB_task.h"
#   include "flexran_agent.h"

#   include "x2ap_eNB.h"
#   include "x2ap_messages_types.h"
#   include "m2ap_eNB.h"
#   include "m2ap_MCE.h"
#   include "m2ap_messages_types.h"
#   include "m3ap_MCE.h"
#   include "m3ap_messages_types.h"
#   define X2AP_ENB_REGISTER_RETRY_DELAY   10

#include "openair1/PHY/INIT/phy_init.h"

extern RAN_CONTEXT_t RC;

#   define MCE_REGISTER_RETRY_DELAY 10

#include "targets/RT/USER/lte-softmodem.h"

static m2ap_mbms_scheduling_information_t * m2ap_mbms_scheduling_information_local = NULL;
static m2ap_setup_resp_t * m2ap_setup_resp_local = NULL;
static m2ap_setup_req_t * m2ap_setup_req_local = NULL;


/*------------------------------------------------------------------------------*/

static uint32_t MCE_app_register(ngran_node_t node_type,uint32_t mce_id_start, uint32_t mce_id_end) {
  uint32_t         mce_id;
  MessageDef      *msg_p;
  uint32_t         register_mce_pending = 0;

  for (mce_id = mce_id_start; (mce_id < mce_id_end) ; mce_id++) {
    {
      // M3AP registration
        /* note:  there is an implicit relationship between the data structure and the message name */
        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M3AP_REGISTER_MCE_REQ);
        //RCconfig_S1(msg_p, mce_id);

        //if (mce_id == 0) 
		//RCconfig_gtpu();

        //LOG_I(MCE_APP,"default drx %d\n",((M3AP_REGISTER_MCE_REQ(msg_p)).default_drx));

        LOG_I(ENB_APP,"[MCE %d] MCE_app_register via M3AP for instance %d\n", mce_id, ENB_MODULE_ID_TO_INSTANCE(mce_id));
        itti_send_msg_to_task (TASK_M3AP, ENB_MODULE_ID_TO_INSTANCE(mce_id), msg_p);

      //if (NODE_IS_DU(node_type)) { // F1AP registration
      //  // configure F1AP here for F1C
      //  LOG_I(ENB_APP,"ngran_eNB_DU: Allocating ITTI message for F1AP_SETUP_REQ\n");
      //  msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, F1AP_SETUP_REQ);
      //  RCconfig_DU_F1(msg_p, enb_id);

      //  LOG_I(ENB_APP,"[eNB %d] eNB_app_register via F1AP for instance %d\n", enb_id, ENB_MODULE_ID_TO_INSTANCE(enb_id));
      //  itti_send_msg_to_task (TASK_DU_F1, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
      //  // configure GTPu here for F1U
      //}
      //else { // S1AP registration
      //  /* note:  there is an implicit relationship between the data structure and the message name */
      //  msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, S1AP_REGISTER_ENB_REQ);
      //  RCconfig_S1(msg_p, enb_id);

      //  if (enb_id == 0) RCconfig_gtpu();

      //  LOG_I(ENB_APP,"default drx %d\n",((S1AP_REGISTER_ENB_REQ(msg_p)).default_drx));

      //  LOG_I(ENB_APP,"[eNB %d] eNB_app_register via S1AP for instance %d\n", enb_id, ENB_MODULE_ID_TO_INSTANCE(enb_id));
      //  itti_send_msg_to_task (TASK_S1AP, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
      //}

      register_mce_pending++;
    }
  }

  return register_mce_pending;
}


/*------------------------------------------------------------------------------*/
//static uint32_t MCE_app_register_x2(uint32_t mce_id_start, uint32_t mce_id_end) {
//  uint32_t         mce_id;
//  MessageDef      *msg_p;
//  uint32_t         register_mce_m2_pending = 0;
//
//  for (mce_id = mce_id_start; (mce_id < mce_id_end) ; mce_id++) {
//    {
//      msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, X2AP_REGISTER_ENB_REQ);
//      RCconfig_X2(msg_p, mce_id);
//      itti_send_msg_to_task (TASK_X2AP, ENB_MODULE_ID_TO_INSTANCE(mce_id), msg_p);
//      register_mce_x2_pending++;
//    }
//  }
//
//  return register_mce_x2_pending;
//}

/*------------------------------------------------------------------------------*/
//static uint32_t MCE_app_register_m2(uint32_t mce_id_start, uint32_t mce_id_end) {
//  uint32_t         mce_id;
//  MessageDef      *msg_p;
//  uint32_t         register_mce_m2_pending = 0;
//
//  LOG_W(MCE_APP,"Register ...");
//  for (mce_id = mce_id_start; (mce_id < mce_id_end) ; mce_id++) {
//    {
//  //	LOG_W(MCE_APP,"Register commes inside ...\n");
//      msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M2AP_REGISTER_MCE_REQ);
//      //RCconfig_M2_MCE(msg_p, mce_id);
//      itti_send_msg_to_task (TASK_M2AP_MCE, ENB_MODULE_ID_TO_INSTANCE(mce_id), msg_p);
//  //	LOG_W(MCE_APP,"Register sent ...\n");
//      register_mce_m2_pending++;
//    }
//  }
//
//  return register_mce_m2_pending;
//}
//
/*------------------------------------------------------------------------------*/
static uint32_t MCE_app_register_m3(uint32_t mce_id_start, uint32_t mce_id_end) {
  uint32_t         mce_id;
  MessageDef      *msg_p;
  uint32_t         register_mce_m3_pending = 0;

  LOG_D(MCE_APP,"Register ...\n");
  for (mce_id = mce_id_start; (mce_id < mce_id_end) ; mce_id++) {
    {
  //	LOG_W(MCE_APP,"Register commes inside ...\n");
      msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M3AP_REGISTER_MCE_REQ);
      RCconfig_M3(msg_p, mce_id);
      itti_send_msg_to_task (TASK_M3AP_MCE, ENB_MODULE_ID_TO_INSTANCE(mce_id), msg_p);
  	LOG_D(MCE_APP,"Register sent ...\n");
      register_mce_m3_pending++;
    }
  }

  return register_mce_m3_pending;
}

/***************************  M3AP MCE handle **********************************/
//static uint32_t MCE_app_handle_m3ap_mbms_session_start_req(instance_t instance){
//  	//uint32_t         mce_id=0;
//  //	MessageDef      *msg_p;
//  //      msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M3AP_MBMS_SESSION_START_RESP);
//  //      itti_send_msg_to_task (TASK_M3AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
//	
//	
//	return 0;
//}

static uint32_t MCE_app_handle_m3ap_mbms_session_stop_req(instance_t instance){
  	//uint32_t         mce_id=0;
  	MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M3AP_MBMS_SESSION_STOP_RESP);
        itti_send_msg_to_task (TASK_M3AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
	
	return 0;
}

static uint32_t MCE_app_handle_m3ap_mbms_session_update_req(instance_t instance){
  	//uint32_t         mce_id=0;
  	MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M3AP_MBMS_SESSION_UPDATE_RESP);
        itti_send_msg_to_task (TASK_M3AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
	
	return 0;
}
//// end M3AP MCE handle **********************************/


/***************************  M2AP MCE handle **********************************/
static uint32_t MCE_app_handle_m2ap_setup_req(instance_t instance){
	
  	//uint32_t         mce_id=0;
  	MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M2AP_SETUP_RESP);
	if(m2ap_setup_resp_local)
		memcpy(&M2AP_SETUP_RESP(msg_p),m2ap_setup_resp_local,sizeof(m2ap_setup_resp_t));
	else
		RCconfig_M2_MCCH(msg_p,0);

	
        itti_send_msg_to_task (TASK_M2AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
	
	return 0;
}

static uint32_t MCE_app_handle_m2ap_mbms_session_start_resp(instance_t instance){
	MessageDef      *msg_p;
  	msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M3AP_MBMS_SESSION_START_RESP); 
	itti_send_msg_to_task (TASK_M3AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
	return 0;
}

//// end M2AP MCE handle **********************************/


/***************************  M2AP MCE send **********************************/
static uint32_t MCE_app_send_m2ap_mbms_scheduling_information(instance_t instance){
	
  	//uint32_t         mce_id=0;
  	MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M2AP_MBMS_SCHEDULING_INFORMATION);
	if(m2ap_mbms_scheduling_information_local)
		memcpy(&M2AP_MBMS_SCHEDULING_INFORMATION(msg_p),m2ap_mbms_scheduling_information_local,sizeof(m2ap_mbms_scheduling_information_t));
	else
		RCconfig_M2_SCHEDULING(msg_p,0);
        itti_send_msg_to_task (TASK_M2AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
	
	return 0;
}

static uint32_t MCE_app_send_m2ap_session_start_req(instance_t instance){
	
  	//uint32_t         mce_id=0;
  	MessageDef      *msg_p;
        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M2AP_MBMS_SESSION_START_REQ);
        itti_send_msg_to_task (TASK_M2AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
	
	return 0;
}
//static uint32_t MCE_app_send_m2ap_session_stop_req(instance_t instance){
//	
//  	//uint32_t         mce_id=0;
//  	MessageDef      *msg_p;
//        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M2AP_MBMS_SESSION_STOP_REQ);
//        itti_send_msg_to_task (TASK_M2AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
//	
//	return 0;
//}
//static uint32_t MCE_app_send_m2ap_mce_configuration_update(instance_t instance){
//	
//  	//uint32_t         mce_id=0;
//  	MessageDef      *msg_p;
//        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M2AP_MCE_CONFIGURATION_UPDATE);
//        itti_send_msg_to_task (TASK_M2AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
//	
//	return 0;
//}
//static uint32_t MCE_app_send_m2ap_enb_configuration_update_ack(instance_t instance){
//	
//  	//uint32_t         mce_id=0;
//  	MessageDef      *msg_p;
//        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M2AP_ENB_CONFIGURATION_UPDATE_ACK);
//        itti_send_msg_to_task (TASK_M2AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
//	
//	return 0;
//}
//static uint32_t MCE_app_send_m2ap_enb_configuration_update_failure(instance_t instance){
//	
//  	//uint32_t         mce_id=0;
//  	MessageDef      *msg_p;
//        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, M2AP_ENB_CONFIGURATION_UPDATE_FAILURE);
//        itti_send_msg_to_task (TASK_M2AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
//	
//	return 0;
//} //// end M2AP MCE send **********************************/

//static uint32_t MCE_app_send_MME_APP(instance_t instance){
//	
//  	//uint32_t         mce_id=0;
//  	MessageDef      *msg_p;
//        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, MESSAGE_TEST);
//        itti_send_msg_to_task (TASK_MME_APP, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
//	
//	return 0;
//}

//static uint32_t MCE_app_send_MME_APP2(instance_t instance){
//	
//  	//uint32_t         mce_id=0;
//  	MessageDef      *msg_p;
//        msg_p = itti_alloc_new_message (TASK_MCE_APP, 0, MESSAGE_TEST);
//        itti_send_msg_to_task (TASK_M3AP_MCE, ENB_MODULE_ID_TO_INSTANCE(instance), msg_p);
//	
//	return 0;
//}

/*------------------------------------------------------------------------------*/
void *MCE_app_task(void *args_p) {
  uint32_t                        mce_nb = 1;//RC.nb_inst; 
  uint32_t                        mce_id_start = 0;
  uint32_t                        mce_id_end = mce_id_start + mce_nb;
  uint32_t                        register_mce_pending=0;
  uint32_t                        registered_mce=0;
  //long                            mce_register_retry_timer_id;
  long 				  mce_scheduling_info_timer_id;
  //uint32_t                        m3_register_mce_pending = 0;
 // uint32_t                        x2_registered_mce = 0;
 // long                            x2_mce_register_retry_timer_id;
 // uint32_t                        m2_register_mce_pending = 0;
 // uint32_t                        m2_registered_mce = 0;
 // long                            m2_mce_register_retry_timer_id;
  MessageDef                     *msg_p           = NULL;
  instance_t                      instance;
  int                             result;
  /* for no gcc warnings */
  (void)instance;
  itti_mark_task_ready (TASK_MCE_APP);

  /* Try to register each MCE */
  // This assumes that node_type of all RRC instances is the same
  if ( EPC_MODE_ENABLED && RC.rrc == NULL )
	  LOG_E(RRC, "inconsistent global variables\n");
  if (EPC_MODE_ENABLED && RC.rrc ) {
    register_mce_pending = MCE_app_register(RC.rrc[0]->node_type, mce_id_start, mce_id_end);
  }

    /* Try to register each MCE with each other */
 // if (is_x2ap_enabled() && !NODE_IS_DU(RC.rrc[0]->node_type)) {
 //   x2_register_enb_pending = MCE_app_register_x2 (enb_id_start, enb_id_end);
 // }
 // MCE_app_send_MME_APP2(0);


 if (is_m2ap_MCE_enabled() /*&& !NODE_IS_DU(RC.rrc[0]->node_type)*/) {
  RCconfig_MCE();

  if(!m2ap_mbms_scheduling_information_local)
	m2ap_mbms_scheduling_information_local = (m2ap_mbms_scheduling_information_t*)calloc(1,sizeof(m2ap_mbms_scheduling_information_t));
  if(m2ap_mbms_scheduling_information_local)
  	RCconfig_m2_scheduling(m2ap_mbms_scheduling_information_local,0);

  if(!m2ap_setup_resp_local)
	m2ap_setup_resp_local = (m2ap_setup_resp_t*)calloc(1,sizeof(m2ap_setup_resp_t));
  if(m2ap_setup_resp_local)
	RCconfig_m2_mcch(m2ap_setup_resp_local,0);
 }

 // /* Try to register each MCE with MCE each other */
 if (is_m3ap_MCE_enabled() /*&& !NODE_IS_DU(RC.rrc[0]->node_type)*/) {
   	///*m3_register_mce_pending =*/ 
	MCE_app_register_m3 (mce_id_start, mce_id_end);
   }

  do {
    // Wait for a message
    itti_receive_msg (TASK_MCE_APP, &msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      LOG_W(MCE_APP, " *** Exiting MCE_APP thread\n");
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(MCE_APP, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;

    case SOFT_RESTART_MESSAGE:
      //handle_reconfiguration(instance);
      break;

    case M3AP_REGISTER_MCE_CNF:
      //AssertFatal(!NODE_IS_DU(RC.rrc[0]->node_type), "Should not have received S1AP_REGISTER_ENB_CNF\n");
          LOG_I(MCE_APP, "[MCE %ld] Received %s: associated MME %d\n", instance, ITTI_MSG_NAME (msg_p),
                M3AP_REGISTER_MCE_CNF(msg_p).nb_mme);
          DevAssert(register_mce_pending > 0);
          register_mce_pending--;

          /* Check if at least MCE is registered with one MME */
          if (M3AP_REGISTER_MCE_CNF(msg_p).nb_mme > 0) {
            registered_mce++;
          }

          /* Check if all register MCE requests have been processed */
          if (register_mce_pending == 0) {
            if (registered_mce == mce_nb) {
              /* If all MCE are registered, start L2L1 task */
             // MessageDef *msg_init_p;
             // msg_init_p = itti_alloc_new_message (TASK_ENB_APP, 0, INITIALIZE_MESSAGE);
             // itti_send_msg_to_task (TASK_L2L1, INSTANCE_DEFAULT, msg_init_p);
            } else {
              LOG_W(MCE_APP, " %d MCE not associated with a MME, retrying registration in %d seconds ...\n",
                    mce_nb - registered_mce,  MCE_REGISTER_RETRY_DELAY);

            //  /* Restart the MCE registration process in MCE_REGISTER_RETRY_DELAY seconds */
            //  if (timer_setup (MCE_REGISTER_RETRY_DELAY, 0, TASK_MCE_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
            //                   NULL, &mce_register_retry_timer_id) < 0) {
            //    LOG_E(MCE_APP, " Can not start MCE register retry timer, use \"sleep\" instead!\n");
            //    sleep(MCE_REGISTER_RETRY_DELAY);
            //    /* Restart the registration process */
            //    registered_mce = 0;
            //    register_mce_pending = MCE_app_register (RC.rrc[0]->node_type,mce_id_start, mce_id_end);
            //  }
            }
          }

      break;

   case M3AP_MBMS_SESSION_START_REQ:
        LOG_I(MCE_APP, "Received M3AP_MBMS_SESSION_START_REQ message %s\n", ITTI_MSG_NAME (msg_p));
	//MCE_app_handle_m3ap_mbms_session_start_req(ITTI_MSG_DESTINATION_INSTANCE(msg_p));
	if(m2ap_setup_req_local)
		if (timer_setup (2, 0, TASK_MCE_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
                               NULL, &mce_scheduling_info_timer_id) < 0) {
   		}


	break;

   case M3AP_MBMS_SESSION_STOP_REQ:
        LOG_I(MCE_APP, "Received M3AP_MBMS_SESSION_STOP_REQ message %s\n", ITTI_MSG_NAME (msg_p));
	MCE_app_handle_m3ap_mbms_session_stop_req(ITTI_MSG_DESTINATION_INSTANCE(msg_p));
	break;

   case M3AP_MBMS_SESSION_UPDATE_REQ:
        LOG_I(MCE_APP, "Received M3AP_MBMS_SESSION_UPDATE_REQ message %s\n", ITTI_MSG_NAME (msg_p));
	MCE_app_handle_m3ap_mbms_session_update_req(ITTI_MSG_DESTINATION_INSTANCE(msg_p));
	break;



   case M3AP_SETUP_RESP:
      LOG_I(MCE_APP, "Received M3AP_SETUP_RESP message %s\n", ITTI_MSG_NAME (msg_p));
   //   //AssertFatal(NODE_IS_DU(RC.rrc[0]->node_type), "Should not have received F1AP_REGISTER_ENB_CNF in CU/MCE\n");

   //   //LOG_I(MCE_APP, "Received %s: associated ngran_MCE_CU %s with %d cells to activate\n", ITTI_MSG_NAME (msg_p),
   //         //F1AP_SETUP_RESP(msg_p).gNB_CU_name,F1AP_SETUP_RESP(msg_p).num_cells_to_activate);
   //   
   //   //handle_f1ap_setup_resp(&F1AP_SETUP_RESP(msg_p));
   //   handle_m3ap_setup_resp(&M3AP_SETUP_RESP(msg_p));

   //   DevAssert(register_mce_pending > 0);
   //   register_mce_pending--;

   //   /* Check if at least MCE is registered with one MME */
   //   //if (M3AP_SETUP_RESP(msg_p).num_cells_to_activate > 0) {
   //   //  registered_enb++;
   //   //}

   //   /* Check if all register MCE requests have been processed */
   //   if (register_mce_pending == 0) {
   //     if (registered_mce == mce_nb) {
   //       /* If all MCE cells are registered, start L2L1 task */
   //       MessageDef *msg_init_p;

   //       //msg_init_p = itti_alloc_new_message (TASK_MCE_APP, 0, INITIALIZE_MESSAGE);
   //       //itti_send_msg_to_task (TASK_L2L1, INSTANCE_DEFAULT, msg_init_p);

   //     } else {
   //       LOG_W(MCE_APP, " %d MCE not associated with a MME, retrying registration in %d seconds ...\n",
   //             mce_nb - registered_mce,  MCE_REGISTER_RETRY_DELAY);

   //       /* Restart the MCE registration process in MCE_REGISTER_RETRY_DELAY seconds */
   //       if (timer_setup (MCE_REGISTER_RETRY_DELAY, 0, TASK_MCE_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
   //                        NULL, &mce_register_retry_timer_id) < 0) {
   //         LOG_E(ENB_APP, " Can not start MCE register retry timer, use \"sleep\" instead!\n");

   //         sleep(MCE_REGISTER_RETRY_DELAY);
   //         /* Restart the registration process */
   //         registered_mce = 0;
   //         register_mce_pending = MCE_app_register (RC.rrc[0]->node_type,mce_id_start, mce_id_end);//, enb_properties_p);
   //       }
   //     }
   //   }

      break;

    case M3AP_DEREGISTERED_MCE_IND:
      if (EPC_MODE_ENABLED) {
  	LOG_W(MCE_APP, "[MCE %ld] Received %s: associated MME %d\n", instance, ITTI_MSG_NAME (msg_p),
  	      M3AP_DEREGISTERED_MCE_IND(msg_p).nb_mme);
  	/* TODO handle recovering of registration */
      }

      break;

    case TIMER_HAS_EXPIRED:
	      LOG_I(MCE_APP, " Received %s: timer_id %ld\n", ITTI_MSG_NAME (msg_p), TIMER_HAS_EXPIRED(msg_p).timer_id);

	      if (TIMER_HAS_EXPIRED (msg_p).timer_id == mce_scheduling_info_timer_id/*mce_register_retry_timer_id*/) {
		/* Restart the registration process */
		//registered_mce = 0;
		//register_mce_pending = MCE_app_register (RC.rrc[0]->node_type, mce_id_start, mce_id_end);
		MCE_app_send_m2ap_mbms_scheduling_information(0);
	      }

	      //if (TIMER_HAS_EXPIRED (msg_p).timer_id == x2_mce_register_retry_timer_id) {
	      //  /* Restart the registration process */
	      //  x2_registered_mce = 0;
	      //  x2_register_mce_pending = MCE_app_register_x2 (mce_id_start, mce_id_end);
	      //}

      break;

 //   case X2AP_DEREGISTERED_ENB_IND:
 //     LOG_W(ENB_APP, "[MCE %d] Received %s: associated MCE %d\n", instance, ITTI_MSG_NAME (msg_p),
 //           X2AP_DEREGISTERED_ENB_IND(msg_p).nb_x2);
 //     /* TODO handle recovering of registration */
 //     break;

 //   case X2AP_REGISTER_ENB_CNF:
 //     LOG_I(ENB_APP, "[MCE %d] Received %s: associated MCE %d\n", instance, ITTI_MSG_NAME (msg_p),
 //           X2AP_REGISTER_ENB_CNF(msg_p).nb_x2);
 //     DevAssert(x2_register_enb_pending > 0);
 //     x2_register_enb_pending--;

 //     /* Check if at least MCE is registered with one target MCE */
 //     if (X2AP_REGISTER_ENB_CNF(msg_p).nb_x2 > 0) {
 //       x2_registered_enb++;
 //     }

 //     /* Check if all register MCE requests have been processed */
 //     if (x2_register_enb_pending == 0) {
 //       if (x2_registered_enb == enb_nb) {
 //         /* If all MCE are registered, start RRC HO task */
 //         } else {
 //         uint32_t x2_not_associated = enb_nb - x2_registered_enb;
 //         LOG_W(ENB_APP, " %d MCE %s not associated with the target\n",
 //               x2_not_associated, x2_not_associated > 1 ? "are" : "is");

 //         // timer to retry
 //         /* Restart the MCE registration process in ENB_REGISTER_RETRY_DELAY seconds */
 //         if (timer_setup (X2AP_ENB_REGISTER_RETRY_DELAY, 0, TASK_ENB_APP,
 //       		   INSTANCE_DEFAULT, TIMER_ONE_SHOT, NULL,
 //       		   &x2_enb_register_retry_timer_id) < 0) {
 //           LOG_E(ENB_APP, " Can not start MCE X2AP register: retry timer, use \"sleep\" instead!\n");
 //           sleep(X2AP_ENB_REGISTER_RETRY_DELAY);
 //           /* Restart the registration process */
 //           x2_registered_enb = 0;
 //           x2_register_enb_pending = MCE_app_register_x2 (enb_id_start, enb_id_end);
 //         }
 //       }
 //     }

 //     break;

 //   case M2AP_DEREGISTERED_ENB_IND:
 //     LOG_W(ENB_APP, "[MCE %d] Received %s: associated MCE %d\n", instance, ITTI_MSG_NAME (msg_p),
 //           M2AP_DEREGISTERED_ENB_IND(msg_p).nb_m2);
 //     /* TODO handle recovering of registration */
 //     break;

 //   case M2AP_REGISTER_ENB_CNF:
 //     LOG_I(ENB_APP, "[MCE %d] Received %s: associated MCE %d\n", instance, ITTI_MSG_NAME (msg_p),
 //           M2AP_REGISTER_ENB_CNF(msg_p).nb_m2);
 //     DevAssert(m2_register_enb_pending > 0);
 //     m2_register_enb_pending--;

 //     /* Check if at least MCE is registered with one target MCE */
 //     if (M2AP_REGISTER_ENB_CNF(msg_p).nb_m2 > 0) {
 //       m2_registered_enb++;
 //     }

 //     /* Check if all register MCE requests have been processed */
 //     if (m2_register_enb_pending == 0) {
 //       if (m2_registered_enb == enb_nb) {
 //         /* If all MCE are registered, start RRC HO task */
 //         } else {
 //         uint32_t m2_not_associated = enb_nb - m2_registered_enb;
 //         LOG_W(ENB_APP, " %d MCE %s not associated with the target\n",
 //               m2_not_associated, m2_not_associated > 1 ? "are" : "is");

 //         // timer to retry
 //         /* Restart the MCE registration process in ENB_REGISTER_RETRY_DELAY seconds */
 //         //if (timer_setup (X2AP_ENB_REGISTER_RETRY_DELAY, 0, TASK_ENB_APP,
 //         //      	   INSTANCE_DEFAULT, TIMER_ONE_SHOT, NULL,
 //         //      	   &x2_enb_register_retry_timer_id) < 0) {
 //         //  LOG_E(ENB_APP, " Can not start MCE X2AP register: retry timer, use \"sleep\" instead!\n");
 //         //  sleep(X2AP_ENB_REGISTER_RETRY_DELAY);
 //         //  /* Restart the registration process */
 //         //  x2_registered_enb = 0;
 //         //  x2_register_enb_pending = MCE_app_register_x2 (enb_id_start, enb_id_end);
 //         //}
 //       }
 //     }

 //     break;
   case M2AP_RESET:
      LOG_I(MCE_APP, "Received M2AP_RESET message %s\n", ITTI_MSG_NAME (msg_p));
	break;

   case M2AP_SETUP_REQ:
      LOG_I(MCE_APP, "Received M2AP_SETUP_REQ message %s\n", ITTI_MSG_NAME (msg_p));
	if(!m2ap_setup_req_local)
		m2ap_setup_req_local = (m2ap_setup_req_t*)calloc(1,sizeof(m2ap_setup_req_t));
	if(m2ap_setup_req_local)
		memcpy(m2ap_setup_req_local,&M2AP_SETUP_REQ(msg_p),sizeof(m2ap_setup_req_t));
	MCE_app_handle_m2ap_setup_req(ITTI_MSG_DESTINATION_INSTANCE(msg_p));
	//MCE_app_send_m2ap_mbms_scheduling_information(0);
	//if (timer_setup (2, 0, TASK_MCE_APP, INSTANCE_DEFAULT, TIMER_ONE_SHOT,
        //                       NULL, &mce_scheduling_info_timer_id) < 0) {
   	//}


   	/*m3_register_mce_pending =*/ //MCE_app_register_m3 (mce_id_start, mce_id_end);

	//MCE_app_send_m2ap_session_start_req(0);
	break;

   case M2AP_MBMS_SESSION_START_RESP:
      LOG_I(MCE_APP, "Received M2AP_MBMS_SESSION_START_RESP message %s\n", ITTI_MSG_NAME (msg_p));
	//MCE_app_send_m2ap_session_stop_req(0);
	MCE_app_handle_m2ap_mbms_session_start_resp(0);
	break;

   case M2AP_MBMS_SESSION_STOP_RESP:
      LOG_I(MCE_APP, "Received M2AP_MBMS_SESSION_STOP_RESP message %s\n", ITTI_MSG_NAME (msg_p));
	//MCE_app_send_m2ap_session_start_req(0);
	//MCE_app_handle_m2ap_mbms_session_start_resp(0);
	//MCE_app_send_MME_APP(0);
	break;

   case M2AP_MBMS_SCHEDULING_INFORMATION_RESP:
      LOG_I(MCE_APP, "Received M2AP_MBMS_SCHEDULING_INFORMATION_RESP message %s\n", ITTI_MSG_NAME (msg_p));
	MCE_app_send_m2ap_session_start_req(0);
	break;

   case M2AP_ENB_CONFIGURATION_UPDATE:
      LOG_I(MCE_APP, "Received M2AP_ENB_CONFIGURATION_UPDATE message %s\n", ITTI_MSG_NAME (msg_p));
	break;
	
   case M2AP_ERROR_INDICATION:
      LOG_I(MCE_APP, "Received M2AP_ERROR_INDICATION message %s\n", ITTI_MSG_NAME (msg_p));
	break;

   case M2AP_MBMS_SESSION_UPDATE_RESP:
      LOG_I(MCE_APP, "Received M2AP_MBMS_SESSION_UPDATE_RESP message %s\n", ITTI_MSG_NAME (msg_p));
	break;

   case M2AP_MBMS_SESSION_UPDATE_FAILURE:
      LOG_I(MCE_APP, "Received M2AP_MBMS_SESSION_UPDATE_FAILURE message %s\n", ITTI_MSG_NAME (msg_p));
	break;

   case M2AP_MBMS_SERVICE_COUNTING_REPORT:
      LOG_I(MCE_APP, "Received M2AP_MBMS_SESSION_UPDATE_REPORT message %s\n", ITTI_MSG_NAME (msg_p));
	break;

   case M2AP_MBMS_OVERLOAD_NOTIFICATION:
      LOG_I(MCE_APP, "Received M2AP_MBMS_OVERLOAD_NOTIFICATION message %s\n", ITTI_MSG_NAME (msg_p));
	break;

   case M2AP_MBMS_SERVICE_COUNTING_RESP:
      LOG_I(MCE_APP, "Received M2AP_MBMS_SESSION_UPDATE_RESP message %s\n", ITTI_MSG_NAME (msg_p));
	break;

   case M2AP_MBMS_SERVICE_COUNTING_FAILURE:
      LOG_I(MCE_APP, "Received M2AP_MBMS_SESSION_UPDATE_FAILURE message %s\n", ITTI_MSG_NAME (msg_p));
	break;

   case M2AP_MCE_CONFIGURATION_UPDATE_ACK:
      LOG_I(MCE_APP, "Received M2AP_MCE_CONFIGURATION_UPDATE_ACK message %s\n", ITTI_MSG_NAME (msg_p));
	break;
   case M2AP_MCE_CONFIGURATION_UPDATE_FAILURE:
      LOG_I(MCE_APP, "Received M2AP_MCE_CONFIGURATION_UPDATE_FAILURE message %s\n", ITTI_MSG_NAME (msg_p));
	break;

    default:
      LOG_E(MCE_APP, "Received unexpected message %s\n", ITTI_MSG_NAME (msg_p));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);

  return NULL;
}

//void handle_reconfiguration(module_id_t mod_id) {
//  struct timespec start, end;
//  clock_gettime(CLOCK_MONOTONIC, &start);
//  flexran_agent_info_t *flexran = RC.flexran[mod_id];
//  LOG_I(ENB_APP, "lte-softmodem soft-restart requested\n");
//
//  if (ENB_WAIT == flexran->node_ctrl_state) {
//    /* this is already waiting, just release */
//    pthread_mutex_lock(&flexran->mutex_node_ctrl);
//    flexran->node_ctrl_state = ENB_NORMAL_OPERATION;
//    pthread_mutex_unlock(&flexran->mutex_node_ctrl);
//    pthread_cond_signal(&flexran->cond_node_ctrl);
//    return;
//  }
//
//  if (stop_L1L2(mod_id) < 0) {
//    LOG_E(ENB_APP, "can not stop lte-softmodem, aborting restart\n");
//    return;
//  }
//
//  /* node_ctrl_state should have value ENB_MAKE_WAIT only if this method is not
//   * executed by the FlexRAN thread */
//  if (ENB_MAKE_WAIT == flexran->node_ctrl_state) {
//    LOG_I(ENB_APP, " * MCE %d: Waiting for FlexRAN RTController command *\n", mod_id);
//    pthread_mutex_lock(&flexran->mutex_node_ctrl);
//    flexran->node_ctrl_state = ENB_WAIT;
//
//    while (ENB_NORMAL_OPERATION != flexran->node_ctrl_state)
//      pthread_cond_wait(&flexran->cond_node_ctrl, &flexran->mutex_node_ctrl);
//
//    pthread_mutex_unlock(&flexran->mutex_node_ctrl);
//  }
//
//  if (restart_L1L2(mod_id) < 0) {
//    LOG_E(ENB_APP, "can not restart, killing lte-softmodem\n");
//    exit_fun("can not restart L1L2, killing lte-softmodem");
//    return;
//  }
//
//  clock_gettime(CLOCK_MONOTONIC, &end);
//  end.tv_sec -= start.tv_sec;
//
//  if (end.tv_nsec >= start.tv_nsec) {
//    end.tv_nsec -= start.tv_nsec;
//  } else {
//    end.tv_sec -= 1;
//    end.tv_nsec = end.tv_nsec - start.tv_nsec + 1000000000;
//  }
//
//  LOG_I(ENB_APP, "lte-softmodem restart succeeded in %ld.%ld s\n", end.tv_sec, end.tv_nsec / 1000000);
//}


