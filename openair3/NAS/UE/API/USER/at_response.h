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

Source    at_response.h

Version   0.1

Date    2012/03/09

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Defines structure of the AT command response returned after
    completion of AT command processing.

*****************************************************************************/
#ifndef __AT_RESPONSE_H__
#define __AT_RESPONSE_H__

#include "at_command.h"
#include "commonDef.h"
#include "networkDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * Total number of characters, including line terminators, in information text
 */
#define AT_RESPONSE_INFO_TEXT_SIZE  2048

/*
 * Value of the mask parameter for AT command reponses without any optional
 * parameters
 */
#define AT_RESPONSE_NO_PARAM    AT_COMMAND_PARAM0

/*
 * Maximum value of an AT command response identifier
 */
#define AT_RESPONSE_ID_MAX    AT_COMMAND_ID_MAX

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * ================
 * General commands
 * ================
 */

/* CGSN: Request Product Serial Number identification (IMEI)
 * ---------------------------------------------------------
 * Returns information text intended to permit the user to identify the
 * individual Mobile Equipment to which it is connected to.
 * Action command:
 * +CGSN returns <sn>
 *  <sn>  information text containing the IMEI
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CGSN_MASK AT_RESPONSE_NO_PARAM

/* CGSN AT command response type */
typedef struct {
  char sn[AT_RESPONSE_INFO_TEXT_SIZE+1];
} at_cgsn_resp_t;

/* CGMI: Request manufacturer identification
 * -----------------------------------------
 * Returns information text intended to permit the user to identify the
 * manufacturer of the Mobile Equipment to which it is connected to.
 * Action command:
 * +CGMI returns <manufacturer>
 *   <manufacturer> information text containing the manufacturer id
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CGMI_MASK AT_RESPONSE_NO_PARAM

/* CGMI AT command response type */
typedef struct {
  char manufacturer[AT_RESPONSE_INFO_TEXT_SIZE+1];
} at_cgmi_resp_t;

/* CGMM: Request model identification
 * ----------------------------------
 * Returns information text intended to permit the user to identify the
 * specific model of the Mobile Equipment to which it is connected to.
 * +CGMM returns <model>
 *   <model> information text containing the model id
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CGMM_MASK AT_RESPONSE_NO_PARAM

/* CGMM AT command response type */
typedef struct {
  char model[AT_RESPONSE_INFO_TEXT_SIZE+1];
} at_cgmm_resp_t;

/* CGMR: Request revision identification
 * -------------------------------------
 * Returns information text intended to permit the user to identify the
 * version, revision level or date, or other pertinent information of the
 * Mobile Equipment to which it is connected to.
 * +CGMR returns <revision>
 *   <revision> information text containing the revision of the firmware
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CGMR_MASK AT_RESPONSE_NO_PARAM

/* CGMR AT command response type */
typedef struct {
  char revision[AT_RESPONSE_INFO_TEXT_SIZE+1];
} at_cgmr_resp_t;

/* CIMI: Request International Mobile Subscriber Identity
 * ------------------------------------------------------
 * Returns <IMSI>, which is intended to permit the user to identify the
 * individual SIM card or active application in the UICC (GSM or USIM)
 * which is attached to the Mobile Equipment to which it is connected to.
 * Action command:
 * +CIMI returns <IMSI>
 *   <IMSI> International Mobile Subscriber Identity (string without
 *    double quotes)
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CIMI_MASK AT_RESPONSE_NO_PARAM

/* CIMI AT command response type */
typedef struct {
  char IMSI[AT_RESPONSE_INFO_TEXT_SIZE+1];
} at_cimi_resp_t;

/*
 * ==============================================
 * Mobile Termination control and status commands
 * ==============================================
 */
/* CFUN: Set phone functionality
 * -----------------------------
 * Used to set the Mobile Equipment to different power consumption states
 * Read parameter command:
 * +CFUN? returns +CFUN: <fun>
 *  <fun> integer type - level of functionality
 * Test parameter command:
 * +CFUN=? returns +CFUN: (list of supported <fun>s),(list of supported <rst>s)
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CFUN_MASK AT_RESPONSE_NO_PARAM

/* CFUN AT command response type */
typedef struct {
  int fun;
} at_cfun_resp_t;

