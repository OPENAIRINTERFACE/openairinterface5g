/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef __GTPUNEW_ITF_H__
#define __GTPUNEW_ITF_H__

#include <stdint.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define GTPNOK -1

# define GTPU_HEADER_OVERHEAD_MAX 64

#include "common/platform_types.h"
#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
struct protocol_ctxt_s;
typedef struct protocol_ctxt_s protocol_ctxt_t;
struct gtpv1u_enb_create_tunnel_req_s;
typedef struct gtpv1u_enb_create_tunnel_req_s gtpv1u_enb_create_tunnel_req_t;
struct gtpv1u_enb_create_tunnel_resp_s;
typedef struct gtpv1u_enb_create_tunnel_resp_s gtpv1u_enb_create_tunnel_resp_t;
struct gtpv1u_enb_delete_tunnel_req_s;
typedef struct gtpv1u_enb_delete_tunnel_req_s gtpv1u_enb_delete_tunnel_req_t;
struct gtpv1u_enb_create_x2u_tunnel_req_s;
typedef struct gtpv1u_enb_create_x2u_tunnel_req_s gtpv1u_enb_create_x2u_tunnel_req_t;
struct gtpv1u_enb_create_x2u_tunnel_resp_s;
typedef struct gtpv1u_enb_create_x2u_tunnel_resp_s gtpv1u_enb_create_x2u_tunnel_resp_t;
struct gtpv1u_gnb_create_tunnel_req_s;
typedef struct gtpv1u_gnb_create_tunnel_req_s gtpv1u_gnb_create_tunnel_req_t;
struct gtpv1u_gnb_create_tunnel_resp_s;
typedef struct gtpv1u_gnb_create_tunnel_resp_s gtpv1u_gnb_create_tunnel_resp_t;
struct gtpv1u_gnb_delete_tunnel_req_s;
typedef struct gtpv1u_gnb_delete_tunnel_req_s gtpv1u_gnb_delete_tunnel_req_t;

  typedef bool (*gtpCallback)(protocol_ctxt_t  *ctxt_pP,
                              const srb_flag_t     srb_flagP,
                              const rb_id_t        rb_idP,
                              const mui_t          muiP,
                              const confirm_t      confirmP,
                              const sdu_size_t     sdu_buffer_sizeP,
                              unsigned char *const sdu_buffer_pP,
                              const pdcp_transmission_mode_t modeP,
                              const uint32_t *sourceL2Id,
                              const uint32_t *destinationL2Id);

  typedef bool (*gtpCallbackSDAP)(protocol_ctxt_t  *ctxt_pP,
                                  const ue_id_t        ue_id,
                                  const srb_flag_t     srb_flagP,
                                  const mui_t          muiP,
                                  const confirm_t      confirmP,
                                  const sdu_size_t     sdu_buffer_sizeP,
                                  unsigned char *const sdu_buffer_pP,
                                  const pdcp_transmission_mode_t modeP,
                                  const uint32_t *sourceL2Id,
                                  const uint32_t *destinationL2Id,
                                  const uint8_t   qfi,
                                  const bool      rqi,
                                  const int       pdusession_id);

  typedef struct openAddr_s {
    char originHost[HOST_NAME_MAX];
    char originService[HOST_NAME_MAX];
    char destinationHost[HOST_NAME_MAX];
    char destinationService[HOST_NAME_MAX];
    instance_t originInstance;
  } openAddr_t;

  typedef struct extensionHeader_s{
    uint8_t buffer[500];
    uint8_t length;
  }extensionHeader_t;

  // the init function create a gtp instance and return the gtp instance id
  // the parameter originInstance will be sent back in each message from gtp to the creator
  void gtpv1uProcessTimeout(int handle,void *arg);
  int gtpv1u_create_s1u_tunnel(const instance_t instance,
                               const gtpv1u_enb_create_tunnel_req_t *create_tunnel_req,
                               gtpv1u_enb_create_tunnel_resp_t *create_tunnel_resp,
                               gtpCallback callBack);
  int gtpv1u_update_s1u_tunnel(const instance_t instanceP,
                               const gtpv1u_enb_create_tunnel_req_t   *create_tunnel_req_pP,
                               const rnti_t prior_rnti
                               );

  int gtpv1u_delete_s1u_tunnel( const instance_t instance, const gtpv1u_enb_delete_tunnel_req_t *const req_pP);
  int gtpv1u_delete_all_s1u_tunnel(const instance_t instance, const rnti_t rnti);

  int gtpv1u_create_x2u_tunnel(const instance_t instanceP,
                               const gtpv1u_enb_create_x2u_tunnel_req_t   *const create_tunnel_req_pP,
                               gtpv1u_enb_create_x2u_tunnel_resp_t *const create_tunnel_resp_pP);

  int gtpv1u_delete_x2u_tunnel( const instance_t instanceP,
                                const gtpv1u_enb_delete_tunnel_req_t *const req_pP);
  int gtpv1u_create_ngu_tunnel(const instance_t instanceP,
                               const gtpv1u_gnb_create_tunnel_req_t *const create_tunnel_req_pP,
                               gtpv1u_gnb_create_tunnel_resp_t *const create_tunnel_resp_pP,
                               gtpCallback callBack,
                               gtpCallbackSDAP callBackSDAP);

  int gtpv1u_update_ue_id(const instance_t instanceP, ue_id_t old_ue_id, ue_id_t new_ue_id);

  // New API
  teid_t newGtpuCreateTunnel(instance_t instance,
                             ue_id_t ue_id,
                             int incoming_bearer_id,
                             int outgoing_bearer_id,
                             teid_t outgoing_teid,
                             transport_layer_addr_t remoteAddr,
                             gtpCallback callBack,
                             gtpCallbackSDAP callBackSDAP);

  void GtpuUpdateTunnelOutgoingAddressAndTeid(instance_t instance,
                                    ue_id_t ue_id,
                                    ebi_t bearer_id,
                                              in_addr_t newOutgoingAddr,
                                              teid_t newOutgoingTeid);

  int newGtpuDeleteOneTunnel(instance_t instance, ue_id_t ue_id, int rb_id);
  int newGtpuDeleteAllTunnels(instance_t instance, ue_id_t ue_id);

  void gtpv1uSendDirect(instance_t instance, ue_id_t ue_id, int bearer_id, uint8_t *buf, size_t len, bool seqNumFlag, bool npduNumFlag);
  void gtpv1uSendDirectWithQFI(instance_t instance, ue_id_t ue_id, int bearer_id, int qfi, uint8_t *buf, size_t len);

  void gtpv1uSendDirectWithNRUSeqNum(instance_t instance,
                                     ue_id_t ue_id,
                                     int bearer_id,
                                     uint8_t *buf,
                                     size_t len);

  instance_t gtpv1Init(openAddr_t context);
  int gtpv1Term(instance_t inst);
  void *gtpv1uTask(void *args);

