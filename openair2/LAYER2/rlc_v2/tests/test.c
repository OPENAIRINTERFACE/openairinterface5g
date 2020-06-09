#include "../rlc_entity.h"
#include "../rlc_entity_am.h"
#include "../rlc_entity_um.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * ENB_AM <rx_maxsize> <tx_maxsize> <t_reordering> <t_status_prohibit>
 *       <t_poll_retransmit> <poll_pdu> <poll_byte> <max_retx_threshold>
 *       create the eNB RLC AM entity with given parameters
 *
 * UE_AM <rx_maxsize> <tx_maxsize> <t_reordering> <t_status_prohibit>
 *      <t_poll_retransmit> <poll_pdu> <poll_byte> <max_retx_threshold>
 *       create the UE RLC AM entity with given parameters
 *
 * ENB_UM <rx_maxsize> <tx_maxsize> <t_reordering> <sn_field_length>
 *     create the eNB RLC UM entity with given parameters
 *
 * UE_UM <rx_maxsize> <tx_maxsize> <t_reordering> <sn_field_length>
 *     create the UE RLC UM entity with given parameters
 *
 * TIME <time>
 *     following actions to be performed at time <time>
 *     <time> starts at 1
 *     You must end your test definition with a line 'TIME, -1'.
 *
 * ENB_SDU <id> <size>
 *     send an SDU to eNB with id <i> and size <size>
 *     the SDU is [00 01 ... ff 01 ...]
 *     (ie. start byte is 00 then we increment for each byte, loop if needed)
 *
 * UE_SDU <id> <size>
 *     same as ENB_SDU but the SDU is sent to the UE
 *
 * ENB_PDU <size> <'size' bytes>
 *     send a custom PDU from eNB to UE (eNB does not see this PDU at all)
 *
 * UE_PDU <size> <'size' bytes>
 *     send a custom PDU from UE to eNB (UE does not see this PDU at all)
 *
 * ENB_PDU_SIZE <size>
 *     set 'enb_pdu_size'
 *
 * UE_PDU_SIZE <size>
 *     set 'ue_pdu_size'
 *
 * ENB_RECV_FAILS <fails>
 *     set the 'enb_recv_fails' flag to <fails>
 *     (1: recv will fail, 0: recv will succeed)
 *
 * UE_RECV_FAILS <fails>
 *     same as ENB_RECV_FAILS but for 'ue_recv_fails'
 *
 * MUST_FAIL
 *     to be used as first command after the first TIME to indicate
 *     that the test must fail (ie. exit with non zero, crash not allowed)
 *
 * ENB_BUFFER_STATUS
 *     call buffer_status for eNB and print result
 *
 * UE_BUFFER_STATUS
 *     call buffer_status for UE and print result
 *
 * ENB_DISCARD_SDU <sdu ID>
 *     discards given SDU
 *
 * UE_DISCARD_SDU <sdu ID>
 *     discards given SDU
 *
 * RE_ESTABLISH
 *     re-establish both eNB and UE
 */

enum action {
  ENB_AM, UE_AM,
  ENB_UM, UE_UM,
  TIME, ENB_SDU, UE_SDU, ENB_PDU, UE_PDU,
  ENB_PDU_SIZE, UE_PDU_SIZE,
  ENB_RECV_FAILS, UE_RECV_FAILS,
  MUST_FAIL,
  ENB_BUFFER_STATUS, UE_BUFFER_STATUS,
  ENB_DISCARD_SDU, UE_DISCARD_SDU,
  RE_ESTABLISH
};

int test[] = {
/* TEST is defined at compilation time */
#include TEST
};

void deliver_sdu_enb_am(void *deliver_sdu_data, struct rlc_entity_t *_entity,
                        char *buf, int size)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  printf("TEST: ENB: %"PRIu64": deliver SDU size %d [",
         entity->t_current, size);
  for (int i = 0; i < size; i++) printf(" %2.2x", (unsigned char)buf[i]);
  printf("]\n");
}

void deliver_sdu_enb_um(void *deliver_sdu_data, struct rlc_entity_t *_entity,
                        char *buf, int size)
{
  rlc_entity_um_t *entity = (rlc_entity_um_t *)_entity;
  printf("TEST: ENB: %"PRIu64": deliver SDU size %d [",
         entity->t_current, size);
  for (int i = 0; i < size; i++) printf(" %2.2x", (unsigned char)buf[i]);
  printf("]\n");
}

void successful_delivery_enb(void *successful_delivery_data,
                             rlc_entity_t *_entity, int sdu_id)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  printf("TEST: ENB: %"PRIu64": SDU %d was successfully delivered.\n",
         entity->t_current, sdu_id);
}

void max_retx_reached_enb(void *max_retx_reached_data, rlc_entity_t *_entity)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  printf("TEST: ENB: %"PRIu64": max RETX reached! radio link failure!\n",
         entity->t_current);
  exit(1);
}

