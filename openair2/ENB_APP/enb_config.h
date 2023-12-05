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
                                enb_config.h
                             -------------------
  AUTHOR  : Lionel GAUTHIER, Navid Nikaein, Laurent Winckel
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr, navid.nikaein@eurecom.fr
*/

#ifndef ENB_CONFIG_H_
#define ENB_CONFIG_H_
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libconfig.h>

#include "commonDef.h"
#include "common/platform_types.h"
#include "common/platform_constants.h"
#include "PHY/defs_eNB.h"
#include "s1ap_messages_types.h"
#include "f1ap_messages_types.h"
#include "LTE_SystemInformationBlockType2.h"
#include "rrc_messages_types.h"
#include "RRC/LTE/rrc_defs.h"
#include <intertask_interface.h>
#include "enb_paramdef.h"

#define IPV4_STR_ADDR_TO_INT_NWBO(AdDr_StR,NwBo,MeSsAgE ) do {\
            struct in_addr inp;\
            if ( inet_aton(AdDr_StR, &inp ) < 0 ) {\
                AssertFatal (0, MeSsAgE);\
            } else {\
                NwBo = inp.s_addr;\
            }\
        } while (0);

/** @defgroup _enb_app ENB APP 
 * @ingroup _oai2
 * @{
 */

// Hard to find a defined value for max enb...
#define MAX_ENB 16

#define MAX_DU	4
#define CU_BALANCING_ALL		127
#define CU_BALANCING_ROUND_ROBIN	126

typedef struct mme_ip_address_s {
  unsigned  ipv4:1;
  unsigned  ipv6:1;
  unsigned  active:1;
  char     *ipv4_address;
  char     *ipv6_address;
} mme_ip_address_t;

typedef struct cu_params {
  const char    *local_ipv4_address;
  const uint16_t local_port;
  const char    *remote_ipv4_address;
  const int16_t  remote_port;
} cudu_params_t;

typedef struct ru_config_s {
  // indicates if local or remote rf is used (1 == LOCAL)
  unsigned  local_rf:1;
  // indicates if UDP socket is used
  unsigned  udp:1;
  // indicates if RAW socket is used
  unsigned  raw:1;
  char      *ru_if_name;
  char     *local_address;
  char     *remote_address;
  uint16_t  local_port;
  uint16_t  remote_port;
  uint8_t   udpif4p5;
  uint8_t   rawif4p5;
  uint8_t   rawif5_mobipass;
  uint8_t   if_compress;
} ru_config_t;

extern void RCconfig_L1(void);
extern void RCconfig_macrlc(void);
extern void UE_config_stub_pnf(void);
extern int  RCconfig_gtpu(void );
extern void RCConfig(void);

void                          enb_config_display(void);
void                          ru_config_display(void);

int RCconfig_RRC(uint32_t i, eNB_RRC_INST *rrc);
int RCconfig_S1(MessageDef *msg_p, uint32_t i);

void read_config_and_init(void);
int RCconfig_X2(MessageDef *msg_p, uint32_t i);
int RCconfig_M2(MessageDef *msg_p, uint32_t i);

#ifdef E2_AGENT
#include "openair2/E2AP/e2_agent_arg.h"
e2_agent_args_t RCconfig_E2agent(void);
#endif

void fill_SL_configuration(RrcConfigurationReq *RRCcfg, ccparams_sidelink_t *SLconfig, int cell_idx, int cc_idx, char *config_fname);
void fill_eMTC_configuration(RrcConfigurationReq *RRCcfg, ccparams_eMTC_t *eMTCconfig, int cell_idx, int cc_idx, char *config_fname, char *brparamspath);

#endif /* ENB_CONFIG_H_ */
/** @} */
