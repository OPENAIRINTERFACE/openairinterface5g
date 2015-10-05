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

#include <string.h> // memset
#include <stdlib.h> // malloc, free

#if defined(NAS_BUILT_IN_EPC)
# include "nas_itti_messaging.h"
#endif
#include "msc.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/


extern int emm_proc_status(unsigned int ueid, int emm_cause);

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * String representation of EMMAS-SAP primitives
 */
static const char *_emm_as_primitive_str[] = {
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

/*
 * Functions executed to process EMM procedures upon receiving
 * data from the network
 */
static int _emm_as_recv(unsigned int ueid, const char *msg, int len,
                        int *emm_cause);


static int _emm_as_establish_req(const emm_as_establish_t *msg, int *emm_cause);
static int _emm_as_cell_info_res(const emm_as_cell_info_t *msg);
static int _emm_as_cell_info_ind(const emm_as_cell_info_t *msg);
static int _emm_as_data_ind(const emm_as_data_t *msg, int *emm_cause);

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

static int _emm_as_send(const emm_as_t *msg);

static int _emm_as_security_req(const emm_as_security_t *,
                                dl_info_transfer_req_t *);
static int _emm_as_security_rej(const emm_as_security_t *,
                                dl_info_transfer_req_t *);
static int _emm_as_establish_cnf(const emm_as_establish_t *,
                                 nas_establish_rsp_t *);
static int _emm_as_establish_rej(const emm_as_establish_t *,
                                 nas_establish_rsp_t *);
static int _emm_as_page_ind(const emm_as_page_t *, paging_req_t *);

static int _emm_as_cell_info_req(const emm_as_cell_info_t *, cell_info_req_t *);

static int _emm_as_data_req(const emm_as_data_t *, ul_info_transfer_req_t *);
static int _emm_as_status_ind(const emm_as_status_t *, ul_info_transfer_req_t *);
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
void emm_as_initialize(void)
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
int emm_as_send(const emm_as_t *msg)
{
  LOG_FUNC_IN;

  int rc;
  int emm_cause = EMM_CAUSE_SUCCESS;
  emm_as_primitive_t primitive = msg->primitive;

  uint32_t ueid = 0;

  LOG_TRACE(INFO, "EMMAS-SAP - Received primitive %s (%d)",
            _emm_as_primitive_str[primitive - _EMMAS_START - 1], primitive);

  switch (primitive) {
  case _EMMAS_DATA_IND:
    rc = _emm_as_data_ind(&msg->u.data, &emm_cause);
    ueid = msg->u.data.ueid;
    break;

  case _EMMAS_ESTABLISH_REQ:
    rc = _emm_as_establish_req(&msg->u.establish, &emm_cause);
    ueid = msg->u.establish.ueid;
    break;

  case _EMMAS_CELL_INFO_RES:
    rc = _emm_as_cell_info_res(&msg->u.cell_info);
    break;

  case _EMMAS_CELL_INFO_IND:
    rc = _emm_as_cell_info_ind(&msg->u.cell_info);
    break;

  default:
    /* Other primitives are forwarded to the Access Stratum */
    rc = _emm_as_send(msg);

    if (rc != RETURNok) {
      LOG_TRACE(ERROR, "EMMAS-SAP - "
                "Failed to process primitive %s (%d)",
                _emm_as_primitive_str[primitive - _EMMAS_START - 1],
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
    rc = emm_proc_status(ueid, emm_cause);
  }

  if (rc != RETURNok) {
    LOG_TRACE(ERROR, "EMMAS-SAP - Failed to process primitive %s (%d)",
              _emm_as_primitive_str[primitive - _EMMAS_START - 1],
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
static int _emm_as_recv(unsigned int ueid, const char *msg, int len,
                        int *emm_cause)
{
  LOG_FUNC_IN;

  int decoder_rc;
  int rc = RETURNerror;

  LOG_TRACE(INFO, "EMMAS-SAP - Received EMM message (length=%d)", len);

  nas_message_t nas_msg;
  memset(&nas_msg, 0 , sizeof(nas_message_t));

  emm_security_context_t       *security = NULL;    /* Current EPS NAS security context     */
#if defined(NAS_BUILT_IN_EPC)
  emm_data_context_t           *emm_ctx  = NULL;
#endif

#if defined(NAS_BUILT_IN_EPC)
  emm_ctx = emm_data_context_get(&_emm_data, ueid);

  if (emm_ctx) {
    security = emm_ctx->security;
  }

#else

  if (ueid < EMM_DATA_NB_UE_MAX) {
    emm_ctx = _emm_data.ctx[ueid];

    if (emm_ctx) {
      security = emm_ctx->security;
    }
  }

#endif

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
    rc = emm_recv_status(ueid, &emm_msg->emm_status, emm_cause);
    break;

  case ATTACH_REQUEST:
    rc = emm_recv_attach_request(
           ueid,
           &emm_msg->attach_request,
           emm_cause);
    break;

  case IDENTITY_RESPONSE:
    rc = emm_recv_identity_response(
           ueid,
           &emm_msg->identity_response,
           emm_cause);
    break;

  case AUTHENTICATION_RESPONSE:
    rc = emm_recv_authentication_response(
           ueid,
           &emm_msg->authentication_response,
           emm_cause);
    break;

  case AUTHENTICATION_FAILURE:
    rc = emm_recv_authentication_failure(
           ueid,
           &emm_msg->authentication_failure,
           emm_cause);
    break;

  case SECURITY_MODE_COMPLETE:
    rc = emm_recv_security_mode_complete(
           ueid,
           &emm_msg->security_mode_complete,
           emm_cause);
    break;

  case SECURITY_MODE_REJECT:
    rc = emm_recv_security_mode_reject(ueid,
                                       &emm_msg->security_mode_reject,
                                       emm_cause);
    break;

  case ATTACH_COMPLETE:
    rc = emm_recv_attach_complete(ueid, &emm_msg->attach_complete,
                                  emm_cause);
    break;

  case TRACKING_AREA_UPDATE_COMPLETE:
  case GUTI_REALLOCATION_COMPLETE:
  case UPLINK_NAS_TRANSPORT:
    /* TODO */
    break;

  case DETACH_REQUEST:
    rc = emm_recv_detach_request(ueid, &emm_msg->detach_request,
                                 emm_cause);
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
static int _emm_as_data_ind(const emm_as_data_t *msg, int *emm_cause)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  LOG_TRACE(INFO, "EMMAS-SAP - Received AS data transfer indication "
            "(ueid="NAS_UE_ID_FMT", delivered=%s, length=%d)", msg->ueid,
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
        emm_data_context_t           *emm_ctx  = NULL;
#if defined(NAS_BUILT_IN_EPC)

        if (msg->ueid > 0) {
          emm_ctx = emm_data_context_get(&_emm_data, msg->ueid);

          if (emm_ctx) {
            security = emm_ctx->security;
          }
        }

#else

        if (msg->ueid < EMM_DATA_NB_UE_MAX) {
          emm_ctx = _emm_data.ctx[msg->ueid];

          if (emm_ctx) {
            security = emm_ctx->security;
          }
        }

#endif
        int bytes = nas_message_decrypt((char *)(msg->NASmsg.value),
                                        plain_msg,
                                        &header,
                                        msg->NASmsg.length,
                                        security
                                       );

        if (bytes < 0) {
          /* Failed to decrypt the message */
          *emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
          LOG_FUNC_RETURN (bytes);
        } else if (header.protocol_discriminator ==
                   EPS_MOBILITY_MANAGEMENT_MESSAGE) {
          /* Process EMM data */
          rc = _emm_as_recv(msg->ueid, plain_msg, bytes, emm_cause);
        } else if (header.protocol_discriminator ==
                   EPS_SESSION_MANAGEMENT_MESSAGE) {
          const OctetString data = {bytes, (uint8_t *)plain_msg};
          /* Foward ESM data to EPS session management */
          rc = lowerlayer_data_ind(msg->ueid, &data);
        }

        free(plain_msg);
      }
    } else {
      /* Process successfull lower layer transfer indication */
      rc = lowerlayer_success(msg->ueid);
    }
  } else {
    /* Process lower layer transmission failure of NAS message */
    rc = lowerlayer_failure(msg->ueid);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_establish_req()                                   **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP connection establish request      **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - AS->EMM: ESTABLISH_REQ - NAS signalling connection         **
 **     The AS notifies the NAS that establishment of the signal-  **
 **     ling connection has been requested to tranfer initial NAS  **
 **     message from the UE.                                       **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     emm_cause: EMM cause code                             **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_establish_req(const emm_as_establish_t *msg, int *emm_cause)
{
  LOG_FUNC_IN;

  struct emm_data_context_s *emm_ctx                = NULL;
  emm_security_context_t    *emm_security_context   = NULL;
  int decoder_rc;
  int rc = RETURNerror;

  LOG_TRACE(INFO, "EMMAS-SAP - Received AS connection establish request");

  nas_message_t nas_msg;
  memset(&nas_msg, 0 , sizeof(nas_message_t));

#if defined(NAS_BUILT_IN_EPC)
  emm_ctx = emm_data_context_get(&_emm_data, msg->ueid);
#else

  if (msg->ueid < EMM_DATA_NB_UE_MAX) {
    emm_ctx = _emm_data.ctx[msg->ueid];
  }

#endif

  if (emm_ctx) {
    emm_security_context = emm_ctx->security;
  }

  /* Decode initial NAS message */
  decoder_rc = nas_message_decode(
                 (char *)(msg->NASmsg.value),
                 &nas_msg,
                 msg->NASmsg.length,
                 emm_security_context);

  if (decoder_rc < TLV_DECODE_FATAL_ERROR) {
    *emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
    LOG_FUNC_RETURN (decoder_rc);
  } else if (decoder_rc == TLV_DECODE_UNEXPECTED_IEI) {
    *emm_cause = EMM_CAUSE_IE_NOT_IMPLEMENTED;
  } else if (decoder_rc < 0) {
    *emm_cause = EMM_CAUSE_PROTOCOL_ERROR;
  }

  /* Process initial NAS message */
  EMM_msg *emm_msg = &nas_msg.plain.emm;

  switch (emm_msg->header.message_type) {
  case ATTACH_REQUEST:
    rc = emm_recv_attach_request(
           msg->ueid,
           &emm_msg->attach_request,
           emm_cause);
    break;

  case DETACH_REQUEST:
    rc = RETURNok;  /* TODO */
    break;

  case TRACKING_AREA_UPDATE_REQUEST:
    rc = emm_recv_tracking_area_update_request(
           msg->ueid,
           &emm_msg->tracking_area_update_request,
           emm_cause);
    break;

  case SERVICE_REQUEST:
    rc = RETURNok;  /* TODO */
    break;

  case EXTENDED_SERVICE_REQUEST:
    rc = RETURNok;  /* TODO */
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
static int _emm_as_cell_info_res(const emm_as_cell_info_t *msg)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO, "EMMAS-SAP - Received AS cell information response");



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
static int _emm_as_send(const emm_as_t *msg)
{
  LOG_FUNC_IN;

  as_message_t as_msg;
  memset(&as_msg, 0 , sizeof(as_message_t));

  switch (msg->primitive) {
  case _EMMAS_DATA_REQ:
    as_msg.msgID = _emm_as_data_req(
                     &msg->u.data,
                     &as_msg.msg.ul_info_transfer_req);
    break;

  case _EMMAS_STATUS_IND:
    as_msg.msgID = _emm_as_status_ind(
                     &msg->u.status,
                     &as_msg.msg.ul_info_transfer_req);
    break;

  case _EMMAS_RELEASE_REQ:
    as_msg.msgID = _emm_as_release_req(
                     &msg->u.release,
                     &as_msg.msg.nas_release_req);
    break;

  case _EMMAS_SECURITY_REQ:
    as_msg.msgID = _emm_as_security_req(
                     &msg->u.security,
                     &as_msg.msg.dl_info_transfer_req);
    break;

  case _EMMAS_SECURITY_REJ:
    as_msg.msgID = _emm_as_security_rej(
                     &msg->u.security,
                     &as_msg.msg.dl_info_transfer_req);
    break;

  case _EMMAS_ESTABLISH_CNF:
    as_msg.msgID = _emm_as_establish_cnf(
                     &msg->u.establish,
                     &as_msg.msg.nas_establish_rsp);
    break;

  case _EMMAS_ESTABLISH_REJ:
    as_msg.msgID = _emm_as_establish_rej(
                     &msg->u.establish,
                     &as_msg.msg.nas_establish_rsp);
    break;

  case _EMMAS_PAGE_IND:
    as_msg.msgID = _emm_as_page_ind(&msg->u.page,
                                    &as_msg.msg.paging_req);
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
#if defined(NAS_BUILT_IN_EPC)
    LOG_TRACE(DEBUG, "EMMAS-SAP - "
              "Sending msg with id 0x%x, primitive %s (%d) to S1AP layer for transmission",
              as_msg.msgID,
              _emm_as_primitive_str[msg->primitive - _EMMAS_START - 1],
              msg->primitive);

    switch (as_msg.msgID) {
    case AS_DL_INFO_TRANSFER_REQ: {
      nas_itti_dl_data_req(
        as_msg.msg.dl_info_transfer_req.UEid,
        as_msg.msg.dl_info_transfer_req.nasMsg.data,
        as_msg.msg.dl_info_transfer_req.nasMsg.length);
      LOG_FUNC_RETURN (RETURNok);
    }
    break;

    case AS_NAS_ESTABLISH_RSP:
    case AS_NAS_ESTABLISH_CNF: {
      if (as_msg.msg.nas_establish_rsp.errCode != AS_SUCCESS) {
        nas_itti_dl_data_req(
          as_msg.msg.nas_establish_rsp.UEid,
          as_msg.msg.nas_establish_rsp.nasMsg.data,
          as_msg.msg.nas_establish_rsp.nasMsg.length);
        LOG_FUNC_RETURN (RETURNok);
      } else {
        LOG_TRACE(DEBUG, "EMMAS-SAP - "
                  "Sending nas_itti_establish_cnf to S1AP UE ID 0x%x"
                  " selected_encryption_algorithm 0x%04X",
                  " selected_integrity_algorithm 0x%04X",
                  as_msg.msg.nas_establish_rsp.UEid,
                  as_msg.msg.nas_establish_rsp.selected_encryption_algorithm,
                  as_msg.msg.nas_establish_rsp.selected_integrity_algorithm);
        /* Handle success case */
        nas_itti_establish_cnf(
          as_msg.msg.nas_establish_rsp.UEid,
          as_msg.msg.nas_establish_rsp.errCode,
          as_msg.msg.nas_establish_rsp.nasMsg.data,
          as_msg.msg.nas_establish_rsp.nasMsg.length,
          as_msg.msg.nas_establish_rsp.selected_encryption_algorithm,
          as_msg.msg.nas_establish_rsp.selected_integrity_algorithm);
        LOG_FUNC_RETURN (RETURNok);
      }
    }
    break;

    default:
      break;
    }

#endif
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
static int _emm_as_data_req(const emm_as_data_t *msg,
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

    case EMM_AS_NAS_DATA_DETACH:
      size = emm_send_detach_accept(msg, &emm_msg->detach_accept);
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
    struct emm_data_context_s *emm_ctx                = NULL;
#if defined(NAS_BUILT_IN_EPC)
    emm_ctx = emm_data_context_get(&_emm_data, msg->ueid);
#else

    if (msg->ueid < EMM_DATA_NB_UE_MAX) {
      emm_ctx = _emm_data.ctx[msg->ueid];
    }

#endif

    if (emm_ctx) {
      emm_security_context = emm_ctx->security;
    }



    if (emm_security_context) {
      nas_msg.header.sequence_number = emm_security_context->dl_count.seq_num;
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
      LOG_FUNC_RETURN (AS_DL_INFO_TRANSFER_REQ);
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
static int _emm_as_status_ind(const emm_as_status_t *msg,
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
    struct emm_data_context_s *emm_ctx                = NULL;
#if defined(NAS_BUILT_IN_EPC)
    emm_ctx = emm_data_context_get(&_emm_data, msg->ueid);
#else

    if (msg->ueid < EMM_DATA_NB_UE_MAX) {
      emm_ctx = _emm_data.ctx[msg->ueid];
    }

#endif

    if (emm_ctx) {
      emm_security_context = emm_ctx->security;
    }

    if (emm_security_context) {
      nas_msg.header.sequence_number = emm_security_context->dl_count.seq_num;
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
      LOG_FUNC_RETURN (AS_DL_INFO_TRANSFER_REQ);
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
 ** Name:    _emm_as_security_req()                                    **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP security request primitive        **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: SECURITY_REQ - Security mode control procedure    **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_security_req(const emm_as_security_t *msg,
                                dl_info_transfer_req_t *as_msg)
{
  LOG_FUNC_IN;

  int size = 0;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS security request");

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

  /* Setup the NAS security message */
  if (emm_msg != NULL) switch (msg->msgType) {
    case EMM_AS_MSG_TYPE_IDENT:
      if (msg->guti) {
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send IDENTITY_REQUEST to s_TMSI %u.%u ", as_msg->s_tmsi.MMEcode, as_msg->s_tmsi.m_tmsi);
      } else {
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send IDENTITY_REQUEST to ue id "NAS_UE_ID_FMT" ", as_msg->UEid);
      }
      size = emm_send_identity_request(msg,
                                       &emm_msg->identity_request);
      break;

    case EMM_AS_MSG_TYPE_AUTH:
      if (msg->guti) {
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send AUTHENTICATION_REQUEST to s_TMSI %u.%u ", as_msg->s_tmsi.MMEcode, as_msg->s_tmsi.m_tmsi);
      } else {
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send AUTHENTICATION_REQUEST to ue id "NAS_UE_ID_FMT" ", as_msg->UEid);
      }
      size = emm_send_authentication_request(msg,
                                             &emm_msg->authentication_request);
      break;

    case EMM_AS_MSG_TYPE_SMC:
      if (msg->guti) {
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send SECURITY_MODE_COMMAND to s_TMSI %u.%u ", as_msg->s_tmsi.MMEcode, as_msg->s_tmsi.m_tmsi);
      } else {
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send SECURITY_MODE_COMMAND to ue id "NAS_UE_ID_FMT" ", as_msg->UEid);
      }
      size = emm_send_security_mode_command(msg,
                                            &emm_msg->security_mode_command);
      break;

    default:
      LOG_TRACE(WARNING, "EMMAS-SAP - Type of NAS security "
                "message 0x%.2x is not valid", msg->msgType);
    }

  if (size > 0) {
    struct emm_data_context_s *emm_ctx                = NULL;
    emm_security_context_t    *emm_security_context   = NULL;
#if defined(NAS_BUILT_IN_EPC)
    emm_ctx = emm_data_context_get(&_emm_data, msg->ueid);
#else

    if (msg->ueid < EMM_DATA_NB_UE_MAX) {
      emm_ctx = _emm_data.ctx[msg->ueid];
    }

#endif

    if (emm_ctx) {
      emm_security_context = emm_ctx->security;

      if (emm_security_context) {
        nas_msg.header.sequence_number = emm_security_context->dl_count.seq_num;
        LOG_TRACE(DEBUG,
                  "Set nas_msg.header.sequence_number -> %u",
                  nas_msg.header.sequence_number);
      }
    }

    /* Encode the NAS security message */
    int bytes = _emm_as_encode(
                  &as_msg->nasMsg,
                  &nas_msg,
                  size,
                  emm_security_context);

    if (bytes > 0) {
      LOG_FUNC_RETURN (AS_DL_INFO_TRANSFER_REQ);
    }
  }

  LOG_FUNC_RETURN (0);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_security_rej()                                    **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP security reject primitive         **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: SECURITY_REJ - Security mode control procedure    **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_security_rej(const emm_as_security_t *msg,
                                dl_info_transfer_req_t *as_msg)
{
  LOG_FUNC_IN;

  int size = 0;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS security reject");

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

  /* Setup the NAS security message */
  if (emm_msg != NULL) switch (msg->msgType) {
    case EMM_AS_MSG_TYPE_AUTH:
      if (msg->guti) {
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send AUTHENTICATION_REJECT to s_TMSI %u.%u ", as_msg->s_tmsi.MMEcode, as_msg->s_tmsi.m_tmsi);
      } else {
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send AUTHENTICATION_REJECT to ue id "NAS_UE_ID_FMT" ", as_msg->UEid);
      }
      size = emm_send_authentication_reject(
               &emm_msg->authentication_reject);
      break;

    default:
      LOG_TRACE(WARNING, "EMMAS-SAP - Type of NAS security "
                "message 0x%.2x is not valid", msg->msgType);
    }

  if (size > 0) {
    struct emm_data_context_s *emm_ctx                = NULL;
    emm_security_context_t    *emm_security_context   = NULL;
#if defined(NAS_BUILT_IN_EPC)
    emm_ctx = emm_data_context_get(&_emm_data, msg->ueid);
#else

    if (msg->ueid < EMM_DATA_NB_UE_MAX) {
      emm_ctx = _emm_data.ctx[msg->ueid];
    }

#endif

    if (emm_ctx) {
      emm_security_context = emm_ctx->security;
      if (emm_security_context) {
      nas_msg.header.sequence_number = emm_security_context->dl_count.seq_num;
      LOG_TRACE(DEBUG,
                "Set nas_msg.header.sequence_number -> %u",
                nas_msg.header.sequence_number);
      } else {
          LOG_TRACE(DEBUG,
                  "No security context, not set nas_msg.header.sequence_number -> %u",
                  nas_msg.header.sequence_number);
      }
    }

    /* Encode the NAS security message */
    int bytes = _emm_as_encode(
                  &as_msg->nasMsg,
                  &nas_msg,
                  size,
                  emm_security_context);

    if (bytes > 0) {
      LOG_FUNC_RETURN (AS_DL_INFO_TRANSFER_REQ);
    }
  }

  LOG_FUNC_RETURN (0);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_establish_cnf()                                   **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP connection establish confirm      **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: ESTABLISH_CNF - NAS signalling connection         **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_establish_cnf(const emm_as_establish_t *msg,
                                 nas_establish_rsp_t *as_msg)
{
  EMM_msg *emm_msg;
  int size = 0;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS connection establish confirmation");

  nas_message_t nas_msg;
  memset(&nas_msg, 0 , sizeof(nas_message_t));

  /* Setup the AS message */
  as_msg->UEid = msg->ueid;

  if (msg->UEid.guti == NULL) {
    LOG_TRACE(WARNING, "EMMAS-SAP - GUTI is NULL...");
    LOG_FUNC_RETURN (0);
  }

  as_msg->s_tmsi.MMEcode = msg->UEid.guti->gummei.MMEcode;
  as_msg->s_tmsi.m_tmsi = msg->UEid.guti->m_tmsi;

  /* Setup the NAS security header */
  emm_msg = _emm_as_set_header(&nas_msg, &msg->sctx);

  /* Setup the initial NAS information message */
  if (emm_msg != NULL) switch (msg->NASinfo) {
    case EMM_AS_NAS_INFO_ATTACH:
      LOG_TRACE(WARNING,
                "EMMAS-SAP - emm_as_establish.nasMSG.length=%d",
                msg->NASmsg.length);

      MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send ATTACH_ACCEPT to s_TMSI %u.%u ", as_msg->s_tmsi.MMEcode, as_msg->s_tmsi.m_tmsi);

      size = emm_send_attach_accept(msg, &emm_msg->attach_accept);
      break;

    default:
      LOG_TRACE(WARNING, "EMMAS-SAP - Type of initial NAS "
                "message 0x%.2x is not valid", msg->NASinfo);
      break;
    }

  if (size > 0) {
    struct emm_data_context_s *emm_ctx                = NULL;
    emm_security_context_t    *emm_security_context   = NULL;
#if defined(NAS_BUILT_IN_EPC)
    emm_ctx = emm_data_context_get(&_emm_data, msg->ueid);
#else

    if (msg->ueid < EMM_DATA_NB_UE_MAX) {
      emm_ctx = _emm_data.ctx[msg->ueid];
    }

#endif

    if (emm_ctx) {
      emm_security_context = emm_ctx->security;

      if (emm_security_context) {
        as_msg->nas_ul_count = 0x00000000 |
                               (emm_security_context->ul_count.overflow << 8) |
                               emm_security_context->ul_count.seq_num;
        LOG_TRACE(DEBUG, "EMMAS-SAP - NAS UL COUNT %8x",
                  as_msg->nas_ul_count);
      }

      nas_msg.header.sequence_number = emm_security_context->dl_count.seq_num;
      LOG_TRACE(DEBUG,
                "Set nas_msg.header.sequence_number -> %u",
                nas_msg.header.sequence_number);

      as_msg->selected_encryption_algorithm = htons(0x8000 >> emm_security_context->selected_algorithms.encryption);
      as_msg->selected_integrity_algorithm = htons(0x8000 >> emm_security_context->selected_algorithms.integrity);

      LOG_TRACE(DEBUG,
                "Set nas_msg.selected_encryption_algorithm -> NBO: 0x%04X (%u)",
                as_msg->selected_encryption_algorithm,
                emm_security_context->selected_algorithms.encryption);
      LOG_TRACE(DEBUG,
                "Set nas_msg.selected_integrity_algorithm -> NBO: 0x%04X (%u)",
                as_msg->selected_integrity_algorithm,
                emm_security_context->selected_algorithms.integrity);

    }

    /* Encode the initial NAS information message */
    int bytes = _emm_as_encode(
                  &as_msg->nasMsg,
                  &nas_msg,
                  size,
                  emm_security_context);

    if (bytes > 0) {
      as_msg->errCode = AS_SUCCESS;
      LOG_FUNC_RETURN (AS_NAS_ESTABLISH_CNF);
    }
  }

  LOG_TRACE(WARNING, "EMMAS-SAP - Size <= 0");
  LOG_FUNC_RETURN (0);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_establish_rej()                                   **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP connection establish reject       **
 **      primitive                                                 **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: ESTABLISH_REJ - NAS signalling connection         **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_establish_rej(const emm_as_establish_t *msg,
                                 nas_establish_rsp_t *as_msg)
{
  EMM_msg *emm_msg;
  int size = 0;
  nas_message_t nas_msg;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS connection establish reject");

  memset(&nas_msg, 0 , sizeof(nas_message_t));

  /* Setup the AS message */
  if (msg->UEid.guti) {
    as_msg->s_tmsi.MMEcode = msg->UEid.guti->gummei.MMEcode;
    as_msg->s_tmsi.m_tmsi = msg->UEid.guti->m_tmsi;
  } else {
    as_msg->UEid = msg->ueid;
  }

  /* Setup the NAS security header */
  emm_msg = _emm_as_set_header(&nas_msg, &msg->sctx);

  /* Setup the NAS information message */
  if (emm_msg != NULL) switch (msg->NASinfo) {
    case EMM_AS_NAS_INFO_ATTACH:
      if (msg->UEid.guti) {
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send ATTACH_REJECT to s_TMSI %u.%u ", as_msg->s_tmsi.MMEcode, as_msg->s_tmsi.m_tmsi);
      } else {
        MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send ATTACH_REJECT to ue id "NAS_UE_ID_FMT" ", as_msg->UEid);
      }
      size = emm_send_attach_reject(msg, &emm_msg->attach_reject);
      break;

    case EMM_AS_NAS_INFO_TAU:
        if (msg->UEid.guti) {
          MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send TRACKING_AREA_UPDATE_REJECT to s_TMSI %u.%u ", as_msg->s_tmsi.MMEcode, as_msg->s_tmsi.m_tmsi);
        } else {
          MSC_LOG_EVENT(MSC_NAS_EMM_MME, "0 send TRACKING_AREA_UPDATE_REJECT to ue id "NAS_UE_ID_FMT" ", as_msg->UEid);
        }
      size = emm_send_tracking_area_update_reject(msg,
             &emm_msg->tracking_area_update_reject);
      break;

    default:
      LOG_TRACE(WARNING, "EMMAS-SAP - Type of initial NAS "
                "message 0x%.2x is not valid", msg->NASinfo);
      break;
    }

  if (size > 0) {
    struct emm_data_context_s *emm_ctx                = NULL;
    emm_security_context_t    *emm_security_context   = NULL;
#if defined(NAS_BUILT_IN_EPC)
    emm_ctx = emm_data_context_get(&_emm_data, msg->ueid);
#else

    if (msg->ueid < EMM_DATA_NB_UE_MAX) {
      emm_ctx = _emm_data.ctx[msg->ueid];
    }

#endif

    if (emm_ctx) {
      emm_security_context = emm_ctx->security;

      if (emm_security_context) {
        nas_msg.header.sequence_number = emm_security_context->dl_count.seq_num;
        LOG_TRACE(DEBUG,
                  "Set nas_msg.header.sequence_number -> %u",
                  nas_msg.header.sequence_number);
      }
    }

    /* Encode the initial NAS information message */
    int bytes = _emm_as_encode(
                  &as_msg->nasMsg,
                  &nas_msg,
                  size,
                  emm_security_context);

    if (bytes > 0) {
      as_msg->errCode = AS_TERMINATED_NAS;
      LOG_FUNC_RETURN (AS_NAS_ESTABLISH_RSP);
    }
  }

  LOG_FUNC_RETURN (0);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_as_page_ind()                                        **
 **                                                                        **
 ** Description: Processes the EMMAS-SAP paging data indication primitive  **
 **                                                                        **
 ** EMMAS-SAP - EMM->AS: PAGE_IND - Paging data procedure                  **
 **                                                                        **
 ** Inputs:  msg:       The EMMAS-SAP primitive to process         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     as_msg:    The message to send to the AS              **
 **      Return:    The identifier of the AS message           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _emm_as_page_ind(const emm_as_page_t *msg, paging_req_t *as_msg)
{
  LOG_FUNC_IN;

  int bytes = 0;

  LOG_TRACE(INFO, "EMMAS-SAP - Send AS data paging indication");

  /* TODO */

  if (bytes > 0) {
    LOG_FUNC_RETURN (AS_PAGING_IND);
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

