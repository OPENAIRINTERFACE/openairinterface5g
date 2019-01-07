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

#ifndef TEST_PDCP_H
#define TEST_PDCP_H

#include "LAYER2/PDCP_v10.1.0/pdcp.h"
#include "LAYER2/PDCP_v10.1.0/pdcp_primitives.h"

/*
 * To suppress following errors
 *
 * /homes/demiray/workspace/openair4G/trunk/openair2/LAYER2/PDCP_v10.1.0/pdcp.o: In function `pdcp_run':
 * /homes/demiray/workspace/openair4G/trunk/openair2/LAYER2/PDCP_v10.1.0/pdcp.c:270: undefined reference to `pdcp_fifo_read_input_sdus'
 * /homes/demiray/workspace/openair4G/trunk/openair2/LAYER2/PDCP_v10.1.0/pdcp.c:273: undefined reference to `pdcp_fifo_flush_sdus'
 */
int pdcp_fifo_flush_sdus (void) { return 0; }
int pdcp_fifo_read_input_sdus_remaining_bytes (void) { return 0; }
int pdcp_fifo_read_input_sdus (void) { return 0; }

BOOL init_pdcp_entity(pdcp_t *pdcp_entity);
BOOL test_tx_window(void);
BOOL test_rx_window(void);
BOOL test_pdcp_data_req(void);
BOOL test_pdcp_data_ind(void);

/*
 * PDCP methods that are going to be utilised throughout the test
 */
extern BOOL pdcp_init_seq_numbers(pdcp_t* pdcp_entity);
extern u16 pdcp_get_next_tx_seq_number(pdcp_t* pdcp_entity);
extern BOOL pdcp_is_rx_seq_number_valid(u16 seq_num, pdcp_t* pdcp_entity);

#endif
