/*! \file gtpv1u_eNB_task.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef GTPV1U_ENB_TASK_H_
#define GTPV1U_ENB_TASK_H_

#include "messages_types.h"

int
gtpv1u_new_data_req(
  uint8_t enb_id,
  uint8_t ue_id,
  uint8_t rab_id,
  uint8_t *buffer,
  uint32_t buf_len);
void *gtpv1u_eNB_task(void *args);

int
gtpv1u_create_s1u_tunnel(
  const instance_t instanceP,
  const gtpv1u_enb_create_tunnel_req_t *  const create_tunnel_req_pP,
        gtpv1u_enb_create_tunnel_resp_t * const create_tunnel_resp_pP);


#endif /* GTPV1U_ENB_TASK_H_ */
