/*
 * basic am test:
 * at time 1, eNB receives an SDU of 16001 bytes
 */

TIME, 1,
    MUST_FAIL,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 16001,
TIME, -1
