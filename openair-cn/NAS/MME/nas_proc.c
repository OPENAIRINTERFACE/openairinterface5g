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
Source      nas_proc.c

Version     0.1

Date        2012/09/20

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Maurel

Description NAS procedure call manager

*****************************************************************************/

#include "nas_proc.h"
#include "nas_log.h"

#include "emm_main.h"
#include "emm_sap.h"

#include "esm_main.h"
#include "esm_sap.h"
#include "msc.h"

#include <stdio.h>  // sprintf

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
 ** Name:    nas_proc_initialize()                                     **
 **                                                                        **
 ** Description:                                                           **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
# if defined(NAS_BUILT_IN_EPC)
void nas_proc_initialize(mme_config_t *mme_config_p)
{
  LOG_FUNC_IN;

  /* Initialize the EMM procedure manager */
  emm_main_initialize(mme_config_p);

  /* Initialize the ESM procedure manager */
  esm_main_initialize();

  LOG_FUNC_OUT;
}
# else
void nas_proc_initialize(void)
{
  LOG_FUNC_IN;

  /* Initialize the EMM procedure manager */
  emm_main_initialize();

  /* Initialize the ESM procedure manager */
  esm_main_initialize();

  LOG_FUNC_OUT;
}
# endif

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_cleanup()                                        **
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
void nas_proc_cleanup(void)
{
  LOG_FUNC_IN;

  /* Perform the EPS Mobility Manager's clean up procedure */
  emm_main_cleanup();

  /* Perform the EPS Session Manager's clean up procedure */
  esm_main_cleanup();

  LOG_FUNC_OUT;
}

/*
 * --------------------------------------------------------------------------
 *          NAS procedures triggered by the user
 * --------------------------------------------------------------------------
 */


