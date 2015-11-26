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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/


#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define MAC_LTE_DLT 147
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

/* Context information for every MAC PDU that will be logged */
typedef struct MAC_Context_Info_t {
  unsigned short radioType;
  unsigned char  direction;
  unsigned char  rntiType;
  unsigned short rnti;
  unsigned short ueid;
  unsigned char  isRetx;
  unsigned char  crcStatusOK;

  unsigned short sysFrameNumber;
  unsigned short subFrameNumber;

  unsigned int   subframesSinceCaptureStart;
} MAC_Context_Info_t;

pcap_hdr_t file_header = {
  0xa1b2c3d4,   /* magic number */
  2, 4,         /* version number is 2.4 */
  0,            /* timezone */
  0,            /* sigfigs - apparently all tools do this */
  65535,        /* snaplen - this should be long enough */
  MAC_LTE_DLT   /* Data Link Type (DLT).  Set as unused value 147 for now */
};
