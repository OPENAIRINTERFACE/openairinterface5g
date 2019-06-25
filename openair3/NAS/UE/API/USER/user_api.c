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

Source          user_api.c

Version         0.1

Date            2012/02/28

Product         NAS stack

Subsystem       Application Programming Interface

Author          Frederic Maurel

Description     Implements the API used by the NAS layer running in the UE
                to send/receive message to/from the user application layer

*****************************************************************************/


#include "user_api.h"
#include "nas_log.h"
#include "socket.h"
#include "device.h"
#include "nas_user.h"

#include "at_response.h"
#include "at_error.h"
#include "esm_ebr.h"

#include "user_indication.h"

#include <string.h> // strerror, memset
#include <netdb.h>  // gai_strerror
#include <errno.h>  // errno
#include <stdio.h>  // sprintf
#include <unistd.h> // gethostname

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * Asynchronous notification procedure handlers
 */
static int _user_api_registration_handler(user_api_id_t *user_api_id, unsigned char id, const void* data, size_t size);
static int _user_api_location_handler(user_api_id_t *user_api_id, unsigned char id, const void* data, size_t size);
static int _user_api_network_handler(user_api_id_t *user_api_id, unsigned char id, const void* data, size_t size);
static int _user_api_pdn_connection_handler(user_api_id_t *user_api_id, unsigned char id, const void* data, size_t size);

static int _user_api_send(user_api_id_t *user_api_id, at_response_t* data);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:        user_api_initialize()                                     **
 **                                                                        **
 ** Description: Initializes the user API from which the NAS layer         **
 **              will send/receive messages to/from the user application   **
 **              layer                                                     **
 **                                                                        **
 ** Inputs:      host:          The name of the host from which the user   **
 **                             application layer will connect             **
 **              port:          The local port number                      **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     Return:        RETURNerror, RETURNok                      **
 **              Others:        _user_api_id                               **
 **                                                                        **
 ***************************************************************************/
