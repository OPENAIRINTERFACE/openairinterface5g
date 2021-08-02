#ifndef __GTPUNEW_ITF_H__
#define __GTPUNEW_ITF_H__
#define GTPNOK -1

# define GTPU_HEADER_OVERHEAD_MAX 64
#ifdef __cplusplus
extern "C" {
#endif

#include <openair3/GTPV1-U/gtpv1u_eNB_defs.h>
#if defined(NEW_GTPU)
#define gtpv1u_create_s1u_tunnel ocp_gtpv1u_create_s1u_tunnel
#define gtpv1u_update_s1u_tunnel ocp_gtpv1u_update_s1u_tunnel
#define gtpv1u_delete_s1u_tunnel ocp_gtpv1u_delete_s1u_tunnel
#define gtpv1u_create_x2u_tunnel ocp_gtpv1u_create_x2u_tunnel
#define gtpv1u_eNB_task          ocp_gtpv1uTask
#define nr_gtpv1u_gNB_task       ocp_gtpv1uTask
#define TASK_VARIABLE            OCP_GTPV1_U
#else
#define TASK_VARIABLE            TASK_GTPV1_U
#endif

typedef boolean_t (*gtpCallback)(
  protocol_ctxt_t  *ctxt_pP,
  const srb_flag_t     srb_flagP,
  const rb_id_t        rb_idP,
  const mui_t          muiP,
  const confirm_t      confirmP,
  const sdu_size_t     sdu_buffer_sizeP,
  unsigned char *const sdu_buffer_pP,
  const pdcp_transmission_mode_t modeP,
  const uint32_t *sourceL2Id,
  const uint32_t *destinationL2Id);

typedef struct openAddr_s {
  char originHost[HOST_NAME_MAX];
  char originService[HOST_NAME_MAX];
  char destinationHost[HOST_NAME_MAX];
  char destinationService[HOST_NAME_MAX];
  instance_t originInstance;
} openAddr_t;

// the init function create a gtp instance and return the gtp instance id
// the parameter originInstance will be sent back in each message from gtp to the creator
void ocp_gtpv1uReceiver(int h);
void ocp_gtpv1uProcessTimeout(int handle,void *arg);
int ocp_gtpv1u_create_s1u_tunnel(const instance_t instance, const gtpv1u_enb_create_tunnel_req_t  *create_tunnel_req,
                                 gtpv1u_enb_create_tunnel_resp_t *create_tunnel_resp);
int ocp_gtpv1u_update_s1u_tunnel(const instance_t instanceP,
                                 const gtpv1u_enb_create_tunnel_req_t   *create_tunnel_req_pP,
                                 const rnti_t prior_rnti
                                );
int ocp_gtpv1u_delete_s1u_tunnel( const instance_t instance, const gtpv1u_enb_delete_tunnel_req_t *const req_pP);
int gtpv1u_delete_s1u_tunnel( const instance_t instance, const gtpv1u_enb_delete_tunnel_req_t *const req_pP);

int ocp_gtpv1u_create_x2u_tunnel(
  const instance_t instanceP,
  const gtpv1u_enb_create_x2u_tunnel_req_t   *const create_tunnel_req_pP,
  gtpv1u_enb_create_x2u_tunnel_resp_t *const create_tunnel_resp_pP);


// New API
teid_t newGtpuCreateTunnel(instance_t instance, rnti_t rnti, int incoming_bearer_id, int outgoing_rb_id, teid_t teid,
                           transport_layer_addr_t remoteAddr, int port, gtpCallback callBack);
instance_t ocp_gtpv1Init(openAddr_t context);
void *ocp_gtpv1uTask(void *args);

#ifdef __cplusplus
}
#endif
#endif
