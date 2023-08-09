/*
 * rlc am test big SDU (size > 2047)
 * first generate SDU with exactly 2047 bytes
 * later on generate SDU with exactly 2048 bytes
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 20,
    ENB_SDU, 1, 2047,
    ENB_SDU, 2, 20,
    ENB_PDU_SIZE, 2200,
TIME, 10,
    ENB_SDU, 3, 20,
    ENB_SDU, 4, 2048,
    ENB_SDU, 5, 20,
TIME, -1