/* CPIN: Enter PIN
 * ---------------
 * Used to enter MT passwords which are needed before any other
 * functionality of the MT can be used
 * Read parameter command:
 * +CPIN? returns +CPIN: <code>
 *  <code>   string - value may be: "READY", "SIM PIN"
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CPIN_MASK AT_RESPONSE_NO_PARAM

/* CPIN AT command response type */
typedef struct {
#define AT_CPIN_RESP_SIZE 16
  char code[AT_CPIN_RESP_SIZE+1];
} at_cpin_resp_t;

/* CSQ: Signal quality
 * -------------------
 * Returns received signal strength indication and channel bit error rate
 * from the Mobile Equipment
 * Action command:
 * +CSQ returns +CSQ: <rssi>, <ber>
 *  <rssi>  integer type - received signal strength indication (in dBm)
 *  <ber> integer type - channel bit error rate (in percent)
 */

/* Received signal strength indicator */
#define AT_CSQ_RSSI_0   0 /* -113 dBm or less   */
#define AT_CSQ_RSSI_31    31  /* -51 dBm or greater   */
#define AT_CSQ_RSSI_UNKNOWN 99  /* not known or not detectable  */

/* Channel bit error rate */
#define AT_CSQ_BER_0    0 /* BER < 0.2 %      */
#define AT_CSQ_BER_1    1 /* 0.2 % < BER < 0.4 %    */
#define AT_CSQ_BER_2    2 /* 0.4 % < BER < 0.8 %    */
#define AT_CSQ_BER_3    3 /* 0.8 % < BER < 1.6 %    */
#define AT_CSQ_BER_4    4 /* 1.6 % < BER < 3.2 %    */
#define AT_CSQ_BER_5    5 /* 3.2 % < BER < 6.4 %    */
#define AT_CSQ_BER_6    6 /* 6.4 % < BER < 12.8 %   */
#define AT_CSQ_BER_7    7 /* 12.8 % < BER     */
#define AT_CSQ_BER_UNKNOWN  99  /* not known or not detectable  */

/* Optional parameter bitmask */
#define AT_RESPONSE_CSQ_MASK  AT_RESPONSE_NO_PARAM

/* CSQ AT command response type */
typedef struct {
  int rssi;
  int ber;
} at_csq_resp_t;

/* CESQ: Extended signal quality
 * -----------------------------
 * Returns received signal quality parameters.
 * Action command:
 * +CESQ returns +CESQ: <rssi>,<ber>,<rscp>,<ecno>,<rsrq>,<rsrp>
 *  <rssi>  integer type - received signal strength indication (in dBm)
 *  <ber> integer type - channel bit error rate (in percent)
 *  <rscp>  integer type - received signal code power (in dBm)
 *  <ecno>  integer type - ratio of the received energy per PN chip (in dBm)
 *  <rsrq>  integer type - reference signal received quality (in dBm)
 *  <rsrp>  integer type - reference signal received power (in dBm)
 */

/* Received signal strength indicator */
#define AT_CESQ_RSSI_0    0 /* -113 dBm or less   */
#define AT_CESQ_RSSI_31   31  /* -51 dBm or greater   */
#define AT_CESQ_RSSI_UNKNOWN  99  /* not known or not detectable  */

/* Channel bit error rate */
#define AT_CESQ_BER_0   0 /* BER < 0.2 %      */
#define AT_CESQ_BER_1   1 /* 0.2 % < BER < 0.4 %    */
#define AT_CESQ_BER_2   2 /* 0.4 % < BER < 0.8 %    */
#define AT_CESQ_BER_3   3 /* 0.8 % < BER < 1.6 %    */
#define AT_CESQ_BER_4   4 /* 1.6 % < BER < 3.2 %    */
#define AT_CESQ_BER_5   5 /* 3.2 % < BER < 6.4 %    */
#define AT_CESQ_BER_6   6 /* 6.4 % < BER < 12.8 %   */
#define AT_CESQ_BER_7   7 /* 12.8 % < BER     */
#define AT_CESQ_BER_UNKNOWN 99  /* not known or not detectable  */

