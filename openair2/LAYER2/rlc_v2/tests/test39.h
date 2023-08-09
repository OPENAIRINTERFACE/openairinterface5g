/*
 * um: SDU too big
 */
TIME, 1,
    MUST_FAIL,
    ENB_UM, 10, 10, 35, 5,
    UE_UM, 100, 100, 35, 5,
    ENB_SDU, 0, 16001,
TIME, -1