void deliver_sdu_ue_am(void *deliver_sdu_data, struct rlc_entity_t *_entity,
                       char *buf, int size)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  printf("TEST: UE: %"PRIu64": deliver SDU size %d [",
         entity->t_current, size);
  for (int i = 0; i < size; i++) printf(" %2.2x", (unsigned char)buf[i]);
  printf("]\n");
}

void deliver_sdu_ue_um(void *deliver_sdu_data, struct rlc_entity_t *_entity,
                       char *buf, int size)
{
  rlc_entity_um_t *entity = (rlc_entity_um_t *)_entity;
  printf("TEST: UE: %"PRIu64": deliver SDU size %d [",
         entity->t_current, size);
  for (int i = 0; i < size; i++) printf(" %2.2x", (unsigned char)buf[i]);
  printf("]\n");
}

void successful_delivery_ue(void *successful_delivery_data,
                            rlc_entity_t *_entity, int sdu_id)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  printf("TEST: UE: %"PRIu64": SDU %d was successfully delivered.\n",
         entity->t_current, sdu_id);
}

void max_retx_reached_ue(void *max_retx_reached_data, rlc_entity_t *_entity)
{
  rlc_entity_am_t *entity = (rlc_entity_am_t *)_entity;
  printf("TEST: UE: %"PRIu64", max RETX reached! radio link failure!\n",
         entity->t_current);
  exit(1);
}

