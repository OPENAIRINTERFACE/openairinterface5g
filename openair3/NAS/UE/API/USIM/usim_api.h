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

Source    usim_api.h

Version   0.1

Date    2012/10/09

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Implements the API used by the NAS layer to read/write
    data to/from the USIM application

*****************************************************************************/
#ifndef __USIM_API_H__
#define __USIM_API_H__

#include "commonDef.h"
#include "networkDef.h"
#include "OctetString.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * Subscriber authentication security key
 */
#define USIM_API_K_SIZE         16
#define USIM_API_K_VALUE        "fec86ba6eb707ed08905757b1bb44b8f"

/*
 * The name of the file where are stored data of the USIM application
 */
#define USIM_API_NVRAM_FILENAME ".usim.nvram"

/*
 * The name of the environment variable which defines the directory
 * where the USIM application file is located
 */
#define USIM_API_NVRAM_DIRNAME  "USIM_DIR"

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * USIM TLV parameter structure
 * ----------------------------
 */
#define USIM_TLV_T(SIZE) struct {uint8_t type; uint8_t length; Byte_t value[SIZE];}

/*
 * Non Access Stratum Configuration
 * --------------------------------
 * TODO: To be encoded as BER-TLV
 */
typedef struct {
  /* Determines the NAS signalling priority included in NAS
   * messages             */
#define USIM_NAS_SIGNALLING_PRIORITY_TAG  0x80U
  USIM_TLV_T(1) NAS_SignallingPriority;
  /* Indicates whether the "NMO I, Network Mode of Operation I"
   * indication is applied by the UE        */
#define USIM_NMO_I_BEHAVIOUR_TAG    0x81U
  USIM_TLV_T(1) NMO_I_Behaviour;
  /* Indicates whether attach with IMSI is performed when moving
   * to a non-equivalent PLMN         */
#define USIM_ATTACH_WITH_IMSI_TAG   0x82U
  USIM_TLV_T(1) AttachWithImsi;
  /* Minimum value in minutes for the timer T controlling the
   * periodic search for higher prioritized PLMNs     */
#define USIM_MINIMUM_PERIODIC_SEARCH_TIMER_TAG  0x83U
  USIM_TLV_T(1) MinimumPeriodicSearchTimer;
  /* Indicates whether the extended access barring is applicable
   * for the UE             */
#define USIM_EXTENDED_ACCESS_BARRING_TAG  0x84U
  USIM_TLV_T(1) ExtendedAccessBarring;
  /* Indicates whether the timer T3245 and the related
   * functionality is used by the UE        */
#define USIM_TIMER_T3245_BEHAVIOUR_TAG    0x85U
  USIM_TLV_T(1) Timer_T3245_Behaviour;
} usim_nasconfig_t;

/*
 * Ciphering and Integrity Keys
 * ----------------------------
 */
typedef struct {
  /* Key set identifier */
#define USIM_KSI_MIN    0b000
#define USIM_KSI_MAX    0b110
#define USIM_KSI_NOT_AVAILABLE  0b111
  Byte_t ksi;
  /* Cyphering key  */
#define USIM_CK_SIZE  16
  Byte_t ck[USIM_CK_SIZE];
  /* Integrity key  */
#define USIM_IK_SIZE  16
  Byte_t ik[USIM_IK_SIZE];
  uint8_t usim_api_k[USIM_API_K_SIZE];
  uint8_t opc[16];
} usim_keys_t;

/*
 * List of last used Sequence Numbers SQN
 */
#define USIM_API_AK_SIZE 6
#define USIM_API_SQN_SIZE USIM_API_AK_SIZE
#define USIM_API_SQNMS_SIZE USIM_API_SQN_SIZE

typedef struct {
  /* Highest sequence number the USIM has ever accepted */
  uint8_t sqn_ms[USIM_API_SQNMS_SIZE];
  /* List of the last used sequence numbers   */
#define USIM_API_SQN_LIST_SIZE  32
  uint8_t n_sqns;
  uint32_t sqn[USIM_API_SQN_LIST_SIZE];
} usim_sqn_data_t;


/*
 * EPS NAS Security Context
 * ------------------------
 * TODO: To be encoded as BER-TLV
 */
