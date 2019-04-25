/*
 * test rlc am simulate rx pdu buffer full
 * eNB sends too big PDU to UE, rejected because buffer full
 */
TIME, 1,
    MUST_FAIL,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 10, 10, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 10,
TIME, -1
