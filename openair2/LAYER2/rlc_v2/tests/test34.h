/*
 * um test: trigger some cases in rlc_um_reception_actions
 * eNB sends several PDUs, only the beginning PDUs and ending PDUs are
 * received. Middle PDUs are not.
 */
TIME, 1,
    ENB_UM, 100000, 100000, 35, 5,
    UE_UM, 100000, 100000, 35, 5,
    ENB_SDU, 0, 40,
    ENB_PDU_SIZE, 2,
TIME, 2,
    UE_RECV_FAILS, 1,
TIME, 8,
    UE_RECV_FAILS, 0,
TIME, -1
