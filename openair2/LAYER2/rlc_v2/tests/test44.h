/*
 * am: test function rlc_entity_am_reestablishment
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    RE_ESTABLISH,
TIME, 2,
    ENB_SDU, 0, 10,
    RE_ESTABLISH,
TIME, 3,
    ENB_SDU, 0, 40,
    ENB_PDU_SIZE, 14,
    UE_RECV_FAILS, 1,
TIME, 4,
    UE_RECV_FAILS, 0,
TIME, 10,
    RE_ESTABLISH,
TIME, -1

