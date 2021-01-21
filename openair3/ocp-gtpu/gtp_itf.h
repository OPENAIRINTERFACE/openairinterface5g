#ifndef __UDP_ITF_H__
#define __UDP_ITF_H__
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
  #endif

typedef struct openAddr_s {
  char originHost[HOST_NAME_MAX];
  char originService[HOST_NAME_MAX];
  char destinationHost[HOST_NAME_MAX];
  char destinationService[HOST_NAME_MAX];
  instance_t originInstance;
} openAddr_t;

// the init function create a gtp instance and return the gtp instance id
// the parameter originInstance will be sent back in each message from gtp to the creator
instance_t ocp_gtpv1Init(openAddr_t context);
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
  const gtpv1u_enb_create_x2u_tunnel_req_t *  const create_tunnel_req_pP,
        gtpv1u_enb_create_x2u_tunnel_resp_t * const create_tunnel_resp_pP);
  void *ocp_gtpv1uTask(void *args);
  
#ifdef __cplusplus
}
#endif
#endif
