/* Shared library add-on to iptables for the GTPUSP target
 *
 * Copyright (c) 2010-2011 Polaris Networks
 * Author: Pradip Biswas <pradip_biswas@polarisnetworks.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/netfilter.h>

#include "xt_GTPUSP.h"

#if (IPTVERSION <= 135)
#include <iptables.h>
#elif (IPTVERSION > 135)
#include <xtables.h>
#endif

#if (IPTVERSION <= 135)
#define GTPUSP_PARAM_BAD_VALUE 0
#define GTPUSP_PARAM_ONLY_ONCE 1
#define XT_GTPUSP_VERSION IPTABLES_VERSION_STRING
#define gtpusp_strtoui(s,v,m,M) !string_to_number(s,m,M,v)
#define gtpusp_exit_error exit_error
#elif (IPTVERSION > 135) && (IPTVERSION <= 141)
#define GTPUSP_PARAM_BAD_VALUE P_BAD_VALUE
#define GTPUSP_PARAM_ONLY_ONCE P_ONLY_ONCE
#define XT_GTPUSP_VERSION XTABLES_VERSION
#define gtpusp_param_act param_act
#define gtpusp_strtoui(s,v,m,M) !string_to_number(s,m,M,v)
#define gtpusp_exit_error exit_error
#elif (IPTVERSION > 141)
#define GTPUSP_PARAM_BAD_VALUE XTF_BAD_VALUE
#define GTPUSP_PARAM_ONLY_ONCE XTF_ONLY_ONCE
#define XT_GTPUSP_VERSION XTABLES_VERSION
#define gtpusp_param_act xtables_param_act
#define gtpusp_strtoui(s,v,m,M) xtables_strtoui(s,NULL,v,m,M)
#define gtpusp_exit_error xtables_error
#endif

enum {
  PARAM_LADDR = 1 << 0,
  PARAM_LTUN = 1 << 1,
  PARAM_RADDR = 1 << 2,
  PARAM_RTUN = 1 << 3,
  PARAM_ACTION = 1 << 4,
};

static void GTPUSP_help(void)
{
  printf(
    "GTPUSP target options\n"
    "  --action         value        Set action <value: add>\n"
    "  --own-ip         value        Set own IP address\n"
    "  --own-tun        value        Set own tunnel id <value: 1-2^31>\n"
    "  --peer-ip        value        Set peer IP address\n"
    "  --peer-tun       value        Set peer tunnel id <value: 1-2^31>\n");
}

#if (IPTVERSION <= 135)
/* Stolen from iptables v1.4.7 code */
void gtpusp_param_act(unsigned int status, const char *p1, ...)
{
  const char *p2, *p3;
  va_list args;
  int b;

  va_start(args, p1);

  switch (status) {
  case GTPUSP_PARAM_ONLY_ONCE:
    p2 = va_arg(args, const char *);
    b  = va_arg(args, unsigned int);

    if (!b)
      return;

    exit_error(PARAMETER_PROBLEM,
               "%s: \"%s\" option may only be specified once",
               p1, p2);
    break;

  case GTPUSP_PARAM_BAD_VALUE:
    p2 = va_arg(args, const char *);
    p3 = va_arg(args, const char *);
    exit_error(PARAMETER_PROBLEM,
               "%s: Bad value for \"%s\" option: \"%s\"",
               p1, p2, p3);
    break;

  default:
    exit_error(status, "%s", "Unknown error");
    break;
  }

  va_end(args);
}

#endif

static void parse_gtpusp_addr(const char *s, struct xt_gtpusp_target_info *info, int flag)
{
  in_addr_t addr;

  if ((addr = inet_addr(s)) == -1) {
    switch (flag) {
    case PARAM_LADDR:
      gtpusp_param_act(GTPUSP_PARAM_BAD_VALUE, "GTPUSP", "--own-ip", s);
      break;

    case PARAM_RADDR:
      gtpusp_param_act(GTPUSP_PARAM_BAD_VALUE, "GTPUSP", "--peer-ip", s);
      break;
    }
  }

  switch (flag) {
  case PARAM_LADDR:
    info->laddr = addr;
    break;

  case PARAM_RADDR:
    info->raddr = addr;
    break;
  }
}

static void parse_gtpusp_tunid(char *s, struct xt_gtpusp_target_info *info, int flag)
{
  unsigned int value;

  if (!gtpusp_strtoui(s, &value, 0, UINT32_MAX)) {
    switch (flag) {
    case PARAM_LTUN:
      gtpusp_param_act(GTPUSP_PARAM_BAD_VALUE, "GTPUSP", "--own-tun", s);
      break;

    case PARAM_RTUN:
      gtpusp_param_act(GTPUSP_PARAM_BAD_VALUE, "GTPUSP", "--peer-tun", s);
      break;
    }
  }

  switch (flag) {
  case PARAM_LTUN:
    info->ltun = value;
    break;

  case PARAM_RTUN:
    info->rtun = value;
    break;
  }
}

static void parse_gtpusp_action(char *s, struct xt_gtpusp_target_info *info, unsigned int *flags)
{
  if (!strcmp(s, "add")) {
    info->action = PARAM_GTPUSP_ACTION_ADD;
    *flags |= PARAM_GTPUSP_ACTION_ADD;
  } else {
    gtpusp_param_act(GTPUSP_PARAM_BAD_VALUE, "GTPUSP", "--action", s);
  }
}

