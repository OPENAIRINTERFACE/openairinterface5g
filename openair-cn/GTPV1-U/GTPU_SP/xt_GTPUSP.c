/*
 * GTPu klm for Linux/iptables
 *
 * Copyright (c) 2010-2011 Polaris Networks
 * Author: Pradip Biswas <pradip_biswas@polarisnetworks.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/route.h>
#include <linux/time.h>
#include <net/checksum.h>
#include <net/ip.h>
#include <linux/in.h>
#include <linux/icmp.h>
#include <net/udp.h>
#include <net/inet_sock.h>
#include <net/route.h>
#include <net/addrconf.h>
#include <net/ip6_checksum.h>
#include <net/ip6_route.h>
#include <net/ipv6.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/delay.h>
#include <linux/inet.h>

// CONNMARK
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_ecache.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_connmark.h>
#if defined(CONFIG_IP6_NF_IPTABLES) || defined(CONFIG_IP6_NF_IPTABLES_MODULE)
#    define WITH_IPV6 1
#endif
#include "xt_GTPUSP.h"
#if !(defined KVERSION)
#    error "Kernel version is not defined!!!! Exiting."
#endif

#define GTPU_TRACE_IN_KERN_LOG 0

#if defined(GTPU_TRACE_IN_KERN_LOG)
#define PR_INFO(fORMAT, aRGS...) pr_info(fORMAT, ##aRGS)
#else
#define PR_INFO(fORMAT, aRGS...)
#endif
#define INT_MODULE_PARM(n, v) static int n = v; module_param(n, int, 0444)
#define STRING_MODULE_PARM(s, v) static char* s = v; module_param(s, charp, 0000);
//-----------------------------------------------------------------------------
static char*        _gtpusp_nf_inet_hook_2_string(int nf_inet_hookP);
static void         _gtpusp_print_hex_octets(unsigned char* data_pP, unsigned short sizeP);
static void         _gtpusp_tg4_add(struct sk_buff *old_skb_pP, const struct xt_action_param *par_pP);
#ifdef WITH_IPV6
static void         _gtpusp_tg6_add(struct sk_buff *old_skb_pP, const struct xt_action_param *par_pP);
static unsigned int _gtpusp_tg6(struct sk_buff *skb_pP, const struct xt_action_param *par_pP);
#endif
static unsigned int _gtpusp_tg4(struct sk_buff *skb_pP, const struct xt_action_param *par_pP);
static int          __init gtpusp_tg_init(void);
static void         __exit gtpusp_tg_exit(void);
static int          _udp_thread(void *data);
static int          _gtpusp_ksocket_send(struct socket *sock_pP, struct sockaddr_in *addr_pP, unsigned char *gtpuh_pP, int len_gtpP, unsigned char *buf_ip_pP, int len_ipP);
static int          _gtpusp_ksocket_receive(struct socket* sock_pP, struct sockaddr_in* addr_pP, unsigned char* buf_pP, int lenP);
static int          _gtpusp_ksocket_process_gtp(const unsigned char * const rx_buf_pP, const int lenP, unsigned char* tx_buf_pP);
//-----------------------------------------------------------------------------
#define MODULE_NAME "GTPUSP"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pradip Biswas <pradip_biswas@polarisnetworks.net>");
MODULE_DESCRIPTION("GTPu Data Path extension on netfilter");
//-----------------------------------------------------------------------------
static struct xt_target gtpusp_tg_reg[] __read_mostly = {
  {
    .name           = MODULE_NAME,
    .revision       = 0,
    .family         = NFPROTO_IPV4,
    .hooks          = (1 << NF_INET_POST_ROUTING) |
    (1 << NF_INET_LOCAL_IN) |
    (1 << NF_INET_FORWARD),
    .table          = "mangle",
    .target         = _gtpusp_tg4,
    .targetsize     = sizeof(struct xt_gtpusp_target_info),
    .me             = THIS_MODULE,
  },
#ifdef WITH_IPV6
  {
    .name           = MODULE_NAME,
    .revision       = 0,
    .family         = NFPROTO_IPV6,
    .hooks          = (1 << NF_INET_POST_ROUTING) |
    (1 << NF_INET_LOCAL_IN) |
    (1 << NF_INET_FORWARD),
    .table          = "mangle",
    .target         = _gtpusp_tg6,
    .targetsize     = sizeof(struct xt_gtpusp_target_info),
    .me             = THIS_MODULE,
  },
#endif
};
#define GTP_ECHO_REQ           1
#define GTP_ECHO_RSP           2
#define GTP_ERROR_INDICATION   26
#define GTP_GPDU               255

typedef struct gtpv1u_msg_s {
  unsigned char    version;
  unsigned char    protocol_type;
  unsigned char    ext_hdr_flag;
  unsigned char    seq_num_flag;
  u_int16_t        npdu_num_flag;
  u_int32_t        msg_type;
  u_int16_t        msg_len;
  u_int32_t        teid;
  u_int16_t        seq_num;
  unsigned char    npdu_num;
  unsigned char    next_ext_hdr_type;
  u_int32_t        msg_buf_len;
  u_int32_t        msg_buf_offset;
  struct gtpv1u_msg_s* next;
} gtpv1u_msg_t;

struct gtpuhdr {
  char      flags;
  char      msgtype;
  u_int16_t length;
  u_int32_t tunid;
};
typedef struct gtpusp_sock_s {
  struct task_struct *thread;
  struct sockaddr_in  addr;
  struct socket      *sock;
  struct sockaddr_in  addr_send;
  int running;
  int thread_stop_requested;
} gtpusp_sock_t;

//-----------------------------------------------------------------------------
#define GTPU_HDR_PNBIT 1
#define GTPU_HDR_SBIT 1 << 1
#define GTPU_HDR_EBIT 1 << 2
#define GTPU_ANY_EXT_HDR_BIT (GTPU_HDR_PNBIT | GTPU_HDR_SBIT | GTPU_HDR_EBIT)

#define GTPU_FAILURE 1
#define GTPU_SUCCESS !GTPU_FAILURE
#define GTPUSP_2_PRINT_BUFFER_LEN 8192

#define IP_MORE_FRAGMENTS 0x2000
#define NIPADDR(addr) \
        (uint8_t)(addr & 0x000000FF), \
        (uint8_t)((addr & 0x0000FF00) >> 8), \
        (uint8_t)((addr & 0x00FF0000) >> 16), \
        (uint8_t)((addr & 0xFF000000) >> 24)
//-----------------------------------------------------------------------------
static char _gtpusp_print_buffer[GTPUSP_2_PRINT_BUFFER_LEN];
gtpusp_sock_t _gtpusp_sock;

INT_MODULE_PARM(gtpu_sgw_port, 2152);
MODULE_PARM_DESC(gtpu_sgw_port, "UDP port number for S1U interface (s-GW side)");
INT_MODULE_PARM(gtpu_enb_port, 2153);
MODULE_PARM_DESC(gtpu_enb_port, "UDP port number for S1U interface (eNB side)");
STRING_MODULE_PARM(sgw_addr, "127.0.0.1");
MODULE_PARM_DESC(sgw_addr, "IPv4 address of the S1U IP interface");



//-----------------------------------------------------------------------------
static char*
_gtpusp_icmph_type_2_string(uint8_t typeP)
//-----------------------------------------------------------------------------
{
  switch (typeP) {
    case ICMP_ECHOREPLY:return "ECHOREPLY";break;
    case ICMP_DEST_UNREACH:return "DEST_UNREACH";break;
    case ICMP_SOURCE_QUENCH:return "SOURCE_QUENCH";break;
    case ICMP_REDIRECT:return "REDIRECT";break;
    case ICMP_ECHO:return "ECHO";break;
    case ICMP_TIME_EXCEEDED:return "TIME_EXCEEDED";break;
    case ICMP_PARAMETERPROB:return "PARAMETERPROB";break;
    case ICMP_TIMESTAMP:return "TIMESTAMP";break;
    case ICMP_TIMESTAMPREPLY:return "TIMESTAMPREPLY";break;
    case ICMP_INFO_REQUEST:return "INFO_REQUEST";break;
    case ICMP_INFO_REPLY:return "INFO_REPLY";break;
    case ICMP_ADDRESS:return "ADDRESS";break;
    case ICMP_ADDRESSREPLY:return "ADDRESSREPLY";break;
    default:return "TYPE?";
  }
}
//-----------------------------------------------------------------------------
static char*
_gtpusp_nf_inet_hook_2_string(int nf_inet_hookP)
{
  //-----------------------------------------------------------------------------
  switch (nf_inet_hookP) {
  case NF_INET_PRE_ROUTING:
    return "NF_INET_PRE_ROUTING";
    break;

  case NF_INET_LOCAL_IN:
    return "NF_INET_LOCAL_IN";
    break;

  case NF_INET_FORWARD:
    return "NF_INET_FORWARD";
    break;

  case NF_INET_LOCAL_OUT:
    return "NF_INET_LOCAL_OUT";
    break;

  case NF_INET_POST_ROUTING:
    return "NF_INET_POST_ROUTING";
    break;

  default:
    return "NF_INET_UNKNOWN";
  }
}
//-----------------------------------------------------------------------------
void
_gtpusp_print_hex_octets(unsigned char* data_pP, unsigned short sizeP)
{
  //-----------------------------------------------------------------------------

  unsigned long octet_index = 0;
  unsigned long buffer_marker = 0;
  unsigned char aindex;
  struct timeval tv;
  char timeofday[64];
  unsigned int h,m,s;

  if (data_pP == NULL) {
    return;
  }

  if (sizeP > 2000) {
    return;
  }

  do_gettimeofday(&tv);
  h = (tv.tv_sec/3600) % 24;
  m = (tv.tv_sec / 60) % 60;
  s = tv.tv_sec % 60;
  snprintf(timeofday, 64, "%02d:%02d:%02d.%06ld", h,m,s,tv.tv_usec);

  buffer_marker+=snprintf(&_gtpusp_print_buffer[buffer_marker], GTPUSP_2_PRINT_BUFFER_LEN - buffer_marker,"%s------+-------------------------------------------------+\n",timeofday);
  buffer_marker+=snprintf(&_gtpusp_print_buffer[buffer_marker], GTPUSP_2_PRINT_BUFFER_LEN - buffer_marker,"%s      |  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f |\n",timeofday);
  buffer_marker+=snprintf(&_gtpusp_print_buffer[buffer_marker], GTPUSP_2_PRINT_BUFFER_LEN - buffer_marker,"%s------+-------------------------------------------------+\n",timeofday);
  pr_info("%s",_gtpusp_print_buffer);
  buffer_marker = 0;

  for (octet_index = 0; octet_index < sizeP; octet_index++) {
    if ((octet_index % 16) == 0) {
      if (octet_index != 0) {
        buffer_marker+=snprintf(&_gtpusp_print_buffer[buffer_marker], GTPUSP_2_PRINT_BUFFER_LEN - buffer_marker, " |\n");
        pr_info("%s",_gtpusp_print_buffer);
        buffer_marker = 0;
      }

      buffer_marker+=snprintf(&_gtpusp_print_buffer[buffer_marker], GTPUSP_2_PRINT_BUFFER_LEN - buffer_marker, "%s %04ld |",timeofday, octet_index);
    }

    /*
     * Print every single octet in hexadecimal form
     */
    buffer_marker+=snprintf(&_gtpusp_print_buffer[buffer_marker], GTPUSP_2_PRINT_BUFFER_LEN - buffer_marker, " %02x", data_pP[octet_index]);
    /*
     * Align newline and pipes according to the octets in groups of 2
     */
  }

  /*
   * Append enough spaces and put final pipe
   */
  for (aindex = octet_index; aindex < 16; ++aindex)
    buffer_marker+=snprintf(&_gtpusp_print_buffer[buffer_marker], GTPUSP_2_PRINT_BUFFER_LEN - buffer_marker, "   ");

  //SGI_IF_DEBUG("   ");
  buffer_marker+=snprintf(&_gtpusp_print_buffer[buffer_marker], GTPUSP_2_PRINT_BUFFER_LEN - buffer_marker, " |\n");
  pr_info("%s",_gtpusp_print_buffer);
}

