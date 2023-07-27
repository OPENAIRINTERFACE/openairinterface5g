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

Source    at_error.c

Version   0.1

Date    2012/03/12

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Defines error codes returned when execution of AT command
    failed.

*****************************************************************************/

#include "at_error.h"
#include "commonDef.h"
#include "nas_log.h"

#include <string.h> // strcpy
#include <stdio.h>  // sprintf

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

// FIXME put this in .h
extern int at_response_format_v1;

/*
 * Result code suppression indicator (set by ATQ0 and ATQ1)
 * -------------------------------------------------------
 *  false - Result codes are transmitted to the user application
 *  true  - Result codes are suppressed and not transmitted
 */
bool at_error_code_suppression_q1 = false;

/*
 * Verbose mode indicator (set by ATV0, ATV1 and AT+CMEE)
 * ------------------------------------------------------
 */
at_error_format_t at_error_format = AT_ERROR_OFF;

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * General errors
 * --------------
 */
static const char* const _at_error_str[] = {
    "phone failure",
    "no connection to phone",
    "phone-adaptor link reserved",
    "operation not allowed",
    "operation not supported",
    "PH-SIM PIN required",
    "PH-FSIM PIN required",
    "PH-FSIM PUK required",
    NULL,
    NULL,
    "SIM not inserted",
    "SIM PIN required",
    "SIM PUK required",
    "SIM failure",
    "SIM busy",
    "SIM wrong",
    "incorrect password",
    "SIM PIN2 required",
    "SIM PUK2 required",
    NULL,
    "memory full",
    "invalid index",
    "not found",
    "memory failure",
    "text string too long",
    "invalid characters in text string",
    "dial string too long",
    "invalid characters in dial string",
    NULL,
    NULL,
    "no network service",
    "network timeout",
    "network not allowed - emergency calls only",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "network personalization PIN required",
    "network personalization PUK required",
    "network subset personalization PIN required",
    "network subset personalization PUK required",
    "service provider personalization PIN required",
    "service provider personalization PUK required",
    "corporate personalization PIN required",
    "corporate personalization PUK required",
    "hidden key required",
    "EAP method not supported",
    "Incorrect parameters",
};

/*
 * GPRS-related errors
 * -------------------
 */
static const char* const _at_error_gprs[] = {
    "Illegal MS",
    NULL,
    NULL,
    "Illegal ME",
    "GPRS services not allowed",
    NULL,
    NULL,
    NULL,
    "PLMN not allowed",
    "Location area not allowed",
    "Roaming not allowed in this location area",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "service option not supported",
    "requested service option not subscribed",
    "service option temporarily out of order",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "unspecified GPRS error",
    "PDP authentication failure",
    "invalid mobile class",
    "Last PDN disconnection not allowed",
};

/*
 * VBS / VGCS and eMLPP -related errors
 * ------------------------------------
 */
static const char* const _at_error_vbs[] = {
    /* 151 "VBS/VGCS not supported by the network" */
    "No service subscription on SIM",
    "No subscription for group ID",
    "Group Id not activated on SIM",
    "No matching notification",
    "VBS/VGCS call already present",
    "Congestion",
    "Network failure",
    "Uplink busy",
    "No access rights for SIM file",
    "No subscription for priority",
    "operation not applicable or not possible",
    "Group Id prefixes not supported",
    "Group Id prefixes not usable for VBS",
    "Group Id prefix value invalid",
};

/* Returns the verbose value of AT command error */
static const char* _at_error_to_string(int error);

static int _at_error_encode_ok(char* buffer);
static int _at_error_encode_ko(char* buffer, at_error_type_t type, int error);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  at_error_set_format()                                     **
 **                                                                        **
 ** Description: Sets the format of the error code                         **
 **                                                                        **
 ** Inputs:  format:  Format of the error code                   **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  None                                       **
 **      Others:  at_error_format                            **
 **                                                                        **
 ***************************************************************************/
