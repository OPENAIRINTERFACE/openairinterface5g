add_library(e2_ran_func_oran STATIC 
          ran_func_kpm.c
          ran_func_rc.c
          # For testing purposes 
  ../../flexric/test/rnd/fill_rnd_data_kpm.c             
  ../../flexric/test/rnd/fill_rnd_data_rc.c              
          )

target_link_libraries(e2_ran_func_oran PUBLIC asn1_nr_rrc nr_rrc asn1_nr_rrc_hdrs)
target_compile_definitions(e2_ran_func_oran PRIVATE ${E2AP_VERSION} ${KPM_VERSION})