// for uplink GTPU traffic on S-GW
//-----------------------------------------------------------------------------
static int _udp_thread(void *data)
//-----------------------------------------------------------------------------
{
  int size, tx_size;
  int bufsize       = 8192;
#if defined(FLAG_GTPV1U_KERNEL_THREAD_SOCK_NO_WAIT)
  int success_read  = 0;
  int failed_read   = 0;
#endif
  unsigned char buf[bufsize+1];
  unsigned char gtp_resp[1024];

  /* kernel thread initialization */
  _gtpusp_sock.running = 1;

  PR_INFO(MODULE_NAME": listening on port %d\n", gtpu_sgw_port);

  /* main loop */
  while(_gtpusp_sock.thread_stop_requested == 0){
    if (kthread_should_stop()) {
    	_gtpusp_sock.running = 0;
    	PR_INFO(MODULE_NAME": kthread_stop initiated exit at %lu \n", jiffies);
		return -1; //Exit from the thread. Return value will be passed to kthread_stop()
    }
	size = _gtpusp_ksocket_receive(_gtpusp_sock.sock, &_gtpusp_sock.addr, buf, bufsize);

	if (size <= 0) {
	  if (size != -EAGAIN) {
        pr_info(MODULE_NAME": error getting datagram, sock_recvmsg error = %d\n", size);
	  }
#if defined(FLAG_GTPV1U_KERNEL_THREAD_SOCK_NO_WAIT)
	  success_read  = 0;
	  failed_read  += 1;
	  if (failed_read > 10) failed_read = 10;
      usleep_range(failed_read*20,failed_read*200);
#endif
    } else {
#if defined(FLAG_GTPV1U_KERNEL_THREAD_SOCK_NO_WAIT)
      success_read += 1;
      failed_read   = 0;
#endif
      PR_INFO(MODULE_NAME": received %d bytes\n", size);

      if ((tx_size = _gtpusp_ksocket_process_gtp(buf, size, gtp_resp)) > 0) {
        //ksocket_send(_gtpusp_sock.sock, &_gtpusp_sock.addr_send, buf, gtp_resp, tx_size, NULL, 0));
      }
    }
  }
  _gtpusp_sock.running = 0;
  if (kthread_should_stop()) {
    PR_INFO(MODULE_NAME": kthread_stop initiated exit at %lu \n", jiffies);
    return -1; //Exit from the thread. Return value will be passed to kthread_stop()
  }
  PR_INFO(MODULE_NAME": kthread do_exit()\n");
  do_exit(0);
}

