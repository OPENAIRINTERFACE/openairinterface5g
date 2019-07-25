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

Source      emm_as.c

Version     0.1

Date        2012/10/16

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EMMAS Service Access Point that provides
        services to the EPS Mobility Management for NAS message
        transfer to/from the Access Stratum sublayer.

*****************************************************************************/

#include "emm_as.h"
#include "emm_recv.h"
#include "emm_send.h"
#include "emmData.h"
#include "commonDef.h"
#include "nas_log.h"

#include "TLVDecoder.h"
#include "as_message.h"
#include "nas_message.h"

#include "emm_cause.h"
#include "LowerLayer.h"

#include "emm_proc.h"

#include <string.h> // memset
#include <stdlib.h> // malloc, free

#if  defined(NAS_BUILT_IN_UE)
# include "nas_itti_messaging.h"
#endif
#include "msc.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * String representation of EMMAS-SAP primitives
 */

char *emmas_to_str(int emmas) {
static char *_emm_as_primitive_str[] = {
  "EMMAS_SECURITY_REQ",
  "EMMAS_SECURITY_IND",
  "EMMAS_SECURITY_RES",
  "EMMAS_SECURITY_REJ",
  "EMMAS_ESTABLISH_REQ",
  "EMMAS_ESTABLISH_CNF",
  "EMMAS_ESTABLISH_REJ",
  "EMMAS_RELEASE_REQ",
  "EMMAS_RELEASE_IND",
  "EMMAS_DATA_REQ",
  "EMMAS_DATA_IND",
  "EMMAS_PAGE_IND",
  "EMMAS_STATUS_IND",
  "EMMAS_CELL_INFO_REQ",
  "EMMAS_CELL_INFO_RES",
  "EMMAS_CELL_INFO_IND",
};
  return _emm_as_primitive_str[emmas];
}
/*
 * Functions executed to process EMM procedures upon receiving
 * data from the network
 */
static int _emm_as_recv(nas_user_t *user, const char *msg, int len,
                        int *emm_cause);

static int _emm_as_establish_cnf(nas_user_t *user, const emm_as_establish_t *msg, int *emm_cause);
static int _emm_as_establish_rej(nas_user_t *user);
static int _emm_as_release_ind(nas_user_t *user, const emm_as_release_t *msg);
static int _emm_as_page_ind(const emm_as_page_t *msg);


static int _emm_as_cell_info_res(nas_user_t *user, const emm_as_cell_info_t *msg);
static int _emm_as_cell_info_ind(const emm_as_cell_info_t *msg);

static int _emm_as_data_ind(nas_user_t *user, const emm_as_data_t *msg, int *emm_cause);

/*
 * Functions executed to send data to the network when requested
 * within EMM procedure processing
 */
static EMM_msg *_emm_as_set_header(nas_message_t *msg,
                                   const emm_as_security_data_t *security);
static int
_emm_as_encode(
  as_nas_info_t *info,
  nas_message_t *msg,
  int length,
  emm_security_context_t     *emm_security_context);

static int _emm_as_encrypt(
  as_nas_info_t *info,
  const nas_message_security_header_t *header,
  const char *buffer,
  int length,
  emm_security_context_t *emm_security_context);

static int _emm_as_send(const nas_user_t *user, const emm_as_t *msg);

static int _emm_as_security_res(const emm_data_t *emm_data, const emm_as_security_t *,
                                ul_info_transfer_req_t *);
static int _emm_as_establish_req(const emm_data_t *emm_data, const emm_as_establish_t *,
                                 nas_establish_req_t *);


static int _emm_as_cell_info_req(const emm_as_cell_info_t *, cell_info_req_t *);

static int _emm_as_data_req(const emm_data_t *emm_data, const emm_as_data_t *msg, ul_info_transfer_req_t *);
static int _emm_as_status_ind(const emm_data_t *emm_data, const emm_as_status_t *, ul_info_transfer_req_t *);
static int _emm_as_release_req(const emm_as_release_t *, nas_release_req_t *);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    emm_as_initialize()                                       **
 **                                                                        **
 ** Description: Initializes the EMMAS Service Access Point                **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    NONE                                       **
 **                                                                        **
 ***************************************************************************/