void at_error_set_format(at_error_format_t format)
{
  LOG_FUNC_IN;

  at_error_format = format;

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:  at_error_get_format()                                     **
 **                                                                        **
 ** Description: Gets the format of the error code                         **
 **                                                                        **
 ** Inputs:  format:  Format of the error code                   **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  None                                       **
 **      Others:  at_error_format                            **
 **                                                                        **
 ***************************************************************************/
at_error_format_t at_error_get_format(void)
{
  LOG_FUNC_IN;

  LOG_FUNC_RETURN (at_error_format);
}

/****************************************************************************
 **                                                                        **
 ** Name:  at_error_encode()                                         **
 **                                                                        **
 ** Description: Encodes result code returned when AT command has been     **
 **    processed. Result code can be OK or 0, if the command     **
 **    succeed, ERROR or 4, if the command failed; depending of  **
 **    the response format selected by the user.                 **
 **                                                                        **
 ** Inputs   type:    Type of the error code to encode           **
 **      error:   Error code to encode                       **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters that have been    **
 **       successfully encoded;                      **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int at_error_encode(char* buffer, at_error_type_t type, int error)
{
  LOG_FUNC_IN;

  int bytes = 0;

  /* Result code suppression (ATQ0, ATQ1) */
  if (at_error_code_suppression_q1) {
    LOG_FUNC_RETURN (0);
  }

  if (type == AT_ERROR_OK) {
    /* Encode success return code */
    bytes = _at_error_encode_ok(buffer);
  } else {
    /* Encode failure return code */
    bytes = _at_error_encode_ko(buffer, type, error);
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  _at_error_encode_ok()                                     **
 **                                                                        **
 ** Description: Encodes AT command success result code message            **
 **                                                                        **
 ** Inputs   None                                                      **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters that have been    **
 **       successfully encoded;                      **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_error_encode_ok(char* buffer)
{
  LOG_FUNC_IN;

  int offset = 0;

  if (at_response_format_v1) {
    offset += sprintf(buffer, "\r\n");
  }

  offset += sprintf(buffer + offset, "OK\r\n");

  LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _at_error_encode_ko()                                     **
 **                                                                        **
 ** Description: Encodes AT command error code message                     **
 **                                                                        **
 ** Inputs   type:    Type of the error code to encode           **
 **      error:   Error code to encode                       **
 **      Others:  at_error_format                            **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters that have been    **
 **       successfully encoded;                      **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _at_error_encode_ko(char* buffer, at_error_type_t type, int error)
{
  LOG_FUNC_IN;

  int offset = 0;

  if (at_response_format_v1) {
    offset += sprintf(buffer, "\r\n");
  }

  /* Result code is disabled */
  if (at_error_format == AT_ERROR_OFF) {
    offset += sprintf(buffer + offset, "ERROR\r\n");
  } else {
    /* Error related to execution of AT command operation */
    if (type == AT_ERROR_CME) {
      offset += sprintf(buffer + offset, "+CME ERROR: ");
    }

    /* Result code is enabled for numeric values */
    if (at_error_format == AT_ERROR_NUMERIC) {
      offset += sprintf(buffer + offset, "%d\r\n", error);
    }

    /* Result code is enabled for verbose values */
    else {
      const char* error_str = _at_error_to_string(error);

      if (error_str != NULL) {
        offset += sprintf(buffer + offset, "%s\r\n", error_str);
      } else {
        /* Error code is not valid */
        offset += sprintf(buffer + offset, "invalid error code\r\n");
      }
    }
  }

  LOG_FUNC_RETURN (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:  at_error_to_string()                                      **
 **                                                                        **
 ** Description: Returns the verbose value of AT command error             **
 **                                                                        **
 ** Inputs:  error:   AT command error identifier                **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  The error's verbose value                  **
 **       NULL if the error code is not valid        **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static const char* _at_error_to_string(int error)
{
  if (error < AT_ERROR_LAST_GENERAL_ERROR + 1) {
    /* General errors */
    if (error > AT_ERROR_FIRST_GENERAL_ERROR - 1) {
      return _at_error_str[error];
    } else {
      return NULL;
    }
  } else if (error < AT_ERROR_LAST_GPRS_RELATED_ERROR + 1) {
    /* GPRS-related errors */
    if (error > AT_ERROR_FIRST_GPRS_RELATED_ERROR - 1) {
      return _at_error_gprs[error - AT_ERROR_FIRST_GPRS_RELATED_ERROR];
    } else {
      return NULL;
    }
  } else if (error < AT_ERROR_LAST_VBS_RELATED_ERROR + 1) {
    /* VBS / VGCS and eMLPP -related errors */
    if (error > AT_ERROR_FIRST_VBS_RELATED_ERROR - 1) {
      return _at_error_vbs[error - AT_ERROR_FIRST_VBS_RELATED_ERROR];
    } else {
      return NULL;
    }
  }

  return NULL;
}
