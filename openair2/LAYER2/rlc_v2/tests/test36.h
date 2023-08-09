/*
 * um: discard according to 36.322 5.1.2.2.2
 * eNB sends many PDUs. 1st is received, then not, then again.
 */
TIME, 1,
    ENB_UM, 100000, 100000, 35, 5,
    UE_UM, 100000, 100000, 35, 5,
    ENB_SDU, 0, 33,
    ENB_PDU_SIZE, 2,
TIME, 2,
    UE_RECV_FAILS, 1,
TIME, 22,
    UE_RECV_FAILS, 0,
TIME, -1