/* Received signal code power */
#define AT_CESQ_RSCP_0    0 /* -120 dBm or less   */
#define AT_CESQ_RSCP_96   96  /* -24 dBm or greater   */
#define AT_CESQ_RSCP_UNKNOWN  255 /* not known or not detectable  */

/* Ratio of the received energy per PN chip (Ec/No) */
#define AT_CESQ_ECNO_0    0 /* -24 dBm or less    */
#define AT_CESQ_ECNO_49   49  /* 0,5 dBm or greater   */
#define AT_CESQ_ECNO_UNKNOWN  255 /* not known or not detectable  */

/* Reference signal received quality */
#define AT_CESQ_RSRQ_0    0 /* -19,5 dBm or less    */
#define AT_CESQ_RSRQ_34   34  /* -2,5 dBm or greater    */
#define AT_CESQ_RSRQ_UNKNOWN  255 /* not known or not detectable  */

/* Reference signal received power */
#define AT_CESQ_RSRP_0    0 /* -140 dBm or less   */
#define AT_CESQ_RSRP_97   97  /* -43 dBm or greater   */
#define AT_CESQ_RSRP_UNKNOWN  255 /* not known or not detectable  */

/* Optional parameter bitmask */
#define AT_RESPONSE_CESQ_MASK AT_RESPONSE_NO_PARAM

/* CSQ AT command response type */
typedef struct {
  int rssi;
  int ber;
  int rscp;
  int ecno;
  int rsrq;
  int rsrp;
} at_cesq_resp_t;

/* CLAC: List all available AT commands
 * ------------------------------------
 * Returns one or more lines of AT Commands that are available for the user.
 * Action command:
 * +CLAC returns +CLAC: <AT Command1>[<CR><LF><AT Command2>[...]]
 *  <AT Command> string - defines the AT command including the prefix AT
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CLAC_MASK AT_RESPONSE_NO_PARAM

/* CLAC AT command response type */
typedef struct {
  int n_acs;
#define AT_CLAC_RESP_SIZE 100
  const char* ac[AT_CLAC_RESP_SIZE];
} at_clac_resp_t;

/*
 * =========================
 * Mobile Termination errors
 * =========================
 */
/*
 * CMEE: Report mobile termination error
 * -------------------------------------
 * Disables or enables the use of final result code +CME ERROR: <err> as an
 * indication of an error relating to the functionality of the MT.
 * Read parameter command:
 * +CMEE? returns +CMEE: <n>
 *  <n> numeric parameter
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CMEE_MASK AT_RESPONSE_NO_PARAM

/* CMEE AT command response type */
typedef at_cmee_t at_cmee_resp_t;

/*
 * ================================
 * Network service related commands
 * ================================
 */
/* CNUM: Subscriber number
 * -----------------------
 * Returns the MSISDNs related to the subscriber.
 * Action command:
 * +CNUM returns +CNUM: <number1>,<type1>[<CR><LF>
 *     +CNUM: <number2>,<type2>[...]]
 *  <numberx> string type phone number of format specified by <typex>
 *  <typex>   type of address octet in integer format
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CNUM_MASK AT_RESPONSE_NO_PARAM

/* CNUM AT command response type */
typedef struct {
#define AT_CNUM_NUMBER_SIZE MSISDN_DIGIT_SIZE
  char number[AT_CNUM_NUMBER_SIZE];
  int type;
} at_cnum_resp_t;

/* CLCK: Facility lock
 * -------------------
 * Used to lock, unlock or interrogate a MT or a network facility <fac>.
 * When <mode>=2, returns +CLCK: <status>
 *  <status> state of the network service
 */

/* Network service status */
#define AT_CLCK_RESP_STATUS_NOT_ACTIVE  0
#define AT_CLCK_RESP_STATUS_ACTIVE  1