int user_api_initialize(user_api_id_t *user_api_id, const char* host, const char* port,
                        const char* devname, const char* devparams)
{
  LOG_FUNC_IN;

  gethostname(user_api_id->send_buffer, USER_API_SEND_BUFFER_SIZE);

  if (devname != NULL) {
    /* Initialize device handlers */
    user_api_id->open  = device_open;
    user_api_id->getfd = device_get_fd;
    user_api_id->recv  = device_read;
    user_api_id->send  = device_write;
    user_api_id->close = device_close;

    /* Initialize communication channel */
    user_api_id->endpoint = user_api_id->open(DEVICE, devname, devparams);

    if (user_api_id->endpoint == NULL) {
      LOG_TRACE(ERROR, "USR-API   - Failed to open connection endpoint, "
                "%s", strerror(errno));
      LOG_FUNC_RETURN (RETURNerror);
    }

    LOG_TRACE(INFO, "USR-API   - User's communication device %d is OPENED "
              "on %s/%s", user_api_get_fd(user_api_id), user_api_id->send_buffer, devname);
  } else {
    /* Initialize network socket handlers */
    user_api_id->open  = socket_udp_open;
    user_api_id->getfd = socket_get_fd;
    user_api_id->recv  = socket_recv;
    user_api_id->send  = socket_send;
    user_api_id->close = socket_close;

    /* Initialize communication channel */
    user_api_id->endpoint = user_api_id->open(SOCKET_SERVER, host, port);

    if (user_api_id->endpoint == NULL) {
      LOG_TRACE(ERROR, "USR-API   - Failed to open connection endpoint, "
                "%s", ( (errno < 0) ?gai_strerror(errno) : strerror(errno) ));
      LOG_FUNC_RETURN (RETURNerror);
    }

    LOG_TRACE(INFO, "USR-API   - User's UDP socket %d is BOUND to %s/%s",
              user_api_get_fd(user_api_id), user_api_id->send_buffer, port);
  }

  /* Register the asynchronous notification handlers */
  if (user_ind_register(USER_IND_REG, 0, _user_api_registration_handler) != RETURNok) {
    LOG_TRACE(WARNING, "USR-API   - "
              "Failed to register registration notification");
  } else if (user_ind_register(USER_IND_LOC, 0, _user_api_location_handler) != RETURNok) {
    LOG_TRACE(WARNING, "USR-API   - "
              "Failed to register location notification");
  } else if (user_ind_register(USER_IND_PLMN, 0, _user_api_network_handler) != RETURNok) {
    LOG_TRACE(WARNING, "USR-API   - "
              "Failed to register network notification");
  } else if (user_ind_register(USER_IND_PLMN, 0, NULL) != RETURNok) {
    LOG_TRACE(WARNING, "USR-API   - Failed to enable network notification");
  } else if (user_ind_register(USER_IND_PDN, 0, _user_api_pdn_connection_handler) != RETURNok) {
    LOG_TRACE(WARNING, "USR-API   - "
              "Failed to register PDN connection notification");
  } else if (user_ind_register(USER_IND_PDN, AT_CGACT, NULL) != RETURNok) {
    LOG_TRACE(WARNING, "USR-API   - "
              "Failed to enable PDN connection notification");
  } else {
    LOG_TRACE(INFO, "USR-API   - "
              "Notification handlers successfully registered");
  }

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:        user_api_get_fd()                                         **
 **                                                                        **
 ** Description: Get the file descriptor of the connection endpoint used   **
 **              to send/receive messages to/from the user application     **
 **              layer                                                     **
 **                                                                        **
 ** Inputs:      None                                                      **
 **              Others:        _user_api_id                               **
 **                                                                        **
 ** Outputs:     Return:        The file descriptor of the connection end- **
 **                             point used by the user application layer   **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
int user_api_get_fd(user_api_id_t *user_api_id)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (user_api_id->getfd(user_api_id->endpoint));
}

/****************************************************************************
 **                                                                        **
 ** Name:        user_api_get_data()                                       **
 **                                                                        **
 ** Description: Get a generic pointer to the user data structure at the   **
 **              given index. Casting to the proper type is necessary      **
 **              before its usage.                                         **
 **                                                                        **
 ** Inputs:      index:         Index of the user data structure to get    **
 **                                                                        **
 ** Outputs:     Return:        A generic pointer to the user data         **
 **                             structure                                  **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
const void* user_api_get_data(user_at_commands_t *commands, int index)
{
  LOG_FUNC_IN;

  if (index < commands->n_cmd) {
    LOG_FUNC_RETURN ((void*)(&commands->cmd[index]));
  }

  LOG_FUNC_RETURN (NULL);
}

/****************************************************************************
 **                                                                        **
 ** Name:        user_api_read_data()                                      **
 **                                                                        **
 ** Description: Read data received from the user application layer        **
 **                                                                        **
 **          Others:            _user_api_id                               **
 **                                                                        **
 ** Outputs: Return:            The number of bytes read when success;     **
 **                             RETURNerror Otherwise                      **
 **          Others:            user_api_id->recv_buffer, _user_api_id        **
 **                                                                        **
 ***************************************************************************/
