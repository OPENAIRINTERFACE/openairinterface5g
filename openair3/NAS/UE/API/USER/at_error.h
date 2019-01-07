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

Source    at_error.h

Version   0.1

Date    2012/03/12

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Defines error codes returned when execution of AT command
    failed.

*****************************************************************************/
#ifndef __AT_ERROR_H__
#define __AT_ERROR_H__

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * Cause code used to notify that the NAS procedure has been successfully
 * processed
 */
#define AT_ERROR_SUCCESS        (-1)

/*
 * General errors
 * --------------
 */
#define AT_ERROR_FIRST_GENERAL_ERROR AT_ERROR_PHONE_FAILURE

#define AT_ERROR_PHONE_FAILURE        0
#define AT_ERROR_NO_CONNECTION_TO_PHONE     1
#define AT_ERROR_PHONE_ADAPTOR_LINK_RESERVED    2
#define AT_ERROR_OPERATION_NOT_ALLOWED      3
#define AT_ERROR_OPERATION_NOT_SUPPORTED    4
#define AT_ERROR_PHSIM_PIN_REQUIRED     5
#define AT_ERROR_PHFSIM_PIN_REQUIRED      6
#define AT_ERROR_PHFSIM_PUK_REQUIRED      7

#define AT_ERROR_SIM_NOT_INSERTED     10
#define AT_ERROR_SIM_PIN_REQUIRED     11
#define AT_ERROR_SIM_PUK_REQUIRED     12
#define AT_ERROR_SIM_FAILURE        13
#define AT_ERROR_SIM_BUSY       14
#define AT_ERROR_SIM_WRONG        15
#define AT_ERROR_INCORRECT_PASSWD     16
#define AT_ERROR_SIM_PIN2_REQUIRED      17
#define AT_ERROR_SIM_PUK2_REQUIRED      18

#define AT_ERROR_MEMORY_FULL        20
#define AT_ERROR_INVALID_INDEX        21
#define AT_ERROR_NOT_FOUND        22
#define AT_ERROR_MEMORY_FAILURE       23
#define AT_ERROR_TEXT_STRING_TOO_LONG     24
#define AT_ERROR_TEXT_STRING_INVALID_CHAR   25
#define AT_ERROR_DIAL_STRING_TOO_LONG     26
#define AT_ERROR_DIAL_STRING_INVALID_CHAR   27

#define AT_ERROR_NO_NETWORK_SERVICE     30
#define AT_ERROR_TIMEOUT        31
#define AT_ERROR_NETWORK_NOT_ALLOWED      32

#define AT_ERROR_NETWORK_PERSO_PIN_REQUIRED   40
#define AT_ERROR_NETWORK_PERSO_PUK_REQUIRED   41
#define AT_ERROR_NETWORK_SUBNET_PERSO_PIN_REQUIRED  42
#define AT_ERROR_SUBNET_PERSO_PUK_REQUIRED    43
#define AT_ERROR_PROVIDER_PERSO_PIN_REQUIRED    44
#define AT_ERROR_PROVIDER_PERSO_PUK_REQUIRED    45
#define AT_ERROR_CORPORATE_PERSO_PIN_REQUIRED   46
#define AT_ERROR_CORPORATE_PERSO_PUK_REQUIRED   47
#define AT_ERROR_HIDDEN_KEY_REQUIRED      48
#define AT_ERROR_EAP_METHOD_NOT_SUPPORTED   49
#define AT_ERROR_INCORRECT_PARAMETERS     50

#define AT_ERROR_LAST_GENERAL_ERROR AT_ERROR_INCORRECT_PARAMETERS

#define AT_ERROR_UNKNOWN        100

/*
 * GPRS-related errors
 * -------------------
 */
/* Errors related to a failure to perform an attach */
#define AT_ERROR_FIRST_GPRS_RELATED_ERROR AT_ERROR_ILLEGAL_MS

