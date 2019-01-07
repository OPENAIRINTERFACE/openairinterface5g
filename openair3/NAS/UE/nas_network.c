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

/*****************************************************************************
Source      nas_network.h

Version     0.1

Date        2012/09/20

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Maurel

Description NAS procedure functions triggered by the network

*****************************************************************************/

#include "nas_network.h"
#include "commonDef.h"
#include "nas_log.h"
#include "nas_timer.h"

#include "as_message.h"
#include "nas_proc.h"
#include "user_defs.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    nas_network_initialize()                                  **
 **                                                                        **
 ** Description: Initializes network internal data                         **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void nas_network_initialize(void)
{
  LOG_FUNC_IN;

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_network_cleanup()                                     **
 **                                                                        **
 ** Description: Performs clean up procedure before the system is shutdown **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **          Return:    None                                       **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void nas_network_cleanup(nas_user_t *user)
{
  LOG_FUNC_IN;

  nas_proc_cleanup(user);

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_network_process_data()                                **
 **                                                                        **
 ** Description: Process Access Stratum messages received from the network **
 **      and call applicable NAS procedure function.               **
 **                                                                        **
 ** Inputs:  msg_id:    AS message identifier                      **
 **          data:      Generic pointer to data structure that has **
 **             to be processed                            **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok if the message has been success-  **
 **             fully processed; RETURNerror otherwise     **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_network_process_data(nas_user_t *user, int msg_id, const void *data)
{
  LOG_FUNC_IN;

  const as_message_t *msg = (as_message_t *)(data);
  int rc = RETURNok;

  /* Sanity check */
  if (msg_id != msg->msgID) {
    LOG_TRACE(ERROR, "NET-MAIN  - Message identifier 0x%x to process "
              "is different from that of the network data (0x%x)",
              msg_id, msg->msgID);
    LOG_FUNC_RETURN (RETURNerror);
  }

  switch (msg_id) {

  case AS_BROADCAST_INFO_IND:
    break;

  case AS_CELL_INFO_CNF: {
    /* Received cell information confirm */
    const cell_info_cnf_t *info = &msg->msg.cell_info_cnf;
    int cell_found = (info->errCode == AS_SUCCESS);
    rc = nas_proc_cell_info(user, cell_found, info->tac,
                            info->cellID, info->rat,
                            info->rsrp, info->rsrq);
    break;
  }

  case AS_CELL_INFO_IND:
    break;

  case AS_PAGING_IND:
    break;

  case AS_NAS_ESTABLISH_CNF: {
    /* Received NAS signalling connection establishment confirm */
    const nas_establish_cnf_t *confirm = &msg->msg.nas_establish_cnf;

    if ( (confirm->errCode == AS_SUCCESS) ||
         (confirm->errCode == AS_TERMINATED_NAS) ) {
      rc = nas_proc_establish_cnf(user, confirm->nasMsg.data,
                                  confirm->nasMsg.length);
    } else {
      LOG_TRACE(WARNING, "NET-MAIN  - "
                "Initial NAS message not delivered");
      rc = nas_proc_establish_rej(user);
    }

    break;
  }

  case AS_NAS_RELEASE_IND:
    /* Received NAS signalling connection releaase indication */
    rc = nas_proc_release_ind(user, msg->msg.nas_release_ind.cause);
    break;

  case AS_UL_INFO_TRANSFER_CNF:

    /* Received uplink data transfer confirm */
    if (msg->msg.ul_info_transfer_cnf.errCode != AS_SUCCESS) {
      LOG_TRACE(WARNING, "NET-MAIN  - "
                "Uplink NAS message not delivered");
      rc = nas_proc_ul_transfer_rej(user);
    } else {
      rc = nas_proc_ul_transfer_cnf(user);
    }

    break;

  case AS_DL_INFO_TRANSFER_IND: {
    const dl_info_transfer_ind_t *info = &msg->msg.dl_info_transfer_ind;
    /* Received downlink data transfer indication */
    rc = nas_proc_dl_transfer_ind(user, info->nasMsg.data,
                                  info->nasMsg.length);
    break;
  }

  case AS_RAB_ESTABLISH_IND:
    break;

  case AS_RAB_RELEASE_IND:
    break;

  default:
    LOG_TRACE(ERROR, "NET-MAIN  - Unexpected AS message type: 0x%x",
              msg_id);
    rc = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

