/*
 * basic um test (SN field size 19, bad size, test must fail):
 * at time 1, gNB receives an SDU of 10 bytes
 * at time 10, UE receives an SDU of 5 bytes
 */

TIME, 1,
    MUST_FAIL,
    GNB_UM, 100000, 100000, 35, 19,
    UE_UM, 100000, 100000, 35, 19,
    GNB_SDU, 0, 10,
    UE_BUFFER_STATUS,
TIME, 10,
    UE_SDU, 0, 5,
TIME, -1
