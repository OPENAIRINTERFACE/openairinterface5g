add_library(e2_ran_func_cuup STATIC
            init_ran_func.c
            read_setup_ran.c
            ../flexric/test/rnd/fill_rnd_data_e2_setup_req.c  # this is not rnd data; it is used to fill E2 Node Component Configuration Addition List in E2 Setup Request message
            CUSTOMIZED/ran_func_gtp.c   # current implementation doesn't take split architecture into account, neither CU/DU nor CU-UP/CU-CP
            CUSTOMIZED/ran_func_pdcp.c  # current implementation doesn't take split architecture into account, neither CU/DU nor CU-UP/CU-CP
            CUSTOMIZED/ran_func_tc.c     # currently, not implemented; therefore, filling rnd data
            ../flexric/test/rnd/fill_rnd_data_tc.c
            O-RAN/ran_func_kpm.c    # this file should only contain PDCP-U/GTP; to be done in the future
            ../flexric/test/rnd/fill_rnd_data_kpm.c  # this dependancy will be taken out once RAN Function Definition is implemented
            O-RAN/ran_func_rc.c     # this file should only contain PDCP-U/GTP; to be done in the future
            O-RAN/ran_func_rc_subs.c
            ../flexric/test/rnd/fill_rnd_data_rc.c   # this dependancy will be taken out once RAN Function Definition is implemented
            )

target_link_libraries(e2_ran_func_cuup PUBLIC asn1_nr_rrc nr_rrc asn1_nr_rrc_hdrs e2_time_obj kpm_ric_info_common_obj 3gpp_derived_ie_obj)
target_compile_definitions(e2_ran_func_cuup PUBLIC ${E2AP_VERSION}  ${KPM_VERSION}  NGRAN_GNB_CUUP)


add_library(e2_ran_func_du_cucp_cuup STATIC
            init_ran_func.c
            read_setup_ran.c
            ../flexric/test/rnd/fill_rnd_data_e2_setup_req.c  # this is not rnd data; it is used to fill E2 Node Component Configuration Addition List in E2 Setup Request message
            CUSTOMIZED/ran_func_gtp.c   # current implementation doesn't take split architecture into account, neither CU/DU nor CU-UP/CU-CP
            CUSTOMIZED/ran_func_pdcp.c  # current implementation doesn't take split architecture into account, neither CU/DU nor CU-UP/CU-CP
            O-RAN/ran_func_kpm.c    # this file should only contain RRC/PDCP-C; to be done in the future
                                    # when nr-softmodem is divided in separate executables
            ../flexric/test/rnd/fill_rnd_data_kpm.c  # this dependancy will be taken out once RAN Function Definition is implemented
            O-RAN/ran_func_rc.c     # this file should only contain RRC/PDCP-C; to be done in the future
                                    # when nr-softmodem is divided in separate executables
            O-RAN/ran_func_rc_subs.c
            ../flexric/test/rnd/fill_rnd_data_rc.c   # this dependancy will be taken out once RAN Function Definition is implemented
            ../flexric/src/sm/rc_sm/ie/rc_data_ie.c
            CUSTOMIZED/ran_func_mac.c
            CUSTOMIZED/ran_func_rlc.c
            CUSTOMIZED/ran_func_slice.c
            CUSTOMIZED/ran_func_tc.c
            ../flexric/test/rnd/fill_rnd_data_tc.c
            ../flexric/test/rnd/fill_rnd_data_slice.c
            )

target_link_libraries(e2_ran_func_du_cucp_cuup PUBLIC asn1_nr_rrc nr_rrc asn1_nr_rrc_hdrs e2_time_obj kpm_ric_info_common_obj 3gpp_derived_ie_obj e2sm_rc_ir_obj sm_common_ie_obj)
target_compile_definitions(e2_ran_func_du_cucp_cuup PUBLIC ${E2AP_VERSION}  ${KPM_VERSION} NGRAN_GNB_DU NGRAN_GNB_CUCP NGRAN_GNB_CUUP)