typedef struct {
  /* Length of all subsequent data        */
#define USIM_EPS_NAS_SECURITY_CONTEXT_TAG 0xA0U
  uint8_t length;
  /* Key set identifier KSI_ASME          */
#define USIM_KSI_ASME_TAG     0x80U
  USIM_TLV_T(1) KSIasme;
  /* ASME key (K_ASME)            */
#define USIM_K_ASME_TAG       0x81U
#define USIM_K_ASME_SIZE    32
  USIM_TLV_T(USIM_K_ASME_SIZE) Kasme;
  /* Uplink NAS count           */
#define USIM_UL_NAS_COUNT_TAG     0x82U
#define USIM_UL_NAS_COUNT_SIZE    4
  USIM_TLV_T(USIM_UL_NAS_COUNT_SIZE) ulNAScount;
  /* Downlink NAS count           */
#define USIM_DL_NAS_COUNT_TAG     0x83U
#define USIM_DL_NAS_COUNT_SIZE    4
  USIM_TLV_T(USIM_DL_NAS_COUNT_SIZE) dlNAScount;
  /* Identifiers of selected NAS integrity and encryption
   * algorithms             */
#define USIM_INT_ENC_ALGORITHMS_TAG   0x84U
#define USIM_INT_EIA0     0x00
#define USIM_INT_EIA1     0x01
#define USIM_INT_EIA2     0x02
#define USIM_INT_EIA3     0x03
#define USIM_INT_EIA4     0x04
#define USIM_INT_EIA5     0x05
#define USIM_INT_EIA6     0x06
#define USIM_INT_EIA7     0x07
#define USIM_ENC_EEA0     0x00
#define USIM_ENC_EEA1     0x10
#define USIM_ENC_EEA2     0x20
#define USIM_ENC_EEA3     0x30
#define USIM_ENC_EEA4     0x40
#define USIM_ENC_EEA5     0x50
#define USIM_ENC_EEA6     0x60
#define USIM_ENC_EEA7     0x70
  USIM_TLV_T(1) algorithmID;
} usim_securityctx_t;

/*
 * Subcriber's Number structure
 * ----------------------------
 */
typedef struct {
  Byte_t length;    /* Length of BCD number/SSC contents  */
  msisdn_t number;    /* Mobile subscriber dialing number */
  Byte_t conf1_record_id; /* Configuration1 Record Identifier */
  Byte_t ext1_record_id;  /* Extension1 Record Identifier   */
} usim_msisdn_t;

/*
 * PLMN Network Name structure
 * ---------------------------
 */
typedef struct {
  /* PLMN Full Name */
#define USIM_PNN_FULLNAME_TAG     0x43U
#define USIM_PNN_FULLNAME_SIZE  NET_FORMAT_LONG_SIZE
  USIM_TLV_T(USIM_PNN_FULLNAME_SIZE) fullname;
  /* PLMN Short Name  */
#define USIM_PNN_SHORTNAME_TAG      0x45U
#define USIM_PNN_SHORTNAME_SIZE NET_FORMAT_SHORT_SIZE
  USIM_TLV_T(USIM_PNN_SHORTNAME_SIZE) shortname;
} usim_pnn_t;

/*
 * Operator PLMN List structure
 * ----------------------------
 */
typedef struct {
  /* Tracking Area Identity range   */
  plmn_t plmn;
  tac_t start;
  tac_t end;
  /* PLMN Network Name Record Identifier  */
  uint8_t record_id;
} usim_opl_t;

/*
 * PLMN with Access Technology structure
 * -------------------------------------
 */
typedef struct {
  plmn_t plmn;
#define USIM_ACT_GSM    0x8000
#define USIM_ACT_COMPACT  0x4000
#define USIM_ACT_CDMA_HRPD  0x2000
#define USIM_ACT_CDMA_1xRTT 0x1000
#define USIM_ACT_UTRAN    0x0080
#define USIM_ACT_EUTRAN   0x0040
  uint16_t AcT;
} usim_plmn_act_t;

/*
 * Location information structure
 * ------------------------------
 */
typedef struct {
  uint32_t tmsi;  /* Temporary Mobile Subscriber Identity   */
  lai_t    lai; /* Location Area Identity     */
  Byte_t   reserved;  /* Reserved for future use      */
#define USIM_LOCI_UPDATED     0x00
#define USIM_LOCI_NOT_UPDATED     0x01
#define USIM_LOCI_PLMN_NOT_ALLOWED    0x02
#define USIM_LOCI_LA_NOT_ALLOWED    0x03
  Byte_t   status;  /* Location update status     */
} usim_loci_t;

/*
 * Packet Switched location information structure
 * ----------------------------------------------
 */
typedef struct {
  uint32_t p_tmsi;  /* Packet Temporary Mobile Subscriber Identity  */
  Byte_t   signature[3];  /* P-TMSI signature value   */
  RAI_t    rai; /* Routing Area Identity      */
#define USIM_PSLOCI_UPDATED     0x00
#define USIM_PSLOCI_NOT_UPDATED     0x01
#define USIM_PSLOCI_PLMN_NOT_ALLOWED    0x02
#define USIM_PSLOCI_LA_NOT_ALLOWED    0x03
  Byte_t   status;  /* Routing Area update status     */
} usim_psloci_t;