/* Optional parameter bitmask */
#define AT_RESPONSE_CLCK_MASK AT_RESPONSE_NO_PARAM

/* CLCK AT command response type */
typedef struct {
  int status;
} at_clck_resp_t;

/* COPS: PLMN selection
 * --------------------
 * Forces an attempt to select and register the GSM/UMTS/EPS network operator
 * using the SIM/USIM card installed in the currently selected card slot.
 * Read parameter command:
 * +COPS? returns +COPS:<mode>[,<format>,<oper>[,<AcT>]]
 *  <mode>   operation mode of the network selection
 *  <format> representation format of the network operator
 *  <oper>   APN operator identifier
 *  <AcT>  access technology
 * Test command returns a set of parameters, each representing an operator
 * present in the network:
 * +COPS=? returns +COPS: [list of supported (<stat>,long alphanumeric <oper>,
 * short alphanumeric <oper>,numeric <oper>[,<AcT>])s][,,(list of supported
 * <mode>s),(list of supported <format>s)]
 *  <stat>   network operator availability
 */

/* Optional parameter bitmask */
#define AT_COPS_RESP_FORMAT_MASK  AT_COMMAND_PARAM2
#define AT_COPS_RESP_OPER_MASK    AT_COMMAND_PARAM3
#define AT_COPS_RESP_ACT_MASK   AT_COMMAND_PARAM4

/* Structure of COPS AT test parameter command */
typedef struct {
  const char* data; /* String representation of the list of operators  */
  int size;   /* Size in byte of the data to be dislayed     */
} at_cops_tst_t;

/* Structure of COPS AT read parameter command */
typedef at_cops_t at_cops_get_t;

/* COPS AT command response type */
typedef union {
  at_cops_tst_t tst;
  at_cops_get_t get;
} at_cops_resp_t;

/* CREG: network registration
 * --------------------------
 * When <n>=1, returns +CREG: <stat>, the Mobile Equipment's circuit mode
 * network registration status in GERA/UTRA/E-UTRA Network.
 * When <n>=2, returns +CREG: <stat>[,<lac>,<ci>[,<AcT>]], the Mobile
 * Equipment's circuit mode network registration status and location information
 * in GERA/UTRA/E-UTRA Network.
 * Read parameter command:
 * +CREG? returns +CREG:<n>,<stat>[,<lac>,<ci>[,<AcT>]]
 *  <n> numeric parameter
 *  <stat>  numeric parameter that indicates the circuit mode registration
 *    status
 *  <lac> string type; two byte location area code in hexadecimal format
 *  <ci>  string type; four byte GERAN/UTRAN/E-UTRAN cell ID in hexadeci-
 *    mal format
 *  <AcT> a numeric parameter that indicates the access technology of
 *    the serving cell
 */

/* Network registration status */
#define AT_CREG_RESP_REG_OFF    NET_REG_STATE_OFF
#define AT_CREG_RESP_REG_HN   NET_REG_STATE_HN
#define AT_CREG_RESP_REG_ON   NET_REG_STATE_ON
#define AT_CREG_RESP_REG_DENIED   NET_REG_STATE_DENIED
#define AT_CREG_RESP_REG_UNKNOWN  NET_REG_STATE_UNKNOWN
#define AT_CREG_RESP_REG_SMS_HN   NET_REG_STATE_SMS_HN
#define AT_CREG_RESP_REG_SMS_ROAMING  NET_REG_STATE_SMS_ROAMING
#define AT_CREG_RESP_REG_ROAMING  NET_REG_STATE_ROAMING

