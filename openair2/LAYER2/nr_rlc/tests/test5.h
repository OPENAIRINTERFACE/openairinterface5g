/*
 * basic am test (SN field size 11, not exiting, test must fail):
 * at time 1, gNB receives an SDU of 10 bytes
 * at time 10, UE receives an SDU of 5 bytes
 */

TIME, 1,
    MUST_FAIL,
    GNB_AM, 100000, 100000, 45, 35, 0, -1, -1, 8, 11,
    UE_AM, 100000, 100000, 45, 35, 0, -1, -1, 8, 11,
    GNB_SDU, 0, 10,
    UE_BUFFER_STATUS,
TIME, 10,
    UE_SDU, 0, 5,
TIME, -1
