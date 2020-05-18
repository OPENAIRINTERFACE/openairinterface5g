/*
 * am test: test function check_t_poll_retransmit
 *               case 'do we meet conditions of 36.322 5.2.2.3?'
 * eNB sends one PDU, UE does not receive
 * just before calling check_t_poll_retransmit, eNB receives a new SDU
 * for the function 'check_poll_after_pdu_assembly' to fail
 * then UE receives all what eNB sends
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 10,
    UE_RECV_FAILS, 1,
TIME, 47,
    ENB_SDU, 1, 10,
    UE_RECV_FAILS, 0,
TIME, -1
