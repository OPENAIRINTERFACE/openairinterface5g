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

Source    at_command.h

Version   0.1

Date    2012/03/07

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Defines the ATtention (AT) command set supported by the NAS
    sublayer protocol

*****************************************************************************/
#ifndef __AT_COMMAND_H__
#define __AT_COMMAND_H__

#include "userDef.h"
#include "networkDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Bit masks indicating presence of AT command's optional parameters */
#define AT_COMMAND_PARAM0 0x0000  /* no any parameter is present  */
#define AT_COMMAND_PARAM1 0x0001  /* first parameter is present */
#define AT_COMMAND_PARAM2 0x0002  /* 2nd parameter is present */
#define AT_COMMAND_PARAM3 0x0004  /* 3rd parameter is present */
#define AT_COMMAND_PARAM4 0x0008  /* 4th parameter is present */
#define AT_COMMAND_PARAM5 0x0010  /* 5th parameter is present */
#define AT_COMMAND_PARAM6 0x0020  /* 6th parameter is present */
#define AT_COMMAND_PARAM7 0x0040  /* 7th parameter is present */
#define AT_COMMAND_PARAM8 0x0080  /* 8th parameter is present */
#define AT_COMMAND_PARAM9 0x0100  /* 9th parameter is present */
#define AT_COMMAND_PARAM10  0x0200  /* 10th parameter is present  */
#define AT_COMMAND_PARAM11  0x0400  /* 11th parameter is present  */
#define AT_COMMAND_PARAM12  0x0800  /* 12th parameter is present  */
#define AT_COMMAND_PARAM13  0x1000  /* 13th parameter is present  */
#define AT_COMMAND_PARAM14  0x2000  /* 14th parameter is present  */
#define AT_COMMAND_PARAM15  0x4000  /* 15th parameter is present  */
#define AT_COMMAND_PARAM16  0x8000  /* 16th parameter is present  */

/* Value of the mask parameter for AT commands without any parameters */
#define AT_COMMAND_NO_PARAM AT_COMMAND_PARAM0

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * AT command identifiers
 * ----------------------
 *  Those currently supported by our Mobile Equipment (ME)
 */
typedef enum {
  /*
   * General commands
   * ----------------
   */
  AT_CGSN=1,  /* Request Product Serial Number identification (IMEI)  */
  AT_CGMI,  /* Request manufacturer identification      */
  AT_CGMM,  /* Request model identification       */
  AT_CGMR,  /* Request revision identification      */
  AT_CIMI,  /* Request International Mobile Subscriber Identity */
  /*
   * Mobile Termination control and status commands
   * ----------------------------------------------
   */
  AT_CFUN,  /* Set phone functionality        */
  AT_CPIN,  /* Enter PIN            */
  AT_CSQ, /* Signal quality         */
  AT_CESQ,  /* Extented signal quality        */
  AT_CLAC,  /* List all available AT commands     */
  /*
   * Mobile Termination errors
   * -------------------------
   */
  AT_CMEE,  /* Report mobile termination error      */
  /*
   *  Network service related commands
   * ---------------------------------
   */
  AT_CNUM,  /* Subscriber number          */
  AT_CLCK,  /* Facility lock          */
  AT_COPS,  /* PLMN selection         */
  AT_CREG,  /* Network registration         */
  /*
   * Commands for Packet Domain
   * ---------------------------
   */
  AT_CGATT, /* PS attach/detach         */
  AT_CGREG, /* GPRS network registration status     */
  AT_CEREG, /* EPS network registration status      */
  AT_CGDCONT, /* Define PDP Context         */
  AT_CGACT, /* PDP context activate or deactivate     */
  AT_CGPADDR, /* Show PDP address(es)         */
  AT_CGEV,  /* Packet Domain event unsolicited result code    */
  AT_COMMAND_ID_MAX
} at_command_id_t;

/*
 * AT command types
 * ----------------
 *  - AT action command, used to execute a particular function, which
 *    generally involves more than the simple storage of a value.
 *  - AT set parameter command, used to store a value or values of
 *    subparameters.
 *  - AT read parameter command, used to check the current values of
 *    subparameters.
 *  - AT test parameter command, used to test the existence of the
 *    command and to give information about the type of its subparameters.
 */
