/*
 * um: some wrong PDUs
 */
TIME, 1,
    ENB_UM, 100000, 100000, 35, 5,
    UE_UM, 100000, 100000, 35, 5,
    /* LI == 0
     *  rlc_pdu_encoder_put_bits(&e, 0, 2);    // FI
     *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 0, 5);    // SN
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 0, 11);   // LI
     */
    ENB_PDU, 3, 0x20, 0x00, 0x00,
    /* no data
     *  rlc_pdu_encoder_put_bits(&e, 0, 2);    // FI
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 0, 5);    // SN
     */
    ENB_PDU, 1, 0x00,
    /* LI == 2 >= data_size == 1
     *  rlc_pdu_encoder_put_bits(&e, 0, 2);    // FI
     *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 0, 5);    // SN
     *  rlc_pdu_encoder_put_bits(&e, 0, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 2, 11);   // LI
     *  rlc_pdu_encoder_align(&e);
     *  rlc_pdu_encoder_put_bits(&e, 0, 8);    // 1 byte of data
     */
    ENB_PDU, 4, 0x20, 0x00, 0x20, 0x00,
    /* PDU with E == 1 but has size 1 byte only (truncated PDU)
     *  rlc_pdu_encoder_put_bits(&e, 0, 2);    // FI
     *  rlc_pdu_encoder_put_bits(&e, 1, 1);    // E
     *  rlc_pdu_encoder_put_bits(&e, 0, 5);    // SN
     */
    ENB_PDU, 1, 0x20,
TIME, -1