/* TS 29.281 Error Indication IEs */
#define GTPU_TEID_I 16 /* Clause 8.3 */
#define GTPU_PEER_ADDRESS 133 /* Clause 8.4 */
#define GTPU_PRIVATE_EXTENSION 255 /* Clause 8.6 */
#define GTPU_RECOVERY_TIME_STAMP 231 /* Clause 8.8 */
#define GTPU_TEID_I_VALUE_OCTETS 4
#define GTPU_PEER_ADDRESS_IPV4_OCTETS 4
#define GTPU_PEER_ADDRESS_IPV6_OCTETS 16

  /* TS 29.281 Table 7.3.1-1: Error Indication IEs */
  typedef struct gtpv1u_error_indication_s {
    // Tunnel Endpoint Identifier Data I (8.3 TS 29.281)
    teid_t teid_i;
    // GTPU Peer Address (8.4 TS 29.281)
    transport_layer_addr_t gtpu_peer_address;
  } gtpv1u_error_indication_t;

  int gtpv1u_decode_error_indication(const uint8_t *msg_buf, uint32_t msg_buf_len, gtpv1u_error_indication_t *out);
  int gtpv1u_encode_error_indication(const gtpv1u_error_indication_t *indication, uint8_t *msg_buf, uint32_t msg_buf_cap);

#ifdef __cplusplus
}
#endif
#endif
