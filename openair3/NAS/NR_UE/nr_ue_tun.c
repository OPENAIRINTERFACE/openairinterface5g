/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "nr_ue_tun.h"
#include "common/utils/assertions.h"
#include "common/utils/tuntap_if.h"

void nr_ue_tun_store_qfi(nas_ue_pdu_tun_t *t, uint8_t qfi)
{
  DevAssert(t != NULL);
  t->qfi = qfi;
}

void nr_ue_tun_create_ip_if(nas_ue_pdu_tun_t *t, const char *ipv4, const char *ipv6, int ue_id, int pdu_session_id)
{
  DevAssert(t != NULL);
  if (t->sock >= 0)
    return;

  t->sock = tuntap_alloc(IFF_TUN, t->ifname);
  tun_config(t->ifname, ipv4, ipv6);
  if (ipv4)
    setup_ue_ipv4_route(t->ifname, ue_id, pdu_session_id, ipv4);
}

void nr_ue_tun_create_eth_if(nas_ue_pdu_tun_t *t)
{
  DevAssert(t != NULL);
  if (t->sock >= 0)
    return;

  t->sock = tuntap_alloc(IFF_TAP, t->ifname);
  tap_config(t->ifname);
}
