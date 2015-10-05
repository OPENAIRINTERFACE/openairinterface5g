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
#if defined(NAS_BUILT_IN_EPC)
void nas_network_initialize(mme_config_t *mme_config_p)
#else
void nas_network_initialize(void)
#endif
{
  LOG_FUNC_IN;

  /* Initialize the internal NAS processing data */
# if defined(NAS_BUILT_IN_EPC)
  nas_timer_init();
  nas_proc_initialize(mme_config_p);
# else
  nas_proc_initialize();
# endif

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
void nas_network_cleanup(void)
{
  LOG_FUNC_IN;

  nas_proc_cleanup();

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
int nas_network_process_data(int msg_id, const void *data)
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


  case AS_NAS_ESTABLISH_IND: {
    /* Received NAS signalling connection establishment indication */
    const nas_establish_ind_t *indication = &msg->msg.nas_establish_ind;
    rc = nas_proc_establish_ind(indication->UEid,
                                indication->tac,
                                indication->initialNasMsg.data,
                                indication->initialNasMsg.length);
    break;
  }

  case AS_DL_INFO_TRANSFER_CNF: {
    const dl_info_transfer_cnf_t *info = &msg->msg.dl_info_transfer_cnf;

    /* Received downlink data transfer confirm */
    if (info->errCode != AS_SUCCESS) {
      LOG_TRACE(WARNING, "NET-MAIN  - "
                "Downlink NAS message not delivered");
      rc = nas_proc_dl_transfer_rej(info->UEid);
    } else {
      rc = nas_proc_dl_transfer_cnf(info->UEid);
    }

    break;
  }

  case AS_UL_INFO_TRANSFER_IND: {
    const ul_info_transfer_ind_t *info = &msg->msg.ul_info_transfer_ind;
    /* Received uplink data transfer indication */
    rc = nas_proc_ul_transfer_ind(info->UEid,
                                  info->nasMsg.data,
                                  info->nasMsg.length);
    break;
  }

  case AS_RAB_ESTABLISH_CNF:
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

