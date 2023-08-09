/*
 * rlc am test resegmentation of PDU segment with several SDUs
 *   eNB sends 3 SDUs [1..10] [11.20] [21..30], not received
 *   eNB retx with smaller PDUs, not received
 *   eNB retx with still smaller PDUs, not received
 *   then reception on, all passes
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_RECV_FAILS, 1,
    ENB_RECV_FAILS, 1,
    ENB_SDU, 0, 10,
    ENB_SDU, 1, 10,
    ENB_SDU, 2, 10,
TIME, 2,
    ENB_PDU_SIZE, 25,
TIME, 48,
    ENB_PDU_SIZE, 15,
TIME, 100,
    UE_RECV_FAILS, 0,
    ENB_RECV_FAILS, 0,
TIME, -1
