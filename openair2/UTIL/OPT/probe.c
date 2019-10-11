/* mac_lte_logger.c
 *
 * Example code for sending MAC LTE frames over UDP
 * Written by Martin Mathieson, with input from Kiran Kumar
 * This header file may also be distributed under
 * the terms of the BSD Licence as follows:
 *
 * Copyright (C) 2009 Martin Mathieson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE
 */

/*! \file probe.c
* \brief
* \author navid nikaein
* \date 2010-2012
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
*/
/** @defgroup _oai System definitions
There is different modules:
- OAI Address
- OAI Components
- \ref _frame

numbering:
-# OAI Address
-# OAI Components
-# \ref _frame

The following diagram is based on graphviz (http://www.graphviz.org/), you need to install the package to view the diagram.
 *
 * \dot
 * digraph group_frame  {
 *     node [shape=rect, fontname=Helvetica, fontsize=8,style=filled,fillcolor=lightgrey];
 *     a [ label = " Trace_pdu"];
 *     b [ label = " dispatcher"];
 *     c [ label = " send_ul_mac_pdu"];
 *     D [ label = " send_Dl_mac_pdu"];
 *     E [ label = " SendFrame"];
 *     F[ label = " _Send_Ra_Mac_Pdu"];
 *      a->b;
 *      b->c;
 *      b->d;
 *  label="Architecture"
 *
 * }
 * \enddot
\section _doxy Doxygen Help
You can use the provided Doxyfile as the configuration file or alternatively run "doxygen -g Doxyfile" to generat the file.
You need at least to set the some variables in the Doxyfile including "PROJECT_NAME","PROJECT_NUMBER","INPUT","IMAGE_PATH".
Doxygen help and commands can be found at http://www.stack.nl/~dimitri/doxygen/commands.html#cmdprotocol

\section _arch Architecture

You need to set the IMAGE_PATH in your Doxyfile

\image html arch.png "Architecture"
\image latex arch.eps "Architecture"

\subsection _mac MAC
thisis the mac
\subsection _rlc RLC
this is the rlc
\subsection _impl Implementation
what about the implementation


*@{*/

#include <pthread.h>
#include <stdint.h>
#include "common/config/config_userapi.h"
#include "opt.h"

int opt_enabled=0;

//static unsigned char g_PDUBuffer[1600];
//static unsigned int g_PDUOffset;

static char *in_ip;
static char *in_path;
FILE *file_fd = NULL;
pcap_hdr_t file_header = {
  0xa1b2c3d4,   /* magic number */
  2, 4,         /* version number is 2.4 */
  0,            /* timezone */
  0,            /* sigfigs - apparently all tools do this */
  65535,        /* snaplen - this should be long enough */
  MAC_LTE_DLT   /* Data Link Type (DLT).  Set as unused value 147 for now */
};

trace_mode_t opt_type = OPT_NONE;
static unsigned int subframesSinceCaptureStart;

static int g_socksd = -1;/* UDP socket used for sending frames */
static struct sockaddr_in g_serv_addr;

typedef struct {
  pthread_t thread;
  int sd;
  struct sockaddr_in address;
} opt_listener_t;

opt_listener_t opt_listener;

static void SendFrame(guint8 radioType, guint8 direction, guint8 rntiType,
                      guint16 rnti, guint16 ueid, guint16 sysframeNumber,
                      guint8 isPredefinedData, guint8 retx, guint8 crcStatus,
                      guint8 oob_event, guint8 oob_event_value,
                      uint8_t *pdu_buffer, unsigned int pdu_buffer_size);

static int MAC_LTE_PCAP_WritePDU(MAC_Context_Info_t *context,
                                 const unsigned char *PDU, unsigned int length);

static void *opt_listener_thread(void *arg) {
  ssize_t ret;
  struct sockaddr_in from_address;
  socklen_t socklen = sizeof(from_address);
  memset(&from_address, 0, sizeof(from_address));

  while(1) {
    /* Simply drop packets */
    ret = recvfrom(opt_listener.sd, NULL, 0, 0, (struct sockaddr *)&from_address,
                   &socklen);

    if (ret == 0) {
      LOG_D(OPT, "Remote host is no more connected, exiting thread\n");
      pthread_exit(NULL);
    } else if (ret < 0) {
      /* Errors */
      LOG_E(OPT, "An error occured during recvfrom:  %s\n", strerror(errno));
      pthread_exit(NULL);
    } else {
      /* Normal read -> discard PDU */
      LOG_D(OPT, "Incoming data received from: %s:%u with length %zd\n",
            inet_ntoa(opt_listener.address.sin_addr),
            ntohs(opt_listener.address.sin_port), ret);
    }
  }

  return NULL;
}

