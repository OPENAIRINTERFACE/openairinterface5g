/*
 * am test: ask for retx with TX buffer too small
 * then ask for status with buffer too small
 */

TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 100,
    UE_RECV_FAILS, 1,
TIME, 47,
    ENB_PDU_SIZE, 4,
    ENB_BUFFER_STATUS,
    UE_BUFFER_STATUS,
TIME, 48,
    ENB_PDU_SIZE, 1000,
    UE_PDU_SIZE, 1,
    UE_BUFFER_STATUS,
    UE_RECV_FAILS, 0,
TIME, 49,
    UE_BUFFER_STATUS,
TIME, 50,
    UE_PDU_SIZE, 1000,
    UE_BUFFER_STATUS,
TIME, -1
