/*
 * rlc am test function rlc_am_reassemble_next_segment
 *        case 'if pdu_byte is not in [so .. so+len-1]'
 *   eNB sends SDU [1..30], not received
 *   eNB retx with smaller PDUs [1..21] [22..30], not received
 *   eNB retx with still smaller PDUs [1..11] [12..21] [22..30], not received
 *   custom PDU [12..21] sent to UE, received
 *   custom PDU [1..21] sent to UE, received
 *
 * Not sure if in a real setup [12..21] is sent and then [1..21] is sent.
 * In the current RLC implementation, this is impossible. If we send [12..21]
 * it means [1..21] has been split and so we won't sent it later on.
 * Maybe with HARQ retransmissions in PHY/MAC in bad radio conditions?
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_RECV_FAILS, 1,
    ENB_RECV_FAILS, 1,
    ENB_SDU, 0, 30,
TIME, 2,
    ENB_PDU_SIZE, 25,
TIME, 48,
    ENB_PDU_SIZE, 15,
TIME, 100,
    UE_RECV_FAILS, 0,
    ENB_RECV_FAILS, 0,
    ENB_PDU, 14, 0xd8, 0x00, 0x00, 0x0b, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
TIME, 101,
    ENB_PDU, 25, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
TIME, -1
