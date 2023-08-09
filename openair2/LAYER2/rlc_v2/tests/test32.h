/*
 * um test: several SDUs in a PDU (field length 10 bits)
 */
TIME, 1,
    ENB_UM, 100000, 100000, 35, 10,
    UE_UM, 100000, 100000, 35, 10,
    ENB_SDU, 0, 10,
    ENB_SDU, 1, 20,
    ENB_SDU, 2, 30,
TIME, -1
