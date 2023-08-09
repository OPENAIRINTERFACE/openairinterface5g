/*
 * am test (SN field size 18):
 * test "range" in NACK, generate a case where so_start > so_end
 * (so so_start and so_end are not from the same PDU)
 */

TIME, 1,
    GNB_AM, 100000, 100000, 45, 35, 0, -1, -1, 8, 18,
    UE_AM, 100000, 100000, 45, 35, 0, -1, -1, 8, 18,
    GNB_PDU_SIZE, 40,
    UE_PDU_SIZE, 80,
    GNB_SDU, 0, 50,
    GNB_SDU, 1, 50,
    GNB_SDU, 2, 50,
TIME, 2,
    UE_RECV_FAILS, 1,
TIME, 4,
    UE_RECV_FAILS, 0,
TIME, -1
