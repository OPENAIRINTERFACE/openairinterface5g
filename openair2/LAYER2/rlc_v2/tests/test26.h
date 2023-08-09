/*
 * am test: test function check_t_poll_retransmit
 *               case 'PDU with SN = VT(S)-1 not found?'
 * eNB sends some PDUs, UE receives none
 * then UE receives the first retransmitted PDU and nothing more
 * until poll retransmit occurs again in the eNB to trigger the case
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 10,
    UE_RECV_FAILS, 1,
TIME, 2,
    ENB_SDU, 1, 10,
TIME, 3,
    ENB_SDU, 2, 10,
TIME, 4,
    ENB_SDU, 3, 10,
TIME, 50,
    UE_RECV_FAILS, 0,
TIME, 51,
    UE_RECV_FAILS, 1,
TIME, 100,
    UE_RECV_FAILS, 0,
TIME, -1
