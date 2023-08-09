/*
 * um: discard PDU because rx buffer full
 * eNB sends a PDU too big
 */
TIME, 1,
    ENB_UM, 100000, 100000, 35, 5,
    UE_UM, 10, 10, 35, 5,
    ENB_SDU, 0, 40,
TIME, -1
