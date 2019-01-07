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

/*

Source      commonDef.h

Version     0.1

Date        2012/02/27

Product     NAS stack

Subsystem   include

Author      Frederic Maurel

Description Contains global common definitions

*****************************************************************************/
#ifndef __COMMONDEF_H__
#define __COMMONDEF_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* boolean_t is also defined in openair2/COMMON/platform_types.h
 * let's protect potential redefinition
 */

#ifndef _BOOLEAN_T_DEFINED_
#define _BOOLEAN_T_DEFINED_

typedef signed char        boolean_t;

#if !defined(TRUE)
#define TRUE               (boolean_t)0x01
#endif

#if !defined(FALSE)
#define FALSE              (boolean_t)0x00
#endif

#define BOOL_NOT(b) (b^TRUE)

#endif /* _BOOLEAN_T_DEFINED_ */

#define NAS_UE_ID_FMT "0x%06x"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

#define RETURNok        (0)
#define RETURNerror     (-1)

/*
 * Name of the environment variable which defines the default directory
 * where the NAS application is executed and where are located files
 * where non-volatile data are stored
 */
#define DEFAULT_NAS_PATH    "PWD"

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
-----------------------------------------------------------------------------
            Standard data type definitions
-----------------------------------------------------------------------------
*/
typedef int8_t      SByte_t;    /* 8 bit  signed integer     */
typedef uint8_t     Byte_t;     /* 8 bit unsigned integer   */


/*
-----------------------------------------------------------------------------
            Common NAS data type definitions
-----------------------------------------------------------------------------
*/

typedef uint8_t     Stat_t;     /* Registration status  */
typedef uint16_t    lac_t;      /* Location Area Code   */
typedef uint8_t     rac_t;      /* Routing Area Code    */
typedef uint16_t    tac_t;      /* Tracking Area Code   */
typedef uint32_t    ci_t;       /* Cell Identifier  */
typedef uint8_t     AcT_t;      /* Access Technology    */

/*
 * International Mobile Subscriber Identity
 */
typedef struct {
  Byte_t length;
  union {
    struct {
      Byte_t digit2:4;
      Byte_t digit1:4;
      Byte_t digit4:4;
      Byte_t digit3:4;
      Byte_t digit6:4;
      Byte_t digit5:4;
      Byte_t digit8:4;
      Byte_t digit7:4;
      Byte_t digit10:4;
      Byte_t digit9:4;
      Byte_t digit12:4;
      Byte_t digit11:4;
      Byte_t digit14:4;
      Byte_t digit13:4;
#define EVEN_PARITY 0
#define ODD_PARITY  1
      Byte_t parity:4;
      Byte_t digit15:4;
    } num;
#define IMSI_SIZE   8
    Byte_t value[IMSI_SIZE];
  } u;
} imsi_t;

#define NAS_IMSI2STR(iMsI_t_PtR,iMsI_sTr, MaXlEn) \
        {\
          int l_offset = 0;\
          int l_ret    = 0;\
          l_ret = snprintf(iMsI_sTr + l_offset, MaXlEn - l_offset, "%u%u%u%u%u",\
	    		  iMsI_t_PtR->u.num.digit1, iMsI_t_PtR->u.num.digit2,\
	    		  iMsI_t_PtR->u.num.digit3, iMsI_t_PtR->u.num.digit4,\
	    		  iMsI_t_PtR->u.num.digit5);\
	      if ((iMsI_t_PtR->u.num.digit6 != 0xf)  && (l_ret > 0)) {\
	    	l_offset += l_ret;\
	    	l_ret = snprintf(iMsI_sTr + l_offset, MaXlEn - l_offset,  "%u", iMsI_t_PtR->u.num.digit6);\
	      }\
	      if (l_ret > 0) {\
	    	l_offset += l_ret;\
	    	l_ret = snprintf(iMsI_sTr + l_offset, MaXlEn - l_offset, "%u%u%u%u%u%u%u%u",\
	    		  iMsI_t_PtR->u.num.digit7, iMsI_t_PtR->u.num.digit8,\
	    		  iMsI_t_PtR->u.num.digit9, iMsI_t_PtR->u.num.digit10,\
	    		  iMsI_t_PtR->u.num.digit11, iMsI_t_PtR->u.num.digit12,\
	    		  iMsI_t_PtR->u.num.digit13, iMsI_t_PtR->u.num.digit14);\
	      }\
	      if ((iMsI_t_PtR->u.num.digit15 != 0xf)   && (l_ret > 0)) {\
	    	l_offset += l_ret;\
	    	l_ret = snprintf(iMsI_sTr + l_offset, MaXlEn - l_offset, "%u", iMsI_t_PtR->u.num.digit15);\
	      }\
	    }

/*
 * Mobile subscriber dialing number
 */
typedef struct {
  Byte_t ext:1;
  /* Type Of Number           */
#define MSISDN_TON_UNKNOWKN     0b000
#define MSISDN_TON_INTERNATIONAL    0b001
#define MSISDN_TON_NATIONAL     0b010
#define MSISDN_TON_NETWORK      0b011
#define MSISDN_TON_SUBCRIBER        0b100
#define MSISDN_TON_ABBREVIATED      0b110
#define MSISDN_TON_RESERVED     0b111
  Byte_t ton:3;
  /* Numbering Plan Identification    */
#define MSISDN_NPI_UNKNOWN      0b0000
#define MSISDN_NPI_ISDN_TELEPHONY   0b0001
#define MSISDN_NPI_GENERIC      0b0010
#define MSISDN_NPI_DATA         0b0011
#define MSISDN_NPI_TELEX        0b0100
#define MSISDN_NPI_MARITIME_MOBILE  0b0101
#define MSISDN_NPI_LAND_MOBILE      0b0110
#define MSISDN_NPI_ISDN_MOBILE      0b0111
#define MSISDN_NPI_PRIVATE      0b1110
#define MSISDN_NPI_RESERVED     0b1111
  Byte_t npi:4;
  /* Dialing Number           */
  struct {
    Byte_t lsb:4;
    Byte_t msb:4;
#define MSISDN_DIGIT_SIZE   10
  } digit[MSISDN_DIGIT_SIZE];
} msisdn_t;