//-----------------------------------------------------------------------------
static int _gtpusp_ksocket_process_gtp(const unsigned char * const rx_buf_pP, const int lenP, unsigned char* tx_buf_pP)
//-----------------------------------------------------------------------------
{
  gtpv1u_msg_t                gtpv1u_msg;
  uint8_t                     msg_type;
  struct iphdr               *iph_p    = NULL;
  struct iphdr               *new_iph_p= NULL;
  struct sk_buff             *skb_p    = NULL;
  const unsigned char *       rx_buf_p = rx_buf_pP;
  int                         err      = 0;
  struct rtable  *rt              = NULL;
  struct flowi                fl = {
	      .u = {
	        .ip4 = {
		          .daddr        = 0,
		          .flowi4_tos   = 0,
		          .flowi4_scope = RT_SCOPE_UNIVERSE,
		    }
	      }
	    };

  msg_type = rx_buf_pP[1];

  switch(msg_type) {
  case GTP_ECHO_REQ:
    PR_INFO(MODULE_NAME": TODO GTP ECHO_REQ, SEND TO GTPV1U TASK USER SPACE\n");
    //TODO;
    return 0;
    break;

  case GTP_ERROR_INDICATION:
	PR_INFO(MODULE_NAME":TODO GTP ERROR INDICATION, SEND TO GTPV1U TASK USER SPACE\n");
    //TODO;
    return 0;
    break;

  case GTP_ECHO_RSP:
    PR_INFO(MODULE_NAME":GTP ECHO_RSP, SEND TO GTPV1U TASK USER SPACE\n");
    return 0;
    break;

  case GTP_GPDU: {
    gtpv1u_msg.version       = ((*rx_buf_p) & 0xE0) >> 5;
    gtpv1u_msg.protocol_type = ((*rx_buf_p) & 0x10) >> 4;
    gtpv1u_msg.ext_hdr_flag  = ((*rx_buf_p) & 0x04) >> 2;
    gtpv1u_msg.seq_num_flag  = ((*rx_buf_p) & 0x02) >> 1;
    gtpv1u_msg.npdu_num_flag = ((*rx_buf_p) & 0x01);
    rx_buf_p++;

    gtpv1u_msg.msg_type      = *(rx_buf_p);
    rx_buf_p++;

    rx_buf_p += 2;

    gtpv1u_msg.teid          = ntohl(*((u_int32_t *)rx_buf_p));
    rx_buf_p += 4;

    if(gtpv1u_msg.ext_hdr_flag || gtpv1u_msg.seq_num_flag || gtpv1u_msg.npdu_num_flag) {
      gtpv1u_msg.seq_num             = ntohs(*(((u_int16_t *)rx_buf_p)));
      rx_buf_p                         += 2;
      gtpv1u_msg.npdu_num            = *(rx_buf_p++);
      gtpv1u_msg.next_ext_hdr_type   = *(rx_buf_p++);
    }

    gtpv1u_msg.msg_buf_offset = (u_int32_t)(rx_buf_p - rx_buf_pP);
    gtpv1u_msg.msg_buf_len = lenP - gtpv1u_msg.msg_buf_offset;
    gtpv1u_msg.msg_len    = lenP;

	iph_p            = (struct iphdr*)(&rx_buf_pP[gtpv1u_msg.msg_buf_offset]);

    fl.u.ip4.daddr        = iph_p->daddr;
    fl.u.ip4.flowi4_tos   = RT_TOS(iph_p->tos);

    rt = ip_route_output_key(&init_net, &fl.u.ip4);

    if (rt == NULL) {
      PR_INFO("GTPURH: Failed to route packet to dst 0x%x. Error: (%d)\n", fl.u.ip4.daddr, err);
      return NF_DROP;
    }

    if (rt->dst.dev == NULL) {
      pr_info("GTPURH: dst dev NULL\n");
      return 0;
    }

    skb_p = alloc_skb(LL_MAX_HEADER + ntohs(iph_p->tot_len), GFP_ATOMIC);
    if (skb_p == NULL) {
      return 0;
      }
    skb_p->priority = rt_tos2priority(iph_p->tos);
    skb_p->pkt_type = PACKET_OTHERHOST;
    skb_dst_set(skb_p, dst_clone(&rt->dst));
    skb_p->dev      = skb_dst(skb_p)->dev;

    skb_reserve(skb_p, LL_MAX_HEADER + ntohs(iph_p->tot_len));
    skb_p->protocol = htons(ETH_P_IP);

    new_iph_p = (void *)skb_push(skb_p, ntohs(iph_p->tot_len) - (iph_p->ihl << 2));
    skb_reset_transport_header(skb_p);
    new_iph_p = (void *)skb_push(skb_p, iph_p->ihl << 2);
    memcpy(new_iph_p, iph_p, ntohs(iph_p->tot_len));
    skb_reset_network_header(skb_p);
    skb_reset_inner_network_header(skb_p);
    skb_reset_inner_transport_header(skb_p);

    skb_p->mark = gtpv1u_msg.teid;

    new_iph_p->ttl        = ip4_dst_hoplimit(skb_dst(skb_p));
    skb_p->ip_summed = CHECKSUM_NONE;

    if (skb_p->len > dst_mtu(skb_dst(skb_p))) {
      PR_INFO("GTPURH: bad length\n");
      goto free_skb;
        }
    ip_local_out(skb_p);
    return 0;
free_skb:
    pr_info("GTPURH: Dropped skb\n");
    kfree_skb(skb_p);
    return 0;
  }
  break;

  default:
    PR_INFO(MODULE_NAME":ERROR GTPU msg type %u\n", msg_type);
    return 0;
  }
}

