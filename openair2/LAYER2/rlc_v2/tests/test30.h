/*
 * am test: test function generate_status
 *               enter the while loop 'go to highest full sn+1 for ACK'
 * eNB sends several PDUs, only the last is received
 * UE sends status PDU of a chosen size that let the code enter the while
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    ENB_SDU, 0, 70,
    ENB_PDU_SIZE, 12,
    UE_RECV_FAILS, 1,
TIME, 7,
    UE_RECV_FAILS, 0,
    UE_PDU_SIZE, 12,
TIME, -1
