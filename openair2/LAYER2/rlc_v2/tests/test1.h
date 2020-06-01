/*
 * basic am test:
 * at time 1, eNB receives an SDU of 10 bytes
 * at time 10, UE receives an SDU of 5 bytes
 */

TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 10,
    UE_BUFFER_STATUS,
TIME, 10,
    UE_SDU, 0, 5,
TIME, -1
