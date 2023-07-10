#include "ran_func_mac.h"
#include "../../flexric/test/rnd/fill_rnd_data_mac.h"
#include <assert.h>

void read_mac_sm(void* data)
{
  assert(data != NULL);

  mac_ind_data_t* mac = (mac_ind_data_t*)data;
  fill_mac_ind_data(mac);
}

void read_mac_setup_sm(void* data)
{
  assert(data != NULL);
  assert(0 !=0 && "Not supported");
}

sm_ag_if_ans_t write_ctrl_mac_sm(void const* data)
{
  assert(data != NULL);
  assert(0 !=0 && "Not supported");
}

