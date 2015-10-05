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
 * Functions executed by the MME upon receiving EMM message from the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_attach_request()                                 **
 **                                                                        **
 ** Description: Processes Attach Request message                          **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_attach_request(unsigned int ueid, const attach_request_msg *msg,
                            int *emm_cause)
{
  int                    rc;
  uint8_t                gea = 0;
  emm_proc_attach_type_t type;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Attach Request message");

  /*
   * Message checking
   */
#if !defined(UPDATE_RELEASE_9)

  /* SR: In releases 9 and beyond the mandatory ie (old GUTI) has been removed */
  if ( (msg->oldgutiorimsi.guti.typeofidentity == EPS_MOBILE_IDENTITY_GUTI) &&
       !(msg->presencemask & ATTACH_REQUEST_OLD_GUTI_TYPE_PRESENT) ) {
    /* The Old GUTI type IE shall be included if the type of identity
     * in the EPS mobile identity IE is set to "GUTI" */
    *emm_cause = EMM_CAUSE_INVALID_MANDATORY_INFO;
    LOG_TRACE(WARNING, "EMMAS-SAP - [%08x] - Received GUTI identity without "
              "old GUTI IEI", ueid);
  } else
#endif /* !defined(UPDATE_RELEASE_9) */
    if (msg->uenetworkcapability.spare != 0b000) {
      /* Spare bits shall be coded as zero */
      *emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
      LOG_TRACE(WARNING, "EMMAS-SAP - [%08x] - Non zero spare bits is suspicious",
                ueid);
    }

  /* Handle message checking error */
  if (*emm_cause != EMM_CAUSE_SUCCESS) {
    /* 3GPP TS 24.301, section 5.5.1.2.7, case b
     * Perform protocol error abnormal case on the network side */
    rc = emm_proc_attach_reject(ueid, *emm_cause);
    *emm_cause = EMM_CAUSE_SUCCESS;
    LOG_FUNC_RETURN (rc);
  }

  /*
   * Message processing
   */
  /* Get the EPS attach type */
  if (msg->epsattachtype == EPS_ATTACH_TYPE_EPS) {
    type = EMM_ATTACH_TYPE_EPS;
  } else if (msg->epsattachtype == EPS_ATTACH_TYPE_IMSI) {
    type = EMM_ATTACH_TYPE_IMSI;
  } else if (msg->epsattachtype == EPS_ATTACH_TYPE_EMERGENCY) {
    type = EMM_ATTACH_TYPE_EMERGENCY;
  } else if (msg->epsattachtype == EPS_ATTACH_TYPE_RESERVED) {
    type = EMM_ATTACH_TYPE_RESERVED;
  } else {
    /* All other values shall be interpreted as "EPS attach" */
    type = EMM_ATTACH_TYPE_EPS;
  }

  /* Get the EPS mobile identity */
  GUTI_t guti, *p_guti = NULL;
  imsi_t imsi, *p_imsi = NULL;
  imei_t imei, *p_imei = NULL;

  if (msg->oldgutiorimsi.guti.typeofidentity == EPS_MOBILE_IDENTITY_GUTI) {
    /* Get the GUTI */
    p_guti = &guti;
    guti.gummei.plmn.MCCdigit1 = msg->oldgutiorimsi.guti.mccdigit1;
    guti.gummei.plmn.MCCdigit2 = msg->oldgutiorimsi.guti.mccdigit2;
    guti.gummei.plmn.MCCdigit3 = msg->oldgutiorimsi.guti.mccdigit3;
    guti.gummei.plmn.MNCdigit1 = msg->oldgutiorimsi.guti.mncdigit1;
    guti.gummei.plmn.MNCdigit2 = msg->oldgutiorimsi.guti.mncdigit2;
    guti.gummei.plmn.MNCdigit3 = msg->oldgutiorimsi.guti.mncdigit3;
    guti.gummei.MMEgid = msg->oldgutiorimsi.guti.mmegroupid;
    guti.gummei.MMEcode = msg->oldgutiorimsi.guti.mmecode;
    guti.m_tmsi = msg->oldgutiorimsi.guti.mtmsi;
  } else if (msg->oldgutiorimsi.imsi.typeofidentity == EPS_MOBILE_IDENTITY_IMSI) {
    /* Get the IMSI */
    p_imsi = &imsi;
    imsi.u.num.digit1 = msg->oldgutiorimsi.imsi.digit1;
    imsi.u.num.digit2 = msg->oldgutiorimsi.imsi.digit2;
    imsi.u.num.digit3 = msg->oldgutiorimsi.imsi.digit3;
    imsi.u.num.digit4 = msg->oldgutiorimsi.imsi.digit4;
    imsi.u.num.digit5 = msg->oldgutiorimsi.imsi.digit5;
    imsi.u.num.digit6 = msg->oldgutiorimsi.imsi.digit6;
    imsi.u.num.digit7 = msg->oldgutiorimsi.imsi.digit7;
    imsi.u.num.digit8 = msg->oldgutiorimsi.imsi.digit8;
    imsi.u.num.digit9 = msg->oldgutiorimsi.imsi.digit9;
    imsi.u.num.digit10 = msg->oldgutiorimsi.imsi.digit10;
    imsi.u.num.digit11 = msg->oldgutiorimsi.imsi.digit11;
    imsi.u.num.digit12 = msg->oldgutiorimsi.imsi.digit12;
    imsi.u.num.digit13 = msg->oldgutiorimsi.imsi.digit13;
    imsi.u.num.digit14 = msg->oldgutiorimsi.imsi.digit14;
    imsi.u.num.digit15 = msg->oldgutiorimsi.imsi.digit15;
    imsi.u.num.parity = msg->oldgutiorimsi.imsi.oddeven;
  } else if (msg->oldgutiorimsi.imei.typeofidentity == EPS_MOBILE_IDENTITY_IMEI) {
    /* Get the IMEI */
    p_imei = &imei;
    imei.u.num.digit1 = msg->oldgutiorimsi.imei.digit1;
    imei.u.num.digit2 = msg->oldgutiorimsi.imei.digit2;
    imei.u.num.digit3 = msg->oldgutiorimsi.imei.digit3;
    imei.u.num.digit4 = msg->oldgutiorimsi.imei.digit4;
    imei.u.num.digit5 = msg->oldgutiorimsi.imei.digit5;
    imei.u.num.digit6 = msg->oldgutiorimsi.imei.digit6;
    imei.u.num.digit7 = msg->oldgutiorimsi.imei.digit7;
    imei.u.num.digit8 = msg->oldgutiorimsi.imei.digit8;
    imei.u.num.digit9 = msg->oldgutiorimsi.imei.digit9;
    imei.u.num.digit10 = msg->oldgutiorimsi.imei.digit10;
    imei.u.num.digit11 = msg->oldgutiorimsi.imei.digit11;
    imei.u.num.digit12 = msg->oldgutiorimsi.imei.digit12;
    imei.u.num.digit13 = msg->oldgutiorimsi.imei.digit13;
    imei.u.num.digit14 = msg->oldgutiorimsi.imei.digit14;
    imei.u.num.digit15 = msg->oldgutiorimsi.imei.digit15;
    imei.u.num.parity = msg->oldgutiorimsi.imei.oddeven;
  }

  /* TODO: Get the UE network capabilities */

  /* Get the Last visited registered TAI */
  tai_t tai, *p_tai  = NULL;

  if (msg->presencemask & ATTACH_REQUEST_LAST_VISITED_REGISTERED_TAI_PRESENT) {
    p_tai = &tai;
    tai.plmn.MCCdigit1 = msg->lastvisitedregisteredtai.mccdigit1;
    tai.plmn.MCCdigit2 = msg->lastvisitedregisteredtai.mccdigit2;
    tai.plmn.MCCdigit3 = msg->lastvisitedregisteredtai.mccdigit3;
    tai.plmn.MNCdigit1 = msg->lastvisitedregisteredtai.mncdigit1;
    tai.plmn.MNCdigit2 = msg->lastvisitedregisteredtai.mncdigit2;
    tai.plmn.MNCdigit3 = msg->lastvisitedregisteredtai.mncdigit3;
    tai.tac = msg->lastvisitedregisteredtai.tac;
  }

  /* Execute the requested UE attach procedure */
#warning " TODO gea to be review"

  if (msg->msnetworkcapability.msnetworkcapabilityvalue.length > 0) {
    gea = (msg->msnetworkcapability.msnetworkcapabilityvalue.value[0] & 0x80) >> 1;

    if ((gea) && (msg->msnetworkcapability.msnetworkcapabilityvalue.length >= 2)) {
      gea |= ((msg->msnetworkcapability.msnetworkcapabilityvalue.value[1] & 0x60) >> 1);
    }
  }

  rc = emm_proc_attach_request(ueid, type,
                               msg->naskeysetidentifier.tsc != NAS_KEY_SET_IDENTIFIER_MAPPED,
                               msg->naskeysetidentifier.naskeysetidentifier,
                               msg->oldgutitype != GUTI_MAPPED, p_guti, p_imsi, p_imei, p_tai,
                               msg->uenetworkcapability.eea,
                               msg->uenetworkcapability.eia,
                               msg->uenetworkcapability.ucs2,
                               msg->uenetworkcapability.uea,
                               msg->uenetworkcapability.uia,
                               gea,
                               msg->uenetworkcapability.umts_present,
                               msg->uenetworkcapability.gprs_present,
                               &msg->esmmessagecontainer.esmmessagecontainercontents);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_attach_complete()                                **
 **                                                                        **
 ** Description: Processes Attach Complete message                         **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_attach_complete(unsigned int ueid, const attach_complete_msg *msg,
                             int *emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Attach Complete message");

  /* Execute the attach procedure completion */
  rc = emm_proc_attach_complete(ueid,
                                &msg->esmmessagecontainer.esmmessagecontainercontents);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_detach_request()                                 **
 **                                                                        **
 ** Description: Processes Detach Request message                          **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_detach_request(unsigned int ueid, const detach_request_msg *msg,
                            int *emm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Detach Request message");

  /*
   * Message processing
   */
  /* Get the detach type */
  emm_proc_detach_type_t type;

  if (msg->detachtype.typeofdetach == DETACH_TYPE_EPS) {
    type = EMM_DETACH_TYPE_EPS;
  } else if (msg->detachtype.typeofdetach == DETACH_TYPE_IMSI) {
    type = EMM_DETACH_TYPE_IMSI;
  } else if (msg->detachtype.typeofdetach == DETACH_TYPE_EPS_IMSI) {
    type = EMM_DETACH_TYPE_EPS_IMSI;
  } else if (msg->detachtype.typeofdetach == DETACH_TYPE_RESERVED_1) {
    type = EMM_DETACH_TYPE_RESERVED;
  } else if (msg->detachtype.typeofdetach == DETACH_TYPE_RESERVED_2) {
    type = EMM_DETACH_TYPE_RESERVED;
  } else {
    /* All other values are interpreted as "combined EPS/IMSI detach" */
    type = DETACH_TYPE_EPS_IMSI;
  }

  /* Get the EPS mobile identity */
  GUTI_t guti, *p_guti = NULL;
  imsi_t imsi, *p_imsi = NULL;
  imei_t imei, *p_imei = NULL;

  if (msg->gutiorimsi.guti.typeofidentity == EPS_MOBILE_IDENTITY_GUTI) {
    /* Get the GUTI */
    p_guti = &guti;
    guti.gummei.plmn.MCCdigit1 = msg->gutiorimsi.guti.mccdigit1;
    guti.gummei.plmn.MCCdigit2 = msg->gutiorimsi.guti.mccdigit2;
    guti.gummei.plmn.MCCdigit3 = msg->gutiorimsi.guti.mccdigit3;
    guti.gummei.plmn.MNCdigit1 = msg->gutiorimsi.guti.mncdigit1;
    guti.gummei.plmn.MNCdigit2 = msg->gutiorimsi.guti.mncdigit2;
    guti.gummei.plmn.MNCdigit3 = msg->gutiorimsi.guti.mncdigit3;
    guti.gummei.MMEgid = msg->gutiorimsi.guti.mmegroupid;
    guti.gummei.MMEcode = msg->gutiorimsi.guti.mmecode;
    guti.m_tmsi = msg->gutiorimsi.guti.mtmsi;
  } else if (msg->gutiorimsi.imsi.typeofidentity == EPS_MOBILE_IDENTITY_IMSI) {
    /* Get the IMSI */
    p_imsi = &imsi;
    imsi.u.num.digit1 = msg->gutiorimsi.imsi.digit1;
    imsi.u.num.digit2 = msg->gutiorimsi.imsi.digit2;
    imsi.u.num.digit3 = msg->gutiorimsi.imsi.digit3;
    imsi.u.num.digit4 = msg->gutiorimsi.imsi.digit4;
    imsi.u.num.digit5 = msg->gutiorimsi.imsi.digit5;
    imsi.u.num.digit6 = msg->gutiorimsi.imsi.digit6;
    imsi.u.num.digit7 = msg->gutiorimsi.imsi.digit7;
    imsi.u.num.digit8 = msg->gutiorimsi.imsi.digit8;
    imsi.u.num.digit9 = msg->gutiorimsi.imsi.digit9;
    imsi.u.num.digit10 = msg->gutiorimsi.imsi.digit10;
    imsi.u.num.digit11 = msg->gutiorimsi.imsi.digit11;
    imsi.u.num.digit12 = msg->gutiorimsi.imsi.digit12;
    imsi.u.num.digit13 = msg->gutiorimsi.imsi.digit13;
    imsi.u.num.digit14 = msg->gutiorimsi.imsi.digit14;
    imsi.u.num.digit15 = msg->gutiorimsi.imsi.digit15;
    imsi.u.num.parity = msg->gutiorimsi.imsi.oddeven;
  } else if (msg->gutiorimsi.imei.typeofidentity == EPS_MOBILE_IDENTITY_IMEI) {
    /* Get the IMEI */
    p_imei = &imei;
    imei.u.num.digit1 = msg->gutiorimsi.imei.digit1;
    imei.u.num.digit2 = msg->gutiorimsi.imei.digit2;
    imei.u.num.digit3 = msg->gutiorimsi.imei.digit3;
    imei.u.num.digit4 = msg->gutiorimsi.imei.digit4;
    imei.u.num.digit5 = msg->gutiorimsi.imei.digit5;
    imei.u.num.digit6 = msg->gutiorimsi.imei.digit6;
    imei.u.num.digit7 = msg->gutiorimsi.imei.digit7;
    imei.u.num.digit8 = msg->gutiorimsi.imei.digit8;
    imei.u.num.digit9 = msg->gutiorimsi.imei.digit9;
    imei.u.num.digit10 = msg->gutiorimsi.imei.digit10;
    imei.u.num.digit11 = msg->gutiorimsi.imei.digit11;
    imei.u.num.digit12 = msg->gutiorimsi.imei.digit12;
    imei.u.num.digit13 = msg->gutiorimsi.imei.digit13;
    imei.u.num.digit14 = msg->gutiorimsi.imei.digit14;
    imei.u.num.digit15 = msg->gutiorimsi.imei.digit15;
    imei.u.num.parity = msg->gutiorimsi.imei.oddeven;
  }

  /* Execute the UE initiated detach procedure completion by the network */
  rc = emm_proc_detach_request(ueid, type,
                               msg->detachtype.switchoff != DETACH_TYPE_NORMAL_DETACH,
                               msg->naskeysetidentifier.tsc != NAS_KEY_SET_IDENTIFIER_MAPPED,
                               msg->naskeysetidentifier.naskeysetidentifier,
                               p_guti, p_imsi, p_imei);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:        emm_recv_tracking_area_update_request()                   **
 **                                                                        **
 ** Description: Processes Tracking Area Update Request message            **
 **                                                                        **
 ** Inputs:      ueid:          UE lower layer identifier                  **
 **              msg:           The received EMM message                   **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     emm_cause:     EMM cause code                             **
 **              Return:        RETURNok, RETURNerror                      **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_tracking_area_update_request(unsigned int  ueid,
    const tracking_area_update_request_msg *msg,
    int *emm_cause)
{
  int rc = RETURNok;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Tracking Area Update Request message");

  /* LW: Not completely implemented; send a Received Tracking Area Update Reject to induce a Attach Request from UE! */
  rc = emm_proc_tracking_area_update_reject(ueid, EMM_CAUSE_IMPLICITLY_DETACHED);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_identity_response()                              **
 **                                                                        **
 ** Description: Processes Identity Response message                       **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_identity_response(unsigned int ueid, identity_response_msg *msg,
                               int *emm_cause)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Identity Response message");

  /*
   * Message processing
   */
  /* Get the mobile identity */
  imsi_t imsi, *p_imsi = NULL;
  imei_t imei, *p_imei = NULL;
  struct tmsi_struct_t {
    uint8_t  digit1:4;
    uint8_t  digit2:4;
    uint8_t  digit3:4;
    uint8_t  digit4:4;
    uint8_t  digit5:4;
    uint8_t  digit6:4;
    uint8_t  digit7:4;
    uint8_t  digit8:4;
  } tmsi, *p_tmsi = NULL;

  if (msg->mobileidentity.imsi.typeofidentity == MOBILE_IDENTITY_IMSI) {
    /* Get the IMSI */
    p_imsi = &imsi;
    imsi.u.num.digit1 = msg->mobileidentity.imsi.digit1;
    imsi.u.num.digit2 = msg->mobileidentity.imsi.digit2;
    imsi.u.num.digit3 = msg->mobileidentity.imsi.digit3;
    imsi.u.num.digit4 = msg->mobileidentity.imsi.digit4;
    imsi.u.num.digit5 = msg->mobileidentity.imsi.digit5;
    imsi.u.num.digit6 = msg->mobileidentity.imsi.digit6;
    imsi.u.num.digit7 = msg->mobileidentity.imsi.digit7;
    imsi.u.num.digit8 = msg->mobileidentity.imsi.digit8;
    imsi.u.num.digit9 = msg->mobileidentity.imsi.digit9;
    imsi.u.num.digit10 = msg->mobileidentity.imsi.digit10;
    imsi.u.num.digit11 = msg->mobileidentity.imsi.digit11;
    imsi.u.num.digit12 = msg->mobileidentity.imsi.digit12;
    imsi.u.num.digit13 = msg->mobileidentity.imsi.digit13;
    imsi.u.num.digit14 = msg->mobileidentity.imsi.digit14;
    imsi.u.num.digit15 = msg->mobileidentity.imsi.digit15;
    imsi.u.num.parity = msg->mobileidentity.imsi.oddeven;
  } else if ((msg->mobileidentity.imei.typeofidentity == MOBILE_IDENTITY_IMEI) ||
             (msg->mobileidentity.imeisv.typeofidentity==MOBILE_IDENTITY_IMEISV)) {
    /* Get the IMEI */
    p_imei = &imei;
    imei.u.num.digit1 = msg->mobileidentity.imei.digit1;
    imei.u.num.digit2 = msg->mobileidentity.imei.digit2;
    imei.u.num.digit3 = msg->mobileidentity.imei.digit3;
    imei.u.num.digit4 = msg->mobileidentity.imei.digit4;
    imei.u.num.digit5 = msg->mobileidentity.imei.digit5;
    imei.u.num.digit6 = msg->mobileidentity.imei.digit6;
    imei.u.num.digit7 = msg->mobileidentity.imei.digit7;
    imei.u.num.digit8 = msg->mobileidentity.imei.digit8;
    imei.u.num.digit9 = msg->mobileidentity.imei.digit9;
    imei.u.num.digit10 = msg->mobileidentity.imei.digit10;
    imei.u.num.digit11 = msg->mobileidentity.imei.digit11;
    imei.u.num.digit12 = msg->mobileidentity.imei.digit12;
    imei.u.num.digit13 = msg->mobileidentity.imei.digit13;
    imei.u.num.digit14 = msg->mobileidentity.imei.digit14;
    imei.u.num.digit15 = msg->mobileidentity.imei.digit15;
    imei.u.num.parity = msg->mobileidentity.imei.oddeven;
  } else if (msg->mobileidentity.tmsi.typeofidentity == MOBILE_IDENTITY_TMSI) {
    /* Get the TMSI */
    p_tmsi = &tmsi;
    tmsi.digit1 = msg->mobileidentity.tmsi.digit2;
    tmsi.digit2 = msg->mobileidentity.tmsi.digit3;
    tmsi.digit3 = msg->mobileidentity.tmsi.digit4;
    tmsi.digit4 = msg->mobileidentity.tmsi.digit5;
    tmsi.digit5 = msg->mobileidentity.tmsi.digit6;
    tmsi.digit6 = msg->mobileidentity.tmsi.digit7;
    tmsi.digit7 = msg->mobileidentity.tmsi.digit8;
    tmsi.digit8 = msg->mobileidentity.tmsi.digit9;
  }

  /* Execute the identification completion procedure */
  rc = emm_proc_identification_complete(ueid, p_imsi, p_imei,
                                        (uint32_t *)(p_tmsi));
  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_authentication_response()                        **
 **                                                                        **
 ** Description: Processes Authentication Response message                 **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_authentication_response(unsigned int ueid,
                                     authentication_response_msg *msg,
                                     int *emm_cause)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Authentication Response message");

  /*
   * Message checking
   */
  if (msg->authenticationresponseparameter.res.length == 0) {
    /* RES parameter shall not be null */
    *emm_cause = EMM_CAUSE_INVALID_MANDATORY_INFO;
  }

  /* Handle message checking error */
  if (*emm_cause != EMM_CAUSE_SUCCESS) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /*
   * Message processing
   */
  /* Execute the authentication completion procedure */
  rc = emm_proc_authentication_complete(ueid, EMM_CAUSE_SUCCESS,
                                        &msg->authenticationresponseparameter.res);
  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_authentication_failure()                         **
 **                                                                        **
 ** Description: Processes Authentication Failure message                  **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_authentication_failure(unsigned int ueid,
                                    authentication_failure_msg *msg,
                                    int *emm_cause)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Authentication Failure message");

  /*
   * Message checking
   */
  if (msg->emmcause == EMM_CAUSE_SUCCESS) {
    *emm_cause = EMM_CAUSE_INVALID_MANDATORY_INFO;
  } else if ( (msg->emmcause == EMM_CAUSE_SYNCH_FAILURE) &&
              !(msg->presencemask &
                AUTHENTICATION_FAILURE_AUTHENTICATION_FAILURE_PARAMETER_PRESENT) ) {
    /* AUTS parameter shall be present in case of "synch failure" */
    *emm_cause = EMM_CAUSE_INVALID_MANDATORY_INFO;
  }

  /* Handle message checking error */
  if (*emm_cause != EMM_CAUSE_SUCCESS) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /*
   * Message processing
   */
  /* Execute the authentication completion procedure */
  rc = emm_proc_authentication_complete(ueid, msg->emmcause,
                                        &msg->authenticationfailureparameter.auts);
  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_security_mode_complete()                         **
 **                                                                        **
 ** Description: Processes Security Mode Complete message                  **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int
emm_recv_security_mode_complete(
  unsigned int                ueid,
  security_mode_complete_msg *msg,
  int                        *emm_cause)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Security Mode Complete message");

  /*
   * Message processing
   */
  /* Execute the NAS security mode control completion procedure */
  rc = emm_proc_security_mode_complete(ueid);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_recv_security_mode_reject()                           **
 **                                                                        **
 ** Description: Processes Security Mode Reject message                    **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      msg:       The received EMM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_recv_security_mode_reject(unsigned int ueid,
                                  security_mode_reject_msg *msg,
                                  int *emm_cause)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO, "EMMAS-SAP - Received Security Mode Reject message "
            "(cause=%d)", msg->emmcause);

  /*
   * Message checking
   */
  if (msg->emmcause == EMM_CAUSE_SUCCESS) {
    *emm_cause = EMM_CAUSE_INVALID_MANDATORY_INFO;
  }

  /* Handle message checking error */
  if (*emm_cause != EMM_CAUSE_SUCCESS) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /*
   * Message processing
   */
  /* Execute the NAS security mode commend not accepted by the UE */
  rc = emm_proc_security_mode_reject(ueid);

  LOG_FUNC_RETURN (rc);
}