typedef enum {
  AT_COMMAND_ACT, /* AT action command (ATCMD)    */
  AT_COMMAND_SET, /* AT set parameter command (ATCMD=)  */
  AT_COMMAND_GET, /* AT read parameter command (ATCMD?) */
  AT_COMMAND_TST, /* AT test parameter command (ATCMD=?)  */
  AT_COMMAND_TYPE_MAX
} at_command_type_t;

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
 *  no parameters
 */

/* CGSN AT command type */
#define AT_COMMAND_CGSN_MASK  AT_COMMAND_NO_PARAM
typedef struct at_cgsn_s {
  // This dummy element is to avoid CLANG warning: empty struct has size 0 in C, size 1 in C++
  // To be removed if the structure is filled
  uint32_t dummy;
} at_cgsn_t;

/* CGMI: Request manufacturer identification
 * -----------------------------------------
 * Returns information text intended to permit the user to identify the
 * manufacturer of the Mobile Equipment to which it is connected to.
 * Action command:
 *  no parameters
 */

/* CGMI AT command type */
#define AT_COMMAND_CGMI_MASK  AT_COMMAND_NO_PARAM
typedef struct at_cgmi_s {
  // This dummy element is to avoid CLANG warning: empty struct has size 0 in C, size 1 in C++
  // To be removed if the structure is filled
  uint32_t dummy;
} at_cgmi_t;

/* CGMM: Request model identification
 * ----------------------------------
 * Returns information text intended to permit the user to identify the
 * specific model of the Mobile Equipment to which it is connected to.
 * Action command:
 *  no parameters
 */

/* CGMM AT command type */
#define AT_COMMAND_CGMM_MASK  AT_COMMAND_NO_PARAM
typedef struct at_cgmm_s {
  // This dummy element is to avoid CLANG warning: empty struct has size 0 in C, size 1 in C++
  // To be removed if the structure is filled
  uint32_t dummy;
} at_cgmm_t;

/* CGMR: Request revision identification
 * -------------------------------------
 * Returns information text intended to permit the user to identify the
 * version, revision level or date, or other pertinent information of the
 * Mobile Equipment to which it is connected to.
 * Action command:
 *  no parameters
 */

/* CGMR AT command type */
#define AT_COMMAND_CGMR_MASK  AT_COMMAND_NO_PARAM
typedef struct at_cgmr_s {
  // This dummy element is to avoid CLANG warning: empty struct has size 0 in C, size 1 in C++
  // To be removed if the structure is filled
  uint32_t dummy;
} at_cgmr_t;

/* CIMI: Request International Mobile Subscriber Identity
 * ------------------------------------------------------
 * Returns <IMSI>, which is intended to permit the user to identify the
 * individual SIM card or active application in the UICC (GSM or USIM)
 * which is attached to the Mobile Equipment to which it is connected to.
 * Action command:
 *  no parameters
 */

/* CIMI AT command type */
#define AT_COMMAND_CIMI_MASK  AT_COMMAND_NO_PARAM
typedef struct at_cimi_s {
  // This dummy element is to avoid CLANG warning: empty struct has size 0 in C, size 1 in C++
  // To be removed if the structure is filled
  uint32_t dummy;
} at_cimi_t;

/*
 * ==============================================
 * Mobile Termination control and status commands
 * ==============================================
 */
/* CFUN: Set phone functionality
 * -----------------------------
 * Used to set the Mobile Equipment to different power consumption states
 * Parameter command:
 * +CFUN=[<fun>[,<rst>]]
 *  <fun> level of functionality
 *  <rst> resetting parameter
 */

/* Functionality levels */
#define AT_CFUN_MIN 0   /* minimum functionality      */
#define AT_CFUN_FULL  1   /* full functionality     */
#define AT_CFUN_SEND  2   /* disable phone transmit RF circuits only  */
#define AT_CFUN_RECV  3   /* disable phone receive RF circuits only */
#define AT_CFUN_BOTH  4   /* disable phone both transmit and receive
           * RF circuits        */
#define AT_CFUN_MAX   AT_CFUN_BOTH
/* Maximum value of supported functionality
 * level          */
#define AT_CFUN_FUN_DEFAULT AT_CFUN_MIN
/* Default functionality level applied when
 * no value is given      */
/* Resetting parameter values */
#define AT_CFUN_NORST 0   /* do not reset the ME before setting it
           * to <fun> power level (default when <rst>
           * is not given)        */
