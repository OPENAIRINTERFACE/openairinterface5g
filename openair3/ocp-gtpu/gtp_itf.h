#ifndef __GTPUNEW_ITF_H__
#define __GTPUNEW_ITF_H__

#define GTPNOK -1

# define GTPU_HEADER_OVERHEAD_MAX 64

#include "common/utils/hashtable/hashtable.h"
#ifdef __cplusplus
extern "C" {
#endif

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
                                  const rb_id_t        rb_idP,
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
  void gtpv1uReceiver(int h);
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

  int gtpv1u_delete_ngu_tunnel( const instance_t instance,
                                gtpv1u_gnb_delete_tunnel_req_t *req);

  int gtpv1u_update_ue_id(const instance_t instanceP, ue_id_t old_ue_id, ue_id_t new_ue_id);

  // New API
  teid_t newGtpuCreateTunnel(instance_t instance,
                             ue_id_t ue_id,
                             int incoming_bearer_id,
                             int outgoing_rb_id,
                             teid_t teid,
                             int outgoing_qfi,
                             transport_layer_addr_t remoteAddr,
                             int port,
                             gtpCallback callBack,
                             gtpCallbackSDAP callBackSDAP);

  void GtpuUpdateTunnelOutgoingAddressAndTeid(instance_t instance,
                                    ue_id_t ue_id,
                                    ebi_t bearer_id,
                                              in_addr_t newOutgoingAddr,
                                              teid_t newOutgoingTeid);

  int newGtpuDeleteOneTunnel(instance_t instance, ue_id_t ue_id, int rb_id);
  int newGtpuDeleteAllTunnels(instance_t instance, ue_id_t ue_id);
  int newGtpuDeleteTunnels(instance_t instance, ue_id_t ue_id, int nbTunnels, pdusessionid_t *pdusession_id);
  instance_t gtpv1Init(openAddr_t context);
  void *gtpv1uTask(void *args);

  // Legacy to fix
  typedef struct gtpv1u_data_s {
    /* RB tree of UEs */
    hash_table_t         *ue_mapping;
  } gtpv1u_data_t;
#define GTPV1U_BEARER_OFFSET 3
#define GTPV1U_MAX_BEARERS_ID     (max_val_LTE_DRB_Identity - GTPV1U_BEARER_OFFSET)
  typedef enum {
    BEARER_DOWN = 0,
    BEARER_IN_CONFIG,
    BEARER_UP,
    BEARER_DL_HANDOVER,
    BEARER_UL_HANDOVER,
    BEARER_MAX,
  } bearer_state_t;

  typedef struct fixMe_gtpv1u_bearer_s {
    /* TEID used in dl and ul */
    teid_t          teid_eNB;                ///< eNB TEID
    uintptr_t       teid_eNB_stack_session;  ///< eNB TEID
    teid_t          teid_sgw;                ///< Remote TEID
    in_addr_t       sgw_ip_addr;
    struct in6_addr sgw_ip6_addr;
    teid_t          teid_teNB;
    in_addr_t       tenb_ip_addr;       ///< target eNB ipv4
    struct in6_addr tenb_ip6_addr;        ///< target eNB ipv6
    tcp_udp_port_t  port;
    //NwGtpv1uStackSessionHandleT stack_session;
    bearer_state_t state;
  } fixMe_gtpv1u_bearer_t;

  typedef struct gtpv1u_ue_data_s {
    /* UE identifier for oaisim stack */
    rnti_t   ue_id;

    /* Unique identifier used between PDCP and GTP-U to distinguish UEs */
    uint32_t instance_id;
    int      num_bearers;
    /* Bearer related data.
     * Note that the first LCID available for data is 3 and we fixed the maximum
     * number of e-rab per UE to be (32 [id range]), max RB is 11. The real rb id will 3 + rab_id (3..32).
     */
    fixMe_gtpv1u_bearer_t bearers[GTPV1U_MAX_BEARERS_ID];

    //RB_ENTRY(gtpv1u_ue_data_s) gtpv1u_ue_node;
  } gtpv1u_ue_data_t;

#ifdef __cplusplus
}
#endif
#endif
