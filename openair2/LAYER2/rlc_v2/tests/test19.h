/*
 * test rlc am bad PDU
 * eNB sends custom PDUs to UE, all of them are wrong for a reason or another
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    /* data PDU, LI == 0
     *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // D/C
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // RF
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // P
     *  rlc_pdu_encoder_put_bits(&e, 0, 2);    // FI
     *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 0, 10);   // SN
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 0, 11);   // LI
     */
    ENB_PDU, 4, 0x84, 0x00, 0x00, 0x00,
    /* data PDU, no data
     *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // D/C
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // RF
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // P
     *  rlc_pdu_encoder_put_bits(&e, 0, 2);    // FI
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 0, 10);   // SN
     */
    ENB_PDU, 2, 0x80, 0x00,
    /* data PDU, LI == 2 > data size == 1
     *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // D/C
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // RF
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // P
     *  rlc_pdu_encoder_put_bits(&e, 0, 2);    // FI
     *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 0, 10);   // SN
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 2, 11);   // LI
     *  rlc_pdu_encoder_align(&e);
     *  rlc_pdu_encoder_put_bits(&e, 0, 8);    // 1 byte of data
     */
    ENB_PDU, 5, 0x84, 0x00, 0x00, 0x20, 0x00,
    /* control PDU, CPT != 0
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // D/C
     *  rlc_pdu_encoder_put_bits(&e, 2, 3);    // CPT
     */
    ENB_PDU, 1, 0x20,
    /* data PDU, but only 1 byte
     *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // D/C
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // RF
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // P
     *  rlc_pdu_encoder_put_bits(&e, 0, 2);    // FI
     *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // E
     */
    ENB_PDU, 1, 0x84,
TIME, -1