#define AT_CFUN_RST 1   /* reset the ME before setting it to <fun>
           * power level        */
#define AT_CFUN_RST_DEFAULT AT_CFUN_NORST
/* Default resetting action applied when
 * no value is given      */
/* Optional parameter bitmask */
#define AT_CFUN_FUN_MASK  AT_COMMAND_PARAM1
#define AT_CFUN_RST_MASK  AT_COMMAND_PARAM2

/* CFUN AT command type */
typedef struct {
  int fun;
  int rst;
} at_cfun_t;

/* CPIN: Enter PIN
 * ---------------
 * Used to enter MT passwords which are needed before any other
 * functionality of the MT can be used
 * Parameter command:
 * +CPIN=<pin>[,<newpin>]
 *  <pin>  personal identification number
 *  <newpin> new personal identification number
 */

/* Optional parameter bitmask */
#define AT_CPIN_NEWPIN_MASK AT_COMMAND_PARAM2

/* CPIN AT command type */
typedef struct {
#define AT_CPIN_SIZE  USER_PIN_SIZE /* Number of characters in pin parameter */
  char pin[AT_CPIN_SIZE+1];
  char newpin[AT_CPIN_SIZE+1];
} at_cpin_t;

/* CSQ: Signal quality
 * -------------------
 * Returns received signal strength indication and channel bit error rate
 * from the Mobile Equipment
 * Action command:
 *  no parameters
 */

/* CSQ AT command type */
#define AT_COMMAND_CSQ_MASK AT_COMMAND_NO_PARAM
typedef struct at_csq_s {
  // This dummy element is to avoid CLANG warning: empty struct has size 0 in C, size 1 in C++
  // To be removed if the structure is filled
  uint32_t dummy;
} at_csq_t;

/* CESQ: Extented signal quality
 * -----------------------------
 * Returns received signal quality parameters.
 * Action command:
 *  no parameters
 */

/* CESQ AT command type */
#define AT_COMMAND_CESQ_MASK  AT_COMMAND_NO_PARAM
typedef struct at_cesq_s {
  // This dummy element is to avoid CLANG warning: empty struct has size 0 in C, size 1 in C++
  // To be removed if the structure is filled
  uint32_t dummy;
} at_cesq_t;

/* CLAC: List all available AT commands
 * ------------------------------------
 * Returns one or more lines of AT Commands that are available for the user.
 * Action command:
 *  no parameters
 */

/* CLAC AT command type */
#define AT_COMMAND_CLAC_MASK  AT_COMMAND_NO_PARAM
typedef struct at_clac_s {
  // This dummy element is to avoid CLANG warning: empty struct has size 0 in C, size 1 in C++
  // To be removed if the structure is filled
  uint32_t dummy;
} at_clac_t;

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
 * <n> = 0, disables result code and displays ERROR instead.
 * <n> = 1, enables result code and displays numeric <err>
 * <n> = 2, enables result code and displays verbose <err>
 * Parameter command:
 * +CMEE=[<n>]
 *  <n> numeric parameter
 */

/* Numeric parameter values */
#define AT_CMEE_OFF 0 /* disable result code      */
#define AT_CMEE_NUMERIC 1 /* enable numeric result code     */
#define AT_CMEE_VERBOSE 2 /* enable verbose result code     */
#define AT_CMEE_N_MIN   AT_CMEE_OFF
/* Minimum value of supported numeric parameter */
#define AT_CMEE_N_MAX   AT_CMEE_VERBOSE
/* Maximum value of supported numeric parameter */
#define AT_CMEE_N_DEFAULT AT_CMEE_OFF
/* Default value of supported numeric parameter */

/* Optional parameter bitmask */
#define AT_CMEE_N_MASK    AT_COMMAND_PARAM1

/* CMEE AT command type */
typedef struct {
  int n;
} at_cmee_t;

/*
 * ================================
 * Network service related commands
 * ================================
 */
/* CNUM: Subscriber number
 * -----------------------
 * Returns the MSISDNs related to the subscriber.
 * Action command:
 *  no parameters
 */

/* CNUM AT command type */
#define AT_COMMAND_CNUM_MASK  AT_COMMAND_NO_PARAM
typedef struct at_cnum_s {
  // This dummy element is to avoid CLANG warning: empty struct has size 0 in C, size 1 in C++
  // To be removed if the structure is filled
  uint32_t dummy;
} at_cnum_t;