/* Access technology indicators */
#define AT_CREG_RESP_GSM  NET_ACCESS_GSM     /* GSM   */
#define AT_CREG_RESP_COMPACT  NET_ACCESS_COMPACT /* GSM Compact */
#define AT_CREG_RESP_UTRAN  NET_ACCESS_UTRAN   /* UTRAN   */
#define AT_CREG_RESP_EGPRS  NET_ACCESS_EGPRS   /* GSM w/EGPRS */
#define AT_CREG_RESP_HSDPA  NET_ACCESS_HSDPA   /* UTRAN w/HSDPA */
#define AT_CREG_RESP_HSUPA  NET_ACCESS_HSUPA   /* UTRAN w/HSUPA */
#define AT_CREG_RESP_HSDUPA NET_ACCESS_HSDUPA  /* w/HSDPA and HSUPA */
#define AT_CREG_RESP_EUTRAN NET_ACCESS_EUTRAN  /* E-UTRAN   */

/* Optional parameter bitmask */
//#define AT_CREG_RESP_N_MASK AT_COMMAND_PARAM1
#define AT_CREG_RESP_LAC_MASK AT_COMMAND_PARAM3
#define AT_CREG_RESP_CI_MASK  AT_COMMAND_PARAM4
#define AT_CREG_RESP_ACT_MASK AT_COMMAND_PARAM5

/* CREG AT command response type */
typedef struct {
  int n;
  int stat;
#define AT_CREG_RESP_LAC_SIZE 4
  char lac[AT_CREG_RESP_LAC_SIZE+1];
#define AT_CREG_RESP_CI_SIZE  8
  char ci[AT_CREG_RESP_CI_SIZE+1];
  int AcT;
} at_creg_resp_t;

/*
 * ==========================
 * Commands for Packet Domain
 * ==========================
 */
/* CGATT: GPRS service attach/detach
 * ---------------------------------
 * Used to attach the MT to, or detach the MT from, the GPRS service.
 * Read parameter command:
 * +CGATT? returns CGATT: <state>
 *  <state> numeric parameter that indicates the state of GPRS attachment
 * Test parameter command:
 * +CGATT=? returns CGATT: (list of supported <state>s)
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CGATT_MASK  AT_RESPONSE_NO_PARAM

/* CGATT AT command response type */
typedef struct {
  int state;
} at_cgatt_resp_t;

/* CGREG: GPRS network registration status
 * ---------------------------------------
 * When <n>=1, returns +CGREG: <stat>, the Mobile Equipment's GPRS network
 * registration status in GERA/UTRA Network.
 * When <n>=2, returns +CGREG: <stat>[,<lac>,<ci>[,<AcT>,<rac>]], the Mobile
 * Equipment's GPRS network registration status and location information in
 * GERA/UTRA Network.
 * Read parameter command:
 * +CGREG? returns +CGREG:<n>,<stat>[,<lac>,<ci>[,<AcT>,<rac>]]
 *  <n> numeric parameter
 *  <stat>  numeric parameter that indicates the GPRS registration status
 *  <lac> string type; two byte location area code in hexadecimal format
 *  <ci>  string type; four byte GERAN/UTRAN cell ID in hexadecimal format
 *  <AcT> a numeric parameter that indicates the access technology of
 *    the serving cell
 *  <rac> string type; one byte routing area code in hexadecimal format
 */

/* Network registration status */
#define AT_CGREG_RESP_REG_OFF   NET_REG_STATE_OFF
#define AT_CGREG_RESP_REG_HN    NET_REG_STATE_HN
#define AT_CGREG_RESP_REG_ON    NET_REG_STATE_ON
#define AT_CGREG_RESP_REG_DENIED  NET_REG_STATE_DENIED
#define AT_CGREG_RESP_REG_UNKNOWN NET_REG_STATE_UNKNOWN
#define AT_CGREG_RESP_REG_ROAMING NET_REG_STATE_ROAMING
#define AT_CGREG_RESP_REG_EMERGENCY NET_REG_STATE_EMERGENCY

