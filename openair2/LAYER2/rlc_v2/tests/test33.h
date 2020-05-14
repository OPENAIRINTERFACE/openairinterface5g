/*
 * um test: test function rlc_um_reassemble_pdu, discard SDU
 *               case '!(fi & 0x02'
 * eNB sends 33 PDUs covering 1 SDU, only PDU 0 received (with SN=0 and FI=1)
 * then eNB sends 1 PDU covering 1 SDU (so SN=1 and FI=0 for this one)
 * received by UE
 */
TIME, 1,
    ENB_UM, 100000, 100000, 35, 5,
    UE_UM, 100000, 100000, 35, 5,
    ENB_SDU, 0, 33,
    ENB_PDU_SIZE, 2,
TIME, 2,
    UE_RECV_FAILS, 1,
TIME, 34,
    UE_RECV_FAILS, 0,
    ENB_SDU, 1, 1,
TIME, -1
