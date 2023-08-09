/*
 * basic am test:
 * at time 1, eNB receives 10 SDUs of 10 bytes
 */

TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 10,
    ENB_SDU, 1, 10,
    ENB_SDU, 2, 10,
    ENB_SDU, 3, 10,
    ENB_SDU, 4, 10,
    ENB_SDU, 5, 10,
    ENB_SDU, 6, 10,
    ENB_SDU, 7, 10,
    ENB_SDU, 8, 10,
    ENB_SDU, 9, 10,
TIME, -1
