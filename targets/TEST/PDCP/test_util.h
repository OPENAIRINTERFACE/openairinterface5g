
/*
 * Helper methods for PDCP test code
 *
 * Author: Baris Demiray <baris.demiray@eurecom.fr>
 */

#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include "LAYER2/PDCP_v10.1.0/pdcp.h"

/*
 * Prints binary representation of given octet prepending
 * passed log message
 *
 * @param Octet as an unsigned character
 * @return None
 */
void print_binary_representation(unsigned char* message, unsigned char byte)
{
  unsigned char index = 0;
  unsigned char mask = 0x80;

  if (message != NULL)
    printf("%s", message);

  for (index = 0; index < 8; ++index) {
    if (byte & mask) printf("1");
    else printf("0");

    mask /= 2;
  }

  printf("\n");
}

/*
 * Prints octets of a byte array in hexadecimal representation
 */
void print_byte_stream(char* message, unsigned char* buffer, unsigned long size)
{
  if (buffer == NULL)
    return;

  unsigned long index = 0lu;

  printf("%s", message);

  for (index = 0; index < size; ++index) {
    msg("%x ", buffer[index]);
  }

  msg("\n");
}

/*
 * Prints PDCP properties
 *
 * @param pdcp_t pointer for relevant PDCP entity
 * @return none
 */
void print_pdcp_properties(pdcp_t* pdcp_entity)
{
  printf("PDCP properties:\n");

  if (pdcp_entity == NULL) {
    printf("PDCP pointer is invalid!\n");
  } else {
    printf("-> Is header compression active? %s\n", ((pdcp_entity->header_compression_active == TRUE) ? "Yes" : "No"));
    printf("-> Sequence number size: %d bits\n", pdcp_entity->seq_num_size);
    printf("-> Next TX sequence number: %d\n", pdcp_entity->next_pdcp_tx_sn);
    printf("-> Next RX sequence number: %d\n", pdcp_entity->next_pdcp_rx_sn);
    printf("-> TX HFN: %d\n", pdcp_entity->tx_hfn);
    printf("-> RX HFN: %d\n", pdcp_entity->rx_hfn);
    printf("-> Last submitted RX sequence number: %d\n", pdcp_entity->last_submitted_pdcp_rx_sn);
  }
}

#endif
