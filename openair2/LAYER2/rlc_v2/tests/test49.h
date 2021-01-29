/*
 * rlc am test ack/nack reporting with segmented SDUs
 *
 * 5 SDUs put into 6 PDUs, all PDUs received expect the 3rd. Then
 * retransmission happens with smaller PDU size.
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 60,
    ENB_SDU, 1, 160,
    ENB_SDU, 2, 130,
    ENB_SDU, 3, 120,
    ENB_SDU, 4, 100,
    ENB_PDU_SIZE, 100,
    UE_PDU_SIZE, 1000,
TIME, 3,
    UE_RECV_FAILS, 1,
TIME, 4,
    UE_RECV_FAILS, 0,
TIME, 7,
    ENB_PDU_SIZE, 40,
TIME, 42,
    UE_RECV_FAILS, 1,
TIME, 80,
    UE_RECV_FAILS, 0,
TIME, -1
