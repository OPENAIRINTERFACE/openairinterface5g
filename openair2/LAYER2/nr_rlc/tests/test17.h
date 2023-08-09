/*
 * um test (SN field size 12):
 * same problem as for test16, but we test by sending [00 .. 08] then
 * [02 .. 09] (they overlap, the full SDU is [00 .. 09]), which probably
 * never occurs in practice but triggers the bug. Doing as for test16 does
 * not trigger the bug.
 */

TIME, 1,
    GNB_UM, 100000, 100000, 35, 12,
    UE_UM, 100000, 100000, 35, 12,
    GNB_PDU_SIZE, 8,
    UE_PDU_SIZE, 20,
    GNB_PDU, 11, 0x40, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
TIME, 2,
    GNB_PDU_SIZE, 20,
    GNB_PDU, 12, 0x80, 0x00, 0x00, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
TIME, -1
