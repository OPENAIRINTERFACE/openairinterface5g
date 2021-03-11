/*
 * rlc am test function rlc_am_segment_full
 *   eNB sends SDU [1..900], not received
 *   eNB retx with smaller PDUs [1..600] [601..900]
 *   nothing received
 *   eNB retx with still smaller PDUs [1..400] [401..600] [601..900]
 *   [401..600] received, ACK goes through
 *   link clean, all goes through
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_RECV_FAILS, 1,
    ENB_RECV_FAILS, 1,
    ENB_SDU, 0, 900,
TIME, 2,
    ENB_PDU_SIZE, 600,
TIME, 48,
    ENB_PDU_SIZE, 400,
TIME, 95,
    UE_RECV_FAILS, 0,
    ENB_RECV_FAILS, 0,
TIME, -1
