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
                                generate_scenario.h
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
*/

#ifndef GENERATE_SCENARIO_H_
#define GENERATE_SCENARIO_H_
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "commonDef.h"
#include "platform_types.h"
#include "platform_constants.h"
#include "PHY/impl_defs_lte.h"
#include "s1ap_messages_types.h"
#ifdef CMAKER
#include "SystemInformationBlockType2.h"
#else
#include "RRC/LITE/MESSAGES/SystemInformationBlockType2.h"
#endif

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
#define EPC_TEST_SCENARIO_MAX_ENB                       4


typedef struct mme_ip_address_s {
  unsigned  ipv4:1;
  unsigned  ipv6:1;
  unsigned  active:1;
  char     *ipv4_address;
  char     *ipv6_address;
} mme_ip_address_t;

typedef struct Enb_properties_s {
  /* Unique eNB_id to identify the eNB within EPC.
   * For macro eNB ids this field should be 20 bits long.
   * For home eNB ids this field should be 28 bits long.
   */
  uint32_t            eNB_id;

  /* The type of the cell */
  enum cell_type_e    cell_type;

  /* Optional name for the cell
   * NOTE: the name can be NULL (i.e no name) and will be cropped to 150
   * characters.
   */
  char               *eNB_name;

  /* Tracking area code */
  uint16_t            tac;

  /* Mobile Country Code
   * Mobile Network Code
   */
  uint16_t            mcc;
  uint16_t            mnc;
  uint8_t             mnc_digit_length;

  /* Nb of MME to connect to */
  uint8_t             nb_mme;
  /* List of MME to connect to */
  mme_ip_address_t    mme_ip_address[S1AP_MAX_NB_MME_IP_ADDRESS];

  int                 sctp_in_streams;
  int                 sctp_out_streams;

  char               *enb_interface_name_for_S1U;
  in_addr_t           enb_ipv4_address_for_S1U;
  tcp_udp_port_t      enb_port_for_S1U;

  char               *enb_interface_name_for_S1_MME;
  in_addr_t           enb_ipv4_address_for_S1_MME;

} Enb_properties_t;

typedef struct Enb_properties_array_s {
  int                  number;
  Enb_properties_t    *properties[EPC_TEST_SCENARIO_MAX_ENB];
} Enb_properties_array_t;

void enb_config_init(const char const * lib_config_file_name_pP);

#endif /* ENB_CONFIG_H_ */
/** @} */