/* Access technology indicators */
#define AT_CGREG_RESP_GSM NET_ACCESS_GSM     /* GSM   */
#define AT_CGREG_RESP_COMPACT NET_ACCESS_COMPACT /* GSM Compact */
#define AT_CGREG_RESP_UTRAN NET_ACCESS_UTRAN   /* UTRAN   */
#define AT_CGREG_RESP_EGPRS NET_ACCESS_EGPRS   /* GSM w/EGPRS */
#define AT_CGREG_RESP_HSDPA NET_ACCESS_HSDPA   /* UTRAN w/HSDPA */
#define AT_CGREG_RESP_HSUPA NET_ACCESS_HSUPA   /* UTRAN w/HSUPA */
#define AT_CGREG_RESP_HSDUPA  NET_ACCESS_HSDUPA  /* w/HSDPA and HSUPA */

/* Optional parameter bitmask */
//#define AT_CGREG_RESP_N_MASK  AT_COMMAND_PARAM1
#define AT_CGREG_RESP_LAC_MASK  AT_COMMAND_PARAM3
#define AT_CGREG_RESP_CI_MASK AT_COMMAND_PARAM4
#define AT_CGREG_RESP_ACT_MASK  AT_COMMAND_PARAM5
#define AT_CGREG_RESP_RAC_MASK  AT_COMMAND_PARAM6

/* CGREG AT command response type */
typedef struct {
  int n;
  int stat;
#define AT_CGREG_RESP_LAC_SIZE  4
  char lac[AT_CGREG_RESP_LAC_SIZE+1];
#define AT_CGREG_RESP_CI_SIZE 8
  char ci[AT_CGREG_RESP_CI_SIZE+1];
  int AcT;
#define AT_CGREG_RESP_RAC_SIZE  2
  char rac[AT_CGREG_RESP_RAC_SIZE+1];
} at_cgreg_resp_t;

/* CEREG: EPS network registration status
 * --------------------------------------
 * When <n>=1, returns +CEREG: <stat>, the Mobile Equipment's EPS services
 * registration status in EUTRA Network.
 * When <n>=2, returns +CEREG: <stat>[,<tac>,<ci>[,<AcT>]], the Mobile
 * Equipment's EPS services registration status and location information in
 * EUTRA Network.
 * Read parameter command:
 * +CEREG? returns +CEREG:<n>,<stat>[,<tac>,<ci>[,<AcT>]]
 *  <n> numeric parameter
 *  <stat>  numeric parameter that indicates the EPS registration status
 *  <tac> string type; two byte tracking area code in hexadecimal format
 *  <ci>  string type; four byte E-UTRAN cell ID in hexadecimal format
 *  <AcT> a numeric parameter that indicates the access technology of
 *    the serving cell
 */

/* Network registration status */
#define AT_CEREG_RESP_REG_OFF   NET_REG_STATE_OFF
#define AT_CEREG_RESP_REG_HN    NET_REG_STATE_HN
#define AT_CEREG_RESP_REG_ON    NET_REG_STATE_ON
#define AT_CEREG_RESP_REG_DENIED  NET_REG_STATE_DENIED
#define AT_CEREG_RESP_REG_UNKNOWN NET_REG_STATE_UNKNOWN
#define AT_CEREG_RESP_REG_ROAMING NET_REG_STATE_ROAMING
#define AT_CEREG_RESP_REG_EMERGENCY NET_REG_STATE_EMERGENCY

/* Access technology indicators */
#define AT_CEREG_RESP_EUTRAN  NET_ACCESS_EUTRAN  /* E-UTRAN   */

/* Optional parameter bitmask */
//#define AT_CEREG_RESP_N_MASK  AT_COMMAND_PARAM1
#define AT_CEREG_RESP_TAC_MASK  AT_COMMAND_PARAM3
#define AT_CEREG_RESP_CI_MASK AT_COMMAND_PARAM4
#define AT_CEREG_RESP_ACT_MASK  AT_COMMAND_PARAM5

/* CEREG AT command response type */
typedef struct {
  int n;
  int stat;
#define AT_CEREG_RESP_TAC_SIZE  4
  char tac[AT_CEREG_RESP_TAC_SIZE+1];
#define AT_CEREG_RESP_CI_SIZE 8
  char ci[AT_CEREG_RESP_CI_SIZE+1];
  int AcT;
} at_cereg_resp_t;

