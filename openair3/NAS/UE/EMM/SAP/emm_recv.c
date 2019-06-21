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

Source      emm_recv.c

Version     0.1

Date        2013/01/30

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines functions executed at the EMMAS Service Access
        Point upon receiving EPS Mobility Management messages
        from the Access Stratum sublayer.

*****************************************************************************/

#include "emm_recv.h"
#include "commonDef.h"
#include "nas_log.h"

#include "emm_msgDef.h"
#include "emm_cause.h"
#include "emm_proc.h"

#include <string.h> // memcpy

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 * Functions executed by both the UE and the MME upon receiving EMM messages
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_status()                                         **
 **                                                                        **
 ** Description: Processes EMM status message                              **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **          msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_status(unsigned int ueid, emm_status_msg *msg, int *emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "EMMAS-SAP - Received EMM Status message (cause=%d)",
            msg->emmcause);

  /*
   * Message checking
   */
  *emm_cause = EMM_CAUSE_SUCCESS;
  /*
   * Message processing
   */
  rc = emm_proc_status_ind(ueid, msg->emmcause);

  LOG_FUNC_RETURN (rc);
}

/*
 * --------------------------------------------------------------------------
 * Functions executed by the UE upon receiving EMM message from the network
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_attach_accept()                                  **
 **                                                                        **
 ** Description: Processes Attach Accept message                           **
 **                                                                        **
 ** Inputs:  msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_attach_accept(nas_user_t *user, attach_accept_msg *msg, int *emm_cause)
{
  LOG_FUNC_IN;

  int rc;
  int i;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Attach Accept message");

  /*
   * Message checking
   */
  // supported cases:
  // typeoflist = 1 Or
  // typeoflist = 0 and numberofelements = 1 (ie numberofelements equal to zero see 3gpp 24.301 9.9.3.33.1)
  LOG_TRACE(DEBUG,"attach accept type of list: %d, number of element: %d\n",msg->tailist.typeoflist, msg->tailist.numberofelements);
  if (!( (msg->tailist.typeoflist == TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_CONSECUTIVE_TACS) ||
         ((msg->tailist.typeoflist == 0) && ( msg->tailist.numberofelements == 0))
       )
     )
  {
    /* Only list of TACs belonging to one PLMN with consecutive
     * TAC values is supported */
    *emm_cause = EMM_CAUSE_IE_NOT_IMPLEMENTED;
  } else if ( (msg->presencemask & ATTACH_ACCEPT_GUTI_PRESENT) &&
              (msg->guti.guti.typeofidentity != EPS_MOBILE_IDENTITY_GUTI) ) {
    /* The only supported type of EPS mobile identity is GUTI */
    *emm_cause = EMM_CAUSE_IE_NOT_IMPLEMENTED;
  }

  /* Handle message checking error */
  if (*emm_cause != EMM_CAUSE_SUCCESS) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /*
   * Message processing
   */
  /* Compute timers value */
  long T3412, T3402 = -1, T3423 = -1;
  T3412 = gprs_timer_value(&msg->t3412value);

  if (msg->presencemask & ATTACH_ACCEPT_T3402_VALUE_PRESENT) {
    T3402 = gprs_timer_value(&msg->t3402value);
  }

  if (msg->presencemask & ATTACH_ACCEPT_T3423_VALUE_PRESENT) {
    T3423 = gprs_timer_value(&msg->t3423value);
  }

  /* Get the tracking area list the UE is registered to */

  /* LW: number of elements is coded as N-1 (0 -> 1 element, 1 -> 2 elements...),
   *  see 3GPP TS 24.301, section 9.9.3.33.1 */
  int n_tais = msg->tailist.numberofelements + 1;
  tai_t tai[n_tais];

  for (i = 0; i < n_tais; i++) {
    tai[i].plmn.MCCdigit1 = msg->tailist.mccdigit1;
    tai[i].plmn.MCCdigit2 = msg->tailist.mccdigit2;
    tai[i].plmn.MCCdigit3 = msg->tailist.mccdigit3;
    tai[i].plmn.MNCdigit1 = msg->tailist.mncdigit1;
    tai[i].plmn.MNCdigit2 = msg->tailist.mncdigit2;
    tai[i].plmn.MNCdigit3 = msg->tailist.mncdigit3;
    tai[i].tac = msg->tailist.tac + i;
  }

  /* Get the GUTI */
  GUTI_t *pguti = NULL;
  GUTI_t guti;

  if (msg->presencemask & ATTACH_ACCEPT_GUTI_PRESENT) {
    pguti = &guti;
    guti.gummei.plmn.MCCdigit1 = msg->guti.guti.mccdigit1;
    guti.gummei.plmn.MCCdigit2 = msg->guti.guti.mccdigit2;
    guti.gummei.plmn.MCCdigit3 = msg->guti.guti.mccdigit3;
    guti.gummei.plmn.MNCdigit1 = msg->guti.guti.mncdigit1;
    guti.gummei.plmn.MNCdigit2 = msg->guti.guti.mncdigit2;
    guti.gummei.plmn.MNCdigit3 = msg->guti.guti.mncdigit3;
    guti.gummei.MMEgid = msg->guti.guti.mmegroupid;
    guti.gummei.MMEcode = msg->guti.guti.mmecode;
    guti.m_tmsi = msg->guti.guti.mtmsi;
  }

  /* Get the list of equivalent PLMNs */
  int n_eplmns = 0;
  plmn_t eplmn;

  if (msg->presencemask & ATTACH_ACCEPT_EQUIVALENT_PLMNS_PRESENT) {
    n_eplmns = 1;
    eplmn.MCCdigit1 = msg->equivalentplmns.mccdigit1;
    eplmn.MCCdigit2 = msg->equivalentplmns.mccdigit2;
    eplmn.MCCdigit3 = msg->equivalentplmns.mccdigit3;
    eplmn.MNCdigit1 = msg->equivalentplmns.mncdigit1;
    eplmn.MNCdigit2 = msg->equivalentplmns.mncdigit2;
    eplmn.MNCdigit3 = msg->equivalentplmns.mncdigit3;
  }

  /* Execute attach procedure accepted by the network */
  rc = emm_proc_attach_accept(user, T3412, T3402, T3423, n_tais, tai, pguti,
                              n_eplmns, &eplmn,
                              &msg->esmmessagecontainer.esmmessagecontainercontents);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_attach_reject()                                  **
 **                                                                        **
 ** Description: Processes Attach Reject message                           **
 **                                                                        **
 ** Inputs:  msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_attach_reject(nas_user_t *user, attach_reject_msg *msg, int *emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Attach Reject message (cause=%d)",
            msg->emmcause);

  /*
   * Message checking
   */
  if ( (msg->emmcause == EMM_CAUSE_ESM_FAILURE) &&
       !(msg->presencemask & ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT) ) {
    /* The ATTACH REJECT message shall be combined with a PDN
     * CONNECTIVITY REJECT message contained in the ESM message
     * container information element */
    *emm_cause = EMM_CAUSE_INVALID_MANDATORY_INFO;
  }

  /* Handle message checking error */
  if (*emm_cause != EMM_CAUSE_SUCCESS) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /*
   * Message processing
   */
  if (msg->presencemask & ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT) {
    /* Execute attach procedure rejected by the network */
    rc = emm_proc_attach_reject(user, msg->emmcause,
                                &msg->esmmessagecontainer.esmmessagecontainercontents);
  } else {
    /* Execute attach procedure rejected by the network */
    rc = emm_proc_attach_reject(user, msg->emmcause, NULL);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_detach_accept()                                  **
 **                                                                        **
 ** Description: Processes Detach Accept message                           **
 **                                                                        **
 ** Inputs:  msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_detach_accept(nas_user_t *user, detach_accept_msg *msg, int *emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Detach Accept message");

  /*
   * Message processing
   */
  /* Execute the UE initiated detach procedure completion by the UE */
  rc = emm_proc_detach_accept(user);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_identity_request()                               **
 **                                                                        **
 ** Description: Processes Identity Request message                        **
 **                                                                        **
 ** Inputs:  msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_identity_request(nas_user_t *user, identity_request_msg *msg, int *emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Identity Request message");

  /*
   * Message processing
   */
  /* Get the requested identity type */
  emm_proc_identity_type_t type;

  if (msg->identitytype == IDENTITY_TYPE_2_IMSI) {
    type = EMM_IDENT_TYPE_IMSI;
  } else if (msg->identitytype == IDENTITY_TYPE_2_IMEI) {
    type = EMM_IDENT_TYPE_IMEI;
  } else if (msg->identitytype == IDENTITY_TYPE_2_IMEISV) {
    type = EMM_IDENT_TYPE_IMEISV;
  } else if (msg->identitytype == IDENTITY_TYPE_2_TMSI) {
    type = EMM_IDENT_TYPE_TMSI;
  } else {
    /* All other values are interpreted as IMSI */
    type = EMM_IDENT_TYPE_IMSI;
  }

  /* Execute the identification procedure initiated by the network */
  rc = emm_proc_identification_request(user, type);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_authentication_request()                         **
 **                                                                        **
 ** Description: Processes Authentication Request message                  **
 **                                                                        **
 ** Inputs:  msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_authentication_request(nas_user_t *user, authentication_request_msg *msg,
                                    int *emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Authentication Request message");

  /*
   * Message checking
   */
  if ( (msg->authenticationparameterrand.rand.length == 0) ||
       (msg->authenticationparameterautn.autn.length == 0) ) {
    /* RAND and AUTN parameters shall not be null */
    *emm_cause = EMM_CAUSE_INVALID_MANDATORY_INFO;
  }

  /* Handle message checking error */
  if (*emm_cause != EMM_CAUSE_SUCCESS) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /*
   * Message processing
   */
  /* Execute the authentication procedure initiated by the network */
  rc = emm_proc_authentication_request(user,
         msg->naskeysetidentifierasme.tsc != NAS_KEY_SET_IDENTIFIER_MAPPED,
         msg->naskeysetidentifierasme.naskeysetidentifier,
         &msg->authenticationparameterrand.rand,
         &msg->authenticationparameterautn.autn);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_authentication_reject()                          **
 **                                                                        **
 ** Description: Processes Authentication Reject message                   **
 **                                                                        **
 ** Inputs:  msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_authentication_reject(nas_user_t *user, authentication_reject_msg *msg,
                                   int *emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Authentication Reject message");

  /*
   * Message processing
   */
  /* Execute the authentication procedure not accepted by the network */
  rc = emm_proc_authentication_reject(user);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_security_mode_command()                          **
 **                                                                        **
 ** Description: Processes Security Mode Command message                   **
 **                                                                        **
 ** Inputs:  msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_security_mode_command(nas_user_t *user, security_mode_command_msg *msg,
                                   int *emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Security Mode Command message");

  /*
   * Message processing
   */
  /* Execute the security mode control procedure initiated by the network */
  LOG_TRACE(INFO,"Execute the security mode control procedure initiated by the network:  imeisvrequest %d\n",msg->imeisvrequest);
  rc = emm_proc_security_mode_command(user,
         msg->naskeysetidentifier.tsc != NAS_KEY_SET_IDENTIFIER_MAPPED,
         msg->naskeysetidentifier.naskeysetidentifier,
         msg->selectednassecurityalgorithms.typeofcipheringalgorithm,
         msg->selectednassecurityalgorithms.typeofintegrityalgorithm,
         msg->replayeduesecuritycapabilities.eea,
         msg->replayeduesecuritycapabilities.eia,
		 msg->imeisvrequest & 0x7);

  LOG_FUNC_RETURN (rc);
}