#define AT_ERROR_ILLEGAL_MS         103
#define AT_ERROR_ILLEGAL_ME         106
#define AT_ERROR_GPRS_NOT_ALLOWED       107
#define AT_ERROR_PLMN_NOT_ALLOWED       111
#define AT_ERROR_LOCATION_AREA_NOT_ALLOWED      112
#define AT_ERROR_ROAMING_NOT_ALLOWED        113
/* Errors related to a failure to activate a context */
#define AT_ERROR_OPTION_NOT_SUPPORTED       132
#define AT_ERROR_OPTION_NOT_SUBSCRIBED        133
#define AT_ERROR_OPTION_TEMPORARILY_OUT_OF_ORDER    134
#define AT_ERROR_PDP_AUTHENTICATION_FAILURE     149
/* Errors related to a failure to disconnect a PDN */
#define AT_ERROR_LAST_PDP_DISCONNECT_NOT_ALLOWED    151
/* Other GPRS errors */
#define AT_ERROR_UNSPECIFIED_GPRS_ERROR       148
#define AT_ERROR_INVALID_MOBILE_CLASS       150

#define AT_ERROR_LAST_GPRS_RELATED_ERROR AT_ERROR_LAST_PDP_DISCONNECT_NOT_ALLOWED

/*
 * VBS / VGCS and eMLPP -related errors
 * ------------------------------------
 */
#define AT_ERROR_FIRST_VBS_RELATED_ERROR AT_ERROR_NO_SERVICE_SUBSCRIPTION_ON_SIM

/* AT_ERROR_VBS_VGCS_NOT_SUPPORTED=151,*/ /* !!! Conflict with AT_ERROR_LAST_PDP_DISCONNECT_NOT_ALLOWED !!! */
#define AT_ERROR_NO_SERVICE_SUBSCRIPTION_ON_SIM     152
#define AT_ERROR_NO_SUBSCRIPTION_FOR_GROUP_ID     153
#define AT_ERROR_GROUP_ID_NOT_ACTIVATED_ON_SIM      154
#define AT_ERROR_NO_MATCHING_NOTIFICATION     155
#define AT_ERROR_VBS_VGCS_CALL_ALREADY_PRESENT      156
#define AT_ERROR_CONGESTION         157
#define AT_ERROR_NETWORK_FAILURE        158
#define AT_ERROR_UPLINK_BUSY          159
#define AT_ERROR_NO_ACCESS_RIGHTS_FOR_SIM_FILE      160
#define AT_ERROR_NO_SUBSCRIPTION_FOR_PRIORITY     161
#define AT_ERROR_OPERATION_NOT_APPLICABLE_OR_NOT_POSSIBLE 162
#define AT_ERROR_GROUP_ID_PREFIXES_NOT_SUPPORTED    163
#define AT_ERROR_GROUP_ID_PREFIXES_NOT_USABLE_FOR_VBS   164
#define AT_ERROR_GROUP_ID_PREFIX_VALUE_INVALID      165

#define AT_ERROR_LAST_VBS_RELATED_ERROR AT_ERROR_GROUP_ID_PREFIX_VALUE_INVALID

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * Error type identifiers
 */
typedef enum {
  AT_ERROR_NONE,  /* No error code displayed      */
  AT_ERROR_OK,  /* AT command successfully processed    */
  AT_ERROR_SYNTAX,  /* AT command syntax error      */
  AT_ERROR_CME, /* Error related to execution of AT command
       * operation          */
} at_error_type_t;

/*
 * Error code format identifiers
 */
typedef enum {
  AT_ERROR_OFF, /* disable result code and use ERROR instead     */
  AT_ERROR_NUMERIC, /* enable result code and use numeric error values */
  AT_ERROR_VERBOSE, /* enable result code and use verbose error values */
} at_error_format_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void at_error_set_format(at_error_format_t format);
at_error_format_t at_error_get_format(void);

int at_error_encode(char* buffer, at_error_type_t type, int error);

#endif /* __AT_ERROR_H__*/
