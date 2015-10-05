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

Source      emm_send.c

Version     0.1

Date        2013/01/30

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines functions executed at the EMMAS Service Access
        Point to send EPS Mobility Management messages to the
        Access Stratum sublayer.

*****************************************************************************/

#include "emm_send.h"
#include "commonDef.h"
#include "nas_log.h"

#include "emm_msgDef.h"
#include "emm_proc.h"

#include <string.h> // strlen

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 * Functions executed by both the UE and the MME to send EMM messages
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_send_status()                                         **
 **                                                                        **
 ** Description: Builds EMM status message                                 **
 **                                                                        **
 **      The EMM status message is sent by the UE or the network   **
 **      at any time to report certain error conditions.           **
 **                                                                        **
 ** Inputs:  emm_cause: EMM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_msg:   The EMM message to be sent                 **
 **      Return:    The size of the EMM message                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_send_status(const emm_as_status_t *msg, emm_status_msg *emm_msg)
{
  LOG_FUNC_IN;

  int size = EMM_HEADER_MAXIMUM_LENGTH;

  LOG_TRACE(WARNING, "EMMAS-SAP - Send EMM Status message (cause=%d)",
            msg->emm_cause);

  /* Mandatory - Message type */
  emm_msg->messagetype = EMM_STATUS;

  /* Mandatory - EMM cause */
  size += EMM_CAUSE_MAXIMUM_LENGTH;
  emm_msg->emmcause = msg->emm_cause;

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_send_detach_accept()                                  **
 **                                                                        **
 ** Description: Builds Detach Accept message                              **
 **                                                                        **
 **      The Detach Accept message is sent by the UE or the net-   **
 **      work to indicate that the detach procedure has been com-  **
 **      pleted.                                                   **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_msg:   The EMM message to be sent                 **
 **      Return:    The size of the EMM message                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_send_detach_accept(const emm_as_data_t *msg,
                           detach_accept_msg *emm_msg)
{
  LOG_FUNC_IN;

  int size = EMM_HEADER_MAXIMUM_LENGTH;

  LOG_TRACE(INFO, "EMMAS-SAP - Send Detach Accept message");

  /* Mandatory - Message type */
  emm_msg->messagetype = DETACH_ACCEPT;

  LOG_FUNC_RETURN (size);
}


/*
 * --------------------------------------------------------------------------
 * Functions executed by the MME to send EMM messages to the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    emm_send_attach_accept()                                  **
 **                                                                        **
 ** Description: Builds Attach Accept message                              **
 **                                                                        **
 **      The Attach Accept message is sent by the network to the   **
 **      UE to indicate that the corresponding attach request has  **
 **      been accepted.                                            **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_msg:   The EMM message to be sent                 **
 **      Return:    The size of the EMM message                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_send_attach_accept(const emm_as_establish_t *msg,
                           attach_accept_msg *emm_msg)
{
  LOG_FUNC_IN;

  int size = EMM_HEADER_MAXIMUM_LENGTH;

  LOG_TRACE(INFO, "EMMAS-SAP - Send Attach Accept message");
  LOG_TRACE(INFO, "EMMAS-SAP - size = EMM_HEADER_MAXIMUM_LENGTH(%d)",
            size);

  /* Mandatory - Message type */
  emm_msg->messagetype = ATTACH_ACCEPT;

  /* Mandatory - EPS attach result */
  size += EPS_ATTACH_RESULT_MAXIMUM_LENGTH;
  emm_msg->epsattachresult = EPS_ATTACH_RESULT_EPS;

  /* Mandatory - T3412 value */
  size += GPRS_TIMER_MAXIMUM_LENGTH;
  emm_msg->t3412value.unit = GPRS_TIMER_UNIT_0S;
  LOG_TRACE(INFO, "EMMAS-SAP - size += GPRS_TIMER_MAXIMUM_LENGTH(%d)  (%d)",
            GPRS_TIMER_MAXIMUM_LENGTH, size);

  /* Mandatory - Tracking area identity list */
  size += TRACKING_AREA_IDENTITY_LIST_MINIMUM_LENGTH;
  emm_msg->tailist.typeoflist =
    TRACKING_AREA_IDENTITY_LIST_ONE_PLMN_CONSECUTIVE_TACS;
  emm_msg->tailist.numberofelements = msg->n_tacs;
  emm_msg->tailist.mccdigit1 = msg->UEid.guti->gummei.plmn.MCCdigit1;
  emm_msg->tailist.mccdigit2 = msg->UEid.guti->gummei.plmn.MCCdigit2;
  emm_msg->tailist.mccdigit3 = msg->UEid.guti->gummei.plmn.MCCdigit3;
  emm_msg->tailist.mncdigit1 = msg->UEid.guti->gummei.plmn.MNCdigit1;
  emm_msg->tailist.mncdigit2 = msg->UEid.guti->gummei.plmn.MNCdigit2;
  emm_msg->tailist.mncdigit3 = msg->UEid.guti->gummei.plmn.MNCdigit3;
  emm_msg->tailist.tac = msg->tac;
  LOG_TRACE(INFO,
            "EMMAS-SAP - size += "\
            "TRACKING_AREA_IDENTITY_LIST_MINIMUM_LENGTH(%d)  (%d)",
            TRACKING_AREA_IDENTITY_LIST_MINIMUM_LENGTH,
            size);

  /* Mandatory - ESM message container */
  size += ESM_MESSAGE_CONTAINER_MINIMUM_LENGTH + msg->NASmsg.length;
  emm_msg->esmmessagecontainer.esmmessagecontainercontents = msg->NASmsg;
  LOG_TRACE(INFO,
            "EMMAS-SAP - size += "\
            "ESM_MESSAGE_CONTAINER_MINIMUM_LENGTH(%d)  (%d)",
            ESM_MESSAGE_CONTAINER_MINIMUM_LENGTH,
            size);

  /* Optional - GUTI */
  if (msg->new_guti) {
    size += EPS_MOBILE_IDENTITY_MAXIMUM_LENGTH;
    emm_msg->presencemask |= ATTACH_ACCEPT_GUTI_PRESENT;
    emm_msg->guti.guti.typeofidentity = EPS_MOBILE_IDENTITY_GUTI;
    emm_msg->guti.guti.oddeven = EPS_MOBILE_IDENTITY_EVEN;
    emm_msg->guti.guti.mmegroupid = msg->new_guti->gummei.MMEgid;
    emm_msg->guti.guti.mmecode = msg->new_guti->gummei.MMEcode;
    emm_msg->guti.guti.mtmsi = msg->new_guti->m_tmsi;
    emm_msg->guti.guti.mccdigit1 = msg->new_guti->gummei.plmn.MCCdigit1;
    emm_msg->guti.guti.mccdigit2 = msg->new_guti->gummei.plmn.MCCdigit2;
    emm_msg->guti.guti.mccdigit3 = msg->new_guti->gummei.plmn.MCCdigit3;
    emm_msg->guti.guti.mncdigit1 = msg->new_guti->gummei.plmn.MNCdigit1;
    emm_msg->guti.guti.mncdigit2 = msg->new_guti->gummei.plmn.MNCdigit2;
    emm_msg->guti.guti.mncdigit3 = msg->new_guti->gummei.plmn.MNCdigit3;
    LOG_TRACE(INFO,
              "EMMAS-SAP - size += "\
              "EPS_MOBILE_IDENTITY_MAXIMUM_LENGTH(%d)  (%d)",
              EPS_MOBILE_IDENTITY_MAXIMUM_LENGTH,
              size);
  }

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_send_attach_reject()                                  **
 **                                                                        **
 ** Description: Builds Attach Reject message                              **
 **                                                                        **
 **      The Attach Reject message is sent by the network to the   **
 **      UE to indicate that the corresponding attach request has  **
 **      been rejected.                                            **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_msg:   The EMM message to be sent                 **
 **      Return:    The size of the EMM message                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_send_attach_reject(const emm_as_establish_t *msg,
                           attach_reject_msg *emm_msg)
{
  LOG_FUNC_IN;

  int size = EMM_HEADER_MAXIMUM_LENGTH;

  LOG_TRACE(INFO, "EMMAS-SAP - Send Attach Reject message (cause=%d)",
            msg->emm_cause);

  /* Mandatory - Message type */
  emm_msg->messagetype = ATTACH_REJECT;

  /* Mandatory - EMM cause */
  size += EMM_CAUSE_MAXIMUM_LENGTH;
  emm_msg->emmcause = msg->emm_cause;

  /* Optional - ESM message container */
  if (msg->NASmsg.length > 0) {
    size += ESM_MESSAGE_CONTAINER_MINIMUM_LENGTH + msg->NASmsg.length;
    emm_msg->presencemask |= ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT;
    emm_msg->esmmessagecontainer.esmmessagecontainercontents = msg->NASmsg;
  }

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:        emm_send_tracking_area_update_reject()                    **
 **                                                                        **
 ** Description: Builds Tracking Area Update Reject message                **
 **                                                                        **
 **              The Tracking Area Update Reject message is sent by the    **
 **              network to the UE to indicate that the corresponding      **
 **              tracking area update has been rejected.                   **
 **                                                                        **
 ** Inputs:      msg:           The EMMAS-SAP primitive to process         **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     emm_msg:       The EMM message to be sent                 **
 **              Return:        The size of the EMM message                **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_send_tracking_area_update_reject(const emm_as_establish_t *msg,
    tracking_area_update_reject_msg *emm_msg)
{
  LOG_FUNC_IN;

  int size = EMM_HEADER_MAXIMUM_LENGTH;

  LOG_TRACE(INFO, "EMMAS-SAP - Send Tracking Area Update Reject message (cause=%d)",
            msg->emm_cause);

  /* Mandatory - Message type */
  emm_msg->messagetype = TRACKING_AREA_UPDATE_REJECT;

  /* Mandatory - EMM cause */
  size += EMM_CAUSE_MAXIMUM_LENGTH;
  emm_msg->emmcause = msg->emm_cause;

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_send_identity_request()                               **
 **                                                                        **
 ** Description: Builds Identity Request message                           **
 **                                                                        **
 **      The Identity Request message is sent by the network to    **
 **      the UE to request the UE to provide the specified identi- **
 **      ty.                                                       **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_msg:   The EMM message to be sent                 **
 **      Return:    The size of the EMM message                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_send_identity_request(const emm_as_security_t *msg,
                              identity_request_msg *emm_msg)
{
  LOG_FUNC_IN;

  int size = EMM_HEADER_MAXIMUM_LENGTH;

  LOG_TRACE(INFO, "EMMAS-SAP - Send Identity Request message");

  /* Mandatory - Message type */
  emm_msg->messagetype = IDENTITY_REQUEST;

  /* Mandatory - Identity type 2 */
  size += IDENTITY_TYPE_2_MAXIMUM_LENGTH;

  if (msg->identType == EMM_IDENT_TYPE_IMSI) {
    emm_msg->identitytype = IDENTITY_TYPE_2_IMSI;
  } else if (msg->identType == EMM_IDENT_TYPE_TMSI) {
    emm_msg->identitytype = IDENTITY_TYPE_2_TMSI;
  } else if (msg->identType == EMM_IDENT_TYPE_IMEI) {
    emm_msg->identitytype = IDENTITY_TYPE_2_IMEI;
  } else {
    /* All other values are interpreted as "IMSI" */
    emm_msg->identitytype = IDENTITY_TYPE_2_IMSI;
  }

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_send_authentication_request()                         **
 **                                                                        **
 ** Description: Builds Authentication Request message                     **
 **                                                                        **
 **      The Authentication Request message is sent by the network **
 **      to the UE to initiate authentication of the UE identity.  **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_msg:   The EMM message to be sent                 **
 **      Return:    The size of the EMM message                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_send_authentication_request(const emm_as_security_t *msg,
                                    authentication_request_msg *emm_msg)
{
  LOG_FUNC_IN;

  int size = EMM_HEADER_MAXIMUM_LENGTH;

  LOG_TRACE(INFO, "EMMAS-SAP - Send Authentication Request message");

  /* Mandatory - Message type */
  emm_msg->messagetype = AUTHENTICATION_REQUEST;

  /* Mandatory - NAS key set identifier */
  size += NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH;
  emm_msg->naskeysetidentifierasme.tsc = NAS_KEY_SET_IDENTIFIER_NATIVE;
  emm_msg->naskeysetidentifierasme.naskeysetidentifier = msg->ksi;

  /* Mandatory - Authentication parameter RAND */
  size += AUTHENTICATION_PARAMETER_RAND_MAXIMUM_LENGTH;
  emm_msg->authenticationparameterrand.rand = *msg->rand;

  /* Mandatory - Authentication parameter AUTN */
  size += AUTHENTICATION_PARAMETER_AUTN_MAXIMUM_LENGTH;
  emm_msg->authenticationparameterautn.autn = *msg->autn;

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_send_authentication_reject()                          **
 **                                                                        **
 ** Description: Builds Authentication Reject message                      **
 **                                                                        **
 **      The Authentication Reject message is sent by the network  **
 **      to the UE to indicate that the authentication procedure   **
 **      has failed and that the UE shall abort all activities.    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_msg:   The EMM message to be sent                 **
 **      Return:    The size of the EMM message                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_send_authentication_reject(authentication_reject_msg *emm_msg)
{
  LOG_FUNC_IN;

  int size = EMM_HEADER_MAXIMUM_LENGTH;

  LOG_TRACE(INFO, "EMMAS-SAP - Send Authentication Reject message");

  /* Mandatory - Message type */
  emm_msg->messagetype = AUTHENTICATION_REJECT;

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_send_security_mode_command()                          **
 **                                                                        **
 ** Description: Builds Security Mode Command message                      **
 **                                                                        **
 **      The Security Mode Command message is sent by the network  **
 **      to the UE to establish NAS signalling security.           **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_msg:   The EMM message to be sent                 **
 **      Return:    The size of the EMM message                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_send_security_mode_command(const emm_as_security_t *msg,
                                   security_mode_command_msg *emm_msg)
{
  LOG_FUNC_IN;

  int size = EMM_HEADER_MAXIMUM_LENGTH;

  LOG_TRACE(INFO, "EMMAS-SAP - Send Security Mode Command message");

  /* Mandatory - Message type */
  emm_msg->messagetype = SECURITY_MODE_COMMAND;

  /* Selected NAS security algorithms */
  size += NAS_SECURITY_ALGORITHMS_MAXIMUM_LENGTH;
  emm_msg->selectednassecurityalgorithms.typeofcipheringalgorithm =
    msg->selected_eea;
  emm_msg->selectednassecurityalgorithms.typeofintegrityalgorithm =
    msg->selected_eia;

  /* NAS key set identifier */
  size += NAS_KEY_SET_IDENTIFIER_MAXIMUM_LENGTH;
  emm_msg->naskeysetidentifier.tsc = NAS_KEY_SET_IDENTIFIER_NATIVE;
  emm_msg->naskeysetidentifier.naskeysetidentifier = msg->ksi;

  /* Replayed UE security capabilities */
  size += UE_SECURITY_CAPABILITY_MAXIMUM_LENGTH;
  emm_msg->replayeduesecuritycapabilities.eea = msg->eea;
  emm_msg->replayeduesecuritycapabilities.eia = msg->eia;
  emm_msg->replayeduesecuritycapabilities.umts_present = msg->umts_present;
  emm_msg->replayeduesecuritycapabilities.gprs_present = msg->gprs_present;

  emm_msg->replayeduesecuritycapabilities.uea = msg->uea;
  emm_msg->replayeduesecuritycapabilities.uia = msg->uia;
  emm_msg->replayeduesecuritycapabilities.gea = msg->gea;

  LOG_FUNC_RETURN (size);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
