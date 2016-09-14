#ifndef S1AP_ENB_NAS_PROCEDURES_H_
#define S1AP_ENB_NAS_PROCEDURES_H_

int s1ap_eNB_handle_nas_downlink(
  const uint32_t               assoc_id,
  const uint32_t               stream,
  struct s1ap_message_s* message_p);

int s1ap_eNB_nas_uplink(instance_t instance, s1ap_uplink_nas_t *s1ap_uplink_nas_p);

void s1ap_eNB_nas_non_delivery_ind(instance_t instance,
                                   s1ap_nas_non_delivery_ind_t *s1ap_nas_non_delivery_ind);

int s1ap_eNB_handle_nas_first_req(
  instance_t instance, s1ap_nas_first_req_t *s1ap_nas_first_req_p);

int s1ap_eNB_initial_ctxt_resp(
  instance_t instance, s1ap_initial_context_setup_resp_t *initial_ctxt_resp_p);

int s1ap_eNB_ue_capabilities(instance_t instance,
                             s1ap_ue_cap_info_ind_t *ue_cap_info_ind_p);


#endif /* S1AP_ENB_NAS_PROCEDURES_H_ */
