/*
 * rlc am test ack/nack reporting with segmented SDUs
 *   eNB sends SDU in big PDU, not received
 *   eNB retx with smaller PDUs, not received except a few
 *   after some time, RX works again, UE should NACK with SOstart/SOend
 *
 * this test was written because in the past we didn't generate NACKs with
 * SOstart/SOend, potentially leading to infinite retransmissions, even with
 * good radio conditions (well, there will be max ReTX reached at some point).
 * The scenario is as follows:
 * the eNB transmits many many PDUs segments and asks for status
 * report at some point (but before sending all the segments). The UE then
 * generates a (wrong) NACK without SOstart/SOend, and then the eNB has to
 * resend everything from start (all bytes of the PDU have been NACKed because
 * there is no SOstart/SOend). Then the eNB will again ask for status report
 * before sending everything and the process repeats all over again.
 */
TIME, 1,
    ENB_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_AM, 100000, 100000, 35, 0, 45, -1, -1, 4,
    UE_RECV_FAILS, 1,
    ENB_RECV_FAILS, 1,
    ENB_SDU, 0, 300,
    ENB_PDU_SIZE, 500,
    UE_PDU_SIZE, 1000,
TIME, 2,
    ENB_PDU_SIZE, 7,
TIME, 50,
    UE_RECV_FAILS, 0,
TIME, 51,
    UE_RECV_FAILS, 1,
TIME, 60,
    UE_RECV_FAILS, 0,
TIME, 61,
    UE_RECV_FAILS, 1,
TIME, 70,
    UE_RECV_FAILS, 0,
TIME, 71,
    UE_RECV_FAILS, 1,
TIME, 80,
    UE_RECV_FAILS, 0,
TIME, 81,
    UE_RECV_FAILS, 1,
TIME, 140,
    UE_RECV_FAILS, 0,
    ENB_RECV_FAILS, 0,
TIME, 150,
    ENB_PDU_SIZE, 200,
TIME, -1