/* CLCK: Facility lock
 * -------------------
 * Used to lock, unlock or interrogate a MT or a network facility <fac>.
 * Action command:
 * +CLCK=<fac>,<mode>[,<passwd>]
 *  <fac>  facility value
 *  <mode>   operation mode
 *  <passwd> password (pin code)
 */

/* Facility values */
#define AT_CLCK_SC  "SC" /* SIM lock/unlock       */

/* Operation mode */
#define AT_CLCK_UNLOCK  0 /* Unlock (enable) a service      */
#define AT_CLCK_LOCK  1 /* Lock (disable) a service     */
#define AT_CLCK_STATUS  2 /* Query the status of a network service  */

/* Optional parameter bitmask */
#define AT_CLCK_PASSWD_MASK AT_COMMAND_PARAM3

/* CLCK AT command type */
typedef struct {
#define AT_CLCK_FAC_SIZE  2
  char fac[AT_CLCK_FAC_SIZE+1];
  int mode;
#define AT_CLCK_PASSWD_SIZE USER_PIN_SIZE
  char passwd[AT_CLCK_PASSWD_SIZE+1];
} at_clck_t;

/* COPS: PLMN selection
 * --------------------
 * Forces an attempt to select and register the GSM/UMTS/EPS network operator
 * using the SIM/USIM card installed in the currently selected card slot.
 * Parameter command:
 * +COPS=[<mode>[,<format>[,<oper>[,<AcT>]]]]
 *  <mode>   operation mode of the network selection
 *  <format> representation format of the network operator
 *  <oper>   APN operator identifier
 *  <AcT>  access technology
 */

/* Network selection operation modes */
#define AT_COPS_AUTO  0 /* automatic mode       */
#define AT_COPS_MANUAL  1 /* manual mode        */
#define AT_COPS_DEREG 2 /* deregister from network      */
#define AT_COPS_FORMAT  3 /* set only <format> for read command +COPS?  */
#define AT_COPS_MANAUTO 4 /* manual/automatic mode; if manual selection
         * fails, automatic mode is entered   */
#define AT_COPS_MODE_DEFAULT  AT_COPS_AUTO
/* Default operation mode applied when no
 * value is given       */
/* Representation formats */
#define AT_COPS_LONG  NET_FORMAT_LONG  /* long format alphanumeric  */
#define AT_COPS_SHORT NET_FORMAT_SHORT /* short format alphanumeric */
#define AT_COPS_NUM NET_FORMAT_NUM   /* numeric     */
#define AT_COPS_FORMAT_MIN  AT_COPS_LONG
/* Minimum value of supported representation
 * format         */
#define AT_COPS_FORMAT_MAX  AT_COPS_NUM
/* Maximum value of supported representation
 * format         */
#define AT_COPS_FORMAT_DEFAULT  AT_COPS_LONG
/* Default representation format applied when
 * no value is given        */
/* Access technology indicators */
#define AT_COPS_GSM   NET_ACCESS_GSM     /* GSM   */
#define AT_COPS_COMPACT   NET_ACCESS_COMPACT /* GSM Compact */
#define AT_COPS_UTRAN   NET_ACCESS_UTRAN   /* UTRAN   */
#define AT_COPS_EGPRS   NET_ACCESS_EGPRS   /* GSM w/EGPRS */
#define AT_COPS_HSDPA   NET_ACCESS_HSDPA   /* UTRAN w/HSDPA */
#define AT_COPS_HSUPA   NET_ACCESS_HSUPA   /* UTRAN w/HSUPA */
#define AT_COPS_HSDUPA    NET_ACCESS_HSDUPA  /* w/HSDPA and HSUPA */
#define AT_COPS_EUTRAN    NET_ACCESS_EUTRAN  /* E-UTRAN   */
#define AT_COPS_ACT_MIN   AT_COPS_GSM
/* Minimum value of supported access
 * technology         */
#define AT_COPS_ACT_MAX   AT_COPS_EUTRAN
/* Maximum value of supported access
 * technology         */
#define AT_COPS_ACT_DEFAULT AT_COPS_GSM
/* Default access technology applied when
 * no value is given        */