//-----------------------------------------------------------------------------
static int _gtpusp_ksocket_receive(struct socket* sock_pP, struct sockaddr_in* addr_pP, unsigned char* buf_pP, int lenP)
//-----------------------------------------------------------------------------
{
  struct msghdr msg;
  struct iovec iov;
  mm_segment_t oldfs;
  int size = 0;

  if (sock_pP->sk==NULL) return 0;

  iov.iov_base = buf_pP;
  iov.iov_len = lenP;

#if defined(FLAG_GTPV1U_KERNEL_THREAD_SOCK_NO_WAIT)
  msg.msg_flags = MSG_DONTWAIT;
#else
  msg.msg_flags = 0;
#endif
  msg.msg_name = addr_pP;
  msg.msg_namelen  = sizeof(struct sockaddr_in);
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;

  oldfs = get_fs();
  set_fs(KERNEL_DS);
  size = sock_recvmsg(sock_pP,&msg,lenP,msg.msg_flags);
  set_fs(oldfs);

  return size;
}

//-----------------------------------------------------------------------------
static int _gtpusp_ksocket_send(struct socket *sock_pP, struct sockaddr_in *addr_pP, unsigned char *gtpuh_pP, int len_gtpP, unsigned char *buf_ip_pP, int len_ipP)
//-----------------------------------------------------------------------------
{
  struct msghdr msg;
  struct iovec  iov[2];
  mm_segment_t  oldfs;
  int           size = 0;
  int           err = 0;
  int           iov_index = 0;

  if ( (err = sock_pP->ops->connect(sock_pP, (struct sockaddr *)addr_pP, sizeof(struct sockaddr), 0)) < 0 ) {
	PR_INFO(MODULE_NAME": Could not connect to socket, error = %d\n", -err);
	return 0;
  }
  if (sock_pP->sk == NULL) {
	return 0;
  }
  if ((gtpuh_pP != NULL) && (len_gtpP > 0)) {
    iov[iov_index].iov_base       = gtpuh_pP;
    iov[iov_index].iov_len        = len_gtpP;
    iov_index += 1;
  }

  if ((buf_ip_pP != NULL) && (len_ipP > 0)) {
    iov[iov_index].iov_base       = buf_ip_pP;
    iov[iov_index].iov_len        = len_ipP;
    iov_index += 1;
  }

  msg.msg_flags      = 0;
  msg.msg_name       = addr_pP;
  msg.msg_namelen    = sizeof(struct sockaddr_in);
  msg.msg_control    = NULL;
  msg.msg_controllen = 0;
  msg.msg_iov        = iov;
  msg.msg_iovlen     = iov_index;
  msg.msg_control    = NULL;

  oldfs = get_fs();
  set_fs(KERNEL_DS);
  size = sock_sendmsg(sock_pP,&msg,len_ipP+len_gtpP);
  set_fs(oldfs);

  return size;
        }

