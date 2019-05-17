/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*****************************************************************************
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    network_simulator.c

Version   0.1

Date    09/10/2012

Product   Network simulator

Subsystem Network simulator main process

Author    Frederic Maurel

Description Implements the network simulator running at the network
    side for NAS_UE testing purpose.

*****************************************************************************/

#include "network_parser.h"

#include "include/commonDef.h"
#include "util/socket.h"
#include "util/nas_timer.h"
#include "api/network/as_message.h"
#include "api/network/nas_message.h"
#include "emm/msg/emm_cause.h"
#include "esm/msg/esm_cause.h"

#include "util/nas_log.h"

#include <stdio.h>  // printf, perror
#include <errno.h>  // errno
#include <netdb.h>  // gai_strerror
#include <stdlib.h> // exit, malloc, rand
#include <signal.h> // sigaction
#include <string.h> // memset, memcpy
#include <unistd.h> // sleep

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * -----------------------------------------------------------------------------
 *            Constants
 * -----------------------------------------------------------------------------
 */

/* Tracking area code */
#define DEFAULT_TAC 0xCAFE    // two byte in hexadecimal format

/* Cell identity */
#define DEFAULT_CI  0x01020304  // four byte in hexadecimal format

/* Reference signal received power */
#define DEFAULT_RSRP  27

/* Reference signal received quality */
#define DEFAULT_RSRQ  55

/* Data bit rates */
#define BIT_RATE_64K  0x40
#define BIT_RATE_128K 0x48
#define BIT_RATE_512K 0x78
#define BIT_RATE_1024K  0x87

/* Eurecom's Access Point Name */
static const OctetString EURECOM_APN = {
  14, (uint8_t*)("www.eurecom.fr")
};

/* PDN IP address */
//static uint8_t FIRST_PDN_IPV4_ADDRESS[] = {0x0A, 0x03, 0x02, 0x3C}; /* 10.3.2.60 */
static uint8_t FIRST_PDN_IPV4_ADDRESS[] = { /* 192.168.02.60 */
  0xC0, 0xA8, 0x02, 0x3C
};
static uint8_t FIRST_PDN_IPV6_ADDRESS[] = { /* FE80::221:70FF:C0A8:023C/64 */
  0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x02, 0x3C
};
static uint8_t FIRST_PDN_IPV4V6_ADDRESS[] = { /* 192.168.02.60, FE80::221:70FF:C0A8:023C/64 */
  0xC0, 0xA8, 0x02, 0x3C, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x02, 0x3C
};
static const OctetString FIRST_IPV4_PDN = {4, FIRST_PDN_IPV4_ADDRESS};
static const OctetString FIRST_IPV6_PDN = {8, FIRST_PDN_IPV6_ADDRESS};
static const OctetString FIRST_IPV4V6_PDN = {12, FIRST_PDN_IPV4V6_ADDRESS};

/* Other PDN IP address */
//static uint8_t OTHER_PDN_IPV4_ADDRESS[] = {0x0A, 0x01, 0x20, 0x37}; /* 10.1.32.55 */
static uint8_t OTHER_PDN_IPV4_ADDRESS[] = { /* 192.168.12.187 */
  0xC0, 0xA8, 0x0C, 0xBB
};
static uint8_t OTHER_PDN_IPV6_ADDRESS[] = { /* FE80::221:70FF:C0A8:CBB/64 */
  0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBB
};
static uint8_t OTHER_PDN_IPV4V6_ADDRESS[] = { /* 192.168.12.187, FE80::221:70FF:C0A8:0CBB/64 */
  0xC0, 0xA8, 0x0C, 0xBB, 0x02, 0x21, 0x70, 0xFF, 0xC0, 0xA8, 0x0C, 0xBB
};
static const OctetString OTHER_IPV4_PDN = {4, OTHER_PDN_IPV4_ADDRESS};
static const OctetString OTHER_IPV6_PDN = {8, OTHER_PDN_IPV6_ADDRESS};
static const OctetString OTHER_IPV4V6_PDN = {12, OTHER_PDN_IPV4V6_ADDRESS};

/* XXX - Random wait time */
#define SLEEP_TIME  1

/*
 * -----------------------------------------------------------------------------
 *          Macros
 * -----------------------------------------------------------------------------
 */

#define PRINT_PLMN_DIGIT(d)     if ((d) != 0xf) printf("%u", (d))
#define PRINT_PLMN(plmn)                \
    PRINT_PLMN_DIGIT((plmn).MCCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MCCdigit3); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit1); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit2); \
    PRINT_PLMN_DIGIT((plmn).MNCdigit3)

/*
 * -----------------------------------------------------------------------------
 *            Variables
 * -----------------------------------------------------------------------------
 */

/*
 * String buffer used to send/receive messages to/from the network
 */
#define NETWORK_SIMULATOR_BUFFER_SIZE   1024
static char _network_simulator_buffer [NETWORK_SIMULATOR_BUFFER_SIZE];

/*
 * The connection endpoint used for communication with the network
 */
static socket_id_t * _network_simulator_sid;

/*
 * String buffer used to encode/decode ESM messages
 */
#define NETWORK_SIMULATOR_ESM_BUFFER_SIZE 1024
static uint8_t _network_simulator_esm_buffer[NETWORK_SIMULATOR_ESM_BUFFER_SIZE];

/*
 * Messages counter
 */
static unsigned int _network_simulator_msg_recv = 0;
static unsigned int _network_simulator_msg_sent = 0;

/*
 * EPS bearer identity
 */
static unsigned int _network_simulator_ebi = EPS_BEARER_IDENTITY_FIRST;

/*
 * Network IP version capability
 */
static const enum
{
  NETWORK_IPV4 = 0,
  NETWORK_IPV6,
  NETWORK_IPV4V6,
  NETWORK_IP_MAX
}
_network_simulator_ip_version = NETWORK_IPV4V6;
static const OctetString* _network_simulator_pdn[NETWORK_IP_MAX][2] = {
  /* IPv4 network capability */
  {&FIRST_IPV4_PDN, &OTHER_IPV4_PDN},
  /* IPv6 network capability */
  {&FIRST_IPV6_PDN, &OTHER_IPV6_PDN},
  /* IPv4v6 network capability */
  {&FIRST_IPV4V6_PDN, &OTHER_IPV4V6_PDN},
};

/*
 * -----------------------------------------------------------------------------
 *            Functions
 * -----------------------------------------------------------------------------
 */

static int _set_signal_handler(int signal, void (handler)(int));
static void _signal_handler(int signal_number);
static int _process_message(int msgID, const as_message_t* req);
static int _assign_pdn_address(int ue_pdn_type, int is_initial, int* pdn_type, OctetString* pdn);

/* Functions used to process messages received from the Access Stratum */
static int _process_cell_info_req(const cell_info_req_t* req, cell_info_cnf_t* rsp);
static int _process_establish_req(const nas_establish_req_t* req, nas_establish_cnf_t* rsp);
static int _process_ul_info_transfer_req(const ul_info_transfer_req_t* req, ul_info_transfer_cnf_t* cnf, dl_info_transfer_ind_t* ind);

/* Functions used to process EMM NAS messages */
static int _process_emm_msg(EMM_msg* msg);
static int _process_attach_request(const attach_request_msg* msg, EMM_msg* rsp, const plmn_t *plmn);
static int _process_attach_complete(const attach_complete_msg* msg);

