#ifndef SCTP_ITTI_MESSAGING_H_
#define SCTP_ITTI_MESSAGING_H_

int sctp_itti_send_new_message_ind(task_id_t task_id, uint32_t assoc_id, uint8_t *buffer,
                                   uint32_t buffer_length, uint16_t stream);

int sctp_itti_send_association_resp(task_id_t task_id, instance_t instance,
                                    int32_t assoc_id,
                                    uint16_t cnx_id, enum sctp_state_e state,
                                    uint16_t out_streams, uint16_t in_streams);

int sctp_itti_send_association_ind(task_id_t task_id, instance_t instance,
                                   int32_t assoc_id, uint16_t port,
                                   uint16_t out_streams, uint16_t in_streams);


#endif /* SCTP_ITTI_MESSAGING_H_ */
