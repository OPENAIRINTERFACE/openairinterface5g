/***************************************************************************
                          graal_ioctl.h  -  description
                             -------------------
    copyright            : (C) 2002 by Eurecom
    email                : michelle.wetterwald@eurecom.fr
                           yan.moret@eurecom.fr
 ***************************************************************************

 ***************************************************************************/
#ifndef _IOCTL_H
#define _IOCTL_H

#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <linux/ipv6.h>
#include <linux/in.h>
#include <linux/in6.h>
//#include <linux/netdevice.h>

//#include <graal_constant.h>
//#define GRAAL_STATE_IDLE      0
//#define GRAAL_STATE_CONNECTED     1
//#define GRAAL_STATE_ESTABLISHMENT_REQUEST 2
//#define GRAAL_STATE_ESTABLISHMENT_FAILURE 3
//#define GRAAL_STATE_RELEASE_FAILURE   4

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

extern int inet_pton(int af, const char *src, void *dst);
extern char *inet_ntop(int af, const void *src, char *dst, size_t sise);

#endif //_IOCTL_H