/* Optional parameter bitmask */
#define AT_COPS_MODE_MASK AT_COMMAND_PARAM1
#define AT_COPS_FORMAT_MASK AT_COMMAND_PARAM2
#define AT_COPS_OPER_MASK AT_COMMAND_PARAM3
#define AT_COPS_ACT_MASK  AT_COMMAND_PARAM4

/* COPS AT command type */
typedef struct {
  int mode;
  int format;
#define AT_COPS_NUM_SIZE  NET_FORMAT_NUM_SIZE
  /* Size of numeric representation format    */
#define AT_COPS_LONG_SIZE NET_FORMAT_LONG_SIZE
  /* Size of long alphanumeric representation format  */
#define AT_COPS_SHORT_SIZE  NET_FORMAT_SHORT_SIZE
  /* Size of short alphanumeric representation format */
  network_plmn_t plmn;
  int AcT;
} at_cops_t;

/* CREG: network registration
 * --------------------------
 * When <n>=1, returns +CREG: <stat>, the Mobile Equipment's circuit mode
 * network registration status in GERA/UTRA/E-UTRA Network.
 * When <n>=2, returns +CREG: <stat>[,<lac>,<ci>[,<AcT>]], the Mobile
 * Equipment's circuit mode network registration status and location information
 * in GERA/UTRA/E-UTRA Network.
 * Parameter command:
 * +CREG=[<n>]
 *  <n> numeric parameter
 */

/* Numeric parameter values */
#define AT_CREG_OFF 0 /* disable network registration unsolicited
         * result code        */
#define AT_CREG_ON  1 /* enable network registration unsolicited
         * result code +CREG: <stat>      */
#define AT_CREG_BOTH  2 /* enable network registration and location
         * information unsolicited result code
         * +CREG: <stat>[,<lac>,<ci>[,<AcT>]]   */
#define AT_CREG_N_MIN   AT_CREG_OFF
/* Minimum value of supported unsolicited
 * result code        */
#define AT_CREG_N_MAX   AT_CREG_BOTH
/* Maximum value of supported unsolicited
 * result code        */
#define AT_CREG_N_DEFAULT AT_CREG_OFF
/* Default unsolicited result code applied
 * when no value is given     */

/* Optional parameter bitmask */
#define AT_CREG_N_MASK    AT_COMMAND_PARAM1

/* CREG AT command type */
typedef struct {
  int n;
} at_creg_t;

/*
 * ==========================
 * Commands for Packet Domain
 * ==========================
 */
/* CGATT: EPS service attach/detach
 * --------------------------------
 * Used to attach the MT to, or detach the MT from, the EPS service.
 * Action command (with optional parameters):
 * +CGATT=[<state>]
 *  <state> numeric parameter that indicates the state of EPS attachment
 */

/* PS attachment status */
#define AT_CGATT_DETACHED 0 /* To detach from EPS service    */
#define AT_CGATT_ATTACHED 1 /* To attach from EPS service    */
#define AT_CGATT_STATE_MIN    AT_CGATT_DETACHED
/* Minimum value of EPS attachment code */
#define AT_CGATT_STATE_MAX    AT_CGATT_ATTACHED
/* Maximum value of EPS attachment code */

/* Optional parameter bitmask */
#define AT_CGATT_STATE_MASK   AT_COMMAND_PARAM1

/* CGATT AT command type */
typedef struct {
  int state;
} at_cgatt_t;

/* CGREG: GPRS network registration status
 * ---------------------------------------
 * When <n>=1, returns +CGREG: <stat>, the Mobile Equipment's GPRS network
 * registration status in GERA/UTRA Network.
 * When <n>=2, returns +CGREG: <stat>[,<lac>,<ci>[,<AcT>,<rac>]], the Mobile
 * Equipment's GPRS network registration status and location information in
 * GERA/UTRA Network.
 * Parameter command:
 * +CGREG=[<n>]
 *  <n> numeric parameter
 */

/* Numeric parameter values */
#define AT_CGREG_OFF  0 /* disable network registration unsolicited
         * result code        */
#define AT_CGREG_ON 1 /* enable network registration unsolicited
         * result code +CGREG: <stat>     */
#define AT_CGREG_BOTH 2 /* enable network registration and location
         * information unsolicited result code
         * +CGREG: <stat>[,<lac>,<ci>[,<AcT>,<rac>]]  */
#define AT_CGREG_N_MIN    AT_CGREG_OFF
/* Minimum value of supported unsolicited
 * result code        */
