/*
 * am test: test function check_t_reordering,
 *               case 'update VR(MS) to first SN >= VR(X) for which not
 *                     all PDU segments have been received'
 * eNB sends 3 PDUs, first not received, two others received
 * later on, everything is received
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 10,
    UE_RECV_FAILS, 1,
TIME, 2,
    UE_RECV_FAILS, 0,
    ENB_SDU, 1, 10,
TIME, 3,
    ENB_SDU, 2, 10,
TIME, -1
