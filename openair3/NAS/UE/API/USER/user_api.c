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

#include "at_command.h"
#include "at_response.h"
#include "at_error.h"

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
static int _user_api_registration_handler(unsigned char id, const void* data, size_t size);
static int _user_api_location_handler(unsigned char id, const void* data, size_t size);
static int _user_api_network_handler(unsigned char id, const void* data, size_t size);
static int _user_api_pdn_connection_handler(unsigned char id, const void* data, size_t size);

static int _user_api_send(at_response_t* data);

/* -------------------
 * Connection endpoint
 * -------------------
 *      The connection endpoint is used to send/receive data to/from the
 *      user application layer. Its definition depends on the underlaying
 *      mechanism chosen to communicate (network socket, I/O terminal device).
 *      A connection endpoint is handled using an identifier, and functions
 *      used to retreive the file descriptor actually allocated by the system,
 *      to receive data, to send data, and to perform clean up when connection
 *      is shut down.
 *      Only one single end to end connection with the user is managed at a
 *      time.
 */
static struct {
  /* Connection endpoint reference  */
  void* endpoint;
  /* Connection endpoint handlers */
  void*   (*open) (int, const char*, const char*);
  int     (*getfd)(const void*);
  ssize_t (*recv) (void*, char*, size_t);
  ssize_t (*send) (const void*, const char*, size_t);
  void    (*close)(void*);
} _user_api_id;

#define USER_API_OPEN(a, b, c)  _user_api_id.open(a, b, c)
#define USER_API_GETFD()  _user_api_id.getfd(_user_api_id.endpoint)
#define USER_API_RECV(a, b) _user_api_id.recv(_user_api_id.endpoint, a, b)
#define USER_API_SEND(a, b) _user_api_id.send(_user_api_id.endpoint, a, b)
#define USER_API_CLOSE()  _user_api_id.close(_user_api_id.endpoint)

/*
 * The buffer used to receive data from the user application layer
 */
#define USER_API_RECV_BUFFER_SIZE 4096
static char _user_api_recv_buffer[USER_API_RECV_BUFFER_SIZE];

/*
 * The buffer used to send data to the user application layer
 */
#define USER_API_SEND_BUFFER_SIZE USER_API_RECV_BUFFER_SIZE
static char _user_api_send_buffer[USER_API_SEND_BUFFER_SIZE];

/*
 * The decoded data received from the user application layer
 */
