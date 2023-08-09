/*
 * rlc am test max_retx_reached
 * eNB sends PDU, never received
 */
TIME, 1,
    MUST_FAIL,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_RECV_FAILS, 1,
    ENB_RECV_FAILS, 1,
    ENB_SDU, 0, 10,
TIME, -1