#define AT_CGREG_N_MAX    AT_CGREG_BOTH
/* Maximum value of supported unsolicited
 * result code        */
#define AT_CGREG_N_DEFAULT  AT_CGREG_OFF
/* Default unsolicited result code applied
 * when no value is given     */

/* Optional parameter bitmask */
#define AT_CGREG_N_MASK   AT_COMMAND_PARAM1

/* CGREG AT command type */
typedef struct {
  int n;
} at_cgreg_t;

/* CEREG: EPS network registration status
 * --------------------------------------
 * When <n>=1, returns +CEREG: <stat>, the Mobile Equipment's EPS services
 * registration status in EUTRA Network.
 * When <n>=2, returns +CEREG: <stat>[,<tac>,<ci>[,<AcT>]], the Mobile
 * Equipment's EPS services registration status and location information in
 * EUTRA Network.
 * Parameter command:
 * +CEREG=[<n>]
 *  <n> numeric parameter
 */

/* Numeric parameter values */
#define AT_CEREG_OFF  0 /* disable network registration unsolicited
         * result code        */
#define AT_CEREG_ON 1 /* enable network registration unsolicited
         * result code +CEREG: <stat>     */
#define AT_CEREG_BOTH 2 /* enable network registration and location
         * information unsolicited result code
         * +CEREG: <stat>[,<tac>,<ci>[,<AcT>]]  */
#define AT_CEREG_N_MIN    AT_CEREG_OFF
/* Minimum value of supported unsolicited
 * result code        */
#define AT_CEREG_N_MAX    AT_CEREG_BOTH
/* Maximum value of supported unsolicited
 * result code        */
#define AT_CEREG_N_DEFAULT  AT_CEREG_OFF
/* Default unsolicited result code applied
 * when no value is given     */

/* Optional parameter bitmask */
#define AT_CEREG_N_MASK   AT_COMMAND_PARAM1

/* CEREG AT command type */
typedef struct {
  int n;
} at_cereg_t;

/* CGDCONT: Define PDP Context
 * ---------------------------
 * Specifies PDP context parameter values for a PDP context identified by
 * the (local) context identification parameter, <cid>.
 * Parameter command:
 * +CGDCONT=[<cid>[,<PDP_type>[,<APN>[,<PDP_addr>[,<d_comp>[,<h_comp>
 *      [,<IPv4AddrAlloc>[,<emergency indication>[,<P-CSCF_discovery>
 *      [,<IM_CN_Signalling_Flag_Ind>]]]]]]]]]]
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

/* PDP context identifier */
#define AT_CGDCONT_CID_MIN  1 /* Minimum value of context identifier  */
#define AT_CGDCONT_CID_DEFAULT    AT_CGDCONT_CID_MIN
/* PDP data compression parameter values */
#define AT_CGDCONT_D_COMP_OFF 0 /* PDP data compression is disabled   */
#define AT_CGDCONT_D_COMP_ON  1 /* manufacturer preferred compression   */
#define AT_CGDCONT_D_COMP_V42B  2 /* V.42bis          */
#define AT_CGDCONT_D_COMP_V44 3 /* V.44         */
#define AT_CGDCONT_D_COMP_MIN   AT_CGDCONT_D_COMP_OFF
#define AT_CGDCONT_D_COMP_MAX   AT_CGDCONT_D_COMP_V44
#define AT_CGDCONT_D_COMP_DEFAULT AT_CGDCONT_D_COMP_OFF
/* PDP header compression parameter values */
#define AT_CGDCONT_H_COMP_OFF 0 /* PDP header compression is disabled   */
#define AT_CGDCONT_H_COMP_ON  1 /* manufacturer preferred compression   */
#define AT_CGDCONT_H_COMP_1144  2 /* RFC1144 (applicable for SNDCP only)  */
#define AT_CGDCONT_H_COMP_2507  3 /* RFC2507          */
#define AT_CGDCONT_H_COMP_3095  4 /* RFC3095 (applicable for PDCP only)   */
#define AT_CGDCONT_H_COMP_MIN   AT_CGDCONT_H_COMP_OFF
#define AT_CGDCONT_H_COMP_MAX   AT_CGDCONT_H_COMP_3095
#define AT_CGDCONT_H_COMP_DEFAULT AT_CGDCONT_H_COMP_OFF
/* IPv4AddrAlloc parameter values */
#define AT_CGDCONT_IPV4_NAS 0 /* IPv4 Address Allocation through NAS
           * Signalling         */
