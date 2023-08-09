/*
 * basic um test (SN field size 6):
 * at time 1, gNB receives an SDU of 10 bytes
 * at time 10, UE receives an SDU of 5 bytes
 */

TIME, 1,
    GNB_UM, 100000, 100000, 35, 6,
    UE_UM, 100000, 100000, 35, 6,
    GNB_SDU, 0, 10,
    UE_BUFFER_STATUS,
TIME, 10,
    UE_SDU, 0, 5,
TIME, -1
