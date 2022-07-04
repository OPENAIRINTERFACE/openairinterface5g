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

Source      emm_asDef.h

Version     0.1

Date        2012/10/16

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EMM primitives available at the EMMAS Service
        Access Point to transfer NAS messages to/from the Access
        Stratum sublayer.

*****************************************************************************/
#ifndef __EMM_ASDEF_H__
#define __EMM_ASDEF_H__

#include "commonDef.h"
#include "OctetString.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * EMMAS-SAP primitives
 */
typedef enum emm_as_primitive_u {
  _EMMAS_START = 200,
  _EMMAS_SECURITY_REQ,  /* EMM->AS: Security request          */
  _EMMAS_SECURITY_IND,  /* AS->EMM: Security indication       */
  _EMMAS_SECURITY_RES,  /* EMM->AS: Security response         */
  _EMMAS_SECURITY_REJ,  /* EMM->AS: Security reject           */
  _EMMAS_ESTABLISH_REQ, /* EMM->AS: Connection establish request  */
  _EMMAS_ESTABLISH_CNF, /* AS->EMM: Connection establish confirm  */
  _EMMAS_ESTABLISH_REJ, /* AS->EMM: Connection establish reject   */
  _EMMAS_RELEASE_REQ,   /* EMM->AS: Connection release request    */
  _EMMAS_RELEASE_IND,   /* AS->EMM: Connection release indication */
  _EMMAS_DATA_REQ,      /* EMM->AS: Data transfer request     */
  _EMMAS_DATA_IND,      /* AS->EMM: Data transfer indication      */
  _EMMAS_PAGE_IND,      /* AS->EMM: Paging data indication        */
  _EMMAS_STATUS_IND,    /* AS->EMM: Status indication         */
  _EMMAS_CELL_INFO_REQ, /* EMM->AS: Cell information request      */
  _EMMAS_CELL_INFO_RES, /* AS->EMM: Cell information response     */
  _EMMAS_CELL_INFO_IND, /* AS->EMM: Cell information indication   */
  _EMMAS_END
} emm_as_primitive_t;

/* Data used to setup EPS NAS security */
typedef struct emm_as_security_data_s {
  uint8_t is_new;     /* New security data indicator      */
#define EMM_AS_NO_KEY_AVAILABLE     0xff
  uint8_t ksi;        /* NAS key set identifier       */
  uint8_t sqn;        /* Sequence number          */
  uint32_t count;     /* NAS counter              */
  const OctetString *k_enc;   /* NAS cyphering key            */
  const OctetString *k_int;   /* NAS integrity key            */
} emm_as_security_data_t;

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * EMMAS primitive for security
 * ----------------------------
 */
typedef struct emm_as_security_s {
  uint32_t ueid;      /* UE lower layer identifier        */
  const GUTI_t *guti;     /* GUTI temporary mobile identity   */
  emm_as_security_data_t sctx;/* EPS NAS security context     */
  int emm_cause;      /* EMM failure cause code       */
  /*
   * Identity request/response
   */
  uint8_t identType;      /* Type of requested UE's identity  */
  const imsi_t *imsi;     /* The requested IMSI of the UE     */
  const imei_t *imei;     /* The requested IMEI of the UE     */
  uint32_t tmsi;      /* The requested TMSI of the UE     */
  /*
   * Authentication request/response
   */
  uint8_t ksi;        /* NAS key set identifier       */
  const OctetString *rand;    /* Random challenge number      */
  const OctetString *autn;    /* Authentication token         */
  const OctetString *res; /* Authentication response      */
  const OctetString *auts;    /* Synchronisation failure      */
  /*
   * Security Mode Command
   */
  uint8_t eea;        /* Replayed EPS encryption algorithms   */
  uint8_t eia;        /* Replayed EPS integrity algorithms    */
  uint8_t uea;        /* Replayed UMTS encryption algorithms  */
  uint8_t ucs2;
  uint8_t uia;        /* Replayed UMTS integrity algorithms   */
  uint8_t gea;        /* Replayed GPRS encryption algorithms   */
  uint8_t imeisv_request;
  uint8_t umts_present;
  uint8_t gprs_present;

  // Added by LG
  uint8_t selected_eea; /* Selected EPS encryption algorithms   */
  uint8_t selected_eia; /* Selected EPS integrity algorithms    */

#define EMM_AS_MSG_TYPE_IDENT   0x01    /* Identification message   */
#define EMM_AS_MSG_TYPE_AUTH    0x02    /* Authentication message   */
#define EMM_AS_MSG_TYPE_SMC 0x03    /* Security Mode Command    */
  uint8_t msgType;    /* Type of NAS security message to transfer */
} emm_as_security_t;

/*
 * EMMAS primitive for connection establishment
 * --------------------------------------------
 */
typedef struct emm_as_EPS_identity_s {
  const GUTI_t *guti; /* The GUTI, if valid               */
  const tai_t  *tai;  /* The last visited registered Tracking
             * Area Identity, if available          */
  const imsi_t *imsi; /* IMSI in case of "AttachWithImsi"     */
  const imei_t *imei; /* UE's IMEI for emergency bearer services  */
} emm_as_EPS_identity_t;

