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
#include "nas_user.h"
#include "utils.h"

#include "emm_main.h"
#include "emm_sap.h"

#include "esm_main.h"
#include "esm_sap.h"

#include <stdio.h>  // sprintf

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * Signal strength/quality value not known or not detectable
 */
#define NAS_PROC_RSRQ_UNKNOWN   255
#define NAS_PROC_RSRP_UNKNOWN   255


static int _nas_proc_activate(nas_user_t *user, int cid, bool apply_to_all);
static int _nas_proc_deactivate(nas_user_t *user, int cid, bool apply_to_all);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/
/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_initialize()                                     **
 **                                                                        **
 ** Description:                                                           **
 **                                                                        **
 ** Inputs:  emm_cb:    Mobility Management indication callback    **
 **      esm_cb:    Session Management indication callback     **
 **      imei:      The IMEI read from the UE's non-volatile   **
 **             memory                                     **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _nas_proc_data                             **
 **                                                                        **
 ***************************************************************************/
void nas_proc_initialize(nas_user_t *user, emm_indication_callback_t emm_cb,
                         esm_indication_callback_t esm_cb, const char *imei)
{
  LOG_FUNC_IN;

  /* Initialize local NAS data */
  user->proc.EPS_capability_status = false;
  user->proc.rsrq = NAS_PROC_RSRQ_UNKNOWN;
  user->proc.rsrp = NAS_PROC_RSRP_UNKNOWN;

  user->authentication_data = calloc_or_fail(1, sizeof(authentication_data_t));
  user->security_data = calloc_or_fail(1, sizeof(security_data_t));

  /* Initialize the EMM procedure manager */
  emm_main_initialize(user, emm_cb, imei);

  /* Initialize the ESM procedure manager */
  esm_main_initialize(user, esm_cb);

  LOG_FUNC_OUT;
}


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
void nas_proc_cleanup(nas_user_t *user)
{
  LOG_FUNC_IN;

  /* Detach the UE from the EPS network */
  int rc = nas_proc_detach(user, true);

  if (rc != RETURNok) {
    LOG_TRACE(ERROR, "NAS-PROC  - Failed to detach from the network");
  }


  /* Perform the EPS Mobility Manager's clean up procedure */
  emm_main_cleanup(user);

  /* Perform the EPS Session Manager's clean up procedure */
  esm_main_cleanup(user->esm_data);

  LOG_FUNC_OUT;
}

