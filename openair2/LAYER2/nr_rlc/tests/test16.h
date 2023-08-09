/*
 * am test (SN field size 18):
 * there was a bug when we receive a full PDU after receiving only the
 * beginning of it; the data was copied at the end but from the start of the
 * full PDU instead of the correct offset. This test captures this case.
 * Gnb sends the start of a PDU then the full PDU. That is the SDU is
 * [00 .. 09]. First gnb sends [00 .. 08] then it sends [00 .. 09].
 */

TIME, 1,
    GNB_AM, 100000, 100000, 45, 35, 0, -1, -1, 8, 18,
    UE_AM, 100000, 100000, 45, 35, 0, -1, -1, 8, 18,
    GNB_PDU_SIZE, 12,
    UE_PDU_SIZE, 20,
    GNB_PDU, 12, 0x90, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
TIME, 2,
    GNB_PDU_SIZE, 20,
    GNB_PDU, 13, 0xc0, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
TIME, -1
