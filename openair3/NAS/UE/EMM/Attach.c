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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************

Source      Attach.c

Version     0.1

Date        2012/12/04

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the attach related EMM procedure executed by the
        Non-Access Stratum.

        To get internet connectivity from the network, the network
        have to know about the UE. When the UE is switched on, it
        has to initiate the attach procedure to get initial access
        to the network and register its presence to the Evolved
        Packet Core (EPC) network in order to receive EPS services.

        As a result of a successful attach procedure, a context is
        created for the UE in the MME, and a default bearer is esta-
        blished between the UE and the PDN-GW. The UE gets the home
        agent IPv4 and IPv6 addresses and full connectivity to the
        IP network.

        The network may also initiate the activation of additional
        dedicated bearers for the support of a specific service.

*****************************************************************************/

#include "emm_proc.h"
#include "networkDef.h"
#include "nas_log.h"
#include "nas_timer.h"

#include "emmData.h"

#include "emm_sap.h"
#include "esm_sap.h"
#include "emm_cause.h"

#include "NasSecurityAlgorithms.h"

#include <string.h> // memcmp, memcpy
#include <stdlib.h> // malloc, free

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of the EPS attach type */
static const char *_emm_attach_type_str[] = {
  "EPS", "IMSI", "EMERGENCY", "RESERVED"
};

/*
 * --------------------------------------------------------------------------
 *      Internal data handled by the attach procedure in the UE
 * --------------------------------------------------------------------------
 */
/*
 * Timer handlers
 */
void *_emm_attach_t3410_handler(void *);
static void *_emm_attach_t3411_handler(void *);
static void *_emm_attach_t3402_handler(void *);

/*
 * Abnormal case attach procedure
 */
static void _emm_attach_abnormal_cases_bcd(emm_sap_t *);

/*
 * Internal data used for attach procedure
 */
