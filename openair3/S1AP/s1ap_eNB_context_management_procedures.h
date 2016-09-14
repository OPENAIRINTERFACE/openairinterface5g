#ifndef S1AP_ENB_CONTEXT_MANAGEMENT_PROCEDURES_H_
#define S1AP_ENB_CONTEXT_MANAGEMENT_PROCEDURES_H_


int s1ap_ue_context_release_complete(instance_t instance,
                                     s1ap_ue_release_complete_t *ue_release_complete_p);

int s1ap_ue_context_release_req(instance_t instance,
                                s1ap_ue_release_req_t *ue_release_req_p);

#endif /* S1AP_ENB_CONTEXT_MANAGEMENT_PROCEDURES_H_ */
