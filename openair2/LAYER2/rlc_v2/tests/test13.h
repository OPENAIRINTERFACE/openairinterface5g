/*
 * rlc am test function process_received_ack with something in
 *             the retransmit_list to put in the ack_list
 * eNB sends 4 PDUs, not received
 * eNB retransmits 4th PDU, received, ACKed with NACKs for PDU 1, 2, 3
 * UE receives custom PDU for 1, 2, 3, 4 (they are not sent by eNB)
 * (4 resent to have the P bit set)
 * UE sends ACK for all, eNB puts from retransmit_list to ack_list
 *
 * Maybe not very realistic (custom PDUs).
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_RECV_FAILS, 1,
    ENB_RECV_FAILS, 1,
    ENB_PDU_SIZE, 12,
    ENB_SDU, 0, 10,
    ENB_SDU, 1, 10,
    ENB_SDU, 2, 10,
    ENB_SDU, 3, 10,
TIME, 10,
    UE_RECV_FAILS, 0,
    ENB_RECV_FAILS, 0,
TIME, 87,
    ENB_PDU, 12, 0x80, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    ENB_PDU, 12, 0x80, 0x01, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
    ENB_PDU, 12, 0x80, 0x02, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
    ENB_PDU, 12, 0xa0, 0x03, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
TIME, -1
