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
                                gnb_config.h
                             -------------------
  AUTHOR  : Lionel GAUTHIER, Navid Nikaein, Laurent Winckel, WEI-TAI CHEN
  COMPANY : EURECOM, NTSUT
  EMAIL   : Lionel.Gauthier@eurecom.fr, navid.nikaein@eurecom.fr, kroempa@gmail.com
*/

#ifndef GNB_CONFIG_H_
#define GNB_CONFIG_H_
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libconfig.h>

#include "commonDef.h"
#include "platform_types.h"
#include "platform_constants.h"
#include "PHY/defs_eNB.h"
#include "s1ap_messages_types.h"
#include "ngap_messages_types.h"
#include "f1ap_messages_types.h"

#include "rrc_messages_types.h"
#include "intertask_interface.h"
#include "RRC/NR/nr_rrc_defs.h"

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
#define MAX_GNB 16

/*
typedef struct mme_ip_address_s {
  unsigned  ipv4:1;
  unsigned  ipv6:1;
  unsigned  active:1;
  char     *ipv4_address;
  char     *ipv6_address;
} mme_ip_address_t;

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
*/
extern void NRRCconfig_RU(void);
extern void RCconfig_nr_flexran(void);
extern void RCconfig_NR_L1(void);
extern void RCconfig_nr_macrlc(void);
extern int  RCconfig_nr_gtpu(void );
extern void NRRCConfig(void);

//void                          enb_config_display(void);
//void                          ru_config_display(void);

void RCconfig_NRRRC(MessageDef *msg_p, uint32_t i, gNB_RRC_INST *rrc);
int RCconfig_NR_NG(MessageDef *msg_p, uint32_t i);
int RCconfig_NR_X2(MessageDef *msg_p, uint32_t i);
int RCconfig_NR_DU_F1(MessageDef *msg_p, uint32_t i);
int gNB_app_handle_f1ap_setup_resp(f1ap_setup_resp_t *resp);
int gNB_app_handle_f1ap_gnb_cu_configuration_update(f1ap_gnb_cu_configuration_update_t *gnb_cu_cfg_update);
void nr_read_config_and_init(void);

#endif /* GNB_CONFIG_H_ */
/** @} */