void emm_as_initialize(nas_user_t *user)
{
  LOG_FUNC_IN;

  /* TODO: Initialize the EMMAS-SAP */

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_as_send()                                             **
 **                                                                        **
 ** Description: Processes the EMMAS Service Access Point primitive.       **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int emm_as_send(nas_user_t *user, const emm_as_t *msg)
{
  LOG_FUNC_IN;

  int rc;
  int emm_cause = EMM_CAUSE_SUCCESS;
  emm_as_primitive_t primitive = msg->primitive;

  LOG_TRACE(INFO, "EMMAS-SAP - Received primitive %s (%d)",
            emmas_to_str(primitive - _EMMAS_START - 1), primitive);

  switch (primitive) {
  case _EMMAS_DATA_IND:
    rc = _emm_as_data_ind(user, &msg->u.data, &emm_cause);
    break;


  case _EMMAS_ESTABLISH_CNF:
    rc = _emm_as_establish_cnf(user, &msg->u.establish, &emm_cause);
    break;

  case _EMMAS_ESTABLISH_REJ:
    rc = _emm_as_establish_rej(user);
    break;

  case _EMMAS_RELEASE_IND:
    rc = _emm_as_release_ind(user, &msg->u.release);
    break;

  case _EMMAS_PAGE_IND:
    rc = _emm_as_page_ind(&msg->u.page);
    break;

  case _EMMAS_CELL_INFO_RES:
    rc = _emm_as_cell_info_res(user, &msg->u.cell_info);
    break;

  case _EMMAS_CELL_INFO_IND:
    rc = _emm_as_cell_info_ind(&msg->u.cell_info);
    break;

  default:
    /* Other primitives are forwarded to the Access Stratum */
    rc = _emm_as_send(user, msg);

    if (rc != RETURNok) {
      LOG_TRACE(ERROR, "EMMAS-SAP - "
                "Failed to process primitive %s (%d)",
                emmas_to_str(primitive - _EMMAS_START - 1),
                primitive);
      LOG_FUNC_RETURN (RETURNerror);
    }

    break;
  }

  /* Handle decoding errors */
  if (emm_cause != EMM_CAUSE_SUCCESS) {
    /* Ignore received message that is too short to contain a complete
     * message type information element */
    if (rc == TLV_DECODE_BUFFER_TOO_SHORT) {
      LOG_FUNC_RETURN (RETURNok);
    }
    /* Ignore received message that contains not supported protocol
     * discriminator */
    else if (rc == TLV_DECODE_PROTOCOL_NOT_SUPPORTED) {
      LOG_FUNC_RETURN (RETURNok);
    } else if (rc == TLV_DECODE_WRONG_MESSAGE_TYPE) {
      emm_cause = EMM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED;
    }

    /* EMM message processing failed */
    LOG_TRACE(WARNING, "EMMAS-SAP - Received EMM message is not valid "
              "(cause=%d)", emm_cause);
    /* Return an EMM status message */
    rc = emm_proc_status(user, emm_cause);
  }

  if (rc != RETURNok) {
    LOG_TRACE(ERROR, "EMMAS-SAP - Failed to process primitive %s (%d)",
              emmas_to_str(primitive - _EMMAS_START - 1),
              primitive);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 * Functions executed to process EMM procedures upon receiving data from the
 * network
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_recv()                                            **
 **                                                                        **
 ** Description: Decodes and processes the EPS Mobility Management message **
 **      received from the Access Stratum                          **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      msg:       The EMM message to process                 **
 **      len:       The length of the EMM message              **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_recv(nas_user_t *user, const char *msg, int len,
                        int *emm_cause)
{
  LOG_FUNC_IN;

  int decoder_rc;
  int rc = RETURNerror;

  LOG_TRACE(INFO, "EMMAS-SAP - Received EMM message (length=%d)", len);

  nas_message_t nas_msg;
  memset(&nas_msg, 0 , sizeof(nas_message_t));

  emm_security_context_t       *security = NULL;    /* Current EPS NAS security context     */

  security = user->emm_data->security;

  /* Decode the received message */
  decoder_rc = nas_message_decode(msg, &nas_msg, len, security);

  if (decoder_rc < 0) {
    LOG_TRACE(WARNING, "EMMAS-SAP - Failed to decode NAS message "
              "(err=%d)", decoder_rc);
    *emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
    LOG_FUNC_RETURN (decoder_rc);
  }

  /* Process NAS message */
  EMM_msg *emm_msg = &nas_msg.plain.emm;

  switch (emm_msg->header.message_type) {
  case EMM_STATUS:
    rc = emm_recv_status(user->ueid, &emm_msg->emm_status, emm_cause);
    break;

  case IDENTITY_REQUEST:
    rc = emm_recv_identity_request(user, &emm_msg->identity_request,
                                   emm_cause);
    break;

  case AUTHENTICATION_REQUEST:
    rc = emm_recv_authentication_request(user,
           &emm_msg->authentication_request,
           emm_cause);
    break;

  case AUTHENTICATION_REJECT:
    rc = emm_recv_authentication_reject(user,
           &emm_msg->authentication_reject,
           emm_cause);
    break;

  case SECURITY_MODE_COMMAND:
    rc = emm_recv_security_mode_command(user,
           &emm_msg->security_mode_command,
           emm_cause);
    break;

  case DETACH_ACCEPT:
    rc = emm_recv_detach_accept(user, &emm_msg->detach_accept, emm_cause);
    break;


  case TRACKING_AREA_UPDATE_ACCEPT:
  case TRACKING_AREA_UPDATE_REJECT:
  case SERVICE_REJECT:
  case GUTI_REALLOCATION_COMMAND:
  case EMM_INFORMATION:
  case DOWNLINK_NAS_TRANSPORT:
  case CS_SERVICE_NOTIFICATION:
    /* TODO */
    break;

  default:
    LOG_TRACE(WARNING, "EMMAS-SAP - EMM message 0x%x is not valid",
              emm_msg->header.message_type);
    *emm_cause = EMM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE;
    break;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_data_ind()                                        **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP data transfer indication          **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - AS->EMM: DATA_IND - Data transfer procedure                **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_data_ind(nas_user_t *user, const emm_as_data_t *msg, int *emm_cause)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  LOG_TRACE(INFO, "EMMAS-SAP - Received AS data transfer indication "
            "(ueid=%u, delivered=%s, length=%d)", msg->ueid,
            (msg->delivered)? "TRUE" : "FALSE", msg->NASmsg.length);

  if (msg->delivered) {
    if (msg->NASmsg.length > 0) {
      /* Process the received NAS message */
      char *plain_msg = (char *)malloc(msg->NASmsg.length);

      if (plain_msg) {
        nas_message_security_header_t header;
        emm_security_context_t       *security = NULL;    /* Current EPS NAS security context     */

        memset(&header, 0, sizeof(header));
        /* Decrypt the received security protected message */

        security = user->emm_data->security;
        int bytes = nas_message_decrypt((char *)(msg->NASmsg.value),
                                        plain_msg,
                                        &header,
                                        msg->NASmsg.length,
                                        security
                                       );

        if (bytes < 0) {
          /* Failed to decrypt the message */
          *emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
          free(plain_msg);
          LOG_FUNC_RETURN (bytes);
        } else if (header.protocol_discriminator ==
                   EPS_MOBILITY_MANAGEMENT_MESSAGE) {
          /* Process EMM data */
          rc = _emm_as_recv(user, plain_msg, bytes, emm_cause);
        } else if (header.protocol_discriminator ==
                   EPS_SESSION_MANAGEMENT_MESSAGE) {
          const OctetString data = {bytes, (uint8_t *)plain_msg};
          /* Foward ESM data to EPS session management */
          rc = lowerlayer_data_ind(user, &data);
        }

        free(plain_msg);
      }
    } else {
      /* Process successfull lower layer transfer indication */
      rc = lowerlayer_success(user);
    }
  } else {
    /* Process lower layer transmission failure of NAS message */
    rc = lowerlayer_failure(user);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_establish_cnf()                                   **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP connection establish confirmation **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - AS->EMM: ESTABLISH_CNF - NAS signalling connection         **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_establish_cnf(nas_user_t *user, const emm_as_establish_t *msg,
                                 int *emm_cause)
{
  LOG_FUNC_IN;

  int decoder_rc;
  int rc = RETURNerror;

  LOG_TRACE(INFO, "EMMAS-SAP - Received AS connection establish confirm");

  if (msg->NASmsg.length > 0) {
    /* The NAS signalling connection is established */
    (void) lowerlayer_establish(user);
  } else {
    /* The initial NAS message has been successfully delivered to
     * lower layers */
    rc = lowerlayer_success(user);
    LOG_FUNC_RETURN (rc);
  }

  nas_message_t nas_msg;
  memset(&nas_msg, 0 , sizeof(nas_message_t));

  /* Decode initial NAS message */
  decoder_rc = nas_message_decode((char *)(msg->NASmsg.value),
                                  &nas_msg,
                                  msg->NASmsg.length,
                                  user->emm_data->security);

  if (decoder_rc < 0) {
    LOG_TRACE(WARNING, "EMMAS-SAP - Failed to decode initial NAS message"
              "(err=%d)", decoder_rc);
    *emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
    LOG_FUNC_RETURN (decoder_rc);
  }

  /* Process initial NAS message */
  EMM_msg *emm_msg = &nas_msg.plain.emm;

  switch (emm_msg->header.message_type) {
  case ATTACH_ACCEPT:
    rc = emm_recv_attach_accept(user, &emm_msg->attach_accept, emm_cause);
    break;

  case ATTACH_REJECT:
    rc = emm_recv_attach_reject(user, &emm_msg->attach_reject, emm_cause);
    break;

  case DETACH_ACCEPT:
    break;

  case TRACKING_AREA_UPDATE_ACCEPT:
    break;

  default:
    LOG_TRACE(WARNING, "EMMAS-SAP - Initial NAS message 0x%x is "
              "not valid", emm_msg->header.message_type);
    *emm_cause = EMM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE;
    break;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_establish_rej()                                   **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP connection establish reject       **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - AS->EMM: ESTABLISH_REJ - NAS signalling connection         **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_establish_rej(nas_user_t *user)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "EMMAS-SAP - Received AS initial NAS message transmission "
            "failure");

  /* Process lower layer transmission failure of initial NAS message */
  rc = lowerlayer_failure(user);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_release_ind()                                     **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP connection release indication     **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - AS->EMM: RELEASE_IND - NAS signalling release procedure    **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_release_ind(nas_user_t *user, const emm_as_release_t *msg)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO, "EMMAS-SAP - Received AS connection release indication "
            "(cause=%d)", msg->cause);

  /* Process NAS signalling connection release indication */
  rc = lowerlayer_release(user, msg->cause);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_page_ind()                                        **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP paging data indication primitive  **
 **                                                                        **
 ** EMMAS-SAP - AS->EMM: PAGE_IND - Paging data procedure                  **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_page_ind(const emm_as_page_t *msg)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO, "EMMAS-SAP - Received AS paging data indication");

  /* TODO */

  LOG_FUNC_RETURN (rc);
}




/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_cell_info_res()                                   **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP cell information response         **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - AS->EMM: CELL_INFO_RES - PLMN and cell selection procedure **
 **     The NAS received a response to cell selection request pre- **
 **     viously sent to the Access-Startum. If a suitable cell is  **
 **     found to serve the selected PLMN with associated Radio Ac- **
 **     cess Technologies, this cell is selected to camp on.       **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_cell_info_res(nas_user_t *user, const emm_as_cell_info_t *msg)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO, "EMMAS-SAP - Received AS cell information response");

  int AcT = NET_ACCESS_EUTRAN;

  if (msg->found == TRUE) {
    /* Get the first supported access technology */
    while (AcT != NET_ACCESS_UNAVAILABLE) {
      if (msg->rat & (1 << AcT)) {
        break;
      }

      AcT -= 1;
    }
  }

  /* Notify EMM that a cell has been found */
  rc = emm_proc_plmn_selection_end(user, msg->found, msg->tac, msg->cellID, AcT);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_cell_info_ind()                                   **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP cell information indication       **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - AS->EMM: CELL_INFO_IND - PLMN and cell selection procedure **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_cell_info_ind(const emm_as_cell_info_t *msg)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO, "EMMAS-SAP - Received AS cell information indication");

  /* TODO */

  LOG_FUNC_RETURN (rc);
}

/*
 * --------------------------------------------------------------------------
 * Functions executed to send data to the network when requested within EMM
 * procedure processing
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_set_header()                                      **
 **                                                                        **
 ** Description: Setup the security header of the given NAS message        **
 **                                                                        **
 ** Inputs:  security:  The NAS security data to use               **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     msg:       The NAS message                            **
 **      Return:    Pointer to the plain NAS message to be se- **
 **             curity protected if setting of the securi- **
 **             ty header succeed;                         **
 **             NULL pointer otherwise                     **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static EMM_msg *_emm_as_set_header(nas_message_t *msg,
                                   const emm_as_security_data_t *security)
{
  LOG_FUNC_IN;

  msg->header.protocol_discriminator = EPS_MOBILITY_MANAGEMENT_MESSAGE;

  if ( security && (security->ksi != EMM_AS_NO_KEY_AVAILABLE) ) {
    /* A valid EPS security context exists */
    if (security->is_new) {
      /* New EPS security context is taken into use */
      if (security->k_int) {
        if (security->k_enc) {
          /* NAS integrity and cyphering keys are available */
          msg->header.security_header_type =
            SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_CYPHERED_NEW;
        } else {
          /* NAS integrity key only is available */
          msg->header.security_header_type =
            SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_NEW;
        }

        LOG_FUNC_RETURN (&msg->security_protected.plain.emm);
      }
    } else if (security->k_int) {
      if (security->k_enc) {
        /* NAS integrity and cyphering keys are available */
        msg->header.security_header_type =
          SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_CYPHERED;
      } else {
        /* NAS integrity key only is available */
        msg->header.security_header_type =
          SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED;
      }

      LOG_FUNC_RETURN (&msg->security_protected.plain.emm);
    } else {
      /* No valid EPS security context exists */
      msg->header.security_header_type = SECURITY_HEADER_TYPE_NOT_PROTECTED;
      LOG_FUNC_RETURN (&msg->plain.emm);
    }
  } else {
    /* No valid EPS security context exists */
    msg->header.security_header_type = SECURITY_HEADER_TYPE_NOT_PROTECTED;
    LOG_FUNC_RETURN (&msg->plain.emm);
  }

  /* A valid EPS security context exists but NAS integrity key
   * is not available */
  LOG_FUNC_RETURN (NULL);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_encode()                                          **
 **                                                                        **
 ** Description: Encodes NAS message into NAS information container        **
 **                                                                        **
 ** Inputs:  msg:       The NAS message to encode                  **
 **      length:    The maximum length of the NAS message      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     info:      The NAS information container              **
 **      msg:       The NAS message to encode                  **
 **      Return:    The number of bytes successfully encoded   **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int
