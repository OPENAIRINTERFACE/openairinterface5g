/*! \file rrc_eNB_GTPV1U.h
 * \brief rrc GTPV1U procedures for eNB
 * \author Lionel GAUTHIER
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */

#ifndef RRC_ENB_GTPV1U_H_
#define RRC_ENB_GTPV1U_H_

# if defined(ENABLE_USE_MME)


#   if defined(ENABLE_ITTI)

/*! \fn rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP(const protocol_ctxt_t* const ctxt_pP, const gtpv1u_enb_create_tunnel_resp_t * const create_tunnel_resp_pP)
 *\brief Process GTPV1U_ENB_CREATE_TUNNEL_RESP message received from GTPV1U, retrieve the enb teid created.
 *\param ctxt_pP Running context
 *\param create_tunnel_resp_pP Message received by RRC.
 *\return 0 when successful, -1 if the UE index can not be retrieved. */
int
rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP(
  const protocol_ctxt_t* const ctxt_pP,
  const gtpv1u_enb_create_tunnel_resp_t * const create_tunnel_resp_pP
);

#   endif
# endif /* defined(ENABLE_USE_MME) */
#endif /* RRC_ENB_GTPV1U_H_ */
