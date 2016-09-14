#include "oaisim.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "UTIL/FIFO/pad_list.h"

#ifndef OAISIM_FUNCTIONS_H_
#define OAISIM_FUNCTIONS_H_

void get_simulation_options(int argc, char *argv[]);

void check_and_adjust_params(void);

void init_omv(void);

void init_seed(uint8_t set_seed);

void init_openair1(void);

void init_openair2(void);

void init_ocm(void);

void init_otg_pdcp_buffer(void);

void update_omg(frame_t frameP);

void update_omg_ocm(void);

void update_ocm(void);

void update_otg_eNB(module_id_t module_idP, unsigned int ctime);

void update_otg_UE(module_id_t module_idP, unsigned int ctime);

void exit_fun(const char* s);

void init_time(void);

void init_pad(void);

void help(void);

int init_slot_isr(void);

void wait_for_slot_isr(void);

#endif /* OAISIM_FUNCTIONS_H_ */
