/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*
 * PDCP test code
 *
 * Author: Baris Demiray <baris.demiray@eurecom.fr>
 */

#include "MAC_INTERFACE/vars.h"
#include "LAYER2/MAC/vars.h"
#include "UTIL/LOG/log_if.h"
#include "test_util.h"
#include "test_pdcp.h"

/*
 * These are the PDCP entities that will be utilised
 * throughout the test
 *
 * For pdcp_data_req() and pdcp_data_ind() these
 * are passed and used
 */
pdcp_t pdcp_array[2];

/*
 * TX list is the one we use to receive PDUs created by pdcp_data_req() and
 * RX list is the one we use to pass these PDUs to pdcp_data_ind(). In test_pdcp_data_req()
 * method every PDU created by pdcp_data_req() are first validated and then added to RX
 * list after it's removed from TX list
 */
list_t test_pdu_tx_list;
list_t test_pdu_rx_list;

/*
 * This is used by a number of methods to determine how
 * many times a packet should be generated/a test run
 */
#define NUMBER_OF_TEST_PACKETS 10000
/*
 * This is determined by the size of Sequence Number
 * field and used to check sequence number synchronisation
 */
#define WINDOW_SIZE 4096       // 12-bit SN

/*
 * Test configuration is STATEFUL. If you want to run DATA_REQUEST after TX_WINDOW
 * then you have to reset/reinitialize PDCP entity state!
 *
 *  TEST_RX_AND_TX_WINDOW Tests TX window code by repetitively asking for new TX
 *                        sequence numbers without generating any packets
 *                        And tests RX window code by supplying sequenced RX sequence
 *                        numbers and asking RX window code to validate, without
 *                        generating any packets
 */
#define TEST_RX_AND_TX_WINDOW 0
/*
 * TEST_PDCP_DATA_REQUEST_AND_INDICATION Tests pdcp_data_req() method by repetitively asking it
 *                                       to create a PDCP PDU out of supplied SDU and parsing
 *                                       particular fields of relevant PDU lateron
 *                                       Afterwards, those PDUs created by pdcp_data_req() are
 *                                       passed to pdcp_data_ind()
 */
#define TEST_PDCP_DATA_REQUEST_AND_INDICATION 1

/*
 * Buffer content used for testing purposes
 */
#define DUMMY_BUFFER ((unsigned char*)"123456789")
#define DUMMY_BUFFER_SIZE 10

int main(int argc, char **argv)
{
  unsigned char index = 0;
  unsigned char test_result = 0;

  /*
   * Initialize memory allocator, list_t for test PDUs, and log generator
   */
  pool_buffer_init();
  list_init(&test_pdu_tx_list, NULL);
  list_init(&test_pdu_rx_list, NULL);
  logInit();

  if (init_pdcp_entity(&pdcp_array[0]) == true && init_pdcp_entity(&pdcp_array[1]) == true)
    msg("[TEST] PDCP entity initialization OK\n");
  else {
    msg("[TEST] Cannot initialize PDCP entities!\n");
    return 1;
  }

  /* Initialize PDCP state variables */
  for (index = 0; index < 2; ++index) {
    if (pdcp_init_seq_numbers(&pdcp_array[index]) == false) {
      msg("[TEST] Cannot initialize sequence numbers of PDCP entity %d!\n", index);
      exit(1);
    } else {
      msg("[TEST] Sequence number state of PDCP entity %d is initialized\n", index);
    }
  }

#if TEST_RX_AND_TX_WINDOW

  /* Test TX window */
  if (test_tx_window() == false)
    test_result = 1;

  /* Test RX window */
  if (test_rx_window() == false)
    test_result = 1;

#endif

#if TEST_PDCP_DATA_REQUEST_AND_INDICATION

  /* Test pdcp_data_req() method in pdcp.c */
  if (test_pdcp_data_req() == false)
    test_result = 1;

  /* Test pdcp_data_ind() method in pdcp.c */
  if (test_pdcp_data_ind() == false)
    test_result = 1;

#endif

  if (test_result) {
    msg("\n\nOne or more tests failed!\n");
  } else {
    msg("\n\nAll tests are successfull!\n");
  }

  return test_result;
}

bool init_pdcp_entity(pdcp_t *pdcp_entity)
{
  if (pdcp_entity == NULL)
    return false;

  /*
   * Initialize sequence number state variables of relevant PDCP entity
   */
  pdcp_entity->next_pdcp_tx_sn = 0;
  pdcp_entity->next_pdcp_rx_sn = 0;
  pdcp_entity->tx_hfn = 0;
  pdcp_entity->rx_hfn = 0;
  /* SN of the last PDCP SDU delivered to upper layers */
  pdcp_entity->last_submitted_pdcp_rx_sn = 4095;
  pdcp_entity->seq_num_size = 12;

  msg("PDCP entity is initialized: Next TX: %d, Next Rx: %d, TX HFN: %d, RX HFN: %d, " \
      "Last Submitted RX: %d, Sequence Number Size: %d\n", pdcp_entity->next_pdcp_tx_sn, \
      pdcp_entity->next_pdcp_rx_sn, pdcp_entity->tx_hfn, pdcp_entity->rx_hfn, \
      pdcp_entity->last_submitted_pdcp_rx_sn, pdcp_entity->seq_num_size);

  return true;
}

