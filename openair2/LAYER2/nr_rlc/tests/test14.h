/*
 * am test (SN field size 18):
 * test status reporting with missing packet
 * at time 1, UE receives an SDU of 50 bytes
 * at time 2, UE receives an SDU of 50 bytes
 * at time 3, UE receives an SDU of 50 bytes
 * at time 4, UE receives an SDU of 50 bytes
 * transmission fails for first SDU, okay for the others
 * then some time with transmission failing
 * then at time 49, UE receives an SDU, transmission okay
 * then at time 50, transmission failing again (we had a bug triggered by this)
 * finally at time 55, transmission okay again to clear buffers
 */

TIME, 1,
    GNB_AM, 100000, 100000, 45, 35, 0, -1, -1, 8, 18,
    UE_AM, 100000, 100000, 45, 35, 0, -1, -1, 8, 18,
    GNB_RECV_FAILS, 1,
    GNB_PDU_SIZE, 80,
    UE_PDU_SIZE, 80,
    UE_SDU, 0, 50,
TIME, 2,
    GNB_RECV_FAILS, 0,
    UE_SDU, 1, 50,
TIME, 3,
    GNB_BUFFER_STATUS,
    UE_SDU, 2, 50,
TIME, 4,
    GNB_BUFFER_STATUS,
    UE_SDU, 3, 50,
TIME, 5,
    GNB_BUFFER_STATUS,
    GNB_RECV_FAILS, 1,
TIME, 6,
    GNB_BUFFER_STATUS,
    UE_BUFFER_STATUS,
TIME, 49,
    GNB_RECV_FAILS, 0,
    UE_SDU, 4, 50,
TIME, 50,
    GNB_RECV_FAILS, 1,
    GNB_BUFFER_STATUS,
    UE_BUFFER_STATUS,
TIME, 55,
    GNB_RECV_FAILS, 0,
TIME, -1
