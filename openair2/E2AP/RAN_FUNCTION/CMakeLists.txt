add_library(e2_ran_func_cuup STATIC
            init_ran_func.c
            read_setup_ran.c
            O-RAN/ran_e2sm_ue_id.c
            O-RAN/ran_func_kpm.c
            O-RAN/ran_func_kpm_subs.c
            O-RAN/ran_func_rc.c
            O-RAN/ran_func_rc_subs.c
            CUSTOMIZED/ran_func_gtp.c    # GTP SM not yet implemented in CU-UP
            CUSTOMIZED/ran_func_pdcp.c
            CUSTOMIZED/ran_func_tc.c     # currently, not implemented; therefore, filling rnd data
            ../flexric/test/rnd/fill_rnd_data_tc.c
            )

target_link_libraries(e2_ran_func_cuup PUBLIC asn1_nr_rrc nr_rrc asn1_nr_rrc_hdrs e2_time_obj kpm_ric_info_common_obj 3gpp_derived_ie_obj sm_common_ie_obj)
target_compile_definitions(e2_ran_func_cuup PUBLIC ${E2AP_VERSION}  ${KPM_VERSION}  NGRAN_GNB_CUUP)


add_library(e2_ran_func_du_cucp_cuup STATIC
            init_ran_func.c
            read_setup_ran.c
            O-RAN/ran_e2sm_ue_id.c
            O-RAN/ran_func_kpm.c
            O-RAN/ran_func_kpm_subs.c
            O-RAN/ran_func_rc.c
            O-RAN/ran_func_rc_subs.c
            ../flexric/src/sm/rc_sm/ie/rc_data_ie.c
            CUSTOMIZED/ran_func_gtp.c
            CUSTOMIZED/ran_func_pdcp.c
            CUSTOMIZED/ran_func_mac.c
            CUSTOMIZED/ran_func_rlc.c
            CUSTOMIZED/ran_func_slice.c  # currently, not implemented; therefore, filling rnd data
            ../flexric/test/rnd/fill_rnd_data_slice.c
            CUSTOMIZED/ran_func_tc.c     # currently, not implemented; therefore, filling rnd data
            ../flexric/test/rnd/fill_rnd_data_tc.c
            )

target_link_libraries(e2_ran_func_du_cucp_cuup PUBLIC asn1_nr_rrc nr_rrc asn1_nr_rrc_hdrs e2_time_obj kpm_ric_info_common_obj 3gpp_derived_ie_obj e2sm_rc_ir_obj sm_common_ie_obj)
target_compile_definitions(e2_ran_func_du_cucp_cuup PUBLIC ${E2AP_VERSION}  ${KPM_VERSION} NGRAN_GNB_DU NGRAN_GNB_CUCP NGRAN_GNB_CUUP)

# Current implementation:
# - we only have one executable nr-softmodem for 4 node types: DU, CU, gNB-mono, CU-CP

# In the future, when nr-softmodem is separated, linking should be done as following:
# - DU: e2_ran_func_du
# - CU-CP : e2_ran_func_cucp
# - CU-UP: e2_ran_func_cuup (this is available at the moment, executable nr-cuup)
# - CU: e2_ran_func_cucp + e2_ran_func_cuup
# - gNB-mono: e2_ran_func_du + e2_ran_func_cucp + e2_ran_func_cuup