typedef struct emm_as_establish_s {
  uint32_t ueid;      /* UE lower layer identifier         */
  emm_as_EPS_identity_t UEid; /* UE's EPS mobile identity      */
  emm_as_security_data_t sctx;/* EPS NAS security context      */
  bool switch_off;     /* true if the UE is switched off    */
  uint8_t type;       /* Network attach/detach type        */
  uint8_t RRCcause;       /* Connection establishment cause    */
  uint8_t RRCtype;        /* Associated call type          */
  const plmn_t *plmnID;   /* Identifier of the selected PLMN   */
  uint8_t ksi;        /* NAS key set identifier        */
  uint8_t encryption:4;   /* Ciphering algorithm           */
  uint8_t integrity:4;    /* Integrity protection algorithm    */
  int emm_cause;      /* EMM failure cause code        */
  const GUTI_t *new_guti; /* New GUTI, if re-allocated         */
  int n_tacs;         /* Number of concecutive tracking areas
                 * the UE is registered to       */
  tac_t tac;          /* Code of the first tracking area the UE
                 * is registered to          */
#define EMM_AS_NAS_INFO_ATTACH  0x01    /* Attach request        */
#define EMM_AS_NAS_INFO_DETACH  0x02    /* Detach request        */
#define EMM_AS_NAS_INFO_TAU 0x03    /* Tracking Area Update request  */
#define EMM_AS_NAS_INFO_SR  0x04    /* Service Request       */
#define EMM_AS_NAS_INFO_EXTSR   0x05    /* Extended Service Request  */
  uint8_t NASinfo;    /* Type of initial NAS information to transfer   */
  OctetString NASmsg;     /* NAS message to be transfered within
                 * initial NAS information message   */
} emm_as_establish_t;

/*
 * EMMAS primitive for connection release
 * --------------------------------------
 */
typedef struct emm_as_release_s {
  uint32_t ueid;      /* UE lower layer identifier          */
  const GUTI_t *guti;     /* GUTI temporary mobile identity     */
#define EMM_AS_CAUSE_AUTHENTICATION 0x01    /* Authentication failure */
#define EMM_AS_CAUSE_DETACH     0x02    /* Detach requested   */
  uint8_t cause;  /* Release cause */
} emm_as_release_t;

/*
 * EMMAS primitive for data transfer
 * ---------------------------------
 */
typedef struct emm_as_data_s {
  uint32_t ueid;      /* UE lower layer identifier        */
  const GUTI_t *guti;     /* GUTI temporary mobile identity   */
  emm_as_security_data_t sctx;/* EPS NAS security context     */
  bool switch_off;     /* true if the UE is switched off   */
  uint8_t type;       /* Network detach type          */
  uint8_t delivered;      /* Data message delivery indicator  */
#define EMM_AS_NAS_DATA_ATTACH  0x01    /* Attach complete      */
#define EMM_AS_NAS_DATA_DETACH  0x02    /* Detach request       */
  uint8_t NASinfo;        /* Type of NAS information to transfer  */
  OctetString NASmsg;     /* NAS message to be transfered     */
} emm_as_data_t;

/*
 * EMMAS primitive for paging
 * --------------------------
 */
typedef struct emm_as_page_s {} emm_as_page_t;

/*
 * EMMAS primitive for status indication
 * -------------------------------------
 */
typedef struct emm_as_status_s {
  uint32_t ueid;      /* UE lower layer identifier        */
  const GUTI_t *guti;     /* GUTI temporary mobile identity   */
  emm_as_security_data_t sctx;/* EPS NAS security context     */
  int emm_cause;      /* EMM failure cause code       */
} emm_as_status_t;

/*
 * EMMAS primitive for cell information
 * ------------------------------------
 */
typedef struct emm_as_cell_info_s {
  uint8_t found;  /* Indicates whether a suitable cell is found   */
#define EMM_AS_PLMN_LIST_SIZE   6
  PLMN_LIST_T(EMM_AS_PLMN_LIST_SIZE) plmnIDs;
  /* List of identifiers of available PLMNs   */
  Byte_t rat;     /* Bitmap of Radio Access Technologies      */
  tac_t tac;      /* Tracking Area Code               */
  ci_t cellID;    /* cell identity                */
} emm_as_cell_info_t;

/*
 * --------------------------------
 * Structure of EMMAS-SAP primitive
 * --------------------------------
 */
typedef struct emm_as_s {
  emm_as_primitive_t primitive;
  union {
    emm_as_security_t security;
    emm_as_establish_t establish;
    emm_as_release_t release;
    emm_as_data_t data;
    emm_as_page_t page;
    emm_as_status_t status;
    emm_as_cell_info_t cell_info;
  } u;
} emm_as_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * Defined in LowerLayer.c
 * Setup security data according to the given EPS security context
 */
void emm_as_set_security_data(emm_as_security_data_t *data, const void *context,
                              int is_new, int is_ciphered);

#endif /* __EMM_ASDEF_H__*/