static
int opt_create_listener_socket(char *ip_address, uint16_t port) {
  /* Create an UDP socket and listen on it.
   * Silently discard PDU received.
   */
  int sd = -1;
  int ret = -1;
  memset(&opt_listener, 0, sizeof(opt_listener_t));
  sd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sd < 0) {
    LOG_E(OPT, "Failed to create listener socket: %s\n", strerror(errno));
    sd = -1;
    opt_type = OPT_NONE;
    return -1;
  }

  opt_listener.sd = sd;
  opt_listener.address.sin_family = AF_INET;
  /* Listening only on provided IP address */
  opt_listener.address.sin_addr.s_addr = inet_addr(ip_address);
  opt_listener.address.sin_port = htons(port);
  ret = bind(opt_listener.sd, (struct sockaddr *) &opt_listener.address, sizeof(opt_listener.address));

  if (ret != 0) {
    LOG_E(OPT, "Failed to bind socket to (%s:%u): %s\n",
          inet_ntoa(opt_listener.address.sin_addr),
          ntohs(opt_listener.address.sin_port), strerror(errno));
    opt_type = OPT_NONE;
    close(opt_listener.sd);
    opt_listener.sd = -1;
    return -1;
  }

  ret = pthread_create(&opt_listener.thread, NULL, opt_listener_thread, NULL);

  if (ret != 0) {
    LOG_E(OPT, "Failed to create thread for server socket: %s\n", strerror(errno));
    opt_type = OPT_NONE;
    close(opt_listener.sd);
    opt_listener.sd = -1;
    return -1;
  }

  return 0;
}

//struct mac_lte_info * mac_info;

// #define WIRESHARK_DEV
/* if you want to define this, then you need to
 * 1. checkout the wireshark dev at : svn co http://anonsvn.wireshark.org/wireshark/trunk wireshark
 * 2. copy the local packet-mac-lte.h and packet-mac-lte.c into epan/dissectors/
 * 3. install it, read INSTALL
 * 4. run the wireshark and capture packets from lo interface, and filter out icmp packet (!icmp)
 * 5. run ./oasim -a -P0 -n 30 | grep OPT
 */
