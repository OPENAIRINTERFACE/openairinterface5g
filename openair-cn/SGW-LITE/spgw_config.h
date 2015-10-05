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
/*! \file spgw_config.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef SPGW_CONFIG_H_
#define SPGW_CONFIG_H_
#include <sys/socket.h> // inet_aton
#include <netinet/in.h> // inet_aton
#include <arpa/inet.h>  // inet_aton

#include "queue.h"

#define SGW_CONFIG_STRING_SGW_CONFIG                            "S-GW"
#define SGW_CONFIG_STRING_NETWORK_INTERFACES_CONFIG             "NETWORK_INTERFACES"
#define SGW_CONFIG_STRING_SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP  "SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP"
#define SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP    "SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP"
#define SGW_CONFIG_STRING_SGW_PORT_FOR_S1U_S12_S4_UP            "SGW_IPV4_PORT_FOR_S1U_S12_S4_UP"
#define SGW_CONFIG_STRING_SGW_INTERFACE_NAME_FOR_S5_S8_UP       "SGW_INTERFACE_NAME_FOR_S5_S8_UP"
#define SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S5_S8_UP         "SGW_IPV4_ADDRESS_FOR_S5_S8_UP"
#define SGW_CONFIG_STRING_SGW_INTERFACE_NAME_FOR_S11            "SGW_INTERFACE_NAME_FOR_S11"
#define SGW_CONFIG_STRING_SGW_IPV4_ADDRESS_FOR_S11              "SGW_IPV4_ADDRESS_FOR_S11"

#define PGW_CONFIG_STRING_PGW_CONFIG                            "P-GW"
#define PGW_CONFIG_STRING_NETWORK_INTERFACES_CONFIG             "NETWORK_INTERFACES"
#define PGW_CONFIG_STRING_PGW_INTERFACE_NAME_FOR_S5_S8          "PGW_INTERFACE_NAME_FOR_S5_S8"
#define PGW_CONFIG_STRING_PGW_IPV4_ADDRESS_FOR_S5_S8            "PGW_IPV4_ADDRESS_FOR_S5_S8"
#define PGW_CONFIG_STRING_PGW_INTERFACE_NAME_FOR_SGI            "PGW_INTERFACE_NAME_FOR_SGI"
#define PGW_CONFIG_STRING_PGW_IPV4_ADDR_FOR_SGI                 "PGW_IPV4_ADDRESS_FOR_SGI"
#define PGW_CONFIG_STRING_PGW_MASQUERADE_SGI                    "PGW_MASQUERADE_SGI"

#define PGW_CONFIG_STRING_IP_ADDRESS_POOL                       "IP_ADDRESS_POOL"
#define PGW_CONFIG_STRING_IPV4_ADDRESS_LIST                     "IPV4_LIST"
#define PGW_CONFIG_STRING_IPV6_ADDRESS_LIST                     "IPV6_LIST"
#define PGW_CONFIG_STRING_IPV4_PREFIX_DELIMITER                 " /"
#define PGW_CONFIG_STRING_IPV6_PREFIX_DELIMITER                 " /"
#define PGW_CONFIG_STRING_DEFAULT_DNS_IPV4_ADDRESS              "DEFAULT_DNS_IPV4_ADDRESS"
#define PGW_CONFIG_STRING_DEFAULT_DNS_SEC_IPV4_ADDRESS          "DEFAULT_DNS_SEC_IPV4_ADDRESS"

#define PGW_CONFIG_STRING_INTERFACE_DISABLED                    "none"

#define IPV4_STR_ADDR_TO_INT_NWBO(AdDr_StR,NwBo,MeSsAgE ) do {\
            struct in_addr inp;\
            if ( inet_aton(AdDr_StR, &inp ) < 0 ) {\
                AssertFatal (0, MeSsAgE);\
            } else {\
                NwBo = inp.s_addr;\
            }\
        } while (0);

typedef struct sgw_config_s {
  struct {
    char     *sgw_interface_name_for_S1u_S12_S4_up;
    uint32_t  sgw_ipv4_address_for_S1u_S12_S4_up;
    int       sgw_ip_netmask_for_S1u_S12_S4_up;

    char     *sgw_interface_name_for_S5_S8_up;
    uint32_t  sgw_ipv4_address_for_S5_S8_up;
    int       sgw_ip_netmask_for_S5_S8_up;

    char     *sgw_interface_name_for_S11;
    uint32_t  sgw_ipv4_address_for_S11;
    int       sgw_ip_netmask_for_S11;
  } ipv4;
  int sgw_udp_port_for_S1u_S12_S4_up;

  uint8_t       sgw_drop_uplink_traffic;
  uint8_t       sgw_drop_downlink_traffic;
  uint8_t       local_to_eNB;
} sgw_config_t;

// may be more
#define PGW_MAX_ALLOCATED_PDN_ADDRESSES 1024


typedef struct pgw_lite_conf_ipv4_list_elm_s {
  STAILQ_ENTRY(pgw_lite_conf_ipv4_list_elm_s) ipv4_entries;
  struct in_addr  addr;
} pgw_lite_conf_ipv4_list_elm_t;


typedef struct pgw_lite_conf_ipv6_list_elm_s {
  STAILQ_ENTRY(pgw_lite_conf_ipv6_list_elm_s) ipv6_entries;
  struct in6_addr addr;
  int             prefix_len;
} pgw_lite_conf_ipv6_list_elm_t;


typedef struct pgw_config_s {
  struct {
    char     *pgw_interface_name_for_S5_S8;
    uint32_t  pgw_ipv4_address_for_S5_S8;
    int       pgw_ip_netmask_for_S5_S8;

    char     *pgw_interface_name_for_SGI;
    uint32_t  pgw_ipv4_address_for_SGI;
    int       pgw_ip_netmask_for_SGI;

    uint32_t  default_dns_v4;    // NBO
    uint32_t  default_dns_sec_v4;// NBO
  } ipv4;
  uint8_t   pgw_masquerade_SGI;

  STAILQ_HEAD(pgw_lite_ipv4_pool_head_s,      pgw_lite_conf_ipv4_list_elm_s) pgw_lite_ipv4_pool_list;
  STAILQ_HEAD(pgw_lite_ipv6_pool_head_s,      pgw_lite_conf_ipv6_list_elm_s) pgw_lite_ipv6_pool_list;
} pgw_config_t;

typedef struct spgw_config_s {
  sgw_config_t sgw_config;
  pgw_config_t pgw_config;
} spgw_config_t;

#ifndef SGW_LITE
extern spgw_config_t spgw_config;
#endif

typedef enum { SPGW_WARN_ON_ERROR = 0, SPGW_ABORT_ON_ERROR} spgw_system_abort_control_e;

int spgw_system(char *command_pP, spgw_system_abort_control_e abort_on_errorP, const char * const file_nameP, const int line_numberP);
int spgw_config_process(spgw_config_t* config_pP);
int spgw_config_init(char* lib_config_file_name_pP, spgw_config_t* config_pP);

#endif /* ENB_CONFIG_H_ */