#define AT_CGDCONT_IPV4_DHCP  1 /* IPv4 Address Allocation through DHCP */
#define AT_CGDCONT_IPV4_MIN   AT_CGDCONT_IPV4_NAS
#define AT_CGDCONT_IPV4_MAX   AT_CGDCONT_IPV4_DHCP
#define AT_CGDCONT_IPV4_DEFAULT   AT_CGDCONT_IPV4_NAS
/* emergency_indication parameter values */
#define AT_CGDCONT_EBS_OFF  0 /* PDP context is not for emergency bearer
           * services         */
#define AT_CGDCONT_EBS_ON 1 /* PDP context is for emergency bearer
           * services         */
#define AT_CGDCONT_EBS_MIN    AT_CGDCONT_EBS_OFF
#define AT_CGDCONT_EBS_MAX    AT_CGDCONT_EBS_ON
#define AT_CGDCONT_EBS_DEFAULT    AT_CGDCONT_EBS_OFF
/* P_CSCF_discovery parameter values */
#define AT_CGDCONT_PCSCF_OFF  0 /* Preference of P-CSCF address discovery
           * not influenced by +CGDCONTPDP    */
#define AT_CGDCONT_PCSCF_NAS  1 /* Preference of P-CSCF address discovery
           * through NAS Signalling     */
#define AT_CGDCONT_PCSCF_DHCP 2 /* Preference of P-CSCF address discovery
           * through DHCP       */
#define AT_CGDCONT_PCSCF_MIN    AT_CGDCONT_PCSCF_OFF
#define AT_CGDCONT_PCSCF_MAX    AT_CGDCONT_PCSCF_DHCP
#define AT_CGDCONT_PCSCF_DEFAULT  AT_CGDCONT_PCSCF_OFF
/* IM_CN_Signalling_Flag_Ind parameter values */
#define AT_CGDCONT_IM_CN_OFF  0 /* UE indicates that the PDP context is not
           * for IM CN subsystem-related signalling
           * only   */
#define AT_CGDCONT_IM_CN_ON 1 /* UE indicates that the PDP context is for
           * IM CN subsystem-related signalling
           * only   */
#define AT_CGDCONT_IM_CM_MIN    AT_CGDCONT_IM_CN_OFF
#define AT_CGDCONT_IM_CM_MAX    AT_CGDCONT_IM_CN_ON
#define AT_CGDCONT_IM_CM_DEFAULT  AT_CGDCONT_IM_CN_OFF

/* Optional parameter bitmask */
#define AT_CGDCONT_CID_MASK       AT_COMMAND_PARAM1
#define AT_CGDCONT_PDP_TYPE_MASK      AT_COMMAND_PARAM2
#define AT_CGDCONT_APN_MASK       AT_COMMAND_PARAM3
#define AT_CGDCONT_PDP_ADDR_MASK      AT_COMMAND_PARAM4
#define AT_CGDCONT_D_COMP_MASK        AT_COMMAND_PARAM5
#define AT_CGDCONT_H_COMP_MASK        AT_COMMAND_PARAM6
#define AT_CGDCONT_IPV4ADDRALLOC_MASK     AT_COMMAND_PARAM7
#define AT_CGDCONT_EMERGECY_INDICATION_MASK   AT_COMMAND_PARAM8
#define AT_CGDCONT_P_CSCF_DISCOVERY_MASK    AT_COMMAND_PARAM9
#define AT_CGDCONT_IM_CN_SIGNALLING_FLAG_IND_MASK AT_COMMAND_PARAM10

/* CGDCONT AT command type */
typedef struct {
  int cid;
#define AT_CGDCONT_PDP_SIZE 6   /* PDP_type may be "IP", "IPV6", "IPV4V6" */
  char PDP_type[AT_CGDCONT_PDP_SIZE+1];
#define AT_CGDCONT_APN_SIZE 100 /* number of characters in APN parameter  */
  char APN[AT_CGDCONT_APN_SIZE+1];
#define AT_CGDCONT_ADDR_SIZE  63  /* number of characters in PDP addr
             * parameter (IPv6 address notation)      */
  char PDP_addr[AT_CGDCONT_ADDR_SIZE+1];  /* Does not apply to EPS      */
  int d_comp;
  int h_comp;
  int IPv4AddrAlloc;
  int emergency_indication;
  int P_CSCF_discovery;
  int IM_CN_Signalling_Flag_Ind;
} at_cgdcont_t;

