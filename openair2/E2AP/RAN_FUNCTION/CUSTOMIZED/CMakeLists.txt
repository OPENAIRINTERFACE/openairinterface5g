add_library(e2_ran_func_cust STATIC 
          ran_func_gtp.c
          ran_func_mac.c
          ran_func_pdcp.c
          ran_func_rlc.c
          ran_func_slice.c
          ran_func_tc.c
          # For testing purposes 
  ../../flexric/test/rnd/fill_rnd_data_gtp.c             
  ../../flexric/test/rnd/fill_rnd_data_tc.c              
  ../../flexric/test/rnd/fill_rnd_data_mac.c             
  ../../flexric/test/rnd/fill_rnd_data_rlc.c             
  ../../flexric/test/rnd/fill_rnd_data_pdcp.c  
  ../../flexric/test/rnd/fill_rnd_data_slice.c           
  ../../flexric/src/util/time_now_us.c
          )