/* Functions used to process ESM NAS messages */
static int _process_esm_msg(ESM_msg* msg);
static int _process_pdn_connectivity_request(const pdn_connectivity_request_msg* msg, ESM_msg* rsp);
static int _process_pdn_disconnect_request(const pdn_disconnect_request_msg* msg, ESM_msg* rsp);
static int _process_activate_default_eps_bearer_context_accept(const activate_default_eps_bearer_context_accept_msg* msg);
static int _process_activate_default_eps_bearer_context_reject(const activate_default_eps_bearer_context_reject_msg* msg);
static int _process_deactivate_eps_bearer_context_accept(const deactivate_eps_bearer_context_accept_msg* msg);

static void _dump_buffer(const Byte_t* buffer, size_t len)
{
  for (int i = 0; i < len; i++) {
    if ( (i%16) == 0 ) printf("\n\t");

    printf("%.2hx ", buffer[i]);
  }

  printf("\n\n");
}

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************/
int main (int argc, const char* argv[])
{
  /*
   * Get the command line options
   */
  if ( network_parser_get_options(argc, argv) != RETURNok ) {
    network_parser_print_usage();
    exit(EXIT_FAILURE);
  }

  const char* host = network_parser_get_host();
  const char* port = network_parser_get_port();

  nas_log_init(0x2f);

  /*
   * Initialize the communication channel to the NAS sublayer
   */
  _network_simulator_sid = socket_udp_open(SOCKET_SERVER, host, port);

  if (_network_simulator_sid == NULL) {
    const char* error = ( (errno < 0) ?
                          gai_strerror(errno) : strerror(errno) );
    printf("ERROR\t: socket_udp_open() failed: %s\n", error);
    exit(EXIT_FAILURE);
  }

  printf("INFO\t: The Network Simulator is now connected to %s/%s (%d)\n",
         host, port, socket_get_fd(_network_simulator_sid));

  /*
   * Set up signal handler
   */
  (void) _set_signal_handler(SIGINT, _signal_handler);
  (void) _set_signal_handler(SIGTERM, _signal_handler);

  /*
   * Network simulator main loop
   */
  while (TRUE) {
    as_message_t msg;

    /* Receive message from the NAS */
    int rbytes = socket_recv(_network_simulator_sid, _network_simulator_buffer,
                             NETWORK_SIMULATOR_BUFFER_SIZE);

    if (rbytes == RETURNerror) {
      perror("ERROR\t: socket_recv() failed");
      continue;
    }

    _network_simulator_buffer[rbytes] = '\0';

    _network_simulator_msg_recv += 1;

    /* Decode the received message */
    int msg_id = as_message_decode(_network_simulator_buffer, &msg, rbytes);

    /* Process the received message */
    int len = _process_message(msg_id, &msg);

    if (len > 0) {
      /* XXX - Sleep a random interval of time before continuing */
      sleep(rand()%SLEEP_TIME);
      /* Send the response message to the NAS sublayer */
      int sbytes = socket_send(_network_simulator_sid, _network_simulator_buffer, len);

      if (sbytes == RETURNerror) {
        perror("ERROR\t: socket_send() failed");
        continue;
      }

      _network_simulator_msg_sent += 1;
    }

    printf("\nINFO\t: %u messages received, %u messages sent\n",
           _network_simulator_msg_recv, _network_simulator_msg_sent);
  }

  /*
   * Termination cleanup
   */
  printf("INFO\t: Closing network socket %d\n", socket_get_fd(_network_simulator_sid));
  socket_close(_network_simulator_sid);

  printf("INFO\t: Network simulator exited\n");
  exit(EXIT_SUCCESS);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * Signal handler setup function
 */
static int _set_signal_handler(int signal, void (handler)(int))
{
  struct sigaction act;

  /* Initialize signal set */
  (void) memset (&act, 0, sizeof (act));
  (void) sigfillset (&act.sa_mask);
  (void) sigdelset (&act.sa_mask, SIGHUP);
  (void) sigdelset (&act.sa_mask, SIGINT);
  (void) sigdelset (&act.sa_mask, SIGTERM);
  (void) sigdelset (&act.sa_mask, SIGILL);
  (void) sigdelset (&act.sa_mask, SIGTRAP);
  (void) sigdelset (&act.sa_mask, SIGIOT);
#ifndef LINUX
  (void) sigdelset (&act.sa_mask, SIGEMT);
#endif
  (void) sigdelset (&act.sa_mask, SIGFPE);
  (void) sigdelset (&act.sa_mask, SIGBUS);
  (void) sigdelset (&act.sa_mask, SIGSEGV);
  (void) sigdelset (&act.sa_mask, SIGSYS);

  /* Initialize signal handler */
  act.sa_handler = handler;

  if ( sigaction (signal, &act, 0) < 0 ) {
    return RETURNerror;
  }

  return RETURNok;
}

/*
 * Signal handler
 */
static void _signal_handler(int signal_number)
{
  printf("\nWARNING\t: Signal %d received\n", signal_number);
  printf("INFO\t: Closing network socket %d\n", socket_get_fd(_network_simulator_sid));
  socket_close(_network_simulator_sid);
  printf("INFO\t: Network simulator exited\n");
  exit(EXIT_SUCCESS);
}

/*
 * -----------------------------------------------------------------------------
 *  Functions used to process messages received from the Access Stratum
 * -----------------------------------------------------------------------------
 */
/*
 * Process messages received from the AS sublayer
 */
static int _process_message(int msgID, const as_message_t* req)
{
  as_message_t rsp;
  int bytes = 0;

  memset(&rsp, 0, sizeof(as_message_t));

  switch (msgID) {
  case AS_CELL_INFO_REQ:
    /* Process cell information request */
    rsp.msgID = _process_cell_info_req(&req->msg.cell_info_req,
                                       &rsp.msg.cell_info_cnf);
    break;

  case AS_NAS_ESTABLISH_REQ:
    /* Process NAS signalling connection establishment request */
    rsp.msgID = _process_establish_req(&req->msg.nas_establish_req,
                                       &rsp.msg.nas_establish_cnf);
    break;

  case AS_UL_INFO_TRANSFER_REQ:
    /* Process Uplink information transfer request */
    rsp.msgID = _process_ul_info_transfer_req(&req->msg.ul_info_transfer_req,
                &rsp.msg.ul_info_transfer_cnf,
                &rsp.msg.dl_info_transfer_ind);
    break;

  default:
    fprintf(stderr, "WARNING\t: Message type %d is not valid\n", msgID);
    break;
  }

  if (rsp.msgID == AS_NAS_RELEASE_IND) {
    /* NAS signalling connection release */
    rsp.msg.nas_release_ind.cause = AS_DETACH;
    printf("\nINFO\t: Send NAS connection release indication: cause = %d\n",
           rsp.msg.nas_release_ind.cause);
  }

  if (rsp.msgID > 0) {
    /* Encode response message */
    bytes = as_message_encode(_network_simulator_buffer, &rsp,
                              NETWORK_SIMULATOR_BUFFER_SIZE);
  }

  return (bytes);
}

/*
 * Process cell information request message
 */
static int _process_cell_info_req(const cell_info_req_t* req,
                                  cell_info_cnf_t* rsp)
{
  printf("\nINFO\t: Received cell selection request: plmnID = ");
  PRINT_PLMN(req->plmnID);
  printf(", rat = 0x%x\n", req->rat);

  /* Setup cell information confirm message */
  //rsp->errCode = AS_FAILURE;
  rsp->errCode = AS_SUCCESS;
  rsp->tac = DEFAULT_TAC;
  rsp->cellID = DEFAULT_CI;
  rsp->rat = AS_EUTRAN;
  rsp->rsrp = DEFAULT_RSRP;
  rsp->rsrq = DEFAULT_RSRQ;
  printf("\nINFO\t: Send cell selection confirm: errCode = %s (%d), "
         "tac = 0x%.4x, cellID = 0x%.8x, rat = 0x%x\n",
         (rsp->errCode != AS_SUCCESS)? "FAILURE" : "SUCCESS", rsp->errCode,
         rsp->tac, rsp->cellID, rsp->rat);

  return (AS_CELL_INFO_CNF);
}

/*
 * Process NAS signalling connection establishment request
 */
static int _process_establish_req(const nas_establish_req_t* req,
                                  nas_establish_cnf_t* rsp)
{
  int rc = RETURNerror;

  printf("\nINFO\t: Received NAS connection establish request: plmnID = ");
  PRINT_PLMN(req->plmnID);
  printf(", S-TMSI = MMEcode <0x%.2x> M-TMSI <0x%.4x>",
         req->s_tmsi.MMEcode, req->s_tmsi.m_tmsi);
  printf(", cause = %d, type = %d\n", req->cause, req->type);

  const as_nas_info_t* nas_msg = &req->initialNasMsg;
  printf("INFO\t: Initial NAS message (length = %d)\n", nas_msg->length);
  _dump_buffer(nas_msg->data, nas_msg->length);

  /* Decode dedicated NAS information data */
  nas_message_t l3_msg;
  rc = nas_message_decode((char*)nas_msg->data, &l3_msg, nas_msg->length);

  if (rc < 0) {
    /* Transmission failure */
    printf("ERROR\t: Failed to decode NAS message (rc=%d)", rc);
    rsp->errCode = AS_FAILURE;
    return (AS_NAS_ESTABLISH_CNF);
  }

  printf("INFO\t: L3 NAS message = 0x%.2x\n", l3_msg.plain.emm.header.message_type);
  printf("INFO\t: Protocol Discriminator = %d\n", l3_msg.header.protocol_discriminator);
  printf("INFO\t: Security Header Type   = %d\n", l3_msg.header.security_header_type);
  printf("INFO\t: EMM message: ");

  switch (l3_msg.plain.emm.header.message_type) {
  case ATTACH_REQUEST:
    rc = _process_attach_request(&l3_msg.plain.emm.attach_request,
                                 &l3_msg.plain.emm,
                                 &req->plmnID);
    break;

  default:
    printf("ERROR\t: L3 NAS message 0x%x is not valid\n",
           l3_msg.plain.emm.header.message_type);
    break;
  }

  static int _establish_req_no_response_counter = 0;

  if (_establish_req_no_response_counter > 0) {
    sleep(rand()%SLEEP_TIME);
    _establish_req_no_response_counter -=1;
    /* XXX - Return no any response to initial NAS message request */
    return (0);
  }


  if (rc != RETURNerror) {
    int bytes = nas_message_encode(_network_simulator_buffer, &l3_msg,
                                   NETWORK_SIMULATOR_BUFFER_SIZE);

    static int _establish_req_failure_counter = 0;

    if (_establish_req_failure_counter > 0) {
      sleep(rand()%SLEEP_TIME);
      _establish_req_failure_counter -= 1;
      /* XXX - Return transmission failure indication */
      bytes = 0;
    }

    if (bytes > 0) {
      /* Setup signalling connection establishment confirm message */
      rsp->errCode = AS_SUCCESS;
      rsp->nasMsg.data = (Byte_t*)malloc(bytes * sizeof(Byte_t));

      if (rsp->nasMsg.data) {
        rsp->nasMsg.length = bytes;
        memcpy((char*)(rsp->nasMsg.data), _network_simulator_buffer, bytes);
      }
    } else {
      /* Transmission failure */
      rsp->errCode = AS_FAILURE;
    }

    printf("\nINFO\t: Send NAS connection establish confirm: errCode = %s (%d)\n",
           (rsp->errCode != AS_SUCCESS)? "FAILURE" : "SUCCESS", rsp->errCode);

    if (rsp->nasMsg.length > 0) {
      printf("INFO\t: NAS message %s (0x%x) (length = %d)\n",
             (l3_msg.plain.emm.header.message_type == ATTACH_ACCEPT)? "ATTACH_ACCEPT" :
             (l3_msg.plain.emm.header.message_type == ATTACH_REJECT)? "ATTACH_REJECT" :
             "Unknown NAS message", l3_msg.plain.emm.header.message_type,
             rsp->nasMsg.length);
      _dump_buffer(rsp->nasMsg.data, rsp->nasMsg.length);
    }

    /* NAS signalling connection confirm */
    rc = AS_NAS_ESTABLISH_CNF;
  } else {
    /* NAS signalling connection release */
    printf("\nINFO\t: Send NAS connection release indication\n");
    rc = AS_NAS_RELEASE_IND;
  }

  return (rc);
}

/*
 * Process Uplink information transfer request
 */
static int _process_ul_info_transfer_req(const ul_info_transfer_req_t* req,
    ul_info_transfer_cnf_t* cnf,
    dl_info_transfer_ind_t* ind)
{
  int rc = RETURNerror;
  int bytes;

  ESM_msg esm_msg;
  memset(&esm_msg, 0, sizeof(ESM_msg));

  printf("\nINFO\t: Received uplink information transfer request: ");
  printf("S-TMSI = MMEcode <0x%.2x> M-TMSI <0x%.4x>\n",
         req->s_tmsi.MMEcode, req->s_tmsi.m_tmsi);

  const as_nas_info_t* nas_msg = &req->nasMsg;
  printf("INFO\t: NAS message (length = %d)\n", nas_msg->length);
  _dump_buffer(nas_msg->data, nas_msg->length);

  /* Decode NAS information data */
  nas_message_t l3_msg;
  rc = nas_message_decode((char*)nas_msg->data, &l3_msg, nas_msg->length);

  if (rc < 0) {
    /* Transmission failure */
    printf("ERROR\t: Failed to decode NAS message (rc=%d)", rc);
    cnf->errCode = AS_FAILURE;
    return (AS_UL_INFO_TRANSFER_CNF);
  }

  if (l3_msg.header.protocol_discriminator != EPS_SESSION_MANAGEMENT_MESSAGE) {
    bytes = _process_emm_msg(&l3_msg.plain.emm);
  } else {
    bytes = _process_esm_msg(&l3_msg.plain.esm);
  }

  if (bytes < 0) {
    /* Transmission failure indication */
    cnf->errCode = AS_FAILURE;
    printf("\nINFO\t: Return transmission failure indication: errCode = %s (%d)\n",
           (cnf->errCode != AS_SUCCESS)? "FAILURE" : "SUCCESS", cnf->errCode);
    return (AS_UL_INFO_TRANSFER_CNF);
  } else if (bytes > 0) {
    /* Some data has to be sent back to the UE */
    ind->nasMsg.data = (Byte_t*)malloc(bytes * sizeof(Byte_t));

    if (ind->nasMsg.data) {
      ind->nasMsg.length = bytes;
      strncpy((char*)ind->nasMsg.data,
              (char*)_network_simulator_esm_buffer, bytes);
    }

    /* Downlink data transfer indication */
    printf("INFO\t: Send downlink data transfer indication\n");
    return (AS_DL_INFO_TRANSFER_IND);
  } else {
    /* Data successfully delivered */
    cnf->errCode = AS_SUCCESS;
    printf("\nINFO\t: Send uplink data transfer confirm: errCode = %s (%d)\n",
           (cnf->errCode != AS_SUCCESS)? "FAILURE" : "SUCCESS", cnf->errCode);
    return (AS_UL_INFO_TRANSFER_CNF);
  }
}

/*
 * -----------------------------------------------------------------------------
 *      Functions used to process EMM NAS messages
 * -----------------------------------------------------------------------------
 */

static int _process_emm_msg(EMM_msg* msg)
{
  int bytes = -1;

  printf("INFO\t: EMM message id         = 0x%.2x\n", msg->header.message_type);
  printf("INFO\t: Protocol Discriminator = %d\n", msg->header.protocol_discriminator);
  printf("INFO\t: Security Header Type   = %d\n", msg->header.security_header_type);
  printf("INFO\t: EMM message: ");

  switch (msg->header.message_type) {
  case ATTACH_COMPLETE:
    bytes = _process_attach_complete(&msg->attach_complete);
    break;

  default:
    printf("ERROR\t: EMM message 0x%x is not valid\n",
           msg->header.message_type);
    break;
  }

  return (bytes);
}

/*
 * Process Attach Request EMM message
 */
static int _process_attach_request(const attach_request_msg* msg, EMM_msg* rsp,
                                   const plmn_t *plmn)
{
  int rc = RETURNerror;
  int bytes = 0;

  printf("Attach Request\n");
  printf("INFO\t:\tProtocolDiscriminator\t= %d\n", msg->protocoldiscriminator);
  printf("INFO\t:\tSecurityHeaderType\t= %d\n", msg->securityheadertype);
  printf("INFO\t:\tMessageType\t\t= 0x%.2x\n", msg->messagetype);
  printf("INFO\t:\tEpsAttachType\t\t= %d\n", msg->epsattachtype);
  printf("INFO\t:\tNasKeySetIdentifier\t= [%d][%d]\n",
         msg->naskeysetidentifier.tsc,
         msg->naskeysetidentifier.naskeysetidentifier);

  printf("INFO\t:\tEpsMobileIdentity\n");

  if (msg->oldgutiorimsi.guti.typeofidentity == EPS_MOBILE_IDENTITY_GUTI) {
    const GutiEpsMobileIdentity_t* guti = &msg->oldgutiorimsi.guti;
    printf("INFO\t:\t    oddeven\t\t= %d\n", guti->oddeven);
    printf("INFO\t:\t    typeofidentity\t= %d (GUTI)\n", guti->typeofidentity);
    printf("INFO\t:\t    PLMN\t\t= %d%d%d %d%d%.x\n",
           guti->mccdigit1, guti->mccdigit2, guti->mccdigit3,
           guti->mncdigit1, guti->mncdigit2, guti->mncdigit3);
    printf("INFO\t:\t    mmegroupid\t\t= 0x%.4x\n", guti->mmegroupid);
    printf("INFO\t:\t    mmecode\t\t= 0x%.2x\n", guti->mmecode);
    printf("INFO\t:\t    mtmsi\t\t= 0x%.8x\n", guti->mtmsi);
  } else if (msg->oldgutiorimsi.imsi.typeofidentity == EPS_MOBILE_IDENTITY_IMSI) {
    const ImeiEpsMobileIdentity_t* imsi = &msg->oldgutiorimsi.imsi;
    printf("INFO\t:\t    oddeven\t= %d\n", imsi->oddeven);
    printf("INFO\t:\t    typeofidentity\t= %d (IMSI)\n", imsi->typeofidentity);
    printf("INFO\t:\t    digit1\t= %d\n", imsi->digit1);
    printf("INFO\t:\t    digit2\t= %d\n", imsi->digit2);
    printf("INFO\t:\t    digit3\t= %d\n", imsi->digit3);
    printf("INFO\t:\t    digit4\t= %d\n", imsi->digit4);
    printf("INFO\t:\t    digit5\t= %d\n", imsi->digit5);
    printf("INFO\t:\t    digit6\t= %d\n", imsi->digit6);
    printf("INFO\t:\t    digit7\t= %d\n", imsi->digit7);
    printf("INFO\t:\t    digit8\t= %d\n", imsi->digit8);
    printf("INFO\t:\t    digit9\t= %d\n", imsi->digit9);
    printf("INFO\t:\t    digit10\t= %d\n", imsi->digit10);
    printf("INFO\t:\t    digit11\t= %d\n", imsi->digit11);
    printf("INFO\t:\t    digit12\t= %d\n", imsi->digit12);
    printf("INFO\t:\t    digit13\t= %d\n", imsi->digit13);
    printf("INFO\t:\t    digit14\t= %d\n", imsi->digit14);
    printf("INFO\t:\t    digit15\t= %d\n", imsi->digit15);
  } else if (msg->oldgutiorimsi.imei.typeofidentity == EPS_MOBILE_IDENTITY_IMEI) {
    const ImeiEpsMobileIdentity_t* imei = &msg->oldgutiorimsi.imei;
    printf("INFO\t:\t    oddeven\t= %d\n", imei->oddeven);
    printf("INFO\t:\t    typeofidentity\t= %d (IMEI)\n", imei->typeofidentity);
    printf("INFO\t:\t    digit1\t= %d\n", imei->digit1);
    printf("INFO\t:\t    digit2\t= %d\n", imei->digit2);
    printf("INFO\t:\t    digit3\t= %d\n", imei->digit3);
    printf("INFO\t:\t    digit4\t= %d\n", imei->digit4);
    printf("INFO\t:\t    digit5\t= %d\n", imei->digit5);
    printf("INFO\t:\t    digit6\t= %d\n", imei->digit6);
    printf("INFO\t:\t    digit7\t= %d\n", imei->digit7);
    printf("INFO\t:\t    digit8\t= %d\n", imei->digit8);
    printf("INFO\t:\t    digit9\t= %d\n", imei->digit9);
    printf("INFO\t:\t    digit10\t= %d\n", imei->digit10);
    printf("INFO\t:\t    digit11\t= %d\n", imei->digit11);
    printf("INFO\t:\t    digit12\t= %d\n", imei->digit12);
    printf("INFO\t:\t    digit13\t= %d\n", imei->digit13);
    printf("INFO\t:\t    digit14\t= %d\n", imei->digit14);
    printf("INFO\t:\t    digit15\t= %d\n", imei->digit15);
  }

  printf("INFO\t:\tUeNetworkCapability\n");
  printf("INFO\t:\t    eea   = %d\n", msg->uenetworkcapability.eea);
  printf("INFO\t:\t    eia   = %d\n", msg->uenetworkcapability.eia);
  printf("INFO\t:\t    uea   = %d\n", msg->uenetworkcapability.uea);
  printf("INFO\t:\t    ucs2  = %d\n", msg->uenetworkcapability.ucs2);
  printf("INFO\t:\t    uia   = %d\n", msg->uenetworkcapability.uia);
  printf("INFO\t:\t    spare = %d\n", msg->uenetworkcapability.spare);
  printf("INFO\t:\t    csfb  = %d\n", msg->uenetworkcapability.csfb);
  printf("INFO\t:\t    lpp   = %d\n", msg->uenetworkcapability.lpp);
  printf("INFO\t:\t    lcs   = %d\n", msg->uenetworkcapability.lcs);
  printf("INFO\t:\t    srvcc = %d\n", msg->uenetworkcapability.srvcc);
  printf("INFO\t:\t    nf    = %d\n", msg->uenetworkcapability.nf);

  printf("INFO\t: ESM message container (length = %d)\n",
         msg->esmmessagecontainer.esmmessagecontainercontents.length);
  _dump_buffer(msg->esmmessagecontainer.esmmessagecontainercontents.value,
               msg->esmmessagecontainer.esmmessagecontainercontents.length);

  /* Decode ESM message container */
  ESM_msg esm_msg;
  rc = esm_msg_decode(&esm_msg, msg->esmmessagecontainer.esmmessagecontainercontents.value, msg->esmmessagecontainer.esmmessagecontainercontents.length);

  if (rc < 0) {
    /* Reject attach request cause = EMM_CAUSE_PROTOCOL_ERROR */
    printf("ERROR\t: Failed to decode ESM message (rc=%d)", rc);
    rc = RETURNerror;
  } else {
    printf("INFO\t: L3 ESM message id              = 0x%.2x\n", esm_msg.header.message_type);
    printf("INFO\t: Protocol Discriminator         = %d\n", esm_msg.header.protocol_discriminator);
    printf("INFO\t: EPS Bearer Identity            = %d\n", esm_msg.header.eps_bearer_identity);
    printf("INFO\t: Procedure Transaction Identity = %d\n", esm_msg.header.procedure_transaction_identity);
    printf("INFO\t: ESM message: ");

    /* Process ESM message */
    switch (esm_msg.header.message_type) {
    case PDN_CONNECTIVITY_REQUEST:
      rc = _process_pdn_connectivity_request(&esm_msg.pdn_connectivity_request, &esm_msg);
      break;

    default:
      printf("ERROR\t: Received ESM message 0x%x is not valid\n",
             esm_msg.header.message_type);
      break;
    }
  }

  /* Setup EMM response message */
  memset(rsp, 0, sizeof(EMM_msg));
  rsp->header.protocol_discriminator = EPS_MOBILITY_MANAGEMENT_MESSAGE;
  rsp->header.security_header_type = SECURITY_HEADER_TYPE_NOT_PROTECTED;

  switch (rc) {
  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST:
    rsp->header.message_type = ATTACH_ACCEPT;
    break;

  case PDN_CONNECTIVITY_REJECT:
    rsp->header.message_type = ATTACH_REJECT;
    break;

  default:
    if (rc != RETURNerror) {
      printf("ERROR\t: ESM message 0x%x to be sent is not valid\n", rc);
    }

    break;
  }

  /* Encode ESM response message */
  if (rc != RETURNerror) {
    bytes = esm_msg_encode(&esm_msg, _network_simulator_esm_buffer,
                           NETWORK_SIMULATOR_ESM_BUFFER_SIZE);
  }

  if (rsp->header.message_type != ATTACH_REJECT) {
    /* Setup Attach Accept EMM message */
    attach_accept_msg* accept = &rsp->attach_accept;
    accept->epsattachresult = EPS_ATTACH_RESULT_EPS;
    accept->t3412value.unit = GPRS_TIMER_UNIT_0S;
    accept->tailist.typeoflist = 1; /* list of TACs belonging to one PLMN,
           * with consecutive TAC values  */
    accept->tailist.numberofelements = 4;
    accept->tailist.mccdigit1 = plmn->MCCdigit1;
    accept->tailist.mccdigit2 = plmn->MCCdigit2;
    accept->tailist.mccdigit3 = plmn->MCCdigit3;
    accept->tailist.mncdigit1 = plmn->MNCdigit1;
    accept->tailist.mncdigit2 = plmn->MNCdigit2;
    accept->tailist.mncdigit3 = plmn->MNCdigit3;
    accept->tailist.tac = DEFAULT_TAC;
    accept->esmmessagecontainer.esmmessagecontainercontents.length = bytes;
    accept->esmmessagecontainer.esmmessagecontainercontents.value = _network_simulator_esm_buffer;
  } else {
    /* Setup Attach Reject EMM message */
    attach_reject_msg* reject = &rsp->attach_reject;

    if (bytes > 0) {
      reject->emmcause = EMM_CAUSE_ESM_FAILURE;
      reject->presencemask = ATTACH_REJECT_ESM_MESSAGE_CONTAINER_PRESENT;
      reject->esmmessagecontainer.esmmessagecontainercontents.length = bytes;
      reject->esmmessagecontainer.esmmessagecontainercontents.value = _network_simulator_esm_buffer;
    } else {
      reject->emmcause = EMM_CAUSE_PROTOCOL_ERROR;
      //reject->emmcause = EMM_CAUSE_ILLEGAL_UE;
      //reject->emmcause = EMM_CAUSE_ILLEGAL_ME;
      //reject->emmcause = EMM_CAUSE_EPS_NOT_ALLOWED;
      //reject->emmcause = EMM_CAUSE_PLMN_NOT_ALLOWED;
      //reject->emmcause = EMM_CAUSE_NOT_AUTHORIZED_IN_PLMN;
      //reject->emmcause = EMM_CAUSE_TA_NOT_ALLOWED;
      //reject->emmcause = EMM_CAUSE_ROAMING_NOT_ALLOWED;
      //reject->emmcause = EMM_CAUSE_EPS_NOT_ALLOWED_IN_PLMN;
      //reject->emmcause = EMM_CAUSE_NO_SUITABLE_CELLS;
      //reject->emmcause = EMM_CAUSE_CONGESTION;
      //reject->emmcause = EMM_CAUSE_CSG_NOT_AUTHORIZED;
    }
  }

  static int _attach_request_release_counter = 0;

  if (_attach_request_release_counter > 0) {
    sleep(rand()%SLEEP_TIME);
    _attach_request_release_counter -= 1;
    /* XXX - Return NAS signalling connection release indication */
    return (RETURNerror);
  }

  return (RETURNok);
}

/*
 * Process Attach Complete EMM message
 */
static int _process_attach_complete(const attach_complete_msg* msg)
{
  int bytes = -1;

  printf("Attach Complete\n");
  printf("INFO\t: ESM message container (length = %d)\n",
         msg->esmmessagecontainer.esmmessagecontainercontents.length);
  _dump_buffer(msg->esmmessagecontainer.esmmessagecontainercontents.value,
               msg->esmmessagecontainer.esmmessagecontainercontents.length);

  /* Decode ESM message container */
  ESM_msg esm_msg;
  bytes = esm_msg_decode(&esm_msg, msg->esmmessagecontainer.esmmessagecontainercontents.value, msg->esmmessagecontainer.esmmessagecontainercontents.length);

  if (bytes < 0) {
    /* Transmission failure */
    printf("ERROR\t: Transmission failure\n");
    return (bytes);
  }

  printf("INFO\t: ESM message id                 = 0x%.2x\n", esm_msg.header.message_type);
  printf("INFO\t: Protocol Discriminator         = %d\n", esm_msg.header.protocol_discriminator);
  printf("INFO\t: EPS Bearer Identity            = %d\n", esm_msg.header.eps_bearer_identity);
  printf("INFO\t: Procedure Transaction Identity = %d\n", esm_msg.header.procedure_transaction_identity);
  printf("INFO\t: ESM message: ");

  switch (esm_msg.header.message_type) {
  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
    bytes = _process_activate_default_eps_bearer_context_accept(
              &esm_msg.activate_default_eps_bearer_context_accept);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
    bytes = _process_activate_default_eps_bearer_context_reject(
              &esm_msg.activate_default_eps_bearer_context_reject);
    break;

  default:
    printf("ERROR\t: ESM message 0x%x is not valid\n", esm_msg.header.message_type);
    break;
  }

  static int _attach_complete_failure_counter = 0;

  if (_attach_complete_failure_counter > 0) {
    sleep(rand()%SLEEP_TIME);
    _attach_complete_failure_counter -= 1;
    /* XXX - Return transmission failure indication */
    return (-1);
  }

  return (bytes);
}

/*
 * -----------------------------------------------------------------------------
 *      Functions used to process ESM NAS messages
 * -----------------------------------------------------------------------------
 */

static int _process_esm_msg(ESM_msg* msg)
{
  int bytes = 0;
  int rc = RETURNerror;

  ESM_msg rsp;

  printf("INFO\t: ESM message id                 = 0x%.2x\n", msg->header.message_type);
  printf("INFO\t: Protocol Discriminator         = %d\n", msg->header.protocol_discriminator);
  printf("INFO\t: EPS Bearer Identity            = %d\n", msg->header.eps_bearer_identity);
  printf("INFO\t: Procedure Transaction Identity = %d\n", msg->header.procedure_transaction_identity);
  printf("INFO\t: ESM message: ");

  switch (msg->header.message_type) {
  case PDN_CONNECTIVITY_REQUEST:
    rc = _process_pdn_connectivity_request(&msg->pdn_connectivity_request,
                                           &rsp);
    break;

  case PDN_DISCONNECT_REQUEST:
    rc = _process_pdn_disconnect_request(&msg->pdn_disconnect_request,
                                         &rsp);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_ACCEPT:
    rc = _process_activate_default_eps_bearer_context_accept(
           &msg->activate_default_eps_bearer_context_accept);
    break;

  case ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REJECT:
    rc = _process_activate_default_eps_bearer_context_reject(
           &msg->activate_default_eps_bearer_context_reject);
    break;

  case DEACTIVATE_EPS_BEARER_CONTEXT_ACCEPT:
    rc = _process_deactivate_eps_bearer_context_accept(
           &msg->deactivate_eps_bearer_context_accept);
    break;

  default:
    printf("ERROR\t: ESM message 0x%x is not valid\n",
           msg->header.message_type);
    break;
  }

  if (rc > 0) {
    /* Encode ESM response message */
    bytes = esm_msg_encode(&rsp, _network_simulator_esm_buffer,
                           NETWORK_SIMULATOR_ESM_BUFFER_SIZE);
    printf("\nINFO\t: Send NAS message %s (0x%x) (length = %d)\n",
           (rsp.header.message_type == PDN_CONNECTIVITY_REJECT)? "PDN_CONNECTIVITY_REJECT" :
           (rsp.header.message_type == PDN_DISCONNECT_REJECT)? "PDN_DISCONNECT_REJECT" :
           (rsp.header.message_type == ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST)? "ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST" :
           (rsp.header.message_type == DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST)? "DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST" :
           "Unknown NAS message", rsp.header.message_type, bytes);
    _dump_buffer(_network_simulator_esm_buffer, bytes);
  }

  return (bytes);
}

/*
 * Process PDN Connectivity Request ESM message
 */
static int _process_pdn_connectivity_request(const pdn_connectivity_request_msg* msg, ESM_msg* rsp)
{
  int pti = msg->proceduretransactionidentity;

  /* Get the APN if any */
  OctetString apn = {0, NULL};

  if (msg->presencemask & PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT) {
    const OctetString* apn_value = &msg->accesspointname.accesspointnamevalue;
    apn.value = (uint8_t*)malloc(apn_value->length * sizeof(uint8_t));

    if (apn.value != NULL) {
      apn.length = apn_value->length;
      apn.value = (uint8_t*)strncpy((char*)apn.value, (char*)apn_value->value, apn.length);
      apn.value[apn.length] = '\0';
    }
  }

  /* Get the PDN type */
  int requested_pdn_type = msg->pdntype;

  printf("PDN Connectivity Request\n");
  printf("INFO\t:\tProtocolDiscriminator\t\t= %d\n", msg->protocoldiscriminator);
  printf("INFO\t:\tEpsBearerIdentity\t\t= %d\n", msg->epsbeareridentity);
  printf("INFO\t:\tProcedureTransactionIdentity\t= %d\n", msg->proceduretransactionidentity);
  printf("INFO\t:\tMessageType\t\t\t= 0x%.2x\n", msg->messagetype);
  printf("INFO\t:\tRequestType\t\t\t= %d\n", msg->requesttype);
  printf("INFO\t:\tPdnType\t\t\t\t= %d\n", msg->pdntype);

  if (apn.length > 0) {
    printf("INFO\t:\taccesspointname\t\t\t= %s\n", apn.value);
  }

  memset(rsp, 0, sizeof(ESM_msg));

  rsp->header.protocol_discriminator = EPS_SESSION_MANAGEMENT_MESSAGE;

  if (_network_simulator_ebi > EPS_BEARER_IDENTITY_LAST) {
    _network_simulator_ebi = EPS_BEARER_IDENTITY_FIRST;
  }

  rsp->header.eps_bearer_identity = _network_simulator_ebi++;
  rsp->header.procedure_transaction_identity = pti;

  static int _pdn_connectivity_reject_counter = 0;

  if (_pdn_connectivity_reject_counter > 0) {
    sleep(rand()%SLEEP_TIME);
    _pdn_connectivity_reject_counter -= 1;
    /* XXX - Setup PDN connectivity Reject message */
    pdn_connectivity_reject_msg* reject = &rsp->pdn_connectivity_reject;
    reject->messagetype = PDN_CONNECTIVITY_REJECT;
    reject->esmcause = ESM_CAUSE_PROTOCOL_ERROR;
  } else if ( (_network_simulator_ip_version != NETWORK_IPV4V6) &&
              (_network_simulator_ip_version != requested_pdn_type) ) {
    /* Setup PDN connectivity Reject message */
    pdn_connectivity_reject_msg* reject = &rsp->pdn_connectivity_reject;
    reject->messagetype = PDN_CONNECTIVITY_REJECT;

    if (_network_simulator_ip_version != NETWORK_IPV4) {
      reject->esmcause = ESM_CAUSE_PDN_TYPE_IPV6_ONLY_ALLOWED;
    } else {
      reject->esmcause = ESM_CAUSE_PDN_TYPE_IPV4_ONLY_ALLOWED;
    }
  } else {
    int pdn_type;
    int esm_cause;
    /* Setup Activate Default EPS Bearer Request message */
    activate_default_eps_bearer_context_request_msg* request = &rsp->activate_default_eps_bearer_context_request;
    request->messagetype = ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST;
    request->epsqos.qci = 3;    /* GBR bearer for playback video */
    request->epsqos.bitRatesPresent = 1;
    request->epsqos.bitRates.maxBitRateForUL = BIT_RATE_128K; /* 128K */
    request->epsqos.bitRates.maxBitRateForDL = BIT_RATE_1024K;  /* 1M */
    request->epsqos.bitRates.guarBitRateForUL = BIT_RATE_64K; /* 64K  */
    request->epsqos.bitRates.guarBitRateForDL = BIT_RATE_512K;  /* 512K */

    /* Assign PDN address */
    if (apn.length > 0) {
      request->accesspointname.accesspointnamevalue = apn;
      esm_cause = _assign_pdn_address(requested_pdn_type, FALSE, &pdn_type,
                                      &request->pdnaddress.pdnaddressinformation);
    } else {
      request->accesspointname.accesspointnamevalue = EURECOM_APN;
      esm_cause = _assign_pdn_address(requested_pdn_type, TRUE, &pdn_type,
                                      &request->pdnaddress.pdnaddressinformation);
    }

    request->pdnaddress.pdntypevalue = pdn_type;

    if (esm_cause != ESM_CAUSE_SUCCESS) {
      /* Set ESM cause */
      request->presencemask |= ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT;
      request->esmcause = esm_cause;
    }

  }

  return (rsp->header.message_type);
}

/*
 * Process PDN Disconnect Request ESM message
 */
static int _process_pdn_disconnect_request(const pdn_disconnect_request_msg* msg, ESM_msg* rsp)
{
  int pti = msg->proceduretransactionidentity;
  int ebi = msg->linkedepsbeareridentity;

  printf("PDN Disconnect Request\n");
  printf("INFO\t:\tProtocolDiscriminator\t\t= %d\n", msg->protocoldiscriminator);
  printf("INFO\t:\tEpsBearerIdentity\t\t= %d\n", msg->epsbeareridentity);
  printf("INFO\t:\tProcedureTransactionIdentity\t= %d\n", msg->proceduretransactionidentity);
  printf("INFO\t:\tMessageType\t\t\t= 0x%.2x\n", msg->messagetype);
  printf("INFO\t:\tLinkedEpsBearerIdentity\t= %d\n", msg->linkedepsbeareridentity);

  memset(rsp, 0, sizeof(ESM_msg));

  rsp->header.protocol_discriminator = EPS_SESSION_MANAGEMENT_MESSAGE;
  rsp->header.eps_bearer_identity = ebi;
  rsp->header.procedure_transaction_identity = pti;

  static int _pdn_disconnect_reject_counter = 0;

  if (_pdn_disconnect_reject_counter > 0) {
    sleep(rand()%SLEEP_TIME);
    _pdn_disconnect_reject_counter -= 1;
    /* XXX - Setup PDN Disconnect Reject message */
    pdn_disconnect_reject_msg* reject = &rsp->pdn_disconnect_reject;
    reject->epsbeareridentity = EPS_BEARER_IDENTITY_UNASSIGNED;
    reject->messagetype = PDN_DISCONNECT_REJECT;
    reject->esmcause = ESM_CAUSE_PROTOCOL_ERROR;
  } else {
    /* Setup Deactivate EPS Bearer Request message */
    deactivate_eps_bearer_context_request_msg* request = &rsp->deactivate_eps_bearer_context_request;
    request->messagetype = DEACTIVATE_EPS_BEARER_CONTEXT_REQUEST;
    request->esmcause = ESM_CAUSE_REGULAR_DEACTIVATION;
  }

  return (rsp->header.message_type);
}

/*
 * Process Activate Default EPS Bearer Context Accept ESM message
 */
static int _process_activate_default_eps_bearer_context_accept(const activate_default_eps_bearer_context_accept_msg* msg)
{
  printf("Activate Default EPS Bearer Context Accept\n");
  printf("INFO\t:\tProtocolDiscriminator\t\t= %d\n", msg->protocoldiscriminator);
  printf("INFO\t:\tEpsBearerIdentity\t\t= %d\n", msg->epsbeareridentity);
  printf("INFO\t:\tProcedureTransactionIdentity\t= %d\n", msg->proceduretransactionidentity);
  printf("INFO\t:\tMessageType\t\t\t= 0x%.2x\n", msg->messagetype);

  return (0);
}

/*
 * Process Activate Default EPS Bearer Context Reject ESM message
 */
static int _process_activate_default_eps_bearer_context_reject(const activate_default_eps_bearer_context_reject_msg* msg)
{
  printf("Activate Default EPS Bearer Context Reject\n");
  printf("INFO\t:\tProtocolDiscriminator\t\t= %d\n", msg->protocoldiscriminator);
  printf("INFO\t:\tEpsBearerIdentity\t\t= %d\n", msg->epsbeareridentity);
  printf("INFO\t:\tProcedureTransactionIdentity\t= %d\n", msg->proceduretransactionidentity);
  printf("INFO\t:\tMessageType\t\t\t= 0x%.2x\n", msg->messagetype);
  printf("INFO\t:\tESM cause\t\t\t= %d\n", msg->esmcause);

  return (0);
}

/*
 * Process Deactivate EPS Bearer Context Accept ESM message
 */
static int _process_deactivate_eps_bearer_context_accept(const deactivate_eps_bearer_context_accept_msg* msg)
{
  printf("Deactivate EPS Bearer Context Accept\n");
  printf("INFO\t:\tProtocolDiscriminator\t\t= %d\n", msg->protocoldiscriminator);
  printf("INFO\t:\tEpsBearerIdentity\t\t= %d\n", msg->epsbeareridentity);
  printf("INFO\t:\tProcedureTransactionIdentity\t= %d\n", msg->proceduretransactionidentity);
  printf("INFO\t:\tMessageType\t\t\t= 0x%.2x\n", msg->messagetype);

  return (0);
}

/*
 * Assign PDN address according to the network IP capability
 */
static int _assign_pdn_address(int ue_pdn_type, int is_initial, int* pdn_type, OctetString* pdn)
{
  if (_network_simulator_ip_version == NETWORK_IPV4) {
    /* The network supports IPv4 addressing only */
    *pdn_type = PDN_VALUE_TYPE_IPV4;
    *pdn = *_network_simulator_pdn[NETWORK_IPV4][is_initial];

    if (ue_pdn_type == PDN_TYPE_IPV4V6) {
      /* Return ESM cause code #50 */
      return (ESM_CAUSE_PDN_TYPE_IPV4_ONLY_ALLOWED);
    }
  } else if (_network_simulator_ip_version == NETWORK_IPV6) {
    /* The network supports IPv6 addressing only */
    *pdn_type = PDN_VALUE_TYPE_IPV6;
    *pdn = *_network_simulator_pdn[NETWORK_IPV6][is_initial];

    if (ue_pdn_type == PDN_TYPE_IPV4V6) {
      /* Return ESM cause code #51 */
      return (ESM_CAUSE_PDN_TYPE_IPV6_ONLY_ALLOWED);
    }
  } else {
    /* The network supports either IPv4 or IPv6 addressing */
    if (ue_pdn_type != PDN_TYPE_IPV4) {
      /* Assign IPv6 address */
      *pdn_type = PDN_VALUE_TYPE_IPV6;
      *pdn = *_network_simulator_pdn[NETWORK_IPV6][is_initial];
    } else {
      /* Assign IPv4 address */
      *pdn_type = PDN_VALUE_TYPE_IPV4;
      *pdn = *_network_simulator_pdn[NETWORK_IPV4][is_initial];
    }

    if (ue_pdn_type == PDN_TYPE_IPV4V6) {
      *pdn_type = PDN_VALUE_TYPE_IPV4V6;
      *pdn = *_network_simulator_pdn[NETWORK_IPV4V6][is_initial];
      /* Return ESM cause code #52 */
      //return (ESM_CAUSE_SINGLE_ADDRESS_BEARERS_ONLY_ALLOWED);
    }
  }

  return (ESM_CAUSE_SUCCESS);
}

/*
        07 41 71 0b f6 02 f8 01 01 02 0f 00 00 00 01 05
        00 00 00 00 00 00 04 02 01 d0 11 52 02 f8 01 00

        07 41 71 0b f6 02 f8 01 01 02 0f 00 00 00 01 05
        00 00 00 00 00 00 04 02 01 d0 11 52 02 f8 01 00
        01 e0
 */

/*
        04 01 04 10 20 8f 10 00 00 00 0f 00 00 00 01 00
        00 00 1b 00 00 00 07 41 71 0b f6 02 f8 01 01 02
        0f 00 00 00 01 05 00 00 00 00 00 00 04 02 01 d0
        11

        04 01 04 10 00 00 0f 00 00 00 01 00 00 00 20 8f
        10 00 1b 00 00 00 07 41 71 0b f6 02 f8 01 01 02
        0f 00 00 00 01 05 00 00 00 00 00 00 04 02 01 d0
        11

        04 01 04 10 00 00 0f 00 00 00 01 00 00 00 20 8f
        10 00 22 00 00 00 07 41 71 0b f6 02 f8 01 01 02
        0f 00 00 00 01 05 80 80 00 00 00 00 04 02 01 d0
        31 52 02 f8 01 00 01 e0

        04 01 04 10 00 00 0f 00 00 00 01 00 00 00 20 8f
        10 00 28 00 00 00 17 00 00 00 00 00 17 41 01 0b
        f6 02 f8 01 01 02 0f 00 00 00 01 05 80 80 00 00
        00 00 04 02 02 d0 31 52 02 f8 01 00 01 e0
*/
/*
  U16 msgID = [04 01] - AS_NAS_ESTABLISH_REQ
  U8 cause = [04] - NET_ESTABLISH_CAUSE_MO_SIGNAL
  U8 type = [10]  - NET_ESTABLISH_TYPE_ORIGINATING_SIGNAL
  plmn_t plmnID = [20 8f 10]
  Byte_t MCCdigit2:4 = [0]
  Byte_t MCCdigit1:4 = [2]
  Byte_t MNCdigit3:4 = [f]
  Byte_t MCCdigit3:4 = [8]
  Byte_t MNCdigit2:4 = [0]
  Byte_t MNCdigit1:4 = [1]
  as_stmsi_t s_tmsi
  U8 MMEcode = [0f]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 1b] (27 octets)
  U8* initialNasMsg = [07 41 71 0b f6 02 f8 01 01 02 0f 00 00 00 01 05]
          [00 00 00 00 00 00 04 02 01 d0 11]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [41]  - Attach Request
  U8:4 NAS KSI = [7]
  U8:4 Attach Type = [1]
  EPS Mobile Identity = [0b f6 02 f8 01 01 02 0f 00 00 00 01]
  Length = [0b] (11 bytes)
  U8:4 = 1111 = [f]
  U8:1 oddeven = 0
  U8:3 typeofidentity = 110 = [6]
  U8:4 mccdigit2 = [0]
  U8:4 mccdigit1 = [2]
  U8:4 mncdigit3 = [f]
  U8:4 mccdigit3 = [8]
  U8:4 mncdigit2 = [0]
  U8:4 mncdigit1 = [1]
  U16  mmegroupid = [01 02]
  U8   mmecode = [0f]
  U32  mtmsi = [00 00 00 01]
  UE network capability = [05 00 00 00 00 00]
  Length = [05] (5 bytes)
  U8   eea = [00]
  U8   eia = [00]
  U8   uea = [00]
  U8:1 ucs2 = 0
  U8:7 uia = 0000000 = [00]
  U8:3 spare = 000
  U8:1 csfb = 0
  U8:1 lpp = 0
  U8:1 lcs = 0
  U8:1 srvcc = 0
  U8:1 nf = 0 = [00]

  ESM message container = [00 04 02 01 d0 11]
  Length = [00 04] (4 bytes)
  ESM message = [02 01 d0 31]
  U8:4 EPS bearer identity = [0]  - Not assigned
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [01]
  U8   ESM message identifier = [d0]  - PDN Connectivity Request
  U8:4 Pdn type = [1]     - IPv4
  U8:4 Request type = [1]   - Initial request
*/

/*
        06 01 0f 00 00 00 01 00 00 00 07 00 00 00 07 43
        00 03 52 00 c2

        06 01 00 00 00 00 0f 00 00 00 01 00 00 00 0d 00
        00 00 27 00 00 00 00 cf 27 43 00 03 52 00 c2
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  as_stmsi_t s_tmsi
  U8 MMEcode = [0f]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 07] (7 octets)
  U8* NasMsg = [07 43 00 03 52 00 c2]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [43]  - Attach Complete

  ESM message container = [00 03 52 00 c2]
  Length = [00 03] (3 bytes)
  ESM message = [52 00 c2]
  U8:4 EPS bearer identity = [5]
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [00] - Not assigned
  U8   ESM message identifier = [c2]  - Activate Default EPS Bearer Context Accept
 */


/*
        04 08 01 00 00 00 03 00 00 00 07 44 03

  U16 msgID = [04 08] - AS_NAS_ESTABLISH_CNF
  U8  errCode = [01]  - AS_SUCCESS

  U32 Length = [00 00 00 03] (3 octets)
  U8* initialNasMsg = [07 44 03]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [44]  - Attach Reject
  U8   EMM cause = [03]   - EMM_CAUSE_ILLEGAL_UE
 */

/*
        06 01 0f 00 00 00 01 00 00 00 17 00 00 00 02 03
        d0 11 28 11 77 77 77 2e 63 6f 6d 34 69 6e 6e 6f
        76 2e 63 6f 6d

  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  as_stmsi_t s_tmsi
  U8 MMEcode = [0f]
  U32 m_tmsi = [00 00 00 01]
  U32 Length = [00 00 00 17] (23 octets)
  U8* NasMsg = [02 03 d0 11 28 11 77 77 77 2e 63 6f 6d 34 69 6e 6e 6f 76 2e 63 6f 6d]
  U8:4 EPS bearer identity = [0]  - Unassigned
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [03]
  U8   ESM message identifier = [d0]  - PDN Connectivity Request
*/

/*
        07 04 04 00 00 00 52 03 d1 6f

  U16 msgID = [07 04] - AS_DL_INFO_TRANSFER_IND
  U32 Length = [04 00 00 00] (4 octets)
  U8* NasMsg = [52 03 d1 6f]
  U8:4 EPS bearer identity = [5]  - EBI5
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [03]
  U8   ESM message identifier = [d1]  - PDN Connectivity Reject
  U8   ESM cause = [6f]     - Protocol error, unspecified
*/
/*
        04 08 01 00 00 00 0a 00 00 00 07 44 13 78 00 04
        01 d1 6f 00
  U16 msgID = [04 08] - AS_NAS_ESTABLISH_CNF
  U8  errCode = [01]  - AS_SUCCESS

  U32 Length = [00 00 00 0a] (10 octets)
  U8* initialNasMsg = [07 44 13 78 00 04 01 d1 6f 00]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [44]  - Attach Reject
  U8   EMM cause = [13]   - EMM_CAUSE_ESM_FAILURE (19)
  TLV = [78 00 04 01 d1 6f 00]
    IEI = 78    - ESM container
    Length = [00 04]  - 4 octets
    [01 d1 6f 00]
    U8:4 EPS bearer identity = [0]
    U8:4 Protocol discriminator = [1] - ESM message
*/

/*
        06 01 00 00 00 00 0f 00 00 00 01 00 00 00 04 00
        00 00 02 03 d2 06
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  as_stmsi_t s_tmsi
  U8 MMEcode = [0f]
  U32 m_tmsi = [00 00 00 01]
  U32 Length = [00 00 00 04] (4 octets)
  U8* NasMsg = [02 03 d2 06]

  U8:4 EPS bearer identity = [0]  - Unassigned
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [03]
  U8   ESM message identifier = [d2]  - PDN Disconnect Request
  U8:4 Linked EPS bearer identity = [6] - EPS bearer identity = 6
*/
