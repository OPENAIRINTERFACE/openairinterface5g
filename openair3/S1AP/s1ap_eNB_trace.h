#ifndef S1AP_ENB_TRACE_H_
#define S1AP_ENB_TRACE_H_

// int s1ap_eNB_generate_trace_failure(sctp_data_t        *sctp_data_p,
//                                     int32_t             stream,
//                                     uint32_t            eNB_ue_s1ap_id,
//                                     uint32_t            mme_ue_s1ap_id,
//                                     E_UTRAN_Trace_ID_t *trace_id,
//                                     Cause_t            *cause_p);

// int s1ap_eNB_handle_trace_start(eNB_mme_desc_t *eNB_desc_p,
//                                 sctp_queue_item_t *packet_p,
//                                 struct s1ap_message_s *message_p);
int s1ap_eNB_handle_trace_start(uint32_t               assoc_id,
                                uint32_t               stream,
                                struct s1ap_message_s *message_p);

// int s1ap_eNB_handle_deactivate_trace(eNB_mme_desc_t *eNB_desc_p,
//                                      sctp_queue_item_t *packet_p,
//                                      struct s1ap_message_s *message_p);
int s1ap_eNB_handle_deactivate_trace(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     struct s1ap_message_s *message_p);

#endif /* S1AP_ENB_TRACE_H_ */
