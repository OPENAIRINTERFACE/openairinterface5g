/*
 * um: test some cases of functions tx_pdu_size and rlc_entity_um_generate_pdu
 * eNB has too much data to fit in one PDU
 * then later eNB wants to send an SDU of size > 2047
 * then later eNB sends several SDUs in one PDU
 */
TIME, 1,
    ENB_UM, 100000, 100000, 35, 5,
    UE_UM, 100000, 100000, 35, 5,
    ENB_PDU_SIZE, 2050,
    ENB_SDU, 0, 1500,
    ENB_SDU, 1, 1500,
    ENB_SDU, 2, 10,
TIME, 10,
    ENB_SDU, 3, 2048,
    ENB_SDU, 4, 10,
TIME, 20,
    ENB_SDU, 5, 10,
    ENB_SDU, 6, 10,
    ENB_SDU, 7, 10,
    ENB_SDU, 8, 10,
TIME, -1
