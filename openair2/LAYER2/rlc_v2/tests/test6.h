/*
 * rlc am test function segment_already_received
 *   eNB sends SDU [1..900], not received
 *   eNB retx with smaller PDUs [1..600] [601..900]
 *   [1..600] is received but ACK/NACK not
 *   eNB retx with still smaller PDUs [1..400] [401..600] [601..900]
 *   all is received, ACKs/NACKs go through
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_RECV_FAILS, 1,
    ENB_RECV_FAILS, 1,
    ENB_SDU, 0, 900,
TIME, 2,
    ENB_PDU_SIZE, 600,
    UE_RECV_FAILS, 0,
TIME, 48,
    UE_RECV_FAILS, 1,
    ENB_PDU_SIZE, 400,
TIME, 90,
    UE_RECV_FAILS, 0,
    ENB_RECV_FAILS, 0,
TIME, -1