int test_main(void)
{
  rlc_entity_t *enb = NULL;
  rlc_entity_t *ue = NULL;
  int i;
  int k;
  char *sdu;
  char *pdu;
  rlc_entity_buffer_status_t buffer_status;
  int enb_do_buffer_status = 0;
  int ue_do_buffer_status = 0;
  int size;
  int pos;
  int next_byte_enb = 0;
  int next_byte_ue = 0;
  int enb_recv_fails = 0;
  int ue_recv_fails = 0;
  int enb_pdu_size = 1000;
  int ue_pdu_size = 1000;

  sdu = malloc(16001);
  pdu = malloc(3000);
  if (sdu == NULL || pdu == NULL) {
    printf("out of memory\n");
    exit(1);
  }

  for (i = 0; i < 16001; i++)
    sdu[i] = i & 255;

  pos = 0;
  if (test[pos] != TIME) {
    printf("%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }

  for (i = 1; i < 1000; i++) {
    if (i == test[pos+1]) {
      pos += 2;
      while (test[pos] != TIME)
        switch (test[pos]) {
        default: printf("fatal: unknown action\n"); exit(1);
        case ENB_AM:
          enb = new_rlc_entity_am(test[pos+1], test[pos+2],
                                  deliver_sdu_enb_am, NULL,
                                  successful_delivery_enb, NULL,
                                  max_retx_reached_enb, NULL,
                                  test[pos+3], test[pos+4], test[pos+5],
                                  test[pos+6], test[pos+7], test[pos+8]);
          pos += 9;
          break;
        case UE_AM:
          ue = new_rlc_entity_am(test[pos+1], test[pos+2],
                                 deliver_sdu_ue_am, NULL,
                                 successful_delivery_ue, NULL,
                                 max_retx_reached_ue, NULL,
                                 test[pos+3], test[pos+4], test[pos+5],
                                 test[pos+6], test[pos+7], test[pos+8]);
          pos += 9;
          break;
        case ENB_UM:
          enb = new_rlc_entity_um(test[pos+1], test[pos+2],
                                  deliver_sdu_enb_um, NULL,
                                  test[pos+3], test[pos+4]);
          pos += 5;
          break;
        case UE_UM:
          ue = new_rlc_entity_um(test[pos+1], test[pos+2],
                                 deliver_sdu_ue_um, NULL,
                                 test[pos+3], test[pos+4]);
          pos += 5;
          break;
        case ENB_SDU:
          for (k = 0; k < test[pos+2]; k++, next_byte_enb++)
            sdu[k] = next_byte_enb;
          printf("TEST: ENB: %d: recv_sdu (id %d): size %d: [",
                 i, test[pos+1], test[pos+2]);
          for (k = 0; k < test[pos+2]; k++)
            printf(" %2.2x", (unsigned char)sdu[k]);
          printf("]\n");
          enb->recv_sdu(enb, sdu, test[pos+2], test[pos+1]);
          pos += 3;
          break;
        case UE_SDU:
          for (k = 0; k < test[pos+2]; k++, next_byte_ue++)
            sdu[k] = next_byte_ue;
          printf("TEST: UE: %d: recv_sdu (id %d): size %d: [",
                 i, test[pos+1], test[pos+2]);
          for (k = 0; k < test[pos+2]; k++)
            printf(" %2.2x", (unsigned char)sdu[k]);
          printf("]\n");
          ue->recv_sdu(ue, sdu, test[pos+2], test[pos+1]);
          pos += 3;
          break;
        case ENB_PDU:
          for (k = 0; k < test[pos+1]; k++)
            pdu[k] = test[pos+2+k];
          printf("TEST: ENB: %d: custom PDU: size %d: [", i, test[pos+1]);
          for (k = 0; k < test[pos+1]; k++) printf(" %2.2x", (unsigned char)pdu[k]);
          printf("]\n");
          if (!ue_recv_fails)
            ue->recv_pdu(ue, pdu, test[pos+1]);
          pos += 2 + test[pos+1];
          break;
        case UE_PDU:
          for (k = 0; k < test[pos+1]; k++)
            pdu[k] = test[pos+2+k];
          printf("TEST: UE: %d: custom PDU: size %d: [", i, test[pos+1]);
          for (k = 0; k < test[pos+1]; k++) printf(" %2.2x", (unsigned char)pdu[k]);
          printf("]\n");
          if (!enb_recv_fails)
            enb->recv_pdu(enb, pdu, test[pos+1]);
          pos += 2 + test[pos+1];
          break;
        case ENB_PDU_SIZE:
          enb_pdu_size = test[pos+1];
          pos += 2;
          break;
        case UE_PDU_SIZE:
          ue_pdu_size = test[pos+1];
          pos += 2;
          break;
        case ENB_RECV_FAILS:
          enb_recv_fails = test[pos+1];
          pos += 2;
          break;
        case UE_RECV_FAILS:
          ue_recv_fails = test[pos+1];
          pos += 2;
          break;
        case MUST_FAIL:
          /* do nothing, only used by caller */
          pos++;
          break;
        case ENB_BUFFER_STATUS:
          enb_do_buffer_status = 1;
          pos++;
          break;
        case UE_BUFFER_STATUS:
          ue_do_buffer_status = 1;
          pos++;
          break;
        case ENB_DISCARD_SDU:
          printf("TEST: ENB: %d: discard SDU %d\n", i, test[pos+1]);
          enb->discard_sdu(enb, test[pos+1]);
          pos += 2;
          break;
        case UE_DISCARD_SDU:
          printf("TEST: UE: %d: discard SDU %d\n", i, test[pos+1]);
          ue->discard_sdu(ue, test[pos+1]);
          pos += 2;
          break;
        case RE_ESTABLISH:
          printf("TEST: %d: re-establish eNB and UE\n", i);
          enb->reestablishment(enb);
          ue->reestablishment(ue);
          pos++;
          break;
        }
    }

    enb->set_time(enb, i);
    ue->set_time(ue, i);

    if (enb_do_buffer_status) {
      enb_do_buffer_status = 0;
      buffer_status = enb->buffer_status(enb, enb_pdu_size);
      printf("TEST: ENB: %d: buffer_status: status_size %d tx_size %d retx_size %d\n",
             i,
             buffer_status.status_size,
             buffer_status.tx_size,
             buffer_status.retx_size);
    }

    size = enb->generate_pdu(enb, pdu, enb_pdu_size);
    if (size) {
      printf("TEST: ENB: %d: generate_pdu: size %d: [", i, size);
      for (k = 0; k < size; k++) printf(" %2.2x", (unsigned char)pdu[k]);
      printf("]\n");
      if (!ue_recv_fails)
        ue->recv_pdu(ue, pdu, size);
    }

    if (ue_do_buffer_status) {
      ue_do_buffer_status = 0;
      buffer_status = ue->buffer_status(ue, ue_pdu_size);
      printf("TEST: UE: %d: buffer_status: status_size %d tx_size %d retx_size %d\n",
             i,
             buffer_status.status_size,
             buffer_status.tx_size,
             buffer_status.retx_size);
    }

    size = ue->generate_pdu(ue, pdu, ue_pdu_size);
    if (size) {
      printf("TEST: UE: %d: generate_pdu: size %d: [", i, size);
      for (k = 0; k < size; k++) printf(" %2.2x", (unsigned char)pdu[k]);
      printf("]\n");
      if (!enb_recv_fails)
        enb->recv_pdu(enb, pdu, size);
    }
  }

  enb->delete(enb);
  ue->delete(ue);

  free(sdu);
  free(pdu);

  return 0;
}

void usage(void)
{
  printf("options:\n");
  printf("    -no-fork\n");
  printf("        don't fork (to ease debugging with gdb)\n");
  exit(0);
}

int main(int n, char **v)
{
  int must_fail = 0;
  int son;
  int status;
  int i;
  int no_fork = 0;

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-no-fork")) { no_fork = 1; continue; }
    usage();
  }

  if (test[2] == MUST_FAIL)
    must_fail = 1;

  if (no_fork) return test_main();

  son = fork();
  if (son == -1) {
    perror("fork");
    return 1;
  }

  if (son == 0)
    return test_main();

  if (wait(&status) == -1) {
    perror("wait");
    return 1;
  }

  /* child must quit properly */
  if (!WIFEXITED(status))
    return 1;

  /* child must fail if expected to */
  if (must_fail && WEXITSTATUS(status) == 0)
    return 1;

  /* child must not fail if not expected to */
  if (!must_fail && WEXITSTATUS(status))
    return 1;

  return 0;
}
