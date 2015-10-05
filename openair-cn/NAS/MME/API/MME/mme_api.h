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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************
Source      mme_api.h

Version     0.1

Date        2013/02/28

Product     NAS stack

Subsystem   Application Programming Interface

Author      Frederic Maurel

Description Implements the API used by the NAS layer running in the MME
        to interact with a Mobility Management Entity

*****************************************************************************/
#ifndef __MME_API_H__
#define __MME_API_H__

# if defined(NAS_BUILT_IN_EPC)
#   include "mme_config.h"
# endif
#include "commonDef.h"
#include "securityDef.h"
#include "OctetString.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Maximum number of UEs the MME may simultaneously support */
#define MME_API_NB_UE_MAX       256


/* Features supported by the MME */
typedef enum mme_api_feature_s {
  MME_API_NO_FEATURE_SUPPORTED    = 0,
  MME_API_EMERGENCY_ATTACH        = (1<<0),
  MME_API_UNAUTHENTICATED_IMSI    = (1<<1),
  MME_API_IPV4                    = (1<<2),
  MME_API_IPV6                    = (1<<3),
  MME_API_SINGLE_ADDR_BEARERS     = (1<<4),
} mme_api_feature_t;

/* Network IP version capability */
typedef enum mme_api_ip_version_e {
  MME_API_IPV4_ADDR,
  MME_API_IPV6_ADDR,
  MME_API_IPV4V6_ADDR,
  MME_API_ADDR_MAX
} mme_api_ip_version_t;

/*
 * EPS Mobility Management configuration data
 * ------------------------------------------
 */
typedef struct mme_api_emm_config_s {
  mme_api_feature_t features; /* Supported features           */
  gummei_t          gummei;   /* EPS Globally Unique MME Identity */
  uint8_t           prefered_integrity_algorithm[8];// choice in NAS_SECURITY_ALGORITHMS_EIA0, etc
  uint8_t           prefered_ciphering_algorithm[8];// choice in NAS_SECURITY_ALGORITHMS_EEA0, etc
} mme_api_emm_config_t;

/*
 * EPS Session Management configuration data
 * -----------------------------------------
 */
typedef struct mme_api_esm_config_s {
  mme_api_feature_t features; /* Supported features           */
  uint8_t   dns_prim_ipv4[4]; /* Network byte order */
  uint8_t   dns_sec_ipv4[4];  /* Network byte order */
} mme_api_esm_config_t;

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/* EPS subscribed QoS profile  */
typedef struct mme_api_qos_s {
#define MME_API_UPLINK      0
#define MME_API_DOWNLINK    1
#define MME_API_DIRECTION   2
  int gbr[MME_API_DIRECTION]; /* Guaranteed Bit Rate          */
  int mbr[MME_API_DIRECTION]; /* Maximum Bit Rate         */
  int qci;            /* QoS Class Identifier         */
} mme_api_qos_t;

/* Traffic Flow Template */
typedef struct mme_api_tft_s {
} mme_api_tft_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#if defined(NAS_BUILT_IN_EPC)
int mme_api_get_emm_config(mme_api_emm_config_t *config,
                           mme_config_t *mme_config_p);
#else
int mme_api_get_emm_config(mme_api_emm_config_t *config);
#endif
int mme_api_get_esm_config(mme_api_esm_config_t *config);

int mme_api_identify_guti(const GUTI_t *guti, auth_vector_t *vector);
int mme_api_identify_imsi(const imsi_t *imsi, auth_vector_t *vector);
int mme_api_identify_imei(const imei_t *imei, auth_vector_t *vector);
int mme_api_new_guti(const imsi_t *imsi, GUTI_t *guti, tac_t *tac, int *n_tacs);

int mme_api_subscribe(OctetString *apn, mme_api_ip_version_t mme_pdn_index, OctetString *pdn_addr,
                      int is_emergency, mme_api_qos_t *qos);
int mme_api_unsubscribe(OctetString *apn);

#endif /* __MME_API_H__*/
