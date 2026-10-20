/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef NR_UE_TUN_H
#define NR_UE_TUN_H

#include <pthread.h>
#include <stdint.h>
#include <net/if.h>

typedef struct nas_ue_pdu_tun_s {
  int sock; /* primary TUN fd (-1 = none) */
  char ifname[IFNAMSIZ];
  int qfi; /* -1 = unset */
  /** TUN reader (idle or connected)
   * lifecycle managed via RRC */
  pthread_t reader_thread;
} nas_ue_pdu_tun_t;

void nr_ue_tun_create_ip_if(nas_ue_pdu_tun_t *t, const char *ipv4, const char *ipv6, int ue_id, int pdu_session_id);
void nr_ue_tun_create_eth_if(nas_ue_pdu_tun_t *t);
void nr_ue_tun_store_qfi(nas_ue_pdu_tun_t *t, uint8_t qfi);

#endif /* NR_UE_TUN_H */
