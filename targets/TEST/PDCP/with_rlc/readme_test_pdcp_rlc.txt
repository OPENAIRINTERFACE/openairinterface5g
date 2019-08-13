#----------------------------------------------------------------------------------------------------------------
The aim of the test code test_pdcp_rlc.c is primarily to test a PDCP protocol entity integrated with RLC layer.
This is a first step, in near future, this test may be extended with an other type of RLC (AM), and may be
integrated with the upcoming traffic generator.
#----------------------------------------------------------------------------------------------------------------
The code implements the following figure:
#----------------------------------------------------------------------------------------------------------------


   +-+-+-+-+              +-+-+-+-+
   | PDCP1 |              | PDCP2 |
   +-+-+-+-+              +-+-+-+-+
       |                      |
   +-+-+-+-+              +-+-+-+-+
   |  RLC1 |              |  RLC2 |
   |   UM  |              |   UM  |
   +-+-+-+-+              +-+-+-+-+
       |                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         MAC LAYER EMULATION   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

#----------------------------------------------------------------------------------------------------------------
Compilation of the code:
#----------------------------------------------------------------------------------------------------------------

make -f Makefile.data_bearer


#----------------------------------------------------------------------------------------------------------------
Execution of the code:
#----------------------------------------------------------------------------------------------------------------

./test_pdcp_rlc

#----------------------------------------------------------------------------------------------------------------
What Could Be Frequently Asked Questions:
#----------------------------------------------------------------------------------------------------------------

#-------------------
Q:How to send a SDU?
A:
pdcp_rlc_test_send_sdu(0, 1, 2); // prototype pdcp_rlc_test_send_sdu(module_id_t module_idP, rb_id_t rab_idP, int sdu_indexP)

#-------------------
Q:How to fix the bitrate of the radio bearer?
A:
pdcp_rlc_test_exchange_pdus(&rlc[0].m_rlc_um_array[rlc[0].m_rlc_pointer[1].rlc_index],
                            &rlc[1].m_rlc_um_array[rlc[1].m_rlc_pointer[1].rlc_index],
                            1000,
                            1000);

Prototype:
pdcp_rlc_test_exchange_pdus(rlc_um_entity_t *um1P,      // RLC UM instance 1
                            rlc_um_entity_t *um2P,      // RLC UM instance 2
                            u16_t           bytes_tx1P, // tx bytes requested at RLC UM instance 1 and sent to RLC UM instance 2
                            u16_t           bytes_tx2P) // tx bytes requested at RLC UM instance 2 and sent to RLC UM instance 1


