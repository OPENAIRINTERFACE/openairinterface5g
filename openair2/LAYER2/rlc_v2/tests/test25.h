/*
 * am test: reject SDU because not enough room in rx buffer
 */
TIME, 1,
    ENB_AM, 10, 10, 35, 0, 45, -1, -1, 4,
    UE_AM, 10, 10, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 50,
TIME, -1
