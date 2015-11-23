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
Source      nas_user.c

Version     0.1

Date        2012/03/09

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Maurel

Description NAS procedure functions triggered by the user

*****************************************************************************/


#include "nas_user.h"
#include "userDef.h"
#include "nas_log.h"
#include "memory.h"

#include "at_command.h"
#include "at_response.h"
#include "at_error.h"
#include "user_indication.h"
#include "nas_proc.h"
#include "user_api.h"

#include <string.h> // memset, strncpy, strncmp
#include <stdlib.h> // free

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * ---------------------------------------------------------------------
 *  Functions executed upon receiving AT command from the user
 * ---------------------------------------------------------------------
 */
static int _nas_user_proc_cgsn    (const at_command_t *data);
static int _nas_user_proc_cgmi    (const at_command_t *data);
static int _nas_user_proc_cgmm    (const at_command_t *data);
static int _nas_user_proc_cgmr    (const at_command_t *data);
static int _nas_user_proc_cimi    (const at_command_t *data);
static int _nas_user_proc_cfun    (const at_command_t *data);
static int _nas_user_proc_cpin    (const at_command_t *data);
static int _nas_user_proc_csq     (const at_command_t *data);
static int _nas_user_proc_cesq    (const at_command_t *data);
static int _nas_user_proc_cops    (const at_command_t *data);
static int _nas_user_proc_cgatt   (const at_command_t *data);
static int _nas_user_proc_creg    (const at_command_t *data);
static int _nas_user_proc_cgreg   (const at_command_t *data);
static int _nas_user_proc_cereg   (const at_command_t *data);
static int _nas_user_proc_cgdcont (const at_command_t *data);
static int _nas_user_proc_cgact   (const at_command_t *data);
static int _nas_user_proc_cmee    (const at_command_t *data);
static int _nas_user_proc_clck    (const at_command_t *data);
static int _nas_user_proc_cgpaddr (const at_command_t *data);
static int _nas_user_proc_cnum    (const at_command_t *data);
static int _nas_user_proc_clac    (const at_command_t *data);

/* NAS procedures applicable to AT commands */
typedef int (*_nas_user_procedure_t) (const at_command_t *);

static _nas_user_procedure_t _nas_user_procedure[AT_COMMAND_ID_MAX] = {
  NULL,
  _nas_user_proc_cgsn,    /* CGSN    */
  _nas_user_proc_cgmi,    /* CGMI    */
  _nas_user_proc_cgmm,    /* CGMM    */
  _nas_user_proc_cgmr,    /* CGMR    */
  _nas_user_proc_cimi,    /* CIMI    */
  _nas_user_proc_cfun,    /* CFUN    */
  _nas_user_proc_cpin,    /* CPIN    */
  _nas_user_proc_csq,     /* CSQ     */
  _nas_user_proc_cesq,    /* CESQ    */
  _nas_user_proc_clac,    /* CLAC    */
  _nas_user_proc_cmee,    /* CMEE    */
  _nas_user_proc_cnum,    /* CNUM    */
  _nas_user_proc_clck,    /* CLCK    */
  _nas_user_proc_cops,    /* COPS    */
  _nas_user_proc_creg,    /* CREG    */
  _nas_user_proc_cgatt,   /* CGATT   */
  _nas_user_proc_cgreg,   /* CGREG   */
  _nas_user_proc_cereg,   /* CEREG   */
  _nas_user_proc_cgdcont, /* CGDCONT */
  _nas_user_proc_cgact,   /* CGACT   */
  _nas_user_proc_cgpaddr, /* CGPADDR */
};

/*
 * Internal representation of data structure returned to the user
 * as the result of NAS procedure function call
 */
static at_response_t _nas_user_data = {};

/*
 * ---------------------------------------------------------------------
 *              Local UE context
 * ---------------------------------------------------------------------
 */
/*
 * MT SIM pending status (see ETSI TS 127 007 V10.6.0, Note 2)
 * Commands which interact with MT that are accepted when MT is pending SIM PIN,
 * SIM PUK, or PH-SIM are: +CGMI, +CGMM, +CGMR, +CGSN, D112; (emergency call),
 * +CPAS, +CFUN, +CPIN, +CPINR, +CDIS (read and test command only), and +CIND
 * (read and test command only).
*/
typedef enum {
  NAS_USER_READY, /* MT is not pending for any password   */
  NAS_USER_SIM_PIN,   /* MT is waiting SIM PIN to be given    */
  NAS_USER_SIM_PUK,   /* MT is waiting SIM PUK to be given    */
  NAS_USER_PH_SIM_PIN /* MT is waiting phone-to-SIM card
             * password to be given         */
} nas_user_sim_status;
static const char *_nas_user_sim_status_str[] = {
  "READY",
  "SIM PIN",
  "SIM PUK",
  "PH-SIM PIN"
};

/*
 * The local UE context
 */
static struct {
  /* Firmware version number          */
  const char *version;
  /* SIM pending status           */
  nas_user_sim_status sim_status;
  /* Level of functionality           */
  int fun;
} _nas_user_context;

/*
 * ---------------------------------------------------------------------
 *  UE parameters stored in the UE's non-volatile memory device
 * ---------------------------------------------------------------------
 */
static user_nvdata_t _nas_user_nvdata;

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_initialize()                                     **
 **                                                                        **
 ** Description: Initializes user internal data                            **
 **                                                                        **
 ** Inputs:  emm_cb:    Mobility Management indication callback    **
 **      esm_cb:    Session Management indication callback     **
 **          version:   Firmware version                           **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **          Others:    _nas_user_nvdata, _nas_user_context        **
 **                                                                        **
 ***************************************************************************/
