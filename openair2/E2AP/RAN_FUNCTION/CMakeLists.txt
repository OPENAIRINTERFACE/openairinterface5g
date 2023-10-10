add_subdirectory(CUSTOMIZED)
add_subdirectory(O-RAN)

add_library(e2_ran_func STATIC 
          init_ran_func.c
          read_setup_ran.c
          ../flexric/test/rnd/fill_rnd_data_e2_setup_req.c 
          )

target_link_libraries(e2_ran_func   
                       PUBLIC  
                       e2_ran_func_cust 
                       e2_ran_func_oran 
                       ) 

# This dependency sucks! Create pointers and forward declarations!
target_compile_definitions(e2_ran_func PRIVATE ${E2AP_VERSION} ${KPM_VERSION})