/* Add framing header to MAC PDU and send. */
static void SendFrame(guint8 radioType, guint8 direction, guint8 rntiType,
                      guint16 rnti, guint16 ueid, guint16 sfnSf,
                      guint8 isPredefinedData, guint8 retx, guint8 crcStatus,
                      guint8 oob_event, guint8 oob_event_value,
                      uint8_t *pdu_buffer, unsigned int pdu_buffer_size) {
  unsigned char frameBuffer[9000];
  unsigned int frameOffset;
  ssize_t bytesSent;
  frameOffset = 0;
  uint16_t tmp16;
  memcpy(frameBuffer+frameOffset, MAC_LTE_START_STRING,
         strlen(MAC_LTE_START_STRING));
  frameOffset += strlen(MAC_LTE_START_STRING);
  /******************************************************************************/
  /* Now write out fixed fields (the mandatory elements of struct mac_lte_info) */
  frameBuffer[frameOffset++] = radioType;
  frameBuffer[frameOffset++] = direction;
  frameBuffer[frameOffset++] = rntiType;
  /*************************************/
  /* Now optional fields               */
  /* RNTI */
  frameBuffer[frameOffset++] = MAC_LTE_RNTI_TAG;
  tmp16 = htons(rnti);
  memcpy(frameBuffer+frameOffset, &tmp16, 2);
  frameOffset += 2;
  /* UEId */
  frameBuffer[frameOffset++] = MAC_LTE_UEID_TAG;
  tmp16 = htons(ueid);
  memcpy(frameBuffer+frameOffset, &tmp16, 2);
  frameOffset += 2;
  /* Subframe number */
  frameBuffer[frameOffset++] = MAC_LTE_FRAME_SUBFRAME_TAG;
  tmp16 = htons(sfnSf); // frame counter : this will give an expert info as wireshark expects SF and not F
  memcpy(frameBuffer+frameOffset, &tmp16, 2);
  frameOffset += 2;
  frameBuffer[frameOffset++] = MAC_LTE_CRC_STATUS_TAG;
  frameBuffer[frameOffset++] = crcStatus;
#ifdef WIRESHARK_DEV
  frameOffset += 2;
  tmp16 = htons(sfnSf); // subframe
  memcpy(frameBuffer+frameOffset, &tmp16, 2);
  frameOffset += 2;
#endif

  /***********************************************************/
  /* For these optional fields, no need to encode if value is default */
  if (!isPredefinedData) {
    frameBuffer[frameOffset++] = MAC_LTE_PREDEFINED_DATA_TAG;
    frameBuffer[frameOffset++] = isPredefinedData;
  }

  if (retx != 0) {
    frameBuffer[frameOffset++] = MAC_LTE_RETX_TAG;
    frameBuffer[frameOffset++] = retx;
  }

  //#ifdef WIRESHARK_DEV

  /* Relating to out-of-band events */
  /* N.B. dissector will only look to these fields if length is 0... */
  if (pdu_buffer_size==0) {
    switch (oob_event) {
      case ltemac_send_preamble :
        LOG_D(OPT,"ltemac_send_preamble event %02x."
              //"%02x."
              "%02x.%02x\n",
              MAC_LTE_SEND_PREAMBLE_TAG,
              //ltemac_send_preamble,
              rnti,
              oob_event_value);
        //frameBuffer[frameOffset++]=0;
        //frameBuffer[frameOffset++]=0;
        //frameBuffer[frameOffset++]=0;
        frameBuffer[frameOffset++] = MAC_LTE_SEND_PREAMBLE_TAG;
        //frameBuffer[frameOffset++]=ltemac_send_preamble;
        frameBuffer[frameOffset++]=rnti; // is the preamble
        frameBuffer[frameOffset++]=oob_event_value;
        break;

      case ltemac_send_sr:
        frameBuffer[frameOffset++]=MAC_LTE_SR_TAG ;
        frameOffset+=2;
        frameBuffer[frameOffset++]=rnti;
        frameOffset++;
        frameBuffer[frameOffset++]=oob_event_value;
        frameOffset++;
        break;

      case ltemac_sr_failure:
      default:
        LOG_W(OPT,"not implemeneted yet\n");
        break;
    }
  }

  //#endif
  /***************************************/
  /* Now write the MAC PDU               */
  frameBuffer[frameOffset++] = MAC_LTE_PAYLOAD_TAG;

  /* Append actual PDU  */
  //memcpy(frameBuffer+frameOffset, g_PDUBuffer, g_PDUOffset);
  //frameOffset += g_PDUOffset;
  if (pdu_buffer != NULL) {
    memcpy(frameBuffer+frameOffset, (void *)pdu_buffer, pdu_buffer_size);
    frameOffset += pdu_buffer_size;
  }

  /* Send out the data over the UDP socket */
  bytesSent = sendto(g_socksd, frameBuffer, frameOffset, 0,
                     (const struct sockaddr *)&g_serv_addr, sizeof(g_serv_addr));

  if (bytesSent != frameOffset) {
    LOG_W(OPT, "sendto() failed (not a thread-safe func)- expected %d bytes, got %ld (errno=%d)\n",
          frameOffset, bytesSent, errno);
    //exit(1);
  }
}