/*
 * International Mobile Equipment Identity
 */
typedef imsi_t imei_t;

/*
 * Public Land Mobile Network identifier
 * PLMN = BCD encoding (Mobile Country Code + Mobile Network Code)
 */
typedef struct {
  Byte_t MCCdigit2:4;
  Byte_t MCCdigit1:4;
  Byte_t MNCdigit3:4;
  Byte_t MCCdigit3:4;
  Byte_t MNCdigit2:4;
  Byte_t MNCdigit1:4;
} plmn_t;

/*
 * Location Area Identification
 */
typedef struct {
  plmn_t plmn;    /* <MCC> + <MNC>    */
  lac_t lac;      /* Location Area Code   */
} lai_t;

/*
 * GPRS Routing Area Identification
 */
typedef struct {
  plmn_t plmn;    /* <MCC> + <MNC>    */
  lac_t lac;      /* Location Area Code   */
  rac_t rac;      /* Routing Area Code    */
} RAI_t;

/*
 * EPS Tracking Area Identification
 */
typedef struct {
  plmn_t plmn;    /* <MCC> + <MNC>    */
  tac_t tac;      /* Tracking Area Code   */
} tai_t;

/*
 * EPS Globally Unique MME Identity
 */
typedef struct {
  plmn_t plmn;    /* <MCC> + <MNC>    */
  uint16_t MMEgid;    /* MME group identifier */
  uint8_t MMEcode;    /* MME code     */
} gummei_t;

/*
 * EPS Globally Unique Temporary UE Identity
 */
typedef struct {
  gummei_t gummei;    /* Globally Unique MME Identity         */
  uint32_t m_tmsi;    /* M-Temporary Mobile Subscriber Identity   */
} GUTI_t;
#define GUTI2STR(GuTi_PtR, GuTi_StR, MaXlEn) \
        {\
          int l_offset = 0;\
          int l_ret    = 0;\
          l_ret += snprintf(GuTi_StR + l_offset,MaXlEn-l_offset, "%03u.",\
	    		  GuTi_PtR->gummei.plmn.MCCdigit3 * 100 +\
                  GuTi_PtR->gummei.plmn.MCCdigit2 * 10 +\
                  GuTi_PtR->gummei.plmn.MCCdigit1);\
          if (l_ret > 0) {\
        	l_offset += l_ret;\
          }  else {\
        	l_offset = MaXlEn;\
          }\
          if (GuTi_PtR->gummei.plmn.MNCdigit1 != 0xf) {\
              l_ret += snprintf(GuTi_StR + l_offset,MaXlEn-l_offset, "%03u|%04x|%02x|%08x",\
    	    		  GuTi_PtR->gummei.plmn.MNCdigit3 * 100 +\
                      GuTi_PtR->gummei.plmn.MNCdigit2 * 10 +\
                      GuTi_PtR->gummei.plmn.MNCdigit1,\
                      GuTi_PtR->gummei.MMEgid,\
                      GuTi_PtR->gummei.MMEcode,\
                      GuTi_PtR->m_tmsi);\
          } else {\
              l_ret += snprintf(GuTi_StR + l_offset,MaXlEn-l_offset, "%02u|%04x|%02x|%08x",\
                      GuTi_PtR->gummei.plmn.MNCdigit2 * 10 +\
                      GuTi_PtR->gummei.plmn.MNCdigit1,\
                      GuTi_PtR->gummei.MMEgid,\
                      GuTi_PtR->gummei.MMEcode,\
                      GuTi_PtR->m_tmsi);\
          }\
	    }




/* Checks PLMN validity */
#define PLMN_IS_VALID(plmn) (((plmn).MCCdigit1 &    \
                              (plmn).MCCdigit2 &    \
                              (plmn).MCCdigit3) != 0x0F)

/* Checks TAC validity */
#define TAC_IS_VALID(tac)   (((tac) != 0x0000) && ((tac) != 0xFFF0))

/* Checks TAI validity */
#define TAI_IS_VALID(tai)   (PLMN_IS_VALID((tai).plmn) &&   \
                             TAC_IS_VALID((tai).tac))
/*
 * A list of PLMNs
 */
#define PLMN_LIST_T(SIZE) struct {Byte_t n_plmns; plmn_t plmn[SIZE];}

/*
 * A list of TACs
 */
#define TAC_LIST_T(SIZE) struct {Byte_t n_tacs; TAC_t tac[SIZE];}

/*
 * A list of TAIs
 */
#define TAI_LIST_T(SIZE) struct {Byte_t n_tais; tai_t tai[SIZE];}

typedef enum eps_protocol_discriminator_e {
  /* Protocol discriminator identifier for EPS Mobility Management */
  EPS_MOBILITY_MANAGEMENT_MESSAGE =   0x7,

  /* Protocol discriminator identifier for EPS Session Management */
  EPS_SESSION_MANAGEMENT_MESSAGE =    0x2,
} eps_protocol_discriminator_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* __COMMONDEF_H__*/