#ifdef WITH_IPV6
//-----------------------------------------------------------------------------
static void
_gtpusp_tg6_add(struct sk_buff *old_skb_pP, const struct xt_action_param *par_pP)
{
  //-----------------------------------------------------------------------------
      }
#endif

//-----------------------------------------------------------------------------
static void
_gtpusp_tg4_add(struct sk_buff *old_skb_pP, const struct xt_action_param *par_pP)
{
  //-----------------------------------------------------------------------------
  struct iphdr   *old_iph_p       = ip_hdr(old_skb_pP);
  struct gtpuhdr  gtpuh;
  uint16_t        orig_iplen = 0;
  // CONNMARK
  enum ip_conntrack_info ctinfo;
  struct nf_conn *ct = NULL;
  u_int32_t newmark;

  if (skb_linearize(old_skb_pP) < 0) {
	PR_INFO(MODULE_NAME": skb no linearize\n");
    return;
  }
  orig_iplen = ntohs(old_iph_p->tot_len);

  //----------------------------------------------------------------------------
  // CONNMARK
  //----------------------------------------------------------------------------
  ct = nf_ct_get(old_skb_pP, &ctinfo);
  if (ct == NULL) {
    PR_INFO(MODULE_NAME": _gtpusp_target_add force targinfo ltun %u to skb_pP mark %u\n",
	            ((const struct xt_gtpusp_target_info *)(par_pP->targinfo))->ltun,
	            old_skb_pP->mark);
    newmark = ((const struct xt_gtpusp_target_info *)(par_pP->targinfo))->ltun;
  } else {
    //XT_CONNMARK_RESTORE:
    newmark          = old_skb_pP->mark ^ ct->mark;

    PR_INFO(MODULE_NAME": _gtpusp_target_add restore mark %u (skb mark %u ct mark %u) len %u sgw addr %x\n",
			newmark, old_skb_pP->mark, ct->mark, orig_iplen,
			((const struct xt_gtpusp_target_info *)(par_pP->targinfo))->raddr);
    if (newmark != ((const struct xt_gtpusp_target_info *)(par_pP->targinfo))->ltun) {
  	  pr_warn(MODULE_NAME": _gtpusp_target_add restore mark 0x%x mismatch ltun 0x%x (rtun 0x%x)",
  			newmark, ((const struct xt_gtpusp_target_info *)(par_pP->targinfo))->ltun,
  			 ((const struct xt_gtpusp_target_info *)(par_pP->targinfo))->rtun);
    }
  }


  /* Add GTPu header */
  gtpuh.flags   = 0x30; /* v1 and Protocol-type=GTP */
  gtpuh.msgtype = 0xff; /* T-PDU */
  gtpuh.length  = htons(orig_iplen);
  gtpuh.tunid   = htonl(((const struct xt_gtpusp_target_info *)(par_pP->targinfo))->rtun);

  _gtpusp_sock.addr_send.sin_addr.s_addr = ((const struct xt_gtpusp_target_info *)(par_pP->targinfo))->raddr;
  _gtpusp_ksocket_send(_gtpusp_sock.sock, &_gtpusp_sock.addr_send, (unsigned char*)&gtpuh, sizeof(gtpuh), (unsigned char*)old_iph_p, orig_iplen);
  return ;
}