int user_api_read_data(user_api_id_t *user_api_id)
{
  LOG_FUNC_IN;

  int rbytes;

  memset(user_api_id->recv_buffer, 0, USER_API_RECV_BUFFER_SIZE);

  /* Receive data from the user application layer */
  rbytes = user_api_id->recv(user_api_id->endpoint, user_api_id->recv_buffer, USER_API_RECV_BUFFER_SIZE);

  if (rbytes == RETURNerror) {
    LOG_TRACE(ERROR, "USR-API   - recv() failed, %s", strerror(errno));
    LOG_FUNC_RETURN (RETURNerror);
  } else if (rbytes == 0) {
    //LOG_TRACE(WARNING, "USR-API   - A signal was caught");
  } else {
    LOG_TRACE(INFO, "USR-API   - %d bytes received "
              "from the user application layer", rbytes);
    LOG_DUMP(user_api_id->recv_buffer, rbytes);
  }

  LOG_FUNC_RETURN (rbytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:        user_api_set_data()                                       **
 **                                                                        **
 ** Description: Set content of data received buffer to allow loop back    **
 **                                                                        **
 ** Inputs:         message:    Message to set into the received buffer    **
 **                                                                        **
 ** Outputs:         Return:    The number of bytes write when success;    **
 **                             RETURNerror Otherwise                      **
 **                  Others:    user_api_id->recv_buffer                      **
 **                                                                        **
 ***************************************************************************/
int user_api_set_data(user_api_id_t *user_api_id, char *message)
{
  LOG_FUNC_IN;

  int rbytes;

  memset(user_api_id->recv_buffer, 0, USER_API_RECV_BUFFER_SIZE);

  strncpy(user_api_id->recv_buffer, message, USER_API_RECV_BUFFER_SIZE - 1);
  rbytes = strlen(user_api_id->recv_buffer);

  LOG_TRACE(INFO, "USR-API   - %d bytes write", rbytes);
  LOG_DUMP(user_api_id->recv_buffer, rbytes);

  LOG_FUNC_RETURN (rbytes);
}

/****************************************************************************
 **                                                                        **
** Name:        user_api_send_data()                                      **
 **                                                                        **
 ** Description: Send data to the user application layer                   **
 **                                                                        **
 **              length:        Number of bytes to send                    **
 **                                                                        **
 ** Outputs:     Return:        The number of bytes sent when success;     **
 **                             RETURNerror Otherwise                      **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
static int _user_api_send_data(user_api_id_t *user_api_id, int length)
{
  int sbytes = user_api_id->send(user_api_id->endpoint, user_api_id->send_buffer, length);

  if (sbytes == RETURNerror) {
    LOG_TRACE(ERROR, "USR-API   - send() failed, %s", strerror(errno));
    return RETURNerror;
  } else if (sbytes == 0) {
    LOG_TRACE(WARNING, "USR-API   - A signal was caught");
  } else {
    LOG_TRACE(INFO, "USR-API   - %d bytes sent "
              "to the user application layer", sbytes);
    LOG_DUMP(user_api_id->send_buffer, sbytes);
  }

  return sbytes;
}

/****************************************************************************
 **                                                                        **
 ** Name:        user_api_close()                                          **
 ***************************************************************************/
 int user_api_send_data(user_api_id_t *user_api_id, int length)
{
  LOG_FUNC_IN;

  /* Send data to the user application layer */
  int sbytes = 0;

  if (length > 0) {
    sbytes = _user_api_send_data(user_api_id, length);
  }

  LOG_FUNC_RETURN (sbytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:        user_api_close()                                          **
 **                                                                        **
 ** Description: Close the user API from which the NAS layer sent/received **
 **              messages to/from the user application layer               **
 **                                                                        **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     Return:        None                                       **
 **                                                                        **
 ***************************************************************************/
void user_api_close(user_api_id_t *user_api_id)
{
  LOG_FUNC_IN;

  /* Cleanup the connection endpoint */
  user_api_id->close(user_api_id->endpoint) ;
  user_api_id->endpoint = NULL;

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:  user_api_decode_data()                                    **
 **                                                                        **
 ** Description: Parses the message received from the user application     **
 **    (mainly AT commands) and fills the user data structure.   **
 **    Returns an AT syntax error code to the user application   **
 **    layer when the AT command failed to be decoded.           **
 **                                                                        **
 ** Inputs:  length:  Number of bytes to decode                  **
 **                                                                        **
 ** Outputs:   Return:  The number of AT commands succeessfully    **
 **       decoded                                    **
 **                                                                        **
 ***************************************************************************/
int user_api_decode_data(user_api_id_t *user_api_id, user_at_commands_t *commands, int length)
{
  LOG_FUNC_IN;

  /* Parse the AT command line */
  LOG_TRACE(INFO, "USR-API   - Decode user data: %s", user_api_id->recv_buffer);
  commands->n_cmd = at_command_decode(user_api_id->recv_buffer, length,
                                       commands->cmd);

  if (commands->n_cmd > 0) {
    /* AT command data received from the user application layer
     * has been successfully decoded */
    LOG_TRACE(INFO, "USR-API   - %d AT command%s ha%s been successfully "
              "decoded", commands->n_cmd,
              (commands->n_cmd > 1) ? "s" : "",
              (commands->n_cmd > 1) ? "ve" : "s");
  } else {
    int bytes;

    /* Failed to decode AT command data received from the user
     * application layer; Return syntax error code message */
    LOG_TRACE(ERROR, "USR-API   - Syntax error: Failed to decode "
              "AT command data %s", user_api_id->recv_buffer);

    /* Encode the syntax error code message */
    bytes = at_error_encode(user_api_id->send_buffer, AT_ERROR_SYNTAX,
                            AT_ERROR_OPERATION_NOT_SUPPORTED);

    // FIXME move _user_data call
    /* Send the syntax error code message */
    _user_api_send_data(user_api_id, bytes);
  }

  LOG_FUNC_RETURN (commands->n_cmd);
}

/****************************************************************************
 **                                                                        **
 ** Name:  user_api_encode_data()                                    **
 **                                                                        **
 ** Description: Encodes AT command response message                       **
 **                                                                        **
 ** Inputs   data:    Generic pointer to the data to encode      **
 **      success_code:  Indicates whether success code has to be   **
 **       displayed or not (covers the case where    **
 **       more than one AT command is executed in    **
 **       the same user command line).               **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  The number of characters that have been    **
 **       successfully encoded;                      **
 **       RETURNerror otherwise.                     **
 **                                                                        **
 ***************************************************************************/
int user_api_encode_data(user_api_id_t *user_api_id, const void* data, int success_code)
{
  LOG_FUNC_IN;

  const at_response_t* user_data = (at_response_t*)(data);
  int bytes;

  /* Encode AT command error message */
  if (user_data->cause_code != AT_ERROR_SUCCESS) {
    bytes = at_error_encode(user_api_id->send_buffer, AT_ERROR_CME,
                            user_data->cause_code);
  }
  /* Encode AT command response message */
  else {
    bytes = at_response_encode(user_api_id->send_buffer, user_data);

    /* Add success result code */
    if ( (success_code) && (bytes != RETURNerror) ) {
      bytes += at_error_encode(&user_api_id->send_buffer[bytes],
                               AT_ERROR_OK, 0);
    }
  }

  if (bytes != RETURNerror) {
    LOG_TRACE(INFO, "USR-API   - %d bytes encoded", bytes);
  } else {
    LOG_TRACE(ERROR, "USR-API   - Syntax error: Failed to encode AT "
              "response data (%d)", user_data->id);
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:  user_api_callback()                                       **
 **                                                                        **
 ** Description: Notifies the user application that asynchronous notifica- **
 **    tion has been received from the EPS Mobility Management   **
 **    sublayer.                                                 **
 **                                                                        **
 ** Inputs:  stat:    Network registration status                **
 **    tac:   Location/Tracking Area Code                **
 **    ci:    Indentifier of the serving cell            **
 **    AcT:   Access Technology supported by the cell    **
 **    data:    Data string to display to the user         **
 **    size:    Size of the notification data (only used   **
 **       to display string information to the user  **
 **       application)                               **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:  RETURNok, RETURNerror                      **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int user_api_emm_callback(user_api_id_t *user_api_id, Stat_t stat, tac_t tac, ci_t ci, AcT_t AcT,
                          const char* data, size_t size)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  if (size > 1) {
    /*
     * The list of available operators present in the network has to be
     * displayed to the user application
     */
    rc = user_ind_notify(user_api_id, USER_IND_PLMN, (void*)data, size);
  } else {
    user_indication_t ind;
    ind.notification.reg.status = stat;

    if (size > 0) {
      /* The UE's network registration status has changed */
      rc = user_ind_notify(user_api_id, USER_IND_REG, (void*)&ind, 0);
    }

    if (rc != RETURNerror) {
      /* The UE's location area has changed or,
       * the UE's network registration status has changed and
       * only location information notification is enabled */
      ind.notification.loc.tac = tac;
      ind.notification.loc.ci  = ci;
      ind.notification.loc.AcT = AcT;
      rc = user_ind_notify(user_api_id, USER_IND_LOC, (void*)&ind, 0);
    }
  }

  if (rc != RETURNerror) {
    LOG_FUNC_RETURN (RETURNok);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:  user_api_esm_callback()                                   **
 **                                                                        **
 ** Description: Notifies the user application that asynchronous notifica- **
 **    tion has been received from the EPS Session Management    **
 **    sublayer.                                                 **
 **                                                                        **
 ** Inputs:  cid:   PDN connection identifier                  **
 **    state:   PDN connection status                      **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:  RETURNok, RETURNerror                      **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int user_api_esm_callback(user_api_id_t *user_api_id, int cid, network_pdn_state_t state)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  user_indication_t ind;
  ind.notification.pdn.cid = cid;
  ind.notification.pdn.status = state;
  /* The status of the specified PDN connection has changed */
  rc = user_ind_notify(user_api_id, USER_IND_PDN, (void*)&ind, 0);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  _user_api_send()                                          **
 **                                                                        **
 ** Description: Encodes and sends data to the user application layer      **
 **                                                                        **
 ** Inputs:  data:    The data to send                           **
 **                                                                        **
 ** Outputs:   Return:  The number of bytes sent when success;     **
 **       RETURNerror Otherwise                      **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _user_api_send(user_api_id_t *user_api_id, at_response_t* data)
{
  LOG_FUNC_IN;

  /* Encode AT command response message */
  int bytes = at_response_encode(user_api_id->send_buffer, data);

  /* Send the AT command response message to the user application */
  if (bytes != RETURNerror) {
    bytes = _user_api_send_data(user_api_id, bytes);
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _user_api_registration_handler()                          **
 **                                                                        **
 ** Description: Procedure executed upon receiving registration notifica-  **
 **    tion whenever there is a change in the UE's network re-   **
 **    gistration status in GERAN/UTRAN/E-UTRAN.                 **
 **    The network registration data are then displayed to the   **
 **    user.                                                     **
 **                                                                        **
 ** Inputs:  id:    Network registration AT command identifier **
 **    data:    Generic pointer to the registration data   **
 **    size:    Not used                                   **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:  The number of bytes actually sent to the   **
 **       user application                           **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _user_api_registration_handler(user_api_id_t *user_api_id, unsigned char id, const void* data,
    size_t size)
{
  LOG_FUNC_IN;

  /* Get registration notification data */
  user_ind_reg_t* reg = (user_ind_reg_t*)data;

  /* Setup AT command response message for AT+CEREG? read command */
  at_response_t at_response;
  at_response.id = id; // may be +CREG, +CGREG, +CEREG
  at_response.type = AT_COMMAND_GET;
  at_response.mask = AT_RESPONSE_NO_PARAM;
  at_response.response.cereg.n = AT_CEREG_ON;
  at_response.response.cereg.stat = reg->status;

  /* Encode and send the AT command response message to the user */
  int bytes = _user_api_send(user_api_id, &at_response);

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _user_api_location_handler()                              **
 **                                                                        **
 ** Description: Procedure executed upon receiving location notification   **
 **    whenever there is a change in the network serving cell    **
 **    in GERAN/UTRAN/E-UTRAN.                                   **
 **    The location data are then displayed to the user.         **
 **                                                                        **
 ** Inputs:  id:    Network registration AT command identifier **
 **    data:    Generic pointer to the registration data   **
 **    size:    Not used                                   **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:  The number of bytes actually sent to the   **
 **       user application                           **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _user_api_location_handler(user_api_id_t *user_api_id, unsigned char id, const void* data,
                                      size_t size)
{
  LOG_FUNC_IN;

  /* Get location notification data */
  user_ind_loc_t* loc = (user_ind_loc_t*)data;

  /* Setup AT command response message for AT+CEREG? read command */
  at_response_t at_response;
  at_response.id = id; // may be +CREG, +CGREG, +CEREG
  at_response.type = AT_COMMAND_GET;
  at_response.mask = (AT_CEREG_RESP_TAC_MASK | AT_CEREG_RESP_CI_MASK);
  at_response.response.cereg.n = AT_CEREG_BOTH;
  at_response.response.cereg.stat = loc->status;
  sprintf(at_response.response.cereg.tac, "%.4x", loc->tac);  // two byte
  sprintf(at_response.response.cereg.ci, "%.8x", loc->ci);  // four byte

  if (at_response.response.cereg.AcT != NET_ACCESS_UNAVAILABLE) {
    at_response.response.cereg.AcT = loc->AcT;
    at_response.mask |= AT_CEREG_RESP_ACT_MASK;
  }

  /* Encode and send the AT command response message to the user */
  int bytes = _user_api_send(user_api_id, &at_response);

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _user_api_network_handler()                               **
 **                                                                        **
 ** Description: Procedure executed whenever the list of operators present **
 **    in the network has to be displayed to the user.           **
 **                                                                        **
 ** Inputs:  id:    Not used                                   **
 **    data:    Generic pointer to the list of operators   **
 **    size:    The size of the data to display            **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:  The number of bytes actually sent to the   **
 **       user application                           **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _user_api_network_handler(user_api_id_t *user_api_id, unsigned char id, const void* data,
                                     size_t size)
{
  LOG_FUNC_IN;

  /* Setup AT command response message for AT+COPS=? test command */
  at_response_t at_response;
  at_response.id = AT_COPS;
  at_response.type = AT_COMMAND_TST;
  at_response.response.cops.tst.data = (char*)data;
  at_response.response.cops.tst.size = size;

  /* Encode and send the AT command response message to the user */
  int bytes = _user_api_send(user_api_id, &at_response);

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _user_api_pdn_connection_handler()                        **
 **                                                                        **
 ** Description: Procedure executed upon receiving PDN connection notifi-  **
 **    cation whenever the user or the network has activated or  **
 **    desactivated a PDN connection.                            **
 **    The PDN connection data are then displayed to the user.   **
 **                                                                        **
 ** Inputs:  id:    Not used                                   **
 **    data:    Generic pointer to the PDN connection data **
 **    size:    Not used                                   **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:  The number of bytes actually sent to the   **
 **       user application                           **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _user_api_pdn_connection_handler(user_api_id_t *user_api_id, unsigned char id, const void* data,
    size_t size)
{
  LOG_FUNC_IN;

  /* Get PDN connection notification data */
  user_ind_pdn_t* pdn = (user_ind_pdn_t*)data;

  /* Setup AT command unsolicited result response message for +CGEV */
  at_response_t at_response;
  at_response.id = AT_CGEV;
  at_response.type = AT_COMMAND_GET;
  at_response.mask = AT_RESPONSE_CGEV_MASK;
  at_response.response.cgev.cid = pdn->cid;
  at_response.response.cgev.code = pdn->status;

  /* Encode and send the AT command response message to the user */
  int bytes = _user_api_send(user_api_id, &at_response);

  LOG_FUNC_RETURN (bytes);
}

