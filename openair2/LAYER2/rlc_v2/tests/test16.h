/*
 * rlc am test process_received_nack
 * Same events as for test15 except the fake control PDU
 * does not ACK anything (ack_sn = 0) so that PDU in the
 * wait_list are not transfered into the ack_list and
 * we cover the case:
 *    } else {
 *      prev = cur;
 *      cur = cur->next;
 *    }
 * for the wait_list case.
 *
 *  code to generate fake control PDU:
 *  rlc_pdu_encoder_init(&e, out, 100);
 *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // D/C
 *  rlc_pdu_encoder_put_bits(&e, 0, 3);    // CPT
 *  rlc_pdu_encoder_put_bits(&e, 0, 10);   // ack_sn
 *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // e1
 *  rlc_pdu_encoder_put_bits(&e, 1, 10);   // nack_sn
 *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // e1
 *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // e2
 *  rlc_pdu_encoder_put_bits(&e, 14, 15);  // so_start
 *  rlc_pdu_encoder_put_bits(&e, 16, 15);  // so_end
 *  rlc_pdu_encoder_align(&e);
 */

TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 8,
    ENB_RECV_FAILS, 1,
TIME, 2,
    UE_RECV_FAILS, 1,
    ENB_SDU, 1, 30,
TIME, 20,
    ENB_PDU_SIZE, 14,
TIME, 48,
    UE_RECV_FAILS, 0,
TIME, 49,
    UE_RECV_FAILS, 1,
TIME, 50,
    UE_RECV_FAILS, 0,
TIME, 60,
    ENB_RECV_FAILS, 0,
    UE_PDU, 8, 0x00, 0x02, 0x00, 0xa0, 0x03, 0x80, 0x08, 0x00,
TIME, 70,
    UE_RECV_FAILS, 0,
TIME, -1