#ifdef WITH_IPV6
//-----------------------------------------------------------------------------
static unsigned int
_gtpusp_tg6(struct sk_buff *skb_pP, const struct xt_action_param *par_pP)
{
  //-----------------------------------------------------------------------------

  const struct xt_gtpusp_target_info *tgi_p = par_pP->targinfo;

  if (tgi_p == NULL) {
    return NF_ACCEPT;
  }

  if (tgi_p->action == PARAM_GTPUSP_ACTION_ADD) {
    _gtpusp_tg6_add(skb_pP, par_pP);
    return NF_DROP; // TODO
  }

  return NF_ACCEPT;
}
#endif

//-----------------------------------------------------------------------------
static unsigned int
_gtpusp_tg4(struct sk_buff *skb_pP, const struct xt_action_param *par_pP)
{
  //-----------------------------------------------------------------------------
  const struct xt_gtpusp_target_info *tgi_p = par_pP->targinfo;

  if (tgi_p == NULL) {
    return NF_ACCEPT;
  }

  if (tgi_p->action == PARAM_GTPUSP_ACTION_ADD) {
    _gtpusp_tg4_add(skb_pP, par_pP);
    return NF_DROP;
  }

  return NF_ACCEPT;
}

//-----------------------------------------------------------------------------
static int
__init gtpusp_tg_init(void)
//-----------------------------------------------------------------------------
{
  int            err;

  pr_info(MODULE_NAME": Initializing module (KVersion: %d)\n", KVERSION);
  pr_info(MODULE_NAME": Copyright Polaris Networks 2010-2011\n");
  pr_info(MODULE_NAME": Modified by EURECOM Lionel GAUTHIER 2014\n");
#ifndef CMAKER
  pr_info(MODULE_NAME": Compiled %s at time %s\n",__DATE__,__TIME__);
#endif
#if defined(WITH_IPV6)
  pr_info(MODULE_NAME": IPv4/IPv6 enabled\n");
#else
  pr_info(MODULE_NAME": IPv4 only enabled\n");
#endif
  pr_info(MODULE_NAME": params gtpu_enb_port=%u, gtpu_sgw_port=%u, sgw_addr=%s\n",
		  gtpu_enb_port, gtpu_sgw_port, sgw_addr);

  // UDP socket socket
  memset(&_gtpusp_sock, 0, sizeof(gtpusp_sock_t));

  /* create a socket */
  if ((err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &_gtpusp_sock.sock)) < 0 ) {
	PR_INFO(": Could not create a datagram socket, error = %d\n", -ENXIO);
	return err;
  }

  _gtpusp_sock.addr.sin_family = AF_INET;
  _gtpusp_sock.addr.sin_port   = htons(gtpu_sgw_port);
  _gtpusp_sock.addr.sin_addr.s_addr   = in_aton(sgw_addr);

  _gtpusp_sock.addr_send.sin_family      = AF_INET;
  _gtpusp_sock.addr_send.sin_port        = htons(gtpu_enb_port);
  _gtpusp_sock.addr_send.sin_addr.s_addr = in_aton(sgw_addr);

  _gtpusp_sock.thread_stop_requested     = 0;

  if ( (err = _gtpusp_sock.sock->ops->bind(_gtpusp_sock.sock, (struct sockaddr *)&_gtpusp_sock.addr, sizeof(struct sockaddr) ) ) < 0) {
    pr_info(MODULE_NAME": Could not bind socket, error = %d\n", -err);
    goto close_and_out;
  }

  // start kernel thread
  _gtpusp_sock.thread = kthread_run((void *)_udp_thread, NULL, MODULE_NAME);
  if (IS_ERR(_gtpusp_sock.thread)) {
    pr_info(MODULE_NAME": unable to start kernel thread\n");
    return -ENOMEM;
  }
  if((_gtpusp_sock.thread)) {
    wake_up_process(_gtpusp_sock.thread);
  }
  return xt_register_targets(gtpusp_tg_reg, ARRAY_SIZE(gtpusp_tg_reg));