_emm_as_encode(
  as_nas_info_t              *info,
  nas_message_t              *msg,
  int                         length,
  emm_security_context_t     *emm_security_context)
{
  LOG_FUNC_IN;

  int bytes = 0;

  if (msg->header.security_header_type != SECURITY_HEADER_TYPE_NOT_PROTECTED) {
    emm_msg_header_t *header = &msg->security_protected.plain.emm.header;
    /* Expand size of protected NAS message */
    length += NAS_MESSAGE_SECURITY_HEADER_SIZE;
    /* Set header of plain NAS message */
    header->protocol_discriminator = EPS_MOBILITY_MANAGEMENT_MESSAGE;
    header->security_header_type = SECURITY_HEADER_TYPE_NOT_PROTECTED;
  }

  /* Allocate memory to the NAS information container */
  info->data = (Byte_t *)malloc(length * sizeof(Byte_t));

  if (info->data != NULL) {
    /* Encode the NAS message */
    bytes = nas_message_encode(
              (char *)(info->data),
              msg,
              length,
              emm_security_context);

    if (bytes > 0) {
      info->length = bytes;
    } else {
      free(info->data);
      info->length = 0;
      info->data = NULL;
    }
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_encrypt()                                         **
 **                                                                        **
 ** Description: Encryts NAS message into NAS information container        **
 **                                                                        **
 ** Inputs:  header:    The Security header in used                **
 **      msg:       The NAS message to encrypt                 **
 **      length:    The maximum length of the NAS message      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     info:      The NAS information container              **
 **      Return:    The number of bytes successfully encrypted **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int
_emm_as_encrypt(
  as_nas_info_t *info,
  const nas_message_security_header_t *header,
  const char *msg,
  int length,
  emm_security_context_t *emm_security_context)
{
  LOG_FUNC_IN;

  int bytes = 0;

  if (header->security_header_type != SECURITY_HEADER_TYPE_NOT_PROTECTED) {
    /* Expand size of protected NAS message */
    length += NAS_MESSAGE_SECURITY_HEADER_SIZE;
  }

  /* Allocate memory to the NAS information container */
  info->data = (Byte_t *)malloc(length * sizeof(Byte_t));

  if (info->data != NULL) {
    /* Encrypt the NAS information message */
    bytes = nas_message_encrypt(
              msg,
              (char *)(info->data),
              header,
              length,
              emm_security_context);

    if (bytes > 0) {
      info->length = bytes;
    } else {
      free(info->data);
      info->length = 0;
      info->data = NULL;
    }
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_send()                                            **
 **                                                                        **
 ** Description: Builds NAS message according to the given EMMAS Service   **
 **      Access Point primitive and sends it to the Access Stratum **
 **      sublayer                                                  **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to be sent         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_send(const nas_user_t *user, const emm_as_t *msg)
{
  LOG_FUNC_IN;

  as_message_t as_msg;
  memset(&as_msg, 0 , sizeof(as_message_t));

  switch (msg->primitive) {
  case _EMMAS_DATA_REQ:
    as_msg.msgID = _emm_as_data_req(user->emm_data,
                     &msg->u.data,
                     &as_msg.msg.ul_info_transfer_req);
    break;

  case _EMMAS_STATUS_IND:
    as_msg.msgID = _emm_as_status_ind(user->emm_data,
                     &msg->u.status,
                     &as_msg.msg.ul_info_transfer_req);
    break;

  case _EMMAS_RELEASE_REQ:
    as_msg.msgID = _emm_as_release_req(
                     &msg->u.release,
                     &as_msg.msg.nas_release_req);
    break;


  case _EMMAS_SECURITY_RES:
    as_msg.msgID = _emm_as_security_res(user->emm_data,
                     &msg->u.security,
                     &as_msg.msg.ul_info_transfer_req);
    break;

  case _EMMAS_ESTABLISH_REQ:
    as_msg.msgID = _emm_as_establish_req(user->emm_data,
                     &msg->u.establish,
                     &as_msg.msg.nas_establish_req);
    break;


  case _EMMAS_CELL_INFO_REQ:
    as_msg.msgID = _emm_as_cell_info_req(&msg->u.cell_info,
                                         &as_msg.msg.cell_info_req);
    /*
     * TODO: NAS may provide a list of equivalent PLMNs, if available,
     * that AS shall use for cell selection and cell reselection.
     */
    break;

  default:
    as_msg.msgID = 0;
    break;
  }

  /* Send the message to the Access Stratum or S1AP in case of MME */
  if (as_msg.msgID > 0) {

# if defined(NAS_BUILT_IN_UE)
    LOG_TRACE(DEBUG, "EMMAS-SAP - "
              "Sending msg with id 0x%x, primitive %s (%d) to RRC layer for transmission",
              as_msg.msgID,
              emmas_to_str(msg->primitive - _EMMAS_START - 1),
              msg->primitive);

    switch (as_msg.msgID) {
    case AS_CELL_INFO_REQ: {
      nas_itti_cell_info_req(
        as_msg.msg.cell_info_req.plmnID,
        as_msg.msg.cell_info_req.rat,
        user->ueid);
      LOG_FUNC_RETURN (RETURNok);
    }
    break;

    case AS_NAS_ESTABLISH_REQ: {
      nas_itti_nas_establish_req(
        as_msg.msg.nas_establish_req.cause,
        as_msg.msg.nas_establish_req.type,
        as_msg.msg.nas_establish_req.s_tmsi,
        as_msg.msg.nas_establish_req.plmnID,
        as_msg.msg.nas_establish_req.initialNasMsg.data,
        as_msg.msg.nas_establish_req.initialNasMsg.length,
        user->ueid);
      LOG_FUNC_RETURN (RETURNok);
    }
    break;

    case AS_UL_INFO_TRANSFER_REQ: {
      nas_itti_ul_data_req(
        as_msg.msg.ul_info_transfer_req.UEid,
        as_msg.msg.ul_info_transfer_req.nasMsg.data,
        as_msg.msg.ul_info_transfer_req.nasMsg.length,
        user->ueid);
      LOG_FUNC_RETURN (RETURNok);
    }
    break;

    case AS_RAB_ESTABLISH_RSP: {
      nas_itti_rab_establish_rsp(
        as_msg.msg.rab_establish_rsp.s_tmsi,
        as_msg.msg.rab_establish_rsp.rabID,
        as_msg.msg.rab_establish_rsp.errCode,
        user->ueid);
      LOG_FUNC_RETURN (RETURNok);
    }
    break;

    default:
      break;
    }

# else
    int bytes = as_message_send(&as_msg);

    if (bytes > 0) {
      LOG_FUNC_RETURN (RETURNok);
    }

# endif
  }

  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_data_req()                                        **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP data transfer request             **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: DATA_REQ - Data transfer procedure                **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_data_req(const emm_data_t *emm_data, const emm_as_data_t *msg,
                            ul_info_transfer_req_t *as_msg)
{
  LOG_FUNC_IN;

  int size = 0;
  int is_encoded = FALSE;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS data transfer request");

  nas_message_t nas_msg;
  memset(&nas_msg, 0 , sizeof(nas_message_t));

  /* Setup the AS message */
  if (msg->guti) {
    as_msg->s_tmsi.MMEcode = msg->guti->gummei.MMEcode;
    as_msg->s_tmsi.m_tmsi = msg->guti->m_tmsi;
  } else {
    as_msg->UEid = msg->ueid;
  }

  /* Setup the NAS security header */
  EMM_msg *emm_msg = _emm_as_set_header(&nas_msg, &msg->sctx);

  /* Setup the NAS information message */
  if (emm_msg != NULL) switch (msg->NASinfo) {

    case EMM_AS_NAS_DATA_ATTACH:
      size = emm_send_attach_complete(msg, &emm_msg->attach_complete);
      break;

    case EMM_AS_NAS_DATA_DETACH:
      size = emm_send_detach_request(msg, &emm_msg->detach_request);
      break;


    default:
      /* Send other NAS messages as already encoded ESM messages */
      size = msg->NASmsg.length;
      is_encoded = TRUE;
      break;
    }

  if (size > 0) {
    int bytes;
    emm_security_context_t    *emm_security_context   = NULL;

    emm_security_context = emm_data->security;

    if (emm_security_context) {

      nas_msg.header.sequence_number = emm_security_context->ul_count.seq_num;
      LOG_TRACE(DEBUG,
                "Set nas_msg.header.sequence_number -> %u",
                nas_msg.header.sequence_number);
    }

    if (!is_encoded) {
      /* Encode the NAS information message */
      bytes = _emm_as_encode(&as_msg->nasMsg,
                             &nas_msg,
                             size,
                             emm_security_context);
    } else {
      /* Encrypt the NAS information message */
      bytes = _emm_as_encrypt(&as_msg->nasMsg,
                              &nas_msg.header,
                              (char *)(msg->NASmsg.value),
                              size,
                              emm_security_context);
    }

    if (bytes > 0) {
      LOG_FUNC_RETURN (AS_UL_INFO_TRANSFER_REQ);
    }
  }

  LOG_FUNC_RETURN (0);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_status_ind()                                      **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP status indication primitive       **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: STATUS_IND - EMM status report procedure          **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_status_ind(const emm_data_t *emm_data, const emm_as_status_t *msg,
                              ul_info_transfer_req_t *as_msg)
{
  LOG_FUNC_IN;

  int size = 0;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS status indication (cause=%d)",
            msg->emm_cause);

  nas_message_t nas_msg;
  memset(&nas_msg, 0 , sizeof(nas_message_t));

  /* Setup the AS message */
  if (msg->guti) {
    as_msg->s_tmsi.MMEcode = msg->guti->gummei.MMEcode;
    as_msg->s_tmsi.m_tmsi = msg->guti->m_tmsi;
  } else {
    as_msg->UEid = msg->ueid;
  }

  /* Setup the NAS security header */
  EMM_msg *emm_msg = _emm_as_set_header(&nas_msg, &msg->sctx);

  /* Setup the NAS information message */
  if (emm_msg != NULL) {
    size = emm_send_status(msg, &emm_msg->emm_status);
  }

  if (size > 0) {
    emm_security_context_t    *emm_security_context   = NULL;

    emm_security_context = emm_data->security;

    if (emm_security_context) {

      nas_msg.header.sequence_number = emm_security_context->ul_count.seq_num;
      LOG_TRACE(DEBUG,
                "Set nas_msg.header.sequence_number -> %u",
                nas_msg.header.sequence_number);
    }

    /* Encode the NAS information message */
    int bytes = _emm_as_encode(
                  &as_msg->nasMsg,
                  &nas_msg,
                  size,
                  emm_security_context);

    if (bytes > 0) {
      LOG_FUNC_RETURN (AS_UL_INFO_TRANSFER_REQ);
    }
  }

  LOG_FUNC_RETURN (0);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_release_req()                                     **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP connection release request        **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: RELEASE_REQ - NAS signalling release procedure    **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_release_req(const emm_as_release_t *msg,
                               nas_release_req_t *as_msg)
{
  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS release request");

  /* Setup the AS message */
  if (msg->guti) {
    as_msg->s_tmsi.MMEcode = msg->guti->gummei.MMEcode;
    as_msg->s_tmsi.m_tmsi = msg->guti->m_tmsi;
  } else {
    as_msg->UEid = msg->ueid;
  }

  if (msg->cause == EMM_AS_CAUSE_AUTHENTICATION) {
    as_msg->cause = AS_AUTHENTICATION_FAILURE;
  } else if (msg->cause == EMM_AS_CAUSE_DETACH) {
    as_msg->cause = AS_DETACH;
  }

  LOG_FUNC_RETURN (AS_NAS_RELEASE_REQ);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_security_res()                                    **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP security response primitive       **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: SECURITY_RES - Security mode control procedure    **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_security_res(const emm_data_t *emm_data, const emm_as_security_t *msg,
                                ul_info_transfer_req_t *as_msg)
{
  LOG_FUNC_IN;

  int size = 0;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS security response");

  nas_message_t nas_msg;
  memset(&nas_msg, 0 , sizeof(nas_message_t));

  /* Setup the AS message */
  if (msg->guti) {
    as_msg->s_tmsi.MMEcode = msg->guti->gummei.MMEcode;
    as_msg->s_tmsi.m_tmsi = msg->guti->m_tmsi;
  }

  /* Setup the NAS security header */
  EMM_msg *emm_msg = _emm_as_set_header(&nas_msg, &msg->sctx);

  /* Setup the NAS security message */
  if (emm_msg != NULL) switch (msg->msgType) {
    case EMM_AS_MSG_TYPE_IDENT:
      size = emm_send_identity_response(
               msg,
               &emm_msg->identity_response);
      break;

    case EMM_AS_MSG_TYPE_AUTH:
      if (msg->emm_cause != EMM_CAUSE_SUCCESS) {
        size = emm_send_authentication_failure(
                 msg,
                 &emm_msg->authentication_failure);
      } else {
        size = emm_send_authentication_response(
                 msg,
                 &emm_msg->authentication_response);
      }

      break;

    case EMM_AS_MSG_TYPE_SMC:
      if (msg->emm_cause != EMM_CAUSE_SUCCESS) {
        size = emm_send_security_mode_reject(
                 msg,
                 &emm_msg->security_mode_reject);
      } else {
        size = emm_send_security_mode_complete(
                 msg,
                 &emm_msg->security_mode_complete);
      }

      break;

    default:
      LOG_TRACE(WARNING, "EMMAS-SAP - Type of NAS security "
                "message 0x%.2x is not valid", msg->msgType);
    }

  if (size > 0) {
    /* Encode the NAS security message */
    int bytes = _emm_as_encode(&as_msg->nasMsg,
                               &nas_msg,
                               size,
                               emm_data->security);

    if (bytes > 0) {
      LOG_FUNC_RETURN (AS_UL_INFO_TRANSFER_REQ);
    }
  }

  LOG_FUNC_RETURN (0);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_establish_req()                                   **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP connection establish request      **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: ESTABLISH_REQ - NAS signalling connection         **
 **     The NAS requests the AS to establish signalling connection **
 **     to tranfer initial NAS message to the network.             **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_establish_req(const emm_data_t *emm_data, const emm_as_establish_t *msg,
                                 nas_establish_req_t *as_msg)
{
  LOG_FUNC_IN;

  int size = 0;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS connection establish request");

  nas_message_t nas_msg;
  memset(&nas_msg, 0 , sizeof(nas_message_t));

  /* Setup the AS message */
  as_msg->cause = msg->RRCcause;
  as_msg->type = msg->RRCtype;
  as_msg->plmnID = *msg->plmnID;

  /* Derive the S-TMSI from the GUTI, if valid */
  if (msg->UEid.guti) {
    as_msg->s_tmsi.MMEcode = msg->UEid.guti->gummei.MMEcode;
    as_msg->s_tmsi.m_tmsi = msg->UEid.guti->m_tmsi;
  }

  /* Setup the NAS security header */
  EMM_msg *emm_msg = _emm_as_set_header(&nas_msg, &msg->sctx);

  /* Setup the initial NAS information message */
  if (emm_msg != NULL) switch (msg->NASinfo) {
    case EMM_AS_NAS_INFO_ATTACH:
      size = emm_send_attach_request(msg, &emm_msg->attach_request);
      break;

    case EMM_AS_NAS_INFO_DETACH:
      size = emm_send_initial_detach_request(msg,
                                             &emm_msg->detach_request);
      break;

    case EMM_AS_NAS_INFO_TAU:
      size = emm_send_initial_tau_request(msg,
                                          &emm_msg->tracking_area_update_request);
      break;

    case EMM_AS_NAS_INFO_SR:
      size = emm_send_initial_sr_request(msg,
                                         &emm_msg->service_request);
      break;

    case EMM_AS_NAS_INFO_EXTSR:
      size = emm_send_initial_extsr_request(msg,
                                            &emm_msg->extended_service_request);
      break;

    default:
      LOG_TRACE(WARNING, "EMMAS-SAP - Type of initial NAS "
                "message 0x%.2x is not valid", msg->NASinfo);
      break;
    }

  if (size > 0) {
    /* Encode the initial NAS information message */
    int bytes = _emm_as_encode(
                  &as_msg->initialNasMsg,
                  &nas_msg,
                  size,
                  emm_data->security);

    if (bytes > 0) {
      LOG_FUNC_RETURN (AS_NAS_ESTABLISH_REQ);
    }
  }

  LOG_FUNC_RETURN (0);
}



/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_cell_info_req()                                   **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP cell information request          **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: CELL_INFO_REQ - PLMN and cell selection procedure **
 **     The NAS requests the AS to select a cell belonging to the  **
 **     selected PLMN with associated Radio Access Technologies.   **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_cell_info_req(const emm_as_cell_info_t *msg,
                                 cell_info_req_t *as_msg)
{
  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS cell information request");

  as_msg->plmnID = msg->plmnIDs.plmn[0];
  as_msg->rat = msg->rat;

  LOG_FUNC_RETURN (AS_CELL_INFO_REQ);
}