/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_establish_ind()                                  **
 **                                                                        **
 ** Description: Processes the NAS signalling connection establishment     **
 **      indication message received from the network              **
 **                                                                        **
 ** Inputs:  ueid:      UE identifier                              **
 **      tac:       The code of the tracking area the initia-  **
 **             ting UE belongs to                         **
 **      data:      The initial NAS message transfered within  **
 **             the message                                **
 **      len:       The length of the initial NAS message      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_establish_ind(uint32_t ueid, tac_t tac,
                           const Byte_t *data, uint32_t len)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  if (len > 0) {
    emm_sap_t emm_sap;

    MSC_LOG_TX_MESSAGE(
    		MSC_NAS_MME,
    		MSC_NAS_EMM_MME,
    		NULL,0,
    		"0 EMMAS_ESTABLISH_REQ ue id "NAS_UE_ID_FMT" tac %u", ueid, tac);
    /*
     * Notify the EMM procedure call manager that NAS signalling
     * connection establishment indication message has been received
     * from the Access-Stratum sublayer
     */
    emm_sap.primitive = EMMAS_ESTABLISH_REQ;
    emm_sap.u.emm_as.u.establish.ueid = ueid;
    emm_sap.u.emm_as.u.establish.NASmsg.length = len;
    emm_sap.u.emm_as.u.establish.NASmsg.value = (uint8_t *)data;
    rc = emm_sap_send(&emm_sap);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_dl_transfer_cnf()                                **
 **                                                                        **
 ** Description: Processes the downlink data transfer confirm message re-  **
 **      ceived from the network while NAS message has been succes-**
 **      sfully delivered to the NAS sublayer on the receiver side.**
 **                                                                        **
 ** Inputs:  ueid:      UE identifier                              **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_dl_transfer_cnf(uint32_t ueid)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  MSC_LOG_TX_MESSAGE(
  		MSC_NAS_MME,
  		MSC_NAS_EMM_MME,
  		NULL,0,
  		"0 EMMAS_DATA_IND dl_transfer_conf ue id "NAS_UE_ID_FMT" ", ueid);

  /*
   * Notify the EMM procedure call manager that downlink NAS message
   * has been successfully delivered to the NAS sublayer on the
   * receiver side
   */
  emm_sap.primitive = EMMAS_DATA_IND;
  emm_sap.u.emm_as.u.data.ueid = ueid;
  emm_sap.u.emm_as.u.data.delivered = TRUE;
  emm_sap.u.emm_as.u.data.NASmsg.length = 0;
  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_dl_transfer_rej()                                **
 **                                                                        **
 ** Description: Processes the downlink data transfer confirm message re-  **
 **      ceived from the network while NAS message has not been    **
 **      delivered to the NAS sublayer on the receiver side.       **
 **                                                                        **
 ** Inputs:  ueid:      UE identifier                              **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_dl_transfer_rej(uint32_t ueid)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  MSC_LOG_TX_MESSAGE(
  		MSC_NAS_MME,
  		MSC_NAS_EMM_MME,
  		NULL,0,
  		"0 EMMAS_DATA_IND dl_transfer_reject ue id "NAS_UE_ID_FMT" ", ueid);

  /*
   * Notify the EMM procedure call manager that transmission
   * failure of downlink NAS message indication has been received
   * from lower layers
   */
  emm_sap.primitive = EMMAS_DATA_IND;
  emm_sap.u.emm_as.u.data.ueid = ueid;
  emm_sap.u.emm_as.u.data.delivered = FALSE;
  emm_sap.u.emm_as.u.data.NASmsg.length = 0;
  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_ul_transfer_ind()                                **
 **                                                                        **
 ** Description: Processes uplink data transfer indication message recei-  **
 **      ved from the network                                      **
 **                                                                        **
 ** Inputs:  ueid:      UE identifier                              **
 **      data:      The transfered NAS message                 **
 **      len:       The length of the NAS message              **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_ul_transfer_ind(uint32_t ueid, const Byte_t *data, uint32_t len)
{
  int rc = RETURNerror;

  LOG_FUNC_IN;

  if (len > 0) {
    emm_sap_t emm_sap;
    /*
     * Notify the EMM procedure call manager that data transfer
     * indication has been received from the Access-Stratum sublayer
     */
    MSC_LOG_TX_MESSAGE(
    		MSC_NAS_MME,
    		MSC_NAS_EMM_MME,
    		NULL,0,
    		"0 EMMAS_DATA_IND ue id "NAS_UE_ID_FMT" len %u", ueid, len);

    emm_sap.primitive = EMMAS_DATA_IND;
    emm_sap.u.emm_as.u.data.ueid = ueid;
    emm_sap.u.emm_as.u.data.delivered = TRUE;
    emm_sap.u.emm_as.u.data.NASmsg.length = len;
    emm_sap.u.emm_as.u.data.NASmsg.value = (uint8_t *)data;
    rc = emm_sap_send(&emm_sap);
  }

  LOG_FUNC_RETURN (rc);
}

# if defined(NAS_BUILT_IN_EPC)
int nas_proc_auth_param_res(emm_cn_auth_res_t *emm_cn_auth_res)
{
  int rc = RETURNerror;
  emm_sap_t emm_sap;

  LOG_FUNC_IN;

  MSC_LOG_TX_MESSAGE(
  		MSC_NAS_MME,
  		MSC_NAS_EMM_MME,
  		NULL,0,
  		"0 EMMCN_AUTHENTICATION_PARAM_RES");

  emm_sap.primitive = EMMCN_AUTHENTICATION_PARAM_RES;
  emm_sap.u.emm_cn.u.auth_res = emm_cn_auth_res;

  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

int nas_proc_auth_param_fail(emm_cn_auth_fail_t *emm_cn_auth_fail)
{
  int rc = RETURNerror;
  emm_sap_t emm_sap;

  LOG_FUNC_IN;

  MSC_LOG_TX_MESSAGE(
  		MSC_NAS_MME,
  		MSC_NAS_EMM_MME,
  		NULL,0,
  		"0 EMMCN_AUTHENTICATION_PARAM_FAIL");

  emm_sap.primitive = EMMCN_AUTHENTICATION_PARAM_FAIL;
  emm_sap.u.emm_cn.u.auth_fail = emm_cn_auth_fail;

  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

int nas_proc_deregister_ue(uint32_t ue_id)
{
  int rc = RETURNerror;
  emm_sap_t emm_sap;

  LOG_FUNC_IN;

  MSC_LOG_TX_MESSAGE(
  		MSC_NAS_MME,
  		MSC_NAS_EMM_MME,
  		NULL,0,
  		"0 EMMCN_DEREGISTER_UE ue_id "NAS_UE_ID_FMT" ",ue_id);

  emm_sap.primitive = EMMCN_DEREGISTER_UE;
  emm_sap.u.emm_cn.u.deregister.UEid = ue_id;

  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

int nas_proc_pdn_connectivity_res(emm_cn_pdn_res_t *emm_cn_pdn_res)
{
  int rc = RETURNerror;
  emm_sap_t emm_sap;

  LOG_FUNC_IN;

  MSC_LOG_TX_MESSAGE(
  		MSC_NAS_MME,
  		MSC_NAS_EMM_MME,
  		NULL,0,
  		"0 EMMCN_PDN_CONNECTIVITY_RES");

  emm_sap.primitive = EMMCN_PDN_CONNECTIVITY_RES;
  emm_sap.u.emm_cn.u.emm_cn_pdn_res = emm_cn_pdn_res;

  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}

int nas_proc_pdn_connectivity_fail(emm_cn_pdn_fail_t *emm_cn_pdn_fail)
{
  int rc = RETURNerror;
  emm_sap_t emm_sap;

  LOG_FUNC_IN;

  MSC_LOG_TX_MESSAGE(
  		MSC_NAS_MME,
  		MSC_NAS_EMM_MME,
  		NULL,0,
  		"0 EMMCN_PDN_CONNECTIVITY_FAIL");

  emm_sap.primitive = EMMCN_PDN_CONNECTIVITY_FAIL;
  emm_sap.u.emm_cn.u.emm_cn_pdn_fail = emm_cn_pdn_fail;

  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN (rc);
}



# endif

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

