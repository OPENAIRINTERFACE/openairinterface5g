/*
 * um test (SN field size 12):
 * test reassembly
 * at time 1, gNB receives an SDU of 50 bytes
 * sends it in 3 parts
 */

TIME, 1,
    GNB_UM, 100000, 100000, 35, 12,
    UE_UM, 100000, 100000, 35, 12,
    GNB_PDU_SIZE, 22,
    GNB_SDU, 0, 50,
TIME, -1
