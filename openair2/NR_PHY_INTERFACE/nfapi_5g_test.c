

//openair2/NR_PHY_INTERFACE/nfapi_5g_test.c
#include "nfapi_nr_interface_scf.h"

#include <stdio.h>  // for printf

int main ( int argc, char** argv)
{
    nfapi_nr_cell_param_t nfapi_nr_cell_param_test;
    nfapi_nr_param_tlv_t tlvs_to_report_list;

    nfapi_nr_cell_param_test.config_tlvs_to_report_list = &tlvs_to_report_list;
    nfapi_nr_cell_param_test.phy_state = 0;
    nfapi_nr_cell_param_test.release_capability = 0;
    nfapi_nr_cell_param_test.skip_blank_dl_config = 0;
    nfapi_nr_cell_param_test.skip_blank_ul_config = 0;

    printf(" test nfapi \n");
    return 0;
}