bool test_tx_window(void)
{
  unsigned long index = 0;

  for (index = 0; index < NUMBER_OF_TEST_PACKETS; ++index) {
    u16 pseudo_tx_sn = pdcp_get_next_tx_seq_number(&pdcp_array[0]);

    if (pseudo_tx_sn == index % WINDOW_SIZE)
      msg("TX packet # %07lu seq # %04d hfn # %04d\n", index, pseudo_tx_sn, pdcp_array[0].tx_hfn);
    else {
      msg("TX packet is out-of-window!\n");
      return false;
    }
  }

  return true;
}

bool test_rx_window(void)
{
  unsigned long index = 0;

  for (index = 0; index < NUMBER_OF_TEST_PACKETS; ++index) {
    u16 pseudo_rx_sn = (index == 0) ? 0 : index % WINDOW_SIZE;

    if (pdcp_is_rx_seq_number_valid(pseudo_rx_sn, &pdcp_array[1]) == true) {
      msg("RX packet # %07lu seq # %04d last-submitted # %04d hfn # %04d\n", \
          index, pdcp_array[1].next_pdcp_rx_sn, pdcp_array[1].last_submitted_pdcp_rx_sn, pdcp_array[1].rx_hfn);
    } else {
      msg("RX packet seq # %04lu is not valid!\n", index);
      return false;
    }
  }

  return true;
}

bool test_pdcp_data_req(void)
{
  unsigned char* pdcp_test_pdu_buffer = NULL;
  unsigned char pdcp_test_pdu_buffer_size = DUMMY_BUFFER_SIZE + PDCP_USER_PLANE_DATA_PDU_LONG_SN_HEADER_SIZE;
  unsigned int index = 0;

  /*
   * Create an unsigned char buffer out of uint8_t
   */
  pdcp_test_pdu_buffer = (unsigned char*) calloc(1, pdcp_test_pdu_buffer_size);

  if (pdcp_test_pdu_buffer == NULL) {
    msg("Cannot allocate a buffer for test!\n");
    return false;
  }

  /*
   * Ask PDCP to handle a number of data requests
   */
  for (index = 0; index < NUMBER_OF_TEST_PACKETS; ++index) {
    msg("\n\nAsking PDCP to send %d/%d SDU...\n", index+1, NUMBER_OF_TEST_PACKETS);

    /*
     * Reset test pdu buffer for every run
     */
    memset(pdcp_test_pdu_buffer, 0x00, pdcp_test_pdu_buffer_size);

    /*
     * Ask PDCP to create a PDU with given buffer and enqueue it to `test_pdu_tx_list`
     */
    if (pdcp_data_req(0, 0, 10, DUMMY_BUFFER, &pdcp_array[0], &test_pdu_tx_list) == true) {
      msg("[TEST] Starting to dissect PDU created by PDCP...\n");

      /*
       * XXX uint8_t doesn't hold buffer size, how do we keep the size
       * information if we pass uint8_ts via a linked list?
       */

      if (pdcp_test_pdu_buffer_size == 0 ) {
        msg("[TEST] PDU created by pdcp_data_req() is invalid!\n");
        return false;
      }

      /*
       * Serialize incoming uint8_t into an unsigned character array
       * and add removed PDU to RX list in order to use it in the next test
       * (test_pdcp_data_ind())
       */
      uint8_t* pdcp_test_pdu = list_remove_head(&test_pdu_tx_list);
      memcpy(pdcp_test_pdu_buffer, pdcp_test_pdu->data, pdcp_test_pdu_buffer_size);
      list_add_tail_eurecom(pdcp_test_pdu, &test_pdu_rx_list);

      /*
       * Verify that this is a data packet by checking
       * if the first bit is 0x00 (PDCP_DATA_PDU)
       */
      if (pdcp_test_pdu_buffer[0] & 0x80) {
        msg("[TEST] First bit is not 0, which means this is not a Data PDU!\n");
        return false;
      } else {
        msg("[TEST] First bit is 0 so this is a Data PDU, OK\n");
      }

      /*
       * Verify that all three reserved bits are 0
       */
      if ((pdcp_test_pdu_buffer[0] & 0x70) != 0) {
        msg("[TEST] Reserved bits are not 0!\n");
        return false;
      } else {
        msg("[TEST] Reserved bits are all 0, OK\n");
      }

      /*
       * Parse and verify sequence number
       */
      u16 sequence_number = pdcp_get_sequence_number_of_pdu_with_long_sn(pdcp_test_pdu_buffer);
      msg("[TEST] Parsed sequence number is %04d\n", sequence_number);

      if (sequence_number != index % WINDOW_SIZE) {
        msg("[TEST] Sequence numbers are out-of-order!\n");
        return false;
      } else {
        msg("[TEST] Sequence number is correct\n");
      }

    } else {
      msg("[TEST] pdcp_data_req() returned FALSE!\n");
      return false;
    }
  }

  return true;
}