/*
 * --------------------------------------------------------------------------
 *          NAS procedures triggered by the user
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_enable_s1_mode()                                 **
 **                                                                        **
 ** Description: Notify the EPS Mobility Manager that the UE can be        **
 **      operated                                                  **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_enable_s1_mode(nas_user_t *user)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /*
   * Notify the EMM procedure call manager that EPS capability
   * of the UE is enabled
   */
  user->proc.EPS_capability_status = true;
  emm_sap.primitive = EMMREG_S1_ENABLED;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_disable_s1_mode()                                **
 **                                                                        **
 ** Description: Notify the EPS Mobility Manager that the S1 mode is no    **
 **      longer activated                                          **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_disable_s1_mode(nas_user_t *user)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /*
   * Notify the EMM procedure call manager that EPS capability
   * of the UE is disabled
   */
  user->proc.EPS_capability_status = false;
  emm_sap.primitive = EMMREG_S1_DISABLED;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_eps()                                        **
 **                                                                        **
 ** Description: Get the current value of the EPS capability status        **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _nas_proc_data                             **
 **                                                                        **
 ** Outputs:     stat:      The current value of the EPS capability    **
 **             status                                     **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_eps(nas_user_t *user, bool *stat)
{
  LOG_FUNC_IN;

  *stat = user->proc.EPS_capability_status;

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_imsi()                                       **
 **                                                                        **
 ** Description: Get the International Mobile Subscriber Identity number   **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     imsi:      The value of the IMSI                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_imsi(emm_data_t *emm_data, char *imsi_str)
{
  LOG_FUNC_IN;

  const imsi_t *imsi = emm_main_get_imsi(emm_data);

  if (imsi != NULL) {
    int offset = 0;
    offset += sprintf(imsi_str + offset, "%u%u%u%u%u",
                      imsi->u.num.digit1, imsi->u.num.digit2,
                      imsi->u.num.digit3, imsi->u.num.digit4,
                      imsi->u.num.digit5);

    if (imsi->u.num.digit6 != 0xf) {
      offset += sprintf(imsi_str + offset, "%u", imsi->u.num.digit6);
    }

    offset += sprintf(imsi_str + offset, "%u%u%u%u%u%u%u%u",
                      imsi->u.num.digit7, imsi->u.num.digit8,
                      imsi->u.num.digit9, imsi->u.num.digit10,
                      imsi->u.num.digit11, imsi->u.num.digit12,
                      imsi->u.num.digit13, imsi->u.num.digit14);

    if (imsi->u.num.digit15 != 0xf) {
      offset += sprintf(imsi_str + offset, "%u", imsi->u.num.digit15);
    }

    LOG_FUNC_RETURN (RETURNok);
  }

  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_msisdn()                                     **
 **                                                                        **
 ** Description: Get the Mobile Subscriber dialing number                  **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msisdn:    The value of the subscriber dialing number **
 **      ton_npi:   Type Of Number / Numbering Plan Indicator  **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_msisdn(nas_user_t *user, char *msisdn_str, int *ton_npi)
{
  LOG_FUNC_IN;

  const msisdn_t *msisdn = emm_main_get_msisdn(user);

  if (msisdn != NULL) {
    union {
      struct {
        Byte_t ext:1;
        Byte_t ton:3;
        Byte_t npi:4;
      } ext_ton_npi;
      Byte_t type;
    } converter;
    converter.ext_ton_npi.ext = msisdn->ext;
    converter.ext_ton_npi.ton = msisdn->ton;
    converter.ext_ton_npi.npi = msisdn->npi;
    *ton_npi = converter.type;

    sprintf(msisdn_str, "%u%u%u%u%u%u%u%u%u%u%u",
            msisdn->digit[0].msb, msisdn->digit[0].lsb,
            msisdn->digit[1].msb, msisdn->digit[1].lsb,
            msisdn->digit[2].msb, msisdn->digit[2].lsb,
            msisdn->digit[3].msb, msisdn->digit[3].lsb,
            msisdn->digit[4].msb, msisdn->digit[4].lsb,
            msisdn->digit[5].msb);

    LOG_FUNC_RETURN (RETURNok);
  }

  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_signal_quality()                             **
 **                                                                        **
 ** Description: Get the signal strength/quality parameters                **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _nas_proc_data                             **
 **                                                                        **
 ** Outputs:     rsrq:      Reference signal received quality value    **
 **      rsrp:      Reference signal received power value      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_signal_quality(nas_user_t *user, int *rsrq, int *rsrp)
{
  LOG_FUNC_IN;

  *rsrq = user->proc.rsrq;
  *rsrp = user->proc.rsrp;

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_register()                                       **
 **                                                                        **
 ** Description: Execute the network selection and registration procedure. **
 **                                                                        **
 ** Inputs:  mode:      Network selection mode of operation        **
 **      format:    Represention format of the operator iden-  **
 **             tifier                                     **
 **      oper:      Identifier of the network operator to re-  **
 **             gister                                     **
 **      AcT:       The selected Access Technology             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_register(nas_user_t *user, int mode, int format, const network_plmn_t *oper, int AcT)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  /*
   * Set the PLMN selection mode of operation
   */
  int index = emm_main_set_plmn_selection_mode(user, mode, format, oper, AcT);

  if ( !(index < 0) ) {
    /*
     * Notify the EMM procedure call manager that network (re)selection
     * procedure has to be executed
     */
    emm_sap_t emm_sap;
    emm_sap.primitive = EMMREG_REGISTER_REQ;
    emm_sap.u.emm_reg.u.regist.index = index;
    rc = emm_sap_send(user, &emm_sap);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_deregister()                                     **
 **                                                                        **
 ** Description: Execute the network deregistration procedure.             **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_deregister(nas_user_t *user)
{
  LOG_FUNC_IN;

  /* TODO: Force an attempt to deregister from the network */
  LOG_TRACE(ERROR, "NAS-PROC  - Network deregistration procedure is "
            "not implemented");

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_reg_data()                                   **
 **                                                                        **
 ** Description: Gets network registration data from EMM                   **
 **                                                                        **
 ** Inputs:  format:    Format of the representation of the net-   **
 **             work operator identifier                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     mode:      The current network selection mode of ope- **
 **             ration                                     **
 **      oper:      The identifier of the selected network     **
 **             operator                                   **
 **      AcT:       The access technology currently used       **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_reg_data(nas_user_t *user, int *mode, bool *selected, int format,
                          network_plmn_t *oper, int *AcT)
{
  LOG_FUNC_IN;

  /* Get the PLMN selection mode of operation */
  *mode = emm_main_get_plmn_selection_mode(user->emm_data);

  /* Get the currently selected operator */
  const char *oper_name = emm_main_get_selected_plmn(user->emm_plmn_list, user->emm_data, oper, format);

  if (oper_name != NULL) {
    /* An operator is currently selected */
    *selected = true;
    /* Get the supported Radio Access Technology */
    *AcT = emm_main_get_plmn_rat(user->emm_data);
  } else {
    /* No any operator is selected */
    *selected = false;
    *AcT = NET_ACCESS_UNAVAILABLE;
  }

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_oper_list()                                  **
 **                                                                        **
 ** Description: Gets the list of operators present in the network         **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     oper_list: The list of operators                      **
 **      Return:    The size of the list in bytes              **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_oper_list(nas_user_t *user, const char **oper_list)
{
  LOG_FUNC_IN;

  int size = emm_main_get_plmn_list(user->emm_plmn_list, user->emm_data, oper_list);

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_reg_status()                                 **
 **                                                                        **
 ** Description: Get the value of the network registration status which    **
 **      shows whether the network has currently indicated the     **
 **      registration of the UE                                    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     stat:      The current network registration status    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_reg_status(nas_user_t *user, int *stat)
{
  LOG_FUNC_IN;

  *stat = emm_main_get_plmn_status(user->emm_data);

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_loc_info()                                   **
 **                                                                        **
 ** Description: Get the location information when the UE is registered in **
 **      the Network                                               **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     tac:       The code of the tracking area the registe- **
 **             red PLMN belongs to                        **
 **      ci:        The identifier of the serving cell         **
 **      AcT:       The access technology in used              **
 **      rac:       The GPRS routing area code, if available   **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_loc_info(nas_user_t *user, char *tac, char *ci, int *AcT)
{
  LOG_FUNC_IN;

  sprintf(tac, "%.4x", emm_main_get_plmn_tac(user->emm_data));  // two byte
  sprintf(ci, "%.8x", emm_main_get_plmn_ci(user->emm_data));    // four byte
  *AcT = emm_main_get_plmn_rat(user->emm_data);             // E-UTRAN

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_detach()                                         **
 **                                                                        **
 ** Description: Initiates a detach procedure                              **
 **                                                                        **
 ** Inputs:  switch_off:    true if the detach is due to UE switch-off **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_detach(nas_user_t *user, bool switch_off)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc = RETURNok;

  if ( emm_main_is_attached(user->emm_data) ) {
    /* Initiate an Detach procedure */
    emm_sap.primitive = EMMREG_DETACH_INIT;
    emm_sap.u.emm_reg.u.detach.switch_off = switch_off;
    rc = emm_sap_send(user, &emm_sap);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_attach()                                         **
 **                                                                        **
 ** Description: Initiates an attach procedure                             **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_attach(nas_user_t *user)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc = RETURNok;

  if ( !emm_main_is_attached(user->emm_data) ) {
    /* Initiate an Attach procedure */
    emm_sap.primitive = EMMREG_ATTACH_INIT;
    emm_sap.u.emm_reg.u.attach.is_emergency = false;
    rc = emm_sap_send(user, &emm_sap);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_attach_status()                              **
 **                                                                        **
 ** Description: Gets the current network attachment status                **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    true if the UE is currently attached to    **
 **             the network                                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
bool nas_proc_get_attach_status(nas_user_t *user)
{
  LOG_FUNC_IN;

  bool is_attached = emm_main_is_attached(user->emm_data);

  LOG_FUNC_RETURN (is_attached);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_pdn_range()                                      **
 **                                                                        **
 ** Description: Gets the maximum value of a PDN context identifier        **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The PDN context identifier maximum value   **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_pdn_range(esm_data_t *esm_data)
{
  LOG_FUNC_IN;

  int max_pdn_id = esm_main_get_nb_pdns_max(esm_data);

  LOG_FUNC_RETURN (max_pdn_id);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_pdn_status()                                 **
 **                                                                        **
 ** Description: Gets the activation state of every defined PDN contexts   **
 **                                                                        **
 ** Inputs:  n_pdn_max: Maximum number of PDN contexts             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     cids:      List of PDN context identifiers            **
 **      states:    List of PDN context activation states      **
 **      Return:    The number of PDN contexts that are cur-   **
 **             rently in a defined state                  **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_pdn_status(nas_user_t *user, int *cids, int *states, int n_pdn_max)
{
  LOG_FUNC_IN;

  int cid;
  int n_defined_pdn = 0;

  /* Get the maximum number of supported PDN contexts */
  int n_pdn = esm_main_get_nb_pdns_max(user->esm_data);

  /* For all PDN contexts */
  for (cid = 1; (cid < n_pdn+1) && (n_defined_pdn < n_pdn_max); cid++) {
    /* Get the status of this PDN */
    bool state = false;
    bool is_defined = esm_main_get_pdn_status(user, cid, &state);

    if (is_defined != false) {
      /* This PDN has been defined */
      *(cids++) = cid;
      *(states++) = state;
      n_defined_pdn += 1;
    }
  }

  LOG_FUNC_RETURN (n_defined_pdn);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_pdn_param()                                  **
 **                                                                        **
 ** Description: Gets the parameters of every defined PDN contexts         **
 **                                                                        **
 ** Inputs:  n_pdn_max: Maximum number of PDN contexts             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     cids:      List of PDN context identifiers            **
 **      types:     List of PDN types (IPv4, IPv6, IPv4v6)     **
 **      apns:      List of Access Point Names                 **
 **      Return:    The number of PDN contexts that are cur-   **
 **             rently in a defined state                  **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_pdn_param(esm_data_t *esm_data, int *cids, int *types, const char **apns,
                           int n_pdn_max)
{
  LOG_FUNC_IN;

  int cid;
  int n_defined_pdn = 0;

  /* Get the maximum number of supported PDN contexts */
  int n_pdn = esm_main_get_nb_pdns_max(esm_data);

  /* For all PDN contexts */
  for (cid = 1; (cid < n_pdn+1) && (n_defined_pdn < n_pdn_max); cid++) {
    bool emergency, active;
    /* Get PDN connection parameters */
    int rc = esm_main_get_pdn(esm_data, cid, types, apns, &emergency, &active);

    if (rc != RETURNerror) {
      /* This PDN has been defined */
      *(cids++) = cid;
      types++;
      apns++;
      n_defined_pdn += 1;
    }
  }

  LOG_FUNC_RETURN (n_defined_pdn);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_get_pdn_addr()                                   **
 **                                                                        **
 ** Description: When the cid parameter value is positive, gets the addres-**
 **      s(es) assigned to the specified PDN context.              **
 **      When the cid parameter value is negative, gets the addres-**
 **      s(es) assigned to each defined PDN context.               **
 **      When the cid parameter value is null, gets the list of    **
 **      defined PDN contexts.                                     **
 **                                                                        **
 ** Inputs:  cid:       PDN context identifier                     **
 **      n_pdn_max: Maximum number of PDN contexts             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     cids:      List of PDN context identifiers            **
 **      addr1:     List of IPv4 addresses                     **
 **      addr2:     List of IPv6 addresses                     **
 **      Return:    The number PDN contexts that have at least **
 **             one IP address assigned                    **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_get_pdn_addr(nas_user_t *user, int cid, int *cids, const char **addr1,
                          const char **addr2, int n_pdn_max)
{
  LOG_FUNC_IN;

  int rc;
  int n_defined_pdn = 0;

  if (cid > 0) {
    /* Get addresses assigned to the specified PDN */
    rc = esm_main_get_pdn_addr(user->esm_data, cid, addr1, addr2);

    if (rc != RETURNerror) {
      *cids = cid;
      n_defined_pdn = 1;
    }
  } else if (cid < 0) {
    /* Get the maximum number of supported PDN contexts */
    int n_pdn = esm_main_get_nb_pdns_max(user->esm_data);

    /* For all PDN contexts */
    for (cid = 1; (cid < n_pdn+1) && (n_defined_pdn < n_pdn_max); cid++) {
      /* Get PDN connection addresses */
      rc = esm_main_get_pdn_addr(user->esm_data, cid, addr1, addr2);

      if (rc != RETURNerror) {
        /* This PDN has been defined */
        *(cids++) = cid;
        addr1++;
        addr2++;
        n_defined_pdn += 1;
      }
    }
  } else {
    /* Get the list of defined PDN contexts */

  }

  LOG_FUNC_RETURN (n_defined_pdn);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_set_pdn()                                        **
 **                                                                        **
 ** Description: Setup parameters of a specified PDN context               **
 **                                                                        **
 ** Inputs:  cid:       Identifier of the PDN context to setup     **
 **      type:      Type of PDN (IPv4, IPv6,IPv4v6)            **
 **      apn:       Access Point Name of the external network  **
 **             to connect to                              **
 **      ipv4_addr: IPv4 address allocation (NAS, DHCP)        **
 **      emergency: Emergency bearer support indication        **
 **      p_cscf:    Preference of P-CSCF address discovery     **
 **      im_cn_signal:  IM CN subsystem-related signalling indica- **
 **             tion parameter                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_set_pdn(nas_user_t *user, int cid, int type, const char *apn, int ipv4_addr,
                     int emergency, int p_cscf, int im_cn_signal)
{
  LOG_FUNC_IN;

  int rc;

  esm_sap_t esm_sap;
  esm_sap.primitive = ESM_PDN_CONNECTIVITY_REQ;
  esm_sap.is_standalone = true;
  esm_sap.data.pdn_connect.is_defined = false;
  esm_sap.data.pdn_connect.cid = cid;
  esm_sap.data.pdn_connect.pdn_type = type;
  esm_sap.data.pdn_connect.apn = apn;
  esm_sap.data.pdn_connect.is_emergency = emergency;
  /*
   * Notify ESM that a new PDN context has to be defined for
   * the specified APN
   */
  rc = esm_sap_send(user, &esm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_reset_pdn()                                      **
 **                                                                        **
 ** Description: Reset parameters of a specified PDN context               **
 **                                                                        **
 ** Inputs:  cid:       Identifier of the PDN context to setup     **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_reset_pdn(nas_user_t *user, int cid)
{
  LOG_FUNC_IN;

  int rc;

  esm_sap_t esm_sap;
  esm_sap.primitive = ESM_PDN_CONNECTIVITY_REJ;
  esm_sap.is_standalone = true;
  esm_sap.data.pdn_connect.is_defined = true;
  esm_sap.data.pdn_connect.cid = cid;
  /*
   * Notify ESM that the specified PDN context has to be undefined
   */
  rc = esm_sap_send(user, &esm_sap);
  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_deactivate_pdn()                                 **
 **                                                                        **
 ** Description: Deactivates specified PDN context or all PDN contexts if  **
 **      specified cid is negative                                 **
 **                                                                        **
 ** Inputs:  cid:       Identifier of the PDN context to be deac-  **
 **             tivate                                     **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_deactivate_pdn(nas_user_t *user, int cid)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  if (cid > 0) {
    /* Deactivate only the specified PDN context */
    rc = _nas_proc_deactivate(user, cid, false);
  } else {
    /* Do not deactivate the PDN connection established during initial
     * network attachment (identifier 1) */
    cid = 2;

    /* Deactivate all active PDN contexts */
    while ((rc != RETURNerror) && (cid < esm_main_get_nb_pdns_max(user->esm_data)+1)) {
      rc = _nas_proc_deactivate(user, cid++, true);
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_activate_pdn()                                   **
 **                                                                        **
 ** Description: Activates specified PDN context or all PDN contexts if    **
 **      specified cid is negative                                 **
 **                                                                        **
 ** Inputs:  cid:       Identifier of the PDN context to be act-   **
 **             tivate                                     **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_activate_pdn(nas_user_t *user, int cid)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  if ( !emm_main_is_attached(user->emm_data) ) {
    /*
     * If the UE is not attached to the network, perform EPS attach
     * procedure prior to attempt to request any PDN connectivity
     */
    LOG_TRACE(WARNING, "NAS-PROC  - UE is not attached to the network");
    rc = nas_proc_attach(user);
  } else if (emm_main_is_emergency(user->emm_data)) {
    /* The UE is attached for emergency bearer services; It shall not
     * request a PDN connection to any other PDN */
    LOG_TRACE(WARNING,"NAS-PROC  - Attached for emergency bearer services");
    rc = RETURNerror;
  }

  if (rc != RETURNerror) {
    if (cid > 0) {
      /* Activate only the specified PDN context */
      rc = _nas_proc_activate(user, cid, false);
    } else {
      cid = 1;

      /* Activate all defined PDN contexts */
      while ((rc != RETURNerror) && (cid < esm_main_get_nb_pdns_max(user->esm_data)+1)) {
        rc = _nas_proc_activate(user, cid++, true);
      }
    }
  }

  LOG_FUNC_RETURN (rc);
}

/*
 * --------------------------------------------------------------------------
 *      NAS procedures triggered by the network
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_cell_info()                                      **
 **                                                                        **
 ** Description: Processes the cell information received from the network  **
 **                                                                        **
 ** Inputs:  found:     Indicates whether a suitable cell is found **
 **             for the selected PLMN to camp on           **
 **      tac:       The code of the tracking area the PLMN     **
 **             belongs to                                 **
 **      ci:        The identifier of a cell serving this PLMN **
 **      AcT:       The access technology supported by the     **
 **             serving cell                               **
 **      rsrq:      Reference signal received quality measure- **
 **             ment                                       **
 **      rsrp:      Reference signal received power measure-   **
 **             ment                                       **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _nas_proc_data                             **
 **                                                                        **
 ***************************************************************************/
int nas_proc_cell_info(nas_user_t *user, int found, tac_t tac, ci_t ci, AcT_t AcT,
                       uint8_t rsrq, uint8_t rsrp)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /* Store LTE signal strength/quality measurement data */
  user->proc.rsrq = rsrq;
  user->proc.rsrp = rsrp;

  /*
   * Notify the EMM procedure call manager that cell information
   * have been received from the Access-Stratum sublayer
   */
  emm_sap.primitive = EMMAS_CELL_INFO_RES;
  emm_sap.u.emm_as.u.cell_info.found = found;
  emm_sap.u.emm_as.u.cell_info.plmnIDs.n_plmns = 0;
  emm_sap.u.emm_as.u.cell_info.rat = AcT;
  emm_sap.u.emm_as.u.cell_info.tac = tac;
  emm_sap.u.emm_as.u.cell_info.cellID = ci;

  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_establish_cnf()                                  **
 **                                                                        **
 ** Description: Processes the NAS signalling connection establishment     **
 **      confirm message received from the network                 **
 **                                                                        **
 ** Inputs:  data:      The initial NAS message transfered within  **
 **             the message                                **
 **      len:       The length of the initial NAS message      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_establish_cnf(nas_user_t *user, const Byte_t *data, uint32_t len)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /*
   * Notify the EMM procedure call manager that NAS signalling
   * connection establishment confirm message has been received
   * from the Access-Stratum sublayer
   */
  emm_sap.primitive = EMMAS_ESTABLISH_CNF;
  emm_sap.u.emm_as.u.establish.NASmsg.length = len;
  emm_sap.u.emm_as.u.establish.NASmsg.value = (uint8_t *)data;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_establish_rej()                                  **
 **                                                                        **
 ** Description: Processes the NAS signalling connection establishment     **
 **      confirm message received from the network while initial   **
 **      NAS message has not been delivered to the NAS sublayer on **
 **      the receiver side.                                        **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_establish_rej(nas_user_t *user)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /*
   * Notify the EMM procedure call manager that transmission
   * failure of initial NAS message indication has been received
   * from lower layers
   */
  emm_sap.primitive = EMMAS_ESTABLISH_REJ;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_release_ind()                                    **
 **                                                                        **
 ** Description: Processes the NAS signalling connection release indica-   **
 **      tion message received from the network                    **
 **                                                                        **
 ** Inputs:  cause:     The release cause                          **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_release_ind(nas_user_t *user, int cause)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /*
   * Notify the EMM procedure call manager that the NAS signalling
   * connection has been terminated by the network
   */
  emm_sap.primitive = EMMAS_RELEASE_IND;
  emm_sap.u.emm_as.u.release.cause = cause;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_ul_transfer_cnf()                                **
 **                                                                        **
 ** Description: Processes the uplink data transfer confirm message recei- **
 **      ved from the network while NAS message has been success-  **
 **      fully delivered to the NAS sublayer on the receiver side. **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_ul_transfer_cnf(nas_user_t *user)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /*
   * Notify the EMM procedure call manager that uplink NAS message
   * has been successfully delivered to the NAS sublayer on the
   * receiver side
   */
  emm_sap.primitive = EMMAS_DATA_IND;
  emm_sap.u.emm_as.u.data.ueid = user->ueid;
  emm_sap.u.emm_as.u.data.delivered = true;
  emm_sap.u.emm_as.u.data.NASmsg.length = 0;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_ul_transfer_rej()                                **
 **                                                                        **
 ** Description: Processes the uplink data transfer confirm message recei- **
 **      ved from the network while NAS message has not been deli- **
 **      vered to the NAS sublayer on the receiver side.           **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_ul_transfer_rej(nas_user_t *user)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /*
   * Notify the EMM procedure call manager that transmission
   * failure of uplink NAS message indication has been received
   * from lower layers
   */
  emm_sap.primitive = EMMAS_DATA_IND;
  emm_sap.u.emm_as.u.data.ueid = user->ueid;
  emm_sap.u.emm_as.u.data.delivered = false;
  emm_sap.u.emm_as.u.data.NASmsg.length = 0;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_proc_dl_transfer_ind()                                **
 **                                                                        **
 ** Description: Processes downlink data transfer indication message re-   **
 **      ceived from the network                                   **
 **                                                                        **
 ** Inputs:  data:      The transfered NAS message                 **
 **      len:       The length of the NAS message              **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_proc_dl_transfer_ind(nas_user_t *user, const Byte_t *data, uint32_t len)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  if (len > 0) {
    emm_sap_t emm_sap;
    /*
     * Notify the EMM procedure call manager that data transfer
     * indication has been received from the Access-Stratum sublayer
     */
    emm_sap.primitive = EMMAS_DATA_IND;
    emm_sap.u.emm_as.u.data.ueid = user->ueid;
    emm_sap.u.emm_as.u.data.delivered = true;
    emm_sap.u.emm_as.u.data.NASmsg.length = len;
    emm_sap.u.emm_as.u.data.NASmsg.value = (uint8_t *)data;
    rc = emm_sap_send(user, &emm_sap);
  }

  LOG_FUNC_RETURN (rc);
}



/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_proc_activate()                                       **
 **                                                                        **
 ** Description: Initiates a PDN connectivity procedure                    **
 **                                                                        **
 ** Inputs:  cid:       Identifier of the PDN context used to es-  **
 **             tablished connectivity to specified PDN    **
 **      apply_to_all:  true if the PDN connectivity procedure is  **
 **             initiated to establish connectivity to all **
 **             defined PDNs                               **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _nas_proc_activate(nas_user_t *user, int cid, bool apply_to_all)
{
  LOG_FUNC_IN;

  int rc;
  bool active = false;

  esm_sap_t esm_sap;

  /* Get PDN context parameters */
  rc = esm_main_get_pdn(user->esm_data, cid, &esm_sap.data.pdn_connect.pdn_type,
                        &esm_sap.data.pdn_connect.apn,
                        &esm_sap.data.pdn_connect.is_emergency, &active);

  if (rc != RETURNok) {
    /* No any context is defined for the specified PDN */
    if (apply_to_all) {
      /* Go ahead to activate next PDN context */
      LOG_FUNC_RETURN (RETURNok);
    }

    /* Return an error */
    LOG_FUNC_RETURN (RETURNerror);
  }

  if (active) {
    /* The PDN context is already active */
    LOG_TRACE(WARNING, "NAS-PROC  - PDN connection %d is active", cid);
    LOG_FUNC_RETURN (RETURNok);
  }

  if (esm_sap.data.pdn_connect.is_emergency) {
    if (esm_main_has_emergency(user->esm_data)) {
      /* There is already a PDN connection for emergency
       * bearer services established; the UE shall not
       * request an additional PDN connection for emer-
       * gency bearer services */
      LOG_TRACE(WARNING, "NAS-PROC  - PDN connection for emergency "
                "bearer services is already established (cid=%d)", cid);
      LOG_FUNC_RETURN (RETURNerror);
    }
  }

  /*
   * Notify ESM that a default EPS bearer has to be established
   * for the specified PDN
   */
  esm_sap.primitive = ESM_PDN_CONNECTIVITY_REQ;
  esm_sap.is_standalone = true;
  esm_sap.data.pdn_connect.is_defined = true;
  esm_sap.data.pdn_connect.cid = cid;
  rc = esm_sap_send(user, &esm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _nas_proc_deactivate()                                    **
 **                                                                        **
 ** Description: Initiates a PDN disconnect procedure                      **
 **                                                                        **
 ** Inputs:  cid:       Identifier of the PDN context              **
 **      apply_to_all:  true if the PDN disconnect procedure is    **
 **             initiated to request disconnection from    **
 **             all active PDNs                            **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _nas_proc_deactivate(nas_user_t *user, int cid, bool apply_to_all)
{
  LOG_FUNC_IN;

  int rc;
  int pdn_type;
  const char *apn;
  bool emergency = false;
  bool active = false;

  /* Get PDN context parameters */
  rc = esm_main_get_pdn(user->esm_data, cid, &pdn_type, &apn, &emergency, &active);

  if (rc != RETURNok) {
    /* No any context is defined for the specified PDN */
    if (apply_to_all) {
      /* Go ahead to deactivate next PDN connection */
      LOG_FUNC_RETURN (RETURNok);
    }

    LOG_FUNC_RETURN (RETURNerror);
  }

  if (!active) {
    /* The PDN connection is already inactive */
    LOG_TRACE(WARNING, "NAS-PROC  - PDN connection %d is not active", cid);
    LOG_FUNC_RETURN (RETURNok);
  }

  if (esm_main_get_nb_pdns(user->esm_data) > 1) {
    /*
     * Notify ESM that all EPS bearers towards the specified PDN
     * has to be released
     */
    esm_sap_t esm_sap;
    esm_sap.primitive = ESM_PDN_DISCONNECT_REQ;
    esm_sap.data.pdn_disconnect.cid = cid;
    rc = esm_sap_send(user, &esm_sap);
  } else {
    /* For EPS, if an attempt is made to disconnect the last PDN
     * connection, then the MT responds with an error */
    LOG_TRACE(WARNING,"NAS-PROC  - "
              "Attempt to disconnect from the last PDN is not allowed");
    rc = RETURNerror;
  }

  LOG_FUNC_RETURN (rc);
}


