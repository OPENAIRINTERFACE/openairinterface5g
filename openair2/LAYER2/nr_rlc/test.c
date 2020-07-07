#include "nr_rlc_entity.h"

#include <stdio.h>

int main(void)
{
  char out[32768];
  nr_rlc_entity_t *am;
  int sdu_id = 0;
  int size;
  int i;

  am = new_nr_rlc_entity_am(100000, 100000,
                            0, 0,
                            0, 0,
                            0, 0,
                            45, 35, 0,
                            -1, -1, 8,
                            12);

  char data[8] = { 1, 2, 3, 4, 8, 7, 6, 5 };

  am->recv_sdu(am, data, sizeof(data), sdu_id++);

  size = am->generate_pdu(am, out, 32768);

  printf("generate_pdu[%d]:", size);
  for (i = 0; i < size; i++) printf(" %2.2x", (unsigned char)out[i]);
  printf("\n");

  return 0;
}
