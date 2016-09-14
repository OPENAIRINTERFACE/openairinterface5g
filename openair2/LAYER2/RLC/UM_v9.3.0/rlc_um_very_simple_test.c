#define RLC_UM_MODULE
#define RLC_UM_VERY_SIMPLE_TEST_C
#include <string.h>
//#include "rtos_header.h"
#include "platform_types.h"

#include "list.h"
#include "rlc_um.h"
#include "rlc.h"
#include "rlc_primitives.h"
#include "mac_primitives.h"
#include "LAYER2/MAC/extern.h"
#include "rlc_um_very_simple_test.h"



//-----------------------------------------------------------------------------
void
rlc_um_test_send_sdu     (rlc_um_entity_t* rlcP,  uint32_t frame, unsigned int sdu_typeP)
{
  //-----------------------------------------------------------------------------
  mem_block_t *sdu_mem;

  switch (sdu_typeP) {
  case RLC_UM_TEST_SDU_TYPE_TCPIP:
    sdu_mem = get_free_mem_block (strlen(tcip_sdu)+ 1 + sizeof (struct rlc_um_data_req_alloc));

    if (sdu_mem != NULL) {
      memset (sdu_mem->data, 0, sizeof (struct rlc_um_data_req_alloc));
      ((struct rlc_um_data_req*)(sdu_mem->data))->data_size      = strlen(tcip_sdu)+ 1;
      ((struct rlc_um_data_req*)(sdu_mem->data))->data_offset    = 0;
      ((struct rlc_um_data_req*)(sdu_mem->data))->use_special_li = sizeof (struct rlc_um_data_req_alloc) - sizeof (struct rlc_um_data_req);
      strcpy (&sdu_mem->data[sizeof (struct rlc_um_data_req_alloc)], tcip_sdu);
      sdu_mem->data[sizeof (struct rlc_um_data_req_alloc)+ strlen(tcip_sdu)+1] = 0;
      rlc_um_data_req     (rlcP,  frame, sdu_mem);
    }

    break;

  case RLC_UM_TEST_SDU_TYPE_VOIP:
    sdu_mem = get_free_mem_block (strlen(voip_sdu)+ 1 + sizeof (struct rlc_um_data_req_alloc));

    if (sdu_mem != NULL) {
      memset (sdu_mem->data, 0, sizeof (struct rlc_um_data_req_alloc));
      ((struct rlc_um_data_req*)(sdu_mem->data))->data_size      = strlen(voip_sdu)+ 1;
      ((struct rlc_um_data_req*)(sdu_mem->data))->data_offset    = sizeof (struct rlc_um_data_req_alloc) - sizeof (struct rlc_um_data_req);
      ((struct rlc_um_data_req*)(sdu_mem->data))->use_special_li = 0;
      strcpy (&sdu_mem->data[sizeof (struct rlc_um_data_req_alloc)], voip_sdu);
      sdu_mem->data[sizeof (struct rlc_um_data_req_alloc)+ strlen(voip_sdu)+1] = 0;
      rlc_um_data_req     (rlcP,  frame, sdu_mem);
    }

    break;

  case RLC_UM_TEST_SDU_TYPE_SMALL:
    sdu_mem = get_free_mem_block (strlen(very_small_sdu)+ 1 + sizeof (struct rlc_um_data_req_alloc));

    if (sdu_mem != NULL) {
      memset (sdu_mem->data, 0, sizeof (struct rlc_um_data_req_alloc));
      ((struct rlc_um_data_req*)(sdu_mem->data))->data_size      = strlen(very_small_sdu)+ 1;
      ((struct rlc_um_data_req*)(sdu_mem->data))->data_offset    = sizeof (struct rlc_um_data_req_alloc) - sizeof (struct rlc_um_data_req);
      ((struct rlc_um_data_req*)(sdu_mem->data))->use_special_li = 0;
      strcpy (&sdu_mem->data[sizeof (struct rlc_um_data_req_alloc)], very_small_sdu);
      sdu_mem->data[sizeof (struct rlc_um_data_req_alloc)+ strlen(very_small_sdu)+1] = 0;
      rlc_um_data_req     (rlcP, frame, sdu_mem);
    }

    break;

  default:
    ;
  }
}
