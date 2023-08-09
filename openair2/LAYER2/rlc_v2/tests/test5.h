/*
 * basic um test: UE field length 10 bits
 * at time 1, eNB receives an SDU of 10 bytes
 * at time 10, UE receives an SDU of 5 bytes
 */

TIME, 1,
    ENB_UM, 100000, 100000, 35, 10,
    UE_UM, 100000, 100000, 35, 10,
    ENB_SDU, 0, 10,
TIME, 10,
    UE_SDU, 0, 5,
TIME, -1
