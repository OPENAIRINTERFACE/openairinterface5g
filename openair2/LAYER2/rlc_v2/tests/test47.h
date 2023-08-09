/*
 * am: test function segment_nack_size, case "if (count == 0) return 12;"
 * The eNB sends 5 PDUs, only first and last received by the UE. Then we
 * ask for the UE's buffer status to trigger the case.
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_RECV_FAILS, 1,
    ENB_PDU_SIZE, 100,
    UE_PDU_SIZE, 100,
TIME, 2,
    ENB_SDU, 0, 10,
TIME, 3,
    UE_RECV_FAILS, 1,
    ENB_SDU, 1, 10,
TIME, 4,
    ENB_SDU, 2, 10,
TIME, 5,
    ENB_SDU, 3, 10,
TIME, 6,
    UE_RECV_FAILS, 0,
    ENB_RECV_FAILS, 0,
    ENB_SDU, 4, 10,
TIME, 42,
    UE_BUFFER_STATUS,
TIME, -1
