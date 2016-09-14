//WARNING: Do not include this header directly. Use intertask_interface.h instead.

MESSAGE_DEF(SCTP_INIT_MSG,          MESSAGE_PRIORITY_MED, SctpInit,                 sctpInit)
MESSAGE_DEF(SCTP_DATA_REQ,          MESSAGE_PRIORITY_MED, sctp_data_req_t,          sctp_data_req)
MESSAGE_DEF(SCTP_DATA_IND,          MESSAGE_PRIORITY_MED, sctp_data_ind_t,          sctp_data_ind)
MESSAGE_DEF(SCTP_NEW_ASSOCIATION,   MESSAGE_PRIORITY_MAX, sctp_new_peer_t,          sctp_new_peer)
MESSAGE_DEF(SCTP_CLOSE_ASSOCIATION, MESSAGE_PRIORITY_MAX, sctp_close_association_t, sctp_close_association)