/* CGACT: PDP context activate or deactivate
 * -----------------------------------------
 * Used to activate or deactivate the specified PDP context(s).
 * Action command (with optional parameters):
 * +CGACT=[<state>[,<cid>[,<cid>[,...]]]]
 *  <state> numeric parameter that indicates the state of PDP context
 *    activation
 *  <cid> a numeric parameter which specifies a particular PDP context
 *    definition
 */

/* PDP context identifier */
#define AT_CGACT_CID_MIN  1 /* Minimum value of context identifier  */
/* PDP context activation status */
#define AT_CGACT_DEACTIVATED  0 /* To deactivate the PDP context  */
#define AT_CGACT_ACTIVATED  1 /* To activate the PDP context  */
#define AT_CGACT_STATE_MIN  AT_CGACT_DEACTIVATED
#define AT_CGACT_STATE_MAX  AT_CGACT_ACTIVATED
#define AT_CGACT_STATE_DEFAULT  AT_CGACT_DEACTIVATED

/* Optional parameter bitmask */
#define AT_CGACT_STATE_MASK   AT_COMMAND_PARAM1
#define AT_CGACT_CID_MASK   AT_COMMAND_PARAM2

/* CGACT AT command type */
typedef struct {
  int state;
  int cid;
} at_cgact_t;

/* CGPADDR: Show PDP address(es)
 * -----------------------------
 * Returns a list of PDP addresses for the specified context identifiers.
 * +CGPADDR=[<cid>[,<cid>[,...]]]
 *  <cid>   a numeric parameter which specifies a particular PDP
 *      context definition
 */

/* PDP context identifier */
#define AT_CGPADDR_CID_MIN  1 /* Minimum value of context identifier  */

/* Optional parameter bitmask */
#define AT_CGPADDR_CID_MASK   AT_COMMAND_PARAM1

/* CGPADDR AT command type */
typedef struct {
  int cid;
} at_cgpaddr_t;

/*
 * =================================
 * Global AT command type definition
 * =================================
 */

/* AT command type */
typedef struct {
  at_command_id_t id;   /* AT command identifier  */
  at_command_type_t type; /* AT command type    */
  int mask;     /* Optional parameter bitmask */
  union {
    at_cgsn_t cgsn;
    at_cgmm_t cgmm;
    at_cgmr_t cgmr;
    at_cimi_t cimi;
    at_cfun_t cfun;
    at_cpin_t cpin;
    at_csq_t csq;
    at_cesq_t cesq;
    at_clac_t clac;
    at_cmee_t cmee;
    at_cnum_t cnum;
    at_clck_t clck;
    at_cops_t cops;
    at_cgatt_t cgatt;
    at_creg_t creg;
    at_cgreg_t cgreg;
    at_cereg_t cereg;
    at_cgdcont_t cgdcont;
    at_cgact_t cgact;
    at_cgpaddr_t cgpaddr;
  } command;
} at_command_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  at_command_decode()                                       **
 **                                                                        **
 ** Description: Parses AT command line and accordingly fills data struc-  **
 **    ture. The main functions of the AT command parser are:    **
 **        - check the AT command syntax                         **
 **        - fill parameter values into the data structure     **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing AT com-   **
 **       mand line                                  **
 **      length:  Number of bytes that should be processed   **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   at_command:  AT command data structure to be filled     **
 **      Return:  The number of AT commands successfully     **
 **       decoded; RETURNerror if an error occurred  **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int at_command_decode(const char* buffer, int length, at_command_t* at_command);

/****************************************************************************
 **                                                                        **
 ** Name:  at_command_get_list()                                     **
 **                                                                        **
 ** Description: Returns the list of supported AT commands.                **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing the       **
 **       string representaton of the first AT com-  **
 **       mand that is supported by the NAS sublayer **
 **      n_max:   Maximum number of AT commands the buffer   **
 **       may contain                                **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:  The number of AT commands actually suppor- **
 **       ted by the NAS sublayer                    **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int at_command_get_list(const char** buffer, int n_max);

#endif /* __AT_COMMAND_H__*/
