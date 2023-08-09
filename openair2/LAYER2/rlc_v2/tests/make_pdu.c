/* gcc -Wall make_pdu.c -I.. ../rlc_pdu.c */

#include "rlc_pdu.h"
#include <stdio.h>

int main(void)
{
  char out[100];
  rlc_pdu_encoder_t e;
  int i;

  rlc_pdu_encoder_init(&e, out, 100);

  rlc_pdu_encoder_put_bits(&e, 0, 1);    // D/C
  rlc_pdu_encoder_put_bits(&e, 0, 3);    // CPT
  rlc_pdu_encoder_put_bits(&e, 0, 10);   // ack_sn
  rlc_pdu_encoder_put_bits(&e, 1, 1);    // e1
  rlc_pdu_encoder_put_bits(&e, 10, 10);  // nack_sn
  rlc_pdu_encoder_put_bits(&e, 0, 1);    // e1
  rlc_pdu_encoder_put_bits(&e, 0, 1);    // e2

  rlc_pdu_encoder_align(&e);

  for (i = 0; i < e.byte; i++) printf(" %2.2x", (unsigned char)e.buffer[i]);

  printf("\n");

  return 0;
}