/* CGDCONT: Define PDP Context
 * ---------------------------
 * Specifies PDP context parameter values for a PDP context identified by
 * the (local) context identification parameter, <cid>.
 * Read parameter command:
 * +CGDCONT? returns +CGDCONT: <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,
 *    <h_comp>[,<IPv4AddrAlloc>[,<emergency indication>[,<P-CSCF_
 *    discovery>[,<IM_CN_Signalling_Flag_Ind>]]]][<CR><LF>
 *    +CGDCONT: <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,
 *    <h_comp>[,<IPv4AddrAlloc>[,<emergency indication>[,<P-CSCF_
 *    discovery>[,<IM_CN_Signalling_Flag_Ind>]]]]
 *    [...]]
 *  <cid>   a numeric parameter which specifies a particular PDP
 *      context definition
 *  <PDP_type>  type of packet data protocol ("IP", "IPV6", "IPV4V6")
 *  <APN>   Access Point logical Name
 *  <PDP_addr>  Mobile Equipment PDP address (not applicable to EPS)
 *  <d_comp>  PDP data compression parameter
 *  <h_comp>  PDP header compression parameter
 *  <IPv4AddrAlloc> IPv4 address allocation parameter
 *  <emergency indication>  emergency bearer services support indication
 *  <P-CSCF_discovery>  P-CSCF address discovery parameter
 *  <IM_CN_Signalling_Flag_Ind> IM CN subsystem-related signalling
 *          support indication
 */

/* Optional parameter bitmask */
#define AT_CGDCONT_RESP_IPV4ADDRALLOC_MASK    AT_COMMAND_PARAM7
#define AT_CGDCONT_RESP_EMERGECY_INDICATION_MASK  AT_COMMAND_PARAM8
#define AT_CGDCONT_RESP_P_CSCF_DISCOVERY_MASK   AT_COMMAND_PARAM9
#define AT_CGDCONT_RESP_IM_CN_SIGNALLING_FLAG_IND_MASK  AT_COMMAND_PARAM10

/* Structure of CGDCONT AT read parameter command */
typedef struct {
#define AT_CGDCONT_RESP_SIZE  8
  int n_pdns;   /* Number of defined PDN contexts in the lists    */
  int cid[AT_CGDCONT_RESP_SIZE];   /* List of PDN context identifiers   */
  int PDP_type[AT_CGDCONT_RESP_SIZE];
  const char* APN[AT_CGDCONT_RESP_SIZE];
  int d_comp[AT_CGDCONT_RESP_SIZE];
  int h_comp[AT_CGDCONT_RESP_SIZE];
} at_cgdcont_get_t;

/* Structure of CGDCONT AT read parameter command */
typedef struct {
  int n_cid;    /* Range of supported PDN context identifiers */
} at_cgdcont_tst_t;

/* CGDCONT AT command response type */
typedef union {
  at_cgdcont_tst_t tst;
  at_cgdcont_get_t get;
} at_cgdcont_resp_t;

/* CGACT: PDP context activate or deactivate
 * -----------------------------------------
 * Read command returns the current activation states for all the defined
 * PDP contexts.
 * Read parameter command:
 * +CGACT? returns +CGACT: <cid>,<state>[<CR><LF>+CGACT: <cid>,<state>[...]]
 *  <cid> a numeric parameter which specifies a particular PDP context
 *    definition
 *  <state> numeric parameter that indicates the state of PDP context
 *    activation
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CGACT_MASK  AT_RESPONSE_NO_PARAM

/* CGACT AT command response type */
typedef struct {
#define AT_CGACT_RESP_SIZE  AT_CGDCONT_RESP_SIZE
  int n_pdns;   /* Number of defined PDN contexts in the lists    */
  int cid[AT_CGACT_RESP_SIZE];   /* List of PDN context identifiers   */
  int state[AT_CGACT_RESP_SIZE]; /* List of PDN context states    */
} at_cgact_resp_t;

