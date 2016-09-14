/** \file rb_config.h
 *  \brief Openair radio bearer configuration header file
 *  \author Raymond Knopp and Navid Nikaein
 */

#ifndef __RB_CONFIG_H__
#define __RB_CONFIG_H__

#include <netinet/in.h>
/*
typedef struct {

  int fd; // socket file descriptor

  int stats;

  int action;  // add or delete

  int rb;
  int cx;
  int inst;

  int saddr_ipv4set;
  int daddr_ipv4set;
  in_addr_t saddr_ipv4;
  in_addr_t daddr_ipv4;

  int saddr_ipv6set;
  int daddr_ipv6set;
  struct in6_addr saddr_ipv6;
  struct in6_addr daddr_ipv6;

  int dscp;



} rb_config;
*/
int rb_validate_config_ipv4(int cx, int inst, int rb);
int rb_conf_ipv4(int action,int cx, int inst, int rb, int dscp, in_addr_t saddr_ipv4, in_addr_t daddr_ipv4);
void rb_ioctl_init(int inst);
int rb_stats_req(int inst);
void init_socket(void);
in_addr_t ipv4_address (int thirdOctet, int fourthOctet);



#endif
