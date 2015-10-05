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
#ifndef MME_DEFAULT_VALUES_H_
#define MME_DEFAULT_VALUES_H_

/*******************************************************************************
 * Timer Constants
 ******************************************************************************/
#define MME_STATISTIC_TIMER_S  (60)

/*******************************************************************************
 * GTPV1 User Plane Constants
 ******************************************************************************/
#define GTPV1_U_PORT_NUMBER    (2152)

/*******************************************************************************
 * S1AP Constants
 ******************************************************************************/

#define S1AP_PORT_NUMBER (36412) ///< S1AP SCTP IANA ASSIGNED Port Number
#define S1AP_SCTP_PPID   (18)    ///< S1AP SCTP Payload Protocol Identifier (PPID)

#define S1AP_OUTCOME_TIMER_DEFAULT (5)     ///< S1AP Outcome drop timer (s)

/*******************************************************************************
 * S6A Constants
 ******************************************************************************/

#define S6A_CONF_FILE "../S6A/freediameter/s6a.conf"

/*******************************************************************************
 * SCTP Constants
 ******************************************************************************/

#define SCTP_RECV_BUFFER_SIZE (1 << 16)
#define SCTP_OUT_STREAMS      (32)
#define SCTP_IN_STREAMS       (32)
#define SCTP_MAX_ATTEMPTS     (5)

/*******************************************************************************
 * MME global definitions
 ******************************************************************************/

#define MAX_NUMBER_OF_ENB       (2)
#define MAX_NUMBER_OF_UE        (64)

#define MMEC                    (0)
#define MMEGID                  (0)
#define PLMN_MCC                (208)
#define PLMN_MNC                (34)
#define PLMN_MNC_LEN            (2)
#define PLMN_TAC                (0)

#define RELATIVE_CAPACITY       (15)

/*******************************************************************************
 * IPv4 Constants
 ******************************************************************************/

/* Default network card to use for IPV4 packets forwarding.
 * up stands for user-plane.
 * cp stands for control-plane
 *
 *        +-----------+          +------+              +-----------+
 *        |  eNB      +------+   |  ovs | VLAN 1+------+    MME    |
 *        |           |cpenb0+------------------+cpmme0|           |
 *        |           +------+   |bridge|       +------+           |
 *        |           |upenb0+-------+  |              |           |
 *        +-----------+------+   |   |  |              +-+------+--+
 *                               +---|--+                |s11mme|
 *                                   |                   +---+--+
 *                                   |                 VLAN3 | (optional)
 *                                   |                   +---+--+
 *                                   |                   |s11sgw|
 *                                   |                 +-+------+--+
 *                                   |                 |  S+P-GW   |
 *                                   |  VLAN2   +------+           +--------+
 *                                   +----------+upsgw0|           |pgwsgi0 +
 *                                              +------+           +--------+
 *                                                     |           |
 *                                                     +-----------+
 */

#define DEFAULT_SGW_INTERFACE_NAME_FOR_S11            ("s11sgw")
#define DEFAULT_SGW_IP_ADDRESS_FOR_S11                ("192.168.10.1")
#define DEFAULT_SGW_IP_NETMASK_FOR_S11                24

#define DEFAULT_SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP  ("upsgw0")
#define DEFAULT_SGW_IP_ADDRESS_FOR_S1U_S12_S4_UP      ("192.168.1.1")
#define DEFAULT_SGW_IP_NETMASK_FOR_S1U_S12_S4_UP      24

#define DEFAULT_SGW_INTERFACE_NAME_FOR_S5_S8_UP       ("upsgw1")
#define DEFAULT_SGW_IP_ADDRESS_FOR_S5_S8_UP           ("192.168.5.2")
#define DEFAULT_SGW_IP_NETMASK_FOR_S5_S8_UP           24

#define DEFAULT_PGW_INTERFACE_NAME_FOR_S5_S8          ("uppgw0")
#define DEFAULT_PGW_IP_ADDRESS_FOR_S5_S8              ("192.168.5.1")
#define DEFAULT_PGW_IP_NETMASK_FOR_S5_S8              24

#define DEFAULT_PGW_INTERFACE_NAME_FOR_SGI            ("eth0")
#define DEFAULT_PGW_IP_ADDR_FOR_SGI                   ("192.168.14.17")
#define DEFAULT_PGW_IP_NETMASK_FOR_SGI                24

#define DEFAULT_MME_INTERFACE_NAME_FOR_S11            ("s11mme")     ///< MME control plane interface
#define DEFAULT_MME_IP_ADDRESS_FOR_S11                ("192.168.10.2")   ///< MME control plane IP address
#define DEFAULT_MME_IP_NETMASK_FOR_S11                24;

#define DEFAULT_MME_INTERFACE_NAME_FOR_S1_MME         ("cpmme0")     ///< MME control plane interface
#define DEFAULT_MME_IP_ADDRESS_FOR_S1_MME             ("192.168.11.1")   ///< MME control plane IP address
#define DEFAULT_MME_IP_NETMASK_FOR_S1_MME             24;


#endif /* MME_DEFAULT_VALUES_H_ */
