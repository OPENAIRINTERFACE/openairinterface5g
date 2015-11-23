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
