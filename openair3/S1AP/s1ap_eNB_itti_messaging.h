#ifndef S1AP_ENB_ITTI_MESSAGING_H_
#define S1AP_ENB_ITTI_MESSAGING_H_

void s1ap_eNB_itti_send_sctp_data_req(instance_t instance, int32_t assoc_id, uint8_t *buffer,
                                      uint32_t buffer_length, uint16_t stream);

void s1ap_eNB_itti_send_nas_downlink_ind(instance_t instance,
    uint16_t ue_initial_id,
    uint32_t eNB_ue_s1ap_id,
    uint8_t *nas_pdu,
    uint32_t nas_pdu_length);

void s1ap_eNB_itti_send_sctp_close_association(instance_t instance,
    int32_t assoc_id);


#endif /* S1AP_ENB_ITTI_MESSAGING_H_ */
