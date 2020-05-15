/*
 * rlc am test function rlc_am_reassemble_next_segment
 *        in r->pdu_byte >= r->so + (r->sdu_offset - r->start->data_offset)
 *                                + r->sdu_len
 *        when case 'if (r->e)' is false
 *   eNB sends 3 SDUs [1..10] [11.20] [21..30], not received
 *   eNB retx with smaller PDUs, not received
 *   eNB retx with still smaller PDUs, not received
 *   then UE reception on
 *   then custom PDUs, first a small part of head of original PDU
 *                     then a bigger part, covering the first part
 *                     so that the beginning of this part triggers the 'while'
 *   then eNB reception on, all passes
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
TIME, 95,
    ENB_BUFFER_STATUS,
TIME, 99,
    UE_RECV_FAILS, 0,
    ENB_PDU, 14, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    ENB_PDU, 25, 0xec, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
TIME, 100,
    ENB_RECV_FAILS, 0,
TIME, 134,
    UE_BUFFER_STATUS,
TIME, -1