void nas_user_initialize(emm_indication_callback_t emm_cb,
                         esm_indication_callback_t esm_cb, const char *version)
{
  LOG_FUNC_IN;

  /* Get UE's data pathname */
  char *path = memory_get_path(USER_NVRAM_DIRNAME, USER_NVRAM_FILENAME);

  if (path == NULL) {
    LOG_TRACE(ERROR, "USR-MAIN  - Failed to get UE's data pathname");
  }

  /* Get UE data stored in the non-volatile memory device */
  else {
    int rc = memory_read(path, &_nas_user_nvdata, sizeof(user_nvdata_t));

    if (rc != RETURNok) {
      LOG_TRACE(ERROR, "USR-MAIN  - Failed to read %s", path);
    }

    free(path);
  }

  _nas_user_context.version = version;
  _nas_user_context.sim_status = NAS_USER_SIM_PIN;
  _nas_user_context.fun = AT_CFUN_FUN_DEFAULT;

  /* Initialize the internal NAS processing data */
  nas_proc_initialize(emm_cb, esm_cb, _nas_user_nvdata.IMEI);

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:        nas_user_receive_and_process()                            **
 **                                                                        **
 ** Description: Receives and process messages from user application       **
 **                                                                        **
 ** Inputs:      fd:            File descriptor of the connection endpoint **
 **                             from which data have been received         **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     Return:        FALSE, TRUE                                **
 **                                                                        **
 ***************************************************************************/
int nas_user_receive_and_process(int *fd, char *message)
{
  LOG_FUNC_IN;

  int ret_code;
  int nb_command;
  int bytes;
  int i;

  if (message != NULL) {
    /* Set the message in receive buffer (Use to simulate reception of data from UserProcess) */
    bytes = user_api_set_data(message);
  } else {
    /* Read the user data message */
    bytes = user_api_read_data (*fd);

    if (bytes == RETURNerror) {
      /* Failed to read data from the user application layer;
       * exit from the receiving loop */
      LOG_TRACE (ERROR, "UE-MAIN   - "
                 "Failed to read data from the user application layer");
      LOG_FUNC_RETURN(TRUE);
    }
  }

  if (bytes == 0) {
    /* A signal was caught before any data were available */
    LOG_FUNC_RETURN(FALSE);
  }

  /* Decode the user data message */
  nb_command = user_api_decode_data (bytes);

  for (i = 0; i < nb_command; i++) {
    /* Get the user data to be processed */
    const void *data = user_api_get_data (i);

    if (data == NULL) {
      /* Failed to get user data at the given index;
       * go ahead and process the next user data */
      LOG_TRACE (ERROR, "UE-MAIN   - "
                 "Failed to get user data at index %d",
                 i);
      continue;
    }

    /* Process the user data message */
    ret_code = nas_user_process_data (data);

    if (ret_code != RETURNok) {
      /* The user data message has not been successfully
       * processed; cause code will be encoded and sent back
       * to the user */
      LOG_TRACE
      (WARNING, "UE-MAIN   - "
       "The user procedure call failed");
    }

    /* Send response to UserProcess (If not in simulated reception) */
    if (message == NULL) {
      /* Encode the user data message */
      bytes = user_api_encode_data (nas_user_get_data (), i == nb_command - 1);

      if (bytes == RETURNerror) {
        /* Failed to encode the user data message;
         * go ahead and process the next user data */
        continue;
      }

      /* Send the data message to the user */
      bytes = user_api_send_data (*fd, bytes);

      if (bytes == RETURNerror) {
        /* Failed to send data to the user application layer;
         * exit from the receiving loop */
        LOG_TRACE (ERROR, "UE-MAIN   - "
                   "Failed to send data to the user application layer");
        LOG_FUNC_RETURN(TRUE);
      }
    }
  }

  LOG_FUNC_RETURN(FALSE);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_process_data()                                   **
 **                                                                        **
 ** Description: Executes AT command operations received from the user and **
 **      calls applicable NAS procedure function.                  **
 **                                                                        **
 ** Inputs:  data:      Generic pointer to data structure that has **
 **             to be processed                            **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok if the command has been success-  **
 **             fully executed; RETURNerror otherwise      **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
int nas_user_process_data(const void *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNerror;

  if (data) {
    const at_command_t *user_data = (at_command_t *)(data);
    _nas_user_procedure_t nas_procedure;

    LOG_TRACE(INFO, "USR-MAIN  - Process %s AT command %d",
              (user_data->type == AT_COMMAND_ACT) ? "action" :
              (user_data->type == AT_COMMAND_SET) ? "set parameter" :
              (user_data->type == AT_COMMAND_GET) ? "get parameter" :
              (user_data->type == AT_COMMAND_TST) ? "test parameter"
              : "unknown type", user_data->id);

    /* Call NAS procedure applicable to the AT command */
    nas_procedure = _nas_user_procedure[user_data->id];

    if (nas_procedure != NULL) {
      ret_code = (*nas_procedure)(user_data);
    } else {
      /* AT command related to result format only */
      _nas_user_data.id = user_data->id;
      _nas_user_data.type = user_data->type;
      _nas_user_data.mask = user_data->mask;
      _nas_user_data.cause_code = AT_ERROR_SUCCESS;
      ret_code = RETURNok;
    }
  } else {
    LOG_TRACE(ERROR, "USR-MAIN  - Data to be processed is null");
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_get_data()                                       **
 **                                                                        **
 ** Description: Get a generic pointer to the internal representation of   **
 **      the data structure returned as the result of NAS proce-   **
 **      dure function call.                                       **
 **      Casting to the proper type is necessary before its usage. **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **          Return:    A generic pointer to the data structure    **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const void *nas_user_get_data(void)
{
  LOG_FUNC_IN;
  LOG_FUNC_RETURN ((void *) &_nas_user_data);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cgsn()                                      **
 **                                                                        **
 ** Description: Executes the AT CGSN command operations:                  **
 **          The AT CGSN command returns information text intended to  **
 **      permit the user to identify the individual Mobile         **
 **      Equipment to which it is connected to.                    **
 **      Typically, the text will consist of a single line contai- **
 **      ning the IMEI.                                            **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_nvdata                           **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cgsn(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cgsn_resp_t *cgsn = &_nas_user_data.response.cgsn;
  memset(cgsn, 0, sizeof(at_cgsn_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CGSN_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_ACT:
    /* Get the Product Serial Number Identification (IMEI) */
    strncpy(cgsn->sn, _nas_user_nvdata.IMEI,
            AT_RESPONSE_INFO_TEXT_SIZE);
    break;

  case AT_COMMAND_TST:
    /* Nothing to do */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CGSN command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cgmi()                                      **
 **                                                                        **
 ** Description: Executes the AT CGMI command operations:                  **
 **      The AT CGMI command returns information text intended to  **
 **      permit the user to identify the manufacturer of the Mobi- **
 **      le Equipment to which it is connected to.                 **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_nvdata                           **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cgmi(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cgmi_resp_t *cgmi = &_nas_user_data.response.cgmi;
  memset(cgmi, 0, sizeof(at_cgmi_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CGMI_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_ACT:
    /* Get the Manufacturer identifier */
    strncpy(cgmi->manufacturer, _nas_user_nvdata.manufacturer,
            AT_RESPONSE_INFO_TEXT_SIZE);
    break;

  case AT_COMMAND_TST:
    /* Nothing to do */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CGMI command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cgmm()                                      **
 **                                                                        **
 ** Description: Executes the AT CGMM command operations:                  **
 **      The AT CGMM command returns information text intended to  **
 **      permit the user to identify specific model of the Mobile  **
 **      Equipment to which it is connected to.                    **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_nvdata                           **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cgmm(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cgmm_resp_t *cgmm = &_nas_user_data.response.cgmm;
  memset(cgmm, 0, sizeof(at_cgmm_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CGMM_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_ACT:
    /* Get the Model identifier */
    strncpy(cgmm->model, _nas_user_nvdata.model,
            AT_RESPONSE_INFO_TEXT_SIZE);
    break;

  case AT_COMMAND_TST:
    /* Nothing to do */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CGMM command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cgmr()                                      **
 **                                                                        **
 ** Description: Executes the AT CGMR command operations:                  **
 **      The AT CGMR command returns information text intended to  **
 **      permit the user to identify the version, revision level   **
 **      or date, or other pertinent information of the Mobile     **
 **      Equipment to which it is connected to.                    **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cgmr(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cgmr_resp_t *cgmr = &_nas_user_data.response.cgmr;
  memset(cgmr, 0, sizeof(at_cgmr_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CGMR_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_ACT:
    /* Get the revision identifier */
    strncpy(cgmr->revision, _nas_user_context.version,
            AT_RESPONSE_INFO_TEXT_SIZE);
    break;

  case AT_COMMAND_TST:
    /* Nothing to do */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CGMR command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cimi()                                      **
 **                                                                        **
 ** Description: Executes the AT CIMI command operations:                  **
 **      The AT CIMI command returns information text intended to  **
 **      permit the user to identify the individual SIM card or    **
 **      active application in the UICC (GSM or USIM) which is     **
 **      attached to the Mobile Equipment to which it is connected **
 **      to.                                                       **
 **      Typically, the text will consist of a single line contai- **
 **      ning the IMSI.                                            **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_nvdata                           **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cimi(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cimi_resp_t *cimi = &_nas_user_data.response.cimi;
  memset(cimi, 0, sizeof(at_cimi_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CIMI_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_ACT:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /* Get the International Mobile Subscriber Identity (IMSI) */
    ret_code = nas_proc_get_imsi(cimi->IMSI);

    if (ret_code != RETURNok) {
      LOG_TRACE(ERROR, "USR-MAIN  - Failed to get IMSI number");
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    }

    break;

  case AT_COMMAND_TST:
    /* Nothing to do */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CIMI command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cfun()                                      **
 **                                                                        **
 ** Description: Executes the AT CFUN command operations:                  **
 **      The AT CFUN command is used to set the Mobile Equipment   **
 **      to different power consumption states.                    **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cfun(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cfun_resp_t *cfun = &_nas_user_data.response.cfun;
  memset(cfun, 0, sizeof(at_cfun_resp_t));

  int fun = AT_CFUN_FUN_DEFAULT;

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CFUN_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:

    /*
     * Set command selects the level of functionality in the MT
     */
    if (data->mask & AT_CFUN_RST_MASK) {
      if (data->command.cfun.rst == AT_CFUN_RST) {
        /* TODO: Reset the MT before setting it to <fun> power level */
      } else if (data->command.cfun.rst != AT_CFUN_NORST) {
        /* The value of the reset parameter is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <rst> parameter is not valid"
                  " (%d)", data->command.cfun.rst);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }
    }

    if (data->mask & AT_CFUN_FUN_MASK) {
      if ( (data->command.cfun.fun < AT_CFUN_MIN) ||
           (data->command.cfun.fun > AT_CFUN_MAX) ) {
        /* The value of the functionality level parameter
         * is not valid; return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <fun> parameter is not valid"
                  " (%d)", data->command.cfun.fun);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      /* Set to the selected parameter value */
      fun = data->command.cfun.fun;
    }

    switch (fun) {
    case AT_CFUN_MIN:
      /* TODO: Shutdown ??? */
      break;

    case AT_CFUN_FULL:
      /* Notify the NAS procedure call manager that the UE is
       * operational */
      ret_code = nas_proc_enable_s1_mode();
      break;

    default:
      /* TODO: Disable the radio ??? */
      break;
    }

    if (ret_code != RETURNerror) {
      /* Update the functionality level */
      _nas_user_context.fun = fun;
    }

    break;

  case AT_COMMAND_GET:
    /* Get the MT's functionality level */
    cfun->fun = _nas_user_context.fun;
    break;

  case AT_COMMAND_TST:
    /*
     * Test command returns values supported as a compound value
     */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CFUN command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cpin()                                      **
 **                                                                        **
 ** Description: Executes the AT CPIN command operations:                  **
 **      The AT CPIN command is used to enter the Mobile Equipment **
 **      passwords which are needed before any other functionality **
 **      can be operated.                                          **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_context, _nas_user_data          **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cpin(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cpin_resp_t *cpin = &_nas_user_data.response.cpin;
  memset(cpin, 0, sizeof(at_cpin_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CPIN_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:

    /*
     * Set command sends to the MT a password which is necessary
     * before it can be operated
     */
    if (_nas_user_context.sim_status == NAS_USER_SIM_PIN) {
      /* The MT is waiting for PIN password; check the PIN code */
      if (strncmp(_nas_user_nvdata.PIN,
                  data->command.cpin.pin, USER_PIN_SIZE) != 0) {
        /* The PIN code is NOT matching; return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - PIN code is not correct "
                  "(%s)", data->command.cpin.pin);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PASSWD;
        ret_code = RETURNerror;
      } else {
        /* The PIN code is matching; update the user's PIN
         * pending status */
        _nas_user_context.sim_status = NAS_USER_READY;
      }
    } else {
      /* The MT is NOT waiting for PIN password;
       * return an error message */
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_ALLOWED;
      ret_code = RETURNerror;
    }

    break;

  case AT_COMMAND_GET:
    /*
     * Read command returns an alphanumeric string indicating
     * whether some password is required or not.
     */
    strncpy(cpin->code,
            _nas_user_sim_status_str[_nas_user_context.sim_status],
            AT_CPIN_RESP_SIZE);
    break;

  case AT_COMMAND_TST:
    /* Nothing to do */
    break;

  default:
    /* Other types of AT CPIN command are not valid */
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CPIN command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_csq()                                       **
 **                                                                        **
 ** Description: Executes the AT CSQ command operations:                   **
 **      The AT CSQ command returns received signal strength       **
 **      indication and channel bit error rate from the Mobile     **
 **      Equipment.                                                **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_csq(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_csq_resp_t *csq = &_nas_user_data.response.csq;
  memset(csq, 0, sizeof(at_csq_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CSQ_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_ACT:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /*
     * Execution command returns received signal strength indication
     * and channel bit error rate <ber> from the MT
     */
    csq->rssi = AT_CESQ_RSSI_UNKNOWN;
    csq->ber  = AT_CESQ_BER_UNKNOWN;
    break;

  case AT_COMMAND_TST:
    /*
     * Test command returns values supported as a compound value
     */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CSQ command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cesq()                                      **
 **                                                                        **
 ** Description: Executes the AT CESQ command operations:                  **
 **      The AT CESQ command returns received signal quality para- **
 **      meters.                                                   **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cesq(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cesq_resp_t *cesq = &_nas_user_data.response.cesq;
  memset(cesq, 0, sizeof(at_cesq_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CESQ_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_ACT:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /*
     * Execution command returns received signal quality parameters
     */
    cesq->rssi = AT_CESQ_RSSI_UNKNOWN;
    cesq->ber  = AT_CESQ_BER_UNKNOWN;
    cesq->rscp = AT_CESQ_RSCP_UNKNOWN;
    cesq->ecno = AT_CESQ_ECNO_UNKNOWN;
    ret_code = nas_proc_get_signal_quality(&cesq->rsrq, &cesq->rsrp);
    break;

  case AT_COMMAND_TST:
    /*
     * Test command returns values supported as a compound value
     */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CESQ command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cops()                                      **
 **                                                                        **
 ** Description: Executes the AT COPS command operations:                  **
 **      The AT COPS command forces an attempt to select and       **
 **      register the GSM/UMTS/EPS network operator using the      **
 **      SIM/USIM card installed in the currently selected card    **
 **      slot.                                                     **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cops(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cops_resp_t *cops = &_nas_user_data.response.cops;
  memset(cops, 0, sizeof(at_cops_resp_t));

  static int read_format = AT_COPS_FORMAT_DEFAULT;

  int mode = AT_COPS_MODE_DEFAULT;
  int format = AT_COPS_FORMAT_DEFAULT;
  int AcT = AT_COPS_ACT_DEFAULT;
  char oper_buffer[NET_FORMAT_MAX_SIZE], *oper = oper_buffer;
  memset(oper, 0, NET_FORMAT_MAX_SIZE);

  int oper_is_selected;

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_NO_PARAM;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /*
     * Set command forces an attempt to select and register the
     * GSM/UMTS/EPS network operator using the SIM/USIM card
     * installed in the currently selected card slot.
     */
    if (data->mask & AT_COPS_MODE_MASK) {
      mode = data->command.cops.mode;

      switch (mode) {
      case AT_COPS_AUTO:
        /*
         * Register in automatic mode
         */
        break;

      case AT_COPS_MANUAL:

        /*
         * Register in manual mode
         */

        /** break is intentionaly missing */

      case AT_COPS_MANAUTO:

        /*
         * Register in manual/automatic mode
         */

        /* <oper> field shall be present */
        if ( !(data->mask & AT_COPS_OPER_MASK) ) {
          LOG_TRACE(ERROR, "USR-MAIN  - <oper> parameter is not present");
          _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
          ret_code = RETURNerror;
          break;
        }

        /* <format> field shall be present */
        if ( !(data->mask & AT_COPS_FORMAT_MASK) ) {
          LOG_TRACE(ERROR, "USR-MAIN  - <format> parameter is not present");
          _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
          ret_code = RETURNerror;
          break;
        }

        /* Set the operator's representation format */
        if ( (data->command.cops.format < AT_COPS_FORMAT_MIN) ||
             (data->command.cops.format > AT_COPS_FORMAT_MAX) ) {
          /* The value of <format> field is not valid */
          LOG_TRACE(ERROR, "USR-MAIN  - <format> parameter is not valid (%d)",
                    data->command.cops.format);
          _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
          ret_code = RETURNerror;
          break;
        }

        read_format = format = data->command.cops.format;

        /* Set the access technology */
        if (data->mask & AT_COPS_ACT_MASK) {
          if ( (data->command.cops.AcT < AT_COPS_ACT_MIN) ||
               (data->command.cops.AcT > AT_COPS_ACT_MAX) ) {
            /* The value of <AcT> field is not valid */
            LOG_TRACE(ERROR, "USR-MAIN  - <AcT> parameter is not valid (%d)",
                      data->command.cops.AcT);
            _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
            ret_code = RETURNerror;
            break;
          }

          AcT = data->command.cops.AcT;
        }

        break;

      case AT_COPS_DEREG:
        /*
         * Deregister from network
         */
        /* Nothing to do there */
        break;

      case AT_COPS_FORMAT:

        /*
         * Set only <format> for read command +COPS?
         */
        if (data->mask & AT_COPS_FORMAT_MASK) {
          if ( (data->command.cops.format < AT_COPS_FORMAT_MIN) ||
               (data->command.cops.format > AT_COPS_FORMAT_MAX) ) {
            /* The value of <format> field is not valid */
            LOG_TRACE(ERROR, "USR-MAIN  - <format> parameter is not valid (%d)",
                      data->command.cops.format);
            _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
            ret_code = RETURNerror;
            break;
          }

          /* Set to the selected representation format */
          read_format = data->command.cops.format;
        } else {
          /* Format is not present; set to default */
          read_format = AT_COPS_FORMAT_DEFAULT;
        }

        break;

      default:
        LOG_TRACE(ERROR, "USR-MAIN  - <mode> parameter is not supported (%d)", mode);
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
        ret_code = RETURNerror;
        break;
      }
    }

    /*
     * Performs network registration
     */
    if (ret_code != RETURNerror) {
      if (mode == AT_COPS_DEREG) {
        /* Force an attempt to deregister from the network */
        ret_code = nas_proc_deregister();

        if (ret_code != RETURNok) {
          LOG_TRACE(ERROR, "USR-MAIN  - Network deregistration failed");
          _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
          break;
        }
      } else if (mode != AT_COPS_FORMAT) {
        /* Force an attempt to automatically/manualy select
         * and register the GSM/UMTS/EPS network operator */
        ret_code = nas_proc_register(mode, format,
                                     &data->command.cops.plmn, AcT);

        if (ret_code != RETURNok) {
          LOG_TRACE(ERROR, "USR-MAIN  - Network registration failed (<mode>=%d)", mode);
          _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
          break;
        }
      }
    }

    break;

  case AT_COMMAND_GET:
    /*
     * Read command returns the current mode, the currently selected
     * network operator and the current Access Technology.
     */

    /* Get the current network registration data */
    ret_code = nas_proc_get_reg_data(&mode,
                                     &oper_is_selected, read_format,
                                     &cops->get.plmn, &cops->get.AcT);

    if (ret_code != RETURNok) {
      LOG_TRACE(ERROR, "USR-MAIN  - Failed to get registration data (<mode>=%d)",
                mode);
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      break;
    }

    /* Set the network selection operating mode */
    if (mode == NET_PLMN_AUTO) {
      cops->get.mode = AT_COPS_AUTO;
    } else if (mode == NET_PLMN_MANUAL) {
      cops->get.mode = AT_COPS_MANUAL;
    } else {
      cops->get.mode = AT_COPS_MANAUTO;
    }

    /* Set optional parameter bitmask */
    if (oper_is_selected) {
      cops->get.format = read_format;
      _nas_user_data.mask |= (AT_COPS_RESP_FORMAT_MASK |
                              AT_COPS_RESP_OPER_MASK);

      if (cops->get.AcT != NET_ACCESS_UNAVAILABLE) {
        _nas_user_data.mask |= AT_COPS_RESP_ACT_MASK;
      }
    }

    break;

  case AT_COMMAND_TST:
    /*
     * Test command returns a set of parameters, each representing
     * an operator present in the network.
     */
    cops->tst.size = nas_proc_get_oper_list(&cops->tst.data);
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+COPS command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cgatt()                                     **
 **                                                                        **
 ** Description: Executes the AT CGATT command operations:                 **
 **      The AT CGATT command is used to attach the MT to,         **
 **      or detach the MT from, the EPS service.                   **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cgatt(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cgatt_resp_t *cgatt = &_nas_user_data.response.cgatt;
  memset(cgatt, 0, sizeof(at_cgatt_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CGATT_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /*
     * The execution command is used to attach the MT to, or detach the
     * MT from, the EPS service
     */
    if (data->mask & AT_CGATT_STATE_MASK) {
      if ( (data->command.cgatt.state < AT_CGATT_STATE_MIN) ||
           (data->command.cgatt.state > AT_CGATT_STATE_MAX) ) {
        /* The value of the EPS attachment code is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <state> parameter is not valid (%d)",
                  data->command.cgatt.state);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      /*
       * Perform EPS service attach/detach
       */
      ret_code = RETURNerror;

      if (data->command.cgatt.state == AT_CGATT_ATTACHED) {
        ret_code = nas_proc_attach();
      } else if (data->command.cgatt.state == AT_CGATT_DETACHED) {
        ret_code = nas_proc_detach(FALSE);
      }

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR, "USR-MAIN  - Failed to attach/detach "
                  "to/from EPS service (<state>=%d)",
                  data->command.cgatt.state);
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }
    }

    break;

  case AT_COMMAND_GET:

    /*
     * Read command returns the current EPS service state.
     */
    if (nas_proc_get_attach_status() != TRUE) {
      cgatt->state = AT_CGATT_DETACHED;
    } else {
      cgatt->state = AT_CGATT_ATTACHED;
    }

    break;

  case AT_COMMAND_TST:
    /*
     * Test command is used for requesting information on the supported
     * EPS service states
     */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CGATT command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_creg()                                      **
 **                                                                        **
 ** Description: Executes the AT CREG command operations:                  **
 **      The AT CREG command returns the Mobile Equipment's        **
 **      circuit mode network registration status and optionnally  **
 **      location information in GERA/UTRA/E-UTRA Network.         **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_creg(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_creg_resp_t *creg = &_nas_user_data.response.creg;
  memset(creg, 0, sizeof(at_creg_resp_t));

  static int n = AT_CREG_N_DEFAULT;

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_NO_PARAM;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /*
     * The set command controls the presentation of an unsolicited
     * result code when there is a change in the MT's circuit mode
     * network registration status, or when there is a change of the
     * network cell in GERAN/UTRAN/E-UTRAN.
     */
    if (data->mask & AT_CREG_N_MASK) {
      if ( (data->command.creg.n < AT_CREG_N_MIN) ||
           (data->command.creg.n > AT_CREG_N_MAX) ) {
        /* The value of the unsolicited result code is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <n> parameter is not valid"
                  " (%d)", data->command.creg.n);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      /* Set to the selected parameter value */
      n = data->command.creg.n;
    } else {
      /* The numeric parameter is not present; set to default */
      n = AT_CREG_N_DEFAULT;
    }

    /* Disable/Enable network logging */
    switch (n) {
    case AT_CREG_OFF:
      /* Disable logging of network registration status */
      ret_code = user_ind_deregister(USER_IND_REG);

      if (ret_code != RETURNerror) {
        /* Disable logging of location information */
        ret_code = user_ind_deregister(USER_IND_LOC);
      }

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR,
                  "USR-MAIN  - Failed to disable logging of network notification");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }

      break;

    case AT_CREG_BOTH:

      /* Location Area Code (lac) is not available */
    case AT_CREG_ON:
      /* Enable logging of the MT's circuit mode network
       * registration status in GERAN/UTRAN/E_UTRAN */
      ret_code = user_ind_register(USER_IND_REG, AT_CREG, NULL);

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR, "USR-MAIN  - Failed to enable logging of registration status");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }

      break;

    default:
      break;
    }

    break;

  case AT_COMMAND_GET:
    /*
     * The read command returns the status of result code presentation,
     * and indication of whether the network has currently indicated
     * the registration of the MT, and available location information
     * elements when the MT is registered in the network.
     */
    creg->n = n;

    switch (n) {
    case AT_CREG_BOTH:

      /* Location Area Code (lac) is not available */
    case AT_CREG_OFF:
    case AT_CREG_ON:
      /* Get network registration status */
      ret_code = nas_proc_get_reg_status(&creg->stat);

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR, "USR-MAIN  - Failed to get registration status");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }

      break;

    default:
      break;
    }

    break;

  case AT_COMMAND_TST:
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CREG command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cgreg()                                     **
 **                                                                        **
 ** Description: Executes the AT CGREG command operations:                 **
 **      The AT CGREG command returns the Mobile Equipment's GPRS  **
 **      network registration status and optionnally location      **
 **      information in GERA/UTRA Network.                         **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cgreg(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cgreg_resp_t *cgreg = &_nas_user_data.response.cgreg;
  memset(cgreg, 0, sizeof(at_cgreg_resp_t));

  static int n = AT_CGREG_N_DEFAULT;

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_NO_PARAM;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /*
     * The set command controls the presentation of an unsolicited
     * result code when there is a change in the MT's GPRS network
     * registration status, or when there is a change of the network
     * cell in GERAN/UTRAN.
     */
    if (data->mask & AT_CGREG_N_MASK) {
      if ( (data->command.cgreg.n < AT_CGREG_N_MIN) ||
           (data->command.cgreg.n > AT_CGREG_N_MAX) ) {
        /* The value of the unsolicited result code is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <n> parameter is not valid"
                  " (%d)", data->command.cgreg.n);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      /* Set to the selected parameter value */
      n = data->command.cgreg.n;
    } else {
      /* The numeric parameter is not present; set to default */
      n = AT_CGREG_N_DEFAULT;
    }

    /* Disable/Enable network logging */
    switch (n) {
    case AT_CGREG_OFF:
      /* Disable logging of network registration status */
      ret_code = user_ind_deregister(USER_IND_REG);

      if (ret_code != RETURNerror) {
        /* Disable logging of location information */
        ret_code = user_ind_deregister(USER_IND_LOC);
      }

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR,
                  "USR-MAIN  - Failed to disable logging of network notification");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }

      break;

    case AT_CGREG_BOTH:

      /* Location Area Code (lac) is not available */
    case AT_CGREG_ON:
      /* Enable logging of the MT's GPRS network registration
       * status in GERAN/UTRAN/E_UTRAN */
      ret_code = user_ind_register(USER_IND_REG, AT_CGREG, NULL);

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR, "USR-MAIN  - Failed to enable logging of registration status");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }

      break;

    default:
      break;
    }

    break;

  case AT_COMMAND_GET:
    /*
     * The read command returns the status of result code presentation,
     * and indication of whether the network has currently indicated
     * the registration of the MT, and available location information
     * elements when the MT is registered in the network.
     */
    cgreg->n = n;

    switch (n) {
    case AT_CGREG_BOTH:

      /* Location Area Code (lac) is not available */
    case AT_CGREG_OFF:
    case AT_CGREG_ON:
      /* Get network registration status */
      ret_code = nas_proc_get_reg_status(&cgreg->stat);

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR, "USR-MAIN  - Failed to get registration status");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }

      break;

    default:
      break;
    }

    break;

  case AT_COMMAND_TST:
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CGREG command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cereg()                                     **
 **                                                                        **
 ** Description: Executes the AT CEREG command operations:                 **
 **      The AT CEREG command returns the Mobile Equipment's EPS   **
 **      services registration status and optionnally location     **
 **      information in E-UTRA Network.                            **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cereg(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cereg_resp_t *cereg = &_nas_user_data.response.cereg;
  memset(cereg, 0, sizeof(at_cereg_resp_t));

  static int n = AT_CEREG_N_DEFAULT;

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_NO_PARAM;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /*
     * The set command controls the presentation of an unsolicited
     * result code when there is a change in the MT's EPS network
     * registration status, or when there is a change of the network
     * cell in E-UTRAN.
     */
    if (data->mask & AT_CEREG_N_MASK) {
      if ( (data->command.cereg.n < AT_CEREG_N_MIN) ||
           (data->command.cereg.n > AT_CEREG_N_MAX) ) {
        /* The value of the unsolicited result code is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <n> parameter is not valid"
                  " (%d)",  data->command.cereg.n);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      /* Set to the selected parameter value */
      n = data->command.cereg.n;
    } else {
      /* The numeric parameter is not present; set to default */
      n = AT_CEREG_N_DEFAULT;
    }

    /* Disable/Enable network logging */
    switch (n) {
    case AT_CEREG_OFF:
      /* Disable logging of network registration status */
      ret_code = user_ind_deregister(USER_IND_REG);

      if (ret_code != RETURNerror) {
        /* Disable logging of location information */
        ret_code = user_ind_deregister(USER_IND_LOC);
      }

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR,
                  "USR-MAIN  - Failed to disable logging of network notification");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }

      break;

    case AT_CEREG_BOTH:
      /* Enable logging of the location information in E-UTRAN */
      ret_code = user_ind_register(USER_IND_LOC, AT_CEREG, NULL);

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR, "USR-MAIN  - Failed to enable logging of location information");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }

      break;

    case AT_CEREG_ON:
      /* Enable logging of the MT's EPS network registration
       * status in E-UTRAN */
      ret_code = user_ind_register(USER_IND_REG, AT_CEREG, NULL);

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR, "USR-MAIN  - Failed to enable logging of registration status");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }

      break;

    default:
      break;
    }

    break;

  case AT_COMMAND_GET:
    /*
     * The read command returns the status of result code presentation,
     * and indication of whether the network has currently indicated
     * the registration of the MT, and available location information
     * elements when the MT is registered in the network.
     */
    cereg->n = n;

    switch (n) {
    case AT_CEREG_BOTH:
      /* Get EPS location information  */
      ret_code = nas_proc_get_loc_info(cereg->tac, cereg->ci,
                                       &cereg->AcT);

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR, "USR-MAIN  - Failed to get location information");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
        break;
      }

      if (cereg->tac[0] != 0) {
        _nas_user_data.mask |= (AT_CEREG_RESP_TAC_MASK |
                                AT_CEREG_RESP_CI_MASK);

        if (cereg->AcT != NET_ACCESS_UNAVAILABLE) {
          _nas_user_data.mask |= (AT_CEREG_RESP_ACT_MASK);
        }
      }

      /** break is intentionaly missing */

    case AT_CEREG_OFF:
    case AT_CEREG_ON:
      /* Get network registration status */
      ret_code = nas_proc_get_reg_status(&cereg->stat);

      if (ret_code != RETURNok) {
        LOG_TRACE(ERROR, "USR-MAIN  - Failed to get registration status");
        _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      }

      break;

    default:
      break;
    }

    break;

  case AT_COMMAND_TST:
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CEREG command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cgdcont()                                   **
 **                                                                        **
 ** Description: Executes the AT CGDCONT command operations:               **
 **      The AT CGDCONT command specifies PDP context parameter    **
 **      values for a PDP context identified by the local context  **
 **      identification parameter (cid).                           **
 **                                                                        **
 **      There is a 1 to 1 mapping between EPS bearer context and  **
 **      PDP context. Therefore a PDP context used for UMTS/GPRS   **
 **      designates a PDN connection and its associated EPS de-    **
 **      fault bearer and traffic flows in EPS.                    **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cgdcont(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cgdcont_get_t *cgdcont = &_nas_user_data.response.cgdcont.get;
  memset(cgdcont, 0, sizeof(at_cgdcont_resp_t));

  int cid = AT_CGDCONT_CID_DEFAULT;
  int pdn_type = NET_PDN_TYPE_IPV4;
  const char *apn = NULL;
  int ipv4_addr_allocation = AT_CGDCONT_IPV4_DEFAULT;
  int emergency = AT_CGDCONT_EBS_DEFAULT;
  int p_cscf = AT_CGDCONT_PCSCF_DEFAULT;
  int im_cn_signalling = AT_CGDCONT_IM_CM_DEFAULT;
  int reset_pdn = TRUE;

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_NO_PARAM;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /*
     * Set command specifies PDN connection and its default
     * EPS bearer context parameter values
     */
    if (data->mask & AT_CGDCONT_CID_MASK) {
      if (data->command.cgdcont.cid < AT_CGDCONT_CID_MIN) {
        /* The value of the PDP context identifier is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <cid> parameter is not valid"
                  " (%d)", data->command.cgdcont.cid);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      cid = data->command.cgdcont.cid;
    }

    if (data->mask & AT_CGDCONT_PDP_TYPE_MASK) {
      if (strcmp(data->command.cgdcont.PDP_type, "IP") == 0) {
        pdn_type = NET_PDN_TYPE_IPV4;
      } else if (strcmp(data->command.cgdcont.PDP_type, "IPV6") == 0) {
        pdn_type = NET_PDN_TYPE_IPV6;
      } else if (strcmp(data->command.cgdcont.PDP_type,"IPV4V6") == 0) {
        pdn_type = NET_PDN_TYPE_IPV4V6;
      } else {
        /* The value of the PDP type is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <PDN_type> parameter is not "
                  "valid (%s)", data->command.cgdcont.PDP_type);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      reset_pdn = FALSE;
    }

    if (data->mask & AT_CGDCONT_APN_MASK) {
      apn = data->command.cgdcont.APN;
    }

    if (data->mask & AT_CGDCONT_D_COMP_MASK) {
      if ( (data->command.cgdcont.d_comp < AT_CGDCONT_D_COMP_MIN) ||
           (data->command.cgdcont.d_comp > AT_CGDCONT_D_COMP_MAX) ) {
        /* The value of the PDP data compression parameter is
         * not valid; return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <d_comp> parameter is not "
                  "valid (%d)", data->command.cgdcont.d_comp);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      /* Not supported: Applicable for SNDCP only */
    }

    if (data->mask & AT_CGDCONT_H_COMP_MASK) {
      if ( (data->command.cgdcont.h_comp < AT_CGDCONT_H_COMP_MIN) ||
           (data->command.cgdcont.h_comp > AT_CGDCONT_H_COMP_MAX) ) {
        /* The value of the PDP header compression parameter is
         * not valid; return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <h_comp> parameter is not "
                  "valid (%d)", data->command.cgdcont.h_comp);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      /* Not supported */
    }

    if (data->mask & AT_CGDCONT_IPV4ADDRALLOC_MASK) {
      if ( (data->command.cgdcont.IPv4AddrAlloc < AT_CGDCONT_IPV4_MIN)
           || (data->command.cgdcont.IPv4AddrAlloc > AT_CGDCONT_IPV4_MAX) ) {
        /* The value of the IPv4 address allocation parameter is
         * not valid; return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <IPv4AddrAlloc> parameter "
                  "is not valid (%d)",
                  data->command.cgdcont.IPv4AddrAlloc);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      ipv4_addr_allocation = data->command.cgdcont.IPv4AddrAlloc;
    }

    if (data->mask & AT_CGDCONT_EMERGECY_INDICATION_MASK) {
      if ( (data->command.cgdcont.emergency_indication < AT_CGDCONT_EBS_MIN)
           || (data->command.cgdcont.emergency_indication > AT_CGDCONT_EBS_MAX) ) {
        /* The value of the emergency indication parameter  is
         * not valid; return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <emergency indication> "
                  "parameter is not valid (%d)",
                  data->command.cgdcont.emergency_indication);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      emergency = data->command.cgdcont.emergency_indication;
    }

    if (data->mask & AT_CGDCONT_P_CSCF_DISCOVERY_MASK) {
      if ( (data->command.cgdcont.P_CSCF_discovery < AT_CGDCONT_PCSCF_MIN)
           || (data->command.cgdcont.P_CSCF_discovery > AT_CGDCONT_PCSCF_MAX) ) {
        /* The value of the P-CSCF address discovery parameter  is
         * not valid; return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <P-CSCF_discovery> "
                  "parameter is not valid (%d)",
                  data->command.cgdcont.P_CSCF_discovery);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      p_cscf = data->command.cgdcont.P_CSCF_discovery;
    }

    if (data->mask & AT_CGDCONT_IM_CN_SIGNALLING_FLAG_IND_MASK) {
      if ( (data->command.cgdcont.IM_CN_Signalling_Flag_Ind < AT_CGDCONT_IM_CM_MIN)
           || (data->command.cgdcont.IM_CN_Signalling_Flag_Ind > AT_CGDCONT_IM_CM_MAX) ) {
        /* The value of the IM CN subsystem-related signalling
         * support indication is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <IM_CN_Signalling_Flag_Ind> "
                  "parameter is not valid (%d)",
                  data->command.cgdcont.IM_CN_Signalling_Flag_Ind);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      im_cn_signalling = data->command.cgdcont.IM_CN_Signalling_Flag_Ind;
    }

    /*
     * Setup PDN context
     */
    if (reset_pdn) {
      /* A special form of the set command, +CGDCONT=<cid> causes
       * the values for context number <cid> to become undefined */
      ret_code = nas_proc_reset_pdn(cid);
    } else {
      /* Define a new PDN connection */
      ret_code = nas_proc_set_pdn(cid, pdn_type, apn,
                                  ipv4_addr_allocation, emergency,
                                  p_cscf, im_cn_signalling);
    }

    if (ret_code != RETURNok) {
      LOG_TRACE(ERROR, "USR-MAIN  - Failed to setup PDN connection "
                "(<cid>=%d)", data->command.cgdcont.cid);
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    }

    break;

  case AT_COMMAND_GET:
    /*
     * Read command returns the current settings for each
     * defined PDN connection/default EPS bearer context
     */
    cgdcont->n_pdns = nas_proc_get_pdn_param(cgdcont->cid,
                      cgdcont->PDP_type,
                      cgdcont->APN,
                      AT_CGDCONT_RESP_SIZE);

    if (cgdcont->n_pdns == 0) {
      LOG_TRACE(ERROR, "USR-MAIN  - No any PDN context is defined");
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    }

    break;

  case AT_COMMAND_TST:
    /*
     * Test command returns values supported as a compound value
     */
  {
    /* Get the maximum value of a PDN context identifier */
    int cid_max = nas_proc_get_pdn_range();

    if (cid_max > AT_CGDCONT_RESP_SIZE) {
      /* The range is defined by the user interface */
      _nas_user_data.response.cgdcont.tst.n_cid =
        AT_CGDCONT_RESP_SIZE;
    } else {
      /* The range is defined by the ESM sublayer application */
      _nas_user_data.response.cgdcont.tst.n_cid = cid_max;
    }
  }
  break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CGDCONT command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cgact()                                     **
 **                                                                        **
 ** Description: Executes the AT CGACT command operations:                 **
 **      The AT CGACT command is used to activate or deactivate    **
 **      the specified PDP context(s) or PDN/EPS bearer context(s) **
 **      for E-UTRAN                                               **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cgact(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cgact_resp_t *cgact = &_nas_user_data.response.cgact;
  memset(cgact, 0, sizeof(at_cgact_resp_t));

  int cid = -1;
  int state = AT_CGACT_STATE_DEFAULT;

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CGACT_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /*
     * The execution command is used to activate or deactivate
     * the specified PDN/EPS bearer context(s).
     */
    if (data->mask & AT_CGACT_STATE_MASK) {
      if ( (data->command.cgact.state < AT_CGACT_STATE_MIN) ||
           (data->command.cgact.state > AT_CGACT_STATE_MAX) ) {
        /* The value of the PDP context activation status is
         * not valid; return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <state> parameter is "
                  "not valid (%d)",  data->command.cgact.state);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      state = data->command.cgact.state;
    }

    if (data->mask & AT_CGACT_CID_MASK) {
      if (data->command.cgact.cid < AT_CGACT_CID_MIN) {
        /* The value of the PDP context identifier is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <cid> parameter is "
                  "not valid (%d)",  data->command.cgact.cid);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      cid = data->command.cgact.cid;
    }

    /*
     * Activate/Deactivate PDN context
     */
    ret_code = RETURNerror;

    if (state == AT_CGACT_DEACTIVATED) {
      ret_code = nas_proc_deactivate_pdn(cid);
    } else if (state == AT_CGACT_ACTIVATED) {
      ret_code = nas_proc_activate_pdn(cid);
    }

    if (ret_code != RETURNok) {
      LOG_TRACE(ERROR, "USR-MAIN  - Failed to %s PDN context "
                "(<state>=%d,<cid>=%d)",
                (state != AT_CGACT_ACTIVATED)? "deactivate" :
                "activate", state, cid);
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    }

    break;

  case AT_COMMAND_GET:
    /*
     * The read command returns the current activation states for
     * all the defined PDN/EPS bearer contexts
     */
    cgact->n_pdns = nas_proc_get_pdn_status(cgact->cid, cgact->state,
                                            AT_CGACT_RESP_SIZE);

    if (cgact->n_pdns == 0) {
      LOG_TRACE(ERROR, "USR-MAIN  - No any PDN context is defined");
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    }

    break;

  case AT_COMMAND_TST:
    /*
     * The test command is used for requesting information on the
     * supported PDN/EPS bearer context activation states.
     */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CGACT command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cmee()                                      **
 **                                                                        **
 ** Description: Executes the AT CMEE command operations:                  **
 **      The AT CMEE command disables or enables the use of final  **
 **      result code +CME ERROR: <err> as an indication of an er-  **
 **      ror relating to the functionality of the MT.              **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cmee(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cmee_resp_t *cmee = &_nas_user_data.response.cmee;
  memset(cmee, 0, sizeof(at_cmee_resp_t));

  int n = AT_CMEE_N_DEFAULT;

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CMEE_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_ACT:    /* ATV0, ATV1 response format commands */
  case AT_COMMAND_SET:

    /*
     * The set command controls the presentation of final result code
     * +CME ERROR: <err> as an indication of an error relating to the
     * functionality of the MT.
     */
    if (data->mask & AT_CMEE_N_MASK) {
      if ( (data->command.cmee.n < AT_CMEE_N_MIN) ||
           (data->command.cmee.n > AT_CMEE_N_MAX) ) {
        /* The value of the numeric parameter is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <n> parameter is not valid"
                  " (%d)", data->command.cmee.n);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      /* Set to the selected parameter value */
      n = data->command.cmee.n;
    }

    /* Disable/Enable final result code logging */
    switch (n) {
    case AT_CMEE_OFF:
      /* Disable logging of final result code */
      at_error_set_format(AT_ERROR_OFF);
      break;

    case AT_CMEE_NUMERIC:
      /* Enable logging of numeric final result code */
      at_error_set_format(AT_ERROR_NUMERIC);
      break;

    case AT_CMEE_VERBOSE:
      /* Enable logging of verbose final result code */
      at_error_set_format(AT_ERROR_VERBOSE);
      break;

    default:
      break;
    }

    break;

  case AT_COMMAND_GET:
    /*
     * Read command returns the status of the final result code
     * presentation.
     */
    n = at_error_get_format();
    cmee->n = ( (n == AT_ERROR_OFF)? AT_CMEE_OFF :
                (n == AT_ERROR_NUMERIC)? AT_CMEE_NUMERIC :
                (n == AT_ERROR_VERBOSE)? AT_CMEE_VERBOSE : RETURNerror);

    if (cmee->n == RETURNerror) {
      LOG_TRACE(ERROR, "USR-MAIN  - Failed to get format of the final result code");
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    }

    break;

  case AT_COMMAND_TST:
    /*
     * Test command returns values supported as a compound value
     */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CMEE command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_clck()                                      **
 **                                                                        **
 ** Description: Executes the AT CLCK command operations:                  **
 **      The AT CLCK command locks, unlocks or interrogates a MT   **
 **      or a network facility.                                    **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_clck(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_clck_resp_t *clck = &_nas_user_data.response.clck;
  memset(clck, 0, sizeof(at_clck_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CLCK_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:

    /*
     * Execution command locks, unlocks or returns status of a network
     * facility
     */

    /* Check facility parameter */
    if (strncmp(data->command.clck.fac, AT_CLCK_SC,
                AT_CLCK_FAC_SIZE) != 0) {
      /* Facilities other than SIM is not supported */
      LOG_TRACE(ERROR, "USR-MAIN  - Facility is not supported (%s)",
                data->command.clck.fac);
      _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
      ret_code = RETURNerror;
      break;
    }

    /* Check password parameter */
    if (data->mask & AT_CLCK_PASSWD_MASK) {
      /* Check the PIN code */
      if (strncmp(_nas_user_nvdata.PIN,
                  data->command.clck.passwd, USER_PIN_SIZE) != 0) {
        /* The PIN code is NOT matching; return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - Password is not correct "
                  "(%s)", data->command.clck.passwd);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PASSWD;
        ret_code = RETURNerror;
        break;
      }
    }

    /* Execute the command with specified mode of operation */
    switch (data->command.clck.mode) {
    case AT_CLCK_UNLOCK:

      /* unlock facility */
      if ( !(data->mask & AT_CLCK_PASSWD_MASK) ) {
        /* unlock requires password */
        LOG_TRACE(ERROR, "USR-MAIN  - unlock mode of operation "
                  "requires a password");
        _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
        ret_code = RETURNerror;
        break;
      }

      LOG_TRACE(ERROR, "USR-MAIN  - unlock mode of operation "
                "is not supported");
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      ret_code = RETURNerror;
      break;

    case AT_CLCK_LOCK:

      /* lock facility */
      if ( !(data->mask & AT_CLCK_PASSWD_MASK) ) {
        /* unlock requires password */
        LOG_TRACE(ERROR, "USR-MAIN  - lock mode of operation "
                  "requires a password");
        _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
        ret_code = RETURNerror;
        break;
      }

      LOG_TRACE(ERROR, "USR-MAIN  - lock mode of operation "
                "is not supported");
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
      ret_code = RETURNerror;
      break;

    case AT_CLCK_STATUS:
      /* Read facility status */
      clck->status = AT_CLCK_RESP_STATUS_NOT_ACTIVE;
      break;

    default:
      LOG_TRACE(ERROR, "USR-MAIN  - <mode> parameter is not valid"
                " (%d)", data->command.clck.mode);
      _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
      ret_code = RETURNerror;
      break;
    }

    break;

  case AT_COMMAND_TST:
    /*
     * Test command returns values supported as a compound value
     */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CLCK command type %d is not supported",
              data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cgpaddr()                                   **
 **                                                                        **
 ** Description: Executes the AT CGPADDR command operations:               **
 **      The AT CGPADDR command returns a list of PDP addresses    **
 **      for the specified context identifiers                     **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cgpaddr(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cgpaddr_resp_t *cgpaddr = &_nas_user_data.response.cgpaddr;
  memset(cgpaddr, 0, sizeof(at_cgpaddr_resp_t));

  int cid = -1;

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CGPADDR_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_SET:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /*
     * The execution command returns a list of PDP addresses for
     * the specified context identifiers
     */
    if (data->mask & AT_CGPADDR_CID_MASK) {
      if (data->command.cgpaddr.cid < AT_CGPADDR_CID_MIN) {
        /* The value of the PDP context identifier is not valid;
         * return an error message */
        LOG_TRACE(ERROR, "USR-MAIN  - <cid> parameter is "
                  "not valid (%d)",  data->command.cgpaddr.cid);
        _nas_user_data.cause_code = AT_ERROR_INCORRECT_PARAMETERS;
        ret_code = RETURNerror;
        break;
      }

      cid = data->command.cgpaddr.cid;
    }

    /*
     * Get the PDP addresses
     */
    cgpaddr->n_pdns = nas_proc_get_pdn_addr(cid, cgpaddr->cid,
                                            cgpaddr->PDP_addr_1,
                                            cgpaddr->PDP_addr_2,
                                            AT_CGPADDR_RESP_SIZE);

    if (cgpaddr->n_pdns == 0) {
      LOG_TRACE(ERROR, "USR-MAIN  - No any PDN context is defined");
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    }

    break;

  case AT_COMMAND_TST:
    /*
     * The test command returns a list of defined <cid>s.
     */
    cgpaddr->n_pdns = nas_proc_get_pdn_addr(cid, cgpaddr->cid,
                                            cgpaddr->PDP_addr_1,
                                            cgpaddr->PDP_addr_2,
                                            AT_CGPADDR_RESP_SIZE);
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CGPADDR command type %d is "
              "not supported", data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_cnum()                                      **
 **                                                                        **
 ** Description: Executes the AT CNUM command operations:                  **
 **      The AT CNUM command returns the MSISDNs related to the    **
 **      subscriber.                                               **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_cnum(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_cnum_resp_t *cnum = &_nas_user_data.response.cnum;
  memset(cnum, 0, sizeof(at_cnum_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CNUM_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_ACT:
    if (_nas_user_context.sim_status != NAS_USER_READY) {
      _nas_user_data.cause_code = AT_ERROR_SIM_PIN_REQUIRED;
      LOG_FUNC_RETURN(RETURNerror);
    }

    /* Get the International Mobile Subscriber Identity (IMSI) */
    ret_code = nas_proc_get_msisdn(cnum->number, &cnum->type);

    if (ret_code != RETURNok) {
      LOG_TRACE(ERROR, "USR-MAIN  - Failed to get MS dialing number");
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    }

    break;

  case AT_COMMAND_TST:
    /* Nothing to do */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CNUM command type %d is "
              "not supported", data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_user_proc_clac()                                      **
 **                                                                        **
 ** Description: Executes the AT CLAC command operations:                  **
 **      The AT CLAC command returns the list of AT Commands that  **
 **      are available for the user.                               **
 **                                                                        **
 ** Inputs:  data:      Pointer to the AT command data structure   **
 **          Others:    _nas_user_context                          **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok; RETURNerror;                     **
 **          Others:    _nas_user_data                             **
 **                                                                        **
 ***************************************************************************/
static int _nas_user_proc_clac(const at_command_t *data)
{
  LOG_FUNC_IN;

  int ret_code = RETURNok;
  at_clac_resp_t *clac = &_nas_user_data.response.clac;
  memset(clac, 0, sizeof(at_clac_resp_t));

  _nas_user_data.id = data->id;
  _nas_user_data.type = data->type;
  _nas_user_data.mask = AT_RESPONSE_CLAC_MASK;
  _nas_user_data.cause_code = AT_ERROR_SUCCESS;

  switch (data->type) {
  case AT_COMMAND_ACT:
    /* Get the list of supported AT commands */
    clac->n_acs = at_command_get_list(clac->ac, AT_CLAC_RESP_SIZE);

    if (clac->n_acs == 0) {
      LOG_TRACE(ERROR, "USR-MAIN  - Failed to get the list of "
                "supported AT commands");
      _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    }

    break;

  case AT_COMMAND_TST:
    /* Nothing to do */
    break;

  default:
    LOG_TRACE(ERROR, "USR-MAIN  - AT+CLAC command type %d is "
              "not supported", data->type);
    _nas_user_data.cause_code = AT_ERROR_OPERATION_NOT_SUPPORTED;
    ret_code = RETURNerror;
    break;
  }

  LOG_FUNC_RETURN (ret_code);
}