bool test_pdcp_data_ind(void)
{
  /*
   * This is the list that pdcp_data_ind() takes to put pdcp_data_ind_header_t
   * packets after it receives/validates PDUs and preprends them with pdcp_data_ind_header_t
   * structure. Here, after this list is filled by pdcp_data_ind() we parse/validate
   * every single element in the list
   */
  list_t test_pdu_indication_list;
  uint8_t* test_sdu = NULL;
  /*
   * pdcp_data_req() method prepended PDU header in front of DUMMY_BUFFER so
   * the size should be 12 bytes
   */
  unsigned char test_sdu_size = PDCP_USER_PLANE_DATA_PDU_LONG_SN_HEADER_SIZE + DUMMY_BUFFER_SIZE;
  unsigned int index = 0;

  /*
   * Initialize linked list
   */
  list_init(&test_pdu_indication_list, NULL);

  /*
   * Ask PDCP to handle a number of data indications
   */
  msg("There are %d PDUs in the list\n", test_pdu_rx_list.nb_elements);

  /*
   * Traverse PDU list and pass each one of them to pdcp_data_ind()
   */
  while (list_get_head(&test_pdu_rx_list) != NULL) {
    msg("\n\nAsking PDCP to receive %d. SDU...\n", 1 + index++);

    test_sdu = list_remove_head(&test_pdu_rx_list);

    if (pdcp_data_ind(0, 0, test_sdu_size, test_sdu, &pdcp_array[0], &test_pdu_indication_list) == false) {
      msg("[TEST] pdcp_data_ind() failed to handle data indication!\n");
      return false;
    } else {
      msg("[TEST] pdcp_data_ind() succcessfuly handled data indication\n");
    }

    /*
     * Parse/validate fields of SDU
     */
    msg("[TEST] Starting to dissect SDU created by PDCP...\n");
    /*
     * Get pdcp_data_ind_header_t added by pdcp_data_ind()
     */
    uint8_t* test_data_ind_header = list_remove_head(&test_pdu_indication_list);

    if (test_data_ind_header == NULL) {
      msg("[TEST] Data indication header is not valid!\n");
      return false;
    } else {
      pdcp_data_ind_header_t* indication_header = (pdcp_data_ind_header_t*)test_data_ind_header->data;

      /*
       * Verify that radio bearer ID is correct (0)
       */
      if (indication_header->rb_id == 0) {
        msg("[TEST] Radio bearer ID is correct\n");
      } else {
        msg("[TEST] Radio bearer ID is not correct! (expected: 0, parsed: %d)\n", indication_header->rb_id);
        return false;
      }

      /*
       * Verify that SDU size is correct (DUMMY_BUFFER_SIZE)
       */
      if (indication_header->data_size == DUMMY_BUFFER_SIZE + PDCP_USER_PLANE_DATA_PDU_LONG_SN_HEADER_SIZE) {
        msg("[TEST] SDU size is correct\n");
      } else {
        msg("[TEST] SDU size is not correct! (expected: %d, parsed: %d)\n", DUMMY_BUFFER_SIZE, indication_header->data_size);
        return false;
      }

      /*
       * XXX Verify `indication_header->inst` when you know what it is
       */
    }

    /*
     * XXX PDCP PDU header should also be checked here
     */

    /*
     * Verify that serialised data is the stream we've supplied
     *
     * Data comes after `pdcp_data_ind_header_t` and `pdcp_user_plane_data_pdu_header_with_long_sn`
     */
    unsigned char data_index = sizeof(pdcp_data_ind_header_t) + PDCP_USER_PLANE_DATA_PDU_LONG_SN_HEADER_SIZE;

    if (memcmp(DUMMY_BUFFER, (unsigned char*)&(test_data_ind_header->data[data_index]), DUMMY_BUFFER_SIZE) == 0) {
      msg("[TEST] Data payload of pdcp_data_ind_header_t matches with the stream we sent\n");

    } else {
      msg("[TEST] Data payload of pdcp_data_ind_header_t does not match with the stream we sent!\n");

      /*
       * Print octets of both streams
       * XXX This could be a method in test_util.h
       */
      print_byte_stream("[TEST] TXed data: ", DUMMY_BUFFER, DUMMY_BUFFER_SIZE);
      print_byte_stream("[TEST] RXed data: ", (unsigned char*)&(test_data_ind_header->data[data_index]), DUMMY_BUFFER_SIZE);

      return false;
    }
  }

  return true;
}