#if (IPTVERSION <= 135)
static int
GTPUSP_parse(int c, char **argv, int invert, unsigned int *flags,
             const struct ipt_entry *entry,
             struct ipt_entry_target **target)
#else
static int
GTPUSP_parse(int c, char **argv, int invert, unsigned int *flags,
             const void *entry, struct xt_entry_target **target)
#endif
{
  struct xt_gtpusp_target_info *info = (struct xt_gtpusp_target_info *) (*target)->data;

  switch (c) {
  case '1':
    gtpusp_param_act(GTPUSP_PARAM_ONLY_ONCE, "GTPUSP", "--own-ip", *flags & PARAM_LADDR);
    parse_gtpusp_addr(optarg, info, PARAM_LADDR);
    *flags |= PARAM_LADDR;
    return 1;

  case '2':
    gtpusp_param_act(GTPUSP_PARAM_ONLY_ONCE, "GTPUSP", "--own-tun", *flags & PARAM_LTUN);
    parse_gtpusp_tunid(optarg, info, PARAM_LTUN);
    *flags |= PARAM_LTUN;
    return 1;

  case '3':
    gtpusp_param_act(GTPUSP_PARAM_ONLY_ONCE, "GTPUSP", "--peer-ip", *flags & PARAM_RADDR);
    parse_gtpusp_addr(optarg, info, PARAM_RADDR);
    *flags |= PARAM_RADDR;
    return 1;

  case '4':
    gtpusp_param_act(GTPUSP_PARAM_ONLY_ONCE, "GTPUSP", "--peer-tun", *flags & PARAM_RTUN);
    parse_gtpusp_tunid(optarg, info, PARAM_RTUN);
    *flags |= PARAM_RTUN;
    return 1;

  case '5':
    gtpusp_param_act(GTPUSP_PARAM_ONLY_ONCE, "GTPUSP", "--action", *flags & PARAM_ACTION);
    parse_gtpusp_action(optarg, info, flags);
    *flags |= PARAM_ACTION;
    return 1;
  }

  return 1;
}

static void GTPUSP_check(unsigned int flags)
{
  if (!(flags & PARAM_ACTION)) {
    gtpusp_exit_error(PARAMETER_PROBLEM, "GTPUSP: You must specify action");
  }

  if (!(flags & PARAM_LADDR)) {
    gtpusp_exit_error(PARAMETER_PROBLEM, "GTPUSP: You must specify local addr");
  }

  if (!(flags & PARAM_LTUN)) {
    gtpusp_exit_error(PARAMETER_PROBLEM, "GTPUSP: You must specify local tunnel id");
  }

  if (!(flags & PARAM_RADDR)) {
    gtpusp_exit_error(PARAMETER_PROBLEM, "GTPUSP: You must specify remote addr");
  }

  if (!(flags & PARAM_RTUN)) {
    gtpusp_exit_error(PARAMETER_PROBLEM, "GTPUSP: You must specify remote tunnel id");
  }
}

static void convert_action_to_string(int action, char *actionstr)
{
  switch(action) {
  case PARAM_GTPUSP_ACTION_ADD:
    sprintf (actionstr, "add");
    break;

  default :
    sprintf (actionstr, "unspecified!!!");
    break;
  }
}

#if (IPTVERSION <= 135)
static void
GTPUSP_print(const struct ipt_ip *ip,
             const struct ipt_entry_target *target,
             int numeric)

#else
static void
GTPUSP_print(const void *ip,
             const struct xt_entry_target *target,
             int numeric)
#endif
{
  const struct xt_gtpusp_target_info *info =
    (struct xt_gtpusp_target_info *) target->data;

  char laddr[64], raddr[64], actionstr[32];

  convert_action_to_string(info->action, actionstr);

  sprintf (laddr, "%s", inet_ntoa(*(struct in_addr*)&info->laddr));
  sprintf (raddr, "%s", inet_ntoa(*(struct in_addr*)&info->raddr));
  printf("GTPUSP self: %s tunnel: 0x%x / peer: %s tunnel: 0x%x / action: %s",
         laddr, info->ltun, raddr, info->rtun, actionstr);
}

static struct option GTPUSP_opts[] = {
  { "own-ip", 1, NULL, '1' },
  { "own-tun", 1, NULL, '2' },
  { "peer-ip", 1, NULL, '3' },
  { "peer-tun", 1, NULL, '4' },
  { "action", 1, NULL, '5' },
  { .name = NULL }
};

#if (IPTVERSION <= 135)
static struct iptables_target gtpusp_tg_reg = {
#else
static struct xtables_target gtpusp_tg_reg = {
#endif
  .name             = "GTPUSP",
  .version          = XT_GTPUSP_VERSION,
#if (IPTVERSION > 135)
  .family           = NFPROTO_IPV4,
#endif
  .size             = XT_ALIGN(sizeof(struct xt_gtpusp_target_info)),
  .userspacesize    = XT_ALIGN(sizeof(struct xt_gtpusp_target_info)),
  .help             = GTPUSP_help,
  .parse            = GTPUSP_parse,
  .final_check      = GTPUSP_check,
  .print            = GTPUSP_print,
  .extra_opts       = GTPUSP_opts,
};

void _init(void)
{
#if (IPTVERSION <= 135)
  register_target(&gtpusp_tg_reg);
#else
  xtables_register_target(&gtpusp_tg_reg);
#endif
}