/* Write an individual PDU (PCAP packet header + mac-context + mac-pdu) */
static int MAC_LTE_PCAP_WritePDU(MAC_Context_Info_t *context,
                                 const uint8_t *PDU, unsigned int length) {
  pcaprec_hdr_t packet_header;
  uint8_t context_header[256];
  int offset = 0;
  unsigned short tmp16;
  /*****************************************************************/
  /* Context information (same as written by UDP heuristic clients */
  context_header[offset++] = context->radioType;
  context_header[offset++] = context->direction;
  context_header[offset++] = context->rntiType;
  /* RNTI */
  context_header[offset++] = MAC_LTE_RNTI_TAG;
  tmp16 = htons(context->rnti);
  memcpy(context_header+offset, &tmp16, 2);
  offset += 2;
  /* UEId */
  context_header[offset++] = MAC_LTE_UEID_TAG;
  tmp16 = htons(context->ueid);
  memcpy(context_header+offset, &tmp16, 2);
  offset += 2;
  /* Subframe number */
  context_header[offset++] = MAC_LTE_FRAME_SUBFRAME_TAG;
  tmp16 = htons(context->subFrameNumber);
  memcpy(context_header+offset, &tmp16, 2);
  offset += 2;
  /* CRC Status */
  context_header[offset++] = MAC_LTE_CRC_STATUS_TAG;
  context_header[offset++] = context->crcStatusOK;
  /* Data tag immediately preceding PDU */
  context_header[offset++] = MAC_LTE_PAYLOAD_TAG;
  /****************************************************************/
  /* PCAP Header                                                  */
  /* TODO: Timestamp might want to be relative to a more sensible
     base time... */
  packet_header.ts_sec = context->subframesSinceCaptureStart / 1000;
  packet_header.ts_usec = (context->subframesSinceCaptureStart % 1000) * 1000;
  packet_header.incl_len = offset + length;
  packet_header.orig_len = offset + length;
  /***************************************************************/
  /* Now write everything to the file                            */
  fwrite(&packet_header, sizeof(pcaprec_hdr_t), 1, file_fd);
  fwrite(context_header, 1, offset, file_fd);
  fwrite(PDU, 1, length, file_fd);
  return 1;
}
#include <common/ran_context.h>
extern RAN_CONTEXT_t RC;
#include <openair1/PHY/phy_extern_ue.h>
/* Remote serveraddress (where Wireshark is running) */
void trace_pdu_implementation(int direction, uint8_t *pdu_buffer, unsigned int pdu_buffer_size,
                              int ueid, int rntiType, int rnti, uint16_t sysFrameNumber, uint8_t subFrameNumber, int oob_event,
                              int oob_event_value) {
  MAC_Context_Info_t pdu_context;
  int radioType=FDD_RADIO;
  LOG_D(OPT,"sending packet to wireshark: direction=%s, size: %d, ueid: %d, rnti: %x, frame/sf: %d.%d\n",
        direction?"DL":"UL", pdu_buffer_size, ueid, rnti, sysFrameNumber,subFrameNumber);

  if (RC.eNB && RC.eNB[0][0]!=NULL)
    radioType=RC.eNB[0][0]->frame_parms.frame_type== FDD ? FDD_RADIO:TDD_RADIO;
  else if (PHY_vars_UE_g && PHY_vars_UE_g[0][0] != NULL)
    radioType=PHY_vars_UE_g[0][0]->frame_parms.frame_type== FDD ? FDD_RADIO:TDD_RADIO;
  else {
    LOG_E(OPT,"not a eNB neither a UE!!! \n");
    return;
  }

  switch (opt_type) {
    case OPT_WIRESHARK :
      if (g_socksd == -1) {
        return;
      }

      SendFrame( radioType,
                 (direction == DIRECTION_DOWNLINK) ? DIRECTION_DOWNLINK : DIRECTION_UPLINK,
                 rntiType, rnti, ueid, (sysFrameNumber<<4) + subFrameNumber,
                 1, 0, 1,  //guint8 isPredefinedData, guint8 retx, guint8 crcStatus
                 oob_event,oob_event_value,
                 pdu_buffer, pdu_buffer_size);
      break;

    case OPT_PCAP:
      if (file_fd == NULL) {
        return;
      }

      pdu_context.radioType =  radioType;
      pdu_context.direction = (direction == DIRECTION_DOWNLINK) ? DIRECTION_DOWNLINK
                              : DIRECTION_UPLINK;
      pdu_context.rntiType = rntiType;
      pdu_context.rnti = rnti;
      pdu_context.ueid = ueid;
      pdu_context.isRetx = 0;
      pdu_context.crcStatusOK =1;
      pdu_context.sysFrameNumber = sysFrameNumber;
      pdu_context.subFrameNumber = subFrameNumber;
      pdu_context.subframesSinceCaptureStart = subframesSinceCaptureStart++;
      MAC_LTE_PCAP_WritePDU( &pdu_context, pdu_buffer, pdu_buffer_size);
      break;

    case OPT_TSHARK:
    default:
      break;
  }
}
/*---------------------------------------------------*/
int init_opt(void) {
  char *in_type=NULL;
  paramdef_t opt_params[]          = OPT_PARAMS_DESC ;
  checkedparam_t opt_checkParams[] = OPTPARAMS_CHECK_DESC;
  uint16_t in_port;
  config_set_checkfunctions(opt_params, opt_checkParams,
                            sizeof(opt_params)/sizeof(paramdef_t));
  config_get( opt_params,sizeof(opt_params)/sizeof(paramdef_t),OPT_CONFIGPREFIX);
  subframesSinceCaptureStart = 0;
  int tmptype = config_get_processedint( &(opt_params[OPTTYPE_IDX]));

  if (tmptype == OPT_NONE) {
    opt_enabled=0;
    LOG_I(OPT,"OPT disabled\n");
    return 0;
  } else if (tmptype == OPT_PCAP && strlen(in_path) > 0) {
    opt_type = OPT_PCAP;
    opt_enabled=1;
    LOG_I(OPT,"Enabling OPT for PCAP  with the following file %s \n",in_path);
  } else if (tmptype == OPT_WIRESHARK && strlen(in_ip) > 0) {
    opt_enabled=1;
    opt_type = OPT_WIRESHARK;
    LOG_I(OPT,"Enabling OPT for wireshark for local interface %s\n",in_ip);
  } else {
    LOG_E(OPT,"Invalid OPT configuration\n");
    config_printhelp(opt_params,sizeof(opt_params)/sizeof(paramdef_t),OPT_CONFIGPREFIX);
  }

  in_port = PACKET_MAC_LTE_DEFAULT_UDP_PORT;

  // trace_mode
  switch (opt_type) {
    case OPT_WIRESHARK:

      /* Create local server socket only if using localhost address */
      if (strcmp(in_ip, "127.0.0.1") == 0) {
        opt_create_listener_socket(in_ip, in_port);
      }

      g_socksd = socket(AF_INET, SOCK_DGRAM, 0);

      if (g_socksd == -1) {
        LOG_E(OPT, "Error trying to create socket (errno=%d)\n", errno);
        LOG_E(OPT, "CREATING SOCKET FAILED\n");
        return (-1);
      }

      /* Get remote IP address from the function argument */
      g_serv_addr.sin_family = AF_INET;
      g_serv_addr.sin_port = htons(in_port);
      g_serv_addr.sin_addr.s_addr = inet_addr(in_ip);
      break;

    case OPT_PCAP:
      file_fd = fopen(in_path, "w");

      if (file_fd == NULL) {
        LOG_D(OPT, "Failed to open file \"%s\" for writing\n", in_path);
        return (-1);
      }

      /* Write the file header */
      fwrite(&file_header, sizeof(pcap_hdr_t), 1, file_fd);
      break;

    case OPT_TSHARK:
      LOG_W(OPT, "Tshark is currently not supported\n");
      opt_type = OPT_NONE;
      break;

    default:
      opt_type = OPT_NONE;
      LOG_W(OPT, "supported Option\n");
      break;
  }

  if ( opt_type == OPT_WIRESHARK )
    LOG_E(OPT,"mode Wireshark: ip %s port %d\n", in_ip, in_port);
  else if (opt_type == OPT_PCAP)
    LOG_E(OPT,"mode PCAB : path is %s \n",in_path);
  else
    LOG_E(OPT,"Unsupported or unknown mode %d \n", opt_type);

  //  mac_info = (mac_info*)malloc16(sizeof(mac_lte_info));
  // memset(mac_info, 0, sizeof(mac_lte_info)+pdu_buffer_size + 8);
  return (1);
}
void terminate_opt(void) {
  /* Close local socket */
  //  free(mac_info);
  if (opt_type != OPT_NONE) {
    pthread_cancel(opt_listener.thread);
  }

  switch (opt_type) {
    case OPT_WIRESHARK:
      close(g_socksd);
      g_socksd = -1;
      break;

    case OPT_PCAP:
      fclose (file_fd);
      file_fd = NULL;
      break;

    default:
      break;
  }
}