/* CGPADDR: Show PDP address(es)
 * -----------------------------
 * Returns a list of PDP addresses for the specified context identifiers.
 * +CGPADDR= returns +CGPADDR: <cid>[,PDP_addr_1[,PDP_addr_2]][<CR><LF>
 *         +CGPADDR: <cid>[,PDP_addr_1[,PDP_addr_2]][...]]
 *  <cid>   a numeric parameter which specifies a particular PDP
 *      context definition
 *  <PDP_addr_x>  string that identifies the MT in the address space
 *      applicable to the PDP
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CGPADDR_MASK  AT_RESPONSE_NO_PARAM

/* CGPADDR AT command response type */
typedef struct {
#define AT_CGPADDR_RESP_SIZE  AT_CGDCONT_RESP_SIZE
  int n_pdns;   /* Number of defined PDP addresses in the lists */
  int cid[AT_CGPADDR_RESP_SIZE]; /* List of PDN context identifiers */
  /* List of IPv4 addresses     */
  const char* PDP_addr_1[AT_CGPADDR_RESP_SIZE];
  /* List of IPv6 addresses     */
  const char* PDP_addr_2[AT_CGPADDR_RESP_SIZE];
} at_cgpaddr_resp_t;

/* CGEV: Unsolicited result
 * ------------------------
 * Returns unsolicited result codes in case of certain events occurring
 * in the Packet Domain MT or the network.
 * +CGREV: <code> <cid>
 *  <code>    unsolicited result code
 *  <cid>   a numeric parameter which specifies a particular PDP
 *      context definition
 */

/* Optional parameter bitmask */
#define AT_RESPONSE_CGEV_MASK AT_RESPONSE_NO_PARAM

/* Unsolicited result codes */
#define AT_CGEV_RESP_NW_PDN_ACT   (-1)  /* Not applicable for LTE */
#define AT_CGEV_RESP_ME_PDN_ACT   NET_PDN_MT_DEFAULT_ACT
#define AT_CGEV_RESP_NW_PDN_DEACT NET_PDN_NW_DEFAULT_DEACT
#define AT_CGEV_RESP_ME_PDN_DEACT NET_PDN_MT_DEFAULT_DEACT
#define AT_CGEV_RESP_NW_ACT   NET_PDN_NW_DEDICATED_ACT
#define AT_CGEV_RESP_ME_ACT   NET_PDN_MT_DEDICATED_ACT
#define AT_CGEV_RESP_NW_DEACT   NET_PDN_NW_DEDICATED_DEACT
#define AT_CGEV_RESP_ME_DEACT   NET_PDN_MT_DEDICATED_DEACT

/* CGEV unsolicited result type */
typedef struct {
  int code;
  int cid;
} at_cgev_resp_t;

/*
 * ==========================================
 * Global AT command response type definition
 * ==========================================
 */

/* AT command response type */
typedef struct {
  at_command_id_t id;   /* AT command identifier  */
  at_command_type_t type; /* AT command type    */
  int mask;     /* Optional parameter bitmask */
  int cause_code;   /* AT command error cause code  */
  union {
    at_cgsn_resp_t cgsn;
    at_cgmi_resp_t cgmi;
    at_cgmm_resp_t cgmm;
    at_cgmr_resp_t cgmr;
    at_cimi_resp_t cimi;
    at_cfun_resp_t cfun;
    at_cpin_resp_t cpin;
    at_csq_resp_t csq;
    at_cesq_resp_t cesq;
    at_clac_resp_t clac;
    at_cmee_resp_t cmee;
    at_cnum_resp_t cnum;
    at_clck_resp_t clck;
    at_cops_resp_t cops;
    at_cgatt_resp_t cgatt;
    at_creg_resp_t creg;
    at_cgreg_resp_t cgreg;
    at_cereg_resp_t cereg;
    at_cgdcont_resp_t cgdcont;
    at_cgact_resp_t cgact;
    at_cgpaddr_resp_t cgpaddr;
    at_cgev_resp_t cgev;
  } response;
} at_response_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int at_response_encode(char* buffer, const at_response_t* at_response);

#endif /* __AT_RESPONSE_H__*/