/*
 * EPS location information structure
 * ----------------------------------
 */
typedef struct {
  GUTI_t guti;  /* Globally Unique Temporary UE Identity  */
  tai_t  tai;   /* Last visited registered Tracking Area Id */
#define USIM_EPSLOCI_UPDATED      0x00
#define USIM_EPSLOCI_NOT_UPDATED    0x01
#define USIM_EPSLOCI_ROAMING_NOT_ALLOWED  0x02
  Byte_t status;  /* EPS update status        */
} usim_epsloci_t;

/*
 * Administrative Data
 * -------------------
 * Defines information concerning the mode of operation according to the
 * type of USIM, such as normal (to be used by PLMN subscribers for 3G
 * operations), type approval (to allow specific use of the ME during
 * type approval procedures of e.g. the radio equipment), cell testing
 * (to allow testing of a cell before commercial use of this cell),
 * manufacturer specific (to allow the ME manufacturer to perform specific
 * proprietary auto-test in its ME during e.g. maintenance phases).
 * It also provides an indication about how some ME features shall work
 * during normal operation as well as information about the length of the
 * MNC, which is part of the International Mobile Subscriber Identity (IMSI).
 */
typedef struct {
#define USIM_NORMAL_MODE      0x00U
#define USIM_TYPE_APPROVAL_MODE     0x80U
#define USIM_NORMAL_SPECIFIC_MODE   0x01U
#define USIM_TYPE_APPROVAL_SPECIFIC_MODE  0x81U
#define USIM_MAINTENANCE_MODE     0x02U
#define USIM_CELL_TEST_MODE     0x04U
  Byte_t UE_Operation_Mode; /* Mode of operation for the UE   */
  uint16_t Additional_Info; /* Additional information depending on
         * the UE operation mode    */
  Byte_t MNC_Length;    /* Number of digits used for extracting
         * the MNC from the IMSI    */
} usim_ad_t;

/*
 *---------------------------------------------------------------------------
 *    Structure of the data handled by the USIM application
 *---------------------------------------------------------------------------
 */

typedef struct {
  /*
   * Mandatory USIM configuration parameters
   * ---------------------------------------
   */
  imsi_t imsi;  /* International Mobile Subscriber Identity */
  usim_keys_t keys;   /* Ciphering and Integrity Keys   */
  Byte_t hpplmn;    /* Higher Priority PLMN search period */
#define USIM_FPLMN_MAX 4
  plmn_t fplmn[USIM_FPLMN_MAX]; /* List of Forbidden PLMNs  */
  usim_loci_t loci;     /* Location Information   */
  usim_psloci_t psloci;   /* PS Location Information  */
  usim_ad_t ad;     /* Administrative Data    */
  /*
   * Optional USIM configuration parameters
   * --------------------------------------
   */
  /* EPS NAS security context         */
  usim_securityctx_t securityctx;
  /* Subcriber's Number           */
  usim_msisdn_t msisdn;
  /* PLMN Network Name            */
#define USIM_PNN_MAX  8
  usim_pnn_t pnn[USIM_PNN_MAX];
  /* Operator PLMN List           */
#define USIM_OPL_MAX  USIM_PNN_MAX
  usim_opl_t opl[USIM_OPL_MAX];
  /* List of Equivalent HPLMNs          */
#define USIM_EHPLMN_MAX 2
  plmn_t ehplmn[USIM_EHPLMN_MAX];
  /* Home PLMN Selector with Access Technology      */
  usim_plmn_act_t hplmn;
  /* User controlled PLMN Selector with Access Technology   */
#define USIM_PLMN_MAX 8
  usim_plmn_act_t plmn[USIM_PLMN_MAX];
  /* Operator controlled PLMN Selector with Access Technology */
#define USIM_OPLMN_MAX  8
  usim_plmn_act_t oplmn[USIM_OPLMN_MAX];
  /* EPS Location Information         */
  usim_epsloci_t epsloci;
  /* Non-Access Stratum configuration       */
  usim_nasconfig_t nasconfig;
  /* usim test mode */
  uint8_t usimtestmode;
  usim_sqn_data_t usim_sqn_data;
} usim_data_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int usim_api_read(const char *filename, usim_data_t* data);

int usim_api_write(const char *filename, const usim_data_t* data);

int usim_api_authenticate(usim_data_t *usim_data, const OctetString* rand_pP, const OctetString* autn_pP,
                          OctetString* auts, OctetString* res,
                          OctetString* ck, OctetString* ik);
int usim_api_authenticate_test(usim_data_t *usim_data,
                               const OctetString* rand, const OctetString* autn,
                               OctetString* auts, OctetString* res,
                               OctetString* ck, OctetString* ik);

#endif /* __USIM_API_H__*/