static struct {
  int n_cmd;    /* number of user data to be processed    */
#define USER_DATA_MAX 10
  at_command_t cmd[USER_DATA_MAX];  /* user data to be processed  */
} _user_data = {};

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
int user_api_initialize(const char* host, const char* port,
                        const char* devname, const char* devparams)
{
  LOG_FUNC_IN;

  gethostname(_user_api_send_buffer, USER_API_SEND_BUFFER_SIZE);

  if (devname != NULL) {
    /* Initialize device handlers */
    _user_api_id.open  = device_open;
    _user_api_id.getfd = device_get_fd;
    _user_api_id.recv  = device_read;
    _user_api_id.send  = device_write;
    _user_api_id.close = device_close;

    /* Initialize communication channel */
    _user_api_id.endpoint = USER_API_OPEN(DEVICE, devname, devparams);

    if (_user_api_id.endpoint == NULL) {
      LOG_TRACE(ERROR, "USR-API   - Failed to open connection endpoint, "
                "%s", strerror(errno));
      LOG_FUNC_RETURN (RETURNerror);
    }

    LOG_TRACE(INFO, "USR-API   - User's communication device %d is OPENED "
              "on %s/%s", user_api_get_fd(), _user_api_send_buffer, devname);
  } else {
    /* Initialize network socket handlers */
    _user_api_id.open  = socket_udp_open;
    _user_api_id.getfd = socket_get_fd;
    _user_api_id.recv  = socket_recv;
    _user_api_id.send  = socket_send;
    _user_api_id.close = socket_close;

    /* Initialize communication channel */
    _user_api_id.endpoint = USER_API_OPEN(SOCKET_SERVER, host, port);

    if (_user_api_id.endpoint == NULL) {
      const char* error = ( (errno < 0) ?
                            gai_strerror(errno) : strerror(errno) );
      LOG_TRACE(ERROR, "USR-API   - Failed to open connection endpoint, "
                "%s", error);
      LOG_FUNC_RETURN (RETURNerror);
    }

    LOG_TRACE(INFO, "USR-API   - User's UDP socket %d is BOUND to %s/%s",
              user_api_get_fd(), _user_api_send_buffer, port);
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
int user_api_get_fd(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN (USER_API_GETFD());
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
 **              Others:        _user_data                                 **
 **                                                                        **
 ** Outputs:     Return:        A generic pointer to the user data         **
 **                             structure                                  **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
const void* user_api_get_data(int index)
{
  LOG_FUNC_IN;

  if (index < _user_data.n_cmd) {
    LOG_FUNC_RETURN ((void*)(&_user_data.cmd[index]));
  }

  LOG_FUNC_RETURN (NULL);
}

/****************************************************************************
 **                                                                        **
 ** Name:        user_api_read_data()                                      **
 **                                                                        **
 ** Description: Read data received from the user application layer        **
 **                                                                        **
 ** Inputs:      fd:            File descriptor of the connection endpoint **
 **                             from which data have been received         **
 **          Others:            _user_api_id                               **
 **                                                                        **
 ** Outputs: Return:            The number of bytes read when success;     **
 **                             RETURNerror Otherwise                      **
 **          Others:            _user_api_recv_buffer, _user_api_id        **
 **                                                                        **
 ***************************************************************************/
int user_api_read_data(int fd)
{
  LOG_FUNC_IN;

  int rbytes;

  /* Sanity check */
  int sfd = user_api_get_fd();

  if (fd != sfd) {
    LOG_TRACE(ERROR, "USR-API   - Endpoint %d is not the one created for communication with the user application layer (%d)", fd, sfd);
    LOG_FUNC_RETURN (RETURNerror);
  }

  memset(_user_api_recv_buffer, 0, USER_API_RECV_BUFFER_SIZE);

  /* Receive data from the user application layer */
  rbytes = USER_API_RECV(_user_api_recv_buffer, USER_API_RECV_BUFFER_SIZE);

  if (rbytes == RETURNerror) {
    LOG_TRACE(ERROR, "USR-API   - recv() failed, %s", strerror(errno));
    LOG_FUNC_RETURN (RETURNerror);
  } else if (rbytes == 0) {
    //LOG_TRACE(WARNING, "USR-API   - A signal was caught");
  } else {
    LOG_TRACE(INFO, "USR-API   - %d bytes received "
              "from the user application layer", rbytes);
    LOG_DUMP(_user_api_recv_buffer, rbytes);
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
 **                  Others:    _user_api_recv_buffer                      **
 **                                                                        **
 ***************************************************************************/
int user_api_set_data(char *message)
{
  LOG_FUNC_IN;

  int rbytes;

  memset(_user_api_recv_buffer, 0, USER_API_RECV_BUFFER_SIZE);

  strncpy(_user_api_recv_buffer, message, USER_API_RECV_BUFFER_SIZE);
  rbytes = strlen(_user_api_recv_buffer);

  LOG_TRACE(INFO, "USR-API   - %d bytes write", rbytes);
  LOG_DUMP(_user_api_recv_buffer, rbytes);

  LOG_FUNC_RETURN (rbytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:        user_api_send_data()                                      **
 **                                                                        **
 ** Description: Send data to the user application layer                   **
 **                                                                        **
 ** Inputs:      fd:            File descriptor of the connection endpoint **
 **                             to which data have to be sent              **
 **              length:        Number of bytes to send                    **
 **              Others:        _user_api_send_buffer, _user_api_id        **
 **                                                                        **
 ** Outputs:     Return:        The number of bytes sent when success;     **
 **                             RETURNerror Otherwise                      **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
static int _user_api_send_data(int length)
{
  int sbytes = USER_API_SEND(_user_api_send_buffer, length);

  if (sbytes == RETURNerror) {
    LOG_TRACE(ERROR, "USR-API   - send() failed, %s", strerror(errno));
    return RETURNerror;
  } else if (sbytes == 0) {
    LOG_TRACE(WARNING, "USR-API   - A signal was caught");
  } else {
    LOG_TRACE(INFO, "USR-API   - %d bytes sent "
              "to the user application layer", sbytes);
    LOG_DUMP(_user_api_send_buffer, sbytes);
  }

  return sbytes;
}
int user_api_send_data(int fd, int length)
{
  LOG_FUNC_IN;

  /* Sanity check */
  int sfd = user_api_get_fd();

  if (fd != sfd) {
    LOG_TRACE(ERROR, "USR-API   - Endpoint %d is not the one created for communication with the user application layer (%d)", fd, sfd);
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Send data to the user application layer */
  int sbytes = 0;

  if (length > 0) {
    sbytes = _user_api_send_data(length);
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
 ** Inputs:      fd:            File descriptor of the connection endpoint **
 **                             allocated by the system to communicate     **
 **                             with the user application layer            **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     Return:        None                                       **
 **              Others:        _user_api_id                               **
 **                                                                        **
 ***************************************************************************/
void user_api_close(int fd)
{
  LOG_FUNC_IN;

  /* Sanity check */
  int sfd = user_api_get_fd();

  if (fd != sfd) {
    LOG_TRACE(ERROR, "USR-API   - Endpoint %d is not the one created for communication with the user application layer (%d)", fd, sfd);
    LOG_FUNC_OUT;
    return;
  }

  /* Cleanup the connection endpoint */
  USER_API_CLOSE();
  _user_api_id.endpoint = NULL;

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
 **      Others:  _user_api_recv_buffer                      **
 **                                                                        **
 ** Outputs:   Return:  The number of AT commands succeessfully    **
 **       decoded                                    **
 **      Others:  _user_api_send_buffer, _user_data          **
 **                                                                        **
 ***************************************************************************/
int user_api_decode_data(int length)
{
  LOG_FUNC_IN;

  /* Parse the AT command line */
  LOG_TRACE(INFO, "USR-API   - Decode user data: %s", _user_api_recv_buffer);
  _user_data.n_cmd = at_command_decode(_user_api_recv_buffer, length,
                                       _user_data.cmd);

  if (_user_data.n_cmd > 0) {
    /* AT command data received from the user application layer
     * has been successfully decoded */
    LOG_TRACE(INFO, "USR-API   - %d AT command%s ha%s been successfully "
              "decoded", _user_data.n_cmd,
              (_user_data.n_cmd > 1) ? "s" : "",
              (_user_data.n_cmd > 1) ? "ve" : "s");
  } else {
    int bytes;

    /* Failed to decode AT command data received from the user
     * application layer; Return syntax error code message */
    LOG_TRACE(ERROR, "USR-API   - Syntax error: Failed to decode "
              "AT command data %s", _user_api_recv_buffer);

    /* Encode the syntax error code message */
    bytes = at_error_encode(_user_api_send_buffer, AT_ERROR_SYNTAX,
                            AT_ERROR_OPERATION_NOT_SUPPORTED);

    /* Send the syntax error code message */
    (void) _user_api_send_data(bytes);
  }

  LOG_FUNC_RETURN (_user_data.n_cmd);
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
 **      Others:  _user_api_send_buffer                      **
 **                                                                        **
 ***************************************************************************/
int user_api_encode_data(const void* data, int success_code)
{
  LOG_FUNC_IN;

  const at_response_t* user_data = (at_response_t*)(data);
  int bytes;

  /* Encode AT command error message */
  if (user_data->cause_code != AT_ERROR_SUCCESS) {
    bytes = at_error_encode(_user_api_send_buffer, AT_ERROR_CME,
                            user_data->cause_code);
  }
  /* Encode AT command response message */
  else {
    bytes = at_response_encode(_user_api_send_buffer, user_data);

    /* Add success result code */
    if ( (success_code) && (bytes != RETURNerror) ) {
      bytes += at_error_encode(&_user_api_send_buffer[bytes],
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
int user_api_emm_callback(Stat_t stat, tac_t tac, ci_t ci, AcT_t AcT,
                          const char* data, size_t size)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  if (size > 1) {
    /*
     * The list of available operators present in the network has to be
     * displayed to the user application
     */
    rc = user_ind_notify(USER_IND_PLMN, (void*)data, size);
  } else {
    user_indication_t ind;
    ind.notification.reg.status = stat;

    if (size > 0) {
      /* The UE's network registration status has changed */
      rc = user_ind_notify(USER_IND_REG, (void*)&ind, 0);
    }

    if (rc != RETURNerror) {
      /* The UE's location area has changed or,
       * the UE's network registration status has changed and
       * only location information notification is enabled */
      ind.notification.loc.tac = tac;
      ind.notification.loc.ci  = ci;
      ind.notification.loc.AcT = AcT;
      rc = user_ind_notify(USER_IND_LOC, (void*)&ind, 0);
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
int user_api_esm_callback(int cid, network_pdn_state_t state)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  user_indication_t ind;
  ind.notification.pdn.cid = cid;
  ind.notification.pdn.status = state;
  /* The status of the specified PDN connection has changed */
  rc = user_ind_notify(USER_IND_PDN, (void*)&ind, 0);

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
 **      Others:  _user_api_send_buffer, _user_api_id        **
 **                                                                        **
 ** Outputs:   Return:  The number of bytes sent when success;     **
 **       RETURNerror Otherwise                      **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _user_api_send(at_response_t* data)
{
  LOG_FUNC_IN;

  /* Encode AT command response message */
  int bytes = at_response_encode(_user_api_send_buffer, data);

  /* Send the AT command response message to the user application */
  if (bytes != RETURNerror) {
    bytes = _user_api_send_data(bytes);
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
static int _user_api_registration_handler(unsigned char id, const void* data,
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
  int bytes = _user_api_send(&at_response);

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
static int _user_api_location_handler(unsigned char id, const void* data,
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
  int bytes = _user_api_send(&at_response);

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
static int _user_api_network_handler(unsigned char id, const void* data,
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
  int bytes = _user_api_send(&at_response);

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
static int _user_api_pdn_connection_handler(unsigned char id, const void* data,
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
  int bytes = _user_api_send(&at_response);

  LOG_FUNC_RETURN (bytes);
}

