/*
 * am test: basic test with poll_pdu == 2
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, 2, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, 2, -1, 4,
    ENB_SDU, 0, 50,
    ENB_PDU_SIZE, 10,
TIME, -1
