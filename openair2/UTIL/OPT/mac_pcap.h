#ifndef __UTIL_OPT_MAC_PCAP__H__
#define __UTIL_OPT_MAC_PCAP__H__

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define MAC_LTE_DLT 147
#define DLT_IPV4 228
/********************************************************/
/* Definitions and descriptions come from:
    http://wiki.wireshark.org/Development/LibpcapFileFormat */

/* This structure gets written to the start of the file */
typedef struct pcap_hdr_s {
  unsigned int   magic_number;   /* magic number */
  unsigned short version_major;  /* major version number */
  unsigned short version_minor;  /* minor version number */
  unsigned int   thiszone;       /* GMT to local correction */
  unsigned int   sigfigs;        /* accuracy of timestamps */
  unsigned int   snaplen;        /* max length of captured packets, in octets */
  unsigned int   network;        /* data link type */
} pcap_hdr_t;

/* This structure precedes each packet */
typedef struct pcaprec_hdr_s {
  unsigned int   ts_sec;         /* timestamp seconds */
  unsigned int   ts_usec;        /* timestamp microseconds */
  unsigned int   incl_len;       /* number of octets of packet saved in file */
  unsigned int   orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

#endif