static struct {
#define EMM_ATTACH_COUNTER_MAX  5
  unsigned int attempt_count; /* Counter used to limit the number of
                 * subsequently rejected attach attempts */
} _emm_attach_data = {0};


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *          Attach procedure executed by the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach()                                         **
 **                                                                        **
 ** Description: Initiate EPS attach procedure to register a UE in PS mode **
 **      of operation for EPS services only, or register a UE for  **
 **      emergency bearer services.                                **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.2                         **
 **      In state EMM-DEREGISTERED, the UE initiates the attach    **
 **      procedure by sending an ATTACH REQUEST message to the MME,**
 **      starting timer T3410 and entering state EMM-REGISTERED-   **
 **      INITIATED.                                                **
 **                                                                        **
 ** Inputs:  type:      Type of the requested attach               **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3402, T3410, T3411                        **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach(emm_proc_attach_type_t type)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  emm_as_establish_t *emm_as = &emm_sap.u.emm_as.u.establish;
  esm_sap_t esm_sap;
  int rc;

  LOG_TRACE(INFO, "EMM-PROC  - Initiate EPS attach type = %s (%d)",
            _emm_attach_type_str[type], type);

  /* Update the emergency bearer service indicator */
  if (type == EMM_ATTACH_TYPE_EMERGENCY) {
    _emm_data.is_emergency = TRUE;
  }

  /* Setup initial NAS information message to transfer */
  emm_as->NASinfo = EMM_AS_NAS_INFO_ATTACH;
  /* Set the attach type */
  emm_as->type = type;

  /* Set the RRC connection establishment cause */
  if (_emm_data.is_emergency) {
    emm_as->RRCcause = NET_ESTABLISH_CAUSE_EMERGENCY;
    emm_as->RRCtype = NET_ESTABLISH_TYPE_EMERGENCY_CALLS;
  } else {
    emm_as->RRCcause = NET_ESTABLISH_CAUSE_MO_SIGNAL;
    emm_as->RRCtype = NET_ESTABLISH_TYPE_ORIGINATING_SIGNAL;
  }

  /* Set the PLMN identifier of the selected PLMN */
  emm_as->plmnID = &_emm_data.splmn;
  /*
   * Process the EPS mobile identity
   */
  emm_as->UEid.guti = NULL;
  emm_as->UEid.tai  = NULL;
  emm_as->UEid.imsi = NULL;
  emm_as->UEid.imei = NULL;

  /* Check whether the UE is configured for "AttachWithIMSI" */
  if (_emm_data.AttachWithImsi) {
    /* Check whether the selected PLMN is neither the registered PLMN
     * nor in the list of equivalent PLMNs */
    if ( (!_emm_data.is_rplmn) && (!_emm_data.is_eplmn) ) {
      LOG_TRACE(INFO, "EMM-PROC  - Initiate EPS attach with IMSI");
      /* Include the IMSI */
      emm_as->UEid.imsi = _emm_data.imsi;
    } else {
      LOG_TRACE(INFO,
                "EMM-PROC  - Initiate EPS attach with NO IMSI, is registered PLMN %d, is equivalent PLMN %d",
                _emm_data.is_rplmn,
                _emm_data.is_eplmn);
    }
  } else if (_emm_data.guti) {
    LOG_TRACE(INFO, "EMM-PROC  - Initiate EPS attach with GUTI");
    /* Include a valid GUTI and the last visited registered TAI */
    emm_as->UEid.guti = _emm_data.guti;
    emm_as->UEid.tai = _emm_data.tai;
  } else if (!_emm_data.is_emergency) {
    LOG_TRACE(INFO, "EMM-PROC  - Initiate EPS attach with IMSI cause is no emergency and no GUTI");
    /* Include the IMSI if no valid GUTI is available */
    emm_as->UEid.imsi = _emm_data.imsi;
  } else {
    /* The UE is attaching for emergency bearer services and
     * does not hold a valid GUTI */
    if (_emm_data.imsi) {
      /* Include the IMSI if valid (USIM is present) */
      LOG_TRACE(INFO, "EMM-PROC  - Initiate EPS attach with IMSI cause is emergency and no GUTI");
      emm_as->UEid.imsi = _emm_data.imsi;
    } else {
      LOG_TRACE(INFO, "EMM-PROC  - Initiate EPS attach with IMSI cause is emergency and no GUTI and no IMSI");
      /* Include the IMEI if the IMSI is not valid */
      emm_as->UEid.imei = _emm_data.imei;
    }
  }

  /* Setup EPS NAS security data */
  emm_as_set_security_data(&emm_as->sctx, _emm_data.security, FALSE, FALSE);
  emm_as->ksi = EMM_AS_NO_KEY_AVAILABLE;

  if (_emm_data.security) {
    if (_emm_data.security->type != EMM_KSI_NOT_AVAILABLE) {
      emm_as->ksi = _emm_data.security->eksi;
    }

    LOG_TRACE(INFO, "EMM-PROC  - eps_encryption 0x%X", _emm_data.security->capability.eps_encryption);
    LOG_TRACE(INFO, "EMM-PROC  - eps_integrity  0x%X", _emm_data.security->capability.eps_integrity);
    emm_as->encryption = _emm_data.security->capability.eps_encryption;
    emm_as->integrity = _emm_data.security->capability.eps_integrity;
  }

  /*
   * Notify ESM that initiation of a PDN connectivity procedure
   * is requested to setup a default EPS bearer
   */
  esm_sap.primitive = ESM_PDN_CONNECTIVITY_REQ;
  esm_sap.is_standalone = FALSE;
  esm_sap.data.pdn_connect.is_defined = TRUE;
  esm_sap.data.pdn_connect.cid = 1;
  /* TODO: PDN type should be set according to the IP capability of the UE */
  esm_sap.data.pdn_connect.pdn_type = NET_PDN_TYPE_IPV4;
  esm_sap.data.pdn_connect.apn = NULL;
  esm_sap.data.pdn_connect.is_emergency = _emm_data.is_emergency;
  rc = esm_sap_send(&esm_sap);

  if (rc != RETURNerror) {
    /* Setup EMM procedure handler to be executed upon receiving
     * lower layer notification */
    rc = emm_proc_lowerlayer_initialize(emm_proc_attach_request,
                                        emm_proc_attach_failure,
                                        emm_proc_attach_release, NULL);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "Failed to initialize EMM procedure handler");
      LOG_FUNC_RETURN (RETURNerror);
    }

    /* Start T3410 timer */
    T3410.id = nas_timer_start(T3410.sec, _emm_attach_t3410_handler, NULL);
    LOG_TRACE(INFO,"EMM-PROC  - Timer T3410 (%d) expires in %ld seconds",
              T3410.id, T3410.sec);
    /* Stop T3402 and T3411 timers if running */
    T3402.id = nas_timer_stop(T3402.id);
    T3411.id = nas_timer_stop(T3411.id);

    /*
     * Notify EMM-AS SAP that a RRC connection establishment procedure
     * is requested from the Access-Stratum to send initial NAS message
     * attach request to the network
     */
    emm_sap.primitive = EMMAS_ESTABLISH_REQ;
    /* Get the PDN connectivity request to transfer within the ESM
     * container of the initial attach request message */
    emm_sap.u.emm_as.u.establish.NASmsg = esm_sap.send;
    rc = emm_sap_send(&emm_sap);
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_request()                                 **
 **                                                                        **
 ** Description: Performs the attach procedure upon receipt of indication  **
 **      from lower layers that Attach Request message has been    **
 **      successfully delivered to the network.                    **
 **                                                                        **
 ** Inputs:  args:      Not used                                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_request(void *args)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /*
   * Notify EMM that Attach Request has been sent to the network
   */
  emm_sap.primitive = EMMREG_ATTACH_REQ;
  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_accept()                                  **
 **                                                                        **
 ** Description: Performs the attach procedure accepted by the network.    **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.4                         **
 **      Upon receiving the ATTACH ACCEPT message, the UE shall    **
 **      stop timer T3410 and send an ATTACH COMPLETE message to   **
 **      the MME.                                                  **
 **                                                                        **
 ** Inputs:  t3412:     Value of the T3412 timer in seconds        **
 **      t3402:     Value of the T3402 timer in seconds        **
 **      t3423:     Value of the T3423 timer in seconds        **
 **      n_tais:    Number of tracking area identities contai- **
 **             ned in the TAI list                        **
 **      tai:       The TAI list that identifies the tracking  **
 **             areas the UE is registered to              **
 **      guti:      New UE's temporary identity assigned by    **
 **             the MME (GUTI reallocation)                **
 **      n_eplmns:  Number of equivalent PLMNs                 **
 **      eplmns:    List of equivalent PLMNs                   **
 **      esm_msg_pP:   Activate default EPS bearer context re-    **
 **             quest ESM message                          **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data, T3412, T3402, T3423             **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_accept(long t3412, long t3402, long t3423,
                           int n_tais, tai_t *tai, GUTI_t *guti,
                           int n_eplmns, plmn_t *eplmn,
                           const OctetString *esm_msg_pP)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  esm_sap_t esm_sap;
  int rc;
  int i;
  int j;

  LOG_TRACE(INFO, "EMM-PROC  - EPS attach accepted by the network");

  /* Stop timer T3410 */
  LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3410 (%d)", T3410.id);
  T3410.id = nas_timer_stop(T3410.id);

  /* Delete old TAI list and store the received TAI list */
  _emm_data.ltai.n_tais = n_tais;

  for (i = 0; (i < n_tais) && (i < EMM_DATA_TAI_MAX); i++) {
    _emm_data.ltai.tai[i] = tai[i];
  }

  /* Update periodic tracking area update timer value */
  T3412.sec = t3412;

  /* Update attach failure timer value */
  if ( !(t3402 < 0) ) {
    T3402.sec = t3402;
  }

  /* Update E-UTRAN deactivate ISR timer value */
  if ( !(t3423 < 0) ) {
    T3423.sec = t3423;
  }

  /* Delete old GUTI and store the new assigned GUTI if provided */
  if (guti) {
    *_emm_data.guti = *guti;
  }

  /* Update the stored list of equivalent PLMNs */
  _emm_data.nvdata.eplmn.n_plmns = 0;

  if (n_eplmns > 0) {
    for (i = 0; (i < n_eplmns) && (i < EMM_DATA_EPLMN_MAX); i++) {
      int is_forbidden = FALSE;

      if (!_emm_data.is_emergency) {
        /* If the attach procedure is not for emergency bearer
         * services, the UE shall remove from the list any PLMN
         * code that is already in the list of forbidden PLMNs */
        for (j = 0; j < _emm_data.fplmn.n_plmns; j++) {
          if (PLMNS_ARE_EQUAL(eplmn[i], _emm_data.fplmn.plmn[j])) {
            is_forbidden = TRUE;
            break;
          }
        }
      }

      if ( !is_forbidden ) {
        _emm_data.nvdata.eplmn.plmn[_emm_data.nvdata.eplmn.n_plmns++] =
          eplmn[i];
      }
    }

    /* Add the PLMN code of the registered PLMN that sent the list */
    if (_emm_data.nvdata.eplmn.n_plmns < EMM_DATA_EPLMN_MAX) {
      _emm_data.nvdata.eplmn.plmn[_emm_data.nvdata.eplmn.n_plmns++] =
        _emm_data.splmn;
    }
  }

  /*
   * Notify ESM that a default EPS bearer has to be activated
   */
  esm_sap.primitive = ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REQ;
  esm_sap.is_standalone = FALSE;
  esm_sap.recv = esm_msg_pP;
  rc = esm_sap_send(&esm_sap);

  if ( (rc != RETURNerror) && (esm_sap.err == ESM_SAP_SUCCESS) ) {
    /* Setup EMM procedure handler to be executed upon receiving
     * lower layer notification */
    rc = emm_proc_lowerlayer_initialize(emm_proc_attach_complete,
                                        emm_proc_attach_failure,
                                        NULL, NULL);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING,
                "EMM-PROC  - Failed to initialize EMM procedure handler");
      LOG_FUNC_RETURN (RETURNerror);
    }

    /*
     * Notify EMM-AS SAP that Attach Complete message together with
     * an Activate Default EPS Bearer Context Accept message has to
     * be sent to the network
     */
    emm_sap.primitive = EMMAS_DATA_REQ;
    emm_sap.u.emm_as.u.data.guti = _emm_data.guti;
    emm_sap.u.emm_as.u.data.ueid = 0;
    /* Setup EPS NAS security data */
    emm_as_set_security_data(&emm_sap.u.emm_as.u.data.sctx,
                             _emm_data.security, FALSE, TRUE);
    /* Get the activate default EPS bearer context accept message
     * to be transfered within the ESM container of the attach
     * complete message */
    emm_sap.u.emm_as.u.data.NASinfo = EMM_AS_NAS_DATA_ATTACH;
    emm_sap.u.emm_as.u.data.NASmsg = esm_sap.send;
    rc = emm_sap_send(&emm_sap);
  } else if (esm_sap.err != ESM_SAP_DISCARDED) {
    /* 3GPP TS 24.301, section 5.5.1.2.6, case j
     * If the ACTIVATE DEFAULT BEARER CONTEXT REQUEST message combined
     * with the ATTACH ACCEPT is not accepted by the UE due to failure
     * in the UE ESM sublayer, then the UE shall initiate the detach
     * procedure by sending a DETACH REQUEST message to the network.
     */
    emm_sap.primitive = EMMREG_DETACH_INIT;
    rc = emm_sap_send(&emm_sap);
  } else {
    /*
     * ESM procedure failed and, received message has been discarded or
     * Status message has been returned; ignore ESM procedure failure
     */
    rc = RETURNok;
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_reject()                                  **
 **                                                                        **
 ** Description: Performs the attach procedure rejected by the network.    **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.5                         **
 **      Upon receiving the ATTACH REJECT message, the UE shall    **
 **      stop timer T3410 and take actions depending on the EMM    **
 **      cause value received.                                     **
 **                                                                        **
 ** Inputs:  emm_cause: EMM cause indicating why the network re-   **
 **             jected the attach request                  **
 **      esm_msg_pP:   PDN connectivity reject ESM message        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data, _emm_attach_data, T3410         **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_reject(int emm_cause, const OctetString *esm_msg_pP)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  LOG_TRACE(WARNING, "EMM-PROC  - EPS attach rejected by the network, "
            "EMM cause = %d", emm_cause);

  /* Stop timer T3410 */
  LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3410 (%d)", T3410.id);
  T3410.id = nas_timer_stop(T3410.id);

  /* Update the EPS update status, the GUTI, the visited registered TAI and
   * the eKSI */
  switch (emm_cause) {
  case EMM_CAUSE_ILLEGAL_UE:
  case EMM_CAUSE_ILLEGAL_ME:
  case EMM_CAUSE_EPS_NOT_ALLOWED:
  case EMM_CAUSE_BOTH_NOT_ALLOWED:
  case EMM_CAUSE_PLMN_NOT_ALLOWED:
  case EMM_CAUSE_NOT_AUTHORIZED_IN_PLMN:
  case EMM_CAUSE_EPS_NOT_ALLOWED_IN_PLMN:
  case EMM_CAUSE_TA_NOT_ALLOWED:
  case EMM_CAUSE_ROAMING_NOT_ALLOWED:
  case EMM_CAUSE_NO_SUITABLE_CELLS:
    /* Set the EPS update status to EU3 ROAMING NOT ALLOWED */
    _emm_data.status = EU3_ROAMING_NOT_ALLOWED;
    /* Delete the GUTI */
    _emm_data.guti = NULL;
    /* Delete the last visited registered TAI */
    _emm_data.tai = NULL;

    /* Delete the eKSI */
    if (_emm_data.security) {
      _emm_data.security->type = EMM_KSI_NOT_AVAILABLE;
    }

    break;

  default :
    break;
  }

  /* Update list of equivalents PLMNs and attach attempt counter */
  switch (emm_cause) {
  case EMM_CAUSE_ILLEGAL_UE:
  case EMM_CAUSE_ILLEGAL_ME:
  case EMM_CAUSE_EPS_NOT_ALLOWED:
  case EMM_CAUSE_BOTH_NOT_ALLOWED:
    /* Consider the USIM as invalid for EPS services */
    _emm_data.usim_is_valid = FALSE;
    /* Delete the list of equivalent PLMNs */
    _emm_data.nvdata.eplmn.n_plmns = 0;
    break;

  case EMM_CAUSE_PLMN_NOT_ALLOWED:
  case EMM_CAUSE_NOT_AUTHORIZED_IN_PLMN:
  case EMM_CAUSE_ROAMING_NOT_ALLOWED:
    /* Delete the list of equivalent PLMNs */
    _emm_data.nvdata.eplmn.n_plmns = 0;
    /* Reset the attach attempt counter */
    _emm_attach_data.attempt_count = 0;
    break;

  case EMM_CAUSE_TA_NOT_ALLOWED:
  case EMM_CAUSE_EPS_NOT_ALLOWED_IN_PLMN:
  case EMM_CAUSE_NO_SUITABLE_CELLS:
    /* Reset the attach attempt counter */
    _emm_attach_data.attempt_count = 0;
    break;

  case EMM_CAUSE_ESM_FAILURE:

    /* 3GPP TS 24.301, section 5.5.1.2.6, case d */
    if (_emm_data.NAS_SignallingPriority != 1) {
      /* The UE is not configured for NAS signalling low priority;
       * set the attach attempt counter to 5 */
      _emm_attach_data.attempt_count = EMM_ATTACH_COUNTER_MAX;
    }

    break;

  case EMM_CAUSE_SEMANTICALLY_INCORRECT:
  case EMM_CAUSE_INVALID_MANDATORY_INFO:
  case EMM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED:
  case EMM_CAUSE_IE_NOT_IMPLEMENTED:
  case EMM_CAUSE_PROTOCOL_ERROR:
    /* 3GPP TS 24.301, section 5.5.1.2.6, case d
     * Set the attach attempt counter to 5 */
    _emm_attach_data.attempt_count = EMM_ATTACH_COUNTER_MAX;
    break;

  default :
    break;
  }

  /* Update "forbidden lists" */
  switch (emm_cause) {
  case EMM_CAUSE_PLMN_NOT_ALLOWED:
  case EMM_CAUSE_NOT_AUTHORIZED_IN_PLMN:
    /* Store the PLMN identity in the "forbidden PLMN list" */
    _emm_data.fplmn.plmn[_emm_data.fplmn.n_plmns++] = _emm_data.splmn;
    break;

  case EMM_CAUSE_TA_NOT_ALLOWED:
    /* Store the current TAI in the list of "forbidden tracking
     * areas for regional provision of service" */
    _emm_data.ftai.tai[_emm_data.ftai.n_tais++] = *_emm_data.tai;
    break;

  case EMM_CAUSE_ROAMING_NOT_ALLOWED:
    /* Store the current TAI in the list of "forbidden tracking
     * areas for roaming" */
    _emm_data.ftai_roaming.tai[_emm_data.ftai_roaming.n_tais++] = *_emm_data.tai;
    break;

  case EMM_CAUSE_EPS_NOT_ALLOWED_IN_PLMN:
    /* Store the PLMN identity in the "forbidden PLMNs for GPRS
     * service" list */
    _emm_data.fplmn_gprs.plmn[_emm_data.fplmn_gprs.n_plmns++] = _emm_data.splmn;
    break;

  default :
    break;
  }

  /* Update state of EMM sublayer */
  switch (emm_cause) {
  case EMM_CAUSE_ILLEGAL_UE:
  case EMM_CAUSE_ILLEGAL_ME:
  case EMM_CAUSE_EPS_NOT_ALLOWED:
  case EMM_CAUSE_BOTH_NOT_ALLOWED:
    /*
     * Notify EMM that EPS attach is rejected
     */
    emm_sap.primitive = EMMREG_ATTACH_REJ;
    break;

  case EMM_CAUSE_PLMN_NOT_ALLOWED:
  case EMM_CAUSE_NOT_AUTHORIZED_IN_PLMN:
  case EMM_CAUSE_EPS_NOT_ALLOWED_IN_PLMN:
    /*
     * Notify EMM that the UE has to perform a PLMN selection because
     * it is not allowed to operate in the currently selected PLMN
     */
    emm_sap.primitive = EMMREG_REGISTER_REQ;
    break;

  case EMM_CAUSE_TA_NOT_ALLOWED:
  case EMM_CAUSE_ROAMING_NOT_ALLOWED:
  case EMM_CAUSE_NO_SUITABLE_CELLS:
    /*
     * Notify EMM that the UE failed to register to the network for
     * EPS services because it is not allowed to operate in the
     * requested tracking area
     */
    emm_sap.primitive = EMMREG_REGISTER_REJ;
    break;

  case EMM_CAUSE_IMEI_NOT_ACCEPTED:
    if (_emm_data.is_emergency) {
      /*
       * Notify EMM that the UE failed to register to the network
       * for emergency bearer services because "IMEI not accepted"
       */
      emm_sap.primitive = EMMREG_NO_IMSI;
      break;
    }

    /* break is volontary missing */

  default :
    /* Other values are considered as abnormal cases
     * 3GPP TS 24.301, section 5.5.1.2.6, case d */
    _emm_attach_abnormal_cases_bcd(&emm_sap);
    break;
  }

  rc = emm_sap_send(&emm_sap);

  /*
   * Notify ESM that the network rejected connectivity to the PDN
   */
  if (esm_msg_pP != NULL) {
    esm_sap_t esm_sap;
    esm_sap.primitive = ESM_PDN_CONNECTIVITY_REJ;
    esm_sap.is_standalone = FALSE;
    esm_sap.recv = esm_msg_pP;
    rc = esm_sap_send(&esm_sap);
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_complete()                                **
 **                                                                        **
 ** Description: Terminates the attach procedure when Attach Complete mes- **
 **      sage has been successfully delivered to the MME.          **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.4                         **
 **      Upon successfully sending the ATTACH COMPLETE message,    **
 **      the UE shall reset the attach attempt counter and tra-    **
 **      cking area updating attempt counter, enter state EMM-     **
 **      REGISTERED and set the EPS update status to EU1-UPDATED.  **
 **                                                                        **
 ** Inputs:  args:      Not used                                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data, _emm_attach_data                **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_complete(void *args)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  esm_sap_t esm_sap;
  int rc;

  LOG_TRACE(INFO, "EMM-PROC  - EPS attach complete");

  /* Reset EMM procedure handler */
  (void) emm_proc_lowerlayer_initialize(NULL, NULL, NULL, NULL);

  /* Reset the attach attempt counter */
  _emm_attach_data.attempt_count = 0;
  /* TODO: Reset the tracking area updating attempt counter */

  /* Set the EPS update status to EU1 UPDATED */
  _emm_data.status = EU1_UPDATED;
  _emm_data.is_attached = TRUE;

  /*
   * Notify EMM that network attach complete message has been delivered
   * to the network
   */
  emm_sap.primitive = EMMREG_ATTACH_CNF;
  rc = emm_sap_send(&emm_sap);

  if (rc != RETURNerror) {
    /*
     * Notify ESM that the Activate Default EPS Bearer Context Accept
     * message has been delivered to the network within the Attach
     * Complete message
     */
    esm_sap.primitive = ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_CNF;
    esm_sap.is_standalone = FALSE;
    rc = esm_sap_send(&esm_sap);
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_failure()                                 **
 **                                                                        **
 ** Description: Performs the attach procedure abnormal case upon receipt  **
 **      of transmission failure of Attach Request message or At-  **
 **      tach Complete message.                                    **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.6, cases h and i          **
 **      The  UE  shall restart the attach  procedure when timer   **
 **      T3411 expires.                                            **
 **                                                                        **
 ** Inputs:  is_initial:    TRUE if the NAS message that failed to be  **
 **             transfered is an initial NAS message (ESM  **
 **             message embedded within an Attach Request  **
 **             message)                                   **
 **          args:      Not used                                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3410, T3411                               **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_failure(int is_initial, void *args)
{
  LOG_FUNC_IN;

  int rc = RETURNok;
  esm_sap_t esm_sap;

  LOG_TRACE(WARNING, "EMM-PROC  - EPS attach failure");

  /* Reset EMM procedure handler */
  (void) emm_proc_lowerlayer_initialize(NULL, NULL, NULL, NULL);

  /* Stop timer T3410 if still running */
  if (T3410.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3410 (%d)", T3410.id);
    T3410.id = nas_timer_stop(T3410.id);
  }

  if (is_initial) {
    /*
     * Notify ESM that the PDN CONNECTIVITY REQUEST message contained
     * in the ESM message container IE of the ATTACH REQUEST has failed
     * to be transmitted
     */
    esm_sap.primitive = ESM_PDN_CONNECTIVITY_REJ;
    esm_sap.is_standalone = FALSE;
    esm_sap.recv = NULL;
  } else {
    /*
     * Notify ESM that ACTIVATE DEFAULT EPS BEARER CONTEXT REQUEST message
     * contained in the ESM message container IE of the ATTACH COMPLETE
     * has failed to be transmitted
     */
    esm_sap.primitive = ESM_DEFAULT_EPS_BEARER_CONTEXT_ACTIVATE_REJ;
    esm_sap.is_standalone = FALSE;
    esm_sap.recv = NULL;
  }

  rc = esm_sap_send(&esm_sap);

  if (rc != RETURNerror) {
    /* Start T3411 timer */
    T3411.id = nas_timer_start(T3411.sec, _emm_attach_t3411_handler, NULL);
    LOG_TRACE(INFO, "EMM-PROC  - Timer T3411 (%d) expires in %ld seconds",
              T3411.id, T3411.sec);
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_release()                                 **
 **                                                                        **
 ** Description: Performs the attach procedure abnormal case upon receipt  **
 **      of NAS signalling connection release indication.          **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.6, case b                 **
 **      The attach procedure shall be aborted and the UE shall    **
 **      execute abnormal case attach procedure.                   **
 **                                                                        **
 ** Inputs:  args:      Not used                                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_release(void *args)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  LOG_TRACE(WARNING, "EMM-PROC  - NAS signalling connection released");

  /* Execute abnormal case attach procedure */
  _emm_attach_abnormal_cases_bcd(&emm_sap);

  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_restart()                                 **
 **                                                                        **
 ** Description: Restarts the attach procedure                             **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_restart(void)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  LOG_TRACE(INFO, "EMM-PROC  - Restart EPS attach procedure");

  /*
   * Notify EMM that the attach procedure has to be restarted
   */
  emm_sap.primitive = EMMREG_ATTACH_INIT;
  emm_sap.u.emm_reg.u.attach.is_emergency = _emm_data.is_emergency;
  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_set_emergency()                           **
 **                                                                        **
 ** Description: Set the emergency bearer services indicator               **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_set_emergency(void)
{
  LOG_FUNC_IN;

  LOG_TRACE(WARNING, "EMM-PROC  - UE is now attached to the network for "
            "emergency bearer services only");

  _emm_data.is_emergency = TRUE;

  LOG_FUNC_RETURN(RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_attach_set_detach()                              **
 **                                                                        **
 ** Description: Reset the network attachment indicator and enter state    **
 **      EMM-DEREGISTERED
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int emm_proc_attach_set_detach(void)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(WARNING,
            "EMM-PROC  - UE is now locally detached from the network");

  /* Reset the network attachment indicator */
  _emm_data.is_attached = FALSE;
  /*
   * Notify that the UE is locally detached from the network
   */
  emm_sap_t emm_sap;
  emm_sap.primitive = EMMREG_DETACH_CNF;
  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN(rc);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *              Timer handlers
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_t3410_handler()                               **
 **                                                                        **
 ** Description: T3410 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.6, case c                 **
 **      Upon T3410 timer expiration, the attach procedure shall   **
 **      be aborted and the UE shall execute abnormal case attach  **
 **      procedure.                                                **
 **      The NAS signalling connection shall be released locally.  **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    T3410                                      **
 **                                                                        **
 ***************************************************************************/
void *_emm_attach_t3410_handler(void *args)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  LOG_TRACE(WARNING, "EMM-PROC  - T3410 timer expired");

  /* Stop T3410 timer */
  T3410.id = nas_timer_stop(T3410.id);
  /* Execute abnormal case attach procedure */
  _emm_attach_abnormal_cases_bcd(&emm_sap);

  rc = emm_sap_send(&emm_sap);

  if (rc != RETURNerror) {
    /* Locally release the NAS signalling connection */
    _emm_data.ecm_status = ECM_IDLE;
  }

  LOG_FUNC_RETURN(NULL);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_t3411_handler()                               **
 **                                                                        **
 ** Description: T3411 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.1.2.6                         **
 **      Upon T3411 timer expiration, the attach procedure shall   **
 **      be restarted, if still required by ESM sublayer.          **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    T3411                                      **
 **                                                                        **
 ***************************************************************************/
static void *_emm_attach_t3411_handler(void *args)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;

  LOG_TRACE(WARNING, "EMM-PROC  - T3411 timer expired");

  /* Stop T3411 timer */
  T3411.id = nas_timer_stop(T3411.id);
  /*
   * Notify EMM that timer T3411 expired and attach procedure has to be
   * restarted
   */
  emm_sap.primitive = EMMREG_ATTACH_INIT;
  emm_sap.u.emm_reg.u.attach.is_emergency = _emm_data.is_emergency;

  (void) emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN(NULL);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_t3402_handler()                               **
 **                                                                        **
 ** Description: T3402 timeout handler                                     **
 **                                                                        **
 **      Upon T3402 timer expiration:                              **
 **              3GPP TS 24.301, section 5.5.1.1                           **
 **      the attach attempt counter shall be reset when the UE is  **
 **      in substate DEREGISTERED.ATTEMPTING-TO-ATTACH;            **
 **              3GPP TS 24.301, section 5.2.2.3.3                         **
 **      the UE shall initiate an attach or combined attach proce- **
 **      dure in substate DEREGISTERED.ATTEMPTING-TO-ATTACH;       **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_attach_data, T3402                    **
 **                                                                        **
 ***************************************************************************/
static void *_emm_attach_t3402_handler(void *args)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;

  LOG_TRACE(WARNING, "EMM-PROC  - T3402 timer expired");

  /* Stop T3402 timer */
  T3402.id = nas_timer_stop(T3402.id);
  /* Reset the attach attempt counter */
  _emm_attach_data.attempt_count = 0;
  /*
   * Notify EMM that timer T3402 expired and attach procedure has to be
   * restarted
   */
  emm_sap.primitive = EMMREG_ATTACH_INIT;
  emm_sap.u.emm_reg.u.attach.is_emergency = _emm_data.is_emergency;

  (void) emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN(NULL);
}

/*
 * --------------------------------------------------------------------------
 *              Abnormal cases in the UE
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_attach_abnormal_cases_bcd()                          **
 **                                                                        **
 ** Description: Performs the abnormal case attach procedure.              **
 **                                                                        **
 **      3GPP TS 24.301, section 5.5.1.2.6, cases b, c and d       **
 **      The Timer T3410 shall be stopped if still running, the    **
 **      attach attempt counter shall be incremented and the UE    **
 **      shall proceed depending on whether the attach attempt     **
 **      counter reached its maximum value or not.                 **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_sap:   EMM service access point                   **
 **      Return:    None                                       **
 **      Others:    _emm_data, _emm_attach_data, T3402, T3410, **
 **             T3411                                      **
 **                                                                        **
 ***************************************************************************/
static void _emm_attach_abnormal_cases_bcd(emm_sap_t *emm_sap)
{
  LOG_FUNC_IN;

  LOG_TRACE(WARNING, "EMM-PROC  - Abnormal case, attach counter = %d",
            _emm_attach_data.attempt_count);

  /* Stop timer T3410 */
  if (T3410.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "EMM-PROC  - Stop timer T3410 (%d)", T3410.id);
    T3410.id = nas_timer_stop(T3410.id);
  }

  if (_emm_attach_data.attempt_count < EMM_ATTACH_COUNTER_MAX) {
    /* Increment the attach attempt counter */
    _emm_attach_data.attempt_count += 1;
    /* Start T3411 timer */
    T3411.id = nas_timer_start(T3411.sec, _emm_attach_t3411_handler, NULL);
    LOG_TRACE(INFO, "EMM-PROC  - Timer T3411 (%d) expires in %ld seconds",
              T3411.id, T3411.sec);
    /*
     * Notify EMM that the attempt to attach for EPS services failed and
     * the attach attempt counter didn't reach its maximum value; network
     * attach procedure shall be restarted when timer T3411 expires.
     */
    emm_sap->primitive = EMMREG_ATTACH_FAILED;
  } else {
    /* Delete the GUTI */
    _emm_data.guti = NULL;
    /* Delete the TAI list */
    _emm_data.ltai.n_tais = 0;
    /* Delete the last visited registered TAI */
    _emm_data.tai = NULL;
    /* Delete the list of equivalent PLMNs */
    _emm_data.nvdata.eplmn.n_plmns = 0;

    /* Delete the eKSI */
    if (_emm_data.security) {
      _emm_data.security->type = EMM_KSI_NOT_AVAILABLE;
    }

    /* Set the EPS update status to EU2 NOT UPDATED */
    _emm_data.status = EU2_NOT_UPDATED;

    /* Start T3402 timer */
    T3402.id = nas_timer_start(T3402.sec, _emm_attach_t3402_handler, NULL);
    LOG_TRACE(INFO, "EMM-PROC  - Timer T3402 (%d) expires in %ld seconds",
              T3402.id, T3402.sec);
    /*
     * Notify EMM that the attempt to attach for EPS services failed and
     * the attach attempt counter reached its maximum value.
     */
    emm_sap->primitive = EMMREG_ATTACH_EXCEEDED;
  }

  LOG_FUNC_OUT;
}