close_and_out:
	sock_release(_gtpusp_sock.sock);
	_gtpusp_sock.sock = NULL;
	return err;
}

//-----------------------------------------------------------------------------
static void
__exit gtpusp_tg_exit(void)
//-----------------------------------------------------------------------------
{
  int err;
  int loop = 0;


  if (_gtpusp_sock.thread==NULL) {
	pr_info(MODULE_NAME": no kernel thread to kill\n");
  } else {
    if (_gtpusp_sock.running > 0) {
      _gtpusp_sock.thread_stop_requested     = 1;
      pr_info(MODULE_NAME": exit kernel thread requested\n");
      do {
        pr_info(MODULE_NAME": waking up thread with datagram\n");
        msleep(5);
        pr_info(MODULE_NAME": waiting for thread...\n");
        loop++;
      } while ((_gtpusp_sock.running > 0) && (loop < 20));
      if (_gtpusp_sock.running > 0) {
        pr_info(MODULE_NAME": stopping  kernel thread\n");
        err = kthread_stop(_gtpusp_sock.thread);
	    if(!err) {
          pr_info(MODULE_NAME": Successfully killed kernel thread!\n");
	    } else {
          pr_info(MODULE_NAME": Unsuccessfully killed kernel thread!\n");
        }
      } else {
        pr_info(MODULE_NAME": Found thread exited by itself\n");
      }
    }
  }

  /* free allocated resources before exit */
  if (_gtpusp_sock.sock != NULL) {
	sock_release(_gtpusp_sock.sock);
	_gtpusp_sock.sock = NULL;
  }
  xt_unregister_targets(gtpusp_tg_reg, ARRAY_SIZE(gtpusp_tg_reg));
  pr_info(MODULE_NAME": Unloading module\n");
}


module_init(gtpusp_tg_init);
module_exit(gtpusp_tg_exit);
MODULE_ALIAS("ipt6_GTPUSP");
MODULE_ALIAS("ipt_GTPUSP");

