/*
 * am test (SN field size 18):
 * test reassembly
 * at time 1, gNB receives an SDU of 50 bytes
 * sends 1st part, received by UE
 * then sends 2nd part, not received
 * then sends 3rd part, received
 * then UE receives all
 */

TIME, 1,
    GNB_AM, 100000, 100000, 45, 35, 0, -1, -1, 8, 18,
    UE_AM, 100000, 100000, 45, 35, 0, -1, -1, 8, 18,
    GNB_PDU_SIZE, 22,
    GNB_SDU, 0, 50,
TIME, 2,
    UE_RECV_FAILS, 1,
TIME, 3,
    UE_RECV_FAILS, 0,
TIME, -1
