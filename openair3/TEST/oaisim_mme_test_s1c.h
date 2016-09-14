typedef struct s1c_test_s {
    int     scenario_index;
    int     tx_next_message_index;
    int     rx_next_message_index;
    int32_t assoc_id;
}s1c_test_t;

void     mme_test_s1_start_test(instance_t instance);
void     mme_test_s1_notify_sctp_data_ind(uint32_t assoc_id, int32_t stream, const uint8_t * const data, const uint32_t data_length);
